#include <temporalio/common/workflow_history.h>

#include <stdexcept>
#include <string>

#include <temporal/api/history/v1/message.pb.h>
#include <google/protobuf/util/json_util.h>

namespace temporalio::common {

WorkflowHistory WorkflowHistory::from_json(const std::string& workflow_id,
                                           const std::string& json) {
    if (json.empty()) {
        throw std::invalid_argument("JSON history string must not be empty");
    }

    // Parse JSON into History protobuf, ignoring unknown fields so that
    // JSON from newer server versions or CLI/UI exports still works.
    temporal::api::history::v1::History history;
    google::protobuf::util::JsonParseOptions parse_opts;
    parse_opts.ignore_unknown_fields = true;
    auto status = google::protobuf::util::JsonStringToMessage(
        json, &history, parse_opts);
    if (!status.ok()) {
        throw std::invalid_argument(
            "Failed to parse workflow history JSON: " +
            std::string(status.message()));
    }

    // Serialize to binary protobuf for internal storage (the bridge
    // expects binary protobuf when replaying).
    std::string serialized;
    if (!history.SerializeToString(&serialized)) {
        throw std::runtime_error(
            "Failed to serialize workflow history to binary protobuf");
    }

    return WorkflowHistory(workflow_id, std::move(serialized));
}

std::string WorkflowHistory::to_json() const {
    // Deserialize binary protobuf back into History.
    temporal::api::history::v1::History history;
    if (!history.ParseFromString(serialized_history_)) {
        throw std::runtime_error(
            "Failed to parse stored workflow history binary protobuf");
    }

    // Convert to JSON string.
    std::string json;
    google::protobuf::util::JsonPrintOptions print_opts;
    print_opts.preserve_proto_field_names = true;
    auto status = google::protobuf::util::MessageToJsonString(
        history, &json, print_opts);
    if (!status.ok()) {
        throw std::runtime_error(
            "Failed to serialize workflow history to JSON: " +
            std::string(status.message()));
    }

    return json;
}

} // namespace temporalio::common
