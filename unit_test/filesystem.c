#include "test.h"
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

/* Build a NauiPath for the shared temp root. */
static Naui_Path temp_root(void)
{
	Naui_Path p;
#if defined(_WIN32) || defined(_WIN64)
	const char* tmp = getenv("TEMP");
	if (!tmp) tmp = "C:\\Temp";
	snprintf(p.data, NAUI_PATH_MAX, "%s\\naui_test", tmp);
#else
	snprintf(p.data, NAUI_PATH_MAX, "/tmp/naui_test");
#endif
	return p;
}

/* This sucks, but it's just for unit testing */
static Naui_Path* tp(const char* sub)
{
	enum { TP_POOL_SIZE = 32 };
	static Naui_Path pool[TP_POOL_SIZE];
	static size_t index = 0;
	index = (index + 1) % TP_POOL_SIZE;

	Naui_Path* p = &pool[index];
	Naui_Path root = temp_root();
#if NAUI_WINDOWS
	snprintf(p->data, NAUI_PATH_MAX, "%s\\%s", root.data, sub);
#else
	snprintf(p->data, NAUI_PATH_MAX, "%s/%s", root.data, sub);
#endif

	return p;
}

/* Write text to a path - used to set up test fixtures. */
static void write_text(Naui_Path* path, const char* text)
{
	naui_file_write_all(path, text, strlen(text));
}

/* Empty NauiPath used to exercise the empty-input guard on each function. */
static Naui_Path* empty_path(void)
{
	static Naui_Path p;
	p.data[0] = '\0';
	return &p;
}

static void test_file_open(void)
{
	TEST_BEGIN("naui_file_open / naui_file_is_valid / naui_file_close");

	{
		Naui_FileHandle h = NAUI_FILE_HANDLE_INIT;

		Naui_Path* open_test = tp("open_test.txt");
		ASSERT(naui_file_open(&h, open_test, NAUI_FILE_WRITE));
		ASSERT(naui_file_is_valid(&h));
		naui_file_close(&h);
		ASSERT(!naui_file_is_valid(&h));

		/* Nonexistent file opened for reading must fail */
		Naui_Path* not_exists = tp("does_not_exist.txt");
		ASSERT(!naui_file_open(&h, not_exists, NAUI_FILE_READ));
		ASSERT(!naui_file_is_valid(&h));

		/* NULL handle */
		Naui_Path* txt_file = tp("x.txt");
		ASSERT(!naui_file_open(NULL, txt_file, NAUI_FILE_WRITE));

		/* Empty path */
		ASSERT(!naui_file_open(&h, empty_path(), NAUI_FILE_WRITE));
	}

	TEST_END();
}

static void test_file_write_read(void)
{
	TEST_BEGIN("naui_file_write / naui_file_read");

	{
		const char* payload = "Hello, Naui!";
		size_t plen = strlen(payload);

		Naui_Path* rw_test = tp("rw_test.txt");
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
		Naui_Path* append_test = tp("append_test.txt");
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
		free(all);
	}

	TEST_END();
}

static void test_file_size(void)
{
	TEST_BEGIN("naui_file_size");

	{
		Naui_Path* size_test = tp("size_test.txt");
		Naui_Path* no_file = tp("no_such_file.txt");

		write_text(size_test, "12345");
		ASSERT(naui_file_size(size_test) == 5);
		ASSERT(naui_file_size(no_file) == 0);
		ASSERT(naui_file_size(empty_path()) == 0);
	}

	TEST_END();
}

static void test_file_read_all(void)
{
	TEST_BEGIN("naui_file_read_all");

	{
		Naui_Path* read_all = tp("readall_test.txt");
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
		Naui_Path* ghost = tp("ghost.txt");
		ASSERT_NULL(naui_file_read_all(ghost, &sz));
		ASSERT_NULL(naui_file_read_all(empty_path(), &sz));
	}

	TEST_END();
}

static void test_file_write_all(void)
{
	TEST_BEGIN("naui_file_write_all");

	{
		Naui_Path* write_all = tp("writeall.txt");
		const char* data = "WriteAll test";

		ASSERT(naui_file_write_all(write_all, data, strlen(data)));
		ASSERT(!naui_file_write_all(empty_path(), data, strlen(data)));
		ASSERT(!naui_file_write_all(write_all, NULL, 10));

		char* back = naui_file_read_all(write_all, NULL);
		ASSERT_NOT_NULL(back);
		ASSERT_STR_EQ(back, data);
		free(back);
	}

	TEST_END();
}

