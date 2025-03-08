#include "lexer.h"
#include "memory.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

void init_lexer(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->current_pos = 0;
}

static void skip_whitespace(Lexer *lexer) {
    while (isspace((unsigned char)lexer->source[lexer->current_pos])) {
        lexer->current_pos++;
    }
}

Token next_token(Lexer *lexer) {
    skip_whitespace(lexer);
    char current = lexer->source[lexer->current_pos];

    if (current == '\0') {
        return (Token){TOKEN_EOF, NULL, 0, NULL};
    }

    // Handle identifiers and keywords
    if (isalpha((unsigned char)current)) {
        int start = lexer->current_pos;
        do {
            lexer->current_pos++;
        } while (isalnum((unsigned char)lexer->source[lexer->current_pos]));

        int length = lexer->current_pos - start;
        char *lexeme = safe_malloc(length + 1);
        strncpy(lexeme, &lexer->source[start], length);
        lexeme[length] = '\0';

        TokenType type = TOKEN_IDENTIFIER;
        if (strcmp(lexeme, "num") == 0) type = TOKEN_KEYWORD_NUM;
        else if (strcmp(lexeme, "str") == 0) type = TOKEN_KEYWORD_STR;
        else if (strcmp(lexeme, "var") == 0) type = TOKEN_KEYWORD_VAR;
        else if (strcmp(lexeme, "if") == 0) type = TOKEN_KEYWORD_IF;
        else if (strcmp(lexeme, "while") == 0) type = TOKEN_KEYWORD_WHILE;
        else if (strcmp(lexeme, "print") == 0) type = TOKEN_KEYWORD_PRINT;

        return (Token){type, lexeme, 0, NULL};
    }

    // Handle numbers
    if (isdigit((unsigned char)current) || current == '.') {
        char *end;
        double num = strtod(&lexer->source[lexer->current_pos], &end);
        lexer->current_pos = end - lexer->source;
        return (Token){TOKEN_NUMBER, NULL, num, NULL};
    }

    // Handle strings
    if (current == '"') {
        lexer->current_pos++;
        int start = lexer->current_pos;
        while (lexer->source[lexer->current_pos] != '"' && lexer->source[lexer->current_pos] != '\0') {
            lexer->current_pos++;
        }
        if (lexer->source[lexer->current_pos] != '"') {
            fprintf(stderr, "Error: Unterminated string literal\n");
            exit(EXIT_FAILURE);
        }
        int length = lexer->current_pos - start;
        char *str = safe_malloc(length + 1);
        strncpy(str, &lexer->source[start], length);
        str[length] = '\0';
        lexer->current_pos++;
        return (Token){TOKEN_STRING, NULL, 0, str};
    }

    // Handle single-character tokens
    switch (current) {
        case '=':
            if (lexer->source[lexer->current_pos + 1] == '=') {
                lexer->current_pos += 2;
                return (Token){TOKEN_OPERATOR_EQUAL, NULL, 0, NULL};
            }
            lexer->current_pos++;
            return (Token){TOKEN_OPERATOR_EQUAL, NULL, 0, NULL};
        case '<':
            lexer->current_pos++;
            return (Token){TOKEN_OPERATOR_LESS, NULL, 0, NULL};
        case '>':
            lexer->current_pos++;
            return (Token){TOKEN_OPERATOR_GREATER, NULL, 0, NULL};
        case '+':
            lexer->current_pos++;
            if (lexer->source[lexer->current_pos] == '+') {
                lexer->current_pos++;
                return (Token){TOKEN_OPERATOR_PLUS_PLUS, NULL, 0, NULL};
            }
            return (Token){TOKEN_OPERATOR_PLUS, NULL, 0, NULL};
        case '(':
            lexer->current_pos++;
            return (Token){TOKEN_LPAREN, NULL, 0, NULL};
        case ')':
            lexer->current_pos++;
            return (Token){TOKEN_RPAREN, NULL, 0, NULL};
        case '{':
            lexer->current_pos++;
            return (Token){TOKEN_LBRACE, NULL, 0, NULL};
        case '}':
            lexer->current_pos++;
            return (Token){TOKEN_RBRACE, NULL, 0, NULL};
        case ';':
            lexer->current_pos++;
            return (Token){TOKEN_SEMICOLON, NULL, 0, NULL};
        default:
            fprintf(stderr, "Error: Unexpected character '%c' (ASCII %d)\n", current, current);
            exit(EXIT_FAILURE);
    }
}