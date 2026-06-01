#include "test.h"
#include "naui/serialization/json_reader.h"
#include "naui/serialization/json_writer.h"
#include "naui/serialization/json.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

static void test_reader_empty(void)
{
	TEST_BEGIN("naui_json_reader - empty input");

	{
		Naui_JsonReader r;
		naui_json_reader_init(&r, "", 0);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_EOF);
	}

	TEST_END();
}

static void test_reader_primitives(void)
{
	TEST_BEGIN("naui_json_reader - primitives");

	{
		Naui_JsonReader r;

		naui_json_reader_init(&r, "true", 4);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_BOOL);
		ASSERT(r.boolean == true);

		naui_json_reader_init(&r, "false", 5);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_BOOL);
		ASSERT(r.boolean == false);

		naui_json_reader_init(&r, "null", 4);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_NULL);

		naui_json_reader_init(&r, "42", 2);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_NUMBER);
		ASSERT(r.number == 42.0);

		naui_json_reader_init(&r, "-3.14", 5);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_NUMBER);
		ASSERT(fabs(r.number - (-3.14)) < 1e-9);

		naui_json_reader_init(&r, "1e2", 3);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_NUMBER);
		ASSERT(r.number == 100.0);
	}

	TEST_END();
}

static void test_reader_string(void)
{
	TEST_BEGIN("naui_json_reader - strings");

	{
		Naui_JsonReader r;
		char buffer[64];

		naui_json_reader_init(&r, "\"hello\"", 7);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_STRING);
		ASSERT(r.len == 5);
		naui_json_reader_copy_str(&r, buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "hello");

		/* Escape sequences */
		naui_json_reader_init(&r, "\"line\\nbreak\"", 13);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_STRING);
		naui_json_reader_copy_str(&r, buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "line\nbreak");

		naui_json_reader_init(&r, "\"tab\\there\"", 11);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_STRING);
		naui_json_reader_copy_str(&r, buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "tab\there");

		naui_json_reader_init(&r, "\"quote\\\"inside\"", 15);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_STRING);
		naui_json_reader_copy_str(&r, buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "quote\"inside");

		/* Unicode escape */
		naui_json_reader_init(&r, "\"\\u0041\"", 8);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_STRING);
		naui_json_reader_copy_str(&r, buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "A");

		/* Unterminated string is an error */
		naui_json_reader_init(&r, "\"oops", 5);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_ERROR);
		ASSERT_NOT_NULL(r.error);
	}

	TEST_END();
}

static void test_reader_object(void)
{
	TEST_BEGIN("naui_json_reader - object");

	{
		const char* src = "{\"name\":\"Naui\",\"version\":1}";
		Naui_JsonReader r;
		char buffer[64];

		naui_json_reader_init(&r, src, strlen(src));

		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_OBJECT_BEGIN);

		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_KEY);
		naui_json_reader_copy_str(&r, buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "name");

		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_STRING);
		naui_json_reader_copy_str(&r, buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "Naui");

		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_KEY);
		naui_json_reader_copy_str(&r, buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "version");

		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_NUMBER);
		ASSERT(r.number == 1.0);

		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_OBJECT_END);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_EOF);
	}

	TEST_END();
}

static void test_reader_array(void)
{
	TEST_BEGIN("naui_json_reader - array");

	{
		const char* src = "[1,2,3]";
		Naui_JsonReader r;
		naui_json_reader_init(&r, src, strlen(src));

		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_ARRAY_BEGIN);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_NUMBER);
		ASSERT(r.number == 1.0);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_NUMBER);
		ASSERT(r.number == 2.0);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_NUMBER);
		ASSERT(r.number == 3.0);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_ARRAY_END);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_EOF);
	}

	TEST_END();
}

