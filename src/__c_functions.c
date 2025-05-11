#include "__c_functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define EPSILON 0.00000000000000001
#define PI 3.1415926535897932384626433
#define E 2.718281828459045235360287471


struct hashmap *__c_functions_meta_map = NULL;

static void __attribute__((constructor)) init___c_function_meta_map() {
    __c_functions_meta_map = hashmap_new(sizeof(__C_FunctionMeta), 0, 0, 0,
        __c_function_meta_hash, __c_function_meta_compare, NULL, NULL);

    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "is_positive", .func = (void *) __is_positive,
        .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "is_negative", .func = (void *) __is_negative,
        .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "is_zero", .func = (void *) __is_zero,
        .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "is_integer", .func = (void *) __is_integer,
        .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "is_float", .func = (void *) __is_float,
        .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "is_even", .func = (void *) __is_even,
        .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "is_odd", .func = (void *) __is_odd,
        .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "is_palindrome", .func = (void *) __is_palindrome,
        .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "is_prime", .func = (void *) __is_prime,
        .dispatcher = dispatch_double_to_int, .ret_type = TYPE_INT, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "floor", .func = (void *) __floor,
        .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "ceil", .func = (void *) __ceil,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "round", .func = (void *) __round,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "sqrt", .func = (void *) __sqrt,
        .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "cbrt", .func = (void *) __cbrt,
        .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "abs", .func = (void *) __abs,
        .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "inverse", .func = (void *) __inverse,
        .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "factorial", .func = (void *) __factorial,
        .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "ln", .func = (void *) __ln,
        .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "log10", .func = (void *) __log10,
        .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "log2", .func = (void *) __log2,
        .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "log", .func = (void *) __log,
        .dispatcher = dispatch_double_double_to_double, .ret_type = TYPE_DOUBLE,
        .param_types = {TYPE_DOUBLE, TYPE_DOUBLE}, .param_count = 2});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "degrees_to_radians", .func = (void *) __degrees_to_radians,
        .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "radians_to_degrees", .func = (void *) __radians_to_degrees,
        .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "sin", .func = (void *) __sin,
            .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "cos", .func = (void *) __cos,
        .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "tan", .func = (void *) __tan,
       .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "cot", .func = (void *) __cot,
       .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "sec", .func = (void *) __sec,
       .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "csc", .func = (void *) __csc,
       .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "arcsin", .func = (void *) __arcsin,
       .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "arccos", .func = (void *) __arccos,
       .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "arctan", .func = (void *) __arctan,
       .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "arccot", .func = (void *) __arccot,
       .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
    hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "arcsec", .func = (void *) __arcsec,
           .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1});
hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "arccsc", .func = (void *) __arccsc, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1 });
hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "sinh", .func = (void *) __sinh, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1 });
hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "cosh", .func = (void *) __cosh, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1 });
hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "tanh", .func = (void *) __tanh, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1 });
hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "coth", .func = (void *) __coth, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1 });
hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "sech", .func = (void *) __sech, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1 });
hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "csch", .func = (void *) __csch, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1 });
hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "arcsinh", .func = (void *) __arcsinh, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1 });
hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "arccosh", .func = (void *) __arccosh, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1 });
hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "arctanh", .func = (void *) __arctanh, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1 });
hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "arccoth", .func = (void *) __arccoth, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1 });
hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "arcsech", .func = (void *) __arcsech, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1 });
hashmap_set(__c_functions_meta_map, &(__C_FunctionMeta) { .name = "arccsch", .func = (void *) __arccsch, .dispatcher = dispatch_double_to_double, .ret_type = TYPE_DOUBLE, .param_types = {TYPE_DOUBLE}, .param_count = 1 });
}

int __c_function_meta_compare(const void *a, const void *b, void *udata) {
    const __C_FunctionMeta *__c_fa = a;
    const __C_FunctionMeta *__c_fb = b;
    return strcmp(__c_fa->name, __c_fb->name);
}

uint64_t __c_function_meta_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const __C_FunctionMeta *__c_meta_function = item;
    return hashmap_sip(__c_meta_function->name, strlen(__c_meta_function->name), seed0, seed1);
}

