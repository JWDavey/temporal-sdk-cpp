# Diagnostics metrics extension for the Temporal C++ SDK.
#
# Provides CustomMetricMeter for recording Temporal SDK metrics
# to a user-provided metrics backend.

add_library(temporalio_diagnostics
    src/custom_metric_meter.cpp
)

target_include_directories(temporalio_diagnostics
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(temporalio_diagnostics
    PUBLIC
        temporalio
)

target_compile_features(temporalio_diagnostics PUBLIC cxx_std_20)
