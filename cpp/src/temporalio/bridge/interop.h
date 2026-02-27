#ifndef TEMPORALIO_BRIDGE_INTEROP_H
#define TEMPORALIO_BRIDGE_INTEROP_H

/// @file interop.h
/// @brief C FFI declarations for the Rust sdk-core-c-bridge.
///
/// This file declares the C-compatible structs, enums, and functions exported
/// by the `temporalio_sdk_core_c_bridge` Rust crate. It is the C++ equivalent
/// of the auto-generated C# file Interop.cs.
///
/// All types here use C-compatible layout and are used with extern "C" functions.

#include <cstddef>
#include <cstdint>

// ── Opaque types ─────────────────────────────────────────────────────────────
// These are Rust-owned opaque types. C++ only ever holds pointers to them.

struct TemporalCoreCancellationToken;
struct TemporalCoreClient;
struct TemporalCoreClientGrpcOverrideRequest;
struct TemporalCoreEphemeralServer;
struct TemporalCoreForwardedLog;
struct TemporalCoreMetric;
struct TemporalCoreMetricAttributes;
struct TemporalCoreMetricMeter;
struct TemporalCoreRandom;
struct TemporalCoreRuntime;
struct TemporalCoreSlotReserveCompletionCtx;
struct TemporalCoreWorker;
struct TemporalCoreWorkerReplayPusher;

// ── Enums ────────────────────────────────────────────────────────────────────

enum class TemporalCoreRpcService : int32_t {
    Workflow = 1,
    Operator,
    Cloud,
    Test,
    Health,
};

enum class TemporalCoreMetricAttributeValueType : int32_t {
    String = 1,
    Int,
    Float,
    Bool,
};

enum class TemporalCoreMetricKind : int32_t {
    CounterInteger = 1,
    HistogramInteger,
    HistogramFloat,
    HistogramDuration,
    GaugeInteger,
    GaugeFloat,
};

enum class TemporalCoreForwardedLogLevel : int32_t {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
};

enum class TemporalCoreOpenTelemetryMetricTemporality : int32_t {
    Cumulative = 1,
    Delta,
};

enum class TemporalCoreOpenTelemetryProtocol : int32_t {
    Grpc = 1,
    Http,
};

enum class TemporalCoreSlotKindType : int32_t {
    WorkflowSlotKindType = 0,
    ActivitySlotKindType,
    LocalActivitySlotKindType,
    NexusSlotKindType,
};

// ── Data structs (C-compatible layout) ───────────────────────────────────────

/// Non-owning byte array reference. Points to externally-owned data.
/// Equivalent to C# TemporalCoreByteArrayRef.
struct TemporalCoreByteArrayRef {
    const uint8_t* data;
    size_t size;
};

/// Array of byte array references.
struct TemporalCoreByteArrayRefArray {
    const TemporalCoreByteArrayRef* data;
    size_t size;
};

/// Owning byte array returned by Rust. Must be freed via temporal_core_byte_array_free.
struct TemporalCoreByteArray {
    const uint8_t* data;
    size_t size;
    size_t cap;
    uint8_t disable_free;
};

// ── Client option structs ────────────────────────────────────────────────────

struct TemporalCoreClientTlsOptions {
    TemporalCoreByteArrayRef server_root_ca_cert;
    TemporalCoreByteArrayRef domain;
    TemporalCoreByteArrayRef client_cert;
    TemporalCoreByteArrayRef client_private_key;
};

struct TemporalCoreClientRetryOptions {
    uint64_t initial_interval_millis;
    double randomization_factor;
    double multiplier;
    uint64_t max_interval_millis;
    uint64_t max_elapsed_time_millis;
    size_t max_retries;
};

struct TemporalCoreClientKeepAliveOptions {
    uint64_t interval_millis;
    uint64_t timeout_millis;
};

struct TemporalCoreClientHttpConnectProxyOptions {
    TemporalCoreByteArrayRef target_host;
    TemporalCoreByteArrayRef username;
    TemporalCoreByteArrayRef password;
};

