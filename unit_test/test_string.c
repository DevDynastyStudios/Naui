#include "test_func.h"
#include "test.h"
#include "naui/utils/string.h"

void test_to_lower() {
    TEST_BEGIN("to_lower");

    ASSERT_SV_EQ(naui_sv_to_lower_temp(NAUI_STR("HeLLo")), NAUI_STR("hello"));
    ASSERT_SV_EQ(naui_sv_to_lower_temp(NAUI_STR("")), (Naui_StringView){0});

    TEST_END();
}

void test_to_upper() {
    TEST_BEGIN("to_upper");

    ASSERT_SV_EQ(naui_sv_to_upper_temp(NAUI_STR("HeLLo")), NAUI_STR("HELLO"));
    ASSERT_SV_EQ(naui_sv_to_upper_temp(NAUI_STR("")), (Naui_StringView){0});

    TEST_END();
}

void test_starts_with() {
    TEST_BEGIN("starts_with");

    ASSERT(naui_sv_starts_with(NAUI_STR("hello"), NAUI_STR("he")));
    ASSERT(!naui_sv_starts_with(NAUI_STR("hello"), NAUI_STR("hi")));
    ASSERT(!naui_sv_starts_with(NAUI_STR("hello"), (Naui_StringView){0}));

    TEST_END();
}

void test_ends_with() {
    TEST_BEGIN("ends_with");

    ASSERT(naui_sv_ends_with(NAUI_STR("hello"), NAUI_STR("lo")));
    ASSERT(!naui_sv_ends_with(NAUI_STR("hello"), NAUI_STR("oo")));
    ASSERT(!naui_sv_ends_with(NAUI_STR("hello"), (Naui_StringView){0}));

    TEST_END();
}

void test_find() {
    TEST_BEGIN("find");

    const Naui_StringView s = NAUI_STR("hello world");
    ASSERT_SV_EQ(naui_sv_find(s, NAUI_STR("world")), NAUI_STR("world"));
    ASSERT_SV_EQ(naui_sv_find(s, NAUI_STR("nope")), (Naui_StringView){0});

    TEST_END();
}

void test_find_char() {
    TEST_BEGIN("find_char");

    ASSERT(naui_sv_valid(naui_sv_find_char(NAUI_STR("abcde"), 'c')));
    ASSERT(!naui_sv_valid(naui_sv_find_char(NAUI_STR("abcde"), 'z')));

    TEST_END();
}

void test_numeric() {
    TEST_BEGIN("numeric");

    // TODO(doomguy): chat lemme cook
    /*
    int i;
    ASSERT(naui_sv_to_int("123", &i) && i == 123);
    ASSERT(!naui_sv_to_int("12x", &i));

    unsigned int u;
    ASSERT(naui_sv_to_uint("456", &u) && u == 456);

    int64_t i64;
    ASSERT(naui_sv_to_int64("789", &i64) && i64 == 789);
    */

    uint64_t u64;
    ASSERT(naui_sv_to_uint64(NAUI_STR("999999"), &u64) && u64 == 999999);

    /*
    float f;
    ASSERT(naui_sv_to_float("3.14", &f) && f > 3.13f && f < 3.15f);

    double d;
    ASSERT(naui_sv_to_double("2.718", &d) && d > 2.717 && d < 2.719);
    */

    TEST_END();
}

void test_trim() {
    TEST_BEGIN("trim");

    ASSERT_SV_EQ(naui_sv_trim(NAUI_STR("   hello   ")), NAUI_STR("hello"));
    ASSERT_SV_EQ(naui_sv_ltrim(NAUI_STR("   hello")), NAUI_STR("hello"));
    ASSERT_SV_EQ(naui_sv_rtrim(NAUI_STR("hello    ")), NAUI_STR("hello"));

    TEST_END();
}

void test_substring() {
    TEST_BEGIN("substring");

    ASSERT_SV_EQ(naui_sv_substring(NAUI_STR("hello"), 1, 3), NAUI_STR("ell"));
    ASSERT_SV_EQ(naui_sv_substring(NAUI_STR("hi"), 5, 2), (Naui_StringView){0});

    TEST_END();
}

void test_replace() {
    TEST_BEGIN("replace");

    // TODO(doomguy): lemme cook
    /*
    char out[64];

    naui_sv_replace("a-b-c", "-", "_", out, sizeof(out));
    ASSERT_SV_EQ(out, "a_b_c");

    naui_sv_replace("hello", "x", "y", out, sizeof(out));
    ASSERT_SV_EQ(out, "hello");
    */

    TEST_END();
}

void test_split() {
    TEST_BEGIN("split");

    Naui_StringView left, right;
    ASSERT(naui_sv_split_by_delim(NAUI_STR("a=b"), &left, &right, '='));
    ASSERT_SV_EQ(left, NAUI_STR("a"));
    ASSERT_SV_EQ(right, NAUI_STR("b"));

    ASSERT(!naui_sv_split_by_delim(NAUI_STR("abb"), &left, &right, '='));

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
    test_split();
}
