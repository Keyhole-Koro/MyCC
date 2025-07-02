#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "AST.h"

ASTNode* parse_expr(Token **cur);

ASTNode* new_number(char *val) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_NUMBER;
    node->number.value = strdup(val);
    return node;
}
ASTNode* new_identifier(char *name) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_IDENTIFIER;
    node->identifier.name = strdup(name);
    return node;
}
ASTNode* new_binary(TokenKind op, ASTNode *left, ASTNode *right) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_BINARY;
    node->binary.op = op;
    node->binary.left = left;
    node->binary.right = right;
    return node;
}
ASTNode* new_unary(TokenKind op, ASTNode *operand) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_UNARY;
    node->unary.op = op;
    node->unary.operand = operand;
    return node;
}
ASTNode* new_assign(ASTNode *left, ASTNode *right) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_ASSIGN;
    node->assign.left = left;
    node->assign.right = right;
    return node;
}
ASTNode* new_expr_stmt(ASTNode *expr) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_EXPR_STMT;
    node->expr_stmt.expr = expr;
    return node;
}
ASTNode* new_if(ASTNode *cond, ASTNode *then_stmt, ASTNode *else_stmt) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_IF;
    node->if_stmt.cond = cond;
    node->if_stmt.then_stmt = then_stmt;
    node->if_stmt.else_stmt = else_stmt;
    return node;
}
ASTNode* new_return(ASTNode *expr) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_RETURN;
    node->ret.expr = expr;
    return node;
}
ASTNode* new_block(ASTNode **stmts, int count) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_BLOCK;
    node->block.stmts = stmts;
    node->block.count = count;
    return node;
}
int match(Token **cur, TokenKind kind) {
    if (*cur && (*cur)->kind == kind) {
        *cur = (*cur)->next;
        return 1;
    }
    return 0;
}
ASTNode* parse_primary(Token **cur) {
    if ((*cur)->kind == NUMBER) {
        ASTNode *node = new_number((*cur)->value);
        *cur = (*cur)->next;
        return node;
    }
    if ((*cur)->kind == IDENTIFIER) {
        ASTNode *node = new_identifier((*cur)->value);
        *cur = (*cur)->next;
        return node;
    }
    if ((*cur)->kind == L_PARENTHESES) {
        *cur = (*cur)->next;
        ASTNode *node = parse_expr(cur);
        match(cur, R_PARENTHESES);
        return node;
    }
    return NULL;
}
ASTNode* parse_unary(Token **cur) {
    if ((*cur)->kind == SUB) {
        TokenKind op = (*cur)->kind;
        *cur = (*cur)->next;
        return new_unary(op, parse_unary(cur));
    }
    return parse_primary(cur);
}
ASTNode* parse_mul(Token **cur) {
    ASTNode *node = parse_unary(cur);
    while ((*cur)->kind == MUL || (*cur)->kind == DIV) {
        TokenKind op = (*cur)->kind;
        *cur = (*cur)->next;
        ASTNode *rhs = parse_unary(cur);
        node = new_binary(op, node, rhs);
    }
    return node;
}
ASTNode* parse_add(Token **cur) {
    ASTNode *node = parse_mul(cur);
    while ((*cur)->kind == ADD || (*cur)->kind == SUB) {
        TokenKind op = (*cur)->kind;
        *cur = (*cur)->next;
        ASTNode *rhs = parse_mul(cur);
        node = new_binary(op, node, rhs);
    }
    return node;
}
ASTNode* parse_assign_expr(Token **cur) {
    ASTNode *lhs = parse_add(cur);
    if ((*cur)->kind == ASSIGN) {
        *cur = (*cur)->next;
        ASTNode *rhs = parse_assign_expr(cur);
        return new_assign(lhs, rhs);
    }
    return lhs;
}
ASTNode* parse_expr(Token **cur) {
    return parse_assign_expr(cur);
}
ASTNode* parse_stmt(Token **cur);
ASTNode* parse_if_stmt(Token **cur) {
    match(cur, IF);
    match(cur, L_PARENTHESES);
    ASTNode *cond = parse_expr(cur);
    match(cur, R_PARENTHESES);
    ASTNode *then_stmt = parse_stmt(cur);
    ASTNode *else_stmt = NULL;
    if ((*cur)->kind == ELSE) {
        match(cur, ELSE);
        else_stmt = parse_stmt(cur);
    }
    return new_if(cond, then_stmt, else_stmt);
}
ASTNode* parse_return_stmt(Token **cur) {
    match(cur, RETURN);
    ASTNode *expr = parse_expr(cur);
    match(cur, SEMICOLON);
    return new_return(expr);
}
ASTNode* parse_block(Token **cur) {
    match(cur, L_BRACE);
    ASTNode **stmts = NULL;
    int count = 0;
    while ((*cur)->kind != R_BRACE && (*cur)->kind != EOT) {
        stmts = realloc(stmts, sizeof(ASTNode*) * (count+1));
        stmts[count++] = parse_stmt(cur);
    }
    match(cur, R_BRACE);
    return new_block(stmts, count);
}
ASTNode* parse_expr_stmt(Token **cur) {
    ASTNode *expr = parse_expr(cur);
    match(cur, SEMICOLON);
    return new_expr_stmt(expr);
}
ASTNode* parse_stmt(Token **cur) {
    if ((*cur)->kind == IF) return parse_if_stmt(cur);
    if ((*cur)->kind == RETURN) return parse_return_stmt(cur);
    if ((*cur)->kind == L_BRACE) return parse_block(cur);
    return parse_expr_stmt(cur);
}
ASTNode* parse_program(Token **cur) {
    ASTNode **stmts = NULL;
    int count = 0;
    while ((*cur)->kind != EOT) {
        stmts = realloc(stmts, sizeof(ASTNode*) * (count+1));
        stmts[count++] = parse_stmt(cur);
    }
    return new_block(stmts, count);
}
void print_ast(ASTNode *node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; i++) printf("  ");
    switch (node->type) {
        case AST_NUMBER:
            printf("Number: %s\n", node->number.value);
            break;
        case AST_IDENTIFIER:
            printf("Identifier: %s\n", node->identifier.name);
            break;
        case AST_BINARY:
            printf("Binary: %d\n", node->binary.op);
            print_ast(node->binary.left, indent+1);
            print_ast(node->binary.right, indent+1);
            break;
        case AST_ASSIGN:
            printf("Assign\n");
            print_ast(node->assign.left, indent+1);
            print_ast(node->assign.right, indent+1);
            break;
        case AST_UNARY:
            printf("Unary: %d\n", node->unary.op);
            print_ast(node->unary.operand, indent+1);
            break;
        case AST_EXPR_STMT:
            printf("ExprStmt\n");
            print_ast(node->expr_stmt.expr, indent+1);
            break;
        case AST_IF:
            printf("If\n");
            print_ast(node->if_stmt.cond, indent+1);
            print_ast(node->if_stmt.then_stmt, indent+1);
            if (node->if_stmt.else_stmt) print_ast(node->if_stmt.else_stmt, indent+1);
            break;
        case AST_RETURN:
            printf("Return\n");
            print_ast(node->ret.expr, indent+1);
            break;
        case AST_BLOCK:
            printf("Block\n");
            for (int i = 0; i < node->block.count; i++)
                print_ast(node->block.stmts[i], indent+1);
            break;
    }
}
void free_ast(ASTNode *node) {
    if (!node) return;
    switch (node->type) {
        case AST_NUMBER: free(node->number.value); break;
        case AST_IDENTIFIER: free(node->identifier.name); break;
        case AST_BINARY:
            free_ast(node->binary.left);
            free_ast(node->binary.right);
            break;
        case AST_ASSIGN:
            free_ast(node->assign.left);
            free_ast(node->assign.right);
            break;
        case AST_UNARY:
            free_ast(node->unary.operand);
            break;
        case AST_EXPR_STMT:
            free_ast(node->expr_stmt.expr);
            break;
        case AST_IF:
            free_ast(node->if_stmt.cond);
            free_ast(node->if_stmt.then_stmt);
            if (node->if_stmt.else_stmt) free_ast(node->if_stmt.else_stmt);
            break;
        case AST_RETURN:
            free_ast(node->ret.expr);
            break;
        case AST_BLOCK:
            for (int i = 0; i < node->block.count; i++)
                free_ast(node->block.stmts[i]);
            free(node->block.stmts);
            break;
    }
    free(node);
}