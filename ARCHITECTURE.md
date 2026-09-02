# Architecture notes

This document explains *why* things are built the way they are, not
just what they do -- the README covers the "what".

## State machine: table-driven, not a switch statement

`VehicleStateMachine::nextState` looks up `(currentState, event)` in a
`constexpr std::array<Transition, N>` rather than branching on state in
a big `switch`. Two reasons:

1. The legal transitions become one readable table instead of logic
   scattered across case labels -- it doubles as documentation of the
   spec.
2. An event with no matching row is automatically rejected. There's no
   way to accidentally fall through to an unintended state, because
   "no match" is the default, not a case that has to be remembered.

The one exception is `ShuttingDown -> Off` on `ShutdownComplete`, which
is handled as a direct check rather than a table row, because it's the
one transition that's unconditional once a shutdown has been requested.

Rejected events are not errors -- most events aren't meaningful in most
states (e.g. `IgnitionOn` while already `Running`). They're recorded in
`history()` and passed to the observer anyway, so a caller can always
see what was *attempted*, not just what succeeded.

## Signal router: interfaces over concrete transports

`SignalRouter` depends only on `ISignalSource`/`ISignalConsumer`. It
has no idea whether a source is CAN, Ethernet, a replay file, or
`ScriptedSignalSource` (a queue-backed test double). That's what makes
it possible to unit test the router and the `SafetyMonitor` without any
real transport, and to swap one in later without touching this code.

## The false-positive bug this design has, and why the fix landed in scenario data, not in `SafetyMonitor`

Building the demo scenarios surfaced a real issue: `SafetyMonitor`'s
agreement check compares the current signal against the *last known*
value of its counterpart. Two sensors reporting the same physical
quantity almost never arrive on the same line/tick -- one signal
updates, then the other does. If the first update is a large jump
(e.g. a demo script setting speed straight from `0` to `42`), the
check fires the instant the first signal updates, comparing the new
value against the *stale* value of the one that hasn't been refreshed
yet. That produced a spurious `SensorDisagreement` fault in an
otherwise fault-free scenario the first time the demo was run end to
end.

This is not a demo-only quirk -- it's the actual behavior of comparing
two asynchronously-updated signals, and a real automotive system runs
into exactly this. The honest fixes are:

- **Debounce/confirmation counters**: only declare a fault after N
  consecutive violations, not the first one. This is standard practice
  and exactly the kind of thing `SafetyMonitor` explicitly does *not*
  implement (see README's "What this deliberately is not") -- adding
  it properly means deciding how many cycles, how that interacts with
  the timeout check, and testing the interaction, which is more scope
  than this project takes on.
- **Realistic scenario data**: physical quantities like vehicle speed
  don't jump instantly, so gradual, small-step changes (as the fixed
  `scenarios/normal_startup.txt` now uses) never produce a transient
  mismatch beyond tolerance in the first place.

This project takes the second option and says so here, rather than
quietly picking scenario numbers that happen to avoid the issue. The
first option is the right one for anything closer to production, and
is called out explicitly as a known gap.

## Watchdog: C HAL wrapped in C++, not reimplemented in C++

`c/watchdog.c` uses plain module-level (`static`) state rather than a
struct instance, because it's modeling one hardware watchdog
peripheral -- there is exactly one, and pretending otherwise (e.g.
letting it take a handle/instance pointer) would misrepresent what
real watchdog hardware looks like. The C++ side (`vehicle::Watchdog`)
is where instance-oriented, RAII behavior actually makes sense: it
enforces "only one live wrapper at a time" by throwing from the
constructor if the (global) C watchdog is already initialized, and
releases it in the destructor.

## Why C++17 rather than C++20/23

Kept to C++17 deliberately: it's what a large fraction of existing
automotive/embedded C++ codebases actually target (compiler and
toolchain support for later standards on embedded targets lags
significantly behind desktop/server use), and there was no feature
used here that needed anything newer.
