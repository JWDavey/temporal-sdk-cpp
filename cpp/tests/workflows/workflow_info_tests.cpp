#include <gtest/gtest.h>

#include <chrono>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>

#include "temporalio/workflows/workflow_info.h"

using namespace temporalio::workflows;
using namespace std::chrono_literals;

// ===========================================================================
// ParentWorkflowInfo tests
// ===========================================================================

TEST(ParentWorkflowInfoTest, DefaultValues) {
    ParentWorkflowInfo info;
    EXPECT_TRUE(info.namespace_.empty());
    EXPECT_TRUE(info.run_id.empty());
    EXPECT_TRUE(info.workflow_id.empty());
}

TEST(ParentWorkflowInfoTest, CustomValues) {
    ParentWorkflowInfo info{
        .namespace_ = "prod",
        .run_id = "parent-run-1",
        .workflow_id = "parent-wf-1",
    };
    EXPECT_EQ(info.namespace_, "prod");
    EXPECT_EQ(info.run_id, "parent-run-1");
    EXPECT_EQ(info.workflow_id, "parent-wf-1");
}

TEST(ParentWorkflowInfoTest, EqualityOperator) {
    ParentWorkflowInfo a{
        .namespace_ = "ns1",
        .run_id = "run-1",
        .workflow_id = "wf-1",
    };
    ParentWorkflowInfo b{
        .namespace_ = "ns1",
        .run_id = "run-1",
        .workflow_id = "wf-1",
    };
    ParentWorkflowInfo c{
        .namespace_ = "ns2",
        .run_id = "run-1",
        .workflow_id = "wf-1",
    };
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

// ===========================================================================
// RootWorkflowInfo tests
// ===========================================================================

TEST(RootWorkflowInfoTest, DefaultValues) {
    RootWorkflowInfo info;
    EXPECT_TRUE(info.run_id.empty());
    EXPECT_TRUE(info.workflow_id.empty());
}

TEST(RootWorkflowInfoTest, CustomValues) {
    RootWorkflowInfo info{
        .run_id = "root-run-1",
        .workflow_id = "root-wf-1",
    };
    EXPECT_EQ(info.run_id, "root-run-1");
    EXPECT_EQ(info.workflow_id, "root-wf-1");
}

TEST(RootWorkflowInfoTest, EqualityOperator) {
    RootWorkflowInfo a{.run_id = "r1", .workflow_id = "w1"};
    RootWorkflowInfo b{.run_id = "r1", .workflow_id = "w1"};
    RootWorkflowInfo c{.run_id = "r2", .workflow_id = "w1"};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

// ===========================================================================
// WorkflowInfo tests
// ===========================================================================

TEST(WorkflowInfoTest, DefaultValues) {
    WorkflowInfo info;
    EXPECT_EQ(info.attempt, 1);
    EXPECT_FALSE(info.continued_run_id.has_value());
    EXPECT_FALSE(info.cron_schedule.has_value());
    EXPECT_FALSE(info.execution_timeout.has_value());
    EXPECT_TRUE(info.first_execution_run_id.empty());
    EXPECT_EQ(info.last_failure, nullptr);
    EXPECT_TRUE(info.namespace_.empty());
    EXPECT_FALSE(info.parent.has_value());
    EXPECT_FALSE(info.root.has_value());
    EXPECT_TRUE(info.run_id.empty());
    EXPECT_FALSE(info.run_timeout.has_value());
    EXPECT_TRUE(info.task_queue.empty());
    EXPECT_TRUE(info.workflow_id.empty());
    EXPECT_TRUE(info.workflow_type.empty());
}

TEST(WorkflowInfoTest, CustomValues) {
    WorkflowInfo info;
    info.attempt = 3;
    info.continued_run_id = "prev-run-123";
    info.cron_schedule = "0 * * * *";
    info.execution_timeout = 60000ms;
    info.first_execution_run_id = "first-run-1";
    info.namespace_ = "production";
    info.parent = ParentWorkflowInfo{
        .namespace_ = "production",
        .run_id = "parent-run",
        .workflow_id = "parent-wf",
    };
    info.root = RootWorkflowInfo{
        .run_id = "root-run",
        .workflow_id = "root-wf",
    };
    info.run_id = "run-abc";
    info.run_timeout = 30000ms;
    info.task_queue = "my-queue";
    info.task_timeout = 10000ms;
    info.workflow_id = "wf-xyz";
    info.workflow_type = "MyWorkflow";

    EXPECT_EQ(info.attempt, 3);
    EXPECT_EQ(info.continued_run_id.value(), "prev-run-123");
    EXPECT_EQ(info.cron_schedule.value(), "0 * * * *");
    EXPECT_EQ(info.execution_timeout.value(), 60000ms);
    EXPECT_EQ(info.first_execution_run_id, "first-run-1");
    EXPECT_TRUE(info.parent.has_value());
    EXPECT_EQ(info.parent->workflow_id, "parent-wf");
    EXPECT_TRUE(info.root.has_value());
    EXPECT_EQ(info.root->workflow_id, "root-wf");
    EXPECT_EQ(info.run_id, "run-abc");
    EXPECT_EQ(info.run_timeout.value(), 30000ms);
    EXPECT_EQ(info.task_queue, "my-queue");
    EXPECT_EQ(info.task_timeout, 10000ms);
    EXPECT_EQ(info.workflow_id, "wf-xyz");
    EXPECT_EQ(info.workflow_type, "MyWorkflow");
}

TEST(WorkflowInfoTest, WithLastFailure) {
    WorkflowInfo info;
    try {
        throw std::runtime_error("previous run failed");
    } catch (...) {
        info.last_failure = std::current_exception();
    }

    EXPECT_NE(info.last_failure, nullptr);
    try {
        std::rethrow_exception(info.last_failure);
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "previous run failed");
    }
}

TEST(WorkflowInfoTest, WithNoParent) {
    WorkflowInfo info;
    info.workflow_id = "standalone-wf";
    EXPECT_FALSE(info.parent.has_value());
}

TEST(WorkflowInfoTest, IsChildWorkflow) {
    WorkflowInfo info;
    info.parent = ParentWorkflowInfo{
        .namespace_ = "ns",
        .run_id = "parent-run",
        .workflow_id = "parent-wf",
    };
    EXPECT_TRUE(info.parent.has_value());
    EXPECT_EQ(info.parent->workflow_id, "parent-wf");
}

// ===========================================================================
// WorkflowUpdateInfo tests
// ===========================================================================

TEST(WorkflowUpdateInfoTest, DefaultValues) {
    WorkflowUpdateInfo info;
    EXPECT_TRUE(info.id.empty());
    EXPECT_TRUE(info.name.empty());
}

TEST(WorkflowUpdateInfoTest, CustomValues) {
    WorkflowUpdateInfo info{
        .id = "update-123",
        .name = "set_value",
    };
    EXPECT_EQ(info.id, "update-123");
    EXPECT_EQ(info.name, "set_value");
}
