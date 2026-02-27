#pragma once

/// @file activity_worker.h
/// @brief Internal activity task poller and dispatcher.

#include <memory>
#include <stop_token>
#include <string>
#include <unordered_map>

#include <temporalio/activities/activity.h>
#include <temporalio/async_/task.h>

namespace temporalio::converters {
struct DataConverter;
}

namespace temporalio::worker::interceptors {
class IWorkerInterceptor;
}

namespace temporalio::worker::internal {

/// Configuration for the internal ActivityWorker.
struct ActivityWorkerOptions {
    /// Task queue being polled.
    std::string task_queue;

    /// Namespace.
    std::string ns;

    /// Registered activity definitions by name.
    std::unordered_map<std::string,
                       std::shared_ptr<activities::ActivityDefinition>>
        activities;

    /// Dynamic activity definition (if any).
    std::shared_ptr<activities::ActivityDefinition> dynamic_activity;

    /// Data converter.
    std::shared_ptr<converters::DataConverter> data_converter;

    /// Worker interceptors.
    std::vector<std::shared_ptr<interceptors::IWorkerInterceptor>>
        interceptors;

    /// Maximum concurrent activities.
    uint32_t max_concurrent{100};
};

/// Internal activity worker that polls activity tasks from the bridge
/// and dispatches them to registered ActivityDefinition handlers.
///
/// This mirrors the C# internal ActivityWorker class.
class ActivityWorker {
public:
    explicit ActivityWorker(ActivityWorkerOptions options);
    ~ActivityWorker();

    // Non-copyable
    ActivityWorker(const ActivityWorker&) = delete;
    ActivityWorker& operator=(const ActivityWorker&) = delete;

    /// Run the poll loop until the bridge signals shutdown.
    /// @return Task that completes when polling stops.
    async_::Task<void> execute_async();

    /// Notify the worker that shutdown is in progress.
    /// This cancels all running activity contexts.
    void notify_shutdown();

private:
    ActivityWorkerOptions options_;
    std::stop_source shutdown_source_;
};

}  // namespace temporalio::worker::internal

