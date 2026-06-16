#include "test.h"
#include "naui/localization/localization.h"
#include "naui/filesystem/filesystem.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

static Naui_Path make_test_dir(void)
{
	Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
	Naui_Path testdir = naui_path_join(cwd, NAUI_PATH("naui_test"));

	if (naui_path_exists(testdir))
		naui_directory_remove_all(testdir);

	naui_directory_create(testdir);
	return testdir;
}

static Naui_Path write_test_lang(Naui_Path testdir, const char* filename, const char* contents)
{
	Naui_Path path = naui_path_join(testdir, NAUI_PATH(filename));
	bool ok = naui_file_write_all(path, contents, strlen(contents));
	ASSERT(ok);
	return path;
}

static void cleanup_test_dir(Naui_Path testdir)
{
	naui_directory_remove_all(testdir);
}

static void test_localization_basic(void)
{
	TEST_BEGIN("Localization - basic load");

	Naui_Path testdir = make_test_dir();

	Naui_Path file = write_test_lang(testdir, "en.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"en-US\","
		"	\"direction\": \"ltr\","
		"	\"name\": \"English\""
		"  },"
		"  \"hello\": \"Hello\","
		"  \"bye\": \"Goodbye\""
		"}"
	);

	Naui_Language lang;
	ASSERT(naui_localization_load_file(file.data, &lang));

	ASSERT_STR_EQ(naui_localization_get(&lang, "hello"), "Hello");
	ASSERT_STR_EQ(naui_localization_get(&lang, "bye"), "Goodbye");
	ASSERT_STR_EQ(naui_localization_get(&lang, "missing"), "missing");

	naui_localization_free(&lang);
	cleanup_test_dir(testdir);

	TEST_END();
}

static void test_localization_interpolation(void)
{
	TEST_BEGIN("Localization - interpolation");

	Naui_Path testdir = make_test_dir();

	Naui_Path file = write_test_lang(testdir, "interp.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"en-US\","
		"	\"direction\": \"ltr\","
		"	\"name\": \"English\""
		"  },"
		"  \"greet\": \"$Hello {name}, you have {count} messages.\""
		"}"
	);

	Naui_Language lang;
	ASSERT(naui_localization_load_file(file.data, &lang));

	const char* args[] = {
		"name",  "User",
		"count", "5"
	};

	char* out = naui_localization_format(&lang, "greet", args, 4);
	ASSERT_NOT_NULL(out);
	ASSERT_STR_EQ(out, "Hello User, you have 5 messages.");

	free(out);
	naui_localization_free(&lang);
	cleanup_test_dir(testdir);

	TEST_END();
}

static void test_localization_missing_arg(void)
{
	TEST_BEGIN("Localization - missing interpolation arg");

	Naui_Path testdir = make_test_dir();

	Naui_Path file = write_test_lang(testdir, "missing.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"en-US\","
		"	\"direction\": \"ltr\","
		"	\"name\": \"English\""
		"  },"
		"  \"greet\": \"$Hello {name}\""
		"}"
	);

	Naui_Language lang;
	ASSERT(naui_localization_load_file(file.data, &lang));

	const char* args[] = { "unused", "value" };
	char* out = naui_localization_format(&lang, "greet", args, 2);
	ASSERT_NOT_NULL(out);
	ASSERT_STR_EQ(out, "Hello {name}");

	free(out);
	naui_localization_free(&lang);
	cleanup_test_dir(testdir);

	TEST_END();
}

