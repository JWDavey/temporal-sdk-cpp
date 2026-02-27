#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "temporalio/extensions/diagnostics/custom_metric_meter.h"

using namespace temporalio::extensions::diagnostics;
using namespace std::chrono_literals;

// Helper to find a tag value by key in a MetricTags (vector<pair>).
static std::string find_tag(const MetricTags& tags, const std::string& key) {
    auto it = std::find_if(tags.begin(), tags.end(),
                           [&](const auto& p) { return p.first == key; });
    if (it != tags.end()) {
        return it->second;
    }
    return {};
}

// ===========================================================================
// Mock implementations for testing
// ===========================================================================
namespace {

class MockCounter : public ICustomMetricCounter {
public:
    void add(int64_t value, const MetricTags& tags) override {
        total += value;
        call_count++;
        last_tags = tags;
    }

    int64_t total = 0;
    int call_count = 0;
    MetricTags last_tags;
};

class MockHistogram : public ICustomMetricHistogram {
public:
    void record(int64_t value, const MetricTags& tags) override {
        values.push_back(value);
        last_tags = tags;
    }

    std::vector<int64_t> values;
    MetricTags last_tags;
};

class MockDurationHistogram : public ICustomMetricDurationHistogram {
public:
    void record(std::chrono::milliseconds value,
                const MetricTags& tags) override {
        durations.push_back(value);
        last_tags = tags;
    }

    std::vector<std::chrono::milliseconds> durations;
    MetricTags last_tags;
};

class MockGauge : public ICustomMetricGauge {
public:
    void set(int64_t value, const MetricTags& tags) override {
        current_value = value;
        last_tags = tags;
    }

    int64_t current_value = 0;
    MetricTags last_tags;
};

// Tracking mock that exposes the last-created instrument via shared_ptr
// so tests can inspect what the adapter delegates.
class TrackingCounter : public ICustomMetricCounter {
public:
    void add(int64_t value, const MetricTags& tags) override {
        total += value;
        call_count++;
        last_tags = tags;
    }
    int64_t total = 0;
    int call_count = 0;
    MetricTags last_tags;
};

class TrackingHistogram : public ICustomMetricHistogram {
public:
    void record(int64_t value, const MetricTags& tags) override {
        values.push_back(value);
        last_tags = tags;
    }
    std::vector<int64_t> values;
    MetricTags last_tags;
};

class TrackingGauge : public ICustomMetricGauge {
public:
    void set(int64_t value, const MetricTags& tags) override {
        current_value = value;
        last_tags = tags;
    }
    int64_t current_value = 0;
    MetricTags last_tags;
};

class MockCustomMetricMeter : public ICustomMetricMeter {
public:
    std::unique_ptr<ICustomMetricCounter> create_counter(
        const std::string& name, const std::optional<std::string>& unit,
        const std::optional<std::string>& description) override {
        last_counter_name = name;
        last_counter_unit = unit;
        last_counter_description = description;
        auto ptr = std::make_unique<TrackingCounter>();
        last_counter = ptr.get();
        return ptr;
    }

    std::unique_ptr<ICustomMetricHistogram> create_histogram(
        const std::string& name, const std::optional<std::string>& unit,
        const std::optional<std::string>& description) override {
        last_histogram_name = name;
        last_histogram_unit = unit;
        auto ptr = std::make_unique<TrackingHistogram>();
        last_histogram = ptr.get();
        return ptr;
    }

    std::unique_ptr<ICustomMetricDurationHistogram>
    create_duration_histogram(
        const std::string& name, const std::optional<std::string>& unit,
        const std::optional<std::string>& description) override {
        last_duration_histogram_name = name;
        return std::make_unique<MockDurationHistogram>();
    }

    std::unique_ptr<ICustomMetricGauge> create_gauge(
        const std::string& name, const std::optional<std::string>& unit,
        const std::optional<std::string>& description) override {
        last_gauge_name = name;
        auto ptr = std::make_unique<TrackingGauge>();
        last_gauge = ptr.get();
        return ptr;
    }

    std::string last_counter_name;
    std::optional<std::string> last_counter_unit;
    std::optional<std::string> last_counter_description;
    std::string last_histogram_name;
    std::optional<std::string> last_histogram_unit;
    std::string last_duration_histogram_name;
    std::string last_gauge_name;

    // Raw pointers to last-created instruments (owned by returned unique_ptr)
    TrackingCounter* last_counter = nullptr;
    TrackingHistogram* last_histogram = nullptr;
    TrackingGauge* last_gauge = nullptr;
};

}  // namespace

