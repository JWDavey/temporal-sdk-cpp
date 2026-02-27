#include <temporalio/common/metric_meter.h>
#include <temporalio/runtime/temporal_runtime.h>

#include <memory>
#include <mutex>

namespace temporalio::runtime {

// ── Impl (pimpl) ───────────────────────────────────────────────────────────

struct TemporalRuntime::Impl {
    // TODO: Hold bridge::RuntimeHandle once bridge layer is wired up.
    // For now, just hold the noop metric meter.
    std::unique_ptr<common::MetricMeter> metric_meter;

    Impl() : metric_meter(std::make_unique<common::NoopMetricMeter>()) {}
};

// ── TemporalRuntime ────────────────────────────────────────────────────────

TemporalRuntime::TemporalRuntime(TemporalRuntimeOptions options)
    : impl_(std::make_unique<Impl>()), options_(std::move(options)) {
    // TODO: Initialize the Rust bridge runtime via:
    //   bridge::RuntimeHandle handle = bridge::create_runtime(options_);
    // and store it in impl_.
}

TemporalRuntime::~TemporalRuntime() = default;

TemporalRuntime::TemporalRuntime(TemporalRuntime&&) noexcept = default;
TemporalRuntime& TemporalRuntime::operator=(TemporalRuntime&&) noexcept =
    default;

std::shared_ptr<TemporalRuntime> TemporalRuntime::default_instance() {
    static std::shared_ptr<TemporalRuntime> instance = [] {
        return std::make_shared<TemporalRuntime>(TemporalRuntimeOptions{});
    }();
    return instance;
}

common::MetricMeter& TemporalRuntime::metric_meter() {
    return *impl_->metric_meter;
}

} // namespace temporalio::runtime
