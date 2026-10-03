# Compile the locked manufacturer's timing source in place; no BSP aggregate or edits.
if("${SDKCONFIG_DEFAULTS}" MATCHES "sdkconfig.defaults.read_pico" OR
   "${SDKCONFIG}" MATCHES "sdkconfig.read_pico")
    idf_component_register(
        SRCS "../../third_party/read_pico_firmware/components/read_pico/read_pico_epd_timing.c"
        INCLUDE_DIRS "../../third_party/read_pico_firmware/components/read_pico/include"
        REQUIRES epdiy e0470_epaper_waveform
    )
else()
    idf_component_register()
endif()
