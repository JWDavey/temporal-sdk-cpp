#pragma once

/// @file temporal_client.h
/// @brief TemporalClient - workflow CRUD, schedule management.

#include <temporalio/async_/task.h>
#include <temporalio/client/temporal_connection.h>
#include <temporalio/client/workflow_handle.h>
#include <temporalio/client/workflow_options.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace temporalio::bridge {
class Client;
} // namespace temporalio::bridge

namespace temporalio::runtime {
class TemporalRuntime;
} // namespace temporalio::runtime

namespace temporalio::client {

class TemporalConnection;

namespace interceptors {
class IClientInterceptor;
} // namespace interceptors

/// Options for creating a TemporalClient.
struct TemporalClientOptions {
    /// Namespace to use. Default: "default".
    std::string ns{"default"};

    /// Client interceptors. Earlier interceptors wrap later ones.
    std::vector<std::shared_ptr<interceptors::IClientInterceptor>>
        interceptors{};
};

/// Options for connecting and creating a TemporalClient.
struct TemporalClientConnectOptions {
    /// Connection options.
    TemporalConnectionOptions connection{};

    /// Client options.
    TemporalClientOptions client{};
};

/// Information about a workflow execution.
struct WorkflowExecution {
    /// Workflow ID.
    std::string workflow_id{};

    /// Run ID.
    std::string run_id{};

    /// Workflow type name.
    std::optional<std::string> workflow_type{};
};

/// Count of workflow executions.
struct WorkflowExecutionCount {
    /// Total count.
    int64_t count{0};
};

/// Main client for interacting with a Temporal server.
/// Thread-safe and designed to be shared (std::shared_ptr).
// TODO: Replace raw std::string args/results with Payload types once
// the DataConverter integration is wired up.
class TemporalClient : public std::enable_shared_from_this<TemporalClient> {
public:
    /// Connect to Temporal and create a client.
    static async_::Task<std::shared_ptr<TemporalClient>> connect(
        TemporalClientConnectOptions options);

    /// Create a client from an existing connection.
    static std::shared_ptr<TemporalClient> create(
        std::shared_ptr<TemporalConnection> connection,
        TemporalClientOptions options = {});

    ~TemporalClient();

    // Non-copyable
    TemporalClient(const TemporalClient&) = delete;
    TemporalClient& operator=(const TemporalClient&) = delete;

    /// Get the connection this client uses.
    std::shared_ptr<TemporalConnection> connection() const noexcept;

    /// Get the namespace this client uses.
    const std::string& ns() const noexcept;

    /// Get the underlying bridge client, or nullptr if not connected.
    /// Used internally by TemporalWorker to create a bridge worker.
    bridge::Client* bridge_client() const noexcept;

    /// Start a workflow execution.
    async_::Task<WorkflowHandle> start_workflow(
        const std::string& workflow_type,
        const std::string& args,
        const WorkflowOptions& options);

    /// Get a handle to an existing workflow.
    WorkflowHandle get_workflow_handle(
        const std::string& workflow_id,
        std::optional<std::string> run_id = std::nullopt);

    /// Signal a workflow.
    async_::Task<void> signal_workflow(
        const std::string& workflow_id,
        const std::string& signal_name,
        const std::string& args = {},
        std::optional<std::string> run_id = std::nullopt);

    /// Query a workflow.
    async_::Task<std::string> query_workflow(
        const std::string& workflow_id,
        const std::string& query_type,
        const std::string& args = {},
        std::optional<std::string> run_id = std::nullopt);

    /// Cancel a workflow.
    async_::Task<void> cancel_workflow(
        const std::string& workflow_id,
        std::optional<std::string> run_id = std::nullopt);

    /// Terminate a workflow.
    async_::Task<void> terminate_workflow(
        const std::string& workflow_id,
        std::optional<std::string> reason = std::nullopt,
        std::optional<std::string> run_id = std::nullopt);

    /// List workflows.
    async_::Task<std::vector<WorkflowExecution>> list_workflows(
        const WorkflowListOptions& options = {});

    /// Count workflows.
    async_::Task<WorkflowExecutionCount> count_workflows(
        const WorkflowCountOptions& options = {});

private:
    TemporalClient(std::shared_ptr<TemporalConnection> connection,
                   TemporalClientOptions options);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace temporalio::client
