#include "interpreter.h"
#include "hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void execute_block(const BlockNode *block) {
    for (int i = 0; i < block->stmt_count; i++) {
        execute(block->statements[i]);
    }
}

static void execute_var_decl(const VarDeclNode *node) {
    if (node->init_expr->type == NODE_EXPR_LITERAL) {
        if (node->type == VAR_NUM || node->type == -1) {
            const double num_val = node->init_expr->data.num_literal.num_val;
            hashmap_set(variable_map, &(Variable) {
                .name = node->name,
                .type = VAR_NUM,
                .type = VAR_NUM,
                .value = { .num_val = num_val }
            });
        } else if (node->type == VAR_STR) {
            char *str_val = node->init_expr->data.str_literal.str_val;
            hashmap_set(variable_map, &(Variable) {
                .name = node->name,
                .type = VAR_STR,
                .value = { .str_val = str_val }
            });
        }
    }
}

static char* get_string_value(const ASTNode *node) {
    char buffer[1024];

    switch (node->type) {
        case NODE_EXPR_LITERAL: {
            if (node->data.str_literal.str_val) {
                return strdup(node->data.str_literal.str_val);
            }
            snprintf(buffer, sizeof(buffer), "%g", node->data.num_literal.num_val);
            return strdup(buffer);
        }
        case NODE_EXPR_VARIABLE: {
            const Variable *variable = hashmap_get(variable_map, &(Variable) {
                .name = node->data.variable.name
            });
            if (variable) {
                if (variable->type == VAR_STR) {
                    return strdup(variable->value.str_val);
                } else {
                    snprintf(buffer, sizeof(buffer), "%g", variable->value.num_val);
                    return strdup(buffer);
                }
            }
            return strdup("<undefined>");
        }
        default:
            return strdup("<unknown>");
    }
}

static void execute_print(const PrintNode *node) {
    char *result = strdup("");

    for (int i = 0; i < node->expr_count; i++) {
        char *part = get_string_value(node->expr_list[i]);
        char *new_result = malloc(strlen(result) + strlen(part) + 1);
        strcpy(new_result, result);
        strcat(new_result, part);
        free(result);
        free(part);
        result = new_result;
    }

    printf("%s\n", result);
    free(result);
}

void execute(ASTNode *node) {
    switch (node->type) {
        case NODE_VAR_DECL:
            execute_var_decl(&node->data.var_decl);
            break;
        case NODE_PRINT:
            execute_print(&node->data.print);
            break;
        case NODE_BLOCK:
            execute_block(&node->data.block);
            break;
        case NODE_IF:
            execute_if(&node->data.if_stmt);
            break;
        default:
            fprintf(stderr, "Unknown node type: %d\n", node->type);
            break;
    }
}

int evaluate_condition(ASTNode *node) {
    if (!node) return 0;

    switch (node->type) {
        case NODE_EXPR_LITERAL:
            return node->data.num_literal.num_val != 0;

        case NODE_EXPR_VARIABLE: {
            const Variable *variable = hashmap_get(variable_map, &(Variable) {
                .name = node->data.variable.name
            });
            if (variable && variable->type == VAR_NUM) {
                return variable->value.num_val != 0;
            }
            return 0;
        }

        case NODE_EXPR_BINARY: {
            double left = 0, right = 0;

            if (node->data.binary_expr.left->type == NODE_EXPR_LITERAL) {
                left = node->data.binary_expr.left->data.num_literal.num_val;
            } else if (node->data.binary_expr.left->type == NODE_EXPR_VARIABLE) {
                const Variable *variable = hashmap_get(variable_map, &(Variable) {
                    .name = node->data.binary_expr.left->data.variable.name
                });
                if (variable && variable->type == VAR_NUM) left = variable->value.num_val;
            }

            if (node->data.binary_expr.right->type == NODE_EXPR_LITERAL) {
                right = node->data.binary_expr.right->data.num_literal.num_val;
            } else if (node->data.binary_expr.right->type == NODE_EXPR_VARIABLE) {
                const Variable *variable = hashmap_get(variable_map, &(Variable) {
                    .name = node->data.binary_expr.right->data.variable.name
                });
                if (variable && variable->type == VAR_NUM) right = variable->value.num_val;
            }

            switch (node->data.binary_expr.op) {
                case OP_LESS: return left < right;
                case OP_GREATER: return left > right;
                case OP_EQUAL: return left == right;
            }
            break;
        }
        default: ;
    }
    return 0;
}

void execute_if(IfNode *if_node) {
    if (evaluate_condition(if_node->condition)) {
        execute(if_node->body);
    }
}

