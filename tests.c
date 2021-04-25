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

    virtual_heap = sbrk(0);
    init_allocator(virtual_heap, 15, 12);
    virtual_info(virtual_heap);
    virtual_malloc(virtual_heap, 1 << 12);
    virtual_malloc(virtual_heap, 1 << 12);
    virtual_malloc(virtual_heap, 1 << 12);
    virtual_malloc(virtual_heap, 1 << 12);
    virtual_malloc(virtual_heap, 1 << 12);
    virtual_malloc(virtual_heap, 1 << 12);
    virtual_info(virtual_heap);

    printf("\n");

    virtual_free(virtual_heap, (uint8_t*) virtual_heap + (1 << 12));
    virtual_info(virtual_heap);

    printf("\n");

    virtual_free(virtual_heap, virtual_heap);
    virtual_info(virtual_heap);

    return 0;
}
