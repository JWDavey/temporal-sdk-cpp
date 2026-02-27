#include "temporalio/testing/workflow_environment.h"

#include <chrono>
#include <stdexcept>
#include <thread>
#include <utility>

namespace temporalio::testing {

// ── Impl ────────────────────────────────────────────────────────────────────

struct WorkflowEnvironment::Impl {
    // TODO: Hold ephemeral server handle when bridge layer supports it.
    bool owns_server{false};
};

// ── WorkflowEnvironment ────────────────────────────────────────────────────

WorkflowEnvironment::WorkflowEnvironment(
    std::shared_ptr<client::TemporalClient> client)
    : client_(std::move(client)), impl_(std::make_unique<Impl>()) {}

WorkflowEnvironment::~WorkflowEnvironment() = default;

WorkflowEnvironment::WorkflowEnvironment(WorkflowEnvironment&&) noexcept =
    default;

WorkflowEnvironment& WorkflowEnvironment::operator=(
    WorkflowEnvironment&&) noexcept = default;

async_::Task<std::unique_ptr<WorkflowEnvironment>>
WorkflowEnvironment::start_local(
    WorkflowEnvironmentStartLocalOptions options) {
    // TODO: Start dev server via bridge layer:
    //   auto server = co_await bridge::EphemeralServer::start_dev_server(...);
    //   options.connection.target_host = server.target();
    //   auto conn = co_await client::TemporalConnection::connect(options.connection);
    //   auto client = client::TemporalClient::create(conn, options.client);
    //   auto env = std::make_unique<WorkflowEnvironment>(client);
    //   env->impl_->owns_server = true;
    //   co_return env;

    // For now, connect to the provided target host (or localhost:7233)
    if (options.connection.target_host.empty()) {
        options.connection.target_host = "localhost:7233";
    }

    auto conn =
        co_await client::TemporalConnection::connect(options.connection);
    auto client = client::TemporalClient::create(conn, options.client);
    auto env = std::make_unique<WorkflowEnvironment>(client);
    co_return std::move(env);
}

async_::Task<std::unique_ptr<WorkflowEnvironment>>
WorkflowEnvironment::start_time_skipping(
    WorkflowEnvironmentStartTimeSkippingOptions options) {
    // TODO: Start time-skipping test server via bridge layer:
    //   auto server = co_await bridge::EphemeralServer::start_test_server(...);
    //   options.connection.target_host = server.target();
    //   ...

    // For now, connect to the provided target host
    if (options.connection.target_host.empty()) {
        options.connection.target_host = "localhost:7233";
    }

    auto conn =
        co_await client::TemporalConnection::connect(options.connection);
    auto client = client::TemporalClient::create(conn, options.client);
    auto env = std::make_unique<WorkflowEnvironment>(client);
    co_return std::move(env);
}

async_::Task<void> WorkflowEnvironment::delay(
    std::chrono::milliseconds duration) {
    // Non-time-skipping: actual sleep
    // TODO: Use async sleep when async runtime is wired up
    std::this_thread::sleep_for(duration);
    co_return;
}

async_::Task<std::chrono::system_clock::time_point>
WorkflowEnvironment::get_current_time() {
    co_return std::chrono::system_clock::now();
}

async_::Task<void> WorkflowEnvironment::shutdown() {
    // TODO: Shutdown ephemeral server when bridge layer supports it
    co_return;
}

} // namespace temporalio::testing
