# examples/CMakeLists.txt - Example programs

# ── Hello World ─────────────────────────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hello_world/main.cpp")
    add_executable(example_hello_world hello_world/main.cpp)
    target_link_libraries(example_hello_world PRIVATE temporalio)
    temporalio_set_compiler_warnings(example_hello_world)
endif()

# ── Signal Workflow ─────────────────────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/signal_workflow/main.cpp")
    add_executable(example_signal_workflow signal_workflow/main.cpp)
    target_link_libraries(example_signal_workflow PRIVATE temporalio)
    temporalio_set_compiler_warnings(example_signal_workflow)
endif()

# ── Activity Worker ─────────────────────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/activity_worker/main.cpp")
    add_executable(example_activity_worker activity_worker/main.cpp)
    target_link_libraries(example_activity_worker PRIVATE temporalio)
    temporalio_set_compiler_warnings(example_activity_worker)
endif()

# ── Workflow Activity ──────────────────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/workflow_activity/main.cpp")
    add_executable(example_workflow_activity workflow_activity/main.cpp)
    target_link_libraries(example_workflow_activity PRIVATE temporalio)
    temporalio_set_compiler_warnings(example_workflow_activity)
endif()

# ── Timer Workflow ─────────────────────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/timer_workflow/main.cpp")
    add_executable(example_timer_workflow timer_workflow/main.cpp)
    target_link_libraries(example_timer_workflow PRIVATE temporalio)
    temporalio_set_compiler_warnings(example_timer_workflow)
endif()

# ── Update Workflow ────────────────────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/update_workflow/main.cpp")
    add_executable(example_update_workflow update_workflow/main.cpp)
    target_link_libraries(example_update_workflow PRIVATE temporalio)
    temporalio_set_compiler_warnings(example_update_workflow)
endif()
