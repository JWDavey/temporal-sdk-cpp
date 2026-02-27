#include <gtest/gtest.h>

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "temporalio/async_/task.h"
#include "temporalio/worker/workflow_replayer.h"
#include "temporalio/workflows/workflow_definition.h"

using namespace temporalio::worker;
using namespace temporalio::workflows;
using namespace temporalio::async_;

// ===========================================================================
// Sample workflow for testing
// ===========================================================================
namespace {

class SayHelloWorkflow {
public:
    Task<std::string> run(std::string name) {
        co_return "Hello, " + name + "!";
    }
};

}  // namespace

// ===========================================================================
// WorkflowReplayerOptions tests
// ===========================================================================

TEST(WorkflowReplayerOptionsTest, DefaultValues) {
    WorkflowReplayerOptions opts;
    EXPECT_TRUE(opts.workflows.empty());
    EXPECT_EQ(opts.ns, "ReplayNamespace");
    EXPECT_EQ(opts.task_queue, "ReplayTaskQueue");
    EXPECT_EQ(opts.data_converter, nullptr);
    EXPECT_TRUE(opts.interceptors.empty());
    EXPECT_FALSE(opts.debug_mode);
}

TEST(WorkflowReplayerOptionsTest, CustomNamespace) {
    WorkflowReplayerOptions opts{.ns = "MyReplayNS"};
    EXPECT_EQ(opts.ns, "MyReplayNS");
}

TEST(WorkflowReplayerOptionsTest, CustomTaskQueue) {
    WorkflowReplayerOptions opts{.task_queue = "MyReplayQueue"};
    EXPECT_EQ(opts.task_queue, "MyReplayQueue");
}

TEST(WorkflowReplayerOptionsTest, DebugMode) {
    WorkflowReplayerOptions opts{.debug_mode = true};
    EXPECT_TRUE(opts.debug_mode);
}

TEST(WorkflowReplayerOptionsTest, WithWorkflow) {
    auto def = WorkflowDefinition::create<SayHelloWorkflow>("SayHelloWorkflow")
                   .run(&SayHelloWorkflow::run)
                   .build();

    WorkflowReplayerOptions opts;
    opts.workflows.push_back(def);

    EXPECT_EQ(opts.workflows.size(), 1u);
    EXPECT_EQ(opts.workflows[0]->name(), "SayHelloWorkflow");
}

// ===========================================================================
// WorkflowReplayResult tests
// ===========================================================================

TEST(WorkflowReplayResultTest, NoFailure) {
    WorkflowReplayResult result;
    result.replay_failure = nullptr;
    EXPECT_FALSE(result.has_failure());
}

TEST(WorkflowReplayResultTest, WithFailure) {
    WorkflowReplayResult result;
    result.replay_failure =
        std::make_exception_ptr(std::runtime_error("nondeterminism"));
    EXPECT_TRUE(result.has_failure());

    try {
        std::rethrow_exception(result.replay_failure);
    } catch (const std::runtime_error& e) {
        EXPECT_EQ(std::string(e.what()), "nondeterminism");
    }
}

TEST(WorkflowReplayResultTest, HistoryPreserved) {
    WorkflowReplayResult result;
    result.history.id = "wf-123";
    result.history.events.push_back(
        WorkflowHistoryEvent{.serialized_data = "event-1"});

    EXPECT_EQ(result.history.id, "wf-123");
    EXPECT_EQ(result.history.events.size(), 1u);
    EXPECT_EQ(result.history.events[0].serialized_data, "event-1");
}

// ===========================================================================
// WorkflowHistoryEvent tests
// ===========================================================================

TEST(WorkflowHistoryEventTest, DefaultEmpty) {
    WorkflowHistoryEvent event;
    EXPECT_TRUE(event.serialized_data.empty());
}

TEST(WorkflowHistoryEventTest, WithData) {
    WorkflowHistoryEvent event{.serialized_data = "proto-bytes"};
    EXPECT_EQ(event.serialized_data, "proto-bytes");
}

// ===========================================================================
// WorkflowHistory (worker namespace) tests
// ===========================================================================

TEST(WorkerWorkflowHistoryTest, DefaultValues) {
    WorkflowHistory history;
    EXPECT_TRUE(history.id.empty());
    EXPECT_TRUE(history.events.empty());
}

TEST(WorkerWorkflowHistoryTest, WithIdAndEvents) {
    WorkflowHistory history{
        .id = "wf-abc",
        .events =
            {
                WorkflowHistoryEvent{.serialized_data = "e1"},
                WorkflowHistoryEvent{.serialized_data = "e2"},
            },
    };
    EXPECT_EQ(history.id, "wf-abc");
    EXPECT_EQ(history.events.size(), 2u);
}

// ===========================================================================
// WorkflowReplayer type traits
// ===========================================================================

TEST(WorkflowReplayerTest, IsNonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<WorkflowReplayer>);
    EXPECT_FALSE(std::is_copy_assignable_v<WorkflowReplayer>);
}

TEST(WorkflowReplayerTest, IsNonMovable) {
    EXPECT_FALSE(std::is_move_constructible_v<WorkflowReplayer>);
    EXPECT_FALSE(std::is_move_assignable_v<WorkflowReplayer>);
}

// ===========================================================================
// WorkflowReplayer construction tests
// ===========================================================================

TEST(WorkflowReplayerTest, ConstructWithWorkflow) {
    auto def = WorkflowDefinition::create<SayHelloWorkflow>("SayHelloWorkflow")
                   .run(&SayHelloWorkflow::run)
                   .build();

    WorkflowReplayerOptions opts;
    opts.workflows.push_back(def);

    WorkflowReplayer replayer(std::move(opts));
    EXPECT_EQ(replayer.options().workflows.size(), 1u);
    EXPECT_EQ(replayer.options().ns, "ReplayNamespace");
    EXPECT_EQ(replayer.options().task_queue, "ReplayTaskQueue");
}

TEST(WorkflowReplayerTest, ConstructWithMultipleWorkflows) {
    auto def1 =
        WorkflowDefinition::create<SayHelloWorkflow>("Workflow1")
            .run(&SayHelloWorkflow::run)
            .build();
    auto def2 =
        WorkflowDefinition::create<SayHelloWorkflow>("Workflow2")
            .run(&SayHelloWorkflow::run)
            .build();

    WorkflowReplayerOptions opts;
    opts.workflows.push_back(def1);
    opts.workflows.push_back(def2);

    WorkflowReplayer replayer(std::move(opts));
    EXPECT_EQ(replayer.options().workflows.size(), 2u);
}

TEST(WorkflowReplayerTest, ConstructWithEmptyWorkflowsThrows) {
    WorkflowReplayerOptions opts;
    EXPECT_THROW(WorkflowReplayer replayer(std::move(opts)),
                 std::invalid_argument);
}

TEST(WorkflowReplayerTest, ConstructPreservesOptions) {
    auto def = WorkflowDefinition::create<SayHelloWorkflow>("SayHelloWorkflow")
                   .run(&SayHelloWorkflow::run)
                   .build();

    WorkflowReplayerOptions opts;
    opts.workflows.push_back(def);
    opts.ns = "custom-ns";
    opts.task_queue = "custom-queue";
    opts.debug_mode = true;

    WorkflowReplayer replayer(std::move(opts));
    EXPECT_EQ(replayer.options().ns, "custom-ns");
    EXPECT_EQ(replayer.options().task_queue, "custom-queue");
    EXPECT_TRUE(replayer.options().debug_mode);
}
