#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <setjmp.h>

#include "ast.h"
#include "variables.h"
#include "hashmap.h"

typedef enum {
    RET_NONE,
    RET_NUM,
    RET_STR
} ReturnType;

typedef struct {
    int is_return;
    ReturnType type;
    union {
        double num_val;
        char *str_val;
    } value;
    jmp_buf *jmp;
    double *ret_ptr;
} ReturnContext;

typedef struct ReturnContextNode {
    ReturnContext *ctx;
    struct ReturnContextNode *prev;
} ReturnContextNode;





Variable *get_variable(struct hashmap *scope, char *name);

void execute(const ASTNode *node, struct hashmap *scope, ReturnContext *ret_ctx);

int evaluate_condition(ASTNode *condition, struct hashmap *scope);

void execute_postfix(const PostfixExprNode *node, struct hashmap *scope);

void execute_if(const IfNode *if_node, struct hashmap *scope, ReturnContext *ret_ctx);

void execute_while(const WhileNode *while_node, struct hashmap *scope, ReturnContext *ret_ctx);

double execute_function_body(const ASTNode *body, struct hashmap *scope);

void execute_func_decl(const FuncDeclNode *func_decl);

void execute_func_call(const FuncCallNode *func_call, struct hashmap *scope, ReturnContext *caller_ret_ctx);

void execute_return(const ReturnNode *node, struct hashmap *scope, ReturnContext *ret_ctx);

char *get_string_value(const ASTNode *node, struct hashmap *scope, ReturnContext *ret_ctx);

void execute_list_decl(const ListDeclNode *node, struct hashmap *scope, ReturnContext *ret_ctx);

#endif //INTERPRETER_H