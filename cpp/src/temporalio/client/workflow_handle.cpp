#include <temporalio/client/workflow_handle.h>
#include <temporalio/client/temporal_client.h>

#include <temporalio/async_/task_completion_source.h>

#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include <temporal/api/workflowservice/v1/request_response.pb.h>
#include <temporal/api/history/v1/message.pb.h>
#include <temporal/api/enums/v1/workflow.pb.h>

#include "temporalio/bridge/client.h"
#include "temporalio/exceptions/temporal_exception.h"

namespace temporalio::client {

namespace {

/// Bridge the callback-based rpc_call_async to a coroutine.
/// Separated from the loop in get_result() to avoid MSVC C7746 errors.
async_::Task<std::vector<uint8_t>> rpc_call(
    bridge::Client& client, bridge::RpcCallOptions opts) {
    auto tcs =
        std::make_shared<async_::TaskCompletionSource<std::vector<uint8_t>>>();

    client.rpc_call_async(
        opts,
        [tcs](std::optional<bridge::RpcCallResult> result,
              std::optional<bridge::RpcCallError> error) {
            if (error) {
                tcs->try_set_exception(std::make_exception_ptr(
                    exceptions::RpcException(
                        static_cast<exceptions::RpcException::StatusCode>(
                            error->status_code),
                        error->message, error->details)));
            } else if (result) {
                tcs->try_set_result(std::move(result->response));
            } else {
                tcs->try_set_exception(std::make_exception_ptr(
                    std::runtime_error("RPC call returned no result")));
            }
        });

    co_return co_await tcs->task();
}

/// Extract workflow result from a GetWorkflowExecutionHistory response.
/// Returns the result string if a close event is found.
/// Throws on failure/cancel/terminate/timeout.
/// Returns nullopt if no close event found (caller should continue polling).
struct PollResult {
    std::optional<std::string> result;
    std::string next_page_token;
};

PollResult process_history_response(const std::vector<uint8_t>& response_bytes) {
    temporal::api::workflowservice::v1::
        GetWorkflowExecutionHistoryResponse resp;
    if (!resp.ParseFromArray(response_bytes.data(),
                              static_cast<int>(response_bytes.size()))) {
        throw std::runtime_error(
            "Failed to parse GetWorkflowExecutionHistory response");
    }

    for (const auto& event : resp.history().events()) {
        if (event.has_workflow_execution_completed_event_attributes()) {
            const auto& attrs =
                event.workflow_execution_completed_event_attributes();
            if (attrs.has_result() &&
                attrs.result().payloads_size() > 0) {
                const auto& payload = attrs.result().payloads(0);
                return PollResult{
                    std::string(payload.data().begin(),
                                payload.data().end()),
                    {}};
            }
            return PollResult{std::string{}, {}};
        }
        if (event.has_workflow_execution_failed_event_attributes()) {
            const auto& attrs =
                event.workflow_execution_failed_event_attributes();
            std::string msg = "Workflow execution failed";
            if (attrs.has_failure()) {
                msg = attrs.failure().message();
            }
            throw std::runtime_error(msg);
        }
        if (event.has_workflow_execution_canceled_event_attributes()) {
            throw std::runtime_error("Workflow execution cancelled");
        }
        if (event.has_workflow_execution_terminated_event_attributes()) {
            throw std::runtime_error("Workflow execution terminated");
        }
        if (event.has_workflow_execution_timed_out_event_attributes()) {
            throw std::runtime_error("Workflow execution timed out");
        }
    }

    // No close event found
    PollResult pr;
    pr.next_page_token = resp.next_page_token();
    return pr;
}

}  // namespace

WorkflowHandle::WorkflowHandle(std::shared_ptr<TemporalClient> client,
                               std::string id,
                               std::optional<std::string> run_id,
                               std::optional<std::string> first_execution_run_id)
    : client_(std::move(client)),
      id_(std::move(id)),
      run_id_(std::move(run_id)),
      first_execution_run_id_(std::move(first_execution_run_id)) {}

async_::Task<std::string> WorkflowHandle::get_result() {
    auto* bridge = client_->bridge_client();
    if (!bridge) {
        throw std::runtime_error("Not connected to Temporal server");
    }

    // Build GetWorkflowExecutionHistoryRequest
    temporal::api::workflowservice::v1::GetWorkflowExecutionHistoryRequest req;
    req.set_namespace_(client_->ns());
    req.mutable_execution()->set_workflow_id(id_);
    if (run_id_) {
        req.mutable_execution()->set_run_id(*run_id_);
    }
    req.set_history_event_filter_type(
        temporal::api::enums::v1::HISTORY_EVENT_FILTER_TYPE_CLOSE_EVENT);
    req.set_wait_new_event(true);

    std::string next_page_token;

    while (true) {
        if (!next_page_token.empty()) {
            req.set_next_page_token(next_page_token);
        }

        bridge::RpcCallOptions rpc_opts;
        rpc_opts.service = TemporalCoreRpcService::Workflow;
        rpc_opts.rpc = "GetWorkflowExecutionHistory";
        std::string serialized = req.SerializeAsString();
        rpc_opts.request = std::vector<uint8_t>(serialized.begin(),
                                                 serialized.end());
        rpc_opts.retry = true;

        auto response_bytes = co_await rpc_call(*bridge, std::move(rpc_opts));

        auto poll_result = process_history_response(response_bytes);
        if (poll_result.result.has_value()) {
            co_return std::move(*poll_result.result);
        }

        next_page_token = std::move(poll_result.next_page_token);
    }
}

async_::Task<void> WorkflowHandle::signal(
    const std::string& signal_name, const std::string& args,
    const WorkflowSignalOptions& options) {
    co_await client_->signal_workflow(id_, signal_name, args, run_id_);
}

async_::Task<std::string> WorkflowHandle::query(
    const std::string& query_type, const std::string& args,
    const WorkflowQueryOptions& options) {
    co_return co_await client_->query_workflow(id_, query_type, args, run_id_);
}

async_::Task<void> WorkflowHandle::cancel(
    const WorkflowCancelOptions& options) {
    co_await client_->cancel_workflow(id_, run_id_);
}

async_::Task<void> WorkflowHandle::terminate(
    const WorkflowTerminateOptions& options) {
    co_await client_->terminate_workflow(id_, options.reason, run_id_);
}

async_::Task<std::string> WorkflowHandle::describe(
    const WorkflowDescribeOptions& options) {
    // TODO: Implement via client RPC
    co_return std::string{};
}

} // namespace temporalio::client
