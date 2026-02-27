# Plan: Port Temporal C# SDK to C++20

## Context

The Temporal C# SDK (`src/Temporalio/`) is a mature library (~469 source files, ~412 tests) wrapping a shared Rust `sdk-core` engine via P/Invoke. The goal is to create an equivalent C++20 library that:

- Builds on both **Windows** (MSVC 2022+) and **Linux** (GCC 11+ / Clang 14+)
- Minimizes third-party dependencies (prefers `std` library)
- Remains fully asynchronous using C++20 coroutines
- Calls the Rust `sdk-core` C FFI directly (no P/Invoke overhead)
- Includes all extension equivalents (OpenTelemetry, metrics)
- Converts and passes all ~412 tests

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

## Phase 1: Foundation (async primitives + bridge + build system)

### 1.1 Project Structure & CMake Build System

```
temporal-sdk-cpp/
  CMakeLists.txt                         # Top-level CMake (vcpkg toolchain)
  vcpkg.json                             # Dependency manifest
  cmake/
    Platform.cmake                       # Platform detection, Rust cargo build
    CompilerWarnings.cmake               # Shared -Wall -Werror flags
  include/temporalio/                    # PUBLIC headers (installed with library)
    async/                               # Coroutine primitives
    client/                              # Client API + interceptors
    worker/                              # Worker + interceptors
    workflows/                           # Workflow ambient API, definitions
    activities/                          # Activity definitions, context
    converters/                          # Payload/failure conversion
    common/                              # Metrics, search attributes, retry
    exceptions/                          # Exception hierarchy
    runtime/                             # TemporalRuntime, telemetry config
    testing/                             # Test utilities
    nexus/                               # Nexus RPC handlers
  src/temporalio/                        # PRIVATE implementation
    bridge/                              # Rust FFI wrappers (NOT in public headers)
      sdk-core/                          # Git submodule (existing Rust core)
      interop.h                          # C FFI declarations from Rust header
      runtime.cpp / client.cpp / worker.cpp
      safe_handle.h                      # RAII handle template
      call_scope.h                       # Scoped FFI memory management
    client/ worker/ workflows/ activities/
    converters/ common/ exceptions/ runtime/
    testing/ nexus/ async/
  extensions/
    opentelemetry/                       # TracingInterceptor (replaces Extensions.OpenTelemetry)
    diagnostics/                         # Metrics adapter (replaces Extensions.DiagnosticSource)
  tests/
    CMakeLists.txt
    fixtures/                            # Shared test server fixture
    client/ worker/ workflows/ activities/
    converters/ common/ runtime/ testing/
    extensions/
  examples/
    hello_world/ signal_workflow/ activity_worker/
```

**Key CMake decisions:**
- Use `vcpkg` manifest mode (`vcpkg.json`) for all dependencies
- Single library target `temporalio` (static or shared via `BUILD_SHARED_LIBS`)
- Rust bridge built as a custom command via `cargo build`
- `cmake_minimum_required(VERSION 3.20)`, `CMAKE_CXX_STANDARD 20`
- Extensions are optional CMake targets (`TEMPORALIO_BUILD_EXTENSIONS`)

### 1.2 Async Primitives (`include/temporalio/async/`)

**Files to create:**
- `task.h` - `Task<T>` coroutine type (lazy, awaitable)
- `cancellation_token.h` - Wraps `std::stop_token` / `std::stop_source`
- `task_completion_source.h` - Bridges callbacks to coroutines
- `coroutine_scheduler.h` - Deterministic single-threaded workflow executor

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

**C# source to port from:** `WorkflowInstance.cs:34` (TaskScheduler), `Bridge/Client.cs:46-70` (TaskCompletionSource pattern)

### 1.3 Bridge Layer (`src/temporalio/bridge/`)

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

// Scoped memory for FFI calls (replaces C# Scope : IDisposable)
class CallScope {
public:
    ~CallScope(); // frees all tracked allocations
    TemporalCoreByteArrayRef byte_array(std::string_view str);
    TemporalCoreByteArrayRef byte_array(std::span<const uint8_t> bytes);
private:
    std::vector<std::string> owned_strings_;
};

} // namespace temporalio::bridge
```

**C# source to port from:** `Bridge/Interop/Interop.cs` (C FFI surface), `Bridge/SafeUnmanagedHandle.cs`, `Bridge/Scope.cs`, `Bridge/Client.cs`, `Bridge/Runtime.cs`, `Bridge/Worker.cs`

---

## Phase 2: Core Types (exceptions, converters, common)

### 2.1 Exception Hierarchy (`include/temporalio/exceptions/`)

Direct 1:1 mapping from C# exception classes:

```
std::exception
  temporalio::exceptions::TemporalException
    ::RpcException                        (gRPC errors)
    ::FailureException                    (base for Temporal failure protocol)
      ::ApplicationFailureException       (user-thrown errors)
      ::CanceledFailureException
      ::TerminatedFailureException
      ::TimeoutFailureException
      ::ServerFailureException
      ::ActivityFailureException
      ::ChildWorkflowFailureException
      ::NexusOperationFailureException
    ::WorkflowAlreadyStartedException
    ::WorkflowFailedException
    ::WorkflowContinuedAsNewException
    ::WorkflowNondeterminismException
    ...
