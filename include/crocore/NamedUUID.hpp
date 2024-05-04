//
// Created by crocdialer on 7/12/22.
//
#pragma once

#include <crocore/uuid.h>
#include <ostream>
#include <string>


namespace crocore
{

//! helper-macro to define a NamedId
#define DEFINE_NAMED_UUID(UUID_NAME)                                                                                   \
    namespace named_uuid_internal                                                                                      \
    {                                                                                                                  \
    struct UUID_NAME##Param;                                                                                           \
    }                                                                                                                  \
    using UUID_NAME = crocore::NamedUUID<named_uuid_internal::UUID_NAME##Param>;

/**
 * @brief   NamedIdBase is a common base-class for all NamedIds and offers a shared,
 *          thread_local boost::uuids::basic_random_generator.
 */
class NamedUUIDBase
{
protected:
    static uuids::uuid_random_generator &generator()
    {
        static thread_local std::optional<uuids::uuid_random_generator> uuid_gen;
        static thread_local std::optional<std::mt19937> rng;

        if(!uuid_gen)
        {
            std::random_device rd;
            auto seed_data = std::array<int, std::mt19937::state_size>{};
            std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
            std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
            rng = std::mt19937(seq);
            uuid_gen = uuids::uuid_random_generator(*rng);
        }
        return *uuid_gen;
    }
};

template<typename T>
class NamedUUID : public NamedUUIDBase
{
public:
    static constexpr NamedUUID nil() { return NamedUUID(uuids::uuid()); }

    static NamedUUID random() { return NamedUUID(generator()()); }

    static NamedUUID from_name(const std::string_view &name, const uuids::uuid &namespace_uuid = {})
    {
        uuids::uuid_name_generator name_generator(namespace_uuid);
        return NamedUUID(name_generator(name));
    }

    NamedUUID() = default;

    explicit NamedUUID(const std::string &str)
    {
        if(auto parsed_uuid = uuids::uuid::from_string(str)) { m_uuid = *parsed_uuid; }
        else { m_uuid = {}; }
    }

    [[nodiscard]] std::string str() const { return uuids::to_string(m_uuid); }

    [[nodiscard]] inline size_t hash() const { return std::hash<uuids::uuid>()(m_uuid); }

    [[nodiscard]] inline bool is_nil() const { return m_uuid.is_nil(); }

    inline explicit operator bool() const { return !is_nil(); }

    inline constexpr bool operator<(const NamedUUID &other) const { return m_uuid < other.m_uuid; }

    inline constexpr bool operator==(const NamedUUID &other) const { return m_uuid == other.m_uuid; }

    inline constexpr bool operator!=(const NamedUUID &other) const { return m_uuid != other.m_uuid; }

    friend void swap(NamedUUID &lhs, NamedUUID &rhs) { lhs.m_uuid.swap(rhs.m_uuid); }

    friend std::ostream &operator<<(std::ostream &os, const NamedUUID &self)
    {
        os << self.str();
        return os;
    }

private:
    explicit NamedUUID(const uuids::uuid &uuid) : m_uuid(uuid){};
    uuids::uuid m_uuid = generator()();
};

}// namespace crocore

//! enables usage of NamedId as key in std::unordered_set / std::unordered_map
namespace std
{
template<typename T>
struct hash<crocore::NamedUUID<T>>
{
    size_t operator()(crocore::NamedUUID<T> const &named_id) const { return named_id.hash(); }
};
}// namespace std