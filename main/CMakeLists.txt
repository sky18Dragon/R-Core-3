if(CONFIG_READ_PICO_PHOTO_VALIDATION AND (CONFIG_READ_PICO_PHOTOFRAME_INTEGRATION OR
   CONFIG_READ_PICO_DISPLAY_VALIDATION OR CONFIG_READ_PICO_STORAGE_VALIDATION))
    message(FATAL_ERROR "Read Pico photo diagnostic requires an exclusive profile")
endif()
if(CONFIG_BOARD_DRIVER_READ_PICO AND NOT CONFIG_READ_PICO_PHOTOFRAME_INTEGRATION)
    if(CONFIG_READ_PICO_STORAGE_VALIDATION AND CONFIG_READ_PICO_DISPLAY_VALIDATION)
        message(FATAL_ERROR "Read Pico SD and display validation must be isolated")
    endif()
    if(CONFIG_READ_PICO_PHOTO_VALIDATION)
        idf_component_register(
            SRCS "read_pico_photo_validation.c" "read_pico_photo_session.c" "read_pico_photo_frame.c"
                 "read_pico_photo_bridge.c" "image_processor.c" "jpeg_metadata.c" "jpeg_stream_probe.c"
                 "jpeg_buffered_source.c"
                 "color_palette.c"
            INCLUDE_DIRS "."
            PRIV_REQUIRES board_hal sdcard esp_driver_usb_serial_jtag vfs espressif__esp_jpeg
                          espressif__libpng espressif__cjson nvs_flash esp_timer esp_partition
                          esp_app_format esp_psram read_pico_display epaper_driver_read_pico
        )
        return()
    endif()
    if(CONFIG_READ_PICO_STORAGE_VALIDATION)
        idf_component_register(
            SRCS "read_pico_storage_validation.c" "sd_file_probe.c" "jpeg_stream_probe.c" "jpeg_file_probe.c"
                 "upload_file.c" "read_pico_upload.c" "upload_probe.c"
            INCLUDE_DIRS "."
            PRIV_REQUIRES board_hal sdcard esp_driver_usb_serial_jtag vfs espressif__esp_jpeg
        )
        return()
    endif()
    set(READ_PICO_ENTRY "read_pico_bringup.c")
    if(CONFIG_READ_PICO_DISPLAY_VALIDATION)
        set(READ_PICO_ENTRY "read_pico_display_validation.c")
    endif()
    # P1 cannot fall through to the PhotoFrame app (display/NVS/SD are not ready).
    idf_component_register(
        # Compile the checked business adapter without starting or linking its unused app flow.
        SRCS "${READ_PICO_ENTRY}" "display_manager.c" "utils.c"
        INCLUDE_DIRS "."
        PRIV_REQUIRES board_hal esp_app_format esp_psram esp_hw_support read_pico_display
                      epaper_driver_read_pico epaper_src espressif__zlib nvs_flash
                      esp_http_client esp_netif esp_wifi espressif__cjson
                      esp_driver_usb_serial_jtag esp_timer esp_partition vfs
    )
    return()
endif()

set(SOURCES
    "album_manager.c"
    "cert_pin.c"
    "http_endpoint.c"
    "color_palette.c"
    "config_manager.c"
    "crash_log.c"
    "crash_record.c"
    "debug_log.c"
    "display_flow.c"
    "display_manager.c"
    "dns_server.c"
    "captive_dns.c"
    "ha_integration.c"
    "http_server.c"
    "image_processor.c"
    "main.c"
    "mdns_service.c"
    "network_backoff.c"
    "nvs_bootstrap.c"
    "ota_manager.c"
    "periodic_tasks.c"
    "png_decoder.c"
    "power_manager.c"
    "processing_settings.c"
    "splash_screen.c"
    "storage.c"
    "cron.c"
    "utils.c"
    "http_auth.c"
    "wifi_manager.c"
    "wifi_credentials.c"
    "wifi_provisioning.c"
)

