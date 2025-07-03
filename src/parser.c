#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "AST.h"

Token *token_head = NULL;

FunctionTable g_func_table = { NULL, 0 };

void add_function(ASTNode *fn) {
    g_func_table.funcs = realloc(g_func_table.funcs, sizeof(ASTNode*) * (g_func_table.count + 1));
    g_func_table.funcs[g_func_table.count++] = fn;
}

ASTNode* find_function(const char *name) {
    for (int i = 0; i < g_func_table.count; i++) {
        if (strcmp(g_func_table.funcs[i]->fundef.name, name) == 0) {
            return g_func_table.funcs[i];
        }
    }
    return NULL;
}

ASTNode* new_param(char *type, char *name) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_PARAM;
    node->param.type = strdup(type);
    node->param.name = strdup(name);
    return node;
}
ASTNode* new_fundef(char *ret_type, char *name, ASTNode **params, int param_count, ASTNode *body) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_FUNDEF;
    node->fundef.ret_type = strdup(ret_type);
    node->fundef.name = strdup(name);
    node->fundef.params = params;
    node->fundef.param_count = param_count;
    node->fundef.body = body;
    return node;
}
ASTNode *new_number(char *val) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_NUMBER;
    node->number.value = strdup(val);
    return node;
}
ASTNode *new_identifier(char *name) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_IDENTIFIER;
    node->identifier.name = strdup(name);
    return node;
}
ASTNode *new_binary(TokenKind op, ASTNode *left, ASTNode *right) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_BINARY;
    node->binary.op = op;
    node->binary.left = left;
    node->binary.right = right;
    return node;
}
ASTNode *new_unary(TokenKind op, ASTNode *operand) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_UNARY;
    node->unary.op = op;
    node->unary.operand = operand;
    return node;
}
ASTNode *new_assign(ASTNode *left, ASTNode *right) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_ASSIGN;
    node->assign.left = left;
    node->assign.right = right;
    return node;
}
ASTNode *new_expr_stmt(ASTNode *expr) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_EXPR_STMT;
    node->expr_stmt.expr = expr;
    return node;
}
ASTNode *new_if(ASTNode *cond, ASTNode *then_stmt, ASTNode *else_stmt) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_IF;
    node->if_stmt.cond = cond;
    node->if_stmt.then_stmt = then_stmt;
    node->if_stmt.else_stmt = else_stmt;
    return node;
}
ASTNode *new_return(ASTNode *expr) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_RETURN;
    node->ret.expr = expr;
    return node;
}
ASTNode *new_block(ASTNode **stmts, int count) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_BLOCK;
    node->block.stmts = stmts;
    node->block.count = count;
    return node;
}
ASTNode *new_call(char *name, ASTNode **args, int arg_count) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_CALL;
    node->call.name = strdup(name);
    node->call.args = args;
    node->call.arg_count = arg_count;
    return node;
}

int match(Token **cur, TokenKind kind) {
    if (*cur && (*cur)->kind == kind) {
        *cur = (*cur)->next;
        return 1;
    }
    return 0;
}
int is_type(TokenKind kind) {
    return kind == INT || kind == VOID;
}

void parse_error(const char *msg, Token *head, Token *cur) {
    // prevs[2] ... prevs[0] ... cur ... nexts[0] ... nexts[2]
    Token *prevs[3] = {NULL, NULL, NULL};
    Token *nexts[3] = {NULL, NULL, NULL};

    Token *p = head, *prev = NULL;
    while (p && p != cur) {
        prev = p;
        p = p->next;
    }
    for (int i = 2; i >= 0; i--) {
        prevs[i] = prev;
        if (prev) prev = head;
        Token *tmp = head;
        while (tmp && tmp != prevs[i]) {
            prev = tmp;
            tmp = tmp->next;
        }
    }

    Token *q = cur;
    for (int i = 0; i < 3 && q; i++) {
        q = q->next;
        nexts[i] = q;
    }

    fprintf(stderr, "Parse error: %s\n", msg);

    for (int i = 2; i >= 0; i--) {
        if (prevs[i])
            fprintf(stderr, "  [prev-%d] kind: %s, value: %s\n",
                3-i, tokenkind2str(prevs[i]->kind),
                prevs[i]->value ? prevs[i]->value : "(null)");
    }
    fprintf(stderr, "  [here] kind: %s, value: %s\n",
            cur ? tokenkind2str(cur->kind) : "(null)",
            cur && cur->value ? cur->value : "(null)");
    for (int i = 0; i < 3; i++) {
        if (nexts[i])
            fprintf(stderr, "  [next+%d] kind: %s, value: %s\n",
                i+1, tokenkind2str(nexts[i]->kind),
                nexts[i]->value ? nexts[i]->value : "(null)");
    }
    exit(1);
}

ASTNode* parse_expr(Token **cur);

