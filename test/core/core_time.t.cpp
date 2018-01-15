#include <gtest/gtest.h>

#include <string.h>
#include <unistd.h>

#include <iostream>

#include <i01_core/Time.hpp>

TEST(core_time, core_time_test)
{
    using i01::core::Timestamp;
    Timestamp t1, t2;
    EXPECT_TRUE(Timestamp::now(t1));
    ::usleep(1000);
    EXPECT_TRUE(Timestamp::now(t2));
    EXPECT_TRUE(t1 < t2);
    EXPECT_TRUE(t1 <= t2);
    EXPECT_FALSE(t1 > t2);
    EXPECT_FALSE(t1 == t2);
    EXPECT_FALSE(t1 >= t2);
    EXPECT_EQ((t1.tv_sec * 1000ULL + t1.tv_nsec / 1000000ULL) % 86400000, t1.milliseconds_since_midnight());
}
