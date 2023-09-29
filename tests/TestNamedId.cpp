#include <gtest/gtest.h>

#include <set>
#include "crocore/NamedId.hpp"

using namespace crocore;

DEFINE_NAMED_ID(TestId)

TEST(NamedId, nilId)
{
    TestId nil = TestId::nil();
    ASSERT_TRUE(nil.is_nil());
    ASSERT_TRUE(!nil);
}

TEST(NamedId, newRandomId)
{
    TestId a;
    TestId b;

    ASSERT_TRUE(!a.is_nil());
    ASSERT_TRUE(!b.is_nil());
    ASSERT_TRUE(a != b);
    ASSERT_TRUE(!(a == b));
    ASSERT_NE(a, b);
}

TEST(NamedId, trivialCopyConstruct)
{
    TestId a;
    TestId b(a);
    ASSERT_EQ(a, b);
}

TEST(NamedId, copyAssign)
{
    TestId a;
    TestId b;
    b = a;
    ASSERT_TRUE(a == b);
}

TEST(NamedId, useInMap)
{
    TestId a;
    TestId b;

    std::map<TestId, std::string> map = {
            {a, "a"},
            {b, "b"}};

    ASSERT_EQ(map[a], "a");
    ASSERT_EQ(map[b], "b");
}

TEST(NamedId, useInUnorderedMap)
{
    TestId a;
    TestId b;

    std::unordered_map<TestId, std::string> map = {
            {a, "a"},
            {b, "b"}};

    ASSERT_EQ(map[a], "a");
    ASSERT_EQ(map[b], "b");
}

TEST(NamedId, useInSet)
{
    TestId a;
    TestId b;
    TestId c;
    std::set<TestId> set = {a, b};

    ASSERT_TRUE(set.contains(a));
    ASSERT_TRUE(!set.contains(c));
}

TEST(NamedId, different)
{
    using AnotherId = NamedId<struct Another>;

    AnotherId a;
    TestId b;

    // Compile failure:
    // a = b;
}
