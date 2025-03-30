#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "interpreter.h"
#include "variables.h"
#include "functions.h"
#include "memory.h"

Variable *get_variable(struct hashmap *scope, char *name) {
    Variable *variable = (Variable *) hashmap_get(scope, &(Variable) {
        .name = name
    });

    if (!variable) {
        variable = (Variable *) hashmap_get(variable_map, &(Variable) {
            .name = name
        });
    }
    return variable == NULL ? NULL : variable;
}

static void execute_block(const BlockNode *block, struct hashmap *scope) {
    for (int i = 0; i < block->stmt_count; i++) {
        execute(block->statements[i], scope);

        const Variable *return_flag = get_variable(scope, "__FUNCTION_RETURN_FLAG");
        if (return_flag && return_flag->type == VAR_NUM && return_flag->value.num_val == 1) {
            break;
        }
    }
}

static double evaluate_expression(const ASTNode *node, struct hashmap *scope) {
    switch (node->type) {
        case NODE_EXPR_LITERAL:
            return node->data.num_literal.num_val;
        case NODE_EXPR_VARIABLE: {
            const Variable *variable = get_variable(scope, node->data.variable.name);
            if (variable && variable->type == VAR_NUM) {
                return variable->value.num_val;
            }
            return 0;
        }
        case NODE_EXPR_BINARY: {
            const double left = evaluate_expression(node->data.binary_expr.left, scope);
            const double right = evaluate_expression(node->data.binary_expr.right, scope);

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
                case OP_LESS:
                    return left < right;
                case OP_GREATER:
                    return left > right;
                case OP_EQUAL:
                    return left == right;
                case OP_NOT_EQUAL:
                    return left != right;
                case OP_LESS_EQUAL:
                    return left <= right;
                case OP_GREATER_EQUAL:
                    return left >= right;
                default:
                    return 0;
            }
        }
        case NODE_FUNC_CALL: {
            execute_func_call(&node->data.func_call, scope);
            
            const Variable *return_val = get_variable(variable_map, "__FUNCTION_NUM_RET_VAL");
            if (return_val && return_val->type == VAR_NUM) {
                return return_val->value.num_val;
            }
            return 0;
        }
        default:
            return 0;
    }
}

static void execute_var_decl(const VarDeclNode *node, struct hashmap *scope) {
    if (node->init_expr->type == NODE_EXPR_LITERAL) {
        if (node->type == VAR_NUM || node->type == -1) {
            const double num_val = node->init_expr->data.num_literal.num_val;
            hashmap_set(scope, &(Variable) {
                .name = node->name,
                .type = VAR_NUM,
                .value = { .num_val = num_val }
            });
        } else if (node->type == VAR_STR) {
            char *str_val = node->init_expr->data.str_literal.str_val;
            hashmap_set(scope, &(Variable) {
                .name = node->name,
                .type = VAR_STR,
                .value = { .str_val = str_val }
            });
        } else if (node->type == VAR_NIL) {
            hashmap_set(scope, &(Variable) {
                .name = node->name,
                .type = VAR_NIL,
                .value = { .nil_val = NULL }
            });
        }
    } else if (node->init_expr->type == NODE_EXPR_BINARY || 
               node->init_expr->type == NODE_EXPR_VARIABLE ||
               node->init_expr->type == NODE_FUNC_CALL) {
        if (node->type == VAR_NUM || node->type == -1) {
            const double num_val = evaluate_expression(node->init_expr, scope);
            hashmap_set(scope, &(Variable) {
                .name = node->name,
                .type = VAR_NUM,
                .value = { .num_val = num_val }
            });
        } else if (node->type == VAR_STR) {
            char *str_val = get_string_value(node->init_expr, scope);
            hashmap_set(scope, &(Variable) {
                .name = node->name,
                .type = VAR_STR,
                .value = { .str_val = str_val }
            });
        }
    }
}

