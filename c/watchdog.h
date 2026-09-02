#ifndef VEHICLE_C_WATCHDOG_H
#define VEHICLE_C_WATCHDOG_H

/* Plain-C watchdog HAL. Models a single hardware watchdog peripheral:
 * global state on purpose, mirroring how a real MCU only has one
 * watchdog register set. The C++ wrapper (include/vehicle/watchdog.hpp)
 * is what gives this an object-oriented, RAII-friendly interface for
 * the rest of the codebase -- this header stays plain C so it can be
 * built and used standalone in a C-only context too. */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WATCHDOG_OK = 0,
    WATCHDOG_ERR_ALREADY_INIT = 1,
    WATCHDOG_ERR_NOT_INIT = 2,
    WATCHDOG_ERR_EXPIRED = 3,
} watchdog_status_t;

/* Initializes the watchdog with a timeout in milliseconds. Must be
 * called before watchdog_kick()/watchdog_check(). Returns
 * WATCHDOG_ERR_ALREADY_INIT if already initialized. */
watchdog_status_t watchdog_init(unsigned int timeout_ms);

/* Resets ("kicks") the watchdog's countdown at time now_ms. Must be
 * called more often than timeout_ms or the next watchdog_check() will
 * report expiry. */
watchdog_status_t watchdog_kick(unsigned long now_ms);

/* Reports whether the watchdog has expired (no kick within timeout_ms
 * of now_ms). A plain time comparison rather than a real hardware
 * timer/interrupt, so it stays deterministic and unit-testable. */
watchdog_status_t watchdog_check(unsigned long now_ms);

/* Releases the watchdog so watchdog_init() can be called again. Mainly
 * for tests -- real hardware would not "deinit" a watchdog once armed. */
void watchdog_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* VEHICLE_C_WATCHDOG_H */
