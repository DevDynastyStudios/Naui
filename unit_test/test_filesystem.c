#include "test.h"
#include "test_func.h"
#include "naui/base.h"
#include "naui/filesystem/filesystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#define SEP "\\"
#else
#define SEP "/"
#endif

static Naui_Path TEST_ROOT;

/* Thin, non-allocating wrap of a C string - callers own the string's
 * lifetime, same contract as naui_path_from_cstr. */
static Naui_Path make_path(const char* s)
{
    return naui_path_from_cstr(s);
}

static void init_test_root(void)
{
    Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
    Naui_Path naui_test = NAUI_PATH("naui_test");
    Naui_Path joined = naui_path_join(cwd, naui_test);
    NAUI_PATH_FREE(cwd, naui_test);

    TEST_ROOT = naui_path_normalize(joined);
    NAUI_PATH_FREE(joined);

    naui_directory_create(TEST_ROOT);
}

/* Builds a path under TEST_ROOT. Always an owned allocation - the
 * caller must naui_path_free() the result. */
static Naui_Path tp(const char* sub)
{
    Naui_Path joined = naui_path_join(TEST_ROOT, make_path(sub));
    Naui_Path result = naui_path_normalize(joined);
    NAUI_PATH_FREE(joined);
    return result;
}

static void write_text(const Naui_Path path, const char* text)
{
    naui_file_write_all(path, text, strlen(text));
}

static void test_file_open(void)
{
    TEST_BEGIN("naui_file_open / naui_file_is_valid / naui_file_close");

    {
        Naui_FileHandle h = NAUI_FILE_HANDLE_INIT;

        Naui_Path open_test = tp("open_test.txt");
        ASSERT(naui_file_open(&h, open_test, NAUI_FILE_WRITE));
        ASSERT(naui_file_is_valid(&h));
        naui_file_close(&h);
        ASSERT(!naui_file_is_valid(&h));
        NAUI_PATH_FREE(open_test);

        /* Nonexistent file opened for reading must fail */
        Naui_Path not_exists = tp("does_not_exist.txt");
        ASSERT(!naui_file_open(&h, not_exists, NAUI_FILE_READ));
        ASSERT(!naui_file_is_valid(&h));
        NAUI_PATH_FREE(not_exists);

        /* NULL handle */
        Naui_Path txt_file = tp("x.txt");
        ASSERT(!naui_file_open(NULL, txt_file, NAUI_FILE_WRITE));
        NAUI_PATH_FREE(txt_file);

        /* Empty path */
        Naui_Path empty = NAUI_PATH("");
        ASSERT(!naui_file_open(&h, empty, NAUI_FILE_WRITE));
        NAUI_PATH_FREE(empty);
    }

    TEST_END();
}

static void test_file_write_read(void)
{
    TEST_BEGIN("naui_file_write / naui_file_read");

    {
        const char* payload = "Hello, Naui!";
        size_t plen = strlen(payload);

        Naui_Path rw_test = tp("rw_test.txt");
        Naui_FileHandle wh = NAUI_FILE_HANDLE_INIT;
        ASSERT(naui_file_open(&wh, rw_test, NAUI_FILE_WRITE));
        ASSERT(naui_file_write(&wh, payload, plen) == plen);
        naui_file_close(&wh);

        char buf[64] = {0};
        Naui_FileHandle rh = NAUI_FILE_HANDLE_INIT;
        ASSERT(naui_file_open(&rh, rw_test, NAUI_FILE_READ));
        size_t got = naui_file_read(&rh, buf, sizeof(buf) - 1);
        naui_file_close(&rh);

        ASSERT(got == plen);
        ASSERT_STR_EQ(buf, payload);
        NAUI_PATH_FREE(rw_test);

        /* Invalid handle returns 0 */
        Naui_FileHandle bad = NAUI_FILE_HANDLE_INIT;
        ASSERT(naui_file_read(&bad, buf, sizeof(buf)) == 0);
    }

    TEST_END();
}

static void test_file_append(void)
{
    TEST_BEGIN("naui_file_append");

    {
        Naui_Path append_test = tp("append_test.txt");
        write_text(append_test, "Line1\n");

        Naui_FileHandle ah = NAUI_FILE_HANDLE_INIT;
        ASSERT(naui_file_open(&ah, append_test, NAUI_FILE_APPEND));
        const char* line2 = "Line2\n";
        naui_file_write(&ah, line2, strlen(line2));
        naui_file_close(&ah);

        size_t sz;
        char* all = naui_file_read_all(append_test, &sz);
        ASSERT_NOT_NULL(all);
        ASSERT_STR_EQ(all, "Line1\nLine2\n");
        free(all); /* raw malloc'd buffer, not a Naui_Path - plain free() */

        NAUI_PATH_FREE(append_test);
    }

    TEST_END();
}

