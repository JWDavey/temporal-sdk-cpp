#include "temporalio/bridge/runtime.h"

#include <algorithm>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "temporalio/bridge/call_scope.h"

namespace temporalio::bridge {

namespace {

// Global registry of active runtimes with log forwarding enabled.
// The Rust log callback is a plain C function pointer that cannot carry
// user_data, so we use a global set of active Runtime instances to
// dispatch log messages. Thread-safe via mutex.
struct LogForwardingRegistry {
    std::mutex mu;
    std::vector<Runtime*> runtimes;

    void add(Runtime* rt) {
        std::lock_guard lock(mu);
        runtimes.push_back(rt);
    }

    void remove(Runtime* rt) {
        std::lock_guard lock(mu);
        runtimes.erase(
            std::remove(runtimes.begin(), runtimes.end(), rt),
            runtimes.end());
    }

    void dispatch(TemporalCoreForwardedLogLevel level,
                  const TemporalCoreForwardedLog* log) {
        std::lock_guard lock(mu);
        for (auto* rt : runtimes) {
            rt->dispatch_log(level, log);
        }
    }
};

LogForwardingRegistry& log_registry() {
    static LogForwardingRegistry registry;
    return registry;
}

}  // namespace

Runtime::Runtime(const RuntimeOptions& options) {
    CallScope scope;

    // Build telemetry options
    TemporalCoreLoggingOptions logging_opts{};
    bool has_logging = false;

    if (options.log_forwarding) {
        logging_opts.filter = scope.byte_array(options.log_forwarding->filter);
        has_logging = true;
    }

    TemporalCoreMetricsOptions metrics_opts{};
    bool has_metrics = options.opentelemetry || options.prometheus ||
                       options.custom_meter ||
                       !options.global_tags.empty() ||
                       !options.metric_prefix.empty();

    if (has_metrics) {
        if (options.opentelemetry) {
            TemporalCoreOpenTelemetryOptions otel_interop{};
            otel_interop.url = scope.byte_array(options.opentelemetry->url);
            otel_interop.headers = scope.byte_array(options.opentelemetry->headers);
            otel_interop.metric_periodicity_millis =
                options.opentelemetry->metric_periodicity_millis;
            otel_interop.metric_temporality =
                options.opentelemetry->metric_temporality;
            otel_interop.durations_as_seconds =
                options.opentelemetry->durations_as_seconds;
            otel_interop.protocol = options.opentelemetry->protocol;
            otel_interop.histogram_bucket_overrides =
                scope.byte_array(options.opentelemetry->histogram_bucket_overrides);
            metrics_opts.opentelemetry = scope.alloc(otel_interop);
        }
        if (options.prometheus) {
            TemporalCorePrometheusOptions prom_interop{};
            prom_interop.bind_address =
                scope.byte_array(options.prometheus->bind_address);
            prom_interop.counters_total_suffix =
                options.prometheus->counters_total_suffix;
            prom_interop.unit_suffix =
                options.prometheus->unit_suffix;
            prom_interop.durations_as_seconds =
                options.prometheus->durations_as_seconds;
            prom_interop.histogram_bucket_overrides =
                scope.byte_array(options.prometheus->histogram_bucket_overrides);
            metrics_opts.prometheus = scope.alloc(prom_interop);
        }
        metrics_opts.custom_meter = options.custom_meter;
        metrics_opts.attach_service_name = options.attach_service_name;
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

    // Store log forwarding options and wire up the static callback
    if (options.log_forwarding && options.log_forwarding->callback) {
        log_forwarding_ = std::make_unique<LogForwardingOptions>(
            LogForwardingOptions{
                options.log_forwarding->filter,
                options.log_forwarding->callback,
            });
        if (runtime_opts.telemetry && runtime_opts.telemetry->logging) {
            auto* logging_ptr =
                const_cast<TemporalCoreLoggingOptions*>(runtime_opts.telemetry->logging);
            logging_ptr->forward_to = &Runtime::log_callback;
        }
    }

    auto result = temporal_core_runtime_new(&runtime_opts);

    if (result.fail != nullptr) {
        std::string message;
        if (result.fail->data && result.fail->size > 0) {
            message.assign(reinterpret_cast<const char*>(result.fail->data),
                           result.fail->size);
        }
        // Free the error byte array and runtime. The C# SDK always calls
        // both free functions unconditionally (the Rust API handles null
        // runtime gracefully, and result.runtime is always non-null when
        // fail is non-null because the runtime is created first before
        // telemetry setup can fail).
        temporal_core_byte_array_free(result.runtime, result.fail);
        temporal_core_runtime_free(result.runtime);
        throw std::runtime_error("Failed to create Temporal runtime: " +
                                 message);
    }

    handle_ = make_shared_handle<TemporalCoreRuntime,
                                    temporal_core_runtime_free>(result.runtime);

    // Register for log dispatch if forwarding is enabled
    if (log_forwarding_) {
        log_registry().add(this);
    }
}

Runtime::~Runtime() {
    if (log_forwarding_) {
        log_registry().remove(this);
    }
}

void Runtime::dispatch_log(TemporalCoreForwardedLogLevel level,
                           const TemporalCoreForwardedLog* log) const {
    if (!log_forwarding_ || !log_forwarding_->callback) {
        return;
    }
    auto target = byte_array_ref_to_string_view(
        temporal_core_forwarded_log_target(log));
    auto message = byte_array_ref_to_string_view(
        temporal_core_forwarded_log_message(log));
    auto timestamp = temporal_core_forwarded_log_timestamp_millis(log);

    log_forwarding_->callback(level, target, message, timestamp);
}

void Runtime::log_callback(TemporalCoreForwardedLogLevel level,
                           const TemporalCoreForwardedLog* log) {
    log_registry().dispatch(level, log);
}

}  // namespace temporalio::bridge
