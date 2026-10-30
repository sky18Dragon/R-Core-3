# 固定 EPDiy 的诊断覆盖：只生成两个传输几何源文件，其余源码直接取自子模块。
# Diagnostic overlay for pinned EPDiy: generate two transport sources and compile the rest in place.
get_filename_component(READ_PICO_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(EPDIY_UPSTREAM "${READ_PICO_PROJECT_ROOT}/third_party/read_pico_firmware/components/epdiy")
set(EPDIY_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(EPDIY_GENERATOR "${READ_PICO_PROJECT_ROOT}/scripts/generate_read_pico_epdiy_overlay.py")

file(MAKE_DIRECTORY "${EPDIY_GENERATED}")
execute_process(
    COMMAND "${PYTHON}" "${EPDIY_GENERATOR}"
        --source-root "${EPDIY_UPSTREAM}"
        --output-dir "${EPDIY_GENERATED}"
    RESULT_VARIABLE EPDIY_GENERATE_RESULT
    OUTPUT_VARIABLE EPDIY_GENERATE_OUTPUT
    ERROR_VARIABLE EPDIY_GENERATE_ERROR
)
if(NOT EPDIY_GENERATE_RESULT EQUAL 0)
    message(FATAL_ERROR
        "Read Pico EPDiy overlay generation failed:\n${EPDIY_GENERATE_OUTPUT}${EPDIY_GENERATE_ERROR}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${EPDIY_GENERATOR}"
    "${EPDIY_UPSTREAM}/src/render.c"
    "${EPDIY_UPSTREAM}/src/output_lcd/lcd_driver.c"
)

set(app_sources
    "${EPDIY_UPSTREAM}/src/epdiy.c"
    "${EPDIY_GENERATED}/render.c"
    "${EPDIY_UPSTREAM}/src/output_lcd/render_lcd.c"
    "${EPDIY_GENERATED}/lcd_driver.c"
    "${EPDIY_UPSTREAM}/src/output_common/rmt_compat.c"
    "${EPDIY_UPSTREAM}/src/output_common/lut.c"
    "${EPDIY_UPSTREAM}/src/output_common/lut.S"
    "${EPDIY_UPSTREAM}/src/output_common/line_queue.c"
    "${EPDIY_UPSTREAM}/src/output_common/render_context.c"
    "${EPDIY_UPSTREAM}/src/font.c"
    "${EPDIY_UPSTREAM}/src/displays.c"
    "${EPDIY_UPSTREAM}/src/diff.S"
    "${EPDIY_UPSTREAM}/src/highlevel.c"
    "${EPDIY_UPSTREAM}/src/board/epd_board.c"
)

if(${IDF_VERSION_MAJOR} LESS 5)
    message(FATAL_ERROR "epdiy requires ESP-IDF 5.x or newer")
endif()

set(epdiy_requires driver esp_timer esp_adc esp_lcd)
if(${IDF_VERSION_MAJOR} GREATER 5 OR
   (${IDF_VERSION_MAJOR} EQUAL 5 AND ${IDF_VERSION_MINOR} GREATER 2))
    list(APPEND epdiy_requires esp_driver_i2c)
endif()
if(DEFINED IDF_VERSION_MAJOR AND IDF_VERSION_MAJOR GREATER_EQUAL 6)
    list(APPEND epdiy_requires esp_driver_gpio esp_driver_rmt esp_driver_dma)
endif()

set(epdiy_private_requires hal soc esp_hw_support esp_rom)
if(DEFINED IDF_VERSION_MAJOR AND IDF_VERSION_MAJOR GREATER_EQUAL 6)
    list(APPEND epdiy_private_requires esp_hal_dma esp_hal_rmt esp_hal_lcd)
endif()

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "${EPDIY_UPSTREAM}/src" "${EPDIY_UPSTREAM}/include"
    PRIV_INCLUDE_DIRS "${EPDIY_UPSTREAM}/src/output_lcd"
    REQUIRES ${epdiy_requires}
    PRIV_REQUIRES ${epdiy_private_requires}
)

# 与固定组件保持相同的编译边界。/ Preserve the pinned component's compile boundaries.
component_compile_options(-Wno-error=format= -Wno-format)
set_source_files_properties(
    "${EPDIY_UPSTREAM}/src/output_common/lut.c"
    PROPERTIES COMPILE_OPTIONS -mno-fix-esp32-psram-cache-issue
)
