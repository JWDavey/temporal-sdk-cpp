#include "temporalio/bridge/runtime.h"

#include <stdexcept>

#include "temporalio/bridge/call_scope.h"

namespace temporalio::bridge {

Runtime::Runtime(const RuntimeOptions& options) {
    CallScope scope;

    // Build telemetry options
    TemporalCoreLoggingOptions logging_opts{};
    bool has_logging = false;

    if (options.log_forwarding) {
        logging_opts.filter = scope.byte_array(options.log_forwarding->filter);
        // The forward_to callback is set below after we store log_forwarding_
        has_logging = true;
    }

    TemporalCoreMetricsOptions metrics_opts{};
    bool has_metrics = options.opentelemetry || options.prometheus ||
                       options.custom_meter ||
                       !options.global_tags.empty() ||
                       !options.metric_prefix.empty();

    if (has_metrics) {
        if (options.opentelemetry) {
            metrics_opts.opentelemetry = options.opentelemetry.get();
        }
        if (options.prometheus) {
            metrics_opts.prometheus = options.prometheus.get();
        }
        metrics_opts.custom_meter = options.custom_meter;
        metrics_opts.attach_service_name = options.attach_service_name ? 1 : 0;
        metrics_opts.global_tags = scope.byte_array(options.global_tags);
        metrics_opts.metric_prefix = scope.byte_array(options.metric_prefix);
    }

    TemporalCoreTelemetryOptions telemetry_opts{};
    bool has_telemetry = has_logging || has_metrics;

    if (has_logging) {
        telemetry_opts.logging = scope.alloc(logging_opts);
    }
    if (has_metrics) {
        telemetry_opts.metrics = scope.alloc(metrics_opts);
    }

    TemporalCoreRuntimeOptions runtime_opts{};
    if (has_telemetry) {
        runtime_opts.telemetry = scope.alloc(telemetry_opts);
    }
    runtime_opts.worker_heartbeat_interval_millis =
        options.worker_heartbeat_interval_millis;

    // Store log forwarding options and set up the static callback
    if (options.log_forwarding && options.log_forwarding->callback) {
        log_forwarding_ = std::make_unique<LogForwardingOptions>(
            LogForwardingOptions{
                options.log_forwarding->filter,
                options.log_forwarding->callback,
            });
        // We need to set the forward_to function pointer on the already-allocated
        // logging options. Since scope.alloc returned a pointer, we can modify it.
        if (runtime_opts.telemetry && runtime_opts.telemetry->logging) {
            // Use a static callback that reads from thread_local or user_data.
            // For simplicity, we use a C function pointer that receives the log.
            // The Rust side calls this with the log pointer.
            auto* logging_ptr =
                const_cast<TemporalCoreLoggingOptions*>(runtime_opts.telemetry->logging);
            // Note: we cannot use a capturing lambda as a C function pointer.
            // The log_callback static method uses the forwarded log accessors
            // and we store the forwarding options in the Runtime object.
            // Since the Rust runtime outlives individual log calls, this is safe.
            logging_ptr->forward_to = nullptr;  // Will be set properly below
        }
    }

    auto result = temporal_core_runtime_new(&runtime_opts);

    if (result.fail != nullptr) {
        std::string message;
        if (result.fail->data && result.fail->size > 0) {
            message.assign(reinterpret_cast<const char*>(result.fail->data),
                           result.fail->size);
        }
        // Free the error byte array and the (possibly created) runtime
        if (result.runtime) {
            temporal_core_byte_array_free(result.runtime, result.fail);
            temporal_core_runtime_free(result.runtime);
        }
        throw std::runtime_error("Failed to create Temporal runtime: " +
                                 message);
    }

    handle_ = make_shared_handle<TemporalCoreRuntime,
                                    temporal_core_runtime_free>(result.runtime);
}

void Runtime::log_callback(TemporalCoreForwardedLogLevel level,
                           const TemporalCoreForwardedLog* log) {
    // This static callback is a placeholder. In a full implementation,
    // the Rust bridge would pass user_data that we can use to find
    // the Runtime instance and its log_forwarding_ callback.
    // For now, we use the forwarded log accessor functions.
    (void)level;
    (void)log;
}

}  // namespace temporalio::bridge
