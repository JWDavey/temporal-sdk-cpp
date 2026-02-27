#ifndef TEMPORALIO_BRIDGE_CANCELLATION_TOKEN_H
#define TEMPORALIO_BRIDGE_CANCELLATION_TOKEN_H

/// @file cancellation_token.h
/// @brief RAII wrapper for Rust cancellation tokens.
///
/// Provides cancellation support for async FFI operations (e.g., RPC calls).
/// Corresponds to C# Bridge/CancellationToken.cs pattern.

#include "temporalio/bridge/interop.h"
#include "temporalio/bridge/safe_handle.h"

namespace temporalio::bridge {

/// RAII wrapper for a Rust-allocated TemporalCoreCancellationToken.
///
/// Creates a cancellation token that can be passed to FFI calls and
/// cancelled from the C++ side. Automatically freed on destruction.
class CancellationToken {
public:
    /// Create a new cancellation token.
    CancellationToken()
        : handle_(temporal_core_cancellation_token_new()) {}

    /// Cancel this token. This will cause any pending FFI operation
    /// using this token to be cancelled.
    void cancel() {
        if (handle_) {
            temporal_core_cancellation_token_cancel(handle_.get());
        }
    }

    /// Get the raw pointer (for passing to FFI calls).
    TemporalCoreCancellationToken* get() const noexcept {
        return handle_.get();
    }

    /// Whether the token is valid (non-null).
    explicit operator bool() const noexcept {
        return static_cast<bool>(handle_);
    }

    // Move-only
    CancellationToken(const CancellationToken&) = delete;
    CancellationToken& operator=(const CancellationToken&) = delete;
    CancellationToken(CancellationToken&&) noexcept = default;
    CancellationToken& operator=(CancellationToken&&) noexcept = default;

private:
    CancellationTokenHandle handle_;
};

}  // namespace temporalio::bridge

#endif  // TEMPORALIO_BRIDGE_CANCELLATION_TOKEN_H
