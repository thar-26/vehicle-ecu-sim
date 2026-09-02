# Vehicle ECU Simulator

A small C++17 / C portfolio project modeling a few core patterns from
automotive ECU software: an explicit vehicle state machine, a
transport-agnostic signal router, two narrowly-scoped safety checks
(sensor plausibility and signal timeout monitoring), and a plain-C
watchdog HAL wrapped in idiomatic C++.

It was built to learn and demonstrate these patterns concretely, as
part of preparing for embedded/automotive software roles. It is
**not** a production system and makes no claim to be one.

## What this is

- **`VehicleStateMachine`** (`include/vehicle/state_machine.hpp`) --
  an explicit, table-driven finite-state machine for vehicle-level
  operational state (`Off -> Booting -> Running`, with fault paths to
  `Degraded` and a `ShuttingDown` state before returning to `Off`).
  Invalid transitions are rejected, not silently ignored -- every
  attempt (accepted or rejected) is recorded and observable.
- **`SignalRouter`** (`include/vehicle/signal_router.hpp`) -- routes
  named signals from any `ISignalSource` to any number of
  `ISignalConsumer`s. The router has no idea whether a source is a CAN
  adapter, a replay file, or a test double (`ScriptedSignalSource`) --
  that's the point of depending on the interface.
- **`SafetyMonitor`** (`include/vehicle/safety_monitor.hpp`) -- two
  patterns common in automotive signal processing: flagging when two
  sensors reporting the same physical quantity disagree beyond a
  tolerance, and flagging when an expected periodic signal goes quiet
  past its timeout.
- **Watchdog** (`c/watchdog.{h,c}` + `include/vehicle/watchdog.hpp`) --
  a plain-C watchdog HAL (global state, modeling a single hardware
  peripheral) wrapped in an RAII C++ class, to demonstrate the
  C/C++ interop `extern "C"` pattern.
- **Scenario runner** (`src/main.cpp`) -- a small CLI that reads a text
  script (see `scenarios/`) and drives all of the above together, so
  the components can be seen working end to end without real CAN
  hardware.

See `ARCHITECTURE.md` for the reasoning behind these design choices,
including a false-positive bug found and fixed while building the demo
scenarios -- a real example of the kind of timing subtlety this pattern
runs into.

## What this deliberately is *not*

Being upfront about scope, because overstating it would undercut the
project rather than help it:

- **Not an AUTOSAR Classic or Adaptive implementation.** The
  source/router/consumer layering is *inspired by* the idea of
  depending on interfaces rather than concrete transports, which is a
  theme in AUTOSAR Adaptive's service-oriented design. It is not an
  AUTOSAR stack, doesn't use ARXML, and isn't compatible with either
  AUTOSAR platform.
- **Not an ISO 26262 safety case.** `SafetyMonitor` implements two
  individual check patterns (plausibility, timeout) that show up in
  real safety-relevant code. It has no ASIL decomposition, no fault
  confirmation/debounce counters, no diagnostic coverage analysis, and
  makes no certification claim.
- **No multi-process IPC and no real CAN bus.** Everything runs in one
  process against scripted/mock data. A real ECU platform would split
  these into services communicating over IPC with reconnection and
  timeout handling -- that's a materially bigger project, deliberately
  left out here.
- **No static-analysis pipeline.** CI runs a real build with
  `-Wall -Wextra -Wpedantic -Werror` and the test suite; there's no
  clang-tidy/cppcheck/coverage tooling wired up.

## Build & run

Requires CMake >= 3.16, a C++17 compiler, and GoogleTest development
files (`libgtest-dev` on Debian/Ubuntu).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

# run the test suite
ctest --test-dir build --output-on-failure

# run a scenario
./build/vehicle_sim scenarios/normal_startup.txt
./build/vehicle_sim scenarios/sensor_fault.txt
./build/vehicle_sim scenarios/communication_loss.txt
```

Example output (`sensor_fault.txt`):

```
[STATE] initial: Off
[STATE] Off -> Booting  (IgnitionOn)
[STATE] Booting -> Running  (InitializationComplete)
[FAULT] vehicle_speed_a and vehicle_speed_b disagree by 84.000000
[STATE] Running -> Degraded  (CriticalFault)
[STATE] Degraded -> ShuttingDown  (ShutdownRequest)
[STATE] ShuttingDown -> Off  (ShutdownComplete)
[DONE]  final state: Off
```

## Project layout

```
vehicle-ecu-sim/
├── include/vehicle/     public C++ headers (the library's API)
├── src/                 C++ implementation + scenario runner (main.cpp)
├── c/                   plain-C watchdog HAL
├── tests/                GoogleTest suite (28 tests)
├── scenarios/            text scripts for the scenario runner
├── .github/workflows/    CI: build with warnings-as-errors + run tests
├── CMakeLists.txt
└── ARCHITECTURE.md       design notes and trade-offs
```

## Testing

28 GoogleTest cases covering the state machine's transition table
(including rejected transitions and history/observer behavior), the
signal router's dispatch logic, the safety monitor's two check types,
and the watchdog wrapper -- run via `ctest`. Coverage is aimed at
actual behaviors, not a target count.
