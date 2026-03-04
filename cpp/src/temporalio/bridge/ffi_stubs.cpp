/// @file ffi_stubs.cpp
/// @brief Stub implementations of Rust FFI functions for testing without cargo.
///
/// When the Rust bridge library is not available (cargo not found), these stubs
/// allow the C++ code to link and tests to run. Functions that are called during
/// tests will abort with a clear error message.

#include "temporalio/bridge/interop.h"

#include <cstdlib>
#include <cstdio>

namespace {
[[noreturn]] void ffi_stub_abort(const char* func) {
    fprintf(stderr, "FATAL: FFI stub called: %s (Rust bridge not available)\n", func);
    std::abort();
}
}  // namespace

extern "C" {

// -- Cancellation token --
TemporalCoreCancellationToken* temporal_core_cancellation_token_new() {
    ffi_stub_abort("temporal_core_cancellation_token_new");
}

void temporal_core_cancellation_token_cancel(TemporalCoreCancellationToken*) {
    ffi_stub_abort("temporal_core_cancellation_token_cancel");
}

void temporal_core_cancellation_token_free(TemporalCoreCancellationToken*) {
    ffi_stub_abort("temporal_core_cancellation_token_free");
}

// -- Client --
void temporal_core_client_connect(
    TemporalCoreRuntime*, const TemporalCoreClientOptions*,
    void*, TemporalCoreClientConnectCallback) {
    ffi_stub_abort("temporal_core_client_connect");
}

void temporal_core_client_free(TemporalCoreClient*) {
    ffi_stub_abort("temporal_core_client_free");
}

void temporal_core_client_update_metadata(
    TemporalCoreClient*, TemporalCoreByteArrayRefArray) {
    ffi_stub_abort("temporal_core_client_update_metadata");
}

void temporal_core_client_update_binary_metadata(
    TemporalCoreClient*, TemporalCoreByteArrayRefArray) {
    ffi_stub_abort("temporal_core_client_update_binary_metadata");
}

void temporal_core_client_update_api_key(
    TemporalCoreClient*, TemporalCoreByteArrayRef) {
    ffi_stub_abort("temporal_core_client_update_api_key");
}

TemporalCoreByteArrayRef temporal_core_client_grpc_override_request_service(
    const TemporalCoreClientGrpcOverrideRequest*) {
    ffi_stub_abort("temporal_core_client_grpc_override_request_service");
}

TemporalCoreByteArrayRef temporal_core_client_grpc_override_request_rpc(
    const TemporalCoreClientGrpcOverrideRequest*) {
    ffi_stub_abort("temporal_core_client_grpc_override_request_rpc");
}

TemporalCoreByteArrayRefArray temporal_core_client_grpc_override_request_headers(
    const TemporalCoreClientGrpcOverrideRequest*) {
    ffi_stub_abort("temporal_core_client_grpc_override_request_headers");
}

TemporalCoreByteArrayRef temporal_core_client_grpc_override_request_proto(
    const TemporalCoreClientGrpcOverrideRequest*) {
    ffi_stub_abort("temporal_core_client_grpc_override_request_proto");
}

void temporal_core_client_grpc_override_request_respond(
    TemporalCoreClientGrpcOverrideRequest*, TemporalCoreClientGrpcOverrideResponse) {
    ffi_stub_abort("temporal_core_client_grpc_override_request_respond");
}

void temporal_core_client_rpc_call(
    TemporalCoreClient*, const TemporalCoreRpcCallOptions*,
    void*, TemporalCoreClientRpcCallCallback) {
    ffi_stub_abort("temporal_core_client_rpc_call");
}

TemporalCoreClientEnvConfigOrFail temporal_core_client_env_config_load(
    const TemporalCoreClientEnvConfigLoadOptions*) {
    ffi_stub_abort("temporal_core_client_env_config_load");
}

TemporalCoreClientEnvConfigProfileOrFail temporal_core_client_env_config_profile_load(
    const TemporalCoreClientEnvConfigProfileLoadOptions*) {
    ffi_stub_abort("temporal_core_client_env_config_profile_load");
}

// -- Metrics --
TemporalCoreMetricMeter* temporal_core_metric_meter_new(TemporalCoreRuntime*) {
    ffi_stub_abort("temporal_core_metric_meter_new");
}

void temporal_core_metric_meter_free(TemporalCoreMetricMeter*) {
    ffi_stub_abort("temporal_core_metric_meter_free");
}

TemporalCoreMetricAttributes* temporal_core_metric_attributes_new(
    const TemporalCoreMetricMeter*, const TemporalCoreMetricAttribute*, size_t) {
    ffi_stub_abort("temporal_core_metric_attributes_new");
}

TemporalCoreMetricAttributes* temporal_core_metric_attributes_new_append(
    const TemporalCoreMetricMeter*, const TemporalCoreMetricAttributes*,
    const TemporalCoreMetricAttribute*, size_t) {
    ffi_stub_abort("temporal_core_metric_attributes_new_append");
}

void temporal_core_metric_attributes_free(TemporalCoreMetricAttributes*) {
    ffi_stub_abort("temporal_core_metric_attributes_free");
}

TemporalCoreMetric* temporal_core_metric_new(
    const TemporalCoreMetricMeter*, const TemporalCoreMetricOptions*) {
    ffi_stub_abort("temporal_core_metric_new");
}

void temporal_core_metric_free(TemporalCoreMetric*) {
    ffi_stub_abort("temporal_core_metric_free");
}

void temporal_core_metric_record_integer(
    const TemporalCoreMetric*, uint64_t, const TemporalCoreMetricAttributes*) {
    ffi_stub_abort("temporal_core_metric_record_integer");
}

void temporal_core_metric_record_float(
    const TemporalCoreMetric*, double, const TemporalCoreMetricAttributes*) {
    ffi_stub_abort("temporal_core_metric_record_float");
}

void temporal_core_metric_record_duration(
    const TemporalCoreMetric*, uint64_t, const TemporalCoreMetricAttributes*) {
    ffi_stub_abort("temporal_core_metric_record_duration");
}

// -- Random --
TemporalCoreRandom* temporal_core_random_new(uint64_t) {
    ffi_stub_abort("temporal_core_random_new");
}

void temporal_core_random_free(TemporalCoreRandom*) {
    ffi_stub_abort("temporal_core_random_free");
}

int32_t temporal_core_random_int32_range(TemporalCoreRandom*, int32_t, int32_t, bool) {
    ffi_stub_abort("temporal_core_random_int32_range");
}

double temporal_core_random_double_range(TemporalCoreRandom*, double, double, bool) {
    ffi_stub_abort("temporal_core_random_double_range");
}

void temporal_core_random_fill_bytes(TemporalCoreRandom*, TemporalCoreByteArrayRef) {
    ffi_stub_abort("temporal_core_random_fill_bytes");
}

// -- Runtime --
// Return a failure result instead of aborting so that bridge::Runtime throws
// a catchable exception. This lets the WorkflowEnvironmentFixture gracefully
// skip integration tests when the Rust bridge is not available.
static const char kStubErrorMsg[] = "Rust bridge not available (FFI stub)";
static const TemporalCoreByteArray kStubErrorArray{
    reinterpret_cast<const uint8_t*>(kStubErrorMsg),
    sizeof(kStubErrorMsg) - 1
};

TemporalCoreRuntimeOrFail temporal_core_runtime_new(
    const TemporalCoreRuntimeOptions*) {
    return TemporalCoreRuntimeOrFail{nullptr, &kStubErrorArray};
}

// No-ops: the runtime constructor calls these on failure path with null handles.
void temporal_core_runtime_free(TemporalCoreRuntime*) {}

void temporal_core_byte_array_free(TemporalCoreRuntime*, const TemporalCoreByteArray*) {}

// -- Forwarded log accessors --
TemporalCoreByteArrayRef temporal_core_forwarded_log_target(
    const TemporalCoreForwardedLog*) {
    ffi_stub_abort("temporal_core_forwarded_log_target");
}

TemporalCoreByteArrayRef temporal_core_forwarded_log_message(
    const TemporalCoreForwardedLog*) {
    ffi_stub_abort("temporal_core_forwarded_log_message");
}

uint64_t temporal_core_forwarded_log_timestamp_millis(
    const TemporalCoreForwardedLog*) {
    ffi_stub_abort("temporal_core_forwarded_log_timestamp_millis");
}

TemporalCoreByteArrayRef temporal_core_forwarded_log_fields_json(
    const TemporalCoreForwardedLog*) {
    ffi_stub_abort("temporal_core_forwarded_log_fields_json");
}

// -- Ephemeral server --
void temporal_core_ephemeral_server_start_dev_server(
    TemporalCoreRuntime*, const TemporalCoreDevServerOptions*,
    void*, TemporalCoreEphemeralServerStartCallback) {
    ffi_stub_abort("temporal_core_ephemeral_server_start_dev_server");
}

void temporal_core_ephemeral_server_start_test_server(
    TemporalCoreRuntime*, const TemporalCoreTestServerOptions*,
    void*, TemporalCoreEphemeralServerStartCallback) {
    ffi_stub_abort("temporal_core_ephemeral_server_start_test_server");
}

void temporal_core_ephemeral_server_free(TemporalCoreEphemeralServer*) {
    ffi_stub_abort("temporal_core_ephemeral_server_free");
}

void temporal_core_ephemeral_server_shutdown(
    TemporalCoreEphemeralServer*, void*, TemporalCoreEphemeralServerShutdownCallback) {
    ffi_stub_abort("temporal_core_ephemeral_server_shutdown");
}

// -- Worker --
TemporalCoreWorkerOrFail temporal_core_worker_new(
    TemporalCoreClient*, const TemporalCoreWorkerOptions*) {
    ffi_stub_abort("temporal_core_worker_new");
}

void temporal_core_worker_free(TemporalCoreWorker*) {
    ffi_stub_abort("temporal_core_worker_free");
}

void temporal_core_worker_validate(
    TemporalCoreWorker*, void*, TemporalCoreWorkerCallback) {
    ffi_stub_abort("temporal_core_worker_validate");
}

const TemporalCoreByteArray* temporal_core_worker_replace_client(
    TemporalCoreWorker*, TemporalCoreClient*) {
    ffi_stub_abort("temporal_core_worker_replace_client");
}

void temporal_core_worker_poll_workflow_activation(
    TemporalCoreWorker*, void*, TemporalCoreWorkerPollCallback) {
    ffi_stub_abort("temporal_core_worker_poll_workflow_activation");
}

void temporal_core_worker_poll_activity_task(
    TemporalCoreWorker*, void*, TemporalCoreWorkerPollCallback) {
    ffi_stub_abort("temporal_core_worker_poll_activity_task");
}

void temporal_core_worker_poll_nexus_task(
    TemporalCoreWorker*, void*, TemporalCoreWorkerPollCallback) {
    ffi_stub_abort("temporal_core_worker_poll_nexus_task");
}

void temporal_core_worker_complete_workflow_activation(
    TemporalCoreWorker*, TemporalCoreByteArrayRef, void*, TemporalCoreWorkerCallback) {
    ffi_stub_abort("temporal_core_worker_complete_workflow_activation");
}

void temporal_core_worker_complete_activity_task(
    TemporalCoreWorker*, TemporalCoreByteArrayRef, void*, TemporalCoreWorkerCallback) {
    ffi_stub_abort("temporal_core_worker_complete_activity_task");
}

void temporal_core_worker_complete_nexus_task(
    TemporalCoreWorker*, TemporalCoreByteArrayRef, void*, TemporalCoreWorkerCallback) {
    ffi_stub_abort("temporal_core_worker_complete_nexus_task");
}

const TemporalCoreByteArray* temporal_core_worker_record_activity_heartbeat(
    TemporalCoreWorker*, TemporalCoreByteArrayRef) {
    ffi_stub_abort("temporal_core_worker_record_activity_heartbeat");
}

void temporal_core_worker_request_workflow_eviction(
    TemporalCoreWorker*, TemporalCoreByteArrayRef) {
    ffi_stub_abort("temporal_core_worker_request_workflow_eviction");
}

void temporal_core_worker_initiate_shutdown(TemporalCoreWorker*) {
    ffi_stub_abort("temporal_core_worker_initiate_shutdown");
}

void temporal_core_worker_finalize_shutdown(
    TemporalCoreWorker*, void*, TemporalCoreWorkerCallback) {
    ffi_stub_abort("temporal_core_worker_finalize_shutdown");
}

// -- Worker replayer --
TemporalCoreWorkerReplayerOrFail temporal_core_worker_replayer_new(
    TemporalCoreRuntime*, const TemporalCoreWorkerOptions*) {
    ffi_stub_abort("temporal_core_worker_replayer_new");
}

void temporal_core_worker_replay_pusher_free(TemporalCoreWorkerReplayPusher*) {
    ffi_stub_abort("temporal_core_worker_replay_pusher_free");
}

TemporalCoreWorkerReplayPushResult temporal_core_worker_replay_push(
    TemporalCoreWorker*, TemporalCoreWorkerReplayPusher*,
    TemporalCoreByteArrayRef, TemporalCoreByteArrayRef) {
    ffi_stub_abort("temporal_core_worker_replay_push");
}

// -- Slot supplier --
bool temporal_core_complete_async_reserve(
    const TemporalCoreSlotReserveCompletionCtx*, size_t) {
    ffi_stub_abort("temporal_core_complete_async_reserve");
}

bool temporal_core_complete_async_cancel_reserve(
    const TemporalCoreSlotReserveCompletionCtx*) {
    ffi_stub_abort("temporal_core_complete_async_cancel_reserve");
}

}  // extern "C"