```

**C# source to port from:** All 30 files in `src/Temporalio/Exceptions/`

### 2.2 Converters (`include/temporalio/converters/`)

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

**C# source to port from:** All 22 files in `src/Temporalio/Converters/`

### 2.3 Common Types (`include/temporalio/common/`)

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

- `TemporalRuntime` - holds Rust runtime handle, telemetry config. `std::shared_ptr` ownership.
- `TemporalRuntimeOptions` - telemetry, logging, metrics configuration
- Default singleton via `TemporalRuntime::default_instance()`

**C# source to port from:** All 17 files in `src/Temporalio/Runtime/`

### 3.2 Client (`include/temporalio/client/`)

- `TemporalConnection` - gRPC connection (`std::shared_ptr`, thread-safe)
- `TemporalClient` - workflow CRUD, schedules (`std::shared_ptr`)
- `WorkflowHandle<TResult>` - value type (client ptr + ID + run ID)
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

**C# source to port from:** All 151 files in `src/Temporalio/Client/`

### 3.3 Client Interceptors (`include/temporalio/client/interceptors/`)

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

**C# source to port from:** `src/Temporalio/Client/Interceptors/`

---

## Phase 4: Workflows & Activities

### 4.1 Workflow Registration (replaces C# Attributes + Reflection)

**Two approaches provided (replaces [Workflow], [WorkflowRun], [WorkflowSignal], etc.):**

**Approach A - Builder API (explicit):**
```cpp
auto def = WorkflowDefinition::builder<GreetingWorkflow>("GreetingWorkflow")
    .run(&GreetingWorkflow::run)
    .signal("greeting", &GreetingWorkflow::on_greeting)
    .query("status", &GreetingWorkflow::current_status)
    .build();
worker_options.add_workflow(def);
```

**Approach B - Macro + auto-registration (convenience):**
```cpp
class GreetingWorkflow {
    TEMPORAL_WORKFLOW_RUN()
    Task<std::string> run(std::string name);

    TEMPORAL_WORKFLOW_SIGNAL("greeting")
    Task<void> on_greeting(std::string greeting);

    TEMPORAL_WORKFLOW_QUERY("status")
    std::string current_status() const;
};
TEMPORAL_REGISTER_WORKFLOW(GreetingWorkflow, "GreetingWorkflow")

// Then:
worker_options.add_workflow<GreetingWorkflow>();
```

Macros expand to `constexpr` static metadata that the builder discovers at compile time. No runtime reflection needed.

**C# source to port from:** `src/Temporalio/Workflows/WorkflowDefinition.cs`, all attribute files

### 4.2 Workflow Ambient API (`include/temporalio/workflows/workflow.h`)

```cpp
namespace temporalio::workflows {
class Workflow {
public:
    static const WorkflowInfo& info();
    static std::stop_token cancellation_token();
    static Task<void> delay(std::chrono::milliseconds duration, std::stop_token ct = {});
    static Task<void> wait_condition(std::function<bool()> condition, std::stop_token ct = {});
    static std::chrono::system_clock::time_point utc_now();
    static ILogger& logger();
    // ... mirrors all static methods from C# Workflow class
};
}
```

Context propagation uses `thread_local WorkflowContext*` (since workflow execution is single-threaded via `CoroutineScheduler`).

**C# source to port from:** `src/Temporalio/Workflows/Workflow.cs` (258 lines, 30+ static members)

### 4.3 WorkflowInstance (Determinism Engine)

This is the **most complex single file** in the SDK. `WorkflowInstance.cs` (1000+ lines) manages:
- Custom TaskScheduler (→ `CoroutineScheduler`)
- Activation processing (start, signal, query, update, timer, activity result)
- Command generation (start timer, schedule activity, start child workflow)
- Condition checking
- Patch/version support
- Stack trace support
- Pending operation tracking (timers, activities, child workflows, signals)

Port approach: Map the `RunOnce()` loop and `QueueTask` override to `CoroutineScheduler::drain()`. Map each activation job type to a handler method. Map each command type to a command struct.

**C# source to port from:** `src/Temporalio/Worker/WorkflowInstance.cs` (the single most critical file)

### 4.4 Activities (`include/temporalio/activities/`)

```cpp
// Activity registration - simpler than workflows (just functions)
worker_options.add_activity("greet", &greet_function);
worker_options.add_activity("process", [](int x) -> Task<int> { co_return x * 2; });
worker_options.add_activity("method", &MyClass::method, &instance);

