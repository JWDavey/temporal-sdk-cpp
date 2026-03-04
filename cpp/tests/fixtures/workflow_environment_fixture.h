#pragma once

/// @file workflow_environment_fixture.h
/// @brief Google Test global fixture for managing a local Temporal dev server.
///
/// Starts a local Temporal dev server once for the entire test run
/// (or connects to an external one if TEMPORAL_TEST_CLIENT_TARGET_HOST is set).
/// Tests that need a live server should extend WorkflowEnvironmentTestBase.

#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <string>

namespace temporalio::testing {

class WorkflowEnvironment;

} // namespace temporalio::testing

namespace temporalio::client {

class TemporalClient;

} // namespace temporalio::client

namespace temporalio::testing {

/// Google Test global environment fixture. Registered in main.cpp via
/// AddGlobalTestEnvironment. Starts a local Temporal dev server on SetUp
/// (or connects to an external one if TEMPORAL_TEST_CLIENT_TARGET_HOST is set)
/// and shuts it down on TearDown.
///
/// The singleton environment/client are shared across all test cases.
class WorkflowEnvironmentFixture : public ::testing::Environment {
public:
    void SetUp() override;
    void TearDown() override;

    /// Get the shared workflow environment (available after SetUp).
    static WorkflowEnvironment* environment();

    /// Get the shared client (available after SetUp).
    static std::shared_ptr<client::TemporalClient> client();

    /// Whether the fixture successfully initialized.
    static bool is_available();

private:
    static std::unique_ptr<WorkflowEnvironment> env_;
    static std::shared_ptr<client::TemporalClient> client_;
    static std::atomic<bool> available_;
};

/// Base class for tests needing a live Temporal server.
///
/// Tests deriving from this class can call client() to get a connected
/// TemporalClient and env() to get the WorkflowEnvironment.
/// If the environment is not available (e.g., no server and no bridge),
/// tests are skipped.
class WorkflowEnvironmentTestBase : public ::testing::Test {
protected:
    void SetUp() override;

    /// Returns the shared workflow environment.
    WorkflowEnvironment* env();

    /// Returns the shared client.
    std::shared_ptr<client::TemporalClient> client();

    /// Returns a unique task queue name per test.
    std::string unique_task_queue();

private:
    static std::atomic<uint64_t> task_queue_counter_;
};

} // namespace temporalio::testing
