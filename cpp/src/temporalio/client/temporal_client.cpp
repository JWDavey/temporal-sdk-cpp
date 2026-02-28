#include <temporalio/client/temporal_client.h>
#include <temporalio/client/temporal_connection.h>

#include <temporalio/async_/task_completion_source.h>
#include <temporalio/exceptions/temporal_exception.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "temporalio/bridge/client.h"

namespace temporalio::client {

// ── Protobuf wire-format helpers ────────────────────────────────────────────
// Minimal protobuf encoding for building gRPC requests without generated
// types.  Once protobuf code generation is wired (Task #4), these helpers
// should be replaced by proper message SerializeToString() calls.

namespace proto {

/// Encode a varint into the buffer.
inline void encode_varint(std::vector<uint8_t>& buf, uint64_t value) {
    while (value > 0x7F) {
        buf.push_back(static_cast<uint8_t>((value & 0x7F) | 0x80));
        value >>= 7;
    }
    buf.push_back(static_cast<uint8_t>(value));
}

/// Encode a length-delimited field (wire type 2).
inline void encode_string_field(std::vector<uint8_t>& buf,
                                uint32_t field_number,
                                const std::string& value) {
    if (value.empty()) return;
    // Tag = (field_number << 3) | 2
    encode_varint(buf, (static_cast<uint64_t>(field_number) << 3) | 2);
    encode_varint(buf, value.size());
    buf.insert(buf.end(), value.begin(), value.end());
}

/// Encode a sub-message field (wire type 2).
inline void encode_message_field(std::vector<uint8_t>& buf,
                                 uint32_t field_number,
                                 const std::vector<uint8_t>& msg) {
    if (msg.empty()) return;
    encode_varint(buf, (static_cast<uint64_t>(field_number) << 3) | 2);
    encode_varint(buf, msg.size());
    buf.insert(buf.end(), msg.begin(), msg.end());
}

/// Encode a varint field (wire type 0).
inline void encode_varint_field(std::vector<uint8_t>& buf,
                                uint32_t field_number, uint64_t value) {
    if (value == 0) return;
    encode_varint(buf, (static_cast<uint64_t>(field_number) << 3) | 0);
    encode_varint(buf, value);
}

/// Encode a bytes field (wire type 2) from raw bytes.
inline void encode_bytes_field(std::vector<uint8_t>& buf,
                               uint32_t field_number,
                               const std::vector<uint8_t>& value) {
    if (value.empty()) return;
    encode_varint(buf, (static_cast<uint64_t>(field_number) << 3) | 2);
    encode_varint(buf, value.size());
    buf.insert(buf.end(), value.begin(), value.end());
}

// --- Field numbers from temporal API protobuf definitions ---
// StartWorkflowExecutionRequest:
//   1: namespace, 2: workflow_id, 3: workflow_type (sub-msg, field 1 = name),
//   4: task_queue (sub-msg, field 1 = name), 5: input (Payloads),
//   6: workflow_execution_timeout, 7: workflow_run_timeout,
//   8: workflow_task_timeout, 9: identity, 10: request_id,
//   11: workflow_id_reuse_policy, 12: retry_policy, 13: cron_schedule
//
// SignalWorkflowExecutionRequest:
//   1: namespace, 2: workflow_execution (sub-msg: 1=workflow_id, 2=run_id),
//   3: signal_name, 4: input (Payloads), 5: identity, 6: request_id
//
// QueryWorkflowRequest:
//   1: namespace, 2: execution (sub-msg), 3: query (sub-msg: 1=query_type,
//   2=query_args)
//
// RequestCancelWorkflowExecutionRequest:
//   1: namespace, 2: workflow_execution (sub-msg), 3: identity, 4: request_id,
//   5: first_execution_run_id, 6: reason
//
// TerminateWorkflowExecutionRequest:
//   1: namespace, 2: workflow_execution (sub-msg), 3: reason, 5: identity
//
// ListWorkflowExecutionsRequest:
//   1: namespace, 2: page_size, 3: next_page_token, 4: query
//
// CountWorkflowExecutionsRequest:
//   1: namespace, 2: query

/// Build a WorkflowExecution sub-message.
inline std::vector<uint8_t> workflow_execution(
    const std::string& workflow_id,
    const std::string& run_id = {}) {
    std::vector<uint8_t> buf;
    encode_string_field(buf, 1, workflow_id);
    encode_string_field(buf, 2, run_id);
    return buf;
}

/// Build a WorkflowType sub-message.
inline std::vector<uint8_t> workflow_type(const std::string& name) {
    std::vector<uint8_t> buf;
    encode_string_field(buf, 1, name);
    return buf;
}

/// Build a TaskQueue sub-message.
inline std::vector<uint8_t> task_queue(const std::string& name) {
    std::vector<uint8_t> buf;
    encode_string_field(buf, 1, name);
    return buf;
}

/// Build a WorkflowQuery sub-message.
inline std::vector<uint8_t> workflow_query(const std::string& query_type) {
    std::vector<uint8_t> buf;
    encode_string_field(buf, 1, query_type);
    return buf;
}

/// Convert duration to protobuf Duration (google.protobuf.Duration).
/// Duration: field 1 = seconds (int64), field 2 = nanos (int32).
inline std::vector<uint8_t> duration_msg(std::chrono::milliseconds ms) {
    std::vector<uint8_t> buf;
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(ms);
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
                     ms - secs)
                     .count();
    if (secs.count() > 0) {
        encode_varint_field(buf, 1, static_cast<uint64_t>(secs.count()));
    }
    if (nanos > 0) {
        encode_varint_field(buf, 2, static_cast<uint64_t>(nanos));
    }
    return buf;
}

/// Build a Payloads message wrapping a single JSON-encoded payload.
/// Payloads: field 1 = repeated Payload
/// Payload: field 1 = map<string,bytes> metadata, field 2 = bytes data
/// Map entries: field 1 = string key, field 2 = bytes value
inline std::vector<uint8_t> json_payloads(const std::string& json_data) {
    // Build metadata map entry: "encoding" -> "json/plain"
    std::vector<uint8_t> map_entry;
    encode_string_field(map_entry, 1, "encoding");
    std::string encoding_value = "json/plain";
    encode_bytes_field(map_entry, 2,
                       std::vector<uint8_t>(encoding_value.begin(),
                                            encoding_value.end()));

    // Build the Payload sub-message
    std::vector<uint8_t> payload;
    encode_message_field(payload, 1, map_entry);  // metadata map entry
    std::vector<uint8_t> data_bytes(json_data.begin(), json_data.end());
    encode_bytes_field(payload, 2, data_bytes);  // data

    // Build the Payloads wrapper
    std::vector<uint8_t> payloads;
    encode_message_field(payloads, 1, payload);  // payloads[0]
    return payloads;
}

}  // namespace proto

