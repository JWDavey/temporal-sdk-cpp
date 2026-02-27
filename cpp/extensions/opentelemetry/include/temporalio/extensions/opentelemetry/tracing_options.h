#pragma once

/// @file tracing_options.h
/// @brief Configuration for the OpenTelemetry tracing interceptor.

#include <optional>
#include <string>

namespace temporalio::extensions::opentelemetry {

/// Options for configuring the TracingInterceptor.
struct TracingInterceptorOptions {
    /// Temporal header key used to propagate trace context.
    /// Default: "_tracer-data".
    std::string header_key{"_tracer-data"};

    /// Tag name for workflow IDs on spans.
    /// Set to nullopt to disable this tag.
    std::optional<std::string> tag_name_workflow_id{"temporalWorkflowID"};

    /// Tag name for run IDs on spans.
    /// Set to nullopt to disable this tag.
    std::optional<std::string> tag_name_run_id{"temporalRunID"};

    /// Tag name for activity IDs on spans.
    /// Set to nullopt to disable this tag.
    std::optional<std::string> tag_name_activity_id{"temporalActivityID"};

    /// Tag name for update IDs on spans.
    /// Set to nullopt to disable this tag.
    std::optional<std::string> tag_name_update_id{"temporalUpdateID"};
};

}  // namespace temporalio::extensions::opentelemetry
