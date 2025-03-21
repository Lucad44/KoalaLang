#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter.h"

#include <math.h>

#include "hashmap.h"
#include "variables.h"
#include "memory.h"

static void execute_block(const BlockNode *block) {
    for (int i = 0; i < block->stmt_count; i++) {
        execute(block->statements[i]);
    }
}

static double evaluate_expression(const ASTNode *node) {
    switch (node->type) {
        case NODE_EXPR_LITERAL:
            return node->data.num_literal.num_val;
        case NODE_EXPR_VARIABLE: {
            const Variable *variable = hashmap_get(variable_map, &(Variable) {
                .name = node->data.variable.name
            });
            if (variable && variable->type == VAR_NUM) {
                return variable->value.num_val;
            }
            return 0;
        }
        case NODE_EXPR_BINARY: {
            const double left = evaluate_expression(node->data.binary_expr.left);
            const double right = evaluate_expression(node->data.binary_expr.right);

            switch (node->data.binary_expr.op) {
                case OP_PLUS: return left + right;
                case OP_MINUS: return left - right;
                case OP_MULTIPLY: return left * right;
                case OP_DIVIDE: 
                    if (right == 0) {
                        fprintf(stderr, "Error: Division by zero\n");
                        exit(EXIT_FAILURE);
                    }
                    return left / right;
                case OP_MODULO:
                    if (right == 0) {
                        fprintf(stderr, "Error: Modulo by zero\n");
                        exit(EXIT_FAILURE);
                    }
                    return (double)((long long)left % (long long)right);
                case OP_POWER:
                    if (left == 0 && right == 0) {
                        fprintf(stderr, "Error: 0 to the power of 0 is undefined\n");
                        exit(EXIT_FAILURE);
                    }
                    return pow(left, right);
                case OP_AND:
                    return (long long) left & (long long) right;
                case OP_OR:
                    return (long long) left | (long long) right;
                case OP_XOR:
                    return (long long) left ^ (long long) right;
                case OP_LESS: return left < right;
                case OP_GREATER: return left > right;
                case OP_EQUAL: return left == right;
                case OP_NOT_EQUAL: return left != right;
                case OP_LESS_EQUAL: return left <= right;
                case OP_GREATER_EQUAL: return left >= right;
                default: return 0;
            }
        }
        default:
            return 0;
    }
}

static void execute_var_decl(const VarDeclNode *node) {
    if (node->init_expr->type == NODE_EXPR_LITERAL) {
        if (node->type == VAR_NUM || node->type == -1) {
            const double num_val = node->init_expr->data.num_literal.num_val;
            hashmap_set(variable_map, &(Variable) {
                .name = node->name,
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
    } else if (node->init_expr->type == NODE_EXPR_BINARY || node->init_expr->type == NODE_EXPR_VARIABLE) {
        if (node->type == VAR_NUM || node->type == -1) {
            const double num_val = evaluate_expression(node->init_expr);
            hashmap_set(variable_map, &(Variable) {
                .name = node->name,
                .type = VAR_NUM,
                .value = { .num_val = num_val }
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
                }
                snprintf(buffer, sizeof(buffer), "%g", variable->value.num_val);
                return strdup(buffer);
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
        const size_t new_len = strlen(result) + strlen(part) + 1;
        char *new_result = safe_malloc(new_len);

        strncpy(new_result, result, new_len);
        new_result[new_len - 1] = '\0';
        strncat(new_result, part, new_len - strlen(new_result) - 1);

        free(result);
        free(part);
        result = new_result;\
    }

    printf("%s\n", result);
    free(result);
}

void execute(const ASTNode *node) {
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
        case NODE_EXPR_POSTFIX:
            execute_postfix(&node->data.postfix_expr);
            break;
        case NODE_IF:
            execute_if(&node->data.if_stmt);
            break;
        case NODE_WHILE:
            execute_while(&node->data.while_stmt);
            break;
        default:
            fprintf(stderr, "Unknown node type: %d\n", node->type);
            break;
    }
}

int evaluate_condition(ASTNode *condition) {
    if (!condition) return 0;

    switch (condition->type) {
        case NODE_EXPR_LITERAL:
            return condition->data.num_literal.num_val != 0;

        case NODE_EXPR_VARIABLE: {
            const Variable *variable = hashmap_get(variable_map, &(Variable) {
                .name = condition->data.variable.name
            });
            if (variable && variable->type == VAR_NUM) {
                return variable->value.num_val != 0;
            }
            return 0;
        }

        case NODE_EXPR_BINARY: {
            double left = 0, right = 0;

            if (condition->data.binary_expr.left->type == NODE_EXPR_LITERAL) {
                left = condition->data.binary_expr.left->data.num_literal.num_val;
            } else if (condition->data.binary_expr.left->type == NODE_EXPR_VARIABLE) {
                const Variable *variable = hashmap_get(variable_map, &(Variable) {
                    .name = condition->data.binary_expr.left->data.variable.name
                });
                if (variable && variable->type == VAR_NUM) left = variable->value.num_val;
            }

            if (condition->data.binary_expr.right->type == NODE_EXPR_LITERAL) {
                right = condition->data.binary_expr.right->data.num_literal.num_val;
            } else if (condition->data.binary_expr.right->type == NODE_EXPR_VARIABLE) {
                const Variable *variable = hashmap_get(variable_map, &(Variable) {
                    .name = condition->data.binary_expr.right->data.variable.name
                });
                if (variable && variable->type == VAR_NUM) right = variable->value.num_val;
            }

            switch (condition->data.binary_expr.op) {
                case OP_LESS: return left < right;
                case OP_GREATER: return left > right;
                case OP_EQUAL: return left == right;
                case OP_NOT_EQUAL: return left != right;
                case OP_LESS_EQUAL: return left <= right;
                case OP_GREATER_EQUAL: return left >= right;
                default: ;
            }
            break;
        }
        default: ;
    }
    return 0;
}

void execute_postfix(const PostfixExprNode *node) {
    Variable *variable = (Variable *) hashmap_get(variable_map, &(Variable) {
        .name = node->var_name
    });

    if (!variable) {
        fprintf(stderr, "Undefined variable: %s\n", node->var_name);
        return;
    }

    if (variable->type != VAR_NUM) {
        fprintf(stderr, "Cannot increment/decrement non-numeric variable\n");
        return;
    }

    switch (node->op) {
        case OP_INC:
            variable->value.num_val++;
        break;
        case OP_DEC:
            variable->value.num_val--;
        break;
    }
}

void execute_if(const IfNode *if_node) {
    if (evaluate_condition(if_node->condition)) {
        execute(if_node->body);
        return;
    }

    for (int i = 0; i < if_node->elif_count; i++) {
        const ASTNode *elif_node = if_node->elif_nodes[i];
        if (evaluate_condition(elif_node->data.if_stmt.condition)) {
            execute(elif_node->data.if_stmt.body);
            return;
        }
    }

    if (if_node->else_body) {
        execute(if_node->else_body);
    }
}

void execute_while(const WhileNode *while_node) {
    while (evaluate_condition(while_node->condition)) {
        execute(while_node->body);
    }
}