if(CONFIG_READ_PICO_PHOTOFRAME_INTEGRATION)
    if(CONFIG_READ_PICO_DISPLAY_VALIDATION OR CONFIG_READ_PICO_STORAGE_VALIDATION)
        message(FATAL_ERROR "Read Pico full-stack integration must use an isolated profile")
    endif()
    # Retain and link the actual app; only its automatic boot entry is replaced.
    set_source_files_properties("main.c" PROPERTIES COMPILE_DEFINITIONS "app_main=photoframe_app_main")
    list(APPEND SOURCES "read_pico_integration_entry.c")
endif()

set(INCLUDE_DIRS ".")

if(CONFIG_BOARD_DRIVER_READ_PICO)
    list(REMOVE_ITEM SOURCES "periodic_tasks.c")
    list(APPEND SOURCES "read_pico_periodic.c" "read_pico_sntp.c" "sntp_stamp.c" "remote_config.c" "config_document.c" "wifi_network_json.c" "config_apply.c" "request_config.c")
    list(APPEND SOURCES "jpeg_metadata.c" "jpeg_stream_probe.c" "jpeg_buffered_source.c"
                        "upload_stream.c" "upload_file.c" "read_pico_upload.c" "upload_asset.c"
                        "download_file.c" "upload_copy.c" "config_read.c" "wifi_text.c"
                        "provisioning_session.c" "provisioning_apply.c" "wifi_link.c" "wifi_watch.c" "wifi_network.c" "touch_gesture.c" "image_history.c" "upload_sweep.c" "ota_local.c" "sleep_policy.c"
                        "wifi_provisioning_ap.c" "read_pico_http_service.c" "read_pico_background.c")
    if(CONFIG_READ_PICO_TOUCH)
        list(APPEND SOURCES "read_pico_touch_nav.c")
    endif()
    if(CONFIG_READ_PICO_LOCAL_FRAME)
        list(APPEND SOURCES "read_pico_ota.c")
    endif()
    if(CONFIG_READ_PICO_PMU_SLEEP)
        list(APPEND SOURCES "read_pico_frame_sleep.c")
    endif()
endif()

set(PRIV_REQUIRES
    app_update
    board_hal
    espressif__qrcode
    epaper
    epaper_src
    epaper_driver_read_pico
    esp_app_format
    esp_psram
    esp_hw_support
    read_pico_display
    read_pico_touch
    esp_driver_gpio
    esp_driver_usb_serial_jtag
    vfs
    esp_event
    esp_http_client
    http_parser
    esp_http_server
    esp_https_ota
    esp-tls
    esp_netif
    esp_timer
    esp_wifi
    espressif__cjson
    espressif__esp_jpeg
    espressif__libpng
    espressif__zlib
    espressif__mdns
    fatfs
    mbedtls
    memfs
    nvs_flash
    sdcard
    espcoredump
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "."
    REQUIRES ""
    PRIV_REQUIRES ${PRIV_REQUIRES} 
    EMBED_FILES "webapp/index.html.gz" "webapp/icon.svg.gz" "webapp/assets/index.css.gz" "webapp/assets/index.js.gz" "webapp/assets/index2.js.gz" "webapp/assets/exif-reader.js.gz" "webapp/assets/browser.js.gz" "webapp/assets/__vite-browser-external.js.gz" "resources/measurement_sample.jpg"
)

# Embed splash screen EPDGZ files (generated at build time by generate_splash.py)
if(CONFIG_BOARD_DRIVER_READ_PICO)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE SNTP_CHECK_RESPONSE=2 SNTP_COMP_ROUNDTRIP=0)
    target_link_options(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=sntp_get_system_time" "-Wl,--wrap=udp_new_ip_type")
endif()
target_add_binary_data(${COMPONENT_LIB} "splash_data/splash.epdgz" BINARY)
target_add_binary_data(${COMPONENT_LIB} "splash_data/setup_complete.epdgz" BINARY)
