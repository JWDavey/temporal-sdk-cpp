#ifndef TEMPORALIO_BRIDGE_CLIENT_H
#define TEMPORALIO_BRIDGE_CLIENT_H

/// @file client.h
/// @brief Bridge wrapper for the Rust client.
///
/// Manages the lifecycle of a TemporalCoreClient, including connection,
/// RPC calls, and metadata updates. Corresponds to C# Bridge/Client.cs.

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "temporalio/bridge/byte_array.h"
#include "temporalio/bridge/interop.h"
#include "temporalio/bridge/runtime.h"
#include "temporalio/bridge/safe_handle.h"

namespace temporalio::bridge {

/// TLS configuration for client connections.
struct ClientTlsOptions {
    std::string server_root_ca_cert;
    std::string domain;
    std::string client_cert;
    std::string client_private_key;
};

/// Retry configuration for client connections.
struct ClientRetryOptions {
    uint64_t initial_interval_millis = 0;
    double randomization_factor = 0.0;
    double multiplier = 0.0;
    uint64_t max_interval_millis = 0;
    uint64_t max_elapsed_time_millis = 0;
    size_t max_retries = 0;
};

/// Keep-alive configuration for client connections.
struct ClientKeepAliveOptions {
    uint64_t interval_millis = 0;
    uint64_t timeout_millis = 0;
};

/// HTTP Connect proxy configuration.
struct ClientHttpConnectProxyOptions {
    std::string target_host;
    std::string username;
    std::string password;
};

/// Options for connecting a client to a Temporal server.
struct ClientOptions {
    std::string target_url;
    std::string client_name;
    std::string client_version;
    std::vector<std::pair<std::string, std::string>> metadata;
    std::vector<std::pair<std::string, std::vector<uint8_t>>> binary_metadata;
    std::string api_key;
    std::string identity;
    std::optional<ClientTlsOptions> tls;
    std::optional<ClientRetryOptions> retry;
    std::optional<ClientKeepAliveOptions> keep_alive;
    std::optional<ClientHttpConnectProxyOptions> http_connect_proxy;
};

/// Options for an RPC call.
struct RpcCallOptions {
    TemporalCoreRpcService service;
    std::string rpc;
    std::vector<uint8_t> request;
    bool retry = false;
    std::vector<std::pair<std::string, std::string>> metadata;
    std::vector<std::pair<std::string, std::vector<uint8_t>>> binary_metadata;
    uint32_t timeout_millis = 0;
};

/// Result of a successful RPC call.
struct RpcCallResult {
    std::vector<uint8_t> response;
};

/// Error from an RPC call. Contains gRPC status code and message.
struct RpcCallError {
    uint32_t status_code;
    std::string message;
    std::vector<uint8_t> details;  // raw gRPC status proto
};

/// Callback type for async client connect.
/// On success: handle is valid, error is empty.
/// On failure: handle is null, error contains the message.
using ClientConnectCallback =
    std::function<void(ClientHandle handle, std::string error)>;

/// Callback type for async RPC calls.
using RpcCallCallback =
    std::function<void(std::optional<RpcCallResult> result,
                       std::optional<RpcCallError> error)>;

/// Bridge wrapper for a Rust-allocated TemporalCoreClient.
///
/// Provides methods for connecting to Temporal, making RPC calls, and
/// updating metadata. The client is thread-safe.
///
/// The underlying handle uses shared ownership (std::shared_ptr) to support
/// the C# SafeHandleReference pattern where Workers hold a reference to the
/// Client handle.
///
/// Async operations use callback-style FFI. Higher-level code (in the
/// client/ layer) bridges these callbacks to coroutines via
/// TaskCompletionSource.
class Client {
public:
    /// Connect to a Temporal server asynchronously.
    /// On success, callback receives a valid ClientHandle and empty error.
    /// On failure, callback receives a null handle and error message.
    static void connect_async(Runtime& runtime,
                              const ClientOptions& options,
                              ClientConnectCallback callback);

    /// Construct from an existing handle.
    Client(Runtime& runtime, ClientHandle handle);

    /// Make an RPC call asynchronously.
    void rpc_call_async(const RpcCallOptions& options,
                        RpcCallCallback callback);

    /// Update client metadata (headers).
    void update_metadata(
        const std::vector<std::pair<std::string, std::string>>& metadata);

    /// Update client binary metadata (binary headers).
    void update_binary_metadata(
        const std::vector<std::pair<std::string, std::vector<uint8_t>>>& metadata);

    /// Update client API key.
    void update_api_key(std::string_view api_key);

    /// Get the raw client pointer (for passing to worker creation etc.).
    TemporalCoreClient* get() const noexcept { return handle_.get(); }

    /// Get the shared client handle (for Worker to hold a reference).
    const ClientHandle& shared_handle() const noexcept { return handle_; }

    /// Get the associated runtime.
    Runtime& runtime() const noexcept { return *runtime_; }

    // Non-copyable but movable.
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) noexcept = default;
    Client& operator=(Client&&) noexcept = default;

private:
    Runtime* runtime_;
    ClientHandle handle_;
};

}  // namespace temporalio::bridge

#endif  // TEMPORALIO_BRIDGE_CLIENT_H
