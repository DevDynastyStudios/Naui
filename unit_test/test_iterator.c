#include "test.h"
#include "test_func.h"
#include "naui/filesystem/iterator.h"

#include <stdbool.h>
#include <stdio.h>

void test_dir_iterator_basic()
{
	TEST_BEGIN("Iterator - Basic");
	Naui_Path root = NAUI_PATH("test_dir_iter_basic");
	ASSERT(naui_directory_create(root));

	Naui_Path a_name = NAUI_PATH("a.txt");
	Naui_Path a_path = naui_path_join(root, a_name);
	naui_file_write_all(a_path, "x", 1);
	NAUI_PATH_FREE(a_name, a_path);
	
	Naui_Path b_name = NAUI_PATH("b.txt");
	Naui_Path b_path = naui_path_join(root, b_name);
	naui_file_write_all(b_path, "x", 1);
	NAUI_PATH_FREE(b_name, b_path);
	
	Naui_Path c_name = NAUI_PATH("c.bin");
	Naui_Path c_path = naui_path_join(root, c_name);
	naui_file_write_all(c_path, "x", 1);
	NAUI_PATH_FREE(c_name, c_path);
	
	Naui_DirIterator it = naui_dir_iterator_open(root, NULL, NULL, false);
	bool saw_a = false;
	bool saw_b = false;
	bool saw_c = false;

	while (naui_dir_iterator_valid(&it)) {
		Naui_StringView name = naui_file_filename(it.entry.path);
		if (naui_sv_cmp(name, NAUI_STR("a.txt"), true)) saw_a = true;
		if (naui_sv_cmp(name, NAUI_STR("b.txt"), true)) saw_b = true;
		if (naui_sv_cmp(name, NAUI_STR("c.bin"), true)) saw_c = true;

		naui_dir_iterator_next(&it);
	}

	naui_dir_iterator_close(&it);

	ASSERT(saw_a);
	ASSERT(saw_b);
	ASSERT(saw_c);

	naui_directory_remove_all(root);
	NAUI_PATH_FREE(root);
	TEST_END();
}

void test_dir_iterator_filter()
{
	TEST_BEGIN("Iterator - Filter");
	Naui_Path root = NAUI_PATH("test_dir_iter_filter");
	ASSERT(naui_directory_create(root));

	Naui_Path apple_name = NAUI_PATH("apple.txt");
	Naui_Path apple_path = naui_path_join(root, apple_name);
	Naui_Path banana_name = NAUI_PATH("banana.txt");
	Naui_Path banana_path = naui_path_join(root, banana_name);
	Naui_Path apricot_name = NAUI_PATH("apricot.txt");
	Naui_Path apricot_path = naui_path_join(root, apricot_name);
	naui_file_write_all(apple_path, "x", 1);
	naui_file_write_all(banana_path, "x", 1);
	naui_file_write_all(apricot_path, "x", 1);
	NAUI_PATH_FREE(apple_name, apple_path, banana_name, banana_path, apricot_name, apricot_path);

	/* NAUI_EXTENSIONS(...) - unlike NAUI_PATH(...) - just builds a
	 * plain local array of string-literal pointers (no allocation),
	 * matching how extensions arrays are used everywhere else in the
	 * codebase (naui_directory_filter etc. take a raw const char**
	 * with no ownership implications), so this is safe to use inline. */
	Naui_DirIterator it = naui_dir_iterator_open(root, "apple", NAUI_EXTENSIONS(".txt"), false);

	bool saw_apple = false;
	bool saw_apricot = false;
	bool saw_banana = false;

	while (naui_dir_iterator_valid(&it)) {
		Naui_StringView name = naui_file_filename(it.entry.path);
		if (naui_sv_cmp(name, NAUI_STR("apple.txt"), true)) saw_apple = true;
		if (naui_sv_cmp(name, NAUI_STR("apricot.txt"), true)) saw_apricot = true;
		if (naui_sv_cmp(name, NAUI_STR("banana.txt"), true)) saw_banana = true;

		naui_dir_iterator_next(&it);
	}

	naui_dir_iterator_close(&it);

	ASSERT(saw_apple);
	ASSERT(!saw_apricot);
	ASSERT(!saw_banana);

	naui_directory_remove_all(root);
	NAUI_PATH_FREE(root);
	TEST_END();
}

