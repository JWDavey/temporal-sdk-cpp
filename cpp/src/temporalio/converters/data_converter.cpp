#include "temporalio/converters/data_converter.h"

#ifdef TEMPORALIO_HAS_PROTOBUF
#include "temporalio/converters/proto_payload_converter.h"
#endif

#include <algorithm>
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "temporalio/exceptions/temporal_exception.h"

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
    nlohmann::json j;

    // Handle nlohmann::json directly
    if (type_idx == std::type_index(typeid(nlohmann::json))) {
        j = std::any_cast<const nlohmann::json&>(value);
    }
    // Handle std::string
    else if (type_idx == std::type_index(typeid(std::string))) {
        j = std::any_cast<const std::string&>(value);
    }
    // Handle bool (must be before int to avoid implicit conversion)
    else if (type_idx == std::type_index(typeid(bool))) {
        j = std::any_cast<bool>(value);
    }
    // Handle int
    else if (type_idx == std::type_index(typeid(int))) {
        j = std::any_cast<int>(value);
    }
    // Handle int64_t
    else if (type_idx == std::type_index(typeid(int64_t))) {
        j = std::any_cast<int64_t>(value);
    }
    // Handle uint64_t
    else if (type_idx == std::type_index(typeid(uint64_t))) {
        j = std::any_cast<uint64_t>(value);
    }
    // Handle float
    else if (type_idx == std::type_index(typeid(float))) {
        j = std::any_cast<float>(value);
    }
    // Handle double
    else if (type_idx == std::type_index(typeid(double))) {
        j = std::any_cast<double>(value);
    }
    // Try registered type serializers
    else {
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

    // Serialize the nlohmann::json to bytes
    std::string json_str = j.dump();
    Payload p;
    p.metadata["encoding"] = std::string(encoding());
    p.data.assign(json_str.begin(), json_str.end());
    return p;
}

std::any JsonPlainConverter::to_value(const Payload& payload,
                                      std::type_index type) const {
    // Try registered type deserializers first (they work on raw bytes)
    auto it = deserializers_.find(type);
    if (it != deserializers_.end()) {
        return it->second(payload.data);
    }

    // Parse JSON once for all built-in types
    std::string json_str(payload.data.begin(), payload.data.end());
    auto parsed = nlohmann::json::parse(json_str, nullptr, false);

    if (parsed.is_discarded()) {
        throw std::invalid_argument(
            "JsonPlainConverter: failed to parse JSON payload");
    }

    // Handle nlohmann::json directly
    if (type == std::type_index(typeid(nlohmann::json))) {
        return std::any(std::move(parsed));
    }

    // Handle std::string
    if (type == std::type_index(typeid(std::string))) {
        if (parsed.is_string()) {
            return std::any(parsed.get<std::string>());
        }
        // Return the raw JSON text for non-string JSON values
        return std::any(json_str);
    }

    // Handle bool
    if (type == std::type_index(typeid(bool))) {
        if (parsed.is_boolean()) {
            return std::any(parsed.get<bool>());
        }
        throw std::invalid_argument(
            "JsonPlainConverter: JSON value is not a boolean");
    }

    // Handle int
    if (type == std::type_index(typeid(int))) {
        if (parsed.is_number_integer()) {
            return std::any(parsed.get<int>());
        }
        throw std::invalid_argument(
            "JsonPlainConverter: JSON value is not an integer");
    }

    // Handle int64_t
    if (type == std::type_index(typeid(int64_t))) {
        if (parsed.is_number_integer()) {
            return std::any(parsed.get<int64_t>());
        }
        throw std::invalid_argument(
            "JsonPlainConverter: JSON value is not an integer");
    }

    // Handle uint64_t
    if (type == std::type_index(typeid(uint64_t))) {
        if (parsed.is_number_unsigned()) {
            return std::any(parsed.get<uint64_t>());
        }
        throw std::invalid_argument(
            "JsonPlainConverter: JSON value is not an unsigned integer");
    }

    // Handle float
    if (type == std::type_index(typeid(float))) {
        if (parsed.is_number()) {
            return std::any(parsed.get<float>());
        }
        throw std::invalid_argument(
            "JsonPlainConverter: JSON value is not a number");
    }

    // Handle double
    if (type == std::type_index(typeid(double))) {
        if (parsed.is_number()) {
            return std::any(parsed.get<double>());
        }
        throw std::invalid_argument(
            "JsonPlainConverter: JSON value is not a number");
    }

    throw std::invalid_argument(
        "JsonPlainConverter: no deserializer registered for requested type");
}

// -- DefaultPayloadConverter --

