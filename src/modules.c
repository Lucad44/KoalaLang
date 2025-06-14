#include "modules.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "hashmap.h"
#include "klc_math.h"
#include "klc_trig.h"
#include "variables.h"

struct hashmap *module_map = NULL;

struct hashmap *imported_modules = NULL;

static void __attribute__((constructor)) init_module_map() {
    module_map = hashmap_new(sizeof(Module), 0, 0, 0,
        module_hash, module_compare, NULL, NULL);
    imported_modules = hashmap_new(sizeof(Module), 0, 0, 0,
        module_hash, module_compare, NULL, NULL);

    struct hashmap *math_functions_map = hashmap_new(sizeof(FunctionMeta), 0, 0, 0,
        function_meta_hash, function_meta_compare, NULL, NULL);
    struct hashmap *math_variables_map = hashmap_new(sizeof(Variable), 0, 0, 0,
        variable_hash, variable_compare, NULL, NULL);

    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "is_positive", .func = (void *) klc_is_positive,
            .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "is_negative", .func = (void *) klc_is_negative,
            .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "is_zero", .func = (void *) klc_is_zero,
            .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "is_integer", .func = (void *) klc_is_integer,
            .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "is_float", .func = (void *) klc_is_float,
            .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "is_even", .func = (void *) klc_is_even,
            .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "is_odd", .func = (void *) klc_is_odd,
            .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "is_palindrome", .func = (void *) klc_is_palindrome,
            .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "is_prime", .func = (void *) klc_is_prime,
            .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "floor", .func = (void *) klc_floor,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "ceil", .func = (void *) klc_ceil,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "round", .func = (void *) klc_round,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "sqrt", .func = (void *) klc_sqrt,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "cbrt", .func = (void *) klc_cbrt,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "abs", .func = (void *) klc_abs,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "inverse", .func = (void *) klc_inverse,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "factorial", .func = (void *) klc_factorial,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "gamma", .func = (void *) klc_gamma,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "fibonacci", .func = (void *) klc_fibonacci,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "ln", .func = (void *) klc_ln,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "log10", .func = (void *) klc_log10,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "log2", .func = (void *) klc_log2,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "log", .func = (void *) klc_log,
            .dispatcher = dispatch_double_double_to_double, .ret_type = TYPE_DOUBLE,
            .param_types = {
                TYPE_DOUBLE,
                TYPE_DOUBLE
            }, .param_count = 2
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "integrate", .func = (void *) klc_integrate,
            .dispatcher = dispatch_string_to_string, .ret_type = TYPE_STRING, .param_types = {
                TYPE_STRING
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "plot_function", .func = (void *) klc_plot_function,
            .dispatcher = dispatch_string_to_void, .ret_type = TYPE_VOID, .param_types = {
                TYPE_STRING
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "plot_multiple_functions", .func = (void *) klc_plot_multiple_functions,
            .dispatcher = dispatch_string_array_to_void, .ret_type = TYPE_VOID, .param_types = {
                TYPE_STRING_ARRAY
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "plot_2vars_function", .func = (void *) klc_plot_2vars_function,
            .dispatcher = dispatch_string_to_void, .ret_type = TYPE_VOID, .param_types = {
                TYPE_STRING
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "plot_csv", .func = (void *) klc_plot_csv,
            .dispatcher = dispatch_string_to_void, .ret_type = TYPE_VOID, .param_types = {
                TYPE_STRING
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "simplify_expression", .func = (void *) klc_simplify_expression,
            .dispatcher = dispatch_string_to_string, .ret_type = TYPE_STRING, .param_types = {
                TYPE_STRING
            }, .param_count = 1
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "evaluate_function", .func = (void *) klc_evaluate_function,
            .dispatcher = dispatch_string_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_STRING,
                TYPE_DOUBLE
            }, .param_count = 2
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "subtract_expressions", .func = (void *) klc_subtract_expressions,
            .dispatcher = dispatch_string_string_to_string, .ret_type = TYPE_STRING, .param_types = {
                TYPE_STRING,
                TYPE_STRING
            }, .param_count = 2
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "differentiate", .func = (void *) klc_differentiate,
            .dispatcher = dispatch_string_string_to_string, .ret_type = TYPE_STRING, .param_types = {
                TYPE_STRING,
                TYPE_STRING
            }, .param_count = 2
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "polynomial_division", .func = (void *) klc_polynomial_division,
            .dispatcher = dispatch_string_string_to_string, .ret_type = TYPE_STRING, .param_types = {
                TYPE_STRING,
                TYPE_STRING
            }, .param_count = 2
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "definite_integral", .func = (void *) klc_definite_integral,
            .dispatcher = dispatch_string_double_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_STRING,
                TYPE_DOUBLE,
                TYPE_DOUBLE
            }, .param_count = 3
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "limit", .func = (void *) klc_limit,
            .dispatcher = dispatch_string_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_STRING,
                TYPE_DOUBLE
            }, .param_count = 2
    });
    hashmap_set(math_functions_map, &(FunctionMeta) {
        .name = "solve_equation", .func = (void *) klc_solve_equation,
            .dispatcher = dispatch_string_to_double_array, .ret_type = TYPE_DOUBLE_ARRAY, .param_types = {
                TYPE_STRING
            }, .param_count = 1
    });

    hashmap_set(math_variables_map, &(Variable) {
        .name = "pi", .type = VAR_NUM, .value = 3.1415926535897932384626433
    });
    hashmap_set(math_variables_map, &(Variable) {
        .name = "e", .type = VAR_NUM, .value = 2.718281828459045235360287471
    });
    hashmap_set(math_variables_map, &(Variable) {
        .name = "phi", .type = VAR_NUM, .value = 1.618033988749894848204586834365
    });
    hashmap_set(math_variables_map, &(Variable) {
        .name = "silver_ratio", .type = VAR_NUM, .value = 2.41421356237309504880
    });
    hashmap_set(math_variables_map, &(Variable) {
        .name = "supergolden_ratio", .type = VAR_NUM, .value = 1.46557123187676802665
    });
    hashmap_set(math_variables_map, &(Variable) {
        .name = "posinf", .type = VAR_NUM, .value = INFINITY
    });
    hashmap_set(math_variables_map, &(Variable) {
        .name = "neginf", .type = VAR_NUM, .value = -INFINITY
    });

    struct hashmap *trig_functions_map = hashmap_new(sizeof(FunctionMeta), 0, 0, 0,
        function_meta_hash, function_meta_compare, NULL, NULL);
    struct hashmap *trig_variables_map = hashmap_new(sizeof(Variable), 0, 0, 0,
        variable_hash, variable_compare, NULL, NULL);
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "sin", .func = (void *) klc_sin,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "cos", .func = (void *) klc_cos,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "tan", .func = (void *) klc_tan,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "cot", .func = (void *) klc_cot,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "sec", .func = (void *) klc_sec,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "csc", .func = (void *) klc_csc,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "arcsin", .func = (void *) klc_arcsin,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "arccos", .func = (void *) klc_arccos,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "arctan", .func = (void *) klc_arctan,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "arccot", .func = (void *) klc_arccot,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "arcsec", .func = (void *) klc_arcsec,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
                TYPE_DOUBLE
            }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "arccsc", .func = (void *) klc_arccsc, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
            TYPE_DOUBLE
        }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "sinh", .func = (void *) klc_sinh, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
            TYPE_DOUBLE
        }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "cosh", .func = (void *) klc_cosh, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
            TYPE_DOUBLE
        }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "tanh", .func = (void *) klc_tanh, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
            TYPE_DOUBLE
        }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "coth", .func = (void *) klc_coth, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
            TYPE_DOUBLE
        }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "sech", .func = (void *) klc_sech, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
            TYPE_DOUBLE
        }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "csch", .func = (void *) klc_csch, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
            TYPE_DOUBLE
        }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "arcsinh", .func = (void *) klc_arcsinh, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
            TYPE_DOUBLE
        }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "arccosh", .func = (void *) klc_arccosh, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
            TYPE_DOUBLE
        }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "arctanh", .func = (void *) klc_arctanh, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
            TYPE_DOUBLE
        }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "arccoth", .func = (void *) klc_arccoth, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
            TYPE_DOUBLE
        }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "arcsech", .func = (void *) klc_arcsech, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
            TYPE_DOUBLE
        }, .param_count = 1
    });
    hashmap_set(trig_functions_map, &(FunctionMeta) {
        .name = "arccsch", .func = (void *) klc_arccsch, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {
            TYPE_DOUBLE
        }, .param_count = 1
    });

    hashmap_set(trig_variables_map, &(Variable) {
        .name = "pi", .type = VAR_NUM, .value = 3.1415926535897932384626433
    });

    Module *math_mod = malloc(sizeof(Module));
    math_mod->name = strdup("math");
    math_mod->module_function_meta_map = math_functions_map;
    math_mod->module_variables_map = math_variables_map;
    hashmap_set(module_map, math_mod);

    Module *trig_mod = malloc(sizeof(Module));
    trig_mod->name = strdup("trig");
    trig_mod->module_function_meta_map = trig_functions_map;
    trig_mod->module_variables_map = trig_variables_map;
    hashmap_set(module_map, trig_mod);
}

