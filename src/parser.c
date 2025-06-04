#include "parser.h"
#include "ast.h"
#include "hashmap.h"
#include "lexer.h"
#include "memory.h"
#include "variables.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "functions.h"
#include "modules.h"

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
    if (parser->current_token.type == TOKEN_OPERATOR_MINUS) {
        advance(parser);
        ASTNode *operand = parse_primary(parser);
        ASTNode *unary_node = safe_malloc(sizeof(ASTNode));
        unary_node->type = NODE_EXPR_UNARY;
        unary_node->data.unary_expr.op = OP_NEGATE;
        unary_node->data.unary_expr.operand = operand;
        return unary_node;
    }

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
            node->type = NODE_NUM_LITERAL;
            node->data.num_literal.num_val = parser->current_token.num_value;
            advance(parser);
            break;
            
        case TOKEN_STRING:
            node->type = NODE_STR_LITERAL;
            node->data.str_literal.str_val = strdup(parser->current_token.str_value);
            advance(parser);
            break;
            
        case TOKEN_LBRACKET:
            // Handle list literals in expressions
            free(node); // Free the allocated node since parse_list_literal creates its own
            return parse_list_literal(parser);
            
        case TOKEN_KEYWORD_CALL:
            // Handle function calls in expressions
            free(node); // Free the allocated node since parse_function_call creates its own
            return parse_function_call(parser);

        case TOKEN_IDENTIFIER: {
            // Handle variable references and module-qualified variables
            char *first = strdup(parser->current_token.lexeme);
            advance(parser);

            char *var_name = NULL;
            char *module_name = NULL;

            // Check if it's module.var
            if (parser->current_token.type == TOKEN_DOT) {
                module_name = first;
                advance(parser);

                if (parser->current_token.type != TOKEN_IDENTIFIER) {
                    fprintf(stderr, "Expected variable name after '%s.'\n", module_name);
                    free(module_name);
                    exit(EXIT_FAILURE);
                }

                var_name = strdup(parser->current_token.lexeme);
                advance(parser);
            } else {
                // Plain variable
                var_name = first;
            }

            // By default, assume this is a simple variable expression
            node->type = NODE_EXPR_VARIABLE;
            node->data.variable.name = var_name;
            node->data.variable.module_name = module_name;

            // Handle list/array access with brackets
            if (parser->current_token.type == TOKEN_LBRACKET) {
                // First index - transform to variable access node
                advance(parser);
                ASTNode *index_expr = parse_expression(parser);

                if (parser->current_token.type != TOKEN_RBRACKET) {
                    fprintf(stderr, "\nError: Expected ']' after index expression.\n");
                    exit(EXIT_FAILURE);
                }
                advance(parser);

                node->type = NODE_VARIABLE_ACCESS;
                node->data.variable_access.name = var_name;
                node->data.variable_access.index_expr = index_expr;
                node->data.variable_access.parent_expr = NULL;

                // Attach module name for top-level access
                if (module_name != NULL) {
                    // Wrap this variable access in a new node for module scoping
                    ASTNode *wrapped = safe_malloc(sizeof(ASTNode));
                    *wrapped = *node;

                    node->type = NODE_EXPR_VARIABLE;
                    node->data.variable.name = var_name;
                    node->data.variable.module_name = module_name;

                    // Promote to access using parent_expr
                    ASTNode *access = safe_malloc(sizeof(ASTNode));
                    access->type = NODE_VARIABLE_ACCESS;
                    access->data.variable_access.name = NULL;
                    access->data.variable_access.index_expr = index_expr;
                    access->data.variable_access.parent_expr = node;

                    *node = *access;
                    free(access);
                }

                // Handle nested access (multi-dimensional)
                while (parser->current_token.type == TOKEN_LBRACKET) {
                    ASTNode *outer_access = safe_malloc(sizeof(ASTNode));
                    *outer_access = *node;

                    advance(parser);
                    ASTNode *next_index_expr = parse_expression(parser);

                    if (parser->current_token.type != TOKEN_RBRACKET) {
                        fprintf(stderr, "\nError: Expected ']' after nested index expression.\n");
                        exit(EXIT_FAILURE);
                    }
                    advance(parser);

                    node->type = NODE_VARIABLE_ACCESS;
                    node->data.variable_access.name = NULL;
                    node->data.variable_access.index_expr = next_index_expr;
                    node->data.variable_access.parent_expr = outer_access;
                }
            }
            break;
        }

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
        {TOKEN_OPERATOR_BITWISE_OR, OP_BITWISE_OR, 1},
        {TOKEN_OPERATOR_BITWISE_XOR, OP_BITWISE_XOR, 2},
        {TOKEN_OPERATOR_BITWISE_AND, OP_BITWISE_AND, 3},

        {TOKEN_OPERATOR_LOGICAL_AND, OP_LOGICAL_AND, 4},
        {TOKEN_OPERATOR_LOGICAL_OR, OP_LOGICAL_OR, 4},
        {TOKEN_OPERATOR_LOGICAL_XOR, OP_LOGICAL_XOR, 4},

        {TOKEN_OPERATOR_EQUAL, OP_EQUAL, 7},
        {TOKEN_OPERATOR_NOT_EQUAL, OP_NOT_EQUAL, 7},

        {TOKEN_OPERATOR_LESS, OP_LESS, 8},
        {TOKEN_OPERATOR_GREATER, OP_GREATER, 8},
        {TOKEN_OPERATOR_LESS_EQUAL, OP_LESS_EQUAL, 8},
        {TOKEN_OPERATOR_GREATER_EQUAL, OP_GREATER_EQUAL, 8},

        {TOKEN_OPERATOR_PLUS, OP_PLUS, 10},
        {TOKEN_OPERATOR_MINUS, OP_MINUS, 10},

        {TOKEN_OPERATOR_MULTIPLY, OP_MULTIPLY, 11},
        {TOKEN_OPERATOR_DIVIDE, OP_DIVIDE, 11},
        {TOKEN_OPERATOR_MODULO, OP_MODULO, 11},

        {TOKEN_OPERATOR_POWER, OP_POWER, 12}
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

