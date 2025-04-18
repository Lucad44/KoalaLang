#ifndef AST_H
#define AST_H

#include "variables.h"

typedef enum {
    NODE_VAR_DECL,
    NODE_LIST_DECL,
    NODE_PRINT,
    NODE_FUNC_DECL,
    NODE_FUNC_CALL,
    NODE_RETURN,
    NODE_IF,
    NODE_ELIF,
    NODE_ELSE,
    NODE_WHILE,
    NODE_BLOCK,
    NODE_EXPR_LITERAL,
    NODE_LIST_LITERAL,
    NODE_EXPR_VARIABLE,
    NODE_EXPR_UNARY,
    NODE_EXPR_BINARY,
    NODE_EXPR_POSTFIX,
    NODE_LIST_ACCESS,
    NODE_ASSIGNMENT
} NodeType;

typedef enum {
    OP_PLUS,
    OP_MINUS,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MODULO,
    OP_POWER,
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_LESS,
    OP_GREATER,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_LESS_EQUAL,
    OP_GREATER_EQUAL
} BinaryOperator;

typedef enum {
    OP_INC,
    OP_DEC
} PostfixOperator;

typedef enum {
    OP_NEGATE,
    OP_NOT
} UnaryOperator;

typedef struct ASTNode ASTNode;

typedef struct {
    UnaryOperator op;
    ASTNode *operand;
} UnaryExprNode;

typedef struct {
    BinaryOperator op;
    ASTNode *left;
    ASTNode *right;
} BinaryExprNode;

typedef struct {
    char *name;
    ASTNode *init_expr;
    int type;
} VarDeclNode;

typedef struct {
    ASTNode **expr_list;
    int expr_count;
} PrintNode;

typedef struct {
    ASTNode **statements;
    int stmt_count;
} BlockNode;

typedef struct {
    double num_val;
} NumLiteralNode;

typedef struct {
    char *str_val;
} StrLiteralNode;

typedef struct {
    char *name;
} VariableNode;

typedef struct {
    PostfixOperator op;
    char *var_name;
} PostfixExprNode;

typedef struct {
    ASTNode *condition;
    ASTNode *body;
    ASTNode **elif_nodes;
    int elif_count;
    ASTNode *else_body;
} IfNode;

typedef struct {
    ASTNode *condition;
    ASTNode *body;
} WhileNode;

typedef struct {
    char *name;
    VarType type; // Represents element type for lists (VAR_NUM/VAR_STR), or VAR_NUM/VAR_STR for scalars
    bool is_list; // Flag to indicate if it's a list parameter
} Parameter;

typedef struct {
    char *name;
    int param_count;
    Parameter *parameters;
    ASTNode *body;
} FuncDeclNode;

typedef struct {
    char *name;
    ASTNode **arguments;
    int arg_count;
} FuncCallNode;

typedef struct {
    ASTNode *expr;
} ReturnNode;

typedef struct {
    VarType element_type;
    ASTNode **elements;
    int element_count;
} ListLiteralNode;

typedef struct {
    char *name;
    VarType element_type;
    ASTNode *init_expr;
} ListDeclNode;

typedef struct {
    char *list_name;
    ASTNode *index_expr;
} ListAccessNode;

typedef struct {
    char *target_name;
    ASTNode *index_expr;
    ASTNode *value_expr;
} AssignmentNode;

struct ASTNode {
    NodeType type;
    union {
        VarDeclNode var_decl;
        ListDeclNode list_decl;
        PrintNode print;
        BlockNode block;
        NumLiteralNode num_literal;
        StrLiteralNode str_literal;
        ListLiteralNode list_literal;
        VariableNode variable;
        IfNode if_stmt;
        WhileNode while_stmt;
        UnaryExprNode unary_expr;
        BinaryExprNode binary_expr;
        PostfixExprNode postfix_expr;
        FuncDeclNode func_decl;
        FuncCallNode func_call;
        ReturnNode return_stmt;
        ListAccessNode list_access;
        AssignmentNode assignment;
    } data;
};

#endif