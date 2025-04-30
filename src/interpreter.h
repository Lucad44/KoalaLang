#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <setjmp.h>

#include "ast.h"
#include "variables.h"
#include "hashmap.h"

typedef enum {
    RET_NONE,
    RET_NUM,
    RET_STR,
    RET_LIST
} ReturnType;

typedef struct {
    ReturnType type;
    union {
        double num_val;
        char *str_val;
        ListNode *list_val;
    } value;
} ReturnValue;

typedef struct {
    int is_return;
    ReturnValue ret_val;
    jmp_buf *jmp;
} ReturnContext;

typedef struct ReturnContextNode {
    ReturnContext *ctx;
    struct ReturnContextNode *prev;
} ReturnContextNode;

int is_truthy(ReturnValue val);

Variable *get_variable(struct hashmap *scope, char *name);

void free_return_value(ReturnType type, ReturnValue *value);

ReturnValue evaluate_expression(const ASTNode *node, struct hashmap *scope, ReturnContext *ret_ctx);

void execute(const ASTNode *node, struct hashmap *scope, ReturnContext *ret_ctx);

void execute_var_decl(const VarDeclNode *node, struct hashmap *scope, ReturnContext *ret_ctx);

int evaluate_condition(ASTNode *condition, struct hashmap *scope);

void execute_postfix(const PostfixExprNode *node, struct hashmap *scope);

void execute_if(const IfNode *if_node, struct hashmap *scope, ReturnContext *ret_ctx);

void execute_while(const WhileNode *while_node, struct hashmap *scope, ReturnContext *ret_ctx);

ReturnValue execute_function_body(const ASTNode *body, struct hashmap *scope, ReturnContext *ret_ctx);

void execute_func_decl(const FuncDeclNode *func_decl);

void execute_func_call(const FuncCallNode *func_call, struct hashmap *scope, ReturnContext *caller_ret_ctx);

void execute_return(const ReturnNode *node, struct hashmap *scope, ReturnContext *ret_ctx);

char *get_string_value(const ASTNode *node, struct hashmap *scope, ReturnContext *ret_ctx);

void execute_list_decl(const ListDeclNode *node, struct hashmap *scope, ReturnContext *ret_ctx);

void execute_assignment(const AssignmentNode *node, struct hashmap *scope);

#endif //INTERPRETER_H