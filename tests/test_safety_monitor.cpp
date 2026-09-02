#include "vehicle/safety_monitor.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace vehicle;

TEST(SafetyMonitorTest, AgreeingSensorsRaiseNoFault) {
    SafetyMonitor monitor;
    monitor.registerAgreementCheck("speed_a", "speed_b", 5.0);
    std::vector<SafetyFault> faults;
    monitor.setFaultHandler([&faults](const SafetyFault& f) { faults.push_back(f); });

    monitor.onSignal(Signal{"speed_a", 70.0, 0});
    monitor.onSignal(Signal{"speed_b", 72.0, 0});

    EXPECT_TRUE(faults.empty());
}

TEST(SafetyMonitorTest, DisagreeingSensorsRaiseFault) {
    SafetyMonitor monitor;
    monitor.registerAgreementCheck("speed_a", "speed_b", 5.0);
    std::vector<SafetyFault> faults;
    monitor.setFaultHandler([&faults](const SafetyFault& f) { faults.push_back(f); });

    monitor.onSignal(Signal{"speed_a", 70.0, 0});
    monitor.onSignal(Signal{"speed_b", 150.0, 0});

    ASSERT_EQ(faults.size(), 1u);
    EXPECT_EQ(faults[0].type, SafetyFaultType::SensorDisagreement);
}

TEST(SafetyMonitorTest, NoFaultUntilBothSensorsHaveReportedAtLeastOnce) {
    SafetyMonitor monitor;
    monitor.registerAgreementCheck("speed_a", "speed_b", 5.0);
    std::vector<SafetyFault> faults;
    monitor.setFaultHandler([&faults](const SafetyFault& f) { faults.push_back(f); });

    monitor.onSignal(Signal{"speed_a", 999.0, 0}); // "B" side never reported
    EXPECT_TRUE(faults.empty());
}

TEST(SafetyMonitorTest, TimeoutNotFlaggedBeforeThreshold) {
    SafetyMonitor monitor;
    monitor.registerTimeoutCheck("vehicle_speed", 500);
    std::vector<SafetyFault> faults;
    monitor.setFaultHandler([&faults](const SafetyFault& f) { faults.push_back(f); });

    monitor.onSignal(Signal{"vehicle_speed", 50.0, 1000});
    monitor.checkTimeouts(1300); // 300ms elapsed, under the 500ms threshold

    EXPECT_TRUE(faults.empty());
}

TEST(SafetyMonitorTest, TimeoutFlaggedPastThreshold) {
    SafetyMonitor monitor;
    monitor.registerTimeoutCheck("vehicle_speed", 500);
    std::vector<SafetyFault> faults;
    monitor.setFaultHandler([&faults](const SafetyFault& f) { faults.push_back(f); });

    monitor.onSignal(Signal{"vehicle_speed", 50.0, 1000});
    monitor.checkTimeouts(1600); // 600ms elapsed, over the 500ms threshold

    ASSERT_EQ(faults.size(), 1u);
    EXPECT_EQ(faults[0].type, SafetyFaultType::SignalTimeout);
}

TEST(SafetyMonitorTest, NeverSeenSignalIsNotATimeout) {
    // A signal that has never arrived at all hasn't "timed out" -- it
    // just hasn't started. Distinguishes cold start from an actual
    // communication loss.
    SafetyMonitor monitor;
    monitor.registerTimeoutCheck("vehicle_speed", 500);
    std::vector<SafetyFault> faults;
    monitor.setFaultHandler([&faults](const SafetyFault& f) { faults.push_back(f); });

    monitor.checkTimeouts(100000);

    EXPECT_TRUE(faults.empty());
}
