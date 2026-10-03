if(CONFIG_BOARD_DRIVER_READ_PICO)
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
    "color_palette.c"
    "config_manager.c"
    "crash_log.c"
    "crash_record.c"
    "debug_log.c"
    "display_flow.c"
    "display_manager.c"
    "dns_server.c"
    "ha_integration.c"
    "http_server.c"
    "image_processor.c"
    "main.c"
    "mdns_service.c"
    "network_backoff.c"
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
    "wifi_provisioning.c"
)

set(INCLUDE_DIRS ".")

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
    esp_driver_gpio
    esp_driver_usb_serial_jtag
    vfs
    esp_event
    esp_http_client
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
target_add_binary_data(${COMPONENT_LIB} "splash_data/splash.epdgz" BINARY)
target_add_binary_data(${COMPONENT_LIB} "splash_data/setup_complete.epdgz" BINARY)
