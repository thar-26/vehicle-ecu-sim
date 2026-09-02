#include "vehicle/watchdog.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

using namespace vehicle;

TEST(WatchdogTest, NotExpiredImmediatelyAfterConstruction) {
    Watchdog wd(500);
    EXPECT_FALSE(wd.expired(0));
}

TEST(WatchdogTest, ExpiresAfterTimeoutWithoutKick) {
    Watchdog wd(500);
    EXPECT_TRUE(wd.expired(600));
}

TEST(WatchdogTest, KickResetsTheCountdown) {
    Watchdog wd(500);
    wd.kick(1000);
    EXPECT_FALSE(wd.expired(1300)); // 300ms since kick
    EXPECT_TRUE(wd.expired(1600));  // 600ms since kick
}

TEST(WatchdogTest, OnlyOneInstanceAllowedAtATime) {
    Watchdog wd(500);
    EXPECT_THROW(Watchdog second(500), std::runtime_error);
}
