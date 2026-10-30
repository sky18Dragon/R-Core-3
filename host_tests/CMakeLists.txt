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

add_executable(wifi_text_test test_wifi_text.cpp ../main/wifi_text.c)
target_include_directories(wifi_text_test PRIVATE ../main)
target_link_libraries(wifi_text_test GTest::gtest_main)
gtest_discover_tests(wifi_text_test)

add_executable(read_pico_provisioning_ap_test test_read_pico_provisioning_ap.cpp
  ../main/wifi_provisioning_ap.c ../main/provisioning_session.c ../main/read_pico_http_service.c)
target_compile_definitions(read_pico_provisioning_ap_test PRIVATE CONFIG_BOARD_DRIVER_READ_PICO=1)
target_include_directories(read_pico_provisioning_ap_test PRIVATE ap_stubs dns_stubs wifi_stubs
  stubs ../main)
target_link_libraries(read_pico_provisioning_ap_test GTest::gtest_main)
gtest_discover_tests(read_pico_provisioning_ap_test)

add_executable(read_pico_http_service_test test_read_pico_http_service.cpp
  ../main/read_pico_http_service.c)
target_include_directories(read_pico_http_service_test PRIVATE ap_stubs stubs ../main)
target_link_libraries(read_pico_http_service_test GTest::gtest_main)
gtest_discover_tests(read_pico_http_service_test)

add_executable(read_pico_background_test test_read_pico_background.cpp
  ../main/read_pico_background.c)
target_include_directories(read_pico_background_test PRIVATE background_stubs stubs ../main)
target_link_libraries(read_pico_background_test GTest::gtest_main)
gtest_discover_tests(read_pico_background_test)

add_executable(read_pico_periodic_test test_read_pico_periodic.cpp
  ../main/read_pico_periodic.c ../main/read_pico_background.c)
target_compile_definitions(read_pico_periodic_test PRIVATE CONFIG_BOARD_DRIVER_READ_PICO=1)
target_include_directories(read_pico_periodic_test PRIVATE periodic_stubs background_stubs config_load_stubs stubs ../main)
set_source_files_properties(../main/read_pico_periodic.c PROPERTIES COMPILE_DEFINITIONS
  "time=read_pico_periodic_test_time")
target_link_libraries(read_pico_periodic_test GTest::gtest_main)
gtest_discover_tests(read_pico_periodic_test)

add_executable(sntp_stamp_test test_sntp_stamp.cpp ../main/sntp_stamp.c)
target_include_directories(sntp_stamp_test PRIVATE ../main)
target_link_libraries(sntp_stamp_test GTest::gtest_main)
gtest_discover_tests(sntp_stamp_test)

add_executable(read_pico_sntp_test test_read_pico_sntp.cpp ../main/read_pico_sntp.c ../main/sntp_stamp.c)
target_compile_definitions(read_pico_sntp_test PRIVATE CONFIG_BOARD_DRIVER_READ_PICO=1
  SNTP_CHECK_RESPONSE=2 SNTP_COMP_ROUNDTRIP=0)
target_include_directories(read_pico_sntp_test PRIVATE sntp_stubs background_stubs config_load_stubs stubs ../main)
set_source_files_properties(../main/read_pico_sntp.c PROPERTIES COMPILE_DEFINITIONS
  "settimeofday=read_pico_sntp_test_settimeofday")
target_link_libraries(read_pico_sntp_test GTest::gtest_main)
gtest_discover_tests(read_pico_sntp_test)

