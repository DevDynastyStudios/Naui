/*
 * test_filesystem.c - Unit tests for naui_filesystem (naui_filesystem)
 *
 * Tests are self-contained: they create temp files/dirs under /tmp/naui_test/
 * (or %TEMP%\naui_test\ on Windows) and clean up after themselves.
 *
 * Each TEST_BEGIN section maps to one area of the API.
 */

#include "test.h"
#include "naui/src/filesystem/filesystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Platform-aware temp root
 * ---------------------------------------------------------------------- */

#if defined(_WIN32) || defined(_WIN64)
#  define SEP "\\"
static const char* temp_root(void)
{
    static char buf[512];
    const char* tmp = getenv("TEMP");
    if (!tmp) tmp = "C:\\Temp";
    snprintf(buf, sizeof(buf), "%s\\naui_test", tmp);
    return buf;
}
#else
#  define SEP "/"
static const char* temp_root(void) { return "/tmp/naui_test"; }
#endif

/* Build a path under the temp root */
static char g_path_buf[1024];
static const char* tp(const char* sub)
{
    snprintf(g_path_buf, sizeof(g_path_buf), "%s%s%s", temp_root(), SEP, sub);
    return g_path_buf;
}

/* -------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------- */

static void write_text(const char* path, const char* text)
{
    naui_file_write_all(path, text, strlen(text));
}

/* -------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */

