#pragma once
#include <stdbool.h>

void test_begin(const char* name);
void test_end(void);
void test_conclusion(void);

void test_assert(bool expr, const char* expr_str, const char* file, int line);
void test_assert_str_eq(const char* a, const char* b, const char* file, int line);
void test_assert_null(const void* ptr, const char* file, int line);
void test_assert_not_null(const void* ptr, const char* file, int line);

#define TEST_BEGIN(name) test_begin(name)
#define TEST_END() test_end()
#define TEST_CONCLUSION() test_conclusion()

#define ASSERT(expr) test_assert((expr), #expr, __FILE__, __LINE__)
#define ASSERT_STR_EQ(a, b) test_assert_str_eq((a), (b), __FILE__, __LINE__)
#define ASSERT_NULL(ptr) test_assert_null((ptr), __FILE__, __LINE__)
#define ASSERT_NOT_NULL(ptr) test_assert_not_null((ptr), __FILE__, __LINE__)