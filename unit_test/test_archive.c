#include "test.h"
#include "test_func.h"
#include "naui/filesystem/archive.h"

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#  define SEP "\\"
#else
#  define SEP "/"
#endif

static Naui_Path temp_root(void)
{
	Naui_Path p;
#if defined(_WIN32) || defined(_WIN64)
	const char* tmp = getenv("TEMP");
	if (!tmp) tmp = "C:\\Temp";
	snprintf(p.data, NAUI_PATH_MAX, "%s\\naui_archive_test", tmp);
#else
	snprintf(p.data, NAUI_PATH_MAX, "/tmp/naui_archive_test");
#endif
	return p;
}

static Naui_Path tp(const char* sub)
{
	enum { TP_POOL_SIZE = 32 };
	static Naui_Path pool[TP_POOL_SIZE];
	static size_t    idx = 0;
	idx = (idx + 1) % TP_POOL_SIZE;

	Naui_Path p = pool[idx];
	Naui_Path  root = temp_root();
	snprintf(p.data, NAUI_PATH_MAX, "%s" SEP "%s", root.data, sub);
	return p;
}

static void write_text(const Naui_Path path, const char* text)
{
	naui_file_write_all(path, text, strlen(text));
}

/*
 * Build a fresh source tree under a given root for each test that needs one.
 *
 *   <root>/
 *     hello.txt       "Hello"
 *     world.txt       "World"
 *     sub/
 *       nested.txt    "Nested"
 */
static void build_src_tree(const Naui_Path root)
{
	naui_directory_create(root);

	Naui_Path sub, hello, world, nested;
	snprintf(sub.data,    NAUI_PATH_MAX, "%s" SEP "sub",             root.data);
	snprintf(hello.data,  NAUI_PATH_MAX, "%s" SEP "hello.txt",       root.data);
	snprintf(world.data,  NAUI_PATH_MAX, "%s" SEP "world.txt",       root.data);
	snprintf(nested.data, NAUI_PATH_MAX, "%s" SEP "sub" SEP "nested.txt", root.data);

	naui_directory_create(sub);
	write_text(hello,  "Hello");
	write_text(world,  "World");
	write_text(nested, "Nested");
}

/* -------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */

static void test_archive_open_invalid(void)
{
	TEST_BEGIN("naui_archive_open - invalid path");

	{
		Naui_Archive a = NAUI_ARCHIVE_INIT;
		naui_archive_open(&a, tp("does_not_exist.zip"), NAUI_ARCHIVE_READ);
		ASSERT(!naui_archive_is_valid(&a));
		naui_archive_close(&a);

		Naui_Archive b = NAUI_ARCHIVE_INIT;
		naui_archive_open(&b, tp("no_dir" SEP "out.zip"), NAUI_ARCHIVE_WRITE);
		ASSERT(!naui_archive_is_valid(&b));
		naui_archive_close(&b);
	}

	TEST_END();
}

static void test_archive_add_extract_file(void)
{
	TEST_BEGIN("naui_archive_add_file / naui_archive_extract_file");

	{
		write_text(tp("single_src.txt"), "SingleFile");

		Naui_Archive w = NAUI_ARCHIVE_INIT;
		naui_archive_open(&w, tp("single.zip"), NAUI_ARCHIVE_WRITE);
		ASSERT(naui_archive_is_valid(&w));

		Naui_Path dest_name;
		snprintf(dest_name.data, NAUI_PATH_MAX, "single_src.txt");
		ASSERT(naui_archive_add_file(&w, tp("single_src.txt"), dest_name));
		naui_archive_close(&w);
		ASSERT(naui_path_exists(tp("single.zip")));

		Naui_Archive r = NAUI_ARCHIVE_INIT;
		naui_archive_open(&r, tp("single.zip"), NAUI_ARCHIVE_READ);
		ASSERT(naui_archive_is_valid(&r));
		ASSERT(naui_archive_extract_file(&r, dest_name, tp("extracted_single.txt")));
		naui_archive_close(&r);

		size_t sz;
		char* content = naui_file_read_all(tp("extracted_single.txt"), &sz);
		ASSERT_NOT_NULL(content);
		ASSERT_STR_EQ(content, "SingleFile");
		free(content);
	}

	TEST_END();
}

