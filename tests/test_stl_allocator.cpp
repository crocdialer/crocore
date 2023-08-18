#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>

#include "crocore/BuddyPool.hpp"

crocore::AllocatorPtr create_base_allocator()
{
    constexpr size_t numBytes_1Mb = 1U << 20U;
    crocore::BuddyPool::create_info_t fmt;

    // 128MB
    fmt.block_size = numBytes_1Mb;
    fmt.min_block_size = 512;

    return crocore::BuddyPool::create(fmt);
}

BOOST_AUTO_TEST_CASE(stl_custom_vector_allocator)
{
    // construct a crocore::stl_allocator, wrapping an existing crocore::Allocator
    auto base_allocator = create_base_allocator();

    // std::vector
    {
        crocore::stl_allocator<int> int_allocator(base_allocator);
        auto stl_vec_custom = std::vector<int, crocore::stl_allocator<int>>(int_allocator);

        constexpr size_t elem_count = 42;
        stl_vec_custom.resize(elem_count);

        BOOST_CHECK_EQUAL(stl_vec_custom.size(), elem_count);
        BOOST_CHECK(base_allocator->state().num_bytes_used >= stl_vec_custom.capacity() * sizeof(int));
    }

    // default ctor
    {
        // default ctor works but no backend-allocator is present -> will throw upon usage
        std::vector<int, crocore::stl_allocator<int>> stl_vec_default_ctor;
        BOOST_CHECK_THROW(stl_vec_default_ctor.resize(10), std::bad_alloc);
    }
    BOOST_CHECK_EQUAL(base_allocator->state().num_bytes_used, 0);
}

BOOST_AUTO_TEST_CASE(stl_custom_map_allocator)
{
    // construct a crocore::stl_allocator, wrapping an existing crocore::Allocator
    auto base_allocator = create_base_allocator();

    using key = std::string;
    using value = int;
    // NOTE: map-keys need to be const'ed when using a custom allocator (seems a bit too strict / stupid, but yeah ...)
    using custom_map_allocator_t = crocore::stl_allocator<std::pair<const key, value>>;

    custom_map_allocator_t custom_allocator(base_allocator);

    //    std::map
    {
        using custom_map_type_t = std::map<key, value, std::less<>, custom_map_allocator_t>;

        auto stl_map_custom = custom_map_type_t(custom_allocator);
        stl_map_custom["poop"] = 69;
        stl_map_custom["foo"] = 42;

        BOOST_CHECK_EQUAL(stl_map_custom.size(), 2);
        BOOST_CHECK_EQUAL(stl_map_custom["foo"], 42);
    }
    BOOST_CHECK_EQUAL(base_allocator->state().num_bytes_used, 0);

    //    std::unordered_map
    {
        using custom_map_type_t =
                std::unordered_map<key, value, std::hash<key>, std::equal_to<>, custom_map_allocator_t>;

        auto stl_map_custom = custom_map_type_t(custom_allocator);
        stl_map_custom["poop"] = 69;
        stl_map_custom["foo"] = 42;
        BOOST_CHECK_EQUAL(stl_map_custom.size(), 2);
        BOOST_CHECK_EQUAL(stl_map_custom["foo"], 42);
    }
    BOOST_CHECK_EQUAL(base_allocator->state().num_bytes_used, 0);
}