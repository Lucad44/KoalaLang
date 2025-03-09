#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"

void execute(const ASTNode *node);

int evaluate_condition(ASTNode *condition);

void execute_postfix(const PostfixExprNode *node);

void execute_if(const IfNode *if_node);

void execute_while(const WhileNode *while_node);

#endif //INTERPRETER_H