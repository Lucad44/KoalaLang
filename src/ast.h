#ifndef AST_H
#define AST_H

#include "variables.h"
#include "modules.h"

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
    NODE_NUM_LITERAL,
    NODE_STR_LITERAL,
    NODE_LIST_LITERAL,
    NODE_EXPR_VARIABLE,
    NODE_EXPR_UNARY,
    NODE_EXPR_BINARY,
    NODE_EXPR_POSTFIX,
    NODE_VARIABLE_ACCESS,
    NODE_ASSIGNMENT,
    NODE_IMPORT
} NodeType;

typedef enum {
    OP_PLUS,
    OP_MINUS,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MODULO,
    OP_POWER,
    OP_BITWISE_AND,
    OP_BITWISE_OR,
    OP_BITWISE_XOR,
    OP_LESS,
    OP_GREATER,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_LESS_EQUAL,
    OP_GREATER_EQUAL,
    OP_LOGICAL_AND,
    OP_LOGICAL_OR,
    OP_LOGICAL_XOR
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
    char *module_name;
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
    VarType type;
    VarType nested_element_type;
    bool is_list;
    bool is_nested;
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
    char *module_name;
} FuncCallNode;

typedef struct {
    ASTNode *expr;
} ReturnNode;

typedef struct {
    VarType element_type;
    VarType nested_element_type; // Type of elements in nested lists
    bool is_nested;              // Whether this is a list of lists
    ASTNode **elements;
    int element_count;
} ListLiteralNode;

typedef struct {
    char *name;
    VarType element_type;
    VarType nested_element_type; // Type of elements in nested lists
    bool is_nested_list;         // Whether this is a list of lists
    ASTNode *init_expr;
} ListDeclNode;


typedef struct VariableAccessNode {
    char *name;          // Variable name (NULL for nested access)
    ASTNode *index_expr; // Index expression
    ASTNode *parent_expr; // Parent expression for nested access
} VariableAccessNode;

typedef struct {
    char *target_name;       // NULL for nested assignment
    ASTNode *index_expr;     // First index (only used for simple assignments)
    ASTNode *target_access;  // Used for nested access assignments
    ASTNode *value_expr;     // The value to assign
} AssignmentNode;

typedef struct {
    Module *module;
} ImportNode;

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
        VariableAccessNode variable_access;
        AssignmentNode assignment;
        ImportNode import;
    } data;
};

#endif