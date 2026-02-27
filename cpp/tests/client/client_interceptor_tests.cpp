#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "temporalio/client/interceptors/client_interceptor.h"
#include "temporalio/client/workflow_options.h"

using namespace temporalio::client;
using namespace temporalio::client::interceptors;

// ===========================================================================
// Interceptor input struct tests
// ===========================================================================

TEST(StartWorkflowInputTest, DefaultValues) {
    StartWorkflowInput input;
    EXPECT_TRUE(input.workflow.empty());
    EXPECT_TRUE(input.args.empty());
    EXPECT_TRUE(input.headers.empty());
}

TEST(StartWorkflowInputTest, CustomValues) {
    StartWorkflowInput input{
        .workflow = "MyWorkflow",
        .args = {"arg1", "arg2"},
        .options = WorkflowOptions{.id = "wf-1", .task_queue = "q"},
        .headers = {{"auth", "token"}},
    };
    EXPECT_EQ(input.workflow, "MyWorkflow");
    ASSERT_EQ(input.args.size(), 2u);
    EXPECT_EQ(input.args[0], "arg1");
    EXPECT_EQ(input.options.id, "wf-1");
    EXPECT_EQ(input.headers.at("auth"), "token");
}

TEST(SignalWorkflowInputTest, DefaultValues) {
    SignalWorkflowInput input;
    EXPECT_TRUE(input.id.empty());
    EXPECT_FALSE(input.run_id.has_value());
    EXPECT_TRUE(input.signal.empty());
    EXPECT_TRUE(input.args.empty());
    EXPECT_FALSE(input.options.has_value());
    EXPECT_TRUE(input.headers.empty());
}

TEST(SignalWorkflowInputTest, CustomValues) {
    SignalWorkflowInput input{
        .id = "wf-123",
        .run_id = "run-456",
        .signal = "my-signal",
        .args = {"data"},
    };
    EXPECT_EQ(input.id, "wf-123");
    EXPECT_EQ(input.run_id.value(), "run-456");
    EXPECT_EQ(input.signal, "my-signal");
    ASSERT_EQ(input.args.size(), 1u);
}

TEST(QueryWorkflowInputTest, DefaultValues) {
    QueryWorkflowInput input;
    EXPECT_TRUE(input.id.empty());
    EXPECT_FALSE(input.run_id.has_value());
    EXPECT_TRUE(input.query.empty());
    EXPECT_TRUE(input.args.empty());
    EXPECT_FALSE(input.options.has_value());
    EXPECT_TRUE(input.headers.empty());
}

TEST(QueryWorkflowInputTest, CustomValues) {
    QueryWorkflowInput input{
        .id = "wf-abc",
        .query = "get_status",
        .args = {"param1"},
    };
    EXPECT_EQ(input.id, "wf-abc");
    EXPECT_EQ(input.query, "get_status");
    ASSERT_EQ(input.args.size(), 1u);
}

TEST(StartWorkflowUpdateInputTest, DefaultValues) {
    StartWorkflowUpdateInput input;
    EXPECT_TRUE(input.id.empty());
    EXPECT_FALSE(input.run_id.has_value());
    EXPECT_FALSE(input.first_execution_run_id.has_value());
    EXPECT_TRUE(input.update.empty());
    EXPECT_TRUE(input.args.empty());
    EXPECT_TRUE(input.headers.empty());
}

TEST(StartWorkflowUpdateInputTest, CustomValues) {
    StartWorkflowUpdateInput input{
        .id = "wf-upd",
        .run_id = "run-1",
        .first_execution_run_id = "first-run",
        .update = "set_value",
        .args = {"new_val"},
        .headers = {{"trace", "abc"}},
    };
    EXPECT_EQ(input.id, "wf-upd");
    EXPECT_EQ(input.run_id.value(), "run-1");
    EXPECT_EQ(input.first_execution_run_id.value(), "first-run");
    EXPECT_EQ(input.update, "set_value");
    ASSERT_EQ(input.args.size(), 1u);
    EXPECT_EQ(input.headers.at("trace"), "abc");
}

TEST(DescribeWorkflowInputTest, DefaultValues) {
    DescribeWorkflowInput input;
    EXPECT_TRUE(input.id.empty());
    EXPECT_FALSE(input.run_id.has_value());
    EXPECT_FALSE(input.options.has_value());
}

TEST(CancelWorkflowInputTest, DefaultValues) {
    CancelWorkflowInput input;
    EXPECT_TRUE(input.id.empty());
    EXPECT_FALSE(input.run_id.has_value());
    EXPECT_FALSE(input.first_execution_run_id.has_value());
    EXPECT_FALSE(input.options.has_value());
}

