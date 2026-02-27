#include "temporalio/worker/workflow_replayer.h"

#include <stdexcept>
#include <utility>

#include "temporalio/exceptions/temporal_exception.h"

namespace temporalio::worker {

WorkflowReplayer::WorkflowReplayer(WorkflowReplayerOptions options)
    : options_(std::move(options)) {
    if (options_.workflows.empty()) {
        throw std::invalid_argument("Must have at least one workflow");
    }
}

WorkflowReplayer::~WorkflowReplayer() = default;

async_::Task<WorkflowReplayResult> WorkflowReplayer::replay_workflow_async(
    WorkflowHistory history,
    bool throw_on_replay_failure) {
    // Delegate to the batch replayer with a single history
    std::vector<WorkflowHistory> histories;
    histories.push_back(std::move(history));

    auto results = co_await replay_workflows_async(
        std::move(histories), throw_on_replay_failure);

    co_return std::move(results.front());
}

async_::Task<std::vector<WorkflowReplayResult>>
WorkflowReplayer::replay_workflows_async(
    std::vector<WorkflowHistory> histories,
    bool throw_on_replay_failure) {
    // The real implementation will:
    // 1. Create a bridge WorkerReplayer from the Rust FFI
    // 2. Create a WorkflowWorker connected to the replayer's bridge worker
    // 3. For each history:
    //    a. Push the history to the bridge replayer
    //    b. Wait for the workflow worker to process and evict
    //    c. Collect the result (success or nondeterminism failure)
    // 4. Shutdown the worker and bridge
    //
    // For now, this is a structural placeholder. Each history is processed
    // as a successful replay since no bridge is connected yet.

    std::vector<WorkflowReplayResult> results;
    results.reserve(histories.size());

    for (auto& history : histories) {
        WorkflowReplayResult result;
        result.history = std::move(history);
        result.replay_failure = nullptr;

        // TODO(bridge): Actually replay via bridge worker
        // When bridge is connected, nondeterminism errors will be caught
        // and stored in replay_failure, optionally re-thrown.

        if (throw_on_replay_failure && result.has_failure()) {
            std::rethrow_exception(result.replay_failure);
        }

        results.push_back(std::move(result));
    }

    co_return results;
}

}  // namespace temporalio::worker
