#ifndef TEMPORALIO_BRIDGE_REPLAYER_H
#define TEMPORALIO_BRIDGE_REPLAYER_H

/// @file replayer.h
/// @brief Bridge wrapper for workflow replay.
///
/// Manages the lifecycle of a Rust-allocated workflow replayer, which
/// creates a special worker for replaying workflow histories.
/// Corresponds to parts of C# Bridge/Worker.cs (replayer section).

#include <span>
#include <string>
#include <string_view>

#include "temporalio/bridge/interop.h"
#include "temporalio/bridge/runtime.h"
#include "temporalio/bridge/safe_handle.h"
#include "temporalio/bridge/worker.h"

namespace temporalio::bridge {

/// Result of creating a workflow replayer.
struct ReplayerResult {
    /// The bridge worker handle (for polling activations).
    WorkerHandle worker;

    /// The replay pusher handle (for pushing histories).
    ReplayPusherHandle pusher;
};

/// Bridge wrapper for workflow replay operations.
///
/// Creates a special worker/pusher pair from the Rust FFI for replaying
/// workflow histories. The worker polls for activations (from pushed
/// histories) and the pusher feeds histories to it.
class Replayer {
public:
    /// Create a new replayer from a runtime and worker options.
    /// Returns a ReplayerResult containing the worker and pusher handles.
    /// Throws std::runtime_error on failure.
    static ReplayerResult create(Runtime& runtime,
                                 const TemporalCoreWorkerOptions& options);

    /// Push a workflow history for replay.
    /// @param worker  The replayer's worker handle.
    /// @param pusher  The replay pusher handle.
    /// @param workflow_id  The workflow ID being replayed.
    /// @param history  Serialized workflow history protobuf.
    /// @param runtime  The runtime (for error message decoding).
    /// Throws std::runtime_error on failure.
    static void push_history(TemporalCoreWorker* worker,
                             TemporalCoreWorkerReplayPusher* pusher,
                             std::string_view workflow_id,
                             std::span<const uint8_t> history,
                             Runtime& runtime);
};

}  // namespace temporalio::bridge

#endif  // TEMPORALIO_BRIDGE_REPLAYER_H