TEST(TerminateWorkflowInputTest, DefaultValues) {
    TerminateWorkflowInput input;
    EXPECT_TRUE(input.id.empty());
    EXPECT_FALSE(input.run_id.has_value());
    EXPECT_FALSE(input.first_execution_run_id.has_value());
    EXPECT_FALSE(input.reason.has_value());
    EXPECT_FALSE(input.options.has_value());
}

TEST(TerminateWorkflowInputTest, WithReason) {
    TerminateWorkflowInput input{
        .id = "wf-term",
        .reason = "manual shutdown",
    };
    EXPECT_EQ(input.id, "wf-term");
    EXPECT_EQ(input.reason.value(), "manual shutdown");
}

TEST(FetchWorkflowHistoryEventPageInputTest, DefaultValues) {
    FetchWorkflowHistoryEventPageInput input;
    EXPECT_TRUE(input.id.empty());
    EXPECT_FALSE(input.run_id.has_value());
    EXPECT_FALSE(input.page_size.has_value());
    EXPECT_FALSE(input.next_page_token.has_value());
    EXPECT_FALSE(input.wait_new_event);
    EXPECT_FALSE(input.skip_archival);
}

TEST(FetchWorkflowHistoryEventPageInputTest, CustomValues) {
    FetchWorkflowHistoryEventPageInput input{
        .id = "wf-hist",
        .page_size = 100,
        .wait_new_event = true,
        .skip_archival = true,
    };
    EXPECT_EQ(input.id, "wf-hist");
    EXPECT_EQ(input.page_size.value(), 100);
    EXPECT_TRUE(input.wait_new_event);
    EXPECT_TRUE(input.skip_archival);
}

TEST(ListWorkflowsInputTest, DefaultValues) {
    ListWorkflowsInput input;
    EXPECT_TRUE(input.query.empty());
    EXPECT_FALSE(input.options.has_value());
}

TEST(CountWorkflowsInputTest, DefaultValues) {
    CountWorkflowsInput input;
    EXPECT_TRUE(input.query.empty());
    EXPECT_FALSE(input.options.has_value());
}

TEST(StartUpdateWithStartWorkflowInputTest, DefaultValues) {
    StartUpdateWithStartWorkflowInput input;
    EXPECT_TRUE(input.update.empty());
    EXPECT_TRUE(input.args.empty());
    EXPECT_TRUE(input.headers.empty());
}

// ===========================================================================
// WorkflowHistoryEventPage tests
// ===========================================================================

TEST(WorkflowHistoryEventPageTest, DefaultValues) {
    WorkflowHistoryEventPage page;
    EXPECT_TRUE(page.events.empty());
    EXPECT_TRUE(page.next_page_token.empty());
}

TEST(WorkflowHistoryEventPageTest, WithEvents) {
    WorkflowHistoryEventPage page{
        .events = {"event1", "event2", "event3"},
        .next_page_token = {0x01, 0x02, 0x03},
    };
    ASSERT_EQ(page.events.size(), 3u);
    EXPECT_EQ(page.events[0], "event1");
    ASSERT_EQ(page.next_page_token.size(), 3u);
    EXPECT_EQ(page.next_page_token[0], 0x01);
}

// ===========================================================================
// IClientInterceptor interface tests
// ===========================================================================

namespace {

// A test interceptor that records calls
class TestClientOutboundInterceptor : public ClientOutboundInterceptor {
public:
    explicit TestClientOutboundInterceptor(
        std::unique_ptr<ClientOutboundInterceptor> next)
        : ClientOutboundInterceptor(std::move(next)) {}

    bool signal_called = false;
};

class TestClientInterceptor : public IClientInterceptor {
public:
    std::unique_ptr<ClientOutboundInterceptor> intercept_client(
        std::unique_ptr<ClientOutboundInterceptor> next) override {
        auto interceptor =
            std::make_unique<TestClientOutboundInterceptor>(std::move(next));
        return interceptor;
    }
};

}  // namespace

TEST(IClientInterceptorTest, FactoryCreatesInterceptor) {
    TestClientInterceptor factory;
    // The factory should accept a nullptr for the "next" chain terminal
    auto interceptor = factory.intercept_client(nullptr);
    EXPECT_NE(interceptor, nullptr);
}

TEST(IClientInterceptorTest, VirtualDestructorWorks) {
    std::unique_ptr<IClientInterceptor> ptr =
        std::make_unique<TestClientInterceptor>();
    // Should not crash
    ptr.reset();
}

// ===========================================================================
// ClientOutboundInterceptor type traits
// ===========================================================================

TEST(ClientOutboundInterceptorTest, IsNonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<ClientOutboundInterceptor>);
    EXPECT_FALSE(std::is_copy_assignable_v<ClientOutboundInterceptor>);
}

TEST(ClientOutboundInterceptorTest, IsMovable) {
    EXPECT_TRUE(std::is_move_constructible_v<ClientOutboundInterceptor>);
    EXPECT_TRUE(std::is_move_assignable_v<ClientOutboundInterceptor>);
}