ASTNode *parse_call(Token **cur, char *name) {
    if (!match(cur, L_PARENTHESES))
        parse_error("expected '(' after function name", NULL, *cur);

    ASTNode **args = NULL;
    int arg_count = 0;
    if ((*cur)->kind != R_PARENTHESES) {
        while (1) {
            ASTNode *arg = parse_expr(cur);
            args = realloc(args, sizeof(ASTNode*) * (arg_count+1));
            args[arg_count++] = arg;
            if ((*cur)->kind == COMMA) {
                *cur = (*cur)->next;
                continue;
            }
            break;
        }
    }
    if (!match(cur, R_PARENTHESES))
        parse_error("expected ')' after arguments", NULL, *cur);
    return new_call(name, args, arg_count);
}

ASTNode *parse_primary(Token **cur) {
    if ((*cur)->kind == NUMBER) {
        ASTNode *node = new_number((*cur)->value);
        *cur = (*cur)->next;
        return node;
    }
    if ((*cur)->kind == IDENTIFIER) {
        char *name = (*cur)->value;
        *cur = (*cur)->next;
        if ((*cur)->kind == L_PARENTHESES) {
            return parse_call(cur, name);
        }
        return new_identifier(name);
    }
    if ((*cur)->kind == L_PARENTHESES) {
        *cur = (*cur)->next;
        ASTNode *node = parse_expr(cur);
        if (!match(cur, R_PARENTHESES)) parse_error("expected ')'", NULL, *cur);
        return node;
    }
    parse_error("expected primary expression", NULL, *cur);
    return NULL;
}

ASTNode *parse_unary(Token **cur) {
    if ((*cur)->kind == SUB) {
        TokenKind op = (*cur)->kind;
        *cur = (*cur)->next;
        return new_unary(op, parse_unary(cur));
    }
    return parse_primary(cur);
}
ASTNode *parse_mul(Token **cur) {
    ASTNode *node = parse_unary(cur);
    while ((*cur)->kind == MUL || (*cur)->kind == DIV) {
        TokenKind op = (*cur)->kind;
        *cur = (*cur)->next;
        ASTNode *rhs = parse_unary(cur);
        node = new_binary(op, node, rhs);
    }
    return node;
}
ASTNode *parse_add(Token **cur) {
    ASTNode *node = parse_mul(cur);
    while ((*cur)->kind == ADD || (*cur)->kind == SUB) {
        TokenKind op = (*cur)->kind;
        *cur = (*cur)->next;
        ASTNode *rhs = parse_mul(cur);
        node = new_binary(op, node, rhs);
    }
    return node;
}
ASTNode *parse_assign_expr(Token **cur) {
    ASTNode *lhs = parse_add(cur);
    if ((*cur)->kind == ASSIGN) {
        *cur = (*cur)->next;
        ASTNode *rhs = parse_assign_expr(cur);
        return new_assign(lhs, rhs);
    }
    return lhs;
}
ASTNode *parse_expr(Token **cur) {
    return parse_assign_expr(cur);
}

ASTNode* parse_param(Token **cur) {
    if (!is_type((*cur)->kind)) parse_error("expected type in parameter", NULL, *cur);
    char *type = (*cur)->value;
    *cur = (*cur)->next;
    if ((*cur)->kind != IDENTIFIER) parse_error("expected identifier in parameter", NULL, *cur);
    char *name = (*cur)->value;
    *cur = (*cur)->next;
    return new_param(type, name);
}

ASTNode** parse_param_list(Token **cur, int *out_count) {
    ASTNode **params = NULL;
    int count = 0;
    if ((*cur)->kind == R_PARENTHESES) {
        *out_count = 0;
        return NULL;
    }
    while (1) {
        ASTNode *param = parse_param(cur);
        params = realloc(params, sizeof(ASTNode*) * (count+1));
        params[count++] = param;
        if ((*cur)->kind == COMMA) {
            *cur = (*cur)->next;
            continue;
        }
        break;
    }
    *out_count = count;
    return params;
}

ASTNode* parse_fundef(Token **cur) {
    if (!is_type((*cur)->kind)) parse_error("expected return type", NULL, *cur);
    char *ret_type = (*cur)->value;
    *cur = (*cur)->next;
    if ((*cur)->kind != IDENTIFIER) parse_error("expected function name", NULL, *cur);
    char *name = (*cur)->value;
    *cur = (*cur)->next;
    if (!match(cur, L_PARENTHESES)) parse_error("expected '(' after function name", NULL, *cur);

    int param_count = 0;
    ASTNode **params = NULL;
    if ((*cur)->kind != R_PARENTHESES)
        params = parse_param_list(cur, &param_count);

    if (!match(cur, R_PARENTHESES)) parse_error("expected ')' after parameter list", NULL, *cur);
    ASTNode *body = new_block(NULL, 0);
    ASTNode *fndef = new_fundef(ret_type, name, params, param_count, body);
    add_function(fndef);
    return fndef;
}

ASTNode *parse_stmt(Token **cur);
ASTNode *parse_if_stmt(Token **cur) {
    if (!match(cur, IF)) parse_error("expected 'if'", token_head, *cur);
    if (!match(cur, L_PARENTHESES)) parse_error("expected '(' after if", token_head, *cur);
    ASTNode *cond = parse_expr(cur);
    if (!match(cur, R_PARENTHESES)) parse_error("expected ')'", token_head, *cur);
    ASTNode *then_stmt = parse_stmt(cur);
    ASTNode *else_stmt = NULL;
    if ((*cur)->kind == ELSE) {
        match(cur, ELSE);
        else_stmt = parse_stmt(cur);
    }
    return new_if(cond, then_stmt, else_stmt);
}