# Optional explicit local fixed SDK path: no download or copied SDK fork. The
# exact unmodified parser joins the real adapter; IO/timeouts are synthetic.
set(READ_PICO_TEST_IDF_PATH "" CACHE PATH "Fixed IDF v6.1 checkout for real SNTP parser tests")
if(READ_PICO_TEST_IDF_PATH)
  set(endpoint_parser_dir "${READ_PICO_TEST_IDF_PATH}/components/http_parser")
  file(SHA256 "${endpoint_parser_dir}/http_parser.c" endpoint_parser_sha)
  file(SHA256 "${endpoint_parser_dir}/http_parser.h" endpoint_header_sha)
  if(NOT endpoint_parser_sha STREQUAL "c164edcb06099b026e7ed853ed4d25536e83b4463d5e79921b28621e16588455" OR
     NOT endpoint_header_sha STREQUAL "21fcfe834198349472a878e0334dd74b8571fafc70c6dd701169f525dd463041")
    message(FATAL_ERROR "HTTP endpoint tests require the fixed unmodified SDK parser")
  endif()
  add_executable(http_endpoint_test test_http_endpoint.cpp ../main/http_endpoint.c
    "${endpoint_parser_dir}/http_parser.c")
  set_source_files_properties(../main/http_endpoint.c PROPERTIES COMPILE_DEFINITIONS
    "malloc=endpoint_test_malloc;free=endpoint_test_free")
  target_include_directories(http_endpoint_test PRIVATE ../main "${endpoint_parser_dir}")
  target_link_libraries(http_endpoint_test GTest::gtest_main)
  gtest_discover_tests(http_endpoint_test)
  set(read_pico_sntp_sdk_source "${READ_PICO_TEST_IDF_PATH}/components/lwip/lwip/src/apps/sntp/sntp.c")
  file(SHA256 "${read_pico_sntp_sdk_source}" read_pico_sntp_sdk_sha)
  if(NOT read_pico_sntp_sdk_sha STREQUAL "8695155c80c807d7ac949801b023d4e2f27db7975f78bcbba726bcdc012020e7")
    message(FATAL_ERROR "SNTP parser tests require the reviewed unmodified fixed SDK source")
  endif()
  add_executable(read_pico_sntp_wire_test test_read_pico_sntp_wire.cpp
    ../main/read_pico_sntp.c ../main/sntp_stamp.c "${read_pico_sntp_sdk_source}")
  target_compile_definitions(read_pico_sntp_wire_test PRIVATE CONFIG_BOARD_DRIVER_READ_PICO=1
    SNTP_CHECK_RESPONSE=2 SNTP_COMP_ROUNDTRIP=0)
  target_include_directories(read_pico_sntp_wire_test PRIVATE sntp_wire_stubs sntp_stubs
    background_stubs config_load_stubs stubs ../main)
  target_link_libraries(read_pico_sntp_wire_test GTest::gtest_main)
  gtest_discover_tests(read_pico_sntp_wire_test)
endif()

add_executable(wifi_network_test test_wifi_network.cpp ../main/wifi_network.c)
target_include_directories(wifi_network_test PRIVATE ../main)
target_link_libraries(wifi_network_test GTest::gtest_main)
gtest_discover_tests(wifi_network_test)

add_executable(captive_dns_test test_captive_dns.cpp ../main/captive_dns.c)
target_include_directories(captive_dns_test PRIVATE ../main)
target_link_libraries(captive_dns_test GTest::gtest_main)
gtest_discover_tests(captive_dns_test)

add_executable(read_pico_dns_server_test test_read_pico_dns_server.cpp
  ../main/dns_server.c ../main/captive_dns.c)
target_compile_definitions(read_pico_dns_server_test PRIVATE CONFIG_BOARD_DRIVER_READ_PICO=1)
target_include_directories(read_pico_dns_server_test PRIVATE dns_stubs wifi_stubs stubs ../main)
target_link_libraries(read_pico_dns_server_test GTest::gtest_main)
gtest_discover_tests(read_pico_dns_server_test)

add_executable(provisioning_apply_test test_provisioning_apply.cpp ../main/provisioning_apply.c
  ../main/wifi_network.c ../main/wifi_text.c)
target_include_directories(provisioning_apply_test PRIVATE ../main)
target_link_libraries(provisioning_apply_test GTest::gtest_main)
gtest_discover_tests(provisioning_apply_test)

add_executable(provisioning_session_test test_provisioning_session.cpp ../main/provisioning_session.c)
target_include_directories(provisioning_session_test PRIVATE ../main)
target_link_libraries(provisioning_session_test GTest::gtest_main)
gtest_discover_tests(provisioning_session_test)

