//
// Created by crocdialer on 2/12/22.
//

#pragma once

#include <unordered_map>
#include <list>

namespace crocore
{
/**
 * @brief   A Least-Recently-Used set container
 * @tparam  Key     Type of the Key
 *
 * LRU is a cache that can store key-value pairs.
 * Keys can be retrieved in the least-recently-used order.
 */
template<typename Key>
class SetLRU
{
public:

    SetLRU() = default;

    /**
     * @brief   Checks if a provided key is already present in the set.
     *
     * @param   key the key to look for.
     * @return  true, if the provided key is present in the set.
     */
    inline bool has(const Key &key) const
    {
        return _objects.find(key) != _objects.end();
    }

    inline void put(const Key &key)
    {
        auto mapIt = _objects.find(key);

        if(mapIt != _objects.end())
        {
            _list.erase(mapIt->second);
            _objects.erase(mapIt);
        }
        auto it = _list.insert(_list.end(), key);
        _objects.emplace(key, it).first;
    }

    void remove(Key k)
    {
        auto it = _objects.find(k);

        if(it != _objects.end())
        {
            _list.erase(it->second.first);
            _objects.erase(it);
        }
    }

    [[nodiscard]] inline size_t empty() const{ return _objects.empty(); }

    [[nodiscard]] inline size_t size() const{ return _objects.size(); }

    inline void clear()
    {
        _objects.clear();
        _list.clear();
    }

    inline void pop_front()
    {
        if(!empty())
        {
            _objects.erase(_list.front());
            _list.pop_front();
        }
    }

    inline typename std::list<Key>::iterator begin() noexcept{ return _list.begin(); }

    inline typename std::list<Key>::const_iterator cbegin() const noexcept{ return _list.cbegin(); }

    inline typename std::list<Key>::iterator end() noexcept{ return _list.end(); }

    inline typename std::list<Key>::const_iterator cend() const noexcept{ return _list.cend(); }

private:

    std::unordered_map<Key, typename std::list<Key>::iterator> _objects;
    std::list<Key> _list;
};

} // namespace crocore