// Callback types
using TemporalCoreClientGrpcOverrideCallback =
    void (*)(TemporalCoreClientGrpcOverrideRequest* request, void* user_data);

struct TemporalCoreClientOptions {
    TemporalCoreByteArrayRef target_url;
    TemporalCoreByteArrayRef client_name;
    TemporalCoreByteArrayRef client_version;
    TemporalCoreByteArrayRefArray metadata;        // TemporalCoreMetadataRef
    TemporalCoreByteArrayRefArray binary_metadata;  // TemporalCoreMetadataRef
    TemporalCoreByteArrayRef api_key;
    TemporalCoreByteArrayRef identity;
    const TemporalCoreClientTlsOptions* tls_options;
    const TemporalCoreClientRetryOptions* retry_options;
    const TemporalCoreClientKeepAliveOptions* keep_alive_options;
    const TemporalCoreClientHttpConnectProxyOptions* http_connect_proxy_options;
    TemporalCoreClientGrpcOverrideCallback grpc_override_callback;
    void* grpc_override_callback_user_data;
};

// ── Client callback types ────────────────────────────────────────────────────

using TemporalCoreClientConnectCallback = void (*)(
    void* user_data,
    TemporalCoreClient* success,
    const TemporalCoreByteArray* fail);

struct TemporalCoreClientGrpcOverrideResponse {
    int32_t status_code;
    TemporalCoreByteArrayRefArray headers;  // TemporalCoreMetadataRef
    TemporalCoreByteArrayRef success_proto;
    TemporalCoreByteArrayRef fail_message;
    TemporalCoreByteArrayRef fail_details;
};

struct TemporalCoreRpcCallOptions {
    TemporalCoreRpcService service;
    TemporalCoreByteArrayRef rpc;
    TemporalCoreByteArrayRef req;
    uint8_t retry;  // bool
    TemporalCoreByteArrayRefArray metadata;         // TemporalCoreMetadataRef
    TemporalCoreByteArrayRefArray binary_metadata;  // TemporalCoreMetadataRef
    uint32_t timeout_millis;
    const TemporalCoreCancellationToken* cancellation_token;
};

using TemporalCoreClientRpcCallCallback = void (*)(
    void* user_data,
    const TemporalCoreByteArray* success,
    uint32_t status_code,
    const TemporalCoreByteArray* failure_message,
    const TemporalCoreByteArray* failure_details);

// ── Client env config structs ────────────────────────────────────────────────

struct TemporalCoreClientEnvConfigOrFail {
    const TemporalCoreByteArray* success;
    const TemporalCoreByteArray* fail;
};

struct TemporalCoreClientEnvConfigLoadOptions {
    TemporalCoreByteArrayRef path;
    TemporalCoreByteArrayRef data;
    uint8_t config_file_strict;  // bool
    TemporalCoreByteArrayRef env_vars;
};

struct TemporalCoreClientEnvConfigProfileOrFail {
    const TemporalCoreByteArray* success;
    const TemporalCoreByteArray* fail;
};

struct TemporalCoreClientEnvConfigProfileLoadOptions {
    TemporalCoreByteArrayRef profile;
    TemporalCoreByteArrayRef path;
    TemporalCoreByteArrayRef data;
    uint8_t disable_file;  // bool
    uint8_t disable_env;   // bool
    uint8_t config_file_strict;  // bool
    TemporalCoreByteArrayRef env_vars;
};

// ── Metric structs ───────────────────────────────────────────────────────────

union TemporalCoreMetricAttributeValue {
    TemporalCoreByteArrayRef string_value;
    int64_t int_value;
    double float_value;
    uint8_t bool_value;  // bool
};

struct TemporalCoreMetricAttribute {
    TemporalCoreByteArrayRef key;
    TemporalCoreMetricAttributeValue value;
    TemporalCoreMetricAttributeValueType value_type;
};

struct TemporalCoreMetricOptions {
    TemporalCoreByteArrayRef name;
    TemporalCoreByteArrayRef description;
    TemporalCoreByteArrayRef unit;
    TemporalCoreMetricKind kind;
};

// ── Custom metric meter callback types ───────────────────────────────────────

