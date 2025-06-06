#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include "interpreter.h"

typedef struct {
    Lexer *lexer;
    Token current_token;
} Parser;

void advance(Parser *parser);

ASTNode *parse_declaration(Parser *parser);

ASTNode *parse_print(Parser *parser);

ASTNode *parse_expression(Parser *parser);

void init_parser(Parser *parser, Lexer *lexer);

ASTNode *parse_braced_block(Parser *parser);

ASTNode *parse_if(Parser *parser);

ASTNode *parse_while(Parser *parser);

ASTNode *parse_program(Parser *parser);

ASTNode *parse_postfix(Parser *parser, ASTNode *left);

ASTNode *parse_function_declaration(Parser *parser);

ASTNode *parse_function_call(Parser *parser);

ASTNode *parse_return(Parser *parser);

ASTNode *parse_list_declaration(Parser *parser);

ASTNode *parse_list_literal(Parser *parser);

ASTNode *parse_expression_statement(Parser *parser);

ASTNode *parse_list_type(Parser *parser);

ASTNode *parse_assignment(Parser *parser);

ASTNode *parse_import(Parser *parser);

bool parse_parameter_type(Parser *parser, Parameter *param);

#endif