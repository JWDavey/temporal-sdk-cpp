#include "temporalio/converters/data_converter.h"

#include <algorithm>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace temporalio::converters {

// -- BinaryNullConverter --

std::string_view BinaryNullConverter::encoding() const {
    return "binary/null";
}

std::optional<Payload> BinaryNullConverter::try_to_payload(
    const std::any& value) const {
    if (!value.has_value()) {
        Payload p;
        p.metadata["encoding"] = std::string(encoding());
        return p;
    }
    return std::nullopt;
}

std::any BinaryNullConverter::to_value(const Payload& /*payload*/,
                                       std::type_index /*type*/) const {
    return std::any{};  // null/empty value
}

// -- BinaryPlainConverter --

std::string_view BinaryPlainConverter::encoding() const {
    return "binary/plain";
}

std::optional<Payload> BinaryPlainConverter::try_to_payload(
    const std::any& value) const {
    if (value.type() == typeid(std::vector<uint8_t>)) {
        auto& bytes = std::any_cast<const std::vector<uint8_t>&>(value);
        Payload p;
        p.metadata["encoding"] = std::string(encoding());
        p.data = bytes;
        return p;
    }
    return std::nullopt;
}

std::any BinaryPlainConverter::to_value(const Payload& payload,
                                        std::type_index type) const {
    if (type == std::type_index(typeid(std::vector<uint8_t>))) {
        return std::any(payload.data);
    }
    throw std::invalid_argument(
        "BinaryPlainConverter can only convert to std::vector<uint8_t>");
}

// -- JsonPlainConverter --

std::string_view JsonPlainConverter::encoding() const {
    return "json/plain";
}

std::optional<Payload> JsonPlainConverter::try_to_payload(
    const std::any& value) const {
    if (!value.has_value()) {
        return std::nullopt;
    }
    auto type_idx = std::type_index(value.type());

    // Handle std::string natively
    if (type_idx == std::type_index(typeid(std::string))) {
        auto& str = std::any_cast<const std::string&>(value);
        // Use nlohmann/json for proper escaping of special characters
        std::string json = nlohmann::json(str).dump();
        Payload p;
        p.metadata["encoding"] = std::string(encoding());
        p.data.assign(json.begin(), json.end());
        return p;
    }

    // Handle int
    if (type_idx == std::type_index(typeid(int))) {
        auto val = std::any_cast<int>(value);
        std::string json = std::to_string(val);
        Payload p;
        p.metadata["encoding"] = std::string(encoding());
        p.data.assign(json.begin(), json.end());
        return p;
    }

    // Handle double
    if (type_idx == std::type_index(typeid(double))) {
        auto val = std::any_cast<double>(value);
        std::string json = std::to_string(val);
        Payload p;
        p.metadata["encoding"] = std::string(encoding());
        p.data.assign(json.begin(), json.end());
        return p;
    }

    // Handle bool
    if (type_idx == std::type_index(typeid(bool))) {
        auto val = std::any_cast<bool>(value);
        std::string json = val ? "true" : "false";
        Payload p;
        p.metadata["encoding"] = std::string(encoding());
        p.data.assign(json.begin(), json.end());
        return p;
    }

    // Try registered type serializers
    auto it = serializers_.find(type_idx);
    if (it != serializers_.end()) {
        auto bytes = it->second(value);
        Payload p;
        p.metadata["encoding"] = std::string(encoding());
        p.data = std::move(bytes);
        return p;
    }

    // Cannot handle this type
    return std::nullopt;
}

std::any JsonPlainConverter::to_value(const Payload& payload,
                                      std::type_index type) const {
    // Handle std::string
    if (type == std::type_index(typeid(std::string))) {
        std::string json(payload.data.begin(), payload.data.end());
        // Use nlohmann/json for proper unescaping of JSON strings
        auto parsed = nlohmann::json::parse(json, nullptr, false);
        if (!parsed.is_discarded() && parsed.is_string()) {
            return std::any(parsed.get<std::string>());
        }
        return std::any(json);
    }

    // Handle int
    if (type == std::type_index(typeid(int))) {
        std::string json(payload.data.begin(), payload.data.end());
        return std::any(std::stoi(json));
    }

    // Handle double
    if (type == std::type_index(typeid(double))) {
        std::string json(payload.data.begin(), payload.data.end());
        return std::any(std::stod(json));
    }

    // Handle bool
    if (type == std::type_index(typeid(bool))) {
        std::string json(payload.data.begin(), payload.data.end());
        return std::any(json == "true");
    }

    // Try registered type deserializers
    auto it = deserializers_.find(type);
    if (it != deserializers_.end()) {
        return it->second(payload.data);
    }

    throw std::invalid_argument(
        "JsonPlainConverter: no deserializer registered for requested type");
}

