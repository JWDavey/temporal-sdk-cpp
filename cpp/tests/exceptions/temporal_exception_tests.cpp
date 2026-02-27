#include <gtest/gtest.h>

#include <chrono>
#include <exception>
#include <stdexcept>
#include <string>

#include "temporalio/exceptions/temporal_exception.h"

using namespace temporalio::exceptions;

// ===========================================================================
// RpcException tests
// ===========================================================================

TEST(RpcExceptionTest, ConstructionAndAccessors) {
    RpcException ex(RpcException::StatusCode::kNotFound, "not found");
    EXPECT_STREQ(ex.what(), "not found");
    EXPECT_EQ(ex.code(), RpcException::StatusCode::kNotFound);
    EXPECT_TRUE(ex.raw_status().empty());
}

TEST(RpcExceptionTest, WithRawStatus) {
    std::vector<uint8_t> raw{0x01, 0x02, 0x03};
    RpcException ex(RpcException::StatusCode::kInternal, "internal error", raw);
    EXPECT_EQ(ex.code(), RpcException::StatusCode::kInternal);
    EXPECT_EQ(ex.raw_status(), raw);
}

TEST(RpcExceptionTest, InheritsFromTemporalException) {
    RpcException ex(RpcException::StatusCode::kUnknown, "test");
    const TemporalException& base = ex;
    EXPECT_STREQ(base.what(), "test");
}

TEST(RpcExceptionTest, InheritsFromStdRuntimeError) {
    RpcException ex(RpcException::StatusCode::kOk, "rpc test");
    const std::runtime_error& base = ex;
    EXPECT_STREQ(base.what(), "rpc test");
}

TEST(RpcExceptionTest, AllStatusCodes) {
    using SC = RpcException::StatusCode;
    // Verify all status codes can be used
    EXPECT_NO_THROW(RpcException(SC::kOk, "ok"));
    EXPECT_NO_THROW(RpcException(SC::kCancelled, "cancelled"));
    EXPECT_NO_THROW(RpcException(SC::kUnknown, "unknown"));
    EXPECT_NO_THROW(RpcException(SC::kInvalidArgument, "invalid"));
    EXPECT_NO_THROW(RpcException(SC::kDeadlineExceeded, "deadline"));
    EXPECT_NO_THROW(RpcException(SC::kNotFound, "not found"));
    EXPECT_NO_THROW(RpcException(SC::kAlreadyExists, "exists"));
    EXPECT_NO_THROW(RpcException(SC::kPermissionDenied, "denied"));
    EXPECT_NO_THROW(RpcException(SC::kResourceExhausted, "exhausted"));
    EXPECT_NO_THROW(RpcException(SC::kFailedPrecondition, "precondition"));
    EXPECT_NO_THROW(RpcException(SC::kAborted, "aborted"));
    EXPECT_NO_THROW(RpcException(SC::kOutOfRange, "range"));
    EXPECT_NO_THROW(RpcException(SC::kUnimplemented, "unimplemented"));
    EXPECT_NO_THROW(RpcException(SC::kInternal, "internal"));
    EXPECT_NO_THROW(RpcException(SC::kUnavailable, "unavailable"));
    EXPECT_NO_THROW(RpcException(SC::kDataLoss, "data loss"));
    EXPECT_NO_THROW(RpcException(SC::kUnauthenticated, "unauth"));
}

// ===========================================================================
// RpcTimeoutOrCanceledException tests
// ===========================================================================

TEST(RpcTimeoutOrCanceledExceptionTest, Construction) {
    RpcTimeoutOrCanceledException ex("timed out");
    EXPECT_STREQ(ex.what(), "timed out");
    EXPECT_EQ(ex.inner(), nullptr);
}

TEST(RpcTimeoutOrCanceledExceptionTest, WithInner) {
    auto inner = std::make_exception_ptr(std::runtime_error("cause"));
    RpcTimeoutOrCanceledException ex("timed out", inner);
    EXPECT_STREQ(ex.what(), "timed out");
    EXPECT_NE(ex.inner(), nullptr);
}

// ===========================================================================
// WorkflowUpdateRpcTimeoutOrCanceledException tests
// ===========================================================================

TEST(WorkflowUpdateRpcTimeoutOrCanceledExceptionTest, Construction) {
    WorkflowUpdateRpcTimeoutOrCanceledException ex;
    // Should have a reasonable default message
    EXPECT_NE(std::string(ex.what()), "");
}

