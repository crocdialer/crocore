// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

namespace crocore
{

template<typename T>
fixed_size_free_list<T>::~fixed_size_free_list()
{
    // Check if we got our Init call
    if(m_pages)
    {
        // Ensure everything is freed before the freelist is destructed
        assert(m_num_free_objects.load(std::memory_order_relaxed) == mNumPages * mPageSize);

        // Free memory for pages
        uint32_t num_pages = mNumObjectsAllocated / mPageSize;
        for(uint32_t page = 0; page < num_pages; ++page) crocore::aligned_free(m_pages[page]);
    }
}

template<typename T>
fixed_size_free_list<T>::fixed_size_free_list(uint32_t inMaxObjects, uint32_t inPageSize)
{
    // Check sanity
    assert(inPageSize > 0 && crocore::is_pow_2(inPageSize));
    assert(m_pages == nullptr);

    // Store configuration parameters
    mNumPages = (inMaxObjects + inPageSize - 1) / inPageSize;
    mPageSize = inPageSize;
    m_page_shift = std::countr_zero(inPageSize);
    mObjectMask = inPageSize - 1;
    //    JPH_IF_ENABLE_ASSERTS(m_num_free_objects = mNumPages * inPageSize;)

    // Allocate page table
    m_pages.reset(new storage*[mNumPages]);

    // We didn't yet use any objects of any page
    mNumObjectsAllocated = 0;
    mFirstFreeObjectInNewPage = 0;

    // Start with 1 as the first tag
    m_allocation_tag = 1;

    // Set first free object (with tag 0)
    mFirstFreeObjectAndTag = cInvalidObjectIndex;
}

template<typename T>
template<typename... Parameters>
uint32_t fixed_size_free_list<T>::ConstructObject(Parameters &&...inParameters)
{
    for(;;)
    {
        // Get first object from the linked list
        uint64_t first_free_object_and_tag = mFirstFreeObjectAndTag.load(std::memory_order_acquire);
        auto first_free = uint32_t(first_free_object_and_tag);
        if(first_free == cInvalidObjectIndex)
        {
            // The free list is empty, we take an object from the page that has never been used before
            first_free = mFirstFreeObjectInNewPage.fetch_add(1, std::memory_order_relaxed);
            if(first_free >= mNumObjectsAllocated)
            {
                // Allocate new page
                std::lock_guard lock(mPageMutex);
                while(first_free >= mNumObjectsAllocated)
                {
                    uint32_t next_page = mNumObjectsAllocated / mPageSize;
                    if(next_page == mNumPages) return cInvalidObjectIndex;// Out of space!
                    m_pages[next_page] = reinterpret_cast<storage *>(crocore::aligned_alloc(
                            mPageSize * sizeof(storage), std::max<size_t>(alignof(storage), k_cache_line_size)));
                    mNumObjectsAllocated += mPageSize;
                }
            }

            // Allocation successful
            //            JPH_IF_ENABLE_ASSERTS(m_num_free_objects.fetch_sub(1, std::memory_order_relaxed);)

            storage &storage = GetStorage(first_free);
            ::new(&storage.object) T(std::forward<Parameters>(inParameters)...);
            storage.next_free_object.store(first_free, std::memory_order_release);
            return first_free;
        }
        else
        {
            // Load next pointer
            uint32_t new_first_free = GetStorage(first_free).next_free_object.load(std::memory_order_acquire);

            // Construct a new first free object tag
            uint64_t new_first_free_object_and_tag =
                    uint64_t(new_first_free) + (uint64_t(m_allocation_tag.fetch_add(1, std::memory_order_relaxed)) << 32);

            // Compare and swap
            if(mFirstFreeObjectAndTag.compare_exchange_weak(first_free_object_and_tag, new_first_free_object_and_tag,
                                                            std::memory_order_release))
            {
                // Allocation successful
                //                JPH_IF_ENABLE_ASSERTS(m_num_free_objects.fetch_sub(1, std::memory_order_relaxed);)
                storage &storage = GetStorage(first_free);
                ::new(&storage.object) T(std::forward<Parameters>(inParameters)...);
                storage.next_free_object.store(first_free, std::memory_order_release);
                return first_free;
            }
        }
    }
}

template<typename T>
void fixed_size_free_list<T>::AddObjectToBatch(Batch &ioBatch, uint32_t inObjectIndex)
{
    // Trying to add a object to the batch that is already in a free list
    assert(GetStorage(inObjectIndex).next_free_object.load(std::memory_order_relaxed) == inObjectIndex);

    // Trying to reuse a batch that has already been freed
    assert(ioBatch.mNumObjects != uint32_t(-1));

    // Link object in batch to free
    if(ioBatch.mFirstObjectIndex == cInvalidObjectIndex) ioBatch.mFirstObjectIndex = inObjectIndex;
    else
        GetStorage(ioBatch.mLastObjectIndex).next_free_object.store(inObjectIndex, std::memory_order_release);
    ioBatch.mLastObjectIndex = inObjectIndex;
    ioBatch.mNumObjects++;
}

template<typename T>
void fixed_size_free_list<T>::DestructObjectBatch(Batch &ioBatch)
{
    if(ioBatch.mFirstObjectIndex != cInvalidObjectIndex)
    {
        // Call destructors
        if constexpr(!std::is_trivially_destructible<T>())
        {
            uint32_t object_idx = ioBatch.mFirstObjectIndex;
            do {
                storage &storage = GetStorage(object_idx);
                storage.object.~Object();
                object_idx = storage.next_free_object.load(std::memory_order_relaxed);
            } while(object_idx != cInvalidObjectIndex);
        }

        // Add to objects free list
        storage &storage = GetStorage(ioBatch.mLastObjectIndex);
        for(;;)
        {
            // Get first object from the list
            uint64_t first_free_object_and_tag = mFirstFreeObjectAndTag.load(std::memory_order_acquire);
            auto first_free = uint32_t(first_free_object_and_tag);

            // Make it the next pointer of the last object in the batch that is to be freed
            storage.next_free_object.store(first_free, std::memory_order_release);

            // Construct a new first free object tag
            uint64_t new_first_free_object_and_tag =
                    uint64(ioBatch.mFirstObjectIndex) +
                    (uint64_t(m_allocation_tag.fetch_add(1, std::memory_order_relaxed)) << 32);

            // Compare and swap
            if(mFirstFreeObjectAndTag.compare_exchange_weak(first_free_object_and_tag, new_first_free_object_and_tag,
                                                            std::memory_order_release))
            {
                // Free successful
                //                JPH_IF_ENABLE_ASSERTS(m_num_free_objects.fetch_add(ioBatch.mNumObjects, std::memory_order_relaxed);)

                // Mark the batch as freed
#ifdef JPH_ENABLE_ASSERTS
                ioBatch.mNumObjects = uint32(-1);
#endif
                return;
            }
        }
    }
}

template<typename T>
void fixed_size_free_list<T>::DestructObject(uint32_t inObjectIndex)
{
    assert(inObjectIndex != cInvalidObjectIndex);

    // Call destructor
    storage &storage = GetStorage(inObjectIndex);
    storage.object.~Object();

    // Add to object free list
    for(;;)
    {
        // Get first object from the list
        uint64_t first_free_object_and_tag = mFirstFreeObjectAndTag.load(std::memory_order_acquire);
        auto first_free = uint32_t(first_free_object_and_tag);

        // Make it the next pointer of the last object in the batch that is to be freed
        storage.next_free_object.store(first_free, std::memory_order_release);

        // Construct a new first free object tag
        uint64_t new_first_free_object_and_tag =
                uint64_t(inObjectIndex) + (uint64_t(m_allocation_tag.fetch_add(1, std::memory_order_relaxed)) << 32);

        // Compare and swap
        if(mFirstFreeObjectAndTag.compare_exchange_weak(first_free_object_and_tag, new_first_free_object_and_tag,
                                                        std::memory_order_release))
        {
            // Free successful
            //            JPH_IF_ENABLE_ASSERTS(m_num_free_objects.fetch_add(1, std::memory_order_relaxed);)
            return;
        }
    }
}

template<typename T>
inline void fixed_size_free_list<T>::DestructObject(T *inObject)
{
    uint32_t index = reinterpret_cast<storage *>(inObject)->next_free_object.load(std::memory_order_relaxed);
    assert(index < mNumObjectsAllocated);
    DestructObject(index);
}

}// namespace crocore
