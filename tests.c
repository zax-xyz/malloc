#include "virtual_alloc.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include "cmocka.h"

#define LINE_LENGTH 128
#define ARR_SIZE(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

void* virtual_heap = NULL;

// pipe for testing stdout
int pipefd[2];
int oldstdout;
FILE* fp;

void* virtual_sbrk(int32_t increment) {
    // Your implementation here (for your testing only)
    return sbrk(increment);
}

static void close_fp() {
    if (fp) {
        fclose(fp);
        fp = NULL;
    }
}

static int setup(void** state) {
    // create pipe for testing stdout
    pipe(pipefd);
    dup2(pipefd[1], fileno(stdout));

    return 0;
}

static int teardown(void** state) {
    // close pipe and restore stdout
    fflush(stdout);
    close(pipefd[1]);
    dup2(oldstdout, fileno(stdout));

    close_fp();

    return 0;
}

static void assert_stdout_equal(const char** expected, unsigned int lines) {
    char line[LINE_LENGTH];

    // close pipe and restore stdout
    fflush(stdout);
    close(pipefd[1]);
    dup2(oldstdout, fileno(stdout));

    fp = fdopen(pipefd[0], "r");

    for (int i = 0; i < lines; i++) {
        // read line from stdout and ensure no error or EOF
        assert_non_null(fgets(line, LINE_LENGTH, fp));

        // removing trailing newline
        line[strcspn(line, "\n")] = 0;
        assert_string_equal(line, expected[i]);
    }

    // ensure this is the end of stdout
    assert_int_equal(getc(fp), EOF);

    close_fp();

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
    assert_stdout_equal(expected, ARR_SIZE(expected));
}

static void test_init_zero_malloc() {
    const char* expected[] = {
        "free 1",
    };

    init_allocator(virtual_heap, 0, 0);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));

    void* block = virtual_malloc(virtual_heap, 1);
    assert_non_null(block);

    const char* expected2[] = {
        "allocated 1",
    };

    virtual_info(virtual_heap);
    assert_stdout_equal(expected2, ARR_SIZE(expected2));

    assert_null(virtual_malloc(virtual_heap, 1));
    virtual_info(virtual_heap);
    assert_stdout_equal(expected2, ARR_SIZE(expected2));

    assert_null(virtual_malloc(virtual_heap, 2));

    virtual_info(virtual_heap);
    assert_stdout_equal(expected2, ARR_SIZE(expected2));

    int res = virtual_free(virtual_heap, block);
    assert_int_equal(res, 0);

    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));
}

static void test_malloc_half() {
    const char* expected[] = {
        "allocated 16384",
        "free 16384",
    };

    init_allocator(virtual_heap, 15, 12);
    virtual_malloc(virtual_heap, 1 << 14);

    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));
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
    assert_stdout_equal(expected, ARR_SIZE(expected));
}

static void test_malloc_full() {
    const char* expected[] = {
        "allocated 32768",
    };

    init_allocator(virtual_heap, 15, 12);
    virtual_malloc(virtual_heap, 1 << 15);

    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));
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
    assert_non_null(virtual_malloc(virtual_heap, 1 << 10));
    assert_non_null(virtual_malloc(virtual_heap, 1 << 9));
    assert_non_null(virtual_malloc(virtual_heap, 1 << 9));
    assert_non_null(virtual_malloc(virtual_heap, 1 << 10));
    assert_non_null(virtual_malloc(virtual_heap, 1 << 8));
    assert_non_null(virtual_malloc(virtual_heap, 1 << 7));
    assert_non_null(virtual_malloc(virtual_heap, 1 << 7));
    assert_non_null(virtual_malloc(virtual_heap, 1 << 12));
    assert_non_null(virtual_malloc(virtual_heap, 1 << 14));
    assert_non_null(virtual_malloc(virtual_heap, 1 << 12));

    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));
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
    for (int i = 0; i < (1 << 3); i++) {
        void* block = virtual_malloc(virtual_heap, 1);
        assert_non_null(block);
    }

    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));
}

static void test_malloc_too_large() {
    const char* expected[] = {
        "free 32",
    };

    init_allocator(virtual_heap, 5, 0);

    void* block = virtual_malloc(virtual_heap, (1 << 5) + 1);
    assert_null(block);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));

    block = virtual_malloc(virtual_heap, UINT8_MAX);
    assert_null(block);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));
}

static void test_malloc_zero() {
    const char* expected[] = {
        "free 32",
    };

    init_allocator(virtual_heap, 5, 0);

    void* block = virtual_malloc(virtual_heap, 0);
    assert_null(block);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));
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
    assert_stdout_equal(expected, ARR_SIZE(expected));
}

static void test_malloc_min_gt_heap() {
    const char* expected[] = {
        "free 4096",
    };

    init_allocator(virtual_heap, 12, 15);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));

    assert_null(virtual_malloc(virtual_heap, 1 << 15));
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));

    assert_null(virtual_malloc(virtual_heap, 1 << 12));
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));
}

static void test_free_full() {
    const char* expected[] = {
        "allocated 32768",
    };

    init_allocator(virtual_heap, 15, 12);

    void* block = virtual_malloc(virtual_heap, 1 << 15);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));

    const char* expected2[] = {
        "free 32768"
    };

    virtual_free(virtual_heap, block);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected2, ARR_SIZE(expected2));
}

static void test_free_merge() {
    init_allocator(virtual_heap, 8, 2);

    const char* expected[] = {
        "allocated 128",
        "free 128",
    };

    void* block = virtual_malloc(virtual_heap, 1 << 7);
    assert_non_null(block);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));

    const char* expected2[] = {
        "free 256",
    };

    virtual_free(virtual_heap, block);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected2, ARR_SIZE(expected2));
}

static void test_free_outside() {
    init_allocator(virtual_heap, 8, 2);

    const char* expected[] = {
        "allocated 256",
    };

    void* block = virtual_malloc(virtual_heap, 1 << 8);
    assert_non_null(block);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));

    // this should be right after the allocated block
    int res = virtual_free(virtual_heap, (void*) ((uint8_t*) block) + (1 << 8));
    assert_int_not_equal(res, 0);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));

    // first 2 bytes in our virtual heap should be reserved
    res = virtual_free(virtual_heap, virtual_heap);
    assert_int_not_equal(res, 0);
    virtual_info(virtual_heap);
    assert_stdout_equal(expected, ARR_SIZE(expected));
}

static void test_free_unallocated() {
    init_allocator(virtual_heap, 15, 12);
}

int main() {
    // Your own testing code here
    virtual_heap = sbrk(0);

    oldstdout = dup(fileno(stdout));

    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_init, setup, teardown),
        cmocka_unit_test_setup_teardown(test_init_zero_malloc, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_half, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_split, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_full, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_many, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_tiny, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_too_large, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_zero, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_lt_min, setup, teardown),
        cmocka_unit_test_setup_teardown(test_malloc_min_gt_heap, setup, teardown),
        cmocka_unit_test_setup_teardown(test_free_full, setup, teardown),
        cmocka_unit_test_setup_teardown(test_free_merge, setup, teardown),
        cmocka_unit_test_setup_teardown(test_free_outside, setup, teardown),
        cmocka_unit_test_setup_teardown(test_free_unallocated, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