DefaultPayloadConverter::DefaultPayloadConverter() {
    converters_.push_back(std::make_unique<BinaryNullConverter>());
    converters_.push_back(std::make_unique<BinaryPlainConverter>());
#ifdef TEMPORALIO_HAS_PROTOBUF
    converters_.push_back(std::make_unique<JsonProtoConverter>());
    converters_.push_back(std::make_unique<BinaryProtoConverter>());
#endif
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

Failure DefaultFailureConverter::to_failure(
    std::exception_ptr exception,
    const IPayloadConverter& converter) const {
    Failure failure;

    try {
        std::rethrow_exception(exception);
    } catch (const exceptions::ApplicationFailureException& e) {
        failure.message = e.what();
        failure.failure_type = "ApplicationFailure";
        failure.error_type = e.error_type().value_or("");
        failure.non_retryable = e.non_retryable();
        if (e.inner()) {
            failure.cause = std::make_shared<Failure>(
                to_failure(e.inner(), converter));
        }
    } catch (const exceptions::CanceledFailureException& e) {
        failure.message = e.what();
        failure.failure_type = "CanceledFailure";
        if (e.inner()) {
            failure.cause = std::make_shared<Failure>(
                to_failure(e.inner(), converter));
        }
    } catch (const exceptions::TerminatedFailureException& e) {
        failure.message = e.what();
        failure.failure_type = "TerminatedFailure";
        if (e.inner()) {
            failure.cause = std::make_shared<Failure>(
                to_failure(e.inner(), converter));
        }
    } catch (const exceptions::TimeoutFailureException& e) {
        failure.message = e.what();
        failure.failure_type = "TimeoutFailure";
        if (e.inner()) {
            failure.cause = std::make_shared<Failure>(
                to_failure(e.inner(), converter));
        }
    } catch (const exceptions::ServerFailureException& e) {
        failure.message = e.what();
        failure.failure_type = "ServerFailure";
        failure.non_retryable = e.non_retryable();
        if (e.inner()) {
            failure.cause = std::make_shared<Failure>(
                to_failure(e.inner(), converter));
        }
    } catch (const exceptions::ActivityFailureException& e) {
        failure.message = e.what();
        failure.failure_type = "ActivityFailure";
        Failure::ActivityFailureInfo afi;
        afi.activity_type = e.activity_type();
        afi.activity_id = e.activity_id();
        afi.identity = e.identity().value_or("");
        afi.retry_state = e.retry_state();
        failure.activity_failure_info = std::move(afi);
        if (e.inner()) {
            failure.cause = std::make_shared<Failure>(
                to_failure(e.inner(), converter));
        }
    } catch (const exceptions::ChildWorkflowFailureException& e) {
        failure.message = e.what();
        failure.failure_type = "ChildWorkflowFailure";
        Failure::ChildWorkflowFailureInfo cwfi;
        cwfi.ns = e.ns();
        cwfi.workflow_id = e.workflow_id();
        cwfi.run_id = e.run_id();
        cwfi.workflow_type = e.workflow_type();
        cwfi.retry_state = e.retry_state();
        failure.child_workflow_failure_info = std::move(cwfi);
        if (e.inner()) {
            failure.cause = std::make_shared<Failure>(
                to_failure(e.inner(), converter));
        }
    } catch (const exceptions::NexusOperationFailureException& e) {
        failure.message = e.what();
        failure.failure_type = "NexusOperationFailure";
        Failure::NexusOperationFailureInfo nofi;
        nofi.endpoint = e.endpoint();
        nofi.service = e.service();
        nofi.operation = e.operation();
        nofi.operation_token = e.operation_token();
        failure.nexus_operation_failure_info = std::move(nofi);
        if (e.inner()) {
            failure.cause = std::make_shared<Failure>(
                to_failure(e.inner(), converter));
        }
    } catch (const exceptions::NexusHandlerFailureException& e) {
        failure.message = e.what();
        failure.failure_type = "NexusHandlerFailure";
        if (e.inner()) {
            failure.cause = std::make_shared<Failure>(
                to_failure(e.inner(), converter));
        }
    } catch (const exceptions::FailureException& e) {
        // Generic FailureException subclass not otherwise caught
        failure.message = e.what();
        failure.failure_type = "Failure";
        failure.stack_trace = e.failure_stack_trace();
        if (e.inner()) {
            failure.cause = std::make_shared<Failure>(
                to_failure(e.inner(), converter));
        }
    } catch (const std::exception& e) {
        // Any non-Temporal exception -> ApplicationFailure with error type name
        failure.message = e.what();
        failure.failure_type = "ApplicationFailure";
    } catch (...) {
        failure.message = "Unknown exception";
        failure.failure_type = "ApplicationFailure";
    }

    // If requested, move message and stack trace to encoded attributes
    if (options_.encode_common_attributes) {
        nlohmann::json attrs;
        attrs["message"] = failure.message;
        attrs["stack_trace"] = failure.stack_trace;
        std::string json_str = attrs.dump();

        Payload encoded_payload;
        encoded_payload.metadata["encoding"] = "json/plain";
        encoded_payload.data.assign(json_str.begin(), json_str.end());
        failure.encoded_attributes = std::move(encoded_payload);

        failure.message = "Encoded failure";
        failure.stack_trace.clear();
    }

    return failure;
}

std::exception_ptr DefaultFailureConverter::to_exception(
    const Failure& failure, const IPayloadConverter& converter) const {
    // Decode encoded attributes if present
    Failure decoded_failure = failure;
    if (decoded_failure.encoded_attributes.has_value()) {
        try {
            std::string data_str(
                decoded_failure.encoded_attributes->data.begin(),
                decoded_failure.encoded_attributes->data.end());
            auto parsed = nlohmann::json::parse(data_str, nullptr, false);
            if (!parsed.is_discarded() && parsed.is_object()) {
                if (parsed.contains("message") &&
                    parsed["message"].is_string()) {
                    decoded_failure.message =
                        parsed["message"].get<std::string>();
                }
                if (parsed.contains("stack_trace") &&
                    parsed["stack_trace"].is_string()) {
                    decoded_failure.stack_trace =
                        parsed["stack_trace"].get<std::string>();
                }
            }
        } catch (...) {
            // If we can't decode attributes, keep as-is
        }
    }

    // Recursively convert the cause
    std::exception_ptr inner_exception;
    if (decoded_failure.cause) {
        inner_exception = to_exception(*decoded_failure.cause, converter);
    }

    // Map failure_type string back to exception types
    const auto& ftype = decoded_failure.failure_type;

    if (ftype == "ApplicationFailure") {
        return std::make_exception_ptr(
            exceptions::ApplicationFailureException(
                decoded_failure.message,
                decoded_failure.error_type.empty()
                    ? std::nullopt
                    : std::optional<std::string>(decoded_failure.error_type),
                decoded_failure.non_retryable,
                std::nullopt,
                inner_exception));
    } else if (ftype == "CanceledFailure") {
        return std::make_exception_ptr(
            exceptions::CanceledFailureException(
                decoded_failure.message, inner_exception));
    } else if (ftype == "TerminatedFailure") {
        return std::make_exception_ptr(
            exceptions::TerminatedFailureException(
                decoded_failure.message, inner_exception));
    } else if (ftype == "TimeoutFailure") {
        return std::make_exception_ptr(
            exceptions::TimeoutFailureException(
                decoded_failure.message,
                exceptions::TimeoutFailureException::TimeoutType::kUnspecified,
                inner_exception));
    } else if (ftype == "ServerFailure") {
        return std::make_exception_ptr(
            exceptions::ServerFailureException(
                decoded_failure.message,
                decoded_failure.non_retryable,
                inner_exception));
    } else if (ftype == "ActivityFailure") {
        auto& afi = decoded_failure.activity_failure_info;
        return std::make_exception_ptr(
            exceptions::ActivityFailureException(
                decoded_failure.message,
                afi ? afi->activity_type : "",
                afi ? afi->activity_id : "",
                afi ? std::optional<std::string>(afi->identity) : std::nullopt,
                afi ? afi->retry_state : 0,
                inner_exception));
    } else if (ftype == "ChildWorkflowFailure") {
        auto& cwfi = decoded_failure.child_workflow_failure_info;
        return std::make_exception_ptr(
            exceptions::ChildWorkflowFailureException(
                decoded_failure.message,
                cwfi ? cwfi->ns : "",
                cwfi ? cwfi->workflow_id : "",
                cwfi ? cwfi->run_id : "",
                cwfi ? cwfi->workflow_type : "",
                cwfi ? cwfi->retry_state : 0,
                inner_exception));
    } else if (ftype == "NexusOperationFailure") {
        auto& nofi = decoded_failure.nexus_operation_failure_info;
        return std::make_exception_ptr(
            exceptions::NexusOperationFailureException(
                decoded_failure.message,
                nofi ? nofi->endpoint : "",
                nofi ? nofi->service : "",
                nofi ? nofi->operation : "",
                nofi ? nofi->operation_token : "",
                inner_exception));
    } else if (ftype == "NexusHandlerFailure") {
        return std::make_exception_ptr(
            exceptions::NexusHandlerFailureException(
                decoded_failure.message, "", 0,
                inner_exception));
    } else {
        // Default: wrap as runtime_error (unknown failure type)
        return std::make_exception_ptr(
            std::runtime_error(decoded_failure.message));
    }
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