// ===========================================================================
// ApplicationFailureException tests
// ===========================================================================

TEST(ApplicationFailureExceptionTest, BasicConstruction) {
    ApplicationFailureException ex("app failure");
    EXPECT_STREQ(ex.what(), "app failure");
    EXPECT_FALSE(ex.error_type().has_value());
    EXPECT_FALSE(ex.non_retryable());
    EXPECT_FALSE(ex.next_retry_delay().has_value());
}

TEST(ApplicationFailureExceptionTest, WithErrorType) {
    ApplicationFailureException ex("fail", std::string("MyError"));
    EXPECT_EQ(ex.error_type().value(), "MyError");
}

TEST(ApplicationFailureExceptionTest, NonRetryable) {
    ApplicationFailureException ex("fail", std::nullopt, true);
    EXPECT_TRUE(ex.non_retryable());
}

TEST(ApplicationFailureExceptionTest, WithNextRetryDelay) {
    using namespace std::chrono_literals;
    ApplicationFailureException ex(
        "fail", std::nullopt, false, 5000ms);
    ASSERT_TRUE(ex.next_retry_delay().has_value());
    EXPECT_EQ(ex.next_retry_delay().value(), 5000ms);
}

TEST(ApplicationFailureExceptionTest, FullConstruction) {
    using namespace std::chrono_literals;
    auto inner = std::make_exception_ptr(std::runtime_error("root cause"));
    ApplicationFailureException ex(
        "full failure", std::string("CustomError"), true, 1000ms, inner);
    EXPECT_STREQ(ex.what(), "full failure");
    EXPECT_EQ(ex.error_type().value(), "CustomError");
    EXPECT_TRUE(ex.non_retryable());
    EXPECT_EQ(ex.next_retry_delay().value(), 1000ms);
    EXPECT_NE(ex.inner(), nullptr);
}

TEST(ApplicationFailureExceptionTest, InheritsFromFailureException) {
    ApplicationFailureException ex("test");
    const FailureException& base = ex;
    EXPECT_STREQ(base.what(), "test");
}

// ===========================================================================
// CanceledFailureException tests
// ===========================================================================

TEST(CanceledFailureExceptionTest, Construction) {
    CanceledFailureException ex("cancelled");
    EXPECT_STREQ(ex.what(), "cancelled");
}

TEST(CanceledFailureExceptionTest, WithInner) {
    auto inner = std::make_exception_ptr(std::runtime_error("cause"));
    CanceledFailureException ex("cancelled", inner);
    EXPECT_NE(ex.inner(), nullptr);
}

TEST(CanceledFailureExceptionTest, InheritsFromFailureException) {
    CanceledFailureException ex("cancelled");
    const FailureException& base = ex;
    EXPECT_STREQ(base.what(), "cancelled");
}

// ===========================================================================
// TerminatedFailureException tests
// ===========================================================================

TEST(TerminatedFailureExceptionTest, Construction) {
    TerminatedFailureException ex("terminated");
    EXPECT_STREQ(ex.what(), "terminated");
}

// ===========================================================================
// TimeoutFailureException tests
// ===========================================================================

TEST(TimeoutFailureExceptionTest, Construction) {
    TimeoutFailureException ex(
        "timed out",
        TimeoutFailureException::TimeoutType::kStartToClose);
    EXPECT_STREQ(ex.what(), "timed out");
    EXPECT_EQ(ex.timeout_type(),
              TimeoutFailureException::TimeoutType::kStartToClose);
}

TEST(TimeoutFailureExceptionTest, AllTimeoutTypes) {
    using TT = TimeoutFailureException::TimeoutType;
    EXPECT_NO_THROW(TimeoutFailureException("t", TT::kUnspecified));
    EXPECT_NO_THROW(TimeoutFailureException("t", TT::kStartToClose));
    EXPECT_NO_THROW(TimeoutFailureException("t", TT::kScheduleToStart));
    EXPECT_NO_THROW(TimeoutFailureException("t", TT::kScheduleToClose));
    EXPECT_NO_THROW(TimeoutFailureException("t", TT::kHeartbeat));
}

// ===========================================================================
// ServerFailureException tests
// ===========================================================================

TEST(ServerFailureExceptionTest, Retryable) {
    ServerFailureException ex("server fail", false);
    EXPECT_STREQ(ex.what(), "server fail");
    EXPECT_FALSE(ex.non_retryable());
}

