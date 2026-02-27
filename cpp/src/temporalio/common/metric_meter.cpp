#include <temporalio/common/metric_meter.h>

#include <memory>

namespace temporalio::common {

namespace {

// No-op implementations for each metric type.

class NoopCounter : public MetricCounter<std::uint64_t> {
public:
    NoopCounter(std::string name, std::optional<std::string> unit,
                std::optional<std::string> description)
        : name_(std::move(name)),
          unit_(std::move(unit)),
          description_(std::move(description)) {}

    void add(std::uint64_t /*value*/,
             const MetricAttributes& /*tags*/) override {}

    const std::string& name() const override { return name_; }
    const std::optional<std::string>& unit() const override { return unit_; }
    const std::optional<std::string>& description() const override {
        return description_;
    }

private:
    std::string name_;
    std::optional<std::string> unit_;
    std::optional<std::string> description_;
};

class NoopHistogram : public MetricHistogram<std::uint64_t> {
public:
    NoopHistogram(std::string name, std::optional<std::string> unit,
                  std::optional<std::string> description)
        : name_(std::move(name)),
          unit_(std::move(unit)),
          description_(std::move(description)) {}

    void record(std::uint64_t /*value*/,
                const MetricAttributes& /*tags*/) override {}

    const std::string& name() const override { return name_; }
    const std::optional<std::string>& unit() const override { return unit_; }
    const std::optional<std::string>& description() const override {
        return description_;
    }

private:
    std::string name_;
    std::optional<std::string> unit_;
    std::optional<std::string> description_;
};

class NoopGauge : public MetricGauge<std::int64_t> {
public:
    NoopGauge(std::string name, std::optional<std::string> unit,
              std::optional<std::string> description)
        : name_(std::move(name)),
          unit_(std::move(unit)),
          description_(std::move(description)) {}

    void set(std::int64_t /*value*/,
             const MetricAttributes& /*tags*/) override {}

    const std::string& name() const override { return name_; }
    const std::optional<std::string>& unit() const override { return unit_; }
    const std::optional<std::string>& description() const override {
        return description_;
    }

private:
    std::string name_;
    std::optional<std::string> unit_;
    std::optional<std::string> description_;
};

} // namespace

std::unique_ptr<MetricCounter<std::uint64_t>> NoopMetricMeter::create_counter(
    const std::string& name, std::optional<std::string> unit,
    std::optional<std::string> description) {
    return std::make_unique<NoopCounter>(name, std::move(unit),
                                         std::move(description));
}

std::unique_ptr<MetricHistogram<std::uint64_t>>
NoopMetricMeter::create_histogram(const std::string& name,
                                   std::optional<std::string> unit,
                                   std::optional<std::string> description) {
    return std::make_unique<NoopHistogram>(name, std::move(unit),
                                           std::move(description));
}

std::unique_ptr<MetricGauge<std::int64_t>> NoopMetricMeter::create_gauge(
    const std::string& name, std::optional<std::string> unit,
    std::optional<std::string> description) {
    return std::make_unique<NoopGauge>(name, std::move(unit),
                                       std::move(description));
}

std::unique_ptr<MetricMeter> NoopMetricMeter::with_tags(
    MetricAttributes /*tags*/) {
    return std::make_unique<NoopMetricMeter>();
}

} // namespace temporalio::common
