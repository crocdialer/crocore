#pragma once

#include <atomic>
#include <bit>
#include <cassert>
#include <crocore/utils.hpp>
#include <cstdint>
#include <mutex>

namespace crocore
{

constexpr uint32_t k_cache_line_size = 64;

/// Class that allows lock free creation / destruction of objects (unless a new page of objects needs to be allocated)
/// It contains a fixed pool of objects and also allows batching up a lot of objects to be destroyed
/// and doing the actual free in a single atomic operation
template<typename T>
class fixed_size_free_list
{
public:
    //! invalid index
    static constexpr uint32_t s_invalid_index = std::numeric_limits<uint32_t>::max();

    fixed_size_free_list() = default;

    inline fixed_size_free_list(fixed_size_free_list &&other) noexcept;

    fixed_size_free_list(const fixed_size_free_list &) = delete;

    inline fixed_size_free_list(uint32_t max_num_objects, uint32_t page_size);

    inline ~fixed_size_free_list();

    inline fixed_size_free_list &operator=(fixed_size_free_list other);

    //! lockless construct a new object, inParameters are passed on to the constructor
    template<typename... Parameters>
    inline uint32_t create(Parameters &&...inParameters);

    //! lockless destruct an object and return it to the free pool
    inline void destroy(uint32_t inObjectIndex);

    //! lockless destruct an object and return it to the free pool
    inline void destroy(T *inObject);

    /// A batch of objects that can be destructed
    struct batch_t
    {
        uint32_t m_first_object_index = s_invalid_index;
        uint32_t m_last_object_index = s_invalid_index;
        uint32_t m_num_objects = 0;
    };

    /// Add a object to an existing batch to be destructed.
    /// Adding objects to a batch does not destroy or modify the objects, this will merely link them
    /// so that the entire batch can be returned to the free list in a single atomic operation
    inline void add_to_batch(batch_t &batch, uint32_t object_index);

    //! lockless destruct batch of objects
    inline void destroy_batch(batch_t &batch);

    //! access an object by index.
    inline T &get(uint32_t inObjectIndex) { return get_storage(inObjectIndex).object; }

    //! access an object by index.
    inline const T &get(uint32_t inObjectIndex) const { return get_storage(inObjectIndex).object; }

    inline void swap(fixed_size_free_list &lhs, fixed_size_free_list &rhs);

private:
    //! storage type, containing an object
    struct storage_t
    {
        T object;

        /// When the object is freed (or in the process of being freed as a batch) this will contain the next free object
        /// When an object is in use it will contain the object's index in the free list
        std::atomic<uint32_t> next_free_object;
    };

    static_assert(alignof(storage_t) == alignof(T), "Object not properly aligned");

    /// Access the object storage given the object index
    inline const storage_t &get_storage(uint32_t inObjectIndex) const
    {
        return m_pages[inObjectIndex >> m_page_shift][inObjectIndex & m_object_mask];
    }
    inline storage_t &get_storage(uint32_t inObjectIndex)
    {
        return m_pages[inObjectIndex >> m_page_shift][inObjectIndex & m_object_mask];
    }

    //! number of objects currently in the free list / new pages
    CROCORE_IF_DEBUG(std::atomic<uint32_t> m_num_free_objects;)

    //! simple counter that makes the first free object pointer update with every CAS so that we don't suffer from the ABA problem
    std::atomic<uint32_t> m_allocation_tag = 1;

    //! index of first free object, the first 32 bits of an object are used to point to the next free object
    std::atomic<uint64_t> m_first_free_object_and_tag = s_invalid_index;

    //! size (in objects) of a single page
    uint32_t m_page_size = 0;

    //! number of bits to shift an object index to the right to get the page number
    uint32_t m_page_shift = 0;

    //! mask to and an object index with to get the page number
    uint32_t m_object_mask = 0;

    //! total number of pages that are usable
    uint32_t m_num_pages = 0;

    //! total number of objects allocated
    uint32_t m_num_objects_allocated = 0;

    //! first free object to use when the free list is empty (may need to allocate a new page)
    std::atomic<uint32_t> m_first_free_object_in_new_page = 0;

    //! array of memory-pages
    std::unique_ptr<storage_t *[]> m_pages = nullptr;

    //! mutex used to allocate a new page if storage runs out
    std::mutex m_page_mutex;
};

}// namespace crocore

#include <crocore/fixed_size_free_list.inl>
