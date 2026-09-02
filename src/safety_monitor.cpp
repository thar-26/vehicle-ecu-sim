#include "vehicle/safety_monitor.hpp"

#include <cmath>

namespace vehicle {

void SafetyMonitor::registerAgreementCheck(const std::string& signalA, const std::string& signalB, double tolerance) {
    agreementChecks_.push_back(AgreementCheck{signalA, signalB, tolerance});
}

void SafetyMonitor::registerTimeoutCheck(const std::string& signalName, std::uint64_t timeoutMs) {
    timeoutChecks_[signalName] = TimeoutCheck{timeoutMs, std::nullopt};
}

void SafetyMonitor::onSignal(const Signal& signal) {
    lastValues_[signal.name] = signal.value;

    auto timeoutIt = timeoutChecks_.find(signal.name);
    if (timeoutIt != timeoutChecks_.end()) {
        timeoutIt->second.lastSeenMs = signal.timestampMs;
    }

    for (const auto& check : agreementChecks_) {
        if (signal.name != check.signalA && signal.name != check.signalB) {
            continue;
        }
        const auto itA = lastValues_.find(check.signalA);
        const auto itB = lastValues_.find(check.signalB);
        if (itA == lastValues_.end() || itB == lastValues_.end()) {
            continue; // haven't observed both sides yet
        }
        const double diff = std::fabs(itA->second - itB->second);
        if (diff > check.tolerance) {
            raise(SafetyFaultType::SensorDisagreement,
                  check.signalA + " and " + check.signalB + " disagree by " + std::to_string(diff),
                  signal.timestampMs);
        }
    }
}

void SafetyMonitor::checkTimeouts(std::uint64_t nowMs) {
    for (auto& [name, check] : timeoutChecks_) {
        if (!check.lastSeenMs.has_value()) {
            continue; // never observed at all -- that's "not started", not "timed out"
        }
        const std::uint64_t last = *check.lastSeenMs;
        const std::uint64_t elapsed = (nowMs >= last) ? (nowMs - last) : 0;
        if (elapsed > check.timeoutMs) {
            raise(SafetyFaultType::SignalTimeout,
                  name + " not received for over " + std::to_string(check.timeoutMs) + "ms",
                  nowMs);
        }
    }
}

void SafetyMonitor::raise(SafetyFaultType type, const std::string& description, std::uint64_t timestampMs) {
    if (faultHandler_) {
        faultHandler_(SafetyFault{type, description, timestampMs});
    }
}

} // namespace vehicle
