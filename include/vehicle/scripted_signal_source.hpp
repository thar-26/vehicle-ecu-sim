#pragma once

#include "vehicle/signal_source.hpp"

#include <deque>

namespace vehicle {

// A deterministic ISignalSource backed by a queue of pre-loaded signals.
// Used by the scenario runner (src/main.cpp) and by the unit tests --
// it stands in for "a real transport" without needing an actual CAN
// adapter or a mocking framework.
class ScriptedSignalSource : public ISignalSource {
public:
    void push(Signal signal) { queue_.push_back(std::move(signal)); }

    std::optional<Signal> receive() override {
        if (queue_.empty()) {
            return std::nullopt;
        }
        Signal front = std::move(queue_.front());
        queue_.pop_front();
        return front;
    }

    bool empty() const noexcept { return queue_.empty(); }

private:
    std::deque<Signal> queue_;
};

} // namespace vehicle
