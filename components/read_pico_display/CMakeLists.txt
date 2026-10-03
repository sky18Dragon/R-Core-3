# Portable format/transaction core only. No EPDiy/BSP or hardware initialization.
idf_component_register(
    SRCS "src/read_pico_frame.c" "src/read_pico_refresh.c" "src/read_pico_validation.c"
         "src/read_pico_pattern.c" "src/read_pico_scan_trace.c"
    INCLUDE_DIRS "include"
)
