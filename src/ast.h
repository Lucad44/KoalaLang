#ifndef AST_H
#define AST_H

typedef enum {
    NODE_VAR_DECL,
    NODE_PRINT,
    NODE_FUNC_DECL,
    NODE_FUNC_CALL,
    NODE_IF,
    NODE_ELIF,
    NODE_ELSE,
    NODE_WHILE,
    NODE_BLOCK,
    NODE_EXPR_LITERAL,
    NODE_EXPR_VARIABLE,
    NODE_EXPR_BINARY,
    NODE_EXPR_POSTFIX
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

typedef struct ASTNode ASTNode;

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
    char *type;
} Parameter;

typedef struct {
    char *name;
    char *type;
    int param_count;
    Parameter *parameters;
    ASTNode *body;
} FuncDeclNode;

typedef struct {
    char *name;
    ASTNode **arguments;
    int arg_count;
} FuncCallNode;

struct ASTNode {
    NodeType type;
    union {
        VarDeclNode var_decl;
        PrintNode print;
        BlockNode block;
        NumLiteralNode num_literal;
        StrLiteralNode str_literal;
        VariableNode variable;
        IfNode if_stmt;
        WhileNode while_stmt;
        BinaryExprNode binary_expr;
        PostfixExprNode postfix_expr;
        FuncDeclNode func_decl;
        FuncCallNode func_call;
    } data;
};

#endif