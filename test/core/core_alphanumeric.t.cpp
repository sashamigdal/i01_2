#include <gtest/gtest.h>

#include <iostream>
#include <i01_core/Alphanumeric.hpp>

using i01::core::Alphanumeric;
template <int OUT_WIDTH>
using RAlphanumeric = Alphanumeric<OUT_WIDTH, false, false, '_'>;

TEST(core_alphanumeric, core_alphanumeric_test)
{
    I01_ASSERT_SIZE(Alphanumeric<14>, 14);
    Alphanumeric<3> a;
    ASSERT_EQ(a.set(62U), true);
    ASSERT_EQ(a.get<int>(), 62);
    ASSERT_LT(a.get<double>(), 62.001);
    ASSERT_GT(a.get<double>(), 61.999);
    Alphanumeric<11> b;
    b.set(1000000000000ULL);
    ASSERT_EQ(b.get<std::uint64_t>(), 1000000000000ULL);
    ASSERT_EQ((std::uint64_t)b.get<double>(), 1000000000000ULL);
    ASSERT_LT(b.get<double>(), 1000000000000.1);
    ASSERT_GT(b.get<double>(),  999999999999.9);
    
    I01_ASSERT_SIZE(RAlphanumeric<14>, 14);
    RAlphanumeric<3> c;
    ASSERT_EQ(c.set(62U), true);
    ASSERT_EQ(c.get<int>(), 62);
    ASSERT_LT(c.get<double>(), 62.001);
    ASSERT_GT(c.get<double>(), 61.999);
    RAlphanumeric<11> d;
    ASSERT_EQ(d.set(1000000000000ULL), true);
    ASSERT_EQ(d.get<std::uint64_t>(), 1000000000000ULL);
    ASSERT_EQ((std::uint64_t)d.get<double>(), 1000000000000ULL);
    ASSERT_LT(d.get<double>(), 1000000000000.1);
    ASSERT_GT(d.get<double>(),  999999999999.9);
}
