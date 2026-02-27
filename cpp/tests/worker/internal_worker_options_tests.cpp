#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "temporalio/worker/internal/activity_worker.h"
#include "temporalio/worker/internal/nexus_worker.h"
#include "temporalio/worker/internal/workflow_worker.h"

using namespace temporalio::worker::internal;

// ===========================================================================
// ActivityWorkerOptions tests
// ===========================================================================

TEST(ActivityWorkerOptionsTest, DefaultValues) {
    ActivityWorkerOptions opts;
    EXPECT_TRUE(opts.task_queue.empty());
    EXPECT_TRUE(opts.ns.empty());
    EXPECT_TRUE(opts.activities.empty());
    EXPECT_EQ(opts.dynamic_activity, nullptr);
    EXPECT_EQ(opts.data_converter, nullptr);
    EXPECT_TRUE(opts.interceptors.empty());
    EXPECT_EQ(opts.max_concurrent, 100u);
}

TEST(ActivityWorkerOptionsTest, CustomValues) {
    ActivityWorkerOptions opts{
        .task_queue = "my-queue",
        .ns = "production",
        .max_concurrent = 50,
    };
    EXPECT_EQ(opts.task_queue, "my-queue");
    EXPECT_EQ(opts.ns, "production");
    EXPECT_EQ(opts.max_concurrent, 50u);
}

// ===========================================================================
// ActivityWorker type traits
// ===========================================================================

TEST(ActivityWorkerTest, IsNonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<ActivityWorker>);
    EXPECT_FALSE(std::is_copy_assignable_v<ActivityWorker>);
}

// ===========================================================================
// WorkflowWorkerOptions tests
// ===========================================================================

TEST(WorkflowWorkerOptionsTest, DefaultValues) {
    WorkflowWorkerOptions opts;
    EXPECT_TRUE(opts.task_queue.empty());
    EXPECT_TRUE(opts.ns.empty());
    EXPECT_TRUE(opts.workflows.empty());
    EXPECT_EQ(opts.dynamic_workflow, nullptr);
    EXPECT_EQ(opts.data_converter, nullptr);
    EXPECT_TRUE(opts.interceptors.empty());
    EXPECT_FALSE(opts.debug_mode);
}

TEST(WorkflowWorkerOptionsTest, CustomValues) {
    WorkflowWorkerOptions opts{
        .task_queue = "wf-queue",
        .ns = "staging",
        .debug_mode = true,
    };
    EXPECT_EQ(opts.task_queue, "wf-queue");
    EXPECT_EQ(opts.ns, "staging");
    EXPECT_TRUE(opts.debug_mode);
}

// ===========================================================================
// WorkflowWorker type traits
// ===========================================================================

TEST(WorkflowWorkerTest, IsNonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<WorkflowWorker>);
    EXPECT_FALSE(std::is_copy_assignable_v<WorkflowWorker>);
}

// ===========================================================================
// NexusWorkerOptions tests
// ===========================================================================

TEST(NexusWorkerOptionsTest, DefaultValues) {
    NexusWorkerOptions opts;
    EXPECT_TRUE(opts.task_queue.empty());
    EXPECT_TRUE(opts.ns.empty());
}

TEST(NexusWorkerOptionsTest, CustomValues) {
    NexusWorkerOptions opts{
        .task_queue = "nexus-queue",
        .ns = "test-ns",
    };
    EXPECT_EQ(opts.task_queue, "nexus-queue");
    EXPECT_EQ(opts.ns, "test-ns");
}

// ===========================================================================
// NexusWorker type traits
// ===========================================================================

TEST(NexusWorkerTest, IsNonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<NexusWorker>);
    EXPECT_FALSE(std::is_copy_assignable_v<NexusWorker>);
}
