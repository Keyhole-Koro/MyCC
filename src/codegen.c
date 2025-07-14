#include "codegen.h"
#include <stdio.h>
#include <string.h>

// Argument registers for first three args
static const char *arg_regs[] = {"r5", "r6", "r7"};

// How many bytes/slot per variable? (usually 4)
#define SLOT_SIZE 4

// Get offset for parameter n (first param: n=0 → bp+4)
static int param_offset(int n) { return -(4 + n * SLOT_SIZE); }
// Get offset for local n (first local: n=0 → bp-4)
static int local_offset(int n) { return -SLOT_SIZE * (n + 1); }

// Find param index by name, or -1
static int param_index(const char *name, char **params, int param_count)
{
    for (int i = 0; i < param_count; i++)
        if (strcmp(name, params[i]) == 0)
            return i;
    return -1;
}
// Find local index by name, or -1
static int local_index(const char *name, char **locals, int local_count)
{
    for (int i = 0; i < local_count; i++)
        if (strcmp(name, locals[i]) == 0)
            return i;
    return -1;
}

// Recursively collect all local variable names in the block and its nested statements
int collect_locals(ASTNode *node, char **locals)
{
    int count = 0;
    if (!node)
        return 0;
    switch (node->type)
    {
    case AST_BLOCK:
        for (int i = 0; i < node->block.count; i++)
        {
            count += collect_locals(node->block.stmts[i], locals + count);
        }
        break;
    case AST_VAR_DECL:
        locals[count++] = node->var_decl.name;
        break;
    case AST_FOR:
        // Collect locals from the init part (e.g. for (int i = ...))
        if (node->for_stmt.init)
            count += collect_locals(node->for_stmt.init, locals + count);
        // Collect from body and inc, just in case there are decls there too
        if (node->for_stmt.body)
            count += collect_locals(node->for_stmt.body, locals + count);
        if (node->for_stmt.inc)
            count += collect_locals(node->for_stmt.inc, locals + count);
        break;
    case AST_IF:
        if (node->if_stmt.then_stmt)
            count += collect_locals(node->if_stmt.then_stmt, locals + count);
        if (node->if_stmt.else_stmt)
            count += collect_locals(node->if_stmt.else_stmt, locals + count);
        break;
    // Add other cases (AST_WHILE, AST_BLOCK, etc.) if needed
    default:
        break;
    }
    return count;
}

// Compute offset for a variable name
int find_var_offset(const char *name, char **params, int param_count,
                    char **locals, int local_count, int *is_param)
{
    int idx = param_index(name, params, param_count);
    if (idx >= 0)
    {
        if (is_param)
            *is_param = 1;
        return param_offset(idx);
    }
    idx = local_index(name, locals, local_count);
    if (idx >= 0)
    {
        if (is_param)
            *is_param = 0;
        return local_offset(idx);
    }
    if (is_param)
        *is_param = -1;
    return 0;
}

void emit_unary_inc_dec(ASTNode *node, StringBuilder *sb, const char *target_reg,
                        char **params, int param_count,
                        char **locals, int local_count)
{
    if (!node || node->type != AST_UNARY || node->unary.operand->type != AST_IDENTIFIER)
    {
        sb_append(sb, "  ; unsupported unary operation\n");
        return;
    }

    const char *var_name = node->unary.operand->identifier.name;

    // Load the current value
    emit_load_var(sb, var_name, "r1", params, param_count, locals, local_count);

    if (node->unary.op == POST_INC || node->unary.op == INC)
    {
        sb_append(sb, "  \n; increment variable '%s'\n", var_name);
        // Increment r1 by 1
        sb_append(sb, "  addis r1, 1\n");
    }
    else if (node->unary.op == POST_DEC || node->unary.op == DEC)
    {
        sb_append(sb, "  \n; decrement variable '%s'\n", var_name);
        // Decrement r1 by 1
        sb_append(sb, "  addis r1, -1\n");
    }
    else
    {
        sb_append(sb, "  ; unknown unary op\n");
        exit(1);
        return;
    }

    // Store back to variable
    emit_store_var(sb, var_name, "r1", params, param_count, locals, local_count);

    // Move result to target register if needed
    if (strcmp(target_reg, "r1") != 0)
        sb_append(sb, "  mov %s, r1\n", target_reg);
}

