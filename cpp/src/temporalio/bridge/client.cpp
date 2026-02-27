#include "temporalio/bridge/client.h"

#include <cstring>
#include <memory>

#include "temporalio/bridge/call_scope.h"

namespace temporalio::bridge {

namespace {

/// Context passed through FFI callbacks for client connect.
struct ConnectCallbackContext {
    Runtime* runtime;
    ClientConnectCallback callback;
};

/// Context passed through FFI callbacks for RPC calls.
struct RpcCallbackContext {
    Runtime* runtime;
    RpcCallCallback callback;
};

}  // namespace

void Client::connect_async(Runtime& runtime,
                           const ClientOptions& options,
                           ClientConnectCallback callback) {
    // We need to heap-allocate the callback context because the FFI call
    // is asynchronous - the callback fires on a Rust thread.
    auto ctx = std::make_unique<ConnectCallbackContext>();
    ctx->runtime = &runtime;
    ctx->callback = std::move(callback);

    // NOTE: CallScope is destroyed when this function returns, which is before
    // the async callback fires. This is safe because the Rust FFI function
    // copies all ByteArrayRef data synchronously before returning.
    CallScope scope;

    // Build the interop client options
    TemporalCoreClientOptions interop_opts{};
    interop_opts.target_url = scope.byte_array(options.target_url);
    interop_opts.client_name = scope.byte_array(options.client_name);
    interop_opts.client_version = scope.byte_array(options.client_version);
    interop_opts.api_key = scope.byte_array(options.api_key);
    interop_opts.identity = scope.byte_array(options.identity);

    // Metadata
    if (!options.metadata.empty()) {
        interop_opts.metadata =
            scope.byte_array_array_kv(options.metadata);
    }

    // Binary metadata - we need to convert to string pairs for the scope
    // (binary metadata uses key\nvalue format where value is bytes)
    if (!options.binary_metadata.empty()) {
        std::vector<std::pair<std::string, std::string>> binary_as_strings;
        binary_as_strings.reserve(options.binary_metadata.size());
        for (const auto& [key, value] : options.binary_metadata) {
            binary_as_strings.emplace_back(
                key,
                std::string(reinterpret_cast<const char*>(value.data()),
                            value.size()));
        }
        interop_opts.binary_metadata =
            scope.byte_array_array_kv(binary_as_strings);
    }

    // TLS
    TemporalCoreClientTlsOptions tls_opts{};
    if (options.tls) {
        tls_opts.server_root_ca_cert =
            scope.byte_array(options.tls->server_root_ca_cert);
        tls_opts.domain = scope.byte_array(options.tls->domain);
        tls_opts.client_cert = scope.byte_array(options.tls->client_cert);
        tls_opts.client_private_key =
            scope.byte_array(options.tls->client_private_key);
        interop_opts.tls_options = scope.alloc(tls_opts);
    }

    // Retry
    if (options.retry) {
        TemporalCoreClientRetryOptions retry_opts{};
        retry_opts.initial_interval_millis =
            options.retry->initial_interval_millis;
        retry_opts.randomization_factor = options.retry->randomization_factor;
        retry_opts.multiplier = options.retry->multiplier;
        retry_opts.max_interval_millis = options.retry->max_interval_millis;
        retry_opts.max_elapsed_time_millis =
            options.retry->max_elapsed_time_millis;
        retry_opts.max_retries = options.retry->max_retries;
        interop_opts.retry_options = scope.alloc(retry_opts);
    }

    // Keep-alive
    if (options.keep_alive) {
        TemporalCoreClientKeepAliveOptions ka_opts{};
        ka_opts.interval_millis = options.keep_alive->interval_millis;
        ka_opts.timeout_millis = options.keep_alive->timeout_millis;
        interop_opts.keep_alive_options = scope.alloc(ka_opts);
    }

    // HTTP Connect proxy
    if (options.http_connect_proxy) {
        TemporalCoreClientHttpConnectProxyOptions proxy_opts{};
        proxy_opts.target_host =
            scope.byte_array(options.http_connect_proxy->target_host);
        proxy_opts.username =
            scope.byte_array(options.http_connect_proxy->username);
        proxy_opts.password =
            scope.byte_array(options.http_connect_proxy->password);
        interop_opts.http_connect_proxy_options = scope.alloc(proxy_opts);
    }

    // The callback context pointer is passed as user_data.
    // The static lambda captures nothing and matches the C callback signature.
    void* user_data = ctx.release();

    temporal_core_client_connect(
        runtime.get(), scope.alloc(interop_opts), user_data,
        [](void* ud, TemporalCoreClient* success,
           const TemporalCoreByteArray* fail) {
            auto ctx_ptr =
                std::unique_ptr<ConnectCallbackContext>(
                    static_cast<ConnectCallbackContext*>(ud));

            if (fail) {
                ByteArray error_bytes(ctx_ptr->runtime->get(), fail);
                ctx_ptr->callback(nullptr, error_bytes.to_string());
            } else {
                ctx_ptr->callback(
                    make_shared_handle<TemporalCoreClient,
                                      temporal_core_client_free>(success),
                    std::string{});
            }
        });
}

Client::Client(Runtime& runtime, ClientHandle handle)
    : runtime_(&runtime), handle_(std::move(handle)) {}

void Client::rpc_call_async(const RpcCallOptions& options,
                            RpcCallCallback callback) {
    auto ctx = std::make_unique<RpcCallbackContext>();
    ctx->runtime = runtime_;
    ctx->callback = std::move(callback);

    CallScope scope;

    TemporalCoreRpcCallOptions interop_opts{};
    interop_opts.service = options.service;
    interop_opts.rpc = scope.byte_array(options.rpc);
    interop_opts.req = scope.byte_array(
        std::span<const uint8_t>(options.request));
    interop_opts.retry = options.retry ? 1 : 0;
    interop_opts.timeout_millis = options.timeout_millis;
    interop_opts.cancellation_token = nullptr;

    if (!options.metadata.empty()) {
        interop_opts.metadata =
            scope.byte_array_array_kv(options.metadata);
    }

    if (!options.binary_metadata.empty()) {
        std::vector<std::pair<std::string, std::string>> binary_as_strings;
        binary_as_strings.reserve(options.binary_metadata.size());
        for (const auto& [key, value] : options.binary_metadata) {
            binary_as_strings.emplace_back(
                key,
                std::string(reinterpret_cast<const char*>(value.data()),
                            value.size()));
        }
        interop_opts.binary_metadata =
            scope.byte_array_array_kv(binary_as_strings);
    }

    void* user_data = ctx.release();

    temporal_core_client_rpc_call(
        handle_.get(), scope.alloc(interop_opts), user_data,
        [](void* ud, const TemporalCoreByteArray* success,
           uint32_t status_code, const TemporalCoreByteArray* failure_message,
           const TemporalCoreByteArray* failure_details) {
            auto ctx_ptr =
                std::unique_ptr<RpcCallbackContext>(
                    static_cast<RpcCallbackContext*>(ud));

            if (failure_message && status_code > 0) {
                // gRPC error with status code
                ByteArray msg(ctx_ptr->runtime->get(), failure_message);
                RpcCallError error;
                error.status_code = status_code;
                error.message = msg.to_string();
                if (failure_details) {
                    ByteArray details(ctx_ptr->runtime->get(), failure_details);
                    error.details = details.to_bytes();
                }
                ctx_ptr->callback(std::nullopt, std::move(error));
            } else if (failure_message) {
                // Generic error
                ByteArray msg(ctx_ptr->runtime->get(), failure_message);
                RpcCallError error;
                error.status_code = 0;
                error.message = msg.to_string();
                ctx_ptr->callback(std::nullopt, std::move(error));
            } else {
                // Success
                ByteArray response(ctx_ptr->runtime->get(), success);
                RpcCallResult result;
                result.response = response.to_bytes();
                ctx_ptr->callback(std::move(result), std::nullopt);
            }
        });
}

void Client::update_metadata(
    const std::vector<std::pair<std::string, std::string>>& metadata) {
    CallScope scope;
    auto arr = scope.byte_array_array_kv(metadata);
    temporal_core_client_update_metadata(handle_.get(), arr);
}

void Client::update_binary_metadata(
    const std::vector<std::pair<std::string, std::vector<uint8_t>>>&
        metadata) {
    CallScope scope;
    std::vector<std::pair<std::string, std::string>> as_strings;
    as_strings.reserve(metadata.size());
    for (const auto& [key, value] : metadata) {
        as_strings.emplace_back(
            key,
            std::string(reinterpret_cast<const char*>(value.data()),
                        value.size()));
    }
    auto arr = scope.byte_array_array_kv(as_strings);
    temporal_core_client_update_binary_metadata(handle_.get(), arr);
}

void Client::update_api_key(std::string_view api_key) {
    CallScope scope;
    auto ref = scope.byte_array(api_key);
    temporal_core_client_update_api_key(handle_.get(), ref);
}

}  // namespace temporalio::bridge
