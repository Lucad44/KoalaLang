#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "interpreter.h"

#include <setjmp.h>

#include "ast.h"
#include "variables.h"
#include "functions.h"
#include "memory.h"
#include "modules.h"

ReturnContextNode* current_return_ctx = NULL;

int is_truthy(const ReturnValue val) {
    if (val.type == RET_NUM) {
        return val.value.num_val != 0.0;
    }
    if (val.type == RET_STR) {
        return val.value.str_val != NULL && strlen(val.value.str_val) > 0;
    }
    if (val.type == RET_LIST) {
        return val.value.list_val.head != NULL;
    }
    return 0;
}


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
            free_list(value->value.list_val.head);
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
    if (node == NULL) {
        fprintf(stderr, "\nError: Trying to evaluate a NULL expression node.\n");
        exit(EXIT_FAILURE);
    }
    ReturnValue result = { .type = RET_NONE };
    switch (node->type) {
        case NODE_NUM_LITERAL:
            result.type = RET_NUM;
            result.value.num_val = node->data.num_literal.num_val;
            break;
        case NODE_STR_LITERAL:
            result.type = RET_STR;
            result.value.str_val = strdup(node->data.str_literal.str_val);
            break;
        case NODE_LIST_LITERAL: {
            const ListLiteralNode *literal = &node->data.list_literal;
            result.type = RET_LIST;
            result.value.list_val.element_type = literal->element_type;
            result.value.list_val.nested_element_type = literal->nested_element_type;
            result.value.list_val.is_nested = literal->is_nested;
            result.value.list_val.head = NULL;

            ListNode *current = NULL;

            if (literal->is_nested) {
                // Handle nested list elements
                for (int i = 0; i < literal->element_count; ++i) {
                    ReturnValue nested_list_val = evaluate_expression(literal->elements[i], scope, ret_ctx);

                    if (nested_list_val.type != RET_LIST) {
                        fprintf(stderr, "\nError: Expected list in nested list literal, got return type %d.\n",
                                nested_list_val.type);
                        free_list(result.value.list_val.head);
                        free_return_value(nested_list_val.type, &nested_list_val);
                        result.type = RET_NONE;
                        return result;
                    }

                    ListElement element;
                    element.type = VAR_LIST;
                    element.value.nested_list.element_type = nested_list_val.value.list_val.element_type;
                    element.value.nested_list.nested_element_type = nested_list_val.value.list_val.nested_element_type;
                    element.value.nested_list.is_nested = nested_list_val.value.list_val.is_nested;
                    element.value.nested_list.head = deep_copy_list(nested_list_val.value.list_val.head);

                    // Free the evaluated nested list since we've copied it
                    free_return_value(nested_list_val.type, &nested_list_val);

                    ListNode *new_node = create_list_node(element);
                    if (!new_node) {
                        fprintf(stderr, "\nError: Failed to allocate memory for list node in literal.\n");
                        free_list(result.value.list_val.head);
                        result.type = RET_NONE;
                        return result;
                    }

                    if (result.value.list_val.head == NULL) {
                        result.value.list_val.head = new_node;
                        current = result.value.list_val.head;
                    } else if (current) {
                        current->next = new_node;
                        current = new_node;
                    }
                }
            } else {
                // Handle regular list elements (primitive types)
                for (int i = 0; i < literal->element_count; ++i) {
                    ListElement element;
                    element.type = literal->element_type;

                    if (element.type == VAR_NUM) {
                        ReturnValue value_val = evaluate_expression(literal->elements[i], scope, ret_ctx);
                        if (value_val.type != RET_NUM) {
                            fprintf(stderr, "\nError: Expected number in list literal, got return type %d.\n",
                                    value_val.type);
                            free_list(result.value.list_val.head);
                            free_return_value(value_val.type, &value_val);
                            result.type = RET_NONE;
                            return result;
                        }
                        element.value.num_val = value_val.value.num_val;
                    } else if (element.type == VAR_STR) {
                        ReturnValue value_val = evaluate_expression(literal->elements[i], scope, ret_ctx);
                        if (value_val.type != RET_STR) {
                            fprintf(stderr, "\nError: Expected string in list literal, got return type %d.\n",
                                    value_val.type);
                            free_list(result.value.list_val.head);
                            free_return_value(value_val.type, &value_val);
                            result.type = RET_NONE;
                            return result;
                        }
                        element.value.str_val = strdup(value_val.value.str_val);
                        free_return_value(value_val.type, &value_val);
                    } else {
                        fprintf(stderr, "\nError: Unsupported element type %d in list literal.\n", element.type);
                        free_list(result.value.list_val.head);
                        result.type = RET_NONE;
                        return result;
                    }

                    ListNode *new_node = create_list_node(element);
                    if (!new_node) {
                        fprintf(stderr, "\nError: Failed to allocate memory for list node in literal.\n");
                        free_list(result.value.list_val.head);
                        if (element.type == VAR_STR) free(element.value.str_val);
                        result.type = RET_NONE;
                        return result;
                    }

                    if (result.value.list_val.head == NULL) {
                        result.value.list_val.head = new_node;
                        current = result.value.list_val.head;
                    } else if (current) {
                        current->next = new_node;
                        current = new_node;
                    }
                }
            }
            break;
        }
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
                    result.value.str_val = strdup(variable->value.str_val);
                    break;
                case VAR_LIST:
                    result.type = RET_LIST;
                    result.value.list_val.element_type = variable->value.list_val.element_type;
                    result.value.list_val.nested_element_type = variable->value.list_val.nested_element_type;
                    result.value.list_val.is_nested = variable->value.list_val.is_nested;
                    result.value.list_val.head = deep_copy_list(variable->value.list_val.head);
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
                    fprintf(stderr, "\nError: Cannot apply unary minus to non-numeric return value.\n");
                    exit(EXIT_FAILURE);
                }
            }

            switch (unary_node->op) {
                case OP_NEGATE:
                    result.type = RET_NUM;
                    result.value.num_val = -operand_value.value.num_val;
                    break;
                case OP_NOT:
                    result.type = RET_NUM;
                    result.value.num_val = !is_truthy(operand_value);
                    break;
                default:
                    fprintf(stderr, "\nError: Unknown unary operator %d.\n", unary_node->op);
                    exit(EXIT_FAILURE);
            }
            break;
        }
        case NODE_EXPR_BINARY: {
            ReturnContext left_eval_ctx = { .is_return = 0, .ret_val.type = RET_NONE };
            ReturnContext right_eval_ctx = { .is_return = 0, .ret_val.type = RET_NONE };
            ReturnValue left_val = evaluate_expression(node->data.binary_expr.left, scope, &left_eval_ctx);
            if (node->data.binary_expr.op == OP_LOGICAL_AND && !is_truthy(left_val)) {
                result.type = RET_NUM;
                result.value.num_val = 0.0;
                free_return_value(left_val.type, &left_val);
                break;
            }
            if (node->data.binary_expr.op == OP_LOGICAL_OR && is_truthy(left_val)) {
                result.type = RET_NUM;
                result.value.num_val = 1.0;
                free_return_value(left_val.type, &left_val);
                break;
            }

            ReturnValue right_val = evaluate_expression(node->data.binary_expr.right, scope, &right_eval_ctx);
            if (node->data.binary_expr.op != OP_LOGICAL_AND && node->data.binary_expr.op != OP_LOGICAL_OR &&
                node->data.binary_expr.op != OP_LOGICAL_XOR)
            {
                 if (left_val.type != RET_NUM || right_val.type != RET_NUM) {
                      fprintf(stderr, "\nError: Binary operator %d requires numeric operands (got types %d and %d).\n",
                              node->data.binary_expr.op, left_val.type, right_val.type);
                      free_return_value(left_val.type, &left_val);
                      free_return_value(right_val.type, &right_val);
                      exit(EXIT_FAILURE);
                  }
            }

            double num_result;
            result.type = RET_NUM;

            switch (node->data.binary_expr.op) {
                case OP_PLUS:
                    num_result = left_val.value.num_val + right_val.value.num_val;
                    break;
                case OP_MINUS:
                    num_result = left_val.value.num_val - right_val.value.num_val;
                    break;
                case OP_MULTIPLY:
                    num_result = left_val.value.num_val * right_val.value.num_val;
                    break;
                case OP_DIVIDE:
                    if (right_val.value.num_val == 0) {
                        fprintf(stderr, "\nError: Division by zero\n"); exit(EXIT_FAILURE);
                    }
                    num_result = left_val.value.num_val / right_val.value.num_val;
                    break;
                case OP_MODULO:
                     if ((long long) right_val.value.num_val == 0) {
                         fprintf(stderr, "\nError: Modulo by zero\n"); exit(EXIT_FAILURE);
                     }
                     num_result = (double)((long long)left_val.value.num_val % (long long)right_val.value.num_val);
                    break;
                case OP_POWER:
                    num_result = pow(left_val.value.num_val, right_val.value.num_val);
                    break;
                case OP_BITWISE_AND:
                    num_result = (double) ((long long) left_val.value.num_val & (long long) right_val.value.num_val);
                    break;
                case OP_BITWISE_OR:
                    num_result = (double) ((long long) left_val.value.num_val | (long long) right_val.value.num_val);
                    break;
                case OP_BITWISE_XOR:
                    num_result = (double) ((long long) left_val.value.num_val ^ (long long) right_val.value.num_val);
                    break;
                case OP_LESS:
                    num_result = left_val.value.num_val < right_val.value.num_val;
                    break;
                case OP_GREATER:
                    num_result = left_val.value.num_val > right_val.value.num_val;
                    break;
                case OP_EQUAL:
                    num_result = left_val.value.num_val == right_val.value.num_val;
                    break;
                case OP_NOT_EQUAL:
                    num_result = left_val.value.num_val != right_val.value.num_val;
                    break;
                case OP_LESS_EQUAL:
                    num_result = left_val.value.num_val <= right_val.value.num_val;
                    break;
                case OP_GREATER_EQUAL:
                    num_result = left_val.value.num_val >= right_val.value.num_val;
                    break;
                case OP_LOGICAL_AND:
                case OP_LOGICAL_OR:
                    num_result = is_truthy(right_val);
                    break;
                case OP_LOGICAL_XOR:
                    num_result = is_truthy(left_val) != is_truthy(right_val);
                    break;
                default:
                    fprintf(stderr, "\nError: Unknown binary operator %d.\n", node->data.binary_expr.op);
                    free_return_value(left_val.type, &left_val);
                    free_return_value(right_val.type, &right_val);
                    exit(EXIT_FAILURE);
            }
            result.value.num_val = num_result;
             free_return_value(left_val.type, &left_val);
             free_return_value(right_val.type, &right_val);
            break;
        }


        case NODE_FUNC_CALL: {
             ReturnContext call_ctx = { .is_return = 0, .ret_val = { .type = RET_NONE } };
             execute_func_call(&node->data.func_call, scope, &call_ctx);
             if (!call_ctx.is_return || call_ctx.ret_val.type == RET_NONE) {
                 fprintf(stderr, "\nError: Function call returned no value.\n");
                 exit(EXIT_FAILURE);
             }

             result.type = call_ctx.ret_val.type;
             result.value = call_ctx.ret_val.value;
             break;
         }

        case NODE_VARIABLE_ACCESS:
            result = evaluate_variable_access(&node->data.variable_access, scope, ret_ctx);
            break;
        case NODE_EXPR_POSTFIX: {
            const PostfixExprNode *postfix_node = &node->data.postfix_expr;
            Variable *variable = get_variable(scope, postfix_node->var_name);

            if (!variable) {
                fprintf(stderr, "\nError: Undefined variable '%s' in postfix operation.\n", postfix_node->var_name);
                exit(EXIT_FAILURE);
            }

            if (variable->type != VAR_NUM) {
                fprintf(stderr, "\nError: Cannot apply postfix operator to non-numeric variable '%s'.\n", postfix_node->var_name);
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
        case NODE_IMPORT: {
            const Module *module = node->data.import.module;
            hashmap_set(imported_modules, &module);
            break;
        }
        default:
            fprintf(stderr, "\nError: Unknown node type: %d.\n", node->type);
            exit(EXIT_FAILURE);
    }
    return result;
}