add_executable(config_read_test test_config_read.cpp ../main/config_read.c)
target_include_directories(config_read_test PRIVATE ../main)
target_link_libraries(config_read_test GTest::gtest_main)
gtest_discover_tests(config_read_test)

set(read_pico_cjson_dir "${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/espressif__cjson/cJSON")
option(READ_PICO_TEST_FETCH_CJSON "Fetch pinned cJSON even if SDK managed cache exists" OFF)
if(READ_PICO_TEST_FETCH_CJSON OR NOT EXISTS "${read_pico_cjson_dir}/cJSON.c")
  # Host CI runs before SDK dependencies are resolved. Fetch source only, not
  # cJSON's own tests/install/shared-library policy; verify identical source.
  if(CMAKE_VERSION VERSION_LESS "3.18")
    FetchContent_Declare(read_pico_cjson GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
      GIT_TAG b2890c8d76bbb64e710585ebc0a917196b9c67e7)
    FetchContent_GetProperties(read_pico_cjson)
    if(NOT read_pico_cjson_POPULATED)
      FetchContent_Populate(read_pico_cjson)
    endif()
  else()
    FetchContent_Declare(read_pico_cjson GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
      GIT_TAG b2890c8d76bbb64e710585ebc0a917196b9c67e7 SOURCE_SUBDIR read-pico-host-source-only)
    FetchContent_MakeAvailable(read_pico_cjson)
  endif()
  set(read_pico_cjson_dir "${read_pico_cjson_SOURCE_DIR}")
endif()
file(SHA256 "${read_pico_cjson_dir}/cJSON.c" read_pico_cjson_source_sha)
file(SHA256 "${read_pico_cjson_dir}/cJSON.h" read_pico_cjson_header_sha)
if(NOT read_pico_cjson_source_sha STREQUAL "607e756460fa0de37d20a7a9181f2de29c97bfb7ce5a0e6c2f548243836cd852" OR
   NOT read_pico_cjson_header_sha STREQUAL "25b0145150d500498e4d209cec69c18c42cf818bffcc54690be3b895a2a16dee")
  message(FATAL_ERROR "Host JSON tests require the fixed cJSON 1.7.19 source/header used by SDK")
endif()
add_executable(remote_config_test test_remote_config.cpp test_wifi_network_json.cpp test_config_document.cpp
  ../main/remote_config.c ../main/config_document.c ../main/config_apply.c ../main/wifi_network_json.c ../main/wifi_network.c "${read_pico_cjson_dir}/cJSON.c")
target_include_directories(remote_config_test PRIVATE "${read_pico_cjson_dir}" stubs ../main)
target_link_libraries(remote_config_test GTest::gtest_main)
gtest_discover_tests(remote_config_test)

add_executable(config_apply_test test_config_apply.cpp ../main/config_apply.c)
target_include_directories(config_apply_test PRIVATE stubs ../main)
target_link_libraries(config_apply_test GTest::gtest_main)
gtest_discover_tests(config_apply_test)

add_executable(read_pico_mdns_test test_read_pico_mdns.cpp ../main/mdns_service.c)
target_compile_definitions(read_pico_mdns_test PRIVATE CONFIG_BOARD_DRIVER_READ_PICO=1 BOARD_HAL_NAME="synthetic")
target_include_directories(read_pico_mdns_test PRIVATE mdns_stubs config_load_stubs stubs ../main "${read_pico_cjson_dir}")
target_link_libraries(read_pico_mdns_test GTest::gtest_main)
gtest_discover_tests(read_pico_mdns_test)

add_executable(read_pico_settings_save_test test_read_pico_settings_save.cpp
  ../main/processing_settings.c ../main/color_palette.c ../main/config_apply.c ../main/request_config.c "${read_pico_cjson_dir}/cJSON.c")
