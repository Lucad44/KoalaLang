#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"

void execute(const ASTNode *node);

int evaluate_condition(ASTNode *condition);

void execute_if(const IfNode *if_node);

#endif //INTERPRETER_H