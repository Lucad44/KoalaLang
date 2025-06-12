#include "klc_math.h"
#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <symengine/cwrapper.h>
#include <symengine/symengine_exception.h>


#define EPSILON 1e-9
#define MAX_ROOTS 100
#define TOLERANCE 1e-9
#define INT_TOLERANCE 1e-9
#define MAX_ITERATIONS 10000
#define SEARCH_MIN - 1000.0
#define SEARCH_MAX 1000.0
#define SEARCH_STEP 0.1

int klc_is_positive(const double n) {
    return n > 0;
}

int klc_is_negative(const double n) {
    return n < 0;
}

int klc_is_zero(const double n) {
    return n == 0;
}

int klc_is_integer(const double n) {
    return floor(n) == n;
}

int klc_is_float(const double n) {
    return !klc_is_integer(n);
}

int klc_is_even(const double n) {
    if (klc_is_float(n)) {
        return false;
    }
    const long long integer_n = (long long) n;
    return integer_n % 2 == 0;
}

int klc_is_odd(const double n) {
    if (klc_is_float(n)) {
        return 0;
    }
    const long long integer_n = (long long) n;
    return integer_n % 2 != 0;
}

int klc_is_palindrome(const double n) {
    const char *str = (char *) &n;
    const int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        if (str[i] != str[len - 1 - i]) {
            return 0;
        }
    }
    return 1;
}

