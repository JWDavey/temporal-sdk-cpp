#ifndef TEMPORALIO_BRIDGE_BYTE_ARRAY_H
#define TEMPORALIO_BRIDGE_BYTE_ARRAY_H

/// @file byte_array.h
/// @brief RAII wrapper for Rust-owned TemporalCoreByteArray.
///
/// When the Rust bridge returns a TemporalCoreByteArray, the data is owned
/// by Rust and must be freed via temporal_core_byte_array_free. This wrapper
/// provides RAII ownership and conversion to C++ types.

#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "temporalio/bridge/interop.h"

namespace temporalio::bridge {

/// RAII wrapper for a Rust-owned TemporalCoreByteArray.
///
/// Takes ownership of a Rust-allocated byte array and frees it on destruction
/// via temporal_core_byte_array_free. Provides conversion to C++ string/vector.
class ByteArray {
public:
    /// Construct from a Rust-returned byte array pointer.
    /// Takes ownership; the pointer must not be used after this.
    ByteArray(TemporalCoreRuntime* runtime,
              const TemporalCoreByteArray* byte_array) noexcept
        : runtime_(runtime), byte_array_(byte_array) {}

    ~ByteArray() {
        if (byte_array_ && runtime_) {
            temporal_core_byte_array_free(runtime_, byte_array_);
        }
    }

    // Move-only
    ByteArray(const ByteArray&) = delete;
    ByteArray& operator=(const ByteArray&) = delete;

    ByteArray(ByteArray&& other) noexcept
        : runtime_(other.runtime_), byte_array_(other.byte_array_) {
        other.byte_array_ = nullptr;
    }

    ByteArray& operator=(ByteArray&& other) noexcept {
        if (this != &other) {
            if (byte_array_ && runtime_) {
                temporal_core_byte_array_free(runtime_, byte_array_);
            }
            runtime_ = other.runtime_;
            byte_array_ = other.byte_array_;
            other.byte_array_ = nullptr;
        }
        return *this;
    }

    /// Convert the byte array contents to a UTF-8 string.
    std::string to_string() const {
        if (!byte_array_ || !byte_array_->data || byte_array_->size == 0) {
            return {};
        }
        return std::string(reinterpret_cast<const char*>(byte_array_->data),
                           byte_array_->size);
    }

    /// Get a string_view into the byte array (valid while this object lives).
    std::string_view to_string_view() const noexcept {
        if (!byte_array_ || !byte_array_->data || byte_array_->size == 0) {
            return {};
        }
        return std::string_view(
            reinterpret_cast<const char*>(byte_array_->data),
            byte_array_->size);
    }

    /// Copy the byte array contents to a vector.
    std::vector<uint8_t> to_bytes() const {
        if (!byte_array_ || !byte_array_->data || byte_array_->size == 0) {
            return {};
        }
        return std::vector<uint8_t>(byte_array_->data,
                                     byte_array_->data + byte_array_->size);
    }

    /// Get a span over the byte array data (valid while this object lives).
    std::span<const uint8_t> to_span() const noexcept {
        if (!byte_array_ || !byte_array_->data || byte_array_->size == 0) {
            return {};
        }
        return std::span<const uint8_t>(byte_array_->data, byte_array_->size);
    }

    /// Returns true if the byte array is non-null and non-empty.
    explicit operator bool() const noexcept {
        return byte_array_ != nullptr && byte_array_->data != nullptr &&
               byte_array_->size > 0;
    }

    /// Returns the size of the byte array.
    size_t size() const noexcept {
        return byte_array_ ? byte_array_->size : 0;
    }

    /// Returns the raw byte array pointer (for passing back to Rust).
    const TemporalCoreByteArray* raw() const noexcept { return byte_array_; }

private:
    TemporalCoreRuntime* runtime_;
    const TemporalCoreByteArray* byte_array_;
};

/// Helper: convert a TemporalCoreByteArrayRef (non-owning) to a string_view.
inline std::string_view byte_array_ref_to_string_view(
    TemporalCoreByteArrayRef ref) noexcept {
    if (!ref.data || ref.size == 0) {
        return {};
    }
    return std::string_view(reinterpret_cast<const char*>(ref.data), ref.size);
}

/// Helper: convert a TemporalCoreByteArrayRef to a string.
inline std::string byte_array_ref_to_string(
    TemporalCoreByteArrayRef ref) {
    return std::string(byte_array_ref_to_string_view(ref));
}

}  // namespace temporalio::bridge

#endif  // TEMPORALIO_BRIDGE_BYTE_ARRAY_H
