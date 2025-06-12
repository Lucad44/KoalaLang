# KLC Language Documentation

This documentation covers the syntax and usage of the KLC programming language, a dynamically-typed language designed for mathematical computations and general-purpose scripting.

## Table of Contents
1. [Data Types](#data-types)
2. [Variable Declaration](#variable-declaration)
3. [Lists](#lists)
4. [Control Structures](#control-structures)
5. [Functions](#functions)
6. [Modules](#modules)
7. [Operators](#operators)
8. [Examples](#examples)

---

## Data Types
KLC supports three fundamental data types:

1. **Numbers**: Double-precision floating point values  
   `num age = 25;`

2. **Strings**: Text enclosed in double quotes  
   `str name = "Alice";`

3. **Lists**: Ordered collections that can contain numbers or strings  
   `list numbers = [1, 2, 3];`

---

## Variable Declaration
Variables must be declared with explicit types:

```js
// Explicit type declaration
num count = 10;
str greeting = "Hello";
list matrix = [[1,2], [3,4]];

// Type inference with 'var'
var inferred_num = 3.14;      // Becomes num
var inferred_str = "world";   // Becomes str
var inferred_list = [1,2,3];  // Becomes list
```

---

## Lists
Lists can be simple or nested:

```js
// Simple list of numbers
list[num] primes = [2, 3, 5, 7, 11];

// Simple list of strings
list[str] colors = ["red", "green", "blue"];

// Nested list (list of lists)
list[list[num]] matrix = [[1,2], [3,4], [5,6]];

// Accessing elements
num first_prime = primes[0];          // 2
str primary_color = colors[0];        // "red"
num matrix_value = matrix[1][0];      // 3

// Modifying elements
primes[0] = 13;
colors[1] = "yellow";
matrix[0][1] = 99;
```

---

## Control Structures
### If-Elif-Else
```js
if (x > 10) {
    print("x is greater than 10");
} elif (x > 5) {
    print("x is greater than 5");
} else {
    print("x is 5 or less");
}
```

### While Loop
```js
num i = 0;
while (i < 5) {
    print(i);
    i++;
}
```

---

## Functions
### Function Declaration
```js
fun add(num a, num b) {
    return a + b;
}

fun greet(str name) {
    return "Hello, " $ name;
}
```

### Function Calls
```js
num result = call add(5, 3);          // 8
str message = call greet("Alice");    // "Hello, Alice"
```

### Return Statements
```js
fun is_even(num n) {
    if (n % 2 == 0) {
        return true;
    }
    return false;
}
```

---

## Modules
### Importing Modules
```js
import math;
import trig;
```

### Using Math Module
```js
num pi = math.pi;
num root = math.sqrt(25);            // 5
num log_val = math.log(100, 10);     // 2

// Plotting
math.plot_function("x^2");
```

### Using Trig Module
```js
num angle = trig.pi / 4;
num sine = trig.sin(angle);          // ≈0.707
num cosine = trig.cos(angle);         // ≈0.707
```

---

## Operators
### Arithmetic Operators
```js
num a = 10 + 5;   // Addition
num b = 10 - 5;   // Subtraction
num c = 10 * 5;   // Multiplication
num d = 10 / 5;   // Division
num e = 10 % 3;   // Modulo
num f = 2 ** 3;   // Exponentiation (8)
```

### Comparison Operators
```js
bool x = (5 == 5);    // Equal
bool y = (5 != 3);    // Not equal
bool z = (5 > 3);     // Greater than
bool w = (5 <= 5);    // Less than or equal
```

### Logical Operators
```js
bool a = true && false;   // AND (false)
bool b = true || false;   // OR (true)
bool c = true ^^ false;   // XOR (true)
bool d = !true;           // NOT (false)
```

### String Concatenation
```js
str greeting = "Hello" $ " " $ "World!";  // "Hello World!"
```

### Increment/Decrement
```js
num counter = 5;
counter++;   // 6
counter--;   // 5
```

---

## Examples
### Factorial Calculation
```js
fun factorial(num n) {
    if (n <= 1) {
        return 1;
    }
    return n * call factorial(n-1);
}

num result = call factorial(5);  // 120
```

### List Operations
```js
list[num] squares = [];
num i = 0;
while (i < 5) {
    squares[i] = i * i;
    i++;
}
// squares = [0, 1, 4, 9, 16]
```

### Using Math Functions
```js
import math;

num a = math.definite_integral("x^2", 0, 1);  // ≈0.333
num b = math.fibonacci(10);                   // 55
str derivative = math.differentiate("x^3 + 2x", "x"); // "3*x^2 + 2"
```

### Plotting Functions
```js
import math;

// Plot single function
math.plot_function("sin(x)");

// Plot multiple functions
list[str] funcs = ["sin(x)", "cos(x)", "x^2"];
math.plot_multiple_functions(funcs);
```