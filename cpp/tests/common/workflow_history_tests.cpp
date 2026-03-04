#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "temporalio/common/workflow_history.h"
#include <temporal/api/history/v1/message.pb.h>

using namespace temporalio::common;

// ===========================================================================
// WorkflowHistory construction tests
// ===========================================================================

TEST(WorkflowHistoryTest, ConstructWithIdAndHistory) {
    // Constructor takes raw binary protobuf bytes
    temporal::api::history::v1::History proto;
    std::string serialized;
    proto.SerializeToString(&serialized);

    WorkflowHistory history("wf-123", serialized);
    EXPECT_EQ(history.id(), "wf-123");
    EXPECT_EQ(history.serialized_history(), serialized);
}

TEST(WorkflowHistoryTest, ConstructWithEmptyId) {
    WorkflowHistory history("", "data");
    EXPECT_TRUE(history.id().empty());
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
    // serialized_history() returns binary protobuf. An empty History proto
    // (no events) serializes to an empty binary string in protobuf3.
    // The key thing is that from_json succeeds without throwing.
}

TEST(WorkflowHistoryTest, FromJsonWithEvents) {
    // A history with at least one event should produce non-empty binary.
    std::string json = R"({"events":[{"eventId":"1","eventType":"EVENT_TYPE_WORKFLOW_EXECUTION_STARTED"}]})";
    auto history = WorkflowHistory::from_json("wf-abc", json);
    EXPECT_EQ(history.id(), "wf-abc");
    EXPECT_FALSE(history.serialized_history().empty());
}

TEST(WorkflowHistoryTest, FromJsonEmptyStringThrows) {
    EXPECT_THROW(WorkflowHistory::from_json("wf-1", ""),
                 std::invalid_argument);
}

TEST(WorkflowHistoryTest, FromJsonInvalidJsonThrows) {
    EXPECT_THROW(WorkflowHistory::from_json("wf-1", "not json"),
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
    // from_json parses JSON -> binary protobuf; to_json converts back to JSON.
    // The output JSON may differ in formatting from the input, but should
    // parse back to the same protobuf.
    std::string json = "{\"events\":[]}";
    auto history = WorkflowHistory::from_json("wf-rt", json);
    std::string output = history.to_json();
    // Should contain "events" field (may be omitted for empty repeated fields
    // depending on protobuf JSON options, so just check it's valid JSON).
    EXPECT_FALSE(output.empty());
    // Round-trip: parse the output back and verify it produces the same binary.
    auto history2 = WorkflowHistory::from_json("wf-rt2", output);
    EXPECT_EQ(history.serialized_history(), history2.serialized_history());
}

TEST(WorkflowHistoryTest, ToJsonFromValidBinary) {
    // Construct with valid binary protobuf and convert to JSON.
    temporal::api::history::v1::History proto;
    std::string serialized;
    proto.SerializeToString(&serialized);

    WorkflowHistory history("wf-1", serialized);
    std::string json = history.to_json();
    EXPECT_FALSE(json.empty());
}

TEST(WorkflowHistoryTest, ToJsonFromEmptyBinary) {
    // An empty binary string is valid protobuf3 (empty History with no events).
    // to_json should succeed and produce valid JSON.
    WorkflowHistory history("wf-1", "");
    std::string json = history.to_json();
    EXPECT_FALSE(json.empty());
}

// ===========================================================================
// WorkflowHistory copy semantics
// ===========================================================================

TEST(WorkflowHistoryTest, CopyConstruction) {
    temporal::api::history::v1::History proto;
    std::string serialized;
    proto.SerializeToString(&serialized);

    WorkflowHistory original("wf-1", serialized);
    WorkflowHistory copy(original);
    EXPECT_EQ(copy.id(), "wf-1");
    EXPECT_EQ(copy.serialized_history(), serialized);
}

TEST(WorkflowHistoryTest, MoveConstruction) {
    temporal::api::history::v1::History proto;
    std::string serialized;
    proto.SerializeToString(&serialized);

    WorkflowHistory original("wf-1", serialized);
    WorkflowHistory moved(std::move(original));
    EXPECT_EQ(moved.id(), "wf-1");
    EXPECT_EQ(moved.serialized_history(), serialized);
}
