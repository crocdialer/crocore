//
// Created by crocdialer on 7/12/22.
//
#pragma once

#include <algorithm>
#include <atomic>

namespace crocore
{

//! helper-macro to define a NamedId
#define DEFINE_NAMED_ID(ID_NAME)                                                                                       \
    namespace named_id_internal                                                                                        \
    {                                                                                                                  \
    struct ID_NAME##Param;                                                                                             \
    }                                                                                                                  \
    using ID_NAME = crocore::NamedId<named_id_internal::ID_NAME##Param>;

template<typename T>
class NamedId
{
public:
    static constexpr NamedId<T> nil() { return NamedId(0); }

    NamedId() = default;

    [[nodiscard]] inline size_t hash() const { return std::hash<uint64_t>()(m_id); }

    [[nodiscard]] inline bool is_nil() const { return !m_id; }

    inline explicit operator bool() const { return !is_nil(); }

    inline constexpr bool operator<(const NamedId &other) const { return m_id < other.m_id; }

    inline constexpr bool operator==(const NamedId &other) const { return m_id == other.m_id; }

    inline constexpr bool operator!=(const NamedId &other) const { return m_id != other.m_id; }

    [[nodiscard]] uint64_t value() const { return m_id; }

    friend void swap(NamedId &lhs, NamedId &rhs) { std::swap(lhs.m_id, rhs.m_id); }

private:
    explicit NamedId(uint64_t id) : m_id(id) {};
    static std::atomic<uint64_t> s_next_id;
    uint64_t m_id = ++s_next_id;
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
