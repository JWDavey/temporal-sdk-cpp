#include "temporalio/bridge/worker.h"

#include <memory>
#include <stdexcept>

#include "temporalio/bridge/call_scope.h"

namespace temporalio::bridge {

namespace {

/// Context passed through FFI callbacks for worker operations.
struct WorkerCallbackContext {
    Runtime* runtime;
    WorkerCallback callback;
};

/// Context passed through FFI callbacks for poll operations.
struct PollCallbackContext {
    Runtime* runtime;
    WorkerPollCallback callback;
};

}  // namespace

Worker::Worker(Client& client, const TemporalCoreWorkerOptions& options)
    : runtime_(&client.runtime()),
      client_handle_(client.shared_handle()) {
    auto result = temporal_core_worker_new(client.get(), &options);

    if (result.fail != nullptr) {
        ByteArray error(runtime_->get(), result.fail);
        throw std::runtime_error("Failed to create worker: " +
                                 error.to_string());
    }

    handle_ = WorkerHandle(result.worker);
}

Worker::Worker(Runtime& runtime, WorkerHandle handle)
    : runtime_(&runtime), handle_(std::move(handle)) {}

void Worker::validate_async(WorkerCallback callback) {
    auto ctx = std::make_unique<WorkerCallbackContext>();
    ctx->runtime = runtime_;
    ctx->callback = std::move(callback);

    void* user_data = ctx.release();

    temporal_core_worker_validate(
        handle_.get(), user_data, handle_worker_callback);
}

void Worker::replace_client(Client& client) {
    auto* fail =
        temporal_core_worker_replace_client(handle_.get(), client.get());
    if (fail) {
        ByteArray error(runtime_->get(), fail);
        throw std::runtime_error("Failed to replace worker client: " +
                                 error.to_string());
    }
    // Update the shared reference so the old client can be released
    // and the new client stays alive while this worker exists.
    client_handle_ = client.shared_handle();
}

void Worker::poll_workflow_activation_async(WorkerPollCallback callback) {
    auto ctx = std::make_unique<PollCallbackContext>();
    ctx->runtime = runtime_;
    ctx->callback = std::move(callback);

    void* user_data = ctx.release();

    temporal_core_worker_poll_workflow_activation(
        handle_.get(), user_data, handle_poll_callback);
}

void Worker::poll_activity_task_async(WorkerPollCallback callback) {
    auto ctx = std::make_unique<PollCallbackContext>();
    ctx->runtime = runtime_;
    ctx->callback = std::move(callback);

    void* user_data = ctx.release();

    temporal_core_worker_poll_activity_task(
        handle_.get(), user_data, handle_poll_callback);
}

void Worker::poll_nexus_task_async(WorkerPollCallback callback) {
    auto ctx = std::make_unique<PollCallbackContext>();
    ctx->runtime = runtime_;
    ctx->callback = std::move(callback);

    void* user_data = ctx.release();

    temporal_core_worker_poll_nexus_task(
        handle_.get(), user_data, handle_poll_callback);
}

void Worker::complete_workflow_activation_async(
    std::span<const uint8_t> completion,
    WorkerCallback callback) {
    auto ctx = std::make_unique<WorkerCallbackContext>();
    ctx->runtime = runtime_;
    ctx->callback = std::move(callback);

    CallScope scope;
    auto completion_ref = scope.byte_array(completion);
    void* user_data = ctx.release();

    // Note: temporal_core_worker_complete_workflow_activation copies the
    // completion data synchronously before returning, so it is safe for
    // `completion_ref` (and the underlying CallScope) to go out of scope
    // after this call.
    temporal_core_worker_complete_workflow_activation(
        handle_.get(), completion_ref, user_data, handle_worker_callback);
}

void Worker::complete_activity_task_async(
    std::span<const uint8_t> completion,
    WorkerCallback callback) {
    auto ctx = std::make_unique<WorkerCallbackContext>();
    ctx->runtime = runtime_;
    ctx->callback = std::move(callback);

    CallScope scope;
    auto completion_ref = scope.byte_array(completion);
    void* user_data = ctx.release();

    temporal_core_worker_complete_activity_task(
        handle_.get(), completion_ref, user_data, handle_worker_callback);
}

void Worker::complete_nexus_task_async(
    std::span<const uint8_t> completion,
    WorkerCallback callback) {
    auto ctx = std::make_unique<WorkerCallbackContext>();
    ctx->runtime = runtime_;
    ctx->callback = std::move(callback);

    CallScope scope;
    auto completion_ref = scope.byte_array(completion);
    void* user_data = ctx.release();

    temporal_core_worker_complete_nexus_task(
        handle_.get(), completion_ref, user_data, handle_worker_callback);
}

void Worker::record_activity_heartbeat(std::span<const uint8_t> heartbeat) {
    CallScope scope;
    auto ref = scope.byte_array(heartbeat);
    auto* fail =
        temporal_core_worker_record_activity_heartbeat(handle_.get(), ref);
    if (fail) {
        ByteArray error(runtime_->get(), fail);
        throw std::runtime_error("Failed to record activity heartbeat: " +
                                 error.to_string());
    }
}

void Worker::request_workflow_eviction(std::string_view run_id) {
    CallScope scope;
    auto ref = scope.byte_array(run_id);
    temporal_core_worker_request_workflow_eviction(handle_.get(), ref);
}

void Worker::initiate_shutdown() {
    temporal_core_worker_initiate_shutdown(handle_.get());
}

void Worker::finalize_shutdown_async(WorkerCallback callback) {
    auto ctx = std::make_unique<WorkerCallbackContext>();
    ctx->runtime = runtime_;
    ctx->callback = std::move(callback);

    void* user_data = ctx.release();

    temporal_core_worker_finalize_shutdown(
        handle_.get(), user_data, handle_worker_callback);
}

void Worker::handle_poll_callback(void* user_data,
                                  const TemporalCoreByteArray* success,
                                  const TemporalCoreByteArray* fail) {
    auto ctx =
        std::unique_ptr<PollCallbackContext>(
            static_cast<PollCallbackContext*>(user_data));

    if (fail) {
        ByteArray error(ctx->runtime->get(), fail);
        ctx->callback(std::nullopt, error.to_string());
    } else if (!success) {
        // Null success means shutdown (poller stopped)
        ctx->callback(std::nullopt, std::string{});
    } else {
        ByteArray data(ctx->runtime->get(), success);
        ctx->callback(data.to_bytes(), std::string{});
    }
}

void Worker::handle_worker_callback(void* user_data,
                                    const TemporalCoreByteArray* fail) {
    auto ctx =
        std::unique_ptr<WorkerCallbackContext>(
            static_cast<WorkerCallbackContext*>(user_data));

    if (fail) {
        ByteArray error(ctx->runtime->get(), fail);
        ctx->callback(error.to_string());
    } else {
        ctx->callback(std::string{});
    }
}

}  // namespace temporalio::bridge
