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
include(GoogleTest)

add_executable(jpeg_buffered_source_test test_jpeg_buffered_source.cpp
  ../main/jpeg_buffered_source.c)
target_include_directories(jpeg_buffered_source_test PRIVATE ../main)
target_link_libraries(jpeg_buffered_source_test GTest::gtest_main)
gtest_discover_tests(jpeg_buffered_source_test)

add_executable(read_pico_photo_test test_read_pico_photo.cpp
  ../main/read_pico_photo_session.c ../main/read_pico_photo_frame.c)
target_include_directories(read_pico_photo_test PRIVATE ../main ../components/read_pico_display/include)
target_link_libraries(read_pico_photo_test GTest::gtest_main)
gtest_discover_tests(read_pico_photo_test)

add_executable(jpeg_metadata_test test_jpeg_metadata.cpp ../main/jpeg_metadata.c)
target_include_directories(jpeg_metadata_test PRIVATE ../main)
target_link_libraries(jpeg_metadata_test GTest::gtest_main)
gtest_discover_tests(jpeg_metadata_test)

# Explicit opt-in: real locked external decoder, never download photos or dependencies.
option(READ_PICO_JPEG_STREAM_TESTS "Build locked esp_jpeg streaming probe" OFF)
if(READ_PICO_JPEG_STREAM_TESTS)
  set(JPEG_COMPONENT "${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/espressif__esp_jpeg")
  file(READ "${JPEG_COMPONENT}/idf_component.yml" JPEG_MANIFEST)
  if(NOT JPEG_MANIFEST MATCHES "version: 1\\.3\\.1(\n|$)")
    message(FATAL_ERROR "JPEG streaming tests require locked esp_jpeg 1.3.1")
  endif()
  file(SHA256 "${JPEG_COMPONENT}/tjpgd/tjpgd.c" JPEG_SOURCE_SHA)
  file(SHA256 "${JPEG_COMPONENT}/tjpgd/tjpgd.h" JPEG_HEADER_SHA)
  file(SHA256 "${JPEG_COMPONENT}/tjpgd/tjpgdcnf.h" JPEG_CONFIG_SHA)
  if(NOT JPEG_SOURCE_SHA STREQUAL "1286f5e32e7a4750ee36fd57d2c442c99a74ffed0e81cb5d2fbf92fccf64a84d" OR
     NOT JPEG_HEADER_SHA STREQUAL "53d1156fd9fcc97d346e5ee8c16ae39c34d71c691dcad3c524424bb91519cd24" OR
     NOT JPEG_CONFIG_SHA STREQUAL "6a2134a4aae53bc361a7c4f1bfc8ef0107452191d6c4ae26bd1030ca441459ae")
    message(FATAL_ERROR "Locked JPEG decoder source/header/config drift")
  endif()
  add_library(jpeg_stream_probe STATIC ../main/jpeg_stream_probe.c ../main/jpeg_file_probe.c
    ../main/jpeg_buffered_source.c
    "${JPEG_COMPONENT}/tjpgd/tjpgd.c")
  target_include_directories(jpeg_stream_probe PUBLIC ../main PRIVATE
    jpeg_stream_stubs "${JPEG_COMPONENT}/tjpgd")
  add_executable(jpeg_stream_probe_cli jpeg_stream_probe_cli.c)
  target_link_libraries(jpeg_stream_probe_cli jpeg_stream_probe)
  add_executable(jpeg_stream_probe_test test_jpeg_stream_probe.cpp)
  target_compile_definitions(jpeg_stream_probe_test PRIVATE
    JPEG_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../process-cli/test/test-albums/Default")
  target_link_libraries(jpeg_stream_probe_test jpeg_stream_probe GTest::gtest_main)
  gtest_discover_tests(jpeg_stream_probe_test)
  add_library(read_pico_jpeg_pipeline STATIC
    ../main/image_processor.c ../main/display_flow.c ../main/jpeg_metadata.c
    stubs/esp_stubs.c stubs/fake_display_manager.c stubs/fake_config_manager.c
    stubs/fake_processing_settings.c stubs/fake_storage.c stubs/fake_jpeg_media.c)
  target_include_directories(read_pico_jpeg_pipeline PUBLIC
    stubs ../main ../components/board_hal/include)
  target_compile_definitions(read_pico_jpeg_pipeline PUBLIC
    CONFIG_BOARD_DRIVER_READ_PICO=1 READ_PICO_JPEG_PIPELINE_TESTS=1)
  # Interpose only this host library's stdio close, leaving test writers real.
  target_compile_definitions(read_pico_jpeg_pipeline PRIVATE fclose=read_pico_test_fclose)
  find_package(PNG REQUIRED)
  target_link_libraries(read_pico_jpeg_pipeline PUBLIC jpeg_stream_probe PNG::PNG m)
  add_executable(read_pico_jpeg_pipeline_test test_read_pico_jpeg_pipeline.cpp)
  target_compile_definitions(read_pico_jpeg_pipeline_test PRIVATE
    JPEG_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../process-cli/test/test-albums/Default")
  target_link_libraries(read_pico_jpeg_pipeline_test read_pico_jpeg_pipeline GTest::gtest_main)
  gtest_discover_tests(read_pico_jpeg_pipeline_test)
  add_executable(read_pico_jpeg_pipeline_cli read_pico_jpeg_pipeline_cli.c)
  target_link_libraries(read_pico_jpeg_pipeline_cli read_pico_jpeg_pipeline)
  add_library(read_pico_photo_diagnostic STATIC
    ../main/read_pico_photo_bridge.c ../main/read_pico_photo_frame.c ../main/image_processor.c
    ../main/jpeg_metadata.c stubs/esp_stubs.c stubs/fake_storage.c stubs/fake_jpeg_media.c)
  target_include_directories(read_pico_photo_diagnostic PUBLIC
    stubs ../main ../components/board_hal/include ../components/read_pico_display/include)
  target_compile_definitions(read_pico_photo_diagnostic PUBLIC
    CONFIG_BOARD_DRIVER_READ_PICO=1 CONFIG_READ_PICO_PHOTO_VALIDATION=1 READ_PICO_JPEG_PIPELINE_TESTS=1)
  target_compile_definitions(read_pico_photo_diagnostic PRIVATE fclose=read_pico_test_fclose)
  target_link_libraries(read_pico_photo_diagnostic PUBLIC jpeg_stream_probe PNG::PNG m)
  add_executable(read_pico_photo_bridge_test test_read_pico_photo_bridge.cpp)
  target_compile_definitions(read_pico_photo_bridge_test PRIVATE
    JPEG_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../process-cli/test/test-albums/Default")
  target_link_libraries(read_pico_photo_bridge_test read_pico_photo_diagnostic GTest::gtest_main)
  gtest_discover_tests(read_pico_photo_bridge_test)
  add_executable(read_pico_photo_diagnostic_cli read_pico_photo_diagnostic_cli.c)
  target_link_libraries(read_pico_photo_diagnostic_cli read_pico_photo_diagnostic)
endif()

add_executable(nvs_bootstrap_test test_nvs_bootstrap.cpp ../main/nvs_bootstrap.c)
target_include_directories(nvs_bootstrap_test PRIVATE nvs_stubs stubs ../main)
target_link_libraries(nvs_bootstrap_test GTest::gtest_main)
gtest_discover_tests(nvs_bootstrap_test)

add_executable(read_pico_nvs_bootstrap_test test_read_pico_nvs_bootstrap.cpp ../main/nvs_bootstrap.c)
target_include_directories(read_pico_nvs_bootstrap_test PRIVATE nvs_stubs stubs ../main)
target_compile_definitions(read_pico_nvs_bootstrap_test PRIVATE CONFIG_BOARD_DRIVER_READ_PICO=1)
target_link_libraries(read_pico_nvs_bootstrap_test GTest::gtest_main)
gtest_discover_tests(read_pico_nvs_bootstrap_test)

add_executable(read_pico_storage_test test_read_pico_storage.cpp ../main/storage.c)
target_include_directories(read_pico_storage_test PRIVATE
  stubs ../main ../components/board_hal/include ../components/memfs/include)
target_compile_definitions(read_pico_storage_test PRIVATE
  CONFIG_BOARD_DRIVER_READ_PICO=1 CONFIG_HAS_SDCARD=1)
target_link_libraries(read_pico_storage_test GTest::gtest_main)
gtest_discover_tests(read_pico_storage_test)

add_executable(read_pico_integration_entry_test
  test_read_pico_integration_entry.cpp ../main/read_pico_integration_entry.c)
target_include_directories(read_pico_integration_entry_test PRIVATE stubs)
target_link_libraries(read_pico_integration_entry_test GTest::gtest_main)
gtest_discover_tests(read_pico_integration_entry_test)

add_executable(power_gpio_mask_test test_power_gpio_mask.cpp)
target_include_directories(power_gpio_mask_test PRIVATE ../main)
target_link_libraries(power_gpio_mask_test GTest::gtest_main)
gtest_discover_tests(power_gpio_mask_test)

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
gtest_discover_tests(cron_test)
gtest_discover_tests(wake_schedule_test)
gtest_discover_tests(http_auth_test)
gtest_discover_tests(network_backoff_test)
gtest_discover_tests(crash_record_test)

# Actual SDMMC backend against host fakes. This verifies board-selected 1/4-bit
# wiring and the no-auto-format contract without mounting or modifying media.
add_executable(sdcard_sdio_test
  test_sdcard_sdio.cpp
  ../components/sdcard/src/sdcard_sdio.c
)
target_include_directories(sdcard_sdio_test PRIVATE
  sdcard_stubs stubs ../components/sdcard/include
)
target_link_libraries(sdcard_sdio_test GTest::gtest_main)
gtest_discover_tests(sdcard_sdio_test)

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

add_executable(read_pico_sd_test
  test_read_pico_sd.cpp
  ../components/board_hal/src/read_pico_sd.c
  ../components/board_hal/src/read_pico_sd_command.c
)
target_include_directories(read_pico_sd_test PRIVATE
  sdcard_stubs read_pico_stubs stubs
  ../components/board_hal/include ../components/sdcard/include
)
target_link_libraries(read_pico_sd_test GTest::gtest_main)
gtest_discover_tests(read_pico_sd_test)

add_executable(sd_file_probe_test
  test_sd_file_probe.cpp
  ../main/sd_file_probe.c
)
target_include_directories(sd_file_probe_test PRIVATE ../main)
target_link_libraries(sd_file_probe_test GTest::gtest_main)
gtest_discover_tests(sd_file_probe_test)

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
