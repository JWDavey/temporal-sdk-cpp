#pragma once

/// @file workflow_environment_fixture.h
/// @brief Google Test global fixture for managing a local Temporal dev server.

#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace temporalio::testing {

/// Google Test global fixture. Starts a local Temporal dev server
/// (or connects to an external one if TEMPORAL_TEST_CLIENT_TARGET_HOST is set).
class WorkflowEnvironmentFixture : public ::testing::Environment {
public:
    void SetUp() override;
    void TearDown() override;
};

/// Base class for tests needing a live Temporal server.
class WorkflowEnvironmentTestBase : public ::testing::Test {
protected:
    /// Returns a unique task queue name per test.
    std::string unique_task_queue();
};

} // namespace temporalio::testing
