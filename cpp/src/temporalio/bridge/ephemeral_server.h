#ifndef TEMPORALIO_BRIDGE_EPHEMERAL_SERVER_H
#define TEMPORALIO_BRIDGE_EPHEMERAL_SERVER_H

/// @file ephemeral_server.h
/// @brief Bridge wrapper for ephemeral (dev/test) Temporal servers.
///
/// Manages the lifecycle of Rust-started ephemeral servers for testing.
/// Corresponds to C# Bridge/EphemeralServer.cs.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "temporalio/bridge/interop.h"
#include "temporalio/bridge/runtime.h"
#include "temporalio/bridge/safe_handle.h"

namespace temporalio::bridge {

/// Options for starting a dev server.
struct DevServerOptions {
    /// Path to an existing server binary (skip download).
    std::string existing_path;

    /// SDK name for download metadata.
    std::string sdk_name = "sdk-cpp";

    /// SDK version for download metadata.
    std::string sdk_version;

    /// Version of the dev server to download.
    std::string download_version;

    /// Directory to download the dev server binary to.
    std::string download_dest_dir;

    /// Port to bind to (0 = auto-select).
    uint16_t port = 0;

    /// Extra command-line arguments for the server.
    std::string extra_args;

    /// TTL for the downloaded binary cache in seconds.
    uint64_t download_ttl_seconds = 0;

    /// Namespace for the dev server.
    std::string namespace_ = "default";

    /// IP address to bind to.
    std::string ip = "127.0.0.1";

    /// Database filename (empty = in-memory).
    std::string database_filename;

    /// Whether to start the UI.
    bool ui = false;

    /// UI port (0 = auto-select).
    uint16_t ui_port = 0;

    /// Log format (e.g., "json", "pretty").
    std::string log_format;

    /// Log level (e.g., "warn", "info").
    std::string log_level;
};

/// Options for starting a test (time-skipping) server.
struct TestServerOptions {
    /// Path to an existing server binary (skip download).
    std::string existing_path;

    /// SDK name for download metadata.
    std::string sdk_name = "sdk-cpp";

    /// SDK version for download metadata.
    std::string sdk_version;

    /// Version of the test server to download.
    std::string download_version;

    /// Directory to download the test server binary to.
    std::string download_dest_dir;

    /// Port to bind to (0 = auto-select).
    uint16_t port = 0;

    /// Extra command-line arguments for the server.
    std::string extra_args;

    /// TTL for the downloaded binary cache in seconds.
    uint64_t download_ttl_seconds = 0;
};

/// Callback type for async server start.
/// On success: target is "host:port", error is empty.
/// On failure: target is empty, error contains the message.
using EphemeralServerStartCallback =
    std::function<void(EphemeralServerHandle handle,
                       std::string target,
                       std::string error)>;

/// Callback type for async server shutdown.
using EphemeralServerShutdownCallback =
    std::function<void(std::string error)>;

/// Bridge wrapper for Rust-started ephemeral servers (dev or test).
///
/// Provides static methods to start dev or test servers asynchronously,
/// and methods to shut them down. The server process is managed by Rust.
class EphemeralServer {
public:
    /// Start a dev server asynchronously.
    static void start_dev_server_async(Runtime& runtime,
                                       const DevServerOptions& options,
                                       EphemeralServerStartCallback callback);

    /// Start a test (time-skipping) server asynchronously.
    static void start_test_server_async(Runtime& runtime,
                                        const TestServerOptions& options,
                                        EphemeralServerStartCallback callback);

    /// Construct from an existing handle and target.
    EphemeralServer(Runtime& runtime,
                    EphemeralServerHandle handle,
                    std::string target,
                    bool has_test_service);

    /// Shutdown the server asynchronously.
    void shutdown_async(EphemeralServerShutdownCallback callback);

    /// Get the target host:port string.
    const std::string& target() const noexcept { return target_; }

    /// Whether this server has the test service (supports time skipping).
    bool has_test_service() const noexcept { return has_test_service_; }

    /// Get the raw server pointer.
    TemporalCoreEphemeralServer* get() const noexcept { return handle_.get(); }

    // Move-only
    EphemeralServer(const EphemeralServer&) = delete;
    EphemeralServer& operator=(const EphemeralServer&) = delete;
    EphemeralServer(EphemeralServer&&) noexcept = default;
    EphemeralServer& operator=(EphemeralServer&&) noexcept = default;

private:
    Runtime* runtime_;
    EphemeralServerHandle handle_;
    std::string target_;
    bool has_test_service_;
};

}  // namespace temporalio::bridge

#endif  // TEMPORALIO_BRIDGE_EPHEMERAL_SERVER_H