int __is_positive(const double n) {
    return n > 0;
}

int __is_negative(const double n) {
    return n < 0;
}

int __is_zero(const double n) {
    return n == 0;
}

int __is_integer(const double n) {
    return floor(n) == n;
}

int __is_float(const double n) {
    return !__is_integer(n);
}

int __is_even(const double n) {
    if (__is_float(n)) {
        return false;
    }
    const long long integer_n = (long long) n;
    return integer_n % 2 == 0;
}

int __is_odd(const double n) {
    if (__is_float(n)) {
        return 0;
    }
    const long long integer_n = (long long) n;
    return integer_n % 2 != 0;
}

int __is_palindrome(const double n) {
    const char *str = (char *) &n;
    const int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        if (str[i] != str[len - 1 - i]) {
            return 0;
        }
    }
    return 1;
}

int __is_prime(const double n) {
    if (floor(n) != n || n < 2) {
        return 0;
    }
    const long long integer_n = (long long) n;
    for (int i = 2; i <= sqrt(integer_n); i++) {
        if (integer_n % i == 0) {
            return 0;
        }
    }
    return 1;
}

double __floor(const double n) {
    return floorl(n);
}

double __ceil(const double n) {
    return ceill(n);
}

double __round(const double n) {
    const double floor = __floor(n);
    const double ceil = __ceil(n);
    if (n - floor > ceil - n) {
        return ceil;
    }
    return floor;
}

double __sqrt(const double n) {
    if (n < 0) {
        fprintf(stderr, "Error: square root of negative number\n");
        exit(EXIT_FAILURE);
    }
    double guess = n / 2.0;
    double prev_guess;

    do {
        prev_guess = guess;
        guess = (guess + n / guess) / 2.0;
    } while ((prev_guess - guess > EPSILON) || (guess - prev_guess > EPSILON));

    return guess;
}

double __cbrt(const double n) {
    if (n == 0) {
        return 0;
    }

    double guess = n / 3.0;
    double prev_guess;

    do {
        prev_guess = guess;
        guess = (2.0 * guess + n / (guess * guess)) / 3.0;
    } while ((prev_guess - guess > EPSILON) || (guess - prev_guess > EPSILON));

    return guess;
}

double __abs(const double n) {
    if (n < 0) {
        return -n;
    }
    return n;
}

double __inverse(const double n) {
    if (n == 0) {
        return 0;
    }
    return 1.0 / n;
}

double __factorial(const double n) {
    if (n <= 0) {
        printf("Error: Factorial not defined for negative integers.\n");
        exit(EXIT_FAILURE);
    }
    return tgamma(n + 1);
}

double __ln(const double n) {
    if (n <= 0) {
        fprintf(stderr, "Error: ln not defined for negative or zero numbers.\n");
        exit(EXIT_FAILURE);
    }
    return log(n);
}

double __log10(const double n) {
    if (n <= 0) {
        fprintf(stderr, "Error: log10 not defined for negative or zero numbers.\n");
        exit(EXIT_FAILURE);
    }
    return log(n) / log(10);
}

double __log2(const double n) {
    if (n <= 0) {
        fprintf(stderr, "Error: log2 not defined for negative or zero numbers.\n");
        exit(EXIT_FAILURE);
    }
    return log(n) / log(2);
}

double __log(const double n, const double base) {
    if (n <= 0) {
        fprintf(stderr, "Error: log not defined for negative or zero numbers.\n");
        exit(EXIT_FAILURE);
    }
    if (base <= 0 || base == 1) {
        fprintf(stderr, "Error: log base must be positive and not equal to 1.\n");
        exit(EXIT_FAILURE);
    }
    return log(n) / log(base);
}

double __degrees_to_radians(const double n) {
    return n * PI / 180;
}

double __radians_to_degrees(const double n) {
    return n * 180 / PI;
}

double __sin(const double n) {
    return sin(n);
}

double __cos(const double n) {
    return cos(n);
}

double __tan(const double n) {
    return tan(n);
}

double __cot(const double n) {
    return 1.0 / tan(n);
}

