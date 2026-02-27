#include <gtest/gtest.h>

#include <any>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "temporalio/worker/interceptors/worker_interceptor.h"

using namespace temporalio::worker::interceptors;
using namespace std::chrono_literals;

// ===========================================================================
// Input type tests
// ===========================================================================

TEST(ExecuteWorkflowInputTest, DefaultValues) {
    ExecuteWorkflowInput input;
    EXPECT_EQ(input.instance, nullptr);
    EXPECT_TRUE(input.args.empty());
}

TEST(HandleSignalInputTest, DefaultValues) {
    HandleSignalInput input;
    EXPECT_TRUE(input.signal.empty());
    EXPECT_EQ(input.definition, nullptr);
    EXPECT_TRUE(input.args.empty());
    EXPECT_TRUE(input.headers.empty());
}

TEST(HandleSignalInputTest, CustomValues) {
    HandleSignalInput input{
        .signal = "my-signal",
        .args = {std::any(42)},
        .headers = {{"trace-id", "abc-123"}},
    };
    EXPECT_EQ(input.signal, "my-signal");
    ASSERT_EQ(input.args.size(), 1u);
    EXPECT_EQ(std::any_cast<int>(input.args[0]), 42);
    EXPECT_EQ(input.headers.at("trace-id"), "abc-123");
}

TEST(HandleQueryInputTest, DefaultValues) {
    HandleQueryInput input;
    EXPECT_TRUE(input.id.empty());
    EXPECT_TRUE(input.query.empty());
    EXPECT_EQ(input.definition, nullptr);
    EXPECT_TRUE(input.args.empty());
    EXPECT_TRUE(input.headers.empty());
}

TEST(HandleQueryInputTest, CustomValues) {
    HandleQueryInput input{
        .id = "query-1",
        .query = "get_status",
        .args = {std::any(std::string("param"))},
    };
    EXPECT_EQ(input.id, "query-1");
    EXPECT_EQ(input.query, "get_status");
    ASSERT_EQ(input.args.size(), 1u);
}

TEST(HandleUpdateInputTest, DefaultValues) {
    HandleUpdateInput input;
    EXPECT_TRUE(input.id.empty());
    EXPECT_TRUE(input.update.empty());
    EXPECT_EQ(input.definition, nullptr);
    EXPECT_TRUE(input.args.empty());
    EXPECT_TRUE(input.headers.empty());
}

TEST(HandleUpdateInputTest, CustomValues) {
    HandleUpdateInput input{
        .id = "upd-1",
        .update = "set_value",
        .args = {std::any(100)},
        .headers = {{"origin", "test"}},
    };
    EXPECT_EQ(input.id, "upd-1");
    EXPECT_EQ(input.update, "set_value");
    ASSERT_EQ(input.args.size(), 1u);
    EXPECT_EQ(input.headers.at("origin"), "test");
}

TEST(DelayAsyncInputTest, DefaultValues) {
    DelayAsyncInput input;
    EXPECT_EQ(input.delay, 0ms);
    EXPECT_TRUE(input.summary.empty());
}

TEST(DelayAsyncInputTest, CustomValues) {
    DelayAsyncInput input{
        .delay = 5000ms,
        .summary = "waiting for condition",
    };
    EXPECT_EQ(input.delay, 5000ms);
    EXPECT_EQ(input.summary, "waiting for condition");
}

TEST(ScheduleActivityInputTest, DefaultValues) {
    ScheduleActivityInput input;
    EXPECT_TRUE(input.activity.empty());
    EXPECT_TRUE(input.args.empty());
    EXPECT_TRUE(input.headers.empty());
}

TEST(ScheduleActivityInputTest, CustomValues) {
    ScheduleActivityInput input{
        .activity = "SendEmail",
        .args = {std::any(std::string("user@example.com"))},
    };
    EXPECT_EQ(input.activity, "SendEmail");
    ASSERT_EQ(input.args.size(), 1u);
}

TEST(ScheduleLocalActivityInputTest, DefaultValues) {
    ScheduleLocalActivityInput input;
    EXPECT_TRUE(input.activity.empty());
    EXPECT_TRUE(input.args.empty());
    EXPECT_TRUE(input.headers.empty());
}

TEST(StartChildWorkflowInputTest, DefaultValues) {
    StartChildWorkflowInput input;
    EXPECT_TRUE(input.workflow.empty());
    EXPECT_TRUE(input.args.empty());
    EXPECT_TRUE(input.headers.empty());
}

TEST(StartChildWorkflowInputTest, CustomValues) {
    StartChildWorkflowInput input{
        .workflow = "ChildWorkflow",
        .args = {std::any(std::string("data"))},
        .headers = {{"parent-id", "wf-1"}},
    };
    EXPECT_EQ(input.workflow, "ChildWorkflow");
    ASSERT_EQ(input.args.size(), 1u);
    EXPECT_EQ(input.headers.at("parent-id"), "wf-1");
}