// ===========================================================================
// MetricTags tests
// ===========================================================================

TEST(MetricTagsTest, EmptyTags) {
    MetricTags tags;
    EXPECT_TRUE(tags.empty());
}

TEST(MetricTagsTest, WithTags) {
    MetricTags tags = {{"env", "prod"}, {"region", "us-west-2"}};
    EXPECT_EQ(tags.size(), 2u);
    EXPECT_EQ(find_tag(tags, "env"), "prod");
    EXPECT_EQ(find_tag(tags, "region"), "us-west-2");
}

// ===========================================================================
// ICustomMetricCounter tests
// ===========================================================================

TEST(ICustomMetricCounterTest, AddAccumulatesValues) {
    MockCounter counter;
    MetricTags tags = {{"method", "GET"}};

    counter.add(5, tags);
    EXPECT_EQ(counter.total, 5);
    EXPECT_EQ(counter.call_count, 1);

    counter.add(3, tags);
    EXPECT_EQ(counter.total, 8);
    EXPECT_EQ(counter.call_count, 2);
}

TEST(ICustomMetricCounterTest, RecordsTags) {
    MockCounter counter;
    MetricTags tags = {{"status", "200"}};

    counter.add(1, tags);
    EXPECT_EQ(find_tag(counter.last_tags, "status"), "200");
}

TEST(ICustomMetricCounterTest, VirtualDestructorWorks) {
    std::unique_ptr<ICustomMetricCounter> ptr = std::make_unique<MockCounter>();
    EXPECT_NO_THROW(ptr.reset());
}

// ===========================================================================
// ICustomMetricHistogram tests
// ===========================================================================

TEST(ICustomMetricHistogramTest, RecordsValues) {
    MockHistogram histogram;
    MetricTags tags;

    histogram.record(10, tags);
    histogram.record(20, tags);
    histogram.record(30, tags);

    ASSERT_EQ(histogram.values.size(), 3u);
    EXPECT_EQ(histogram.values[0], 10);
    EXPECT_EQ(histogram.values[1], 20);
    EXPECT_EQ(histogram.values[2], 30);
}

TEST(ICustomMetricHistogramTest, VirtualDestructorWorks) {
    std::unique_ptr<ICustomMetricHistogram> ptr =
        std::make_unique<MockHistogram>();
    EXPECT_NO_THROW(ptr.reset());
}

// ===========================================================================
// ICustomMetricDurationHistogram tests
// ===========================================================================

TEST(ICustomMetricDurationHistogramTest, RecordsDurations) {
    MockDurationHistogram histogram;
    MetricTags tags;

    histogram.record(100ms, tags);
    histogram.record(250ms, tags);

    ASSERT_EQ(histogram.durations.size(), 2u);
    EXPECT_EQ(histogram.durations[0], 100ms);
    EXPECT_EQ(histogram.durations[1], 250ms);
}

TEST(ICustomMetricDurationHistogramTest, VirtualDestructorWorks) {
    std::unique_ptr<ICustomMetricDurationHistogram> ptr =
        std::make_unique<MockDurationHistogram>();
    EXPECT_NO_THROW(ptr.reset());
}

// ===========================================================================
// ICustomMetricGauge tests
// ===========================================================================

TEST(ICustomMetricGaugeTest, SetsValue) {
    MockGauge gauge;
    MetricTags tags;

    gauge.set(42, tags);
    EXPECT_EQ(gauge.current_value, 42);

    gauge.set(-5, tags);
    EXPECT_EQ(gauge.current_value, -5);
}

TEST(ICustomMetricGaugeTest, VirtualDestructorWorks) {
    std::unique_ptr<ICustomMetricGauge> ptr = std::make_unique<MockGauge>();
    EXPECT_NO_THROW(ptr.reset());
}

// ===========================================================================
// ICustomMetricMeter tests
// ===========================================================================

TEST(ICustomMetricMeterTest, CreateCounter) {
    MockCustomMetricMeter meter;
    auto counter =
        meter.create_counter("requests", "count", "Total requests");

    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(meter.last_counter_name, "requests");
    EXPECT_EQ(meter.last_counter_unit.value(), "count");
    EXPECT_EQ(meter.last_counter_description.value(), "Total requests");
}

TEST(ICustomMetricMeterTest, CreateCounterNoOptionals) {
    MockCustomMetricMeter meter;
    auto counter = meter.create_counter("requests", std::nullopt, std::nullopt);

    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(meter.last_counter_name, "requests");
    EXPECT_FALSE(meter.last_counter_unit.has_value());
    EXPECT_FALSE(meter.last_counter_description.has_value());
}

