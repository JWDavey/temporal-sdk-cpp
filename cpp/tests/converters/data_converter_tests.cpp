#include <gtest/gtest.h>

#include <any>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <vector>

#include "temporalio/converters/data_converter.h"

using namespace temporalio::converters;

// ===========================================================================
// Payload struct tests
// ===========================================================================

TEST(PayloadTest, DefaultEmpty) {
    Payload p;
    EXPECT_TRUE(p.metadata.empty());
    EXPECT_TRUE(p.data.empty());
}

// ===========================================================================
// RawValue tests
// ===========================================================================

TEST(RawValueTest, StoresPayload) {
    Payload p;
    p.metadata["encoding"] = "test";
    p.data = {1, 2, 3};

    RawValue rv(p);
    EXPECT_EQ(rv.payload().metadata.at("encoding"), "test");
    EXPECT_EQ(rv.payload().data, (std::vector<uint8_t>{1, 2, 3}));
}

// ===========================================================================
// BinaryNullConverter tests
// ===========================================================================

TEST(BinaryNullConverterTest, Encoding) {
    BinaryNullConverter conv;
    EXPECT_EQ(conv.encoding(), "binary/null");
}

TEST(BinaryNullConverterTest, ConvertNullToPayload) {
    BinaryNullConverter conv;
    std::any empty_val;  // has_value() == false
    auto result = conv.try_to_payload(empty_val);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->metadata.at("encoding"), "binary/null");
    EXPECT_TRUE(result->data.empty());
}

TEST(BinaryNullConverterTest, RejectsNonNull) {
    BinaryNullConverter conv;
    auto result = conv.try_to_payload(std::any(42));
    EXPECT_FALSE(result.has_value());
}

TEST(BinaryNullConverterTest, ToValueReturnsEmpty) {
    BinaryNullConverter conv;
    Payload p;
    p.metadata["encoding"] = "binary/null";
    auto val = conv.to_value(p, std::type_index(typeid(void)));
    EXPECT_FALSE(val.has_value());
}

// ===========================================================================
// BinaryPlainConverter tests
// ===========================================================================

TEST(BinaryPlainConverterTest, Encoding) {
    BinaryPlainConverter conv;
    EXPECT_EQ(conv.encoding(), "binary/plain");
}

TEST(BinaryPlainConverterTest, ConvertBytesToPayload) {
    BinaryPlainConverter conv;
    std::vector<uint8_t> bytes = {0x01, 0x02, 0x03};
    auto result = conv.try_to_payload(std::any(bytes));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->metadata.at("encoding"), "binary/plain");
    EXPECT_EQ(result->data, bytes);
}

TEST(BinaryPlainConverterTest, RejectsNonBytes) {
    BinaryPlainConverter conv;
    auto result = conv.try_to_payload(std::any(std::string("not bytes")));
    EXPECT_FALSE(result.has_value());
}

TEST(BinaryPlainConverterTest, ToValueReturnsBytes) {
    BinaryPlainConverter conv;
    Payload p;
    p.metadata["encoding"] = "binary/plain";
    p.data = {0xAA, 0xBB};
    auto val =
        conv.to_value(p, std::type_index(typeid(std::vector<uint8_t>)));
    auto result = std::any_cast<std::vector<uint8_t>>(val);
    EXPECT_EQ(result, (std::vector<uint8_t>{0xAA, 0xBB}));
}

TEST(BinaryPlainConverterTest, ToValueWrongTypeThrows) {
    BinaryPlainConverter conv;
    Payload p;
    p.metadata["encoding"] = "binary/plain";
    EXPECT_THROW(conv.to_value(p, std::type_index(typeid(std::string))),
                 std::invalid_argument);
}

// ===========================================================================
// JsonPlainConverter tests
// ===========================================================================

TEST(JsonPlainConverterTest, Encoding) {
    JsonPlainConverter conv;
    EXPECT_EQ(conv.encoding(), "json/plain");
}

TEST(JsonPlainConverterTest, ConvertStringToPayload) {
    JsonPlainConverter conv;
    auto result = conv.try_to_payload(std::any(std::string("hello")));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->metadata.at("encoding"), "json/plain");
    // String should be JSON-quoted
    std::string data_str(result->data.begin(), result->data.end());
    EXPECT_EQ(data_str, "\"hello\"");
}

