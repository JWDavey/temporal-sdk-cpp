#include "temporalio/exceptions/temporal_exception.h"

namespace temporalio::exceptions {

// TemporalException

TemporalException::TemporalException(const std::string& message)
    : std::runtime_error(message) {}

TemporalException::TemporalException(const std::string& message,
                                     std::exception_ptr inner)
    : std::runtime_error(message), inner_(inner) {}

bool TemporalException::is_canceled_exception(const std::exception_ptr& e) {
    if (!e) return false;
    try {
        std::rethrow_exception(e);
    } catch (const CanceledFailureException&) {
        return true;
    } catch (const ActivityFailureException& ex) {
        return is_canceled_exception(ex.inner());
    } catch (const ChildWorkflowFailureException& ex) {
        return is_canceled_exception(ex.inner());
    } catch (const NexusOperationFailureException& ex) {
        return is_canceled_exception(ex.inner());
    } catch (...) {
        return false;
    }
}

// RpcException

RpcException::RpcException(StatusCode code, const std::string& message,
                           std::vector<uint8_t> raw_status)
    : TemporalException(message),
      code_(code),
      raw_status_(std::move(raw_status)) {}

// RpcTimeoutOrCanceledException

RpcTimeoutOrCanceledException::RpcTimeoutOrCanceledException(
    const std::string& message, std::exception_ptr inner)
    : TemporalException(message, inner) {}

// WorkflowUpdateRpcTimeoutOrCanceledException

WorkflowUpdateRpcTimeoutOrCanceledException::
    WorkflowUpdateRpcTimeoutOrCanceledException(std::exception_ptr inner)
    : RpcTimeoutOrCanceledException(
          "Timeout or cancellation waiting for update", inner) {}

// FailureException

FailureException::FailureException(const std::string& message,
                                   std::exception_ptr inner)
    : TemporalException(message, inner) {}

FailureException::FailureException(const std::string& message,
                                   const std::string& failure_stack_trace,
                                   std::exception_ptr inner)
    : TemporalException(message, inner),
      failure_stack_trace_(failure_stack_trace) {}

// ApplicationFailureException

ApplicationFailureException::ApplicationFailureException(
    const std::string& message, std::optional<std::string> error_type,
    bool non_retryable,
    std::optional<std::chrono::milliseconds> next_retry_delay,
    std::exception_ptr inner)
    : FailureException(message, inner),
      error_type_(std::move(error_type)),
      non_retryable_(non_retryable),
      next_retry_delay_(next_retry_delay) {}

// CanceledFailureException

CanceledFailureException::CanceledFailureException(const std::string& message,
                                                   std::exception_ptr inner)
    : FailureException(message, inner) {}

// TerminatedFailureException

TerminatedFailureException::TerminatedFailureException(
    const std::string& message, std::exception_ptr inner)
    : FailureException(message, inner) {}

// TimeoutFailureException

TimeoutFailureException::TimeoutFailureException(const std::string& message,
                                                 TimeoutType timeout_type,
                                                 std::exception_ptr inner)
    : FailureException(message, inner), timeout_type_(timeout_type) {}

// ServerFailureException

ServerFailureException::ServerFailureException(const std::string& message,
                                               bool non_retryable,
                                               std::exception_ptr inner)
    : FailureException(message, inner), non_retryable_(non_retryable) {}

// ActivityFailureException (FailureException-derived, for workflow use)

ActivityFailureException::ActivityFailureException(
    const std::string& message, std::string activity_type,
    std::string activity_id, std::optional<std::string> identity,
    int retry_state, std::exception_ptr inner)
    : FailureException(message, inner),
      activity_type_(std::move(activity_type)),
      activity_id_(std::move(activity_id)),
      identity_(std::move(identity)),
      retry_state_(retry_state) {}

// ChildWorkflowFailureException

ChildWorkflowFailureException::ChildWorkflowFailureException(
    const std::string& message, std::string ns, std::string workflow_id,
    std::string run_id, std::string workflow_type, int retry_state,
    std::exception_ptr inner)
    : FailureException(message, inner),
      namespace_(std::move(ns)),
      workflow_id_(std::move(workflow_id)),
      run_id_(std::move(run_id)),
      workflow_type_(std::move(workflow_type)),
      retry_state_(retry_state) {}

// NexusOperationFailureException

NexusOperationFailureException::NexusOperationFailureException(
    const std::string& message, std::string endpoint, std::string service,
    std::string operation, std::string operation_token,
    std::exception_ptr inner)
    : FailureException(message, inner),
      endpoint_(std::move(endpoint)),
      service_(std::move(service)),
      operation_(std::move(operation)),
      operation_token_(std::move(operation_token)) {}

// NexusHandlerFailureException

NexusHandlerFailureException::NexusHandlerFailureException(
    const std::string& message, std::string raw_error_type, int retry_behavior,
    std::exception_ptr inner)
    : FailureException(message, inner),
      raw_error_type_(std::move(raw_error_type)),
      retry_behavior_(retry_behavior) {}

// WorkflowAlreadyStartedException

WorkflowAlreadyStartedException::WorkflowAlreadyStartedException(
    const std::string& message, std::string workflow_id,
    std::string workflow_type, std::string run_id)
    : FailureException(message),
      workflow_id_(std::move(workflow_id)),
      workflow_type_(std::move(workflow_type)),
      run_id_(std::move(run_id)) {}

// ActivityAlreadyStartedException

ActivityAlreadyStartedException::ActivityAlreadyStartedException(
    const std::string& message, std::string activity_id,
    std::string activity_type, std::optional<std::string> run_id)
    : FailureException(message),
      activity_id_(std::move(activity_id)),
      activity_type_(std::move(activity_type)),
      run_id_(std::move(run_id)) {}

// ScheduleAlreadyRunningException

ScheduleAlreadyRunningException::ScheduleAlreadyRunningException()
    : FailureException("Schedule already running") {}

// WorkflowFailedException

WorkflowFailedException::WorkflowFailedException(std::exception_ptr inner)
    : TemporalException("Workflow failed", inner) {}

// ActivityFailedException (TemporalException-derived, for standalone
// activities)

ActivityFailedException::ActivityFailedException(std::exception_ptr inner)
    : TemporalException("Activity failed", inner) {}

// ContinueAsNewException (thrown from within a workflow)

ContinueAsNewException::ContinueAsNewException(std::string workflow_type,
                                               std::string task_queue)
    : TemporalException("Continue as new"),
      workflow_type_(std::move(workflow_type)),
      task_queue_(std::move(task_queue)) {}

// WorkflowContinuedAsNewException

WorkflowContinuedAsNewException::WorkflowContinuedAsNewException(
    std::string new_run_id)
    : TemporalException("Workflow continued as new"),
      new_run_id_(std::move(new_run_id)) {}

// WorkflowQueryFailedException

WorkflowQueryFailedException::WorkflowQueryFailedException(
    const std::string& message)
    : TemporalException(message) {}

// WorkflowQueryRejectedException

WorkflowQueryRejectedException::WorkflowQueryRejectedException(
    int workflow_status)
    : TemporalException("Query rejected, workflow status: " +
                        std::to_string(workflow_status)),
      workflow_status_(workflow_status) {}

// WorkflowUpdateFailedException

WorkflowUpdateFailedException::WorkflowUpdateFailedException(
    std::exception_ptr inner)
    : TemporalException("Workflow update failed", inner) {}

// AsyncActivityCanceledException

AsyncActivityCanceledException::AsyncActivityCanceledException()
    : TemporalException("Activity cancelled") {}

// InvalidWorkflowOperationException

InvalidWorkflowOperationException::InvalidWorkflowOperationException(
    const std::string& message)
    : TemporalException(message) {}

// WorkflowNondeterminismException

WorkflowNondeterminismException::WorkflowNondeterminismException(
    const std::string& message)
    : InvalidWorkflowOperationException(message) {}

// InvalidWorkflowSchedulerException

InvalidWorkflowSchedulerException::InvalidWorkflowSchedulerException(
    const std::string& message,
    std::optional<std::string> stack_trace_override)
    : InvalidWorkflowOperationException(message),
      stack_trace_override_(std::move(stack_trace_override)) {}

const char* InvalidWorkflowSchedulerException::what() const noexcept {
    if (stack_trace_override_) {
        if (what_cache_.empty()) {
            what_cache_ =
                std::string(InvalidWorkflowOperationException::what()) +
                "\nStack trace: " + *stack_trace_override_;
        }
        return what_cache_.c_str();
    }
    return InvalidWorkflowOperationException::what();
}

}  // namespace temporalio::exceptions
