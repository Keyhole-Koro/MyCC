#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "AST.h"

typedef struct FunctionTable {
    ASTNode **funcs;
    int count;
} FunctionTable;

#define MAX_TYPE_NAME 128
typedef struct TypeTable {
    char **typenames;
    int count;
} TypeTable;

typedef struct StructDef {
    char *name;
    ASTNode **members;
    int member_count;
} StructDef;

typedef struct StructTable {
    StructDef **structs;
    int count;
} StructTable;

Token *token_head = NULL;
ASTNode *root;
StructTable g_struct_table = { NULL, 0 };

FunctionTable g_func_table = { NULL, 0 };
TypeTable g_type_table = { NULL, 0 };

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

void add_typename(const char *name) {
    g_type_table.typenames = realloc(g_type_table.typenames, sizeof(char*) * (g_type_table.count + 1));
    g_type_table.typenames[g_type_table.count++] = strdup(name);
}

int is_user_typename(const char *name) {
    for (int i = 0; i < g_type_table.count; i++) {
        if (strcmp(g_type_table.typenames[i], name) == 0) return 1;
    }
    return 0;
}

void add_structdef(char *name, ASTNode **members, int member_count) {
    StructDef *def = malloc(sizeof(StructDef));
    def->name = strdup(name);
    def->members = members;
    def->member_count = member_count;
    g_struct_table.structs = realloc(g_struct_table.structs, sizeof(StructDef*) * (g_struct_table.count + 1));
    g_struct_table.structs[g_struct_table.count++] = def;
}

StructDef *find_structdef(const char *name) {
    for (int i = 0; i < g_struct_table.count; i++) {
        if (strcmp(g_struct_table.structs[i]->name, name) == 0) return g_struct_table.structs[i];
    }
    return NULL;
}

ASTNode *new_string_literal(char *val);
ASTNode *new_var_decl(ASTNode *type, char *name, ASTNode *init);

ASTNode *new_char_literal(char value) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_CHAR_LITERAL;
    node->char_literal.value = value;
    return node;
}

ASTNode *new_type_array(ASTNode *elem_type, int size) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_TYPE_ARRAY;
    node->type_array.element_type = elem_type;
    node->type_array.array_size = size;
    return node;
}

ASTNode *new_var_decl(ASTNode *type, char *name, ASTNode *init) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_VAR_DECL;
    node->var_decl.var_type = type;
    node->var_decl.name = strdup(name);
    node->var_decl.init = init;
    return node;
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
ASTNode *new_type_node(ASTNode *base_type, int pointer_level, int modifiers) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_TYPE;
    printf("base type %d\n", base_type->type);
    node->type_node.base_type = base_type;
    node->type_node.pointer_level = pointer_level;
    node->type_node.type_modifiers = modifiers;
    return node;
}

ASTNode *new_expr_stmt(ASTNode *expr) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_EXPR_STMT;
    node->expr_stmt.expr = expr;
    return node;
}

ASTNode *new_typedef(char *orig_type, char *new_type) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_TYPEDEF;
    node->typedef_stmt.orig_type = strdup(orig_type);
    node->typedef_stmt.new_type = strdup(new_type);
    return node;
}

ASTNode *new_typedef_struct(char *struct_name, ASTNode **members, int member_count, char *typedef_name) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_TYPEDEF_STRUCT;
    node->typedef_struct.struct_name = strdup(struct_name);
    node->typedef_struct.members = members;
    node->typedef_struct.member_count = member_count;
    node->typedef_struct.typedef_name = strdup(typedef_name);
    return node;
}

ASTNode *new_struct(char *name, ASTNode **members, int member_count) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_STRUCT;
    node->struct_stmt.name = strdup(name);
    node->struct_stmt.members = members;
    node->struct_stmt.member_count = member_count;
    return node;
}

ASTNode *new_struct_member(char *type, char *name) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_STRUCT_MEMBER;
    node->struct_member.type = strdup(type);
    node->struct_member.name = strdup(name);
    return node;
}

ASTNode *new_while(ASTNode *cond, ASTNode *body) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_WHILE;
    node->while_stmt.cond = cond;
    node->while_stmt.body = body;
    return node;
}
ASTNode *new_for(ASTNode *init, ASTNode *cond, ASTNode *inc, ASTNode *body) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_FOR;
    node->for_stmt.init = init;
    node->for_stmt.cond = cond;
    node->for_stmt.inc = inc;
    node->for_stmt.body = body;
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