static void test_file_size(void)
{
    TEST_BEGIN("naui_file_size");

    {
        Naui_Path size_test = tp("size_test.txt");
        Naui_Path no_file = tp("no_such_file.txt");
        Naui_Path empty = NAUI_PATH("");

        write_text(size_test, "12345");
        ASSERT(naui_file_size(size_test) == 5);
        ASSERT(naui_file_size(no_file) == 0);
        ASSERT(naui_file_size(empty) == 0);

        NAUI_PATH_FREE(size_test, no_file, empty);
    }

    TEST_END();
}

static void test_file_read_all(void)
{
    TEST_BEGIN("naui_file_read_all");

    {
        Naui_Path read_all = tp("readall_test.txt");
        const char* content = "All your base";
        write_text(read_all, content);

        size_t sz = 0;
        char* buf = naui_file_read_all(read_all, &sz);
        ASSERT_NOT_NULL(buf);
        ASSERT(sz == strlen(content));
        ASSERT_STR_EQ(buf, content);
        free(buf);

        /* out_size can be NULL */
        buf = naui_file_read_all(read_all, NULL);
        ASSERT_NOT_NULL(buf);
        free(buf);

        /* Nonexistent / empty path return NULL */
        Naui_Path ghost = tp("ghost.txt");
        Naui_Path empty = NAUI_PATH("");
        ASSERT_NULL(naui_file_read_all(ghost, &sz));
        ASSERT_NULL(naui_file_read_all(empty, &sz));

        NAUI_PATH_FREE(read_all, ghost, empty);
    }

    TEST_END();
}

static void test_file_write_all(void)
{
    TEST_BEGIN("naui_file_write_all");

    {
        Naui_Path write_all = tp("writeall.txt");
        Naui_Path empty = NAUI_PATH("");
        const char* data = "WriteAll test";

        ASSERT(naui_file_write_all(write_all, data, strlen(data)));
        ASSERT(!naui_file_write_all(empty, data, strlen(data)));
        ASSERT(!naui_file_write_all(write_all, NULL, 10));

        char* back = naui_file_read_all(write_all, NULL);
        ASSERT_NOT_NULL(back);
        ASSERT_STR_EQ(back, data);
        free(back);

        NAUI_PATH_FREE(write_all, empty);
    }

    TEST_END();
}

static void test_file_seek(void)
{
    TEST_BEGIN("naui_file_seek");

    {
        Naui_Path seek_test = tp("seek_test.txt");
        write_text(seek_test, "ABCDEFGHIJ");

        Naui_FileHandle h = NAUI_FILE_HANDLE_INIT;
        ASSERT(naui_file_open(&h, seek_test, NAUI_FILE_READ));

        char buf[4] = {0};

        ASSERT(naui_file_seek(&h, 3, SEEK_SET));
        naui_file_read(&h, buf, 3);
        buf[3] = '\0';
        ASSERT_STR_EQ(buf, "DEF");

        ASSERT(naui_file_seek(&h, -2, SEEK_CUR));
        naui_file_read(&h, buf, 2);
        buf[2] = '\0';
        ASSERT_STR_EQ(buf, "EF");

        naui_file_close(&h);

        ASSERT(!naui_file_seek(&h, 0, SEEK_SET));
        ASSERT(!naui_file_seek(NULL, 0, SEEK_SET));

        NAUI_PATH_FREE(seek_test);
    }

    TEST_END();
}

static void test_file_delete_rename(void)
{
    TEST_BEGIN("naui_file_delete / naui_file_rename");

    {
        Naui_Path not_test = NAUI_PATH("Test.txt");
        ASSERT(!naui_path_exists(not_test));
        NAUI_PATH_FREE(not_test);

        Naui_Path delete_me = tp("del_me.txt");
        write_text(delete_me, "bye");
        ASSERT(naui_path_exists(delete_me));
        ASSERT(naui_file_delete(delete_me));
        ASSERT(!naui_path_exists(delete_me));
        ASSERT(!naui_file_delete(delete_me));
        NAUI_PATH_FREE(delete_me);

        Naui_Path rename_src = tp("rename_src.txt");
        Naui_Path rename_dst = tp("rename_dst.txt");
        write_text(rename_src, "rename");
        ASSERT(naui_file_rename(rename_src, rename_dst));
        ASSERT(!naui_path_exists(rename_src));
        ASSERT(naui_path_exists(rename_dst));
        naui_file_delete(rename_dst);
        NAUI_PATH_FREE(rename_src, rename_dst);

        Naui_Path x_files = tp("x.txt");
        Naui_Path empty = NAUI_PATH("");
        ASSERT(!naui_file_delete(empty));
        ASSERT(!naui_file_rename(empty, x_files));
        ASSERT(!naui_file_rename(x_files, empty));
        NAUI_PATH_FREE(x_files, empty);
    }

    TEST_END();
}