int klc_is_prime(const double n) {
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

double klc_floor(const double n) {
    return floorl(n);
}

double klc_ceil(const double n) {
    return ceill(n);
}

double klc_round(const double n) {
    const double floor = klc_floor(n);
    const double ceil = klc_ceil(n);
    if (n - floor > ceil - n) {
        return ceil;
    }
    return floor;
}

double klc_sqrt(const double n) {
    if (n < 0) {
        fprintf(stderr, "\nError: square root of negative number\n");
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

double klc_cbrt(const double n) {
    if (n == 0) {
        return 0;
    }

    double guess = n / 3.0;
    double prev_guess;

    do {
        prev_guess = guess;
        guess = (2.0 *guess + n / (guess *guess)) / 3.0;
    } while ((prev_guess - guess > EPSILON) || (guess - prev_guess > EPSILON));

    return guess;
}

double klc_abs(const double n) {
    if (n < 0) {
        return -n;
    }
    return n;
}

double klc_inverse(const double n) {
    if (n == 0) {
        return 0;
    }
    return 1.0 / n;
}

double klc_factorial(const double n) {
    if (n < 0 || floor(n) != n) {
        printf("\nError: Factorial not defined for negative integers or non-integers.\n");
        exit(EXIT_FAILURE);
    }
    double result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

double klc_gamma(const double n) {
    if (n <= 0) {
        fprintf(stderr, "\nError: gamma not defined for negative or zero integers.\n");
        exit(EXIT_FAILURE);
    }
    return tgamma(n);
}

double klc_fibonacci(const double n) {
    if (n < 0) {
        fprintf(stderr, "\nError: fibonacci not defined for negative integers.\n");
        exit(EXIT_FAILURE);
    }
    if (n <= 1) {
        return n;
    }
    int n1 = 0, n2 = 1;
    double result = 0;
    for (int i = 2; i <= n; i++) {
        result = n1 + n2;
        n1 = n2;
        n2 = result;
    }
    return result;
}

double klc_ln(const double n) {
    if (n <= 0) {
        fprintf(stderr, "\nError: ln not defined for negative or zero numbers.\n");
        exit(EXIT_FAILURE);
    }
    return log(n);
}

double klc_log10(const double n) {
    if (n <= 0) {
        fprintf(stderr, "\nError: log10 not defined for negative or zero numbers.\n");
        exit(EXIT_FAILURE);
    }
    return log(n) / log(10);
}

double klc_log2(const double n) {
    if (n <= 0) {
        fprintf(stderr, "\nError: log2 not defined for negative or zero numbers.\n");
        exit(EXIT_FAILURE);
    }
    return log(n) / log(2);
}

double klc_log(const double n,
    const double base) {
    if (n <= 0) {
        fprintf(stderr, "\nError: log not defined for negative or zero numbers.\n");
        exit(EXIT_FAILURE);
    }
    if (base <= 0 || base == 1) {
        fprintf(stderr, "\nError: log base must be positive and not equal to 1.\n");
        exit(EXIT_FAILURE);
    }
    return log(n) / log(base);
}

void klc_plot_function(const char *input_expr) {
    FILE *gp = popen("gnuplot -persistent", "w");
    if (!gp) {
        fprintf(stderr, "\nWarning: gnuplot not found. Can't plot function.\n");
        return;
    }

    fprintf(gp, "set title 'Plot of %s'\n", input_expr);
    fprintf(gp, "set xlabel 'X-axis'\n");
    fprintf(gp, "set ylabel 'Y-axis'\n");
    fprintf(gp, "plot %s with lines lw 2 lc rgb 'blue'\n", input_expr);
    fclose(gp);
}

void klc_plot_multiple_functions(const char *input_exprs[],
    const int count) {
    FILE *gp = popen("gnuplot -persistent", "w");
    if (gp == NULL) {
        fprintf(stderr, "Error: gnuplot not found.\n");
        return;
    }

    fprintf(gp, "set title 'Multiple Function Plot'\n");
    fprintf(gp, "set xlabel 'X'\n");
    fprintf(gp, "set ylabel 'Y'\n");

    fprintf(gp, "plot ");

    for (int i = 0; i < count; i++) {
        const char *color = colors[i % color_count];
        fprintf(gp, "%s with lines lw 2 lc rgb '%s' title '%s'", input_exprs[i], color, input_exprs[i]);
        if (i < count - 1)
            fprintf(gp, ", ");
        else
            fprintf(gp, "\n");
    }

    fclose(gp);
}

void klc_plot_2vars_function(const char *input_expr) {

    FILE *gnuplot = popen("gnuplot -persistent", "w");
    if (gnuplot == NULL) {
        fprintf(stderr, "Could not open pipe to Gnuplot.\n");
        exit(EXIT_FAILURE);
    }

    fprintf(gnuplot, "set title '3D Plot: %s'\n", input_expr);
    fprintf(gnuplot, "set xlabel 'X'\n");
    fprintf(gnuplot, "set ylabel 'Y'\n");
    fprintf(gnuplot, "set zlabel 'Z'\n");
    fprintf(gnuplot, "set xrange [-10:10]\n");
    fprintf(gnuplot, "set yrange [-10:10]\n");
    fprintf(gnuplot, "set zrange [-1:1]\n");
    fprintf(gnuplot, "set hidden3d\n");
    fprintf(gnuplot, "set isosamples 60,60\n");

    fprintf(gnuplot, "splot %s with lines title ''\n", input_expr);

    fflush(gnuplot);

}

void klc_plot_csv(const char *csv_path) {
    FILE *file = fopen(csv_path, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open CSV file: %s\n", csv_path);
        return;
    }

    char line[4096];
    if (!fgets(line, sizeof(line), file)) {
        fprintf(stderr, "Error: Failed to read CSV file.\n");
        fclose(file);
        return;
    }
    fclose(file);

    char *headers[128];
    int num_columns = 0;
    char *token = strtok(line, ",\n\r");
    while (token && num_columns < 128) {
        headers[num_columns] = strdup(token);
        num_columns++;
        token = strtok(NULL, ",\n\r");
    }

    if (num_columns < 2) {
        fprintf(stderr, "Error: Need at least two columns to plot.\n");
        return;
    }

    FILE *gnuplot = popen("gnuplot -persistent", "w");
    if (!gnuplot) {
        fprintf(stderr, "Error: Could not open pipe to Gnuplot.\n");
        return;
    }

    fprintf(gnuplot, "set datafile separator ','\n");
    fprintf(gnuplot, "set title 'CSV Plot: %s'\n", csv_path);
    fprintf(gnuplot, "set xlabel '%s'\n", headers[0]);
    fprintf(gnuplot, "set ylabel 'Y'\n");
    fprintf(gnuplot, "set key outside\n");
    fprintf(gnuplot, "set grid\n");

    fprintf(gnuplot, "plot ");

    for (int col = 1; col < num_columns; col++) {
        fprintf(gnuplot,
            "'%s' every ::1 using 1:%d with lines title '%s'%s",
            csv_path, col + 1, headers[col],
            (col < num_columns - 1) ? ", " : "\n");
    }

    fflush(gnuplot);

    for (int i = 0; i < num_columns; i++) {
        free(headers[i]);
    }
}

char *klc_simplify_expression(const char *input_expr) {
    if (!input_expr) {
        return NULL;
    }

    basic expr, expanded;
    basic_new_stack(expr);
    basic_new_stack(expanded);

    char *result_str = NULL;

    if (basic_parse(expr, input_expr) != SYMENGINE_NO_EXCEPTION) {
        fprintf(stderr, "Error: Failed to parse expression: %s\n", input_expr);
        goto cleanup;
    }

    basic_expand(expanded, expr);

    char *temp_str = basic_str(expanded);
    if (!temp_str) {
        fprintf(stderr, "Error: Failed to convert expanded expression to string\n");
        goto cleanup;
    }

    size_t len = strlen(temp_str);
    result_str = (char *) malloc(len + 1);
    if (result_str) {
        strcpy(result_str, temp_str);
    }

    basic_str_free(temp_str);

    cleanup:

        basic_free_stack(expr);
    basic_free_stack(expanded);

    return result_str;
}

char *klc_differentiate(const char *input_expr,
    const char *variable) {
    if (!input_expr || !variable) {
        return NULL;
    }

    basic expr,
    var, result;
    basic_new_stack(expr);
    basic_new_stack(var);
    basic_new_stack(result);

    char *result_str = NULL;

    if (basic_parse(expr, input_expr) != SYMENGINE_NO_EXCEPTION) {
        fprintf(stderr, "Error: Failed to parse expression: %s\n", input_expr);
        goto cleanup;
    }

    symbol_set(var, variable);

    basic_diff(result, expr,
        var);

    char *temp_str = basic_str(result);
    if (!temp_str) {
        fprintf(stderr, "Error: Failed to convert result to string\n");
        goto cleanup;
    }

    size_t len = strlen(temp_str);
    result_str = (char *) malloc(len + 1);
    if (result_str) {
        strcpy(result_str, temp_str);
    }

    basic_str_free(temp_str);

    cleanup:

        basic_free_stack(expr);
    basic_free_stack(var);
    basic_free_stack(result);

    return result_str;
}

char *klc_integrate(const char *input_expr,
    const char *variable) {
    /*if (!input_expr || !variable) {
        return NULL;
    }
    
    
    basic expr, var, result;
    basic_new_stack(expr);
    basic_new_stack(var);
    basic_new_stack(result);
    
    char *result_str = NULL;
    
    
    if (basic_parse(expr, input_expr) != SYMENGINE_NO_EXCEPTION) {
        fprintf(stderr, "Error: Failed to parse expression: %s\n", input_expr);
        goto cleanup;
    }
    
    
    symbol_set(var, variable);
    
    
    if (basic_(result, expr, var) != SYMENGINE_NO_EXCEPTION) {
        fprintf(stderr, "Error: Failed to integrate expression\n");
        goto cleanup;
    }
    
    
    char *temp_str = basic_str(result);
    if (!temp_str) {
        fprintf(stderr, "Error: Failed to convert result to string\n");
        goto cleanup;
    }
    
    
    size_t len = strlen(temp_str);
    result_str = (char *)malloc(len + 1);
    if (result_str) {
        strcpy(result_str, temp_str);
    }
    
    
    basic_str_free(temp_str);
    
cleanup:
    
    basic_free_stack(expr);
    basic_free_stack(var);
    basic_free_stack(result);
    
    return result_str; */
    return "TODO";
}

double klc_evaluate_function(const char *expr_str, double x) {

    basic expr, x_var, x_val, result;

    basic_new_stack(expr);
    basic_new_stack(x_var);
    basic_new_stack(x_val);
    basic_new_stack(result);

    if (basic_parse(expr, expr_str) != SYMENGINE_NO_EXCEPTION) {
        fprintf(stderr, "Error parsing expression: %s\n", expr_str);
        basic_free_stack(expr);
        basic_free_stack(x_var);
        basic_free_stack(x_val);
        basic_free_stack(result);
        return 0.0;
    }

    symbol_set(x_var, "x");

    real_double_set_d(x_val, x);

    basic_subs2(result, expr, x_var, x_val);

    double output = real_double_get_d(result);

    basic_free_stack(expr);
    basic_free_stack(x_var);
    basic_free_stack(x_val);
    basic_free_stack(result);

    return output;
}

char *klc_subtract_expressions(const char *a,
    const char *b) {
    basic_struct *expr_a, *expr_b, *result;

    expr_a = basic_new_heap();
    expr_b = basic_new_heap();
    result = basic_new_heap();

    if (basic_parse(expr_a, a) != 0) {
        fprintf(stderr, "Error parsing expression a: %s\n", a);
        basic_free_heap(expr_a);
        basic_free_heap(expr_b);
        basic_free_heap(result);
        return NULL;
    }

    if (basic_parse(expr_b, b) != 0) {
        fprintf(stderr, "Error parsing expression b: %s\n", b);
        basic_free_heap(expr_a);
        basic_free_heap(expr_b);
        basic_free_heap(result);
        return NULL;
    }

    basic_sub(result, expr_a, expr_b);

    basic_struct *simplified = basic_new_heap();
    basic_expand(simplified, result);
    basic_free_heap(result);
    result = simplified;

    char *result_str = basic_str(result);

    char *return_str = malloc(strlen(result_str) + 1);
    strcpy(return_str, result_str);

    basic_str_free(result_str);
    basic_free_heap(expr_a);
    basic_free_heap(expr_b);
    basic_free_heap(result);

    return return_str;
}

double integrate_simpson(const char *func, double a, double b, int n) {

    if (n % 2 != 0) n++;

    double h = (b - a) / n;
    double sum = klc_evaluate_function(func, a) + klc_evaluate_function(func, b);

    for (int i = 1; i < n; i++) {
        double x = a + i *h;
        if (i % 2 == 1) {
            sum += 4 *klc_evaluate_function(func, x);
        } else {
            sum += 2 *klc_evaluate_function(func, x);
        }
    }

    return sum *h / 3.0;
}

double bisection_solve(const char *expr, double a, double b, int *success) {
    double fa = klc_evaluate_function(expr, a);
    double fb = klc_evaluate_function(expr, b);

    if (fa *fb > 0) {
        *success = 0;
        return 0.0;
    }

    double c, fc;
    int iterations = 0;

    while (fabs(b - a) > TOLERANCE && iterations < MAX_ITERATIONS) {
        c = (a + b) / 2.0;
        fc = klc_evaluate_function(expr, c);

        if (fabs(fc) < TOLERANCE) {
            *success = 1;
            return c;
        }

        if (fa *fc < 0) {
            b = c;
            fb = fc;
        } else {
            a = c;
            fa = fc;
        }

        iterations++;
    }

    *success = 1;
    return (a + b) / 2.0;
}

int is_duplicate_root(double *roots, int count, double new_root) {
    for (int i = 0; i < count; i++) {
        if (fabs(roots[i] - new_root) < TOLERANCE *10) {
            return 1;
        }
    }
    return 0;
}

double klc_definite_integral(const char *input_expr, double a, double b) {
    return integrate_simpson(input_expr, a, b, 1000);
}

char *klc_polynomial_division(const char *dividend_expr,
    const char *divisor_expr) {
    basic_struct *expr_a, *expr_b, *result;

    expr_a = basic_new_heap();
    expr_b = basic_new_heap();
    result = basic_new_heap();

    if (basic_parse(expr_a, dividend_expr) != 0) {
        fprintf(stderr, "Error parsing expression a: %s\n", dividend_expr);
        basic_free_heap(expr_a);
        basic_free_heap(expr_b);
        basic_free_heap(result);
        return NULL;
    }

    if (basic_parse(expr_b, divisor_expr) != 0) {
        fprintf(stderr, "Error parsing expression b: %s\n", divisor_expr);
        basic_free_heap(expr_a);
        basic_free_heap(expr_b);
        basic_free_heap(result);
        return NULL;
    }

    basic_div(result, expr_a, expr_b);

    basic_struct *simplified = basic_new_heap();
    basic_expand(simplified, result);
    basic_free_heap(result);
    result = simplified;

    char *result_str = basic_str(result);

    char *return_str = malloc(strlen(result_str) + 1);
    strcpy(return_str, result_str);

    basic_str_free(result_str);
    basic_free_heap(expr_a);
    basic_free_heap(expr_b);
    basic_free_heap(result);

    return return_str;
}

double klc_limit(const char *input_expr, double limit_point) {
    return klc_evaluate_function(input_expr, limit_point);
}

void split_equation(const char *equation, char **left, char **right) {
    const char *eq_pos = strchr(equation, '=');
    if (eq_pos) {
        size_t left_len = eq_pos - equation;
        *left = (char *) malloc(left_len + 1);
        strncpy( *left, equation, left_len);
        ( *left)[left_len] = '\0';

        *right = strdup(eq_pos + 1);
    } else {
        *left = strdup(equation);
        *right = NULL;
    }
}

double snap_to_integer(double x) {
    double r = round(x);
    if (fabs(x - r) < INT_TOLERANCE) {
        return r;
    }
    return x;
}

double *klc_solve_equation(const char *input_expr) {
    if (!input_expr) return NULL;

    char *left = NULL;
    char *right = NULL;
    split_equation(input_expr, &left, &right);
    if (right)
        input_expr = klc_subtract_expressions(left, right);

    double *roots = malloc((MAX_ROOTS + 1) *sizeof *roots);
    if (!roots) return NULL;

    int root_count = 0;
    double x = SEARCH_MIN;

    while (x < SEARCH_MAX && root_count < MAX_ROOTS) {
        double x2 = x + SEARCH_STEP;
        double f1 = klc_evaluate_function(input_expr, x);
        double f2 = klc_evaluate_function(input_expr, x2);

        if (isnan(f1) || isinf(f1) ||
            isnan(f2) || isinf(f2)) {
            x = x2;
            continue;
        }

        /*bracket if sign-change or endpoint zero */
        if (f1 *f2 <= 0.0) {
            /*if f1 ≈ 0, snap x */
            if (fabs(f1) < TOLERANCE) {
                double r = snap_to_integer(x);
                if (!is_duplicate_root(roots, root_count, r)) {
                    roots[root_count++] = r;
                }
                x = x2;
                continue;
            }
            /*if f2 ≈ 0, snap x2 */
            if (fabs(f2) < TOLERANCE) {
                double r = snap_to_integer(x2);
                if (!is_duplicate_root(roots, root_count, r)) {
                    roots[root_count++] = r;
                }
                x = x2;
                continue;
            }

            /*true sign-change → bisect */
            int success = 0;
            double root = bisection_solve(input_expr, x, x2, &success);
            if (success) {
                root = snap_to_integer(root);
                if (!is_duplicate_root(roots, root_count, root)) {
                    roots[root_count++] = root;
                }
            }
        }

        x = x2;
    }

    roots[root_count] = NAN; /*sentinel */
    return roots;
}