void execute_var_decl(const VarDeclNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
    ReturnContext init_eval_ctx = { .is_return = 0, .ret_val.type = RET_NONE };
    ReturnValue initial_value_result = evaluate_expression(node->init_expr, scope, &init_eval_ctx);

    if (init_eval_ctx.is_return) {
        ret_ctx->is_return = 1;
        ret_ctx->ret_val = init_eval_ctx.ret_val;
         free_return_value(initial_value_result.type, &initial_value_result);
        return;
    }


    if ((node->type == VAR_NUM || node->type == -1) && initial_value_result.type == RET_NUM) {
        hashmap_set(scope ? scope : variable_map, &(Variable) {
            .name = node->name,
            .type = VAR_NUM,
            .value = { .num_val = initial_value_result.value.num_val }
        });
    } else if (node->type == VAR_STR && initial_value_result.type == RET_STR) {
        char *str_val_copy = NULL;
        if (initial_value_result.value.str_val != NULL) {
            str_val_copy = strdup(initial_value_result.value.str_val);
            if (!str_val_copy) {
                 fprintf(stderr, "\nError: Memory allocation failed duplicating string for variable '%s'.\n", node->name);
                 free_return_value(initial_value_result.type, &initial_value_result);
                 exit(EXIT_FAILURE);
            }
        }
        hashmap_set(scope ? scope : variable_map, &(Variable) {
            .name = node->name,
            .type = VAR_STR,
            .value = { .str_val = str_val_copy }
        });
        free_return_value(initial_value_result.type, &initial_value_result);
    } else {
        fprintf(stderr, "\nError: Type mismatch during declaration of variable '%s'. Expected type %d, but initializer evaluated to type %d.\n",
                node->name, node->type, initial_value_result.type);
        free_return_value(initial_value_result.type, &initial_value_result);
        exit(EXIT_FAILURE);
    }

     if (hashmap_oom(scope ? scope : variable_map)) {
         fprintf(stderr, "\nError: Out of memory while declaring variable '%s'.\n", node->name);
         exit(EXIT_FAILURE);
     }
}

