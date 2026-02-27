#ifndef TEMPORALIO_BRIDGE_WORKER_H
#define TEMPORALIO_BRIDGE_WORKER_H

/// @file worker.h
/// @brief Bridge wrapper for the Rust worker.
///
/// Manages the lifecycle of a TemporalCoreWorker, including creation,
/// polling for tasks, completing tasks, and shutdown.
/// Corresponds to C# Bridge/Worker.cs.

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "temporalio/bridge/byte_array.h"
#include "temporalio/bridge/client.h"
#include "temporalio/bridge/interop.h"
#include "temporalio/bridge/runtime.h"
#include "temporalio/bridge/safe_handle.h"

namespace temporalio::bridge {

/// Callback for async operations that either succeed (no data) or fail with a message.
using WorkerCallback = std::function<void(std::string error)>;

/// Callback for poll operations that return optional bytes or an error.
using WorkerPollCallback =
    std::function<void(std::optional<std::vector<uint8_t>> result,
                       std::string error)>;

/// Bridge wrapper for a Rust-allocated TemporalCoreWorker.
///
/// Provides methods for polling workflow activations / activity tasks / nexus
/// tasks, completing them, heartbeating, and shutdown.
///
/// The Worker uses unique ownership for its own handle (WorkerHandle) but
/// holds a shared reference to the Client handle (ClientHandle) to prevent
/// the client from being freed while the worker is active. This mirrors
/// the C# SafeHandleReference<SafeClientHandle> pattern.
///
/// Async operations use callback-style FFI. Higher-level code bridges these
/// to coroutines via TaskCompletionSource.
class Worker {
public:
    /// Create a new worker from a client and options.
    /// Holds a shared reference to the client handle.
    /// Throws std::runtime_error on failure.
    Worker(Client& client, const TemporalCoreWorkerOptions& options);

    /// Construct from an existing handle (e.g., from replayer).
    Worker(Runtime& runtime, WorkerHandle handle);

    /// Validate the worker asynchronously.
    void validate_async(WorkerCallback callback);

    /// Replace the client used by this worker.
    void replace_client(Client& client);

    /// Poll for the next workflow activation asynchronously.
    /// On success, callback receives serialized protobuf bytes (or nullopt on shutdown).
    /// On failure, callback receives empty result and error message.
    void poll_workflow_activation_async(WorkerPollCallback callback);

    /// Poll for the next activity task asynchronously.
    void poll_activity_task_async(WorkerPollCallback callback);

    /// Poll for the next Nexus task asynchronously.
    void poll_nexus_task_async(WorkerPollCallback callback);

    /// Complete a workflow activation asynchronously.
    /// @param completion Serialized WorkflowActivationCompletion protobuf.
    void complete_workflow_activation_async(
        std::span<const uint8_t> completion,
        WorkerCallback callback);

    /// Complete an activity task asynchronously.
    /// @param completion Serialized ActivityTaskCompletion protobuf.
    void complete_activity_task_async(
        std::span<const uint8_t> completion,
        WorkerCallback callback);

    /// Complete a Nexus task asynchronously.
    /// @param completion Serialized NexusTaskCompletion protobuf.
    void complete_nexus_task_async(
        std::span<const uint8_t> completion,
        WorkerCallback callback);

    /// Record an activity heartbeat (synchronous).
    /// @param heartbeat Serialized ActivityHeartbeat protobuf.
    /// Throws std::runtime_error on failure.
    void record_activity_heartbeat(std::span<const uint8_t> heartbeat);

    /// Request eviction of a cached workflow execution.
    void request_workflow_eviction(std::string_view run_id);

    /// Initiate shutdown of the worker (synchronous, non-blocking).
    void initiate_shutdown();

    /// Finalize shutdown (async, waits for all pending work to complete).
    void finalize_shutdown_async(WorkerCallback callback);

    /// Get the raw worker pointer.
    TemporalCoreWorker* get() const noexcept { return handle_.get(); }

    /// Get the associated runtime.
    Runtime& runtime() const noexcept { return *runtime_; }

    // Move-only
    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;
    Worker(Worker&&) noexcept = default;
    Worker& operator=(Worker&&) noexcept = default;

private:
    Runtime* runtime_;
    WorkerHandle handle_;

    /// Shared reference to the client handle, keeping it alive while this
    /// worker exists. Mirrors C# SafeHandleReference<SafeClientHandle>.AddRef.
    ClientHandle client_handle_;

    /// Helper for poll callbacks (shared between workflow/activity/nexus polls).
    static void handle_poll_callback(void* user_data,
                                     const TemporalCoreByteArray* success,
                                     const TemporalCoreByteArray* fail);

    /// Helper for worker callbacks (validate, complete, finalize).
    static void handle_worker_callback(void* user_data,
                                       const TemporalCoreByteArray* fail);
};

}  // namespace temporalio::bridge

#endif  // TEMPORALIO_BRIDGE_WORKER_H