static void test_reader_nested(void)
{
	TEST_BEGIN("naui_json_reader - nested");

	{
		const char* src = "{\"a\":{\"b\":true},\"c\":[1,null]}";
		Naui_JsonReader r;
		naui_json_reader_init(&r, src, strlen(src));

		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_OBJECT_BEGIN);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_KEY);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_OBJECT_BEGIN);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_KEY);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_BOOL);
		ASSERT(r.boolean == true);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_OBJECT_END);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_KEY);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_ARRAY_BEGIN);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_NUMBER);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_NULL);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_ARRAY_END);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_OBJECT_END);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_EOF);
	}

	TEST_END();
}

static void test_reader_skip(void)
{
	TEST_BEGIN("naui_json_reader - skip");

	{
		const char* src = "{\"skip\":{\"a\":1,\"b\":2},\"keep\":true}";
		Naui_JsonReader r;
		char buffer[64];
		naui_json_reader_init(&r, src, strlen(src));

		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_OBJECT_BEGIN);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_KEY); /* "skip" */
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_OBJECT_BEGIN);

		naui_json_reader_skip(&r); /* skips the inner object */

		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_KEY);
		naui_json_reader_copy_str(&r, buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "keep");

		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_BOOL);
		ASSERT(r.boolean == true);
	}

	TEST_END();
}

static void test_reader_whitespace(void)
{
	TEST_BEGIN("naui_json_reader - whitespace and formatting");

	{
		const char* src =
			"{\n"
			"  \"key\" : \"value\" ,\n"
			"  \"num\" : 99\n"
			"}";

		Naui_JsonReader r;
		char buffer[64];
		naui_json_reader_init(&r, src, strlen(src));

		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_OBJECT_BEGIN);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_KEY);
		naui_json_reader_copy_str(&r, buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "key");
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_STRING);
		naui_json_reader_copy_str(&r, buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "value");
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_KEY);
		naui_json_reader_copy_str(&r, buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "num");
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_NUMBER);
		ASSERT(r.number == 99.0);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_OBJECT_END);
	}

	TEST_END();
}

static void test_reader_error(void)
{
	TEST_BEGIN("naui_json_reader - error cases");

	{
		Naui_JsonReader r;

		/* Unexpected character */
		naui_json_reader_init(&r, "x", 1);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_ERROR);

		/* Mismatched bracket */
		naui_json_reader_init(&r, "[}", 2);
		naui_json_reader_next(&r); /* [ */
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_ERROR);

		/* Trailing garbage after valid value */
		naui_json_reader_init(&r, "truex", 5);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_ERROR);

		/* Invalid number */
		naui_json_reader_init(&r, "1.", 2);
		ASSERT(naui_json_reader_next(&r) == NAUI_JSON_TOKEN_ERROR);
	}

	TEST_END();
}

static void test_writer_flat_object(void)
{
	TEST_BEGIN("naui_json_writer - flat object");

	{
		char buffer[256];
		Naui_JsonWriter w;
		naui_json_writer_init(&w, buffer, sizeof(buffer), false);

		naui_json_writer_object_begin(&w);
		naui_json_writer_key(&w, "name");
		naui_json_writer_string(&w, "Naui");
		naui_json_writer_key(&w, "version");
		naui_json_writer_int(&w, 1);
		naui_json_writer_key(&w, "active");
		naui_json_writer_bool(&w, true);
		naui_json_writer_key(&w, "ratio");
		naui_json_writer_number(&w, 3.14);
		naui_json_writer_key(&w, "nothing");
		naui_json_writer_null(&w);
		naui_json_writer_object_end(&w);

		int len = naui_json_writer_finish(&w);
		ASSERT(len > 0);
		ASSERT(!w.has_error);
		ASSERT_STR_EQ(buffer, "{\"name\":\"Naui\",\"version\":1,\"active\":true,\"ratio\":3.14,\"nothing\":null}");
	}

	TEST_END();
}

