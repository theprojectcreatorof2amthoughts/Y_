#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_identifier_start(int ch) {
    return isalpha((unsigned char)ch) || ch == '_';
}

static int is_identifier_char(int ch) {
    return isalnum((unsigned char)ch) || ch == '_';
}

static char *library_name(const char *path) {
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    size_t length = strlen(base);
    if (length <= 3 || strcmp(base + length - 3, ".hly") != 0) return NULL;
    length -= 3;
    if (!is_identifier_start((unsigned char)base[0])) return NULL;
    for (size_t i = 1; i < length; ++i)
        if (!is_identifier_char((unsigned char)base[i])) return NULL;
    char *name = malloc(length + 1);
    if (!name) return NULL;
    memcpy(name, base, length);
    name[length] = '\0';
    return name;
}

static int emit_source(FILE *output, const char *path, size_t index) {
    FILE *input = fopen(path, "rb");
    if (!input) {
        perror(path);
        return 0;
    }
    fprintf(output, "static const char y_library_%zu[] =\n    \"", index);
    int ch;
    while ((ch = fgetc(input)) != EOF) {
        switch (ch) {
            case '\n': fputs("\\n\"\n    \"", output); break;
            case '\r': fputs("\\r", output); break;
            case '\t': fputs("\\t", output); break;
            case '\\': fputs("\\\\", output); break;
            case '"': fputs("\\\"", output); break;
            default:
                if (ch >= 32 && ch <= 126) fputc(ch, output);
                else fprintf(output, "\\%03o", (unsigned char)ch);
                break;
        }
    }
    int okay = !ferror(input);
    fclose(input);
    fputs("\";\n", output);
    return okay;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s output.c library.hly [library.hly ...]\n", argv[0]);
        return 2;
    }
    FILE *output = fopen(argv[1], "w");
    if (!output) {
        perror(argv[1]);
        return 1;
    }
    fputs("#include <string.h>\n#include \"embedded_libraries.h\"\n\n", output);
    char **names = calloc((size_t)(argc - 2), sizeof(*names));
    if (!names) {
        fclose(output);
        return 1;
    }
    int okay = 1;
    for (int i = 2; i < argc; ++i) {
        names[i - 2] = library_name(argv[i]);
        if (!names[i - 2]) {
            fprintf(stderr, "%s: library filename must be a .hly file with an identifier name\n", argv[i]);
            okay = 0;
            break;
        }
        for (int prior = 2; prior < i; ++prior) {
            if (strcmp(names[prior - 2], names[i - 2]) == 0) {
                fprintf(stderr, "duplicate embedded library name: %s\n", names[i - 2]);
                okay = 0;
                break;
            }
        }
        if (!okay || !emit_source(output, argv[i], (size_t)(i - 2))) {
            okay = 0;
            break;
        }
        fputc('\n', output);
    }
    if (okay) {
        fputs("const char *y_embedded_library(const char *name) {\n", output);
        for (int i = 2; i < argc; ++i)
            fprintf(output, "    if (strcmp(name, \"%s\") == 0) return y_library_%d;\n", names[i - 2], i - 2);
        fputs("    return NULL;\n}\n", output);
    }
    for (int i = 0; i < argc - 2; ++i) free(names[i]);
    free(names);
    if (fclose(output) != 0) okay = 0;
    return okay ? 0 : 1;
}