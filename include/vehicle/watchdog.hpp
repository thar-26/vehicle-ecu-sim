#pragma once

#include <cstdint>

namespace vehicle {

// RAII C++ wrapper around the plain-C watchdog HAL (c/watchdog.{h,c}).
// The watchdog models one hardware peripheral, which is genuinely
// process-global state in C -- this wrapper does not pretend you can
// have two independent Watchdog objects: constructing a second one
// while the first is still alive throws std::runtime_error.
class Watchdog {
public:
    explicit Watchdog(std::uint32_t timeoutMs);
    ~Watchdog();

    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;

    void kick(std::uint64_t nowMs);
    bool expired(std::uint64_t nowMs) const;

private:
    bool active_ = false;
};

} // namespace vehicle