static void test_archive_add_folder_extract_to(void)
{
	TEST_BEGIN("naui_archive_add_folder / naui_archive_extract_to");

	{
		build_src_tree(tp("src_folder"));

		Naui_Archive w = NAUI_ARCHIVE_INIT;
		naui_archive_open(&w, tp("folder.zip"), NAUI_ARCHIVE_WRITE);
		ASSERT(naui_archive_is_valid(&w));

		Naui_Path no_prefix;
		no_prefix.data[0] = '\0';
		ASSERT(naui_archive_add_folder(&w, tp("src_folder"), no_prefix));
		naui_archive_close(&w);
		ASSERT(naui_path_exists(tp("folder.zip")));

		naui_directory_create(tp("dst_folder"));
		Naui_Archive r = NAUI_ARCHIVE_INIT;
		naui_archive_open(&r, tp("folder.zip"), NAUI_ARCHIVE_READ);
		ASSERT(naui_archive_is_valid(&r));
		ASSERT(naui_archive_extract_to(&r, tp("dst_folder")));
		naui_archive_close(&r);

		size_t sz;
		char* hello = naui_file_read_all(tp("dst_folder" SEP "hello.txt"), &sz);
		ASSERT_NOT_NULL(hello);
		ASSERT_STR_EQ(hello, "Hello");
		free(hello);

		char* nested = naui_file_read_all(tp("dst_folder" SEP "sub" SEP "nested.txt"), &sz);
		ASSERT_NOT_NULL(nested);
		ASSERT_STR_EQ(nested, "Nested");
		free(nested);
	}

	TEST_END();
}

static void test_archive_list_entries(void)
{
	TEST_BEGIN("naui_archive_list_entries");

	{
		/* Build and pack a fresh tree — does not depend on any other test */
		build_src_tree(tp("src_list"));

		Naui_Archive w = NAUI_ARCHIVE_INIT;
		naui_archive_open(&w, tp("list.zip"), NAUI_ARCHIVE_WRITE);
		ASSERT(naui_archive_is_valid(&w));
		Naui_Path no_prefix;
		no_prefix.data[0] = '\0';
		ASSERT(naui_archive_add_folder(&w, tp("src_list"), no_prefix));
		naui_archive_close(&w);

		Naui_Archive r = NAUI_ARCHIVE_INIT;
		naui_archive_open(&r, tp("list.zip"), NAUI_ARCHIVE_READ);
		ASSERT(naui_archive_is_valid(&r));

		Naui_List(Naui_ArchiveEntry) entries = naui_archive_list_entries(&r);
		naui_archive_close(&r);

		ASSERT_NOT_NULL(entries);
		ASSERT(naui_list_len(entries) >= 3);

		bool found_hello  = false;
		bool found_nested = false;
		for (ptrdiff_t i = 0; i < naui_list_len(entries); ++i)
		{
			if (strstr(entries[i].path.data, "hello.txt"))  found_hello  = true;
			if (strstr(entries[i].path.data, "nested.txt")) found_nested = true;
		}

		ASSERT(found_hello);
		ASSERT(found_nested);
		naui_archive_list_free(entries);
	}

	TEST_END();
}

static void test_archive_move(void)
{
	TEST_BEGIN("naui_archive_move");

	{
		/* Build the archive this test needs independently */
		write_text(tp("move_src.txt"), "MoveTest");

		Naui_Archive w = NAUI_ARCHIVE_INIT;
		naui_archive_open(&w, tp("move.zip"), NAUI_ARCHIVE_WRITE);
		ASSERT(naui_archive_is_valid(&w));
		Naui_Path dest_name;
		snprintf(dest_name.data, NAUI_PATH_MAX, "move_src.txt");
		ASSERT(naui_archive_add_file(&w, tp("move_src.txt"), dest_name));
		naui_archive_close(&w);

		Naui_Archive src = NAUI_ARCHIVE_INIT;
		naui_archive_open(&src, tp("move.zip"), NAUI_ARCHIVE_READ);
		ASSERT(naui_archive_is_valid(&src));

		Naui_Archive dst;
		memset(&dst, 0, sizeof(dst));
		naui_archive_move(&dst, &src);

		ASSERT(naui_archive_is_valid(&dst));
		ASSERT(!naui_archive_is_valid(&src));

		naui_archive_close(&dst);
	}

	TEST_END();
}