static void test_file_filename(void)
{
    TEST_BEGIN("naui_file_filename");

    {
        /* naui_file_filename returns a VIEW into its input - it must be
         * read before the input is freed, and must never be freed itself. */
        Naui_Path p1 = tp("foo/bar/baz.txt");
        Naui_StringView f1 = naui_file_filename(p1);
        ASSERT_STR_EQ(f1.data, "baz.txt");
        NAUI_PATH_FREE(p1);

        Naui_Path p2 = tp("baz.txt");
        Naui_StringView f2 = naui_file_filename(p2);
        ASSERT_STR_EQ(f2.data, "baz.txt");
        NAUI_PATH_FREE(p2);

        Naui_Path p3 = NAUI_PATH("");
        Naui_StringView f3 = naui_file_filename(p3);
        ASSERT(f3.len == 0);
        NAUI_PATH_FREE(p3);
    }

    TEST_END();
}

static void test_file_stem(void)
{
    TEST_BEGIN("naui_file_stem");

    {
        /* Unlike filename/extension, stem is a PREFIX of the filename,
         * so it can't safely be a view (the byte after it is '.', not
         * a null terminator) - it's always an owned allocation. */
        Naui_Path file = tp("dir/file.txt");
        Naui_StringView s1 = naui_file_stem(file);
        ASSERT(naui_sv_cmp(s1, NAUI_STR("file"), true));
        NAUI_PATH_FREE(file);

        Naui_Path archive = tp("dir/archive.tar.gz");
        Naui_StringView s2 = naui_file_stem(archive);
        ASSERT(naui_sv_cmp(s2, NAUI_STR("archive.tar"), true));
        NAUI_PATH_FREE(archive);

        /* No extension - full filename is the stem */
        Naui_Path noext = tp("dir/noext");
        Naui_StringView s3 = naui_file_stem(noext);
        ASSERT_STR_EQ(s3.data, "noext");
        NAUI_PATH_FREE(noext);

        /* Dotfile - the whole name is the stem */
        Naui_Path hidden = tp("dir/.hidden");
        Naui_StringView s4 = naui_file_stem(hidden);
        ASSERT_STR_EQ(s4.data, ".hidden");
        NAUI_PATH_FREE(hidden);
    }

    TEST_END();
}

static void test_file_extension(void)
{
    TEST_BEGIN("naui_file_extension");

    {
        /* extension IS a suffix, so (unlike stem) it's a view - use it
         * before freeing the path it came from. */
        Naui_Path file = tp("file.txt");
        Naui_StringView e1 = naui_file_extension(file);
        ASSERT_STR_EQ(e1.data, ".txt");
        NAUI_PATH_FREE(file);

        Naui_Path archive = tp("archive.tar.gz");
        Naui_StringView e2 = naui_file_extension(archive);
        ASSERT_STR_EQ(e2.data, ".gz");
        NAUI_PATH_FREE(archive);

        /* No extension - empty result */
        Naui_Path noext = tp("noext");
        Naui_StringView e3 = naui_file_extension(noext);
        ASSERT(e3.len == 0);
        NAUI_PATH_FREE(noext);

        /* Dotfile has no extension */
        Naui_Path hidden = tp(".hidden");
        Naui_StringView e4 = naui_file_extension(hidden);
        ASSERT(e4.len == 0);
        NAUI_PATH_FREE(hidden);
    }

    TEST_END();
}

