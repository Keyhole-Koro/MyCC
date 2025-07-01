#ifndef AST_H
#define AST_H

#include <stdlib.h>

typedef enum {
    AST_NUMBER,

    AST_ADD,
    AST_SUB,
    AST_MUL,
    AST_DIV,

    AST_EXPR,

    AST_PROGRAM,

    AST_FUNCTION,
    AST_FUNCTION_DETAILS,
    AST_FUNCTION_RESULT,
    AST_PARAMETER,
    AST_RETURN,
    AST_RETURN_SIZE,
    AST_CALL_FUNC,
    AST_CALL_FUNC_PARAM,
    AST_ARG,

    AST_STACK_FRAME_SIZE,

    AST_DECLARE_VAR,
    AST_IDENTIFIER,
    AST_VARIABLE,
    AST_VARIABLE_OFFSET,
    AST_TYPE,
    AST_TYPE_SIZE,
    AST_ASSIGN,

    AST_ARRAY,
    AST_INDEX,
    AST_ARRAY_OFFSET,

    AST_POINTER,
    AST_DEREFENCE_OPERATOR,
    AST_ADDRESS_OPERATOR,
    AST_NO_OPERATOR,

    AST_STATEMENT,

    AST_IF,
    AST_ELSE_IF,
    AST_ELSE,
    AST_CONDITION,
    AST_CONDITIONAL_STATEMTNT,
    AST_EQ,       // ==
    AST_NEQ,      // !=
    AST_LT,       // <
    AST_GT,       // >
    AST_LTE,      // <=
    AST_GTE,      // >=
    AST_AND,      // &&
    AST_OR,       // ||

} AST_Type;

#endif