static void test_file_seek(void)
{
	TEST_BEGIN("naui_file_seek");

	{
		Naui_Path* seek_test = tp("seek_test.txt");
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
	}

	TEST_END();
}

static void test_file_delete_rename(void)
{
	TEST_BEGIN("naui_file_delete / naui_file_rename");

	{
		ASSERT(!naui_path_exists(&NAUI_PATH("Test.txt")));

		Naui_Path* delete_me = tp("del_me.txt");
		write_text(delete_me, "bye");
		ASSERT(naui_path_exists(delete_me));
		ASSERT(naui_file_delete(delete_me));
		ASSERT(!naui_path_exists(delete_me));
		ASSERT(!naui_file_delete(delete_me));

		Naui_Path* rename_src = tp("rename_src.txt");
		Naui_Path* rename_dst = tp("rename_dst.txt");
		write_text(rename_src, "rename");
		ASSERT(naui_file_rename(rename_src, rename_dst));
		ASSERT(!naui_path_exists(rename_src));
		ASSERT(naui_path_exists(rename_dst));
		naui_file_delete(rename_dst);

		Naui_Path* x_files = tp("x.txt");
		ASSERT(!naui_file_delete(empty_path()));
		ASSERT(!naui_file_rename(empty_path(), x_files));
		ASSERT(!naui_file_rename(x_files, empty_path()));
	}

	TEST_END();
}

static void test_file_filename(void)
{
	TEST_BEGIN("naui_file_filename");

	{
		Naui_Path* p1 = tp("foo/bar/baz.txt");
		ASSERT_STR_EQ(naui_file_filename(p1), "baz.txt");

		Naui_Path* p2 = tp("baz.txt");
		ASSERT_STR_EQ(naui_file_filename(p2), "baz.txt");

		Naui_Path* p3 = empty_path();
		ASSERT_STR_EQ(naui_file_filename(p3), "");
	}

	TEST_END();
}

static void test_file_stem(void)
{
	TEST_BEGIN("naui_file_stem");

	{
		Naui_Path* file = tp("dir/file.txt");
		Naui_Path s1 = naui_file_stem(file);
		ASSERT_STR_EQ(s1.data, "file");

		Naui_Path* archive = tp("dir/archive.tar.gz");
		Naui_Path s2 = naui_file_stem(archive);
		ASSERT_STR_EQ(s2.data, "archive.tar");

		/* No extension - full filename is the stem */
		Naui_Path* noext = tp("dir/noext");
		Naui_Path s3 = naui_file_stem(noext);
		ASSERT_STR_EQ(s3.data, "noext");

		/* Dotfile - the whole name is the stem */
		Naui_Path* hidden = tp("dir/.hidden");
		Naui_Path s4 = naui_file_stem(hidden);
		ASSERT_STR_EQ(s4.data, ".hidden");
	}

	TEST_END();
}

static void test_file_extension(void)
{
	TEST_BEGIN("naui_file_extension");

	{
		Naui_Path* file = tp("file.txt");
		Naui_Path e1 = naui_file_extension(file);
		ASSERT_STR_EQ(e1.data, ".txt");

		Naui_Path* archive = tp("archive.tar.gz");
		Naui_Path e2 = naui_file_extension(archive);
		ASSERT_STR_EQ(e2.data, ".gz");

		/* No extension - empty path returned */
		Naui_Path* noext = tp("noext");
		Naui_Path e3 = naui_file_extension(noext);
		ASSERT(e3.data[0] == '\0');

		/* Dotfile has no extension */
		Naui_Path* hidden = tp(".hidden");
		Naui_Path e4 = naui_file_extension(hidden);
		ASSERT(e4.data[0] == '\0');
	}

	TEST_END();
}

