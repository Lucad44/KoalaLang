#include <stdio.h>
#include <string.h>
#include "variables.h"
#include "hashmap.h"

struct hashmap *variable_map = NULL;

static void __attribute__((constructor)) init_variable_map() {
    variable_map = hashmap_new(sizeof(Variable), 0, 0, 0,
        variable_hash, variable_compare, NULL, NULL);

    hashmap_set(variable_map, &(Variable) {
        .name = "__FUNCTION_NUM_RET_VAL",
        .type = VAR_NUM,
        .value = { .num_val = 0 }
    });
    hashmap_set(variable_map, &(Variable) {
        .name = "__FUNCTION_STR_RET_VAL",
        .type = VAR_STR,
        .value = { .str_val = "" }
    });
    hashmap_set(variable_map, &(Variable) {
        .name = "__FUNCTION_RETURN_FLAG",
        .type = VAR_NUM,
        .value = { .num_val = 0 }
    });
}

int variable_compare(const void *a, const void *b, void *udata) {
    const Variable *va = a;
    const Variable *vb = b;
    return strcmp(va->name, vb->name);
}

bool variable_iter(const void *item, void *udata) {
    const Variable *variable = item;
    if (variable->type == VAR_NUM) {
        printf("%s = %f\n", variable->name, variable->value.num_val);
    } else if (variable->type == VAR_STR) {
        printf("%s = %s\n", variable->name, variable->value.str_val);
    }
    return true;
}

uint64_t variable_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const Variable *variable = item;
    return hashmap_sip(variable->name, strlen(variable->name), seed0, seed1);
}
