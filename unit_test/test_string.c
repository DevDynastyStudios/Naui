#include "test_func.h"
#include "test.h"
#include "naui/utils/string.h"

void test_to_lower() {
    TEST_BEGIN("to_lower");

    char out[32];

    naui_str_to_lower("HeLLo", out, sizeof(out));
    ASSERT_STR_EQ(out, "hello");

    naui_str_to_lower("", out, sizeof(out));
    ASSERT_STR_EQ(out, "");

    TEST_END();
}

void test_to_upper() {
    TEST_BEGIN("to_upper");

    char out[32];

    naui_str_to_upper("HeLLo", out, sizeof(out));
    ASSERT_STR_EQ(out, "HELLO");

    naui_str_to_upper("", out, sizeof(out));
    ASSERT_STR_EQ(out, "");

    TEST_END();
}

void test_starts_with() {
    TEST_BEGIN("starts_with");

    ASSERT(naui_str_starts_with("hello", "he"));
    ASSERT(!naui_str_starts_with("hello", "hi"));
    ASSERT(naui_str_starts_with("hello", ""));

    TEST_END();
}

void test_ends_with() {
    TEST_BEGIN("ends_with");

    ASSERT(naui_str_ends_with("hello", "lo"));
    ASSERT(!naui_str_ends_with("hello", "oo"));
    ASSERT(naui_str_ends_with("hello", ""));

    TEST_END();
}

void test_find() {
    TEST_BEGIN("find");

    const char* s = "hello world";

    ASSERT(naui_str_find(s, "world") == s + 6);
    ASSERT_NULL(naui_str_find(s, "nope"));

    TEST_END();
}

void test_find_char() {
    TEST_BEGIN("find_char");

    const char* s = "abcde";

    ASSERT(naui_str_find_char(s, 'c') == s + 2);
    ASSERT_NULL(naui_str_find_char(s, 'z'));

    TEST_END();
}

void test_numeric() {
    TEST_BEGIN("numeric");

    int i;
    ASSERT(naui_str_to_int("123", &i) && i == 123);
    ASSERT(!naui_str_to_int("12x", &i));

    unsigned int u;
    ASSERT(naui_str_to_uint("456", &u) && u == 456);

    int64_t i64;
    ASSERT(naui_str_to_int64("789", &i64) && i64 == 789);

    uint64_t u64;
    ASSERT(naui_str_to_uint64("999999", &u64) && u64 == 999999);

    float f;
    ASSERT(naui_str_to_float("3.14", &f) && f > 3.13f && f < 3.15f);

    double d;
    ASSERT(naui_str_to_double("2.718", &d) && d > 2.717 && d < 2.719);

    TEST_END();
}

void test_trim() {
    TEST_BEGIN("trim");

    char out[64];

    naui_str_trim("   hello   ", out, sizeof(out));
    ASSERT_STR_EQ(out, "hello");

    naui_str_ltrim("   hello", out, sizeof(out));
    ASSERT_STR_EQ(out, "hello");

    naui_str_rtrim("hello   ", out, sizeof(out));
    ASSERT_STR_EQ(out, "hello");

    TEST_END();
}

void test_substring() {
    TEST_BEGIN("substring");

    char out[32];

    naui_str_substring("hello", 1, 3, out, sizeof(out));
    ASSERT_STR_EQ(out, "ell");

    // Out of range → expect empty string
    naui_str_substring("hi", 5, 2, out, sizeof(out));
    ASSERT_STR_EQ(out, "");

    TEST_END();
}

void test_replace() {
    TEST_BEGIN("replace");

    char out[64];

    naui_str_replace("a-b-c", "-", "_", out, sizeof(out));
    ASSERT_STR_EQ(out, "a_b_c");

    naui_str_replace("hello", "x", "y", out, sizeof(out));
    ASSERT_STR_EQ(out, "hello");

    TEST_END();
}

void test_split_once() {
    TEST_BEGIN("split_once");

    char left[32], right[32];

    ASSERT(naui_str_split_once("a=b", '=', left, sizeof(left), right, sizeof(right)));
    ASSERT_STR_EQ(left, "a");
    ASSERT_STR_EQ(right, "b");

    ASSERT(!naui_str_split_once("abc", '=', left, sizeof(left), right, sizeof(right)));

    TEST_END();
}

void string_test()
{
    test_to_lower();
    test_to_upper();
    test_starts_with();
    test_ends_with();
    test_find();
    test_find_char();
    test_numeric();
    test_trim();
    test_substring();
    test_replace();
    test_split_once();
}