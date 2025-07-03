#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef enum {
    AST_NUMBER,
    AST_IDENTIFIER,
    AST_BINARY,
    AST_ASSIGN,
    AST_UNARY,
    AST_EXPR_STMT,
    AST_IF,
    AST_RETURN,
    AST_BLOCK,
    AST_FUNDEF,
    AST_PARAM,
    AST_CALL
} ASTNodeType;

typedef struct ASTNode ASTNode;
struct ASTNode {
    ASTNodeType type;
    union {
        struct { char *value; } number;
        struct { char *name; } identifier;
        struct { TokenKind op; ASTNode *left, *right; } binary;
        struct { ASTNode *left, *right; } assign;
        struct { TokenKind op; ASTNode *operand; } unary;
        struct { ASTNode *expr; } expr_stmt;
        struct { ASTNode *cond, *then_stmt, *else_stmt; } if_stmt;
        struct { ASTNode *expr; } ret;
        struct { ASTNode **stmts; int count; } block;
        struct { 
            char *ret_type;
            char *name;
            ASTNode **params;
            int param_count;
            ASTNode *body;
        } fundef;
        struct {
            char *type;
            char *name;
        } param;
        struct {
            char *name;
            ASTNode **args;
            int arg_count;
        } call;
    };
};

typedef struct FunctionTable {
    ASTNode **funcs;
    int count;
} FunctionTable;

#endif
