#include <gtest/gtest.h>
#include "crocore/crocore.hpp"
#include "crocore/set_lru.hpp"

//____________________________________________________________________________//

TEST(test_set_lru, basic)
{
    crocore::set_lru<std::string> string_set;
    ASSERT_EQ(string_set.size(), 0);
    ASSERT_TRUE(string_set.empty());

    string_set.push_back("foo");
    ASSERT_TRUE(!string_set.empty());
    ASSERT_TRUE(string_set.size() == 1);

    crocore::set_lru<int> int_set;
    int_set.push_back(9);
    int_set.push_back(3);
    int_set.push_back(2);
    int_set.push_back(1);
    int_set.push_back(69);
    int_set.push_back(1);
    int_set.push_back(2);
    int_set.push_back(3);

    ASSERT_TRUE(int_set.contains(9));
    ASSERT_TRUE(int_set.contains(69));
    ASSERT_TRUE(int_set.contains(1));

    // pop first key
    int_set.pop_front();

    // remove specific key
    int_set.remove(69);

    // we expect least recently used order
    std::vector<int> truth = {1, 2, 3};
    std::vector<int> set_content = {int_set.begin(), int_set.end()};
    ASSERT_TRUE(set_content == truth);

    int_set.clear();
    ASSERT_TRUE(int_set.empty());
}

//____________________________________________________________________________//

// EOF