void parse_error(const char *msg, Token *head, Token *cur) {
    print_ast(root, 0);
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
int expect(Token **cur, TokenKind kind) {
    if (*cur && (*cur)->kind == kind) {
        *cur = (*cur)->next;
        return 1;
    }
    return 0;
}
int is_type(TokenKind kind, Token *cur) {
    if (kind == CONST || kind == UNSIGNED || kind == SIGNED) return 1;

    if (kind == VOID ||
        kind == INT ||
        kind == CHAR ||
        kind == FLOAT ||
        kind == DOUBLE ||
        kind == BOOL
    ) return 1;
    if (kind == IDENTIFIER && is_user_typename(cur->value)) return 1;
    return 0;
}

ASTNode *parse_expr(Token **cur);
ASTNode *parse_variable_declaration(Token **cur, int need_semicolon);
ASTNode *parse_struct(Token **cur);

ASTNode *parse_primary(Token **cur) {

    if ((*cur)->kind == NUMBER) {
        ASTNode *node = new_number((*cur)->value);
        *cur = (*cur)->next;
        return node;
    }
    if ((*cur)->kind == STRING_LITERAL) {
        ASTNode *node = new_string_literal((*cur)->value);
        *cur = (*cur)->next;
        return node;
    }
    if ((*cur)->kind == CHAR_LITERAL) {
        ASTNode *node = new_char_literal((*cur)->value[0]);
        *cur = (*cur)->next;
        return node;
    }

    if ((*cur)->kind == IDENTIFIER) {
        char *name = (*cur)->value;
        *cur = (*cur)->next;
        if ((*cur)->kind == L_PARENTHESES) {
            *cur = (*cur)->next;
            ASTNode **args = NULL;
            int arg_count = 0;
            if ((*cur)->kind != R_PARENTHESES) {
                while (1) {
                    ASTNode *arg = parse_expr(cur);
                    args = realloc(args, sizeof(ASTNode*) * (arg_count + 1));
                    args[arg_count++] = arg;
                    if ((*cur)->kind == COMMA) { *cur = (*cur)->next; continue; }
                    break;
                }
            }
            if (!expect(cur, R_PARENTHESES)) parse_error("expected ')' after args", token_head, *cur);
            return new_call(name, args, arg_count);
        }
        return new_identifier(name);
    }
    if ((*cur)->kind == L_PARENTHESES) {
        *cur = (*cur)->next;
        ASTNode *node = parse_expr(cur);
        if (!expect(cur, R_PARENTHESES)) parse_error("expected ')'", token_head, *cur);
        return node;
    }
    parse_error("expected primary", token_head, *cur);

    return NULL;
}
ASTNode *parse_base_type(Token **cur) {
    if (!is_type((*cur)->kind, *cur))
        parse_error("expected type", token_head, *cur);
    ASTNode *base = new_identifier((*cur)->value);
    *cur = (*cur)->next;
    return base;
}

ASTNode *parse_type(Token **cur) {
    int modifiers = 0;

    while ((*cur)->kind == CONST || (*cur)->kind == UNSIGNED || (*cur)->kind == SIGNED) {
        if ((*cur)->kind == CONST)    modifiers |= TYPEMOD_CONST;
        if ((*cur)->kind == UNSIGNED) modifiers |= TYPEMOD_UNSIGNED;
        if ((*cur)->kind == SIGNED)   modifiers |= TYPEMOD_SIGNED;
        *cur = (*cur)->next;
    }

    if (!is_type((*cur)->kind, *cur))
        parse_error("expected base type", token_head, *cur);

    ASTNode *base_type = parse_base_type(cur);

    int pointer_level = 0;
    while ((*cur)->kind == ASTARISK) {
        pointer_level++;
        *cur = (*cur)->next;
    }
   return new_type_node(base_type, pointer_level, modifiers);

}

ASTNode *new_string_literal(char *val) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_STRING_LITERAL;
    node->string_literal.value = strdup(val);
    return node;
}
ASTNode *parse_postfix(Token **cur) {
    ASTNode *node = parse_primary(cur);
    while ((*cur)->kind == INC || (*cur)->kind == DEC) {
        TokenKind op = (*cur)->kind;
        *cur = (*cur)->next;
        node = new_unary(op == INC ? POST_INC : POST_DEC, node);
    }
    return node;
}