target_compile_definitions(read_pico_settings_save_test PRIVATE CONFIG_BOARD_DRIVER_READ_PICO=1)
target_include_directories(read_pico_settings_save_test PRIVATE "${read_pico_cjson_dir}" config_load_stubs stubs ../main)
target_link_libraries(read_pico_settings_save_test GTest::gtest_main)
if(READ_PICO_TEST_IDF_PATH)
  set(request_header_sdk "${READ_PICO_TEST_IDF_PATH}/components/esp_http_client/lib")
  set(request_header_sdk_files http_header.c http_utils.c include/http_header.h include/http_utils.h)
  set(request_header_sdk_hashes
    23056c41505aa38885cf210e22117e1fa57d964080ff537a36a5c9d99d76701e
    50bfee8e6ef9d0b74e590f58975c9d35ce32012579994012a6d0717743c6a84b
    77f7e4faf9f2fe2d3afee3d100a3d642cdd3354d7556147d430ed0fe5e2c8f69
    7e4110110767661e60a01d639565f2926ffe30f9d449558a3a5008de5dfeecc2)
  foreach(request_header_index RANGE 0 3)
    list(GET request_header_sdk_files ${request_header_index} request_header_file)
    list(GET request_header_sdk_hashes ${request_header_index} request_header_expected)
    file(SHA256 "${request_header_sdk}/${request_header_file}" request_header_actual)
    if(NOT request_header_actual STREQUAL request_header_expected)
      message(FATAL_ERROR "Header tests require reviewed fixed SDK source/headers")
    endif()
  endforeach()
  target_sources(read_pico_settings_save_test PRIVATE "${request_header_sdk}/http_header.c" "${request_header_sdk}/http_utils.c")
  set_source_files_properties("${request_header_sdk}/http_header.c" "${request_header_sdk}/http_utils.c"
    PROPERTIES COMPILE_DEFINITIONS "calloc=request_test_calloc;realloc=request_test_realloc;strdup=request_test_strdup;free=request_test_free")
  target_include_directories(read_pico_settings_save_test PRIVATE header_stubs "${request_header_sdk}/include")
  target_compile_definitions(read_pico_settings_save_test PRIVATE READ_PICO_REQUEST_HEADER_SDK_TESTS=1)
endif()
gtest_discover_tests(read_pico_settings_save_test)

foreach(config_profile IN ITEMS read_pico legacy local_frame)
  add_executable(${config_profile}_config_load_test test_config_load.cpp
    ../main/config_manager.c ../main/config_read.c ../main/cron.c
    ../main/wifi_credentials.c ../main/wifi_text.c ../main/wifi_network.c)
  if(config_profile STREQUAL "read_pico" OR config_profile STREQUAL "local_frame")
    target_compile_definitions(${config_profile}_config_load_test PRIVATE CONFIG_BOARD_DRIVER_READ_PICO=1)
  else()
    target_compile_definitions(${config_profile}_config_load_test PRIVATE CONFIG_BOARD_DRIVER_READ_PICO=0)
  endif()
  if(config_profile STREQUAL "local_frame")
    target_compile_definitions(${config_profile}_config_load_test PRIVATE CONFIG_READ_PICO_LOCAL_FRAME=1)
  endif()
  target_include_directories(${config_profile}_config_load_test PRIVATE config_load_stubs stubs ../main)
  target_link_libraries(${config_profile}_config_load_test GTest::gtest_main)
  if(config_profile STREQUAL "local_frame")
    gtest_discover_tests(${config_profile}_config_load_test TEST_PREFIX "R1.")
  else()
    gtest_discover_tests(${config_profile}_config_load_test)
  endif()
endforeach()
set_source_files_properties(../main/config_manager.c PROPERTIES COMPILE_DEFINITIONS
  "malloc=config_load_test_malloc;setenv=config_load_test_setenv;tzset=config_load_test_tzset;time=config_load_test_time;localtime_r=config_load_test_localtime_r;strftime=config_load_test_strftime")

add_executable(upload_sweep_test test_upload_sweep.cpp ../main/upload_sweep.c)
target_include_directories(upload_sweep_test PRIVATE ../main)
target_link_libraries(upload_sweep_test GTest::gtest_main)
gtest_discover_tests(upload_sweep_test)

