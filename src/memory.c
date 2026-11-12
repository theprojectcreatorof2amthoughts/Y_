#include "memory.h"

#include <stdlib.h>

Memory *memory_create(size_t size) {
    Memory *memory = malloc(sizeof(*memory));
    if (!memory) return NULL;
    memory->bytes = calloc(size ? size : 1, sizeof(*memory->bytes));
    if (!memory->bytes) {
        free(memory);
        return NULL;
    }
    memory->size = size;
    return memory;
}

void memory_destroy(Memory *memory) {
    if (!memory) return;
    free(memory->bytes);
    free(memory);
}

bool memory_read(const Memory *memory, size_t index, unsigned char *value) {
    if (!memory || !value || index >= memory->size) return false;
    *value = memory->bytes[index];
    return true;
}

bool memory_write(Memory *memory, size_t index, unsigned char value) {
    if (!memory || index >= memory->size) return false;
    memory->bytes[index] = value;
    return true;
}