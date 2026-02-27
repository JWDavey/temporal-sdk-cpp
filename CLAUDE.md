# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Is

The official Temporal .NET SDK — a client library for authoring and running [Temporal](https://temporal.io/) workflows (durable execution) in C#. It wraps the shared Rust `sdk-core` engine via P/Invoke.

## Build & Development Commands

**Prerequisites:** .NET SDK, Rust (`cargo`), Protobuf Compiler (`protoc`). Clone recursively (git submodule in `src/Temporalio/Bridge/sdk-core/`).

```bash
dotnet build                          # Build (also compiles Rust bridge via cargo)
dotnet build --configuration Release  # Release build (uses release-lto Rust profile)
dotnet format                         # Format code (StyleCop + .editorconfig)
dotnet format --verify-no-changes     # Check formatting without modifying
dotnet test                           # Run all tests (auto-downloads local dev server)
```

**Run a single test:**
```bash
dotnet test --filter "FullyQualifiedName=Temporalio.Tests.Client.TemporalClientTests.ConnectAsync_Connection_Succeeds"
```

**Run tests with verbose logging:**
```bash
dotnet test --logger "console;verbosity=detailed"
```

**Run as in-proc test program** (better for debugging native pieces):
```bash
dotnet run --project tests/Temporalio.Tests -- -verbose
dotnet run --project tests/Temporalio.Tests -- -method "*.SomeTestMethod"
```

**Regenerate interop bindings** (after Rust bridge header changes):
```bash
ClangSharpPInvokeGenerator @src/Temporalio/Bridge/GenerateInterop.rsp
```

**Regenerate protobuf types:**
```bash
dotnet run --project src/Temporalio.Api.Generator
```

## Project Structure

Solution: `Temporalio.sln` — 5 projects:

| Project | Purpose |
|---|---|
| `src/Temporalio/` | Core SDK (NuGet package). Targets `netcoreapp3.1`, `netstandard2.0`, `net462` |
| `src/Temporalio.Extensions.Hosting/` | .NET Generic Host / DI integration |
| `src/Temporalio.Extensions.OpenTelemetry/` | OpenTelemetry tracing interceptor |
| `src/Temporalio.Extensions.DiagnosticSource/` | `System.Diagnostics.Metrics` adapter |
| `tests/Temporalio.Tests/` | xUnit tests (targets `net10.0`, `OutputType=Exe`) |

## Architecture

```
User Code (.NET)
     │
Public API Layer (Client, Workflows, Activities, Worker, Testing, Converters)
     │
Bridge Layer (Temporalio.Bridge — SafeHandle wrappers, P/Invoke)
     │
Native Library (temporalio_sdk_core_c_bridge — Rust compiled per-platform)
```

### Key Namespaces in `src/Temporalio/`

- **`Client/`** — `TemporalClient`, `TemporalConnection`, `WorkflowHandle`, gRPC service access, interceptor chain (`IClientInterceptor` → `ClientOutboundInterceptor`)
- **`Workflows/`** — `[Workflow]`/`[WorkflowRun]`/`[WorkflowSignal]`/`[WorkflowQuery]`/`[WorkflowUpdate]` attributes, `Workflow` static class (ambient API for workflow code), `WorkflowDefinition`
- **`Activities/`** — `[Activity]` attribute, `ActivityDefinition`, `ActivityExecutionContext`
- **`Worker/`** — `TemporalWorker`, internal dispatchers (`ActivityWorker`, `WorkflowWorker`, `NexusWorker`), `WorkflowInstance` (determinism engine — custom `TaskScheduler`), `WorkflowReplayer`
- **`Bridge/`** — Rust interop: auto-generated `Interop/Interop.cs` (ClangSharp), `Runtime.cs`/`Client.cs`/`Worker.cs` (SafeHandle wrappers)
- **`Converters/`** — `DataConverter` record (`IPayloadConverter` + `IFailureConverter` + optional `IPayloadCodec`)
- **`Api/`** — Auto-generated protobuf types (30+ subdirs)
- **`Nexus/`** — Nexus RPC operation handlers
- **`Runtime/`** — `TemporalRuntime`, telemetry configuration
- **`Testing/`** — `WorkflowEnvironment` (local dev server + time-skipping support)
- **`Worker/Interceptors/`** — `IWorkerInterceptor` → `WorkflowInboundInterceptor`/`WorkflowOutboundInterceptor`/`ActivityInboundInterceptor`/`ActivityOutboundInterceptor`

### Core Abstractions Flow

1. **`TemporalRuntime`** → holds Rust thread pool + telemetry config
2. **`TemporalConnection`** → gRPC connection to Temporal server (thread-safe, reusable)
3. **`TemporalClient`** → workflow CRUD, schedule management, built on a connection
4. **`TemporalWorker`** → polls a task queue, dispatches to registered workflows/activities
5. **`WorkflowInstance`** → per-execution determinism engine, extends `TaskScheduler` for single-threaded coroutine execution

### Interceptor System

Two independent chains: **client-side** (`IClientInterceptor` → `ClientOutboundInterceptor`) and **worker-side** (`IWorkerInterceptor` → inbound/outbound interceptors for workflows, activities, and Nexus operations). The OpenTelemetry extension implements both in a single `TracingInterceptor` class.

## Code Conventions

- **Nullable enabled** globally, **TreatWarningsAsErrors** enabled
- **LangVersion 9.0** across all TFMs for consistency
- **AllowUnsafeBlocks** in the core project (for native P/Invoke)
- **Allman brace style**, space indentation (enforced by `.editorconfig`)
- **StyleCop analyzers** (`1.2.0-beta`) + **Microsoft.VisualStudio.Threading.Analyzers**
- All public APIs require XML doc comments (`GenerateDocumentationFile=true`)
- `InternalsVisibleTo` exposes internals to `Temporalio.Tests`
- No `this.` prefix required (SA1101 disabled)
- Using directives allowed outside namespace (SA1200 disabled)

## Testing Conventions

- **xUnit** with `Xunit.SkippableFact` for conditional tests
- Tests needing a live server extend `WorkflowEnvironmentTestBase` which uses a collection fixture that auto-downloads/starts a local Temporal dev server
- Set `TEMPORAL_TEST_CLIENT_TARGET_HOST` + `TEMPORAL_TEST_CLIENT_NAMESPACE` env vars to test against an external server
- Test project is also an executable (`OutputType=Exe`) for in-proc debugging with `dotnet run`
