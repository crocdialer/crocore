// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

namespace crocore
{

template<typename T>
inline void fixed_size_free_list<T>::swap(fixed_size_free_list &lhs, fixed_size_free_list &rhs)
{
    std::lock(lhs.m_page_mutex, rhs.m_page_mutex);
    std::unique_lock<std::mutex> lock_lhs(lhs.m_page_mutex, std::adopt_lock);
    std::unique_lock<std::mutex> lock_rhs(rhs.m_page_mutex, std::adopt_lock);

    lhs.m_num_free_objects = rhs.m_num_free_objects.exchange(lhs.m_num_free_objects);
    lhs.m_allocation_tag = rhs.m_allocation_tag.exchange(lhs.m_allocation_tag);
    lhs.m_first_free_object_and_tag = rhs.m_first_free_object_and_tag.exchange(lhs.m_first_free_object_and_tag);
    lhs.m_first_free_object_in_new_page =
            rhs.m_first_free_object_in_new_page.exchange(lhs.m_first_free_object_in_new_page);

    std::swap(lhs.m_page_size, rhs.m_page_size);
    std::swap(lhs.m_page_shift, rhs.m_page_shift);
    std::swap(lhs.m_object_mask, rhs.m_object_mask);
    std::swap(lhs.m_num_pages, rhs.m_num_pages);
    std::swap(lhs.m_num_objects_allocated, rhs.m_num_objects_allocated);
    std::swap(lhs.m_pages, rhs.m_pages);
}

template<typename T>
fixed_size_free_list<T>::fixed_size_free_list(fixed_size_free_list &&other) noexcept : fixed_size_free_list()
{
    swap(*this, other);
}

template<typename T>
fixed_size_free_list<T> &fixed_size_free_list<T>::operator=(fixed_size_free_list other)
{
    swap(*this, other);
    return *this;
}

template<typename T>
fixed_size_free_list<T>::~fixed_size_free_list()
{
    // Check if we got our Init call
    if(m_pages)
    {
        // Ensure everything is freed before the freelist is destructed
        assert(m_num_free_objects.load(std::memory_order_relaxed) == m_num_pages * m_page_size);

        // Free memory for pages
        uint32_t num_pages = m_num_objects_allocated / m_page_size;
        for(uint32_t page = 0; page < num_pages; ++page) { crocore::aligned_free(m_pages[page]); }
    }
}

template<typename T>
fixed_size_free_list<T>::fixed_size_free_list(uint32_t inMaxObjects, uint32_t inPageSize)
{
    // Check sanity
    assert(inPageSize > 0 && crocore::is_pow_2(inPageSize));
    assert(m_pages == nullptr);

    // Store configuration parameters
    m_num_pages = (inMaxObjects + inPageSize - 1) / inPageSize;
    m_page_size = inPageSize;
    m_page_shift = std::countr_zero(inPageSize);
    m_object_mask = inPageSize - 1;
    m_num_free_objects = m_num_pages * inPageSize;

    // Allocate page table
    m_pages.reset(new storage *[m_num_pages]);

    // We didn't yet use any objects of any page
    m_num_objects_allocated = 0;
    m_first_free_object_in_new_page = 0;

    // Start with 1 as the first tag
    m_allocation_tag = 1;

    // Set first free object (with tag 0)
    m_first_free_object_and_tag = s_invalid_index;
}

template<typename T>
template<typename... Parameters>
uint32_t fixed_size_free_list<T>::ConstructObject(Parameters &&...inParameters)
{
    for(;;)
    {
        // Get first object from the linked list
        uint64_t first_free_object_and_tag = m_first_free_object_and_tag.load(std::memory_order_acquire);
        auto first_free = uint32_t(first_free_object_and_tag);
        if(first_free == s_invalid_index)
        {
            // The free list is empty, we take an object from the page that has never been used before
            first_free = m_first_free_object_in_new_page.fetch_add(1, std::memory_order_relaxed);
            if(first_free >= m_num_objects_allocated)
            {
                // Allocate new page
                std::lock_guard lock(m_page_mutex);
                while(first_free >= m_num_objects_allocated)
                {
                    uint32_t next_page = m_num_objects_allocated / m_page_size;
                    if(next_page == m_num_pages) return s_invalid_index;// Out of space!
                    m_pages[next_page] = reinterpret_cast<storage *>(crocore::aligned_alloc(
                            m_page_size * sizeof(storage), std::max<size_t>(alignof(storage), k_cache_line_size)));
                    m_num_objects_allocated += m_page_size;
                }
            }

            // Allocation successful
            m_num_free_objects.fetch_sub(1, std::memory_order_relaxed);

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
                    uint64_t(new_first_free) +
                    (uint64_t(m_allocation_tag.fetch_add(1, std::memory_order_relaxed)) << 32);

            // Compare and swap
            if(m_first_free_object_and_tag.compare_exchange_weak(
                       first_free_object_and_tag, new_first_free_object_and_tag, std::memory_order_release))
            {
                // Allocation successful
                m_num_free_objects.fetch_sub(1, std::memory_order_relaxed);
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
    if(ioBatch.mFirstObjectIndex == s_invalid_index) ioBatch.mFirstObjectIndex = inObjectIndex;
    else
        GetStorage(ioBatch.mLastObjectIndex).next_free_object.store(inObjectIndex, std::memory_order_release);
    ioBatch.mLastObjectIndex = inObjectIndex;
    ioBatch.mNumObjects++;
}

template<typename T>
void fixed_size_free_list<T>::DestructObjectBatch(Batch &ioBatch)
{
    if(ioBatch.mFirstObjectIndex != s_invalid_index)
    {
        // Call destructors
        if constexpr(!std::is_trivially_destructible<T>())
        {
            uint32_t object_idx = ioBatch.mFirstObjectIndex;
            do {
                storage &storage = GetStorage(object_idx);
                storage.object.~Object();
                object_idx = storage.next_free_object.load(std::memory_order_relaxed);
            } while(object_idx != s_invalid_index);
        }

        // Add to objects free list
        storage &storage = GetStorage(ioBatch.mLastObjectIndex);
        for(;;)
        {
            // Get first object from the list
            uint64_t first_free_object_and_tag = m_first_free_object_and_tag.load(std::memory_order_acquire);
            auto first_free = uint32_t(first_free_object_and_tag);

            // Make it the next pointer of the last object in the batch that is to be freed
            storage.next_free_object.store(first_free, std::memory_order_release);

            // Construct a new first free object tag
            uint64_t new_first_free_object_and_tag =
                    uint64(ioBatch.mFirstObjectIndex) +
                    (uint64_t(m_allocation_tag.fetch_add(1, std::memory_order_relaxed)) << 32);

            // Compare and swap
            if(m_first_free_object_and_tag.compare_exchange_weak(
                       first_free_object_and_tag, new_first_free_object_and_tag, std::memory_order_release))
            {
                // Free successful
                m_num_free_objects.fetch_add(ioBatch.mNumObjects, std::memory_order_relaxed);

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
    assert(inObjectIndex != s_invalid_index);

    // Call destructor
    storage &storage = GetStorage(inObjectIndex);
    storage.object.~T();

    // Add to object free list
    for(;;)
    {
        // Get first object from the list
        uint64_t first_free_object_and_tag = m_first_free_object_and_tag.load(std::memory_order_acquire);
        auto first_free = uint32_t(first_free_object_and_tag);

        // Make it the next pointer of the last object in the batch that is to be freed
        storage.next_free_object.store(first_free, std::memory_order_release);

        // Construct a new first free object tag
        uint64_t new_first_free_object_and_tag =
                uint64_t(inObjectIndex) + (uint64_t(m_allocation_tag.fetch_add(1, std::memory_order_relaxed)) << 32);

        // Compare and swap
        if(m_first_free_object_and_tag.compare_exchange_weak(first_free_object_and_tag, new_first_free_object_and_tag,
                                                             std::memory_order_release))
        {
            // Free successful
            m_num_free_objects.fetch_add(1, std::memory_order_relaxed);
            return;
        }
    }
}

template<typename T>
inline void fixed_size_free_list<T>::DestructObject(T *inObject)
{
    uint32_t index = reinterpret_cast<storage *>(inObject)->next_free_object.load(std::memory_order_relaxed);
    assert(index < m_num_objects_allocated);
    DestructObject(index);
}

}// namespace crocore
