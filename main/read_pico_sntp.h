#pragma once
#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

// Worker-context API. allowed may check the live connection; clock_gate is
// called in TCPIP context and MUST be a nonblocking, side-effect-free gate.
// Confirmed time publishes only after checked clock application and own exit.
esp_err_t read_pico_sntp_sync(const char *server, bool (*allowed)(void),
                             bool (*clock_gate)(void), time_t *confirmed);
void read_pico_sntp_request_stop(void);
// Permanently closes admission this boot. Pending DNS isn't cancelled/freed;
// NOT_FINISHED/busy/failure must prevent further radio teardown.
esp_err_t read_pico_sntp_stop(void);