int module_compare(const void *a,
    const void *b, void *udata) {
    const Module *ma = a;
    const Module *mb = b;
    return strcmp(ma->name, mb->name);
}

uint64_t module_hash(const void *item,
    const uint64_t seed0,
        const uint64_t seed1) {
    const Module *module = item;
    return hashmap_sip(module->name, strlen(module->name), seed0, seed1);
}

bool module_iter(const void *item, void *udata) {
    const Module *module = item;
    printf("\nmodule %s", module->name);
    return true;
}

int function_meta_compare(const void *a,
    const void *b, void *udata) {
    const FunctionMeta *klc_c_fa = a;
    const FunctionMeta *klc_c_fb = b;
    return strcmp(klc_c_fa->name, klc_c_fb->name);
}

uint64_t function_meta_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const FunctionMeta *klc_c_meta_function = item;
    return hashmap_sip(klc_c_meta_function->name, strlen(klc_c_meta_function->name), seed0, seed1);
}

const FunctionMeta *get_function_meta_from_module(struct hashmap *map,
    const char *func_name, char *module_name) {
    if (!module_name || !func_name) {
        return NULL;
    }
    const Module *module = hashmap_get(imported_modules, &(Module) {
        .name = module_name
    });
    if (!module) {
        return NULL;
    }
    const FunctionMeta *function_meta = hashmap_get(module->module_function_meta_map, &(FunctionMeta) {
        .name = func_name
    });
    return function_meta;
}