// ── Impl ────────────────────────────────────────────────────────────────────

struct TemporalClient::Impl {
    std::shared_ptr<TemporalConnection> connection;
    TemporalClientOptions options;
};

TemporalClient::TemporalClient(std::shared_ptr<TemporalConnection> connection,
                               TemporalClientOptions options)
    : impl_(std::make_unique<Impl>()) {
    impl_->connection = std::move(connection);
    impl_->options = std::move(options);
}

TemporalClient::~TemporalClient() = default;

// ── Private RPC helper ──────────────────────────────────────────────────────

namespace {

/// Bridge the callback-based rpc_call_async to a coroutine.
/// Returns the raw response bytes on success, throws RpcException on failure.
async_::Task<std::vector<uint8_t>> rpc_call(
    bridge::Client& client, bridge::RpcCallOptions opts) {
    auto tcs =
        std::make_shared<async_::TaskCompletionSource<std::vector<uint8_t>>>();

    client.rpc_call_async(
        opts,
        [tcs](std::optional<bridge::RpcCallResult> result,
              std::optional<bridge::RpcCallError> error) {
            if (error) {
                tcs->try_set_exception(std::make_exception_ptr(
                    exceptions::RpcException(
                        static_cast<exceptions::RpcException::StatusCode>(
                            error->status_code),
                        error->message, error->details)));
            } else if (result) {
                tcs->try_set_result(std::move(result->response));
            } else {
                tcs->try_set_exception(std::make_exception_ptr(
                    std::runtime_error("RPC call returned no result")));
            }
        });

    co_return co_await tcs->task();
}

}  // namespace

// ── Factory methods ─────────────────────────────────────────────────────────

async_::Task<std::shared_ptr<TemporalClient>> TemporalClient::connect(
    TemporalClientConnectOptions options) {
    auto conn =
        co_await TemporalConnection::connect(std::move(options.connection));
    co_return TemporalClient::create(std::move(conn),
                                     std::move(options.client));
}

std::shared_ptr<TemporalClient> TemporalClient::create(
    std::shared_ptr<TemporalConnection> connection,
    TemporalClientOptions options) {
    return std::shared_ptr<TemporalClient>(
        new TemporalClient(std::move(connection), std::move(options)));
}

