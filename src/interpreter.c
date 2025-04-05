#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "interpreter.h"

#include <setjmp.h>

#include "variables.h"
#include "functions.h"
#include "memory.h"

jmp_buf *current_jmp_buf = NULL;

ReturnContextNode* current_return_ctx = NULL;

double return_value = 0.0;

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

static void execute_block(const BlockNode *block, struct hashmap *scope, ReturnContext *ret_ctx) {
    for (int i = 0; i < block->stmt_count; i++) {
        execute(block->statements[i], scope, ret_ctx);
        if (ret_ctx->is_return) {
            break;
        }
    }
}

static double evaluate_expression(const ASTNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
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
            ReturnContext left_eval_ctx = { .is_return = 0, .type = RET_NONE };
            ReturnContext right_eval_ctx = { .is_return = 0, .type = RET_NONE };

            const double left = evaluate_expression(node->data.binary_expr.left, scope, &left_eval_ctx);
            const double right = evaluate_expression(node->data.binary_expr.right, scope, &right_eval_ctx);

            double result = 0;
            switch (node->data.binary_expr.op) {
                case OP_PLUS:
                    result = left + right;
                    break;
                case OP_MINUS:
                    result = left - right;
                    break;
                case OP_MULTIPLY:
                    result = left * right;
                    break;
                case OP_DIVIDE:
                    if (right == 0) {
                        fprintf(stderr, "Error: Division by zero\n");
                        exit(EXIT_FAILURE);
                    }
                    result = left / right;
                    break;
                case OP_MODULO:
                    if (right == 0) {
                        fprintf(stderr, "Error: Modulo by zero\n");
                        exit(EXIT_FAILURE);
                    }
                    result = (double)((long long)left % (long long)right);
                    break;
                case OP_POWER:
                    if (left == 0 && right == 0) {
                        fprintf(stderr, "Error: 0 to the power of 0 is undefined\n");
                        exit(EXIT_FAILURE);
                    }
                    result = pow(left, right);
                    break;
                case OP_AND:
                    result = (long long) left & (long long) right;
                    break;
                case OP_OR:
                    result = (long long) left | (long long) right;
                    break;
                case OP_XOR:
                    result = (long long) left ^ (long long) right;
                    break;
                case OP_LESS:
                    result = left < right;
                    break;
                case OP_GREATER:
                    result = left > right;
                    break;
                case OP_EQUAL:
                    result = left == right;
                    break;
                case OP_NOT_EQUAL:
                    result = left != right;
                    break;
                case OP_LESS_EQUAL:
                    result = left <= right;
                    break;
                case OP_GREATER_EQUAL:
                    result = left >= right;
                    break;
                default:
                    result = 0;
                    break;
            }

            return result;
        }

        case NODE_FUNC_CALL: {
            execute_func_call(&node->data.func_call, scope, ret_ctx);
            if (ret_ctx->is_return && ret_ctx->type == RET_NUM) {
                return ret_ctx->value.num_val;
            }
            return 0.0;
        }
        default:
            return 0;
    }
}

static void execute_var_decl(const VarDeclNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
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
        }
    } else if (node->init_expr->type == NODE_EXPR_BINARY ||
               node->init_expr->type == NODE_EXPR_VARIABLE ||
               node->init_expr->type == NODE_FUNC_CALL) {
        if (node->type == VAR_NUM || node->type == -1) {
            const double num_val = evaluate_expression(node->init_expr, scope, ret_ctx);
            hashmap_set(scope, &(Variable) {
                .name = node->name,
                .type = VAR_NUM,
                .value = { .num_val = num_val }
            });
        } else if (node->type == VAR_STR) {
            char *str_val = get_string_value(node->init_expr, scope, ret_ctx);
            hashmap_set(scope, &(Variable) {
                .name = node->name,
                .type = VAR_STR,
                .value = { .str_val = str_val }
            });
        }
    }
}

char *get_string_value(const ASTNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
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
            ReturnContext local_ret_ctx = { .is_return = 0, .type = RET_NONE };
            execute_func_call(&node->data.func_call, scope, &local_ret_ctx);
            if (local_ret_ctx.is_return) {
                if (local_ret_ctx.type == RET_STR) {
                    return strdup(local_ret_ctx.value.str_val);
                }
                if (local_ret_ctx.type == RET_NUM) {
                    snprintf(buffer, sizeof(buffer), "%g", local_ret_ctx.value.num_val);
                    return strdup(buffer);
                }
            }
            return strdup("<no return value>");
        }
        default: {
            const double ret = evaluate_expression(node, scope, ret_ctx);
            snprintf(buffer, sizeof(buffer), "%g", ret);
            return strdup(buffer);
        }
    }
}

void execute_print(const PrintNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
    char *result = strdup("");
    if (!result) {
        fprintf(stderr, "Error: Memory allocation failed in print.\n");
        return;
    }

    for (int i = 0; i < node->expr_count; i++) {
        char *part = get_string_value(node->expr_list[i], scope, ret_ctx);
        if (!part) {
            fprintf(stderr, "Error: Failed to get string value for print argument %d.\n", i + 1);
            continue;
        }

        const size_t current_len = strlen(result);
        const size_t part_len = strlen(part);
        char *new_result = safe_realloc(result, current_len + part_len + 1);

        if (!new_result) {
            fprintf(stderr, "Error: Memory reallocation failed during print concatenation.\n");
            free(result);
            free(part);
            return;
        }
        result = new_result;
        strcpy(result + current_len, part);
        free(part);
    }

    printf("%s\n", result);
    free(result);
}

