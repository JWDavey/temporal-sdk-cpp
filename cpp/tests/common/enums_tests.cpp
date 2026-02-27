#include <temporalio/common/enums.h>

#include <gtest/gtest.h>

namespace temporalio::common {
namespace {

TEST(EnumsTest, VersioningBehaviorValues) {
    EXPECT_EQ(static_cast<int>(VersioningBehavior::kUnspecified), 0);
    EXPECT_EQ(static_cast<int>(VersioningBehavior::kPinned), 1);
    EXPECT_EQ(static_cast<int>(VersioningBehavior::kAutoUpgrade), 2);
}

TEST(EnumsTest, ParentClosePolicyValues) {
    EXPECT_EQ(static_cast<int>(ParentClosePolicy::kUnspecified), 0);
    EXPECT_EQ(static_cast<int>(ParentClosePolicy::kTerminate), 1);
    EXPECT_EQ(static_cast<int>(ParentClosePolicy::kAbandon), 2);
    EXPECT_EQ(static_cast<int>(ParentClosePolicy::kRequestCancel), 3);
}

TEST(EnumsTest, WorkflowIdConflictPolicyValues) {
    EXPECT_EQ(static_cast<int>(WorkflowIdConflictPolicy::kUnspecified), 0);
    EXPECT_EQ(static_cast<int>(WorkflowIdConflictPolicy::kFail), 1);
    EXPECT_EQ(static_cast<int>(WorkflowIdConflictPolicy::kUseExisting), 2);
    EXPECT_EQ(static_cast<int>(WorkflowIdConflictPolicy::kTerminateExisting),
              3);
}

TEST(EnumsTest, WorkflowIdReusePolicyValues) {
    EXPECT_EQ(static_cast<int>(WorkflowIdReusePolicy::kUnspecified), 0);
    EXPECT_EQ(static_cast<int>(WorkflowIdReusePolicy::kAllowDuplicate), 1);
    EXPECT_EQ(
        static_cast<int>(WorkflowIdReusePolicy::kAllowDuplicateFailedOnly),
        2);
    EXPECT_EQ(static_cast<int>(WorkflowIdReusePolicy::kRejectDuplicate), 3);
    EXPECT_EQ(static_cast<int>(WorkflowIdReusePolicy::kTerminateIfRunning),
              4);
}

TEST(EnumsTest, TaskReachabilityValues) {
    EXPECT_EQ(static_cast<int>(TaskReachability::kUnspecified), 0);
    EXPECT_EQ(static_cast<int>(TaskReachability::kNewWorkflows), 1);
    EXPECT_EQ(static_cast<int>(TaskReachability::kExistingWorkflows), 2);
    EXPECT_EQ(static_cast<int>(TaskReachability::kOpenWorkflows), 3);
    EXPECT_EQ(static_cast<int>(TaskReachability::kClosedWorkflows), 4);
}

TEST(PriorityTest, DefaultValues) {
    Priority p;
    EXPECT_FALSE(p.priority_key.has_value());
    EXPECT_FALSE(p.fairness_key.has_value());
    EXPECT_FALSE(p.fairness_weight.has_value());
}

TEST(PriorityTest, Equality) {
    Priority a;
    Priority b;
    EXPECT_EQ(a, b);

    a.priority_key = 1;
    EXPECT_NE(a, b);

    b.priority_key = 1;
    EXPECT_EQ(a, b);

    a.fairness_key = "queue-a";
    a.fairness_weight = 1.5f;
    EXPECT_NE(a, b);

    b.fairness_key = "queue-a";
    b.fairness_weight = 1.5f;
    EXPECT_EQ(a, b);
}

} // namespace
} // namespace temporalio::common
