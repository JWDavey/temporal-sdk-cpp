#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>

#include "temporalio/client/temporal_client.h"
#include "temporalio/client/workflow_handle.h"

using namespace temporalio::client;

// ===========================================================================
// WorkflowHandle construction tests
// ===========================================================================

// Note: WorkflowHandle requires a valid TemporalClient shared_ptr.
// Since we can't connect to a real server in unit tests, we test
// what we can about the handle structure.

TEST(WorkflowHandleTest, ConstructWithIdOnly) {
    // Create with nullptr client - just testing data storage
    WorkflowHandle handle(nullptr, "wf-123");
    EXPECT_EQ(handle.id(), "wf-123");
    EXPECT_FALSE(handle.run_id().has_value());
    EXPECT_FALSE(handle.first_execution_run_id().has_value());
}

TEST(WorkflowHandleTest, ConstructWithRunId) {
    WorkflowHandle handle(nullptr, "wf-123", "run-456");
    EXPECT_EQ(handle.id(), "wf-123");
    EXPECT_TRUE(handle.run_id().has_value());
    EXPECT_EQ(handle.run_id().value(), "run-456");
    EXPECT_FALSE(handle.first_execution_run_id().has_value());
}

TEST(WorkflowHandleTest, ConstructWithAllIds) {
    WorkflowHandle handle(nullptr, "wf-123", "run-456",
                          "first-run-789");
    EXPECT_EQ(handle.id(), "wf-123");
    EXPECT_TRUE(handle.run_id().has_value());
    EXPECT_EQ(handle.run_id().value(), "run-456");
    EXPECT_TRUE(handle.first_execution_run_id().has_value());
    EXPECT_EQ(handle.first_execution_run_id().value(), "first-run-789");
}

TEST(WorkflowHandleTest, EmptyWorkflowId) {
    WorkflowHandle handle(nullptr, "");
    EXPECT_TRUE(handle.id().empty());
}

TEST(WorkflowHandleTest, NulloptRunId) {
    WorkflowHandle handle(nullptr, "wf-1", std::nullopt);
    EXPECT_FALSE(handle.run_id().has_value());
}

TEST(WorkflowHandleTest, EmptyStringRunId) {
    WorkflowHandle handle(nullptr, "wf-1", std::string(""));
    EXPECT_TRUE(handle.run_id().has_value());
    EXPECT_TRUE(handle.run_id().value().empty());
}

// ===========================================================================
// WorkflowHandle copy/move semantics
// ===========================================================================

TEST(WorkflowHandleTest, CopyConstruction) {
    WorkflowHandle original(nullptr, "wf-copy", "run-copy");
    WorkflowHandle copy(original);
    EXPECT_EQ(copy.id(), "wf-copy");
    EXPECT_EQ(copy.run_id().value(), "run-copy");
}

TEST(WorkflowHandleTest, MoveConstruction) {
    WorkflowHandle original(nullptr, "wf-move", "run-move");
    WorkflowHandle moved(std::move(original));
    EXPECT_EQ(moved.id(), "wf-move");
    EXPECT_EQ(moved.run_id().value(), "run-move");
}

TEST(WorkflowHandleTest, CopyAssignment) {
    WorkflowHandle a(nullptr, "wf-a", "run-a");
    WorkflowHandle b(nullptr, "wf-b");
    b = a;
    EXPECT_EQ(b.id(), "wf-a");
    EXPECT_EQ(b.run_id().value(), "run-a");
}

TEST(WorkflowHandleTest, MoveAssignment) {
    WorkflowHandle a(nullptr, "wf-a", "run-a");
    WorkflowHandle b(nullptr, "wf-b");
    b = std::move(a);
    EXPECT_EQ(b.id(), "wf-a");
    EXPECT_EQ(b.run_id().value(), "run-a");
}

// ===========================================================================
// WorkflowExecution (already tested in client_options_tests.cpp but
// adding more thorough tests here)
// ===========================================================================

TEST(WorkflowExecutionTest, Equality) {
    WorkflowExecution a{.workflow_id = "wf-1", .run_id = "run-1"};
    WorkflowExecution b{.workflow_id = "wf-1", .run_id = "run-1"};
    WorkflowExecution c{.workflow_id = "wf-2", .run_id = "run-1"};

    EXPECT_EQ(a.workflow_id, b.workflow_id);
    EXPECT_EQ(a.run_id, b.run_id);
    EXPECT_NE(a.workflow_id, c.workflow_id);
}

TEST(WorkflowExecutionTest, WithWorkflowType) {
    WorkflowExecution exec{
        .workflow_id = "wf-1",
        .run_id = "run-1",
        .workflow_type = "MyWorkflow",
    };
    EXPECT_TRUE(exec.workflow_type.has_value());
    EXPECT_EQ(exec.workflow_type.value(), "MyWorkflow");
}

TEST(WorkflowExecutionTest, WithoutWorkflowType) {
    WorkflowExecution exec{
        .workflow_id = "wf-1",
        .run_id = "run-1",
    };
    EXPECT_FALSE(exec.workflow_type.has_value());
}
