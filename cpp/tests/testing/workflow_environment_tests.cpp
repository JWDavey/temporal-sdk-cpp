#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "temporalio/testing/workflow_environment.h"

using namespace temporalio::testing;
using namespace temporalio::client;
using namespace std::chrono_literals;

// ===========================================================================
// WorkflowEnvironmentStartLocalOptions tests
// ===========================================================================

TEST(WorkflowEnvironmentStartLocalOptionsTest, DefaultValues) {
    WorkflowEnvironmentStartLocalOptions opts;
    EXPECT_TRUE(opts.connection.target_host.empty());
    EXPECT_EQ(opts.client.ns, "default");
    EXPECT_FALSE(opts.download_directory.has_value());
    EXPECT_FALSE(opts.ui);
    EXPECT_EQ(opts.ui_port, 0);
    EXPECT_EQ(opts.runtime, nullptr);
    EXPECT_TRUE(opts.extra_args.empty());
}

TEST(WorkflowEnvironmentStartLocalOptionsTest, CustomValues) {
    WorkflowEnvironmentStartLocalOptions opts{
        .connection = {.target_host = "localhost:7233"},
        .client = {.ns = "test-ns"},
        .download_directory = "/tmp/temporal",
        .ui = true,
        .ui_port = 8080,
        .extra_args = {"--log-level", "debug"},
    };
    EXPECT_EQ(opts.connection.target_host, "localhost:7233");
    EXPECT_EQ(opts.client.ns, "test-ns");
    EXPECT_EQ(opts.download_directory.value(), "/tmp/temporal");
    EXPECT_TRUE(opts.ui);
    EXPECT_EQ(opts.ui_port, 8080);
    ASSERT_EQ(opts.extra_args.size(), 2u);
    EXPECT_EQ(opts.extra_args[0], "--log-level");
}

// ===========================================================================
// WorkflowEnvironmentStartTimeSkippingOptions tests
// ===========================================================================

TEST(WorkflowEnvironmentStartTimeSkippingOptionsTest, DefaultValues) {
    WorkflowEnvironmentStartTimeSkippingOptions opts;
    EXPECT_TRUE(opts.connection.target_host.empty());
    EXPECT_EQ(opts.client.ns, "default");
    EXPECT_FALSE(opts.download_directory.has_value());
    EXPECT_EQ(opts.runtime, nullptr);
    EXPECT_TRUE(opts.extra_args.empty());
}

TEST(WorkflowEnvironmentStartTimeSkippingOptionsTest, CustomValues) {
    WorkflowEnvironmentStartTimeSkippingOptions opts{
        .connection = {.target_host = "localhost:7234"},
        .client = {.ns = "time-skip-ns"},
        .download_directory = "/tmp/temporal-ts",
        .extra_args = {"--port", "7234"},
    };
    EXPECT_EQ(opts.connection.target_host, "localhost:7234");
    EXPECT_EQ(opts.client.ns, "time-skip-ns");
    EXPECT_EQ(opts.download_directory.value(), "/tmp/temporal-ts");
    ASSERT_EQ(opts.extra_args.size(), 2u);
}

// ===========================================================================
// WorkflowEnvironment type traits
// ===========================================================================

TEST(WorkflowEnvironmentTest, IsNonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<WorkflowEnvironment>);
    EXPECT_FALSE(std::is_copy_assignable_v<WorkflowEnvironment>);
}

TEST(WorkflowEnvironmentTest, IsMovable) {
    EXPECT_TRUE(std::is_move_constructible_v<WorkflowEnvironment>);
    EXPECT_TRUE(std::is_move_assignable_v<WorkflowEnvironment>);
}
