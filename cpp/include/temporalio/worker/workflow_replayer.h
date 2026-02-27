#pragma once

/// @file workflow_replayer.h
/// @brief WorkflowReplayer - replay workflows from existing history for
///        testing determinism and debugging.

#include <exception>
#include <memory>
#include <string>
#include <vector>

#include <temporalio/async_/task.h>
#include <temporalio/workflows/workflow_definition.h>

namespace temporalio::converters {
struct DataConverter;
}

namespace temporalio::worker::interceptors {
class IWorkerInterceptor;
}

namespace temporalio::worker {

/// A recorded workflow history event for replay.
struct WorkflowHistoryEvent {
    // Placeholder for the protobuf HistoryEvent type.
    // Will be replaced with the actual proto type when bridge is wired up.
    std::string serialized_data;
};

/// A workflow history to replay.
struct WorkflowHistory {
    /// Workflow ID from the original execution.
    std::string id;

    /// The recorded history events.
    std::vector<WorkflowHistoryEvent> events;
};

/// Result of a single workflow replay.
struct WorkflowReplayResult {
    /// The history that was replayed.
    WorkflowHistory history;

    /// Workflow task failure during replay (e.g. nondeterminism).
    /// Null if replay succeeded. Note: normal workflow failures
    /// are NOT captured here.
    std::exception_ptr replay_failure;

    /// Whether the replay had a failure.
    bool has_failure() const noexcept {
        return replay_failure != nullptr;
    }
};

/// Options for creating a WorkflowReplayer.
struct WorkflowReplayerOptions {
    /// Workflow definitions to register for replay.
    std::vector<std::shared_ptr<workflows::WorkflowDefinition>> workflows;

    /// Namespace to use for replay. Default: "ReplayNamespace".
    std::string ns{"ReplayNamespace"};

    /// Task queue to use for replay. Default: "ReplayTaskQueue".
    std::string task_queue{"ReplayTaskQueue"};

    /// Data converter for deserialization.
    std::shared_ptr<converters::DataConverter> data_converter;

    /// Worker interceptors.
    std::vector<std::shared_ptr<interceptors::IWorkerInterceptor>>
        interceptors;

    /// Whether to run in debug mode (disables deadlock detection).
    bool debug_mode{false};
};

/// Replayer for replaying workflows from existing history to verify
/// determinism and debug workflow logic.
///
/// Usage:
///   WorkflowReplayerOptions opts;
///   opts.workflows.push_back(my_workflow_def);
///   WorkflowReplayer replayer(opts);
///   auto result = co_await replayer.replay_workflow_async(history);
class WorkflowReplayer {
public:
    /// Create a replayer with the given options.
    /// @throws std::invalid_argument if no workflows are provided.
    explicit WorkflowReplayer(WorkflowReplayerOptions options);

    ~WorkflowReplayer();

    // Non-copyable, non-movable
    WorkflowReplayer(const WorkflowReplayer&) = delete;
    WorkflowReplayer& operator=(const WorkflowReplayer&) = delete;

    /// Replay a single workflow from the given history.
    /// @param history The history to replay.
    /// @param throw_on_replay_failure If true (default), throws on workflow
    ///        task failure (e.g. nondeterminism).
    /// @return The replay result.
    async_::Task<WorkflowReplayResult> replay_workflow_async(
        WorkflowHistory history,
        bool throw_on_replay_failure = true);

    /// Replay multiple workflows from the given histories.
    /// @param histories The histories to replay.
    /// @param throw_on_replay_failure If true, throws on the first workflow
    ///        task failure encountered.
    /// @return The replay results.
    async_::Task<std::vector<WorkflowReplayResult>> replay_workflows_async(
        std::vector<WorkflowHistory> histories,
        bool throw_on_replay_failure = false);

    /// Get the replayer options.
    const WorkflowReplayerOptions& options() const noexcept {
        return options_;
    }

private:
    WorkflowReplayerOptions options_;
};

}  // namespace temporalio::worker

