#include <temporalio/client/workflow_handle.h>
#include <temporalio/client/temporal_client.h>

#include <utility>

namespace temporalio::client {

WorkflowHandle::WorkflowHandle(std::shared_ptr<TemporalClient> client,
                               std::string id,
                               std::optional<std::string> run_id,
                               std::optional<std::string> first_execution_run_id)
    : client_(std::move(client)),
      id_(std::move(id)),
      run_id_(std::move(run_id)),
      first_execution_run_id_(std::move(first_execution_run_id)) {}

async_::Task<std::string> WorkflowHandle::get_result() {
    // TODO: Implement via client RPC - poll for workflow completion
    co_return std::string{};
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