void test_dir_iterator_extensions()
{
	TEST_BEGIN("Iterator - Extensions");
	Naui_Path root = NAUI_PATH("test_dir_iter_ext");
	ASSERT(naui_directory_create(root));

	Naui_Path a_name = NAUI_PATH("a.txt");
	Naui_Path a_path = naui_path_join(root, a_name);
	Naui_Path b_name = NAUI_PATH("b.bin");
	Naui_Path b_path = naui_path_join(root, b_name);
	Naui_Path c_name = NAUI_PATH("c.txt");
	Naui_Path c_path = naui_path_join(root, c_name);
	naui_file_write_all(a_path, "x", 1);
	naui_file_write_all(b_path, "x", 1);
	naui_file_write_all(c_path, "x", 1);
	NAUI_PATH_FREE(a_name, a_path, b_name, b_path, c_name, c_path);

	const char* exts[] = { ".txt", NULL };

	Naui_DirIterator it = naui_dir_iterator_open(root, NULL, exts, false);

	bool saw_a = false;
	bool saw_b = false;
	bool saw_c = false;

	while (naui_dir_iterator_valid(&it)) {
		Naui_StringView name = naui_file_filename(it.entry.path);
		if (naui_sv_cmp(name, NAUI_STR("a.txt"), true)) saw_a = true;
		if (naui_sv_cmp(name, NAUI_STR("b.bin"), true)) saw_b = true;
		if (naui_sv_cmp(name, NAUI_STR("c.txt"), true)) saw_c = true;

		naui_dir_iterator_next(&it);
	}

	naui_dir_iterator_close(&it);

	ASSERT(saw_a);
	ASSERT(!saw_b);
	ASSERT(saw_c);

	naui_directory_remove_all(root);
	NAUI_PATH_FREE(root);
	TEST_END();
}

void test_dir_iterator_mixed()
{
	TEST_BEGIN("Iterator - MIXED");
	Naui_Path root = NAUI_PATH("test_dir_iter_mixed");
	ASSERT(naui_directory_create(root));

	// Files
	Naui_Path a_name = NAUI_PATH("a.txt");
	Naui_Path a_path = naui_path_join(root, a_name);
	Naui_Path b_name = NAUI_PATH("b.txt");
	Naui_Path b_path = naui_path_join(root, b_name);
	naui_file_write_all(a_path, "x", 1);
	naui_file_write_all(b_path, "x", 1);
	NAUI_PATH_FREE(a_name, a_path, b_name, b_path);

	// Directory
	Naui_Path sub_name = NAUI_PATH("folder");
	Naui_Path sub = naui_path_join(root, sub_name);
	naui_directory_create(sub);
	NAUI_PATH_FREE(sub_name, sub);

	Naui_DirIterator it = naui_dir_iterator_open(root, NULL, NULL, false);

	bool saw_a = false;
	bool saw_b = false;
	bool saw_folder = false;

	while (naui_dir_iterator_valid(&it)) {
		Naui_StringView name = naui_file_filename(it.entry.path);
		if (naui_sv_cmp(name, NAUI_STR("a.txt"), true)) saw_a = true;
		if (naui_sv_cmp(name, NAUI_STR("b.txt"), true)) saw_b = true;
		if (naui_sv_cmp(name, NAUI_STR("folder"), true) && it.entry.is_directory)
			saw_folder = true;

		naui_dir_iterator_next(&it);
	}

	naui_dir_iterator_close(&it);

	ASSERT(saw_a);
	ASSERT(saw_b);
	ASSERT(saw_folder);

	naui_directory_remove_all(root);
	NAUI_PATH_FREE(root);
	TEST_END();
}

void test_dir_iterator_empty()
{
	TEST_BEGIN("Iterator - Empty Directory");
	Naui_Path root = NAUI_PATH("test_iter_empty");
	ASSERT(naui_directory_create(root));
	Naui_DirIterator it = naui_dir_iterator_open(root, NULL, NULL, false);
	ASSERT(!naui_dir_iterator_valid(&it));

	naui_dir_iterator_close(&it);
	naui_directory_remove_all(root);
	NAUI_PATH_FREE(root);
	TEST_END();
}

