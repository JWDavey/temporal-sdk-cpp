#ifndef TEMPORALIO_BRIDGE_RUNTIME_H
#define TEMPORALIO_BRIDGE_RUNTIME_H

/// @file runtime.h
/// @brief Bridge wrapper for the Rust runtime.
///
/// Manages the lifecycle of a TemporalCoreRuntime, including creation with
/// telemetry options and cleanup. Corresponds to C# Bridge/Runtime.cs.

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "temporalio/bridge/byte_array.h"
#include "temporalio/bridge/interop.h"
#include "temporalio/bridge/safe_handle.h"

namespace temporalio::bridge {

/// Configuration for log forwarding from the Rust core.
struct LogForwardingOptions {
    /// Filter string (e.g., "temporal_sdk_core=DEBUG").
    std::string filter;

    /// Callback invoked for each forwarded log.
    std::function<void(TemporalCoreForwardedLogLevel level,
                       std::string_view target,
                       std::string_view message,
                       uint64_t timestamp_millis)>
        callback;
};

/// Options for creating a runtime.
struct RuntimeOptions {
    /// Optional log forwarding configuration.
    std::unique_ptr<LogForwardingOptions> log_forwarding;

    /// Optional OpenTelemetry metrics endpoint.
    std::unique_ptr<TemporalCoreOpenTelemetryOptions> opentelemetry;

    /// Optional Prometheus metrics endpoint.
    std::unique_ptr<TemporalCorePrometheusOptions> prometheus;

    /// Optional custom metric meter.
    const TemporalCoreCustomMetricMeter* custom_meter = nullptr;

    /// Whether to attach service name to metrics.
    bool attach_service_name = false;

    /// Global metric tags (newline-delimited key=value pairs).
    std::string global_tags;

    /// Metric prefix.
    std::string metric_prefix;

    /// Worker heartbeat interval in milliseconds (0 = use default).
    uint64_t worker_heartbeat_interval_millis = 0;
};

/// Bridge wrapper for a Rust-allocated TemporalCoreRuntime.
///
/// Manages the runtime lifecycle. The runtime holds the Rust thread pool
/// and telemetry configuration. Typically there is one runtime per process,
/// but multiple are allowed.
///
/// The underlying handle uses shared ownership (std::shared_ptr) so that
/// clients and other objects can hold a reference to the runtime and prevent
/// premature destruction.
///
/// Thread-safe: the underlying Rust runtime is internally synchronized.
class Runtime {
public:
    /// Create a new runtime with the given options.
    /// Throws std::runtime_error on failure.
    explicit Runtime(const RuntimeOptions& options);

    /// Get the raw runtime pointer (for passing to other FFI functions).
    TemporalCoreRuntime* get() const noexcept { return handle_.get(); }

    /// Get the shared handle (for other objects that need to share ownership).
    const RuntimeHandle& shared_handle() const noexcept { return handle_; }

    /// Check if the runtime is valid.
    explicit operator bool() const noexcept {
        return handle_ != nullptr;
    }

    /// Free a Rust-owned byte array using this runtime.
    void free_byte_array(const TemporalCoreByteArray* bytes) const {
        if (bytes && handle_) {
            temporal_core_byte_array_free(handle_.get(), bytes);
        }
    }

    // Non-copyable but movable. The shared_ptr inside allows sharing
    // via shared_handle(), but the Runtime wrapper itself is not copied.
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime(Runtime&&) noexcept = default;
    Runtime& operator=(Runtime&&) noexcept = default;

private:
    RuntimeHandle handle_;

    /// Static callback that dispatches to the LogForwardingOptions callback.
    static void log_callback(TemporalCoreForwardedLogLevel level,
                             const TemporalCoreForwardedLog* log);

    /// Stored log forwarding options (kept alive for the lifetime of the runtime).
    std::unique_ptr<LogForwardingOptions> log_forwarding_;
};

}  // namespace temporalio::bridge

#endif  // TEMPORALIO_BRIDGE_RUNTIME_H