TEST(ServerFailureExceptionTest, NonRetryable) {
    ServerFailureException ex("server fail", true);
    EXPECT_TRUE(ex.non_retryable());
}

// ===========================================================================
// ActivityFailureException tests
// ===========================================================================

TEST(ActivityFailureExceptionTest, Construction) {
    ActivityFailureException ex(
        "activity failed", "MyActivity", "act-123");
    EXPECT_STREQ(ex.what(), "activity failed");
    EXPECT_EQ(ex.activity_type(), "MyActivity");
    EXPECT_EQ(ex.activity_id(), "act-123");
    EXPECT_FALSE(ex.identity().has_value());
    EXPECT_EQ(ex.retry_state(), 0);
}

TEST(ActivityFailureExceptionTest, FullConstruction) {
    auto inner = std::make_exception_ptr(std::runtime_error("cause"));
    ActivityFailureException ex(
        "activity failed", "MyActivity", "act-456",
        std::string("worker-1"), 2, inner);
    EXPECT_EQ(ex.activity_type(), "MyActivity");
    EXPECT_EQ(ex.activity_id(), "act-456");
    EXPECT_EQ(ex.identity().value(), "worker-1");
    EXPECT_EQ(ex.retry_state(), 2);
    EXPECT_NE(ex.inner(), nullptr);
}

// ===========================================================================
// ChildWorkflowFailureException tests
// ===========================================================================

TEST(ChildWorkflowFailureExceptionTest, Construction) {
    ChildWorkflowFailureException ex(
        "child failed", "default", "wf-123", "run-abc",
        "ChildWorkflow");
    EXPECT_STREQ(ex.what(), "child failed");
    EXPECT_EQ(ex.ns(), "default");
    EXPECT_EQ(ex.workflow_id(), "wf-123");
    EXPECT_EQ(ex.run_id(), "run-abc");
    EXPECT_EQ(ex.workflow_type(), "ChildWorkflow");
    EXPECT_EQ(ex.retry_state(), 0);
}

TEST(ChildWorkflowFailureExceptionTest, WithRetryState) {
    ChildWorkflowFailureException ex(
        "child failed", "ns", "wf-1", "run-1",
        "ChildWf", 3);
    EXPECT_EQ(ex.retry_state(), 3);
}

// ===========================================================================
// NexusOperationFailureException tests
// ===========================================================================

TEST(NexusOperationFailureExceptionTest, Construction) {
    NexusOperationFailureException ex(
        "nexus op failed", "endpoint-1", "my-service",
        "my-operation");
    EXPECT_STREQ(ex.what(), "nexus op failed");
    EXPECT_EQ(ex.endpoint(), "endpoint-1");
    EXPECT_EQ(ex.service(), "my-service");
    EXPECT_EQ(ex.operation(), "my-operation");
    EXPECT_TRUE(ex.operation_token().empty());
}

TEST(NexusOperationFailureExceptionTest, WithToken) {
    NexusOperationFailureException ex(
        "nexus op failed", "ep", "svc", "op", "token-xyz");
    EXPECT_EQ(ex.operation_token(), "token-xyz");
}

// ===========================================================================
// NexusHandlerFailureException tests
// ===========================================================================

TEST(NexusHandlerFailureExceptionTest, Construction) {
    NexusHandlerFailureException ex(
        "handler failed", "BAD_REQUEST");
    EXPECT_STREQ(ex.what(), "handler failed");
    EXPECT_EQ(ex.raw_error_type(), "BAD_REQUEST");
    EXPECT_EQ(ex.retry_behavior(), 0);
}

TEST(NexusHandlerFailureExceptionTest, WithRetryBehavior) {
    NexusHandlerFailureException ex("handler failed", "INTERNAL", 1);
    EXPECT_EQ(ex.retry_behavior(), 1);
}

// ===========================================================================
// WorkflowAlreadyStartedException tests
// ===========================================================================

TEST(WorkflowAlreadyStartedExceptionTest, Construction) {
    WorkflowAlreadyStartedException ex(
        "already started", "wf-123", "GreetingWorkflow", "run-abc");
    EXPECT_STREQ(ex.what(), "already started");
    EXPECT_EQ(ex.workflow_id(), "wf-123");
    EXPECT_EQ(ex.workflow_type(), "GreetingWorkflow");
    EXPECT_EQ(ex.run_id(), "run-abc");
}

