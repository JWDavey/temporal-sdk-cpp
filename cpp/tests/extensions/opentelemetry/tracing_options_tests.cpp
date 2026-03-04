#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "temporalio/extensions/opentelemetry/tracing_options.h"

using namespace temporalio::extensions::opentelemetry;

// ===========================================================================
// TracingInterceptorOptions tests
// ===========================================================================

TEST(TracingInterceptorOptionsTest, DefaultValues) {
    TracingInterceptorOptions opts;
    EXPECT_EQ(opts.header_key, "_tracer-data");
    EXPECT_TRUE(opts.tag_name_workflow_id.has_value());
    EXPECT_EQ(opts.tag_name_workflow_id.value(), "temporalWorkflowID");
    EXPECT_TRUE(opts.tag_name_run_id.has_value());
    EXPECT_EQ(opts.tag_name_run_id.value(), "temporalRunID");
    EXPECT_TRUE(opts.tag_name_activity_id.has_value());
    EXPECT_EQ(opts.tag_name_activity_id.value(), "temporalActivityID");
    EXPECT_TRUE(opts.tag_name_update_id.has_value());
    EXPECT_EQ(opts.tag_name_update_id.value(), "temporalUpdateID");
}

TEST(TracingInterceptorOptionsTest, CustomHeaderKey) {
    TracingInterceptorOptions opts{
        .header_key = "my-custom-trace-header",
    };
    EXPECT_EQ(opts.header_key, "my-custom-trace-header");
}

TEST(TracingInterceptorOptionsTest, DisableWorkflowIdTag) {
    TracingInterceptorOptions opts{
        .tag_name_workflow_id = std::nullopt,
    };
    EXPECT_FALSE(opts.tag_name_workflow_id.has_value());
    // Other tags should still have defaults
    EXPECT_TRUE(opts.tag_name_run_id.has_value());
    EXPECT_TRUE(opts.tag_name_activity_id.has_value());
    EXPECT_TRUE(opts.tag_name_update_id.has_value());
}

TEST(TracingInterceptorOptionsTest, DisableAllTags) {
    TracingInterceptorOptions opts{
        .tag_name_workflow_id = std::nullopt,
        .tag_name_run_id = std::nullopt,
        .tag_name_activity_id = std::nullopt,
        .tag_name_update_id = std::nullopt,
    };
    EXPECT_FALSE(opts.tag_name_workflow_id.has_value());
    EXPECT_FALSE(opts.tag_name_run_id.has_value());
    EXPECT_FALSE(opts.tag_name_activity_id.has_value());
    EXPECT_FALSE(opts.tag_name_update_id.has_value());
}

TEST(TracingInterceptorOptionsTest, CustomTagNames) {
    TracingInterceptorOptions opts{
        .tag_name_workflow_id = "wf.id",
        .tag_name_run_id = "run.id",
        .tag_name_activity_id = "act.id",
        .tag_name_update_id = "upd.id",
    };
    EXPECT_EQ(opts.tag_name_workflow_id.value(), "wf.id");
    EXPECT_EQ(opts.tag_name_run_id.value(), "run.id");
    EXPECT_EQ(opts.tag_name_activity_id.value(), "act.id");
    EXPECT_EQ(opts.tag_name_update_id.value(), "upd.id");
}
