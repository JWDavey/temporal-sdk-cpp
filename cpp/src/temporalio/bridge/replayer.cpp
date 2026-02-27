#include "temporalio/bridge/replayer.h"

#include <stdexcept>

#include "temporalio/bridge/byte_array.h"
#include "temporalio/bridge/call_scope.h"

namespace temporalio::bridge {

ReplayerResult Replayer::create(Runtime& runtime,
                                const TemporalCoreWorkerOptions& options) {
    auto result = temporal_core_worker_replayer_new(runtime.get(), &options);

    if (result.fail != nullptr) {
        ByteArray error(runtime.get(), result.fail);
        throw std::runtime_error("Failed to create workflow replayer: " +
                                 error.to_string());
    }

    return ReplayerResult{
        WorkerHandle(result.worker),
        ReplayPusherHandle(result.worker_replay_pusher),
    };
}

void Replayer::push_history(TemporalCoreWorker* worker,
                            TemporalCoreWorkerReplayPusher* pusher,
                            std::string_view workflow_id,
                            std::span<const uint8_t> history,
                            Runtime& runtime) {
    CallScope scope;
    auto wf_id_ref = scope.byte_array(workflow_id);
    auto history_ref = scope.byte_array(history);

    auto result = temporal_core_worker_replay_push(
        worker, pusher, wf_id_ref, history_ref);

    if (result.fail != nullptr) {
        ByteArray error(runtime.get(), result.fail);
        throw std::runtime_error("Failed to push replay history: " +
                                 error.to_string());
    }
}

}  // namespace temporalio::bridge