TEST(SignalChildWorkflowInputTest, DefaultValues) {
    SignalChildWorkflowInput input;
    EXPECT_TRUE(input.signal.empty());
    EXPECT_TRUE(input.args.empty());
}

TEST(SignalExternalWorkflowInputTest, DefaultValues) {
    SignalExternalWorkflowInput input;
    EXPECT_TRUE(input.workflow_id.empty());
    EXPECT_TRUE(input.run_id.empty());
    EXPECT_TRUE(input.signal.empty());
    EXPECT_TRUE(input.args.empty());
}

TEST(SignalExternalWorkflowInputTest, CustomValues) {
    SignalExternalWorkflowInput input{
        .workflow_id = "ext-wf-1",
        .run_id = "run-1",
        .signal = "notify",
        .args = {std::any(true)},
    };
    EXPECT_EQ(input.workflow_id, "ext-wf-1");
    EXPECT_EQ(input.run_id, "run-1");
    EXPECT_EQ(input.signal, "notify");
    ASSERT_EQ(input.args.size(), 1u);
}

TEST(CancelExternalWorkflowInputTest, DefaultValues) {
    CancelExternalWorkflowInput input;
    EXPECT_TRUE(input.workflow_id.empty());
    EXPECT_TRUE(input.run_id.empty());
}

TEST(ExecuteActivityInputTest, DefaultValues) {
    ExecuteActivityInput input;
    EXPECT_EQ(input.activity, nullptr);
    EXPECT_TRUE(input.args.empty());
    EXPECT_TRUE(input.headers.empty());
}

TEST(HeartbeatInputTest, DefaultValues) {
    HeartbeatInput input;
    EXPECT_TRUE(input.details.empty());
}

TEST(HeartbeatInputTest, WithDetails) {
    HeartbeatInput input{
        .details = {std::any(std::string("progress-50%"))},
    };
    ASSERT_EQ(input.details.size(), 1u);
    EXPECT_EQ(std::any_cast<std::string>(input.details[0]), "progress-50%");
}

// ===========================================================================
// WorkflowOutboundInterceptor tests
// ===========================================================================

TEST(WorkflowOutboundInterceptorTest, NextThrowsWhenNull) {
    class TestOutbound : public WorkflowOutboundInterceptor {
    public:
        TestOutbound() : WorkflowOutboundInterceptor() {}
        using WorkflowOutboundInterceptor::next;
    };

    TestOutbound interceptor;
    EXPECT_THROW(interceptor.next(), std::runtime_error);
}

TEST(WorkflowOutboundInterceptorTest, VirtualDestructorWorks) {
    class TestOutbound : public WorkflowOutboundInterceptor {
    public:
        TestOutbound() : WorkflowOutboundInterceptor() {}
    };

    std::unique_ptr<WorkflowOutboundInterceptor> ptr =
        std::make_unique<TestOutbound>();
    EXPECT_NO_THROW(ptr.reset());
}

// ===========================================================================
// ActivityOutboundInterceptor tests
// ===========================================================================

TEST(ActivityOutboundInterceptorTest, NextThrowsWhenNull) {
    class TestOutbound : public ActivityOutboundInterceptor {
    public:
        TestOutbound() : ActivityOutboundInterceptor() {}
        using ActivityOutboundInterceptor::next;
    };

    TestOutbound interceptor;
    EXPECT_THROW(interceptor.next(), std::runtime_error);
}

TEST(ActivityOutboundInterceptorTest, VirtualDestructorWorks) {
    class TestOutbound : public ActivityOutboundInterceptor {
    public:
        TestOutbound() : ActivityOutboundInterceptor() {}
    };

    std::unique_ptr<ActivityOutboundInterceptor> ptr =
        std::make_unique<TestOutbound>();
    EXPECT_NO_THROW(ptr.reset());
}

// ===========================================================================
// WorkflowInboundInterceptor tests
// ===========================================================================

TEST(WorkflowInboundInterceptorTest, NextThrowsWhenNull) {
    class TestInbound : public WorkflowInboundInterceptor {
    public:
        TestInbound() : WorkflowInboundInterceptor() {}
        using WorkflowInboundInterceptor::next;
    };

    TestInbound interceptor;
    EXPECT_THROW(interceptor.next(), std::runtime_error);
}

TEST(WorkflowInboundInterceptorTest, VirtualDestructorWorks) {
    class TestInbound : public WorkflowInboundInterceptor {
    public:
        TestInbound() : WorkflowInboundInterceptor() {}
    };

    std::unique_ptr<WorkflowInboundInterceptor> ptr =
        std::make_unique<TestInbound>();
    EXPECT_NO_THROW(ptr.reset());
}

// ===========================================================================
// ActivityInboundInterceptor tests
// ===========================================================================

TEST(ActivityInboundInterceptorTest, NextThrowsWhenNull) {
    class TestInbound : public ActivityInboundInterceptor {
    public:
        TestInbound() : ActivityInboundInterceptor() {}
        using ActivityInboundInterceptor::next;
    };

    TestInbound interceptor;
    EXPECT_THROW(interceptor.next(), std::runtime_error);
}

