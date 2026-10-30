if("${SDKCONFIG_DEFAULTS}" MATCHES "sdkconfig.defaults.read_pico" OR
   "${SDKCONFIG}" MATCHES "sdkconfig.read_pico")
    set(TOUCH_SOURCES "")
    if(CONFIG_READ_PICO_TOUCH)
        list(APPEND TOUCH_SOURCES "src/read_pico_touch.c")
    endif()
    # The vendor CST836U driver is used unmodified; this adapter only adds the
    # board lock and never drives TP_RST (FCA9555 P0.7).
    idf_component_register(
        SRCS ${TOUCH_SOURCES}
        INCLUDE_DIRS "include"
        PRIV_REQUIRES board_hal cst836u esp_driver_i2c esp_driver_gpio freertos
    )
else()
    idf_component_register(INCLUDE_DIRS "include")
endif()
