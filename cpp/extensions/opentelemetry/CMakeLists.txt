# OpenTelemetry tracing extension for the Temporal C++ SDK.
#
# Provides TracingInterceptor for automatic span creation and context
# propagation across workflow/activity/Nexus boundaries.

find_package(opentelemetry-cpp CONFIG REQUIRED)

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