using TemporalCoreCustomMetricMeterMetricNewCallback = const void* (*)(
    TemporalCoreByteArrayRef name,
    TemporalCoreByteArrayRef description,
    TemporalCoreByteArrayRef unit,
    TemporalCoreMetricKind kind);

using TemporalCoreCustomMetricMeterMetricFreeCallback =
    void (*)(const void* metric);

using TemporalCoreCustomMetricMeterMetricRecordIntegerCallback =
    void (*)(const void* metric, uint64_t value, const void* attributes);

using TemporalCoreCustomMetricMeterMetricRecordFloatCallback =
    void (*)(const void* metric, double value, const void* attributes);

using TemporalCoreCustomMetricMeterMetricRecordDurationCallback =
    void (*)(const void* metric, uint64_t value_ms, const void* attributes);

struct TemporalCoreCustomMetricAttributeValueString {
    const uint8_t* data;
    size_t size;
};

union TemporalCoreCustomMetricAttributeValue {
    TemporalCoreCustomMetricAttributeValueString string_value;
    int64_t int_value;
    double float_value;
    uint8_t bool_value;  // bool
};

struct TemporalCoreCustomMetricAttribute {
    TemporalCoreByteArrayRef key;
    TemporalCoreCustomMetricAttributeValue value;
    TemporalCoreMetricAttributeValueType value_type;
};

using TemporalCoreCustomMetricMeterAttributesNewCallback =
    const void* (*)(const void* append_from,
                    const TemporalCoreCustomMetricAttribute* attributes,
                    size_t attributes_size);

using TemporalCoreCustomMetricMeterAttributesFreeCallback =
    void (*)(const void* attributes);

struct TemporalCoreCustomMetricMeter;

using TemporalCoreCustomMetricMeterMeterFreeCallback =
    void (*)(const TemporalCoreCustomMetricMeter* meter);

struct TemporalCoreCustomMetricMeter {
    TemporalCoreCustomMetricMeterMetricNewCallback metric_new;
    TemporalCoreCustomMetricMeterMetricFreeCallback metric_free;
    TemporalCoreCustomMetricMeterMetricRecordIntegerCallback metric_record_integer;
    TemporalCoreCustomMetricMeterMetricRecordFloatCallback metric_record_float;
    TemporalCoreCustomMetricMeterMetricRecordDurationCallback metric_record_duration;
    TemporalCoreCustomMetricMeterAttributesNewCallback attributes_new;
    TemporalCoreCustomMetricMeterAttributesFreeCallback attributes_free;
    TemporalCoreCustomMetricMeterMeterFreeCallback meter_free;
};

// ── Telemetry / Runtime option structs ───────────────────────────────────────

using TemporalCoreForwardedLogCallback = void (*)(
    TemporalCoreForwardedLogLevel level,
    const TemporalCoreForwardedLog* log);

struct TemporalCoreLoggingOptions {
    TemporalCoreByteArrayRef filter;
    TemporalCoreForwardedLogCallback forward_to;
};

struct TemporalCoreOpenTelemetryOptions {
    TemporalCoreByteArrayRef url;
    TemporalCoreByteArrayRef headers;  // TemporalCoreNewlineDelimitedMapRef
    uint32_t metric_periodicity_millis;
    TemporalCoreOpenTelemetryMetricTemporality metric_temporality;
    uint8_t durations_as_seconds;  // bool
    TemporalCoreOpenTelemetryProtocol protocol;
    TemporalCoreByteArrayRef histogram_bucket_overrides;  // TemporalCoreNewlineDelimitedMapRef
};

struct TemporalCorePrometheusOptions {
    TemporalCoreByteArrayRef bind_address;
    uint8_t counters_total_suffix;   // bool
    uint8_t unit_suffix;             // bool
    uint8_t durations_as_seconds;    // bool
    TemporalCoreByteArrayRef histogram_bucket_overrides;  // TemporalCoreNewlineDelimitedMapRef
};

