#include <gtest/gtest.h>
#include "crocore/Application.hpp"

constexpr uint32_t num_runs = 100;

class TestApplication : public crocore::Application
{

public:

    explicit TestApplication(const crocore::Application::create_info_t &create_info) : crocore::Application(create_info) {};

    bool setup_complete = false;

    bool teardown_complete = false;

    bool background_task_complete = false;

    uint32_t num_updates = 0;

    uint32_t num_poll_events = 0;

private:

    void setup() override { setup_complete = true; }

    void update(double time_delta) override
    {
        if(++num_updates >= num_runs){ running = false; }
    }

    void teardown() override
    {
        teardown_complete = true;
        auto future = background_queue().post([&] { background_task_complete = true; });
        future.wait();
    }

    void poll_events() override { num_poll_events++; }
};

TEST(testApplication, basic)
{
    crocore::Application::create_info_t create_info = {};
    auto app = std::make_shared<TestApplication>(create_info);

    ASSERT_EQ(app->run(), EXIT_SUCCESS);
    ASSERT_TRUE(app->setup_complete);
    ASSERT_TRUE(app->teardown_complete);
    ASSERT_TRUE(app->background_task_complete);
    ASSERT_EQ(app->num_poll_events, app->num_updates);
    ASSERT_EQ(num_runs, app->num_updates);
}
