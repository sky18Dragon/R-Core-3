#pragma once
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#ifdef __cplusplus
extern "C" {
#endif
void probe_test_enter(portMUX_TYPE *);
void probe_test_exit(portMUX_TYPE *);
int xPortGetCoreID(void);
#ifdef __cplusplus
}
#endif
#define portENTER_CRITICAL_SAFE(mux) probe_test_enter(mux)
#define portEXIT_CRITICAL_SAFE(mux) probe_test_exit(mux)
