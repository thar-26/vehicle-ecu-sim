#include "watchdog.h"

#include <stdbool.h>

static bool s_initialized = false;
static unsigned int s_timeout_ms = 0;
static unsigned long s_last_kick_ms = 0;

watchdog_status_t watchdog_init(unsigned int timeout_ms) {
    if (s_initialized) {
        return WATCHDOG_ERR_ALREADY_INIT;
    }
    s_initialized = true;
    s_timeout_ms = timeout_ms;
    s_last_kick_ms = 0;
    return WATCHDOG_OK;
}

watchdog_status_t watchdog_kick(unsigned long now_ms) {
    if (!s_initialized) {
        return WATCHDOG_ERR_NOT_INIT;
    }
    s_last_kick_ms = now_ms;
    return WATCHDOG_OK;
}

watchdog_status_t watchdog_check(unsigned long now_ms) {
    if (!s_initialized) {
        return WATCHDOG_ERR_NOT_INIT;
    }
    unsigned long elapsed = (now_ms >= s_last_kick_ms) ? (now_ms - s_last_kick_ms) : 0UL;
    if (elapsed > s_timeout_ms) {
        return WATCHDOG_ERR_EXPIRED;
    }
    return WATCHDOG_OK;
}

void watchdog_deinit(void) {
    s_initialized = false;
    s_timeout_ms = 0;
    s_last_kick_ms = 0;
}