TEST(ICustomMetricMeterTest, CreateHistogram) {
    MockCustomMetricMeter meter;
    auto histogram =
        meter.create_histogram("latency", "ms", "Request latency");

    ASSERT_NE(histogram, nullptr);
    EXPECT_EQ(meter.last_histogram_name, "latency");
}

TEST(ICustomMetricMeterTest, CreateDurationHistogram) {
    MockCustomMetricMeter meter;
    auto histogram = meter.create_duration_histogram(
        "task_duration", "ms", "Task execution time");

    ASSERT_NE(histogram, nullptr);
    EXPECT_EQ(meter.last_duration_histogram_name, "task_duration");
}

TEST(ICustomMetricMeterTest, CreateGauge) {
    MockCustomMetricMeter meter;
    auto gauge = meter.create_gauge("active_tasks", std::nullopt,
                                    "Currently active tasks");

    ASSERT_NE(gauge, nullptr);
    EXPECT_EQ(meter.last_gauge_name, "active_tasks");
}

TEST(ICustomMetricMeterTest, VirtualDestructorWorks) {
    std::unique_ptr<ICustomMetricMeter> ptr =
        std::make_unique<MockCustomMetricMeter>();
    EXPECT_NO_THROW(ptr.reset());
}

// ===========================================================================
// CustomMetricMeterOptions tests
// ===========================================================================

TEST(CustomMetricMeterOptionsTest, DefaultValues) {
    CustomMetricMeterOptions opts;
    EXPECT_FALSE(opts.use_float_seconds);
}

TEST(CustomMetricMeterOptionsTest, CustomValues) {
    CustomMetricMeterOptions opts{.use_float_seconds = true};
    EXPECT_TRUE(opts.use_float_seconds);
}

// ===========================================================================
// CustomMetricMeter tests
// ===========================================================================

TEST(CustomMetricMeterTest, ConstructionWithDefaults) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    CustomMetricMeter meter(mock);

    EXPECT_FALSE(meter.options().use_float_seconds);
    // meter() should return the same mock
    EXPECT_EQ(&meter.meter(), mock.get());
}

TEST(CustomMetricMeterTest, ConstructionWithOptions) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    CustomMetricMeter meter(mock, {.use_float_seconds = true});

    EXPECT_TRUE(meter.options().use_float_seconds);
    EXPECT_EQ(&meter.meter(), mock.get());
}

TEST(CustomMetricMeterTest, NullMeterThrows) {
    EXPECT_THROW(CustomMetricMeter(nullptr), std::invalid_argument);
}

// ===========================================================================
// CustomMetricMeterAdapter tests
// ===========================================================================

TEST(CustomMetricMeterAdapterTest, ConstructionDefaults) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    CustomMetricMeterAdapter adapter(mock);
    // Should construct without throwing
}

TEST(CustomMetricMeterAdapterTest, NullMeterThrows) {
    EXPECT_THROW(CustomMetricMeterAdapter(nullptr), std::invalid_argument);
}

TEST(CustomMetricMeterAdapterTest, CreateCounterDelegatesToMock) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    CustomMetricMeterAdapter adapter(mock);

    auto counter = adapter.create_counter("requests", "count", "Total reqs");
    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(counter->name(), "requests");
    EXPECT_EQ(counter->unit().value(), "count");
    EXPECT_EQ(counter->description().value(), "Total reqs");
    EXPECT_EQ(mock->last_counter_name, "requests");
}

TEST(CustomMetricMeterAdapterTest, CounterAddForwardsToUnderlying) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    CustomMetricMeterAdapter adapter(mock);

    auto counter = adapter.create_counter("reqs", std::nullopt, std::nullopt);
    ASSERT_NE(counter, nullptr);
    ASSERT_NE(mock->last_counter, nullptr);

    temporalio::common::MetricAttributes attrs = {{"method", "GET"}};
    counter->add(5, attrs);

    EXPECT_EQ(mock->last_counter->total, 5);
    EXPECT_EQ(mock->last_counter->call_count, 1);
    EXPECT_EQ(find_tag(mock->last_counter->last_tags, "method"), "GET");
}

TEST(CustomMetricMeterAdapterTest, CreateHistogramDelegatesToMock) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    CustomMetricMeterAdapter adapter(mock);

    auto histogram = adapter.create_histogram("latency", "ms", "Latency");
    ASSERT_NE(histogram, nullptr);
    EXPECT_EQ(histogram->name(), "latency");
    EXPECT_EQ(mock->last_histogram_name, "latency");
}

