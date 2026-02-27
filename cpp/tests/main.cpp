#include <gtest/gtest.h>

#include "fixtures/workflow_environment_fixture.h"

// Register the global WorkflowEnvironmentFixture so that a local Temporal
// dev server is started once before all tests run (and shut down after).
//
// To connect to an external server instead, set the environment variables:
//   TEMPORAL_TEST_CLIENT_TARGET_HOST=host:port
//   TEMPORAL_TEST_CLIENT_NAMESPACE=my-namespace
//
// Tests extending WorkflowEnvironmentTestBase will be automatically skipped
// if the environment fails to initialize (e.g., bridge not available).

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(
        new temporalio::testing::WorkflowEnvironmentFixture());
    return RUN_ALL_TESTS();
}
