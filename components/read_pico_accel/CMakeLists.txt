if("${SDKCONFIG_DEFAULTS}" MATCHES "sdkconfig.defaults.read_pico" OR
   "${SDKCONFIG}" MATCHES "sdkconfig.read_pico")
    set(ACCEL_SOURCES "")
    if(CONFIG_READ_PICO_LOCAL_FRAME)
        list(APPEND ACCEL_SOURCES "src/read_pico_accel.c")
    endif()
    # The vendor SC7A20H driver is used unmodified; this adapter only adds the
    # board lock and leaves the INT1 pin unconfigured (polled, no interrupts).
    idf_component_register(
        SRCS ${ACCEL_SOURCES}
        INCLUDE_DIRS "include"
        PRIV_REQUIRES board_hal sc7a20h esp_driver_i2c esp_driver_gpio freertos
    )
else()
    idf_component_register(INCLUDE_DIRS "include")
endif()
