#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "memory.h"
#include "ast.h"
#include "variables.h"
#include "hashmap.h"

void* safe_malloc(const size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* safe_calloc(const size_t num, const size_t size) {
    void *ptr = calloc(num, size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* safe_realloc(void *ptr, const size_t new_size) {
    void *new_ptr = realloc(ptr, new_size);
    if (!new_ptr && new_size > 0) {
        fprintf(stderr, "Memory reallocation failed\n");
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

void free_ast(void *node) {
    ASTNode *ast_node = node;
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
        case NODE_LIST_ACCESS:
            free(ast_node->data.list_access.list_name);
            free_ast(ast_node->data.list_access.index_expr); // Recursively free the index expression
            break;
        case NODE_EXPR_UNARY:
            free_ast(ast_node->data.unary_expr.operand);
            break;
        case NODE_ASSIGNMENT:
            free(ast_node->data.assignment.target_name);
            // Free index expression only if it exists (list assignment)
            if (ast_node->data.assignment.index_expr != NULL) {
                free_ast(ast_node->data.assignment.index_expr);
            }
            // Free the value expression
            free_ast(ast_node->data.assignment.value_expr);
            break;
        default:
            break;
    }
    free(ast_node);
}
