#ifndef TEMPORALIO_BRIDGE_CALL_SCOPE_H
#define TEMPORALIO_BRIDGE_CALL_SCOPE_H

/// @file call_scope.h
/// @brief RAII scoped memory management for FFI calls.
///
/// Replaces the C# Scope : IDisposable pattern. A CallScope keeps temporary
/// data alive for the duration of an FFI call, then frees everything on
/// destruction.

#include <cstdint>
#include <cstring>
#include <deque>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "temporalio/bridge/interop.h"

namespace temporalio::bridge {

/// Manages temporary memory allocations needed during a single FFI call.
///
/// When calling Rust FFI functions, we often need to create ByteArrayRef
/// structs that point to string or byte data. The data must remain valid
/// for the duration of the call. CallScope owns copies of this data and
/// frees everything when it goes out of scope.
///
/// Usage:
///   {
///       CallScope scope;
///       auto ref = scope.byte_array("hello");
///       // ref.data points to scope-owned copy of "hello"
///       some_ffi_call(ref);
///   }  // scope destructor frees owned data
///
/// NOT thread-safe. Each FFI call should use its own CallScope.
class CallScope {
public:
    CallScope() = default;
    ~CallScope() = default;

    // Non-copyable, non-movable (prevents accidental dangling refs)
    CallScope(const CallScope&) = delete;
    CallScope& operator=(const CallScope&) = delete;
    CallScope(CallScope&&) = delete;
    CallScope& operator=(CallScope&&) = delete;

    /// Create a ByteArrayRef from a string_view.
    /// The scope takes ownership of a copy of the string data.
    TemporalCoreByteArrayRef byte_array(std::string_view str) {
        if (str.empty()) {
            return empty_byte_array_ref();
        }
        owned_strings_.emplace_back(str);
        const auto& owned = owned_strings_.back();
        return TemporalCoreByteArrayRef{
            reinterpret_cast<const uint8_t*>(owned.data()),
            owned.size(),
        };
    }

    /// Create a ByteArrayRef from a byte span.
    /// The scope takes ownership of a copy of the byte data.
    TemporalCoreByteArrayRef byte_array(std::span<const uint8_t> bytes) {
        if (bytes.empty()) {
            return empty_byte_array_ref();
        }
        owned_strings_.emplace_back(
            reinterpret_cast<const char*>(bytes.data()), bytes.size());
        const auto& owned = owned_strings_.back();
        return TemporalCoreByteArrayRef{
            reinterpret_cast<const uint8_t*>(owned.data()),
            owned.size(),
        };
    }

    /// Create a ByteArrayRef from a byte vector.
    TemporalCoreByteArrayRef byte_array(const std::vector<uint8_t>& bytes) {
        return byte_array(std::span<const uint8_t>(bytes));
    }

    /// Create a ByteArrayRef for a key=value pair (newline-delimited format
    /// used by metadata).
    TemporalCoreByteArrayRef byte_array_kv(std::string_view key,
                                            std::string_view value) {
        std::string combined;
        combined.reserve(key.size() + 1 + value.size());
        combined.append(key);
        combined.push_back('\n');
        combined.append(value);
        owned_strings_.push_back(std::move(combined));
        const auto& owned = owned_strings_.back();
        return TemporalCoreByteArrayRef{
            reinterpret_cast<const uint8_t*>(owned.data()),
            owned.size(),
        };
    }

    /// Create a newline-delimited ByteArrayRef from a collection of strings.
    TemporalCoreByteArrayRef newline_delimited(
        const std::vector<std::string>& values) {
        if (values.empty()) {
            return empty_byte_array_ref();
        }
        std::string combined;
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                combined.push_back('\n');
            }
            combined.append(values[i]);
        }
        owned_strings_.push_back(std::move(combined));
        const auto& owned = owned_strings_.back();
        return TemporalCoreByteArrayRef{
            reinterpret_cast<const uint8_t*>(owned.data()),
            owned.size(),
        };
    }

    /// Create a ByteArrayRefArray from a collection of strings.
    TemporalCoreByteArrayRefArray byte_array_array(
        const std::vector<std::string>& strings) {
        if (strings.empty()) {
            return empty_byte_array_ref_array();
        }
        auto& arr = owned_ref_arrays_.emplace_back();
        arr.reserve(strings.size());
        for (const auto& s : strings) {
            arr.push_back(byte_array(s));
        }
        return TemporalCoreByteArrayRefArray{
            arr.data(),
            arr.size(),
        };
    }

    /// Create a ByteArrayRefArray from key-value pairs (for metadata).
    TemporalCoreByteArrayRefArray byte_array_array_kv(
        const std::vector<std::pair<std::string, std::string>>& pairs) {
        if (pairs.empty()) {
            return empty_byte_array_ref_array();
        }
        auto& arr = owned_ref_arrays_.emplace_back();
        arr.reserve(pairs.size());
        for (const auto& [key, value] : pairs) {
            arr.push_back(byte_array_kv(key, value));
        }
        return TemporalCoreByteArrayRefArray{
            arr.data(),
            arr.size(),
        };
    }

    /// Allocate and track a value on the heap, returning a pointer to it.
    /// The scope owns the allocation and frees it on destruction.
    template <typename T>
    T* alloc(T value) {
        auto ptr = std::make_unique<T>(std::move(value));
        T* raw = ptr.get();
        owned_allocs_.push_back(
            OwnedAlloc{raw, [](void* p) { delete static_cast<T*>(p); }});
        ptr.release();
        return raw;
    }

    /// Returns an empty ByteArrayRef (null data, zero size).
    static TemporalCoreByteArrayRef empty_byte_array_ref() noexcept {
        return TemporalCoreByteArrayRef{nullptr, 0};
    }

    /// Returns an empty ByteArrayRefArray.
    static TemporalCoreByteArrayRefArray empty_byte_array_ref_array() noexcept {
        return TemporalCoreByteArrayRefArray{nullptr, 0};
    }

private:
    /// String data backing ByteArrayRef pointers.
    /// Uses deque instead of vector: deque::push_back never invalidates
    /// references to existing elements, which is critical since ByteArrayRef
    /// pointers point directly into these strings.
    std::deque<std::string> owned_strings_;

    /// Arrays of ByteArrayRef (for ByteArrayRefArray results).
    std::vector<std::vector<TemporalCoreByteArrayRef>> owned_ref_arrays_;

    /// Type-erased heap allocation with custom deleter.
    struct OwnedAlloc {
        void* ptr;
        void (*deleter)(void*);
        ~OwnedAlloc() {
            if (ptr && deleter) {
                deleter(ptr);
            }
        }
        // Move-only
        OwnedAlloc(void* p, void (*d)(void*)) : ptr(p), deleter(d) {}
        OwnedAlloc(const OwnedAlloc&) = delete;
        OwnedAlloc& operator=(const OwnedAlloc&) = delete;
        OwnedAlloc(OwnedAlloc&& o) noexcept : ptr(o.ptr), deleter(o.deleter) {
            o.ptr = nullptr;
            o.deleter = nullptr;
        }
        OwnedAlloc& operator=(OwnedAlloc&& o) noexcept {
            if (this != &o) {
                if (ptr && deleter) {
                    deleter(ptr);
                }
                ptr = o.ptr;
                deleter = o.deleter;
                o.ptr = nullptr;
                o.deleter = nullptr;
            }
            return *this;
        }
    };

    std::vector<OwnedAlloc> owned_allocs_;
};

}  // namespace temporalio::bridge

#endif  // TEMPORALIO_BRIDGE_CALL_SCOPE_H
