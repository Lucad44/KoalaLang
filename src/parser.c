#include "parser.h"
#include "memory.h"
#include "variables.h"
#include "interpreter.h"
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
        case TOKEN_LPAREN: {
            advance(parser);
            ASTNode *expr = parse_expression(parser);
            
            if (parser->current_token.type != TOKEN_RPAREN) {
                fprintf(stderr, "Expected closing parenthesis ')'\n");
                exit(EXIT_FAILURE);
            }
            advance(parser);
            return expr;
        }
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
        case TOKEN_KEYWORD_CALL: {
            advance(parser);
            if (parser->current_token.type != TOKEN_IDENTIFIER) {
                fprintf(stderr, "Expected function name after 'call'\n");
                exit(EXIT_FAILURE);
            }
            char *func_name = strdup(parser->current_token.lexeme);
            advance(parser);

            if (parser->current_token.type != TOKEN_LPAREN) {
                fprintf(stderr, "Expected '(' after function name\n");
                exit(EXIT_FAILURE);
            }
            advance(parser);

            ASTNode **arguments = NULL;
            int arg_count = 0;

            while (parser->current_token.type != TOKEN_RPAREN) {
                ASTNode *arg = parse_expression(parser);
                arguments = safe_realloc(arguments, (arg_count + 1) * sizeof(ASTNode *));
                arguments[arg_count++] = arg;

                if (parser->current_token.type == TOKEN_RPAREN) break;
                if (parser->current_token.type != TOKEN_COMMA) {
                    fprintf(stderr, "Expected ',' or ')' after argument\n");
                    exit(EXIT_FAILURE);
                }
                advance(parser);
            }
            advance(parser);

            node->type = NODE_FUNC_CALL;
            node->data.func_call.name = func_name;
            node->data.func_call.arguments = arguments;
            node->data.func_call.arg_count = arg_count;
            break;
        }
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
        {TOKEN_OPERATOR_OR, OP_OR, 1},           // |
        {TOKEN_OPERATOR_XOR, OP_XOR, 2},         // ^
        {TOKEN_OPERATOR_AND, OP_AND, 3},         // &
        
        {TOKEN_OPERATOR_EQUAL, OP_EQUAL, 7},     // == != 
        {TOKEN_OPERATOR_NOT_EQUAL, OP_NOT_EQUAL, 7},
        
        {TOKEN_OPERATOR_LESS, OP_LESS, 8},       // < > <= >=
        {TOKEN_OPERATOR_GREATER, OP_GREATER, 8},
        {TOKEN_OPERATOR_LESS_EQUAL, OP_LESS_EQUAL, 8},
        {TOKEN_OPERATOR_GREATER_EQUAL, OP_GREATER_EQUAL, 8},
        
        {TOKEN_OPERATOR_PLUS, OP_PLUS, 10},      // + -
        {TOKEN_OPERATOR_MINUS, OP_MINUS, 10},
        
        {TOKEN_OPERATOR_MULTIPLY, OP_MULTIPLY, 11}, // * / %
        {TOKEN_OPERATOR_DIVIDE, OP_DIVIDE, 11},
        {TOKEN_OPERATOR_MODULO, OP_MODULO, 11},
        
        {TOKEN_OPERATOR_POWER, OP_POWER, 12}    // **
    };

    while (1) {
        const TokenType lookahead = parser->current_token.type;
        int found = 0;
        BinaryOperator op = {};
        int op_prec = 0;

        for (size_t i = 0; i < sizeof(ops) / sizeof(ops[0]); i++) {
            if (ops[i].token == lookahead && ops[i].prec >= min_prec) {
                found = 1;
                op = ops[i].op;
                op_prec = ops[i].prec;
                break;
            }
        }

        if (!found) break;

        advance(parser);
        ASTNode *right = parse_primary(parser);
        right = parse_postfix(parser, right);
        right = parse_binary_expr(parser, right, op_prec + 1);

        ASTNode *new_node = safe_malloc(sizeof(ASTNode));
        new_node->type = NODE_EXPR_BINARY;
        new_node->data.binary_expr.op = op;
        new_node->data.binary_expr.left = left;
        new_node->data.binary_expr.right = right;
        left = new_node;
    }

    return left;
}

