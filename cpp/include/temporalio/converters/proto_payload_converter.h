#pragma once

/// @file converters/proto_payload_converter.h
/// @brief Encoding converters for protobuf messages: "binary/protobuf" and
///        "json/protobuf".
///
/// These converters handle serialization/deserialization of google::protobuf::Message
/// subclasses to/from Temporal payloads.

#include <any>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <temporalio/converters/data_converter.h>

namespace temporalio::converters {

/// Handles protobuf messages with "binary/protobuf" encoding.
/// Serializes using the standard protobuf binary wire format.
class BinaryProtoConverter : public IEncodingConverter {
public:
    std::string_view encoding() const override { return "binary/protobuf"; }

    /// Register a protobuf message type for conversion.
    /// @tparam T A google::protobuf::Message subclass.
    template <typename T>
    void register_type() {
        static_assert(std::is_base_of_v<google::protobuf::Message, T>,
                      "T must be a google::protobuf::Message subclass");
        auto type_idx = std::type_index(typeid(T));
        serializers_[type_idx] = [](const std::any& val) -> std::vector<uint8_t> {
            const auto& msg = std::any_cast<const T&>(val);
            std::string bytes;
            if (!msg.SerializeToString(&bytes)) {
                throw std::runtime_error(
                    "Failed to serialize protobuf message");
            }
            return std::vector<uint8_t>(bytes.begin(), bytes.end());
        };
        deserializers_[type_idx] =
            [](const std::vector<uint8_t>& data) -> std::any {
            T msg;
            if (!msg.ParseFromArray(data.data(),
                                    static_cast<int>(data.size()))) {
                throw std::runtime_error(
                    "Failed to parse protobuf message");
            }
            return std::any(std::move(msg));
        };
        // Store the message type name for the metadata
        type_names_[type_idx] = T::descriptor()->full_name();
    }

    std::optional<Payload> try_to_payload(const std::any& value) const override {
        if (!value.has_value()) {
            return std::nullopt;
        }
        auto it = serializers_.find(std::type_index(value.type()));
        if (it == serializers_.end()) {
            return std::nullopt;
        }
        auto bytes = it->second(value);
        Payload p;
        p.metadata["encoding"] = std::string(encoding());
        auto name_it = type_names_.find(std::type_index(value.type()));
        if (name_it != type_names_.end()) {
            p.metadata["messageType"] = name_it->second;
        }
        p.data = std::move(bytes);
        return p;
    }

    std::any to_value(const Payload& payload,
                      std::type_index type) const override {
        auto it = deserializers_.find(type);
        if (it == deserializers_.end()) {
            throw std::invalid_argument(
                "BinaryProtoConverter: no deserializer registered for type");
        }
        return it->second(payload.data);
    }

private:
    std::unordered_map<std::type_index,
                       std::function<std::vector<uint8_t>(const std::any&)>>
        serializers_;
    std::unordered_map<std::type_index,
                       std::function<std::any(const std::vector<uint8_t>&)>>
        deserializers_;
    std::unordered_map<std::type_index, std::string> type_names_;
};

/// Handles protobuf messages with "json/protobuf" encoding.
/// Serializes using protobuf's canonical JSON representation.
class JsonProtoConverter : public IEncodingConverter {
public:
    std::string_view encoding() const override { return "json/protobuf"; }

    /// Register a protobuf message type for conversion.
    /// @tparam T A google::protobuf::Message subclass.
    template <typename T>
    void register_type() {
        static_assert(std::is_base_of_v<google::protobuf::Message, T>,
                      "T must be a google::protobuf::Message subclass");
        auto type_idx = std::type_index(typeid(T));
        serializers_[type_idx] = [](const std::any& val) -> std::vector<uint8_t> {
            const auto& msg = std::any_cast<const T&>(val);
            std::string json;
            auto status =
                google::protobuf::util::MessageToJsonString(msg, &json);
            if (!status.ok()) {
                throw std::runtime_error(
                    "Failed to serialize protobuf to JSON: " +
                    std::string(status.message()));
            }
            return std::vector<uint8_t>(json.begin(), json.end());
        };
        deserializers_[type_idx] =
            [](const std::vector<uint8_t>& data) -> std::any {
            T msg;
            std::string json(data.begin(), data.end());
            auto status =
                google::protobuf::util::JsonStringToMessage(json, &msg);
            if (!status.ok()) {
                throw std::runtime_error(
                    "Failed to parse protobuf from JSON: " +
                    std::string(status.message()));
            }
            return std::any(std::move(msg));
        };
        type_names_[type_idx] = T::descriptor()->full_name();
    }

    std::optional<Payload> try_to_payload(const std::any& value) const override {
        if (!value.has_value()) {
            return std::nullopt;
        }
        auto it = serializers_.find(std::type_index(value.type()));
        if (it == serializers_.end()) {
            return std::nullopt;
        }
        auto bytes = it->second(value);
        Payload p;
        p.metadata["encoding"] = std::string(encoding());
        auto name_it = type_names_.find(std::type_index(value.type()));
        if (name_it != type_names_.end()) {
            p.metadata["messageType"] = name_it->second;
        }
        p.data = std::move(bytes);
        return p;
    }

    std::any to_value(const Payload& payload,
                      std::type_index type) const override {
        auto it = deserializers_.find(type);
        if (it == deserializers_.end()) {
            throw std::invalid_argument(
                "JsonProtoConverter: no deserializer registered for type");
        }
        return it->second(payload.data);
    }

private:
    std::unordered_map<std::type_index,
                       std::function<std::vector<uint8_t>(const std::any&)>>
        serializers_;
    std::unordered_map<std::type_index,
                       std::function<std::any(const std::vector<uint8_t>&)>>
        deserializers_;
    std::unordered_map<std::type_index, std::string> type_names_;
};

}  // namespace temporalio::converters