static void test_file_hide_is_hidden(void)
{
    TEST_BEGIN("naui_file_hide / naui_file_is_hidden");

    {
        Naui_Path original = tp("hide_test.txt");
        write_text(original, "hidden");

        ASSERT(!naui_file_is_hidden(original));

        /* naui_file_hide's contract: it may hand back the exact same
         * path unchanged (a view aliasing the input - e.g. Windows,
         * which only flips an attribute) or a freshly renamed, owned
         * path (e.g. POSIX, which adds/strips a leading dot). Which
         * one you get isn't tracked at runtime, so we tell them apart
         * by pointer identity before freeing, which is correct on
         * both platforms without needing an #ifdef here. */
        Naui_Path hidden = naui_file_hide(original, true);
        ASSERT(naui_file_is_hidden(hidden));
        ASSERT(naui_path_exists(hidden));

        Naui_Path unhidden = naui_file_hide(hidden, false);
        ASSERT(!naui_file_is_hidden(unhidden));
        ASSERT(naui_path_exists(unhidden));

        naui_file_delete(unhidden);

        if (unhidden.data != hidden.data)
            NAUI_PATH_FREE(unhidden);
        if (hidden.data != original.data)
            NAUI_PATH_FREE(hidden);
        NAUI_PATH_FREE(original);

        /* Empty path: hide is a no-op and always returns empty */
        Naui_Path empty = NAUI_PATH("");
        Naui_Path result = naui_file_hide(empty, true);
        ASSERT(naui_path_is_empty(result));
        ASSERT(!naui_file_is_hidden(empty));
        NAUI_PATH_FREE(empty, result);
    }

    TEST_END();
}

static void test_path_exists(void)
{
    TEST_BEGIN("naui_path_exists");

    {
        Naui_Path exists = tp("exists_check.txt");
        Naui_Path ghost = tp("ghost_file_xyz.txt");
        Naui_Path empty = NAUI_PATH("");

        write_text(exists, "x");
        ASSERT(naui_path_exists(exists));
        ASSERT(!naui_path_exists(ghost));
        ASSERT(!naui_path_exists(empty));

        naui_file_delete(exists);
        NAUI_PATH_FREE(exists, ghost, empty);
    }

    TEST_END();
}

static void test_path_parent(void)
{
    TEST_BEGIN("naui_path_parent");

    {
        Naui_Path baz = tp("foo/bar/baz.txt");
        Naui_Path p1 = naui_path_parent(baz);

        Naui_Path foo = tp("foo/bar");
        ASSERT_STR_EQ(p1.data, foo.data);
        NAUI_PATH_FREE(baz, p1, foo);

        Naui_Path root = NAUI_PATH("/foo");
        Naui_Path p2 = naui_path_parent(root);
        ASSERT_STR_EQ(p2.data, "/");
        NAUI_PATH_FREE(root, p2);

        Naui_Path bare = NAUI_PATH("only_filename");
        Naui_Path p3 = naui_path_parent(bare);
        ASSERT_STR_EQ(p3.data, ".");
        NAUI_PATH_FREE(bare, p3);

        /* Parent of an empty path is "." - same as a bare filename
         * with no separators, not an empty result. */
        Naui_Path empty = NAUI_PATH("");
        Naui_Path p4 = naui_path_parent(empty);
        ASSERT_STR_EQ(p4.data, ".");
        NAUI_PATH_FREE(empty, p4);
    }

    TEST_END();
}

static void test_path_join(void)
{
    TEST_BEGIN("naui_path_join");

    {
        Naui_Path base = NAUI_PATH("/foo/bar");
        Naui_Path rel = NAUI_PATH("baz.txt");

        Naui_Path joined = naui_path_join(base, rel);
        ASSERT_STR_EQ(joined.data, "/foo/bar" SEP "baz.txt");
        NAUI_PATH_FREE(joined);

        /* naui_path_join always allocates fresh, even in these
         * "trivial" cases - never a passthrough view - so every result
         * here is safe to free independently. */
        Naui_Path abs_b = NAUI_PATH("/absolute/path");
        Naui_Path joined_abs = naui_path_join(base, abs_b);
        ASSERT_STR_EQ(joined_abs.data, "/foo/bar" SEP "absolute/path");
        NAUI_PATH_FREE(abs_b, joined_abs);

        /* Empty a returns (a copy of) b */
        Naui_Path empty = NAUI_PATH("");
        Naui_Path from_empty = naui_path_join(empty, rel);
        ASSERT_STR_EQ(from_empty.data, rel.data);
        NAUI_PATH_FREE(from_empty);

        Naui_Path abs = NAUI_PATH("/foo/bar");
        Naui_Path abs_joined = naui_path_join(empty, abs);
        ASSERT_STR_EQ(abs_joined.data, abs.data);
        NAUI_PATH_FREE(abs_joined);

        NAUI_PATH_FREE(base, rel, empty, abs);
    }

    TEST_END();
}

