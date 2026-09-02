#pragma once

#include <cstdint>
#include <string>

namespace vehicle {

// A decoded, named value. In a real system this would come out of a
// DBC-style signal database keyed by CAN ID; here decoding happens
// wherever a concrete ISignalSource produces its Signal objects, so the
// router itself never has to know about raw frame layout.
struct Signal {
    std::string name;
    double value;
    std::uint64_t timestampMs;
};

} // namespace vehicle