void test_dir_iterator_invalid_path()
{
	TEST_BEGIN("Iterator - Invalid Path");
	Naui_Path root = NAUI_PATH("this_path_should_not_exist_12345");
	Naui_DirIterator it = naui_dir_iterator_open(root, NULL, NULL, false);
	ASSERT(!naui_dir_iterator_valid(&it));
	naui_dir_iterator_close(&it);
	NAUI_PATH_FREE(root);
	TEST_END();
}

void test_dir_iterator_hidden_files()
{
	TEST_BEGIN("Iterator - Hidden Files");
	Naui_Path root = NAUI_PATH("test_iter_hidden");
	ASSERT(naui_directory_create(root));

	// Create visible file
	Naui_Path visible_name = NAUI_PATH("visible.txt");
	Naui_Path visible = naui_path_join(root, visible_name);
	naui_file_write_all(visible, "x", 1);
	NAUI_PATH_FREE(visible_name);

	// Create hidden file
	Naui_Path hidden_name = NAUI_PATH("secret.txt");
	Naui_Path hidden_orig = naui_path_join(root, hidden_name);
	naui_file_write_all(hidden_orig, "x", 1);
	NAUI_PATH_FREE(hidden_name);

	/* naui_file_hide's contract: it may hand back a view aliasing the
	 * input (Windows, attribute-only - the name on disk never changes)
	 * or a freshly renamed, owned path (POSIX, which adds a leading
	 * dot). The ORIGINAL version of this test hardcoded the expected
	 * name as ".secret.txt", which only holds on POSIX - on Windows
	 * the file stays named "secret.txt", so that assertion would
	 * silently and permanently fail there. Fixed to check against
	 * whatever naui_file_hide actually produced, which is correct on
	 * both platforms. */
	Naui_Path hidden = naui_file_hide(hidden_orig, true);
	ASSERT(naui_file_is_hidden(hidden));

	Naui_StringView expected_hidden_name = naui_file_filename(hidden);

	Naui_DirIterator it = naui_dir_iterator_open(root, NULL, NULL, false);

	bool saw_visible = false;
	bool saw_hidden = false;

	while (naui_dir_iterator_valid(&it)) {
		Naui_StringView name = naui_file_filename(it.entry.path);
		if (naui_sv_cmp(name, NAUI_STR("visible.txt"), true)) saw_visible = true;
		if (naui_sv_cmp(name, expected_hidden_name, true) && naui_file_is_hidden(it.entry.path))
			saw_hidden = true;

		naui_dir_iterator_next(&it);
	}

	naui_dir_iterator_close(&it);

	ASSERT(saw_visible);
	ASSERT(saw_hidden);

	naui_directory_remove_all(root);

	/* Tell apart the view-vs-owned cases by pointer identity before
	 * freeing, same pattern as the filesystem test suite - correct on
	 * both platforms without needing an #ifdef. */
	if (hidden.data != hidden_orig.data)
		NAUI_PATH_FREE(hidden);
	NAUI_PATH_FREE(hidden_orig, visible, root);

	TEST_END();
}

void test_dir_iterator_filter_and_ext()
{
	TEST_BEGIN("Iterator - Filter and Exit");
	Naui_Path root = NAUI_PATH("test_iter_filter_ext");
	ASSERT(naui_directory_create(root));

	Naui_Path apple_name = NAUI_PATH("apple.txt");
	Naui_Path apple_path = naui_path_join(root, apple_name);
	Naui_Path banana_name = NAUI_PATH("banana.png");
	Naui_Path banana_path = naui_path_join(root, banana_name);
	Naui_Path apricot_name = NAUI_PATH("apricot.bin");
	Naui_Path apricot_path = naui_path_join(root, apricot_name);
	naui_file_write_all(apple_path, "x", 1);
	naui_file_write_all(banana_path, "x", 1);
	naui_file_write_all(apricot_path, "x", 1);
	NAUI_PATH_FREE(apple_name, apple_path, banana_name, banana_path, apricot_name, apricot_path);

	const char* exts[] = { ".txt", ".bin", NULL };

	Naui_DirIterator it = naui_dir_iterator_open(root, "apricot", exts, true);

	bool saw_apple = false;
	bool saw_banana = false;
	bool saw_apricot = false;

	while (naui_dir_iterator_valid(&it)) {
		Naui_StringView name = naui_file_filename(it.entry.path);
		if (naui_sv_cmp(name, NAUI_STR("apple.txt"), true)) saw_apple = true;
		if (naui_sv_cmp(name, NAUI_STR("banana.txt"), true)) saw_banana = true;
		if (naui_sv_cmp(name, NAUI_STR("apricot.bin"), true)) saw_apricot = true;

		naui_dir_iterator_next(&it);
	}

	naui_dir_iterator_close(&it);

	ASSERT(!saw_apple);
	ASSERT(!saw_banana);
	ASSERT(saw_apricot);

	naui_directory_remove_all(root);
	NAUI_PATH_FREE(root);
	TEST_END();
}

