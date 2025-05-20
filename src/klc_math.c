#include "klc_math.h"

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

char *klc_integrate(const char *input_expr) {
   /* basic x, expr, result;
    basic_new_stack(x);
    basic_new_stack(expr);
    basic_new_stack(result);

    symbol_set(x, "x");
    if (basic_parse(expr, input_expr) != 0) {
        fprintf(stderr, "\nError: Failed to parse expression: %s\n", input_expr);
        basic_free_stack(x);
        basic_free_stack(expr);
        basic_free_stack(result);
        exit(EXIT_FAILURE);
    }
    basic_integrate(result, expr, x);
    char *result_str = basic_str(result);
    basic_free_stack(x);
    basic_free_stack(expr);
    basic_free_stack(result);
    return result_str;*/
    return "TODO";
}
