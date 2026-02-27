cmake_minimum_required(VERSION 3.20)
project(temporalio VERSION 0.1.0 LANGUAGES CXX)

# ── C++20 standard ──────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Options ─────────────────────────────────────────────────────────────────
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(TEMPORALIO_BUILD_EXTENSIONS "Build OpenTelemetry and Diagnostics extensions" ON)
option(TEMPORALIO_BUILD_TESTS "Build test suite" ON)
option(TEMPORALIO_BUILD_EXAMPLES "Build example programs" ON)

# ── CMake modules ───────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Platform)
include(CompilerWarnings)

# ── Dependencies (vcpkg manifest mode) ──────────────────────────────────────
find_package(Protobuf CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# ── Rust bridge custom build (must be before library target) ────────────────
temporalio_build_rust_bridge(
    TARGET temporalio_rust_bridge
    CARGO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/Temporalio/Bridge/sdk-core"
    CRATE_NAME temporal_sdk_core_c_bridge
)

# ── Collect C++ source files ────────────────────────────────────────────────
# All C++ code lives under cpp/ (this directory). Source files are under src/.
# NOTE: GLOB_RECURSE is convenient during early development but fragile -- new
# files won't trigger a reconfigure. Switch to explicit source lists before release.
file(GLOB_RECURSE TEMPORALIO_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/temporalio/*.cpp"
)

file(GLOB_RECURSE TEMPORALIO_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/temporalio/*.h"
)

# Guard: if no .cpp files exist yet, generate a placeholder so the target is
# always valid even in a fresh checkout.
if(NOT TEMPORALIO_SOURCES)
    message(STATUS "Temporalio: No .cpp source files found — creating placeholder library")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/temporalio_placeholder.cpp"
        "// Auto-generated placeholder — remove once real sources exist\n")
    set(TEMPORALIO_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/temporalio_placeholder.cpp")
endif()

# ── Main library target ────────────────────────────────────────────────────
add_library(temporalio ${TEMPORALIO_SOURCES})

target_include_directories(temporalio
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(temporalio
    PUBLIC
        protobuf::libprotobuf
        nlohmann_json::nlohmann_json
    PRIVATE
        temporalio_rust_bridge
)

target_compile_features(temporalio PUBLIC cxx_std_20)

temporalio_set_compiler_warnings(temporalio)

# ── Extensions ──────────────────────────────────────────────────────────────
if(TEMPORALIO_BUILD_EXTENSIONS)
    add_subdirectory(extensions/opentelemetry)
    add_subdirectory(extensions/diagnostics)
endif()

# ── Tests ───────────────────────────────────────────────────────────────────
if(TEMPORALIO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Examples ────────────────────────────────────────────────────────────────
if(TEMPORALIO_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ── Install ─────────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS temporalio
    EXPORT temporalioTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/temporalio
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT temporalioTargets
    FILE temporalioTargets.cmake
    NAMESPACE temporalio::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/temporalio
)
