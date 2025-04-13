#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "ast.h"
#include "hashmap.h"

typedef struct {
    char *name;
    int param_count;
    Parameter *parameters;
    ASTNode *body;
} Function;

extern struct hashmap *function_map;

int function_compare(const void *a, const void *b, void *udata);

bool function_iter(const void *item, void *udata);

uint64_t function_hash(const void *item, uint64_t seed0, uint64_t seed1);

#endif //FUNCTIONS_H