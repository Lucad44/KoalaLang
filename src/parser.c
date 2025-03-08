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

 ASTNode *parse_binary_expr(Parser *parser, ASTNode *left, int min_prec) {
     const struct {
        TokenType token;
        BinaryOperator op;
        int prec;
    } ops[] = {
        {TOKEN_OPERATOR_LESS, OP_LESS, 1},
        {TOKEN_OPERATOR_GREATER, OP_GREATER, 1},
        {TOKEN_OPERATOR_EQUAL, OP_EQUAL, 2},
    };

    while (1) {
        TokenType lookahead = parser->current_token.type;
        int found = 0;
        BinaryOperator op;
        int prec;

        for (size_t i = 0; i < sizeof(ops)/sizeof(ops[0]); i++) {
            if (ops[i].token == lookahead && ops[i].prec >= min_prec) {
                found = 1;
                op = ops[i].op;
                prec = ops[i].prec;
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

ASTNode *parse_expression(Parser *parser) {
    ASTNode *node = safe_malloc(sizeof(ASTNode));

    if (parser->current_token.type == TOKEN_NUMBER) {
        node->type = NODE_EXPR_LITERAL;
        node->data.num_literal.num_val = parser->current_token.num_value;
        advance(parser);
    }
    else if (parser->current_token.type == TOKEN_STRING) {
        node->type = NODE_EXPR_LITERAL;
        node->data.str_literal.str_val = strdup(parser->current_token.str_value);
        advance(parser);
    }
    else if (parser->current_token.type == TOKEN_IDENTIFIER) {
        node->type = NODE_EXPR_VARIABLE;
        node->data.variable.name = strdup(parser->current_token.lexeme);
        advance(parser);
    }
    else {
        fprintf(stderr, "Unexpected token in expression: %d\n", parser->current_token.type);
        exit(EXIT_FAILURE);
    }

    return node;
}

 ASTNode *parse_print(Parser *parser) {
    advance(parser); // Consume 'print'

    ASTNode **expressions = NULL;
    int expr_count = 0;

    if (parser->current_token.type != TOKEN_LPAREN) {
        fprintf(stderr, "Expected '(' after print\n");
        exit(EXIT_FAILURE);
    }
    advance(parser); // Consume '('

    while (parser->current_token.type != TOKEN_RPAREN) {
        ASTNode *expr = parse_expression(parser);
        expressions = safe_realloc(expressions, (expr_count + 1) * sizeof(ASTNode *));
        expressions[expr_count++] = expr;

        if (parser->current_token.type == TOKEN_OPERATOR_PLUS) {
            advance(parser); // Consume '+'
        }
    }

    advance(parser); // Consume ')'

    if (parser->current_token.type != TOKEN_SEMICOLON) {
        fprintf(stderr, "Expected ';' after print statement\n");
        exit(EXIT_FAILURE);
    }
    advance(parser); // Consume ';'

    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_PRINT;
    node->data.print.expr_list = expressions;
    node->data.print.expr_count = expr_count;
    return node;
}

 ASTNode *parse_declaration(Parser *parser) {
    TokenType decl_type = parser->current_token.type;
    advance(parser); // Consume type keyword

    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected identifier after type\n");
        exit(EXIT_FAILURE);
    }
    char *name = strdup(parser->current_token.lexeme);
    advance(parser); // Consume identifier

    if (parser->current_token.type != TOKEN_OPERATOR_EQUAL) {
        fprintf(stderr, "Expected '=' in declaration\n");
        exit(EXIT_FAILURE);
    }
    advance(parser); // Consume '='

    ASTNode *init_expr = parse_expression(parser);

    if (parser->current_token.type != TOKEN_SEMICOLON) {
        fprintf(stderr, "Expected ';' after declaration\n");
        exit(EXIT_FAILURE);
    }
    advance(parser); // Consume ';'

    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_VAR_DECL;
    node->data.var_decl.name = name;
    node->data.var_decl.init_expr = init_expr;
    node->data.var_decl.type = (decl_type == TOKEN_KEYWORD_NUM) ? VAR_NUM :
                               (decl_type == TOKEN_KEYWORD_STR) ? VAR_STR : -1;
    return node;
}

static ASTNode *parse_condition(Parser *parser) {
    ASTNode *left = parse_primary(parser);

    // Ensure the next token is a comparison operator
    TokenType op_type = parser->current_token.type;
    if (op_type == TOKEN_OPERATOR_LESS ||
        op_type == TOKEN_OPERATOR_GREATER ||
        op_type == TOKEN_OPERATOR_EQUAL) {

        advance(parser); // Consume the operator
        ASTNode *right = parse_primary(parser); // Get the right-hand expression

        // Create a binary expression node
        ASTNode *comparison = safe_malloc(sizeof(ASTNode));
        comparison->type = NODE_EXPR_BINARY;
        comparison->data.binary_expr.op =
            (op_type == TOKEN_OPERATOR_LESS) ? OP_LESS :
            (op_type == TOKEN_OPERATOR_GREATER) ? OP_GREATER : OP_EQUAL;
        comparison->data.binary_expr.left = left;
        comparison->data.binary_expr.right = right;

        return comparison;
        }

    return left; // If no operator, return the single expression
}


ASTNode *parse_if(Parser *parser) {
    advance(parser); // Consume 'if'

    if (parser->current_token.type != TOKEN_LPAREN) {
        fprintf(stderr, "Expected '(' after if\n");
        exit(EXIT_FAILURE);
    }
    advance(parser); // Consume '('

    ASTNode *condition = parse_condition(parser);

    if (parser->current_token.type != TOKEN_RPAREN) {
        fprintf(stderr, "Expected ')', got %d (lexeme: %s)\n",
               parser->current_token.type,
               parser->current_token.lexeme ? parser->current_token.lexeme : "NULL");
        exit(EXIT_FAILURE);
    }
    advance(parser); // Consume ')'

    ASTNode *body = parse_braced_block(parser);

    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_IF;
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.body = body;
    return node;
}

 ASTNode *parse_braced_block(Parser *parser) {
    if (parser->current_token.type != TOKEN_LBRACE) {
        fprintf(stderr, "Expected '{' at position %d\n", parser->lexer->current_pos);
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
            default:
                fprintf(stderr, "Unexpected token in block: %d\n", parser->current_token.type);
            exit(EXIT_FAILURE);
        }
        statements = safe_realloc(statements, (count + 1) * sizeof(ASTNode *));
        statements[count++] = stmt;
    }

    if (parser->current_token.type != TOKEN_RBRACE) {
        fprintf(stderr, "Expected '}', got %d\n", parser->current_token.type);
        exit(EXIT_FAILURE);
    }
    advance(parser); // Consume '}'

    ASTNode *block = safe_malloc(sizeof(ASTNode));
    block->type = NODE_BLOCK;
    block->data.block.statements = statements;
    block->data.block.stmt_count = count;
    return block;
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
