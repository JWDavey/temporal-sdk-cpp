#pragma once

/// @file workflow_handle.h
/// @brief Handle to a running or completed workflow execution.

#include <temporalio/async_/task.h>
#include <temporalio/client/workflow_options.h>

#include <memory>
#include <optional>
#include <string>

namespace temporalio::client {

class TemporalClient;

/// Handle to a workflow execution, used for signaling, querying, etc.
/// This is a lightweight value type that holds a reference to the client.
class WorkflowHandle {
public:
    /// Construct a workflow handle.
    WorkflowHandle(std::shared_ptr<TemporalClient> client,
                   std::string id,
                   std::optional<std::string> run_id = std::nullopt,
                   std::optional<std::string> first_execution_run_id = std::nullopt);

    /// Workflow ID.
    const std::string& id() const noexcept { return id_; }

    /// Run ID (may be empty for "latest run").
    const std::optional<std::string>& run_id() const noexcept {
        return run_id_;
    }

    /// First execution run ID for continue-as-new following.
    const std::optional<std::string>& first_execution_run_id() const noexcept {
        return first_execution_run_id_;
    }

    /// Get the result of this workflow execution.
    /// Waits for the workflow to complete and returns the result.
    async_::Task<std::string> get_result();

    /// Signal this workflow.
    async_::Task<void> signal(const std::string& signal_name,
                              const std::string& args = {},
                              const WorkflowSignalOptions& options = {});

    /// Query this workflow.
    async_::Task<std::string> query(
        const std::string& query_type,
        const std::string& args = {},
        const WorkflowQueryOptions& options = {});

    /// Cancel this workflow.
    async_::Task<void> cancel(const WorkflowCancelOptions& options = {});

    /// Terminate this workflow.
    async_::Task<void> terminate(const WorkflowTerminateOptions& options = {});

    /// Describe this workflow.
    async_::Task<std::string> describe(
        const WorkflowDescribeOptions& options = {});

private:
    std::shared_ptr<TemporalClient> client_;
    std::string id_;
    std::optional<std::string> run_id_;
    std::optional<std::string> first_execution_run_id_;
};

} // namespace temporalio::client
