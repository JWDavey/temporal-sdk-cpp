#include <gtest/gtest.h>

// Global test environment setup.
// When the bridge layer is fully wired up, this will register a
// WorkflowEnvironmentFixture that starts/stops a local dev server.
//
// For now, tests can check the TEMPORAL_TEST_CLIENT_TARGET_HOST env var
// to determine if an external server is available.

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
