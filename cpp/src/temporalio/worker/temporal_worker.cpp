#include "temporalio/worker/temporal_worker.h"

#include <stdexcept>
#include <utility>

#include "temporalio/client/temporal_client.h"
#include "temporalio/exceptions/temporal_exception.h"

namespace temporalio::worker {

TemporalWorker::TemporalWorker(std::shared_ptr<client::TemporalClient> client,
                               TemporalWorkerOptions options)
    : client_(std::move(client)), options_(std::move(options)) {
    if (options_.task_queue.empty()) {
        throw std::invalid_argument(
            "TemporalWorkerOptions must have a task_queue set");
    }
    if (options_.workflows.empty() && options_.activities.empty() &&
        options_.nexus_services.empty()) {
        throw std::invalid_argument(
            "Must have at least one workflow, activity, and/or Nexus service");
    }

    // Build workflow name -> definition lookup map
    for (const auto& wf : options_.workflows) {
        if (!wf) {
            throw std::invalid_argument("Null workflow definition provided");
        }
        const auto& name = wf->name();
        if (name.empty()) {
            // Dynamic workflow (unnamed) -- only one allowed
            if (dynamic_workflow_) {
                throw std::invalid_argument(
                    "Multiple dynamic workflows provided");
            }
            dynamic_workflow_ = wf;
        } else {
            auto [_, inserted] = workflow_map_.emplace(name, wf);
            if (!inserted) {
                throw std::invalid_argument(
                    "Duplicate workflow named " + name);
            }
        }
    }

    // Build activity name -> definition lookup map
    for (const auto& act : options_.activities) {
        if (!act) {
            throw std::invalid_argument("Null activity definition provided");
        }
        const auto& name = act->name();
        if (name.empty()) {
            throw std::invalid_argument(
                "Activity definition must have a name");
        }
        auto [_, inserted] = activity_map_.emplace(name, act);
        if (!inserted) {
            throw std::invalid_argument(
                "Duplicate activity named " + name);
        }
    }
}

TemporalWorker::~TemporalWorker() = default;

async_::Task<void> TemporalWorker::execute_async(
    std::stop_token shutdown_token) {
    // The real implementation will:
    // 1. Create a bridge worker via the Rust FFI
    // 2. Start polling loops for workflow tasks and activity tasks
    // 3. Dispatch workflow activations to WorkflowInstance
    // 4. Dispatch activity tasks to ActivityDefinition::execute()
    // 5. Respect the shutdown_token for graceful shutdown
    //
    // For now, this is a structural placeholder that yields until shutdown
    // is requested. The actual bridge integration will be connected when
    // the FFI layer is wired up.

    // TODO(bridge): Create bridge worker from client's bridge connection
    // TODO(bridge): Start workflow poller coroutine
    // TODO(bridge): Start activity poller coroutine
    // TODO(bridge): co_await when_any(pollers..., shutdown_signal)
    // TODO(bridge): Initiate graceful shutdown
    // TODO(bridge): Wait for in-flight tasks to drain
    // TODO(bridge): Finalize shutdown

    co_return;
}

}  // namespace temporalio::worker
