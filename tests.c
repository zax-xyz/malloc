#include "virtual_alloc.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include "cmocka.h"

#define LINE_LENGTH 128
#define STR_ARR_SIZE(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

void* virtual_heap = NULL;

// pipe for testing stdout
int pipefd[2];
int oldstdout;

void* virtual_sbrk(int32_t increment) {
    // Your implementation here (for your testing only)
    return sbrk(increment);
}

static int setup(void** state) {
    // create pipe for testing stdout
    pipe(pipefd);
    dup2(pipefd[1], fileno(stdout));

    return 0;
}

static int teardown(void** state) {
    // close pipe and restore stdout
    close(pipefd[1]);
    dup2(oldstdout, fileno(stdout));

    return 0;
}

static void assert_stdout_equal(const char** expected, unsigned int lines) {
    char line[LINE_LENGTH];

    // close pipe and restore stdout
    fflush(stdout);
    close(pipefd[1]);
    dup2(oldstdout, fileno(stdout));

    FILE* fp = fdopen(pipefd[0], "r");

    for (int i = 0; i < lines; i++) {
        if (fgets(line, LINE_LENGTH, fp) == NULL) {
            fail_msg("Reached unexpected EOF in stdout. Expected: %s",
                    expected[i]);
        }

        // removing trailing newline
        line[strcspn(line, "\n")] = 0;
        assert_string_equal(line, expected[i]);
    }

    // recreate stdout pipe for any further testing
    pipe(pipefd);
    dup2(pipefd[1], fileno(stdout));
}

static void test_init() {
    const char* expected[] = {
        "free 32768",
    };

    init_allocator(virtual_heap, 15, 12);

    virtual_info(virtual_heap);
    assert_stdout_equal(expected, STR_ARR_SIZE(expected));
}

static void test_malloc_half() {
    const char* expected[] = {
        "allocated 16384",
    };

    init_allocator(virtual_heap, 15, 12);
    virtual_malloc(virtual_heap, 1 << 14);

    virtual_info(virtual_heap);
    assert_stdout_equal(expected, STR_ARR_SIZE(expected));
}

static void test_malloc_split() {
    const char* expected[] = {
        "allocated 4096",
        "free 4096",
        "free 8192",
        "free 16384",
    };

    init_allocator(virtual_heap, 15, 12);
    virtual_malloc(virtual_heap, 1 << 12);

    virtual_info(virtual_heap);
    assert_stdout_equal(expected, STR_ARR_SIZE(expected));
}

static void test_malloc_full() {
    const char* expected[] = {
        "allocated 32768",
    };

    init_allocator(virtual_heap, 15, 12);
    virtual_malloc(virtual_heap, 1 << 15);

    virtual_info(virtual_heap);
    assert_stdout_equal(expected, STR_ARR_SIZE(expected));
}

static void test_malloc_many() {
    const char* expected[] = {
        "allocated 1024",
        "allocated 512",
        "allocated 512",
        "allocated 1024",
        "allocated 256",
        "allocated 128",
        "allocated 128",
        "free 512",
        "allocated 4096",
        "allocated 4096",
        "free 4096",
        "allocated 16384",
    };

    init_allocator(virtual_heap, 15, 5);
    virtual_malloc(virtual_heap, 1 << 10);
    virtual_malloc(virtual_heap, 1 << 9);
    virtual_malloc(virtual_heap, 1 << 9);
    virtual_malloc(virtual_heap, 1 << 10);
    virtual_malloc(virtual_heap, 1 << 8);
    virtual_malloc(virtual_heap, 1 << 7);
    virtual_malloc(virtual_heap, 1 << 7);
    virtual_malloc(virtual_heap, 1 << 12);
    virtual_malloc(virtual_heap, 1 << 14);
    virtual_malloc(virtual_heap, 1 << 12);

    virtual_info(virtual_heap);
    assert_stdout_equal(expected, STR_ARR_SIZE(expected));
}

static void test_malloc_tiny() {
    const char* expected[] = {
        "allocated 1",
        "allocated 1",
        "allocated 1",
        "allocated 1",
        "allocated 1",
        "allocated 1",
        "allocated 1",
        "allocated 1",
    };

    init_allocator(virtual_heap, 3, 0);
    for (int i = 0; i < (1 << 3); i++)
        virtual_malloc(virtual_heap, 1);

    virtual_info(virtual_heap);
    assert_stdout_equal(expected, STR_ARR_SIZE(expected));
}

static void test_malloc_too_large() {
    const char* expected[] = {
        "free 32",
    };

    init_allocator(virtual_heap, 5, 0);

    void* block = virtual_malloc(virtual_heap, (1 << 5) + 1);
    assert_ptr_equal(block, NULL);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, STR_ARR_SIZE(expected));

    block = virtual_malloc(virtual_heap, UINT8_MAX);
    assert_ptr_equal(block, NULL);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, STR_ARR_SIZE(expected));
}

static void test_malloc_zero() {
    const char* expected[] = {
        "free 32",
    };

    init_allocator(virtual_heap, 5, 0);

    void* block = virtual_malloc(virtual_heap, 0);
    assert_ptr_equal(block, NULL);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, STR_ARR_SIZE(expected));
}

static void test_malloc_lt_min() {
    const char* expected[] = {
        "allocated 4096",
        "free 4096",
        "free 8192",
        "free 16384",
    };

    init_allocator(virtual_heap, 15, 12);
    virtual_malloc(virtual_heap, 1 << 4);

    virtual_info(virtual_heap);
    assert_stdout_equal(expected, STR_ARR_SIZE(expected));
}

static void test_free_full() {
    const char* expected[] = {
        "allocated 32768",
    };

    init_allocator(virtual_heap, 15, 12);

    void* block = virtual_malloc(virtual_heap, 1 << 15);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, STR_ARR_SIZE(expected));

    const char* expected2[] = {
        "free 32768"
    };

    virtual_free(virtual_heap, block);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected2, STR_ARR_SIZE(expected2));
}

static void test_free_merge() {
    init_allocator(virtual_heap, 8, 2);

    const char* expected[] = {
        "allocated 128",
        "free 128",
    };

    void* block = virtual_malloc(virtual_heap, 1 << 7);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, STR_ARR_SIZE(expected));

    const char* expected2[] = {
        "free 256",
    };

    virtual_free(virtual_heap, block);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected2, STR_ARR_SIZE(expected2));
}

int main() {
    // Your own testing code here
    oldstdout = dup(fileno(stdout));

    virtual_heap = sbrk(0);

    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_init, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_half, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_split, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_full, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_many, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_tiny, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_too_large, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_zero, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_lt_min, setup, teardown),
        cmocka_unit_test_setup_teardown(test_free_full, setup, teardown),
        cmocka_unit_test_setup_teardown(test_free_merge, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);

    return 0;
}
