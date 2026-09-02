#pragma once

#include "vehicle/signal_source.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace vehicle {

// Routes signals, by name, from any number of ISignalSources to any
// number of ISignalConsumers. Deliberately source-agnostic: swapping a
// real CAN adapter for a scripted test source (see
// ScriptedSignalSource) requires no change here.
class SignalRouter {
public:
    void addSource(std::shared_ptr<ISignalSource> source);
    void subscribe(const std::string& signalName, ISignalConsumer* consumer);

    // Drains every registered source once (one receive() call each) and
    // dispatches whatever came back to the consumers subscribed to that
    // signal's name. A signal with no subscriber is silently dropped --
    // that's a deliberate choice (unrouted signals are common and not,
    // by themselves, a fault) but it does mean a typo'd subscription
    // name fails silently; see the README for that trade-off.
    // Returns the number of (signal, consumer) dispatches performed.
    std::size_t pollOnce();

private:
    std::vector<std::shared_ptr<ISignalSource>> sources_;
    std::unordered_map<std::string, std::vector<ISignalConsumer*>> subscribers_;
};

} // namespace vehicle