// -- DefaultPayloadConverter --

DefaultPayloadConverter::DefaultPayloadConverter() {
    converters_.push_back(std::make_unique<BinaryNullConverter>());
    converters_.push_back(std::make_unique<BinaryPlainConverter>());
    // JsonProto and BinaryProto would be added here when proto support
    // is integrated. For now, we include only the basic converters.
    converters_.push_back(std::make_unique<JsonPlainConverter>());

    for (auto& conv : converters_) {
        indexed_[std::string(conv->encoding())] = conv.get();
    }
}

DefaultPayloadConverter::DefaultPayloadConverter(
    std::vector<std::unique_ptr<IEncodingConverter>> converters)
    : converters_(std::move(converters)) {
    for (auto& conv : converters_) {
        indexed_[std::string(conv->encoding())] = conv.get();
    }
}

Payload DefaultPayloadConverter::to_payload(const std::any& value) const {
    // Handle RawValue directly
    if (value.type() == typeid(RawValue)) {
        return std::any_cast<const RawValue&>(value).payload();
    }

    for (auto& conv : converters_) {
        auto result = conv->try_to_payload(value);
        if (result) {
            return std::move(*result);
        }
    }
    throw std::invalid_argument(
        "Value has no known converter for any encoding");
}

std::any DefaultPayloadConverter::to_value(const Payload& payload,
                                           std::type_index type) const {
    // Handle RawValue request
    if (type == std::type_index(typeid(RawValue))) {
        return std::any(RawValue(payload));
    }

    auto enc_it = payload.metadata.find("encoding");
    if (enc_it == payload.metadata.end()) {
        throw std::invalid_argument("Payload missing 'encoding' metadata");
    }

    auto conv_it = indexed_.find(enc_it->second);
    if (conv_it == indexed_.end()) {
        throw std::invalid_argument("Unknown payload encoding: " +
                                    enc_it->second);
    }

    return conv_it->second->to_value(payload, type);
}

// -- DefaultFailureConverter --

DefaultFailureConverter::DefaultFailureConverter()
    : options_{} {}

DefaultFailureConverter::DefaultFailureConverter(
    DefaultFailureConverterOptions options)
    : options_(options) {}

Payload DefaultFailureConverter::to_failure(
    std::exception_ptr exception,
    const IPayloadConverter& converter) const {
    // Serialize exception info into a payload.
    // This is a simplified implementation. When proto types are integrated,
    // this will create a proper Failure proto message.
    std::string message;
    try {
        std::rethrow_exception(exception);
    } catch (const std::exception& e) {
        message = e.what();
    } catch (...) {
        message = "Unknown exception";
    }

    Payload failure;
    failure.metadata["encoding"] = "json/plain";
    // Use nlohmann/json for proper escaping of message content
    nlohmann::json j;
    j["message"] = message;
    std::string json = j.dump();
    failure.data.assign(json.begin(), json.end());

    return failure;
}

std::exception_ptr DefaultFailureConverter::to_exception(
    const Payload& failure, const IPayloadConverter& /*converter*/) const {
    // Extract message from the failure payload using nlohmann/json.
    // Simplified implementation until proto types are integrated.
    std::string data(failure.data.begin(), failure.data.end());

    std::string message = data;
    auto parsed = nlohmann::json::parse(data, nullptr, false);
    if (!parsed.is_discarded() && parsed.is_object() &&
        parsed.contains("message") && parsed["message"].is_string()) {
        message = parsed["message"].get<std::string>();
    }

    return std::make_exception_ptr(std::runtime_error(message));
}

// -- DataConverter --

DataConverter DataConverter::default_instance() {
    return DataConverter{
        std::make_shared<DefaultPayloadConverter>(),
        std::make_shared<DefaultFailureConverter>(),
        nullptr,
    };
}

}  // namespace temporalio::converters