static void test_path_variadic_join(void)
{
	TEST_BEGIN("NAUI_PATH variadic join");

	{
		/* Single argument behaves like the old naui_path_from_cstr,
		 * except NAUI_PATH always allocates. */
		Naui_Path p = NAUI_PATH("file.txt");
		ASSERT_STR_EQ(p.data, "file.txt");
		NAUI_PATH_FREE(p);
	}

	{
		/* Two plain parts. */
		Naui_Path p = NAUI_PATH("Language", "/en-US.lang");
		ASSERT_STR_EQ(p.data, "Language" SEP "en-US.lang");
		NAUI_PATH_FREE(p);
	}

	{
		/* Three parts, the classic motivating example. */
		Naui_Path p = NAUI_PATH("Some/", "Folder", "file.txt");
		ASSERT_STR_EQ(p.data, "Some" SEP "Folder" SEP "file.txt");
		NAUI_PATH_FREE(p);
	}

	{
		/* Trailing separator on an earlier part doesn't double up. */
		Naui_Path p = NAUI_PATH("a" SEP, "b");
		ASSERT_STR_EQ(p.data, "a" SEP "b");
		NAUI_PATH_FREE(p);
	}

	{
		/* Leading separator on a later part is normalized, not a
		 * reset - consistent with naui_path_join's no-reset behavior,
		 * since NAUI_PATH(...) shares the same join logic. */
		Naui_Path p = NAUI_PATH("a", SEP "b", SEP "c");
		ASSERT_STR_EQ(p.data, "a" SEP "b" SEP "c");
		NAUI_PATH_FREE(p);
	}

	{
		/* Separators on both sides of a seam collapse to exactly one. */
		Naui_Path p = NAUI_PATH("a" SEP, SEP "b");
		ASSERT_STR_EQ(p.data, "a" SEP "b");
		NAUI_PATH_FREE(p);
	}

	{
		/* Empty string parts are skipped, no double separators. */
		Naui_Path p = NAUI_PATH("a", "", "b");
		ASSERT_STR_EQ(p.data, "a" SEP "b");
		NAUI_PATH_FREE(p);
	}

	{
		/* A leading separator on the FIRST part is preserved - that's
		 * what makes an absolute path absolute. */
		Naui_Path p = NAUI_PATH(SEP "root", "child");
		ASSERT_STR_EQ(p.data, SEP "root" SEP "child");
		NAUI_PATH_FREE(p);
	}

	{
		/* Many parts, mixed separator placement at each seam. */
		Naui_Path p = NAUI_PATH("one", "two" SEP, SEP "three", "four" SEP);
		ASSERT_STR_EQ(p.data, "one" SEP "two" SEP "three" SEP "four");
		NAUI_PATH_FREE(p);
	}

	{
		/* Using NAUI_PATH(...) to build from an existing Naui_Path's
		 * .data plus string literals - the realistic call pattern
		 * (e.g. bin_dir.data, "Language", code, ".lang"). */
		Naui_Path bin_dir = NAUI_PATH("usr", "local", "MyApp");
		char filename[64];
		snprintf(filename, sizeof(filename), "%s.lang", "en-US");

		Naui_Path lang_file = NAUI_PATH(bin_dir.data, "Language", filename);
		ASSERT_STR_EQ(lang_file.data, "usr" SEP "local" SEP "MyApp" SEP "Language" SEP "en-US.lang");
		NAUI_PATH_FREE(bin_dir, lang_file);
	}

	TEST_END();
}

static void test_path_normalize(void)
{
    TEST_BEGIN("naui_path_normalize");

    {
        Naui_Path p1 = NAUI_PATH("/foo/bar/../baz/./qux");
        Naui_Path n1 = naui_path_normalize(p1);
#if NAUI_WINDOWS
        ASSERT_STR_EQ(n1.data, "\\foo\\baz\\qux");
#else
        ASSERT_STR_EQ(n1.data, "/foo/baz/qux");
#endif
        NAUI_PATH_FREE(p1, n1);

        Naui_Path p2 = NAUI_PATH(".");
        Naui_Path n2 = naui_path_normalize(p2);
        ASSERT_STR_EQ(n2.data, ".");
        NAUI_PATH_FREE(p2, n2);

        Naui_Path p3 = NAUI_PATH("a/b/../../c");
        Naui_Path n3 = naui_path_normalize(p3);
        ASSERT_STR_EQ(n3.data, "c");
        NAUI_PATH_FREE(p3, n3);
    }

    TEST_END();
}

