#pragma once

/// @file retry_policy.h
/// @brief RetryPolicy configuration for workflows and activities.

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace temporalio::common {

/// Retry policy for workflow or activity executions.
struct RetryPolicy {
    /// Backoff interval for the first retry. Default is 1s.
    std::chrono::milliseconds initial_interval{1000};

    /// Coefficient to multiply previous backoff interval by. Default is 2.0.
    double backoff_coefficient{2.0};

    /// Maximum backoff interval between retries. Nullopt means no maximum.
    std::optional<std::chrono::milliseconds> maximum_interval{};

    /// Maximum number of attempts. 0 means no maximum (default).
    int maximum_attempts{0};

    /// Error types that are not retryable.
    /// Refers to ApplicationFailureException::error_type().
    std::vector<std::string> non_retryable_error_types{};

    bool operator==(const RetryPolicy&) const = default;
};

} // namespace temporalio::common
