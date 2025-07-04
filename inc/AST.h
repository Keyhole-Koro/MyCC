#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef enum {
    AST_NUMBER,
    AST_IDENTIFIER,
    AST_BINARY,
    AST_VAR_DECL,
    AST_ASSIGN,
    AST_UNARY,
    AST_EXPR_STMT,
    AST_IF,
    AST_RETURN,
    AST_BLOCK,
    AST_FUNDEF,
    AST_PARAM,
    AST_CALL,
    AST_WHILE,
    AST_FOR,
} ASTNodeType;

typedef struct ASTNode ASTNode;
struct ASTNode {
    ASTNodeType type;
    union {
        struct { char *value; } number;
        struct { char *name; } identifier;
        struct { TokenKind op; ASTNode *left, *right; } binary;
        struct { ASTNode *left, *right; } assign;
        struct { char *type; char *name; ASTNode *init; } var_decl;
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
        struct {
            ASTNode *cond;
            ASTNode *body;
        } while_stmt;
        
        struct {
            ASTNode *init;
            ASTNode *cond;
            ASTNode *inc;
            ASTNode *body;
        } for_stmt;
    };
};

typedef struct FunctionTable {
    ASTNode **funcs;
    int count;
} FunctionTable;

void print_ast(ASTNode *node, int indent);

#endif
