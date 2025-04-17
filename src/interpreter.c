#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "interpreter.h"

#include <setjmp.h>

#include "variables.h"
#include "functions.h"
#include "memory.h"

typedef struct {
    struct hashmap *map_to_clean;
} CleanupData;

jmp_buf *current_jmp_buf = NULL;

ReturnContextNode* current_return_ctx = NULL;

double return_value = 0.0;

static void free_function_scope_lists(struct hashmap *scope) {
    if (!scope) return;
    size_t i = 0;
    void *item;
    // Create a temporary list of list heads to free
    ListNode **heads_to_free = NULL;
    int count = 0;

    while (hashmap_iter(scope, &i, &item)) {
        const Variable *var = (Variable *)item;
        if (var->type == VAR_LIST && var->value.list_val.head != NULL) {
            // Add head to our temporary list
            heads_to_free = safe_realloc(heads_to_free, (count + 1) * sizeof(ListNode*));
            heads_to_free[count++] = var->value.list_val.head;
            // Important: Set the head in the variable to NULL *after* adding
            // it to the list, so hashmap_free doesn't try to double-free.
            // However, hashmap_free currently has NULL for elfree, so this isn't strictly needed now.
            // var->value.list_val.head = NULL; // If hashmap had elfree, this would be crucial.
        } else if (var->type == VAR_STR && var->value.str_val != NULL) {
            // Free string value allocated during argument passing
            free(var->value.str_val);
            // var->value.str_val = NULL; // If hashmap had elfree
        }
    }

    // Now free the collected list heads
    for (int j = 0; j < count; ++j) {
        free_list(heads_to_free[j]);
    }
    free(heads_to_free); // Free the temporary list itself
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
        case NODE_EXPR_UNARY: {
            const UnaryExprNode *unary_node = &node->data.unary_expr;

            // Recursively evaluate the operand
            // Use a temporary context in case the operand itself involves returns/jumps
            ReturnContext operand_ctx = { .is_return = 0, .type = RET_NONE };
            double operand_value = evaluate_expression(unary_node->operand, scope, &operand_ctx);

            // Handle potential early return from operand evaluation if necessary
            // For simple negation, we usually just need the numeric result.
            if (operand_ctx.is_return) {
                if (operand_ctx.type == RET_NUM) {
                    operand_value = operand_ctx.value.num_val;
                    // Propagate the return status ONLY if your language design requires it here.
                    // For simple expression evaluation, often you don't propagate here.
                    // ret_ctx->is_return = 1;
                    // ret_ctx->type = RET_NUM;
                    // ret_ctx->value.num_val = -operand_value; // Apply op before propagating? Design choice.
                    // return -operand_value; // Or just return the result
                } else {
                    fprintf(stderr, "Error: Cannot apply unary minus to non-numeric return value.\n");
                    exit(EXIT_FAILURE);
                }
            }

            // Apply the unary operation
            switch (unary_node->op) {
                case OP_NEGATE:
                    return -operand_value;
                // case OP_NOT: // Example for later boolean not
                //    return !operand_value; // Requires careful handling of truthiness
                default:
                    fprintf(stderr, "Error: Unknown unary operator %d.\n", unary_node->op);
                exit(EXIT_FAILURE);
            }
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
        case NODE_LIST_ACCESS: {
            const ListAccessNode *access_node = &node->data.list_access;
            Variable *list_var = get_variable(scope, access_node->list_name);

            // Check if the variable exists and is a list (keep existing checks)
            if (!list_var) {
                fprintf(stderr, "Error: List variable '%s' not found.\n", access_node->list_name);
                exit(EXIT_FAILURE);
            }
            if (list_var->type != VAR_LIST) {
                fprintf(stderr, "Error: Variable '%s' is not a list.\n", access_node->list_name);
                exit(EXIT_FAILURE);
            }

            // ---> Add: Calculate list size <---
            int list_size = 0;
            ListNode *counter_node = list_var->value.list_val.head;
            while (counter_node != NULL) {
                list_size++;
                counter_node = counter_node->next;
            }
            // ---> End Add <---


            // Evaluate the index expression (keep existing code)
            ReturnContext index_eval_ctx = { .is_return = 0, .type = RET_NONE };
            double index_val_double = evaluate_expression(access_node->index_expr, scope, &index_eval_ctx);

            // ---> Modify: Index validation and conversion <---
            int index;
            // Check if index is an integer first
            if (index_val_double != floor(index_val_double)) {
                 fprintf(stderr, "Error: List index for '%s' must be an integer, got %f.\n",
                         access_node->list_name, index_val_double);
                 exit(EXIT_FAILURE);
            }

            index = (int)index_val_double; // Cast to int

            // Handle negative index
            if (index < 0) {
                index = list_size + index; // Convert to positive index from the end
            }

            // Bounds check (now unified for positive and converted negative indices)
            if (index < 0 || index >= list_size) {
                fprintf(stderr, "Error: List index %d (calculated from %f) out of bounds for list '%s' of size %d.\n",
                        index, index_val_double, access_node->list_name, list_size);
                exit(EXIT_FAILURE);
            }
            // ---> End Modify <---


            // Traverse the list (use the final 'index' value)
            ListNode *current = list_var->value.list_val.head;
            int count = 0;
            // The loop condition is correct as 'index' is now guaranteed non-negative
            while (count < index) { // No need to check current != NULL here, bounds check covers it
                current = current->next;
                count++;
            }

            // Node must exist due to bounds check unless list_size was 0 (handled by index >= list_size check)
            // if (current == NULL) { ... } // This check is now redundant

            // Return the value (keep existing logic, assuming numeric lists for this function)
            if (list_var->value.list_val.element_type == VAR_NUM) {
                 if (current->element.type == VAR_NUM) {
                    return current->element.value.num_val;
                 } else {
                     fprintf(stderr, "Internal Error: List '%s' element type mismatch at index %d.\n", access_node->list_name, index);
                     exit(EXIT_FAILURE);
                 }
            } else if (list_var->value.list_val.element_type == VAR_STR) {
                 fprintf(stderr, "Error: Accessing string list elements directly within numerical expressions is not yet supported.\n");
                 exit(EXIT_FAILURE);
            } else {
                 fprintf(stderr, "Internal Error: Unknown list element type for '%s'.\n", access_node->list_name);
                 exit(EXIT_FAILURE);
            }
             // Dead code path due to exits above
             // return 0.0;
        }
        case NODE_EXPR_POSTFIX: {
            const PostfixExprNode *postfix_node = &node->data.postfix_expr;
            Variable *variable = get_variable(scope, postfix_node->var_name);

            if (!variable) {
                fprintf(stderr, "Error: Undefined variable '%s' in postfix operation.\n", postfix_node->var_name);
                // Optionally set ret_ctx to an error state or exit
                exit(EXIT_FAILURE); // Or handle error more gracefully
                // return 0.0; // Or return a default value
            }

            if (variable->type != VAR_NUM) {
                fprintf(stderr, "Error: Cannot apply postfix operator to non-numeric variable '%s'.\n", postfix_node->var_name);
                // Optionally set ret_ctx to an error state or exit
                exit(EXIT_FAILURE); // Or handle error more gracefully
                // return 0.0; // Or return a default value
            }

            // Store the original value *before* modification
            double original_value = variable->value.num_val;

            // Modify the variable's value based on the operator
            switch (postfix_node->op) {
                case OP_INC:
                    variable->value.num_val++;
                break;
                case OP_DEC:
                    variable->value.num_val--;
                break;
                // No default needed as enum covers all cases
            }

            // Return the original value
            return original_value;
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

void execute_list_decl(const ListDeclNode *node, struct hashmap *scope, ReturnContext *ret_ctx) {
    ListNode *head = NULL; // Head of the linked list
    ListNode *current = NULL; // Pointer to the last node added

    // Check if there's an initializer (a list literal)
    if (node->init_expr && node->init_expr->type == NODE_LIST_LITERAL) {
        const ListLiteralNode *literal = &node->init_expr->data.list_literal;

        // Ensure literal type matches declaration type
        if (literal->element_type != node->element_type) {
            fprintf(stderr, "Error: List literal type mismatch for variable '%s'. Expected %d, got %d.\n",
                    node->name, node->element_type, literal->element_type);
            // Cleanup might be needed if some nodes were already created
            exit(EXIT_FAILURE);
        }

        // Iterate through the literal elements and build the linked list
        for (int i = 0; i < literal->element_count; ++i) {
            ListElement element;
            element.type = node->element_type; // Should match literal->element_type

            // Evaluate the expression for the current element
            if (element.type == VAR_NUM) {
                element.value.num_val = evaluate_expression(literal->elements[i], scope, ret_ctx);
                 // Check ret_ctx for errors from evaluate_expression?
            } else if (element.type == VAR_STR) {
                 // get_string_value allocates memory, list nodes need to own this
                element.value.str_val = get_string_value(literal->elements[i], scope, ret_ctx);
                if (!element.value.str_val) {
                     fprintf(stderr, "Error: Failed to evaluate string element %d for list '%s'.\n", i, node->name);
                     // Cleanup previously created nodes and strings
                     free_list(head);
                     exit(EXIT_FAILURE);
                 }
            } else {
                // Should not happen based on parser validation
                 fprintf(stderr, "Internal Error: Invalid element type %d in list literal for '%s'.\n", element.type, node->name);
                 free_list(head);
                 exit(EXIT_FAILURE);
            }

            // Create a new list node
            ListNode *new_node = create_list_node(element); // Need to implement create_list_node
             if (!new_node) {
                  fprintf(stderr, "Error: Failed to allocate memory for list node for '%s'.\n", node->name);
                  free_list(head); // Free already created part of the list
                  // Also free the string if just allocated by get_string_value
                  if (element.type == VAR_STR) free(element.value.str_val);
                  exit(EXIT_FAILURE);
             }


            // Append the new node to the list
            if (head == NULL) {
                head = new_node;
                current = head;
            } else {
                current->next = new_node;
                current = new_node;
            }
        }
    } // else: No initializer, head remains NULL (empty list)

    // Create the variable structure
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

    // Check for out-of-memory from hashmap_set?
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
            return strdup("<undefined>");
        }
        case NODE_EXPR_UNARY: {
            // Evaluate the expression first
            ReturnContext temp_ctx = {0}; // Use a clean context
            const double value = evaluate_expression(node, scope, &temp_ctx);
            // Handle potential return from evaluation if necessary (like above)
            // Format the result
            snprintf(buffer, sizeof(buffer), "%g", value);
            return strdup(buffer);
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
        case NODE_LIST_ACCESS: {
            const ListAccessNode *access_node = &node->data.list_access;
            Variable *list_var = get_variable(scope, access_node->list_name);
             if (!list_var || list_var->type != VAR_LIST) {
                 return strdup("<list access error: not found or not a list>"); // More specific error
             }

             // ---> Add: Calculate list size <---
             int list_size = 0;
             ListNode *counter_node = list_var->value.list_val.head;
             while (counter_node != NULL) {
                 list_size++;
                 counter_node = counter_node->next;
             }
             // ---> End Add <---


             // Evaluate index (keep existing code)
             ReturnContext index_eval_ctx = { .is_return = 0, .type = RET_NONE };
             double index_val_double = evaluate_expression(access_node->index_expr, scope, &index_eval_ctx);


             // ---> Modify: Index validation and conversion <---
             int index;
             // Check if index is an integer first
             if (index_val_double != floor(index_val_double)) {
                  snprintf(buffer, sizeof(buffer), "<invalid index: non-integer %f>", index_val_double);
                  return strdup(buffer);
             }
             index = (int)index_val_double;

             // Handle negative index
             if (index < 0) {
                 index = list_size + index; // Convert to positive index from the end
             }

             // Bounds check
             if (index < 0 || index >= list_size) {
                 snprintf(buffer, sizeof(buffer), "<index %d (from %f) out of bounds for size %d>", index, index_val_double, list_size);
                 return strdup(buffer);
             }
             // ---> End Modify <---


             // Traverse the list (use the final 'index' value)
             ListNode *current = list_var->value.list_val.head;
             int count = 0;
             while (count < index) {
                 current = current->next;
                 count++;
             }

             // if (current == NULL) { ... } // Redundant due to bounds check

             // Convert element to string (keep existing logic)
             if (current->element.type == VAR_NUM) {
                 snprintf(buffer, sizeof(buffer), "%g", current->element.value.num_val);
                 return strdup(buffer);
             } else if (current->element.type == VAR_STR) {
                 return strdup(current->element.value.str_val ? current->element.value.str_val : "");
             } else {
                 return strdup("<unknown list element type>");
             }
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
                                                  variable_hash, variable_compare,
                                                  NULL, NULL);
     if (!function_scope) {
         fprintf(stderr, "Failed to allocate scope for function %s\n", func_call->name);
         exit(EXIT_FAILURE);
     }


    // Process arguments and populate function scope
    for (int i = 0; i < func_call->arg_count; i++) {
        const Parameter *param = &function->parameters[i];
        const ASTNode *arg_node = func_call->arguments[i];
        ReturnContext arg_eval_ctx = {0, RET_NONE, {0}, NULL, NULL}; // Context for argument evaluation

        if (param->is_list) {
            // --- Handle List Parameter ---
            ListNode *list_to_pass_head = NULL;
            VarType list_element_type = param->type; // Expected type

            // ---> MODIFICATION START <---
            if (arg_node->type == NODE_LIST_LITERAL) {
                 // Argument is a list literal: evaluate it in the *caller's* scope
                 const ListLiteralNode *literal_node = &arg_node->data.list_literal;
                 ListNode *current_new_node = NULL;

                // NOTE: Parser passed a placeholder type. Check compatibility now.
                // Although, the literal itself doesn't strictly have a type until evaluated.

                 for (int j = 0; j < literal_node->element_count; ++j) {
                     ListElement element;
                     element.type = list_element_type; // Assign expected type

                     // Evaluate element expression in the CALLER'S scope (variable 'scope')
                     if (list_element_type == VAR_NUM) {
                         element.value.num_val = evaluate_expression(literal_node->elements[j], scope, &arg_eval_ctx);
                         // Check arg_eval_ctx for errors/returns from evaluation if needed
                     } else if (list_element_type == VAR_STR) {
                         element.value.str_val = get_string_value(literal_node->elements[j], scope, &arg_eval_ctx);
                         if (!element.value.str_val) {
                             fprintf(stderr, "Error evaluating string element %d for list literal argument %d in call to '%s'.\n",
                                      j, i + 1, func_call->name);
                             free_list(list_to_pass_head); // Clean up partially built list
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

                     // Create the list node
                     ListNode *new_node = create_list_node(element);
                      if (!new_node) {
                           fprintf(stderr, "Error: Failed to allocate memory for list node from literal for function '%s'.\n", func_call->name);
                           // Free potentially allocated string
                           if (list_element_type == VAR_STR && element.value.str_val) free(element.value.str_val);
                           free_list(list_to_pass_head);
                           free_function_scope_lists(function_scope);
                           hashmap_free(function_scope);
                           exit(EXIT_FAILURE);
                      }


                     // Append node to the temporary list
                     if (list_to_pass_head == NULL) {
                         list_to_pass_head = new_node;
                         current_new_node = new_node;
                     } else {
                         current_new_node->next = new_node;
                         current_new_node = new_node;
                     }
                 }
                 // list_to_pass_head now holds the evaluated list literal

            } else if (arg_node->type == NODE_EXPR_VARIABLE) {
                 // Argument is a variable name (existing behavior)
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
                 // Copy the list from the caller's variable
                 list_to_pass_head = copy_list(list_var_caller->value.list_val.head, list_element_type);
                 if (!list_to_pass_head && list_var_caller->value.list_val.head != NULL) {
                       fprintf(stderr, "Error: Failed to copy list argument %d for function '%s'.\n", i + 1, func_call->name);
                       free_function_scope_lists(function_scope);
                       hashmap_free(function_scope);
                       exit(EXIT_FAILURE);
                 }
            } else {
                 // Argument is neither a list literal nor a variable
                 fprintf(stderr, "Error: Invalid argument type (%d) for list parameter '%s' in function '%s'. Expected list variable or literal.\n",
                         arg_node->type, param->name, func_call->name);
                 free_function_scope_lists(function_scope);
                 hashmap_free(function_scope);
                 exit(EXIT_FAILURE);
            }
            // ---> MODIFICATION END <---


            // Create variable in function scope using list_to_pass_head
            Variable function_param_var = {
                .name = strdup(param->name), // Copy parameter name
                .type = VAR_LIST,
                .value = { .list_val = { .element_type = list_element_type, .head = list_to_pass_head } } // Use the created/copied list
            };
            if (!function_param_var.name) {
                 fprintf(stderr, "Error: Failed to allocate memory for parameter name '%s'.\n", param->name);
                 free_list(list_to_pass_head); // Free the created/copied list
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
            // --- Handle Num Parameter (existing behavior) ---
            double value = evaluate_expression(arg_node, scope, &arg_eval_ctx);
            // Handle potential return from arg evaluation if needed
             Variable function_param_var = {
                  .name = strdup(param->name),
                  .type = VAR_NUM,
                  .value = { .num_val = value }
              };
             if (!function_param_var.name) { /* ... OOM check ... */ free_function_scope_lists(function_scope); hashmap_free(function_scope); exit(EXIT_FAILURE); }
             hashmap_set(function_scope, &function_param_var);
              if (hashmap_oom(function_scope)) { /* ... OOM check ... */ free(function_param_var.name); free_function_scope_lists(function_scope); hashmap_free(function_scope); exit(EXIT_FAILURE); }

        } else if (param->type == VAR_STR) {
            // --- Handle Str Parameter (existing behavior) ---
            char *value = get_string_value(arg_node, scope, &arg_eval_ctx);
             // get_string_value already strdup's, so we own 'value'
             Variable function_param_var = {
                  .name = strdup(param->name), // Need name for the parameter entry
                  .type = VAR_STR,
                  .value = { .str_val = value } // Pass ownership of 'value' string
              };
             if (!function_param_var.name) { free(value); /* ... OOM check ... */ free_function_scope_lists(function_scope); hashmap_free(function_scope); exit(EXIT_FAILURE); }
             hashmap_set(function_scope, &function_param_var);
              if (hashmap_oom(function_scope)) { free(value); free(function_param_var.name); /* ... OOM check ... */ free_function_scope_lists(function_scope); hashmap_free(function_scope); exit(EXIT_FAILURE); }
        }
    }

    // Execute function body with the populated scope
    const double func_ret = execute_function_body(function->body, function_scope); // Adapt return handling if needed

    // Set caller's return context
    // TODO: Need proper handling for non-numeric return types from functions
    caller_ret_ctx->is_return = 1;
    caller_ret_ctx->type = RET_NUM; // Assuming numeric return only for now
    caller_ret_ctx->value.num_val = func_ret;

    // Cleanup Function Scope
    free_function_scope_lists(function_scope); // This should now free lists created from literals as well
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

void execute_assignment(const AssignmentNode *node, struct hashmap *scope) {
    // Check if it's list or variable assignment based on index_expr
    if (node->index_expr != NULL) {
        // === List Assignment ===
        Variable *list_var = get_variable(scope, node->target_name);

        // Check if list variable exists and is a list
        if (!list_var) {
            fprintf(stderr, "Error: List variable '%s' not found for assignment.\n", node->target_name);
            exit(EXIT_FAILURE);
        }
        if (list_var->type != VAR_LIST) {
            fprintf(stderr, "Error: Variable '%s' is not a list for assignment.\n", node->target_name);
            exit(EXIT_FAILURE);
        }

        // Calculate list size
        int list_size = 0;
        ListNode *counter_node = list_var->value.list_val.head;
        while (counter_node != NULL) {
            list_size++;
            counter_node = counter_node->next;
        }

        // Evaluate and validate index (using logic from list access)
        ReturnContext index_eval_ctx = {0};
        double index_val_double = evaluate_expression(node->index_expr, scope, &index_eval_ctx);
        // Handle potential return from index expression if necessary

        if (index_val_double != floor(index_val_double)) {
            fprintf(stderr, "Error: List index for '%s' must be an integer, got %f.\n", node->target_name, index_val_double);
            exit(EXIT_FAILURE);
        }
        int index = (int)index_val_double;
        if (index < 0) {
            index = list_size + index; // Handle negative index
        }
        if (index < 0 || index >= list_size) {
            fprintf(stderr, "Error: List index %d (calculated from %f) out of bounds for list '%s' of size %d during assignment.\n",
                    index, index_val_double, node->target_name, list_size);
            exit(EXIT_FAILURE);
        }

        // Find the target list node
        ListNode *target_lnode = list_var->value.list_val.head;
        for (int i = 0; i < index; ++i) {
            target_lnode = target_lnode->next; // Should not be NULL due to bounds check
        }

        // Evaluate the value expression and check type
        ReturnContext value_eval_ctx = {0}; // Use clean context for value evaluation
        if (list_var->value.list_val.element_type == VAR_NUM) {
            double value_to_assign = evaluate_expression(node->value_expr, scope, &value_eval_ctx);
            // Handle potential return from value expression if needed
            if (target_lnode->element.type != VAR_NUM) { // Sanity check
                 fprintf(stderr, "Internal Error: List '%s' node type mismatch at index %d.\n", node->target_name, index);
                 exit(EXIT_FAILURE);
            }
            target_lnode->element.value.num_val = value_to_assign;
        } else if (list_var->value.list_val.element_type == VAR_STR) {
            char *value_to_assign_str = get_string_value(node->value_expr, scope, &value_eval_ctx);
            // Handle potential return from value expression if needed

            if (!value_to_assign_str) { // Check if get_string_value failed
                 fprintf(stderr, "Error: Failed to evaluate string value for assignment to list '%s' at index %d.\n", node->target_name, index);
                 exit(EXIT_FAILURE); // Or handle error string from get_string_value if changed
            }
             if (target_lnode->element.type != VAR_STR) { // Sanity check
                  fprintf(stderr, "Internal Error: List '%s' node type mismatch at index %d.\n", node->target_name, index);
                  free(value_to_assign_str);
                  exit(EXIT_FAILURE);
             }
            // Free the old string before assigning the new one
            if (target_lnode->element.value.str_val != NULL) {
                free(target_lnode->element.value.str_val);
            }
            target_lnode->element.value.str_val = value_to_assign_str; // Assign the newly allocated string
        } else {
             fprintf(stderr, "Internal Error: Assignment to list '%s' with unknown element type.\n", node->target_name);
             exit(EXIT_FAILURE);
        }

    } else {
        // === Variable Assignment ===
        Variable *existing_var = get_variable(scope, node->target_name);

        if (!existing_var) {
            fprintf(stderr, "Error: Variable '%s' not declared before assignment.\n", node->target_name);
             // If your language allows implicit declaration on assignment, change this.
            exit(EXIT_FAILURE);
        }

        // Evaluate the value and check type against existing variable
        ReturnContext value_eval_ctx = {0};
        if (existing_var->type == VAR_NUM) {
            double value_to_assign = evaluate_expression(node->value_expr, scope, &value_eval_ctx);
            // Handle potential return
            existing_var->value.num_val = value_to_assign;
            // Note: hashmap_set is not strictly needed if modifying in place, BUT if
            // get_variable could return from a different scope (e.g., global),
            // directly modifying might be wrong. Using hashmap_set ensures update
            // in the correct scope (assuming 'scope' is the target scope).
            // However, modifying in place is usually fine for simple local scopes. Let's modify in place.
            // hashmap_set(scope, existing_var); // Re-set to update hashmap if needed by its design
        } else if (existing_var->type == VAR_STR) {
            char *value_to_assign_str = get_string_value(node->value_expr, scope, &value_eval_ctx);
            // Handle potential return
             if (!value_to_assign_str) {
                  fprintf(stderr, "Error: Failed to evaluate string value for assignment to variable '%s'.\n", node->target_name);
                  exit(EXIT_FAILURE);
             }
            // Free the old string in the variable before assigning the new one
            if (existing_var->value.str_val != NULL) {
                free(existing_var->value.str_val);
            }
            existing_var->value.str_val = value_to_assign_str;
            // hashmap_set(scope, existing_var); // See comment above
        } else if (existing_var->type == VAR_LIST) {
             fprintf(stderr, "Error: Cannot assign directly to a list variable '%s' using '='. Use list declaration or modify elements.\n", node->target_name);
             exit(EXIT_FAILURE);
        } else {
             fprintf(stderr, "Internal Error: Assignment to variable '%s' with unknown type.\n", node->target_name);
             exit(EXIT_FAILURE);
        }
    }
}