ASTNode *parse_if(Parser *parser) {
    advance(parser);
    if (parser->current_token.type != TOKEN_LPAREN) {
        fprintf(stderr, "Expected '(' after 'if'\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);
    ASTNode *condition = parse_expression(parser);

    if (parser->current_token.type != TOKEN_RPAREN) {
        fprintf(stderr, "Expected ')' after if condition, got %d (lexeme: %s)\n",
               parser->current_token.type,
               parser->current_token.lexeme ? parser->current_token.lexeme : "NULL");
        free_ast(condition);
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
            free_ast(node->data.if_stmt.condition);
            free_ast(node->data.if_stmt.body);
            for (int i = 0; i < node->data.if_stmt.elif_count; ++i) {
                free_ast(node->data.if_stmt.elif_nodes[i]->data.if_stmt.condition);
                free_ast(node->data.if_stmt.elif_nodes[i]->data.if_stmt.body);
                free(node->data.if_stmt.elif_nodes[i]);
            }
            free(node->data.if_stmt.elif_nodes);
            free(node);
            exit(EXIT_FAILURE);
        }
        advance(parser);
        ASTNode *elif_condition = parse_expression(parser);

        if (parser->current_token.type != TOKEN_RPAREN) {
            fprintf(stderr, "Expected ')' after elif condition\n");
            free_ast(elif_condition);
            free_ast(node->data.if_stmt.condition);
            free_ast(node->data.if_stmt.body);
             for (int i = 0; i < node->data.if_stmt.elif_count; ++i) {
                 free_ast(node->data.if_stmt.elif_nodes[i]->data.if_stmt.condition);
                 free_ast(node->data.if_stmt.elif_nodes[i]->data.if_stmt.body);
                 free(node->data.if_stmt.elif_nodes[i]);
             }
             free(node->data.if_stmt.elif_nodes);
             free(node);
            exit(EXIT_FAILURE);
        }
        advance(parser);

        ASTNode *elif_body = parse_braced_block(parser);

        ASTNode *elif_node = safe_malloc(sizeof(ASTNode));
        elif_node->type = NODE_ELIF;
        elif_node->data.if_stmt.condition = elif_condition;
        elif_node->data.if_stmt.body = elif_body;
        elif_node->data.if_stmt.elif_nodes = NULL;
        elif_node->data.if_stmt.elif_count = 0;
        elif_node->data.if_stmt.else_body = NULL;
        node->data.if_stmt.elif_count++;
        node->data.if_stmt.elif_nodes = safe_realloc(node->data.if_stmt.elif_nodes,
            node->data.if_stmt.elif_count * sizeof(ASTNode*));
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
    ASTNode *condition = parse_expression(parser);

    if (parser->current_token.type != TOKEN_RPAREN) {
        fprintf(stderr, "Expected ')' after while condition, got %d (lexeme: %s)\n",
               parser->current_token.type,
               parser->current_token.lexeme ? parser->current_token.lexeme : "NULL");
        free_ast(condition);
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
            case TOKEN_KEYWORD_IMPORT:
                stmt = parse_import(parser);
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

ASTNode *parse_function_declaration(Parser *parser) {
    advance(parser);

    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected function name\n");
        exit(EXIT_FAILURE);
    }
    char *func_name = strdup(parser->current_token.lexeme);

    if (get_function_meta_from_modules(imported_modules, func_name)) {
        fprintf(stderr, "Function '%s' already defined in an imported module."
                        "\nCan't overwrite functions.\n"
                        "It is possible to see a full list of functions for each module in the documentation.", func_name);
        exit(EXIT_FAILURE);
    }

    if (hashmap_get(function_map, &(Function) { .name = func_name }) != NULL) {
        fprintf(stderr, "Function '%s' already defined\n.", func_name);
        exit(EXIT_FAILURE);
    }

    advance(parser);

    if (parser->current_token.type != TOKEN_LPAREN) {
        fprintf(stderr, "Expected '(' after function name\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    Parameter *parameters = NULL;
    int param_count = 0;

    while (parser->current_token.type != TOKEN_RPAREN) {
        Parameter param = {0};
        
        // Parse parameter type (including nested lists)
        if (!parse_parameter_type(parser, &param)) {
            fprintf(stderr, "Failed to parse parameter type\n");
            free(parameters);
            exit(EXIT_FAILURE);
        }

        if (parser->current_token.type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Expected parameter name\n");
            free(parameters);
            exit(EXIT_FAILURE);
        }
        param.name = strdup(parser->current_token.lexeme);
        advance(parser);

        parameters = safe_realloc(parameters, (param_count + 1) * sizeof(Parameter));
        parameters[param_count] = param;
        param_count++;

        if (parser->current_token.type == TOKEN_RPAREN) {
            break;
        }
        if (parser->current_token.type != TOKEN_COMMA) {
            fprintf(stderr, "Expected ',' or ')' after parameter\n");
            free(parameters);
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
    // consume 'call'
    advance(parser);
    
    // must have at least an identifier
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected function name or module name after 'call'\n");
        exit(EXIT_FAILURE);
    }
    
    // grab the first identifier (either a func name or module name)
    char *first = strdup(parser->current_token.lexeme);
    advance(parser);
    
    char *module_name = NULL;
    char *func_name = NULL;
    
    // check for module-qualified call: module.func
    if (parser->current_token.type == TOKEN_DOT) {
        // 'module' is in `first`
        module_name = first;
        advance(parser); // consume '.'
        
        if (parser->current_token.type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Expected function name after '%s.'\n", module_name);
            free(module_name);
            exit(EXIT_FAILURE);
        }
        
        // now this identifier is the actual function name
        func_name = strdup(parser->current_token.lexeme);
        advance(parser);
    } else {
        // no module; `first` is the function name
        func_name = first;
    }
    
    // expect '('
    if (parser->current_token.type != TOKEN_LPAREN) {
        fprintf(stderr, "Expected '(' after function name '%s'\n", func_name);
        free(module_name);
        free(func_name);
        exit(EXIT_FAILURE);
    }
    advance(parser);
    
    // parse arguments
    ASTNode **arguments = NULL;
    int arg_count = 0;
    
    while (parser->current_token.type != TOKEN_RPAREN) {
        ASTNode *arg;
        if (parser->current_token.type == TOKEN_LBRACKET) {
            arg = parse_list_literal(parser);
        } else {
            arg = parse_expression(parser);
        }
        
        arguments = safe_realloc(arguments, (arg_count + 1) * sizeof(ASTNode *));
        arguments[arg_count++] = arg;
        
        if (parser->current_token.type == TOKEN_COMMA) {
            advance(parser);
        } else if (parser->current_token.type != TOKEN_RPAREN) {
            fprintf(stderr, "Expected ',' or ')' after argument in function call '%s'\n", func_name);
            for (int i = 0; i < arg_count; ++i) free_ast(arguments[i]);
            free(arguments);
            free(module_name);
            free(func_name);
            exit(EXIT_FAILURE);
        }
    }
    advance(parser); // consume ')'
    
    // DO NOT require semicolon here - let the calling context handle statement termination
    
    // build AST node
    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_FUNC_CALL;
    node->data.func_call.name = func_name;
    node->data.func_call.arguments = arguments;
    node->data.func_call.arg_count = arg_count;
    node->data.func_call.module_name = module_name; // NULL for unqualified calls
    return node;
}


ASTNode *parse_return(Parser* parser) {
    advance(parser);
    ASTNode *expr = parse_expression(parser);
    if (parser->current_token.type != TOKEN_SEMICOLON) {
        fprintf(stderr, "\nError: Expected semicolon after return statement\n");
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
            case TOKEN_KEYWORD_LIST:
                stmt = parse_list_declaration(parser);
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
            case TOKEN_KEYWORD_IMPORT:
                stmt = parse_import(parser);
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

ASTNode *parse_list_literal(Parser *parser) {
    if (parser->current_token.type != TOKEN_LBRACKET) {
        fprintf(stderr, "Internal Parser Error: Expected '[' for list literal.\n");
        exit(EXIT_FAILURE);
    }
    advance(parser); // consume '['

    // Collect elements
    ASTNode **elements = NULL;
    int element_count = 0;

    while (parser->current_token.type != TOKEN_RBRACKET) {
        ASTNode *elem;
        if (parser->current_token.type == TOKEN_LBRACKET) {
            // Nested list: recurse
            elem = parse_list_literal(parser);
        } else {
            // Primitive literal: number or string
            if (parser->current_token.type == TOKEN_NUMBER) {
                elem = safe_malloc(sizeof(ASTNode));
                elem->type = NODE_NUM_LITERAL;
                elem->data.num_literal.num_val = parser->current_token.num_value;
                advance(parser);
            } else if (parser->current_token.type == TOKEN_STRING) {
                elem = safe_malloc(sizeof(ASTNode));
                elem->type = NODE_STR_LITERAL;
                elem->data.str_literal.str_val = strdup(parser->current_token.str_value);
                advance(parser);
            } else {
                fprintf(stderr, "Unexpected token in list literal: %d\n", parser->current_token.type);
                exit(EXIT_FAILURE);
            }
        }

        // Append to array
        elements = safe_realloc(elements, (element_count + 1) * sizeof(ASTNode *));
        elements[element_count++] = elem;

        // Comma separation
        if (parser->current_token.type == TOKEN_COMMA) {
            advance(parser);
            continue;
        }
        break;
    }

    if (parser->current_token.type != TOKEN_RBRACKET) {
        fprintf(stderr, "Expected ']' at end of list literal, got %d\n", parser->current_token.type);
        exit(EXIT_FAILURE);
    }
    advance(parser); // consume ']'

    // Determine list metadata
    VarType element_type;
    VarType nested_element_type = VAR_NUM; /* default */
    bool is_nested = false;

    if (element_count > 0 && elements && elements[0]->type == NODE_LIST_LITERAL) {
        // Immediate child is a list
        is_nested = true;
        element_type = VAR_LIST;
        // nested_element_type = type of elements inside those child lists
        nested_element_type = elements[0]->data.list_literal.element_type;
    } else if (element_count > 0 && elements) {
        // primitives
        is_nested = false;
        if (elements[0]->type == NODE_NUM_LITERAL) {
            element_type = VAR_NUM;
        } else {
            element_type = VAR_STR;
        }
    } else {
        // Empty list defaults to num
        element_type = VAR_NUM;
    }

    // Build AST node
    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_LIST_LITERAL;
    node->data.list_literal.element_type = element_type;
    node->data.list_literal.nested_element_type = nested_element_type;
    node->data.list_literal.is_nested = is_nested;
    node->data.list_literal.elements = elements;
    node->data.list_literal.element_count = element_count;
    return node;
}


ASTNode *parse_list_declaration(Parser *parser) {
    advance(parser); // Skip past the 'list' keyword

    // Parse the list type (nested or simple)
    ASTNode *type_node = parse_list_type(parser);
    VarType element_type = type_node->data.list_decl.element_type;
    bool is_nested_list = type_node->data.list_decl.is_nested_list;
    VarType nested_element_type = type_node->data.list_decl.nested_element_type;
    free(type_node);

    char *name = NULL;
    ASTNode *init_expr = NULL;

    // If the next token is an identifier, consume it as the variable name
    if (parser->current_token.type == TOKEN_IDENTIFIER) {
        name = strdup(parser->current_token.lexeme);
        advance(parser);
    }

    // If we see '=', parse the initialization expression
    if (parser->current_token.type == TOKEN_OPERATOR_EQUAL) {
        advance(parser);
        
        if (parser->current_token.type == TOKEN_LBRACKET) {
            // List literal initialization
            init_expr = parse_list_literal(parser);
        } else if (parser->current_token.type == TOKEN_KEYWORD_CALL) {
            // Function call initialization
            init_expr = parse_function_call(parser);
        } else {
            // Any other expression (variable reference, etc.)
            init_expr = parse_expression(parser);
        }
    }

    // Handle semicolon for named declarations
    if (name != NULL) {
        // Named declarations require a semicolon
        if (parser->current_token.type != TOKEN_SEMICOLON) {
            fprintf(stderr, "Expected ';' after named list declaration\n");
            free(name);
            free_ast(init_expr);
            exit(EXIT_FAILURE);
        }
        advance(parser);
    }
    // For anonymous declarations (like in expressions), don't require semicolon

    // Build the AST node
    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_LIST_DECL;
    node->data.list_decl.name = name;                      // NULL for anonymous
    node->data.list_decl.element_type = element_type;
    node->data.list_decl.is_nested_list = is_nested_list;
    node->data.list_decl.nested_element_type = nested_element_type;
    node->data.list_decl.init_expr = init_expr;
    return node;
}

ASTNode *parse_expression_statement(Parser *parser) {
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Internal Parser \nError: Expected identifier.\n");
        exit(EXIT_FAILURE);
    }

    // Grab first identifier (could be variable name or module name)
    char *first = strdup(parser->current_token.lexeme);
    advance(parser);

    char *module_name = NULL;
    char *var_name = NULL;

    // Handle optional module.var
    if (parser->current_token.type == TOKEN_DOT) {
        module_name = first;
        advance(parser);
        if (parser->current_token.type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "\nError: Expected variable name after '%s.'\n", module_name);
            free(module_name);
            exit(EXIT_FAILURE);
        }
        var_name = strdup(parser->current_token.lexeme);
        advance(parser);
    } else {
        var_name = first;
    }

    ASTNode *node = NULL;

    // Check for list-style assignment
    if (parser->current_token.type == TOKEN_LBRACKET) {
        // 1) Parse the *first* [ expr ]
        advance(parser);  // skip '['
        ASTNode *first_idx = parse_expression(parser);
        if (parser->current_token.type != TOKEN_RBRACKET) {
            fprintf(stderr, "\nError: Expected ']' after list index in assignment target.\n");
            free(var_name);
            free_ast(first_idx);
            exit(EXIT_FAILURE);
        }
        advance(parser);  // skip ']'

        // 2) Build the initial access node: var[first_idx]
        ASTNode *access = safe_malloc(sizeof(ASTNode));
        access->type = NODE_VARIABLE_ACCESS;
        access->data.variable_access.name = var_name;
        access->data.variable_access.index_expr = first_idx;
        access->data.variable_access.parent_expr = NULL;

        // Handle module scoping by wrapping in an expression node
        if (module_name != NULL) {
            ASTNode *module_expr = safe_malloc(sizeof(ASTNode));
            module_expr->type = NODE_EXPR_VARIABLE;
            module_expr->data.variable.name = var_name;
            module_expr->data.variable.module_name = module_name;

            ASTNode *wrapped_access = safe_malloc(sizeof(ASTNode));
            wrapped_access->type = NODE_VARIABLE_ACCESS;
            wrapped_access->data.variable_access.name = NULL;
            wrapped_access->data.variable_access.index_expr = first_idx;
            wrapped_access->data.variable_access.parent_expr = module_expr;

            access = wrapped_access;
        }

        // 3) Handle further “[ expr ]” nesting
        while (parser->current_token.type == TOKEN_LBRACKET) {
            advance(parser);  // skip '['
            ASTNode *idx = parse_expression(parser);
            if (parser->current_token.type != TOKEN_RBRACKET) {
                fprintf(stderr, "\nError: Expected ']' after list index in assignment target.\n");
                free_ast(idx);
                free_ast(access);
                exit(EXIT_FAILURE);
            }
            advance(parser);  // skip ']'

            ASTNode *wrapper = safe_malloc(sizeof(ASTNode));
            wrapper->type = NODE_VARIABLE_ACCESS;
            wrapper->data.variable_access.name = NULL;
            wrapper->data.variable_access.index_expr = idx;
            wrapper->data.variable_access.parent_expr = access;
            access = wrapper;
        }

        // 4) Require '='
        if (parser->current_token.type != TOKEN_OPERATOR_EQUAL) {
            fprintf(stderr, "\nError: Expected '=' after list index target in assignment.\n");
            free_ast(access);
            exit(EXIT_FAILURE);
        }
        advance(parser);

        // 5) Parse RHS
        ASTNode *value_expr = parse_expression(parser);

        // 6) Construct assignment node
        node = safe_malloc(sizeof(ASTNode));
        node->type = NODE_ASSIGNMENT;
        node->data.assignment.target_name = NULL;  // Using target_access instead
        node->data.assignment.index_expr = NULL;
        node->data.assignment.target_access = access;
        node->data.assignment.value_expr = value_expr;
    } else {
        // Simple assignment: var = expr;
        if (parser->current_token.type != TOKEN_OPERATOR_EQUAL) {
            fprintf(stderr, "\nError: Expected '=' after variable name in assignment.\n");
            free(var_name);
            if (module_name) free(module_name);
            exit(EXIT_FAILURE);
        }
        advance(parser);

        ASTNode *value_expr = parse_expression(parser);

        node = safe_malloc(sizeof(ASTNode));
        node->type = NODE_ASSIGNMENT;
        node->data.assignment.target_name = var_name;
        node->data.assignment.index_expr = NULL;
        node->data.assignment.target_access = NULL;
        node->data.assignment.value_expr = value_expr;

        // Attach module name if present
        if (module_name != NULL) {
            ASTNode *access = safe_malloc(sizeof(ASTNode));
            access->type = NODE_EXPR_VARIABLE;
            access->data.variable.name = var_name;
            access->data.variable.module_name = module_name;

            node->data.assignment.target_name = NULL;
            node->data.assignment.target_access = access;
        }
    }

    return node;
}

ASTNode *parse_list_type(Parser *parser) {
    if (parser->current_token.type != TOKEN_LBRACKET) {
        fprintf(stderr, "Expected '[' in list type specifier\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    VarType element_type;
    bool is_nested_list = false;
    VarType nested_element_type = VAR_NUM; // Default, will be overridden if nested

    if (parser->current_token.type == TOKEN_KEYWORD_NUM) {
        element_type = VAR_NUM;
        advance(parser);
    } else if (parser->current_token.type == TOKEN_KEYWORD_STR) {
        element_type = VAR_STR;
        advance(parser);
    } else if (parser->current_token.type == TOKEN_KEYWORD_LIST) {
        element_type = VAR_LIST;
        is_nested_list = true;
        advance(parser);
        // Parse the nested list type
        ASTNode *nested_type = parse_list_type(parser);
        nested_element_type = nested_type->data.list_decl.element_type;
        free(nested_type); // We don't need the full AST node, just the type info
    } else {
        fprintf(stderr, "Expected 'num', 'str', or 'list' inside list type specifier '[ ]'\n");
        exit(EXIT_FAILURE);
    }

    if (parser->current_token.type != TOKEN_RBRACKET) {
        fprintf(stderr, "Expected ']' after list element type\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    // Create a temporary node to return type information
    ASTNode *type_node = safe_malloc(sizeof(ASTNode));
    type_node->type = NODE_LIST_DECL;
    type_node->data.list_decl.element_type = element_type;
    type_node->data.list_decl.is_nested_list = is_nested_list;
    type_node->data.list_decl.nested_element_type = nested_element_type;

    return type_node;
}

ASTNode *parse_assignment(Parser *parser) {
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "\nError: Expected variable name for assignment.\n");
        exit(EXIT_FAILURE);
    }

    char *target_name = strdup(parser->current_token.lexeme);
    advance(parser);

    // Handle array/list indexing with brackets
    ASTNode *first_index = NULL;
    ASTNode *parent_access = NULL;

    // Process nested indices if any
    while (parser->current_token.type == TOKEN_LBRACKET) {
        advance(parser);
        ASTNode *index_expr = parse_expression(parser);

        if (parser->current_token.type != TOKEN_RBRACKET) {
            fprintf(stderr, "\nError: Expected ']' after index expression.\n");
            exit(EXIT_FAILURE);
        }
        advance(parser);

        if (first_index == NULL) {
            // First index - remember it
            first_index = index_expr;
        } else {
            // Create a chained access for nested indices
            ASTNode *access_node = safe_malloc(sizeof(ASTNode));
            access_node->type = NODE_VARIABLE_ACCESS;

            if (parent_access == NULL) {
                // This is the second index in the chain
                access_node->data.variable_access.name = target_name;
                access_node->data.variable_access.index_expr = first_index;
                parent_access = access_node;
            } else {
                // Link to previous access node in the chain
                access_node->data.variable_access.name = NULL;  // Indicates nested access
                access_node->data.variable_access.index_expr = index_expr;
                access_node->data.variable_access.parent_expr = parent_access;
                parent_access = access_node;
            }
        }
    }

    if (parser->current_token.type != TOKEN_OPERATOR_EQUAL) {
        fprintf(stderr, "\nError: Expected '=' after variable name or indexing.\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    ASTNode *value_expr = parse_expression(parser);

    if (parser->current_token.type != TOKEN_SEMICOLON) {
        fprintf(stderr, "\nError: Expected ';' after assignment expression.\n");
        exit(EXIT_FAILURE);
    }
    advance(parser);

    ASTNode *node = safe_malloc(sizeof(ASTNode));
    node->type = NODE_ASSIGNMENT;

    if (parent_access != NULL) {
        // We have nested access - this is handled specially
        node->data.assignment.target_name = NULL;  // Indicates nested assignment
        node->data.assignment.index_expr = NULL;   // Not used for nested assignment
        node->data.assignment.target_access = parent_access;  // Pass the access AST
        node->data.assignment.value_expr = value_expr;
    } else {
        // Standard assignment with at most one index
        node->data.assignment.target_name = target_name;
        node->data.assignment.index_expr = first_index;  // May be NULL for direct assignment
        node->data.assignment.target_access = NULL;      // Not used for standard assignment
        node->data.assignment.value_expr = value_expr;
    }

    return node;
}

ASTNode *parse_import(Parser *parser) {
    advance(parser);  // consume 'import'

    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "\nError: Expected module name after 'import'.\n");
        exit(EXIT_FAILURE);
    }

    char *module_name = strdup(parser->current_token.lexeme);
    advance(parser);  // consume module name

    if (parser->current_token.type != TOKEN_SEMICOLON) {
        fprintf(stderr, "\nError: Expected ';' after module name.\n");
        free(module_name);
        exit(EXIT_FAILURE);
    }

    advance(parser);  // consume ';'

    // Get the module from the module map
    const Module *module = hashmap_get(module_map, &(Module) { .name = module_name });
    if (!module) {
        fprintf(stderr, "\nError: Module '%s' not found.\n", module_name);
        free(module_name);
        exit(EXIT_FAILURE);
    }

    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "\nError: Memory allocation failed for import node.\n");
        free(module_name);
        exit(EXIT_FAILURE);
    }
    node->type = NODE_IMPORT;

    // Allocate memory for the Module struct in the ImportNode
    node->data.import.module = malloc(sizeof(Module));
    if (!node->data.import.module) {
        fprintf(stderr, "\nError: Memory allocation failed for module.\n");
        free(module_name);
        free(node);
        exit(EXIT_FAILURE);
    }

    // Copy module name
    node->data.import.module->name = strdup(module->name);
    if (!node->data.import.module->name) {
        fprintf(stderr, "\nError: Memory allocation failed for module name.\n");
        free(module_name);
        free(node->data.import.module);
        free(node);
        exit(EXIT_FAILURE);
    }

    // Deep copy the function meta map
    node->data.import.module->module_function_meta_map = hashmap_deep_copy(module->module_function_meta_map, deep_copy_function_meta, NULL);
    node->data.import.module->module_variables_map = hashmap_deep_copy(module->module_variables_map, deep_copy_variable, NULL);
    if (!node->data.import.module->module_function_meta_map || !node->data.import.module->module_variables_map) {
        fprintf(stderr, "\nError: Failed to copy function and/or variable map for module '%s'.\n", module_name);
        free(module_name);
        free(node->data.import.module->name);
        free(node->data.import.module);
        free(node);
        exit(EXIT_FAILURE);
    }

    free(module_name);
    return node;
}

bool parse_parameter_type(Parser *parser, Parameter *param) {
    if (parser->current_token.type == TOKEN_KEYWORD_LIST) {
        param->is_list = true;
        advance(parser);
        
        if (parser->current_token.type != TOKEN_LBRACKET) {
            fprintf(stderr, "Expected '[' after 'list' in parameter type\n");
            return false;
        }
        advance(parser);

        // Check for nested list
        if (parser->current_token.type == TOKEN_KEYWORD_LIST) {
            param->is_nested = true;
            advance(parser);
            
            if (parser->current_token.type != TOKEN_LBRACKET) {
                fprintf(stderr, "Expected '[' after nested 'list' in parameter type\n");
                return false;
            }
            advance(parser);
            
            // Parse inner type for nested list
            if (parser->current_token.type == TOKEN_KEYWORD_NUM) {
                param->nested_element_type = VAR_NUM;
            } else if (parser->current_token.type == TOKEN_KEYWORD_STR) {
                param->nested_element_type = VAR_STR;
            } else {
                fprintf(stderr, "Expected 'num' or 'str' for nested list element type\n");
                return false;
            }
            advance(parser);
            
            if (parser->current_token.type != TOKEN_RBRACKET) {
                fprintf(stderr, "Expected ']' after nested list element type\n");
                return false;
            }
            advance(parser);
            
            // Set outer type to list for nested lists
            param->type = VAR_LIST;
        } else {
            // Simple list
            param->is_nested = false;
            if (parser->current_token.type == TOKEN_KEYWORD_NUM) {
                param->type = VAR_NUM;
            } else if (parser->current_token.type == TOKEN_KEYWORD_STR) {
                param->type = VAR_STR;
            } else {
                fprintf(stderr, "Expected 'num' or 'str' inside list parameter type '[ ]'\n");
                return false;
            }
            advance(parser);
        }
        
        if (parser->current_token.type != TOKEN_RBRACKET) {
            fprintf(stderr, "Expected ']' after list parameter element type\n");
            return false;
        }
        advance(parser);
        
    } else if (parser->current_token.type == TOKEN_KEYWORD_NUM) {
        param->is_list = false;
        param->is_nested = false;
        param->type = VAR_NUM;
        advance(parser);
    } else if (parser->current_token.type == TOKEN_KEYWORD_STR) {
        param->is_list = false;
        param->is_nested = false;
        param->type = VAR_STR;
        advance(parser);
    } else {
        fprintf(stderr, "Expected parameter type (num, str, or list[type])\n");
        return false;
    }
    
    return true;
}
    