ASTNode *parse_unary(Token **cur) {
    if ((*cur)->kind == SUB) {
        *cur = (*cur)->next;
        return new_unary(SUB, parse_unary(cur));
    }
    if ((*cur)->kind == AMPERSAND) {
        *cur = (*cur)->next;
        return new_unary(AMPERSAND, parse_unary(cur));
    }
    if ((*cur)->kind == ASTARISK) {
        *cur = (*cur)->next;
        return new_unary(ASTARISK, parse_unary(cur));
    }
    if ((*cur)->kind == INC) {
        *cur = (*cur)->next;
        return new_unary(INC, parse_unary(cur));
    }
    if ((*cur)->kind == DEC) {
        *cur = (*cur)->next;
        return new_unary(DEC, parse_unary(cur));
    }
    return parse_postfix(cur);
}
ASTNode *parse_mul(Token **cur) {
    ASTNode *node = parse_unary(cur);
    while ((*cur)->kind == ASTARISK || (*cur)->kind == DIV) {
        TokenKind op = (*cur)->kind;
        *cur = (*cur)->next;
        node = new_binary(op, node, parse_unary(cur));
    }
    return node;
}
ASTNode *parse_add(Token **cur) {
    ASTNode *node = parse_mul(cur);
    while ((*cur)->kind == ADD || (*cur)->kind == SUB) {
        TokenKind op = (*cur)->kind;
        *cur = (*cur)->next;
        node = new_binary(op, node, parse_mul(cur));
    }
    return node;
}
ASTNode *parse_relational(Token **cur) {
    ASTNode *node = parse_add(cur);
    while (1) {
        if ((*cur)->kind == LT) {
            *cur = (*cur)->next;
            node = new_binary(LT, node, parse_add(cur));
        } else if ((*cur)->kind == GT) {
            *cur = (*cur)->next;
            node = new_binary(GT, node, parse_add(cur));
        } else if ((*cur)->kind == LTE) {
            *cur = (*cur)->next;
            node = new_binary(LTE, node, parse_add(cur));
        } else if ((*cur)->kind == GTE) {
            *cur = (*cur)->next;
            node = new_binary(GTE, node, parse_add(cur));
        } else break;
    }
    return node;
}
ASTNode *parse_equality(Token **cur) {
    ASTNode *node = parse_relational(cur);
    while (1) {
        if ((*cur)->kind == EQ) {
            *cur = (*cur)->next;
            node = new_binary(EQ, node, parse_relational(cur));
        } else if ((*cur)->kind == NEQ) {
            *cur = (*cur)->next;
            node = new_binary(NEQ, node, parse_relational(cur));
        } else break;
    }
    return node;
}
ASTNode *parse_assign_expr(Token **cur) {
    ASTNode *node = parse_equality(cur);
    if ((*cur)->kind == ASSIGN) {
        *cur = (*cur)->next;
        node = new_assign(node, parse_assign_expr(cur));
    }
    return node;
}
ASTNode *parse_expr(Token **cur) {
    return parse_assign_expr(cur);
}

ASTNode* parse_param(Token **cur) {
    char *type = parse_type(cur);
    if ((*cur)->kind != IDENTIFIER) parse_error("expected param name", token_head, *cur);
    char *name = (*cur)->value;
    *cur = (*cur)->next;
    return new_param(type, name);
}

ASTNode** parse_param_list(Token **cur, int *out_count) {
    ASTNode **params = NULL;
    int count = 0;
    if ((*cur)->kind == R_PARENTHESES) { *out_count = 0; return NULL; }
    while (1) {
        ASTNode *param = parse_param(cur);
        params = realloc(params, sizeof(ASTNode*) * (count+1));
        params[count++] = param;
        if ((*cur)->kind == COMMA) { *cur = (*cur)->next; continue; }
        break;
    }
    *out_count = count;
    return params;
}

ASTNode *parse_stmt(Token **cur);

ASTNode *parse_block(Token **cur) {
    if (!expect(cur, L_BRACE)) parse_error("expected '{'", token_head, *cur);
    ASTNode **stmts = NULL;
    int count = 0;
    while ((*cur)->kind != R_BRACE && (*cur)->kind != EOT) {
        stmts = realloc(stmts, sizeof(ASTNode*) * (count+1));
        stmts[count++] = parse_stmt(cur);
    }
    if (!expect(cur, R_BRACE)) parse_error("expected '}'", token_head, *cur);
    root = new_block(stmts, count);
    return root;
}

