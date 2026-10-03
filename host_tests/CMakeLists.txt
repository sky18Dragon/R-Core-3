cmake_minimum_required(VERSION 3.14)
project(esp32_photoframe_tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Cron engine tests
add_executable(
  cron_test
  test_cron.cpp
  ../main/cron.c
)

target_link_libraries(
  cron_test
  GTest::gtest_main
)

target_include_directories(
  cron_test
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../main
)

# Wake-schedule simulation (early-wake re-sleep loop vs. the real cron engine)
add_executable(
  wake_schedule_test
  test_wake_schedule.cpp
  ../main/cron.c
)

target_link_libraries(
  wake_schedule_test
  GTest::gtest_main
)

target_include_directories(
  wake_schedule_test
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../main
)

# Backoff between failed unattended network wakes (#121), against the real cron engine
add_executable(
  network_backoff_test
  test_network_backoff.cpp
  ../main/network_backoff.c
  ../main/cron.c
)

target_link_libraries(
  network_backoff_test
  GTest::gtest_main
)

target_include_directories(
  network_backoff_test
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../main
)

# HTTP Basic credential checking for the optional device API password (#130)
add_executable(
  http_auth_test
  test_http_auth.cpp
  ../main/http_auth.c
)

target_link_libraries(
  http_auth_test
  GTest::gtest_main
)

target_include_directories(
  http_auth_test
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../main
)

# Last-crash record: reason and the one-line summary
add_executable(
  crash_record_test
  test_crash_record.cpp
  ../main/crash_record.c
)

target_link_libraries(
  crash_record_test
  GTest::gtest_main
)

target_include_directories(
  crash_record_test
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../main
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(cron_test)
gtest_discover_tests(wake_schedule_test)
gtest_discover_tests(http_auth_test)
gtest_discover_tests(network_backoff_test)
gtest_discover_tests(crash_record_test)

# On-device image pipeline characterization tests (host build of
# main/image_processor.c against ESP-IDF stubs + system libpng)
find_package(PNG REQUIRED)

add_executable(
  image_pipeline_test
  test_image_pipeline.cpp
  ../main/image_processor.c
  stubs/esp_stubs.c
  stubs/fake_display_manager.c
  stubs/fake_config_manager.c
  stubs/fake_processing_settings.c
)

target_include_directories(
  image_pipeline_test
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs
  ${CMAKE_CURRENT_SOURCE_DIR}/../main
)

target_link_libraries(
  image_pipeline_test
  GTest::gtest_main
  PNG::PNG
  m
)

gtest_discover_tests(image_pipeline_test)

# .current.* / thumbnail lifecycle tests for the shared display flow.
# FS_MOUNT_POINT is redirected to a local directory so the tests exercise
# the real path scheme against the host filesystem.
add_executable(
  display_flow_test
  test_display_flow.cpp
  ../main/display_flow.c
  ../main/image_processor.c
  stubs/esp_stubs.c
  stubs/fake_display_manager.c
  stubs/fake_config_manager.c
  stubs/fake_processing_settings.c
  stubs/fake_storage.c
)

target_compile_definitions(
  display_flow_test
  PRIVATE
  FS_MOUNT_POINT="pf_storage"
)

target_include_directories(
  display_flow_test
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs
  ${CMAKE_CURRENT_SOURCE_DIR}/../main
)

target_link_libraries(
  display_flow_test
  GTest::gtest_main
  PNG::PNG
  m
)

gtest_discover_tests(display_flow_test)

# Read-only PMU codec: no ESP-IDF or board I/O in the portable core.
add_executable(read_pico_pmu_test
  test_read_pico_pmu.cpp
  ../components/board_hal/src/read_pico_pmu_codec.c
)
target_include_directories(read_pico_pmu_test PRIVATE
  ../components/board_hal/include
  ../third_party/read_pico_firmware/components/read_pico_pmu/include
)
target_link_libraries(read_pico_pmu_test GTest::gtest_main)
gtest_discover_tests(read_pico_pmu_test)

add_executable(read_pico_board_test
  test_read_pico_board.cpp
  ../components/board_hal/src/driver_read_pico.c
  ../components/board_hal/src/read_pico_pmu_codec.c
)
target_include_directories(read_pico_board_test PRIVATE
  read_pico_stubs
  stubs
  ../components/board_hal/include
  ../third_party/read_pico_firmware/components/read_pico_pmu/include
)
target_link_libraries(read_pico_board_test GTest::gtest_main)
gtest_discover_tests(read_pico_board_test)

add_executable(read_pico_display_test
  test_read_pico_display.cpp
  ../components/epaper_src/GUI_Paint.c
  ../components/read_pico_display/src/read_pico_frame.c
  ../components/read_pico_display/src/read_pico_refresh.c
)
target_include_directories(read_pico_display_test PRIVATE
  ../components/read_pico_display/include
  ../components/epaper_src
  ../components/epaper_src/Fonts
  stubs
)
target_link_libraries(read_pico_display_test GTest::gtest_main)
gtest_discover_tests(read_pico_display_test)

# The locked vendor source is compiled unchanged for its real fault decoder.
# GPIO/I2C/timing are host fakes, not physical power/timeout validation.
add_executable(read_pico_sy_power_test
  test_read_pico_sy_power.cpp
  ../components/epaper_driver_read_pico/src/read_pico_sy_power.c
  ../third_party/read_pico_firmware/components/sy7636a/sy7636a.c
)
target_include_directories(read_pico_sy_power_test PRIVATE
  read_pico_sy_stubs read_pico_stubs stubs
  ../components/epaper_driver_read_pico/include
  ../components/epaper_driver_read_pico/src
  ../third_party/read_pico_firmware/components/sy7636a/include
)
target_link_libraries(read_pico_sy_power_test GTest::gtest_main)
gtest_discover_tests(read_pico_sy_power_test)

find_package(ZLIB REQUIRED)
add_executable(read_pico_display_commit_test
  test_read_pico_display_commit.cpp
  ../main/display_manager.c
  ../components/epaper_src/GUI_Paint.c
  ../components/epaper_src/GUI_RawBuffer.c
)
target_compile_definitions(read_pico_display_commit_test PRIVATE
  CONFIG_BOARD_DRIVER_READ_PICO=1 FS_MOUNT_POINT="pf_display_checked")
target_include_directories(read_pico_display_commit_test PRIVATE
  display_checked_stubs stubs ../main
  ../components/epaper/include ../components/epaper_src ../components/epaper_src/Fonts
  ../components/epaper_driver_read_pico/include ../components/read_pico_display/include
)
target_link_libraries(read_pico_display_commit_test GTest::gtest_main ZLIB::ZLIB m)
gtest_discover_tests(read_pico_display_commit_test)

# Real outer backend + real SY adapter/core, renderer/HAL/RTOS/heap boundary fakes.
# The separately compiled IDF firmware validates actual vendor ABI/types.
add_executable(read_pico_native_test
  test_read_pico_native.cpp
  ../components/epaper_driver_read_pico/src/driver_read_pico.c
  ../components/epaper_driver_read_pico/src/read_pico_vendor_gray16.c
  ../components/epaper_driver_read_pico/src/read_pico_sy_power.c
  ../components/read_pico_display/src/read_pico_frame.c
  ../components/read_pico_display/src/read_pico_refresh.c
  ../components/read_pico_display/src/read_pico_pattern.c
  ../components/read_pico_display/src/read_pico_validation.c
  ../third_party/read_pico_firmware/components/sy7636a/sy7636a.c
)
target_include_directories(read_pico_native_test PRIVATE
  read_pico_native_stubs read_pico_sy_stubs read_pico_stubs stubs
  ../components/epaper/include ../components/epaper_driver_read_pico/include
  ../components/epaper_driver_read_pico/src
  ../components/read_pico_display/include ../components/board_hal/include
  ../third_party/read_pico_firmware/components/sy7636a/include
)
target_link_libraries(read_pico_native_test GTest::gtest_main)
target_compile_definitions(read_pico_native_test PRIVATE CONFIG_READ_PICO_DISPLAY_VALIDATION=1)
gtest_discover_tests(read_pico_native_test)

add_executable(read_pico_validation_test
  test_read_pico_validation.cpp
  ../components/read_pico_display/src/read_pico_validation.c
)
target_include_directories(read_pico_validation_test PRIVATE
  ../components/read_pico_display/include
)
target_link_libraries(read_pico_validation_test GTest::gtest_main)
gtest_discover_tests(read_pico_validation_test)

add_executable(read_pico_pattern_test
  test_read_pico_pattern.cpp
  ../components/read_pico_display/src/read_pico_pattern.c
  ../components/read_pico_display/src/read_pico_frame.c
)
target_include_directories(read_pico_pattern_test PRIVATE
  ../components/read_pico_display/include
)
target_link_libraries(read_pico_pattern_test GTest::gtest_main)
gtest_discover_tests(read_pico_pattern_test)

add_executable(read_pico_scan_probe_test
  test_read_pico_scan_probe.cpp
  ../components/read_pico_display/src/read_pico_scan_trace.c
  ../components/epaper_driver_read_pico/src/read_pico_scan_probe.c
)
target_include_directories(read_pico_scan_probe_test PRIVATE
  read_pico_probe_stubs read_pico_stubs stubs ../components/read_pico_display/include
  ../components/epaper_driver_read_pico/include
)
target_link_libraries(read_pico_scan_probe_test GTest::gtest_main)
gtest_discover_tests(read_pico_scan_probe_test)

# Real locked vendor CPU drawing and highlevel wrapper, never its IO/DMA renderer.
add_executable(read_pico_vendor_test
  test_read_pico_vendor.cpp
  ../components/epaper_driver_read_pico/src/read_pico_vendor_gray16.c
  ../components/read_pico_display/src/read_pico_pattern.c
  ../components/read_pico_display/src/read_pico_frame.c
  ../third_party/read_pico_firmware/components/epdiy/src/epdiy.c
  ../third_party/read_pico_firmware/components/epdiy/src/highlevel.c
)
target_include_directories(read_pico_vendor_test PRIVATE
  read_pico_vendor_stubs stubs read_pico_stubs
  ../components/epaper_driver_read_pico/src ../components/read_pico_display/include
  ../third_party/read_pico_firmware/components/epdiy/src
  ../third_party/read_pico_firmware/components/epdiy/include
)
target_link_libraries(read_pico_vendor_test GTest::gtest_main)
gtest_discover_tests(read_pico_vendor_test)