static void test_writer_array(void)
{
	TEST_BEGIN("naui_json_writer - array");

	{
		char buffer[128];
		Naui_JsonWriter w;
		naui_json_writer_init(&w, buffer, sizeof(buffer), false);

		naui_json_writer_array_begin(&w);
		naui_json_writer_int(&w, 1);
		naui_json_writer_int(&w, 2);
		naui_json_writer_int(&w, 3);
		naui_json_writer_string(&w, "hello");
		naui_json_writer_bool(&w, false);
		naui_json_writer_null(&w);
		naui_json_writer_array_end(&w);

		naui_json_writer_finish(&w);
		ASSERT(!w.has_error);
		ASSERT_STR_EQ(buffer, "[1,2,3,\"hello\",false,null]");
	}

	TEST_END();
}

static void test_writer_nested(void)
{
	TEST_BEGIN("naui_json_writer - nested");

	{
		char buffer[256];
		Naui_JsonWriter w;
		naui_json_writer_init(&w, buffer, sizeof(buffer), false);

		naui_json_writer_object_begin(&w);
		naui_json_writer_key(&w, "tags");
		naui_json_writer_array_begin(&w);
		naui_json_writer_string(&w, "a");
		naui_json_writer_string(&w, "b");
		naui_json_writer_array_end(&w);
		naui_json_writer_key(&w, "meta");
		naui_json_writer_object_begin(&w);
		naui_json_writer_key(&w, "x");
		naui_json_writer_int(&w, 99);
		naui_json_writer_object_end(&w);
		naui_json_writer_object_end(&w);

		naui_json_writer_finish(&w);
		ASSERT(!w.has_error);
		ASSERT_STR_EQ(buffer, "{\"tags\":[\"a\",\"b\"],\"meta\":{\"x\":99}}");
	}

	TEST_END();
}

static void test_writer_escape(void)
{
	TEST_BEGIN("naui_json_writer - string escaping");

	{
		char buffer[128];
		Naui_JsonWriter w;
		naui_json_writer_init(&w, buffer, sizeof(buffer), false);

		naui_json_writer_array_begin(&w);
		naui_json_writer_string(&w, "line\nbreak");
		naui_json_writer_string(&w, "tab\there");
		naui_json_writer_string(&w, "quote\"inside");
		naui_json_writer_string(&w, "back\\slash");
		naui_json_writer_array_end(&w);

		naui_json_writer_finish(&w);
		ASSERT(!w.has_error);
		ASSERT_STR_EQ(buffer, "[\"line\\nbreak\",\"tab\\there\",\"quote\\\"inside\",\"back\\\\slash\"]");
	}

	TEST_END();
}

static void test_writer_truncation(void)
{
	TEST_BEGIN("naui_json_writer - truncation sets error");

	{
		char buffer[10];
		Naui_JsonWriter w;
		naui_json_writer_init(&w, buffer, sizeof(buffer), false);

		naui_json_writer_object_begin(&w);
		naui_json_writer_key(&w, "this_key_is_too_long_for_the_ bufferfer");
		naui_json_writer_string(&w, "value");
		naui_json_writer_object_end(&w);

		int len = naui_json_writer_finish(&w);
		ASSERT(len == -1);
		ASSERT(w.has_error);
	}

	TEST_END();
}

static void test_writer_heap(void)
{
	TEST_BEGIN("naui_json_writer - heap mode");

	{
		Naui_JsonWriter w;
		naui_json_writer_init_heap(&w, false);

		naui_json_writer_object_begin(&w);
		naui_json_writer_key(&w, "hello");
		naui_json_writer_string(&w, "world");
		naui_json_writer_object_end(&w);

		size_t len;
		char* result = naui_json_writer_finish_heap(&w, &len);
		ASSERT_NOT_NULL(result);
		ASSERT(len > 0);
		ASSERT_STR_EQ(result, "{\"hello\":\"world\"}");
		free(result);
	}

	TEST_END();
}

static void test_writer_pretty(void)
{
	TEST_BEGIN("naui_json_writer - pretty print");

	{
		char buffer[256];
		Naui_JsonWriter w;
		naui_json_writer_init(&w, buffer, sizeof(buffer), true);

		naui_json_writer_object_begin(&w);
		naui_json_writer_key(&w, "a");
		naui_json_writer_int(&w, 1);
		naui_json_writer_key(&w, "b");
		naui_json_writer_int(&w, 2);
		naui_json_writer_object_end(&w);

		naui_json_writer_finish(&w);
		ASSERT(!w.has_error);

		/* Just verify it contains newlines and indentation — exact
		 * formatting can vary, but these must be present. */
		ASSERT(strchr(buffer, '\n') != NULL);
		ASSERT(strstr(buffer, "  ") != NULL);
	}

	TEST_END();
}

