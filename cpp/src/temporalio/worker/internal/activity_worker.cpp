#include "temporalio/worker/internal/activity_worker.h"

#include <stdexcept>
#include <utility>

namespace temporalio::worker::internal {

ActivityWorker::ActivityWorker(ActivityWorkerOptions options)
    : options_(std::move(options)) {}

ActivityWorker::~ActivityWorker() = default;

async_::Task<void> ActivityWorker::execute_async() {
    // The real implementation will:
    // 1. Poll activity tasks from bridge_worker.poll_activity_task()
    // 2. For each task:
    //    a. Look up ActivityDefinition by name
    //    b. Set up ActivityExecutionContext (thread_local)
    //    c. Build interceptor chain and call execute_activity_async()
    //    d. Send completion back via bridge_worker.complete_activity_task()
    // 3. Handle cancellation tasks
    // 4. Continue until poll returns null (shutdown)
    // 5. On shutdown, cancel all running activities and wait for completion

    // TODO(bridge): Implement poll loop against bridge worker
    co_return;
}

void ActivityWorker::notify_shutdown() {
    shutdown_source_.request_stop();
}

}  // namespace temporalio::worker::internal