// ── Accessors ───────────────────────────────────────────────────────────────

std::shared_ptr<TemporalConnection> TemporalClient::connection()
    const noexcept {
    return impl_->connection;
}

const std::string& TemporalClient::ns() const noexcept {
    return impl_->options.ns;
}

bridge::Client* TemporalClient::bridge_client() const noexcept {
    return impl_->connection ? impl_->connection->bridge_client() : nullptr;
}

// ── Workflow operations ─────────────────────────────────────────────────────

async_::Task<WorkflowHandle> TemporalClient::start_workflow(
    const std::string& workflow_type, const std::string& args,
    const WorkflowOptions& options) {
    if (options.id.empty()) {
        throw std::invalid_argument("Workflow ID is required");
    }
    if (options.task_queue.empty()) {
        throw std::invalid_argument("Task queue is required");
    }

    auto* client = bridge_client();
    if (!client) {
        throw std::runtime_error("Not connected to Temporal server");
    }

    // Build StartWorkflowExecutionRequest
    std::vector<uint8_t> request;
    proto::encode_string_field(request, 1, impl_->options.ns);
    proto::encode_string_field(request, 2, options.id);
    proto::encode_message_field(request, 3,
                                proto::workflow_type(workflow_type));
    proto::encode_message_field(request, 4,
                                proto::task_queue(options.task_queue));

    // Input payload (field 5) - wrap args as a Payloads message
    if (!args.empty()) {
        proto::encode_message_field(request, 5,
                                    proto::json_payloads(args));
    }

    // Timeouts (fields 6, 7, 8)
    if (options.execution_timeout) {
        proto::encode_message_field(
            request, 6, proto::duration_msg(*options.execution_timeout));
    }
    if (options.run_timeout) {
        proto::encode_message_field(
            request, 7, proto::duration_msg(*options.run_timeout));
    }
    if (options.task_timeout) {
        proto::encode_message_field(
            request, 8, proto::duration_msg(*options.task_timeout));
    }

    // Identity (field 9)
    auto identity =
        impl_->connection->options().identity.value_or("cpp-sdk");
    proto::encode_string_field(request, 9, identity);

    // Request ID (field 10)
    if (options.request_id) {
        proto::encode_string_field(request, 10, *options.request_id);
    }

    // Workflow ID reuse policy (field 11)
    if (options.id_reuse_policy !=
        common::WorkflowIdReusePolicy::kUnspecified) {
        proto::encode_varint_field(request, 11,
                                   static_cast<uint64_t>(
                                       options.id_reuse_policy));
    }

    // Cron schedule (field 13)
    if (options.cron_schedule) {
        proto::encode_string_field(request, 13, *options.cron_schedule);
    }

    bridge::RpcCallOptions rpc_opts;
    rpc_opts.service = TemporalCoreRpcService::Workflow;
    rpc_opts.rpc = "StartWorkflowExecution";
    rpc_opts.request = std::move(request);
    rpc_opts.retry = true;

    auto response = co_await rpc_call(*client, std::move(rpc_opts));

    // Parse run_id from StartWorkflowExecutionResponse (field 1 = run_id)
    // For now, extract the first string field from the response.
    std::string run_id;
    size_t pos = 0;
    while (pos < response.size()) {
        uint64_t tag = 0;
        // Decode varint tag
        int shift = 0;
        while (pos < response.size()) {
            uint8_t b = response[pos++];
            tag |= static_cast<uint64_t>(b & 0x7F) << shift;
            shift += 7;
            if ((b & 0x80) == 0) break;
        }
        uint32_t field_num = static_cast<uint32_t>(tag >> 3);
        uint32_t wire_type = static_cast<uint32_t>(tag & 0x7);

        if (wire_type == 2) {
            // Length-delimited
            uint64_t len = 0;
            shift = 0;
            while (pos < response.size()) {
                uint8_t b = response[pos++];
                len |= static_cast<uint64_t>(b & 0x7F) << shift;
                shift += 7;
                if ((b & 0x80) == 0) break;
            }
            if (field_num == 1 && pos + len <= response.size()) {
                run_id.assign(
                    reinterpret_cast<const char*>(response.data() + pos),
                    static_cast<size_t>(len));
            }
            pos += static_cast<size_t>(len);
        } else if (wire_type == 0) {
            // Varint - skip
            while (pos < response.size() && (response[pos] & 0x80)) ++pos;
            if (pos < response.size()) ++pos;
        } else if (wire_type == 1) {
            pos += 8;  // 64-bit
        } else if (wire_type == 5) {
            pos += 4;  // 32-bit
        } else {
            break;  // Unknown wire type
        }
    }

    co_return WorkflowHandle(shared_from_this(), options.id,
                              run_id.empty() ? std::nullopt
                                             : std::optional(run_id),
                              run_id.empty() ? std::nullopt
                                             : std::optional(run_id));
}

