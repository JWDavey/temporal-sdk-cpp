#include <gtest/gtest.h>

#include <any>
#include <string>
#include <vector>

#include "temporalio/activities/activity.h"
#include "temporalio/async_/task.h"

using namespace temporalio::activities;
using namespace temporalio::async_;

// ===========================================================================
// Sample activities for testing
// ===========================================================================
namespace {

Task<std::string> greet(std::string name) {
    co_return "Hello, " + name;
}

Task<int> add_one(int x) { co_return x + 1; }

Task<void> noop() { co_return; }

class MyService {
public:
    Task<std::string> process(std::string input) {
        co_return "Processed: " + input;
    }

    Task<int> compute() { co_return 100; }
};

}  // namespace

// ===========================================================================
// ActivityDefinition from free function
// ===========================================================================

TEST(ActivityDefinitionTest, CreateFromFreeFunction) {
    auto def = ActivityDefinition::create("greet", &greet);
    EXPECT_EQ(def->name(), "greet");
    EXPECT_FALSE(def->is_dynamic());
}

TEST(ActivityDefinitionTest, CreateFromFreeFunctionNoArg) {
    auto def = ActivityDefinition::create("noop", &noop);
    EXPECT_EQ(def->name(), "noop");
}

// ===========================================================================
// ActivityDefinition from lambda
// ===========================================================================

TEST(ActivityDefinitionTest, CreateFromLambda) {
    auto def = ActivityDefinition::create(
        "double_it", []() -> Task<int> { co_return 42; });
    EXPECT_EQ(def->name(), "double_it");
}

// ===========================================================================
// ActivityDefinition from member function
// ===========================================================================

TEST(ActivityDefinitionTest, CreateFromMemberFunction) {
    MyService service;
    auto def =
        ActivityDefinition::create("process", &MyService::process, &service);
    EXPECT_EQ(def->name(), "process");
}

TEST(ActivityDefinitionTest, CreateFromMemberFunctionNoArg) {
    MyService service;
    auto def =
        ActivityDefinition::create("compute", &MyService::compute, &service);
    EXPECT_EQ(def->name(), "compute");
}

// ===========================================================================
// Dynamic activity
// ===========================================================================

TEST(ActivityDefinitionTest, DynamicActivity) {
    auto def = ActivityDefinition::create("", &noop);
    EXPECT_TRUE(def->is_dynamic());
    EXPECT_TRUE(def->name().empty());
}

// ===========================================================================
// ActivityInfo tests
// ===========================================================================

TEST(ActivityInfoTest, DefaultValues) {
    ActivityInfo info;
    EXPECT_TRUE(info.activity_id.empty());
    EXPECT_TRUE(info.activity_type.empty());
    EXPECT_EQ(info.attempt, 1);
    EXPECT_FALSE(info.is_local);
    EXPECT_FALSE(info.heartbeat_timeout.has_value());
    EXPECT_FALSE(info.schedule_to_close_timeout.has_value());
    EXPECT_FALSE(info.start_to_close_timeout.has_value());
    EXPECT_FALSE(info.workflow_id.has_value());
    EXPECT_FALSE(info.is_workflow_activity());
}

TEST(ActivityInfoTest, IsWorkflowActivity) {
    ActivityInfo info;
    info.workflow_id = "wf-123";
    EXPECT_TRUE(info.is_workflow_activity());
}

TEST(ActivityInfoTest, CustomValues) {
    ActivityInfo info;
    info.activity_id = "act-1";
    info.activity_type = "MyActivity";
    info.attempt = 3;
    info.namespace_ = "production";
    info.task_queue = "my-queue";
    info.is_local = true;

    EXPECT_EQ(info.activity_id, "act-1");
    EXPECT_EQ(info.activity_type, "MyActivity");
    EXPECT_EQ(info.attempt, 3);
    EXPECT_EQ(info.namespace_, "production");
    EXPECT_EQ(info.task_queue, "my-queue");
    EXPECT_TRUE(info.is_local);
}
