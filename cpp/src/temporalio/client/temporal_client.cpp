#include <temporalio/client/temporal_client.h>
#include <temporalio/client/temporal_connection.h>

#include <stdexcept>
#include <utility>

namespace temporalio::client {

struct TemporalClient::Impl {
    std::shared_ptr<TemporalConnection> connection;
    TemporalClientOptions options;
    // TODO: Interceptor chain, data converter, etc.
};

TemporalClient::TemporalClient(std::shared_ptr<TemporalConnection> connection,
                               TemporalClientOptions options)
    : impl_(std::make_unique<Impl>()) {
    impl_->connection = std::move(connection);
    impl_->options = std::move(options);
}

TemporalClient::~TemporalClient() = default;

async_::Task<std::shared_ptr<TemporalClient>> TemporalClient::connect(
    TemporalClientConnectOptions options) {
    auto conn =
        co_await TemporalConnection::connect(std::move(options.connection));
    co_return TemporalClient::create(std::move(conn),
                                     std::move(options.client));
}

std::shared_ptr<TemporalClient> TemporalClient::create(
    std::shared_ptr<TemporalConnection> connection,
    TemporalClientOptions options) {
    return std::shared_ptr<TemporalClient>(
        new TemporalClient(std::move(connection), std::move(options)));
}

std::shared_ptr<TemporalConnection> TemporalClient::connection()
    const noexcept {
    return impl_->connection;
}

const std::string& TemporalClient::ns() const noexcept {
    return impl_->options.ns;
}

async_::Task<WorkflowHandle> TemporalClient::start_workflow(
    const std::string& workflow_type, const std::string& args,
    const WorkflowOptions& options) {
    // TODO: Implement via interceptor chain and bridge RPC
    // For now, return a handle with the given workflow ID
    if (options.id.empty()) {
        throw std::invalid_argument("Workflow ID is required");
    }
    if (options.task_queue.empty()) {
        throw std::invalid_argument("Task queue is required");
    }
    co_return WorkflowHandle(shared_from_this(), options.id);
}

WorkflowHandle TemporalClient::get_workflow_handle(
    const std::string& workflow_id,
    std::optional<std::string> run_id) {
    return WorkflowHandle(shared_from_this(), workflow_id, std::move(run_id));
}

async_::Task<void> TemporalClient::signal_workflow(
    const std::string& workflow_id, const std::string& signal_name,
    const std::string& args, std::optional<std::string> run_id) {
    // TODO: Implement via interceptor chain and bridge RPC
    co_return;
}

async_::Task<std::string> TemporalClient::query_workflow(
    const std::string& workflow_id, const std::string& query_type,
    const std::string& args, std::optional<std::string> run_id) {
    // TODO: Implement via interceptor chain and bridge RPC
    co_return std::string{};
}

async_::Task<void> TemporalClient::cancel_workflow(
    const std::string& workflow_id, std::optional<std::string> run_id) {
    // TODO: Implement via interceptor chain and bridge RPC
    co_return;
}

async_::Task<void> TemporalClient::terminate_workflow(
    const std::string& workflow_id, std::optional<std::string> reason,
    std::optional<std::string> run_id) {
    // TODO: Implement via interceptor chain and bridge RPC
    co_return;
}

async_::Task<std::vector<WorkflowExecution>> TemporalClient::list_workflows(
    const WorkflowListOptions& options) {
    // TODO: Implement via interceptor chain and bridge RPC
    co_return std::vector<WorkflowExecution>{};
}

async_::Task<WorkflowExecutionCount> TemporalClient::count_workflows(
    const WorkflowCountOptions& options) {
    // TODO: Implement via interceptor chain and bridge RPC
    co_return WorkflowExecutionCount{};
}

} // namespace temporalio::client
