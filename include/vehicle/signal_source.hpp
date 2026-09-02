#pragma once

#include "vehicle/signal.hpp"

#include <optional>

namespace vehicle {

// Anything that can be polled for the next available signal: a CAN
// adapter, an Ethernet/SOME-IP link, a log replay, or a test double.
// The router (and everything downstream of it) only ever depends on
// this interface, never on a concrete transport.
class ISignalSource {
public:
    virtual ~ISignalSource() = default;

    // Non-blocking poll. Returns nullopt when nothing is currently
    // available -- callers are expected to poll repeatedly, not block.
    virtual std::optional<Signal> receive() = 0;
};

// Anything that wants to be notified when a signal it subscribed to
// arrives.
class ISignalConsumer {
public:
    virtual ~ISignalConsumer() = default;
    virtual void onSignal(const Signal& signal) = 0;
};

} // namespace vehicle