double __sec(const double n) {
    return 1.0 / cos(n);
}

double __csc(const double n) {
    return 1.0 / sin(n);
}

double __arcsin(const double n) {
    if (n < -1 || n > 1) {
        fprintf(stderr, "Error: arcsin argument must be in the range [−1,1].\n");
        exit(EXIT_FAILURE);
    }
    return asin(n);
}

double __arccos(const double n) {
    if (n < -1 || n > 1) {
        fprintf(stderr, "Error: arccos argument must be in the range [−1,1].\n");
        exit(EXIT_FAILURE);
    }
    return acos(n);
}

double __arctan(const double n) {
    return atan(n);
}

double __arccot(const double n) {
    return atan(1.0 / n);
}

double __arcsec(const double n) {
    if (n > -1 && n < 1) {
        fprintf(stderr, "Error: arcsec argument must be in the range (−inf,−1]U[1,+inf).\n");
        exit(EXIT_FAILURE);
    }
    return acos(1.0 / n);
}

double __arccsc(const double n) {
    if (n > -1 && n < 1) {
        fprintf(stderr, "Error: arccsc argument must be in the range (−inf,−1]U[1,+inf).\n");
        exit(EXIT_FAILURE);
    }
    return asin(1.0 / n);
}

double __sinh(const double n) {
    return sinh(n);
}

double __cosh(const double n) {
    return cosh(n);
}

double __tanh(const double n) {
    return tanh(n);
}

double __coth(const double n) {
    if (n == 0) {
        fprintf(stderr, "Error: coth argument must be in the range (−inf,0)U(0,+inf).\n");
        exit(EXIT_FAILURE);
    }
    return 1.0 / tanh(n);
}

double __sech(const double n) {
    return 1.0 / cosh(n);
}

double __csch(const double n) {
    if (n == 0) {
        fprintf(stderr, "Error: csch argument must be in the range (−inf,0)U(0,+inf).\n");
        exit(EXIT_FAILURE);
    }
    return 1.0 / sinh(n);
}

double __arcsinh(const double n) {
    return asinh(n);
}

double __arccosh(const double n) {
    if (n < 1) {
        fprintf(stderr, "Error: arccosh argument must be in the range [1,+inf).\n");
        exit(EXIT_FAILURE);
    }
    return acosh(n);
}

double __arctanh(const double n) {
    if (n <= -1 || n >= 1) {
        fprintf(stderr, "Error: arctanh argument must be in the range (−1,1).\n");
        exit(EXIT_FAILURE);
    }
    return atanh(n);
}

double __arccoth(const double n) {
    if (n >= -1 && n <= 1) {
        fprintf(stderr, "Error: arccoth argument must be in the range (−inf,−1)U(1,+inf).\n");
        exit(EXIT_FAILURE);
    }
    return atanh(1.0 / n);
}

double __arcsech(const double n) {
    if (n <= 0 || n > 1) {
        fprintf(stderr, "Error: arcsech argument must be in the range (0,1].\n");
        exit(EXIT_FAILURE);
    }
    return acosh(1.0 / n);
}

double __arccsch(const double n) {
    if (n > -1 && n < 1)  {
        fprintf(stderr, "Error: arccsch argument must be in the range (−inf,−1]U[1,+inf).\n");
        exit(EXIT_FAILURE);
    }
    return asinh(1.0 / n);
}

void dispatch_double_to_int(void *func_ptr, void **args, void *ret_out) {
    int (*func)(double) = (int (*)(double)) func_ptr;
    const double n = *(double *) args[0];
    *(int *) ret_out = func(n);
}

void dispatch_double_to_double(void *func_ptr, void **args, void *ret_out) {
    double (*func)(double) = (double (*)(double)) func_ptr;
    const double n = *(double *) args[0];
    *(double *) ret_out = func(n);
}

void dispatch_double_double_to_double(void *func_ptr, void **args, void *ret_out) {
    double (*func)(double, double) = (double (*)(double, double)) func_ptr;
    const double a = *(double *) args[0];
    const double b = *(double *) args[1];
    *(double *) ret_out = func(a, b);
}
