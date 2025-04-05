#ifndef VARIABLES_H
#define VARIABLES_H

#include <stdint.h>

#include "hashmap.h"

typedef enum {
    VAR_NUM,
    VAR_STR,
    VAR_NIL,
} VarType;

typedef  struct {
    double num_val;
    char *str_val;
} VariableValue;

typedef struct {
    char *name;
    VarType type;
    VariableValue value;
} Variable;

extern struct hashmap *variable_map;

int variable_compare(const void *a, const void *b, void *udata);

bool variable_iter(const void *item, void *udata);

uint64_t variable_hash(const void *item, uint64_t seed0, uint64_t seed1);

#endif