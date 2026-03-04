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

/// OpenTelemetry options (owning, C++ friendly).
struct OpenTelemetryOptions {
    std::string url;
    std::string headers;  // newline-delimited key=value pairs
    uint32_t metric_periodicity_millis = 0;
    TemporalCoreOpenTelemetryMetricTemporality metric_temporality =
        TemporalCoreOpenTelemetryMetricTemporality::Cumulative;
    bool durations_as_seconds = false;
    TemporalCoreOpenTelemetryProtocol protocol =
        TemporalCoreOpenTelemetryProtocol::Grpc;
    std::string histogram_bucket_overrides;
};

/// Prometheus options (owning, C++ friendly).
struct PrometheusOptions {
    std::string bind_address;
    bool counters_total_suffix = false;
    bool unit_suffix = false;
    bool durations_as_seconds = false;
    std::string histogram_bucket_overrides;
};

/// Options for creating a runtime.
struct RuntimeOptions {
    /// Optional log forwarding configuration.
    std::unique_ptr<LogForwardingOptions> log_forwarding;

    /// Optional OpenTelemetry metrics.
    std::unique_ptr<OpenTelemetryOptions> opentelemetry;

    /// Optional Prometheus metrics.
    std::unique_ptr<PrometheusOptions> prometheus;

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

    /// Destructor. Unregisters from log forwarding if enabled.
    ~Runtime();

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

    /// Dispatch a forwarded log to this runtime's callback. Called by the
    /// global log registry. Public so the registry can call it.
    void dispatch_log(TemporalCoreForwardedLogLevel level,
                      const TemporalCoreForwardedLog* log) const;

    // Non-copyable, non-movable (registered in global log registry by pointer).
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime(Runtime&&) = delete;
    Runtime& operator=(Runtime&&) = delete;

private:
    RuntimeHandle handle_;

    /// Static callback that dispatches to all registered runtimes.
    static void log_callback(TemporalCoreForwardedLogLevel level,
                             const TemporalCoreForwardedLog* log);

    /// Stored log forwarding options (kept alive for the lifetime of the runtime).
    std::unique_ptr<LogForwardingOptions> log_forwarding_;
};

}  // namespace temporalio::bridge

#endif  // TEMPORALIO_BRIDGE_RUNTIME_H