ASTNode *parse_return_stmt(Token **cur) {
    if (!match(cur, RETURN)) parse_error("expected 'return'", token_head, *cur);
    ASTNode *expr = parse_expr(cur);
    if (!match(cur, SEMICOLON)) parse_error("expected ';' after return", token_head, *cur);
    return new_return(expr);
}

ASTNode *parse_block(Token **cur) {
    if (!match(cur, L_BRACE)) parse_error("expected '{'", token_head, *cur);
    ASTNode **stmts = NULL;
    int count = 0;
    while ((*cur)->kind != R_BRACE && (*cur)->kind != EOT) {
        stmts = realloc(stmts, sizeof(ASTNode*) * (count+1));
        stmts[count++] = parse_stmt(cur);
    }
    if (!match(cur, R_BRACE)) parse_error("expected '}'", token_head, *cur);
    return new_block(stmts, count);
}

ASTNode *parse_expr_stmt(Token **cur) {
    ASTNode *expr = parse_expr(cur);
    if (!match(cur, SEMICOLON)) parse_error("expected ';' after expression", token_head, *cur);
    return new_expr_stmt(expr);
}

ASTNode *parse_variable_declaration(Token **cur) {
    if (!is_type((*cur)->kind)) parse_error("expected type for variable declaration", token_head, *cur);
    char *type = (*cur)->value;
    *cur = (*cur)->next;
    if ((*cur)->kind != IDENTIFIER) parse_error("expected identifier for variable name", token_head, *cur);
    char *name = (*cur)->value;
    *cur = (*cur)->next;
    ASTNode *init_expr = NULL;
    if (match(cur, ASSIGN)) {
        init_expr = parse_expr(cur);
        if (!match(cur, SEMICOLON)) parse_error("expected ';' after variable declaration", token_head, *cur);
    } else {
        if (!match(cur, SEMICOLON)) parse_error("expected ';' after variable declaration", token_head, *cur);
    }
    return new_assign(new_identifier(name), init_expr ? init_expr : new_number("0"));
}

ASTNode *parse_variable_assignment(Token **cur) {
    if ((*cur)->kind != IDENTIFIER) parse_error("expected identifier for variable assignment", token_head, *cur);
    char *name = (*cur)->value;
    *cur = (*cur)->next;
    if (!match(cur, ASSIGN)) parse_error("expected '=' for variable assignment", token_head, *cur);
    ASTNode *expr = parse_expr(cur);
    if (!match(cur, SEMICOLON)) parse_error("expected ';' after variable assignment", token_head, *cur);
    return new_assign(new_identifier(name), expr);
}

ASTNode *parse_stmt(Token **cur) {
    if ((*cur)->kind == IF) return parse_if_stmt(cur);
    if ((*cur)->kind == RETURN) return parse_return_stmt(cur);
    if ((*cur)->kind == L_BRACE) return parse_block(cur);
    if (is_type((*cur)->kind)) return parse_variable_declaration(cur);
    return parse_variable_assignment(cur);
}

ASTNode* parse_toplevel(Token **cur) {
    if (is_type((*cur)->kind)) {
        Token *save = *cur;
        ASTNode *fn = parse_fundef(cur);
        if (fn) return fn;
        *cur = save;
    }
    ASTNode *stmt = parse_stmt(cur);
    if (!stmt) parse_error("unexpected toplevel construct", token_head, *cur);
    return stmt;
}

ASTNode* parse_program(Token **cur) {
    ASTNode **nodes = NULL;
    int count = 0;
    while ((*cur)->kind != EOT) {
        ASTNode *node = parse_toplevel(cur);
        if (!node) parse_error("failed to parse toplevel", token_head, *cur);
        nodes = realloc(nodes, sizeof(ASTNode*) * (count+1));
        nodes[count++] = node;
    }
    return new_block(nodes, count);
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
        case AST_FUNDEF:
            printf("Function: %s %s\n", node->fundef.ret_type, node->fundef.name);
            for (int i = 0; i < node->fundef.param_count; i++) {
                for (int j = 0; j < indent+1; j++) printf("  ");
                printf("Param: %s %s\n", node->fundef.params[i]->param.type, node->fundef.params[i]->param.name);
            }
            print_ast(node->fundef.body, indent+1);
            break;
        case AST_PARAM:
            printf("Param: %s %s\n", node->param.type, node->param.name);
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
        case AST_FUNDEF:
            free(node->fundef.ret_type);
            free(node->fundef.name);
            for (int i = 0; i < node->fundef.param_count; i++)
                free_ast(node->fundef.params[i]);
            free(node->fundef.params);
            free_ast(node->fundef.body);
            break;
        case AST_PARAM:
            free(node->param.type);
            free(node->param.name);
            break;
    }
    free(node);
}