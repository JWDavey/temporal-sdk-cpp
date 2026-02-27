cmake_minimum_required(VERSION 3.20)
project(temporalio VERSION 0.1.0 LANGUAGES CXX)

# ── C++20 standard ──────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC: enable /FS (force synchronous PDB writes) to prevent C1041 errors
# when parallel builds compile multiple source files into the same PDB.
# This is needed for FetchContent dependencies (abseil, protobuf) and our code.
if(MSVC)
    add_compile_options(/FS)
endif()

# ── Options ─────────────────────────────────────────────────────────────────
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(TEMPORALIO_BUILD_EXTENSIONS "Build OpenTelemetry and Diagnostics extensions" ON)
option(TEMPORALIO_BUILD_TESTS "Build test suite" ON)
option(TEMPORALIO_BUILD_EXAMPLES "Build example programs" ON)
option(TEMPORALIO_BUILD_PROTOS "Generate and build Temporal API protobuf types" ON)

# ── CMake modules ───────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Platform)
include(CompilerWarnings)

# ── Dependencies ───────────────────────────────────────────────────────────
# Try vcpkg / system packages first, fall back to FetchContent.
include(FetchContent)

find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
    find_package(Protobuf QUIET)
endif()
if(NOT Protobuf_FOUND)
    message(STATUS "Temporalio: Protobuf not found via find_package, using FetchContent")
    set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
    set(protobuf_MSVC_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
    set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
    set(ABSL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(ABSL_MSVC_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
    # Fetch abseil-cpp first (protobuf depends on it)
    FetchContent_Declare(
        abseil-cpp
        URL https://github.com/abseil/abseil-cpp/releases/download/20240722.0/abseil-cpp-20240722.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(abseil-cpp)
    set(protobuf_ABSL_PROVIDER "package" CACHE STRING "" FORCE)
    set(utf8_range_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        protobuf
        URL https://github.com/protocolbuffers/protobuf/releases/download/v29.3/protobuf-29.3.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(protobuf)
else()
    message(STATUS "Temporalio: Found Protobuf via find_package")
endif()

find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "Temporalio: nlohmann_json not found via find_package, using FetchContent")
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
else()
    message(STATUS "Temporalio: Found nlohmann_json via find_package")
endif()

# Determine protobuf target name (differs between find_package and FetchContent).
# This must be set before ProtobufGenerate.cmake is included, since the proto
# library target links against it.
if(TARGET protobuf::libprotobuf)
    set(TEMPORALIO_PROTOBUF_TARGET protobuf::libprotobuf)
elseif(TARGET protobuf::libprotobuf-lite)
    set(TEMPORALIO_PROTOBUF_TARGET protobuf::libprotobuf-lite)
else()
    set(TEMPORALIO_PROTOBUF_TARGET protobuf::libprotobuf)
endif()

# ── Rust bridge custom build (must be before library target) ────────────────
temporalio_build_rust_bridge(
    TARGET temporalio_rust_bridge
    CARGO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/Temporalio/Bridge/sdk-core"
    CRATE_NAME temporal_sdk_core_c_bridge
)

# ── Protobuf code generation ──────────────────────────────────────────────
# Generate C++ types from Temporal API .proto files. The generated .pb.h/.pb.cc
# files are placed in ${CMAKE_CURRENT_BINARY_DIR}/proto_gen/ and compiled into
# the temporalio_proto static library.
set(TEMPORALIO_HAS_PROTOS FALSE)
if(TEMPORALIO_BUILD_PROTOS)
    # ProtobufGenerate must be included AFTER Protobuf is found (needs protoc)
    include(ProtobufGenerate)
    set(TEMPORALIO_PROTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../src/Temporalio/Bridge/sdk-core/crates/common/protos")
    if(EXISTS "${TEMPORALIO_PROTO_ROOT}")
        temporalio_generate_protos(
            TARGET temporalio_proto
            PROTO_ROOT "${TEMPORALIO_PROTO_ROOT}"
        )
        set(TEMPORALIO_HAS_PROTOS TRUE)
    else()
        message(WARNING "Temporalio: Proto root not found at ${TEMPORALIO_PROTO_ROOT} - "
            "run 'git submodule update --init --recursive' to fetch proto definitions")
    endif()
endif()

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
        ${TEMPORALIO_PROTOBUF_TARGET}
        nlohmann_json::nlohmann_json
    PRIVATE
        temporalio_rust_bridge
)

# Link generated proto types if available
if(TEMPORALIO_HAS_PROTOS)
    target_link_libraries(temporalio PUBLIC temporalio_proto)
endif()

target_compile_features(temporalio PUBLIC cxx_std_20)

# Define TEMPORALIO_HAS_PROTOBUF so that proto payload converters are compiled in.
# If protobuf is not available, data_converter.cpp omits JsonProtoConverter and
# BinaryProtoConverter from the default converter set.
target_compile_definitions(temporalio PUBLIC TEMPORALIO_HAS_PROTOBUF)

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

# ── Documentation ──────────────────────────────────────────────────────────
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
else()
    message(STATUS "Temporalio: Doxygen not found - 'docs' target will not be available")
endif()

# ── Install & Packaging ─────────────────────────────────────────────────────
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Determine whether CMake export sets are possible. install(EXPORT ...) requires
# every linked dependency target to also be in an export set. When dependencies
# come from FetchContent (not system find_package), they are not exported and
# CMake generation will fail. Only enable EXPORT when deps are system-provided.
set(TEMPORALIO_CAN_EXPORT TRUE)
if(NOT Protobuf_FOUND OR NOT nlohmann_json_FOUND)
    set(TEMPORALIO_CAN_EXPORT FALSE)
endif()

if(TEMPORALIO_CAN_EXPORT)
    install(TARGETS temporalio
        EXPORT temporalioTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
else()
    install(TARGETS temporalio
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Install public headers preserving directory structure.
install(DIRECTORY include/temporalio
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Install extension targets if they were built.
if(TARGET temporalio_diagnostics)
    if(TEMPORALIO_CAN_EXPORT)
        install(TARGETS temporalio_diagnostics
            EXPORT temporalioTargets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    else()
        install(TARGETS temporalio_diagnostics
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
    install(DIRECTORY extensions/diagnostics/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
    )
endif()

if(TARGET temporalio_opentelemetry)
    if(TEMPORALIO_CAN_EXPORT)
        install(TARGETS temporalio_opentelemetry
            EXPORT temporalioTargets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    else()
        install(TARGETS temporalio_opentelemetry
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
    install(DIRECTORY extensions/opentelemetry/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
    )
endif()

# Export targets and CMake package config for downstream find_package().
# Only available when all dependencies were found via find_package (system-installed).
if(TEMPORALIO_CAN_EXPORT)
    install(EXPORT temporalioTargets
        FILE temporalioTargets.cmake
        NAMESPACE temporalio::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/temporalio
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/temporalioConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/temporalioConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/temporalio
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/temporalioConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/temporalioConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/temporalioConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/temporalio
    )
else()
    message(STATUS "Temporalio: CMake export/find_package support disabled "
        "(dependencies come from FetchContent, not system packages)")
endif()

# Generate pkg-config file (always available regardless of export support).
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/temporalio.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/temporalio.pc"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/temporalio.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
