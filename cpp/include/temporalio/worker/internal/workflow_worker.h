#pragma once

/// @file workflow_worker.h
/// @brief Internal workflow task poller and dispatcher.

#include <memory>
#include <string>
#include <unordered_map>

#include <temporalio/async_/task.h>
#include <temporalio/worker/workflow_instance.h>
#include <temporalio/workflows/workflow_definition.h>

namespace temporalio::converters {
struct DataConverter;
}

namespace temporalio::worker::interceptors {
class IWorkerInterceptor;
}

namespace temporalio::worker::internal {

/// Configuration for the internal WorkflowWorker.
struct WorkflowWorkerOptions {
    /// Task queue being polled.
    std::string task_queue;

    /// Namespace.
    std::string ns;

    /// Registered workflow definitions by name.
    std::unordered_map<std::string,
                       std::shared_ptr<workflows::WorkflowDefinition>>
        workflows;

    /// Dynamic workflow definition (if any).
    std::shared_ptr<workflows::WorkflowDefinition> dynamic_workflow;

    /// Data converter.
    std::shared_ptr<converters::DataConverter> data_converter;

    /// Worker interceptors.
    std::vector<std::shared_ptr<interceptors::IWorkerInterceptor>>
        interceptors;

    /// Whether to run in debug mode (disables deadlock detection).
    bool debug_mode{false};
};

/// Internal workflow worker that polls workflow activations from the
/// bridge and dispatches them to WorkflowInstance objects.
///
/// This mirrors the C# internal WorkflowWorker class.
class WorkflowWorker {
public:
    explicit WorkflowWorker(WorkflowWorkerOptions options);
    ~WorkflowWorker();

    // Non-copyable
    WorkflowWorker(const WorkflowWorker&) = delete;
    WorkflowWorker& operator=(const WorkflowWorker&) = delete;

    /// Run the poll loop until the bridge signals shutdown.
    /// @return Task that completes when polling stops.
    async_::Task<void> execute_async();

private:
    WorkflowWorkerOptions options_;

    /// Running workflow instances keyed by run ID.
    std::unordered_map<std::string, std::unique_ptr<WorkflowInstance>>
        running_workflows_;
};

}  // namespace temporalio::worker::internal

