#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "read_pico_pmu_codec.h"
#ifdef __cplusplus
extern "C" {
#endif
// State-changing PMU commands, used ONLY by the opt-in sleep path
// (CONFIG_READ_PICO_PMU_SLEEP). The observation codec stays read-only.
// Frame layout and payloads follow read_pico_pmu docs/llms-full_zh_cn.md
// (protocol 1.1, firmware 1.0.8).
#define READ_PICO_PMU_PAYLOAD_MAX 44
#define READ_PICO_PMU_ALARM_MIN_S 10u
#define READ_PICO_PMU_ALARM_MAX_S 2678400u
#define READ_PICO_PMU_TIME_MIN 946684800u
#define READ_PICO_PMU_TIME_MAX 4102444799u

// Encode a request frame (64 bytes). False for a bad session/sequence or an
// oversized payload; the frame is zeroed in that case.
bool read_pico_pmu_encode_request(const read_pico_identity_t *identity, uint16_t sequence,
                                  uint16_t opcode, const uint8_t *payload, size_t payload_length,
                                  uint8_t *frame, size_t frame_length);
// Decode the response to (session, sequence, opcode). status receives the
// PMU status code; payload (optional) receives up to capacity bytes.
bool read_pico_pmu_decode_response(const uint8_t *frame, size_t frame_length,
                                   const read_pico_identity_t *identity, uint16_t sequence,
                                   uint16_t opcode, uint16_t *status, uint8_t *payload,
                                   size_t capacity, size_t *payload_length);
// Payload builders; return the payload length, 0 when the argument is invalid.
size_t read_pico_pmu_time_sync_payload(uint32_t unix_seconds, uint8_t *out);
// mode 1 = once. Seconds must be within the PMU's 10 s .. 31 day range.
size_t read_pico_pmu_alarm_once_payload(uint32_t seconds, uint8_t *out);
#ifdef __cplusplus
}
#endif
