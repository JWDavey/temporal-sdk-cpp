# Plan: Port Temporal C# SDK to C++20

## Context

The Temporal C# SDK (`src/Temporalio/`) is a mature library (~469 source files, ~412 tests) wrapping a shared Rust `sdk-core` engine via P/Invoke. The goal is to create an equivalent C++20 library that:

- Builds on both **Windows** (MSVC 2022+) and **Linux** (GCC 11+ / Clang 14+)
- Minimizes third-party dependencies (prefers `std` library)
- Remains fully asynchronous using C++20 coroutines
- Calls the Rust `sdk-core` C FFI directly (no P/Invoke overhead)
- Includes all extension equivalents (OpenTelemetry, metrics)
- Converts and passes all ~412 tests

## Current Status

> **Last updated:** 2026-02-27

### Summary

All 7 implementation phases have been completed at the **skeleton/structural level**. The full
C++ project contains **118 source files** (36 public headers, 32 src files, 37 test files,
5 extension files, 3 examples, 5 CMakeLists.txt) with **687 unit tests** defined across
37 test files.

### What Has Been Built

| Category | Count | Status |
|----------|-------|--------|
| Public headers (`cpp/include/`) | 33 | Complete |
| Extension headers (`cpp/extensions/*/include/`) | 3 | Complete |
| Implementation files (`cpp/src/`) | 25 `.cpp` + 7 `.h` | Complete |
| Extension implementations | 2 `.cpp` | Complete |
| Test files (`cpp/tests/`) | 37 (incl. `main.cpp`) | Complete |
| Example programs | 3 | Complete |
| CMake build files | 5 | Complete |
| Build config (`vcpkg.json`, `.cmake`) | 3 | Complete |
| **Total C++ files** | **118** | |
| **Total test cases (TEST/TEST_F)** | **687** | |

### Phase Completion Status

| Phase | Description | Status | Notes |
|-------|-------------|--------|-------|
| Phase 1 | Foundation (CMake, async primitives, bridge) | **COMPLETE** | CMake + vcpkg, Task\<T\>, CancellationToken, CoroutineScheduler, SafeHandle, CallScope, interop.h |
| Phase 2 | Core Types (exceptions, converters, common) | **COMPLETE** | Full exception hierarchy (20+ classes), DataConverter, MetricMeter, SearchAttributes, RetryPolicy, enums |
| Phase 3 | Runtime & Client | **COMPLETE** | TemporalRuntime, TemporalConnection, TemporalClient, WorkflowHandle, ClientOutboundInterceptor |
| Phase 4 | Workflows & Activities | **COMPLETE** | WorkflowDefinition builder, WorkflowInstance determinism engine, ActivityDefinition, TemporalWorker, sub-workers |
| Phase 5 | Nexus & Testing | **COMPLETE** | NexusServiceDefinition, OperationHandler, WorkflowEnvironment, ActivityEnvironment |
| Phase 6 | Extensions | **COMPLETE** | TracingInterceptor (OpenTelemetry), CustomMetricMeter (Diagnostics) |
| Phase 7 | Tests | **COMPLETE** | 687 tests across 37 files covering all components |

### Critical Bugs Found and Fixed During Implementation

1. **Nexus ODR violation** — Duplicate `NexusOperationExecutionContext` definitions in two headers. Fixed by consolidating into `operation_handler.h` with `ContextScope` RAII class.
2. **WorkflowInstance `run_once()` missing condition loop** — Conditions that became true could trigger new conditions, requiring a loop. Fixed with proper `while` loop matching C# behavior.
3. **WorkflowInstance handler count bug** — `run_top_level` decremented handler count for the main workflow run (should only decrement for signal/update handlers). Fixed with `is_handler` parameter.
4. **WorkflowInstance initialization sequence** — Missing `workflow_initialized_` flag causing premature initialization. Fixed with two-phase split of `handle_start_workflow`.
5. **Missing `activity_environment.h` header** — Test file referenced a header that didn't exist. Created the missing header.

---

## PENDING WORK

> **IMPORTANT: No builds or tests have been run yet.** The code has been written but not
> compiled or tested against an actual compiler. The items below represent the remaining
> work to bring this project to a production-ready state.

### 1. Build Verification (HIGH PRIORITY)

The C++ code has never been compiled. The first priority is:

- [ ] Run `cmake -B build -S cpp` and resolve any configuration errors
- [ ] Run `cmake --build build` and fix all compilation errors
- [ ] Ensure the Rust `sdk-core-c-bridge` builds and links correctly
- [ ] Verify `vcpkg` installs all dependencies (protobuf, nlohmann/json, gtest, opentelemetry)
- [ ] Test on MSVC 2022+ (Windows) and GCC 11+ / Clang 14+ (Linux)

### 2. Bridge FFI Integration (HIGH PRIORITY)

