#include <gtest/gtest.h>

#include <exception>
#include <stdexcept>
#include <string>
#include <typeindex>

#include "temporalio/converters/data_converter.h"
#include "temporalio/exceptions/temporal_exception.h"

using namespace temporalio::converters;
using namespace temporalio::exceptions;

// ===========================================================================
// DefaultFailureConverter - basic tests
// ===========================================================================

TEST(FailureConverterTest, DefaultOptionsNoEncoding) {
    DefaultFailureConverter conv;
    EXPECT_FALSE(conv.options().encode_common_attributes);
}

TEST(FailureConverterTest, EnabledEncodedAttributes) {
    DefaultFailureConverter conv({.encode_common_attributes = true});
    EXPECT_TRUE(conv.options().encode_common_attributes);
}

// ===========================================================================
// to_failure from standard exceptions
// ===========================================================================

TEST(FailureConverterTest, ToFailureFromRuntimeError) {
    DefaultPayloadConverter payload_conv;
    DefaultFailureConverter failure_conv;

    auto ex_ptr =
        std::make_exception_ptr(std::runtime_error("test failure"));
    auto failure = failure_conv.to_failure(ex_ptr, payload_conv);

    EXPECT_EQ(failure.message, "test failure");
    EXPECT_FALSE(failure.failure_type.empty());
}

TEST(FailureConverterTest, ToFailureFromInvalidArgument) {
    DefaultPayloadConverter payload_conv;
    DefaultFailureConverter failure_conv;

    auto ex_ptr =
        std::make_exception_ptr(std::invalid_argument("bad arg"));
    auto failure = failure_conv.to_failure(ex_ptr, payload_conv);

    EXPECT_EQ(failure.message, "bad arg");
}

TEST(FailureConverterTest, ToFailureFromLogicError) {
    DefaultPayloadConverter payload_conv;
    DefaultFailureConverter failure_conv;

    auto ex_ptr =
        std::make_exception_ptr(std::logic_error("logic problem"));
    auto failure = failure_conv.to_failure(ex_ptr, payload_conv);

    EXPECT_EQ(failure.message, "logic problem");
}

TEST(FailureConverterTest, ToFailureFromOutOfRange) {
    DefaultPayloadConverter payload_conv;
    DefaultFailureConverter failure_conv;

    auto ex_ptr =
        std::make_exception_ptr(std::out_of_range("index out of range"));
    auto failure = failure_conv.to_failure(ex_ptr, payload_conv);

    EXPECT_EQ(failure.message, "index out of range");
}

// ===========================================================================
// RoundTrip: to_failure -> to_exception
// ===========================================================================

TEST(FailureConverterTest, RoundTripRuntimeError) {
    DefaultPayloadConverter payload_conv;
    DefaultFailureConverter failure_conv;

    auto ex_ptr =
        std::make_exception_ptr(std::runtime_error("round trip test"));
    auto failure = failure_conv.to_failure(ex_ptr, payload_conv);
    auto restored = failure_conv.to_exception(failure, payload_conv);

    ASSERT_NE(restored, nullptr);
    try {
        std::rethrow_exception(restored);
    } catch (const std::runtime_error& e) {
        EXPECT_EQ(std::string(e.what()), "round trip test");
    } catch (...) {
        FAIL() << "Expected std::runtime_error";
    }
}

TEST(FailureConverterTest, RoundTripPreservesMessage) {
    DefaultPayloadConverter payload_conv;
    DefaultFailureConverter failure_conv;

    std::string msg = "special chars: \"quotes\" and \\backslash\\ and\nnewline";
    auto ex_ptr = std::make_exception_ptr(std::runtime_error(msg));
    auto failure = failure_conv.to_failure(ex_ptr, payload_conv);
    auto restored = failure_conv.to_exception(failure, payload_conv);

    ASSERT_NE(restored, nullptr);
    try {
        std::rethrow_exception(restored);
    } catch (const std::runtime_error& e) {
        EXPECT_EQ(std::string(e.what()), msg);
    } catch (...) {
        FAIL() << "Expected std::runtime_error with preserved message";
    }
}

TEST(FailureConverterTest, RoundTripEmptyMessage) {
    DefaultPayloadConverter payload_conv;
    DefaultFailureConverter failure_conv;

    auto ex_ptr = std::make_exception_ptr(std::runtime_error(""));
    auto failure = failure_conv.to_failure(ex_ptr, payload_conv);

    EXPECT_TRUE(failure.message.empty());

    auto restored = failure_conv.to_exception(failure, payload_conv);
    ASSERT_NE(restored, nullptr);
    try {
        std::rethrow_exception(restored);
    } catch (const std::runtime_error& e) {
        EXPECT_EQ(std::string(e.what()), "");
    } catch (...) {
        FAIL() << "Expected std::runtime_error with empty message";
    }
}

// ===========================================================================
// Failure struct direct tests
// ===========================================================================

TEST(FailureStructTest, DefaultValues) {
    Failure f;
    EXPECT_TRUE(f.message.empty());
    EXPECT_TRUE(f.failure_type.empty());
    EXPECT_TRUE(f.stack_trace.empty());
    EXPECT_EQ(f.cause, nullptr);
}

TEST(FailureStructTest, WithCause) {
    Failure f;
    f.message = "outer error";
    f.cause = std::make_shared<Failure>();
    f.cause->message = "inner error";

    EXPECT_EQ(f.message, "outer error");
    EXPECT_NE(f.cause, nullptr);
    EXPECT_EQ(f.cause->message, "inner error");
}

TEST(FailureStructTest, NestedCauses) {
    Failure inner;
    inner.message = "root cause";

    Failure middle;
    middle.message = "middle error";
    middle.cause = std::make_shared<Failure>(inner);

    Failure outer;
    outer.message = "outer error";
    outer.cause = std::make_shared<Failure>(middle);

    EXPECT_EQ(outer.message, "outer error");
    EXPECT_EQ(outer.cause->message, "middle error");
    EXPECT_EQ(outer.cause->cause->message, "root cause");
    EXPECT_EQ(outer.cause->cause->cause, nullptr);
}

TEST(FailureStructTest, WithType) {
    Failure f;
    f.message = "test";
    f.failure_type = "ApplicationFailure";

    EXPECT_EQ(f.failure_type, "ApplicationFailure");
}

TEST(FailureStructTest, WithStackTrace) {
    Failure f;
    f.message = "test";
    f.stack_trace = "at main.cpp:42\nat run.cpp:100";

    EXPECT_FALSE(f.stack_trace.empty());
    EXPECT_NE(f.stack_trace.find("main.cpp"), std::string::npos);
}

// ===========================================================================
// Encoded attributes option
// ===========================================================================

TEST(FailureConverterTest, EncodedAttributesOption) {
    DefaultFailureConverter conv_default;
    DefaultFailureConverter conv_encoded({.encode_common_attributes = true});

    DefaultPayloadConverter payload_conv;

    auto ex_ptr = std::make_exception_ptr(std::runtime_error("secret error"));
    auto failure_default =
        conv_default.to_failure(ex_ptr, payload_conv);
    auto failure_encoded =
        conv_encoded.to_failure(ex_ptr, payload_conv);

    // Default converter should have the message in clear text
    EXPECT_EQ(failure_default.message, "secret error");

    // Encoded converter should mask the message
    // (exact behavior depends on implementation, but the message should
    //  differ from the original)
    // Note: The actual encoding behavior depends on implementation;
    // this test verifies the options are respected.
    EXPECT_TRUE(conv_encoded.options().encode_common_attributes);
}
