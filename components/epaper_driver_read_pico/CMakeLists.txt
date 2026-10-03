if("${SDKCONFIG_DEFAULTS}" MATCHES "sdkconfig.defaults.read_pico" OR
   "${SDKCONFIG}" MATCHES "sdkconfig.read_pico")
    set(READ_PICO_SOURCES "src/driver_read_pico.c" "src/read_pico_sy_power.c")
    if(CONFIG_READ_PICO_DISPLAY_VALIDATION)
        list(APPEND READ_PICO_SOURCES "src/read_pico_scan_probe.c" "src/read_pico_vendor_gray16.c")
    endif()
    idf_component_register(
        SRCS ${READ_PICO_SOURCES}
        INCLUDE_DIRS "include"
        REQUIRES epaper read_pico_display
        PRIV_REQUIRES board_hal epdiy e0470_epaper_waveform read_pico_timing sy7636a
                      esp_driver_i2c esp_driver_gpio esp_rom esp_hw_support freertos esp_timer
    )
    if(CONFIG_READ_PICO_DISPLAY_VALIDATION)
        target_link_options(${COMPONENT_LIB} INTERFACE
            "-Wl,--wrap=epd_lcd_line_source_cb" "-Wl,--wrap=epd_lcd_frame_done_cb"
            "-Wl,--wrap=epd_lcd_start_frame")
    endif()
else()
    idf_component_register(INCLUDE_DIRS "include" REQUIRES epaper read_pico_display)
endif()