void execute(const ASTNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
    if (ret_ctx->is_return) return;

    switch (node->type) {
        case NODE_VAR_DECL:
            execute_var_decl(&node->data.var_decl, scope, ret_ctx);
        break;
        case NODE_PRINT:
            execute_print(&node->data.print, scope, ret_ctx);
        break;
        case NODE_BLOCK:
            execute_block(&node->data.block, scope, ret_ctx);
        break;
        case NODE_EXPR_POSTFIX:
            execute_postfix(&node->data.postfix_expr, scope);
        break;
        case NODE_IF:
            execute_if(&node->data.if_stmt, scope, ret_ctx);
        break;
        case NODE_WHILE:
            execute_while(&node->data.while_stmt, scope, ret_ctx);
        break;
        case NODE_FUNC_DECL:
            execute_func_decl(&node->data.func_decl);
        break;
        case NODE_FUNC_CALL:
            execute_func_call(&node->data.func_call, scope, ret_ctx);
        break;
        case NODE_RETURN:
            execute_return(&node->data.return_stmt, scope, ret_ctx);
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

void execute_if(const IfNode *if_node, struct hashmap *scope, ReturnContext *ret_ctx) {
    struct hashmap *if_scope = hashmap_new(sizeof(Variable), 0, 0, 0,
        variable_hash, variable_compare, NULL, NULL);

    if (evaluate_condition(if_node->condition, scope)) {
        execute(if_node->body, if_scope, ret_ctx);
        return;
    }

    for (int i = 0; i < if_node->elif_count; i++) {
        const ASTNode *elif_node = if_node->elif_nodes[i];
        if_scope = hashmap_new(sizeof(Variable), 0, 0, 0,
            variable_hash, variable_compare, NULL, NULL);
        if (evaluate_condition(elif_node->data.if_stmt.condition, scope)) {
            execute(elif_node->data.if_stmt.body, if_scope, ret_ctx);
            hashmap_free(if_scope);
            return;
        }
    }

    if_scope = hashmap_new(sizeof(Variable), 0, 0, 0,
            variable_hash, variable_compare, NULL, NULL);
    if (if_node->else_body) {
        execute(if_node->else_body, if_scope, ret_ctx);
    }

    hashmap_free(if_scope);
}

void execute_while(const WhileNode *while_node, struct hashmap *scope, ReturnContext *ret_ctx) {
    struct hashmap *while_scope = hashmap_new(sizeof(Variable), 0, 0, 0,
        variable_hash, variable_compare, NULL, NULL);

    while (evaluate_condition(while_node->condition, scope)) {
        execute(while_node->body, while_scope, ret_ctx);
    }

    hashmap_free(while_scope);
}

void execute_func_decl(const FuncDeclNode *func_decl) {
    Function *func = malloc(sizeof(Function));
    if (!func) {
        fprintf(stderr, "Memory allocation failed for function %s\n", func_decl->name);
        exit(EXIT_FAILURE);
    }
    // Make a persistent copy of the function name.
    func->name = strdup(func_decl->name);
    if (!func->name) {
        fprintf(stderr, "Memory allocation failed for function name %s\n", func_decl->name);
        exit(EXIT_FAILURE);
    }
    func->param_count = func_decl->param_count;
    func->parameters = func_decl->parameters;
    func->body = func_decl->body;
    hashmap_set(function_map, func);
}

double execute_function_body(const ASTNode *body, struct hashmap *scope) {
    struct {
        jmp_buf buf;
        double ret_val;
    } local_buf;

    ReturnContext local_ctx = {0};
    local_ctx.jmp = &local_buf.buf;
    local_ctx.ret_ptr = &local_buf.ret_val;

    ReturnContextNode node;
    node.ctx = &local_ctx;
    node.prev = current_return_ctx;
    current_return_ctx = &node;

    if (setjmp(local_buf.buf) == 0) {
        execute(body, scope, &local_ctx);
        current_return_ctx = node.prev;
        return 0.0;
    }
    const double ret = local_buf.ret_val;
    current_return_ctx = node.prev;
    return ret;
}

void execute_func_call(const FuncCallNode *func_call, struct hashmap *scope, ReturnContext *caller_ret_ctx) {
    const Function *function = hashmap_get(function_map, &(Function){ .name = func_call->name });
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
        ReturnContext arg_ctx = {0, RET_NONE, {0}, NULL, NULL};
        if (param->type[0] == 'n') {
            double value = evaluate_expression(arg, scope, &arg_ctx);
            hashmap_set(function_scope, &(Variable){
                .name = param->name,
                .type = VAR_NUM,
                .value = { .num_val = value }
            });
        } else if (param->type[0] == 's') {
            char *value = get_string_value(arg, scope, &arg_ctx);
            hashmap_set(function_scope, &(Variable){
                .name = param->name,
                .type = VAR_STR,
                .value = { .str_val = value }
            });
        }
    }

    const double func_ret = execute_function_body(function->body, function_scope);

    caller_ret_ctx->is_return = 1;
    caller_ret_ctx->type = RET_NUM;
    caller_ret_ctx->value.num_val = func_ret;

    hashmap_free(function_scope);
}




void execute_return(const ReturnNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
    double ret_val = 0.0;
    if (node->expr) {
        ReturnContext temp = {0, RET_NONE, {0}, NULL, NULL};
        ret_val = evaluate_expression(node->expr, scope, &temp);
    }
    if (ret_ctx->ret_ptr) {
        *(ret_ctx->ret_ptr) = ret_val;
    }
    if (current_return_ctx && current_return_ctx->ctx && current_return_ctx->ctx->jmp) {
        longjmp(*(current_return_ctx->ctx->jmp), 1);
    }
    ret_ctx->is_return = 1;
    ret_ctx->type = RET_NUM;
    ret_ctx->value.num_val = ret_val;
}
