#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include "crocore/crocore.hpp"
#include "crocore/set_lru.hpp"

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_set_lru )
{
    crocore::set_lru<std::string> string_set;
    BOOST_CHECK(string_set.size() == 0);
    BOOST_CHECK(string_set.empty());

    string_set.push_back("foo");
    BOOST_CHECK(!string_set.empty());
    BOOST_CHECK(string_set.size() == 1);

    crocore::set_lru<int> int_set;
    int_set.push_back(9);
    int_set.push_back(3);
    int_set.push_back(2);
    int_set.push_back(1);
    int_set.push_back(69);
    int_set.push_back(1);
    int_set.push_back(2);
    int_set.push_back(3);

    BOOST_CHECK(int_set.contains(9));
    BOOST_CHECK(int_set.contains(69));
    BOOST_CHECK(int_set.contains(1));

    // pop first key
    int_set.pop_front();

    // remove specific key
    int_set.remove(69);

    // we expect least recently used order
    std::vector<int> truth = {1, 2, 3};
    std::vector<int> set_content = {int_set.begin(), int_set.end()};
    BOOST_CHECK(set_content == truth);

    int_set.clear();
    BOOST_CHECK(int_set.empty());
}

//____________________________________________________________________________//

// EOF
