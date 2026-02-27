#pragma once

/// @file Deterministic single-threaded coroutine scheduler for workflow replay.

#include <coroutine>
#include <deque>

namespace temporalio::async_ {

/// Deterministic single-threaded executor for workflow replay.
///
/// Replaces C# WorkflowInstance : TaskScheduler (MaximumConcurrencyLevel = 1).
///
/// This scheduler is NOT thread-safe. It is designed for use within a single
/// workflow execution where all coroutines run on the same thread, ensuring
/// deterministic replay.
///
/// Usage:
///   CoroutineScheduler scheduler;
///   scheduler.schedule(some_coroutine.handle());
///   while (scheduler.drain()) {
///       // process results, schedule more work
///   }
class CoroutineScheduler {
public:
    CoroutineScheduler() = default;

    // Non-copyable, non-movable
    CoroutineScheduler(const CoroutineScheduler&) = delete;
    CoroutineScheduler& operator=(const CoroutineScheduler&) = delete;
    CoroutineScheduler(CoroutineScheduler&&) = delete;
    CoroutineScheduler& operator=(CoroutineScheduler&&) = delete;

    /// Enqueue a coroutine handle to the ready queue.
    /// The handle will be resumed on the next drain() call.
    void schedule(std::coroutine_handle<> handle);

    /// Run all queued coroutines until the ready queue is empty.
    /// Returns true if any coroutines were executed, false if the queue
    /// was already empty.
    ///
    /// This mirrors the C# WorkflowInstance.RunOnce() inner loop:
    ///   while (scheduledTasks.Count > 0) { pop last; execute; }
    bool drain();

    /// Returns the number of coroutines currently in the ready queue.
    size_t size() const noexcept { return ready_queue_.size(); }

    /// Returns true if the ready queue is empty.
    bool empty() const noexcept { return ready_queue_.empty(); }

private:
    std::deque<std::coroutine_handle<>> ready_queue_;
};

}  // namespace temporalio::async_