struct TemporalCoreMetricsOptions {
    const TemporalCoreOpenTelemetryOptions* opentelemetry;
    const TemporalCorePrometheusOptions* prometheus;
    const TemporalCoreCustomMetricMeter* custom_meter;
    uint8_t attach_service_name;  // bool
    TemporalCoreByteArrayRef global_tags;   // TemporalCoreNewlineDelimitedMapRef
    TemporalCoreByteArrayRef metric_prefix;
};

struct TemporalCoreTelemetryOptions {
    const TemporalCoreLoggingOptions* logging;
    const TemporalCoreMetricsOptions* metrics;
};

struct TemporalCoreRuntimeOptions {
    const TemporalCoreTelemetryOptions* telemetry;
    uint64_t worker_heartbeat_interval_millis;
};

struct TemporalCoreRuntimeOrFail {
    TemporalCoreRuntime* runtime;
    const TemporalCoreByteArray* fail;
};

// ── Ephemeral server structs ─────────────────────────────────────────────────

struct TemporalCoreTestServerOptions {
    TemporalCoreByteArrayRef existing_path;
    TemporalCoreByteArrayRef sdk_name;
    TemporalCoreByteArrayRef sdk_version;
    TemporalCoreByteArrayRef download_version;
    TemporalCoreByteArrayRef download_dest_dir;
    uint16_t port;
    TemporalCoreByteArrayRef extra_args;
    uint64_t download_ttl_seconds;
};

struct TemporalCoreDevServerOptions {
    const TemporalCoreTestServerOptions* test_server;
    TemporalCoreByteArrayRef namespace_;
    TemporalCoreByteArrayRef ip;
    TemporalCoreByteArrayRef database_filename;
    uint8_t ui;  // bool
    uint16_t ui_port;
    TemporalCoreByteArrayRef log_format;
    TemporalCoreByteArrayRef log_level;
};

using TemporalCoreEphemeralServerStartCallback = void (*)(
    void* user_data,
    TemporalCoreEphemeralServer* success,
    const TemporalCoreByteArray* success_target,
    const TemporalCoreByteArray* fail);

using TemporalCoreEphemeralServerShutdownCallback = void (*)(
    void* user_data,
    const TemporalCoreByteArray* fail);

// ── Worker structs ───────────────────────────────────────────────────────────

struct TemporalCoreWorkerOrFail {
    TemporalCoreWorker* worker;
    const TemporalCoreByteArray* fail;
};

struct TemporalCoreWorkerVersioningNone {
    TemporalCoreByteArrayRef build_id;
};

struct TemporalCoreWorkerDeploymentVersion {
    TemporalCoreByteArrayRef deployment_name;
    TemporalCoreByteArrayRef build_id;
};

struct TemporalCoreWorkerDeploymentOptions {
    TemporalCoreWorkerDeploymentVersion version;
    uint8_t use_worker_versioning;  // bool
    int32_t default_versioning_behavior;
};

struct TemporalCoreLegacyBuildIdBasedStrategy {
    TemporalCoreByteArrayRef build_id;
};

enum class TemporalCoreWorkerVersioningStrategy_Tag : int32_t {
    None = 0,
    DeploymentBased,
    LegacyBuildIdBased,
};

struct TemporalCoreWorkerVersioningStrategy {
    TemporalCoreWorkerVersioningStrategy_Tag tag;
    union {
        TemporalCoreWorkerVersioningNone none;
        TemporalCoreWorkerDeploymentOptions deployment_based;
        TemporalCoreLegacyBuildIdBasedStrategy legacy_build_id_based;
    };
};

struct TemporalCoreFixedSizeSlotSupplier {
    size_t num_slots;
};

struct TemporalCoreResourceBasedTunerOptions {
    double target_memory_usage;
    double target_cpu_usage;
};

struct TemporalCoreResourceBasedSlotSupplier {
    size_t minimum_slots;
    size_t maximum_slots;
    uint64_t ramp_throttle_ms;
    TemporalCoreResourceBasedTunerOptions tuner_options;
};

// ── Slot supplier types ──────────────────────────────────────────────────────

struct TemporalCoreSlotReserveCtx {
    TemporalCoreSlotKindType slot_type;
    TemporalCoreByteArrayRef task_queue;
    TemporalCoreByteArrayRef worker_identity;
    TemporalCoreByteArrayRef worker_build_id;
    uint8_t is_sticky;  // bool
};

