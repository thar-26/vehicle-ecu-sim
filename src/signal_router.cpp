#include "vehicle/signal_router.hpp"

namespace vehicle {

void SignalRouter::addSource(std::shared_ptr<ISignalSource> source) {
    sources_.push_back(std::move(source));
}

void SignalRouter::subscribe(const std::string& signalName, ISignalConsumer* consumer) {
    subscribers_[signalName].push_back(consumer);
}

std::size_t SignalRouter::pollOnce() {
    std::size_t dispatched = 0;

    for (auto& source : sources_) {
        std::optional<Signal> signal = source->receive();
        if (!signal.has_value()) {
            continue;
        }

        auto it = subscribers_.find(signal->name);
        if (it == subscribers_.end()) {
            continue; // no subscriber for this signal -- see header comment
        }

        for (ISignalConsumer* consumer : it->second) {
            consumer->onSignal(*signal);
            ++dispatched;
        }
    }

    return dispatched;
}

} // namespace vehicle