TEST(JsonPlainConverterTest, ConvertIntToPayload) {
    JsonPlainConverter conv;
    auto result = conv.try_to_payload(std::any(42));
    ASSERT_TRUE(result.has_value());
    std::string data_str(result->data.begin(), result->data.end());
    EXPECT_EQ(data_str, "42");
}

TEST(JsonPlainConverterTest, ConvertBoolTrueToPayload) {
    JsonPlainConverter conv;
    auto result = conv.try_to_payload(std::any(true));
    ASSERT_TRUE(result.has_value());
    std::string data_str(result->data.begin(), result->data.end());
    EXPECT_EQ(data_str, "true");
}

TEST(JsonPlainConverterTest, ConvertBoolFalseToPayload) {
    JsonPlainConverter conv;
    auto result = conv.try_to_payload(std::any(false));
    ASSERT_TRUE(result.has_value());
    std::string data_str(result->data.begin(), result->data.end());
    EXPECT_EQ(data_str, "false");
}

TEST(JsonPlainConverterTest, ConvertDoubleToPayload) {
    JsonPlainConverter conv;
    auto result = conv.try_to_payload(std::any(3.14));
    ASSERT_TRUE(result.has_value());
    std::string data_str(result->data.begin(), result->data.end());
    // stod(data_str) should round-trip
    EXPECT_DOUBLE_EQ(std::stod(data_str), 3.14);
}

TEST(JsonPlainConverterTest, RejectsNull) {
    JsonPlainConverter conv;
    std::any empty;
    auto result = conv.try_to_payload(empty);
    EXPECT_FALSE(result.has_value());
}

TEST(JsonPlainConverterTest, ToValueString) {
    JsonPlainConverter conv;
    Payload p;
    p.metadata["encoding"] = "json/plain";
    std::string json = "\"world\"";
    p.data.assign(json.begin(), json.end());

    auto val = conv.to_value(p, std::type_index(typeid(std::string)));
    EXPECT_EQ(std::any_cast<std::string>(val), "world");
}

TEST(JsonPlainConverterTest, ToValueInt) {
    JsonPlainConverter conv;
    Payload p;
    p.metadata["encoding"] = "json/plain";
    std::string json = "123";
    p.data.assign(json.begin(), json.end());

    auto val = conv.to_value(p, std::type_index(typeid(int)));
    EXPECT_EQ(std::any_cast<int>(val), 123);
}

TEST(JsonPlainConverterTest, ToValueBool) {
    JsonPlainConverter conv;
    Payload p;
    p.metadata["encoding"] = "json/plain";
    std::string json = "true";
    p.data.assign(json.begin(), json.end());

    auto val = conv.to_value(p, std::type_index(typeid(bool)));
    EXPECT_TRUE(std::any_cast<bool>(val));
}

TEST(JsonPlainConverterTest, ToValueDouble) {
    JsonPlainConverter conv;
    Payload p;
    p.metadata["encoding"] = "json/plain";
    std::string json = "2.718";
    p.data.assign(json.begin(), json.end());

    auto val = conv.to_value(p, std::type_index(typeid(double)));
    EXPECT_DOUBLE_EQ(std::any_cast<double>(val), 2.718);
}

// ===========================================================================
// JsonPlainConverter custom type registration
// ===========================================================================

struct Point {
    int x, y;
};

TEST(JsonPlainConverterTest, RegisteredTypeRoundTrip) {
    JsonPlainConverter conv;

    conv.register_type<Point>(
        [](const Point& p) -> std::vector<uint8_t> {
            std::string json =
                "{\"x\":" + std::to_string(p.x) +
                ",\"y\":" + std::to_string(p.y) + "}";
            return std::vector<uint8_t>(json.begin(), json.end());
        },
        [](const std::vector<uint8_t>& data) -> Point {
            std::string json(data.begin(), data.end());
            // Simplified parsing for test
            int x = 0, y = 0;
            auto xpos = json.find("\"x\":");
            if (xpos != std::string::npos) {
                x = std::stoi(json.substr(xpos + 4));
            }
            auto ypos = json.find("\"y\":");
            if (ypos != std::string::npos) {
                y = std::stoi(json.substr(ypos + 4));
            }
            return Point{x, y};
        });

    EXPECT_TRUE(conv.has_type(std::type_index(typeid(Point))));

    auto payload_opt = conv.try_to_payload(std::any(Point{10, 20}));
    ASSERT_TRUE(payload_opt.has_value());

    auto result = conv.to_value(payload_opt.value(),
                                std::type_index(typeid(Point)));
    auto pt = std::any_cast<Point>(result);
    EXPECT_EQ(pt.x, 10);
    EXPECT_EQ(pt.y, 20);
}