const FunctionMeta *get_function_meta_from_modules(struct hashmap *map,
    const char *func_name) {
    size_t iter = 0;
    void *item;
    while (hashmap_iter(map, &iter, &item)) {
        const Module *module = item;
        const FunctionMeta *function_meta = get_function_meta_from_module(module->module_function_meta_map, func_name, module->name);
        if (function_meta) {
            return function_meta;
        }
    }
    return NULL;
}

const Variable *get_variable_from_module(struct hashmap *map, char *var_name, char *module_name) {
    if (!map || !module_name || !var_name) {
        return NULL;
    }
    const Module *module = hashmap_get(imported_modules, &(Module) {
        .name = module_name
    });
    if (!module) {
        return NULL;
    }
    const Variable *variable = hashmap_get(module->module_variables_map, &(Variable) {
        .name = var_name
    });
    return variable;
}

const Variable *get_variable_from_modules(struct hashmap *map, char *var_name) {
    size_t iter = 0;
    void *item;
    while (hashmap_iter(map, &iter, &item)) {
        const Module *module = item;
        const Variable *
            var = get_variable(module->module_variables_map, var_name);
        if (var) {
            return var;
        }
    }
    return NULL;
}

void deep_copy_function_meta(void *dest,
    const void *src, void *udata) {
    const FunctionMeta *src_meta = src;
    FunctionMeta *dest_meta = dest;

    if (src_meta->name) {
        dest_meta->name = strdup(src_meta->name);
    } else {
        dest_meta->name = NULL;
    }

    dest_meta->func = src_meta->func;
    dest_meta->dispatcher = src_meta->dispatcher;
    dest_meta->ret_type = src_meta->ret_type;
    dest_meta->param_count = src_meta->param_count;
    memcpy(dest_meta->param_types, src_meta->param_types, sizeof(DataType) *128);
}

