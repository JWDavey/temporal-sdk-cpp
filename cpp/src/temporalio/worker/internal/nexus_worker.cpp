#include "temporalio/worker/internal/nexus_worker.h"

#include <utility>

namespace temporalio::worker::internal {

NexusWorker::NexusWorker(NexusWorkerOptions options)
    : options_(std::move(options)) {}

NexusWorker::~NexusWorker() = default;

async_::Task<void> NexusWorker::execute_async() {
    // The real implementation will:
    // 1. Poll Nexus tasks from bridge_worker.poll_nexus_task()
    // 2. For each task:
    //    a. Look up NexusService and NexusOperation by name
    //    b. Build interceptor chain and dispatch
    //    c. Send completion back via bridge_worker.complete_nexus_task()
    // 3. Continue until poll returns null (shutdown)
    //
    // WARNING: Nexus support is experimental.

    // TODO(bridge): Implement poll loop against bridge worker
    co_return;
}

}  // namespace temporalio::worker::internal
