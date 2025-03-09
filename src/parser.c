#include "parser.h"
#include "memory.h"
#include "variables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_parser(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    advance(parser);
}

void advance(Parser *parser) {
    if (parser->current_token.type != TOKEN_EOF) {
        parser->current_token = next_token(parser->lexer);
    }
}

ASTNode *parse_primary(Parser *parser) {
    ASTNode *node = safe_malloc(sizeof(ASTNode));

    switch (parser->current_token.type) {
        case TOKEN_NUMBER:
            node->type = NODE_EXPR_LITERAL;
            node->data.num_literal.num_val = parser->current_token.num_value;
            advance(parser);
            break;
        case TOKEN_STRING:
            node->type = NODE_EXPR_LITERAL;
            node->data.str_literal.str_val = strdup(parser->current_token.str_value);
            advance(parser);
            break;
        case TOKEN_IDENTIFIER:
            node->type = NODE_EXPR_VARIABLE;
            node->data.variable.name = strdup(parser->current_token.lexeme);
            advance(parser);
            break;
        default:
            fprintf(stderr, "Unexpected token in expression: %d\n", parser->current_token.type);
            exit(EXIT_FAILURE);
    }
    return node;
}

ASTNode *parse_binary_expr(Parser *parser, ASTNode *left, const int min_prec) {
    const struct {
        TokenType token;
        BinaryOperator op;
        int prec;
        } ops[] = {
        {TOKEN_OPERATOR_LESS, OP_LESS, 1},
        {TOKEN_OPERATOR_GREATER, OP_GREATER, 1},
        {TOKEN_OPERATOR_EQUAL, OP_EQUAL, 2},
        {TOKEN_OPERATOR_NOT_EQUAL, OP_NOT_EQUAL, 3},
        {TOKEN_OPERATOR_LESS_EQUAL, OP_LESS_EQUAL, 4},
        {TOKEN_OPERATOR_GREATER_EQUAL, OP_GREATER_EQUAL, 5},
    };

    while (1) {
        const TokenType lookahead = parser->current_token.type;
        int found = 0;
        BinaryOperator op = {};

        for (size_t i = 0; i < sizeof(ops)/sizeof(ops[0]); i++) {
            if (ops[i].token == lookahead && ops[i].prec >= min_prec) {
                found = 1;
                op = ops[i].op;
                break;
            }
        }

        if (!found) break;

        advance(parser);
        ASTNode *right = parse_primary(parser);

        ASTNode *new_node = safe_malloc(sizeof(ASTNode));
        new_node->type = NODE_EXPR_BINARY;
        new_node->data.binary_expr.op = op;
        new_node->data.binary_expr.left = left;
        new_node->data.binary_expr.right = right;
        left = new_node;
    }

    return left;
}

static ASTNode *parse_postfix(Parser *parser, ASTNode *left) {
    while (1) {
        if (parser->current_token.type == TOKEN_OPERATOR_PLUS_PLUS ||
            parser->current_token.type == TOKEN_OPERATOR_MINUS_MINUS)
        {
            const PostfixOperator op = parser->current_token.type == TOKEN_OPERATOR_PLUS_PLUS ? OP_INC : OP_DEC;

            if (left->type != NODE_EXPR_VARIABLE) {
                fprintf(stderr, "Postfix operator can only be applied to numeric variables\n");
                exit(EXIT_FAILURE);
            }

            advance(parser);

            ASTNode *node = safe_malloc(sizeof(ASTNode));
            node->type = NODE_EXPR_POSTFIX;
            node->data.postfix_expr.op = op;
            node->data.postfix_expr.var_name = strdup(left->data.variable.name);
            free_ast(left);
            left = node;
            } else {
                break;
            }
    }
    return left;
}

ASTNode *parse_expression(Parser *parser) {
    ASTNode *left = parse_primary(parser);
    return parse_postfix(parser, left);
}