Throughout the codebase, bridge calls are marked with `TODO(bridge)` comments indicating
where real FFI calls to the Rust `sdk-core-c-bridge` need to be wired up. Key areas:

- [ ] `interop.h` — Verify all `extern "C"` declarations match the actual Rust C header
- [ ] `bridge/runtime.cpp` — Wire `temporal_core_runtime_new` / `temporal_core_runtime_free`
- [ ] `bridge/client.cpp` — Wire `temporal_core_client_connect` and all RPC calls
- [ ] `bridge/worker.cpp` — Wire poll/complete/shutdown functions
- [ ] Link the compiled Rust `.lib`/`.a` library via CMake

### 3. Test Execution (HIGH PRIORITY)

- [ ] Get Google Test tests compiling and running via `ctest`
- [ ] Fix any test failures from compilation issues
- [ ] Validate unit tests pass without a live server (pure logic tests)
- [ ] Set up `WorkflowEnvironmentFixture` to auto-download the local dev server
- [ ] Run integration tests against a live Temporal server
- [ ] Target: all 687 tests green

### 4. Protobuf Integration (MEDIUM PRIORITY)

- [ ] Generate C++ protobuf types from Temporal API `.proto` files
- [ ] Wire protobuf serialization/deserialization in bridge layer
- [ ] Replace `std::vector<uint8_t>` raw bytes with proper protobuf message types where appropriate

### 5. Converter Implementation (MEDIUM PRIORITY)

- [ ] Implement `JsonPayloadConverter` using nlohmann/json
- [ ] Implement `ProtobufPayloadConverter` using protobuf library
- [ ] Implement `DefaultFailureConverter` (Failure proto ↔ exception mapping)
- [ ] Wire `IPayloadCodec` pipeline for encryption support

### 6. Worker Poll Loop Wiring (MEDIUM PRIORITY)

- [ ] Wire `TemporalWorker::execute_async()` to spawn sub-worker poll loops
- [ ] Wire `WorkflowWorker` to poll activations and dispatch to `WorkflowInstance`
- [ ] Wire `ActivityWorker` to poll tasks and dispatch to registered activities
- [ ] Wire `NexusWorker` to poll tasks and dispatch to operation handlers
- [ ] Implement graceful shutdown with `std::stop_token`

### 7. CI/CD Pipeline (LOWER PRIORITY)

- [ ] GitHub Actions workflow for Windows (MSVC) + Linux (GCC + Clang)
- [ ] AddressSanitizer and UndefinedBehaviorSanitizer CI runs
- [ ] Code coverage reporting
- [ ] Example programs verified in CI

### 8. Documentation & Packaging (LOWER PRIORITY)

- [ ] Doxygen generation from `@file` / `///` doc comments
- [ ] Install targets (`cmake --install`) produce correct header + lib layout
- [ ] `find_package(temporalio)` support via CMake config files
- [ ] README.md with build instructions and getting started guide

---

## Third-Party Dependencies

| Library | Purpose | Source |
|---------|---------|--------|
| **Protobuf** (required) | Temporal API types, bridge serialization | vcpkg |
| **nlohmann/json** | JSON payload conversion (replaces System.Text.Json) | vcpkg (header-only) |
| **Google Test + GMock** | Test framework (replaces xUnit) | vcpkg |
| **OpenTelemetry C++ SDK** | Tracing/metrics extensions | vcpkg |
| **Rust toolchain + Cargo** | Building the `sdk-core-c-bridge` native library | Pre-installed |

No other third-party libraries. Everything else uses C++20 standard library.

---

## Actual Project Structure

> **Note:** The C++ project lives under `cpp/` (not at repository root) to avoid
> case-insensitive path collisions with the C# `src/Temporalio/` on Windows.

