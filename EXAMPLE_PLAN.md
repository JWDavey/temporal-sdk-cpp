# Plan: Create C++ Examples Equivalent to C# SDK Examples

## Context

The C# SDK has integration examples (SimpleBench, SmokeTest) showing end-to-end Temporal patterns: client connects, workflow calls activities, worker runs both. The C++ SDK has 3 existing examples but they're incomplete stubs - critically, **no example shows a workflow actually calling an activity** because `Workflow::execute_activity()` doesn't exist yet as a public API, even though the infrastructure (interceptors, commands, pending maps, resolution handlers) is fully implemented.

## Gap Analysis

| Feature | C# Coverage | C++ Status |
|---|---|---|
| Client connect + start workflow | SimpleBench, SmokeTest | hello_world (done) |
| Signals + queries | Tests | signal_workflow (done) |
| Activity definition + worker | SimpleBench | activity_worker (done, but workflow is a stub) |
| **Workflow calls activity** | **SimpleBench** | **MISSING - needs API** |
| Timers + conditions | Tests | API exists, no example |
| Workflow updates | Tests | API exists, no example |
| Versioning/patches | Tests | API exists, no example |

## Phase A: Add `Workflow::execute_activity()` API

This is the prerequisite for the most important example. Follows the exact pattern of `Workflow::delay()` → `WorkflowContext::start_timer()` → `WorkflowInstance::start_timer()`.

### A1. Create `activity_options.h`

**New file:** `cpp/include/temporalio/workflows/activity_options.h`

```cpp
struct ActivityOptions {
    std::optional<std::chrono::milliseconds> schedule_to_close_timeout{};
    std::optional<std::chrono::milliseconds> schedule_to_start_timeout{};
    std::optional<std::chrono::milliseconds> start_to_close_timeout{};
    std::optional<std::chrono::milliseconds> heartbeat_timeout{};
    std::optional<common::RetryPolicy> retry_policy{};
    ActivityCancellationType cancellation_type{kTryCancel};
    std::optional<std::stop_token> cancellation_token{};
    std::optional<std::string> task_queue{};
    std::optional<std::string> activity_id{};
};
```

Reuses existing `common::RetryPolicy` from `cpp/include/temporalio/common/retry_policy.h`.

### A2. Add `schedule_activity()` to `WorkflowContext`

**Modify:** `cpp/include/temporalio/workflows/workflow.h`

Add virtual method to `WorkflowContext`:
```cpp
virtual async_::Task<std::any> schedule_activity(
    const std::string& activity_type,
    std::vector<std::any> args,
    const ActivityOptions& options) = 0;
```

### A3. Add `ScheduleActivityData` + implement in `WorkflowInstance`

**Modify:** `cpp/include/temporalio/worker/workflow_instance.h` - add `ScheduleActivityData` struct, declare override
**Modify:** `cpp/src/temporalio/worker/workflow_instance.cpp` - implement using TCS pattern:
1. `++activity_counter_` for sequence number
2. Create `TaskCompletionSource<any>` with `make_resume_callback()`
3. Store in `pending_activities_[seq]`
4. Emit `kScheduleActivity` command with `ScheduleActivityData`
5. Register cancel callback via `stop_token`
6. `co_await tcs->task()` → inspect `ActivityResolution` status

### A4. Add `Workflow::execute_activity()` static methods

**Modify:** `cpp/include/temporalio/workflows/workflow.h` - declare methods
**Modify:** `cpp/src/temporalio/workflows/workflow.cpp` - implement via `require_context().schedule_activity()`

Three overloads: `(name, vector<any> args, opts)`, `(name, any arg, opts)`, `(name, opts)`

### A5. Wire command serialization

**Modify:** `cpp/src/temporalio/worker/internal/workflow_worker.cpp`

Update `kScheduleActivity` case to extract `ScheduleActivityData` and set all protobuf fields (activity_type, task_queue, timeouts, retry_policy, args as payloads).