static void test_localization_freeing(void)
{
	TEST_BEGIN("Localization - freeing");

	Naui_Path testdir = make_test_dir();

	Naui_Path file = write_test_lang(testdir, "free.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"en-US\","
		"	\"direction\": \"ltr\","
		"	\"name\": \"English\""
		"  },"
		"  \"hello\": \"Hello\""
		"}"
	);

	Naui_Language lang;
	ASSERT(naui_localization_load_file(file.data, &lang));

	naui_localization_free(&lang);
	ASSERT_STR_EQ(naui_localization_get(&lang, "hello"), "hello");
	cleanup_test_dir(testdir);

	TEST_END();
}
static void test_localization_global_set(void)
{
	TEST_BEGIN("Localization - global set_current");

	Naui_Path bin_dir = naui_directory_get(NAUI_DIR_BIN);
	Naui_Path lang_dir = naui_path_join(bin_dir, NAUI_PATH("Language"));
	naui_directory_create(lang_dir);

	Naui_Path file = write_test_lang(lang_dir, "en-US.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"en-US\","
		"	\"direction\": \"ltr\","
		"	\"name\": \"English\""
		"  },"
		"  \"hello\": \"Hello\""
		"}"
	);

	naui_localization_set_current("en-US");

	Naui_Language* cur = naui_localization_get_current();
	ASSERT_NOT_NULL(cur);
	ASSERT_STR_EQ(naui_localization_get(cur, "hello"), "Hello");
	naui_file_delete(file);

	TEST_END();
}

static void test_localization_TR_macro(void)
{
	TEST_BEGIN("Localization - NAUI_TR macro");

	Naui_Path testdir = make_test_dir();
	Naui_Path file = write_test_lang(testdir, "en-US.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"en-US\","
		"	\"direction\": \"ltr\","
		"	\"name\": \"English\""
		"  },"
		"  \"hello\": \"Hello World\""
		"}"
	);

	Naui_Language lang;
	ASSERT(naui_localization_load_file(file.data, &lang));
	naui_localization_set_current_lang(&lang);

	ASSERT_STR_EQ(NAUI_TR("hello"), "Hello World");
	ASSERT_STR_EQ(NAUI_TR("missing"), "missing");

	cleanup_test_dir(testdir);
	TEST_END();
}

static void test_localization_global_fallback(void)
{
	TEST_BEGIN("Localization - global fallback");

	Naui_Path testdir = make_test_dir();
	Naui_Path file = write_test_lang(testdir, "fr-FR.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"fr-FR\","
		"	\"direction\": \"ltr\","
		"	\"name\": \"French\""
		"  },"
		"  \"hello\": \"Bonjour\""
		"}"
	);

	Naui_Language lang;
	ASSERT(naui_localization_load_file(file.data, &lang));
	naui_localization_set_current_lang(&lang);

	Naui_Language* cur = naui_localization_get_current();
	ASSERT_NOT_NULL(cur);
	ASSERT_STR_EQ(naui_localization_get(cur, "hello"), "Bonjour");

	cleanup_test_dir(testdir);
	TEST_END();
}

static void test_localization_meta_cache_basic(void)
{
	TEST_BEGIN("Localization - meta cache basic");

	Naui_Path bin_dir  = naui_directory_get(NAUI_DIR_BIN);
	Naui_Path lang_dir = naui_path_join(bin_dir, NAUI_PATH("Language"));
	naui_directory_create(lang_dir);

	Naui_Path en_file = write_test_lang(lang_dir, "en-US.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"en-US\","
		"	\"direction\": \"ltr\","
		"	\"name\": \"English\""
		"  },"
		"  \"hello\": \"Hello\""
		"}"
	);

	Naui_Path fr_file = write_test_lang(lang_dir, "fr-FR.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"fr-FR\","
		"	\"direction\": \"ltr\","
		"	\"name\": \"French\""
		"  },"
		"  \"hello\": \"Bonjour\""
		"}"
	);

	naui_localization_reload_meta_cache();

	Naui_List(Naui_LanguageMeta) langs = naui_localization_get_languages();
	ASSERT(naui_list_len(langs) >= 2);

	bool found_en = false;
	bool found_fr = false;
	for (ptrdiff_t i = 0; i < naui_list_len(langs); ++i)
	{
		if (strcmp(langs[i].language_code, "en") == 0)
			found_en = true;
		if (strcmp(langs[i].language_code, "fr") == 0)
			found_fr = true;
	}

	ASSERT(found_en);
	ASSERT(found_fr);

	naui_file_delete(en_file);
	naui_file_delete(fr_file);

	TEST_END();
}