// Emit code to load variable (param/local/global) to target_reg
void emit_load_var(StringBuilder *sb, const char *name, const char *target_reg,
                   char **params, int param_count,
                   char **locals, int local_count)
{
    int is_param = 0;
    int idx = param_index(name, params, param_count);
    int offset;
    if (idx >= 0)
    {
        is_param = 1;
        if (idx < 3)
        {
            // bp-4, bp-8, bp-12…
            offset = -(4 + idx * SLOT_SIZE);
            sb_append(sb, "  \n; load param '%s' (arg%d, reg) into %s\n", name, idx + 1, target_reg);
            sb_append(sb, "  mov   r3, bp\n");
            sb_append(sb, "  addis r3, %d\n", offset);
            sb_append(sb, "  load  %s, r3\n", target_reg);
        }
        else
        {
            // bp+N
            offset = 4 + (idx - 3) * SLOT_SIZE;
            sb_append(sb, "  \n; load param '%s' (arg%d, stack) into %s\n", name, idx + 1, target_reg);
            sb_append(sb, "  mov   r3, bp\n");
            sb_append(sb, "  addis r3, %d\n", offset);
            sb_append(sb, "  load  %s, r3\n", target_reg);
        }
    }
    else
    {
        int local_idx = local_index(name, locals, local_count);
        if (local_idx >= 0)
        {
            // bp-4, bp-8, ...
            offset = -SLOT_SIZE * (local_idx + 1);
            sb_append(sb, "  \n; load local '%s' into %s\n", name, target_reg);
            sb_append(sb, "  mov   r3, bp\n");
            sb_append(sb, "  addis r3, %d\n", offset);
            sb_append(sb, "  load  %s, r3\n", target_reg);
        }
        else
        {
            // fallback global
            sb_append(sb, "  movi  r2, %s\n", name);
            sb_append(sb, "  load  %s, r2\n", target_reg);
        }
    }
}

// Emit code to store target_reg to variable (param/local/global)
void emit_store_var(StringBuilder *sb, const char *name, const char *src_reg,
                    char **params, int param_count,
                    char **locals, int local_count)
{
    int is_param = 0;
    int offset = find_var_offset(name, params, param_count, locals, local_count, &is_param);
    if (is_param == 1 || is_param == 0)
    {
        sb_append(sb, "  \n; store %s to var '%s'\n", src_reg, name);
        sb_append(sb, "  mov   r3, bp\n");
        sb_append(sb, "  addis r3, %d\n", offset);
        sb_append(sb, "  store r3, %s\n", src_reg);
    }
    else
    {
        // fallback global
        sb_append(sb, "  movi  r3, %s\n", name);
        sb_append(sb, "  store r3, %s\n", src_reg);
    }
}

// Emit conditional jump based on binary comparison operator
// If the condition is true, jump to `trueLabel`
// If the condition is false, jump to `falseLabel` (optional)
// Supported operators: ==, !=, <, >, <=, >= using basic jz, jnz, jl, jg
void emit_cond_jump(ASTNode *left, ASTNode *right, TokenKind op, StringBuilder *sb,
                    char **params, int param_count, char **locals, int local_count,
                    const char *trueLabel, const char *falseLabel)
{
    // Generate left and right expressions into r2 and r3
    gen_expr(left, sb, "r2", params, param_count, locals, local_count);
    gen_expr(right, sb, "r3", params, param_count, locals, local_count);
    sb_append(sb, "  cmp r2, r3\n");

    // Emit jump instructions based on operator
    switch (op)
    {
    case EQ: // ==
        sb_append(sb, "  jz %s\n", trueLabel);
        if (falseLabel)
            sb_append(sb, "  jmp %s\n", falseLabel);
        break;
    case NEQ: // !=
        sb_append(sb, "  jnz %s\n", trueLabel);
        if (falseLabel)
            sb_append(sb, "  jmp %s\n", falseLabel);
        break;
    case LT: // <
        sb_append(sb, "  jl %s\n", trueLabel);
        if (falseLabel)
            sb_append(sb, "  jmp %s\n", falseLabel);
        break;
    case GT: // >
        sb_append(sb, "  jg %s\n", trueLabel);
        if (falseLabel)
            sb_append(sb, "  jmp %s\n", falseLabel);
        break;
    case LTE: // <= → !(a > b)
        if (falseLabel)
            sb_append(sb, "  jg %s\n", falseLabel); // if a > b → jump to false
        sb_append(sb, "  jmp %s\n", trueLabel);     // else → true
        break;
    case GTE: // >= → !(a < b)
        if (falseLabel)
            sb_append(sb, "  jl %s\n", falseLabel); // if a < b → jump to false
        sb_append(sb, "  jmp %s\n", trueLabel);     // else → true
        break;
    default:
        // Fallback: treat nonzero as true
        sb_append(sb, "  jnz %s\n", trueLabel);
        if (falseLabel)
            sb_append(sb, "  jmp %s\n", falseLabel);
        break;
    }
}

