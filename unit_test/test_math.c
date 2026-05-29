#pragma once
#include "test.h"
#include "test_func.h"
#include "naui/math/math.h"
#include "naui/math/vec2.h"
#include "naui/math/vec3.h"
#include "naui/math/vec4.h"

#define FLOAT_EQ(a,b) (fabsf((a)-(b)) < 1e-6f)

static void test_vec2(void)
{
    TEST_BEGIN("Naui_Vec2");

    Naui_Vec2 v1 = {1.0f, -2.0f};
    Naui_Vec2 v2 = {-3.0f, 4.0f};

    Naui_Vec2 add = naui_vec2_add(v1, v2);
    ASSERT(FLOAT_EQ(add.x, -2.0f) && FLOAT_EQ(add.y, 2.0f));

    Naui_Vec2 sub = naui_vec2_sub(v1, v2);
    ASSERT(FLOAT_EQ(sub.x, 4.0f) && FLOAT_EQ(sub.y, -6.0f));

    Naui_Vec2 scale = naui_vec2_scale(v1, 2.0f);
    ASSERT(FLOAT_EQ(scale.x, 2.0f) && FLOAT_EQ(scale.y, -4.0f));

    Naui_Vec2 absv = naui_vec2_abs(v1);
    ASSERT(FLOAT_EQ(absv.x, 1.0f) && FLOAT_EQ(absv.y, 2.0f));

    Naui_Vec2 neg = naui_vec2_negate(v1);
    ASSERT(FLOAT_EQ(neg.x, -1.0f) && FLOAT_EQ(neg.y, 2.0f));

    Naui_Vec2 minv = naui_vec2_min(v1, v2);
    ASSERT(FLOAT_EQ(minv.x, -3.0f) && FLOAT_EQ(minv.y, -2.0f));

    Naui_Vec2 maxv = naui_vec2_max(v1, v2);
    ASSERT(FLOAT_EQ(maxv.x, 1.0f) && FLOAT_EQ(maxv.y, 4.0f));

    Naui_Vec2 lerp1 = naui_vec2_lerp(v1, v2, 0.0f);
    ASSERT(FLOAT_EQ(lerp1.x, v1.x) && FLOAT_EQ(lerp1.y, v1.y));

    Naui_Vec2 lerp2 = naui_vec2_lerp(v1, v2, 1.0f);
    ASSERT(FLOAT_EQ(lerp2.x, v2.x) && FLOAT_EQ(lerp2.y, v2.y));

    Naui_Vec2 lerpHalf = naui_vec2_lerp(v1, v2, 0.5f);
    ASSERT(FLOAT_EQ(lerpHalf.x, -1.0f) && FLOAT_EQ(lerpHalf.y, 1.0f));

    float len = naui_vec2_length(v1);
    ASSERT(FLOAT_EQ(len, sqrtf(5.0f)));

    float dist = naui_vec2_distance(v1, v2);
    ASSERT(FLOAT_EQ(dist, sqrtf(52.0f)));

    float dot = naui_vec2_dot(v1, v2);
    ASSERT(FLOAT_EQ(dot, -11.0f));

    TEST_END();
}

