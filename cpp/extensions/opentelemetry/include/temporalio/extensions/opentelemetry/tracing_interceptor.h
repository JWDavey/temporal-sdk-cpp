#pragma once

/// @file tracing_interceptor.h
/// @brief OpenTelemetry tracing interceptor for the Temporal C++ SDK.
///
/// TracingInterceptor implements both IClientInterceptor and
/// IWorkerInterceptor to create and propagate OpenTelemetry spans for
/// client calls, workflow executions, activity executions, and Nexus
/// operations.
///
/// Usage:
///   auto interceptor = std::make_shared<TracingInterceptor>();
///   // Add to client options
///   client_options.interceptors.push_back(interceptor);
///   // It will automatically apply to workers created from this client.

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "temporalio/extensions/opentelemetry/tracing_options.h"
#include "temporalio/worker/interceptors/worker_interceptor.h"

// Include the actual client interceptor interface
// (located in the cpp/ relocated tree or include/)
namespace temporalio::client::interceptors {
class IClientInterceptor;
class ClientOutboundInterceptor;
}  // namespace temporalio::client::interceptors

namespace temporalio::extensions::opentelemetry {

/// OpenTelemetry tracing interceptor for both client and worker sides.
///
/// Creates spans for:
/// - Client: StartWorkflow, SignalWorkflow, QueryWorkflow, etc.
/// - Workflow: Execute, HandleSignal, HandleQuery, HandleUpdate
/// - Activity: Execute
/// - Nexus: StartOperation, CancelOperation
///
/// Propagates trace context via Temporal headers using the configured
/// header key.
class TracingInterceptor
    : public worker::interceptors::IWorkerInterceptor,
      public std::enable_shared_from_this<TracingInterceptor> {
public:
    /// Source names used for span creation.
    static constexpr const char* kClientSourceName =
        "Temporalio.Extensions.OpenTelemetry.Client";
    static constexpr const char* kWorkflowSourceName =
        "Temporalio.Extensions.OpenTelemetry.Workflow";
    static constexpr const char* kActivitySourceName =
        "Temporalio.Extensions.OpenTelemetry.Activity";
    static constexpr const char* kNexusSourceName =
        "Temporalio.Extensions.OpenTelemetry.Nexus";

    /// Create a tracing interceptor with default options.
    TracingInterceptor();

    /// Create a tracing interceptor with the given options.
    explicit TracingInterceptor(TracingInterceptorOptions options);

    ~TracingInterceptor() override;

    /// Get the options.
    const TracingInterceptorOptions& options() const noexcept {
        return options_;
    }

    // -- IWorkerInterceptor --

    /// Create a workflow inbound interceptor that wraps the given next.
    worker::interceptors::WorkflowInboundInterceptor* intercept_workflow(
        worker::interceptors::WorkflowInboundInterceptor* next) override;

    /// Create an activity inbound interceptor that wraps the given next.
    worker::interceptors::ActivityInboundInterceptor* intercept_activity(
        worker::interceptors::ActivityInboundInterceptor* next) override;

    /// Serialize trace context into Temporal headers.
    /// @param headers Existing headers (may be empty).
    /// @return Updated headers with trace context injected.
    std::unordered_map<std::string, std::string> inject_context(
        std::unordered_map<std::string, std::string> headers) const;

    /// Extract trace context from Temporal headers.
    /// @param headers Headers to extract from.
    /// @return True if trace context was found and extracted.
    bool extract_context(
        const std::unordered_map<std::string, std::string>& headers) const;

private:
    TracingInterceptorOptions options_;

    // Owned interceptor instances (kept alive for the interceptor chain)
    std::vector<
        std::unique_ptr<worker::interceptors::WorkflowInboundInterceptor>>
        workflow_interceptors_;
    std::vector<
        std::unique_ptr<worker::interceptors::ActivityInboundInterceptor>>
        activity_interceptors_;
};

}  // namespace temporalio::extensions::opentelemetry
