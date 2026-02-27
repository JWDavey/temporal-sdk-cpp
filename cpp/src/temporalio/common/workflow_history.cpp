#include <temporalio/common/workflow_history.h>

#include <stdexcept>

namespace temporalio::common {

WorkflowHistory WorkflowHistory::from_json(const std::string& workflow_id,
                                           const std::string& json) {
    // TODO: Implement proper protobuf JSON parsing once protobuf types are
    // generated. For now, store the raw JSON as the serialized history.
    if (json.empty()) {
        throw std::invalid_argument("JSON history string must not be empty");
    }
    return WorkflowHistory(workflow_id, json);
}

std::string WorkflowHistory::to_json() const {
    // TODO: Implement proper protobuf JSON serialization once protobuf types
    // are generated. For now, return the stored serialized history.
    return serialized_history_;
}

} // namespace temporalio::common
