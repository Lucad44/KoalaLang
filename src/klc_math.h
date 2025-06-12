#ifndef KLC_MATH_H
#define KLC_MATH_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

static
const char *colors[] = {
    "red",
    "blue",
    "green",
    "magenta",
    "cyan",
    "orange",
    "black",
    "violet"
};

static
const int color_count = sizeof(colors) / sizeof(colors[0]);

int klc_is_positive(double n);

int klc_is_negative(double n);

int klc_is_zero(double n);

int klc_is_integer(double n);

int klc_is_float(double n);

int klc_is_even(double n);

int klc_is_odd(double n);

int klc_is_palindrome(double n);

int klc_is_prime(double n);

double klc_floor(double n);

double klc_ceil(double n);

double klc_round(double n);

double klc_sqrt(double n);

double klc_cbrt(double n);

double klc_abs(double n);

double klc_inverse(double n);

double klc_factorial(double n);

double klc_gamma(double n);

double klc_fibonacci(double n);

double klc_ln(double n);

double klc_log10(double n);

double klc_log2(double n);

double klc_log(double n, double base);

double klc_evaluate_function(const char *expr_str, double x);

char *klc_subtract_expressions(const char *a,
    const char *b);

char *klc_integrate(const char *input_expr,
    const char *variable);

double klc_definite_integral(const char *input_expr, double a, double b);

void klc_plot_function(const char *input_expr);

void klc_plot_multiple_functions(const char *input_exprs[], int count);

void klc_plot_2vars_function(const char *input_expr);

void klc_plot_csv(const char *csv_path);

char *klc_simplify_expression(const char *input_expr);

char *klc_differentiate(const char *input_expr,
    const char *variable);

char *klc_polynomial_division(const char *dividend_expr,
    const char *divisor_expr);

double klc_limit(const char *input_expr, double limit_point);

double *klc_solve_equation(const char *input_expr);

#endif