static void test_localization_meta_cache_stale_list(void)
{
	TEST_BEGIN("Localization - meta cache stale list after reload");

	Naui_Path bin_dir  = naui_directory_get(NAUI_DIR_BIN);
	Naui_Path lang_dir = naui_path_join(bin_dir, NAUI_PATH("Language"));
	naui_directory_create(lang_dir);

	Naui_Path en_file = write_test_lang(lang_dir, "en-US.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"en-US\","
		"	\"direction\": \"ltr\","
		"	\"name\": \"English\""
		"  },"
		"  \"hello\": \"Hello\""
		"}"
	);

	naui_localization_reload_meta_cache();

	/* Grab a snapshot of the list pointer before the reload. */
	Naui_List(Naui_LanguageMeta) stale = naui_localization_get_languages();
	ptrdiff_t stale_len = naui_list_len(stale);
	ASSERT(stale_len >= 1);

	/* Add a second language and reload — stale must not be used after this. */
	Naui_Path de_file = write_test_lang(lang_dir, "de-DE.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"de-DE\","
		"	\"direction\": \"ltr\","
		"	\"name\": \"German\""
		"  },"
		"  \"hello\": \"Hallo\""
		"}"
	);

	naui_localization_reload_meta_cache();

	/* Fresh list must reflect the new file. */
	Naui_List(Naui_LanguageMeta) fresh = naui_localization_get_languages();
	ASSERT(naui_list_len(fresh) > stale_len);

	bool found_de = false;
	for (ptrdiff_t i = 0; i < naui_list_len(fresh); ++i)
	{
		if (strcmp(fresh[i].language_code, "de") == 0)
			found_de = true;
	}

	ASSERT(found_de);

	naui_file_delete(en_file);
	naui_file_delete(de_file);

	TEST_END();
}

static void test_localization_meta_cache_removed_file(void)
{
	TEST_BEGIN("Localization - meta cache reflects removed file");

	Naui_Path bin_dir  = naui_directory_get(NAUI_DIR_BIN);
	Naui_Path lang_dir = naui_path_join(bin_dir, NAUI_PATH("Language"));
	naui_directory_create(lang_dir);

	Naui_Path en_file = write_test_lang(lang_dir, "en-US.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"en-US\","
		"	\"direction\": \"ltr\","
		"	\"name\": \"English\""
		"  },"
		"  \"hello\": \"Hello\""
		"}"
	);

	Naui_Path es_file = write_test_lang(lang_dir, "es-ES.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"es-ES\","
		"	\"direction\": \"ltr\","
		"	\"name\": \"Spanish\""
		"  },"
		"  \"hello\": \"Hola\""
		"}"
	);

	naui_localization_reload_meta_cache();

	ptrdiff_t count_before = naui_list_len(naui_localization_get_languages());
	ASSERT(count_before >= 2);

	/* Remove one file and reload — it must disappear from the list. */
	naui_file_delete(es_file);
	naui_localization_reload_meta_cache();

	Naui_List(Naui_LanguageMeta) langs = naui_localization_get_languages();
	ASSERT(naui_list_len(langs) == count_before - 1);

	bool found_es = false;
	for (ptrdiff_t i = 0; i < naui_list_len(langs); ++i)
	{
		if (strcmp(langs[i].language_code, "es") == 0)
			found_es = true;
	}

	ASSERT(!found_es);

	naui_file_delete(en_file);

	TEST_END();
}

static void test_localization_meta_cache_empty_dir(void)
{
	TEST_BEGIN("Localization - meta cache empty language dir");

	Naui_Path bin_dir  = naui_directory_get(NAUI_DIR_BIN);
	Naui_Path lang_dir = naui_path_join(bin_dir, NAUI_PATH("Language"));

	/* Remove any existing lang files so the directory is empty. */
	if (naui_path_exists(lang_dir))
		naui_directory_remove_all(lang_dir);

	naui_directory_create(lang_dir);
	naui_localization_reload_meta_cache();

	Naui_List(Naui_LanguageMeta) langs = naui_localization_get_languages();
	ASSERT(naui_list_len(langs) == 0);

	TEST_END();
}

