#pragma once

#include <string>

namespace vehicle {

// Events the state machine reacts to. Kept to a small set that maps
// directly to something a real ECU would actually observe -- ignition
// line, boot completion, loss of bus traffic, a raised fault, an
// explicit shutdown request, a missed watchdog kick, and the point at
// which an in-progress shutdown has actually finished (flushing state,
// releasing the watchdog, etc. -- shutdown is not instantaneous).
enum class EventType {
    IgnitionOn,
    IgnitionOff,
    InitializationComplete,
    CommunicationLost,
    CriticalFault,
    ShutdownRequest,
    WatchdogTimeout,
    ShutdownComplete,
};

const char* toString(EventType type);

struct VehicleEvent {
    EventType type;
    // Optional free-text context for logging/diagnostics, e.g. which
    // sensor disagreed or which bus dropped out. Not interpreted by the
    // state machine itself.
    std::string detail;

    explicit VehicleEvent(EventType t, std::string d = "")
        : type(t), detail(std::move(d)) {}
};

} // namespace vehicle
