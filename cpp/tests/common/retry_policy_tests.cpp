#include <temporalio/common/retry_policy.h>

#include <gtest/gtest.h>

#include <chrono>

namespace temporalio::common {
namespace {

TEST(RetryPolicyTest, DefaultValues) {
    RetryPolicy policy;
    EXPECT_EQ(policy.initial_interval, std::chrono::milliseconds{1000});
    EXPECT_DOUBLE_EQ(policy.backoff_coefficient, 2.0);
    EXPECT_FALSE(policy.maximum_interval.has_value());
    EXPECT_EQ(policy.maximum_attempts, 0);
    EXPECT_TRUE(policy.non_retryable_error_types.empty());
}

TEST(RetryPolicyTest, CustomValues) {
    RetryPolicy policy;
    policy.initial_interval = std::chrono::milliseconds{500};
    policy.backoff_coefficient = 1.5;
    policy.maximum_interval = std::chrono::milliseconds{30000};
    policy.maximum_attempts = 5;
    policy.non_retryable_error_types = {"NotFound", "InvalidArgument"};

    EXPECT_EQ(policy.initial_interval, std::chrono::milliseconds{500});
    EXPECT_DOUBLE_EQ(policy.backoff_coefficient, 1.5);
    EXPECT_EQ(policy.maximum_interval.value(),
              std::chrono::milliseconds{30000});
    EXPECT_EQ(policy.maximum_attempts, 5);
    EXPECT_EQ(policy.non_retryable_error_types.size(), 2u);
    EXPECT_EQ(policy.non_retryable_error_types[0], "NotFound");
    EXPECT_EQ(policy.non_retryable_error_types[1], "InvalidArgument");
}

TEST(RetryPolicyTest, Equality) {
    RetryPolicy a;
    RetryPolicy b;
    EXPECT_EQ(a, b);

    a.maximum_attempts = 3;
    EXPECT_NE(a, b);

    b.maximum_attempts = 3;
    EXPECT_EQ(a, b);
}

TEST(RetryPolicyTest, EqualityWithOptionalInterval) {
    RetryPolicy a;
    RetryPolicy b;
    a.maximum_interval = std::chrono::milliseconds{5000};
    EXPECT_NE(a, b);

    b.maximum_interval = std::chrono::milliseconds{5000};
    EXPECT_EQ(a, b);
}

} // namespace
} // namespace temporalio::common
