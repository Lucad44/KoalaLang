#ifndef __C_FUNCTIONS_H
#define __C_FUNCTIONS_H

#include "hashmap.h"

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_VOID
} __C_DataType;

typedef void (*__C_DispatcherFunc) (void *func_ptr, void **args, void *ret_out);

typedef struct {
    const char *name;
    void *func;
    __C_DispatcherFunc dispatcher;
    __C_DataType ret_type;
    __C_DataType param_types[128];
    int param_count;
} __C_FunctionMeta;

extern struct hashmap *__c_functions_meta_map;

int __c_function_meta_compare(const void *a, const void *b, void *udata);

uint64_t __c_function_meta_hash(const void *item, uint64_t seed0, uint64_t seed1);

int __is_positive(double n);

int __is_negative(double n);

int __is_zero(double n);

int __is_integer(double n);

int __is_float(double n);

int __is_even(double n);

int __is_odd(double n);

int __is_palindrome(double n);

int __is_prime(double n);

double __floor(double n);

double __ceil(double n);

double __round(double n);

double __sqrt(double n);

double __cbrt(double n);

double __abs(double n);

double __inverse(double n);

double __factorial(double n);

double __ln(double n);

double __log10(double n);

double __log2(double n);

double __log(double n, double base);

double __degrees_to_radians(double n);

double __radians_to_degrees(double n);

double __sin(double n);

double __cos(double n);

double __tan(double n);

double __cot(double n);

double __sec(double n);

double __csc(double n);

double __arcsin(double n);

double __arccos(double n);

double __arctan(double n);

double __arccot(double n);

double __arcsec(double n);

double __arccsc(double n);

double __sinh(double n);

double __cosh(double n);

double __tanh(double n);

double __coth(double n);

double __sech(double n);

double __csch(double n);

double __arcsinh(double n);

double __arccosh(double n);

double __arctanh(double n);

double __arccoth(double n);

double __arcsech(double n);

double __arccsch(double n);

void dispatch_double_to_int(void *func_ptr, void **args, void *ret_out);

void dispatch_double_to_double(void *func_ptr, void **args, void *ret_out);

void dispatch_double_double_to_double(void *func_ptr, void **args, void *ret_out);

#endif //__C_FUNCTIONS_H
