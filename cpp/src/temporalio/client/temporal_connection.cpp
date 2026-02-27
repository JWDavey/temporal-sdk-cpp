#include <temporalio/client/temporal_connection.h>
#include <temporalio/runtime/temporal_runtime.h>

#include <mutex>

namespace temporalio::client {

struct TemporalConnection::Impl {
    bool connected{false};
    std::mutex mutex;
    // TODO: Hold bridge::ClientHandle once bridge is wired up
};

TemporalConnection::TemporalConnection(TemporalConnectionOptions options)
    : impl_(std::make_unique<Impl>()), options_(std::move(options)) {
    // Set default identity if not provided
    if (!options_.identity) {
        // TODO: Use pid@hostname as default
        options_.identity = "cpp-sdk";
    }
}

TemporalConnection::~TemporalConnection() = default;

async_::Task<std::shared_ptr<TemporalConnection>>
TemporalConnection::connect(TemporalConnectionOptions options) {
    auto conn = std::shared_ptr<TemporalConnection>(
        new TemporalConnection(std::move(options)));
    // TODO: Actually connect via bridge
    conn->impl_->connected = true;
    co_return conn;
}

std::shared_ptr<TemporalConnection> TemporalConnection::create_lazy(
    TemporalConnectionOptions options) {
    return std::shared_ptr<TemporalConnection>(
        new TemporalConnection(std::move(options)));
}

async_::Task<bool> TemporalConnection::check_health() {
    // TODO: Implement actual health check via bridge RPC
    co_return impl_->connected;
}

bool TemporalConnection::is_connected() const noexcept {
    return impl_->connected;
}

} // namespace temporalio::client