void dispatch_double_to_int(void *func_ptr, void **args, void *ret_out) {
    int( *func)(double) = (int( *)(double)) func_ptr;
    const double n = *(double *) args[0];
    *(int *) ret_out = func(n);
}

void dispatch_double_to_double(void *func_ptr, void **args, void *ret_out) {
    double( *func)(double) = (double( *)(double)) func_ptr;
    const double n = *(double *) args[0];
    *(double *) ret_out = func(n);
}

void dispatch_double_double_to_double(void *func_ptr, void **args, void *ret_out) {
    double( *func)(double, double) = (double( *)(double, double)) func_ptr;
    const double a = *(double *) args[0];
    const double b = *(double *) args[1];
    *(double *) ret_out = func(a, b);
}

void dispatch_string_to_string(void *func_ptr, void **args, void *ret_out) {
    char *( *func)(char *) = (char *( *)(char *)) func_ptr;
    char *input_expr = *(char **) args[0];
    *(char **) ret_out = func(input_expr);
}

void dispatch_string_to_void(void *func_ptr, void **args, void *ret_out) {
    (void) ret_out;
    void( *func)(const char *) = (void( *)(const char *)) func_ptr;
    func( *(const char **) args[0]);
}

void dispatch_string_array_to_void(void *func_ptr, void **args, void *ret_out) {
    (void) ret_out;
    void( *func)(const char **, int) = (void( *)(const char **, int)) func_ptr;

    VariableValue *list_value = (VariableValue *) args[0];

    ListNode *current = list_value->list_val.head;
    int count = 0;
    while (current != NULL) {
        count++;
        current = current->next;
    }

    const char **arr = malloc(count *sizeof(const char *));
    if (!arr) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    current = list_value->list_val.head;
    for (int i = 0; i < count; i++) {
        if (current->element.type != VAR_STR) {
            arr[i] = "";
        } else {
            arr[i] = current->element.value.str_val;
        }
        current = current->next;
    }

    func(arr, count);

    free(arr);
}

void dispatch_string_string_to_string(void *func_ptr, void **args, void *ret_out) {
    char *( *func)(char *, char *) = (char *( *)(char *, char *)) func_ptr;
    char *input1 = *(char **) args[0];
    char *input2 = *(char **) args[1];
    *(char **) ret_out = func(input1, input2);
}

void dispatch_string_double_double_to_double(void *fptr, void **args, void *ret_out) {
    double( *func)(const char *, double, double) = (double( *)(const char *, double, double)) fptr;

    const char *input_expr = *(const char **) args[0];
    double a = *(double *) args[1];
    double b = *(double *) args[2];

    if (ret_out) {
        *(double *) ret_out = func(input_expr, a, b);
    }
}

void dispatch_string_double_to_double(void *fptr, void **args, void *ret_out) {
    double( *func)(const char *, double) = (double( *)(const char *, double)) fptr;

    const char *s = *(const char **) args[0];
    double d = *(double *) args[1];

    if (ret_out) {
        *(double *) ret_out = func(s, d);
    }
}

void dispatch_string_to_double_array(void *func_ptr, void **args, void *ret_out) {

    double *( *func)(const char *) =
        (double *( *)(const char *)) func_ptr;

    const char *input = *(char **) args[0];

    double *arr = func(input);
    if (!arr) {

        ((VariableValue *) ret_out)->list_val.element_type = VAR_NUM;
        ((VariableValue *) ret_out)->list_val.nested_element_type = VAR_NUM;
        ((VariableValue *) ret_out)->list_val.is_nested = false;
        ((VariableValue *) ret_out)->list_val.head = NULL;
        return;
    }

    VariableValue *vv = (VariableValue *) ret_out;
    vv->list_val.element_type = VAR_NUM;
    vv->list_val.nested_element_type = VAR_NUM;
    vv->list_val.is_nested = false;
    vv->list_val.head = NULL;

    ListNode *prev = NULL;
    for (int i = 0; arr[i] == arr[i]; i++) {
        ListElement elem = {
            .type = VAR_NUM,
            .value.num_val = arr[i]
        };
        ListNode *node = malloc(sizeof *node);
        if (!node) break;
        node->element = elem;
        node->next = NULL;

        if (!prev) vv->list_val.head = node;
        else prev->next = node;
        prev = node;
    }

    free(arr);
}