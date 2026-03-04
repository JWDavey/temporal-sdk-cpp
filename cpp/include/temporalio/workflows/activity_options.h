#pragma once

/// @file Activity options for scheduling activities from workflows.

#include <chrono>
#include <optional>
#include <stop_token>
#include <string>

#include <temporalio/common/retry_policy.h>

namespace temporalio::workflows {

/// How the workflow will wait (or not) for cancellation of the activity to be
/// confirmed. Mirrors the protobuf ActivityCancellationType enum.
enum class ActivityCancellationType {
    /// Initiate a cancellation request and immediately report cancellation to
    /// the workflow.
    kTryCancel = 0,
    /// Wait for activity cancellation completion. Note that activity must
    /// heartbeat to receive a cancellation notification.
    kWaitCancellationCompleted = 1,
    /// Do not request cancellation of the activity and immediately report
    /// cancellation to the workflow.
    kAbandon = 2,
};

/// Options for scheduling an activity from a workflow.
/// At least one of schedule_to_close_timeout or start_to_close_timeout must
/// be set.
struct ActivityOptions {
    /// Maximum time from scheduling to completion of the activity. Limits how
    /// long retries will be attempted. Either this or start_to_close_timeout
    /// must be specified.
    std::optional<std::chrono::milliseconds> schedule_to_close_timeout{};

    /// Maximum time an activity task can stay in a task queue before a worker
    /// picks it up.
    std::optional<std::chrono::milliseconds> schedule_to_start_timeout{};

    /// Maximum time an activity is allowed to execute after being picked up by
    /// a worker. Either this or schedule_to_close_timeout must be specified.
    std::optional<std::chrono::milliseconds> start_to_close_timeout{};

    /// Maximum time between successful worker heartbeats.
    std::optional<std::chrono::milliseconds> heartbeat_timeout{};

    /// Retry policy for the activity.
    std::optional<common::RetryPolicy> retry_policy{};

    /// How the workflow will wait for cancellation of the activity.
    ActivityCancellationType cancellation_type{ActivityCancellationType::kTryCancel};

    /// Cancellation token for the activity.
    std::optional<std::stop_token> cancellation_token{};

    /// Override the task queue for the activity. If not set, the workflow's
    /// task queue is used.
    std::optional<std::string> task_queue{};

    /// Override the activity ID. If not set, the server generates one.
    std::optional<std::string> activity_id{};
};

}  // namespace temporalio::workflows
