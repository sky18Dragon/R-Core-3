cmake_minimum_required(VERSION 3.16)

# idf.py passes its own interpreter as -DPYTHON=...; fall back to python3 for
# direct cmake invocations. On Windows a bare "python3" resolves to the
# Microsoft Store stub, so the helper scripts below would silently not run.
if(NOT PYTHON)
    set(PYTHON python3)
endif()

# Get version from centralized script
execute_process(
    COMMAND ${PYTHON} ${CMAKE_SOURCE_DIR}/scripts/get_version.py --type auto
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE PROJECT_VER
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Fallback if script fails
if(NOT PROJECT_VER)
    set(PROJECT_VER "dev-unknown")
endif()

message(STATUS "Project version: ${PROJECT_VER}")

# Allow FIRMWARE_VERSION to be overridden from command line, otherwise use PROJECT_VER
if(NOT DEFINED FIRMWARE_VERSION)
    set(FIRMWARE_VERSION "${PROJECT_VER}")
endif()

message(STATUS "Firmware version: ${FIRMWARE_VERSION}")

add_compile_options(-Wno-missing-field-initializers)
add_compile_definitions(FIRMWARE_VERSION="${FIRMWARE_VERSION}")

# Generate partitions.csv by reading config from sdkconfig / defaults files.
set(_config_files "")
if(DEFINED SDKCONFIG_DEFAULTS)
    foreach(_f ${SDKCONFIG_DEFAULTS})
        list(APPEND _config_files ${CMAKE_SOURCE_DIR}/${_f})
    endforeach()
else()
    list(APPEND _config_files ${CMAKE_SOURCE_DIR}/sdkconfig.defaults)
endif()
if(DEFINED SDKCONFIG AND EXISTS "${SDKCONFIG}")
    list(APPEND _config_files "${SDKCONFIG}")
elseif(NOT DEFINED SDKCONFIG AND EXISTS "${CMAKE_SOURCE_DIR}/sdkconfig")
    list(APPEND _config_files "${CMAKE_SOURCE_DIR}/sdkconfig")
endif()

# The factory diagnostic uses a static, separately reviewed table. Do not regenerate
# the shared OTA table for this profile or infer a migration from flash size alone.
if(NOT "${SDKCONFIG_DEFAULTS}" MATCHES "sdkconfig.defaults.read_pico_factory_diagnostic")
    execute_process(
        COMMAND ${PYTHON} ${CMAKE_SOURCE_DIR}/scripts/generate_partitions.py
            --sdkconfig ${_config_files}
            --output ${CMAKE_SOURCE_DIR}/partitions.csv
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

# Select only the manufacturer's display dependencies, never its Demo/BSP aggregate.
# CONFIG_* is not available until after IDF's early component requirement expansion.
if("${SDKCONFIG_DEFAULTS}" MATCHES "sdkconfig.defaults.read_pico" OR
   "${SDKCONFIG}" MATCHES "sdkconfig.read_pico")
    if("${SDKCONFIG_DEFAULTS}" MATCHES "sdkconfig.defaults.read_pico_display_validation")
        # 显示诊断只覆盖扫描传输几何；固定子模块保持原样。
        # Display diagnostics override scan transport geometry only; the pinned submodule stays clean.
        list(APPEND EXTRA_COMPONENT_DIRS
            "${CMAKE_SOURCE_DIR}/overlays/read_pico_epdiy/epdiy")
    elseif("${SDKCONFIG_DEFAULTS}" MATCHES "sdkconfig.defaults.read_pico_photo_validation")
        # The single-photo diagnostic uses the same locked 684-line geometry.
        list(APPEND EXTRA_COMPONENT_DIRS
            "${CMAKE_SOURCE_DIR}/overlays/read_pico_epdiy/epdiy")
    elseif("${SDKCONFIG_DEFAULTS}" MATCHES "sdkconfig.defaults.read_pico_photoframe_integration")
        # Same verified transport geometry for the full-stack integration build.
        list(APPEND EXTRA_COMPONENT_DIRS
            "${CMAKE_SOURCE_DIR}/overlays/read_pico_epdiy/epdiy")
    else()
        list(APPEND EXTRA_COMPONENT_DIRS
            "${CMAKE_SOURCE_DIR}/third_party/read_pico_firmware/components/epdiy")
    endif()
    list(APPEND EXTRA_COMPONENT_DIRS
        "${CMAKE_SOURCE_DIR}/third_party/read_pico_firmware/components/e0470_epaper_waveform"
        "${CMAKE_SOURCE_DIR}/third_party/read_pico_firmware/components/sy7636a"
        "${CMAKE_SOURCE_DIR}/third_party/read_pico_firmware/components/cst836u")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-photoframe)

# Outer adaptation only: keep the pinned SDK source unchanged. The numeric
# Read Pico SNTP client requires sender/port and originate timestamp checks.
if(CONFIG_BOARD_DRIVER_READ_PICO AND CONFIG_READ_PICO_PHOTOFRAME_INTEGRATION)
    idf_component_get_property(read_pico_lwip_lib lwip COMPONENT_LIB)
    target_compile_definitions(${read_pico_lwip_lib} PRIVATE SNTP_CHECK_RESPONSE=2 SNTP_COMP_ROUNDTRIP=0)
endif()
