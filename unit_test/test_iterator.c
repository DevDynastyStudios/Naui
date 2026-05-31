#include "test.h"
#include "test_func.h"
#include "naui/filesystem/iterator.h"

#include <stdbool.h>

void test_dir_iterator_basic()
{
    TEST_BEGIN("Iterator - Basic");
    Naui_Path root = NAUI_PATH("test_dir_iter_basic");
    ASSERT(naui_directory_create(root));

    // Create files
    {
        Naui_Path p = naui_path_join(root, NAUI_PATH("a.txt"));
        naui_file_write_all(p, "x", 1);
    }
    {
        Naui_Path p = naui_path_join(root, NAUI_PATH("b.txt"));
        naui_file_write_all(p, "x", 1);
    }
    {
        Naui_Path p = naui_path_join(root, NAUI_PATH("c.bin"));
        naui_file_write_all(p, "x", 1);
    }

    // Iterate
    Naui_DirIterator it = naui_dir_iterator_open(root, NULL, NULL, false);

    bool saw_a = false;
    bool saw_b = false;
    bool saw_c = false;

    while (naui_dir_iterator_valid(&it)) {
        const char* name = naui_file_filename(it.entry.path);

        if (strcmp(name, "a.txt") == 0) saw_a = true;
        if (strcmp(name, "b.txt") == 0) saw_b = true;
        if (strcmp(name, "c.bin") == 0) saw_c = true;

        naui_dir_iterator_next(&it);
    }

    naui_dir_iterator_close(&it);

    ASSERT(saw_a);
    ASSERT(saw_b);
    ASSERT(saw_c);

    naui_directory_remove_all(root);
    TEST_END();
}

void test_dir_iterator_filter()
{
    TEST_BEGIN("Iterator - Filter");
    Naui_Path root = NAUI_PATH("test_dir_iter_filter");
    ASSERT(naui_directory_create(root));

    Naui_Path apple_path = naui_path_join(root, NAUI_PATH("apple.txt"));
    Naui_Path banana_path = naui_path_join(root, NAUI_PATH("banana.txt"));
    Naui_Path apricot_path = naui_path_join(root, NAUI_PATH("apricot.txt"));
    naui_file_write_all(apple_path, "x", 1);
    naui_file_write_all(banana_path, "x", 1);
    naui_file_write_all(apricot_path, "x", 1);

    Naui_DirIterator it = naui_dir_iterator_open(root, "apple", NAUI_EXTENSIONS(".txt"), false);

    bool saw_apple = false;
    bool saw_apricot = false;
    bool saw_banana = false;

    while (naui_dir_iterator_valid(&it)) {
        const char* name = naui_file_filename(it.entry.path);

        if (strcmp(name, "apple.txt") == 0) saw_apple = true;
        if (strcmp(name, "apricot.txt") == 0) saw_apricot = true;
        if (strcmp(name, "banana.txt") == 0) saw_banana = true;

        naui_dir_iterator_next(&it);
    }

    naui_dir_iterator_close(&it);

    ASSERT(saw_apple);
    ASSERT(!saw_apricot);
    ASSERT(!saw_banana);

    naui_directory_remove_all(root);
    TEST_END();
}

void test_dir_iterator_extensions()
{
    TEST_BEGIN("Iterator - Extensions");
    Naui_Path root = NAUI_PATH("test_dir_iter_ext");
    ASSERT(naui_directory_create(root));

    Naui_Path a_path = naui_path_join(root, NAUI_PATH("a.txt"));
    Naui_Path b_path = naui_path_join(root, NAUI_PATH("b.bin"));
    Naui_Path c_path = naui_path_join(root, NAUI_PATH("c.txt"));
    naui_file_write_all(a_path, "x", 1);
    naui_file_write_all(b_path, "x", 1);
    naui_file_write_all(c_path, "x", 1);

    const char* exts[] = { ".txt", NULL };

    Naui_DirIterator it = naui_dir_iterator_open(root, NULL, exts, false);

    bool saw_a = false;
    bool saw_b = false;
    bool saw_c = false;

    while (naui_dir_iterator_valid(&it)) {
        const char* name = naui_file_filename(it.entry.path);

        if (strcmp(name, "a.txt") == 0) saw_a = true;
        if (strcmp(name, "b.bin") == 0) saw_b = true;
        if (strcmp(name, "c.txt") == 0) saw_c = true;

        naui_dir_iterator_next(&it);
    }

    naui_dir_iterator_close(&it);

    ASSERT(saw_a);
    ASSERT(!saw_b);
    ASSERT(saw_c);

    naui_directory_remove_all(root);
    TEST_END();
}

