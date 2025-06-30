#ifndef FLS_MEMORY_H
#define FLS_MEMORY_H

#include "common.h"

// A macro for allocating a block of memory.
#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

// A macro to calculate the new capacity for a dynamic array.
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

// A macro to handle resizing a dynamic array.
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

// A macro to free a dynamic array.
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

// The core memory management function.
void* reallocate(void* pointer, size_t oldSize, size_t newSize);

#endif // FLS_MEMORY_H