static void test_path_absolute(void)
{
    TEST_BEGIN("naui_path_absolute");

    {
        /* An already-absolute path is returned as a view aliasing the
         * input - only the input needs freeing. */
        Naui_Path abs_in = NAUI_PATH("/already/absolute");
        Naui_Path abs_out = naui_path_absolute(abs_in);
        ASSERT_STR_EQ(abs_out.data, "/already/absolute");
        NAUI_PATH_FREE(abs_in);

        /* A relative path gets the cwd prepended - owned, since it's
         * genuinely new data. */
        Naui_Path rel = NAUI_PATH("relative_file.txt");
        Naui_Path made_abs = naui_path_absolute(rel);

        Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
        ASSERT(strncmp(made_abs.data, cwd.data, strlen(cwd.data)) == 0);

        NAUI_PATH_FREE(rel, made_abs, cwd);
    }

    TEST_END();
}

static void test_path_canonical(void)
{
    TEST_BEGIN("naui_path_canonical");

    {
        Naui_Path canon = naui_path_canonical(TEST_ROOT);
        ASSERT(!naui_path_is_empty(canon));
        NAUI_PATH_FREE(canon);

        Naui_Path missing = tp("does_not_exist_for_canonical");
        Naui_Path bad = naui_path_canonical(missing);
        ASSERT(naui_path_is_empty(bad));
        NAUI_PATH_FREE(missing, bad);

        Naui_Path empty_in = NAUI_PATH("");
        Naui_Path empty_out = naui_path_canonical(empty_in);
        ASSERT(naui_path_is_empty(empty_out));
        NAUI_PATH_FREE(empty_in, empty_out);
    }

    TEST_END();
}

static void test_path_weakly_canonical(void)
{
    TEST_BEGIN("naui_path_weakly_canonical");

    {
        /* Fully existing path behaves like canonical */
        Naui_Path wc1 = naui_path_weakly_canonical(TEST_ROOT);
        ASSERT(!naui_path_is_empty(wc1));
        NAUI_PATH_FREE(wc1);

        /* Partially existing: existing prefix is canonicalized,
         * non-existing tail is normalized and appended. */
        Naui_Path check_dir = tp("nonexistent_dir/and/sub");
        Naui_Path wc2 = naui_path_weakly_canonical(check_dir);
        ASSERT(!naui_path_is_empty(wc2));
        ASSERT(strstr(wc2.data, "nonexistent_dir") != NULL);
        NAUI_PATH_FREE(check_dir, wc2);
    }

    TEST_END();
}

static void test_directory_create_remove(void)
{
    TEST_BEGIN("naui_directory_create / naui_directory_remove");

    {
        Naui_Path mkdir_path = tp("mkdir_test");
        ASSERT(naui_directory_create(mkdir_path));
        ASSERT(naui_path_exists(mkdir_path));
        ASSERT(naui_directory_create(mkdir_path)); /* EEXIST is OK */
        ASSERT(naui_directory_remove(mkdir_path));
        ASSERT(!naui_path_exists(mkdir_path));
        NAUI_PATH_FREE(mkdir_path);

        Naui_Path empty = NAUI_PATH("");
        ASSERT(!naui_directory_create(empty));
        ASSERT(!naui_directory_remove(empty));
        NAUI_PATH_FREE(empty);
    }

    TEST_END();
}

static void test_directory_remove_all(void)
{
    TEST_BEGIN("naui_directory_remove_all");

    {
        Naui_Path rm = tp("rm_root");
        naui_directory_create(rm);

        Naui_Path sub = tp("rm_root" SEP "sub");
        naui_directory_create(sub);

        Naui_Path leaf = tp("rm_root" SEP "sub" SEP "leaf.txt");
        write_text(leaf, "delete me");

        ASSERT(naui_path_exists(leaf));
        ASSERT(naui_directory_remove_all(rm));
        ASSERT(!naui_path_exists(rm));

        Naui_Path empty = NAUI_PATH("");
        ASSERT(!naui_directory_remove_all(empty));

        NAUI_PATH_FREE(rm, sub, leaf, empty);
    }

    TEST_END();
}

