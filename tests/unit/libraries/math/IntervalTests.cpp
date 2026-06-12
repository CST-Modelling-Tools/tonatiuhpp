#include <gtest/gtest.h>

#include "libraries/math/1D/Interval.h"

TEST(IntervalTest, OrdersConstructorLimits)
{
    const Interval interval(4.0, -2.0);

    EXPECT_TRUE(interval.isValid());
    EXPECT_DOUBLE_EQ(interval.min(), -2.0);
    EXPECT_DOUBLE_EQ(interval.max(), 4.0);
    EXPECT_DOUBLE_EQ(interval.size(), 6.0);
    EXPECT_DOUBLE_EQ(interval.mid(), 1.0);
}

TEST(IntervalTest, ExpandsToContainPoints)
{
    Interval interval;

    EXPECT_FALSE(interval.isValid());

    interval.expand(3.0);
    interval.expand(-1.0);

    EXPECT_TRUE(interval.isValid());
    EXPECT_DOUBLE_EQ(interval.min(), -1.0);
    EXPECT_DOUBLE_EQ(interval.max(), 3.0);
    EXPECT_TRUE(interval.isInside(-1.0));
    EXPECT_TRUE(interval.isInside(3.0));
    EXPECT_FALSE(interval.isInside(3.5));
}

TEST(IntervalTest, NormalizesAndRestoresValues)
{
    const Interval interval(10.0, 20.0);

    EXPECT_DOUBLE_EQ(interval.toNormalized(15.0), 0.5);
    EXPECT_DOUBLE_EQ(interval.fromNormalized(0.25), 12.5);
    EXPECT_DOUBLE_EQ(interval.fromNormalized(interval.toNormalized(18.0)), 18.0);
}
