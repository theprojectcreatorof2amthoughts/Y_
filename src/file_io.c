#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>

char *read_entire_file(const char *path, size_t *length) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        perror(path);
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        perror(path);
        fclose(file);
        return NULL;
    }
    long end = ftell(file);
    if (end < 0 || fseek(file, 0, SEEK_SET) != 0) {
        perror(path);
        fclose(file);
        return NULL;
    }
    size_t size = (size_t)end;
    char *source = malloc(size + 1);
    if (!source) {
        fclose(file);
        return NULL;
    }
    size_t bytes_read = fread(source, 1, size, file);
    if (bytes_read != size && ferror(file)) {
        perror(path);
        free(source);
        fclose(file);
        return NULL;
    }
    fclose(file);
    source[bytes_read] = '\0';
    if (length) *length = bytes_read;
    return source;
}