static void test_directory_rename(void)
{
    TEST_BEGIN("naui_directory_rename");

    {
        Naui_Path src = tp("dir_rename_src");
        Naui_Path dst = tp("dir_rename_dst");

        naui_directory_create(src);
        ASSERT(naui_directory_rename(src, dst));
        ASSERT(!naui_path_exists(src));
        ASSERT(naui_path_exists(dst));

        Naui_Path empty = NAUI_PATH("");
        ASSERT(!naui_directory_rename(empty, dst));
        ASSERT(!naui_directory_rename(dst, empty));
        ASSERT(naui_directory_remove(dst));

        NAUI_PATH_FREE(src, dst, empty);
    }

    TEST_END();
}

static void test_directory_get(void)
{
    TEST_BEGIN("naui_directory_get");

    {
        /* WORKING is backed by a mutable cache, so it's always an
         * owned copy - must be freed. */
        Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
        ASSERT(!naui_path_is_empty(cwd));
        NAUI_PATH_FREE(cwd);

        /* HOME/BIN are write-once caches returned as views into static
         * storage - must NOT be freed. */
        Naui_Path home = naui_directory_get(NAUI_DIR_HOME);
        ASSERT(!naui_path_is_empty(home));

        Naui_Path bin = naui_directory_get(NAUI_DIR_BIN);
        ASSERT(!naui_path_is_empty(bin));

        /* Invalid enum - naui_path_empty(), harmless either way */
        Naui_Path bad = naui_directory_get((Naui_Dir)9999);
        ASSERT(naui_path_is_empty(bad));
    }

    TEST_END();
}

static void test_directory_filter(void)
{
    TEST_BEGIN("naui_directory_filter");

    {
        Naui_Path root = tp("filter_root");
        naui_directory_create(root);

        Naui_Path a = tp("filter_root" SEP "a.txt");
        Naui_Path b = tp("filter_root" SEP "b.log");
        Naui_Path c = tp("filter_root" SEP "c.txt");

        write_text(a, "A");
        write_text(b, "B");
        write_text(c, "C");

        /* We want only .txt files */
        const char* exts[] = { ".txt" };

        Naui_List(Naui_DirEntry) list =
            naui_directory_filter(root, NULL, exts, 1);

        ASSERT(list != NULL);

        size_t count = (size_t)naui_list_len(list);
        ASSERT(count == 2);

        bool saw_a = false;
        bool saw_c = false;

        for (size_t i = 0; i < count; i++)
        {
            const char* p = list[i].path.data;

            if (strcmp(p, a.data) == 0) saw_a = true;
            if (strcmp(p, c.data) == 0) saw_c = true;
        }

        ASSERT(saw_a);
        ASSERT(saw_c);

        naui_directory_filter_free(list); /* frees each entry's path, then the list */
        naui_directory_remove_all(root);

        NAUI_PATH_FREE(root, a, b, c);
    }

    TEST_END();
}

static void test_path_absolute_parent(void)
{
    TEST_BEGIN("naui_path_absolute with parent component");

#if NAUI_WINDOWS
    Naui_Path rel = NAUI_PATH("..\\foo");
#else
    Naui_Path rel = NAUI_PATH("../foo");
#endif

    Naui_Path abs = naui_path_absolute(rel);
    Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);

    char expected[NAUI_PATH_MAX];
#if NAUI_WINDOWS
    snprintf(expected, NAUI_PATH_MAX, "%s\\..\\foo", cwd.data);
#else
    snprintf(expected, NAUI_PATH_MAX, "%s/../foo", cwd.data);
#endif

    ASSERT_STR_EQ(abs.data, expected);

    NAUI_PATH_FREE(rel, abs, cwd);

    TEST_END();
}

static void test_path_lock(void)
{
    TEST_BEGIN("naui_path_lock / naui_path_is_locked / naui_path_unlock");

    {
        Naui_Path lockfile = tp("lock_test.txt");
        write_text(lockfile, "lock");

        ASSERT(!naui_path_is_locked(lockfile));
        ASSERT(naui_path_lock(lockfile));
        ASSERT(naui_path_is_locked(lockfile));

        /* Already locked */
        ASSERT(!naui_path_lock(lockfile));

        naui_path_unlock(lockfile);
        ASSERT(!naui_path_is_locked(lockfile));

        /* Unlocking again should be harmless */
        naui_path_unlock(lockfile);

        /* Invalid inputs */
        Naui_Path empty = NAUI_PATH("");
        ASSERT(!naui_path_lock(empty));
        ASSERT(!naui_path_is_locked(empty));
        naui_path_unlock(empty);

        NAUI_PATH_FREE(lockfile, empty);
    }

    TEST_END();
}

