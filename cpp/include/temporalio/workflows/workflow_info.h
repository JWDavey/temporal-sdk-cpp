#pragma once

/// @file Workflow execution information and metadata types.

#include <chrono>
#include <exception>
#include <optional>
#include <string>

namespace temporalio::workflows {

/// Information about a parent workflow.
struct ParentWorkflowInfo {
    std::string namespace_;
    std::string run_id;
    std::string workflow_id;

    bool operator==(const ParentWorkflowInfo&) const = default;
};

/// Information about the root workflow.
struct RootWorkflowInfo {
    std::string run_id;
    std::string workflow_id;

    bool operator==(const RootWorkflowInfo&) const = default;
};

/// Information about the running workflow.
/// This is immutable for the life of the workflow run.
struct WorkflowInfo {
    /// Current workflow attempt.
    int attempt = 1;
    /// Run ID if this was continued.
    std::optional<std::string> continued_run_id;
    /// Cron schedule if applicable.
    std::optional<std::string> cron_schedule;
    /// Execution timeout for the workflow.
    std::optional<std::chrono::milliseconds> execution_timeout;
    /// The very first run ID the workflow ever had.
    std::string first_execution_run_id;
    /// Failure if this is a continuation of a failure.
    std::exception_ptr last_failure;
    /// Namespace for the workflow.
    std::string namespace_;
    /// Parent information if this is a child workflow.
    std::optional<ParentWorkflowInfo> parent;
    /// Root workflow information.
    std::optional<RootWorkflowInfo> root;
    /// Run ID for the workflow.
    std::string run_id;
    /// Run timeout for the workflow.
    std::optional<std::chrono::milliseconds> run_timeout;
    /// Time when the first workflow task started.
    std::chrono::system_clock::time_point start_time;
    /// Task queue for the workflow.
    std::string task_queue;
    /// Task timeout for the workflow.
    std::chrono::milliseconds task_timeout{};
    /// ID for the workflow.
    std::string workflow_id;
    /// Time when the workflow started on the server.
    std::chrono::system_clock::time_point workflow_start_time;
    /// Workflow type name.
    std::string workflow_type;
};

/// Information about the current update handler invocation.
struct WorkflowUpdateInfo {
    /// Current update ID.
    std::string id;
    /// Current update name.
    std::string name;
};

}  // namespace temporalio::workflows

