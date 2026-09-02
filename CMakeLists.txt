cmake_minimum_required(VERSION 3.16)
project(vehicle_ecu_sim CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Warnings-as-errors on the project's own code. Kept deliberately simple
# (no clang-tidy/cppcheck wiring) rather than pretending this has a full
# static-analysis pipeline behind it.
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# ---- core library -----------------------------------------------------
add_library(vehicle_core
    src/state_machine.cpp
    src/signal_router.cpp
    src/safety_monitor.cpp
    src/watchdog_wrapper.cpp
    c/watchdog.c
)
target_include_directories(vehicle_core PUBLIC include)
target_include_directories(vehicle_core PRIVATE c)

# ---- scenario runner CLI ----------------------------------------------
add_executable(vehicle_sim src/main.cpp)
target_link_libraries(vehicle_sim PRIVATE vehicle_core)

# ---- tests --------------------------------------------------------------
option(BUILD_TESTING_VEHICLE_SIM "Build the GoogleTest suite" ON)
if(BUILD_TESTING_VEHICLE_SIM)
  enable_testing()
  find_package(GTest REQUIRED)

  add_executable(vehicle_tests
      tests/test_state_machine.cpp
      tests/test_signal_router.cpp
      tests/test_safety_monitor.cpp
      tests/test_watchdog.cpp
  )
  target_link_libraries(vehicle_tests PRIVATE vehicle_core GTest::gtest GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(vehicle_tests)
endif()
