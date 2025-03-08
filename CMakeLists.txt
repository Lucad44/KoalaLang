cmake_minimum_required(VERSION 3.29)
project(KoalaLang C)

set(CMAKE_C_STANDARD 23)

add_executable(KoalaLang src/main.c
        src/variables.c
        src/variables.h
        src/lexer.c
        src/lexer.h
        src/ast.h
        src/parser.c
        src/parser.h
        src/interpreter.c
        src/interpreter.h
        src/memory.c
        src/memory.h
        src/tinyexpr/tinyexpr.c
        src/tinyexpr/tinyexpr.h
        src/hashmap.c
        src/hashmap.h
        src/hashmap.c)
