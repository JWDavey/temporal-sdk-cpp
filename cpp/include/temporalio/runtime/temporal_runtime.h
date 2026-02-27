#pragma once

/// @file temporal_runtime.h
/// @brief TemporalRuntime - holds Rust runtime handle and telemetry configuration.

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace temporalio::common {
class MetricMeter;
} // namespace temporalio::common

namespace temporalio::runtime {

/// Telemetry filter options (log level filtering).
struct TelemetryFilterOptions {
    /// Core logging filter level. Default: "WARN".
    std::string core_level{"WARN"};

    /// Additional filter directives (e.g., "temporalio=DEBUG").
    std::vector<std::string> additional_directives{};
};

/// Log forwarding options.
struct LogForwardingOptions {
    /// Log level for forwarding.
    std::string level{"WARN"};
};

/// Logging options for a runtime.
struct LoggingOptions {
    /// Filter options.
    TelemetryFilterOptions filter{};

    /// Log forwarding options. If not set, logs are not forwarded.
    std::optional<LogForwardingOptions> forwarding{};
};

/// OpenTelemetry metric temporality.
enum class OpenTelemetryMetricTemporality : int {
    kCumulative = 0,
    kDelta = 1,
};

/// OpenTelemetry protocol.
enum class OpenTelemetryProtocol : int {
    kGrpc = 0,
    kHttp = 1,
};

/// OpenTelemetry metrics options.
struct OpenTelemetryOptions {
    /// Endpoint URL for the OpenTelemetry collector.
    std::string url{};

    /// Headers to include with requests.
    std::vector<std::pair<std::string, std::string>> headers{};

    /// Metric temporality.
    OpenTelemetryMetricTemporality metric_temporality{
        OpenTelemetryMetricTemporality::kCumulative};

    /// Protocol to use.
    OpenTelemetryProtocol protocol{OpenTelemetryProtocol::kGrpc};

    /// Metric export interval.
    std::optional<std::chrono::milliseconds> metric_periodicity{};

    /// Whether to use seconds for durations.
    bool durations_as_seconds{false};
};

/// Prometheus metrics options.
struct PrometheusOptions {
    /// Address to bind the Prometheus HTTP exporter to.
    std::string bind_address{};

    /// Whether histogram counters share buckets.
    bool counters_total_suffix{false};

    /// Whether unit suffixes are applied.
    bool unit_suffix{false};

    /// Whether to use seconds for durations.
    bool durations_as_seconds{false};
};

/// Metrics options for a runtime. Exactly one of prometheus, opentelemetry,
/// or custom_metric_meter should be set.
struct MetricsOptions {
    /// Prometheus metrics options (optional).
    std::optional<PrometheusOptions> prometheus{};

    /// OpenTelemetry metrics options (optional).
    std::optional<OpenTelemetryOptions> opentelemetry{};

    /// Custom metric meter for user-provided implementations (optional).
    std::shared_ptr<common::MetricMeter> custom_metric_meter{};

    /// Whether the service name is attached to metrics. Default: true.
    bool attach_service_name{true};

    /// Global tags to put on every metric.
    std::vector<std::pair<std::string, std::string>> global_tags{};

    /// Metric prefix for internal Temporal metrics. Default: "temporal_".
    std::optional<std::string> metric_prefix{};
};

/// Telemetry options for a runtime.
struct TelemetryOptions {
    /// Logging options.
    std::optional<LoggingOptions> logging{LoggingOptions{}};

    /// Metrics options.
    std::optional<MetricsOptions> metrics{};
};

/// Options for creating a TemporalRuntime.
struct TemporalRuntimeOptions {
    /// Telemetry options.
    TelemetryOptions telemetry{};

    /// Worker heartbeat interval.
    std::optional<std::chrono::milliseconds> worker_heartbeat_interval{
        std::chrono::seconds{60}};
};

/// Runtime for the Temporal SDK.
///
/// This runtime carries the internal core engine and telemetry options.
/// All connections/clients created using it, and any workers created from
/// them, will be associated with the runtime.
///
/// This is internally reference-counted via std::shared_ptr.
class TemporalRuntime {
public:
    /// Create a new runtime with the given options.
    /// This creates an entirely new thread pool and runtime in the Core backend.
    explicit TemporalRuntime(TemporalRuntimeOptions options = {});

    ~TemporalRuntime();

    // Non-copyable, movable
    TemporalRuntime(const TemporalRuntime&) = delete;
    TemporalRuntime& operator=(const TemporalRuntime&) = delete;
    TemporalRuntime(TemporalRuntime&&) noexcept;
    TemporalRuntime& operator=(TemporalRuntime&&) noexcept;

    /// Get or create the default runtime (lazily initialized).
    static std::shared_ptr<TemporalRuntime> default_instance();

    /// Get the metric meter associated with this runtime.
    common::MetricMeter& metric_meter();

    /// Get the options this runtime was created with.
    const TemporalRuntimeOptions& options() const noexcept { return options_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    TemporalRuntimeOptions options_;
};

} // namespace temporalio::runtime
