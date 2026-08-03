#include "test.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int g_passed = 0;
static int g_failed = 0;

static int s_passed = 0;
static int s_failed = 0;
static int s_active = false;
static const char* s_suite = "";

static void fatal(const char* msg, const char* file, int line)
{
	printf("\033[1;31mFATAL ERROR:\033[0m %s (%s:%d)\n", msg, file, line);
	fflush(stdout);
	abort();
}

static void ensure_suite_active(const char* file, int line)
{
	if (!s_active)
		fatal("ASSERT called outside of TEST_BEGIN/TEST_END", file, line);
}

void test_begin(const char* name)
{
	if (s_active)
		fatal("TEST_BEGIN called while a test suite is already active", __FILE__, __LINE__);

	s_suite = name;
	s_passed = 0;
	s_failed = 0;
	s_active = true;

	printf("\n\033[1;34m-- %s --\033[0m\n", s_suite);
}
void test_end(void)
{
	if (!s_active)
		fatal("TEST_END called without an active test suite", __FILE__, __LINE__);

	printf("\nSuite summary: " "\033[1;32m%d passed\033[0m, " "\033[1;31m%d failed\033[0m\n", s_passed, s_failed);
	printf("Result: %s\n\n", (s_failed == 0) ? "\033[1;32mPASSED\033[0m" : "\033[1;31mFAILED\033[0m");
	s_active = false;
}

static void record(bool ok)
{
	if (ok) {
		++g_passed;
		++s_passed;
	} else {
		++g_failed;
		++s_failed;
	}
}

void test_assert(bool expr, const char* expr_str, const char* file, int line)
{
	ensure_suite_active(file, line);

	if (expr) {
		printf("  \033[1;32mPASS\033[0m  %s\n", expr_str);
		record(true);
	} else {
		printf("  \033[1;31mFAIL\033[0m  %s  (%s:%d)\n", expr_str, file, line);
		record(false);
	}
}

void test_assert_str_eq(const char* a, const char* b, const char* file, int line)
{
	ensure_suite_active(file, line);

	bool ok = a && b && strcmp(a, b) == 0;
	if (ok) {
		printf("  \033[1;32mPASS\033[0m  \"%s\" == \"%s\"\n", a, b);
		record(true);
	} else {
		printf("  \033[1;31mFAIL\033[0m  str_eq: got \"%s\", want \"%s\"  (%s:%d)\n", a ? a : "(null)", b ? b : "(null)", file, line);
		record(false);
	}
}

void test_assert_sv_eq(Naui_StringView a, Naui_StringView b, const char* file, int line)
{
	ensure_suite_active(file, line);

	bool ok = naui_sv_cmp(a, b, true);
	if (ok) {
		printf("  \033[1;32mPASS\033[0m  \"%.*s\" == \"%.*s\"\n", (int)a.len, a.data, (int)b.len, b.data);
		record(true);
	} else {
        if (!naui_sv_valid(a)) a = NAUI_STR("(null)");
        if (!naui_sv_valid(b)) b = NAUI_STR("(null)");
		printf("  \033[1;31mFAIL\033[0m  str_eq: got \"%.*s\", want \"%.*s\"  (%s:%d)\n", (int)a.len, a.data, (int)b.len, b.data, file, line);
		record(false);
	}
}

void test_assert_null(const void* ptr, const char* file, int line)
{
	ensure_suite_active(file, line);

	bool ok = (ptr == NULL);
	if (ok) {
		printf("  \033[1;32mPASS\033[0m  ptr is NULL\n");
		record(true);
	} else {
		printf("  \033[1;31mFAIL\033[0m  ptr expected NULL  (%s:%d)\n", file, line);
		record(false);
	}
}

void test_assert_not_null(const void* ptr, const char* file, int line)
{
	ensure_suite_active(file, line);

	bool ok = (ptr != NULL);
	if (ok) {
		printf("  \033[1;32mPASS\033[0m  ptr != NULL\n");
		record(true);
	} else {
		printf("  \033[1;31mFAIL\033[0m  ptr expected non-NULL  (%s:%d)\n", file, line);
		record(false);
	}
}

void test_conclusion(void)
{
	if (s_active)
		fatal("TEST_CONCLUSION called before TEST_END for the active suite", __FILE__, __LINE__);
		
	printf("\n============================\n");
	printf("  TOTAL PASSED: \033[1;32m%d\033[0m\n", g_passed);
	printf("  TOTAL FAILED: \033[1;31m%d\033[0m\n", g_failed);
	printf("  OVERALL RESULT: %s\n", (g_failed == 0) ? "\033[1;32mPASSED\033[0m" : "\033[1;31mFAILED\033[0m");
	printf("============================\n");
}
