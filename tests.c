#include "virtual_alloc.h"
#include <unistd.h>

void * virtual_heap = NULL;

void * virtual_sbrk(int32_t increment) {
    // Your implementation here (for your testing only)
    return sbrk(increment);
    // return (void *)(-1);
}

int main() {
    // Your own testing code here
    printf("%d\n", log_2(20000));
    printf("%d\n", log_2(2048));

    virtual_heap = sbrk(0);
    // init_allocator(virtual_heap, 15, 12);
    // virtual_info(virtual_heap);
    // virtual_malloc(virtual_heap, 1 << 12);
    // virtual_malloc(virtual_heap, 1 << 12);
    // virtual_malloc(virtual_heap, 1 << 12);
    // virtual_malloc(virtual_heap, 1 << 12);
    // virtual_malloc(virtual_heap, 1 << 12);
    // virtual_malloc(virtual_heap, 1 << 12);
    // virtual_info(virtual_heap);

    // printf("\n");

    // virtual_free(virtual_heap, (uint8_t*) virtual_heap + (1 << 12));
    // virtual_info(virtual_heap);

    // printf("\n");

    // virtual_free(virtual_heap, virtual_heap);
    // virtual_info(virtual_heap);

    // printf("\n");

    // init_allocator(virtual_heap, 18, 11);
    // virtual_malloc(virtual_heap, 2049);
    // virtual_malloc(virtual_heap, 2048);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_free(virtual_heap, virtual_heap);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_free(virtual_heap, (uint8_t*) virtual_heap + 4096);
    // virtual_info(virtual_heap);

    // init_allocator(virtual_heap, 18, 11);
    // virtual_malloc(virtual_heap, 4095);
    // virtual_malloc(virtual_heap, 2048);
    // virtual_malloc(virtual_heap, 5000);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_free(virtual_heap, virtual_heap);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_malloc(virtual_heap, 2048);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_free(virtual_heap, (uint8_t*) virtual_heap + 8192);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_free(virtual_heap, (uint8_t*) virtual_heap + 4096);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_free(virtual_heap, (uint8_t*) virtual_heap + 6144);
    // virtual_info(virtual_heap);

    // init_allocator(virtual_heap, 11, 8);
    // virtual_malloc(virtual_heap, 1024);
    // virtual_malloc(virtual_heap, 512);
    // virtual_malloc(virtual_heap, 256);
    // virtual_malloc(virtual_heap, 256);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_free(virtual_heap, (uint8_t*) virtual_heap + 2 + 1024);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_malloc(virtual_heap, 256);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_free(virtual_heap, virtual_heap + 2);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_malloc(virtual_heap, 256);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_free(virtual_heap, (uint8_t*) virtual_heap + 2 + 1024 + 256 * 2);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_free(virtual_heap, (uint8_t*) virtual_heap + 2 + 1024);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_free(virtual_heap, (uint8_t*) virtual_heap + 2 + 1024 + 256);
    // virtual_info(virtual_heap);
    // printf("\n");
    // virtual_free(virtual_heap, (uint8_t*) virtual_heap + 2 + 1024 + 512 + 256);
    // virtual_info(virtual_heap);

    init_allocator(virtual_heap, 11, 8);
    virtual_malloc(virtual_heap, 1024);
    virtual_realloc(virtual_heap, virtual_heap + 2, 2048);
    virtual_info(virtual_heap);

    return 0;
}
