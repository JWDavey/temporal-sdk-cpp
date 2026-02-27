#pragma once

/// @file Data conversion between user types and Temporal payloads.

#include <any>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace temporalio::converters {

/// Represents a Temporal Payload (encoding metadata + data bytes).
/// This is a simplified representation; full protobuf Payload will be used
/// when the proto types are generated. For now, this provides the core
/// abstraction.
struct Payload {
    /// Metadata key-value pairs (e.g., "encoding" -> "json/plain").
    std::unordered_map<std::string, std::string> metadata;
    /// Raw payload data bytes.
    std::vector<uint8_t> data;
};

/// Represents a raw, unconverted payload value.
class RawValue {
public:
    explicit RawValue(Payload payload) : payload_(std::move(payload)) {}

    const Payload& payload() const noexcept { return payload_; }
    Payload& payload() noexcept { return payload_; }

private:
    Payload payload_;
};

/// Interface for a specific encoding converter.
/// Each encoding converter handles one encoding type (e.g., "json/plain",
/// "binary/null", "binary/plain", "json/protobuf", "binary/protobuf").
class IEncodingConverter {
public:
    virtual ~IEncodingConverter() = default;

    /// The encoding name (placed in payload metadata "encoding" key).
    virtual std::string_view encoding() const = 0;

    /// Try to convert the given value to a payload. Returns nullopt if this
    /// converter cannot handle the value and the next should be tried.
    virtual std::optional<Payload> try_to_payload(
        const std::any& value) const = 0;

    /// Convert the given payload to a value of the specified type.
    /// Only called for payloads whose encoding matches this converter.
    virtual std::any to_value(const Payload& payload,
                              std::type_index type) const = 0;
};

/// Interface for converting values to/from Temporal payloads.
/// Deterministic, synchronous -- used in workflows.
class IPayloadConverter {
public:
    virtual ~IPayloadConverter() = default;

    /// Convert the given value to a payload.
    virtual Payload to_payload(const std::any& value) const = 0;

    /// Convert the given payload to a value of the specified type.
    virtual std::any to_value(const Payload& payload,
                              std::type_index type) const = 0;

    /// Convenience template: convert a typed value to a payload.
    template <typename T>
    Payload to_payload_typed(const T& value) const {
        return to_payload(std::any(value));
    }

    /// Convenience template: convert a payload to a typed value.
    template <typename T>
    T to_value_typed(const Payload& payload) const {
        return std::any_cast<T>(to_value(payload, std::type_index(typeid(T))));
    }
};

/// Interface for converting exceptions to/from Temporal failure
/// representations. Deterministic, synchronous -- used in workflows.
class IFailureConverter {
public:
    virtual ~IFailureConverter() = default;

    /// Convert exception to a failure payload.
    /// The Payload here represents the serialized Failure proto. When proto
    /// types are integrated, this will use the actual Failure message type.
    virtual Payload to_failure(std::exception_ptr exception,
                               const IPayloadConverter& converter) const = 0;

    /// Convert a failure payload to an exception.
    virtual std::exception_ptr to_exception(
        const Payload& failure,
        const IPayloadConverter& converter) const = 0;
};

/// Interface for encoding/decoding payloads (e.g., encryption, compression).
class IPayloadCodec {
public:
    virtual ~IPayloadCodec() = default;

    /// Encode the given payloads.
    virtual std::vector<Payload> encode(
        std::vector<Payload> payloads) const = 0;

    /// Decode the given payloads.
    virtual std::vector<Payload> decode(
        std::vector<Payload> payloads) const = 0;
};

/// Default payload converter that iterates over encoding converters.
/// Tries each converter in order for to_payload; looks up by encoding for
/// to_value.
class DefaultPayloadConverter : public IPayloadConverter {
public:
    /// Constructs with the standard encoding converter set:
    /// BinaryNull, BinaryPlain, JsonProto, BinaryProto, JsonPlain.
    DefaultPayloadConverter();