using TemporalCoreCustomSlotSupplierReserveCallback = void (*)(
    const TemporalCoreSlotReserveCtx* ctx,
    const TemporalCoreSlotReserveCompletionCtx* completion_ctx,
    void* user_data);

using TemporalCoreCustomSlotSupplierCancelReserveCallback = void (*)(
    const TemporalCoreSlotReserveCompletionCtx* completion_ctx,
    void* user_data);

using TemporalCoreCustomSlotSupplierTryReserveCallback = size_t (*)(
    const TemporalCoreSlotReserveCtx* ctx,
    void* user_data);

enum class TemporalCoreSlotInfo_Tag : int32_t {
    WorkflowSlotInfo = 0,
    ActivitySlotInfo,
    LocalActivitySlotInfo,
    NexusSlotInfo,
};

struct TemporalCoreWorkflowSlotInfo_Body {
    TemporalCoreByteArrayRef workflow_type;
    uint8_t is_sticky;  // bool
};

struct TemporalCoreActivitySlotInfo_Body {
    TemporalCoreByteArrayRef activity_type;
};

struct TemporalCoreLocalActivitySlotInfo_Body {
    TemporalCoreByteArrayRef activity_type;
};

struct TemporalCoreNexusSlotInfo_Body {
    TemporalCoreByteArrayRef operation;
    TemporalCoreByteArrayRef service;
};

struct TemporalCoreSlotInfo {
    TemporalCoreSlotInfo_Tag tag;
    union {
        TemporalCoreWorkflowSlotInfo_Body workflow_slot_info;
        TemporalCoreActivitySlotInfo_Body activity_slot_info;
        TemporalCoreLocalActivitySlotInfo_Body local_activity_slot_info;
        TemporalCoreNexusSlotInfo_Body nexus_slot_info;
    };
};

struct TemporalCoreSlotMarkUsedCtx {
    TemporalCoreSlotInfo slot_info;
    size_t slot_permit;
};

using TemporalCoreCustomSlotSupplierMarkUsedCallback = void (*)(
    const TemporalCoreSlotMarkUsedCtx* ctx,
    void* user_data);

struct TemporalCoreSlotReleaseCtx {
    const TemporalCoreSlotInfo* slot_info;
    size_t slot_permit;
};

using TemporalCoreCustomSlotSupplierReleaseCallback = void (*)(
    const TemporalCoreSlotReleaseCtx* ctx,
    void* user_data);

using TemporalCoreCustomSlotSupplierAvailableSlotsCallback =
    bool (*)(size_t* available_slots, void* user_data);

struct TemporalCoreCustomSlotSupplierCallbacks;

using TemporalCoreCustomSlotSupplierFreeCallback = void (*)(
    const TemporalCoreCustomSlotSupplierCallbacks* userimpl);

struct TemporalCoreCustomSlotSupplierCallbacks {
    TemporalCoreCustomSlotSupplierReserveCallback reserve;
    TemporalCoreCustomSlotSupplierCancelReserveCallback cancel_reserve;
    TemporalCoreCustomSlotSupplierTryReserveCallback try_reserve;
    TemporalCoreCustomSlotSupplierMarkUsedCallback mark_used;
    TemporalCoreCustomSlotSupplierReleaseCallback release;
    TemporalCoreCustomSlotSupplierAvailableSlotsCallback available_slots;
    TemporalCoreCustomSlotSupplierFreeCallback free;
    void* user_data;
};

enum class TemporalCoreSlotSupplier_Tag : int32_t {
    FixedSize = 0,
    ResourceBased,
    Custom,
};

struct TemporalCoreSlotSupplier {
    TemporalCoreSlotSupplier_Tag tag;
    union {
        TemporalCoreFixedSizeSlotSupplier fixed_size;
        TemporalCoreResourceBasedSlotSupplier resource_based;
        struct {
            const TemporalCoreCustomSlotSupplierCallbacks* callbacks;
        } custom;
    };
};

