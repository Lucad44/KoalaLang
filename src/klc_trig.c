#include "klc_trig.h"

#define PI 3.1415926535897932384626433

double klc_degrees_to_radians(const double n) {
    return n * PI / 180;
}

double klc_radians_to_degrees(const double n) {
    return n * 180 / PI;
}

double klc_sin(const double n) {
    return sin(n);
}

double klc_cos(const double n) {
    return cos(n);
}

double klc_tan(const double n) {
    return tan(n);
}

double klc_cot(const double n) {
    return 1.0 / tan(n);
}

double klc_sec(const double n) {
    return 1.0 / cos(n);
}

double klc_csc(const double n) {
    return 1.0 / sin(n);
}

double klc_arcsin(const double n) {
    if (n < -1 || n > 1) {
        fprintf(stderr, "\nError: arcsin argument must be in the range [-1,1].\n");
        exit(EXIT_FAILURE);
    }
    return asin(n);
}

double klc_arccos(const double n) {
    if (n < -1 || n > 1) {
        fprintf(stderr, "\nError: arccos argument must be in the range [-1,1].\n");
        exit(EXIT_FAILURE);
    }
    return acos(n);
}

double klc_arctan(const double n) {
    return atan(n);
}

double klc_arccot(const double n) {
    return atan(1.0 / n);
}

double klc_arcsec(const double n) {
    if (n > -1 && n < 1) {
        fprintf(stderr, "\nError: arcsec argument must be in the range (-inf,-1]U[1,+inf).\n");
        exit(EXIT_FAILURE);
    }
    return acos(1.0 / n);
}

double klc_arccsc(const double n) {
    if (n > -1 && n < 1) {
        fprintf(stderr, "\nError: arccsc argument must be in the range (-inf,-1]U[1,+inf).\n");
        exit(EXIT_FAILURE);
    }
    return asin(1.0 / n);
}

double klc_sinh(const double n) {
    return sinh(n);
}

double klc_cosh(const double n) {
    return cosh(n);
}

double klc_tanh(const double n) {
    return tanh(n);
}

double klc_coth(const double n) {
    if (n == 0) {
        fprintf(stderr, "\nError: coth argument must be in the range (-inf,0)U(0,+inf).\n");
        exit(EXIT_FAILURE);
    }
    return 1.0 / tanh(n);
}

double klc_sech(const double n) {
    return 1.0 / cosh(n);
}

double klc_csch(const double n) {
    if (n == 0) {
        fprintf(stderr, "\nError: csch argument must be in the range (-inf,0)U(0,+inf).\n");
        exit(EXIT_FAILURE);
    }
    return 1.0 / sinh(n);
}

double klc_arcsinh(const double n) {
    return asinh(n);
}

double klc_arccosh(const double n) {
    if (n < 1) {
        fprintf(stderr, "\nError: arccosh argument must be in the range [1,+inf).\n");
        exit(EXIT_FAILURE);
    }
    return acosh(n);
}

double klc_arctanh(const double n) {
    if (n <= -1 || n >= 1) {
        fprintf(stderr, "\nError: arctanh argument must be in the range (-1,1).\n");
        exit(EXIT_FAILURE);
    }
    return atanh(n);
}

double klc_arccoth(const double n) {
    if (n >= -1 && n <= 1) {
        fprintf(stderr, "\nError: arccoth argument must be in the range (-inf,-1)U(1,+inf).\n");
        exit(EXIT_FAILURE);
    }
    return atanh(1.0 / n);
}

double klc_arcsech(const double n) {
    if (n <= 0 || n > 1) {
        fprintf(stderr, "\nError: arcsech argument must be in the range (0,1].\n");
        exit(EXIT_FAILURE);
    }
    return acosh(1.0 / n);
}

double klc_arccsch(const double n) {
    if (n > -1 && n < 1)  {
        fprintf(stderr, "\nError: arccsch argument must be in the range (-inf,-1]U[1,+inf).\n");
        exit(EXIT_FAILURE);
    }
    return asinh(1.0 / n);
}
