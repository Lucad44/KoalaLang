#include <string.h>
#include <stdio.h>
#include "functions.h"

struct hashmap *function_map = NULL;

static void __attribute__((constructor)) init_function_map() {
    function_map = hashmap_new(sizeof(Function), 0, 0, 0,
        function_hash, function_compare, NULL, NULL);
}

int function_compare(const void *a, const void *b, void *udata) {
    const Function *fa = a;
    const Function *fb = b;
    return strcmp(fa->name, fb->name);
}

uint64_t function_hash(const void *item, const uint64_t seed0, const uint64_t seed1) {
    const Function *function = item;
    return hashmap_sip(function->name, strlen(function->name), seed0, seed1);
}

bool function_iter(const void *item, void *udata) {
    const Function *function = item;
    printf("\n%s(", function->name);
    for (int i = 0; i < function->param_count; i++) {
        if (function->parameters[i].type == VAR_NUM) {
            printf("int %s", function->parameters[i].name);
        } else if (function->parameters[i].type == VAR_STR) {
            printf("str %s", function->parameters[i].name);
        } else if (function->parameters[i].type == VAR_LIST) {
            printf("list[] %s", function->parameters[i].name);
        }
        if (i != function->param_count - 1) {
            printf(", ");
        }
    }
    printf(")\n");
    return true;
}