// gen_expr: output result to target_reg (should be r5/r6/r7)
void gen_expr(ASTNode *node, StringBuilder *sb, const char *target_reg,
              char **params, int param_count,
              char **locals, int local_count);

// gen binary op (result to target_reg)
void gen_expr_binop(ASTNode *node, StringBuilder *sb, const char *target_reg,
                    char **params, int param_count, char **locals, int local_count)
{
    // left → r2, right → r1
    gen_expr(node->binary.left, sb, "r2", params, param_count, locals, local_count);
    gen_expr(node->binary.right, sb, "r1", params, param_count, locals, local_count);
    switch (node->binary.op)
    {
    case ADD:
        sb_append(sb, "\n; addition\n  add  r1, r2\n");
        break;
    case SUB:
        sb_append(sb, "\n; subtraction\n  sub  r1, r2\n");
        break;

    // case ASTARISK: sb_append(sb, "  mcp  r1, r2\n"); break;
    // case DIV:      sb_append(sb, "  div  r1, r2\n"); break;
    default:
        sb_append(sb, "  \n; unknown binary op\n");
        exit(1);
    }
    if (strcmp(target_reg, "r1") != 0)
        sb_append(sb, "  mov %s, r1\n", target_reg);
}

void gen_call(ASTNode *node, StringBuilder *sb, const char *target_reg,
              char **params, int param_count, char **locals, int local_count)
{
    int argc = node->call.arg_count;

    // Allocate space for stack-passed arguments (4th and beyond)
    if (argc > 3)
    {
        sb_append(sb, "  ; allocate stack space for stack arguments\n");
        for (int i = 3; i < argc; i++)
        {
            gen_expr(node->call.args[i], sb, "r1", params, param_count, locals, local_count);
            sb_append(sb, "  mov r2, sp\n");
            sb_append(sb, "  addis r2, %d\n", (i - 3) * SLOT_SIZE);
            sb_append(sb, "  store r2, r1\n"); // Store the argument value at [sp + offset]
        }
    }

    // Pass the first 3 arguments via registers r5, r6, r7 (left to right)
    for (int i = 0; i < argc && i < 3; i++)
    {
        gen_expr(node->call.args[i], sb, arg_regs[i], params, param_count, locals, local_count);
    }

    sb_append(sb, "  call f_%s\n", node->call.name);

    // After call, restore stack pointer
    if (argc > 3)
    {
        sb_append(sb, "  ; restore sp after call\n");
        sb_append(sb, "  addis sp, %d\n", (argc - 3) * SLOT_SIZE);
    }

    // Move return value to target register if needed
    if (strcmp(target_reg, "r1") != 0)
        sb_append(sb, "  mov %s, r1\n", target_reg);
}