static void test_path_lock_independent(void)
{
    TEST_BEGIN("naui_path_lock independent paths");

    {
        Naui_Path a = tp("lockA.txt");
        Naui_Path b = tp("lockB.txt");

        write_text(a, "A");
        write_text(b, "B");

        /* Lock A and B independently */
        ASSERT(naui_path_lock(a));
        ASSERT(naui_path_lock(b));
        ASSERT(naui_path_is_locked(a));
        ASSERT(naui_path_is_locked(b));

        /* Unlock A does not unlock B */
        naui_path_unlock(a);
        ASSERT(!naui_path_is_locked(a));
        ASSERT(naui_path_is_locked(b));

        /* Unlock B */
        naui_path_unlock(b);
        ASSERT(!naui_path_is_locked(b));

        NAUI_PATH_FREE(a, b);
    }

    TEST_END();
}

static void test_current_directory_all(void)
{
    TEST_BEGIN("current directory set/get behavior");

    Naui_Path original = naui_directory_get(NAUI_DIR_WORKING);

    {
        Naui_Path dir = tp("cwd_test_valid");
        naui_directory_create(dir);

        ASSERT(naui_path_set_current(dir));

        Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
        ASSERT_STR_EQ(cwd.data, dir.data);
        NAUI_PATH_FREE(cwd);

        ASSERT(naui_path_set_current(original));
        NAUI_PATH_FREE(dir);
    }

    {
        Naui_Path before = naui_directory_get(NAUI_DIR_WORKING);

        Naui_Path empty = NAUI_PATH("");
        ASSERT(!naui_path_set_current(empty));
        NAUI_PATH_FREE(empty);

        Naui_Path after = naui_directory_get(NAUI_DIR_WORKING);
        ASSERT_STR_EQ(before.data, after.data);
        NAUI_PATH_FREE(before, after);
    }

    {
        Naui_Path before = naui_directory_get(NAUI_DIR_WORKING);

        Naui_Path bad = tp("this_directory_should_not_exist_12345");
        ASSERT(!naui_path_set_current(bad));
        NAUI_PATH_FREE(bad);

        Naui_Path after = naui_directory_get(NAUI_DIR_WORKING);
        ASSERT_STR_EQ(before.data, after.data);
        NAUI_PATH_FREE(before, after);
    }

    {
        Naui_Path dir = tp("cwd_test_after_set");
        naui_directory_create(dir);

        ASSERT(naui_path_set_current(dir));

        Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
        ASSERT_STR_EQ(cwd.data, dir.data);
        NAUI_PATH_FREE(cwd);

        ASSERT(naui_path_set_current(original));
        NAUI_PATH_FREE(dir);
    }

    {
        /* Ensure we have a stable cwd */
        ASSERT(naui_path_set_current(original));

        Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
        Naui_Path parent = naui_path_parent(cwd);

        ASSERT(naui_path_set_current(parent));

        Naui_Path now = naui_directory_get(NAUI_DIR_WORKING);
        ASSERT_STR_EQ(now.data, parent.data);
        NAUI_PATH_FREE(now);

        ASSERT(naui_path_set_current(original));
        NAUI_PATH_FREE(cwd, parent);
    }

    NAUI_PATH_FREE(original);

    TEST_END();
}

static void test_cleanup(void)
{
	TEST_BEGIN("Cleanup test root directory");
	ASSERT(naui_directory_remove_all(TEST_ROOT));
	ASSERT(!naui_path_exists(TEST_ROOT));
	ASSERT(!naui_directory_remove_all(TEST_ROOT));
	NAUI_PATH_FREE(TEST_ROOT); /* release TEST_ROOT's own memory - separate from removing the directory from disk above */
	TEST_END();
}

void filesystem_test(void)
{
    init_test_root();

    test_file_open();
    test_file_write_read();
    test_file_append();
    test_file_size();
    test_file_read_all();
    test_file_write_all();
    test_file_seek();
    test_file_delete_rename();
    test_file_filename();
    test_file_stem();
    test_file_extension();
    test_file_hide_is_hidden();
    test_path_exists();
    test_path_parent();
    test_path_join();
	test_path_variadic_join();
    test_path_normalize();
    test_path_absolute();
    test_path_canonical();
    test_path_weakly_canonical();
    test_directory_create_remove();
    test_directory_remove_all();
    test_directory_rename();
    test_directory_get();
    test_path_absolute_parent();
    test_directory_filter();
    test_path_lock();
    test_path_lock_independent();
    test_current_directory_all();
	test_cleanup();
}
