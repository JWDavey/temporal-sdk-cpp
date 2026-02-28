# Temporal C++ SDK

[![MIT](https://img.shields.io/github/license/temporalio/sdk-dotnet.svg?style=for-the-badge)](LICENSE)

[Temporal](https://temporal.io/) is a distributed, scalable, durable, and highly available orchestration engine used to
execute asynchronous, long-running business logic in a scalable and resilient way.

**Temporal C++ SDK** is a C++20 client library for authoring and running Temporal workflows, activities, and Nexus
operations. It wraps the shared Rust `sdk-core` engine via a C FFI bridge, providing fully asynchronous coroutine-based
APIs.

> **Status: Pre-release** -- The SDK builds and all 646 unit tests pass on MSVC 2022 (Windows). Integration testing
> against a live Temporal server is in progress. The API is not yet stable.

## Features

- **C++20 coroutines** -- `co_await` / `co_return` for all async operations (workflows, client calls, timers)
- **Lazy `Task<T>`** with symmetric transfer for efficient coroutine chaining
- **Deterministic `CoroutineScheduler`** for workflow replay (single-threaded FIFO, matching the Temporal execution model)
- **Full Temporal API coverage** -- workflows, activities (with `execute_activity()`), signals, queries, updates, child workflows, Nexus operations
- **Type-safe workflow invocation** via template deduction on member function pointers
- **Interceptor chains** for both client-side and worker-side operations
- **Protobuf integration** -- 74 Temporal API proto files auto-generated at build time
- **OpenTelemetry extension** -- tracing interceptor for workflow/activity spans
- **Diagnostics extension** -- metrics adapter for counters, histograms, and gauges

## Prerequisites

- **CMake** 3.20+
- **C++20 compiler**: MSVC 2022+ (Windows), GCC 11+ or Clang 14+ (Linux)
- **Rust toolchain** (`cargo`) -- for building the `sdk-core` C bridge library
- **Git** -- clone recursively to fetch the `sdk-core` submodule

## Quick Start

### Clone

```bash
git clone --recursive https://github.com/temporalio/sdk-dotnet.git temporal-sdk-cpp
cd temporal-sdk-cpp
```

### Build

Dependencies (abseil, protobuf, nlohmann/json, Google Test) are fetched automatically via CMake FetchContent.

```bash
# Configure
cmake -B cpp/build -S cpp

# Build the library
cmake --build cpp/build --target temporalio

# Build and run tests
cmake --build cpp/build --target temporalio_tests
ctest --test-dir cpp/build --output-on-failure
```

To skip optional components:

```bash
cmake -B cpp/build -S cpp \
  -DTEMPORALIO_BUILD_EXTENSIONS=OFF \
  -DTEMPORALIO_BUILD_EXAMPLES=OFF \
  -DTEMPORALIO_BUILD_TESTS=OFF
```

### Usage Example

```cpp
#include <temporalio/client/temporal_client.h>
#include <temporalio/client/temporal_connection.h>
#include <temporalio/worker/temporal_worker.h>
#include <temporalio/workflows/workflow.h>
#include <temporalio/workflows/workflow_definition.h>

using namespace temporalio;

// Define a workflow
class GreetingWorkflow {
public:
    async_::Task<std::string> run(std::string name) {
        co_return "Hello, " + name + "!";
    }
};

// Register and start a worker
async_::Task<void> run_worker() {
    // Connect to Temporal
    client::TemporalConnectionOptions conn_opts;
    conn_opts.target_host = "localhost:7233";
    auto connection = co_await client::TemporalConnection::connect(conn_opts);

    auto client = std::make_shared<client::TemporalClient>(connection);

    // Register the workflow
    auto workflow_def = workflows::WorkflowDefinition::builder<GreetingWorkflow>("GreetingWorkflow")
        .run(&GreetingWorkflow::run)
        .build();

    // Start the worker
    worker::TemporalWorkerOptions worker_opts;
    worker_opts.task_queue = "greeting-queue";
    worker_opts.workflows.push_back(workflow_def);

    worker::TemporalWorker worker(client, worker_opts);
    std::stop_source stop;
    co_await worker.execute_async(stop.get_token());
}
```

## Project Structure

```
cpp/
  CMakeLists.txt                    # Top-level CMake build
  vcpkg.json                       # Dependency manifest
  cmake/
    Platform.cmake                 # Platform detection, Rust cargo build
    CompilerWarnings.cmake         # Warning flags
    ProtobufGenerate.cmake         # Proto code generation

  include/temporalio/              # Public headers (34 files)
    async_/                        # Task<T>, CancellationToken, CoroutineScheduler, TaskCompletionSource
    client/                        # TemporalClient, TemporalConnection, WorkflowHandle
      interceptors/                # IClientInterceptor, ClientOutboundInterceptor
    common/                        # RetryPolicy, SearchAttributes, MetricMeter, enums
    converters/                    # DataConverter, IPayloadConverter, IFailureConverter
    exceptions/                    # 20+ exception types (TemporalException hierarchy)
    nexus/                         # NexusServiceDefinition, OperationHandler
    runtime/                       # TemporalRuntime, telemetry config
    testing/                       # WorkflowEnvironment, ActivityEnvironment
    worker/                        # TemporalWorker, WorkflowInstance
      interceptors/                # IWorkerInterceptor, inbound/outbound interceptors
      internal/                    # ActivityWorker, WorkflowWorker, NexusWorker
    workflows/                     # Workflow ambient API, WorkflowDefinition builder, ActivityOptions

  src/temporalio/                  # Private implementation (25 .cpp + 8 .h)
    bridge/                        # Rust FFI wrappers (SafeHandle, CallScope, interop)

  extensions/
    opentelemetry/                 # TracingInterceptor
    diagnostics/                   # CustomMetricMeter

  tests/                           # Google Test suite (37 files, 646 tests)
  examples/                        # 6 examples (see Examples section below)
```

## Examples

Six examples in `cpp/examples/` demonstrate key Temporal patterns. All require a running Temporal server (`temporal server start-dev`).

| Example | What it demonstrates |
|---------|---------------------|
| `hello_world` | Connect to Temporal, start a workflow, get the result |
| `signal_workflow` | Send signals to a running workflow, query workflow state |
| `activity_worker` | Define activities and register them with a worker |
| `workflow_activity` | **Full lifecycle**: workflow calls `execute_activity()`, worker runs both |
| `timer_workflow` | Deterministic timers (`delay()`), conditions (`wait_condition()`), `utc_now()` |
| `update_workflow` | Update handlers with validators, queries, signals, graceful handler draining |

```bash
# Build all examples
cmake --build cpp/build

# Run an example (requires: temporal server start-dev)
./cpp/build/example_workflow_activity
```

## Architecture

```
User Code (C++20)
       |
Public API Layer (Client, Workflows, Activities, Worker, Testing, Converters)
       |
Bridge Layer (SafeHandle + CallScope + FFI wrappers)
       |
Native Library (temporal_sdk_core_c_bridge -- Rust, compiled via cargo)
```

### Key Design Decisions

| Concept | C++ Approach |
|---------|-------------|
| Async/await | C++20 coroutines (`co_await` / `co_return`) with lazy `Task<T>` |
| Deterministic execution | `CoroutineScheduler` (FIFO deque, single-threaded) |
| Cancellation | `std::stop_token` / `std::stop_source` |
| Context propagation | `thread_local` pointers with RAII scopes |
| FFI memory management | `CallScope` (scoped allocations with `std::deque` for stable references) |
| Rust handle ownership | `SafeHandle<T>` RAII template with custom deleters |
| Callback-to-coroutine bridge | `TaskCompletionSource<T>` |
| Type-erased payloads | `std::any` |
| Shared ownership | `std::shared_ptr` (runtime, connection, client) |
| Pimpl pattern | `std::unique_ptr<Impl>` for ABI stability |

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `TEMPORALIO_BUILD_EXTENSIONS` | `ON` | Build OpenTelemetry and Diagnostics extensions |
| `TEMPORALIO_BUILD_TESTS` | `ON` | Build the Google Test suite |
| `TEMPORALIO_BUILD_EXAMPLES` | `ON` | Build example programs |
| `TEMPORALIO_BUILD_PROTOS` | `ON` | Generate C++ types from Temporal API `.proto` files |
| `BUILD_SHARED_LIBS` | `OFF` | Build as shared library instead of static |

## Dependencies

| Library | Purpose | Source |
|---------|---------|--------|
| **Protobuf** v29.3 | Temporal API types, bridge serialization | FetchContent or vcpkg |
| **abseil-cpp** | Required by Protobuf | FetchContent or vcpkg |
| **nlohmann/json** | JSON payload conversion | FetchContent or vcpkg |
| **Google Test** | Unit testing framework | FetchContent or vcpkg |
| **OpenTelemetry C++** | Tracing extension (optional) | vcpkg |
| **Rust toolchain** | Building the `sdk-core` bridge library | Pre-installed |

All dependencies except Rust are fetched automatically via CMake FetchContent if not found on the system.

## Testing

```bash
# Build and run all tests
cmake --build cpp/build --target temporalio_tests
ctest --test-dir cpp/build --output-on-failure

# Run tests with verbose output
ctest --test-dir cpp/build --output-on-failure --verbose
```

646 unit tests cover:
- Async primitives (Task, CancellationToken, CoroutineScheduler, TaskCompletionSource)
- Bridge layer (CallScope, SafeHandle)
- Client (connection options, interceptors, workflow handle)
- Common types (enums, metrics, retry policy, search attributes)
- Converters (data converter, failure converter)
- Exceptions (full hierarchy)
- Nexus (operation handlers)
- Runtime (lifecycle)
- Testing utilities (activity environment, workflow environment)
- Worker (options, interceptors, workflow instance, replayer)
- Workflows (ambient API, definitions, info types)

Set environment variables to test against an external Temporal server:

```bash
export TEMPORAL_TEST_CLIENT_TARGET_HOST="localhost:7233"
export TEMPORAL_TEST_CLIENT_NAMESPACE="default"
```

## Extensions

### OpenTelemetry Tracing

```cpp
#include <temporalio/extensions/opentelemetry/tracing_interceptor.h>

// Add tracing to client and worker
auto tracing = std::make_shared<extensions::opentelemetry::TracingInterceptor>();
client_opts.interceptors.push_back(tracing);
worker_opts.interceptors.push_back(tracing);
```

### Diagnostics Metrics

```cpp
#include <temporalio/extensions/diagnostics/custom_metric_meter.h>

// Adapt Temporal metrics to OpenTelemetry Metrics API
auto meter = std::make_shared<extensions::diagnostics::CustomMetricMeter>(otel_meter);
runtime_opts.metrics.custom_meter = meter;
```

## Ported From

This SDK is a C++20 port of the [Temporal .NET SDK](https://github.com/temporalio/sdk-dotnet). The original C# SDK
(~469 source files, ~412 tests) was systematically converted to idiomatic C++20, replacing:

- C# `async`/`await` with C++20 coroutines
- .NET `Task<T>` with a custom lazy coroutine `Task<T>`
- P/Invoke with direct C FFI calls
- `CancellationToken` with `std::stop_token`
- Reflection-based registration with template-based builders
- xUnit with Google Test

## License

MIT License - see [LICENSE](LICENSE) for details.
