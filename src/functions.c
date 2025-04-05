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

bool function_iter(const void *item, void *udata) {
    const Function *function = item;
    printf("\n%s(", function->name);
    for (int i = 0; i < function->param_count; i++) {
        printf("%s %s, ", function->parameters[i].type, function->parameters[i].name);
    }
    printf(")\n");
    return true;
}

uint64_t function_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const Function *function = item;
    return hashmap_sip(function->name, strlen(function->name), seed0, seed1);
}
