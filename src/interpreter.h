#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "variables.h"

void execute(ASTNode *node);

int evaluate_condition(ASTNode *condition);

void execute_if(IfNode *if_node);

#endif //INTERPRETER_H