struct TemporalCoreTunerHolder {
    TemporalCoreSlotSupplier workflow_slot_supplier;
    TemporalCoreSlotSupplier activity_slot_supplier;
    TemporalCoreSlotSupplier local_activity_slot_supplier;
    TemporalCoreSlotSupplier nexus_task_slot_supplier;
};

struct TemporalCoreWorkerTaskTypes {
    uint8_t enable_workflows;          // bool
    uint8_t enable_local_activities;   // bool
    uint8_t enable_remote_activities;  // bool
    uint8_t enable_nexus;              // bool
};

struct TemporalCorePollerBehaviorSimpleMaximum {
    size_t simple_maximum;
};

struct TemporalCorePollerBehaviorAutoscaling {
    size_t minimum;
    size_t maximum;
    size_t initial;
};

struct TemporalCorePollerBehavior {
    const TemporalCorePollerBehaviorSimpleMaximum* simple_maximum;
    const TemporalCorePollerBehaviorAutoscaling* autoscaling;
};

struct TemporalCoreWorkerOptions {
    TemporalCoreByteArrayRef namespace_;
    TemporalCoreByteArrayRef task_queue;
    TemporalCoreWorkerVersioningStrategy versioning_strategy;
    TemporalCoreByteArrayRef identity_override;
    uint32_t max_cached_workflows;
    TemporalCoreTunerHolder tuner;
    TemporalCoreWorkerTaskTypes task_types;
    uint64_t sticky_queue_schedule_to_start_timeout_millis;
    uint64_t max_heartbeat_throttle_interval_millis;
    uint64_t default_heartbeat_throttle_interval_millis;
    double max_activities_per_second;
    double max_task_queue_activities_per_second;
    uint64_t graceful_shutdown_period_millis;
    TemporalCorePollerBehavior workflow_task_poller_behavior;
    float nonsticky_to_sticky_poll_ratio;
    TemporalCorePollerBehavior activity_task_poller_behavior;
    TemporalCorePollerBehavior nexus_task_poller_behavior;
    uint8_t nondeterminism_as_workflow_fail;  // bool
    TemporalCoreByteArrayRefArray nondeterminism_as_workflow_fail_for_types;
    TemporalCoreByteArrayRefArray plugins;
};

// ── Worker callback types ────────────────────────────────────────────────────

using TemporalCoreWorkerCallback = void (*)(
    void* user_data,
    const TemporalCoreByteArray* fail);

using TemporalCoreWorkerPollCallback = void (*)(
    void* user_data,
    const TemporalCoreByteArray* success,
    const TemporalCoreByteArray* fail);

// ── Worker replayer structs ──────────────────────────────────────────────────

struct TemporalCoreWorkerReplayerOrFail {
    TemporalCoreWorker* worker;
    TemporalCoreWorkerReplayPusher* worker_replay_pusher;
    const TemporalCoreByteArray* fail;
};

struct TemporalCoreWorkerReplayPushResult {
    const TemporalCoreByteArray* fail;
};

// ── Extern "C" function declarations ─────────────────────────────────────────

