#include "temporalio/async_/coroutine_scheduler.h"

namespace temporalio::async_ {

void CoroutineScheduler::schedule(std::coroutine_handle<> handle) {
    ready_queue_.push_back(handle);
}

bool CoroutineScheduler::drain() {
    if (ready_queue_.empty()) {
        return false;
    }

    // Process all currently queued coroutines plus any newly enqueued ones.
    // This mirrors the C# pattern in WorkflowInstance.RunOnce():
    //   while (scheduledTasks.Count > 0) { pop last; TryExecuteTask; }
    while (!ready_queue_.empty()) {
        auto handle = ready_queue_.front();
        ready_queue_.pop_front();
        handle.resume();
    }

    return true;
}

}  // namespace temporalio::async_