// ===========================================================================
// ActivityAlreadyStartedException tests
// ===========================================================================

TEST(ActivityAlreadyStartedExceptionTest, Construction) {
    ActivityAlreadyStartedException ex(
        "activity exists", "act-1", "MyActivity");
    EXPECT_STREQ(ex.what(), "activity exists");
    EXPECT_EQ(ex.activity_id(), "act-1");
    EXPECT_EQ(ex.activity_type(), "MyActivity");
    EXPECT_FALSE(ex.run_id().has_value());
}

TEST(ActivityAlreadyStartedExceptionTest, WithRunId) {
    ActivityAlreadyStartedException ex(
        "activity exists", "act-1", "MyActivity",
        std::string("run-xyz"));
    EXPECT_EQ(ex.run_id().value(), "run-xyz");
}

// ===========================================================================
// ScheduleAlreadyRunningException tests
// ===========================================================================

TEST(ScheduleAlreadyRunningExceptionTest, Construction) {
    ScheduleAlreadyRunningException ex;
    // Should have a default message
    EXPECT_NE(std::string(ex.what()), "");
}

// ===========================================================================
// WorkflowFailedException tests
// ===========================================================================

TEST(WorkflowFailedExceptionTest, Construction) {
    WorkflowFailedException ex;
    EXPECT_EQ(ex.inner(), nullptr);
}

TEST(WorkflowFailedExceptionTest, WithInner) {
    auto inner = std::make_exception_ptr(
        ApplicationFailureException("app fail"));
    WorkflowFailedException ex(inner);
    EXPECT_NE(ex.inner(), nullptr);
}

// ===========================================================================
// ActivityFailedException tests
// ===========================================================================

TEST(ActivityFailedExceptionTest, Construction) {
    ActivityFailedException ex;
    EXPECT_EQ(ex.inner(), nullptr);
}

// ===========================================================================
// WorkflowContinuedAsNewException tests
// ===========================================================================

TEST(WorkflowContinuedAsNewExceptionTest, Construction) {
    WorkflowContinuedAsNewException ex("new-run-id-123");
    EXPECT_EQ(ex.new_run_id(), "new-run-id-123");
}

// ===========================================================================
// WorkflowQueryFailedException tests
// ===========================================================================

TEST(WorkflowQueryFailedExceptionTest, Construction) {
    WorkflowQueryFailedException ex("query failed");
    EXPECT_STREQ(ex.what(), "query failed");
}

// ===========================================================================
// WorkflowQueryRejectedException tests
// ===========================================================================

TEST(WorkflowQueryRejectedExceptionTest, Construction) {
    WorkflowQueryRejectedException ex(2);
    EXPECT_EQ(ex.workflow_status(), 2);
}

// ===========================================================================
// WorkflowUpdateFailedException tests
// ===========================================================================

TEST(WorkflowUpdateFailedExceptionTest, Construction) {
    WorkflowUpdateFailedException ex;
    EXPECT_EQ(ex.inner(), nullptr);
}

TEST(WorkflowUpdateFailedExceptionTest, WithInner) {
    auto inner = std::make_exception_ptr(std::runtime_error("cause"));
    WorkflowUpdateFailedException ex(inner);
    EXPECT_NE(ex.inner(), nullptr);
}

// ===========================================================================
// AsyncActivityCanceledException tests
// ===========================================================================

TEST(AsyncActivityCanceledExceptionTest, Construction) {
    AsyncActivityCanceledException ex;
    EXPECT_NE(std::string(ex.what()), "");
}

// ===========================================================================
// InvalidWorkflowOperationException tests
// ===========================================================================

TEST(InvalidWorkflowOperationExceptionTest, Construction) {
    InvalidWorkflowOperationException ex("invalid op");
    EXPECT_STREQ(ex.what(), "invalid op");
}

// ===========================================================================
// WorkflowNondeterminismException tests
// ===========================================================================

TEST(WorkflowNondeterminismExceptionTest, Construction) {
    WorkflowNondeterminismException ex("nondeterminism detected");
    EXPECT_STREQ(ex.what(), "nondeterminism detected");
}

TEST(WorkflowNondeterminismExceptionTest, InheritsFromInvalidWorkflowOperation) {
    WorkflowNondeterminismException ex("test");
    const InvalidWorkflowOperationException& base = ex;
    EXPECT_STREQ(base.what(), "test");
}

// ===========================================================================
// InvalidWorkflowSchedulerException tests
// ===========================================================================

