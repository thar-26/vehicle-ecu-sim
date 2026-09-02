#include "vehicle/state_machine.hpp"

#include <gtest/gtest.h>

using namespace vehicle;

TEST(StateMachineTest, StartsInOffState) {
    VehicleStateMachine sm;
    EXPECT_EQ(sm.currentState(), VehicleState::Off);
}

TEST(StateMachineTest, IgnitionOnFromOffEntersBooting) {
    VehicleStateMachine sm;
    EXPECT_TRUE(sm.handleEvent(VehicleEvent{EventType::IgnitionOn}));
    EXPECT_EQ(sm.currentState(), VehicleState::Booting);
}

TEST(StateMachineTest, DirectOffToRunningIsRejected) {
    VehicleStateMachine sm;
    EXPECT_FALSE(sm.handleEvent(VehicleEvent{EventType::InitializationComplete}));
    EXPECT_EQ(sm.currentState(), VehicleState::Off);
}

TEST(StateMachineTest, FullStartupSequenceReachesRunning) {
    VehicleStateMachine sm;
    ASSERT_TRUE(sm.handleEvent(VehicleEvent{EventType::IgnitionOn}));
    ASSERT_TRUE(sm.handleEvent(VehicleEvent{EventType::InitializationComplete}));
    EXPECT_EQ(sm.currentState(), VehicleState::Running);
}

TEST(StateMachineTest, CriticalFaultWhileRunningEntersDegraded) {
    VehicleStateMachine sm;
    sm.handleEvent(VehicleEvent{EventType::IgnitionOn});
    sm.handleEvent(VehicleEvent{EventType::InitializationComplete});
    ASSERT_TRUE(sm.handleEvent(VehicleEvent{EventType::CriticalFault, "brake sensor disagreement"}));
    EXPECT_EQ(sm.currentState(), VehicleState::Degraded);
}

TEST(StateMachineTest, WatchdogTimeoutWhileBootingIsRejected) {
    // WatchdogTimeout only has a defined transition from Running in this
    // table -- during Booting the watchdog isn't being monitored yet.
    VehicleStateMachine sm;
    sm.handleEvent(VehicleEvent{EventType::IgnitionOn});
    EXPECT_FALSE(sm.handleEvent(VehicleEvent{EventType::WatchdogTimeout}));
    EXPECT_EQ(sm.currentState(), VehicleState::Booting);
}

TEST(StateMachineTest, WatchdogTimeoutWhileRunningEntersDegraded) {
    VehicleStateMachine sm;
    sm.handleEvent(VehicleEvent{EventType::IgnitionOn});
    sm.handleEvent(VehicleEvent{EventType::InitializationComplete});
    EXPECT_TRUE(sm.handleEvent(VehicleEvent{EventType::WatchdogTimeout}));
    EXPECT_EQ(sm.currentState(), VehicleState::Degraded);
}

TEST(StateMachineTest, ShutdownSequenceReturnsToOff) {
    VehicleStateMachine sm;
    sm.handleEvent(VehicleEvent{EventType::IgnitionOn});
    sm.handleEvent(VehicleEvent{EventType::InitializationComplete});
    ASSERT_TRUE(sm.handleEvent(VehicleEvent{EventType::ShutdownRequest}));
    EXPECT_EQ(sm.currentState(), VehicleState::ShuttingDown);
    ASSERT_TRUE(sm.handleEvent(VehicleEvent{EventType::ShutdownComplete}));
    EXPECT_EQ(sm.currentState(), VehicleState::Off);
}

TEST(StateMachineTest, DegradedCanStillShutDown) {
    VehicleStateMachine sm;
    sm.handleEvent(VehicleEvent{EventType::IgnitionOn});
    sm.handleEvent(VehicleEvent{EventType::InitializationComplete});
    sm.handleEvent(VehicleEvent{EventType::CriticalFault});
    ASSERT_EQ(sm.currentState(), VehicleState::Degraded);
    EXPECT_TRUE(sm.handleEvent(VehicleEvent{EventType::IgnitionOff}));
    EXPECT_EQ(sm.currentState(), VehicleState::ShuttingDown);
}

TEST(StateMachineTest, RejectedTransitionsAreRecordedInHistory) {
    VehicleStateMachine sm;
    sm.handleEvent(VehicleEvent{EventType::InitializationComplete}); // rejected from Off
    ASSERT_EQ(sm.history().size(), 1u);
    EXPECT_FALSE(sm.history()[0].accepted);
    EXPECT_EQ(sm.history()[0].from, VehicleState::Off);
    EXPECT_EQ(sm.history()[0].to, VehicleState::Off); // unchanged on rejection
}

TEST(StateMachineTest, ObserverIsCalledForEveryEventAcceptedOrNot) {
    VehicleStateMachine sm;
    int callCount = 0;
    sm.setObserver([&callCount](const TransitionRecord&) { ++callCount; });
    sm.handleEvent(VehicleEvent{EventType::IgnitionOn});      // accepted
    sm.handleEvent(VehicleEvent{EventType::IgnitionOn});      // rejected, already Booting
    EXPECT_EQ(callCount, 2);
}

TEST(StateMachineTest, ToStringCoversStatesAndEvents) {
    EXPECT_STREQ(toString(VehicleState::Off), "Off");
    EXPECT_STREQ(toString(VehicleState::Running), "Running");
    EXPECT_STREQ(toString(EventType::CriticalFault), "CriticalFault");
}