ASTNode *parse_while_stmt(Token **cur) {
    if (!expect(cur, WHILE)) parse_error("expected 'while'", token_head, *cur);
    if (!expect(cur, L_PARENTHESES)) parse_error("expected '(' after while", token_head, *cur);
    ASTNode *cond = parse_expr(cur);
    if (!expect(cur, R_PARENTHESES)) parse_error("expected ')'", token_head, *cur);
    ASTNode *body = parse_stmt(cur);
    return new_while(cond, body);
}

ASTNode *parse_for_stmt(Token **cur) {
    if (!expect(cur, FOR)) parse_error("expected 'for'", token_head, *cur);
    if (!expect(cur, L_PARENTHESES)) parse_error("expected '(' after for", token_head, *cur);

    // for (init; cond; inc)
    ASTNode *init = NULL, *cond = NULL, *inc = NULL;

    if ((*cur)->kind != SEMICOLON) {
        if (is_type((*cur)->kind, *cur)) {
            init = parse_variable_declaration(cur, 0);
        } else {
            init = parse_expr(cur);
        }
    }
    if (!expect(cur, SEMICOLON)) parse_error("expected ';' after for-init", token_head, *cur);

    if ((*cur)->kind != SEMICOLON) {
        cond = parse_expr(cur);
    }
    if (!expect(cur, SEMICOLON)) parse_error("expected second ';' in for", token_head, *cur);

    if ((*cur)->kind != R_PARENTHESES) {
        inc = parse_expr(cur);
    }
    if (!expect(cur, R_PARENTHESES)) parse_error("expected ')' after for", token_head, *cur);

    ASTNode *body = parse_stmt(cur);
    return new_for(init, cond, inc, body);
}

ASTNode *parse_if_stmt(Token **cur) {
    if (!expect(cur, IF)) parse_error("expected 'if'", token_head, *cur);
    if (!expect(cur, L_PARENTHESES)) parse_error("expected '(' after if", token_head, *cur);
    ASTNode *cond = parse_expr(cur);
    if (!expect(cur, R_PARENTHESES)) parse_error("expected ')'", token_head, *cur);
    ASTNode *then_stmt = parse_stmt(cur);
    ASTNode *else_stmt = NULL;
    if ((*cur)->kind == ELSE) {
        expect(cur, ELSE);
        else_stmt = parse_stmt(cur);
    }
    return new_if(cond, then_stmt, else_stmt);
}
ASTNode *parse_return_stmt(Token **cur) {
    if (!expect(cur, RETURN)) parse_error("expected 'return'", token_head, *cur);
    ASTNode *expr = parse_expr(cur);
    if (!expect(cur, SEMICOLON)) parse_error("expected ';' after return", token_head, *cur);
    return new_return(expr);
}
ASTNode *parse_expr_stmt(Token **cur) {
    ASTNode *expr = parse_expr(cur);
    if (!expect(cur, SEMICOLON)) parse_error("expected ';' after expression", token_head, *cur);
    return new_expr_stmt(expr);
}
ASTNode *parse_variable_declaration(Token **cur, int need_semicolon) {
    ASTNode *type = parse_type(cur);
    if ((*cur)->kind != IDENTIFIER)
        parse_error("expected identifier for variable name", token_head, *cur);
    char *name = (*cur)->value;
    *cur = (*cur)->next;

    ASTNode *final_type = type;
    while ((*cur)->kind == L_BRACKET) {
        *cur = (*cur)->next;
        int size = -1;
        if ((*cur)->kind == NUMBER) {
            size = atoi((*cur)->value);
            *cur = (*cur)->next;
        }
        if (!expect(cur, R_BRACKET)) parse_error("expected ']' for array", token_head, *cur);
        final_type = new_type_array(final_type, size);
    }

    ASTNode *init = NULL;
    if (expect(cur, ASSIGN)) {
        init = parse_expr(cur);
    }
    if (need_semicolon) {
        if (!expect(cur, SEMICOLON))
            parse_error("expected ';' after variable declaration", token_head, *cur);
    }
    return new_var_decl(final_type, name, init);
}


