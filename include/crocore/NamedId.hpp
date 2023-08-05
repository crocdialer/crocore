//
// Created by crocdialer on 7/12/22.
//
#pragma once

#include <atomic>

namespace crocore
{

template<typename T>
class NamedId
{
public:
    static NamedId nil() { return {0}; }

    NamedId() = default;

    [[nodiscard]] inline size_t hash() const { return std::hash<uint64_t>()(m_id); }

    [[nodiscard]] inline bool is_nil() const { return !m_id; }

    inline explicit operator bool() const { return !is_nil(); }

    friend inline bool operator<(const NamedId &lhs, const NamedId &rhs) { return lhs.m_id < rhs.m_id; }

    friend inline bool operator==(const NamedId &lhs, const NamedId &rhs) { return lhs.m_id == rhs.m_id; }

    friend inline bool operator!=(const NamedId &lhs, const NamedId &rhs) { return lhs.m_id != rhs.m_id; }

    friend void swap(NamedId &lhs, NamedId &rhs) { std::swap(lhs.m_id, rhs.m_id); }

    friend std::ostream &operator<<(std::ostream &os, const NamedId &self)
    {
        os << self.str();
        return os;
    }

private:
    explicit NamedId(uint64_t id) : m_id(id){};
    static std::atomic<uint64_t> s_next_id;
    uint64_t m_id = s_next_id++;
};

template<typename T>
std::atomic<uint64_t> NamedId<T>::s_next_id = 0;

}// namespace crocore

//! enables usage of NamedId as key in std::unordered_set / std::unordered_map
namespace std
{
template<typename T>
struct hash<crocore::NamedId<T>>
{
    size_t operator()(crocore::NamedId<T> const &named_id) const { return named_id.hash(); }
};
}// namespace std