### A6. Add unit tests

**Modify:** `cpp/tests/worker/workflow_instance_tests.cpp`

- Test: `kScheduleActivity` command emitted with correct fields
- Test: `kResolveActivity` job → workflow completes with activity result
- Test: Activity failure → workflow failure
- Test: Activity cancellation

## Phase B: Create New Examples

### B1. `workflow_activity` -- End-to-End (depends on Phase A)

**New file:** `cpp/examples/workflow_activity/main.cpp`

The most critical example. Mirrors C# SimpleBench pattern.

- Activity: `greet(string name) → "Hello, " + name + "!"`
- Workflow: calls `Workflow::execute_activity("greet", name, {start_to_close_timeout=30s})`
- Main: connects client, creates worker on background thread (`std::jthread`), starts workflow, gets result, shuts down worker
- Demonstrates complete Temporal lifecycle: client → worker → workflow → activity → result

### B2. `timer_workflow` -- Timers & Conditions (no Phase A dependency)

**New file:** `cpp/examples/timer_workflow/main.cpp`

- Workflow with `Workflow::delay()` and `Workflow::wait_condition(pred, timeout)`
- Signal handler sets state, condition waits for it
- Demonstrates deterministic time (`Workflow::utc_now()`)
- Shows timeout fallback pattern

### B3. `update_workflow` -- Workflow Updates (no Phase A dependency)

**New file:** `cpp/examples/update_workflow/main.cpp`

- Shopping cart workflow: update handler with validator (`validate_add_item` + `add_item`)
- Query handler for item count
- Signal for checkout
- `Workflow::wait_condition(() => all_handlers_finished())` for graceful drain
- Shows the validator/handler/query/signal combination

### B4. CMakeLists.txt Update

**Modify:** `cpp/examples/CMakeLists.txt`

Add entries for `example_workflow_activity`, `example_timer_workflow`, `example_update_workflow`.

## Files Modified/Created Summary

| File | Action |
|---|---|
| `cpp/include/temporalio/workflows/activity_options.h` | **CREATE** |
| `cpp/include/temporalio/workflows/workflow.h` | MODIFY (add schedule_activity + execute_activity) |
| `cpp/src/temporalio/workflows/workflow.cpp` | MODIFY (implement execute_activity) |
| `cpp/include/temporalio/worker/workflow_instance.h` | MODIFY (add ScheduleActivityData, declare override) |
| `cpp/src/temporalio/worker/workflow_instance.cpp` | MODIFY (implement schedule_activity) |
| `cpp/src/temporalio/worker/internal/workflow_worker.cpp` | MODIFY (wire protobuf serialization) |
| `cpp/tests/worker/workflow_instance_tests.cpp` | MODIFY (add execute_activity tests) |
| `cpp/examples/workflow_activity/main.cpp` | **CREATE** |
| `cpp/examples/timer_workflow/main.cpp` | **CREATE** |
| `cpp/examples/update_workflow/main.cpp` | **CREATE** |
| `cpp/examples/CMakeLists.txt` | MODIFY (add new targets) |

## Verification

1. **Unit tests:** Run `cd cpp/build && cmake --build . && ctest` — verify workflow_instance_tests pass including new execute_activity tests
2. **Build examples:** `cmake --build . --target example_workflow_activity example_timer_workflow example_update_workflow`
3. **Manual test (requires running Temporal server via `temporal server start-dev`):**
   - Run `example_timer_workflow` — should show timer/condition behavior
   - Run `example_update_workflow` — should show update/query patterns
   - Run `example_workflow_activity` — should show full workflow→activity→result cycle

## Implementation Order

1. A1 → A2 → A3 → A4 → A5 → A6 (sequential: each depends on prior)
2. B2, B3 (parallel with Phase A — use existing APIs only)
3. B1 (after Phase A completes)
4. B4 (after all examples created)
