#pragma once

/// @file api/proto_util.h
/// @brief Utilities for working with Temporal protobuf types in C++.
///
/// Provides conversion functions between serialized byte representations
/// and protobuf message objects.

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <google/protobuf/message.h>

namespace temporalio::api {

/// Serialize a protobuf message to a byte vector.
/// @param msg The protobuf message to serialize.
/// @return Serialized bytes.
/// @throws std::runtime_error if serialization fails.
inline std::vector<uint8_t> serialize_proto(
    const google::protobuf::Message& msg) {
    std::string bytes;
    if (!msg.SerializeToString(&bytes)) {
        throw std::runtime_error(
            std::string("Failed to serialize protobuf message: ") +
            msg.GetTypeName());
    }
    return std::vector<uint8_t>(bytes.begin(), bytes.end());
}

/// Deserialize a protobuf message from a byte vector.
/// @tparam T The protobuf message type to deserialize into.
/// @param data The serialized bytes.
/// @return The deserialized message.
/// @throws std::runtime_error if deserialization fails.
template <typename T>
T deserialize_proto(const std::vector<uint8_t>& data) {
    T msg;
    if (!msg.ParseFromArray(data.data(), static_cast<int>(data.size()))) {
        throw std::runtime_error(
            std::string("Failed to deserialize protobuf message: ") +
            msg.GetTypeName());
    }
    return msg;
}

/// Deserialize a protobuf message from a string.
/// @tparam T The protobuf message type to deserialize into.
/// @param data The serialized string.
/// @return The deserialized message.
/// @throws std::runtime_error if deserialization fails.
template <typename T>
T deserialize_proto(const std::string& data) {
    T msg;
    if (!msg.ParseFromString(data)) {
        throw std::runtime_error(
            std::string("Failed to deserialize protobuf message: ") +
            msg.GetTypeName());
    }
    return msg;
}

/// Try to deserialize a protobuf message, returning nullopt on failure.
/// @tparam T The protobuf message type.
/// @param data The serialized bytes.
/// @return The deserialized message, or nullopt if deserialization fails.
template <typename T>
std::optional<T> try_deserialize_proto(const std::vector<uint8_t>& data) {
    T msg;
    if (!msg.ParseFromArray(data.data(), static_cast<int>(data.size()))) {
        return std::nullopt;
    }
    return msg;
}

}  // namespace temporalio::api
