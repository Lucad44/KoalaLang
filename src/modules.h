#ifndef MODULES_H
#define MODULES_H

#include <stdint.h>
#include "hashmap.h"

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_STRING_ARRAY,
    TYPE_VOID
} DataType;

typedef void (*DispatcherFunc) (void *func_ptr, void **args, void *ret_out);

typedef struct {
    const char *name;
    void *func;
    DispatcherFunc dispatcher;
    DataType ret_type;
    DataType param_types[128];
    int param_count;
} FunctionMeta;

typedef struct {
    char *name;
    struct hashmap *function_meta_map;
} Module;

extern struct hashmap *module_map;

extern struct hashmap *imported_modules;

int module_compare(const void *a, const void *b, void *udata);

uint64_t module_hash(const void *item, uint64_t seed0, uint64_t seed1);

int function_meta_compare(const void *a, const void *b, void *udata);

uint64_t function_meta_hash(const void *item, uint64_t seed0, uint64_t seed1);

bool module_iter(const void *item, void *udata);

const FunctionMeta *get_function_meta(struct hashmap *map, const char *func_name);

void deep_copy_function_meta(void *dest, const void *src, void *udata);

void dispatch_double_to_int(void *func_ptr, void **args, void *ret_out);

void dispatch_double_to_double(void *func_ptr, void **args, void *ret_out);

void dispatch_double_double_to_double(void *func_ptr, void **args, void *ret_out);

void dispatch_string_to_string(void *func_ptr, void **args, void *ret_out);

void dispatch_string_to_void(void *func_ptr, void **args, void *ret_out);

void dispatch_const_char_ptr_array_to_void(void* fptr, void** args, void* ret_out);

#endif