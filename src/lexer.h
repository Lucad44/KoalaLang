#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_KEYWORD_NUM,
    TOKEN_KEYWORD_STR,
    TOKEN_KEYWORD_VAR,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_PRINT,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_OPERATOR_PLUS,
    TOKEN_OPERATOR_PLUS_PLUS,
    TOKEN_OPERATOR_LESS,
    TOKEN_OPERATOR_LESS_EQUAL,
    TOKEN_OPERATOR_GREATER,
    TOKEN_OPERATOR_GREATER_EQUAL,
    TOKEN_OPERATOR_EQUAL,
    TOKEN_SEMICOLON,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char *lexeme;
    double num_value;
    char *str_value;
} Token;

typedef struct {
    const char *source;
    int current_pos;
} Lexer;

void init_lexer(Lexer *lexer, const char *source);

Token next_token(Lexer *lexer);

#endif