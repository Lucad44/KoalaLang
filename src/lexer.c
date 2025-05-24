#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "memory.h"

void init_lexer(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->current_pos = 0;
}

static void skip_whitespace(Lexer *lexer) {
    while (isspace(lexer->source[lexer->current_pos])) {
        lexer->current_pos++;
    }
}

Token next_token(Lexer *lexer) {
    skip_whitespace(lexer);
    const char current = lexer->source[lexer->current_pos];

    if (current == '\0') {
        return (Token) {TOKEN_EOF, NULL, 0, NULL, NULL};
    }

    if (isalpha(current)) {
        const int start = lexer->current_pos;
        do {
            lexer->current_pos++;
        } while (isalnum(lexer->source[lexer->current_pos]) || lexer->source[lexer->current_pos] == '_');

        const int length = lexer->current_pos - start;
        char *lexeme = safe_malloc(length + 1);
        strncpy(lexeme, &lexer->source[start], length);
        lexeme[length] = '\0';

        TokenType type = TOKEN_IDENTIFIER;
        if (strcmp(lexeme, "num") == 0) type = TOKEN_KEYWORD_NUM;
        else if (strcmp(lexeme, "str") == 0) type = TOKEN_KEYWORD_STR;
        else if (strcmp(lexeme, "list") == 0) type = TOKEN_KEYWORD_LIST;
        else if (strcmp(lexeme, "nil") == 0) type = TOKEN_KEYWORD_NIL;
        else if (strcmp(lexeme, "var") == 0) type = TOKEN_KEYWORD_VAR;
        else if (strcmp(lexeme, "if") == 0) type = TOKEN_KEYWORD_IF;
        else if (strcmp(lexeme, "elif") == 0) type = TOKEN_KEYWORD_ELIF;
        else if (strcmp(lexeme, "else") == 0) type = TOKEN_KEYWORD_ELSE;
        else if (strcmp(lexeme, "while") == 0) type = TOKEN_KEYWORD_WHILE;
        else if (strcmp(lexeme, "print") == 0) type = TOKEN_KEYWORD_PRINT;
        else if (strcmp(lexeme, "fun") == 0) type = TOKEN_KEYWORD_FUN;
        else if (strcmp(lexeme, "call") == 0) type = TOKEN_KEYWORD_CALL;
        else if (strcmp(lexeme, "return") == 0) type = TOKEN_KEYWORD_RETURN;
        else if (strcmp(lexeme, "import") == 0) type = TOKEN_KEYWORD_IMPORT;

        return (Token) {type, lexeme, 0, NULL, NULL};
    }

    if (isdigit(current) || (current == '.' && isdigit(lexer->source[lexer->current_pos + 1]))) { // Improved number parsing
        char *end;
        const double num = strtod(&lexer->source[lexer->current_pos], &end);
        lexer->current_pos = end - lexer->source;
        return (Token) {TOKEN_NUMBER, NULL, num, NULL, NULL};
    }

    if (current == '"') {
        lexer->current_pos++;
        const int start = lexer->current_pos;
        while (lexer->source[lexer->current_pos] != '"' && lexer->source[lexer->current_pos] != '\0') {
            lexer->current_pos++;
        }
        if (lexer->source[lexer->current_pos] != '"') {
            fprintf(stderr, "\nError: Unterminated string literal\n");
            exit(EXIT_FAILURE);
        }
        const int length = lexer->current_pos - start;
        char *str = safe_malloc(length + 1);
        strncpy(str, &lexer->source[start], length);
        str[length] = '\0';
        lexer->current_pos++;
        return (Token) {TOKEN_STRING, NULL, 0, str, NULL};
    }

    switch (current) {
        case '$':
            lexer->current_pos++;
            return (Token) {TOKEN_OPERATOR_CONCAT, NULL, 0, NULL, NULL};
        case '=':
            if (lexer->source[lexer->current_pos + 1] == '=') {
                lexer->current_pos += 2;
                return (Token) {TOKEN_OPERATOR_EQUAL, NULL, 0, NULL, NULL}; // Keep for comparison ==
            }
            lexer->current_pos++;
            return (Token) {TOKEN_OPERATOR_EQUAL, NULL, 0, NULL, NULL}; // Keep for assignment =
        case '<':
            if (lexer->source[lexer->current_pos + 1] == '=') {
                lexer->current_pos += 2;
                return (Token) {TOKEN_OPERATOR_LESS_EQUAL, NULL, 0, NULL, NULL};
            }
            lexer->current_pos++;
            return (Token) {TOKEN_OPERATOR_LESS, NULL, 0, NULL, NULL};
        case '>':
            if (lexer->source[lexer->current_pos + 1] == '=') {
                lexer->current_pos += 2;
                return (Token) {TOKEN_OPERATOR_GREATER_EQUAL, NULL, 0, NULL, NULL};
            }
            lexer->current_pos++;
            return (Token) {TOKEN_OPERATOR_GREATER, NULL, 0, NULL, NULL};
        case '!':
            if (lexer->source[lexer->current_pos + 1] == '=') {
                lexer->current_pos += 2;
                return (Token) {TOKEN_OPERATOR_NOT_EQUAL, NULL, 0, NULL, NULL};
            }
            lexer->current_pos++;
            return (Token) {TOKEN_OPERATOR_NOT, NULL, 0, NULL, NULL};
        case '+':
            if (lexer->source[lexer->current_pos + 1] == '+') {
                lexer->current_pos += 2;
                return (Token) {TOKEN_OPERATOR_PLUS_PLUS, NULL, 0, NULL, NULL};
            }
            lexer->current_pos++;
            return (Token) {TOKEN_OPERATOR_PLUS, NULL, 0, NULL, NULL};
        case '-':
            if (lexer->source[lexer->current_pos + 1] == '-') {
                lexer->current_pos += 2;
                return (Token) {TOKEN_OPERATOR_MINUS_MINUS, NULL, 0, NULL, NULL};
            }
            lexer->current_pos++;
            return (Token) {TOKEN_OPERATOR_MINUS, NULL, 0, NULL, NULL};
        case '*':
            if (lexer->source[lexer->current_pos + 1] == '*') {
                lexer->current_pos += 2;
                return (Token) {TOKEN_OPERATOR_POWER, NULL, 0, NULL, NULL};
            }
            lexer->current_pos++;
            return (Token) {TOKEN_OPERATOR_MULTIPLY, NULL, 0, NULL, NULL};
        case '/':
            lexer->current_pos++;
            return (Token) {TOKEN_OPERATOR_DIVIDE, NULL, 0, NULL, NULL};
        case '%':
            lexer->current_pos++;
            return (Token) {TOKEN_OPERATOR_MODULO, NULL, 0, NULL, NULL};
        case '&':
            if (lexer->source[lexer->current_pos + 1] == '&') {
                lexer->current_pos += 2;
                return (Token) {TOKEN_OPERATOR_LOGICAL_AND, NULL, 0, NULL, NULL};
            }
            lexer->current_pos++;
            return (Token) {TOKEN_OPERATOR_BITWISE_AND, NULL, 0, NULL, NULL};
        case '|':
            if (lexer->source[lexer->current_pos + 1] == '|') {
                lexer->current_pos += 2;
                return (Token) {TOKEN_OPERATOR_LOGICAL_OR, NULL, 0, NULL, NULL};
            }
            lexer->current_pos++;
            return (Token) {TOKEN_OPERATOR_BITWISE_OR, NULL, 0, NULL, NULL};
        case '^':
            if (lexer->source[lexer->current_pos + 1] == '^') {
                lexer->current_pos += 2;
                return (Token) {TOKEN_OPERATOR_LOGICAL_XOR, NULL, 0, NULL, NULL};
            }
            lexer->current_pos++;
            return (Token) {TOKEN_OPERATOR_BITWISE_XOR, NULL, 0, NULL, NULL};
        case '(':
            lexer->current_pos++;
            return (Token) {TOKEN_LPAREN, NULL, 0, NULL, NULL};
        case ')':
            lexer->current_pos++;
            return (Token) {TOKEN_RPAREN, NULL, 0, NULL, NULL};
        case '{':
            lexer->current_pos++;
            return (Token) {TOKEN_LBRACE, NULL, 0, NULL, NULL};
        case '}':
            lexer->current_pos++;
            return (Token) {TOKEN_RBRACE, NULL, 0, NULL, NULL};
        case '[':
            lexer->current_pos++;
            return (Token) {TOKEN_LBRACKET, NULL, 0, NULL, NULL};
        case ']':
            lexer->current_pos++;
            return (Token) {TOKEN_RBRACKET, NULL, 0, NULL, NULL};
        case ';':
            lexer->current_pos++;
            return (Token) {TOKEN_SEMICOLON, NULL, 0, NULL, NULL};
        case ',':
            lexer->current_pos++;
            return (Token) {TOKEN_COMMA, NULL, 0, NULL, NULL};
        case '.':
            lexer->current_pos++;
            return (Token) { TOKEN_DOT, NULL, 0, NULL, NULL};
        default:
            fprintf(stderr, "\nError: Unexpected character '%c' (ASCII %d)\n", current, current);
            exit(EXIT_FAILURE);
    }
}