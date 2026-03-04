# OpenTelemetry tracing extension for the Temporal C++ SDK.
#
# Provides TracingInterceptor for automatic span creation and context
# propagation across workflow/activity/Nexus boundaries.

find_package(opentelemetry-cpp CONFIG QUIET)
if(NOT opentelemetry-cpp_FOUND)
    message(STATUS "Temporalio: opentelemetry-cpp not found - skipping OpenTelemetry extension")
    return()
endif()

add_library(temporalio_opentelemetry
    src/tracing_interceptor.cpp
)

target_include_directories(temporalio_opentelemetry
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(temporalio_opentelemetry
    PUBLIC
        temporalio
        opentelemetry-cpp::api
        opentelemetry-cpp::sdk
)

target_compile_features(temporalio_opentelemetry PUBLIC cxx_std_20)
