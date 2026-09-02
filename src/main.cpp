// Scenario runner: reads a small text script (see scenarios/*.txt) and
// drives the state machine, signal router, safety monitor and watchdog
// together, printing what happens. This is the "demo" entry point --
// not a real vehicle interface, just a way to see the components work
// end to end without needing an actual CAN bus.
//
// Scenario file commands, one per line (# starts a comment):
//   EVENT <EventName> [free-text detail...]   feed an event to the state machine
//   SIGNAL <name> <value> <timestampMs>       feed a signal through the router
//   KICK <timestampMs>                        kick the watchdog
//   TIME <timestampMs>                        advance "now": checks signal
//                                              timeouts and watchdog expiry

#include "vehicle/safety_monitor.hpp"
#include "vehicle/scripted_signal_source.hpp"
#include "vehicle/signal_router.hpp"
#include "vehicle/state_machine.hpp"
#include "vehicle/watchdog.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace vehicle;

namespace {

std::optional<EventType> parseEvent(const std::string& name) {
    static const std::unordered_map<std::string, EventType> table{
        {"IgnitionOn", EventType::IgnitionOn},
        {"IgnitionOff", EventType::IgnitionOff},
        {"InitializationComplete", EventType::InitializationComplete},
        {"CommunicationLost", EventType::CommunicationLost},
        {"CriticalFault", EventType::CriticalFault},
        {"ShutdownRequest", EventType::ShutdownRequest},
        {"WatchdogTimeout", EventType::WatchdogTimeout},
        {"ShutdownComplete", EventType::ShutdownComplete},
    };
    const auto it = table.find(name);
    return it == table.end() ? std::nullopt : std::make_optional(it->second);
}

void printTransition(const TransitionRecord& record) {
    if (record.accepted) {
        std::cout << "[STATE] " << toString(record.from) << " -> " << toString(record.to)
                  << "  (" << toString(record.event) << ")\n";
    } else {
        std::cout << "[WARN]  rejected " << toString(record.event) << " while in "
                  << toString(record.from) << "\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <scenario-file>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "could not open scenario file: " << argv[1] << "\n";
        return 1;
    }

    VehicleStateMachine stateMachine;
    stateMachine.setObserver(printTransition);

    SignalRouter router;
    auto source = std::make_shared<ScriptedSignalSource>();
    router.addSource(source);

    SafetyMonitor safety;
    safety.registerAgreementCheck("vehicle_speed_a", "vehicle_speed_b", 5.0);
    safety.registerTimeoutCheck("vehicle_speed_a", 500);
    safety.setFaultHandler([&stateMachine](const SafetyFault& fault) {
        std::cout << "[FAULT] " << fault.description << "\n";
        stateMachine.handleEvent(VehicleEvent{EventType::CriticalFault, fault.description});
    });
    router.subscribe("vehicle_speed_a", &safety);
    router.subscribe("vehicle_speed_b", &safety);

    // 1000ms is a generous demo timeout; a real ECU would size this to
    // its actual main-loop period, typically single-digit milliseconds.
    Watchdog watchdog(1000);

    std::cout << "[STATE] initial: " << toString(stateMachine.currentState()) << "\n";

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "EVENT") {
            std::string eventName;
            iss >> eventName;
            std::string detail;
            std::getline(iss, detail);
            if (!detail.empty() && detail.front() == ' ') {
                detail.erase(0, 1);
            }

            const auto type = parseEvent(eventName);
            if (!type) {
                std::cerr << "[ERROR] unknown event: " << eventName << "\n";
                continue;
            }
            stateMachine.handleEvent(VehicleEvent{*type, detail});

        } else if (command == "SIGNAL") {
            std::string name;
            double value = 0.0;
            std::uint64_t timestampMs = 0;
            iss >> name >> value >> timestampMs;
            source->push(Signal{name, value, timestampMs});
            router.pollOnce();

        } else if (command == "KICK") {
            std::uint64_t nowMs = 0;
            iss >> nowMs;
            watchdog.kick(nowMs);

        } else if (command == "TIME") {
            std::uint64_t nowMs = 0;
            iss >> nowMs;
            safety.checkTimeouts(nowMs);
            if (watchdog.expired(nowMs) && stateMachine.currentState() == VehicleState::Running) {
                stateMachine.handleEvent(VehicleEvent{EventType::WatchdogTimeout, "no kick within timeout"});
            }

        } else {
            std::cerr << "[ERROR] unknown command: " << command << "\n";
        }
    }

    std::cout << "[DONE]  final state: " << toString(stateMachine.currentState()) << "\n";
    return 0;
}