TEST(JsonPlainConverterTest, UnregisteredTypeRejected) {
    JsonPlainConverter conv;
    struct Unknown {};
    auto result = conv.try_to_payload(std::any(Unknown{}));
    EXPECT_FALSE(result.has_value());
}

TEST(JsonPlainConverterTest, UnregisteredDeserializeThrows) {
    JsonPlainConverter conv;
    Payload p;
    p.metadata["encoding"] = "json/plain";
    std::string json = "{}";
    p.data.assign(json.begin(), json.end());

    struct Unknown {};
    EXPECT_THROW(conv.to_value(p, std::type_index(typeid(Unknown))),
                 std::invalid_argument);
}

// ===========================================================================
// DefaultPayloadConverter tests
// ===========================================================================

TEST(DefaultPayloadConverterTest, ConvertNull) {
    DefaultPayloadConverter conv;
    std::any null_val;
    auto payload = conv.to_payload(null_val);
    EXPECT_EQ(payload.metadata.at("encoding"), "binary/null");
}

TEST(DefaultPayloadConverterTest, ConvertBytes) {
    DefaultPayloadConverter conv;
    std::vector<uint8_t> bytes = {0x01, 0x02};
    auto payload = conv.to_payload(std::any(bytes));
    EXPECT_EQ(payload.metadata.at("encoding"), "binary/plain");
    EXPECT_EQ(payload.data, bytes);
}

TEST(DefaultPayloadConverterTest, ConvertString) {
    DefaultPayloadConverter conv;
    auto payload = conv.to_payload(std::any(std::string("test")));
    EXPECT_EQ(payload.metadata.at("encoding"), "json/plain");
}

TEST(DefaultPayloadConverterTest, ConvertInt) {
    DefaultPayloadConverter conv;
    auto payload = conv.to_payload(std::any(100));
    EXPECT_EQ(payload.metadata.at("encoding"), "json/plain");
}

TEST(DefaultPayloadConverterTest, RoundTripString) {
    DefaultPayloadConverter conv;
    std::string original = "round trip test";
    auto payload = conv.to_payload_typed(original);
    auto result = conv.to_value_typed<std::string>(payload);
    EXPECT_EQ(result, original);
}

TEST(DefaultPayloadConverterTest, RoundTripInt) {
    DefaultPayloadConverter conv;
    int original = 42;
    auto payload = conv.to_payload_typed(original);
    auto result = conv.to_value_typed<int>(payload);
    EXPECT_EQ(result, original);
}

TEST(DefaultPayloadConverterTest, RoundTripBool) {
    DefaultPayloadConverter conv;
    bool original = true;
    auto payload = conv.to_payload_typed(original);
    auto result = conv.to_value_typed<bool>(payload);
    EXPECT_EQ(result, original);
}

TEST(DefaultPayloadConverterTest, RoundTripBytes) {
    DefaultPayloadConverter conv;
    std::vector<uint8_t> original = {0x10, 0x20, 0x30};
    auto payload = conv.to_payload_typed(original);
    auto result = conv.to_value_typed<std::vector<uint8_t>>(payload);
    EXPECT_EQ(result, original);
}

TEST(DefaultPayloadConverterTest, RawValuePassthrough) {
    DefaultPayloadConverter conv;
    Payload original;
    original.metadata["encoding"] = "custom";
    original.data = {0xFF};

    auto payload = conv.to_payload(std::any(RawValue(original)));
    EXPECT_EQ(payload.metadata.at("encoding"), "custom");
    EXPECT_EQ(payload.data, (std::vector<uint8_t>{0xFF}));
}

TEST(DefaultPayloadConverterTest, RawValueDeserialization) {
    DefaultPayloadConverter conv;
    Payload p;
    p.metadata["encoding"] = "json/plain";
    p.data = {'4', '2'};

    auto val = conv.to_value(p, std::type_index(typeid(RawValue)));
    auto rv = std::any_cast<RawValue>(val);
    EXPECT_EQ(rv.payload().metadata.at("encoding"), "json/plain");
}

