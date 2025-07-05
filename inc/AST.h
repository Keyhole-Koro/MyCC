#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef enum {
    AST_NUMBER,
    AST_IDENTIFIER,
    AST_BINARY,
    AST_TYPE,
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
    AST_TYPEDEF,
    AST_STRUCT,
    AST_STRUCT_MEMBER,
    AST_TYPEDEF_STRUCT,
    AST_STRING_LITERAL,
    AST_CHAR_LITERAL,
} ASTNodeType;

typedef enum {
    TYPEMOD_NONE    = 0,
    TYPEMOD_CONST   = 1 << 0,
    TYPEMOD_UNSIGNED= 1 << 1,
    TYPEMOD_SIGNED  = 1 << 2,
} TypeModifier;

typedef struct ASTNode ASTNode;
struct ASTNode {
    ASTNodeType type;
    union {
        struct { char *value; } number;
        struct { char *name; } identifier;
        struct { TokenKind op; ASTNode *left, *right; } binary;
        struct { ASTNode *left, *right; } assign;
        struct {
            ASTNode *base_type;
            int pointer_level; // number of pointers
            int type_modifiers; // bitmask of TypeModifier
        } type_node;
        struct {
            ASTNode *var_type;
            char *name;
            ASTNode *init;
        } var_decl;
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
        struct {
            char *orig_type;
            char *new_type;
        } typedef_stmt;
        
        struct {
            char *name;
            ASTNode **members;
            int member_count;
        } struct_stmt;
        
        struct {
            char *type;
            char *name;
        } struct_member;

        struct {
            char *struct_name;
            ASTNode **members;
            int member_count;
            char *typedef_name;
        } typedef_struct;
        struct {
            char value;
        } char_literal;
        
        struct { char *value; } string_literal;
    };
};

void print_ast(ASTNode *node, int indent);

#endif
