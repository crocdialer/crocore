//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test

// each test module could contain no more then one 'main' file with init function defined
// alternatively you could define init function yourself
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include "crocore/Application.hpp"

constexpr uint32_t num_runs = 100;

class TestApplication : public crocore::Application
{

public:

    explicit TestApplication(int argc = 0, char *argv[] = nullptr) : crocore::Application(argc, argv) {};

    bool setup_complete = false;

    bool teardown_complete = false;

    uint32_t num_updates = 0;

    uint32_t num_poll_events = 0;

private:

    void setup() override { setup_complete = true; }

    void update(double time_delta) override
    {
        if(++num_updates >= num_runs){ set_running(false); }
    }

    void teardown() override { teardown_complete = true; }

    void poll_events() override { num_poll_events++; }
};

BOOST_AUTO_TEST_CASE(testApplication)
{
    auto app = std::make_shared<TestApplication>();
    BOOST_CHECK_EQUAL(app->run(), EXIT_SUCCESS);
    BOOST_CHECK(app->setup_complete);
    BOOST_CHECK(app->teardown_complete);
    BOOST_CHECK_EQUAL(app->num_poll_events, app->num_updates);
    BOOST_CHECK_EQUAL(num_runs, app->num_updates);
}