void execute_list_decl(const ListDeclNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
    ListNode *head = NULL;
    ListNode *current = NULL;

    if (node->init_expr && node->init_expr->type == NODE_LIST_LITERAL) {
        const ListLiteralNode *literal = &node->init_expr->data.list_literal;

        // Check that the list types match
        if (literal->element_type != node->element_type) {
            fprintf(stderr, "\nError: List literal type mismatch for variable '%s'. Expected %d, got %d.\n",
                    node->name, node->element_type, literal->element_type);
            exit(EXIT_FAILURE);
        }

        // Handle nested lists
        if (node->element_type == VAR_LIST && literal->is_nested) {
            for (int i = 0; i < literal->element_count; ++i) {
                ListElement element;
                element.type = VAR_LIST;

                // Create a nested list by recursively evaluating the nested list literal
                ReturnValue nested_list_val = evaluate_expression(literal->elements[i], scope, ret_ctx);

                if (nested_list_val.type != RET_LIST) {
                    fprintf(stderr, "\nError: Expected list element %d for list '%s', but got type %d.\n",
                            i, node->name, nested_list_val.type);
                    free_list(head);
                    free_return_value(nested_list_val.type, &nested_list_val);
                    exit(EXIT_FAILURE);
                }

                // Store the nested list in the current list element
                element.value.nested_list.element_type        = nested_list_val.value.list_val.element_type;
                element.value.nested_list.nested_element_type = nested_list_val.value.list_val.nested_element_type;
                element.value.nested_list.is_nested           = nested_list_val.value.list_val.is_nested;
                element.value.nested_list.head                 = deep_copy_list(nested_list_val.value.list_val.head);

                // Free the return value - we've copied its contents
                free_return_value(nested_list_val.type, &nested_list_val);

                // Add the element to our list
                ListNode *new_node = create_list_node(element);
                if (!new_node) {
                    fprintf(stderr, "\nError: Failed to allocate memory for list node for '%s'.\n", node->name);
                    free_list(head);
                    exit(EXIT_FAILURE);
                }

                if (head == NULL) {
                    head = new_node;
                    current = head;
                } else if (current) {
                    current->next = new_node;
                    current = new_node;
                }
            }
        } else {
            // Handle regular lists with primitive elements
            for (int i = 0; i < literal->element_count; ++i) {
                ListElement element;
                element.type = node->element_type;

                if (element.type == VAR_NUM) {
                    element.value.num_val = evaluate_expression(literal->elements[i], scope, ret_ctx).value.num_val;
                } else if (element.type == VAR_STR) {
                    ReturnValue value_val = evaluate_expression(literal->elements[i], scope, ret_ctx);
                    if (value_val.type != RET_STR) {
                        fprintf(stderr, "\nError: Expected string element %d for list '%s', but got type %d.\n",
                                i, node->name, value_val.type);
                        free_list(head);
                        free_return_value(value_val.type, &value_val);
                        exit(EXIT_FAILURE);
                    }
                    element.value.str_val = strdup(value_val.value.str_val);
                    free_return_value(value_val.type, &value_val);
                    if (!element.value.str_val) {
                        fprintf(stderr, "\nError: Failed to evaluate string element %d for list '%s'.\n",
                                i, node->name);
                        free_list(head);
                        exit(EXIT_FAILURE);
                    }
                } else {
                    fprintf(stderr, "Internal Error: Invalid element type %d in list literal for '%s'.\n",
                            element.type, node->name);
                    free_list(head);
                    exit(EXIT_FAILURE);
                }

                ListNode *new_node = create_list_node(element);
                if (!new_node) {
                    fprintf(stderr, "\nError: Failed to allocate memory for list node for '%s'.\n", node->name);
                    free_list(head);
                    if (element.type == VAR_STR) free(element.value.str_val);
                    exit(EXIT_FAILURE);
                }

                if (head == NULL) {
                    head = new_node;
                    current = head;
                } else if (current) {
                    current->next = new_node;
                    current = new_node;
                }
            }
        }
    }

    // Create the variable for the list
    const Variable new_list_var = {
        .name = strdup(node->name),
        .type = VAR_LIST,
        .value = { .list_val = {
            .element_type = node->element_type,
            .nested_element_type = node->nested_element_type,
            .is_nested = node->is_nested_list,
            .head = head
        }}
    };

    if (!new_list_var.name) {
        fprintf(stderr, "\nError: Failed to allocate memory for list variable name '%s'.\n", node->name);
        free_list(head);
        exit(EXIT_FAILURE);
    }

    hashmap_set(scope ? scope : variable_map, &new_list_var);

    if (hashmap_oom(scope ? scope : variable_map)) {
        fprintf(stderr, "\nError: Out of memory while storing list variable '%s'.\n", node->name);
        free(new_list_var.name);
        free_list(head);
        exit(EXIT_FAILURE);
    }
}

char *get_string_value(const ReturnValue value) {
    switch (value.type) {
        case RET_NUM:
            char ret[1024];
            snprintf(ret, sizeof(ret), "%lf", value.value.num_val);
            return strdup(ret);
        case RET_STR:
            return strdup(value.value.str_val);
        case RET_LIST:
            return strdup(list_to_string(value.value.list_val.head, value.value.list_val.element_type));
        default:
            return NULL;
    }
}