void test_dir_iterator_reuse()
{
	TEST_BEGIN("Iterator - Reuse");
	Naui_Path root = NAUI_PATH("test_iter_reuse");
	ASSERT(naui_directory_create(root));

	Naui_Path a_name = NAUI_PATH("a.txt");
	Naui_Path a_path = naui_path_join(root, a_name);
	naui_file_write_all(a_path, "x", 1);
	NAUI_PATH_FREE(a_name, a_path);

	// First iteration
	{
		Naui_DirIterator it = naui_dir_iterator_open(root, NULL, NULL, false);
		ASSERT(naui_dir_iterator_valid(&it));
		naui_dir_iterator_next(&it);
		ASSERT(!naui_dir_iterator_valid(&it));
		naui_dir_iterator_close(&it);
	}

	// Second iteration (fresh)
	{
		Naui_DirIterator it = naui_dir_iterator_open(root, NULL, NULL, false);
		ASSERT(naui_dir_iterator_valid(&it));
		naui_dir_iterator_close(&it);
	}

	naui_directory_remove_all(root);
	NAUI_PATH_FREE(root);
	TEST_END();
}

void test_dir_iterator_no_parent_entries()
{
	TEST_BEGIN("Iterator - No Parent Entries");
	Naui_Path root = NAUI_PATH("test_iter_no_parent");
	ASSERT(naui_directory_create(root));

	Naui_DirIterator it = naui_dir_iterator_open(root, NULL, NULL, false);

	while (naui_dir_iterator_valid(&it)) {
		Naui_StringView name = naui_file_filename(it.entry.path);
		ASSERT(!naui_sv_cmp(name, NAUI_STR("."), true));
		ASSERT(!naui_sv_cmp(name, NAUI_STR(".."), true));
		naui_dir_iterator_next(&it);
	}

	naui_dir_iterator_close(&it);
	naui_directory_remove_all(root);
	NAUI_PATH_FREE(root);
	TEST_END();
}

void test_dir_iterator_large()
{
	TEST_BEGIN("Iterator - Large Directory");

	Naui_Path root = NAUI_PATH("test_iter_large");
	ASSERT(naui_directory_create(root));

	const int COUNT = 500;

	for (int i = 0; i < COUNT; i++)
	{
		/* A small fixed buffer, not NAUI_PATH_MAX (32768 on Windows) -
		 * this is always going to be "file_NNN.txt". The original had
		 * `Naui_Path name; snprintf(name.data, NAUI_PATH_MAX, ...)`,
		 * which writes through an uninitialized pointer - name.data
		 * was never assigned, so this was undefined behaviour / a
		 * likely crash. */
		char name_buf[64];
		snprintf(name_buf, sizeof(name_buf), "file_%03d.txt", i);
		Naui_Path name = naui_path_from_cstr(name_buf); /* view - no allocation */
		Naui_Path p = naui_path_join(root, name);
		naui_file_write_all(p, "x", 1);
		NAUI_PATH_FREE(p);
	}

	Naui_DirIterator it = naui_dir_iterator_open(root, NULL, NULL, false);

	int seen = 0;

	while (naui_dir_iterator_valid(&it)) {
		seen++;
		naui_dir_iterator_next(&it);
	}

	naui_dir_iterator_close(&it);

	ASSERT(seen == COUNT);
	ASSERT(seen > 1);

	naui_directory_remove_all(root);
	NAUI_PATH_FREE(root);
	TEST_END();
}

void iterator_test()
{
	test_dir_iterator_basic();
	test_dir_iterator_filter();
	test_dir_iterator_extensions();
	test_dir_iterator_mixed();
	test_dir_iterator_empty();
	test_dir_iterator_invalid_path();
	test_dir_iterator_hidden_files();
	test_dir_iterator_filter_and_ext();
	test_dir_iterator_reuse();
	test_dir_iterator_no_parent_entries();
	test_dir_iterator_large();
}