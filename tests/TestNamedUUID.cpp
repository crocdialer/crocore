//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test

// each test module could contain no more then one 'main' file with init function defined
// alternatively you could define init function yourself
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <set>
#include "crocore/NamedUUID.hpp"

using namespace crocore;

DEFINE_NAMED_UUID(TestId)
DEFINE_NAMED_UUID(AnotherId)

BOOST_AUTO_TEST_CASE(nilId)
{
    TestId nil = TestId::nil();
    BOOST_CHECK(nil.is_nil());
    BOOST_CHECK(!nil);
}

BOOST_AUTO_TEST_CASE(newRandomId)
{
    TestId a;
    TestId b;

    BOOST_CHECK(!a.is_nil());
    BOOST_CHECK(!b.is_nil());
    BOOST_CHECK(a != b);
    BOOST_CHECK(!(a == b));
    BOOST_CHECK_NE(a, b);
}

BOOST_AUTO_TEST_CASE(trivialCopyConstruct)
{
    TestId a;
    TestId b(a);
    BOOST_CHECK_EQUAL(a, b);
}

BOOST_AUTO_TEST_CASE(copyAssign)
{
    TestId a;
    TestId b;
    b = a;
    BOOST_CHECK(a == b);
}

BOOST_AUTO_TEST_CASE(useInMap)
{
    TestId a;
    TestId b;

    std::map<TestId, std::string> map = {
            {a, "a"},
            {b, "b"}};

    BOOST_CHECK_EQUAL(map[a], "a");
    BOOST_CHECK_EQUAL(map[b], "b");
}

BOOST_AUTO_TEST_CASE(useInUnorderedMap)
{
    TestId a;
    TestId b;

    std::unordered_map<TestId, std::string> map = {
            {a, "a"},
            {b, "b"}};

    BOOST_CHECK_EQUAL(map[a], "a");
    BOOST_CHECK_EQUAL(map[b], "b");
}

BOOST_AUTO_TEST_CASE(useInSet)
{
    TestId a;
    TestId b;
    TestId c;
    std::set<TestId> set = {a, b};

    BOOST_CHECK(set.contains(a));
    BOOST_CHECK(!set.contains(c));
}

BOOST_AUTO_TEST_CASE(different)
{
    AnotherId a;
    TestId b;

    // Compile failure:
    // a = b;
}
