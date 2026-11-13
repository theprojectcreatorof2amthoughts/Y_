#ifndef Y_LANG_H
#define Y_LANG_H

#include <stddef.h>

int y_program_main(int argc, char **argv);
int y_program_run_source(const char *source, const char *path);
int y_program_check_sources(const char *const *sources, const char *const *paths,
                            size_t count, size_t entry_index);
int y_program_run_sources(const char *const *sources, const char *const *paths,
                          size_t count, size_t entry_index);

#endif
