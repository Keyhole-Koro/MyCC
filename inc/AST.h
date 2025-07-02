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
    AST_BLOCK
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
    };
};

#endif
