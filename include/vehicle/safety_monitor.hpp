#pragma once

#include "vehicle/signal_source.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vehicle {

enum class SafetyFaultType {
    SensorDisagreement,
    SignalTimeout,
};

struct SafetyFault {
    SafetyFaultType type;
    std::string description;
    std::uint64_t timestampMs;
};

// Two narrowly-scoped checks that are common building blocks in
// automotive signal processing: (1) do two sensors reporting the same
// physical quantity agree within tolerance, and (2) has an expected
// periodic signal gone quiet longer than its timeout.
//
// This is NOT a safety case, an ISO 26262 implementation, or anything
// certifiable. It's an honestly-scoped demonstration of the pattern --
// see README.md for what it deliberately does not attempt (voting
// across more than two sensors, fault confirmation/debounce counters,
// ASIL decomposition, etc).
class SafetyMonitor : public ISignalConsumer {
public:
    using FaultHandler = std::function<void(const SafetyFault&)>;

    // Flags a fault when |signalA - signalB| exceeds `tolerance`, once
    // both signals have been observed at least once.
    void registerAgreementCheck(const std::string& signalA, const std::string& signalB, double tolerance);

    // Flags a fault when `signalName` hasn't been observed for more
    // than `timeoutMs`. Time is only advanced by explicit calls to
    // checkTimeouts() -- this class has no clock of its own, which
    // keeps it deterministic and easy to unit test.
    void registerTimeoutCheck(const std::string& signalName, std::uint64_t timeoutMs);

    void setFaultHandler(FaultHandler handler) { faultHandler_ = std::move(handler); }

    // ISignalConsumer
    void onSignal(const Signal& signal) override;

    // Must be called periodically (e.g. once per SignalRouter::pollOnce
    // cycle) with the current time, so timeout checks that depend on
    // elapsed time -- not just on new signals arriving -- actually fire.
    void checkTimeouts(std::uint64_t nowMs);

private:
    struct AgreementCheck {
        std::string signalA;
        std::string signalB;
        double tolerance;
    };
    struct TimeoutCheck {
        std::uint64_t timeoutMs;
        std::optional<std::uint64_t> lastSeenMs;
    };

    void raise(SafetyFaultType type, const std::string& description, std::uint64_t timestampMs);

    std::vector<AgreementCheck> agreementChecks_;
    std::unordered_map<std::string, TimeoutCheck> timeoutChecks_;
    std::unordered_map<std::string, double> lastValues_;
    FaultHandler faultHandler_;
};

} // namespace vehicle
