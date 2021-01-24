//
// Created by crocdialer on 4/12/20.
//

#pragma once

#include <unordered_map>
#include <mutex>
#include <shared_mutex>

namespace crocore
{

/**
 * @brief   Cache_ is a class-template to cache objects based on a key in a threadsafe way.
 */
template<typename Key, typename Value>
class Cache_
{
public:

    Cache_() = default;

    Cache_(Cache_&& other) noexcept: Cache_()
    {
        swap(*this, other);
    }

    Cache_(const Cache_& other) = delete;

    Cache_& operator=(Cache_ other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(Cache_& lhs, Cache_& rhs)
    {
        if (&lhs == &rhs) { return; }
        std::lock(lhs.m_mutex, rhs.m_mutex);
        std::lock_guard<std::shared_mutex> lock_lhs(lhs.m_mutex, std::adopt_lock);
        std::lock_guard<std::shared_mutex> lock_rhs(rhs.m_mutex, std::adopt_lock);

        std::swap(lhs._objects, rhs._objects);
    }

    /**
     * @brief   Put an object into the cache, replacing any previously existing object.
     *
     * @param   key     the key associated with the object.
     * @param   object  the object to put into the cache.
     * @return  a const-ref to the cached object.
     */
    inline const Value& put(const Key& key, Value object)
    {
        std::unique_lock<std::shared_mutex> lock{m_mutex};
        auto new_object_it = _objects.insert(std::make_pair(key, std::move(object))).first;
        return new_object_it->second;
    }

    /**
     * @brief   Get i.e. "retrieve" an already existing object from the cache.
     *          Throws an exception if the provided key could not be found.
     *
     * @param   key the key to look for.
     * @return  a const-ref to the cached object.
     */
    inline const Value& get(const Key& key) const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto searchIt = _objects.find(key);
        if (searchIt != _objects.end()) { return searchIt->second; }
        throw std::out_of_range("cache miss");
    }

    /**
     * @brief   Checks if a provided key is already present in the cache.
     *
     * @param   key the key to look for.
     * @return  true, if the provided key is present in the cache.
     */
    inline bool has(const Key& key) const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return _objects.find(key) != _objects.end();
    }

    /**
     * @brief   Remove an item from the cache.
     *
     * @param   key the key to remove.
     * @return  true, if an item was removed.
     */
    inline bool remove(const Key& key)
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        auto searchIt = _objects.find(key);

        if (searchIt != _objects.end())
        {
            _objects.erase(searchIt);
            return true;
        }
        return false;
    }

    /**
     * @brief   Clear the cache.
     */
    inline void clear()
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        _objects.clear();
    }

private:

    mutable std::shared_mutex m_mutex;
    std::unordered_map<Key, Value> _objects;
};

}// namespace crocore