TEST(CustomMetricMeterAdapterTest, HistogramRecordForwardsToUnderlying) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    CustomMetricMeterAdapter adapter(mock);

    auto histogram = adapter.create_histogram("lat", "ms", "Latency");
    ASSERT_NE(mock->last_histogram, nullptr);

    temporalio::common::MetricAttributes attrs;
    histogram->record(42, attrs);

    ASSERT_EQ(mock->last_histogram->values.size(), 1u);
    EXPECT_EQ(mock->last_histogram->values[0], 42);
}

TEST(CustomMetricMeterAdapterTest, CreateGaugeDelegatesToMock) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    CustomMetricMeterAdapter adapter(mock);

    auto gauge = adapter.create_gauge("active", std::nullopt, "Active count");
    ASSERT_NE(gauge, nullptr);
    EXPECT_EQ(gauge->name(), "active");
    EXPECT_EQ(mock->last_gauge_name, "active");
}

TEST(CustomMetricMeterAdapterTest, GaugeSetForwardsToUnderlying) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    CustomMetricMeterAdapter adapter(mock);

    auto gauge = adapter.create_gauge("queue_depth", std::nullopt, std::nullopt);
    ASSERT_NE(mock->last_gauge, nullptr);

    temporalio::common::MetricAttributes attrs;
    gauge->set(100, attrs);

    EXPECT_EQ(mock->last_gauge->current_value, 100);
}

TEST(CustomMetricMeterAdapterTest, HistogramUnitAdjustedForDuration) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    CustomMetricMeterAdapter adapter(mock);

    // "duration" unit should be converted to "ms" (default)
    auto histogram =
        adapter.create_histogram("task_time", "duration", "Task time");
    ASSERT_NE(histogram, nullptr);
    EXPECT_EQ(histogram->unit().value(), "ms");
    // The mock also receives the adjusted unit
    EXPECT_EQ(mock->last_histogram_unit.value(), "ms");
}

TEST(CustomMetricMeterAdapterTest, HistogramUnitAdjustedToSeconds) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    CustomMetricMeterAdapter adapter(mock, {.use_float_seconds = true});

    // With use_float_seconds, "duration" should become "s"
    auto histogram =
        adapter.create_histogram("task_time", "duration", "Task time");
    ASSERT_NE(histogram, nullptr);
    EXPECT_EQ(histogram->unit().value(), "s");
    EXPECT_EQ(mock->last_histogram_unit.value(), "s");
}

TEST(CustomMetricMeterAdapterTest, HistogramNonDurationUnitUnchanged) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    CustomMetricMeterAdapter adapter(mock);

    auto histogram =
        adapter.create_histogram("size", "bytes", "Message size");
    ASSERT_NE(histogram, nullptr);
    EXPECT_EQ(histogram->unit().value(), "bytes");
    EXPECT_EQ(mock->last_histogram_unit.value(), "bytes");
}

TEST(CustomMetricMeterAdapterTest, WithTagsCreatesNewAdapter) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    CustomMetricMeterAdapter adapter(mock);

    temporalio::common::MetricAttributes tags = {{"env", "prod"}};
    auto tagged = adapter.with_tags(tags);

    ASSERT_NE(tagged, nullptr);
    EXPECT_NE(tagged.get(), &adapter);
}

TEST(CustomMetricMeterAdapterTest, WithTagsMergesBaseTags) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    temporalio::common::MetricAttributes base_tags = {{"env", "test"}};
    CustomMetricMeterAdapter adapter(mock, {}, base_tags);

    auto counter = adapter.create_counter("reqs", std::nullopt, std::nullopt);
    ASSERT_NE(mock->last_counter, nullptr);

    // Add with extra tags -- both base and extra should be merged
    temporalio::common::MetricAttributes extra = {{"method", "POST"}};
    counter->add(1, extra);

    // The underlying counter should see merged tags
    EXPECT_EQ(find_tag(mock->last_counter->last_tags, "env"), "test");
    EXPECT_EQ(find_tag(mock->last_counter->last_tags, "method"), "POST");
}

TEST(CustomMetricMeterAdapterTest, IsMetricMeter) {
    auto mock = std::make_shared<MockCustomMetricMeter>();
    auto adapter = std::make_unique<CustomMetricMeterAdapter>(mock);

    temporalio::common::MetricMeter* base = adapter.get();
    EXPECT_NE(base, nullptr);
}
