#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include "temporalio/runtime/temporal_runtime.h"

using namespace temporalio::runtime;
using namespace std::chrono_literals;

// ===========================================================================
// TelemetryFilterOptions tests
// ===========================================================================

TEST(TelemetryFilterOptionsTest, DefaultValues) {
    TelemetryFilterOptions opts;
    EXPECT_EQ(opts.core_level, "WARN");
    EXPECT_TRUE(opts.additional_directives.empty());
}

TEST(TelemetryFilterOptionsTest, CustomValues) {
    TelemetryFilterOptions opts{
        .core_level = "DEBUG",
        .additional_directives = {"temporalio=TRACE"},
    };
    EXPECT_EQ(opts.core_level, "DEBUG");
    ASSERT_EQ(opts.additional_directives.size(), 1u);
    EXPECT_EQ(opts.additional_directives[0], "temporalio=TRACE");
}

// ===========================================================================
// LoggingOptions tests
// ===========================================================================

TEST(LoggingOptionsTest, DefaultValues) {
    LoggingOptions opts;
    EXPECT_EQ(opts.filter.core_level, "WARN");
    EXPECT_FALSE(opts.forwarding.has_value());
}

TEST(LoggingOptionsTest, WithForwarding) {
    LoggingOptions opts{
        .filter = {.core_level = "INFO"},
        .forwarding = LogForwardingOptions{.level = "DEBUG"},
    };
    EXPECT_EQ(opts.filter.core_level, "INFO");
    ASSERT_TRUE(opts.forwarding.has_value());
    EXPECT_EQ(opts.forwarding->level, "DEBUG");
}

// ===========================================================================
// OpenTelemetryOptions tests
// ===========================================================================

TEST(OpenTelemetryOptionsTest, DefaultValues) {
    OpenTelemetryOptions opts;
    EXPECT_TRUE(opts.url.empty());
    EXPECT_EQ(opts.metric_temporality,
              OpenTelemetryMetricTemporality::kCumulative);
    EXPECT_EQ(opts.protocol, OpenTelemetryProtocol::kGrpc);
    EXPECT_FALSE(opts.metric_periodicity.has_value());
    EXPECT_FALSE(opts.durations_as_seconds);
}

TEST(OpenTelemetryOptionsTest, CustomValues) {
    OpenTelemetryOptions opts{
        .url = "http://localhost:4317",
        .headers = {{"Authorization", "Bearer token"}},
        .metric_temporality = OpenTelemetryMetricTemporality::kDelta,
        .protocol = OpenTelemetryProtocol::kHttp,
        .metric_periodicity = 30000ms,
        .durations_as_seconds = true,
    };
    EXPECT_EQ(opts.url, "http://localhost:4317");
    ASSERT_EQ(opts.headers.size(), 1u);
    EXPECT_EQ(opts.headers[0].first, "Authorization");
    EXPECT_EQ(opts.metric_temporality,
              OpenTelemetryMetricTemporality::kDelta);
    EXPECT_EQ(opts.protocol, OpenTelemetryProtocol::kHttp);
    EXPECT_EQ(opts.metric_periodicity.value(), 30000ms);
    EXPECT_TRUE(opts.durations_as_seconds);
}

// ===========================================================================
// PrometheusOptions tests
// ===========================================================================

TEST(PrometheusOptionsTest, DefaultValues) {
    PrometheusOptions opts;
    EXPECT_TRUE(opts.bind_address.empty());
    EXPECT_FALSE(opts.counters_total_suffix);
    EXPECT_FALSE(opts.unit_suffix);
    EXPECT_FALSE(opts.durations_as_seconds);
}

TEST(PrometheusOptionsTest, CustomValues) {
    PrometheusOptions opts{
        .bind_address = "0.0.0.0:9090",
        .counters_total_suffix = true,
        .unit_suffix = true,
        .durations_as_seconds = true,
    };
    EXPECT_EQ(opts.bind_address, "0.0.0.0:9090");
    EXPECT_TRUE(opts.counters_total_suffix);
}

// ===========================================================================
// MetricsOptions tests
// ===========================================================================

TEST(MetricsOptionsTest, DefaultValues) {
    MetricsOptions opts;
    EXPECT_FALSE(opts.prometheus.has_value());
    EXPECT_FALSE(opts.opentelemetry.has_value());
    EXPECT_EQ(opts.custom_metric_meter, nullptr);
    EXPECT_TRUE(opts.attach_service_name);
    EXPECT_TRUE(opts.global_tags.empty());
    EXPECT_FALSE(opts.metric_prefix.has_value());
}

TEST(MetricsOptionsTest, WithPrometheus) {
    MetricsOptions opts{
        .prometheus =
            PrometheusOptions{.bind_address = "0.0.0.0:9090"},
    };
    ASSERT_TRUE(opts.prometheus.has_value());
    EXPECT_EQ(opts.prometheus->bind_address, "0.0.0.0:9090");
}

TEST(MetricsOptionsTest, WithGlobalTags) {
    MetricsOptions opts{
        .global_tags = {{"env", "prod"}, {"region", "us-west-2"}},
        .metric_prefix = "myapp_",
    };
    ASSERT_EQ(opts.global_tags.size(), 2u);
    EXPECT_EQ(opts.global_tags[0].first, "env");
    EXPECT_EQ(opts.global_tags[0].second, "prod");
    EXPECT_EQ(opts.metric_prefix.value(), "myapp_");
}

// ===========================================================================
// TemporalRuntimeOptions tests
// ===========================================================================

TEST(TemporalRuntimeOptionsTest, DefaultValues) {
    TemporalRuntimeOptions opts;
    ASSERT_TRUE(opts.telemetry.logging.has_value());
    EXPECT_EQ(opts.telemetry.logging->filter.core_level, "WARN");
    EXPECT_FALSE(opts.telemetry.metrics.has_value());
    ASSERT_TRUE(opts.worker_heartbeat_interval.has_value());
    EXPECT_EQ(opts.worker_heartbeat_interval.value(), 60000ms);
}

TEST(TemporalRuntimeOptionsTest, CustomValues) {
    TemporalRuntimeOptions opts{
        .telemetry =
            {
                .logging =
                    LoggingOptions{
                        .filter = {.core_level = "DEBUG"},
                    },
                .metrics =
                    MetricsOptions{
                        .prometheus =
                            PrometheusOptions{.bind_address = ":9090"},
                    },
            },
        .worker_heartbeat_interval = 30000ms,
    };
    EXPECT_EQ(opts.telemetry.logging->filter.core_level, "DEBUG");
    ASSERT_TRUE(opts.telemetry.metrics.has_value());
    ASSERT_TRUE(opts.telemetry.metrics->prometheus.has_value());
    EXPECT_EQ(opts.worker_heartbeat_interval.value(), 30000ms);
}

// ===========================================================================
// TemporalRuntime type traits
// ===========================================================================

TEST(TemporalRuntimeTest, IsNonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<TemporalRuntime>);
    EXPECT_FALSE(std::is_copy_assignable_v<TemporalRuntime>);
}

TEST(TemporalRuntimeTest, IsMovable) {
    EXPECT_TRUE(std::is_move_constructible_v<TemporalRuntime>);
    EXPECT_TRUE(std::is_move_assignable_v<TemporalRuntime>);
}