void gen_if(ASTNode *node, StringBuilder *sb,
            char **params, int param_count,
            char **locals, int local_count)
{
    static int label_count = 0;
    int cur_label = label_count++;

    char then_label[32], else_label[32], end_label[32];
    snprintf(then_label, sizeof(then_label), "b_L_then_%d", cur_label);
    snprintf(end_label, sizeof(end_label), "b_L_end_%d", cur_label);

    if (node->if_stmt.else_stmt)
        snprintf(else_label, sizeof(else_label), "b_L_else_%d", cur_label);
    else
        strcpy(else_label, end_label);

    ASTNode *cond = node->if_stmt.cond;

    if (cond->type == AST_BINARY)
    {
        emit_cond_jump(cond->binary.left, cond->binary.right, cond->binary.op, sb,
                       params, param_count, locals, local_count, then_label, else_label);
    }
    else
    {
        // General case: treat cond as value
        gen_expr(cond, sb, "r1", params, param_count, locals, local_count);
        sb_append(sb, "  cmp r1, 0\n");
        sb_append(sb, "  jnz %s\n", then_label);
        sb_append(sb, "  jmp %s\n", else_label);
    }

    sb_append(sb, "%s:\n", then_label);
    gen_stmt(node->if_stmt.then_stmt, sb, params, param_count, locals, local_count);
    sb_append(sb, "  jmp %s\n", end_label);

    if (node->if_stmt.else_stmt)
    {
        sb_append(sb, "%s:\n", else_label);
        gen_stmt(node->if_stmt.else_stmt, sb, params, param_count, locals, local_count);
    }
    sb_append(sb, "%s:\n", end_label);
}
void gen_for(ASTNode *node, StringBuilder *sb,
             char **params, int param_count,
             char **locals, int local_count)
{
    static int label_count = 0;
    int cur_label = label_count++;
    char for_cond[32], for_body[32], for_inc[32], for_end[32];
    snprintf(for_cond, sizeof(for_cond), "b_L_for_cond_%d", cur_label);
    snprintf(for_body, sizeof(for_body), "b_L_for_body_%d", cur_label);
    snprintf(for_inc, sizeof(for_inc), "b_L_for_inc_%d", cur_label);
    snprintf(for_end, sizeof(for_end), "b_L_for_end_%d", cur_label);

    if (node->for_stmt.init)
        gen_stmt(node->for_stmt.init, sb, params, param_count, locals, local_count);

    sb_append(sb, "%s:\n", for_cond);

    if (node->for_stmt.cond && node->for_stmt.cond->type == AST_BINARY)
    {
        emit_cond_jump(node->for_stmt.cond->binary.left, node->for_stmt.cond->binary.right,
                       node->for_stmt.cond->binary.op, sb,
                       params, param_count, locals, local_count,
                       for_body, for_end);
    }
    else if (node->for_stmt.cond)
    {
        gen_expr(node->for_stmt.cond, sb, "r1", params, param_count, locals, local_count);
        sb_append(sb, "  cmp r1, 0\n");
        sb_append(sb, "  jnz %s\n", for_body);
        sb_append(sb, "  jmp %s\n", for_end);
    }
    else
    {
        sb_append(sb, "  jmp %s\n", for_body);
    }

    sb_append(sb, "%s:\n", for_body);
    gen_stmt(node->for_stmt.body, sb, params, param_count, locals, local_count);

    sb_append(sb, "%s:\n", for_inc);
    if (node->for_stmt.inc)
        gen_stmt(node->for_stmt.inc, sb, params, param_count, locals, local_count);

    sb_append(sb, "  jmp %s\n", for_cond);
    sb_append(sb, "%s:\n", for_end);
}

void gen_expr(ASTNode *node, StringBuilder *sb, const char *target_reg,
              char **params, int param_count,
              char **locals, int local_count)
{
    switch (node->type)
    {
    case AST_NUMBER:
        sb_append(sb, "  \n; load constant %s into %s\n", node->number.value, target_reg);
        sb_append(sb, "  movi  %s, %s\n", target_reg, node->number.value);
        break;
    case AST_UNARY:
        switch (node->unary.op) {
        case ASTARISK: // *
            gen_expr(node->unary.operand, sb, "r3", params, param_count, locals, local_count);
            sb_append(sb, "  ; dereference *expr\n");
            sb_append(sb, "  load %s, r3\n", target_reg);
            break;
        case AMPERSAND: // &
            if (node->unary.operand->type == AST_IDENTIFIER) {
                const char *var_name = node->unary.operand->identifier.name;
                int is_param;
                int offset = find_var_offset(var_name, params, param_count, locals, local_count, &is_param);
                sb_append(sb, "  ; address-of &%s\n", var_name);
                sb_append(sb, "  mov %s, bp\n", target_reg);
                sb_append(sb, "  addis %s, %d\n", target_reg, offset);
            } else {
                sb_append(sb, "  ; & of non-identifier not supported\n");
                exit(1);
            }
            break;
        default:
            emit_unary_inc_dec(node, sb, target_reg, params, param_count, locals, local_count);
        }
        break;
    
    case AST_IDENTIFIER:
        emit_load_var(sb, node->identifier.name, target_reg, params, param_count, locals, local_count);
        break;
    case AST_BINARY:
        gen_expr_binop(node, sb, target_reg, params, param_count, locals, local_count);
        break;
    case AST_CALL:
        gen_call(node, sb, target_reg, params, param_count, locals, local_count);
        break;
    default:
        sb_append(sb, "  \n; unknown expr node %d\n", node->type);
        exit(1);
    }
}