TEST(InvalidWorkflowSchedulerExceptionTest, Construction) {
    InvalidWorkflowSchedulerException ex("bad scheduler");
    EXPECT_NE(std::string(ex.what()), "");
}

// ===========================================================================
// Inheritance hierarchy verification
// ===========================================================================

TEST(ExceptionHierarchyTest, FailureExceptionDerivedFromTemporal) {
    ApplicationFailureException ex("test");
    // Should be catchable as TemporalException
    try {
        throw ex;
    } catch (const TemporalException& e) {
        EXPECT_STREQ(e.what(), "test");
    }
}

TEST(ExceptionHierarchyTest, CatchAsStdException) {
    ApplicationFailureException ex("test");
    try {
        throw ex;
    } catch (const std::exception& e) {
        EXPECT_STREQ(e.what(), "test");
    }
}

TEST(ExceptionHierarchyTest, InnerExceptionChaining) {
    auto root = std::make_exception_ptr(std::runtime_error("root cause"));
    auto inner = std::make_exception_ptr(
        ApplicationFailureException("app fail", std::nullopt, false,
                                     std::nullopt, root));
    WorkflowFailedException outer(inner);

    EXPECT_NE(outer.inner(), nullptr);

    // Unwrap the chain
    try {
        std::rethrow_exception(outer.inner());
    } catch (const ApplicationFailureException& app_ex) {
        EXPECT_STREQ(app_ex.what(), "app fail");
        EXPECT_NE(app_ex.inner(), nullptr);
        try {
            std::rethrow_exception(app_ex.inner());
        } catch (const std::runtime_error& root_ex) {
            EXPECT_STREQ(root_ex.what(), "root cause");
        }
    }
}

// ===========================================================================
// FailureException stack trace
// ===========================================================================

TEST(FailureExceptionTest, StackTraceDefaultEmpty) {
    ApplicationFailureException ex("test");
    EXPECT_TRUE(ex.failure_stack_trace().empty());
}

// ===========================================================================
// is_canceled_exception static method
// ===========================================================================

TEST(TemporalExceptionTest, IsCanceledExceptionWithCanceledFailure) {
    auto ex = std::make_exception_ptr(
        CanceledFailureException("cancelled"));
    EXPECT_TRUE(TemporalException::is_canceled_exception(ex));
}

TEST(TemporalExceptionTest, IsCanceledExceptionWithRegularException) {
    auto ex = std::make_exception_ptr(
        std::runtime_error("not cancelled"));
    EXPECT_FALSE(TemporalException::is_canceled_exception(ex));
}

TEST(TemporalExceptionTest, IsCanceledExceptionWithApplicationFailure) {
    auto ex = std::make_exception_ptr(
        ApplicationFailureException("not cancelled"));
    EXPECT_FALSE(TemporalException::is_canceled_exception(ex));
}

TEST(TemporalExceptionTest, IsCanceledExceptionWithActivityWrappingCanceled) {
    auto canceled = std::make_exception_ptr(
        CanceledFailureException("cancelled"));
    auto ex = std::make_exception_ptr(
        ActivityFailureException("activity failed", "MyAct", "act-1",
                                  std::nullopt, 0, canceled));
    EXPECT_TRUE(TemporalException::is_canceled_exception(ex));
}

TEST(TemporalExceptionTest, IsCanceledExceptionWithChildWfWrappingCanceled) {
    auto canceled = std::make_exception_ptr(
        CanceledFailureException("cancelled"));
    auto ex = std::make_exception_ptr(
        ChildWorkflowFailureException(
            "child failed", "ns", "wf-1", "run-1", "ChildWf", 0, canceled));
    EXPECT_TRUE(TemporalException::is_canceled_exception(ex));
}

TEST(TemporalExceptionTest, IsCanceledExceptionWithNexusWrappingCanceled) {
    auto canceled = std::make_exception_ptr(
        CanceledFailureException("cancelled"));
    auto ex = std::make_exception_ptr(
        NexusOperationFailureException(
            "nexus failed", "ep", "svc", "op", "", canceled));
    EXPECT_TRUE(TemporalException::is_canceled_exception(ex));
}

TEST(TemporalExceptionTest, IsCanceledExceptionWithNullptr) {
    // A null exception_ptr should not be canceled
    std::exception_ptr null_ptr;
    EXPECT_FALSE(TemporalException::is_canceled_exception(null_ptr));
}
