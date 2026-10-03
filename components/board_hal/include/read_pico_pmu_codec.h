#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Portable observation codec; deliberately exposes no state-changing opcode.
typedef struct {
    uint8_t firmware[3];
    uint8_t hardware;
    uint32_t session;
    uint32_t capabilities;
} read_pico_identity_t;

typedef enum {
    READ_PICO_POWER_UNKNOWN,
    READ_PICO_POWER_EXTERNAL,
} read_pico_external_power_t;

typedef struct {
    int battery_mv;
    int battery_percent;
    int charging;  // -1 unknown, 0 not charging, 1 charging; NOT a USB detector.
    read_pico_external_power_t external_power;
    uint8_t power_state;
    uint32_t fault_flags;
    uint16_t last_sequence;
} read_pico_status_t;

uint16_t read_pico_crc16(const uint8_t *data, size_t length);
bool read_pico_decode_identity(const uint8_t *raw, size_t length, read_pico_identity_t *out);
bool read_pico_decode_status(const uint8_t *raw, size_t length,
                             const read_pico_identity_t *identity, read_pico_status_t *out);
uint16_t read_pico_next_sequence(uint16_t previous);
bool read_pico_encode_vcom_get(const read_pico_identity_t *identity, uint16_t sequence,
                               uint8_t *frame, size_t length);
bool read_pico_decode_vcom(const uint8_t *frame, size_t length,
                           const read_pico_identity_t *identity, uint16_t sequence, int *mv);

#ifdef __cplusplus
}
#endif