extern "C" {

// -- Cancellation token --
TemporalCoreCancellationToken* temporal_core_cancellation_token_new();
void temporal_core_cancellation_token_cancel(TemporalCoreCancellationToken* token);
void temporal_core_cancellation_token_free(TemporalCoreCancellationToken* token);

// -- Client --
void temporal_core_client_connect(
    TemporalCoreRuntime* runtime,
    const TemporalCoreClientOptions* options,
    void* user_data,
    TemporalCoreClientConnectCallback callback);

void temporal_core_client_free(TemporalCoreClient* client);

void temporal_core_client_update_metadata(
    TemporalCoreClient* client,
    TemporalCoreByteArrayRefArray metadata);

void temporal_core_client_update_binary_metadata(
    TemporalCoreClient* client,
    TemporalCoreByteArrayRefArray metadata);

void temporal_core_client_update_api_key(
    TemporalCoreClient* client,
    TemporalCoreByteArrayRef api_key);

TemporalCoreByteArrayRef temporal_core_client_grpc_override_request_service(
    const TemporalCoreClientGrpcOverrideRequest* req);

TemporalCoreByteArrayRef temporal_core_client_grpc_override_request_rpc(
    const TemporalCoreClientGrpcOverrideRequest* req);

TemporalCoreByteArrayRefArray temporal_core_client_grpc_override_request_headers(
    const TemporalCoreClientGrpcOverrideRequest* req);

TemporalCoreByteArrayRef temporal_core_client_grpc_override_request_proto(
    const TemporalCoreClientGrpcOverrideRequest* req);

void temporal_core_client_grpc_override_request_respond(
    TemporalCoreClientGrpcOverrideRequest* req,
    TemporalCoreClientGrpcOverrideResponse resp);

void temporal_core_client_rpc_call(
    TemporalCoreClient* client,
    const TemporalCoreRpcCallOptions* options,
    void* user_data,
    TemporalCoreClientRpcCallCallback callback);

TemporalCoreClientEnvConfigOrFail temporal_core_client_env_config_load(
    const TemporalCoreClientEnvConfigLoadOptions* options);

TemporalCoreClientEnvConfigProfileOrFail temporal_core_client_env_config_profile_load(
    const TemporalCoreClientEnvConfigProfileLoadOptions* options);

// -- Metrics --
TemporalCoreMetricMeter* temporal_core_metric_meter_new(
    TemporalCoreRuntime* runtime);

void temporal_core_metric_meter_free(TemporalCoreMetricMeter* meter);

TemporalCoreMetricAttributes* temporal_core_metric_attributes_new(
    const TemporalCoreMetricMeter* meter,
    const TemporalCoreMetricAttribute* attrs,
    size_t size);

TemporalCoreMetricAttributes* temporal_core_metric_attributes_new_append(
    const TemporalCoreMetricMeter* meter,
    const TemporalCoreMetricAttributes* orig,
    const TemporalCoreMetricAttribute* attrs,
    size_t size);

void temporal_core_metric_attributes_free(TemporalCoreMetricAttributes* attrs);

TemporalCoreMetric* temporal_core_metric_new(
    const TemporalCoreMetricMeter* meter,
    const TemporalCoreMetricOptions* options);

void temporal_core_metric_free(TemporalCoreMetric* metric);

void temporal_core_metric_record_integer(
    const TemporalCoreMetric* metric,
    uint64_t value,
    const TemporalCoreMetricAttributes* attrs);

void temporal_core_metric_record_float(
    const TemporalCoreMetric* metric,
    double value,
    const TemporalCoreMetricAttributes* attrs);

void temporal_core_metric_record_duration(
    const TemporalCoreMetric* metric,
    uint64_t value_ms,
    const TemporalCoreMetricAttributes* attrs);

// -- Random --
TemporalCoreRandom* temporal_core_random_new(uint64_t seed);
void temporal_core_random_free(TemporalCoreRandom* random);
int32_t temporal_core_random_int32_range(
    TemporalCoreRandom* random,
    int32_t min,
    int32_t max,
    uint8_t max_inclusive);
double temporal_core_random_double_range(
    TemporalCoreRandom* random,
    double min,
    double max,
    uint8_t max_inclusive);
void temporal_core_random_fill_bytes(
    TemporalCoreRandom* random,
    TemporalCoreByteArrayRef bytes);

// -- Runtime --
TemporalCoreRuntimeOrFail temporal_core_runtime_new(
    const TemporalCoreRuntimeOptions* options);

void temporal_core_runtime_free(TemporalCoreRuntime* runtime);

void temporal_core_byte_array_free(
    TemporalCoreRuntime* runtime,
    const TemporalCoreByteArray* bytes);

// -- Forwarded log accessors --
TemporalCoreByteArrayRef temporal_core_forwarded_log_target(
    const TemporalCoreForwardedLog* log);

TemporalCoreByteArrayRef temporal_core_forwarded_log_message(
    const TemporalCoreForwardedLog* log);

uint64_t temporal_core_forwarded_log_timestamp_millis(
    const TemporalCoreForwardedLog* log);

TemporalCoreByteArrayRef temporal_core_forwarded_log_fields_json(
    const TemporalCoreForwardedLog* log);

// -- Ephemeral server --
void temporal_core_ephemeral_server_start_dev_server(
    TemporalCoreRuntime* runtime,
    const TemporalCoreDevServerOptions* options,
    void* user_data,
    TemporalCoreEphemeralServerStartCallback callback);

void temporal_core_ephemeral_server_start_test_server(
    TemporalCoreRuntime* runtime,
    const TemporalCoreTestServerOptions* options,
    void* user_data,
    TemporalCoreEphemeralServerStartCallback callback);

void temporal_core_ephemeral_server_free(TemporalCoreEphemeralServer* server);

void temporal_core_ephemeral_server_shutdown(
    TemporalCoreEphemeralServer* server,
    void* user_data,
    TemporalCoreEphemeralServerShutdownCallback callback);

// -- Worker --
TemporalCoreWorkerOrFail temporal_core_worker_new(
    TemporalCoreClient* client,
    const TemporalCoreWorkerOptions* options);

void temporal_core_worker_free(TemporalCoreWorker* worker);

void temporal_core_worker_validate(
    TemporalCoreWorker* worker,
    void* user_data,
    TemporalCoreWorkerCallback callback);

const TemporalCoreByteArray* temporal_core_worker_replace_client(
    TemporalCoreWorker* worker,
    TemporalCoreClient* new_client);

void temporal_core_worker_poll_workflow_activation(
    TemporalCoreWorker* worker,
    void* user_data,
    TemporalCoreWorkerPollCallback callback);

void temporal_core_worker_poll_activity_task(
    TemporalCoreWorker* worker,
    void* user_data,
    TemporalCoreWorkerPollCallback callback);

void temporal_core_worker_poll_nexus_task(
    TemporalCoreWorker* worker,
    void* user_data,
    TemporalCoreWorkerPollCallback callback);

void temporal_core_worker_complete_workflow_activation(
    TemporalCoreWorker* worker,
    TemporalCoreByteArrayRef completion,
    void* user_data,
    TemporalCoreWorkerCallback callback);

void temporal_core_worker_complete_activity_task(
    TemporalCoreWorker* worker,
    TemporalCoreByteArrayRef completion,
    void* user_data,
    TemporalCoreWorkerCallback callback);

void temporal_core_worker_complete_nexus_task(
    TemporalCoreWorker* worker,
    TemporalCoreByteArrayRef completion,
    void* user_data,
    TemporalCoreWorkerCallback callback);

const TemporalCoreByteArray* temporal_core_worker_record_activity_heartbeat(
    TemporalCoreWorker* worker,
    TemporalCoreByteArrayRef heartbeat);

void temporal_core_worker_request_workflow_eviction(
    TemporalCoreWorker* worker,
    TemporalCoreByteArrayRef run_id);

void temporal_core_worker_initiate_shutdown(TemporalCoreWorker* worker);

void temporal_core_worker_finalize_shutdown(
    TemporalCoreWorker* worker,
    void* user_data,
    TemporalCoreWorkerCallback callback);

// -- Worker replayer --
TemporalCoreWorkerReplayerOrFail temporal_core_worker_replayer_new(
    TemporalCoreRuntime* runtime,
    const TemporalCoreWorkerOptions* options);

void temporal_core_worker_replay_pusher_free(
    TemporalCoreWorkerReplayPusher* worker_replay_pusher);

TemporalCoreWorkerReplayPushResult temporal_core_worker_replay_push(
    TemporalCoreWorker* worker,
    TemporalCoreWorkerReplayPusher* worker_replay_pusher,
    TemporalCoreByteArrayRef workflow_id,
    TemporalCoreByteArrayRef history);

// -- Slot supplier --
bool temporal_core_complete_async_reserve(
    const TemporalCoreSlotReserveCompletionCtx* completion_ctx,
    size_t permit_id);

bool temporal_core_complete_async_cancel_reserve(
    const TemporalCoreSlotReserveCompletionCtx* completion_ctx);

}  // extern "C"

#endif  // TEMPORALIO_BRIDGE_INTEROP_H
