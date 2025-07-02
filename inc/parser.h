#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "AST.h"

ASTNode* parse_program(Token **cur);
void print_ast(ASTNode *node, int indent);
void free_ast(ASTNode *node);

#endif