void test_dir_iterator_mixed()
{
    TEST_BEGIN("Iterator - MIXED");
    Naui_Path root = NAUI_PATH("test_dir_iter_mixed");
    ASSERT(naui_directory_create(root));

    // Files
    Naui_Path a_path = naui_path_join(root, NAUI_PATH("a.txt"));
    Naui_Path b_path = naui_path_join(root, NAUI_PATH("b.txt"));
    naui_file_write_all(a_path, "x", 1);
    naui_file_write_all(b_path, "x", 1);

    // Directory
    Naui_Path sub = naui_path_join(root, NAUI_PATH("folder"));
    naui_directory_create(sub);

    Naui_DirIterator it = naui_dir_iterator_open(root, NULL, NULL, false);

    bool saw_a = false;
    bool saw_b = false;
    bool saw_folder = false;

    while (naui_dir_iterator_valid(&it)) {
        const char* name = naui_file_filename(it.entry.path);

        if (strcmp(name, "a.txt") == 0) saw_a = true;
        if (strcmp(name, "b.txt") == 0) saw_b = true;
        if (strcmp(name, "folder") == 0 && it.entry.is_directory)
            saw_folder = true;

        naui_dir_iterator_next(&it);
    }

    naui_dir_iterator_close(&it);

    ASSERT(saw_a);
    ASSERT(saw_b);
    ASSERT(saw_folder);

    naui_directory_remove_all(root);
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
    TEST_END();
}

void test_dir_iterator_invalid_path()
{
    TEST_BEGIN("Iterator - Invalid Path");
    Naui_Path root = NAUI_PATH("this_path_should_not_exist_12345");
    Naui_DirIterator it = naui_dir_iterator_open(root, NULL, NULL, false);
    ASSERT(!naui_dir_iterator_valid(&it));
    naui_dir_iterator_close(&it);
    TEST_END();
}

void test_dir_iterator_hidden_files()
{
    TEST_BEGIN("Iterator - Hidden Files");
    Naui_Path root = NAUI_PATH("test_iter_hidden");
    ASSERT(naui_directory_create(root));

    // Create visible file
    Naui_Path visible = naui_path_join(root, NAUI_PATH("visible.txt"));
    naui_file_write_all(visible, "x", 1);

    // Create hidden file
    Naui_Path hidden = naui_path_join(root, NAUI_PATH("secret.txt"));
    naui_file_write_all(hidden, "x", 1);
    hidden = naui_file_hide(hidden, true);

    ASSERT(naui_file_is_hidden(hidden));

    Naui_DirIterator it = naui_dir_iterator_open(root, NULL, NULL, false);

    bool saw_visible = false;
    bool saw_hidden = false;

    while (naui_dir_iterator_valid(&it)) {
        const char* name = naui_file_filename(it.entry.path);

        if (strcmp(name, "visible.txt") == 0) saw_visible = true;
        if (strcmp(name, "secret.txt") == 0) saw_hidden = true;

        naui_dir_iterator_next(&it);
    }

    naui_dir_iterator_close(&it);

    ASSERT(saw_visible);
    ASSERT(saw_hidden);

    naui_directory_remove_all(root);
    TEST_END();
}

void test_dir_iterator_filter_and_ext()
{
    TEST_BEGIN("Iterator - Filter and Exit");
    Naui_Path root = NAUI_PATH("test_iter_filter_ext");
    ASSERT(naui_directory_create(root));

    Naui_Path apple_path = naui_path_join(root, NAUI_PATH("apple.txt"));
    Naui_Path banana_path = naui_path_join(root, NAUI_PATH("banana.png"));
    Naui_Path apricot_path = naui_path_join(root, NAUI_PATH("apricot.bin"));
    naui_file_write_all(apple_path, "x", 1);
    naui_file_write_all(banana_path, "x", 1);
    naui_file_write_all(apricot_path, "x", 1);

    const char* exts[] = { ".txt", ".bin", NULL };

    Naui_DirIterator it = naui_dir_iterator_open(root, "apricot", exts, true);

    bool saw_apple = false;
    bool saw_banana = false;
    bool saw_apricot = false;

    while (naui_dir_iterator_valid(&it)) {
        const char* name = naui_file_filename(it.entry.path);

        if (strcmp(name, "apple.txt") == 0) saw_apple = true;
        if (strcmp(name, "banana.txt") == 0) saw_banana = true;
        if (strcmp(name, "apricot.bin") == 0) saw_apricot = true;

        naui_dir_iterator_next(&it);
    }

    naui_dir_iterator_close(&it);

    ASSERT(!saw_apple);
    ASSERT(!saw_banana);
    ASSERT(saw_apricot);

    naui_directory_remove_all(root);
    TEST_END();
}

void test_dir_iterator_reuse()
{
    TEST_BEGIN("Iterator - Reuse");
    Naui_Path root = NAUI_PATH("test_iter_reuse");
    ASSERT(naui_directory_create(root));

    Naui_Path a_path = naui_path_join(root, NAUI_PATH("a.txt"));
    naui_file_write_all(a_path, "x", 1);

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
    TEST_END();
}

void test_dir_iterator_no_parent_entries()
{
    TEST_BEGIN("Iterator - No Parent Entries");
    Naui_Path root = NAUI_PATH("test_iter_no_parent");
    ASSERT(naui_directory_create(root));

    Naui_DirIterator it = naui_dir_iterator_open(root, NULL, NULL, false);

    while (naui_dir_iterator_valid(&it)) {
        const char* name = naui_file_filename(it.entry.path);

        ASSERT(strcmp(name, ".") != 0);
        ASSERT(strcmp(name, "..") != 0);

        naui_dir_iterator_next(&it);
    }

    naui_dir_iterator_close(&it);
    naui_directory_remove_all(root);
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
        Naui_Path name;
        snprintf(name.data, NAUI_PATH_MAX, "file_%03d.txt", i);
        Naui_Path p = naui_path_join(root, name);
        naui_file_write_all(p, "x", 1);
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