add_executable(touch_gesture_test test_touch_gesture.cpp ../main/touch_gesture.c)
target_include_directories(touch_gesture_test PRIVATE ../main)
target_link_libraries(touch_gesture_test GTest::gtest_main)
gtest_discover_tests(touch_gesture_test)

add_executable(image_history_test test_image_history.cpp ../main/image_history.c)
target_include_directories(image_history_test PRIVATE ../main)
target_link_libraries(image_history_test GTest::gtest_main)
gtest_discover_tests(image_history_test)

add_executable(wifi_watch_test test_wifi_watch.cpp ../main/wifi_watch.c)
target_include_directories(wifi_watch_test PRIVATE ../main)
target_link_libraries(wifi_watch_test GTest::gtest_main)
gtest_discover_tests(wifi_watch_test)

add_executable(wifi_link_test test_wifi_link.cpp ../main/wifi_link.c)
target_include_directories(wifi_link_test PRIVATE ../main)
target_link_libraries(wifi_link_test GTest::gtest_main)
gtest_discover_tests(wifi_link_test)

add_executable(read_pico_wifi_manager_test test_read_pico_wifi_manager.cpp
  ../main/wifi_manager.c ../main/wifi_link.c ../main/wifi_watch.c ../main/wifi_text.c ../main/wifi_network.c)
target_compile_definitions(read_pico_wifi_manager_test PRIVATE CONFIG_BOARD_DRIVER_READ_PICO=1)
target_include_directories(read_pico_wifi_manager_test PRIVATE wifi_stubs config_load_stubs
  display_checked_stubs stubs ../main)
target_link_libraries(read_pico_wifi_manager_test GTest::gtest_main)
gtest_discover_tests(read_pico_wifi_manager_test)

add_executable(upload_stream_test test_upload_stream.cpp ../main/upload_stream.c)
target_include_directories(upload_stream_test PRIVATE ../main)
target_link_libraries(upload_stream_test GTest::gtest_main)
gtest_discover_tests(upload_stream_test)

add_executable(upload_file_test test_upload_file.cpp ../main/upload_file.c)
target_include_directories(upload_file_test PRIVATE ../main)
target_link_libraries(upload_file_test GTest::gtest_main)
gtest_discover_tests(upload_file_test)

add_executable(upload_asset_test test_upload_asset.cpp ../main/upload_asset.c ../main/upload_file.c
  ../main/download_file.c ../main/upload_copy.c)
target_include_directories(upload_asset_test PRIVATE ../main)
target_link_libraries(upload_asset_test GTest::gtest_main)
gtest_discover_tests(upload_asset_test)

add_executable(read_pico_upload_test test_read_pico_upload.cpp
  ../main/read_pico_upload.c ../main/upload_file.c ../main/upload_probe.c)
target_include_directories(read_pico_upload_test PRIVATE stubs ../main ../components/board_hal/include)
set_source_files_properties(../main/read_pico_upload.c PROPERTIES COMPILE_DEFINITIONS
  "open=upload_test_open;write=upload_test_write;fsync=upload_test_fsync;close=upload_test_close;rename=upload_test_rename")
target_link_libraries(read_pico_upload_test GTest::gtest_main)
gtest_discover_tests(read_pico_upload_test)

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
    ../main/upload_asset.c ../main/upload_file.c
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

add_executable(read_pico_storage_test test_read_pico_storage.cpp ../main/storage.c ../main/wifi_text.c)
target_include_directories(read_pico_storage_test PRIVATE
  stubs ../main ../components/board_hal/include ../components/memfs/include)
target_compile_definitions(read_pico_storage_test PRIVATE
  CONFIG_BOARD_DRIVER_READ_PICO=1 CONFIG_HAS_SDCARD=1 FS_MOUNT_POINT="pf_wifi_storage")
set_source_files_properties(../main/storage.c PROPERTIES COMPILE_DEFINITIONS
  "fclose=wifi_storage_test_fclose")
target_link_libraries(read_pico_storage_test GTest::gtest_main)
gtest_discover_tests(read_pico_storage_test)

