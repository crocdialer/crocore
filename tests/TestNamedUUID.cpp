#include <gtest/gtest.h>

#include "crocore/NamedUUID.hpp"
#include <set>

using namespace crocore;

DEFINE_NAMED_UUID(TestId)
DEFINE_NAMED_UUID(AnotherId)

TEST(NamedUUID, nilId)
{
    TestId nil = TestId::nil();
    ASSERT_TRUE(nil.is_nil());
    ASSERT_TRUE(!nil);
}

TEST(NamedUUID, newRandomId)
{
    TestId a;
    TestId b;

    ASSERT_TRUE(!a.is_nil());
    ASSERT_TRUE(!b.is_nil());
    ASSERT_TRUE(a != b);
    ASSERT_TRUE(!(a == b));
    ASSERT_NE(a, b);
}

TEST(NamedUUID, by_name)
{
    constexpr auto test_namespace_uuid = uuids::uuid::from_string("47183823-2574-4bfd-b411-99ed177d3e43");
    ASSERT_TRUE(test_namespace_uuid);
    TestId a = TestId::from_name("foo");
    TestId b = TestId::from_name("foo");
    TestId c = TestId::from_name("foo", *test_namespace_uuid);
    TestId d = TestId::from_name("bar");

    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_TRUE(d);
    ASSERT_EQ(a, b);
    ASSERT_NE(a, c);
    ASSERT_NE(a, d);
}

TEST(NamedUUID, trivialCopyConstruct)
{
    TestId a;
    TestId b(a);
    ASSERT_EQ(a, b);
}

TEST(NamedUUID, copyAssign)
{
    TestId a;
    TestId b;
    b = a;
    ASSERT_TRUE(a == b);
}

TEST(NamedUUID, useInMap)
{
    TestId a;
    TestId b;

    std::map<TestId, std::string> map = {{a, "a"}, {b, "b"}};

    ASSERT_EQ(map[a], "a");
    ASSERT_EQ(map[b], "b");
}

TEST(NamedUUID, hashing)
{
    TestId a;

    std::hash<TestId> hasher;
    size_t h = hasher(a);
    (void) h;
}

TEST(NamedUUID, useInUnorderedMap)
{
    TestId a;
    TestId b;

    std::unordered_map<TestId, std::string> map = {{a, "a"}, {b, "b"}};

    ASSERT_EQ(map[a], "a");
    ASSERT_EQ(map[b], "b");
}

TEST(NamedUUID, useInSet)
{
    TestId a;
    TestId b;
    TestId c;
    std::set<TestId> set = {a, b};

    ASSERT_TRUE(set.contains(a));
    ASSERT_TRUE(!set.contains(c));
}

TEST(NamedUUID, different)
{
    AnotherId a;
    TestId b;

    // Compile failure:
    // a = b;
}
