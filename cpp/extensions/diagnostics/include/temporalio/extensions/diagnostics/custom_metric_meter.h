#pragma once

/// @file custom_metric_meter.h
/// @brief Custom metric meter adapter for the Temporal C++ SDK.
///
/// Provides an interface for recording Temporal SDK internal metrics
/// (counters, histograms, gauges) to a user-provided metrics backend.
/// This mirrors the C# CustomMetricMeter in
/// Temporalio.Extensions.DiagnosticSource.

#include <temporalio/common/metric_meter.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace temporalio::extensions::diagnostics {

/// A set of metric tags (key-value pairs).
/// Uses the same type as common::MetricAttributes for consistency.
using MetricTags = common::MetricAttributes;

/// Interface for a custom counter metric.
class ICustomMetricCounter {
public:
    virtual ~ICustomMetricCounter() = default;

    /// Add a value to the counter.
    /// @param value Value to add (must be >= 0).
    /// @param tags Tags for this measurement.
    virtual void add(int64_t value, const MetricTags& tags) = 0;
};

/// Interface for a custom histogram metric.
class ICustomMetricHistogram {
public:
    virtual ~ICustomMetricHistogram() = default;

    /// Record a value in the histogram.
    /// @param value Value to record.
    /// @param tags Tags for this measurement.
    virtual void record(int64_t value, const MetricTags& tags) = 0;
};

/// Interface for a custom histogram metric with duration values.
class ICustomMetricDurationHistogram {
public:
    virtual ~ICustomMetricDurationHistogram() = default;

    /// Record a duration in the histogram.
    /// @param value Duration to record.
    /// @param tags Tags for this measurement.
    virtual void record(std::chrono::milliseconds value,
                        const MetricTags& tags) = 0;
};

/// Interface for a custom gauge metric.
class ICustomMetricGauge {
public:
    virtual ~ICustomMetricGauge() = default;

    /// Set the gauge value.
    /// @param value New gauge value.
    /// @param tags Tags for this measurement.
    virtual void set(int64_t value, const MetricTags& tags) = 0;
};

/// Interface for creating custom metric instruments.
///
/// Implement this interface to bridge Temporal SDK metrics to your
/// preferred metrics backend (e.g., Prometheus, StatsD, etc.).
class ICustomMetricMeter {
public:
    virtual ~ICustomMetricMeter() = default;

    /// Create a counter.
    /// @param name Metric name.
    /// @param unit Optional unit (e.g., "requests", "bytes").
    /// @param description Optional human-readable description.
    /// @return A counter instrument.
    virtual std::unique_ptr<ICustomMetricCounter> create_counter(
        const std::string& name,
        const std::optional<std::string>& unit,
        const std::optional<std::string>& description) = 0;

    /// Create a histogram.
    /// @param name Metric name.
    /// @param unit Optional unit (e.g., "ms", "bytes").
    /// @param description Optional human-readable description.
    /// @return A histogram instrument.
    virtual std::unique_ptr<ICustomMetricHistogram> create_histogram(
        const std::string& name,
        const std::optional<std::string>& unit,
        const std::optional<std::string>& description) = 0;

    /// Create a duration histogram.
    /// @param name Metric name.
    /// @param unit Optional unit (defaults to "ms").
    /// @param description Optional human-readable description.
    /// @return A duration histogram instrument.
    virtual std::unique_ptr<ICustomMetricDurationHistogram>
    create_duration_histogram(
        const std::string& name,
        const std::optional<std::string>& unit,
        const std::optional<std::string>& description) = 0;

    /// Create a gauge.
    /// @param name Metric name.
    /// @param unit Optional unit.
    /// @param description Optional human-readable description.
    /// @return A gauge instrument.
    virtual std::unique_ptr<ICustomMetricGauge> create_gauge(
        const std::string& name,
        const std::optional<std::string>& unit,
        const std::optional<std::string>& description) = 0;
};

/// Options for CustomMetricMeter.
struct CustomMetricMeterOptions {
    /// Whether to report histograms as float seconds instead of
    /// long milliseconds. Default: false (milliseconds).
    bool use_float_seconds{false};
};

/// Adapter that bridges the Temporal SDK's internal metric system
/// to a user-provided ICustomMetricMeter implementation.
///
/// Set this on the runtime's MetricsOptions to record SDK metrics.
class CustomMetricMeter {
public:
    /// Create with a custom meter implementation.
    /// @param meter The custom meter to delegate to.
    /// @param options Options for the meter.
    explicit CustomMetricMeter(
        std::shared_ptr<ICustomMetricMeter> meter,
        CustomMetricMeterOptions options = {});

    ~CustomMetricMeter();

    /// Get the underlying custom meter.
    ICustomMetricMeter& meter() const noexcept { return *meter_; }

    /// Get the options.
    const CustomMetricMeterOptions& options() const noexcept {
        return options_;
    }

private:
    std::shared_ptr<ICustomMetricMeter> meter_;
    CustomMetricMeterOptions options_;
};

/// Adapter that implements the SDK's common::MetricMeter interface
/// by delegating to an ICustomMetricMeter.
///
/// This is the bridge that connects the SDK's internal metric system
/// to user-provided metric backends. Metrics created from this meter
/// will convert common::MetricAttributes to MetricTags and forward
/// all operations to the underlying ICustomMetricMeter.
///
/// Mirrors the C# CustomMetricMeter inner classes that wrap
/// System.Diagnostics.Metrics instruments.
class CustomMetricMeterAdapter : public common::MetricMeter {
public:
    /// Create an adapter with the given custom meter and options.
    /// @param meter The custom meter to delegate to.
    /// @param options Options controlling unit conversion.
    /// @param base_tags Tags to apply to all metrics created from this meter.
    explicit CustomMetricMeterAdapter(
        std::shared_ptr<ICustomMetricMeter> meter,
        CustomMetricMeterOptions options = {},
        common::MetricAttributes base_tags = {});

    std::unique_ptr<common::MetricCounter<std::uint64_t>> create_counter(
        const std::string& name,
        std::optional<std::string> unit = std::nullopt,
        std::optional<std::string> description = std::nullopt) override;

    std::unique_ptr<common::MetricHistogram<std::uint64_t>> create_histogram(
        const std::string& name,
        std::optional<std::string> unit = std::nullopt,
        std::optional<std::string> description = std::nullopt) override;

    std::unique_ptr<common::MetricGauge<std::int64_t>> create_gauge(
        const std::string& name,
        std::optional<std::string> unit = std::nullopt,
        std::optional<std::string> description = std::nullopt) override;

    std::unique_ptr<common::MetricMeter> with_tags(
        common::MetricAttributes tags) override;

private:
    /// Convert common::MetricAttributes to MetricTags, merging base tags.
    MetricTags to_metric_tags(const common::MetricAttributes& extra) const;

    /// Adjust histogram unit based on options (e.g., "duration" -> "ms"/"s").
    std::optional<std::string> adjust_histogram_unit(
        const std::optional<std::string>& unit) const;

    std::shared_ptr<ICustomMetricMeter> meter_;
    CustomMetricMeterOptions options_;
    common::MetricAttributes base_tags_;
};

}  // namespace temporalio::extensions::diagnostics