TEST(ActivityInboundInterceptorTest, VirtualDestructorWorks) {
    class TestInbound : public ActivityInboundInterceptor {
    public:
        TestInbound() : ActivityInboundInterceptor() {}
    };

    std::unique_ptr<ActivityInboundInterceptor> ptr =
        std::make_unique<TestInbound>();
    EXPECT_NO_THROW(ptr.reset());
}

// ===========================================================================
// IWorkerInterceptor tests
// ===========================================================================

TEST(IWorkerInterceptorTest, DefaultInterceptWorkflowReturnsNext) {
    class TestWorkerInterceptor : public IWorkerInterceptor {};

    class StubInbound : public WorkflowInboundInterceptor {
    public:
        StubInbound() : WorkflowInboundInterceptor() {}
    };

    TestWorkerInterceptor interceptor;
    StubInbound stub;
    auto* result = interceptor.intercept_workflow(&stub);
    EXPECT_EQ(result, &stub);
}

TEST(IWorkerInterceptorTest, DefaultInterceptActivityReturnsNext) {
    class TestWorkerInterceptor : public IWorkerInterceptor {};

    class StubInbound : public ActivityInboundInterceptor {
    public:
        StubInbound() : ActivityInboundInterceptor() {}
    };

    TestWorkerInterceptor interceptor;
    StubInbound stub;
    auto* result = interceptor.intercept_activity(&stub);
    EXPECT_EQ(result, &stub);
}

TEST(IWorkerInterceptorTest, CustomInterceptWorkflow) {
    class LoggingInbound : public WorkflowInboundInterceptor {
    public:
        explicit LoggingInbound(WorkflowInboundInterceptor* next)
            : WorkflowInboundInterceptor(next) {}
    };

    class LoggingInterceptor : public IWorkerInterceptor {
    public:
        WorkflowInboundInterceptor* intercept_workflow(
            WorkflowInboundInterceptor* next) override {
            wrapper = std::make_unique<LoggingInbound>(next);
            return wrapper.get();
        }

        std::unique_ptr<LoggingInbound> wrapper;
    };

    class StubInbound : public WorkflowInboundInterceptor {
    public:
        StubInbound() : WorkflowInboundInterceptor() {}
    };

    StubInbound stub;
    LoggingInterceptor interceptor;
    auto* result = interceptor.intercept_workflow(&stub);
    EXPECT_NE(result, &stub);
    EXPECT_EQ(result, interceptor.wrapper.get());
}

TEST(IWorkerInterceptorTest, VirtualDestructorWorks) {
    class TestWorkerInterceptor : public IWorkerInterceptor {};

    std::unique_ptr<IWorkerInterceptor> ptr =
        std::make_unique<TestWorkerInterceptor>();
    EXPECT_NO_THROW(ptr.reset());
}

// ===========================================================================
// Interceptor chain wiring test
// ===========================================================================

TEST(WorkflowInterceptorChainTest, ChainedInboundDelegatesToNext) {
    class RootInbound : public WorkflowInboundInterceptor {
    public:
        RootInbound() : WorkflowInboundInterceptor() {}

        std::any handle_query(HandleQueryInput /*input*/) override {
            return std::any(std::string("root-result"));
        }
    };

    class MiddleInbound : public WorkflowInboundInterceptor {
    public:
        explicit MiddleInbound(WorkflowInboundInterceptor* next)
            : WorkflowInboundInterceptor(next) {}

        std::any handle_query(HandleQueryInput input) override {
            auto result = next().handle_query(std::move(input));
            auto str = std::any_cast<std::string>(result);
            return std::any(std::string("middle-") + str);
        }
    };

    RootInbound root;
    MiddleInbound middle(&root);

    HandleQueryInput input{.query = "status"};
    auto result = middle.handle_query(std::move(input));
    EXPECT_EQ(std::any_cast<std::string>(result), "middle-root-result");
}

TEST(ActivityInterceptorChainTest, ChainedOutboundDelegatesToNext) {
    class RootOutbound : public ActivityOutboundInterceptor {
    public:
        RootOutbound() : ActivityOutboundInterceptor() {}
        int heartbeat_count = 0;

        void heartbeat(HeartbeatInput /*input*/) override { ++heartbeat_count; }
    };

    class MiddleOutbound : public ActivityOutboundInterceptor {
    public:
        explicit MiddleOutbound(ActivityOutboundInterceptor* next)
            : ActivityOutboundInterceptor(next) {}

        void heartbeat(HeartbeatInput input) override {
            next().heartbeat(std::move(input));
        }
    };

    RootOutbound root;
    MiddleOutbound middle(&root);

    HeartbeatInput input;
    middle.heartbeat(std::move(input));
    EXPECT_EQ(root.heartbeat_count, 1);
}
