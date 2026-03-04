#ifndef TEMPORALIO_BRIDGE_SAFE_HANDLE_H
#define TEMPORALIO_BRIDGE_SAFE_HANDLE_H

/// @file safe_handle.h
/// @brief RAII handle templates for Rust-allocated resources.
///
/// Provides two ownership models:
/// - SafeHandle<T, Deleter>: unique ownership (move-only), for Worker, etc.
/// - SharedHandle<T>: shared ownership via std::shared_ptr with custom deleter,
///   for Runtime and Client which may be referenced by multiple objects.
///
/// Replaces the C# SafeUnmanagedHandle<T> and SafeHandleReference<T> patterns.

#include <memory>
#include <utility>

#include "temporalio/bridge/interop.h"

namespace temporalio::bridge {

// ── Unique ownership (move-only) ─────────────────────────────────────────────

/// RAII wrapper for opaque Rust-allocated resources with unique ownership.
///
/// The Deleter function pointer is a template parameter, ensuring zero overhead
/// at runtime (the compiler can inline the free call). Move-only semantics
/// guarantee exactly one owner.
///
/// @tparam T       The opaque Rust type (e.g., TemporalCoreWorker).
/// @tparam Deleter A pointer to the C free function (e.g., temporal_core_worker_free).
template <typename T, void (*Deleter)(T*)>
class SafeHandle {
public:
    SafeHandle() noexcept : ptr_(nullptr) {}

    /// Takes ownership of a raw pointer.
    explicit SafeHandle(T* ptr) noexcept : ptr_(ptr) {}

    ~SafeHandle() {
        if (ptr_) {
            Deleter(ptr_);
        }
    }

    // Move-only
    SafeHandle(const SafeHandle&) = delete;
    SafeHandle& operator=(const SafeHandle&) = delete;

    SafeHandle(SafeHandle&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    SafeHandle& operator=(SafeHandle&& other) noexcept {
        if (this != &other) {
            if (ptr_) {
                Deleter(ptr_);
            }
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    /// Returns the raw pointer without releasing ownership.
    T* get() const noexcept { return ptr_; }

    /// Releases ownership and returns the raw pointer.
    T* release() noexcept {
        T* tmp = ptr_;
        ptr_ = nullptr;
        return tmp;
    }

    /// Returns true if the handle holds a non-null pointer.
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

private:
    T* ptr_;
};

// ── Shared ownership ─────────────────────────────────────────────────────────

/// Creates a std::shared_ptr with a custom deleter for a Rust-allocated resource.
///
/// This replaces C#'s SafeHandle reference counting (DangerousAddRef/DangerousRelease)
/// and SafeHandleReference<T>. Multiple objects (e.g., a Client and its Worker)
/// can hold a shared_ptr to the same Rust resource; the last one to drop it
/// calls the free function.
///
/// @tparam T       The opaque Rust type.
/// @tparam Deleter A pointer to the C free function.
/// @param ptr      Raw pointer to take ownership of. May be null.
/// @return A shared_ptr that will call Deleter(ptr) when the last reference drops.
template <typename T, void (*Deleter)(T*)>
std::shared_ptr<T> make_shared_handle(T* ptr) {
    if (!ptr) {
        return nullptr;
    }
    return std::shared_ptr<T>(ptr, [](T* p) {
        if (p) {
            Deleter(p);
        }
    });
}

// ── Handle type aliases ──────────────────────────────────────────────────────

// Shared handles: used for resources that may be referenced by multiple owners.
// RuntimeHandle: a Client holds a ref to the Runtime (for byte_array_free etc.)
// ClientHandle: a Worker holds a ref to the Client handle (for replace_client etc.)
using RuntimeHandle = std::shared_ptr<TemporalCoreRuntime>;
using ClientHandle = std::shared_ptr<TemporalCoreClient>;

// Unique handles: used for resources with a single owner.
using WorkerHandle =
    SafeHandle<TemporalCoreWorker, temporal_core_worker_free>;

using EphemeralServerHandle =
    SafeHandle<TemporalCoreEphemeralServer, temporal_core_ephemeral_server_free>;

using CancellationTokenHandle =
    SafeHandle<TemporalCoreCancellationToken, temporal_core_cancellation_token_free>;

using MetricMeterHandle =
    SafeHandle<TemporalCoreMetricMeter, temporal_core_metric_meter_free>;

using MetricHandle =
    SafeHandle<TemporalCoreMetric, temporal_core_metric_free>;

using MetricAttributesHandle =
    SafeHandle<TemporalCoreMetricAttributes, temporal_core_metric_attributes_free>;

using RandomHandle =
    SafeHandle<TemporalCoreRandom, temporal_core_random_free>;

using ReplayPusherHandle =
    SafeHandle<TemporalCoreWorkerReplayPusher, temporal_core_worker_replay_pusher_free>;

}  // namespace temporalio::bridge

#endif  // TEMPORALIO_BRIDGE_SAFE_HANDLE_H