WorkflowHandle TemporalClient::get_workflow_handle(
    const std::string& workflow_id,
    std::optional<std::string> run_id) {
    return WorkflowHandle(shared_from_this(), workflow_id, std::move(run_id));
}

async_::Task<void> TemporalClient::signal_workflow(
    const std::string& workflow_id, const std::string& signal_name,
    const std::string& args, std::optional<std::string> run_id) {
    auto* client = bridge_client();
    if (!client) {
        throw std::runtime_error("Not connected to Temporal server");
    }

    // Build SignalWorkflowExecutionRequest
    std::vector<uint8_t> request;
    proto::encode_string_field(request, 1, impl_->options.ns);
    proto::encode_message_field(
        request, 2,
        proto::workflow_execution(workflow_id, run_id.value_or("")));
    proto::encode_string_field(request, 3, signal_name);

    // Signal input (field 4) - wrap args as a Payloads message
    if (!args.empty()) {
        proto::encode_message_field(request, 4,
                                    proto::json_payloads(args));
    }

    auto identity =
        impl_->connection->options().identity.value_or("cpp-sdk");
    proto::encode_string_field(request, 5, identity);

    bridge::RpcCallOptions rpc_opts;
    rpc_opts.service = TemporalCoreRpcService::Workflow;
    rpc_opts.rpc = "SignalWorkflowExecution";
    rpc_opts.request = std::move(request);
    rpc_opts.retry = true;

    co_await rpc_call(*client, std::move(rpc_opts));
}

async_::Task<std::string> TemporalClient::query_workflow(
    const std::string& workflow_id, const std::string& query_type,
    const std::string& args, std::optional<std::string> run_id) {
    auto* client = bridge_client();
    if (!client) {
        throw std::runtime_error("Not connected to Temporal server");
    }

    // Build QueryWorkflowRequest
    std::vector<uint8_t> request;
    proto::encode_string_field(request, 1, impl_->options.ns);
    proto::encode_message_field(
        request, 2,
        proto::workflow_execution(workflow_id, run_id.value_or("")));
    proto::encode_message_field(request, 3,
                                proto::workflow_query(query_type));

    bridge::RpcCallOptions rpc_opts;
    rpc_opts.service = TemporalCoreRpcService::Workflow;
    rpc_opts.rpc = "QueryWorkflow";
    rpc_opts.request = std::move(request);
    rpc_opts.retry = true;

    auto response = co_await rpc_call(*client, std::move(rpc_opts));
    // Return raw response bytes as string for now.
    // When protobuf types are available, this should deserialize the
    // QueryWorkflowResponse and extract the query result payloads.
    co_return std::string(response.begin(), response.end());
}

async_::Task<void> TemporalClient::cancel_workflow(
    const std::string& workflow_id, std::optional<std::string> run_id) {
    auto* client = bridge_client();
    if (!client) {
        throw std::runtime_error("Not connected to Temporal server");
    }

    // Build RequestCancelWorkflowExecutionRequest
    std::vector<uint8_t> request;
    proto::encode_string_field(request, 1, impl_->options.ns);
    proto::encode_message_field(
        request, 2,
        proto::workflow_execution(workflow_id, run_id.value_or("")));

    auto identity =
        impl_->connection->options().identity.value_or("cpp-sdk");
    proto::encode_string_field(request, 3, identity);

    bridge::RpcCallOptions rpc_opts;
    rpc_opts.service = TemporalCoreRpcService::Workflow;
    rpc_opts.rpc = "RequestCancelWorkflowExecution";
    rpc_opts.request = std::move(request);
    rpc_opts.retry = true;

    co_await rpc_call(*client, std::move(rpc_opts));
}

