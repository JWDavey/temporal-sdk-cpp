#include <gtest/gtest.h>

#include <any>
#include <stdexcept>
#include <stop_token>
#include <string>

#include "temporalio/activities/activity_context.h"

using namespace temporalio::activities;

// ===========================================================================
// ActivityExecutionContext tests
// ===========================================================================

TEST(ActivityExecutionContextTest, NoCurrentContextThrows) {
    EXPECT_FALSE(ActivityExecutionContext::has_current());
    EXPECT_THROW(ActivityExecutionContext::current(), std::runtime_error);
}

TEST(ActivityExecutionContextTest, ScopeSetsAndRestoresContext) {
    ActivityInfo info;
    info.activity_id = "test-act";
    info.activity_type = "TestActivity";

    std::stop_source cancel_source;
    std::stop_source shutdown_source;

    ActivityExecutionContext ctx(info, cancel_source.get_token(),
                                 shutdown_source.get_token());

    EXPECT_FALSE(ActivityExecutionContext::has_current());

    {
        ActivityContextScope scope(ctx);
        EXPECT_TRUE(ActivityExecutionContext::has_current());
        EXPECT_EQ(ActivityExecutionContext::current().info().activity_id,
                  "test-act");
        EXPECT_EQ(ActivityExecutionContext::current().info().activity_type,
                  "TestActivity");
    }

    EXPECT_FALSE(ActivityExecutionContext::has_current());
}

TEST(ActivityExecutionContextTest, NestedScopes) {
    ActivityInfo info1;
    info1.activity_id = "outer";
    ActivityInfo info2;
    info2.activity_id = "inner";

    std::stop_source s1, s2;

    ActivityExecutionContext ctx1(info1, s1.get_token(), s1.get_token());
    ActivityExecutionContext ctx2(info2, s2.get_token(), s2.get_token());

    {
        ActivityContextScope scope1(ctx1);
        EXPECT_EQ(ActivityExecutionContext::current().info().activity_id,
                  "outer");

        {
            ActivityContextScope scope2(ctx2);
            EXPECT_EQ(
                ActivityExecutionContext::current().info().activity_id,
                "inner");
        }

        // Restored to outer
        EXPECT_EQ(ActivityExecutionContext::current().info().activity_id,
                  "outer");
    }

    EXPECT_FALSE(ActivityExecutionContext::has_current());
}

TEST(ActivityExecutionContextTest, CancellationToken) {
    ActivityInfo info;
    std::stop_source cancel_source;
    std::stop_source shutdown_source;

    ActivityExecutionContext ctx(info, cancel_source.get_token(),
                                 shutdown_source.get_token());

    EXPECT_FALSE(ctx.cancellation_token().stop_requested());
    cancel_source.request_stop();
    EXPECT_TRUE(ctx.cancellation_token().stop_requested());
}

TEST(ActivityExecutionContextTest, WorkerShutdownToken) {
    ActivityInfo info;
    std::stop_source cancel_source;
    std::stop_source shutdown_source;

    ActivityExecutionContext ctx(info, cancel_source.get_token(),
                                 shutdown_source.get_token());

    EXPECT_FALSE(ctx.worker_shutdown_token().stop_requested());
    shutdown_source.request_stop();
    EXPECT_TRUE(ctx.worker_shutdown_token().stop_requested());
}

TEST(ActivityExecutionContextTest, Heartbeat) {
    ActivityInfo info;
    std::stop_source s;

    ActivityExecutionContext ctx(info, s.get_token(), s.get_token());

    int heartbeat_count = 0;
    std::string last_detail;

    ctx.set_heartbeat_callback([&](const std::any& details) {
        ++heartbeat_count;
        if (details.has_value()) {
            last_detail = std::any_cast<std::string>(details);
        }
    });

    ctx.heartbeat(std::any(std::string("progress-1")));
    EXPECT_EQ(heartbeat_count, 1);
    EXPECT_EQ(last_detail, "progress-1");

    ctx.heartbeat(std::any(std::string("progress-2")));
    EXPECT_EQ(heartbeat_count, 2);
    EXPECT_EQ(last_detail, "progress-2");
}

TEST(ActivityExecutionContextTest, HeartbeatWithoutCallback) {
    ActivityInfo info;
    std::stop_source s;

    ActivityExecutionContext ctx(info, s.get_token(), s.get_token());

    // Should not crash even without a callback set
    EXPECT_NO_THROW(ctx.heartbeat());
}

TEST(ActivityExecutionContextTest, HeartbeatEmptyDetails) {
    ActivityInfo info;
    std::stop_source s;

    ActivityExecutionContext ctx(info, s.get_token(), s.get_token());

    bool called = false;
    ctx.set_heartbeat_callback([&](const std::any& details) {
        called = true;
        EXPECT_FALSE(details.has_value());
    });

    ctx.heartbeat();
    EXPECT_TRUE(called);
}

// ===========================================================================
// CompleteAsyncException tests
// ===========================================================================

TEST(CompleteAsyncExceptionTest, Message) {
    CompleteAsyncException ex;
    EXPECT_NE(std::string(ex.what()), "");
}

TEST(CompleteAsyncExceptionTest, InheritsFromStdException) {
    CompleteAsyncException ex;
    const std::exception& base = ex;
    EXPECT_NE(std::string(base.what()), "");
}

// ===========================================================================
// ActivityContextScope non-copyable
// ===========================================================================

TEST(ActivityContextScopeTest, IsNonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<ActivityContextScope>);
    EXPECT_FALSE(std::is_copy_assignable_v<ActivityContextScope>);
}