static void test_vec3(void)
{
    TEST_BEGIN("Naui_Vec3");

    Naui_Vec3 v1 = {1.0f, -2.0f, 3.0f};
    Naui_Vec3 v2 = {-3.0f, 4.0f, -5.0f};

    Naui_Vec3 add = naui_vec3_add(v1, v2);
    ASSERT(FLOAT_EQ(add.x, -2.0f) && FLOAT_EQ(add.y, 2.0f) && FLOAT_EQ(add.z, -2.0f));

    Naui_Vec3 sub = naui_vec3_sub(v1, v2);
    ASSERT(FLOAT_EQ(sub.x, 4.0f) && FLOAT_EQ(sub.y, -6.0f) && FLOAT_EQ(sub.z, 8.0f));

    Naui_Vec3 scale = naui_vec3_scale(v1, 2.0f);
    ASSERT(FLOAT_EQ(scale.x, 2.0f) && FLOAT_EQ(scale.y, -4.0f) && FLOAT_EQ(scale.z, 6.0f));

    Naui_Vec3 absv = naui_vec3_abs(v1);
    ASSERT(FLOAT_EQ(absv.x, 1.0f) && FLOAT_EQ(absv.y, 2.0f) && FLOAT_EQ(absv.z, 3.0f));

    Naui_Vec3 neg = naui_vec3_negate(v1);
    ASSERT(FLOAT_EQ(neg.x, -1.0f) && FLOAT_EQ(neg.y, 2.0f) && FLOAT_EQ(neg.z, -3.0f));

    Naui_Vec3 minv = naui_vec3_min(v1, v2);
    ASSERT(FLOAT_EQ(minv.x, -3.0f) && FLOAT_EQ(minv.y, -2.0f) && FLOAT_EQ(minv.z, -5.0f));

    Naui_Vec3 maxv = naui_vec3_max(v1, v2);
    ASSERT(FLOAT_EQ(maxv.x, 1.0f) && FLOAT_EQ(maxv.y, 4.0f) && FLOAT_EQ(maxv.z, 3.0f));

    Naui_Vec3 lerpHalf = naui_vec3_lerp(v1, v2, 0.5f);
    ASSERT(FLOAT_EQ(lerpHalf.x, -1.0f) && FLOAT_EQ(lerpHalf.y, 1.0f) && FLOAT_EQ(lerpHalf.z, -1.0f));

    float len = naui_vec3_length(v1);
    ASSERT(FLOAT_EQ(len, sqrtf(14.0f)));

    float dist = naui_vec3_distance(v1, v2);
    ASSERT(FLOAT_EQ(dist, sqrtf(116.0f)));

    float dot = naui_vec3_dot(v1, v2);
    ASSERT(FLOAT_EQ(dot, -26.0f));

    TEST_END();
}

static void test_vec4(void)
{
    TEST_BEGIN("Naui_Vec4");

    Naui_Vec4 v1 = {1.0f, -2.0f, 3.0f, -4.0f};
    Naui_Vec4 v2 = {-3.0f, 4.0f, -5.0f, 6.0f};

    Naui_Vec4 add = naui_vec4_add(v1, v2);
    ASSERT(FLOAT_EQ(add.x, -2.0f) && FLOAT_EQ(add.y, 2.0f) && FLOAT_EQ(add.z, -2.0f) && FLOAT_EQ(add.w, 2.0f));

    Naui_Vec4 sub = naui_vec4_sub(v1, v2);
    ASSERT(FLOAT_EQ(sub.x, 4.0f) && FLOAT_EQ(sub.y, -6.0f) && FLOAT_EQ(sub.z, 8.0f) && FLOAT_EQ(sub.w, -10.0f));

    Naui_Vec4 scale = naui_vec4_scale(v1, 2.0f);
    ASSERT(FLOAT_EQ(scale.x, 2.0f) && FLOAT_EQ(scale.y, -4.0f) && FLOAT_EQ(scale.z, 6.0f) && FLOAT_EQ(scale.w, -8.0f));

    Naui_Vec4 absv = naui_vec4_abs(v1);
    ASSERT(FLOAT_EQ(absv.x, 1.0f) && FLOAT_EQ(absv.y, 2.0f) && FLOAT_EQ(absv.z, 3.0f) && FLOAT_EQ(absv.w, 4.0f));

    Naui_Vec4 neg = naui_vec4_negate(v1);
    ASSERT(FLOAT_EQ(neg.x, -1.0f) && FLOAT_EQ(neg.y, 2.0f) && FLOAT_EQ(neg.z, -3.0f) && FLOAT_EQ(neg.w, 4.0f));

    Naui_Vec4 minv = naui_vec4_min(v1, v2);
    ASSERT(FLOAT_EQ(minv.x, -3.0f) && FLOAT_EQ(minv.y, -2.0f) && FLOAT_EQ(minv.z, -5.0f) && FLOAT_EQ(minv.w, -4.0f));

    Naui_Vec4 maxv = naui_vec4_max(v1, v2);
    ASSERT(FLOAT_EQ(maxv.x, 1.0f) && FLOAT_EQ(maxv.y, 4.0f) && FLOAT_EQ(maxv.z, 3.0f) && FLOAT_EQ(maxv.w, 6.0f));

    Naui_Vec4 lerpHalf = naui_vec4_lerp(v1, v2, 0.5f);
    ASSERT(FLOAT_EQ(lerpHalf.x, -1.0f) && FLOAT_EQ(lerpHalf.y, 1.0f) && FLOAT_EQ(lerpHalf.z, -1.0f) && FLOAT_EQ(lerpHalf.w, 1.0f));

    float len = naui_vec4_length(v1);
    ASSERT(FLOAT_EQ(len, sqrtf(30.0f)));

    float dist = naui_vec4_distance(v1, v2);
    ASSERT(FLOAT_EQ(dist, sqrtf(216.0f)));

    float dot = naui_vec4_dot(v1, v2);
    ASSERT(FLOAT_EQ(dot, -50.0f));

    TEST_END();
}

