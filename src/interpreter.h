#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "variables.h"
#include "hashmap.h"

Variable *get_variable(struct hashmap *scope, char *name);

void execute(const ASTNode *node, struct hashmap *scope);

int evaluate_condition(ASTNode *condition, struct hashmap *scope);

void execute_postfix(const PostfixExprNode *node, struct hashmap *scope);

void execute_if(const IfNode *if_node, struct hashmap *scope);

void execute_while(const WhileNode *while_node, struct hashmap *scope);

void execute_func_decl(const FuncDeclNode *func_decl);

void execute_func_call(const FuncCallNode *func_call, struct hashmap *scope);

#endif //INTERPRETER_H