```
temporal-sdk-cpp/
  cpp/
    CMakeLists.txt                              # Top-level CMake (vcpkg toolchain)
    vcpkg.json                                  # Dependency manifest
    cmake/
      Platform.cmake                            # Platform detection, Rust cargo build
      CompilerWarnings.cmake                    # Shared -Wall -Werror flags

    include/temporalio/                         # PUBLIC headers (33 files)
      version.h
      async_/
        cancellation_token.h                    # Wraps std::stop_token/std::stop_source
        coroutine_scheduler.h                   # Deterministic FIFO workflow executor
        task.h                                  # Lazy coroutine Task<T> with symmetric transfer
        task_completion_source.h                # Callback-to-coroutine bridge
      client/
        temporal_client.h                       # Workflow CRUD, schedules
        temporal_connection.h                   # gRPC connection (thread-safe)
        workflow_handle.h                       # Value type: client + workflow ID + run ID
        workflow_options.h                      # Start/signal/query/update options
        interceptors/
          client_interceptor.h                  # IClientInterceptor + ClientOutboundInterceptor
      common/
        enums.h                                 # Priority, VersioningBehavior, ParentClosePolicy, etc.
        metric_meter.h                          # MetricMeter, Counter, Histogram, Gauge
        retry_policy.h                          # RetryPolicy struct
        search_attributes.h                     # SearchAttributeKey<T>, SearchAttributeCollection
        workflow_history.h                      # WorkflowHistory for replay
      converters/
        data_converter.h                        # DataConverter, IPayloadConverter, IFailureConverter
      exceptions/
        temporal_exception.h                    # Full exception hierarchy (20+ classes)
      nexus/
        operation_handler.h                     # NexusServiceDefinition, OperationHandler, ContextScope
      runtime/
        temporal_runtime.h                      # TemporalRuntime, options, telemetry config
      testing/
        activity_environment.h                  # Isolated activity testing
        workflow_environment.h                  # Local dev server lifecycle
      worker/
        temporal_worker.h                       # TemporalWorker + TemporalWorkerOptions
        workflow_instance.h                     # Per-execution determinism engine
        workflow_replayer.h                     # WorkflowReplayer for replay testing
        interceptors/
          worker_interceptor.h                  # IWorkerInterceptor + inbound/outbound interceptors
        internal/
          activity_worker.h                     # Activity task poller/dispatcher
          nexus_worker.h                        # Nexus task poller/dispatcher
          workflow_worker.h                     # Workflow activation poller/dispatcher
      workflows/
        workflow.h                              # Workflow ambient API (static methods)
        workflow_definition.h                   # WorkflowDefinition builder + registration
        workflow_info.h                         # WorkflowInfo, WorkflowUpdateInfo

    src/temporalio/                             # PRIVATE implementation (25 .cpp + 7 .h)
      temporalio.cpp                            # Library version info
      activities/
        activity_context.cpp
      async_/
        coroutine_scheduler.cpp
      bridge/
        byte_array.h                            # ByteArray RAII wrapper for Rust-allocated bytes
        call_scope.h                            # Scoped FFI memory management
        client.h / client.cpp                   # Client FFI wrappers
        interop.h                               # C FFI declarations (extern "C")
        runtime.h / runtime.cpp                 # Runtime FFI wrappers
        safe_handle.h                           # RAII handle template
        worker.h / worker.cpp                   # Worker FFI wrappers
      client/
        interceptors/client_interceptor.cpp
        temporal_client.cpp
        temporal_connection.cpp
        workflow_handle.cpp
      common/
        metric_meter.cpp
        workflow_history.cpp
      converters/
        data_converter.cpp
      exceptions/
        temporal_exception.cpp
      nexus/
        operation_handler.cpp
      runtime/
        temporal_runtime.cpp
      testing/
        activity_environment.cpp
        workflow_environment.cpp
      worker/
        internal/
          activity_worker.cpp
          nexus_worker.cpp
          workflow_worker.cpp
        temporal_worker.cpp
        workflow_instance.cpp
        workflow_replayer.cpp
      workflows/
        workflow.cpp

    extensions/
      opentelemetry/                            # TracingInterceptor (3 files)
        CMakeLists.txt
        include/temporalio/extensions/opentelemetry/
          tracing_interceptor.h
          tracing_options.h
        src/
          tracing_interceptor.cpp
      diagnostics/                              # CustomMetricMeter (2 files)
        CMakeLists.txt
        include/temporalio/extensions/diagnostics/
          custom_metric_meter.h
        src/
          custom_metric_meter.cpp

    tests/                                      # Google Test suite (37 files, 687 tests)
      CMakeLists.txt
      main.cpp                                  # Custom gtest main with env fixture
      activities/
        activity_context_tests.cpp
        activity_definition_tests.cpp
      async/
        cancellation_token_tests.cpp
        coroutine_scheduler_tests.cpp
        task_completion_source_tests.cpp
        task_tests.cpp
      bridge/
        call_scope_tests.cpp
        safe_handle_tests.cpp
      client/
        client_interceptor_tests.cpp
        client_options_tests.cpp
        connection_options_tests.cpp
        workflow_handle_tests.cpp
      common/
        enums_tests.cpp
        metric_meter_tests.cpp
        retry_policy_tests.cpp
        search_attributes_tests.cpp
        workflow_history_tests.cpp
      converters/
        data_converter_tests.cpp
        failure_converter_tests.cpp
      exceptions/
        temporal_exception_tests.cpp
      extensions/
        diagnostics/
          custom_metric_meter_tests.cpp
        opentelemetry/
          tracing_interceptor_tests.cpp
          tracing_options_tests.cpp
      general/
        version_tests.cpp
      nexus/
        operation_handler_tests.cpp
      runtime/
        temporal_runtime_tests.cpp
      testing/
        activity_environment_tests.cpp
        workflow_environment_tests.cpp
      worker/
        internal_worker_options_tests.cpp
        worker_interceptor_tests.cpp
        worker_options_tests.cpp
        workflow_instance_tests.cpp
        workflow_replayer_tests.cpp
      workflows/
        workflow_ambient_tests.cpp
        workflow_definition_tests.cpp
        workflow_info_tests.cpp

    examples/
      CMakeLists.txt
      hello_world/main.cpp
      signal_workflow/main.cpp
      activity_worker/main.cpp
```