ASTNode *parse_variable_assignment(Token **cur) {
    if ((*cur)->kind != IDENTIFIER) parse_error("expected identifier for assignment", token_head, *cur);
    char *name = (*cur)->value;
    *cur = (*cur)->next;
    if (!expect(cur, ASSIGN)) parse_error("expected '=' for assignment", token_head, *cur);
    ASTNode *expr = parse_expr(cur);
    if (!expect(cur, SEMICOLON)) parse_error("expected ';' after assignment", token_head, *cur);
    return new_assign(new_identifier(name), expr);
}

ASTNode *parse_stmt(Token **cur) {
    if ((*cur)->kind == IF) return parse_if_stmt(cur);
    if ((*cur)->kind == WHILE) return parse_while_stmt(cur);
    if ((*cur)->kind == FOR) return parse_for_stmt(cur);
    if ((*cur)->kind == RETURN) return parse_return_stmt(cur);
    if ((*cur)->kind == L_BRACE) return parse_block(cur);
    if (is_type((*cur)->kind, *cur)) return parse_variable_declaration(cur, 1);
    if ((*cur)->kind == IDENTIFIER && (*cur)->next && (*cur)->next->kind == ASSIGN) {
        return parse_variable_assignment(cur);
    }
    return parse_expr_stmt(cur);
}