ASTNode *parse_postfix(Parser *parser, ASTNode *left) {
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
    left = parse_postfix(parser, left);
    return parse_binary_expr(parser, left, 0);
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

        if (parser->current_token.type == TOKEN_OPERATOR_CONCAT) {
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
        fprintf(stderr, "Expected '(' after conditional statement\n");
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
    node->data.if_stmt.elif_nodes = NULL;
    node->data.if_stmt.elif_count = 0;
    node->data.if_stmt.else_body = NULL;

    while (parser->current_token.type == TOKEN_KEYWORD_ELIF) {
        advance(parser);
        
        if (parser->current_token.type != TOKEN_LPAREN) {
            fprintf(stderr, "Expected '(' after elif\n");
            exit(EXIT_FAILURE);
        }
        advance(parser);
        
        ASTNode *elif_condition = parse_condition(parser);
        
        if (parser->current_token.type != TOKEN_RPAREN) {
            fprintf(stderr, "Expected ')' after elif condition\n");
            exit(EXIT_FAILURE);
        }
        advance(parser);
        
        ASTNode *elif_body = parse_braced_block(parser);
        
        node->data.if_stmt.elif_count++;
        node->data.if_stmt.elif_nodes = safe_realloc(node->data.if_stmt.elif_nodes,
            node->data.if_stmt.elif_count * sizeof(ASTNode*));
        
        ASTNode *elif_node = safe_malloc(sizeof(ASTNode));
        elif_node->type = NODE_ELIF;
        elif_node->data.if_stmt.condition = elif_condition;
        elif_node->data.if_stmt.body = elif_body;
        
        node->data.if_stmt.elif_nodes[node->data.if_stmt.elif_count - 1] = elif_node;
    }

    if (parser->current_token.type == TOKEN_KEYWORD_ELSE) {
        advance(parser);
        node->data.if_stmt.else_body = parse_braced_block(parser);
    }

    return node;
}

ASTNode *parse_else(Parser *parser) {
    advance(parser);

    ASTNode *body = parse_braced_block(parser);

    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_ELSE;
    node->data.if_stmt.else_body = body;
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
            case TOKEN_KEYWORD_ELIF:
            case TOKEN_KEYWORD_ELSE:
                stmt = parse_if(parser);
                break;
            case TOKEN_KEYWORD_WHILE:
                stmt = parse_while(parser);
                break;
            case TOKEN_KEYWORD_CALL:
                stmt = parse_function_call(parser);
                break;
            case TOKEN_KEYWORD_RETURN:
                stmt = parse_return(parser);
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

ASTNode *parse_function_declaration(Parser *parser) {
    advance(parser);

    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected function name\n");
        exit(EXIT_FAILURE);
    }
    char *func_name = strdup(parser->current_token.lexeme);
    advance(parser);


    if (parser->current_token.type != TOKEN_LPAREN) {
        fprintf(stderr, "Expected '(' after function name\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    Parameter *parameters = NULL;
    int param_count = 0;

    while (parser->current_token.type != TOKEN_RPAREN) {
        if (parser->current_token.type != TOKEN_KEYWORD_NUM && 
            parser->current_token.type != TOKEN_KEYWORD_STR)
        {
            fprintf(stderr, "Expected parameter type (num or str)\n");
            exit(EXIT_FAILURE);
        }
        char *param_type = strdup(parser->current_token.lexeme);
        advance(parser);

        if (parser->current_token.type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Expected parameter name\n");
            exit(EXIT_FAILURE);
        }
        char *param_name = strdup(parser->current_token.lexeme);
        advance(parser);

        parameters = safe_realloc(parameters, (param_count + 1) * sizeof(*parameters));
        parameters[param_count].name = param_name;
        parameters[param_count].type = param_type;
        param_count++;

        if (parser->current_token.type == TOKEN_RPAREN) {
            break;
        }
        if (parser->current_token.type != TOKEN_COMMA) {
            fprintf(stderr, "Expected ',' or ')' after parameter\n");
            exit(EXIT_FAILURE);
        }
        advance(parser);
    }
    advance(parser);

    ASTNode *body = parse_braced_block(parser);

    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_FUNC_DECL;
    node->data.func_decl.name = func_name;
    node->data.func_decl.parameters = parameters;
    node->data.func_decl.param_count = param_count;
    node->data.func_decl.body = body;

    return node;
}

ASTNode *parse_function_call(Parser *parser) {
    advance(parser);

    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected function name after 'call'\n");
        exit(EXIT_FAILURE);
    }
    char *func_name = strdup(parser->current_token.lexeme);
    advance(parser);

    if (parser->current_token.type != TOKEN_LPAREN) {
        fprintf(stderr, "Expected '(' after function name\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    ASTNode **arguments = NULL;
    int arg_count = 0;

    while (parser->current_token.type != TOKEN_RPAREN) {
        ASTNode *arg = parse_expression(parser);

        arguments = safe_realloc(arguments, (arg_count + 1) * sizeof(ASTNode *));
        arguments[arg_count++] = arg;

        if (parser->current_token.type == TOKEN_RPAREN) {
            break;
        }
        if (parser->current_token.type != TOKEN_COMMA) {
            fprintf(stderr, "Expected ',' or ')' after argument\n");
            exit(EXIT_FAILURE);
        }
        advance(parser);
    }
    advance(parser);

    if (parser->current_token.type != TOKEN_SEMICOLON) {
        fprintf(stderr, "Expected ';' after function call\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_FUNC_CALL;
    node->data.func_call.name = func_name;
    node->data.func_call.arguments = arguments;
    node->data.func_call.arg_count = arg_count;

    return node;
}

ASTNode *parse_return(Parser* parser) {
    advance(parser);

    ASTNode *expr = parse_expression(parser);

    if (parser->current_token.type != TOKEN_SEMICOLON) {
        fprintf(stderr, "Error: Expected semicolon after return statement\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_RETURN;
    node->data.return_stmt.expr = expr;
    return node;
}

ASTNode *parse_program(Parser *parser) {
    ASTNode **statements = NULL;
    int count = 0;

    while (parser->current_token.type != TOKEN_EOF) {
        ASTNode *stmt = NULL;

        switch (parser->current_token.type) {
            case TOKEN_KEYWORD_NUM:
            case TOKEN_KEYWORD_STR:
            case TOKEN_KEYWORD_NIL:
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
            case TOKEN_KEYWORD_FUN:
                stmt = parse_function_declaration(parser);
                break;
            case TOKEN_KEYWORD_CALL:
                stmt = parse_function_call(parser);
                break;
            case TOKEN_KEYWORD_RETURN:
                stmt = parse_return(parser);
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
