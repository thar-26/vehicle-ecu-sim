#include "vehicle/state_machine.hpp"

#include <array>

namespace vehicle {

const char* toString(EventType type) {
    switch (type) {
        case EventType::IgnitionOn:             return "IgnitionOn";
        case EventType::IgnitionOff:            return "IgnitionOff";
        case EventType::InitializationComplete: return "InitializationComplete";
        case EventType::CommunicationLost:      return "CommunicationLost";
        case EventType::CriticalFault:          return "CriticalFault";
        case EventType::ShutdownRequest:        return "ShutdownRequest";
        case EventType::WatchdogTimeout:        return "WatchdogTimeout";
        case EventType::ShutdownComplete:       return "ShutdownComplete";
    }
    return "UnknownEvent";
}

const char* toString(VehicleState state) {
    switch (state) {
        case VehicleState::Off:          return "Off";
        case VehicleState::Booting:      return "Booting";
        case VehicleState::Running:      return "Running";
        case VehicleState::Degraded:     return "Degraded";
        case VehicleState::ShuttingDown: return "ShuttingDown";
    }
    return "UnknownState";
}

namespace {

struct Transition {
    VehicleState from;
    EventType event;
    VehicleState to;
};

// The full set of legal transitions. Anything not listed here is
// rejected by nextState(). Read top to bottom, this is meant to double
// as the spec: "from state X, event Y always leads to state Z, and
// nothing else does".
constexpr std::array<Transition, 12> kTransitionTable{{
    {VehicleState::Off,          EventType::IgnitionOn,             VehicleState::Booting},

    {VehicleState::Booting,      EventType::InitializationComplete, VehicleState::Running},
    {VehicleState::Booting,      EventType::CriticalFault,          VehicleState::Degraded},
    {VehicleState::Booting,      EventType::CommunicationLost,      VehicleState::Degraded},
    {VehicleState::Booting,      EventType::IgnitionOff,            VehicleState::ShuttingDown},

    {VehicleState::Running,      EventType::CriticalFault,          VehicleState::Degraded},
    {VehicleState::Running,      EventType::CommunicationLost,      VehicleState::Degraded},
    {VehicleState::Running,      EventType::WatchdogTimeout,        VehicleState::Degraded},
    {VehicleState::Running,      EventType::IgnitionOff,            VehicleState::ShuttingDown},
    {VehicleState::Running,      EventType::ShutdownRequest,        VehicleState::ShuttingDown},

    {VehicleState::Degraded,     EventType::ShutdownRequest,        VehicleState::ShuttingDown},
    {VehicleState::Degraded,     EventType::IgnitionOff,            VehicleState::ShuttingDown},
}};

} // namespace

VehicleStateMachine::VehicleStateMachine(VehicleState initial) : state_(initial) {}

std::optional<VehicleState> VehicleStateMachine::nextState(VehicleState from, EventType event) {
    for (const auto& t : kTransitionTable) {
        if (t.from == from && t.event == event) {
            return t.to;
        }
    }
    // ShuttingDown -> Off is handled as a special case rather than a
    // table row: it is the one transition that is unconditional once
    // requested (a shutdown in progress always ends at Off).
    if (from == VehicleState::ShuttingDown && event == EventType::ShutdownComplete) {
        return VehicleState::Off;
    }
    return std::nullopt;
}

bool VehicleStateMachine::handleEvent(const VehicleEvent& event) {
    const auto next = nextState(state_, event.type);
    const VehicleState from = state_;
    const bool accepted = next.has_value();

    if (accepted) {
        state_ = *next;
    }

    const TransitionRecord record{from, event.type, accepted ? *next : from, accepted};
    history_.push_back(record);
    if (observer_) {
        observer_(record);
    }
    return accepted;
}

} // namespace vehicle
