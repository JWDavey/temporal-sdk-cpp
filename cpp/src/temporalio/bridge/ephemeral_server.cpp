#include "temporalio/bridge/ephemeral_server.h"

#include <memory>

#include "temporalio/bridge/byte_array.h"
#include "temporalio/bridge/call_scope.h"

namespace temporalio::bridge {

namespace {

/// Context passed through FFI callbacks for server start.
struct StartCallbackContext {
    Runtime* runtime;
    EphemeralServerStartCallback callback;
    bool has_test_service;
};

/// Context passed through FFI callbacks for server shutdown.
struct ShutdownCallbackContext {
    Runtime* runtime;
    EphemeralServerShutdownCallback callback;
};

}  // namespace

void EphemeralServer::start_dev_server_async(
    Runtime& runtime,
    const DevServerOptions& options,
    EphemeralServerStartCallback callback) {
    auto ctx = std::make_unique<StartCallbackContext>();
    ctx->runtime = &runtime;
    ctx->callback = std::move(callback);
    ctx->has_test_service = false;

    CallScope scope;

    // Build test server options (shared between dev and test server)
    TemporalCoreTestServerOptions test_opts{};
    test_opts.existing_path = scope.byte_array(options.existing_path);
    test_opts.sdk_name = scope.byte_array(options.sdk_name);
    test_opts.sdk_version = scope.byte_array(options.sdk_version);
    test_opts.download_version = scope.byte_array(options.download_version);
    test_opts.download_dest_dir = scope.byte_array(options.download_dest_dir);
    test_opts.port = options.port;
    test_opts.extra_args = scope.byte_array(options.extra_args);
    test_opts.download_ttl_seconds = options.download_ttl_seconds;

    TemporalCoreDevServerOptions dev_opts{};
    dev_opts.test_server = scope.alloc(test_opts);
    dev_opts.namespace_ = scope.byte_array(options.namespace_);
    dev_opts.ip = scope.byte_array(options.ip);
    dev_opts.database_filename = scope.byte_array(options.database_filename);
    dev_opts.ui = options.ui;
    dev_opts.ui_port = options.ui_port;
    dev_opts.log_format = scope.byte_array(options.log_format);
    dev_opts.log_level = scope.byte_array(options.log_level);

    void* user_data = ctx.release();

    temporal_core_ephemeral_server_start_dev_server(
        runtime.get(), scope.alloc(dev_opts), user_data,
        [](void* ud, TemporalCoreEphemeralServer* success,
           const TemporalCoreByteArray* success_target,
           const TemporalCoreByteArray* fail) {
            auto ctx_ptr =
                std::unique_ptr<StartCallbackContext>(
                    static_cast<StartCallbackContext*>(ud));

            if (fail) {
                ByteArray error_bytes(ctx_ptr->runtime->get(), fail);
                ctx_ptr->callback(
                    EphemeralServerHandle{},
                    std::string{},
                    error_bytes.to_string());
            } else {
                ByteArray target_bytes(ctx_ptr->runtime->get(), success_target);
                std::string target = target_bytes.to_string();

                auto handle = EphemeralServerHandle(success);
                ctx_ptr->callback(
                    std::move(handle),
                    std::move(target),
                    std::string{});
            }
        });
}

void EphemeralServer::start_test_server_async(
    Runtime& runtime,
    const TestServerOptions& options,
    EphemeralServerStartCallback callback) {
    auto ctx = std::make_unique<StartCallbackContext>();
    ctx->runtime = &runtime;
    ctx->callback = std::move(callback);
    ctx->has_test_service = true;

    CallScope scope;

    TemporalCoreTestServerOptions test_opts{};
    test_opts.existing_path = scope.byte_array(options.existing_path);
    test_opts.sdk_name = scope.byte_array(options.sdk_name);
    test_opts.sdk_version = scope.byte_array(options.sdk_version);
    test_opts.download_version = scope.byte_array(options.download_version);
    test_opts.download_dest_dir = scope.byte_array(options.download_dest_dir);
    test_opts.port = options.port;
    test_opts.extra_args = scope.byte_array(options.extra_args);
    test_opts.download_ttl_seconds = options.download_ttl_seconds;

    void* user_data = ctx.release();

    temporal_core_ephemeral_server_start_test_server(
        runtime.get(), scope.alloc(test_opts), user_data,
        [](void* ud, TemporalCoreEphemeralServer* success,
           const TemporalCoreByteArray* success_target,
           const TemporalCoreByteArray* fail) {
            auto ctx_ptr =
                std::unique_ptr<StartCallbackContext>(
                    static_cast<StartCallbackContext*>(ud));

            if (fail) {
                ByteArray error_bytes(ctx_ptr->runtime->get(), fail);
                ctx_ptr->callback(
                    EphemeralServerHandle{},
                    std::string{},
                    error_bytes.to_string());
            } else {
                ByteArray target_bytes(ctx_ptr->runtime->get(), success_target);
                std::string target = target_bytes.to_string();

                auto handle = EphemeralServerHandle(success);
                ctx_ptr->callback(
                    std::move(handle),
                    std::move(target),
                    std::string{});
            }
        });
}

EphemeralServer::EphemeralServer(Runtime& runtime,
                                 EphemeralServerHandle handle,
                                 std::string target,
                                 bool has_test_service)
    : runtime_(&runtime),
      handle_(std::move(handle)),
      target_(std::move(target)),
      has_test_service_(has_test_service) {}

void EphemeralServer::shutdown_async(EphemeralServerShutdownCallback callback) {
    auto ctx = std::make_unique<ShutdownCallbackContext>();
    ctx->runtime = runtime_;
    ctx->callback = std::move(callback);

    void* user_data = ctx.release();

    temporal_core_ephemeral_server_shutdown(
        handle_.get(), user_data,
        [](void* ud, const TemporalCoreByteArray* fail) {
            auto ctx_ptr =
                std::unique_ptr<ShutdownCallbackContext>(
                    static_cast<ShutdownCallbackContext*>(ud));

            if (fail) {
                ByteArray error_bytes(ctx_ptr->runtime->get(), fail);
                ctx_ptr->callback(error_bytes.to_string());
            } else {
                ctx_ptr->callback(std::string{});
            }
        });
}

}  // namespace temporalio::bridge