char *get_string_value(const ASTNode *node, struct hashmap *scope) {
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
            const Variable *variable = get_variable(scope, node->data.variable.name);
            if (variable) {
                if (variable->type == VAR_STR) {
                    return strdup(variable->value.str_val);
                }
                snprintf(buffer, sizeof(buffer), "%g", variable->value.num_val);
                return strdup(buffer);
            }
            return strdup("<undefined>");
        }
        case NODE_FUNC_CALL: {
            execute_func_call(&node->data.func_call, scope);
            
            const Variable *str_val = get_variable(variable_map, "__FUNCTION_STR_RET_VAL");
            if (str_val && str_val->type == VAR_STR) {
                return strdup(str_val->value.str_val);
            }
            
            const Variable *num_val = get_variable(variable_map, "__FUNCTION_NUM_RET_VAL");
            if (num_val && num_val->type == VAR_NUM) {
                snprintf(buffer, sizeof(buffer), "%g", num_val->value.num_val);
                return strdup(buffer);
            }
            
            return strdup("<no return value>");
        }
        default:
            const double ret = evaluate_expression(node, scope);
            snprintf(buffer, sizeof(buffer), "%g", ret);
            return strdup(buffer);
    }
}

static void execute_print(const PrintNode *node, struct hashmap *scope) {
    char *result = strdup("");

    for (int i = 0; i < node->expr_count; i++) {
        char *part = get_string_value(node->expr_list[i], scope);
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

void execute(const ASTNode *node, struct hashmap *scope) {
    switch (node->type) {
        case NODE_VAR_DECL:
            execute_var_decl(&node->data.var_decl, scope);
            break;
        case NODE_PRINT:
            execute_print(&node->data.print, scope);
            break;
        case NODE_BLOCK:
            execute_block(&node->data.block, scope);
            break;
        case NODE_EXPR_POSTFIX:
            execute_postfix(&node->data.postfix_expr, scope);
            break;
        case NODE_IF:
            execute_if(&node->data.if_stmt, scope);
            break;
        case NODE_WHILE:
            execute_while(&node->data.while_stmt, scope);
            break;
        case NODE_FUNC_DECL:
            execute_func_decl(&node->data.func_decl);
            break;
        case NODE_FUNC_CALL:
            execute_func_call(&node->data.func_call, scope);
            break;
        case NODE_RETURN:
            execute_return(&node->data.return_stmt, scope);
            break;
        default:
            fprintf(stderr, "Unknown node type: %d\n", node->type);
            break;
    }
}

int evaluate_condition(ASTNode *condition, struct hashmap *scope) {
    if (!condition) return 0;

    switch (condition->type) {
        case NODE_EXPR_LITERAL:
            return condition->data.num_literal.num_val != 0;

        case NODE_EXPR_VARIABLE: {
            const Variable *variable = get_variable(scope, condition->data.variable.name);
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
                const Variable *variable = get_variable(scope, condition->data.binary_expr.left->data.variable.name);
                if (variable && variable->type == VAR_NUM) left = variable->value.num_val;
            }

            if (condition->data.binary_expr.right->type == NODE_EXPR_LITERAL) {
                right = condition->data.binary_expr.right->data.num_literal.num_val;
            } else if (condition->data.binary_expr.right->type == NODE_EXPR_VARIABLE) {
                const Variable *variable = get_variable(scope, condition->data.binary_expr.right->data.variable.name);
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

void execute_postfix(const PostfixExprNode *node, struct hashmap *scope) {
    Variable *variable = get_variable(scope, node->var_name);

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

void execute_if(const IfNode *if_node, struct hashmap *scope) {
    struct hashmap *if_scope = hashmap_new(sizeof(Variable), 0, 0, 0,
        variable_hash, variable_compare, NULL, NULL);

    if (evaluate_condition(if_node->condition, scope)) {
        execute(if_node->body, if_scope);
        return;
    }

    for (int i = 0; i < if_node->elif_count; i++) {
        const ASTNode *elif_node = if_node->elif_nodes[i];
        if_scope = hashmap_new(sizeof(Variable), 0, 0, 0,
            variable_hash, variable_compare, NULL, NULL);
        if (evaluate_condition(elif_node->data.if_stmt.condition, scope)) {
            execute(elif_node->data.if_stmt.body, if_scope);
            hashmap_free(if_scope);
            return;
        }
    }

    if_scope = hashmap_new(sizeof(Variable), 0, 0, 0,
            variable_hash, variable_compare, NULL, NULL);
    if (if_node->else_body) {
        execute(if_node->else_body, if_scope);
    }

    hashmap_free(if_scope);
}

void execute_while(const WhileNode *while_node, struct hashmap *scope) {
    struct hashmap *while_scope = hashmap_new(sizeof(Variable), 0, 0, 0,
        variable_hash, variable_compare, NULL, NULL);

    while (evaluate_condition(while_node->condition, scope)) {
        execute(while_node->body, while_scope);
    }

    hashmap_free(while_scope);
}

void execute_func_decl(const FuncDeclNode *func_decl) {
    hashmap_set(function_map, &(Function) {
        .name = func_decl->name,
        .type = func_decl->type,
        .param_count = func_decl->param_count,
        .parameters = func_decl->parameters,
        .body = func_decl->body
    });
}

void execute_func_call(const FuncCallNode *func_call, struct hashmap *scope) {
    const Function *function = hashmap_get(function_map, &(Function) {
        .name = func_call->name
    });

    if (!function) {
        fprintf(stderr, "Undefined function: %s\n", func_call->name);
        exit(EXIT_FAILURE);
    }

    if (func_call->arg_count != function->param_count) {
        fprintf(stderr, "Function %s expects %d arguments, but got %d\n",
            func_call->name, function->param_count, func_call->arg_count);
        exit(EXIT_FAILURE);
    }

    struct hashmap *function_scope = hashmap_new(sizeof(Variable), 0, 0, 0,
        variable_hash, variable_compare, NULL, NULL);

    for (int i = 0; i < func_call->arg_count; i++) {
        const Parameter *param = &function->parameters[i];
        const ASTNode *arg = func_call->arguments[i];

        if (param->type[0] == 'n') {
            const double value = evaluate_expression(arg, scope);
            hashmap_set(function_scope, &(Variable) {
                .name = param->name,
                .type = VAR_NUM,
                .value = { .num_val = value }
            });
        } else if (param->type[0] == 's') {
            char *value = get_string_value(arg, scope);
            hashmap_set(function_scope, &(Variable) {
                .name = param->name,
                .type = VAR_STR,
                .value = { .str_val = value }
            });
        }
    }

    execute(function->body, function_scope);
    hashmap_free(function_scope);
}

void execute_return(const ReturnNode *node, struct hashmap *scope) {
    if (node->expr->type == NODE_EXPR_LITERAL) {
        if (node->expr->data.str_literal.str_val) {
            hashmap_set(variable_map, &(Variable) {
                .name = "__FUNCTION_STR_RET_VAL",
                .type = VAR_STR,
                .value = { .str_val = strdup(node->expr->data.str_literal.str_val) }
            });
        } else {
            hashmap_set(variable_map, &(Variable) {
                .name = "__FUNCTION_NUM_RET_VAL",
                .type = VAR_NUM,
                .value = { .num_val = node->expr->data.num_literal.num_val }
            });
        }
    } else if (node->expr->type == NODE_EXPR_BINARY || 
               node->expr->type == NODE_EXPR_VARIABLE || 
               node->expr->type == NODE_FUNC_CALL)
    {
        if (node->expr->type == NODE_EXPR_VARIABLE) {
            const Variable *var = get_variable(scope, node->expr->data.variable.name);
            if (var && var->type == VAR_STR) {
                hashmap_set(variable_map, &(Variable) {
                    .name = "__FUNCTION_STR_RET_VAL",
                    .type = VAR_STR,
                    .value = { .str_val = strdup(var->value.str_val) }
                });
                goto set_return_flag;
            }
        }
        
        if (node->expr->type == NODE_FUNC_CALL) {
            execute_func_call(&node->expr->data.func_call, scope);
            const Variable *str_ret = get_variable(variable_map, "__FUNCTION_STR_RET_VAL");
            if (str_ret && str_ret->type == VAR_STR) {
                hashmap_set(variable_map, &(Variable) {
                    .name = "__FUNCTION_STR_RET_VAL",
                    .type = VAR_STR,
                    .value = { .str_val = strdup(str_ret->value.str_val) }
                });
                goto set_return_flag;
            }
        }

        const double num_val = evaluate_expression(node->expr, scope);
        hashmap_set(variable_map, &(Variable) {
            .name = "__FUNCTION_NUM_RET_VAL",
            .type = VAR_NUM,
            .value = { .num_val = num_val }
        });
    }

set_return_flag:
    hashmap_set(variable_map, &(Variable) {
        .name = "__FUNCTION_RETURN_FLAG",
        .type = VAR_NUM,
        .value = { .num_val = 1 }
    });
}