ASTNode* parse_fundef(Token **cur) {
    char *ret_type = parse_type(cur);
    if ((*cur)->kind != IDENTIFIER) parse_error("expected function name", token_head, *cur);
    char *name = (*cur)->value;
    *cur = (*cur)->next;
    if (!expect(cur, L_PARENTHESES)) parse_error("expected '(' after function name", token_head, *cur);

    int param_count = 0;
    ASTNode **params = NULL;
    if ((*cur)->kind != R_PARENTHESES)
        params = parse_param_list(cur, &param_count);

    if (!expect(cur, R_PARENTHESES)) parse_error("expected ')' after parameter list", token_head, *cur);
    ASTNode *body = parse_block(cur);
    ASTNode *fndef = new_fundef(ret_type, name, params, param_count, body);
    add_function(fndef);
    return fndef;
}
ASTNode* parse_toplevel(Token **cur) {
    //if ((*cur)->kind == TYPEDEF) return parse_typedef(cur);
    //if ((*cur)->kind == STRUCT) return parse_struct(cur);
    if (is_type((*cur)->kind, *cur)) {
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
            printf("Binary: %s\n", tokenkind2str(node->binary.op));
            print_ast(node->binary.left, indent+1);
            print_ast(node->binary.right, indent+1);
            break;
        case AST_TYPE:
            printf("Type: ");
            print_ast(node->type_node.base_type, 0);
            printf(" pointers: %d\n", node->type_node.pointer_level);
            printf(" modifiers: %d\n", node->type_node.type_modifiers);
            if (node->type_node.type_modifiers & TYPEMOD_CONST) printf(" const");
            if (node->type_node.type_modifiers & TYPEMOD_UNSIGNED) printf(" unsigned");
            if (node->type_node.type_modifiers & TYPEMOD_SIGNED) printf(" signed");
            printf("\n");
            break;

        case AST_TYPE_ARRAY:
            printf("TypeArray: ");
            print_ast(node->type_array.element_type, 0);
            printf(" size: %d\n", node->type_array.array_size);
            break;
        
        case AST_VAR_DECL:
            printf("VarDecl:\n");
            for (int i = 0; i < indent+1; i++) printf("  ");
            printf("Type:\n");
            print_ast(node->var_decl.var_type, indent+2);
            for (int i = 0; i < indent+1; i++) printf("  ");
            printf("Name: %s\n", node->var_decl.name);
            if (node->var_decl.init) {
                for (int i = 0; i < indent+1; i++) printf("  ");
                printf("Init:\n");
                print_ast(node->var_decl.init, indent+2);
            }
            break;
        
        case AST_ASSIGN:
            printf("Assign\n");
            print_ast(node->assign.left, indent+1);
            print_ast(node->assign.right, indent+1);
            break;
        case AST_UNARY:
            if(node->unary.op == AMPERSAND)
                printf("Unary: & (address)\n");
            else if(node->unary.op == SUB)
                printf("Unary: - (negate)\n");
            else if(node->unary.op == INC)
                printf("Unary: ++ (post-increment)\n");
            else if(node->unary.op == DEC)
                printf("Unary: -- (post-decrement)\n");
            else if(node->unary.op == ASTARISK)
                printf("Unary: * (dereference)\n");
            else if(node->unary.op == POST_INC)
                printf("Unary: ++ (pre-increment)\n");
            else if(node->unary.op == POST_DEC)
                printf("Unary: -- (pre-decrement)\n");
            else
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
        case AST_CALL:
            printf("Call: %s\n", node->call.name);
            for (int i = 0; i < node->call.arg_count; i++) {
                print_ast(node->call.args[i], indent + 1);
            }
            break;
        case AST_WHILE:
            printf("While\n");
            print_ast(node->while_stmt.cond, indent+1);
            print_ast(node->while_stmt.body, indent+1);
            break;
        case AST_FOR:
            printf("For\n");
            if (node->for_stmt.init) {
                for (int i = 0; i < indent+1; i++) printf("  ");
                printf("Init:\n");
                print_ast(node->for_stmt.init, indent+2);
            }
        case AST_PARAM:
            printf("Param: %s %s\n", node->param.type, node->param.name);
            break;
        case AST_TYPEDEF:
            printf("Typedef: %s -> %s\n", node->typedef_stmt.orig_type, node->typedef_stmt.new_type);
            break;
        case AST_STRUCT:
            printf("Struct: %s\n", node->struct_stmt.name);
            for (int i = 0; i < node->struct_stmt.member_count; i++) {
                for (int j = 0; j < indent+1; j++) printf("  ");
                printf("Member: %s %s\n", node->struct_stmt.members[i]->struct_member.name, node->struct_stmt.members[i]->struct_member.name);
            }
            break;
        case AST_STRUCT_MEMBER:
            printf("StructMember: %s %s\n", node->struct_member.type, node
->struct_member.name);
            break;
        case AST_TYPEDEF_STRUCT:
            printf("TypedefStruct: %s -> %s\n", node->typedef_struct.struct_name, node->typedef_struct.typedef_name);
            for (int i = 0; i < node->typedef_struct.member_count; i++) {
                for (int j = 0; j < indent+1; j++) printf("  ");
                printf("Member: %s %s\n", node->typedef_struct.members[i]->struct_member.type, node->typedef_struct.members[i]->struct_member.name);
            }
            break;
        case AST_STRING_LITERAL:
            printf("StringLiteral: %s\n", node->string_literal.value);
            break;
        case AST_CHAR_LITERAL:
            printf("CharLiteral: '%c'\n", node->char_literal.value);
            break;
        
        default:
            printf("Unknown AST Node Type: %d\n", node->type);
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
        case AST_VAR_DECL:
            free(node->var_decl.var_type);
            free(node->var_decl.name);
            if (node->var_decl.init) free_ast(node->var_decl.init);
            break;
        case AST_TYPE:
            free_ast(node->type_node.base_type);
            free(node->type_node.base_type);
            free(node->type_node.pointer_level);
            free(node->type_node.type_modifiers);
            break;
        case AST_STRING_LITERAL:
            free(node->string_literal.value);
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
        case AST_CALL:
            free(node->call.name);
            for (int i = 0; i < node->call.arg_count; i++)
                free_ast(node->call.args[i]);
            free(node->call.args);
            break;

        case AST_PARAM:
            free(node->param.type);
            free(node->param.name);
            break;
        case AST_TYPEDEF:
            free(node->typedef_stmt.orig_type);
            free(node->typedef_stmt.new_type);
            break;
        case AST_STRUCT:
            free(node->struct_stmt.name);
            for (int i = 0; i < node->struct_stmt.member_count; i++)
                free_ast(node->struct_stmt.members[i]);
            free(node->struct_stmt.members);
            break;
        case AST_STRUCT_MEMBER:
            free(node->struct_member.type);
            free(node->struct_member.name);
            break;  
        case AST_WHILE:
            free_ast(node->while_stmt.cond);
            free_ast(node->while_stmt.body);
            break;
        case AST_FOR:
            if (node->for_stmt.init) free_ast(node->for_stmt.init); 
            if (node->for_stmt.cond) free_ast(node->for_stmt.cond);
            if (node->for_stmt.inc) free_ast(node->for_stmt.inc);
            free_ast(node->for_stmt.body);
            break;
            
        default:
            fprintf(stderr, "Unknown AST Node Type: %d\n", node->type);
            exit(1);
    }
    free(node);
}