static void test_scalar_math(void)
{
    TEST_BEGIN("Scalar Math");

    ASSERT(FLOAT_EQ(naui_lerp(0.0f, 10.0f, 0.0f), 0.0f));
    ASSERT(FLOAT_EQ(naui_lerp(0.0f, 10.0f, 1.0f), 10.0f));
    ASSERT(FLOAT_EQ(naui_lerp(0.0f, 10.0f, 0.5f), 5.0f));
    ASSERT(FLOAT_EQ(naui_lerp(-5.0f, 5.0f, 0.25f), -2.5f));

    ASSERT(FLOAT_EQ(naui_clamp(5.0f, 0.0f, 10.0f), 5.0f));
    ASSERT(FLOAT_EQ(naui_clamp(-5.0f, 0.0f, 10.0f), 0.0f)); // below min
    ASSERT(FLOAT_EQ(naui_clamp(15.0f, 0.0f, 10.0f), 10.0f)); // above max
    ASSERT(FLOAT_EQ(naui_clamp(0.0f, 0.0f, 10.0f), 0.0f)); // exactly min
    ASSERT(FLOAT_EQ(naui_clamp(10.0f, 0.0f, 10.0f), 10.0f)); // exactly max

    // wrap within [0, 10)
    ASSERT(FLOAT_EQ(naui_wrap(5.0f, 0.0f, 10.0f), 5.0f)); // inside range
    ASSERT(FLOAT_EQ(naui_wrap(-2.0f, 0.0f, 10.0f), 8.0f)); // negative
    ASSERT(FLOAT_EQ(naui_wrap(12.0f, 0.0f, 10.0f), 2.0f)); // above max
    ASSERT(FLOAT_EQ(naui_wrap(0.0f, 0.0f, 10.0f), 0.0f)); // exact min
    ASSERT(FLOAT_EQ(naui_wrap(10.0f, 0.0f, 10.0f), 0.0f)); // exact max wraps to min

    // wrap with negative range
    ASSERT(FLOAT_EQ(naui_wrap(5.0f, -5.0f, 5.0f), -5.0f)); // upper bound maps to min
    ASSERT(FLOAT_EQ(naui_wrap(-7.0f, -5.0f, 5.0f), 3.0f)); // below min
    ASSERT(FLOAT_EQ(naui_wrap(12.0f, -5.0f, 5.0f), 2.0f)); // above max

    TEST_END();
}

void math_test(void)
{
    test_vec2();
    test_vec3();
    test_vec4();
	test_scalar_math();
}