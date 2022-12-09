//
// Created by crocdialer on 2/12/22.
//

#pragma once

#include <unordered_map>
#include <list>

namespace crocore
{
/**
 * @brief   SetLRU is a Least-Recently-Used set container.
 * @tparam  Key     Type of the Key
 *
 * SetLRU is a set-like container offering both constant-time lookup and iteration in order of insertion.
 */
template<typename Key>
class SetLRU
{
public:

    SetLRU() = default;

    template<typename InputIterator>
    SetLRU(InputIterator first, InputIterator last)
    {
        for(auto it = first; it < last; ++it){ push_back(*it); }
    }

    SetLRU(std::initializer_list<Key> items)
    {
        for(const auto &item : items){ push_back(item); }
    }

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

    inline void push_back(const Key &key)
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

    inline void pop_front()
    {
        if(!empty())
        {
            _objects.erase(_list.front());
            _list.pop_front();
        }
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

    inline typename std::list<Key>::iterator begin() noexcept{ return _list.begin(); }
    inline typename std::list<Key>::iterator rbegin() noexcept{ return _list.rbegin(); }

    inline typename std::list<Key>::const_iterator cbegin() const noexcept{ return _list.cbegin(); }
    inline typename std::list<Key>::const_iterator rcbegin() const noexcept{ return _list.rcbegin(); }

    inline typename std::list<Key>::iterator end() noexcept{ return _list.end(); }
    inline typename std::list<Key>::iterator rend() noexcept{ return _list.rend(); }

    inline typename std::list<Key>::const_iterator cend() const noexcept{ return _list.cend(); }
    inline typename std::list<Key>::const_iterator rcend() const noexcept{ return _list.rcend(); }

private:

    std::unordered_map<Key, typename std::list<Key>::iterator> _objects;
    std::list<Key> _list;
};

} // namespace crocore

