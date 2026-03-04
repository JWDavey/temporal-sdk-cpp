#include "temporalio/extensions/diagnostics/custom_metric_meter.h"

#include <stdexcept>
#include <utility>

namespace temporalio::extensions::diagnostics {

// ── CustomMetricMeter ───────────────────────────────────────────────────────

CustomMetricMeter::CustomMetricMeter(
    std::shared_ptr<ICustomMetricMeter> meter,
    CustomMetricMeterOptions options)
    : meter_(std::move(meter)), options_(std::move(options)) {
    if (!meter_) {
        throw std::invalid_argument(
            "CustomMetricMeter requires a non-null ICustomMetricMeter");
    }
}

CustomMetricMeter::~CustomMetricMeter() = default;

// ── Adapter counter ─────────────────────────────────────────────────────────

namespace {

class AdapterCounter : public common::MetricCounter<std::uint64_t> {
public:
    AdapterCounter(std::unique_ptr<ICustomMetricCounter> counter,
                   std::string name,
                   std::optional<std::string> unit,
                   std::optional<std::string> description,
                   const common::MetricAttributes& base_tags)
        : counter_(std::move(counter)),
          name_(std::move(name)),
          unit_(std::move(unit)),
          description_(std::move(description)),
          base_tags_(base_tags) {}

    void add(std::uint64_t value,
             const common::MetricAttributes& tags) override {
        counter_->add(static_cast<int64_t>(value), merge_tags(tags));
    }

    const std::string& name() const override { return name_; }
    const std::optional<std::string>& unit() const override { return unit_; }
    const std::optional<std::string>& description() const override {
        return description_;
    }

private:
    MetricTags merge_tags(const common::MetricAttributes& extra) const {
        MetricTags result = base_tags_;
        result.insert(result.end(), extra.begin(), extra.end());
        return result;
    }

    std::unique_ptr<ICustomMetricCounter> counter_;
    std::string name_;
    std::optional<std::string> unit_;
    std::optional<std::string> description_;
    common::MetricAttributes base_tags_;
};

// ── Adapter histogram ───────────────────────────────────────────────────────

class AdapterHistogram : public common::MetricHistogram<std::uint64_t> {
public:
    AdapterHistogram(std::unique_ptr<ICustomMetricHistogram> histogram,
                     std::string name,
                     std::optional<std::string> unit,
                     std::optional<std::string> description,
                     const common::MetricAttributes& base_tags)
        : histogram_(std::move(histogram)),
          name_(std::move(name)),
          unit_(std::move(unit)),
          description_(std::move(description)),
          base_tags_(base_tags) {}

    void record(std::uint64_t value,
                const common::MetricAttributes& tags) override {
        histogram_->record(static_cast<int64_t>(value), merge_tags(tags));
    }

    const std::string& name() const override { return name_; }
    const std::optional<std::string>& unit() const override { return unit_; }
    const std::optional<std::string>& description() const override {
        return description_;
    }

private:
    MetricTags merge_tags(const common::MetricAttributes& extra) const {
        MetricTags result = base_tags_;
        result.insert(result.end(), extra.begin(), extra.end());
        return result;
    }

    std::unique_ptr<ICustomMetricHistogram> histogram_;
    std::string name_;
    std::optional<std::string> unit_;
    std::optional<std::string> description_;
    common::MetricAttributes base_tags_;
};

// ── Adapter gauge ───────────────────────────────────────────────────────────

class AdapterGauge : public common::MetricGauge<std::int64_t> {
public:
    AdapterGauge(std::unique_ptr<ICustomMetricGauge> gauge,
                 std::string name,
                 std::optional<std::string> unit,
                 std::optional<std::string> description,
                 const common::MetricAttributes& base_tags)
        : gauge_(std::move(gauge)),
          name_(std::move(name)),
          unit_(std::move(unit)),
          description_(std::move(description)),
          base_tags_(base_tags) {}

    void set(std::int64_t value,
             const common::MetricAttributes& tags) override {
        gauge_->set(value, merge_tags(tags));
    }

    const std::string& name() const override { return name_; }
    const std::optional<std::string>& unit() const override { return unit_; }
    const std::optional<std::string>& description() const override {
        return description_;
    }

private:
    MetricTags merge_tags(const common::MetricAttributes& extra) const {
        MetricTags result = base_tags_;
        result.insert(result.end(), extra.begin(), extra.end());
        return result;
    }

    std::unique_ptr<ICustomMetricGauge> gauge_;
    std::string name_;
    std::optional<std::string> unit_;
    std::optional<std::string> description_;
    common::MetricAttributes base_tags_;
};

}  // namespace

// ── CustomMetricMeterAdapter ────────────────────────────────────────────────

CustomMetricMeterAdapter::CustomMetricMeterAdapter(
    std::shared_ptr<ICustomMetricMeter> meter,
    CustomMetricMeterOptions options,
    common::MetricAttributes base_tags)
    : meter_(std::move(meter)),
      options_(std::move(options)),
      base_tags_(std::move(base_tags)) {
    if (!meter_) {
        throw std::invalid_argument(
            "CustomMetricMeterAdapter requires a non-null ICustomMetricMeter");
    }
}

std::unique_ptr<common::MetricCounter<std::uint64_t>>
CustomMetricMeterAdapter::create_counter(
    const std::string& name,
    std::optional<std::string> unit,
    std::optional<std::string> description) {
    auto counter = meter_->create_counter(name, unit, description);
    return std::make_unique<AdapterCounter>(
        std::move(counter), name, std::move(unit), std::move(description),
        base_tags_);
}

std::unique_ptr<common::MetricHistogram<std::uint64_t>>
CustomMetricMeterAdapter::create_histogram(
    const std::string& name,
    std::optional<std::string> unit,
    std::optional<std::string> description) {
    // Adjust unit for histograms (e.g., "duration" -> "ms" or "s")
    auto adjusted_unit = adjust_histogram_unit(unit);
    auto histogram =
        meter_->create_histogram(name, adjusted_unit, description);
    return std::make_unique<AdapterHistogram>(
        std::move(histogram), name, std::move(adjusted_unit),
        std::move(description), base_tags_);
}

std::unique_ptr<common::MetricGauge<std::int64_t>>
CustomMetricMeterAdapter::create_gauge(
    const std::string& name,
    std::optional<std::string> unit,
    std::optional<std::string> description) {
    auto gauge = meter_->create_gauge(name, unit, description);
    return std::make_unique<AdapterGauge>(
        std::move(gauge), name, std::move(unit), std::move(description),
        base_tags_);
}

std::unique_ptr<common::MetricMeter> CustomMetricMeterAdapter::with_tags(
    common::MetricAttributes tags) {
    // Merge existing base tags with new tags
    common::MetricAttributes merged = base_tags_;
    merged.insert(merged.end(), tags.begin(), tags.end());
    return std::make_unique<CustomMetricMeterAdapter>(
        meter_, options_, std::move(merged));
}

MetricTags CustomMetricMeterAdapter::to_metric_tags(
    const common::MetricAttributes& extra) const {
    MetricTags result = base_tags_;
    result.insert(result.end(), extra.begin(), extra.end());
    return result;
}

std::optional<std::string> CustomMetricMeterAdapter::adjust_histogram_unit(
    const std::optional<std::string>& unit) const {
    // Mirrors C# behavior: if unit is "duration", change to "ms" or "s"
    // depending on options.
    if (unit.has_value() && *unit == "duration") {
        return options_.use_float_seconds ? "s" : "ms";
    }
    return unit;
}

}  // namespace temporalio::extensions::diagnostics