ASTNode *parse_print(Parser *parser) {
    advance(parser);

    ASTNode **expressions = NULL;
    int expr_count = 0;

    if (parser->current_token.type != TOKEN_LPAREN) {
        fprintf(stderr, "Expected '(' after print\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    while (parser->current_token.type != TOKEN_RPAREN) {
        ASTNode *expr = parse_expression(parser);
        expressions = safe_realloc(expressions, (expr_count + 1) * sizeof(ASTNode *));
        expressions[expr_count++] = expr;

        if (parser->current_token.type == TOKEN_OPERATOR_PLUS) {
            advance(parser);
        }
    }

    advance(parser);

    if (parser->current_token.type != TOKEN_SEMICOLON) {
        fprintf(stderr, "Expected ';' after print statement\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_PRINT;
    node->data.print.expr_list = expressions;
    node->data.print.expr_count = expr_count;
    return node;
}

ASTNode *parse_declaration(Parser *parser) {
    const TokenType decl_type = parser->current_token.type;
    advance(parser);

    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected identifier after type\n");
        exit(EXIT_FAILURE);
    }
    char *name = strdup(parser->current_token.lexeme);
    advance(parser);

    if (parser->current_token.type != TOKEN_OPERATOR_EQUAL) {
        fprintf(stderr, "Expected '=' in declaration\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    ASTNode *init_expr = parse_expression(parser);

    if (parser->current_token.type != TOKEN_SEMICOLON) {
    fprintf(stderr, "Expected ';' after declaration\n");
    exit(EXIT_FAILURE);
    }
    advance(parser);

    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_VAR_DECL;
    node->data.var_decl.name = name;
    node->data.var_decl.init_expr = init_expr;
    node->data.var_decl.type = decl_type == TOKEN_KEYWORD_NUM ? VAR_NUM :
                           decl_type == TOKEN_KEYWORD_STR ? VAR_STR : -1;
    return node;
}

static ASTNode *parse_condition(Parser *parser) {
    ASTNode *left = parse_primary(parser);

    const TokenType op_type = parser->current_token.type;
    if (op_type == TOKEN_OPERATOR_LESS ||
        op_type == TOKEN_OPERATOR_GREATER ||
        op_type == TOKEN_OPERATOR_EQUAL ||
        op_type == TOKEN_OPERATOR_NOT_EQUAL ||
        op_type == TOKEN_OPERATOR_LESS_EQUAL ||
        op_type == TOKEN_OPERATOR_GREATER_EQUAL)
    {
        advance(parser);
        ASTNode *right = parse_primary(parser);

        ASTNode *comparison = safe_malloc(sizeof(ASTNode));
        comparison->type = NODE_EXPR_BINARY;
        comparison->data.binary_expr.op =
            op_type == TOKEN_OPERATOR_LESS ? OP_LESS :
            op_type == TOKEN_OPERATOR_GREATER ? OP_GREATER :
            op_type == TOKEN_OPERATOR_EQUAL ? OP_EQUAL :
            op_type == TOKEN_OPERATOR_NOT_EQUAL ? OP_NOT_EQUAL :
            op_type == TOKEN_OPERATOR_LESS_EQUAL ? OP_LESS_EQUAL :
            op_type == TOKEN_OPERATOR_GREATER_EQUAL ? OP_GREATER_EQUAL : -1;
        comparison->data.binary_expr.left = left;
        comparison->data.binary_expr.right = right;

        return comparison;
    }

    return left;
}


ASTNode *parse_if(Parser *parser) {
    advance(parser);

    if (parser->current_token.type != TOKEN_LPAREN) {
        fprintf(stderr, "Expected '(' after if\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    ASTNode *condition = parse_condition(parser);

    if (parser->current_token.type != TOKEN_RPAREN) {
        fprintf(stderr, "Expected ')', got %d (lexeme: %s)\n",
               parser->current_token.type,
               parser->current_token.lexeme ? parser->current_token.lexeme : "NULL");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    ASTNode *body = parse_braced_block(parser);

    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_IF;
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.body = body;
    return node;
}

ASTNode *parse_while(Parser *parser) {
    advance(parser);

    if (parser->current_token.type != TOKEN_LPAREN) {
        fprintf(stderr, "Expected '(' after while\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    ASTNode *condition = parse_condition(parser);

    if (parser->current_token.type != TOKEN_RPAREN) {
        fprintf(stderr, "Expected ')', got %d (lexeme: %s)\n",
               parser->current_token.type,
               parser->current_token.lexeme ? parser->current_token.lexeme : "NULL");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    ASTNode *body = parse_braced_block(parser);

    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_WHILE;
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;
    return node;
}

ASTNode *parse_braced_block(Parser *parser) {
    if (parser->current_token.type != TOKEN_LBRACE) {
        fprintf(stderr, "Expected '{', got %d\n", parser->current_token.type);
        exit(EXIT_FAILURE);
    }
    advance(parser);

    ASTNode **statements = NULL;
    int count = 0;

    while (parser->current_token.type != TOKEN_RBRACE && parser->current_token.type != TOKEN_EOF) {
        ASTNode *stmt = NULL;
        switch (parser->current_token.type) {
            case TOKEN_KEYWORD_NUM:
            case TOKEN_KEYWORD_STR:
            case TOKEN_KEYWORD_VAR:
                stmt = parse_declaration(parser);
                break;
            case TOKEN_KEYWORD_PRINT:
                stmt = parse_print(parser);
                break;
            case TOKEN_KEYWORD_IF:
                stmt = parse_if(parser);
                break;
            case TOKEN_KEYWORD_WHILE:
                stmt = parse_while(parser);
                break;
            case TOKEN_IDENTIFIER:
                stmt = parse_expression_statement(parser);
                break;
            default:
                fprintf(stderr, "Unexpected token in block: %d (lexeme: %s)\n",
                      parser->current_token.type,
                      parser->current_token.lexeme ? parser->current_token.lexeme : "NULL");
                exit(EXIT_FAILURE);
        }
        statements = safe_realloc(statements, (count + 1) * sizeof(ASTNode *));
        statements[count++] = stmt;
    }

    if (parser->current_token.type != TOKEN_RBRACE) {
        fprintf(stderr, "Expected '}', got %d\n", parser->current_token.type);
        exit(EXIT_FAILURE);
    }
    advance(parser);

    ASTNode *block = safe_malloc(sizeof(ASTNode));
    block->type = NODE_BLOCK;
    block->data.block.statements = statements;
    block->data.block.stmt_count = count;
    return block;
}

ASTNode *parse_expression_statement(Parser *parser) {
    ASTNode *expr = parse_expression(parser);

    if (parser->current_token.type != TOKEN_SEMICOLON) {
        fprintf(stderr, "Expected ';' after expression\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    return expr;
}

ASTNode *parse_program(Parser *parser) {
    ASTNode **statements = NULL;
    int count = 0;

    while (parser->current_token.type != TOKEN_EOF) {
        ASTNode *stmt = NULL;

        switch (parser->current_token.type) {
            case TOKEN_KEYWORD_NUM:
            case TOKEN_KEYWORD_STR:
            case TOKEN_KEYWORD_VAR:
                stmt = parse_declaration(parser);
                break;
            case TOKEN_KEYWORD_PRINT:
                stmt = parse_print(parser);
                break;
            case TOKEN_KEYWORD_IF:
                stmt = parse_if(parser);
                break;
            case TOKEN_KEYWORD_WHILE:
                stmt = parse_while(parser);
                break;
            case TOKEN_IDENTIFIER:
                stmt = parse_expression_statement(parser);
                break;
            default:
                fprintf(stderr, "Unexpected token: %d (lexeme: %s)\n",
                      parser->current_token.type,
                      parser->current_token.lexeme ? parser->current_token.lexeme : "(null)");
                exit(EXIT_FAILURE);
        }

        statements = safe_realloc(statements, (count + 1) * sizeof(ASTNode *));
        statements[count++] = stmt;
    }

    ASTNode *block = safe_malloc(sizeof(ASTNode));
    block->type = NODE_BLOCK;
    block->data.block.statements = statements;
    block->data.block.stmt_count = count;
    return block;
}