static void test_dom_parse_object(void)
{
	TEST_BEGIN("naui_json DOM - parse object");

	{
		const char* src = "{\"name\":\"Naui\",\"version\":2,\"active\":true,\"ratio\":1.5,\"tag\":null}";
		Naui_Json r = naui_json_parse(src, strlen(src));

		ASSERT_NULL(r.error);
		ASSERT_NOT_NULL(r.root);
		ASSERT(r.root->type == NAUI_JSON_OBJECT);

		char buffer[64];

		Naui_JsonValue* name = naui_json_object_get(r.root, "name");
		ASSERT_NOT_NULL(name);
		ASSERT(name->type == NAUI_JSON_STRING);
		naui_json_copy_string(name, buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "Naui");

		Naui_JsonValue* version = naui_json_object_get(r.root, "version");
		ASSERT_NOT_NULL(version);
		ASSERT(naui_json_get_int(version, 0) == 2);

		Naui_JsonValue* active = naui_json_object_get(r.root, "active");
		ASSERT_NOT_NULL(active);
		ASSERT(naui_json_get_bool(active, false) == true);

		Naui_JsonValue* ratio = naui_json_object_get(r.root, "ratio");
		ASSERT_NOT_NULL(ratio);
		ASSERT(fabs(naui_json_get_number(ratio, 0.0) - 1.5) < 1e-9);

		Naui_JsonValue* tag = naui_json_object_get(r.root, "tag");
		ASSERT_NOT_NULL(tag);
		ASSERT(naui_json_is_null(tag));

		/* Missing key returns NULL */
		ASSERT_NULL(naui_json_object_get(r.root, "missing"));

		/* Default values on wrong types */
		ASSERT(naui_json_get_int(name, -1) == -1);
		ASSERT(naui_json_get_bool(version, false) == false);

		naui_json_free(&r);
	}

	TEST_END();
}

static void test_dom_parse_array(void)
{
	TEST_BEGIN("naui_json DOM - parse array");

	{
		const char* src = "[10,20,30]";
		Naui_Json r = naui_json_parse(src, strlen(src));

		ASSERT_NULL(r.error);
		ASSERT_NOT_NULL(r.root);
		ASSERT(r.root->type == NAUI_JSON_ARRAY);
		ASSERT(r.root->array.count == 3);

		ASSERT(naui_json_get_int(naui_json_array_get(r.root, 0), 0) == 10);
		ASSERT(naui_json_get_int(naui_json_array_get(r.root, 1), 0) == 20);
		ASSERT(naui_json_get_int(naui_json_array_get(r.root, 2), 0) == 30);

		/* Out of bounds returns NULL */
		ASSERT_NULL(naui_json_array_get(r.root, 3));

		naui_json_free(&r);
	}

	TEST_END();
}

static void test_dom_parse_nested(void)
{
	TEST_BEGIN("naui_json DOM - parse nested");

	{
		const char* src =
			"{"
			" \"user\": {"
			"  \"id\": 42,"
			"  \"tags\": [\"admin\", \"user\"]"
			" }"
			"}";

		Naui_Json r = naui_json_parse(src, strlen(src));
		ASSERT_NULL(r.error);

		Naui_JsonValue* user = naui_json_object_get(r.root, "user");
		ASSERT_NOT_NULL(user);
		ASSERT(user->type == NAUI_JSON_OBJECT);

		Naui_JsonValue* id = naui_json_object_get(user, "id");
		ASSERT(naui_json_get_int(id, 0) == 42);

		Naui_JsonValue* tags = naui_json_object_get(user, "tags");
		ASSERT_NOT_NULL(tags);
		ASSERT(tags->type == NAUI_JSON_ARRAY);
		ASSERT(tags->array.count == 2);

		char buffer[32];
		naui_json_copy_string(naui_json_array_get(tags, 0), buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "admin");
		naui_json_copy_string(naui_json_array_get(tags, 1), buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "user");

		naui_json_free(&r);
	}

	TEST_END();
}