static void test_localization_meta_cache_corrupt_file(void)
{
	TEST_BEGIN("Localization - meta cache skips corrupt file");

	Naui_Path bin_dir  = naui_directory_get(NAUI_DIR_BIN);
	Naui_Path lang_dir = naui_path_join(bin_dir, NAUI_PATH("Language"));
	naui_directory_create(lang_dir);

	Naui_Path good_file = write_test_lang(lang_dir, "en-US.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"en-US\","
		"	\"direction\": \"ltr\","
		"	\"name\": \"English\""
		"  },"
		"  \"hello\": \"Hello\""
		"}"
	);

	/* Deliberately malformed JSON. */
	Naui_Path bad_file = write_test_lang(lang_dir, "bad.lang",
		"{ this is not valid json !!!"
	);

	naui_localization_reload_meta_cache();

	/* The corrupt file must be silently skipped — good file still loads. */
	Naui_List(Naui_LanguageMeta) langs = naui_localization_get_languages();
	ASSERT(naui_list_len(langs) >= 1);

	bool found_bad = false;
	for (ptrdiff_t i = 0; i < naui_list_len(langs); ++i)
	{
		if (langs[i].language_code &&
			strcmp(langs[i].language_code, "bad") == 0)
			found_bad = true;
	}

	ASSERT(!found_bad);

	naui_file_delete(good_file);
	naui_file_delete(bad_file);

	TEST_END();
}

static void test_localization_meta_cache_no_meta_block(void)
{
	TEST_BEGIN("Localization - meta cache file with no _meta block");

	Naui_Path bin_dir  = naui_directory_get(NAUI_DIR_BIN);
	Naui_Path lang_dir = naui_path_join(bin_dir, NAUI_PATH("Language"));
	naui_directory_create(lang_dir);

	/* Valid JSON, valid entries, but no _meta block at all. */
	Naui_Path file = write_test_lang(lang_dir, "zz-ZZ.lang",
		"{"
		"  \"hello\": \"Hello\""
		"}"
	);

	naui_localization_reload_meta_cache();

	/* Should still appear in the cache, code derived from filename. */
	Naui_List(Naui_LanguageMeta) langs = naui_localization_get_languages();

	bool found = false;
	for (ptrdiff_t i = 0; i < naui_list_len(langs); ++i)
	{
		if (langs[i].language_code &&
			strcmp(langs[i].language_code, "zz") == 0)
			found = true;
	}

	ASSERT(found);

	naui_file_delete(file);

	TEST_END();
}

static void test_localization_meta_cache_metadata_fields(void)
{
	TEST_BEGIN("Localization - meta cache metadata fields");

	Naui_Path bin_dir  = naui_directory_get(NAUI_DIR_BIN);
	Naui_Path lang_dir = naui_path_join(bin_dir, NAUI_PATH("Language"));
	naui_directory_create(lang_dir);

	Naui_Path file = write_test_lang(lang_dir, "ar-SA.lang",
		"{"
		"  \"_meta\": {"
		"	\"language\": \"ar-SA\","
		"	\"direction\": \"rtl\","
		"	\"name\": \"Arabic\""
		"  },"
		"  \"hello\": \"مرحبا\""
		"}"
	);

	naui_localization_reload_meta_cache();

	Naui_List(Naui_LanguageMeta) langs = naui_localization_get_languages();

	bool found = false;
	for (ptrdiff_t i = 0; i < naui_list_len(langs); ++i)
	{
		Naui_LanguageMeta* m = &langs[i];
		if (!m->language_code || strcmp(m->language_code, "ar") != 0)
			continue;

		found = true;
		ASSERT_STR_EQ(m->region_code,   "SA");
		ASSERT_STR_EQ(m->display_name,  "Arabic");
		ASSERT(m->text_direction == NAUI_TEXT_RTL);
		break;
	}

	ASSERT(found);

	naui_file_delete(file);

	TEST_END();
}

void localization_test(void)
{
	test_localization_basic();
	test_localization_interpolation();
	test_localization_missing_arg();
	test_localization_freeing();
	test_localization_global_set();
	test_localization_TR_macro();
	test_localization_global_fallback();
	test_localization_meta_cache_basic();
	test_localization_meta_cache_stale_list();
	test_localization_meta_cache_removed_file();
	test_localization_meta_cache_empty_dir();
	test_localization_meta_cache_corrupt_file();
	test_localization_meta_cache_no_meta_block();
	test_localization_meta_cache_metadata_fields();
}