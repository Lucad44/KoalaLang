#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

void* safe_malloc(size_t size);

void* safe_calloc(size_t num, size_t size);

void* safe_realloc(void *ptr, size_t new_size);

void free_ast(void *node);

#endif // MEMORY_H