---

## Phase 1: Foundation (async primitives + bridge + build system)

### 1.1 Project Structure & CMake Build System

**Status: COMPLETE**

**Key CMake decisions:**
- Use `vcpkg` manifest mode (`vcpkg.json`) for all dependencies
- Single library target `temporalio` (static or shared via `BUILD_SHARED_LIBS`)
- Rust bridge built as a custom command via `cargo build`
- `cmake_minimum_required(VERSION 3.20)`, `CMAKE_CXX_STANDARD 20`
- Extensions are optional CMake targets (`TEMPORALIO_BUILD_EXTENSIONS`)
- Tests are optional via `TEMPORALIO_BUILD_TESTS`

### 1.2 Async Primitives (`include/temporalio/async_/`)

**Status: COMPLETE**

**Files created:**
- `task.h` — `Task<T>` lazy coroutine type with symmetric transfer via `FinalAwaiter`
- `cancellation_token.h` — Wraps `std::stop_token` / `std::stop_source`
- `task_completion_source.h` — Bridges callbacks to coroutines, thread-safe
- `coroutine_scheduler.h` + `.cpp` — Deterministic single-threaded workflow executor with FIFO deque

**Design (replaces C# Task/TaskScheduler/CancellationToken/TaskCompletionSource):**

```cpp
namespace temporalio::async_ {

// Lazy coroutine task - does not execute until awaited
template<typename T = void>
class Task {
public:
    struct promise_type { /* ... */ };
    bool await_ready() const noexcept;
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller);
    T await_resume();
};

// Uses C++20 std::stop_token/std::stop_source
class CancellationTokenSource {
    std::stop_source source_;
public:
    std::stop_token token() const;
    void cancel();
};

// Bridges FFI callbacks to coroutines (replaces C# TaskCompletionSource<T>)
template<typename T = void>
class TaskCompletionSource {
public:
    Task<T> task();
    void set_result(T value);
    void set_exception(std::exception_ptr ex);
};

// Deterministic single-threaded executor for workflow replay
// Replaces C# WorkflowInstance : TaskScheduler (MaximumConcurrencyLevel = 1)
class CoroutineScheduler {
public:
    void schedule(std::coroutine_handle<> handle);
    bool drain();  // Run all queued coroutines, returns true if any ran
private:
    std::deque<std::coroutine_handle<>> ready_queue_;
};

} // namespace temporalio::async_
```

**C# source ported from:** `WorkflowInstance.cs:34` (TaskScheduler), `Bridge/Client.cs:46-70` (TaskCompletionSource pattern)

### 1.3 Bridge Layer (`src/temporalio/bridge/`)

**Status: COMPLETE** (structure complete; FFI wiring pending — see "Pending Work")

**Files created:**
- `safe_handle.h` — RAII handle template with custom deleters
- `call_scope.h` — Scoped memory for FFI calls
- `byte_array.h` — ByteArray RAII wrapper for Rust-allocated byte arrays
- `interop.h` — C FFI declarations (`extern "C"`)
- `runtime.h` / `runtime.cpp` — Runtime FFI wrappers
- `client.h` / `client.cpp` — Client FFI wrappers (connect, RPC, metadata)
- `worker.h` / `worker.cpp` — Worker FFI wrappers (poll, complete, shutdown)

**Design (replaces C# SafeHandle + P/Invoke + Scope):**

```cpp
namespace temporalio::bridge {

// RAII handle for Rust-allocated resources (replaces SafeHandle)
template<typename T, void(*Deleter)(T*)>
class SafeHandle {
    std::unique_ptr<T, decltype([](T* p) { Deleter(p); })> ptr_;
public:
    T* get() const noexcept;
    explicit operator bool() const noexcept;
};

using RuntimeHandle = SafeHandle<TemporalCoreRuntime, temporal_core_runtime_free>;
using ClientHandle  = SafeHandle<TemporalCoreClient, temporal_core_client_free>;
using WorkerHandle  = SafeHandle<TemporalCoreWorker, temporal_core_worker_free>;

// shared_ptr-based handle for shared ownership (Client, Runtime)
template<typename T, void(*Deleter)(T*)>
SharedHandle<T, Deleter> make_shared_handle(T* raw);

// Scoped memory for FFI calls (replaces C# Scope : IDisposable)
class CallScope {
public:
    ~CallScope(); // frees all tracked allocations
    TemporalCoreByteArrayRef byte_array(std::string_view str);
    TemporalCoreByteArrayRef byte_array(std::span<const uint8_t> bytes);
    TemporalCoreByteArrayKeyValueArray byte_array_array_kv(
        const std::vector<std::pair<std::string, std::string>>& pairs);
    template<typename T> T* alloc(const T& value);
private:
    std::vector<std::string> owned_strings_;
};

} // namespace temporalio::bridge
```

**C# source ported from:** `Bridge/Interop/Interop.cs` (C FFI surface), `Bridge/SafeUnmanagedHandle.cs`, `Bridge/Scope.cs`, `Bridge/Client.cs`, `Bridge/Runtime.cs`, `Bridge/Worker.cs`

---

## Phase 2: Core Types (exceptions, converters, common)

### 2.1 Exception Hierarchy (`include/temporalio/exceptions/`)

**Status: COMPLETE**

Direct 1:1 mapping from C# exception classes (20+ exception types in single header):

```
std::exception
  std::runtime_error
    temporalio::exceptions::TemporalException
      ::RpcException                        (gRPC errors with StatusCode enum)
      ::RpcTimeoutOrCanceledException
        ::WorkflowUpdateRpcTimeoutOrCanceledException
      ::FailureException                    (base for Temporal failure protocol)
        ::ApplicationFailureException       (user-thrown errors, retry control)
        ::CanceledFailureException
        ::TerminatedFailureException
        ::TimeoutFailureException           (with TimeoutType enum)
        ::ServerFailureException
        ::ActivityFailureException
        ::ChildWorkflowFailureException
        ::NexusOperationFailureException
        ::NexusHandlerFailureException
        ::WorkflowAlreadyStartedException
        ::ActivityAlreadyStartedException
        ::ScheduleAlreadyRunningException
      ::WorkflowFailedException
      ::ActivityFailedException
      ::ContinueAsNewException
      ::WorkflowContinuedAsNewException
      ::WorkflowQueryFailedException
      ::WorkflowQueryRejectedException
      ::WorkflowUpdateFailedException
      ::AsyncActivityCanceledException
      ::InvalidWorkflowOperationException
        ::WorkflowNondeterminismException
        ::InvalidWorkflowSchedulerException
```

**C# source ported from:** All 30 files in `src/Temporalio/Exceptions/`

### 2.2 Converters (`include/temporalio/converters/`)

**Status: COMPLETE** (interfaces defined; JSON/protobuf implementations pending)

```cpp
class IPayloadConverter {
public:
    virtual ~IPayloadConverter() = default;
    template<typename T> Payload to_payload(const T& value);
    template<typename T> T to_value(const Payload& payload);
    // Type-erased internals use typeid + registered serializers
};

class IFailureConverter { /* exception <-> Failure proto */ };
class IPayloadCodec    { /* encode/decode for encryption */ };

struct DataConverter {
    std::shared_ptr<IPayloadConverter> payload_converter;
    std::shared_ptr<IFailureConverter> failure_converter;
    std::shared_ptr<IPayloadCodec> payload_codec; // optional
    static DataConverter default_instance();
};
```

JSON conversion uses **nlohmann/json** with user-registered `to_json`/`from_json` ADL hooks.
Protobuf conversion uses the protobuf C++ library's `SerializeToString`/`ParseFromString`.

**C# source ported from:** All 22 files in `src/Temporalio/Converters/`

### 2.3 Common Types (`include/temporalio/common/`)

**Status: COMPLETE**

- `RetryPolicy` struct (mirrors C# record)
- `SearchAttributeKey<T>` / `SearchAttributeCollection`
- `MetricMeter`, `MetricCounter<T>`, `MetricHistogram<T>`, `MetricGauge<T>`
- `Priority`, `VersioningBehavior`, `ParentClosePolicy` enums
- `WorkflowHistory` for replay

**C# → C++ type mappings used throughout:**
| C# | C++ |
|----|-----|
| `string?` | `std::optional<std::string>` |
| `TimeSpan?` | `std::optional<std::chrono::milliseconds>` |
| `int?` | `std::optional<int>` |
| `record` | `struct` with `operator==(…) = default` |
| `IReadOnlyCollection<T>` | `const std::vector<T>&` |
| `IReadOnlyDictionary<K,V>` | `const std::unordered_map<K,V>&` |
| `ConcurrentDictionary<K,V>` | `std::unordered_map<K,V>` + `std::shared_mutex` |
| `Lazy<T>` | `std::once_flag` + `std::optional<T>` or custom `Lazy<T>` |
| `LINQ (.Select, .Where, etc.)` | `std::ranges` views + utility functions |
| `Action<T>` / `Func<T,R>` | `std::function<void(T)>` / `std::function<R(T)>` |
| `AsyncLocal<T>` | `thread_local` pointer |

---

## Phase 3: Runtime & Client

### 3.1 Runtime (`include/temporalio/runtime/`)

**Status: COMPLETE**

- `TemporalRuntime` — holds Rust runtime handle, telemetry config. `std::shared_ptr` ownership.
- `TemporalRuntimeOptions` — telemetry, logging, metrics configuration
- Default singleton via `TemporalRuntime::default_instance()`

**C# source ported from:** All 17 files in `src/Temporalio/Runtime/`

### 3.2 Client (`include/temporalio/client/`)

**Status: COMPLETE**

- `TemporalConnection` — gRPC connection (`std::shared_ptr`, thread-safe)
- `TemporalClient` — workflow CRUD, schedules (`std::shared_ptr`)
- `WorkflowHandle<TResult>` — value type (client ptr + ID + run ID)
- `WorkflowOptions`, `SignalWorkflowInput`, `QueryWorkflowInput`, etc.
- All operations return `Task<T>` (coroutine-based)

**Type-safe API (replaces C# Expression Trees):**

```cpp
// Start workflow - member function pointer identifies the method
auto handle = co_await client->start_workflow(
    &GreetingWorkflow::run,    // compile-time method identification
    std::string("World"),      // arguments
    workflow_options
);

// Signal
co_await handle.signal(&GreetingWorkflow::on_greeting, std::string("Hi"));

// Query
auto status = co_await handle.query(&GreetingWorkflow::current_status);
```

Implementation uses template deduction on member function pointers. The workflow name is looked up from the static registry populated during `WorkflowDefinition` creation.

**C# source ported from:** All 151 files in `src/Temporalio/Client/`

### 3.3 Client Interceptors (`include/temporalio/client/interceptors/`)

**Status: COMPLETE**

Chain-of-responsibility pattern using virtual base classes:

```cpp
class ClientOutboundInterceptor {
public:
    explicit ClientOutboundInterceptor(std::unique_ptr<ClientOutboundInterceptor> next);
    virtual Task<WorkflowHandle> start_workflow(StartWorkflowInput input);
    virtual Task<void> signal_workflow(SignalWorkflowInput input);
    // ... all operations with default delegation to next_
protected:
    ClientOutboundInterceptor& next();
};

class IClientInterceptor {
public:
    virtual std::unique_ptr<ClientOutboundInterceptor> intercept_client(
        std::unique_ptr<ClientOutboundInterceptor> next) = 0;
};
```

**C# source ported from:** `src/Temporalio/Client/Interceptors/`

---

## Phase 4: Workflows & Activities

### 4.1 Workflow Registration (replaces C# Attributes + Reflection)

**Status: COMPLETE**

**Builder API (explicit):**
```cpp
auto def = WorkflowDefinition::builder<GreetingWorkflow>("GreetingWorkflow")
    .run(&GreetingWorkflow::run)
    .signal("greeting", &GreetingWorkflow::on_greeting)
    .query("status", &GreetingWorkflow::current_status)
    .build();
worker_options.add_workflow(def);
```

Macros expand to `constexpr` static metadata that the builder discovers at compile time. No runtime reflection needed.

**C# source ported from:** `src/Temporalio/Workflows/WorkflowDefinition.cs`, all attribute files

### 4.2 Workflow Ambient API (`include/temporalio/workflows/workflow.h`)

**Status: COMPLETE**

```cpp
namespace temporalio::workflows {
class Workflow {
public:
    static const WorkflowInfo& info();
    static std::stop_token cancellation_token();
    static Task<void> delay(std::chrono::milliseconds duration, std::stop_token ct = {});
    static Task<void> wait_condition(std::function<bool()> condition, std::stop_token ct = {});
    static std::chrono::system_clock::time_point utc_now();
    // ... mirrors all static methods from C# Workflow class
};
}
```

Context propagation uses `thread_local WorkflowContext*` (since workflow execution is single-threaded via `CoroutineScheduler`).

**C# source ported from:** `src/Temporalio/Workflows/Workflow.cs` (258 lines, 30+ static members)

### 4.3 WorkflowInstance (Determinism Engine)

**Status: COMPLETE**

This is the **most complex single file** in the SDK. The C++ `WorkflowInstance` manages:
- `CoroutineScheduler` for deterministic coroutine execution
- Activation processing (start, signal, query, update, timer, activity, Nexus results)
- Command generation (start timer, schedule activity, start child workflow, etc.)
- Condition checking with chain-reaction loop
- Patch/version support with memoization
- Pending operation tracking (timers, activities, child workflows)
- Modern event loop mode (initialize after all jobs applied)
- `run_top_level()` wrapper for exception-to-command conversion

**C# source ported from:** `src/Temporalio/Worker/WorkflowInstance.cs` (the single most critical file)

### 4.4 Activities (`include/temporalio/activities/`)

**Status: COMPLETE**

```cpp
// Activity context (uses thread_local)
class ActivityExecutionContext {
public:
    static ActivityExecutionContext& current();
    const ActivityInfo& info() const;
    std::stop_token cancellation_token() const;
    void heartbeat(const std::any& details = {});
};
```

**C# source ported from:** All 8 files in `src/Temporalio/Activities/`

### 4.5 Worker (`include/temporalio/worker/`)

**Status: COMPLETE**

```cpp
class TemporalWorker {
public:
    // Runs the worker until shutdown. Returns Task that resolves on completion.
    Task<void> execute_async(std::stop_token shutdown_token);
};

struct TemporalWorkerOptions {
    std::string task_queue;
    std::vector<std::shared_ptr<WorkflowDefinition>> workflows;
    std::vector<std::shared_ptr<ActivityDefinition>> activities;
    std::vector<std::shared_ptr<NexusServiceDefinition>> nexus_services;
    std::vector<std::shared_ptr<IWorkerInterceptor>> interceptors;
    std::shared_ptr<DataConverter> data_converter;
    // ... all tuning options (max_concurrent_*, poll ratios, etc.)
};
```

Internal sub-workers:
- `WorkflowWorker` — polls workflow activations, dispatches to `WorkflowInstance`
- `ActivityWorker` — polls activity tasks, dispatches to registered activities
- `NexusWorker` — polls Nexus tasks, dispatches to operation handlers

Worker interceptors:
- `IWorkerInterceptor` — factory for inbound/outbound interceptors
- `WorkflowInboundInterceptor` / `WorkflowOutboundInterceptor`
- `ActivityInboundInterceptor` / `ActivityOutboundInterceptor`
- `NexusOperationInboundInterceptor` / `NexusOperationOutboundInterceptor`

**C# source ported from:** All 66 files in `src/Temporalio/Worker/`

---

## Phase 5: Nexus & Testing

### 5.1 Nexus (`include/temporalio/nexus/`)

**Status: COMPLETE**

- `NexusServiceDefinition` — Service name + operation map
- `OperationHandler` — Base class with `start`, `cancel`, `fetch_info`, `fetch_result`
- `ContextScope` — RAII class that sets/restores thread-local Nexus context
- `NexusOperationExecutionContext` — Thread-local context with links and request info

**C# source ported from:** All 6 files in `src/Temporalio/Nexus/`

### 5.2 Testing (`include/temporalio/testing/`)

**Status: COMPLETE**

- `WorkflowEnvironment` — manages local dev server lifecycle (auto-download + start)
- `ActivityEnvironment` — isolated activity testing with mock context
- Google Test fixture integration via `WorkflowEnvironmentFixture`

**C# source ported from:** All 7 files in `src/Temporalio/Testing/`

---

## Phase 6: Extensions

### 6.1 OpenTelemetry Extension (`extensions/opentelemetry/`)

**Status: COMPLETE**

Uses the **OpenTelemetry C++ SDK** (via vcpkg) to implement `TracingInterceptor`:
- Implements both `IClientInterceptor` and `IWorkerInterceptor`
- Creates spans for workflow/activity/Nexus operations
- Propagates trace context via Temporal headers
- Configurable via `TracingOptions` (filter function, span naming)

**C# source ported from:** 5 files in `src/Temporalio.Extensions.OpenTelemetry/`

### 6.2 Diagnostics Extension (`extensions/diagnostics/`)

**Status: COMPLETE**

Adapts `temporalio::common::MetricMeter` to the OpenTelemetry Metrics API:
- `CustomMetricMeter` implementing `ICustomMetricMeter`
- Counter, Histogram, Gauge support with tag propagation

**C# source ported from:** 2 files in `src/Temporalio.Extensions.DiagnosticSource/`

---

## Phase 7: Tests

**Status: COMPLETE** (687 tests written; execution pending — see "Pending Work")

### Test Infrastructure

```cpp
// Google Test global fixture (replaces C# WorkflowEnvironment collection fixture)
class WorkflowEnvironmentFixture : public ::testing::Environment {
public:
    void SetUp() override;    // Start local dev server or connect to external
    void TearDown() override;
    std::shared_ptr<TemporalClient> client();
};

// Base class for tests needing a server
class WorkflowEnvironmentTestBase : public ::testing::Test {
protected:
    std::shared_ptr<TemporalClient> client();
    std::string unique_task_queue(); // unique per test
};
```

Environment variables `TEMPORAL_TEST_CLIENT_TARGET_HOST` / `TEMPORAL_TEST_CLIENT_NAMESPACE` for external server testing (same as C#).

### Test Files (37 files, 687 test cases)

| Test File | Area |
|-----------|------|
| `activities/activity_context_tests.cpp` | Activity context propagation |
| `activities/activity_definition_tests.cpp` | Activity registration |
| `async/cancellation_token_tests.cpp` | CancellationToken/Source |
| `async/coroutine_scheduler_tests.cpp` | Deterministic scheduler |
| `async/task_completion_source_tests.cpp` | Callback-to-coroutine bridge |
| `async/task_tests.cpp` | Task\<T\> coroutine type |
| `bridge/call_scope_tests.cpp` | FFI memory management |
| `bridge/safe_handle_tests.cpp` | RAII handle template |
| `client/client_interceptor_tests.cpp` | Interceptor chain |
| `client/client_options_tests.cpp` | Client configuration |
| `client/connection_options_tests.cpp` | Connection options |
| `client/workflow_handle_tests.cpp` | WorkflowHandle operations |
| `common/enums_tests.cpp` | Enum types |
| `common/metric_meter_tests.cpp` | Metrics API |
| `common/retry_policy_tests.cpp` | RetryPolicy validation |
| `common/search_attributes_tests.cpp` | Search attribute types |
| `common/workflow_history_tests.cpp` | WorkflowHistory |
| `converters/data_converter_tests.cpp` | DataConverter |
| `converters/failure_converter_tests.cpp` | FailureConverter |
| `exceptions/temporal_exception_tests.cpp` | Exception hierarchy |
| `extensions/diagnostics/custom_metric_meter_tests.cpp` | Diagnostics extension |
| `extensions/opentelemetry/tracing_interceptor_tests.cpp` | OTel tracing |
| `extensions/opentelemetry/tracing_options_tests.cpp` | OTel options |
| `general/version_tests.cpp` | Version info |
| `nexus/operation_handler_tests.cpp` | Nexus handlers |
| `runtime/temporal_runtime_tests.cpp` | Runtime lifecycle |
| `testing/activity_environment_tests.cpp` | Activity test env |
| `testing/workflow_environment_tests.cpp` | Workflow test env |
| `worker/internal_worker_options_tests.cpp` | Internal worker config |
| `worker/worker_interceptor_tests.cpp` | Worker interceptors |
| `worker/worker_options_tests.cpp` | TemporalWorkerOptions |
| `worker/workflow_instance_tests.cpp` | Determinism engine |
| `worker/workflow_replayer_tests.cpp` | Workflow replay |
| `workflows/workflow_ambient_tests.cpp` | Workflow ambient API |
| `workflows/workflow_definition_tests.cpp` | Workflow registration |
| `workflows/workflow_info_tests.cpp` | WorkflowInfo types |

---

## Namespace Mapping Summary

| C# Namespace | C++ Namespace |
|---|---|
| `Temporalio.Client` | `temporalio::client` |
| `Temporalio.Client.Interceptors` | `temporalio::client::interceptors` |
| `Temporalio.Worker` | `temporalio::worker` |
| `Temporalio.Worker.Interceptors` | `temporalio::worker::interceptors` |
| `Temporalio.Workflows` | `temporalio::workflows` |
| `Temporalio.Activities` | `temporalio::activities` |
| `Temporalio.Converters` | `temporalio::converters` |
| `Temporalio.Common` | `temporalio::common` |
| `Temporalio.Exceptions` | `temporalio::exceptions` |
| `Temporalio.Runtime` | `temporalio::runtime` |
| `Temporalio.Testing` | `temporalio::testing` |
| `Temporalio.Nexus` | `temporalio::nexus` |
| `Temporalio.Bridge` (internal) | `temporalio::bridge` (private) |
| (new) | `temporalio::async_` (coroutine primitives) |
| `Temporalio.Extensions.OpenTelemetry` | `temporalio::extensions::opentelemetry` |
| `Temporalio.Extensions.DiagnosticSource` | `temporalio::extensions::diagnostics` |

---

## Implementation Order

The phases were implemented in order due to dependencies:

1. **Foundation** - CMake + vcpkg + async primitives + bridge layer
2. **Core Types** - Exceptions, converters, common utilities
3. **Runtime & Client** - Connection, client, interceptors
4. **Workflows & Activities** - Registration, definitions, worker, WorkflowInstance
5. **Nexus & Testing** - Nexus handlers, test environment
6. **Extensions** - OpenTelemetry tracing, diagnostics metrics
7. **Tests** - 687 tests across 37 files

---

## Verification Plan

> **Status: NOT YET STARTED** — All items below are pending.

1. **Build verification**: `cmake --build . --config Release` succeeds on both Windows (MSVC) and Linux (GCC)
2. **Unit tests**: All 687 ported tests pass via `ctest`
3. **Integration tests**: Tests that use `WorkflowEnvironmentFixture` successfully start a local dev server and execute workflows
4. **Example programs**: `hello_world`, `signal_workflow`, `activity_worker` examples compile and run
5. **Cross-platform CI**: GitHub Actions matrix with Windows MSVC + Linux GCC + Linux Clang
6. **Memory safety**: Run tests under AddressSanitizer (`-fsanitize=address`) and UndefinedBehaviorSanitizer
7. **Extension verification**: OpenTelemetry tracing interceptor produces valid spans; metrics adapter records counters/histograms
