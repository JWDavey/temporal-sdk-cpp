#include "temporalio/worker/internal/workflow_worker.h"

#include <stdexcept>
#include <utility>

namespace temporalio::worker::internal {

WorkflowWorker::WorkflowWorker(WorkflowWorkerOptions options)
    : options_(std::move(options)) {}

WorkflowWorker::~WorkflowWorker() = default;

async_::Task<void> WorkflowWorker::execute_async() {
    // The real implementation will:
    // 1. Poll workflow activations from bridge_worker.poll_workflow_activation()
    // 2. For each activation:
    //    a. Look up or create WorkflowInstance by run ID
    //    b. Call instance.activate(jobs) to get commands
    //    c. Send completion back via bridge_worker.complete_workflow_activation()
    // 3. Handle cache eviction (remove_from_cache jobs)
    // 4. Continue until poll returns null (shutdown)
    //
    // Deadlock detection:
    //    If an activation takes longer than 2 seconds (configurable via
    //    debug_mode), the workflow is considered deadlocked and a failure
    //    completion is sent back to the server.

    // TODO(bridge): Implement poll loop against bridge worker
    co_return;
}

}  // namespace temporalio::worker::internal
