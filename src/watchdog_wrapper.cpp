#include "vehicle/watchdog.hpp"

extern "C" {
#include "watchdog.h"
}

#include <stdexcept>

namespace vehicle {

Watchdog::Watchdog(std::uint32_t timeoutMs) {
    const watchdog_status_t status = watchdog_init(timeoutMs);
    if (status != WATCHDOG_OK) {
        throw std::runtime_error("Watchdog already initialized -- only one instance is supported, "
                                  "it models a single hardware peripheral");
    }
    active_ = true;
}

Watchdog::~Watchdog() {
    if (active_) {
        watchdog_deinit();
    }
}

void Watchdog::kick(std::uint64_t nowMs) {
    watchdog_kick(static_cast<unsigned long>(nowMs));
}

bool Watchdog::expired(std::uint64_t nowMs) const {
    return watchdog_check(static_cast<unsigned long>(nowMs)) == WATCHDOG_ERR_EXPIRED;
}

} // namespace vehicle