static void test_dom_parse_error(void)
{
	TEST_BEGIN("naui_json DOM - parse errors");

	{
		Naui_Json r;

		r = naui_json_parse("", 0);
		ASSERT_NOT_NULL(r.error);
		naui_json_free(&r);

		r = naui_json_parse("{bad}", 5);
		ASSERT_NOT_NULL(r.error);
		naui_json_free(&r);

		r = naui_json_parse("[1,2,", 5);
		ASSERT_NOT_NULL(r.error);
		naui_json_free(&r);
	}

	TEST_END();
}


static void test_dom_build_object(void)
{
	TEST_BEGIN("naui_json DOM - build object");

	{
		Naui_Json r = naui_json_result_create();
		Naui_JsonValue* root = naui_json_object(&r);

		naui_json_set_string(&r, root, "name", "Naui");
		naui_json_set_int(&r, root, "version", 3);
		naui_json_set_bool(&r, root, "active", true);
		naui_json_set_null(&r, root, "tag");
		naui_json_set_number(&r, root, "ratio", 2.5);

		/* Verify via query */
		char buffer[64];
		naui_json_copy_string(naui_json_object_get(root, "name"), buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "Naui");
		ASSERT(naui_json_get_int(naui_json_object_get(root, "version"), 0) == 3);
		ASSERT(naui_json_get_bool(naui_json_object_get(root, "active"), false) == true);
		ASSERT(naui_json_is_null(naui_json_object_get(root, "tag")));
		ASSERT(fabs(naui_json_get_number(naui_json_object_get(root, "ratio"), 0.0) - 2.5) < 1e-9);

		/* Overwrite existing key */
		naui_json_set_int(&r, root, "version", 99);
		ASSERT(naui_json_get_int(naui_json_object_get(root, "version"), 0) == 99);

		naui_json_free(&r);
	}

	TEST_END();
}

static void test_dom_build_array(void)
{
	TEST_BEGIN("naui_json DOM - build array");

	{
		Naui_Json r = naui_json_result_create();
		Naui_JsonValue* root = naui_json_array(&r);

		naui_json_push_int(&r, root, 1);
		naui_json_push_int(&r, root, 2);
		naui_json_push_string(&r, root, "three");
		naui_json_push_bool(&r, root, false);
		naui_json_push_null(&r, root);

		ASSERT(root->array.count == 5);
		ASSERT(naui_json_get_int(naui_json_array_get(root, 0), 0) == 1);
		ASSERT(naui_json_get_int(naui_json_array_get(root, 1), 0) == 2);

		char buffer[32];
		naui_json_copy_string(naui_json_array_get(root, 2), buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "three");

		ASSERT(naui_json_get_bool(naui_json_array_get(root, 3), true) == false);
		ASSERT(naui_json_is_null(naui_json_array_get(root, 4)));

		naui_json_free(&r);
	}

	TEST_END();
}

static void test_dom_build_nested(void)
{
	TEST_BEGIN("naui_json DOM - build nested");

	{
		Naui_Json r = naui_json_result_create();
		Naui_JsonValue* root = naui_json_object(&r);

		Naui_JsonValue* user = naui_json_set_object(&r, root, "user");
		naui_json_set_string(&r, user, "name", "Alice");
		naui_json_set_int(&r, user, "age", 30);

		Naui_JsonValue* scores = naui_json_set_array(&r, root, "scores");
		naui_json_push_int(&r, scores, 10);
		naui_json_push_int(&r, scores, 20);
		naui_json_push_int(&r, scores, 30);

		/* Verify */
		Naui_JsonValue* got_user = naui_json_object_get(root, "user");
		ASSERT_NOT_NULL(got_user);

		char buffer[32];
		naui_json_copy_string(naui_json_object_get(got_user, "name"), buffer, sizeof(buffer));
		ASSERT_STR_EQ(buffer, "Alice");
		ASSERT(naui_json_get_int(naui_json_object_get(got_user, "age"), 0) == 30);

		Naui_JsonValue* got_scores = naui_json_object_get(root, "scores");
		ASSERT(got_scores->array.count == 3);
		ASSERT(naui_json_get_int(naui_json_array_get(got_scores, 2), 0) == 30);

		naui_json_free(&r);
	}

	TEST_END();
}

