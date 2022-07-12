//
// Created by crocdialer on 7/12/22.
//
#pragma once

#include <string>
#include <ostream>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace crocore
{

/**
 * @brief   NamedIdBase is a common base-class for all NamedIds and offers a shared,
 *          thread_local boost::uuids::basic_random_generator.
 */
class NamedIdBase
{
protected:

    static boost::uuids::random_generator &generator()
    {
        static thread_local boost::uuids::random_generator gen;
        return gen;
    }
};

template<typename T>
class NamedId : public NamedIdBase
{
public:

    static NamedId nil(){ return {boost::uuids::nil_uuid()}; }

    NamedId() = default;

    explicit NamedId(const std::string &str) : m_uuid(boost::uuids::string_generator()(str)){}

    [[nodiscard]] std::string str() const{ return boost::uuids::to_string(m_uuid); }

    [[nodiscard]] inline size_t hash() const
    {
        return boost::uuids::hash_value(m_uuid);
    }

    [[nodiscard]] inline bool is_nil() const{ return m_uuid.is_nil(); }

    inline explicit operator bool() const{ return !is_nil(); }

    friend inline bool operator<(const NamedId &lhs, const NamedId &rhs)
    {
        return lhs.m_uuid < rhs.m_uuid;
    }

    friend inline bool operator==(const NamedId &lhs, const NamedId &rhs)
    {
        return lhs.m_uuid == rhs.m_uuid;
    }

    friend inline bool operator!=(const NamedId &lhs, const NamedId &rhs)
    {
        return lhs.m_uuid != rhs.m_uuid;
    }

    friend void swap(NamedId &lhs, NamedId &rhs)
    {
        lhs.m_uuid.swap(rhs.m_uuid);
    }

    friend std::ostream &operator<<(std::ostream &os, const NamedId &self)
    {
        os << self.str();
        return os;
    }

private:
    explicit NamedId(const boost::uuids::uuid &uuid) : m_uuid(uuid){};
    boost::uuids::uuid m_uuid = generator()();
};

}

//! enables usage of NamedId as key in std::unordered_set / std::unordered_map
namespace std
{
template<typename T>
struct hash<crocore::NamedId<T>>
{
    size_t operator()(crocore::NamedId<T> const &named_id) const{ return named_id.hash(); }
};
} // namespace std