static void test_archive_mode_guard(void)
{
	TEST_BEGIN("naui_archive - read/write mode guards");

	{
		/* Build the archive this test needs independently */
		write_text(tp("guard_src.txt"), "GuardTest");

		Naui_Archive w = NAUI_ARCHIVE_INIT;
		naui_archive_open(&w, tp("guard.zip"), NAUI_ARCHIVE_WRITE);
		ASSERT(naui_archive_is_valid(&w));
		Naui_Path dest_name;
		snprintf(dest_name.data, NAUI_PATH_MAX, "guard_src.txt");
		ASSERT(naui_archive_add_file(&w, tp("guard_src.txt"), dest_name));
		naui_archive_close(&w);

		/* Adding to a read archive must fail */
		Naui_Archive r = NAUI_ARCHIVE_INIT;
		naui_archive_open(&r, tp("guard.zip"), NAUI_ARCHIVE_READ);
		ASSERT(naui_archive_is_valid(&r));
		ASSERT(!naui_archive_add_file(&r, tp("guard_src.txt"), dest_name));
		naui_archive_close(&r);

		/* Listing from a write archive must fail */
		Naui_Archive wg = NAUI_ARCHIVE_INIT;
		naui_archive_open(&wg, tp("guard_write.zip"), NAUI_ARCHIVE_WRITE);
		ASSERT(naui_archive_is_valid(&wg));
		Naui_List(Naui_ArchiveEntry) list = naui_archive_list_entries(&wg);
		ASSERT_NULL(list);
		naui_archive_close(&wg);
	}

	TEST_END();
}

static void test_archive_custom_create_extract(void)
{
	TEST_BEGIN("naui_archive_create_custom / naui_archive_extract_custom");

	{
		build_src_tree(tp("src_custom"));

		ASSERT(naui_archive_create_custom(tp("src_custom"), tp("custom.naui")));
		ASSERT(naui_path_exists(tp("custom.naui")));

		naui_directory_create(tp("custom_out"));
		ASSERT(naui_archive_extract_custom(tp("custom.naui"), tp("custom_out")));

		size_t sz;
		char* hello = naui_file_read_all(tp("custom_out" SEP "hello.txt"), &sz);
		ASSERT_NOT_NULL(hello);
		ASSERT_STR_EQ(hello, "Hello");
		free(hello);

		char* world = naui_file_read_all(tp("custom_out" SEP "world.txt"), &sz);
		ASSERT_NOT_NULL(world);
		ASSERT_STR_EQ(world, "World");
		free(world);

		char* nested = naui_file_read_all(tp("custom_out" SEP "sub" SEP "nested.txt"), &sz);
		ASSERT_NOT_NULL(nested);
		ASSERT_STR_EQ(nested, "Nested");
		free(nested);
	}

	TEST_END();
}

static void test_archive_custom_bad_magic(void)
{
	TEST_BEGIN("naui_archive_extract_custom - bad magic");

	{
		write_text(tp("bad.naui"), "NOT_A_VALID_ARCHIVE");
		naui_directory_create(tp("bad_out"));
		ASSERT(!naui_archive_extract_custom(tp("bad.naui"), tp("bad_out")));
	}

	TEST_END();
}

void archive_test()
{
	Naui_Path root = temp_root();
	naui_directory_create(root);

	test_archive_open_invalid();
	test_archive_add_extract_file();
	test_archive_add_folder_extract_to();
	test_archive_list_entries();
	test_archive_move();
	test_archive_mode_guard();
	test_archive_custom_create_extract();
	test_archive_custom_bad_magic();

	naui_directory_remove_all(root);
}