add_executable(read_pico_integration_entry_test
  test_read_pico_integration_entry.cpp ../main/read_pico_integration_entry.c)
target_include_directories(read_pico_integration_entry_test PRIVATE stubs)
target_link_libraries(read_pico_integration_entry_test GTest::gtest_main)
gtest_discover_tests(read_pico_integration_entry_test)

add_executable(read_pico_local_frame_entry_test
  test_read_pico_integration_entry.cpp ../main/read_pico_integration_entry.c)
target_compile_definitions(read_pico_local_frame_entry_test PRIVATE CONFIG_READ_PICO_LOCAL_FRAME=1)
target_include_directories(read_pico_local_frame_entry_test PRIVATE stubs)
target_link_libraries(read_pico_local_frame_entry_test GTest::gtest_main)
gtest_discover_tests(read_pico_local_frame_entry_test)

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

# Cases clean the same pf_storage directory; never run them concurrently.
gtest_discover_tests(display_flow_test PROPERTIES RESOURCE_LOCK host_display_flow_storage)

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

# State-changing PMU commands for the opt-in sleep path (portable codec only).
add_executable(read_pico_pmu_command_test
  test_read_pico_pmu_command.cpp
  ../components/board_hal/src/read_pico_pmu_command.c
  ../components/board_hal/src/read_pico_pmu_codec.c
)
target_include_directories(read_pico_pmu_command_test PRIVATE
  ../components/board_hal/include
  ../third_party/read_pico_firmware/components/read_pico_pmu/include
)
target_link_libraries(read_pico_pmu_command_test GTest::gtest_main)
gtest_discover_tests(read_pico_pmu_command_test)

add_executable(sleep_log_test test_sleep_log.cpp ../main/sleep_log.c)
target_include_directories(sleep_log_test PRIVATE ../main)
target_link_libraries(sleep_log_test GTest::gtest_main)
gtest_discover_tests(sleep_log_test)

add_executable(sleep_policy_test test_sleep_policy.cpp ../main/sleep_policy.c)
target_include_directories(sleep_policy_test PRIVATE ../main)
target_link_libraries(sleep_policy_test GTest::gtest_main)
gtest_discover_tests(sleep_policy_test)

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

add_executable(read_pico_fs_test test_read_pico_fs.cpp
  ../components/board_hal/src/read_pico_fs.c)
target_include_directories(read_pico_fs_test PRIVATE stubs ../components/board_hal/include)
find_package(Threads REQUIRED)
target_link_libraries(read_pico_fs_test GTest::gtest_main Threads::Threads)
gtest_discover_tests(read_pico_fs_test)

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
  ../main/upload_file.c
  ../main/display_manager.c
  ../main/image_history.c
  ../components/epaper_src/GUI_Paint.c
  ../components/epaper_src/GUI_RawBuffer.c
)
target_compile_definitions(read_pico_display_commit_test PRIVATE
  CONFIG_BOARD_DRIVER_READ_PICO=1 FS_MOUNT_POINT="pf_display_checked")
set_source_files_properties(../main/display_manager.c PROPERTIES
  COMPILE_DEFINITIONS "fclose=read_pico_display_test_fclose")
target_include_directories(read_pico_display_commit_test PRIVATE
  display_checked_stubs stubs ../main ../components/board_hal/include
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

add_executable(sd_sector_diag_test test_sd_sector_diag.cpp ../components/sdcard/src/sd_sector_diag.c)
target_include_directories(sd_sector_diag_test PRIVATE ../components/sdcard/include)
target_link_libraries(sd_sector_diag_test GTest::gtest_main)
gtest_discover_tests(sd_sector_diag_test)

add_executable(read_pico_white_first_test test_read_pico_white_first.cpp
  ../components/epaper_driver_read_pico/src/read_pico_white_first.c)
target_include_directories(read_pico_white_first_test PRIVATE read_pico_native_stubs
  ../components/epaper_driver_read_pico/src)
target_link_libraries(read_pico_white_first_test GTest::gtest_main)
gtest_discover_tests(read_pico_white_first_test)