    /// Constructs with a custom set of encoding converters.
    explicit DefaultPayloadConverter(
        std::vector<std::unique_ptr<IEncodingConverter>> converters);

    Payload to_payload(const std::any& value) const override;
    std::any to_value(const Payload& payload,
                      std::type_index type) const override;

    /// Access the encoding converters.
    const std::vector<std::unique_ptr<IEncodingConverter>>& encoding_converters()
        const noexcept {
        return converters_;
    }

private:
    std::vector<std::unique_ptr<IEncodingConverter>> converters_;
    std::unordered_map<std::string, IEncodingConverter*> indexed_;
};

/// Options for the default failure converter.
struct DefaultFailureConverterOptions {
    /// If true, move message and stack trace to encoded attributes
    /// (subject to codecs, useful for encryption).
    bool encode_common_attributes = false;
};

/// Default failure converter implementation.
class DefaultFailureConverter : public IFailureConverter {
public:
    DefaultFailureConverter();
    explicit DefaultFailureConverter(DefaultFailureConverterOptions options);

    Payload to_failure(std::exception_ptr exception,
                       const IPayloadConverter& converter) const override;

    std::exception_ptr to_exception(
        const Payload& failure,
        const IPayloadConverter& converter) const override;

    const DefaultFailureConverterOptions& options() const noexcept {
        return options_;
    }

private:
    DefaultFailureConverterOptions options_;
};

/// Data converter which combines a payload converter, a failure converter,
/// and an optional payload codec.
struct DataConverter {
    std::shared_ptr<IPayloadConverter> payload_converter;
    std::shared_ptr<IFailureConverter> failure_converter;
    std::shared_ptr<IPayloadCodec> payload_codec;  // may be null

    /// Returns the default data converter instance.
    static DataConverter default_instance();
};

// -- Built-in encoding converters --

/// Handles null/empty values with "binary/null" encoding.
class BinaryNullConverter : public IEncodingConverter {
public:
    std::string_view encoding() const override;
    std::optional<Payload> try_to_payload(const std::any& value) const override;
    std::any to_value(const Payload& payload,
                      std::type_index type) const override;
};

/// Handles raw byte vectors with "binary/plain" encoding.
class BinaryPlainConverter : public IEncodingConverter {
public:
    std::string_view encoding() const override;
    std::optional<Payload> try_to_payload(const std::any& value) const override;
    std::any to_value(const Payload& payload,
                      std::type_index type) const override;
};

/// Handles JSON serialization with "json/plain" encoding.
/// Uses nlohmann/json internally.
class JsonPlainConverter : public IEncodingConverter {
public:
    std::string_view encoding() const override;
    std::optional<Payload> try_to_payload(const std::any& value) const override;
    std::any to_value(const Payload& payload,
                      std::type_index type) const override;

    /// Register a type for JSON serialization.
    /// The serializer converts a std::any containing T to JSON bytes.
    /// The deserializer converts JSON bytes to a std::any containing T.
    template <typename T>
    void register_type(
        std::function<std::vector<uint8_t>(const T&)> serializer,
        std::function<T(const std::vector<uint8_t>&)> deserializer) {
        auto type_idx = std::type_index(typeid(T));
        serializers_[type_idx] = [ser = std::move(serializer)](
                                     const std::any& val) {
            return ser(std::any_cast<const T&>(val));
        };
        deserializers_[type_idx] = [deser = std::move(deserializer)](
                                       const std::vector<uint8_t>& data) {
            return std::any(deser(data));
        };
        registered_types_.insert(type_idx);
    }

    /// Check if a type is registered for JSON conversion.
    bool has_type(std::type_index type) const {
        return registered_types_.count(type) > 0;
    }

private:
    std::unordered_map<std::type_index,
                       std::function<std::vector<uint8_t>(const std::any&)>>
        serializers_;
    std::unordered_map<std::type_index,
                       std::function<std::any(const std::vector<uint8_t>&)>>
        deserializers_;
    std::set<std::type_index> registered_types_;
};

}  // namespace temporalio::converters