int main(void)
{
    /* Make sure the temp root exists before any test runs */
    naui_directory_create(temp_root());

    /* =================================================================== */
    TEST_BEGIN("naui_file_open / naui_file_is_valid / naui_file_close");

    {
        NauiFileHandle h = NAUI_FILE_HANDLE_INIT;

        /* Open a new file for writing */
        ASSERT(naui_file_open(&h, tp("open_test.txt"), NAUI_FILE_WRITE));
        ASSERT(naui_file_is_valid(&h));
        naui_file_close(&h);
        ASSERT(!naui_file_is_valid(&h));  /* closed handle must be invalid */

        /* Open nonexistent file for reading must fail */
        ASSERT(!naui_file_open(&h, tp("does_not_exist_xyz.txt"), NAUI_FILE_READ));
        ASSERT(!naui_file_is_valid(&h));

        /* NULL handle/path guards */
        ASSERT(!naui_file_open(NULL, tp("x.txt"), NAUI_FILE_WRITE));
        ASSERT(!naui_file_open(&h, NULL, NAUI_FILE_WRITE));
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_file_write / naui_file_read");

    {
        const char* payload = "Hello, Naui!";
        size_t      plen    = strlen(payload);

        NauiFileHandle wh = NAUI_FILE_HANDLE_INIT;
        ASSERT(naui_file_open(&wh, tp("rw_test.txt"), NAUI_FILE_WRITE));
        ASSERT(naui_file_write(&wh, payload, plen) == plen);
        naui_file_close(&wh);

        char buf[64] = {0};
        NauiFileHandle rh = NAUI_FILE_HANDLE_INIT;
        ASSERT(naui_file_open(&rh, tp("rw_test.txt"), NAUI_FILE_READ));
        size_t got = naui_file_read(&rh, buf, sizeof(buf) - 1);
        naui_file_close(&rh);

        ASSERT(got == plen);
        ASSERT_STR_EQ(buf, payload);

        /* Read on invalid handle returns 0 */
        NauiFileHandle bad = NAUI_FILE_HANDLE_INIT;
        ASSERT(naui_file_read(&bad, buf, sizeof(buf)) == 0);
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_file_append");

    {
        write_text(tp("append_test.txt"), "Line1\n");

        NauiFileHandle ah = NAUI_FILE_HANDLE_INIT;
        ASSERT(naui_file_open(&ah, tp("append_test.txt"), NAUI_FILE_APPEND));
        const char* line2 = "Line2\n";
        naui_file_write(&ah, line2, strlen(line2));
        naui_file_close(&ah);

        size_t sz;
        char* all = naui_file_read_all(tp("append_test.txt"), &sz);
        ASSERT_NOT_NULL(all);
        ASSERT_STR_EQ(all, "Line1\nLine2\n");
        naui_filesystem_free(all);
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_file_size");

    {
        write_text(tp("size_test.txt"), "12345");
        ASSERT(naui_file_size(tp("size_test.txt")) == 5);
        ASSERT(naui_file_size(tp("no_such_file.txt")) == 0);
        ASSERT(naui_file_size(NULL) == 0);
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_file_read_all");

    {
        const char* content = "All your base";
        write_text(tp("readall_test.txt"), content);

        size_t sz = 0;
        char* buf = naui_file_read_all(tp("readall_test.txt"), &sz);
        ASSERT_NOT_NULL(buf);
        ASSERT(sz == strlen(content));
        ASSERT_STR_EQ(buf, content);
        naui_filesystem_free(buf);

        /* out_size can be NULL */
        buf = naui_file_read_all(tp("readall_test.txt"), NULL);
        ASSERT_NOT_NULL(buf);
        naui_filesystem_free(buf);

        /* Nonexistent returns NULL */
        ASSERT_NULL(naui_file_read_all(tp("ghost.txt"), &sz));
        ASSERT_NULL(naui_file_read_all(NULL, &sz));
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_file_write_all");

    {
        const char* data = "WriteAll test";
        ASSERT(naui_file_write_all(tp("writeall.txt"), data, strlen(data)));
        ASSERT(!naui_file_write_all(NULL, data, strlen(data)));
        ASSERT(!naui_file_write_all(tp("writeall.txt"), NULL, 10));

        /* Verify what was written */
        char* back = naui_file_read_all(tp("writeall.txt"), NULL);
        ASSERT_NOT_NULL(back);
        ASSERT_STR_EQ(back, data);
        naui_filesystem_free(back);
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_file_seek");

    {
        write_text(tp("seek_test.txt"), "ABCDEFGHIJ");

        NauiFileHandle h = NAUI_FILE_HANDLE_INIT;
        ASSERT(naui_file_open(&h, tp("seek_test.txt"), NAUI_FILE_READ));

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

        /* Seek on invalid handle */
        ASSERT(!naui_file_seek(&h, 0, SEEK_SET));
        ASSERT(!naui_file_seek(NULL, 0, SEEK_SET));
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_file_delete / naui_file_rename");

    {
        write_text(tp("del_me.txt"), "bye");
        ASSERT(naui_path_exists(tp("del_me.txt")));
        ASSERT(naui_file_delete(tp("del_me.txt")));
        ASSERT(!naui_path_exists(tp("del_me.txt")));
        ASSERT(!naui_file_delete(tp("del_me.txt"))); /* already gone */

        {
            char src[512], dst[512];
            snprintf(src, sizeof(src), "%s%srename_src.txt", temp_root(), SEP);
            snprintf(dst, sizeof(dst), "%s%srename_dst.txt", temp_root(), SEP);
            write_text(src, "rename");
            ASSERT(naui_file_rename(src, dst));
            ASSERT(!naui_path_exists(src));
            ASSERT(naui_path_exists(dst));
            naui_file_delete(dst);
        }

        ASSERT(!naui_file_delete(NULL));
        ASSERT(!naui_file_rename(NULL, tp("x.txt")));
        ASSERT(!naui_file_rename(tp("x.txt"), NULL));
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_path_filename");

    {
        ASSERT_STR_EQ(naui_path_filename("/foo/bar/baz.txt"), "baz.txt");
        ASSERT_STR_EQ(naui_path_filename("baz.txt"),          "baz.txt");
        ASSERT_STR_EQ(naui_path_filename("/foo/bar/"),        "");  /* trailing slash */
        ASSERT_NULL(naui_path_filename(NULL));
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_path_get_extension");

    {
        char* ext;

        ext = naui_path_get_extension("file.txt");
        ASSERT_NOT_NULL(ext);
        ASSERT_STR_EQ(ext, ".txt");
        naui_filesystem_free(ext);

        ext = naui_path_get_extension("/a/b/archive.tar.gz");
        ASSERT_NOT_NULL(ext);
        ASSERT_STR_EQ(ext, ".gz");
        naui_filesystem_free(ext);

        ext = naui_path_get_extension("no_extension");
        ASSERT_NULL(ext);

        ext = naui_path_get_extension(".dotfile");
        ASSERT_NULL(ext);  /* leading dot is not an extension */

        ASSERT_NULL(naui_path_get_extension(NULL));
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_path_parent");

    {
        char* p;

        p = naui_path_parent("/foo/bar/baz.txt");
        ASSERT_NOT_NULL(p);
        ASSERT_STR_EQ(p, "/foo/bar");
        naui_filesystem_free(p);

        p = naui_path_parent("/foo");
        ASSERT_NOT_NULL(p);
        ASSERT_STR_EQ(p, "/");
        naui_filesystem_free(p);

        p = naui_path_parent("only_filename");
        ASSERT_NOT_NULL(p);
        ASSERT_STR_EQ(p, ".");
        naui_filesystem_free(p);

        ASSERT_NULL(naui_path_parent(NULL));
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_path_exists");

    {
        write_text(tp("exists_check.txt"), "x");
        ASSERT(naui_path_exists(tp("exists_check.txt")));
        ASSERT(!naui_path_exists(tp("ghost_file_xyz.txt")));
        ASSERT(!naui_path_exists(NULL));
        naui_file_delete(tp("exists_check.txt"));
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_directory_create / naui_directory_remove");

    {
        const char* dir = tp("mkdir_test");
        ASSERT(naui_directory_create(dir));
        ASSERT(naui_path_exists(dir));

        /* Creating again must not fail (EEXIST is OK) */
        ASSERT(naui_directory_create(dir));

        ASSERT(naui_directory_remove(dir));
        ASSERT(!naui_path_exists(dir));

        ASSERT(!naui_directory_create(NULL));
        ASSERT(!naui_directory_remove(NULL));
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_directory_remove_all");

    {
        /* Build:  rm_root/sub/leaf.txt */
        const char* root = tp("rm_root");
        char sub[1024], leaf[1024];
        snprintf(sub,  sizeof(sub),  "%s%ssub",      root, SEP);
        snprintf(leaf, sizeof(leaf), "%s%sleaf.txt", sub,  SEP);

        naui_directory_create(root);
        naui_directory_create(sub);
        write_text(leaf, "delete me");

        ASSERT(naui_path_exists(leaf));
        ASSERT(naui_directory_remove_all(root));
        ASSERT(!naui_path_exists(root));

        ASSERT(!naui_directory_remove_all(NULL));
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_directory_rename");

    {
        char src[512], dst[512], file_in_src[512];
        snprintf(src,         sizeof(src),         "%s%sdir_rename_src",         temp_root(), SEP);
        snprintf(dst,         sizeof(dst),         "%s%sdir_rename_dst",         temp_root(), SEP);
        snprintf(file_in_src, sizeof(file_in_src), "%s%sdir_rename_src%sfile.txt", temp_root(), SEP, SEP);

        naui_directory_create(src);
        write_text(file_in_src, "hi");

        ASSERT(naui_directory_rename(src, dst));
        ASSERT(!naui_path_exists(src));
        ASSERT(naui_path_exists(dst));

        naui_directory_remove_all(dst);
        ASSERT(!naui_directory_rename(NULL, dst));
        ASSERT(!naui_directory_rename(src, NULL));
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_directory_get");

    {
        /* These should all return non-empty strings on any supported platform */
        const char* home     = naui_directory_get(NAUI_DIR_HOME);
        const char* working  = naui_directory_get(NAUI_DIR_WORKING);
        const char* appdata  = naui_directory_get(NAUI_DIR_APPDATA);
        const char* downloads= naui_directory_get(NAUI_DIR_DOWNLOADS);
        const char* bin      = naui_directory_get(NAUI_DIR_BIN);

        ASSERT(home      && home[0]     != '\0');
        ASSERT(working   && working[0]  != '\0');
        ASSERT(appdata   && appdata[0]  != '\0');
        ASSERT(downloads && downloads[0]!= '\0');
        ASSERT(bin       && bin[0]      != '\0');

        printf("  INFO  HOME=%s\n",      home);
        printf("  INFO  WORKING=%s\n",   working);
        printf("  INFO  APPDATA=%s\n",   appdata);
        printf("  INFO  DOWNLOADS=%s\n", downloads);
        printf("  INFO  BIN=%s\n",       bin);
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_directory_filter");

    {
        const char* dir = tp("filter_dir");
        naui_directory_create(dir);

        char p1[512], p2[512], p3[512];
        snprintf(p1, sizeof(p1), "%s%sa.txt", dir, SEP);
        snprintf(p2, sizeof(p2), "%s%sb.txt", dir, SEP);
        snprintf(p3, sizeof(p3), "%s%sc.png", dir, SEP);
        write_text(p1, "a");
        write_text(p2, "b");
        write_text(p3, "c");

        /* No filter — all 3 */
        Naui_List(NauiDirEntry) all = naui_directory_filter(dir, NULL, NULL, 0);
        ASSERT_NOT_NULL(all);
        ASSERT(naui_list_len(all) == 3);
        naui_directory_filter_free(all);

        /* Extension filter — only .txt */
        const char* exts[] = { ".txt" };
        Naui_List(NauiDirEntry) txts = naui_directory_filter(dir, NULL, exts, 1);
        ASSERT_NOT_NULL(txts);
        ASSERT(naui_list_len(txts) == 2);
        naui_directory_filter_free(txts);

        /* Glob filter */
        Naui_List(NauiDirEntry) glob = naui_directory_filter(dir, "a*", NULL, 0);
        ASSERT_NOT_NULL(glob);
        ASSERT(naui_list_len(glob) == 1);
        naui_directory_filter_free(glob);

        /* Entries have correct metadata */
        Naui_List(NauiDirEntry) all2 = naui_directory_filter(dir, NULL, NULL, 0);
        for (size_t i = 0; i < naui_list_len(all2); ++i)
        {
            ASSERT_NOT_NULL(all2[i].path);
            ASSERT(!all2[i].is_directory);
            ASSERT(all2[i].size == 1);
        }
        naui_directory_filter_free(all2);

        /* NULL path returns an empty (not NULL) list */
        Naui_List(NauiDirEntry) bad = naui_directory_filter(NULL, NULL, NULL, 0);
        ASSERT_NOT_NULL(bad);
        ASSERT(naui_list_len(bad) == 0);
        naui_directory_filter_free(bad);

        naui_directory_remove_all(dir);
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_path_lock / naui_is_locked / naui_path_unlock");

    {
        const char* lpath = tp("locked_file.txt");
        write_text(lpath, "lock test");

        ASSERT(!naui_is_locked(lpath));
        ASSERT(naui_path_lock(lpath));
        ASSERT(naui_is_locked(lpath));

        /* Double-lock should fail */
        ASSERT(!naui_path_lock(lpath));

        naui_path_unlock(lpath);
        ASSERT(!naui_is_locked(lpath));

        /* Unlocking already-unlocked is a no-op */
        naui_path_unlock(lpath);
        ASSERT(!naui_is_locked(lpath));

        ASSERT(!naui_path_lock(NULL));
        ASSERT(!naui_is_locked(NULL));

        naui_file_delete(lpath);
    }

    TEST_END();

    /* =================================================================== */
    TEST_BEGIN("naui_hide_file / naui_is_hidden");

    {
#if defined(_WIN32) || defined(_WIN64)
        char hpath[512];
        snprintf(hpath, sizeof(hpath), "%s%shide_test.txt", temp_root(), SEP);
        write_text(hpath, "hidden");

        ASSERT(!naui_is_hidden(hpath));
        ASSERT(naui_hide_file(hpath, true));
        ASSERT(naui_is_hidden(hpath));
        ASSERT(naui_hide_file(hpath, false));
        ASSERT(!naui_is_hidden(hpath));
        naui_file_delete(hpath);
#else
        /*
         * On Unix, hide_file renames to prepend/remove the leading dot.
         * We need separate buffers since tp() reuses one static buffer.
         */
        char visible[512], hidden[512];
        snprintf(visible, sizeof(visible), "%s%shide_me.txt",  temp_root(), SEP);
        snprintf(hidden,  sizeof(hidden),  "%s%s.hide_me.txt", temp_root(), SEP);

        write_text(visible, "hide me");
        ASSERT(!naui_is_hidden(visible));
        ASSERT(naui_hide_file(visible, true));
        ASSERT(naui_path_exists(hidden));
        ASSERT(naui_is_hidden(hidden));

        ASSERT(naui_hide_file(hidden, false));
        ASSERT(naui_path_exists(visible));
        ASSERT(!naui_is_hidden(visible));
        naui_file_delete(visible);
#endif
        ASSERT(!naui_hide_file(NULL, true));
        ASSERT(!naui_is_hidden(NULL));
    }

    TEST_END();

    /* =================================================================== */
    /* Clean up temp root */
    naui_directory_remove_all(temp_root());

    return test_summary();
}