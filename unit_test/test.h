#pragma once

/*
 * test.h - Minimal unit test harness for Naui engine tests.
 *
 * Usage:
 *   TEST_BEGIN("suite name")
 *       ASSERT(expr)
 *       ASSERT_STR_EQ(a, b)
 *       ASSERT_NULL(ptr)
 *       ASSERT_NOT_NULL(ptr)
 *   TEST_END()
 *
 * Call test_summary() at the end of main() and return its value.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static int t_passed = 0;
static int t_failed = 0;
static const char* t_suite = "";

#define TEST_BEGIN(name) \
    do { \
        t_suite = (name); \
        printf("\n-- %s --\n", t_suite); \
    } while (0)

#define TEST_END() /* nothing — suites are just a label */

#define ASSERT(expr) \
    do { \
        if (expr) { \
            printf("  PASS  %s\n", #expr); \
            ++t_passed; \
        } else { \
            printf("  FAIL  %s  (%s:%d)\n", #expr, __FILE__, __LINE__); \
            ++t_failed; \
        } \
    } while (0)

#define ASSERT_STR_EQ(a, b) \
    do { \
        const char* _a = (a); \
        const char* _b = (b); \
        bool _ok = _a && _b && strcmp(_a, _b) == 0; \
        if (_ok) { \
            printf("  PASS  \"%s\" == \"%s\"\n", _a, _b); \
            ++t_passed; \
        } else { \
            printf("  FAIL  str_eq: got \"%s\", want \"%s\"  (%s:%d)\n", \
                   _a ? _a : "(null)", _b ? _b : "(null)", __FILE__, __LINE__); \
            ++t_failed; \
        } \
    } while (0)

#define ASSERT_NULL(ptr) \
    do { \
        if (!(ptr)) { ++t_passed; printf("  PASS  %s is NULL\n", #ptr); } \
        else { ++t_failed; printf("  FAIL  %s expected NULL  (%s:%d)\n", \
               #ptr, __FILE__, __LINE__); } \
    } while (0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr)) { ++t_passed; printf("  PASS  %s != NULL\n", #ptr); } \
        else { ++t_failed; printf("  FAIL  %s expected non-NULL  (%s:%d)\n", \
               #ptr, __FILE__, __LINE__); } \
    } while (0)

static int test_summary(void)
{
    printf("\n============================\n");
    printf("  %d passed, %d failed\n", t_passed, t_failed);
    printf("============================\n");
    return t_failed ? 1 : 0;
}