static void test_file_hide_is_hidden(void)
{
	TEST_BEGIN("naui_file_hide / naui_file_is_hidden");

	{
#if defined(_WIN32) || defined(_WIN64)
		Naui_Path* hide = tp("hide_test.txt");
		write_text(hide, "hidden");

		ASSERT(!naui_file_is_hidden(hide));
		ASSERT(naui_file_hide(hide, true));
		ASSERT(naui_file_is_hidden(hide));
		ASSERT(naui_file_hide(hide, false));
		ASSERT(!naui_file_is_hidden(hide));
		naui_file_delete(hide);
#else
		/* On Unix, hiding renames the file by prepending a dot. */
		Naui_Path* visible = tp("hide_me.txt");
		Naui_Path* hidden = tp(".hide_me.txt");

		write_text(visible, "hide me");
		ASSERT(!naui_file_is_hidden(visible));
		ASSERT(naui_file_hide(visible, true));
		ASSERT(naui_path_exists(hidden));
		ASSERT(naui_file_is_hidden(hidden));
		ASSERT(naui_file_hide(hidden, false));
		ASSERT(naui_path_exists(visible));
		ASSERT(!naui_file_is_hidden(visible));
		naui_file_delete(visible);
#endif
		ASSERT(!naui_file_hide(empty_path(), true));
		ASSERT(!naui_file_is_hidden(empty_path()));
	}

	TEST_END();
}

static void test_path_exists(void)
{
	TEST_BEGIN("naui_path_exists");

	{
		Naui_Path* exists = tp("exists_check.txt");
		Naui_Path* ghost = tp("ghost_file_xyz.txt");

		write_text(exists, "x");
		ASSERT(naui_path_exists(exists));
		ASSERT(!naui_path_exists(ghost));
		ASSERT(!naui_path_exists(empty_path()));
		naui_file_delete(exists);
	}

	TEST_END();
}

static void test_path_parent(void)
{
	TEST_BEGIN("naui_path_parent");

	{
		Naui_Path* baz = tp("foo/bar/baz.txt");
		Naui_Path p1 = naui_path_parent(baz);

		Naui_Path* foo = tp("foo/bar");
		ASSERT_STR_EQ(p1.data, foo->data);

		Naui_Path root;
		snprintf(root.data, NAUI_PATH_MAX, "/foo");
		Naui_Path p2 = naui_path_parent(&root);
		ASSERT_STR_EQ(p2.data, "/");

		Naui_Path bare;
		snprintf(bare.data, NAUI_PATH_MAX, "only_filename");
		Naui_Path p3 = naui_path_parent(&bare);
		ASSERT_STR_EQ(p3.data, ".");

		Naui_Path p4 = naui_path_parent(empty_path());
		ASSERT_STR_EQ(p4.data, ".");
	}

	TEST_END();
}

static void test_path_join(void)
{
	TEST_BEGIN("naui_path_join");

	{
		Naui_Path base;
		snprintf(base.data, NAUI_PATH_MAX, "/foo/bar");

		Naui_Path rel;
		snprintf(rel.data, NAUI_PATH_MAX, "baz.txt");

		Naui_Path joined = naui_path_join(&base, &rel);
		ASSERT_STR_EQ(joined.data, "/foo/bar" SEP "baz.txt");

		/* Absolute b replaces a */
		Naui_Path abs_b;
		snprintf(abs_b.data, NAUI_PATH_MAX, "/absolute/path");
		Naui_Path replaced = naui_path_join(&base, &abs_b);
		ASSERT_STR_EQ(replaced.data, "/absolute/path");

		/* Empty a returns b */
		Naui_Path* empty = empty_path();
		Naui_Path from_empty = naui_path_join(empty, &rel);
		ASSERT_STR_EQ(from_empty.data, rel.data);

		Naui_Path* abs = tp("/foo/bar");
		Naui_Path abs_joined = naui_path_join(empty, abs);
		ASSERT_STR_EQ(abs_joined.data, abs->data);
	}

	TEST_END();
}

static void test_path_normalize(void)
{
	TEST_BEGIN("naui_path_normalize");

	{
		Naui_Path p1;
		snprintf(p1.data, NAUI_PATH_MAX, "/foo/bar/../baz/./qux");
		Naui_Path n1 = naui_path_normalize(&p1);
#if defined(_WIN32) || defined(_WIN64)
		ASSERT_STR_EQ(n1.data, "\\foo\\baz\\qux");
#else
		ASSERT_STR_EQ(n1.data, "/foo/baz/qux");
#endif

		Naui_Path p2;
		snprintf(p2.data, NAUI_PATH_MAX, ".");
		Naui_Path n2 = naui_path_normalize(&p2);
		ASSERT_STR_EQ(n2.data, ".");

		Naui_Path p3;
		snprintf(p3.data, NAUI_PATH_MAX, "a/b/../../c");
		Naui_Path n3 = naui_path_normalize(&p3);
		ASSERT_STR_EQ(n3.data, "c");
	}

	TEST_END();
}

