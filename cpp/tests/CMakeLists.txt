# tests/CMakeLists.txt - Google Test targets for Temporal C++ SDK

find_package(GTest CONFIG REQUIRED)

include(GoogleTest)

# ── Collect test source files ───────────────────────────────────────────────
file(GLOB_RECURSE TEMPORALIO_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

if(NOT TEMPORALIO_TEST_SOURCES)
    message(STATUS "Temporalio: No test sources found yet, creating placeholder target")
    # Create a minimal test main so the target always exists
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test_main_placeholder.cpp"
        "#include <gtest/gtest.h>\n"
        "// Placeholder - real tests will be added as source files appear\n"
    )
    set(TEMPORALIO_TEST_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/test_main_placeholder.cpp")
endif()

# ── Test executable ─────────────────────────────────────────────────────────
add_executable(temporalio_tests ${TEMPORALIO_TEST_SOURCES})

target_link_libraries(temporalio_tests
    PRIVATE
        temporalio
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

# Link extension libraries if they are being built, so tests can
# include extension headers.
if(TARGET temporalio_diagnostics)
    target_link_libraries(temporalio_tests PRIVATE temporalio_diagnostics)
endif()
if(TARGET temporalio_opentelemetry)
    target_link_libraries(temporalio_tests PRIVATE temporalio_opentelemetry)
endif()

target_include_directories(temporalio_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures
        # Bridge headers live under src/ (PRIVATE to the main lib),
        # but tests may need direct access for unit-testing bridge code.
        ${CMAKE_SOURCE_DIR}/src
)

temporalio_set_compiler_warnings(temporalio_tests)

# ── Register tests with CTest ──────────────────────────────────────────────
gtest_discover_tests(temporalio_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTIES
        TIMEOUT 120
)
