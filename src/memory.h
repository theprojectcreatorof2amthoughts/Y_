#ifndef Y_MEMORY_H
#define Y_MEMORY_H

#include <stdbool.h>
#include <stddef.h>

typedef struct Memory {
    unsigned char *bytes;
    size_t size;
} Memory;

Memory *memory_create(size_t size);
void memory_destroy(Memory *memory);
bool memory_read(const Memory *memory, size_t index, unsigned char *value);
bool memory_write(Memory *memory, size_t index, unsigned char value);

#endif