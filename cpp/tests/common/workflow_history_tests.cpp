#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "temporalio/common/workflow_history.h"

using namespace temporalio::common;

// ===========================================================================
// WorkflowHistory construction tests
// ===========================================================================

TEST(WorkflowHistoryTest, ConstructWithIdAndHistory) {
    WorkflowHistory history("wf-123", "serialized-data");
    EXPECT_EQ(history.id(), "wf-123");
    EXPECT_EQ(history.serialized_history(), "serialized-data");
}

TEST(WorkflowHistoryTest, ConstructWithEmptyId) {
    WorkflowHistory history("", "data");
    EXPECT_TRUE(history.id().empty());
    EXPECT_EQ(history.serialized_history(), "data");
}

TEST(WorkflowHistoryTest, ConstructWithEmptyHistory) {
    WorkflowHistory history("wf-1", "");
    EXPECT_EQ(history.id(), "wf-1");
    EXPECT_TRUE(history.serialized_history().empty());
}

// ===========================================================================
// WorkflowHistory::from_json tests
// ===========================================================================

TEST(WorkflowHistoryTest, FromJsonValidInput) {
    auto history = WorkflowHistory::from_json("wf-abc", "{\"events\": []}");
    EXPECT_EQ(history.id(), "wf-abc");
    EXPECT_EQ(history.serialized_history(), "{\"events\": []}");
}

TEST(WorkflowHistoryTest, FromJsonEmptyStringThrows) {
    EXPECT_THROW(WorkflowHistory::from_json("wf-1", ""),
                 std::invalid_argument);
}

TEST(WorkflowHistoryTest, FromJsonEmptyWorkflowId) {
    auto history = WorkflowHistory::from_json("", "{\"events\": []}");
    EXPECT_TRUE(history.id().empty());
}

// ===========================================================================
// WorkflowHistory::to_json tests
// ===========================================================================

TEST(WorkflowHistoryTest, ToJsonRoundTrip) {
    std::string json = "{\"events\": [{\"eventId\": 1}]}";
    auto history = WorkflowHistory::from_json("wf-rt", json);
    EXPECT_EQ(history.to_json(), json);
}

TEST(WorkflowHistoryTest, ToJsonFromConstructor) {
    WorkflowHistory history("wf-1", "raw-data");
    EXPECT_EQ(history.to_json(), "raw-data");
}

// ===========================================================================
// WorkflowHistory copy semantics
// ===========================================================================

TEST(WorkflowHistoryTest, CopyConstruction) {
    WorkflowHistory original("wf-1", "data-1");
    WorkflowHistory copy(original);
    EXPECT_EQ(copy.id(), "wf-1");
    EXPECT_EQ(copy.serialized_history(), "data-1");
}

TEST(WorkflowHistoryTest, MoveConstruction) {
    WorkflowHistory original("wf-1", "data-1");
    WorkflowHistory moved(std::move(original));
    EXPECT_EQ(moved.id(), "wf-1");
    EXPECT_EQ(moved.serialized_history(), "data-1");
}