static void test_dom_roundtrip(void)
{
	TEST_BEGIN("naui_json DOM - round-trip build -> write -> parse");

	{
		/* Build */
		Naui_Json build = naui_json_result_create();
		Naui_JsonValue* root = naui_json_object(&build);
		naui_json_set_string(&build, root, "engine", "Naui");
		naui_json_set_int(&build, root, "build", 42);
		naui_json_set_bool(&build, root, "debug", true);
		Naui_JsonValue* tags = naui_json_set_array(&build, root, "tags");
		naui_json_push_string(&build, tags, "alpha");
		naui_json_push_string(&build, tags, "beta");

		/* Write to bufferfer */
		char buffer[512];
		int len = naui_json_write(root, buffer, sizeof(buffer), false);
		ASSERT(len > 0);

		naui_json_free(&build);

		/* Re-parse */
		Naui_Json parsed = naui_json_parse(buffer, (size_t)len);
		ASSERT_NULL(parsed.error);
		ASSERT_NOT_NULL(parsed.root);

		char str[64];
		naui_json_copy_string(naui_json_object_get(parsed.root, "engine"), str, sizeof(str));
		ASSERT_STR_EQ(str, "Naui");
		ASSERT(naui_json_get_int(naui_json_object_get(parsed.root, "build"), 0) == 42);
		ASSERT(naui_json_get_bool(naui_json_object_get(parsed.root, "debug"), false) == true);

		Naui_JsonValue* ptags = naui_json_object_get(parsed.root, "tags");
		ASSERT_NOT_NULL(ptags);
		ASSERT(ptags->array.count == 2);
		naui_json_copy_string(naui_json_array_get(ptags, 0), str, sizeof(str));
		ASSERT_STR_EQ(str, "alpha");
		naui_json_copy_string(naui_json_array_get(ptags, 1), str, sizeof(str));
		ASSERT_STR_EQ(str, "beta");

		naui_json_free(&parsed);
	}

	TEST_END();
}

static void test_dom_write_measure(void)
{
	TEST_BEGIN("naui_json DOM - write with NULL dest measures size");

	{
		Naui_Json r = naui_json_result_create();
		Naui_JsonValue* root = naui_json_object(&r);
		naui_json_set_string(&r, root, "k", "v");

		int size = naui_json_write(root, NULL, 0, false);
		ASSERT(size > 0);

		char* buffer = (char*)malloc((size_t)size + 1);
		int written = naui_json_write(root, buffer, (size_t)size + 1, false);
		ASSERT(written == size);
		ASSERT_STR_EQ(buffer, "{\"k\":\"v\"}");
		free(buffer);
		naui_json_free(&r);
	}

	TEST_END();
}

void json_test(void)
{
	test_reader_empty();
	test_reader_primitives();
	test_reader_string();
	test_reader_object();
	test_reader_array();
	test_reader_nested();
	test_reader_skip();
	test_reader_whitespace();
	test_reader_error();

	test_writer_flat_object();
	test_writer_array();
	test_writer_nested();
	test_writer_escape();
	test_writer_truncation();
	test_writer_heap();
	test_writer_pretty();

	test_dom_parse_object();
	test_dom_parse_array();
	test_dom_parse_nested();
	test_dom_parse_error();

	test_dom_build_object();
	test_dom_build_array();
	test_dom_build_nested();

	test_dom_roundtrip();
	test_dom_write_measure();
}