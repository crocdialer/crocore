#include <gtest/gtest.h>

#include "crocore/crocore.hpp"
#include "crocore/CircularBuffer.hpp"


TEST(testCircularBuffer, basic)
{
    crocore::CircularBuffer<float> circ_buf(8);
    ASSERT_TRUE(circ_buf.capacity() == 8);
    ASSERT_TRUE(circ_buf.empty());
    
    for(uint32_t i = 0; i < 1000; i++)
    {
        circ_buf.set_capacity(i);
        ASSERT_TRUE(circ_buf.capacity() == i);
        
        circ_buf = crocore::CircularBuffer<float>(i);
        ASSERT_TRUE(circ_buf.capacity() == i);
    }
    
    circ_buf.set_capacity(6);
    ASSERT_TRUE(circ_buf.capacity() == 6);

    circ_buf.push_back(1);
    ASSERT_TRUE(circ_buf.size() == 1);
    ASSERT_TRUE(circ_buf[0] == 1);

    circ_buf.push_back(2);
    circ_buf.push_back(3);
    ASSERT_TRUE(circ_buf.size() == 3);

    // fill buffer completely
    circ_buf.push_back(4);
    circ_buf.push_back(5);
    circ_buf.push_back(6);
    ASSERT_TRUE(circ_buf.size() == 6);

    // pop first element, decreasing size
    auto val = circ_buf.front();
    circ_buf.pop_front();
    ASSERT_TRUE(val == 1);
    ASSERT_TRUE(circ_buf.size() == 5);
    ASSERT_TRUE(circ_buf[0] == 2);
    
    // overfill buffer
    circ_buf.push_back(101);
    circ_buf.push_back(102);
    circ_buf.push_back(666);
    circ_buf.push_back(103);
    ASSERT_TRUE(circ_buf.size() == 6);

    ASSERT_TRUE(circ_buf[0] == 5);
    ASSERT_TRUE(circ_buf[1] == 6);
    ASSERT_TRUE(circ_buf[2] == 101);
    ASSERT_TRUE(circ_buf[3] == 102);
    ASSERT_TRUE(circ_buf[4] == 666);
    ASSERT_TRUE(circ_buf[5] == 103);

    auto median = crocore::median(circ_buf);
    ASSERT_TRUE(median == 101.5);
    
    // new capacity, buffer is empty after resize
    circ_buf.set_capacity(7);
    ASSERT_TRUE(circ_buf.empty());
    
    // push 8 elements into the 7-sized buffer
    circ_buf.push_back(89);// will fall out
    ASSERT_TRUE(circ_buf.size() == 1);
    circ_buf.push_back(2);
    ASSERT_TRUE(circ_buf.size() == 2);
    circ_buf.push_back(46);
    ASSERT_TRUE(circ_buf.size() == 3);
    circ_buf.push_back(4);// will be the median
    ASSERT_TRUE(circ_buf.size() == 4);
    circ_buf.push_back(88);
    ASSERT_TRUE(circ_buf.size() == 5);
    circ_buf.push_back(3);
    ASSERT_TRUE(circ_buf.size() == 6);
    circ_buf.push_back(87);
    ASSERT_TRUE(circ_buf.size() == 7);
    circ_buf.push_back(1);
    ASSERT_TRUE(circ_buf.size() == 7);
    ASSERT_TRUE(crocore::median(circ_buf) == 4);
    
    int k = 0;
    for(const auto &v : circ_buf){ printf("val[%d]: %.2f\n", k, v); k++; }
    
    printf("median: %.2f\n", crocore::median(circ_buf));
    
    circ_buf.clear();
    ASSERT_TRUE(circ_buf.empty());
    
    constexpr uint32_t num_elems = 1500;
    constexpr uint32_t capacity = 750;

    printf("\npushing %d elements in range (0 - 100):\n", num_elems);
    circ_buf.set_capacity(capacity);
    for(uint32_t i = 0; i < num_elems; i++)
    {
        ASSERT_TRUE(circ_buf.size() == std::min<uint32_t>(i, capacity));
        circ_buf.push_back(static_cast<float>(crocore::random_int(0, 100)));
    }
    printf("mean: %.2f\n", crocore::mean(circ_buf));
    printf("standard deviation: %.2f\n", crocore::standard_deviation(circ_buf));
    ASSERT_TRUE(circ_buf.size() == capacity);
    
    for(uint32_t i = 0; i < num_elems; i++){ circ_buf.pop_front(); }
    ASSERT_TRUE(circ_buf.size() == 0);
    ASSERT_TRUE(circ_buf.empty());
}

//____________________________________________________________________________//

// EOF
