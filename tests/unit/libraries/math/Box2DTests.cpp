#include <gtest/gtest.h>

#include "libraries/math/2D/Box2D.h"

namespace
{
void ExpectVec2dEq(const vec2d& actual, const vec2d& expected)
{
    EXPECT_DOUBLE_EQ(actual.x, expected.x);
    EXPECT_DOUBLE_EQ(actual.y, expected.y);
}
}

TEST(Box2DTest, OrdersConstructorLimitsPerAxis)
{
    const Box2D box(vec2d(4.0, -3.0), vec2d(-2.0, 5.0));

    EXPECT_TRUE(box.isValid());
    ExpectVec2dEq(box.min(), vec2d(-2.0, -3.0));
    ExpectVec2dEq(box.max(), vec2d(4.0, 5.0));
    ExpectVec2dEq(box.size(), vec2d(6.0, 8.0));
    ExpectVec2dEq(box.center(), vec2d(1.0, 1.0));
    EXPECT_DOUBLE_EQ(box.area(), 48.0);
}

TEST(Box2DTest, ExpandsToContainPointsAndBoxes)
{
    Box2D box;

    EXPECT_FALSE(box.isValid());

    box.expand(vec2d(2.0, -1.0));
    box.expand(vec2d(-4.0, 3.0));
    box.expand(Box2D(vec2d(-5.0, -2.0), vec2d(1.0, 4.0)));

    EXPECT_TRUE(box.isValid());
    ExpectVec2dEq(box.min(), vec2d(-5.0, -2.0));
    ExpectVec2dEq(box.max(), vec2d(2.0, 4.0));
    EXPECT_TRUE(box.isInside(vec2d(-5.0, 4.0)));
    EXPECT_FALSE(box.isInside(vec2d(2.5, 0.0)));
}

TEST(Box2DTest, NormalizesAndRestoresCoordinates)
{
    const Box2D box(vec2d(10.0, -2.0), vec2d(20.0, 6.0));

    ExpectVec2dEq(box.toNormalized(vec2d(15.0, 2.0)), vec2d(0.5, 0.5));
    ExpectVec2dEq(box.fromNormalized(vec2d(0.25, 0.75)), vec2d(12.5, 4.0));
    ExpectVec2dEq(box.fromNormalized(box.toNormalized(vec2d(18.0, 1.0))), vec2d(18.0, 1.0));
}