void execute_print(const PrintNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
    char *result = strdup("");
    if (!result) {
        fprintf(stderr, "\nError: Memory allocation failed in print.\n");
        return;
    }

    for (int i = 0; i < node->expr_count; i++) {
        ReturnValue value_val = evaluate_expression(node->expr_list[i], scope, ret_ctx);
        value_val.value.str_val = get_string_value(value_val);
        char *part = strdup(value_val.value.str_val);
        free_return_value(value_val.type, &value_val);
        if (!part) {
            fprintf(stderr, "\nError: Failed to get string value for print argument %d.\n", i + 1);
            continue;
        }

        const size_t current_len = strlen(result);
        const size_t part_len = strlen(part);
        char *new_result = safe_realloc(result, current_len + part_len + 1);

        if (!new_result) {
            fprintf(stderr, "\nError: Memory reallocation failed during print concatenation.\n");
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
        case NODE_IMPORT:
            execute_import(&node->data.import);
            break;
        default:
            fprintf(stderr, "Unknown node type: %d\n", node->type);
            break;
    }
}

int evaluate_condition(ASTNode *condition, struct hashmap *scope) {
    if (!condition) return 0;

    switch (condition->type) {
        case NODE_NUM_LITERAL:
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

            if (condition->data.binary_expr.left->type == NODE_NUM_LITERAL) {
                left = condition->data.binary_expr.left->data.num_literal.num_val;
            } else if (condition->data.binary_expr.left->type == NODE_EXPR_VARIABLE) {
                const Variable *variable = get_variable(scope, condition->data.binary_expr.left->data.variable.name);
                if (variable && variable->type == VAR_NUM) left = variable->value.num_val;
            }

            if (condition->data.binary_expr.right->type == NODE_NUM_LITERAL) {
                right = condition->data.binary_expr.right->data.num_literal.num_val;
            } else if (condition->data.binary_expr.right->type == NODE_EXPR_VARIABLE) {
                const Variable *variable = get_variable(scope, condition->data.binary_expr.right->data.variable.name);
                if (variable && variable->type == VAR_NUM) right = variable->value.num_val;
            }

            switch (condition->data.binary_expr.op) {
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
                case OP_LOGICAL_AND:
                    return evaluate_condition(condition->data.binary_expr.left, scope) &&
                        evaluate_condition(condition->data.binary_expr.right, scope);
                case OP_LOGICAL_OR:
                    return evaluate_condition(condition->data.binary_expr.left, scope) ||
                        evaluate_condition(condition->data.binary_expr.right, scope);
                case OP_LOGICAL_XOR:
                    return evaluate_condition(condition->data.binary_expr.left, scope) !=
                        evaluate_condition(condition->data.binary_expr.right, scope);
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
    struct hashmap *if_scope = NULL;

    ReturnContext condition_ctx = { .is_return = 0, .ret_val.type = RET_NONE };
    ReturnValue condition_result = evaluate_expression(if_node->condition, scope, &condition_ctx);

    if (condition_ctx.is_return) {
        ret_ctx->is_return = 1;
        ret_ctx->ret_val = condition_ctx.ret_val;
        return;
    }


    const int condition_is_true = is_truthy(condition_result);
    free_return_value(condition_result.type, &condition_result);

    if (condition_is_true) {
        if_scope = hashmap_new(sizeof(Variable), 0, 0, 0, variable_hash, variable_compare, NULL, NULL);
        execute(if_node->body, if_scope, ret_ctx);
        free_function_scope_lists(if_scope);
        hashmap_free(if_scope);
        return;
    }

    for (int i = 0; i < if_node->elif_count; i++) {
        const ASTNode *elif_node = if_node->elif_nodes[i];
        condition_ctx.is_return = 0;
        condition_ctx.ret_val.type = RET_NONE;

        ReturnValue elif_condition_result = evaluate_expression(elif_node->data.if_stmt.condition, scope, &condition_ctx);

         if (condition_ctx.is_return) {
             ret_ctx->is_return = 1;
             ret_ctx->ret_val = condition_ctx.ret_val;
             return;
         }

        const int elif_condition_is_true = is_truthy(elif_condition_result);
        free_return_value(elif_condition_result.type, &elif_condition_result);

        if (elif_condition_is_true) {
            if_scope = hashmap_new(sizeof(Variable), 0, 0, 0, variable_hash, variable_compare, NULL, NULL);
            execute(elif_node->data.if_stmt.body, if_scope, ret_ctx);
             free_function_scope_lists(if_scope);
            hashmap_free(if_scope);
            return;
        }
    }
    if (if_node->else_body) {
         if_scope = hashmap_new(sizeof(Variable), 0, 0, 0, variable_hash, variable_compare, NULL, NULL);
        execute(if_node->else_body, if_scope, ret_ctx);
         free_function_scope_lists(if_scope);
        hashmap_free(if_scope);
    }
}



void execute_while(const WhileNode *while_node, struct hashmap *scope, ReturnContext *ret_ctx) {
    struct hashmap *while_scope = NULL;

    while (1) {
        ReturnContext condition_ctx = { .is_return = 0, .ret_val.type = RET_NONE };
        ReturnValue condition_result = evaluate_expression(while_node->condition, scope, &condition_ctx);

         if (condition_ctx.is_return) {
             ret_ctx->is_return = 1;
             ret_ctx->ret_val = condition_ctx.ret_val;
             break;
         }


        const int condition_is_true = is_truthy(condition_result);
        free_return_value(condition_result.type, &condition_result);

        if (!condition_is_true) {
            break;
        }

        while_scope = hashmap_new(sizeof(Variable), 0, 0, 0, variable_hash, variable_compare, NULL, NULL);
        if (!while_scope) {
             fprintf(stderr, "Failed to allocate scope for while loop iteration.\n");
             exit(EXIT_FAILURE);
        }

        execute(while_node->body, while_scope, ret_ctx);

         free_function_scope_lists(while_scope);
         hashmap_free(while_scope);
         while_scope = NULL;


        if (ret_ctx->is_return) {
            break;
        }
    }
     free_function_scope_lists(while_scope);
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
                ret_ctx->ret_val.value.str_val = NULL;
                break;
            case RET_LIST:
                result.value.list_val = ret_ctx->ret_val.value.list_val;
                result.type = RET_LIST;
                ret_ctx->ret_val.value.list_val.head = NULL;
                break;
            case RET_NONE:
                break;
            default:
                fprintf(stderr, "Internal \nError: Unknown return type %d in execute_function_body.\n", ret_ctx->ret_val.type);
            exit(EXIT_FAILURE);
        }
    }

    current_return_ctx = node.prev;
    return result;
}

void execute_func_call(const FuncCallNode *func_call, struct hashmap *scope, ReturnContext *caller_ret_ctx) {
    const FunctionMeta *c_func = get_function_meta(imported_modules, func_call->name);
    if (c_func) {
        if (func_call->arg_count != c_func->param_count) {
            fprintf(stderr, "\nError: Function %s expects %d arguments, got %d\n",
                    func_call->name, c_func->param_count, func_call->arg_count);
            exit(EXIT_FAILURE);
        }

        void **args = malloc(sizeof(void *) * c_func->param_count);
        if (!args) {
            fprintf(stderr, "\nError: Memory allocation failed for arguments in function %s\n", func_call->name);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < c_func->param_count; i++) {
            ReturnContext arg_eval_ctx = {0};
            ReturnValue arg_val = evaluate_expression(func_call->arguments[i], scope, &arg_eval_ctx);
            if (arg_eval_ctx.is_return) {
                caller_ret_ctx->is_return = 1;
                caller_ret_ctx->ret_val = arg_eval_ctx.ret_val;
                free(args);
                return;
            }

            DataType param_type = c_func->param_types[i];
            switch (param_type) {
                case TYPE_DOUBLE:
                    if (arg_val.type != RET_NUM) {
                        fprintf(stderr, "\nError: Argument %d to %s must be a number\n", i + 1, func_call->name);
                        free_return_value(arg_val.type, &arg_val);
                        free(args);
                        exit(EXIT_FAILURE);
                    }
                    args[i] = malloc(sizeof(double));
                    *(double *)args[i] = arg_val.value.num_val;
                    break;
                case TYPE_STRING:
                    if (arg_val.type != RET_STR) {
                        fprintf(stderr, "\nError: Argument %d to %s must be a string\n", i + 1, func_call->name);
                        free_return_value(arg_val.type, &arg_val);
                        free(args);
                        exit(EXIT_FAILURE);
                    }
                    args[i] = malloc(sizeof(char *));
                    *(char **)args[i] = strdup(arg_val.value.str_val);
                    break;
                default:
                    fprintf(stderr, "\nError: Unsupported parameter type %d in native function %s\n",
                            param_type, func_call->name);
                    free_return_value(arg_val.type, &arg_val);
                    free(args);
                    exit(EXIT_FAILURE);
            }
            free_return_value(arg_val.type, &arg_val);
        }

        void *ret_out = NULL;
        switch (c_func->ret_type) {
            case TYPE_INT:
                ret_out = malloc(sizeof(int));
                break;
            case TYPE_DOUBLE:
                ret_out = malloc(sizeof(double));
                break;
            case TYPE_STRING:
                ret_out = malloc(sizeof(char *));
                break;
            case TYPE_VOID:
                break;
            default:
                fprintf(stderr, "\nError: Unsupported return type %d for native function %s\n",
                        c_func->ret_type, func_call->name);
                for (int i = 0; i < c_func->param_count; i++) {
                    free(args[i]);
                }
                free(args);
                exit(EXIT_FAILURE);
        }

        c_func->dispatcher(c_func->func, args, ret_out);

        if (ret_out == NULL) {
            fprintf(stderr, "\nError: Standard library function %s "
                            "returned value with unexpected type\n", func_call->name);
            for (int i = 0; i < c_func->param_count; i++) {
                free(args[i]);
            }
            free(args);
            exit(EXIT_FAILURE);
        }

        ReturnValue ret_val;
        switch (c_func->ret_type) {
            case TYPE_INT:
                ret_val.type = RET_NUM;
                ret_val.value.num_val = *(int *) ret_out;
                break;
            case TYPE_DOUBLE:
                ret_val.type = RET_NUM;
                ret_val.value.num_val = *(double *) ret_out;
                break;
            case TYPE_STRING:
                ret_val.type = RET_STR;
                ret_val.value.str_val = strdup(*(char **) ret_out);
                free(*(char **)ret_out);
                break;
            case TYPE_VOID:
                ret_val.type = RET_NONE;
                break;
            default:
                fprintf(stderr, "\nError: Unsupported return type %d for native function %s\n",
                        c_func->ret_type, func_call->name);
                exit(EXIT_FAILURE);
        }

        for (int i = 0; i < c_func->param_count; i++) {
            if (c_func->param_types[i] == TYPE_STRING) {
                free(*(char **)args[i]);
            }
            free(args[i]);
        }
        free(args);
        free(ret_out);

        caller_ret_ctx->is_return = 1;
        caller_ret_ctx->ret_val = ret_val;
        return;
    }

    const Function *function = hashmap_get(function_map, &(Function) { .name = func_call->name });
    if (!function) {
        fprintf(stderr, "Undefined function: %s.\n"
            "Have you forgot to import a module?\n", func_call->name);
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
                         element.value.num_val = evaluate_expression(literal_node->elements[j], scope,
                             &arg_eval_ctx).value.num_val;
                     } else if (list_element_type == VAR_STR) {
                         ReturnValue value_val = evaluate_expression(literal_node->elements[j], scope, &arg_eval_ctx);
                         if (value_val.type != RET_STR) {
                             fprintf(stderr, "Error evaluating string element %d for list literal argument %d "
                                             "in call to '%s'.\n", j, i + 1, func_call->name);
                             free_list(list_to_pass_head);
                             free_function_scope_lists(function_scope);
                             hashmap_free(function_scope);
                             exit(EXIT_FAILURE);
                         }
                         element.value.str_val = strdup(value_val.value.str_val);
                         free_return_value(value_val.type, &value_val);
                         if (!element.value.str_val) {
                             fprintf(stderr, "Error evaluating string element %d for list literal argument %d"
                                             " in call to '%s'.\n",
                                      j, i + 1, func_call->name);
                             free_list(list_to_pass_head);
                             free_function_scope_lists(function_scope);
                             hashmap_free(function_scope);
                             exit(EXIT_FAILURE);
                         }
                     } else {
                          fprintf(stderr, "Internal \nError: Unsupported list element type %d for parameter '%s'"
                                          " in function '%s'.\n",
                                  list_element_type, param->name, func_call->name);
                         free_list(list_to_pass_head);
                         free_function_scope_lists(function_scope);
                         hashmap_free(function_scope);
                         exit(EXIT_FAILURE);
                     }

                     ListNode *new_node = create_list_node(element);
                      if (!new_node) {
                           fprintf(stderr, "\nError: Failed to allocate memory for list node from literal for function '%s'.\n", func_call->name);
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
                     fprintf(stderr, "\nError: List variable '%s' (argument %d for function '%s') not found in caller scope.\n",
                              list_arg_name, i + 1, func_call->name);
                     free_function_scope_lists(function_scope);
                     hashmap_free(function_scope);
                     exit(EXIT_FAILURE);
                 }
                 if (list_var_caller->type != VAR_LIST) {
                      fprintf(stderr, "\nError: Variable '%s' passed as argument %d to function '%s' is not a list.\n",
                              list_arg_name, i + 1, func_call->name);
                      free_function_scope_lists(function_scope);
                      hashmap_free(function_scope);
                      exit(EXIT_FAILURE);
                 }
                 if (list_var_caller->value.list_val.element_type != list_element_type) {
                      fprintf(stderr, "\nError: Type mismatch for list argument %d ('%s') for function '%s'. Expected list of type %d, got %d.\n",
                              i + 1, list_arg_name, func_call->name, list_element_type, list_var_caller->value.list_val.element_type);
                      free_function_scope_lists(function_scope);
                      hashmap_free(function_scope);
                      exit(EXIT_FAILURE);
                 }
                 list_to_pass_head = deep_copy_list(list_var_caller->value.list_val.head);
                 if (!list_to_pass_head && list_var_caller->value.list_val.head != NULL) {
                       fprintf(stderr, "\nError: Failed to copy list argument %d for function '%s'.\n", i + 1, func_call->name);
                       free_function_scope_lists(function_scope);
                       hashmap_free(function_scope);
                       exit(EXIT_FAILURE);
                 }
            } else {
                 fprintf(stderr, "\nError: Invalid argument type (%d) for list parameter '%s' in function '%s'. Expected list variable or literal.\n",
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
                 fprintf(stderr, "\nError: Failed to allocate memory for parameter name '%s'.\n", param->name);
                 free_list(list_to_pass_head);
                 free_function_scope_lists(function_scope);
                 hashmap_free(function_scope);
                 exit(EXIT_FAILURE);
            }

            hashmap_set(function_scope, &function_param_var);
            if (hashmap_oom(function_scope)) {
                  fprintf(stderr, "\nError: Out of memory setting list parameter '%s'.\n", param->name);
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
            ReturnValue value_val = evaluate_expression(arg_node, scope, &arg_eval_ctx);
            if (value_val.type != RET_STR) {
                fprintf(stderr, "\nError: Invalid type (%d) for string parameter '%s' in function '%s'."
                                " Expected string.\n", value_val.type, param->name, func_call->name);
                free_return_value(value_val.type, &value_val);
                free_function_scope_lists(function_scope);
                hashmap_free(function_scope);
                exit(EXIT_FAILURE);
            }
            char *value = strdup(value_val.value.str_val);
            free_return_value(value_val.type, &value_val);
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
                    fprintf(stderr, "\nError: Failed to duplicate string during return.\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                ret_ctx->ret_val.value.str_val = NULL;
            }
            break;

        case RET_LIST:
            if (temp.ret_val.value.list_val.head) {
                ret_ctx->ret_val.value.list_val.head = deep_copy_list(temp.ret_val.value.list_val.head);
                if (!ret_ctx->ret_val.value.list_val.head) {
                    fprintf(stderr, "\nError: Failed to copy list during return.\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                ret_ctx->ret_val.value.list_val.head = NULL;
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
    // Handle direct variable assignment (no indexing)
    if (node->index_expr == NULL && node->target_access == NULL) {
        Variable *existing_var = get_variable(scope, node->target_name);

        if (!existing_var) {
            fprintf(stderr, "\nError: Variable '%s' not declared before assignment.\n", node->target_name);
            exit(EXIT_FAILURE);
        }

        ReturnContext value_eval_ctx = {0};
        if (existing_var->type == VAR_NUM) {
            const double value_to_assign = evaluate_expression(node->value_expr, scope, &value_eval_ctx).value.num_val;
            existing_var->value.num_val = value_to_assign;
        } else if (existing_var->type == VAR_STR) {
            ReturnValue value_val = evaluate_expression(node->value_expr, scope, &value_eval_ctx);
            if (value_val.type != RET_STR) {
                fprintf(stderr, "\nError: Failed to evaluate string value for assignment to variable '%s'.\n",
                    node->target_name);
                exit(EXIT_FAILURE);
            }
            char *value_to_assign_str = strdup(value_val.value.str_val);
            free_return_value(value_val.type, &value_val);
            if (!value_to_assign_str) {
                fprintf(stderr, "\nError: Failed to evaluate string value for assignment to variable '%s'.\n",
                    node->target_name);
                exit(EXIT_FAILURE);
            }
            if (existing_var->value.str_val != NULL) {
                free(existing_var->value.str_val);
            }
            existing_var->value.str_val = value_to_assign_str;
        } else if (existing_var->type == VAR_LIST) {
            fprintf(stderr, "\nError: Cannot assign directly to a list variable '%s' using '='."
                " Use list declaration or modify elements.\n", node->target_name);
            exit(EXIT_FAILURE);
        } else {
            fprintf(stderr, "Internal \nError: Assignment to variable '%s' with unknown type.\n",
                node->target_name);
            exit(EXIT_FAILURE);
        }
        return;
    }

    // Handle nested assignment (through the target_access field)
    if (node->target_name == NULL && node->target_access != NULL) {
        // Get the underlying expression to evaluate
        ASTNode *access_expr = node->target_access;

        // We need to follow the chain through parent_expr fields
        // But we need to find the base variable first
        ASTNode *current = access_expr;
        while (current->data.variable_access.name == NULL &&
               current->data.variable_access.parent_expr != NULL) {
            current = current->data.variable_access.parent_expr;
        }

        // Now 'current' should point to the base variable access node
        if (current->data.variable_access.name == NULL) {
            fprintf(stderr, "\nInternal Error: Invalid nested access chain.\n");
            exit(EXIT_FAILURE);
        }

        // Get the variable
        Variable *var = get_variable(scope, current->data.variable_access.name);
        if (!var) {
            fprintf(stderr, "\nError: Variable '%s' not found for assignment.\n",
                    current->data.variable_access.name);
            exit(EXIT_FAILURE);
        }

        // Make sure it's a list
        if (var->type != VAR_LIST) {
            fprintf(stderr, "\nError: Cannot index into non-list variable '%s'.\n",
                    current->data.variable_access.name);
            exit(EXIT_FAILURE);
        }

        // Build a path of indices to follow
        int max_depth = 16;  // Reasonable maximum nesting depth
        int indices[max_depth];
        int depth = 0;

        // Walk back through the chain, collecting indices
        ASTNode *idx_node = access_expr;
        while (idx_node != NULL && depth < max_depth) {
            // Evaluate the index at this level
            ReturnContext idx_ctx = {0};
            double idx_val = evaluate_expression(idx_node->data.variable_access.index_expr,
                                                scope, &idx_ctx).value.num_val;

            if (idx_val != floor(idx_val)) {
                fprintf(stderr, "\nError: List index must be an integer, got %f.\n", idx_val);
                exit(EXIT_FAILURE);
            }

            indices[depth++] = (int)idx_val;

            // Move to the parent (actually going in reverse, from innermost to outermost)
            if (idx_node->data.variable_access.parent_expr == NULL) {
                idx_node = NULL;
            } else {
                idx_node = idx_node->data.variable_access.parent_expr;
            }
        }

        // We collected indices from innermost to outermost, so reverse them
        for (int i = 0; i < depth / 2; i++) {
            int temp = indices[i];
            indices[i] = indices[depth - 1 - i];
            indices[depth - 1 - i] = temp;
        }

        // Now follow the path of indices to find the element to assign
        ListNode *list_ptr = var->value.list_val.head;
        VarType current_type = var->value.list_val.element_type;
        VarType nested_type = var->value.list_val.nested_element_type;
        bool is_nested = var->value.list_val.is_nested;

        // For all but the last index, navigate through the nested lists
        for (int i = 0; i < depth - 1; i++) {
            int idx = indices[i];

            // Count the list size at this level
            int list_size = 0;
            ListNode *counter = list_ptr;
            while (counter != NULL) {
                list_size++;
                counter = counter->next;
            }

            // Handle negative indexing
            if (idx < 0) idx += list_size;

            // Check bounds
            if (idx < 0 || idx >= list_size) {
                fprintf(stderr, "\nError: List index %d out of bounds for list of size %d.\n",
                        idx, list_size);
                exit(EXIT_FAILURE);
            }

            // Navigate to the element
            ListNode *target = list_ptr;
            for (int j = 0; j < idx; j++) {
                target = target->next;
            }

            // Make sure this element is a nested list
            if (target->element.type != VAR_LIST) {
                fprintf(stderr, "\nError: Cannot index into non-list element.\n");
                exit(EXIT_FAILURE);
            }

            // Update for next iteration
            list_ptr = target->element.value.nested_list.head;
            current_type = target->element.value.nested_list.element_type;
            nested_type = target->element.value.nested_list.nested_element_type;
            is_nested = target->element.value.nested_list.is_nested;
        }

        // Process the final index (actual assignment)
        int final_idx = indices[depth - 1];

        // Count the list size at the final level
        int list_size = 0;
        ListNode *counter = list_ptr;
        while (counter != NULL) {
            list_size++;
            counter = counter->next;
        }

        // Handle negative indexing
        if (final_idx < 0) final_idx += list_size;

        // Check bounds
        if (final_idx < 0 || final_idx >= list_size) {
            fprintf(stderr, "\nError: List index %d out of bounds for list of size %d.\n",
                    final_idx, list_size);
            exit(EXIT_FAILURE);
        }

        // Navigate to the target element
        ListNode *target = list_ptr;
        for (int j = 0; j < final_idx; j++) {
            target = target->next;
        }

        // Evaluate the value to assign
        ReturnContext value_ctx = {0};
        ReturnValue value = evaluate_expression(node->value_expr, scope, &value_ctx);

        // Assign based on the element type
        if (target->element.type == VAR_NUM) {
            if (value.type != RET_NUM) {
                fprintf(stderr, "\nError: Cannot assign non-numeric value to numeric list element.\n");
                exit(EXIT_FAILURE);
            }
            target->element.value.num_val = value.value.num_val;
        } else if (target->element.type == VAR_STR) {
            if (value.type != RET_STR) {
                fprintf(stderr, "\nError: Cannot assign non-string value to string list element.\n");
                exit(EXIT_FAILURE);
            }
            if (target->element.value.str_val != NULL) {
                free(target->element.value.str_val);
            }
            target->element.value.str_val = strdup(value.value.str_val);
        } else if (target->element.type == VAR_LIST) {
            if (value.type != RET_LIST) {
                fprintf(stderr, "\nError: Cannot assign non-list value to nested list element.\n");
                exit(EXIT_FAILURE);
            }
            // Free the existing nested list
            free_list(target->element.value.nested_list.head);
            // Copy the new list
            target->element.value.nested_list.head = deep_copy_list(value.value.list_val.head);
            target->element.value.nested_list.element_type = value.value.list_val.element_type;
            target->element.value.nested_list.nested_element_type = value.value.list_val.nested_element_type;
            target->element.value.nested_list.is_nested = value.value.list_val.is_nested;
        }

        // Clean up
        free_return_value(value.type, &value);
        return;
    }

    // Handle single-index assignment (list[index] = value)
    Variable *var = get_variable(scope, node->target_name);
    if (!var) {
        fprintf(stderr, "\nError: Variable '%s' not found for assignment.\n", node->target_name);
        exit(EXIT_FAILURE);
    }

    // Make sure the variable is a list
    if (var->type != VAR_LIST) {
        fprintf(stderr, "\nError: Assignment to non-list variable '%s' with index.\n", node->target_name);
        exit(EXIT_FAILURE);
    }

    // Evaluate the index expression
    ReturnContext index_eval_ctx = {0};
    double index_val_double = evaluate_expression(node->index_expr, scope, &index_eval_ctx).value.num_val;
    if (index_val_double != floor(index_val_double)) {
        fprintf(stderr, "\nError: List index for '%s' must be an integer, got %f.\n",
                node->target_name, index_val_double);
        exit(EXIT_FAILURE);
    }

    // Get the list size
    int list_size = 0;
    ListNode *counter_node = var->value.list_val.head;
    while (counter_node != NULL) {
        list_size++;
        counter_node = counter_node->next;
    }

    // Convert the index to an integer
    int index = (int)index_val_double;
    // Handle negative indices (Python style)
    if (index < 0) index += list_size;
    // Check bounds
    if (index < 0 || index >= list_size) {
        fprintf(stderr, "\nError: List index %d out of bounds for list '%s' of size %d.\n",
                index, node->target_name, list_size);
        exit(EXIT_FAILURE);
    }

    // Traverse to the target node
    ListNode *current = var->value.list_val.head;
    for (int i = 0; i < index; ++i) {
        current = current->next;
    }

    // Evaluate the value to assign
    ReturnContext value_eval_ctx = {0};
    ReturnValue value_val = evaluate_expression(node->value_expr, scope, &value_eval_ctx);

    // Handle nested lists
    if (current->element.type == VAR_LIST) {
        // For nested list assignment, we need a different approach
        // Check if we're assigning to a nested list element
        if (value_val.type == RET_LIST) {
            // Free the existing nested list
            free_list(current->element.value.nested_list.head);
            // Copy the new list to the element
            current->element.value.nested_list.head = deep_copy_list(value_val.value.list_val.head);
            current->element.value.nested_list.element_type = value_val.value.list_val.element_type;
            current->element.value.nested_list.nested_element_type = value_val.value.list_val.nested_element_type;
            current->element.value.nested_list.is_nested = value_val.value.list_val.is_nested;
        } else {
            fprintf(stderr, "\nError: Cannot assign non-list value to nested list element.\n");
            exit(EXIT_FAILURE);
        }
    } else {
        // Handle primitive element assignment
        if (var->value.list_val.element_type == VAR_NUM) {
            if (value_val.type != RET_NUM) {
                fprintf(stderr, "\nError: Type mismatch in assignment to list '%s'.\n", node->target_name);
                exit(EXIT_FAILURE);
            }
            current->element.value.num_val = value_val.value.num_val;
        } else if (var->value.list_val.element_type == VAR_STR) {
            if (value_val.type != RET_STR) {
                fprintf(stderr, "\nError: Type mismatch in assignment to list '%s'.\n", node->target_name);
                exit(EXIT_FAILURE);
            }
            free(current->element.value.str_val);
            current->element.value.str_val = strdup(value_val.value.str_val);
        } else {
            fprintf(stderr, "Internal Error: Unknown list element type for '%s'.\n", node->target_name);
            exit(EXIT_FAILURE);
        }
    }
    free_return_value(value_val.type, &value_val);
}

ReturnValue evaluate_str_access(const VariableAccessNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
    Variable *str_var = get_variable(scope, node->name);

    if (!str_var) {
        fprintf(stderr, "\nError: Undefined variable '%s'.\n", node->name);
        exit(EXIT_FAILURE);
    }

    if (str_var->type != VAR_STR) {
        fprintf(stderr, "\nError: Cannot index a non-string variable '%s'.\n", node->name);
        exit(EXIT_FAILURE);
    }

    // Get the string value and its length
    char *str = str_var->value.str_val;
    int str_len = strlen(str);

    // Evaluate the index expression
    ReturnValue index_val = evaluate_expression(node->index_expr, scope, ret_ctx);

    if (index_val.type != RET_NUM) {
        fprintf(stderr, "\nError: String index must be a number.\n");
        exit(EXIT_FAILURE);
    }

    // Convert the index to integer
    int index = (int)index_val.value.num_val;

    // Handle negative indices (Python-like behavior)
    if (index < 0) {
        index = str_len + index;
    }

    // Check bounds
    if (index < 0 || index >= str_len) {
        fprintf(stderr, "\nError: String index out of range.\n");
        exit(EXIT_FAILURE);
    }

    // Create a single character string result
    ReturnValue result;
    result.type = RET_STR;
    result.value.str_val = safe_malloc(2);
    result.value.str_val[0] = str[index];
    result.value.str_val[1] = '\0';

    return result;
}

ReturnValue evaluate_variable_access(const VariableAccessNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
    ReturnValue result = {0};

    // Handle nested access (when name is NULL)
    if (node->name == NULL && node->parent_expr != NULL) {
        // Evaluate the parent expression first (which could be another nested access)
        ReturnValue parent_val = evaluate_expression(node->parent_expr, scope, ret_ctx);

        // Parent must be a list
        if (parent_val.type != RET_LIST) {
            fprintf(stderr, "\nError: Cannot index into non-list value.\n");
            exit(EXIT_FAILURE);
        }

        // Evaluate the index expression
        ReturnContext index_ctx = {0};
        double index_val_double = evaluate_expression(node->index_expr, scope, &index_ctx).value.num_val;

        // Index must be an integer
        if (index_val_double != floor(index_val_double)) {
            fprintf(stderr, "\nError: List index must be an integer, got %f.\n", index_val_double);
            exit(EXIT_FAILURE);
        }

        int index = (int)index_val_double;

        // Count the list size
        int list_size = 0;
        ListNode *counter_node = parent_val.value.list_val.head;
        while (counter_node != NULL) {
            list_size++;
            counter_node = counter_node->next;
        }

        // Handle negative indices (Python-like)
        if (index < 0) index += list_size;

        // Check bounds
        if (index < 0 || index >= list_size) {
            fprintf(stderr, "\nError: List index %d out of bounds for list of size %d.\n",
                    index, list_size);
            exit(EXIT_FAILURE);
        }

        // Traverse to the target node
        ListNode *current = parent_val.value.list_val.head;
        for (int i = 0; i < index; ++i) {
            current = current->next;
        }

        // Handle different element types
        if (current->element.type == VAR_NUM) {
            result.type = RET_NUM;
            result.value.num_val = current->element.value.num_val;
        } else if (current->element.type == VAR_STR) {
            result.type = RET_STR;
            result.value.str_val = strdup(current->element.value.str_val);
        } else if (current->element.type == VAR_LIST) {
            // This is a nested list
            result.type = RET_LIST;
            result.value.list_val.element_type = current->element.value.nested_list.element_type;
            result.value.list_val.nested_element_type = current->element.value.nested_list.nested_element_type;
            result.value.list_val.is_nested = current->element.value.nested_list.is_nested;
            result.value.list_val.head = deep_copy_list(current->element.value.nested_list.head);
        }

        // Clean up the parent value
        free_return_value(parent_val.type, &parent_val);

        return result;
    }

    // Handle direct variable access (first level)
    Variable *var = get_variable(scope, node->name);
    if (!var) {
        fprintf(stderr, "\nError: Variable '%s' not found.\n", node->name);
        exit(EXIT_FAILURE);
    }

    // Handle string access
    if (var->type == VAR_STR) {
        return evaluate_str_access(node, scope, ret_ctx);
    }

    // Make sure it's a list
    if (var->type != VAR_LIST) {
        fprintf(stderr, "\nError: Cannot index into non-list variable '%s'.\n", node->name);
        exit(EXIT_FAILURE);
    }

    ReturnContext index_ctx = {0};
    double index_val_double = evaluate_expression(node->index_expr, scope, &index_ctx).value.num_val;

    // Index must be an integer
    if (index_val_double != floor(index_val_double)) {
        fprintf(stderr, "\nError: List index for '%s' must be an integer, got %f.\n",
                node->name, index_val_double);
        exit(EXIT_FAILURE);
    }

    int index = (int)index_val_double;

    int list_size = 0;
    ListNode *counter_node = var->value.list_val.head;
    while (counter_node != NULL) {
        list_size++;
        counter_node = counter_node->next;
    }

    if (index < 0) index += list_size;

    // Check bounds
    if (index < 0 || index >= list_size) {
        fprintf(stderr, "\nError: List index %d out of bounds for list '%s' of size %d.\n",
                index, node->name, list_size);
        exit(EXIT_FAILURE);
    }

    // Find the indexed element
    ListNode *current = var->value.list_val.head;
    for (int i = 0; i < index; ++i) {
        current = current->next;
    }

    // Return the appropriate value based on element type
    if (current->element.type == VAR_NUM) {
        result.type = RET_NUM;
        result.value.num_val = current->element.value.num_val;
    } else if (current->element.type == VAR_STR) {
        result.type = RET_STR;
        result.value.str_val = strdup(current->element.value.str_val);
    } else if (current->element.type == VAR_LIST) {
        // This is a nested list
        result.type = RET_LIST;
        result.value.list_val.element_type = current->element.value.nested_list.element_type;
        result.value.list_val.nested_element_type = current->element.value.nested_list.nested_element_type;
        result.value.list_val.is_nested = current->element.value.nested_list.is_nested;
        result.value.list_val.head = deep_copy_list(current->element.value.nested_list.head);
    }

    return result;
}

void execute_import(const ImportNode *node) {
    Module *module = node->module;
    if (!module) {
        fprintf(stderr, "\nError: Module import failed. Module '%s' not found.\n", module->name);
        exit(EXIT_FAILURE);
    }
    hashmap_set(imported_modules, module);
}
