#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "interpreter.h"

#include <setjmp.h>

#include "variables.h"
#include "functions.h"
#include "memory.h"

ReturnContextNode* current_return_ctx = NULL;

void free_return_value(const ReturnType type, ReturnValue *value) {
    if (!value) return;

    switch (type) {
        case RET_STR:
            if (value->value.str_val) {
                free(value->value.str_val);
                value->value.str_val = NULL;
            }
            break;
        case RET_LIST:
            if (value->value.list_val) {
                free_list(value->value.list_val);
                value->value.list_val = NULL;
            }
            break;
        case RET_NUM:
        case RET_NONE:
        default:
            break;
    }
}

static void free_function_scope_lists(struct hashmap *scope) {
    if (!scope) return;
    size_t i = 0;
    void *item;
    ListNode **heads_to_free = NULL;
    int count = 0;

    while (hashmap_iter(scope, &i, &item)) {
        const Variable *var = (Variable *)item;
        if (var->type == VAR_LIST && var->value.list_val.head != NULL) {
            heads_to_free = safe_realloc(heads_to_free, (count + 1) * sizeof(ListNode*));
            heads_to_free[count++] = var->value.list_val.head;
        } else if (var->type == VAR_STR && var->value.str_val != NULL) {
            free(var->value.str_val);
        }
    }

    for (int j = 0; j < count; ++j) {
        if (heads_to_free != NULL)
            free_list(heads_to_free[j]);
    }
    free(heads_to_free);
}

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

