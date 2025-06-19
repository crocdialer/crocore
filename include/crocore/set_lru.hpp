//
// Created by crocdialer on 2/12/22.
//

#pragma once

#include <list>
#include <unordered_map>

namespace crocore
{
/**
 * @brief   set_lru is a Least-Recently-Used set container.
 * @tparam  Key     Type of the Key
 *
 * set_lru is a set-like container offering both constant-time lookup and iteration in order of insertion.
 */
template<typename Key>
class set_lru
{
public:
    set_lru() = default;

    template<typename InputIterator>
    set_lru(InputIterator first, InputIterator last)
    {
        for(auto it = first; it < last; ++it) { push_back(*it); }
    }

    set_lru(std::initializer_list<Key> items)
    {
        for(const auto &item: items) { push_back(item); }
    }

    /**
     * @brief   Checks if a provided key is already present in the set.
     *
     * @param   key the key to look for.
     * @return  true, if the provided key is present in the set.
     */
    inline bool contains(const Key &key) const { return m_objects.find(key) != m_objects.end(); }

    inline void push_back(const Key &key)
    {
        auto mapIt = m_objects.find(key);

        if(mapIt != m_objects.end())
        {
            m_list.erase(mapIt->second);
            m_objects.erase(mapIt);
        }
        auto it = m_list.insert(m_list.end(), key);
        m_objects.emplace(key, it);
    }

    inline void pop_front()
    {
        if(!empty())
        {
            m_objects.erase(m_list.front());
            m_list.pop_front();
        }
    }

    void remove(Key k)
    {
        auto it = m_objects.find(k);

        if(it != m_objects.end())
        {
            m_list.erase(it->second);
            m_objects.erase(it);
        }
    }

    [[nodiscard]] inline size_t empty() const { return m_objects.empty(); }

    [[nodiscard]] inline size_t size() const { return m_objects.size(); }

    inline void clear()
    {
        m_objects.clear();
        m_list.clear();
    }

    inline typename std::list<Key>::iterator begin() noexcept { return m_list.begin(); }
    inline typename std::list<Key>::reverse_iterator rbegin() noexcept { return m_list.rbegin(); }

    inline typename std::list<Key>::const_iterator cbegin() const noexcept { return m_list.cbegin(); }
    inline typename std::list<Key>::const_reverse_iterator rcbegin() const noexcept { return m_list.rcbegin(); }

    inline typename std::list<Key>::iterator end() noexcept { return m_list.end(); }
    inline typename std::list<Key>::reverse_iterator rend() noexcept { return m_list.rend(); }

    inline typename std::list<Key>::const_iterator cend() const noexcept { return m_list.cend(); }
    inline typename std::list<Key>::const_reverse_iterator rcend() const noexcept { return m_list.rcend(); }

private:
    std::unordered_map<Key, typename std::list<Key>::iterator> m_objects;
    std::list<Key> m_list;
};

}// namespace crocore