// Activity context (uses thread_local)
class ActivityExecutionContext {
public:
    static ActivityExecutionContext& current();
    const ActivityInfo& info() const;
    std::stop_token cancellation_token() const;
    void heartbeat(const std::any& details = {});
};
```

**C# source to port from:** All 8 files in `src/Temporalio/Activities/`

### 4.5 Worker (`include/temporalio/worker/`)

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
    std::vector<std::shared_ptr<IWorkerInterceptor>> interceptors;
    // ...
};
```

**C# source to port from:** All 66 files in `src/Temporalio/Worker/`

---

## Phase 5: Nexus & Testing

### 5.1 Nexus (`include/temporalio/nexus/`)

Port the 6 Nexus files for RPC operation handlers.

### 5.2 Testing (`include/temporalio/testing/`)

- `WorkflowEnvironment` - manages local dev server lifecycle
- `ActivityEnvironment` - isolated activity testing
- Test fixture for Google Test integration

**C# source to port from:** All 7 files in `src/Temporalio/Testing/`

---

## Phase 6: Extensions

### 6.1 OpenTelemetry Extension (`extensions/opentelemetry/`)

Uses the **OpenTelemetry C++ SDK** (via vcpkg) to implement `TracingInterceptor`:
- Implements both `IClientInterceptor` and `IWorkerInterceptor`
- Creates spans for workflow/activity/Nexus operations
- Propagates trace context via Temporal headers

**C# source to port from:** 5 files in `src/Temporalio.Extensions.OpenTelemetry/`

### 6.2 Diagnostics Extension (`extensions/diagnostics/`)

Adapts `temporalio::common::MetricMeter` to the OpenTelemetry Metrics API:
- `CustomMetricMeter` implementing `ICustomMetricMeter`
- Counter, Histogram, Gauge support

**C# source to port from:** 2 files in `src/Temporalio.Extensions.DiagnosticSource/`

---

## Phase 7: Tests

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

### Test Files to Port (~412 tests across 38 files)

| C# Test File | C++ Test File | Tests |
|---|---|---|
| `ActivityDefinitionTests.cs` | `activities/activity_definition_tests.cpp` | 23 |
| `TemporalClientTests.cs` | `client/temporal_client_tests.cpp` | 3 |
| `TemporalClientWorkflowTests.cs` | `client/workflow_tests.cpp` | 13 |
| `TemporalClientActivityTests.cs` | `client/activity_tests.cpp` | 12 |
| `TemporalClientScheduleTests.cs` | `client/schedule_tests.cpp` | 4 |
| `TemporalConnectionOptionsTests.cs` | `client/connection_options_tests.cpp` | 10 |
| `ClientConfigTests.cs` | `common/client_config_tests.cpp` | 40 |
| `WorkflowWorkerTests.cs` | `worker/workflow_worker_tests.cpp` | 118 |
| `ActivityWorkerTests.cs` | `worker/activity_worker_tests.cpp` | 35 |
| `NexusWorkerTests.cs` | `worker/nexus_worker_tests.cpp` | 27 |
| `WorkflowDefinitionTests.cs` | `workflows/workflow_definition_tests.cpp` | 35 |
| ... (remaining 27 files) | ... | ~92 |

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

---

## Implementation Order

The phases must be implemented in order due to dependencies:

1. **Foundation** - CMake + vcpkg + async primitives + bridge layer
2. **Core Types** - Exceptions, converters, common utilities
3. **Runtime & Client** - Connection, client, interceptors
4. **Workflows & Activities** - Registration, definitions, worker, WorkflowInstance
5. **Nexus & Testing** - Nexus handlers, test environment
6. **Extensions** - OpenTelemetry tracing, diagnostics metrics
7. **Tests** - Port all 412 tests, validate against local dev server

Each phase should be fully compilable and testable before moving to the next.

---

## Verification Plan

1. **Build verification**: `cmake --build . --config Release` succeeds on both Windows (MSVC) and Linux (GCC)
2. **Unit tests**: All ~412 ported tests pass via `ctest`
3. **Integration tests**: Tests that use `WorkflowEnvironmentFixture` successfully start a local dev server and execute workflows
4. **Example programs**: `hello_world`, `signal_workflow`, `activity_worker` examples compile and run
5. **Cross-platform CI**: GitHub Actions matrix with Windows MSVC + Linux GCC + Linux Clang
6. **Memory safety**: Run tests under AddressSanitizer (`-fsanitize=address`) and UndefinedBehaviorSanitizer
7. **Extension verification**: OpenTelemetry tracing interceptor produces valid spans; metrics adapter records counters/histograms
