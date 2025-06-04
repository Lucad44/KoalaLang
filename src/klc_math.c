#include "klc_math.h"

#include <symengine/cwrapper.h>
#include <symengine/symengine_exception.h>

#define EPSILON 0.00000000000000001
#define PI 3.1415926535897932384626433
#define E 2.718281828459045235360287471

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
        guess = (2.0 * guess + n / (guess * guess)) / 3.0;
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

double klc_log(const double n, const double base) {
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

void klc_plot_multiple_functions(const char *input_exprs[], const int count) {
    FILE *gp = popen("gnuplot -persistent", "w");
    if (gp == NULL) {
        fprintf(stderr, "Error: gnuplot not found.\n");
        return;
    }

    fprintf(gp, "set title 'Multiple Function Plot'\n");
    fprintf(gp, "set xlabel 'X-axis'\n");
    fprintf(gp, "set ylabel 'Y-axis'\n");

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

char *klc_simplify_expression(const char *input_expr) {
    if (!input_expr) {
        return NULL;
    }
    
    // Create basic objects
    basic expr, expanded;
    basic_new_stack(expr);
    basic_new_stack(expanded);
    
    char *result_str = NULL;
    
    // Parse the input expression
    if (basic_parse(expr, input_expr) != SYMENGINE_NO_EXCEPTION) {
        fprintf(stderr, "Error: Failed to parse expression: %s\n", input_expr);
        goto cleanup;
    }
    
    // Expand the expression using basic_expand
    basic_expand(expanded, expr);
    
    // Convert the expanded expression back to string
    char *temp_str = basic_str(expanded);
    if (!temp_str) {
        fprintf(stderr, "Error: Failed to convert expanded expression to string\n");
        goto cleanup;
    }
    
    // Allocate memory for the result and copy the string
    size_t len = strlen(temp_str);
    result_str = (char*)malloc(len + 1);
    if (result_str) {
        strcpy(result_str, temp_str);
    }
    
    // Free the temporary string
    basic_str_free(temp_str);
    
cleanup:
    // Free the basic objects
    basic_free_stack(expr);
    basic_free_stack(expanded);
    
    return result_str;
}

char *klc_differentiate(const char *input_expr, const char *variable) {
    if (!input_expr || !variable) {
        return NULL;
    }
    
    // Create basic objects
    basic expr, var, result;
    basic_new_stack(expr);
    basic_new_stack(var);
    basic_new_stack(result);
    
    char *result_str = NULL;
    
    // Parse the input expression
    if (basic_parse(expr, input_expr) != SYMENGINE_NO_EXCEPTION) {
        fprintf(stderr, "Error: Failed to parse expression: %s\n", input_expr);
        goto cleanup;
    }
    
    // Create the variable symbol
    symbol_set(var, variable);
    
    // Calculate the derivative (this function exists in SymEngine C wrapper)
    basic_diff(result, expr, var);
    
    // Convert the result back to string
    char *temp_str = basic_str(result);
    if (!temp_str) {
        fprintf(stderr, "Error: Failed to convert result to string\n");
        goto cleanup;
    }
    
    // Allocate memory for the result and copy the string
    size_t len = strlen(temp_str);
    result_str = (char*)malloc(len + 1);
    if (result_str) {
        strcpy(result_str, temp_str);
    }
    
    // Free the temporary string
    basic_str_free(temp_str);
    
cleanup:
    // Free the basic objects
    basic_free_stack(expr);
    basic_free_stack(var);
    basic_free_stack(result);
    
    return result_str;
}

char *klc_integrate(const char *input_expr, const char *variable) {
   /* if (!input_expr || !variable) {
        return NULL;
    }
    
    // Create basic objects
    basic expr, var, result;
    basic_new_stack(expr);
    basic_new_stack(var);
    basic_new_stack(result);
    
    char *result_str = NULL;
    
    // Parse the input expression
    if (basic_parse(expr, input_expr) != SYMENGINE_NO_EXCEPTION) {
        fprintf(stderr, "Error: Failed to parse expression: %s\n", input_expr);
        goto cleanup;
    }
    
    // Create the variable symbol
    symbol_set(var, variable);
    
    // Calculate the indefinite integral
    if (basic_(result, expr, var) != SYMENGINE_NO_EXCEPTION) {
        fprintf(stderr, "Error: Failed to integrate expression\n");
        goto cleanup;
    }
    
    // Convert the result back to string
    char *temp_str = basic_str(result);
    if (!temp_str) {
        fprintf(stderr, "Error: Failed to convert result to string\n");
        goto cleanup;
    }
    
    // Allocate memory for the result and copy the string
    size_t len = strlen(temp_str);
    result_str = (char*)malloc(len + 1);
    if (result_str) {
        strcpy(result_str, temp_str);
    }
    
    // Free the temporary string
    basic_str_free(temp_str);
    
cleanup:
    // Free the basic objects
    basic_free_stack(expr);
    basic_free_stack(var);
    basic_free_stack(result);
    
    return result_str;*/
    return "TODO";
}

double evaluate_function(const char* expr_str, double x) {
    // Initialize SymEngine basic variables
    basic expr, x_var, x_val, result;
    
    // Initialize the basic variables
    basic_new_stack(expr);
    basic_new_stack(x_var);
    basic_new_stack(x_val);
    basic_new_stack(result);
    
    // Parse the expression string
    if (basic_parse(expr, expr_str) != SYMENGINE_NO_EXCEPTION) {
        fprintf(stderr, "Error parsing expression: %s\n", expr_str);
        basic_free_stack(expr);
        basic_free_stack(x_var);
        basic_free_stack(x_val);
        basic_free_stack(result);
        return 0.0;
    }
    
    // Create symbol 'x'
    symbol_set(x_var, "x");
    
    // Create the numeric value for substitution
    real_double_set_d(x_val, x);
    
    // Substitute x with the given value using basic_subs2
    basic_subs2(result, expr, x_var, x_val);
    
    // Convert result to double
    double output = real_double_get_d(result);
    
    // Clean up
    basic_free_stack(expr);
    basic_free_stack(x_var);
    basic_free_stack(x_val);
    basic_free_stack(result);
    
    return output;
}

double integrate_simpson(const char *func, double a, double b, int n) {
    // n must be even for Simpson's rule
    if (n % 2 != 0) n++;
    
    double h = (b - a) / n;
    double sum = evaluate_function(func, a) + evaluate_function(func, b);
    
    // Add terms with alternating coefficients 4 and 2
    for (int i = 1; i < n; i++) {
        double x = a + i * h;
        if (i % 2 == 1) {
            sum += 4 * evaluate_function(func, x);  // Odd indices get coefficient 4
        } else {
            sum += 2 * evaluate_function(func, x);  // Even indices get coefficient 2
        }
    }
    
    return sum * h / 3.0;
}

double klc_definite_integral(const char *input_expr, double a, double b) {
    return integrate_simpson(input_expr, a, b, 1000);
}

char *klc_polynomial_division(const char *dividend_expr, const char *divisor_expr) {
  /* // Initialize SymEngine symbols and basic objects
    basic dividend, divisor, x, quotient, remainder;
    basic_new_stack(dividend);
    basic_new_stack(divisor);
    basic_new_stack(x);
    basic_new_stack(quotient);
    basic_new_stack(remainder);
    
    // Create symbol 'x'
    symbol_set(x, "x");
    
    // Parse dividend expression
    if (basic_parse(dividend, dividend_expr) != 0) {
        return NULL;
    }
    
    // Parse divisor expression
    if (basic_parse(divisor, divisor_expr) != 0) {
        basic_free_stack(dividend);
        return NULL;
    }

    
    
    // Perform polynomial division
    int result = symengine_pdiv(dividend, divisor, x, quotient, remainder);
    if (result != 0) {
        basic_free_stack(dividend);
        basic_free_stack(divisor);
        basic_free_stack(x);
        basic_free_stack(quotient);
        basic_free_stack(remainder);
        return NULL;
    }
    
    // Convert quotient and remainder to strings
    char *q_str = basic_str(quotient);
    char *r_str = basic_str(remainder);
    
    // Format result as "(quotient)(remainder)"
    size_t len = strlen(q_str) + strlen(r_str) + 3; // +3 for "()" and null terminator
    char *output = malloc(len);
    if (output == NULL) {
        free(q_str);
        free(r_str);
        basic_free_stack(dividend);
        basic_free_stack(divisor);
        basic_free_stack(x);
        basic_free_stack(quotient);
        basic_free_stack(remainder);
        return NULL;
    }
    snprintf(output, len, "(%s)(%s)", q_str, r_str);
    
    // Clean up
    free(q_str);
    free(r_str);
    basic_free_stack(dividend);
    basic_free_stack(divisor);
    basic_free_stack(x);
    basic_free_stack(quotient);
    basic_free_stack(remainder);
    
    return output;*/
    return "tidi";
}