ReturnValue evaluate_expression(const ASTNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
    ReturnValue result = { .type = RET_NONE };  // Always initialize safely
    switch (node->type) {
        case NODE_EXPR_LITERAL:
            result.type = RET_NUM;
            result.value.num_val = node->data.num_literal.num_val;
            break;

        case NODE_EXPR_VARIABLE: {
            const Variable *variable = get_variable(scope, node->data.variable.name);
            if (!variable) {
                fprintf(stderr, "Undefined variable: %s\n", node->data.variable.name);
                exit(EXIT_FAILURE);
            }
            switch (variable->type) {
                case VAR_NUM:
                    result.type = RET_NUM;
                    result.value.num_val = variable->value.num_val;
                    break;
                case VAR_STR:
                    result.type = RET_STR;
                    result.value.str_val = variable->value.str_val;
                    break;
                case VAR_LIST:
                    result.type = RET_LIST;
                    result.value.list_val = variable->value.list_val.head;
                    break;
                default:
                    fprintf(stderr, "Unknown variable type: %d\n", variable->type);
                    exit(EXIT_FAILURE);
            }
            break;
        }
        case NODE_EXPR_UNARY: {
            const UnaryExprNode *unary_node = &node->data.unary_expr;
            ReturnContext operand_ctx = { .is_return = 0, .ret_val.type = RET_NONE };
            ReturnValue operand_value = evaluate_expression(unary_node->operand, scope, &operand_ctx);
            if (operand_ctx.is_return) {
                if (operand_ctx.ret_val.type == RET_NUM) {
                    operand_value.value.num_val = operand_ctx.ret_val.value.num_val;
                } else {
                    fprintf(stderr, "Error: Cannot apply unary minus to non-numeric return value.\n");
                    exit(EXIT_FAILURE);
                }
            }

            switch (unary_node->op) {
                case OP_NEGATE:
                    result.type = RET_NUM;
                    result.value.num_val = -operand_value.value.num_val;
                    break;
                default:
                    fprintf(stderr, "Error: Unknown unary operator %d.\n", unary_node->op);
                    exit(EXIT_FAILURE);
            }
        }
        case NODE_EXPR_BINARY: {
            ReturnContext left_eval_ctx = { .is_return = 0, .ret_val.type = RET_NONE };
            ReturnContext right_eval_ctx = { .is_return = 0, .ret_val.type = RET_NONE };

            const double left = evaluate_expression(node->data.binary_expr.left, scope, &left_eval_ctx).value.num_val;
            const double right = evaluate_expression(node->data.binary_expr.right, scope, &right_eval_ctx).value.num_val;

            double binary_result;
            switch (node->data.binary_expr.op) {
                case OP_PLUS:
                    binary_result = left + right;
                    break;
                case OP_MINUS:
                    binary_result = left - right;
                    break;
                case OP_MULTIPLY:
                    binary_result = left * right;
                    break;
                case OP_DIVIDE:
                    if (right == 0) {
                        fprintf(stderr, "Error: Division by zero\n");
                        exit(EXIT_FAILURE);
                    }
                    binary_result = left / right;
                    break;
                case OP_MODULO:
                    if (right == 0) {
                        fprintf(stderr, "Error: Modulo by zero\n");
                        exit(EXIT_FAILURE);
                    }
                    binary_result = (double) ((long long) left % (long long) right);
                    break;
                case OP_POWER:
                    if (left == 0 && right == 0) {
                        fprintf(stderr, "Error: 0 to the power of 0 is undefined\n");
                        exit(EXIT_FAILURE);
                    }
                    binary_result = pow(left, right);
                    break;
                case OP_AND:
                    binary_result = (long long) left & (long long) right;
                    break;
                case OP_OR:
                    binary_result = (long long) left | (long long) right;
                    break;
                case OP_XOR:
                    binary_result = (long long) left ^ (long long) right;
                    break;
                case OP_LESS:
                    binary_result = left < right;
                    break;
                case OP_GREATER:
                    binary_result = left > right;
                    break;
                case OP_EQUAL:
                    binary_result = left == right;
                    break;
                case OP_NOT_EQUAL:
                    binary_result = left != right;
                    break;
                case OP_LESS_EQUAL:
                    binary_result = left <= right;
                    break;
                case OP_GREATER_EQUAL:
                    binary_result = left >= right;
                    break;
                default:
                    binary_result = 0;
                    break;
            }
            result.type = RET_NUM;
            result.value.num_val = binary_result;
            break;
        }

        case NODE_FUNC_CALL:
            execute_func_call(&node->data.func_call, scope, ret_ctx);
            if (ret_ctx->ret_val.type == RET_NONE) {
                fprintf(stderr, "Error: Function call returned no value.\n");
                exit(EXIT_FAILURE);
            }
            result.type = ret_ctx->ret_val.type;
            result.value = ret_ctx->ret_val.value;
            break;

        case NODE_LIST_ACCESS: {
            const ListAccessNode *access_node = &node->data.list_access;
            Variable *list_var = get_variable(scope, access_node->list_name);
            if (!list_var) {
                fprintf(stderr, "Error: List variable '%s' not found.\n", access_node->list_name);
                exit(EXIT_FAILURE);
            }
            if (list_var->type != VAR_LIST) {
                fprintf(stderr, "Error: Variable '%s' is not a list.\n", access_node->list_name);
                exit(EXIT_FAILURE);
            }
            int list_size = 0;
            const ListNode *counter_node = list_var->value.list_val.head;
            while (counter_node != NULL) {
                list_size++;
                counter_node = counter_node->next;
            }
            ReturnContext index_eval_ctx = { .is_return = 0, .ret_val.type = RET_NONE };
            const double index_val_double = evaluate_expression(access_node->index_expr, scope, &index_eval_ctx).value.num_val;
            if (index_val_double != floor(index_val_double)) {
                 fprintf(stderr, "Error: List index for '%s' must be an integer, got %f.\n",
                         access_node->list_name, index_val_double);
                 exit(EXIT_FAILURE);
            }

            int index = (int) index_val_double;
            if (index < 0) {
                index = list_size + index;
            }
            if (index < 0 || index >= list_size) {
                fprintf(stderr, "Error: List index %d (calculated from %f) out of bounds for list '%s' of size %d.\n",
                        index, index_val_double, access_node->list_name, list_size);
                exit(EXIT_FAILURE);
            }
            const ListNode *current = list_var->value.list_val.head;
            int count = 0;
            while (count < index) {
                current = current->next;
                count++;
            }

            if (list_var->value.list_val.element_type == VAR_NUM) {
                 if (current->element.type == VAR_NUM) {
                     result.type = RET_NUM;
                     result.value.num_val = current->element.value.num_val;
                 }
                 fprintf(stderr, "Internal Error: List '%s' element type mismatch at index %d.\n", access_node->list_name, index);
                 exit(EXIT_FAILURE);
            }
            if (list_var->value.list_val.element_type == VAR_STR) {
                fprintf(stderr, "Error: Accessing string list elements directly within numerical expressions is not yet supported.\n");
                exit(EXIT_FAILURE);
            }
            fprintf(stderr, "Internal Error: Unknown list element type for '%s'.\n", access_node->list_name);
            exit(EXIT_FAILURE);
        }
        case NODE_EXPR_POSTFIX: {
            const PostfixExprNode *postfix_node = &node->data.postfix_expr;
            Variable *variable = get_variable(scope, postfix_node->var_name);

            if (!variable) {
                fprintf(stderr, "Error: Undefined variable '%s' in postfix operation.\n", postfix_node->var_name);
                exit(EXIT_FAILURE);
            }

            if (variable->type != VAR_NUM) {
                fprintf(stderr, "Error: Cannot apply postfix operator to non-numeric variable '%s'.\n", postfix_node->var_name);
                exit(EXIT_FAILURE);
            }

            const double original_value = variable->value.num_val;

            switch (postfix_node->op) {
                case OP_INC:
                    variable->value.num_val++;
                    break;
                case OP_DEC:
                    variable->value.num_val--;
                    break;
            }

            result.type = RET_NUM;
            result.value.num_val = original_value;
            break;
        }
        default:
            fprintf(stderr, "Error: Unknown node type: %d.\n", node->type);
            exit(EXIT_FAILURE);
    }
    return result;
}

