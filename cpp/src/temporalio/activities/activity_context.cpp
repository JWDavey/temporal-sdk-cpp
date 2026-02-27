#include "temporalio/activities/activity_context.h"

namespace temporalio::activities {

thread_local ActivityExecutionContext* ActivityExecutionContext::current_ptr_ =
    nullptr;

ActivityExecutionContext::ActivityExecutionContext(
    ActivityInfo info, std::stop_token cancellation_token,
    std::stop_token worker_shutdown_token)
    : info_(std::move(info)),
      cancellation_token_(cancellation_token),
      worker_shutdown_token_(worker_shutdown_token) {}

void ActivityExecutionContext::heartbeat(const std::any& details) {
    if (heartbeat_callback_) {
        heartbeat_callback_(details);
    }
}

}  // namespace temporalio::activities