static void test_path_absolute(void)
{
	TEST_BEGIN("naui_path_absolute");

	{
		/* An already-absolute path is returned unchanged */
		Naui_Path abs_in;
		snprintf(abs_in.data, NAUI_PATH_MAX, "/already/absolute");
		Naui_Path abs_out = naui_path_absolute(&abs_in);
		ASSERT_STR_EQ(abs_out.data, "/already/absolute");

		/* A relative path gets the cwd prepended */
		Naui_Path rel;
		snprintf(rel.data, NAUI_PATH_MAX, "relative_file.txt");
		Naui_Path made_abs = naui_path_absolute(&rel);

		Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
		ASSERT(strncmp(made_abs.data, cwd.data, strlen(cwd.data)) == 0);
	}

	TEST_END();
}

static void test_path_canonical(void)
{
	TEST_BEGIN("naui_path_canonical");

	{
		Naui_Path temp = temp_root();
		Naui_Path canon = naui_path_canonical(&temp);
		ASSERT(canon.data[0] != '\0');

		Naui_Path* canonical = tp("does_not_exist_for_canonical");
		Naui_Path bad = naui_path_canonical(canonical);
		ASSERT(bad.data[0] == '\0');

		Naui_Path empty = naui_path_canonical(empty_path());
		ASSERT(empty.data[0] == '\0');
	}

	TEST_END();
}

static void test_path_weakly_canonical(void)
{
	TEST_BEGIN("naui_path_weakly_canonical");

	{
		/* Fully existing path behaves like canonical */
		Naui_Path temp = temp_root();
		Naui_Path wc1 = naui_path_weakly_canonical(&temp);
		ASSERT(wc1.data[0] != '\0');

		/* Partially existing: existing prefix is canonicalized,
		 * non-existing tail is normalized and appended. */
		Naui_Path* check_dir = tp("nonexistent_dir/and/sub");
		Naui_Path wc2 = naui_path_weakly_canonical(check_dir);
		ASSERT(wc2.data[0] != '\0');
		ASSERT(strstr(wc2.data, "nonexistent_dir") != NULL);
	}

	TEST_END();
}

static void test_directory_create_remove(void)
{
	TEST_BEGIN("naui_directory_create / naui_directory_remove");

	{
		Naui_Path* mkdir = tp("mkdir_test");
		ASSERT(naui_directory_create(mkdir));
		ASSERT(naui_path_exists(mkdir));
		ASSERT(naui_directory_create(mkdir)); /* EEXIST is OK */
		ASSERT(naui_directory_remove(mkdir));
		ASSERT(!naui_path_exists(mkdir));

		ASSERT(!naui_directory_create(empty_path()));
		ASSERT(!naui_directory_remove(empty_path()));
	}

	TEST_END();
}

static void test_directory_remove_all(void)
{
	TEST_BEGIN("naui_directory_remove_all");

	{
		Naui_Path* rm = tp("rm_root");
		naui_directory_create(rm);

		Naui_Path* sub = tp("rm_root" SEP "sub");
		naui_directory_create(sub);

		Naui_Path* leaf = tp("rm_root" SEP "sub" SEP "leaf.txt");
		write_text(leaf, "delete me");

		ASSERT(naui_path_exists(leaf));
		ASSERT(naui_directory_remove_all(rm));
		ASSERT(!naui_path_exists(rm));

		ASSERT(!naui_directory_remove_all(empty_path()));
	}

	TEST_END();
}

static void test_directory_rename(void)
{
	TEST_BEGIN("naui_directory_rename");

	{
		Naui_Path* src = tp("dir_rename_src");
		Naui_Path* dst = tp("dir_rename_dst");

		naui_directory_create(src);
		ASSERT(naui_directory_rename(src, dst));
		ASSERT(!naui_path_exists(src));
		ASSERT(naui_path_exists(dst));

		ASSERT(!naui_directory_rename(empty_path(), dst));
		ASSERT(!naui_directory_rename(dst, empty_path()));

		naui_directory_remove(dst);
	}

	TEST_END();
}

static void test_directory_get(void)
{
	TEST_BEGIN("naui_directory_get");

	{
		Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
		ASSERT(cwd.data[0] != '\0');

		Naui_Path home = naui_directory_get(NAUI_DIR_HOME);
		ASSERT(home.data[0] != '\0');

		Naui_Path bin = naui_directory_get(NAUI_DIR_BIN);
		ASSERT(bin.data[0] != '\0');

		/* Invalid enum */
		Naui_Path bad = naui_directory_get((Naui_Dir)9999);
		ASSERT(bad.data[0] == '\0');
	}

	TEST_END();
}