void execute_var_decl(const VarDeclNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
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
               node->init_expr->type == NODE_FUNC_CALL)
    {
        if (node->type == VAR_NUM || node->type == -1) {
            const double num_val = evaluate_expression(node->init_expr, scope, ret_ctx).value.num_val;
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

void execute_list_decl(const ListDeclNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
    ListNode *head = NULL;
    ListNode *current = NULL;

    if (node->init_expr && node->init_expr->type == NODE_LIST_LITERAL) {
        const ListLiteralNode *literal = &node->init_expr->data.list_literal;

        if (literal->element_type != node->element_type) {
            fprintf(stderr, "Error: List literal type mismatch for variable '%s'. Expected %d, got %d.\n",
                    node->name, node->element_type, literal->element_type);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < literal->element_count; ++i) {
            ListElement element;
            element.type = node->element_type;

            if (element.type == VAR_NUM) {
                element.value.num_val = evaluate_expression(literal->elements[i], scope, ret_ctx).value.num_val;
            } else if (element.type == VAR_STR) {
                element.value.str_val = get_string_value(literal->elements[i], scope, ret_ctx);
                if (!element.value.str_val) {
                     fprintf(stderr, "Error: Failed to evaluate string element %d for list '%s'.\n", i, node->name);
                     free_list(head);
                     exit(EXIT_FAILURE);
                 }
            } else {
                 fprintf(stderr, "Internal Error: Invalid element type %d in list literal for '%s'.\n", element.type, node->name);
                 free_list(head);
                 exit(EXIT_FAILURE);
            }

            ListNode *new_node = create_list_node(element);
             if (!new_node) {
                  fprintf(stderr, "Error: Failed to allocate memory for list node for '%s'.\n", node->name);
                  free_list(head);
                  if (element.type == VAR_STR) free(element.value.str_val);
                  exit(EXIT_FAILURE);
             }

            if (head == NULL) {
                head = new_node;
                current = head;
            } else if (current != NULL) {
                current->next = new_node;
                current = new_node;
            }
        }
    }

    const Variable new_list_var = {
        .name = strdup(node->name),
        .type = VAR_LIST,
        .value = { .list_val = { .element_type = node->element_type, .head = head } }
    };
     if (!new_list_var.name) {
         fprintf(stderr, "Error: Failed to allocate memory for list variable name '%s'.\n", node->name);
         free_list(head);
         exit(EXIT_FAILURE);
     }

    hashmap_set(scope, &new_list_var);

    if (hashmap_oom(scope ? scope : variable_map)) {
         fprintf(stderr, "Error: Out of memory while storing list variable '%s'.\n", node->name);
         free(new_list_var.name);
         free_list(head);
         exit(EXIT_FAILURE);
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
                if (variable->type == VAR_NUM) {
                    snprintf(buffer, sizeof(buffer), "%g", variable->value.num_val);
                    return strdup(buffer);
                }
                if (variable->type == VAR_LIST) {
                    return strdup(list_to_string(variable->value.list_val.head, variable->value.list_val.element_type));
                }
            }
            fprintf(stderr, "Undefined variable: %s\n", node->data.variable.name);
            exit(EXIT_FAILURE);
        }
        case NODE_EXPR_UNARY: {
            ReturnContext temp_ctx = {0};
            const double value = evaluate_expression(node, scope, &temp_ctx).value.num_val;
            snprintf(buffer, sizeof(buffer), "%g", value);
            return strdup(buffer);
        }
        case NODE_FUNC_CALL: {
            ReturnContext local_ret_ctx = { .is_return = 0 };
            execute_func_call(&node->data.func_call, scope, &local_ret_ctx);

            if (local_ret_ctx.is_return) {
                switch (local_ret_ctx.ret_val.type) {
                    case RET_STR:
                        return strdup(local_ret_ctx.ret_val.value.str_val);
                    case RET_NUM: {
                        char num_buffer[64];
                        snprintf(num_buffer, sizeof(num_buffer), "%g", local_ret_ctx.ret_val.value.num_val);
                        return strdup(num_buffer);
                    }
                    case RET_LIST: {
                        char *list_str = list_to_string(local_ret_ctx.ret_val.value.list_val, VAR_NUM);
                        return list_str ? list_str : strdup("[Error: Failed to convert list]");
                    }
                    default:
                        fprintf(stderr, "Error: Unsupported return type in string context.\n");
                    exit(EXIT_FAILURE);
                }
            }
            fprintf(stderr, "Error: Function call did not return a value in string context.\n");
            exit(EXIT_FAILURE);
        }
        case NODE_LIST_ACCESS: {
            const ListAccessNode *access_node = &node->data.list_access;
            Variable *list_var = get_variable(scope, access_node->list_name);
             if (!list_var || list_var->type != VAR_LIST) {
                 fprintf(stderr, "Error: List variable '%s' not found or is not a list.\n", access_node->list_name);
                 exit(EXIT_FAILURE);
             }

             int list_size = 0;
            const ListNode *counter_node = list_var->value.list_val.head;
             while (counter_node != NULL) {
                 list_size++;
                 counter_node = counter_node->next;
             }

             ReturnContext index_eval_ctx = { .is_return = 0, .ret_val.type = RET_NONE };
            const double index_val_double = evaluate_expression(access_node->index_expr, scope, &index_eval_ctx).value.num_val;
             if (index_val_double != floor(index_val_double)) {
                  fprintf(stderr, "Error: List index must be an integer.\n");
                 exit(EXIT_FAILURE);
             }
             int index = (int) index_val_double;

             if (index < 0) {
                 index = list_size + index;
             }

             if (index < 0 || index >= list_size) {
                 fprintf(stderr, "Error: List index out of bounds.\n");
                 exit(EXIT_FAILURE);
             }

            const ListNode *current = list_var->value.list_val.head;
             int count = 0;
             while (count < index) {
                 current = current->next;
                 count++;
             }

             if (current->element.type == VAR_NUM) {
                 snprintf(buffer, sizeof(buffer), "%g", current->element.value.num_val);
                 return strdup(buffer);
             }
             if (current->element.type == VAR_STR) {
                return strdup(current->element.value.str_val ? current->element.value.str_val : "");
             }
            fprintf(stderr, "Error: unknown list element type.\n");
            exit(EXIT_FAILURE);
        }
        default: {
            // TODO: REMOVE/MODIF THIS SECTIOn
            const double ret = evaluate_expression(node, scope, ret_ctx).value.num_val;
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
        case NODE_LIST_DECL:
            execute_list_decl(&node->data.list_decl, scope, ret_ctx);
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
        case NODE_ASSIGNMENT:
            execute_assignment(&node->data.assignment, scope);
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

ReturnValue execute_function_body(const ASTNode *body, struct hashmap *scope, ReturnContext *ret_ctx) {
    jmp_buf buf;
    ret_ctx->jmp = &buf;

    ReturnContextNode node;
    node.ctx = ret_ctx;
    node.prev = current_return_ctx;
    current_return_ctx = &node;

    ReturnValue result = { .type = RET_NONE };

    if (setjmp(buf) == 0) {
        execute(body, scope, ret_ctx);
    } else {
        switch (ret_ctx->ret_val.type) {
            case RET_NUM:
                result.value.num_val = ret_ctx->ret_val.value.num_val;
                result.type = RET_NUM;
                break;
            case RET_STR:
                result.value.str_val = ret_ctx->ret_val.value.str_val;
                result.type = RET_STR;
                ret_ctx->ret_val.value.str_val = NULL; // Prevent double-free
                break;
            case RET_LIST:
                result.value.list_val = ret_ctx->ret_val.value.list_val;
                result.type = RET_LIST;
                ret_ctx->ret_val.value.list_val = NULL; // Prevent double-free
                break;
            case RET_NONE:
                break;
            default:
                fprintf(stderr, "Internal Error: Unknown return type %d in execute_function_body.\n", ret_ctx->ret_val.type);
            exit(EXIT_FAILURE);
        }
    }

    current_return_ctx = node.prev;
    return result;
}

void execute_func_call(const FuncCallNode *func_call, struct hashmap *scope, ReturnContext *caller_ret_ctx) {
    const Function *function = hashmap_get(function_map, &(Function) { .name = func_call->name });
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
                                                  variable_hash, variable_compare,
                                                  NULL, NULL);
    if (!function_scope) {
        fprintf(stderr, "Failed to allocate scope for function %s\n", func_call->name);
        exit(EXIT_FAILURE);
    }


    for (int i = 0; i < func_call->arg_count; i++) {
        const Parameter *param = &function->parameters[i];
        const ASTNode *arg_node = func_call->arguments[i];
        ReturnContext arg_eval_ctx = {0, RET_NONE, {0}, NULL};

        if (param->is_list) {
            ListNode *list_to_pass_head = NULL;
            const VarType list_element_type = param->type;

            if (arg_node->type == NODE_LIST_LITERAL) {
                 const ListLiteralNode *literal_node = &arg_node->data.list_literal;
                 ListNode *current_new_node = NULL;

                 for (int j = 0; j < literal_node->element_count; ++j) {
                     ListElement element;
                     element.type = list_element_type;
                     if (list_element_type == VAR_NUM) {
                         element.value.num_val = evaluate_expression(literal_node->elements[j], scope, &arg_eval_ctx).value.num_val;
                     } else if (list_element_type == VAR_STR) {
                         element.value.str_val = get_string_value(literal_node->elements[j], scope, &arg_eval_ctx);
                         if (!element.value.str_val) {
                             fprintf(stderr, "Error evaluating string element %d for list literal argument %d in call to '%s'.\n",
                                      j, i + 1, func_call->name);
                             free_list(list_to_pass_head);
                             free_function_scope_lists(function_scope);
                             hashmap_free(function_scope);
                             exit(EXIT_FAILURE);
                         }
                     } else {
                          fprintf(stderr, "Internal Error: Unsupported list element type %d for parameter '%s' in function '%s'.\n",
                                  list_element_type, param->name, func_call->name);
                         free_list(list_to_pass_head);
                         free_function_scope_lists(function_scope);
                         hashmap_free(function_scope);
                         exit(EXIT_FAILURE);
                     }

                     ListNode *new_node = create_list_node(element);
                      if (!new_node) {
                           fprintf(stderr, "Error: Failed to allocate memory for list node from literal for function '%s'.\n", func_call->name);
                           if (list_element_type == VAR_STR && element.value.str_val) free(element.value.str_val);
                           free_list(list_to_pass_head);
                           free_function_scope_lists(function_scope);
                           hashmap_free(function_scope);
                           exit(EXIT_FAILURE);
                      }
                     if (list_to_pass_head == NULL) {
                         list_to_pass_head = new_node;
                         current_new_node = new_node;
                    } else if (current_new_node != NULL) {
                         current_new_node->next = new_node;
                         current_new_node = new_node;
                    }
                 }

            } else if (arg_node->type == NODE_EXPR_VARIABLE) {
                 char *list_arg_name = arg_node->data.variable.name;
                 Variable *list_var_caller = get_variable(scope, list_arg_name);

                 if (!list_var_caller) {
                     fprintf(stderr, "Error: List variable '%s' (argument %d for function '%s') not found in caller scope.\n",
                              list_arg_name, i + 1, func_call->name);
                     free_function_scope_lists(function_scope);
                     hashmap_free(function_scope);
                     exit(EXIT_FAILURE);
                 }
                 if (list_var_caller->type != VAR_LIST) {
                      fprintf(stderr, "Error: Variable '%s' passed as argument %d to function '%s' is not a list.\n",
                              list_arg_name, i + 1, func_call->name);
                      free_function_scope_lists(function_scope);
                      hashmap_free(function_scope);
                      exit(EXIT_FAILURE);
                 }
                 if (list_var_caller->value.list_val.element_type != list_element_type) {
                      fprintf(stderr, "Error: Type mismatch for list argument %d ('%s') for function '%s'. Expected list of type %d, got %d.\n",
                              i + 1, list_arg_name, func_call->name, list_element_type, list_var_caller->value.list_val.element_type);
                      free_function_scope_lists(function_scope);
                      hashmap_free(function_scope);
                      exit(EXIT_FAILURE);
                 }
                 list_to_pass_head = deep_copy_list(list_var_caller->value.list_val.head);
                 if (!list_to_pass_head && list_var_caller->value.list_val.head != NULL) {
                       fprintf(stderr, "Error: Failed to copy list argument %d for function '%s'.\n", i + 1, func_call->name);
                       free_function_scope_lists(function_scope);
                       hashmap_free(function_scope);
                       exit(EXIT_FAILURE);
                 }
            } else {
                 fprintf(stderr, "Error: Invalid argument type (%d) for list parameter '%s' in function '%s'. Expected list variable or literal.\n",
                         arg_node->type, param->name, func_call->name);
                 free_function_scope_lists(function_scope);
                 hashmap_free(function_scope);
                 exit(EXIT_FAILURE);
            }
            Variable function_param_var = {
                .name = strdup(param->name),
                .type = VAR_LIST,
                .value = { .list_val = { .element_type = list_element_type, .head = list_to_pass_head } }
            };
            if (!function_param_var.name) {
                 fprintf(stderr, "Error: Failed to allocate memory for parameter name '%s'.\n", param->name);
                 free_list(list_to_pass_head);
                 free_function_scope_lists(function_scope);
                 hashmap_free(function_scope);
                 exit(EXIT_FAILURE);
            }

            hashmap_set(function_scope, &function_param_var);
            if (hashmap_oom(function_scope)) {
                  fprintf(stderr, "Error: Out of memory setting list parameter '%s'.\n", param->name);
                  free(function_param_var.name);
                  free_list(list_to_pass_head);
                  free_function_scope_lists(function_scope);
                  hashmap_free(function_scope);
                  exit(EXIT_FAILURE);
            }

        } else if (param->type == VAR_NUM) {
            const double value = evaluate_expression(arg_node, scope, &arg_eval_ctx).value.num_val;
             Variable function_param_var = {
                  .name = strdup(param->name),
                  .type = VAR_NUM,
                  .value = { .num_val = value }
             };
             if (!function_param_var.name) {
                 free_function_scope_lists(function_scope);
                 hashmap_free(function_scope);
                 exit(EXIT_FAILURE);
             }
             hashmap_set(function_scope, &function_param_var);
              if (hashmap_oom(function_scope)) {
                  free(function_param_var.name);
                  free_function_scope_lists(function_scope);
                  hashmap_free(function_scope);
                  exit(EXIT_FAILURE);
              }

        } else if (param->type == VAR_STR) {
            char *value = get_string_value(arg_node, scope, &arg_eval_ctx);
             Variable function_param_var = {
                  .name = strdup(param->name),
                  .type = VAR_STR,
                  .value = { .str_val = value }
             };
             if (!function_param_var.name) {
                 free(value);
                 free_function_scope_lists(function_scope); hashmap_free(function_scope); exit(EXIT_FAILURE);
             }
             hashmap_set(function_scope, &function_param_var);
              if (hashmap_oom(function_scope)) {
                  free(value);
                  free(function_param_var.name);
                  free_function_scope_lists(function_scope);
                  hashmap_free(function_scope);
                  exit(EXIT_FAILURE);
              }
        }
    }

    const ReturnValue func_ret = execute_function_body(function->body, function_scope, caller_ret_ctx);

    caller_ret_ctx->is_return = 1;
    caller_ret_ctx->ret_val.type = func_ret.type;
    caller_ret_ctx->ret_val.value = func_ret.value;
    free_function_scope_lists(function_scope);
    hashmap_free(function_scope);
}

void execute_return(const ReturnNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
    ReturnContext temp = {0, RET_NONE, {0}, NULL};

    if (node->expr) {
        temp.ret_val = evaluate_expression(node->expr, scope, &temp);
    }

    free_return_value(ret_ctx->ret_val.type, &ret_ctx->ret_val);

    ret_ctx->is_return = 1;
    ret_ctx->ret_val.type = temp.ret_val.type;

    switch (temp.ret_val.type) {
        case RET_NUM:
            ret_ctx->ret_val.value.num_val = temp.ret_val.value.num_val;
            break;

        case RET_STR:
            if (temp.ret_val.value.str_val) {
                ret_ctx->ret_val.value.str_val = strdup(temp.ret_val.value.str_val);
                if (!ret_ctx->ret_val.value.str_val) {
                    fprintf(stderr, "Error: Failed to duplicate string during return.\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                ret_ctx->ret_val.value.str_val = NULL;
            }
            break;

        case RET_LIST:
            if (temp.ret_val.value.list_val) {
                ret_ctx->ret_val.value.list_val = deep_copy_list(temp.ret_val.value.list_val);
                if (!ret_ctx->ret_val.value.list_val) {
                    fprintf(stderr, "Error: Failed to copy list during return.\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                ret_ctx->ret_val.value.list_val = NULL;
            }
            break;

        case RET_NONE:
        default:
            break;
    }

    if (current_return_ctx && current_return_ctx->ctx && current_return_ctx->ctx->jmp) {
        longjmp(*(current_return_ctx->ctx->jmp), 1);
    }
}

void execute_assignment(const AssignmentNode *node, struct hashmap *scope) {
    if (node->index_expr != NULL) {
        Variable *list_var = get_variable(scope, node->target_name);

        if (!list_var) {
            fprintf(stderr, "Error: List variable '%s' not found for assignment.\n", node->target_name);
            exit(EXIT_FAILURE);
        }
        if (list_var->type != VAR_LIST) {
            fprintf(stderr, "Error: Variable '%s' is not a list for assignment.\n", node->target_name);
            exit(EXIT_FAILURE);
        }

        int list_size = 0;
        const ListNode *counter_node = list_var->value.list_val.head;
        while (counter_node != NULL) {
            list_size++;
            counter_node = counter_node->next;
        }

        ReturnContext index_eval_ctx = {0};
        const double index_val_double = evaluate_expression(node->index_expr, scope, &index_eval_ctx).value.num_val;

        if (index_val_double != floor(index_val_double)) {
            fprintf(stderr, "Error: List index for '%s' must be an integer, got %f.\n", node->target_name, index_val_double);
            exit(EXIT_FAILURE);
        }
        int index = (int)index_val_double;
        if (index < 0) {
            index = list_size + index;
        }
        if (index < 0 || index >= list_size) {
            fprintf(stderr, "Error: List index %d (calculated from %f) out of bounds for list '%s' of size %d during assignment.\n",
                    index, index_val_double, node->target_name, list_size);
            exit(EXIT_FAILURE);
        }

        ListNode *target_lnode = list_var->value.list_val.head;
        for (int i = 0; i < index; ++i) {
            target_lnode = target_lnode->next;
        }

        ReturnContext value_eval_ctx = {0};
        if (list_var->value.list_val.element_type == VAR_NUM) {
            const double value_to_assign = evaluate_expression(node->value_expr, scope, &value_eval_ctx).value.num_val;
            if (target_lnode->element.type != VAR_NUM) {
                 fprintf(stderr, "Internal Error: List '%s' node type mismatch at index %d.\n", node->target_name, index);
                 exit(EXIT_FAILURE);
            }
            target_lnode->element.value.num_val = value_to_assign;
        } else if (list_var->value.list_val.element_type == VAR_STR) {
            char *value_to_assign_str = get_string_value(node->value_expr, scope, &value_eval_ctx);

            if (!value_to_assign_str) {
                 fprintf(stderr, "Error: Failed to evaluate string value for assignment to list '%s' at index %d.\n", node->target_name, index);
                 exit(EXIT_FAILURE);
            }
             if (target_lnode->element.type != VAR_STR) {
                  fprintf(stderr, "Internal Error: List '%s' node type mismatch at index %d.\n", node->target_name, index);
                  free(value_to_assign_str);
                  exit(EXIT_FAILURE);
             }
            if (target_lnode->element.value.str_val != NULL) {
                free(target_lnode->element.value.str_val);
            }
            target_lnode->element.value.str_val = value_to_assign_str;
        } else {
             fprintf(stderr, "Internal Error: Assignment to list '%s' with unknown element type.\n", node->target_name);
             exit(EXIT_FAILURE);
        }

    } else {
        Variable *existing_var = get_variable(scope, node->target_name);

        if (!existing_var) {
            fprintf(stderr, "Error: Variable '%s' not declared before assignment.\n", node->target_name);
            exit(EXIT_FAILURE);
        }

        ReturnContext value_eval_ctx = {0};
        if (existing_var->type == VAR_NUM) {
            const double value_to_assign = evaluate_expression(node->value_expr, scope, &value_eval_ctx).value.num_val;
            existing_var->value.num_val = value_to_assign;
        } else if (existing_var->type == VAR_STR) {
            char *value_to_assign_str = get_string_value(node->value_expr, scope, &value_eval_ctx);
             if (!value_to_assign_str) {
                  fprintf(stderr, "Error: Failed to evaluate string value for assignment to variable '%s'.\n", node->target_name);
                  exit(EXIT_FAILURE);
             }
            if (existing_var->value.str_val != NULL) {
                free(existing_var->value.str_val);
            }
            existing_var->value.str_val = value_to_assign_str;
        } else if (existing_var->type == VAR_LIST) {
             fprintf(stderr, "Error: Cannot assign directly to a list variable '%s' using '='. Use list declaration or modify elements.\n", node->target_name);
             exit(EXIT_FAILURE);
        } else {
             fprintf(stderr, "Internal Error: Assignment to variable '%s' with unknown type.\n", node->target_name);
             exit(EXIT_FAILURE);
        }
    }
}