async_::Task<void> TemporalClient::terminate_workflow(
    const std::string& workflow_id, std::optional<std::string> reason,
    std::optional<std::string> run_id) {
    auto* client = bridge_client();
    if (!client) {
        throw std::runtime_error("Not connected to Temporal server");
    }

    // Build TerminateWorkflowExecutionRequest
    std::vector<uint8_t> request;
    proto::encode_string_field(request, 1, impl_->options.ns);
    proto::encode_message_field(
        request, 2,
        proto::workflow_execution(workflow_id, run_id.value_or("")));

    if (reason) {
        proto::encode_string_field(request, 3, *reason);
    }

    auto identity =
        impl_->connection->options().identity.value_or("cpp-sdk");
    proto::encode_string_field(request, 5, identity);

    bridge::RpcCallOptions rpc_opts;
    rpc_opts.service = TemporalCoreRpcService::Workflow;
    rpc_opts.rpc = "TerminateWorkflowExecution";
    rpc_opts.request = std::move(request);
    rpc_opts.retry = true;

    co_await rpc_call(*client, std::move(rpc_opts));
}

async_::Task<std::vector<WorkflowExecution>> TemporalClient::list_workflows(
    const WorkflowListOptions& options) {
    auto* client = bridge_client();
    if (!client) {
        throw std::runtime_error("Not connected to Temporal server");
    }

    // Build ListWorkflowExecutionsRequest
    std::vector<uint8_t> request;
    proto::encode_string_field(request, 1, impl_->options.ns);

    if (options.page_size) {
        proto::encode_varint_field(request, 2,
                                   static_cast<uint64_t>(*options.page_size));
    }

    if (options.query) {
        proto::encode_string_field(request, 4, *options.query);
    }

    bridge::RpcCallOptions rpc_opts;
    rpc_opts.service = TemporalCoreRpcService::Workflow;
    rpc_opts.rpc = "ListWorkflowExecutions";
    rpc_opts.request = std::move(request);
    rpc_opts.retry = true;

    auto response = co_await rpc_call(*client, std::move(rpc_opts));

    // Parse ListWorkflowExecutionsResponse.
    // Field 1 = repeated WorkflowExecutionInfo (sub-messages).
    // WorkflowExecutionInfo field 1 = execution (WorkflowExecution:
    //   field 1 = workflow_id, field 2 = run_id), field 2 = type
    //   (WorkflowType: field 1 = name).
    // Full deserialization requires protobuf types; return raw list for now.
    // When protobuf is available, replace with proper deserialization.
    std::vector<WorkflowExecution> results;
    co_return results;
}

async_::Task<WorkflowExecutionCount> TemporalClient::count_workflows(
    const WorkflowCountOptions& options) {
    auto* client = bridge_client();
    if (!client) {
        throw std::runtime_error("Not connected to Temporal server");
    }

    // Build CountWorkflowExecutionsRequest
    std::vector<uint8_t> request;
    proto::encode_string_field(request, 1, impl_->options.ns);

    if (options.query) {
        proto::encode_string_field(request, 2, *options.query);
    }

    bridge::RpcCallOptions rpc_opts;
    rpc_opts.service = TemporalCoreRpcService::Workflow;
    rpc_opts.rpc = "CountWorkflowExecutions";
    rpc_opts.request = std::move(request);
    rpc_opts.retry = true;

    auto response = co_await rpc_call(*client, std::move(rpc_opts));

    // Parse CountWorkflowExecutionsResponse (field 1 = count, int64).
    WorkflowExecutionCount result;
    size_t pos = 0;
    while (pos < response.size()) {
        uint64_t tag = 0;
        int shift = 0;
        while (pos < response.size()) {
            uint8_t b = response[pos++];
            tag |= static_cast<uint64_t>(b & 0x7F) << shift;
            shift += 7;
            if ((b & 0x80) == 0) break;
        }
        uint32_t field_num = static_cast<uint32_t>(tag >> 3);
        uint32_t wire_type = static_cast<uint32_t>(tag & 0x7);

        if (wire_type == 0) {
            uint64_t value = 0;
            shift = 0;
            while (pos < response.size()) {
                uint8_t b = response[pos++];
                value |= static_cast<uint64_t>(b & 0x7F) << shift;
                shift += 7;
                if ((b & 0x80) == 0) break;
            }
            if (field_num == 1) {
                result.count = static_cast<int64_t>(value);
            }
        } else if (wire_type == 2) {
            uint64_t len = 0;
            shift = 0;
            while (pos < response.size()) {
                uint8_t b = response[pos++];
                len |= static_cast<uint64_t>(b & 0x7F) << shift;
                shift += 7;
                if ((b & 0x80) == 0) break;
            }
            pos += static_cast<size_t>(len);
        } else if (wire_type == 1) {
            pos += 8;
        } else if (wire_type == 5) {
            pos += 4;
        } else {
            break;
        }
    }

    co_return result;
}

} // namespace temporalio::client
