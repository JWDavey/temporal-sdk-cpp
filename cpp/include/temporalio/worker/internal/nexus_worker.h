#pragma once

/// @file nexus_worker.h
/// @brief Internal Nexus task poller and dispatcher.
/// WARNING: Nexus support is experimental.

#include <memory>
#include <string>
#include <vector>

#include <temporalio/async_/task.h>

namespace temporalio::nexus {
class NexusServiceDefinition;
}

namespace temporalio::worker::internal {

/// Configuration for the internal NexusWorker.
struct NexusWorkerOptions {
    /// Task queue being polled.
    std::string task_queue;

    /// Namespace.
    std::string ns;

    /// Registered Nexus service definitions.
    std::vector<std::shared_ptr<nexus::NexusServiceDefinition>> services;
};

/// Internal Nexus worker that polls Nexus tasks from the bridge
/// and dispatches them to registered Nexus operation handlers.
///
/// This mirrors the C# internal NexusWorker class.
/// WARNING: Nexus support is experimental.
class NexusWorker {
public:
    explicit NexusWorker(NexusWorkerOptions options);
    ~NexusWorker();

    // Non-copyable
    NexusWorker(const NexusWorker&) = delete;
    NexusWorker& operator=(const NexusWorker&) = delete;

    /// Run the poll loop until the bridge signals shutdown.
    /// @return Task that completes when polling stops.
    async_::Task<void> execute_async();

private:
    NexusWorkerOptions options_;

    /// Service definitions indexed by name for O(1) lookup.
    std::vector<std::shared_ptr<nexus::NexusServiceDefinition>> services_;
};

}  // namespace temporalio::worker::internal

