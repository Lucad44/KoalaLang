#include "memory.h"
#include "ast.h"
#include "variables.h"
#include "hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* safe_calloc(size_t num, size_t size) {
    void *ptr = calloc(num, size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* safe_realloc(void *ptr, size_t new_size) {
    void *new_ptr = realloc(ptr, new_size);
    if (!new_ptr && new_size > 0) {
        fprintf(stderr, "Memory reallocation failed\n");
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

void free_ast(void *node) {
    ASTNode *ast_node = (ASTNode*)node;
    if (!ast_node) return;

    switch (ast_node->type) {
        case NODE_VAR_DECL:
            free(ast_node->data.var_decl.name);
            free_ast(ast_node->data.var_decl.init_expr);
            break;
        case NODE_PRINT:
            for (int i = 0; i < ast_node->data.print.expr_count; i++) {
                free_ast(ast_node->data.print.expr_list[i]);
            }
            free(ast_node->data.print.expr_list);
            break;
        case NODE_BLOCK:
            for (int i = 0; i < ast_node->data.block.stmt_count; i++) {
                free_ast(ast_node->data.block.statements[i]);
            }
            free(ast_node->data.block.statements);
            break;
        case NODE_EXPR_LITERAL:
            if (ast_node->data.str_literal.str_val) {
                free(ast_node->data.str_literal.str_val);
            }
            break;
        case NODE_EXPR_VARIABLE:
            free(ast_node->data.variable.name);
            break;
        default:
            break;
    }
    free(ast_node);
}

void free_variable_map(void) {
    hashmap_free(variable_map);
}
