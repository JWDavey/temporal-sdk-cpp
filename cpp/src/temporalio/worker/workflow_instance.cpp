#include "temporalio/worker/workflow_instance.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "temporalio/exceptions/temporal_exception.h"

namespace temporalio::worker {

WorkflowInstance::WorkflowInstance(Config config)
    : definition_(std::move(config.definition)),
      info_(std::move(config.info)),
      random_(config.randomness_seed) {}

WorkflowInstance::~WorkflowInstance() = default;

std::vector<WorkflowInstance::Command> WorkflowInstance::activate(
    const std::vector<Job>& jobs) {
    commands_.clear();
    current_activation_exception_ = nullptr;

    // Set this instance as the current workflow context
    workflows::WorkflowContextScope scope(this);

    try {
        // Process each job
        for (const auto& job : jobs) {
            switch (job.type) {
                case JobType::kStartWorkflow:
                    handle_start_workflow(job);
                    break;
                case JobType::kFireTimer:
                    handle_fire_timer(job);
                    break;
                case JobType::kResolveActivity:
                    handle_resolve_activity(job);
                    break;
                case JobType::kSignalWorkflow:
                    handle_signal_workflow(job);
                    break;
                case JobType::kQueryWorkflow:
                    handle_query_workflow(job);
                    break;
                case JobType::kResolveChildWorkflow:
                    handle_resolve_child_workflow(job);
                    break;
                case JobType::kCancelWorkflow:
                    handle_cancel_workflow(job);
                    break;
                case JobType::kDoUpdate:
                    handle_do_update(job);
                    break;
                case JobType::kResolveSignalExternalWorkflow:
                    handle_resolve_signal_external_workflow(job);
                    break;
                case JobType::kResolveRequestCancelExternalWorkflow:
                    handle_resolve_request_cancel_external_workflow(job);
                    break;
                case JobType::kNotifyHasPatch:
                    handle_notify_has_patch(job);
                    break;
                case JobType::kResolveNexusOperation:
                    handle_resolve_nexus_operation(job);
                    break;
                case JobType::kUpdateWorkflow:
                    // Legacy update type, handled through kDoUpdate
                    break;
            }
        }

        // Modern event loop: initialize the workflow AFTER all jobs are
        // applied (not during handle_start_workflow). This matches C# where
        // InitializeWorkflow() is called after the job loop.
        if (!workflow_initialized_ && workflow_started_) {
            initialize_workflow();
        }

        // Run the event loop (modern mode: all jobs applied first, then drain)
        bool has_non_query =
            std::any_of(jobs.begin(), jobs.end(), [](const Job& j) {
                return j.type != JobType::kQueryWorkflow;
            });
        run_once(has_non_query);
    } catch (const std::exception& e) {
        // Activation-level failure
        Command fail_cmd;
        fail_cmd.type = CommandType::kFailWorkflow;
        fail_cmd.data = std::string(e.what());
        commands_.push_back(std::move(fail_cmd));
    }

    return std::move(commands_);
}

void WorkflowInstance::run_once(bool check_conds) {
    // Mirrors C# RunOnce(): outer loop keeps going as long as the scheduler
    // has work. Checking conditions may complete TCS's which enqueue more
    // coroutines, so the outer loop ensures chain reactions are fully drained.
    bool has_work = true;
    while (has_work) {
        // Inner loop: drain all queued coroutines
        while (scheduler_.drain()) {
            if (current_activation_exception_) {
                std::rethrow_exception(current_activation_exception_);
            }
        }

        // Check conditions if requested. Completing a condition's TCS may
        // schedule new tasks, which will cause the outer loop to continue.
        has_work = false;
        if (check_conds && !conditions_.empty()) {
            // In modern event loop logic, break after the first condition
            // is resolved (matching C# behavior). The outer loop will re-drain
            // and re-check, ensuring one condition is resolved per iteration.
            auto it = conditions_.begin();
            while (it != conditions_.end()) {
                auto& [pred, tcs] = *it;
                if (pred()) {
                    tcs->try_set_result(true);
                    it = conditions_.erase(it);
                    // A condition was met -- its TCS completion may have
                    // enqueued new tasks. Break to re-drain before checking
                    // more conditions (modern event loop behavior).
                    has_work = true;
                    break;
                } else {
                    ++it;
                }
            }
        }
    }
}

std::stop_token WorkflowInstance::cancellation_token() const {
    return cancellation_source_.token();
}

bool WorkflowInstance::all_handlers_finished() const {
    return in_progress_handler_count_ == 0;
}

const workflows::WorkflowUpdateInfo*
WorkflowInstance::current_update_info() const {
    return current_update_info_ ? &*current_update_info_ : nullptr;
}

bool WorkflowInstance::patched(const std::string& patch_id) {
    // Check memoized first
    auto memo_it = patches_memoized_.find(patch_id);
    if (memo_it != patches_memoized_.end()) {
        return memo_it->second;
    }

    // If notified, it's patched
    bool is_patched = patches_notified_.count(patch_id) > 0;
    if (!is_patched && !is_replaying_) {
        // During non-replay, patched() returns true and emits a command
        is_patched = true;
        Command cmd;
        cmd.type = CommandType::kSetPatchMarker;
        cmd.data = patch_id;
        commands_.push_back(std::move(cmd));
    }
    patches_memoized_[patch_id] = is_patched;
    return is_patched;
}

void WorkflowInstance::deprecate_patch(const std::string& patch_id) {
    // During non-replay, emit the marker
    if (!is_replaying_) {
        Command cmd;
        cmd.type = CommandType::kSetPatchMarker;
        cmd.data = patch_id;
        commands_.push_back(std::move(cmd));
    }
    patches_memoized_[patch_id] = true;
}

void WorkflowInstance::handle_start_workflow(const Job& /*job*/) {
    if (workflow_started_) {
        return;
    }
    workflow_started_ = true;

    // Create the workflow instance but do NOT start the run coroutine yet.
    // In modern event loop mode, InitializeWorkflow() is called AFTER all
    // jobs are applied, not during handle_start_workflow(). This matches the
    // C# pattern where instance creation and args decoding happen here, but
    // the actual coroutine start happens in InitializeWorkflow().
    instance_ = definition_->create_instance();
    if (!instance_) {
        throw std::runtime_error(
            "Failed to create workflow instance for type: " +
            definition_->name());
    }
}

void WorkflowInstance::initialize_workflow() {
    if (workflow_initialized_) {
        throw std::runtime_error("Workflow unexpectedly initialized");
    }
    workflow_initialized_ = true;

    // Start the run coroutine
    auto& run_func = definition_->run_func();
    if (!run_func) {
        throw std::runtime_error("Workflow definition missing run function");
    }

    // Wrap in run_top_level to catch workflow exceptions and convert
    // to commands (ContinueAsNew, Fail, Cancel).
    // is_handler=false: the main workflow run does not count as a handler.
    auto task =
        run_top_level(run_func, instance_.get(), {}, /*is_handler=*/false);
    // Schedule the task's initial suspension point
    scheduler_.schedule(task.handle());
    // Store the task to keep the coroutine alive (prevents UAF when
    // the scheduler holds a handle to the coroutine frame).
    running_tasks_.push_back(std::move(task));
}

void WorkflowInstance::handle_fire_timer(const Job& job) {
    auto seq = std::any_cast<uint32_t>(job.data);
    auto it = pending_timers_.find(seq);
    if (it != pending_timers_.end()) {
        it->second->try_set_result(std::any{});
        pending_timers_.erase(it);
    }
}

void WorkflowInstance::handle_resolve_activity(const Job& job) {
    auto seq = std::any_cast<uint32_t>(job.data);
    auto it = pending_activities_.find(seq);
    if (it != pending_activities_.end()) {
        it->second->try_set_result(std::any{});
        pending_activities_.erase(it);
    }
}

void WorkflowInstance::handle_signal_workflow(const Job& job) {
    // Get signal name and args from job data
    auto& signal_data =
        std::any_cast<const std::pair<std::string, std::vector<std::any>>&>(
            job.data);
    const auto& signal_name = signal_data.first;
    const auto& args = signal_data.second;

    // Look up signal handler.
    // Signal handlers return Task<void> so we wrap them through
    // run_top_level to handle exceptions properly.
    auto& signals = definition_->signals();
    auto it = signals.find(signal_name);

    // Adapter: wraps a Task<void> handler as Task<any> for run_top_level
    auto wrap_void_handler =
        [](std::function<async_::Task<void>(void*, std::vector<std::any>)> fn)
        -> std::function<async_::Task<std::any>(void*, std::vector<std::any>)> {
        return [fn = std::move(fn)](void* inst,
                                    std::vector<std::any> a)
            -> async_::Task<std::any> {
            co_await fn(inst, std::move(a));
            co_return std::any{};
        };
    };

    if (it != signals.end()) {
        ++in_progress_handler_count_;
        auto task = run_top_level(
            wrap_void_handler(it->second.handler), instance_.get(), args,
            /*is_handler=*/true);
        scheduler_.schedule(task.handle());
        running_tasks_.push_back(std::move(task));
    } else if (definition_->dynamic_signal()) {
        ++in_progress_handler_count_;
        auto task = run_top_level(
            wrap_void_handler(definition_->dynamic_signal()->handler),
            instance_.get(), args, /*is_handler=*/true);
        scheduler_.schedule(task.handle());
        running_tasks_.push_back(std::move(task));
    }
    // Else: signal is buffered/ignored
}

void WorkflowInstance::handle_query_workflow(const Job& job) {
    // Queries are handled synchronously -- they don't go through the
    // scheduler. Results use kRespondQuery (not kCompleteWorkflow) and
    // are routed to the query response section of the activation
    // completion, not the workflow command list.
    auto& query_data =
        std::any_cast<const std::pair<std::string, std::vector<std::any>>&>(
            job.data);
    const auto& query_name = query_data.first;
    const auto& args = query_data.second;

    auto& queries = definition_->queries();
    auto it = queries.find(query_name);
    if (it != queries.end()) {
        try {
            auto result = it->second.handler(instance_.get(), args);
            Command cmd;
            cmd.type = CommandType::kRespondQuery;
            cmd.data = std::move(result);
            commands_.push_back(std::move(cmd));
        } catch (const std::exception& e) {
            Command cmd;
            cmd.type = CommandType::kRespondQuery;
            cmd.data = std::string(e.what());  // Error response
            commands_.push_back(std::move(cmd));
        }
    } else if (definition_->dynamic_query()) {
        try {
            auto result =
                definition_->dynamic_query()->handler(instance_.get(), args);
            Command cmd;
            cmd.type = CommandType::kRespondQuery;
            cmd.data = std::move(result);
            commands_.push_back(std::move(cmd));
        } catch (const std::exception& e) {
            Command cmd;
            cmd.type = CommandType::kRespondQuery;
            cmd.data = std::string(e.what());
            commands_.push_back(std::move(cmd));
        }
    }
}

void WorkflowInstance::handle_cancel_workflow(const Job& /*job*/) {
    cancellation_source_.cancel();
}

void WorkflowInstance::handle_resolve_child_workflow(const Job& job) {
    auto seq = std::any_cast<uint32_t>(job.data);
    auto it = pending_child_workflows_.find(seq);
    if (it != pending_child_workflows_.end()) {
        it->second->try_set_result(std::any{});
        pending_child_workflows_.erase(it);
    }
}

void WorkflowInstance::handle_do_update(const Job& job) {
    // TODO: Full implementation requires validation, acceptance, and
    // rejection phases (237 lines in C#). For now, look up the update
    // handler and run it through run_top_level.
    auto& update_data =
        std::any_cast<const std::pair<std::string, std::vector<std::any>>&>(
            job.data);
    const auto& update_name = update_data.first;
    const auto& args = update_data.second;

    auto& updates = definition_->updates();
    auto it = updates.find(update_name);
    if (it != updates.end()) {
        // Run validator synchronously if present
        if (it->second.validator) {
            try {
                it->second.validator(instance_.get(), args);
            } catch (...) {
                // Validation failure -- reject the update
                // TODO: emit proper rejection command
                return;
            }
        }

        // Run handler through run_top_level
        ++in_progress_handler_count_;
        auto task =
            run_top_level(it->second.handler, instance_.get(), args,
                          /*is_handler=*/true);
        scheduler_.schedule(task.handle());
        running_tasks_.push_back(std::move(task));
    } else if (definition_->dynamic_update()) {
        if (definition_->dynamic_update()->validator) {
            try {
                definition_->dynamic_update()->validator(instance_.get(), args);
            } catch (...) {
                return;
            }
        }

        ++in_progress_handler_count_;
        auto task = run_top_level(
            definition_->dynamic_update()->handler, instance_.get(), args,
            /*is_handler=*/true);
        scheduler_.schedule(task.handle());
        running_tasks_.push_back(std::move(task));
    }
    // Else: unknown update is rejected
}

void WorkflowInstance::handle_resolve_signal_external_workflow(
    const Job& /*job*/) {
    // TODO: Implement when signal-external-workflow support is added.
    // The job data should contain a sequence number to resolve a pending
    // TaskCompletionSource.
}

void WorkflowInstance::handle_resolve_request_cancel_external_workflow(
    const Job& /*job*/) {
    // TODO: Implement when cancel-external-workflow support is added.
}

void WorkflowInstance::handle_notify_has_patch(const Job& job) {
    auto patch_id = std::any_cast<std::string>(job.data);
    patches_notified_.insert(std::move(patch_id));
}

void WorkflowInstance::handle_resolve_nexus_operation(const Job& /*job*/) {
    // TODO: Implement when Nexus operation support is added to workflows.
}

async_::Task<std::any> WorkflowInstance::run_top_level(
    std::function<async_::Task<std::any>(void*, std::vector<std::any>)> func,
    void* instance, std::vector<std::any> args, bool is_handler) {
    // Mirrors C# WorkflowInstance.RunTopLevelAsync().
    // Catches workflow-level exceptions and converts them to commands
    // instead of letting them crash the instance.
    try {
        try {
            co_await func(instance, std::move(args));
        } catch (const exceptions::ContinueAsNewException& e) {
            // Workflow wants to continue as new
            Command cmd;
            cmd.type = CommandType::kContinueAsNew;
            cmd.data = std::any(e);
            commands_.push_back(std::move(cmd));
        } catch (const std::exception& e) {
            if (cancellation_source_.is_cancellation_requested() &&
                exceptions::TemporalException::is_canceled_exception(
                    std::current_exception())) {
                // Cancellation requested and this is a cancel exception --
                // emit a cancel command instead of failing the workflow.
                Command cmd;
                cmd.type = CommandType::kCancelWorkflow;
                commands_.push_back(std::move(cmd));
            } else {
                // Workflow failure -- emit a fail command.
                Command cmd;
                cmd.type = CommandType::kFailWorkflow;
                cmd.data = std::string(e.what());
                commands_.push_back(std::move(cmd));
            }
        }
    } catch (const std::exception& e) {
        // Unexpected failure (e.g., failure converter itself threw).
        // This becomes an activation-level exception.
        current_activation_exception_ = std::current_exception();
    }

    // Only decrement handler count for signal/update handlers, not
    // for the main workflow run (which never incremented it).
    if (is_handler && in_progress_handler_count_ > 0) {
        --in_progress_handler_count_;
    }

    co_return std::any{};
}

}  // namespace temporalio::worker