static void test_directory_filter(void)
{
	TEST_BEGIN("naui_directory_filter");

	{
		Naui_Path* root = tp("filter_root");
		naui_directory_create(root);

		Naui_Path* a = tp("filter_root" SEP "a.txt");
		Naui_Path* b = tp("filter_root" SEP "b.log");
		Naui_Path* c = tp("filter_root" SEP "c.txt");

		write_text(a, "A");
		write_text(b, "B");
		write_text(c, "C");

		/* We want only .txt files */
		const char* exts[] = { ".txt" };

		Naui_List(Naui_DirEntry) list =
			naui_directory_filter(root, NULL, exts, 1);

		ASSERT(list != NULL);

		size_t count = naui_list_len(list);
		ASSERT(count == 2);

		bool saw_a = false;
		bool saw_c = false;

		for (size_t i = 0; i < count; i++)
		{
			const char* p = list[i].path.data;

			if (strcmp(p, a->data) == 0) saw_a = true;
			if (strcmp(p, c->data) == 0) saw_c = true;
		}

		ASSERT(saw_a);
		ASSERT(saw_c);

		naui_directory_filter_free(list);
		naui_directory_remove_all(root);
	}

	TEST_END();
}

static void test_path_absolute_parent(void)
{
	TEST_BEGIN("naui_path_absolute with parent component");

#if NAUI_WINDOWS
	Naui_Path* rel = &NAUI_PATH("..\\foo");
#else
	Naui_Path* rel = &NAUI_PATH("../foo");
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

	TEST_END();
}

static void test_path_lock(void)
{
	TEST_BEGIN("naui_path_lock / naui_path_is_locked / naui_path_unlock");

	{
		Naui_Path* lockfile = tp("lock_test.txt");
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
		ASSERT(!naui_path_lock(empty_path()));
		ASSERT(!naui_path_is_locked(empty_path()));
		naui_path_unlock(empty_path());
	}

	TEST_END();
}

static void test_path_lock_independent(void)
{
	TEST_BEGIN("naui_path_lock independent paths");

	{
		Naui_Path* a = tp("lockA.txt");
		Naui_Path* b = tp("lockB.txt");

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
	}

	TEST_END();
}

static void test_current_directory_all(void)
{
	TEST_BEGIN("current directory set/get behavior");
	Naui_Path original = naui_directory_get(NAUI_DIR_WORKING);

	{
		Naui_Path* dir = tp("cwd_test_valid");
		naui_directory_create(dir);

		ASSERT(naui_path_set_current(dir));

		Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
		ASSERT_STR_EQ(cwd.data, dir->data);

		ASSERT(naui_path_set_current(&original));
	}

	{
		Naui_Path before = naui_directory_get(NAUI_DIR_WORKING);

		Naui_Path* empty = empty_path();
		ASSERT(!naui_path_set_current(empty));

		Naui_Path after = naui_directory_get(NAUI_DIR_WORKING);
		ASSERT_STR_EQ(before.data, after.data);
	}

	{
		Naui_Path before = naui_directory_get(NAUI_DIR_WORKING);

		Naui_Path* bad = tp("this_directory_should_not_exist_12345");
		ASSERT(!naui_path_set_current(bad));

		Naui_Path after = naui_directory_get(NAUI_DIR_WORKING);
		ASSERT_STR_EQ(before.data, after.data);
	}

	{
		Naui_Path* dir = tp("cwd_test_after_set");
		naui_directory_create(dir);

		ASSERT(naui_path_set_current(dir));

		Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
		ASSERT_STR_EQ(cwd.data, dir->data);

		ASSERT(naui_path_set_current(&original));
	}

	{
		/* Ensure we have a stable cwd */
		ASSERT(naui_path_set_current(&original));

		Naui_Path cwd = naui_directory_get(NAUI_DIR_WORKING);
		Naui_Path parent = naui_path_parent(&cwd);

		ASSERT(naui_path_set_current(&parent));

		Naui_Path now = naui_directory_get(NAUI_DIR_WORKING);
		ASSERT_STR_EQ(now.data, parent.data);

		ASSERT(naui_path_set_current(&original));
	}

	TEST_END();
}


int main(void)
{
	Naui_Path root = temp_root();
	naui_directory_create(&root);

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

	return test_summary();
}
