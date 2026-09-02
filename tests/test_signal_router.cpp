#include "vehicle/signal_router.hpp"
#include "vehicle/scripted_signal_source.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

using namespace vehicle;

namespace {
class RecordingConsumer : public ISignalConsumer {
public:
    void onSignal(const Signal& signal) override { received.push_back(signal); }
    std::vector<Signal> received;
};
} // namespace

TEST(SignalRouterTest, RoutesSignalToSubscriber) {
    SignalRouter router;
    auto source = std::make_shared<ScriptedSignalSource>();
    source->push(Signal{"vehicle_speed", 42.0, 1000});
    router.addSource(source);

    RecordingConsumer consumer;
    router.subscribe("vehicle_speed", &consumer);

    EXPECT_EQ(router.pollOnce(), 1u);
    ASSERT_EQ(consumer.received.size(), 1u);
    EXPECT_DOUBLE_EQ(consumer.received[0].value, 42.0);
}

TEST(SignalRouterTest, UnsubscribedSignalIsDroppedNotCrashed) {
    SignalRouter router;
    auto source = std::make_shared<ScriptedSignalSource>();
    source->push(Signal{"unrouted_signal", 1.0, 0});
    router.addSource(source);

    EXPECT_EQ(router.pollOnce(), 0u);
}

TEST(SignalRouterTest, DispatchesToMultipleSubscribers) {
    SignalRouter router;
    auto source = std::make_shared<ScriptedSignalSource>();
    source->push(Signal{"brake_status", 1.0, 500});
    router.addSource(source);

    RecordingConsumer a, b;
    router.subscribe("brake_status", &a);
    router.subscribe("brake_status", &b);

    EXPECT_EQ(router.pollOnce(), 2u);
    EXPECT_EQ(a.received.size(), 1u);
    EXPECT_EQ(b.received.size(), 1u);
}

TEST(SignalRouterTest, OnlyDispatchesToMatchingSubscription) {
    SignalRouter router;
    auto source = std::make_shared<ScriptedSignalSource>();
    source->push(Signal{"vehicle_speed", 10.0, 0});
    router.addSource(source);

    RecordingConsumer wrongConsumer;
    router.subscribe("brake_status", &wrongConsumer);

    router.pollOnce();
    EXPECT_TRUE(wrongConsumer.received.empty());
}

TEST(SignalRouterTest, PollsMultipleSourcesInOneCall) {
    SignalRouter router;
    auto sourceA = std::make_shared<ScriptedSignalSource>();
    auto sourceB = std::make_shared<ScriptedSignalSource>();
    sourceA->push(Signal{"speed_a", 1.0, 0});
    sourceB->push(Signal{"speed_b", 2.0, 0});
    router.addSource(sourceA);
    router.addSource(sourceB);

    RecordingConsumer consumer;
    router.subscribe("speed_a", &consumer);
    router.subscribe("speed_b", &consumer);

    EXPECT_EQ(router.pollOnce(), 2u);
    EXPECT_EQ(consumer.received.size(), 2u);
}

TEST(SignalRouterTest, EmptySourceYieldsNoDispatch) {
    SignalRouter router;
    auto source = std::make_shared<ScriptedSignalSource>();
    router.addSource(source);

    EXPECT_EQ(router.pollOnce(), 0u);
}