TEST(DefaultPayloadConverterTest, UnknownEncodingThrows) {
    DefaultPayloadConverter conv;
    Payload p;
    p.metadata["encoding"] = "unknown/encoding";
    EXPECT_THROW(conv.to_value(p, std::type_index(typeid(std::string))),
                 std::invalid_argument);
}

TEST(DefaultPayloadConverterTest, MissingEncodingThrows) {
    DefaultPayloadConverter conv;
    Payload p;  // no "encoding" in metadata
    EXPECT_THROW(conv.to_value(p, std::type_index(typeid(std::string))),
                 std::invalid_argument);
}

TEST(DefaultPayloadConverterTest, HasStandardConverters) {
    DefaultPayloadConverter conv;
    EXPECT_GE(conv.encoding_converters().size(), 3u);
}

// ===========================================================================
// DefaultFailureConverter tests
// ===========================================================================

TEST(DefaultFailureConverterTest, DefaultOptions) {
    DefaultFailureConverter conv;
    EXPECT_FALSE(conv.options().encode_common_attributes);
}

TEST(DefaultFailureConverterTest, CustomOptions) {
    DefaultFailureConverter conv({.encode_common_attributes = true});
    EXPECT_TRUE(conv.options().encode_common_attributes);
}

TEST(DefaultFailureConverterTest, RoundTrip) {
    DefaultPayloadConverter payload_conv;
    DefaultFailureConverter failure_conv;

    auto ex_ptr =
        std::make_exception_ptr(std::runtime_error("test failure"));
    auto failure = failure_conv.to_failure(ex_ptr, payload_conv);

    auto restored = failure_conv.to_exception(failure, payload_conv);
    ASSERT_NE(restored, nullptr);

    try {
        std::rethrow_exception(restored);
    } catch (const std::runtime_error& e) {
        EXPECT_EQ(std::string(e.what()), "test failure");
    } catch (...) {
        FAIL() << "Expected std::runtime_error";
    }
}

// ===========================================================================
// DataConverter tests
// ===========================================================================

TEST(DataConverterTest, DefaultInstance) {
    auto dc = DataConverter::default_instance();
    EXPECT_NE(dc.payload_converter, nullptr);
    EXPECT_NE(dc.failure_converter, nullptr);
    EXPECT_EQ(dc.payload_codec, nullptr);
}

TEST(DataConverterTest, DefaultInstanceRoundTrip) {
    auto dc = DataConverter::default_instance();
    auto payload = dc.payload_converter->to_payload_typed(std::string("value"));
    auto result = dc.payload_converter->to_value_typed<std::string>(payload);
    EXPECT_EQ(result, "value");
}

// ===========================================================================
// IPayloadCodec (pass-through test)
// ===========================================================================

namespace {

class XorCodec : public IPayloadCodec {
public:
    std::vector<Payload> encode(std::vector<Payload> payloads) const override {
        for (auto& p : payloads) {
            for (auto& b : p.data) {
                b ^= 0xFF;
            }
            p.metadata["encoded"] = "xor";
        }
        return payloads;
    }

    std::vector<Payload> decode(std::vector<Payload> payloads) const override {
        for (auto& p : payloads) {
            for (auto& b : p.data) {
                b ^= 0xFF;
            }
            p.metadata.erase("encoded");
        }
        return payloads;
    }
};

}  // namespace

TEST(PayloadCodecTest, XorCodecRoundTrip) {
    XorCodec codec;

    Payload original;
    original.metadata["encoding"] = "json/plain";
    std::string data = "hello";
    original.data.assign(data.begin(), data.end());

    auto encoded = codec.encode({original});
    ASSERT_EQ(encoded.size(), 1u);
    // Data should be XOR'd
    EXPECT_NE(encoded[0].data, original.data);
    EXPECT_EQ(encoded[0].metadata.at("encoded"), "xor");

    auto decoded = codec.decode(std::move(encoded));
    ASSERT_EQ(decoded.size(), 1u);
    EXPECT_EQ(decoded[0].data, original.data);
    EXPECT_EQ(decoded[0].metadata.count("encoded"), 0u);
}