// Statement codegen
void gen_stmt(ASTNode *node, StringBuilder *sb,
              char **params, int param_count,
              char **locals, int local_count)
{
    switch (node->type)
    {
        printf("gen_stmt: processing node type %d\n", node->type);
    case AST_VAR_DECL:
        if (node->var_decl.init)
        {
            gen_expr(node->var_decl.init, sb, "r1", params, param_count, locals, local_count);
            emit_store_var(sb, node->var_decl.name, "r1", params, param_count, locals, local_count);
        }
        break;
    case AST_UNARY:
        emit_unary_inc_dec(node, sb, "r1", params, param_count, locals, local_count);
        break;
    case AST_ASSIGN:
        if (node->assign.left->type == AST_UNARY && node->assign.left->unary.op == ASTARISK) {
            // *p = val
            gen_expr(node->assign.left->unary.operand, sb, "r3", params, param_count, locals, local_count); // r3 = addr
            gen_expr(node->assign.right, sb, "r1", params, param_count, locals, local_count); // r1 = value
            sb_append(sb, "  ; *ptr = value\n");
            sb_append(sb, "  store r3, r1\n");
        } else {
            gen_expr(node->assign.right, sb, "r1", params, param_count, locals, local_count);
            emit_store_var(sb, node->assign.left->identifier.name, "r1", params, param_count, locals, local_count);
        }
        break;
    
    case AST_EXPR_STMT:
        gen_expr(node->expr_stmt.expr, sb, "r1", params, param_count, locals, local_count);
        break;
    case AST_IF:
        gen_if(node, sb, params, param_count, locals, local_count);
        break;
    case AST_FOR:
        gen_for(node, sb, params, param_count, locals, local_count);
        break;
    case AST_RETURN:
        gen_expr(node->ret.expr, sb, "r1", params, param_count, locals, local_count);
        // r1 = return value. No 'ret' for main.
        sb_append(sb, "  \n; return\n");
        break;
    case AST_BLOCK:
        for (int i = 0; i < node->block.count; i++)
        {
            gen_stmt(node->block.stmts[i], sb, params, param_count, locals, local_count);
        }
        break;
    default:
        sb_append(sb, "  \n; [stmt] unknown node type: %d\n", node->type);
        exit(1);
    }
}

void gen_func(ASTNode *node, StringBuilder *sb)
{
    if (node->type != AST_FUNDEF)
        return;

    char *fname = strcmp(node->fundef.name, "main") == 0 ? "__START__" : node->fundef.name;
    int param_count = node->fundef.param_count;
    char *params[16] = {0};
    for (int i = 0; i < param_count; i++)
    {
        params[i] = node->fundef.params[i]->param.name;
    }

    char *locals[32] = {0};
    int local_count = collect_locals(node->fundef.body, locals);

    sb_append(sb, "\n");
    sb_append(sb, "%s%s:\n", strcmp(fname, "__START__") == 0 ? "" : "f_", fname);
    sb_append(sb, "; prologue\n");
    sb_append(sb, "  push bp\n");
    sb_append(sb, "  mov bp, sp\n  addis sp, -%d\n",
              (local_count + param_count) * SLOT_SIZE);

    // Store first 3 parameters from registers to stack frame
    for (int i = 0; i < param_count && i < 3; i++)
    {
        sb_append(sb, "  ; store parameter '%s' from register %s\n", params[i], arg_regs[i]);
        sb_append(sb, "  mov   r3, bp\n");
        sb_append(sb, "  addis r3, %d\n", param_offset(i));
        sb_append(sb, "  store r3, %s\n", arg_regs[i]);
    }
    // For parameters 4 and above: nothing is needed; they are already in [bp+offset] positions

    // Function body
    gen_stmt(node->fundef.body, sb, params, param_count, locals, local_count);

    sb_append(sb, "  addis sp, %d\n", (local_count + param_count) * SLOT_SIZE);
    sb_append(sb, "; epilogue\n  pop  bp\n");

    // Epilogue (not for main)
    if (strcmp(fname, "__START__") != 0)
    {
        sb_append(sb, "  mov  pc, lr\n");
    }
    if (strcmp(fname, "__START__") == 0)
        sb_append(sb, "  halt");
}

char *codegen(ASTNode *root)
{
    StringBuilder sb;
    sb_init(&sb);

    // Output __START__ (main) first
    for (int i = 0; i < root->block.count; i++)
    {
        ASTNode *fn = root->block.stmts[i];
        if (fn->type == AST_FUNDEF && strcmp(fn->fundef.name, "main") == 0)
        {
            gen_func(fn, &sb);
            break;
        }
    }
    // Output all other functions
    for (int i = 0; i < root->block.count; i++)
    {
        ASTNode *fn = root->block.stmts[i];
        if (fn->type == AST_FUNDEF && strcmp(fn->fundef.name, "main") != 0)
        {
            gen_func(fn, &sb);
        }
    }
    return sb_dump(&sb);
}
