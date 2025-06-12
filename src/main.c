#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "variables.h"
#include "functions.h"
#include "memory.h"
#include "hashmap.h"


void print_token_stream(const char *source) {
    Lexer debug_lexer;
    init_lexer( &debug_lexer, source);
    printf("\n=== Token Stream Debug ===\n");
    Token t;
    do {
        t = next_token( &debug_lexer);
        printf("Token type: %2d | Lexeme: %-10s | Num: %-5g | Str: %s\n",
            t.type,
            t.lexeme ? t.lexeme : "NULL",
            t.num_value,
            t.str_value ? t.str_value : "NULL");
    } while (t.type != TOKEN_EOF);
    printf("=========================\n\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.kl>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("File open failed");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    const long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *source = safe_malloc(length + 1);
    fread(source, 1, length, file);
    source[length] = '\0';
    fclose(file);

    // Debug tokenization
    print_token_stream(source);

    Lexer lexer;
    init_lexer( &lexer, source);

    Parser parser;
    init_parser( &parser, &lexer);
    ASTNode *program = parse_program( &parser);

    ReturnContext ret_ctx = {
        .is_return = 0,
        .ret_val.type = RET_NONE
    };
    execute(program, variable_map, &ret_ctx);

    printf("\n-- iterate over all variables (hashmap_scan) --\n");
    hashmap_scan(variable_map, variable_iter, NULL);

    printf("\n-- iterate over all functions (hashmap_scan) --\n");
    hashmap_scan(function_map, function_iter, NULL);

    // Cleanup
    free_ast(program);
    free(source);
    hashmap_free(variable_map);

    return 0;
}
