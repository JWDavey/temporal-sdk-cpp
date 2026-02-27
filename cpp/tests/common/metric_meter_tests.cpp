#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "temporalio/common/metric_meter.h"

using namespace temporalio::common;

// ===========================================================================
// NoopMetricMeter tests
// ===========================================================================

TEST(NoopMetricMeterTest, CreateCounter) {
    NoopMetricMeter meter;
    auto counter = meter.create_counter("requests");
    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(counter->name(), "requests");
    EXPECT_FALSE(counter->unit().has_value());
    EXPECT_FALSE(counter->description().has_value());
}

TEST(NoopMetricMeterTest, CreateCounterWithUnitAndDescription) {
    NoopMetricMeter meter;
    auto counter = meter.create_counter("request_size", "bytes",
                                        "Size of incoming requests");
    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(counter->name(), "request_size");
    EXPECT_EQ(counter->unit().value(), "bytes");
    EXPECT_EQ(counter->description().value(), "Size of incoming requests");
}

TEST(NoopMetricMeterTest, CounterAddDoesNotCrash) {
    NoopMetricMeter meter;
    auto counter = meter.create_counter("requests");
    EXPECT_NO_THROW(counter->add(1));
    EXPECT_NO_THROW(counter->add(100));
    EXPECT_NO_THROW(counter->add(0));
}

TEST(NoopMetricMeterTest, CounterAddWithTagsDoesNotCrash) {
    NoopMetricMeter meter;
    auto counter = meter.create_counter("requests");
    MetricAttributes tags = {{"method", "GET"}, {"path", "/api/v1"}};
    EXPECT_NO_THROW(counter->add(1, tags));
}

TEST(NoopMetricMeterTest, CreateHistogram) {
    NoopMetricMeter meter;
    auto histogram = meter.create_histogram("latency");
    ASSERT_NE(histogram, nullptr);
    EXPECT_EQ(histogram->name(), "latency");
    EXPECT_FALSE(histogram->unit().has_value());
    EXPECT_FALSE(histogram->description().has_value());
}

TEST(NoopMetricMeterTest, CreateHistogramWithUnitAndDescription) {
    NoopMetricMeter meter;
    auto histogram =
        meter.create_histogram("latency", "ms", "Request latency");
    ASSERT_NE(histogram, nullptr);
    EXPECT_EQ(histogram->name(), "latency");
    EXPECT_EQ(histogram->unit().value(), "ms");
    EXPECT_EQ(histogram->description().value(), "Request latency");
}

TEST(NoopMetricMeterTest, HistogramRecordDoesNotCrash) {
    NoopMetricMeter meter;
    auto histogram = meter.create_histogram("latency");
    EXPECT_NO_THROW(histogram->record(42));
    EXPECT_NO_THROW(histogram->record(0));
}

TEST(NoopMetricMeterTest, HistogramRecordWithTagsDoesNotCrash) {
    NoopMetricMeter meter;
    auto histogram = meter.create_histogram("latency");
    MetricAttributes tags = {{"operation", "start_workflow"}};
    EXPECT_NO_THROW(histogram->record(150, tags));
}

TEST(NoopMetricMeterTest, CreateGauge) {
    NoopMetricMeter meter;
    auto gauge = meter.create_gauge("active_workflows");
    ASSERT_NE(gauge, nullptr);
    EXPECT_EQ(gauge->name(), "active_workflows");
    EXPECT_FALSE(gauge->unit().has_value());
    EXPECT_FALSE(gauge->description().has_value());
}

TEST(NoopMetricMeterTest, CreateGaugeWithUnitAndDescription) {
    NoopMetricMeter meter;
    auto gauge = meter.create_gauge("active_workflows", std::nullopt,
                                    "Number of active workflows");
    ASSERT_NE(gauge, nullptr);
    EXPECT_EQ(gauge->name(), "active_workflows");
    EXPECT_FALSE(gauge->unit().has_value());
    EXPECT_EQ(gauge->description().value(), "Number of active workflows");
}

TEST(NoopMetricMeterTest, GaugeSetDoesNotCrash) {
    NoopMetricMeter meter;
    auto gauge = meter.create_gauge("active_workflows");
    EXPECT_NO_THROW(gauge->set(10));
    EXPECT_NO_THROW(gauge->set(-5));
    EXPECT_NO_THROW(gauge->set(0));
}

TEST(NoopMetricMeterTest, GaugeSetWithTagsDoesNotCrash) {
    NoopMetricMeter meter;
    auto gauge = meter.create_gauge("active_workflows");
    MetricAttributes tags = {{"namespace", "default"}};
    EXPECT_NO_THROW(gauge->set(42, tags));
}

TEST(NoopMetricMeterTest, WithTagsReturnsNewMeter) {
    NoopMetricMeter meter;
    MetricAttributes tags = {{"env", "prod"}};
    auto new_meter = meter.with_tags(tags);
    ASSERT_NE(new_meter, nullptr);

    // New meter should still work
    auto counter = new_meter->create_counter("test");
    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(counter->name(), "test");
}

TEST(NoopMetricMeterTest, WithTagsChaining) {
    NoopMetricMeter meter;
    auto m1 = meter.with_tags({{"env", "prod"}});
    auto m2 = m1->with_tags({{"region", "us-west-2"}});
    ASSERT_NE(m2, nullptr);

    auto counter = m2->create_counter("chained");
    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(counter->name(), "chained");
}

// ===========================================================================
// MetricAttributes tests
// ===========================================================================

TEST(MetricAttributesTest, EmptyAttributes) {
    MetricAttributes attrs;
    EXPECT_TRUE(attrs.empty());
}

TEST(MetricAttributesTest, SingleAttribute) {
    MetricAttributes attrs = {{"key", "value"}};
    ASSERT_EQ(attrs.size(), 1u);
    EXPECT_EQ(attrs[0].first, "key");
    EXPECT_EQ(attrs[0].second, "value");
}

TEST(MetricAttributesTest, MultipleAttributes) {
    MetricAttributes attrs = {
        {"method", "GET"},
        {"status", "200"},
        {"path", "/api"},
    };
    ASSERT_EQ(attrs.size(), 3u);
    EXPECT_EQ(attrs[0].first, "method");
    EXPECT_EQ(attrs[1].first, "status");
    EXPECT_EQ(attrs[2].first, "path");
}
