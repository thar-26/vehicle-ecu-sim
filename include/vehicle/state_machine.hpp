#pragma once

#include "vehicle/event.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace vehicle {

enum class VehicleState {
    Off,
    Booting,
    Running,
    Degraded,
    ShuttingDown,
};

const char* toString(VehicleState state);

// One accepted-or-rejected transition attempt, kept for history/logging.
struct TransitionRecord {
    VehicleState from;
    EventType event;
    VehicleState to;   // == from when the transition was rejected
    bool accepted;
};

// Explicit finite-state machine for vehicle-level operational state.
//
// Deliberately table-driven rather than a big switch/if-chain: the legal
// transitions live in one place (transitionTable(), state_machine.cpp)
// and can be read/audited independently of the dispatch logic. An event
// that has no entry for the current state is rejected -- state does not
// change, and the rejection is still recorded so callers can see it was
// attempted.
class VehicleStateMachine {
public:
    explicit VehicleStateMachine(VehicleState initial = VehicleState::Off);

    // Applies an event. Returns true if it caused an accepted state
    // transition, false if it was rejected (invalid for the current
    // state). Rejection is not an error -- plenty of events are simply
    // not meaningful in a given state (e.g. IgnitionOn while already
    // Running).
    bool handleEvent(const VehicleEvent& event);

    VehicleState currentState() const noexcept { return state_; }

    // Called for every handled event, accepted or rejected. Intended for
    // logging/diagnostics hookup, not for driving further logic.
    using TransitionObserver = std::function<void(const TransitionRecord&)>;
    void setObserver(TransitionObserver observer) { observer_ = std::move(observer); }

    const std::vector<TransitionRecord>& history() const noexcept { return history_; }

private:
    VehicleState state_;
    TransitionObserver observer_;
    std::vector<TransitionRecord> history_;

    static std::optional<VehicleState> nextState(VehicleState from, EventType event);
};

} // namespace vehicle
