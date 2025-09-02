#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "AST.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.c> <output.asm>\n", argv[0]);
        return 1;
    }
    char *input_path = argv[1];
    char *output_path = argv[2];

    char *input = readSampleInput(input_path);

    Token *tokens = lexer(input);
    for (Token *t = tokens; t; t = t->next) {
        printf("Token: kind=%s, value=%s\n", tokenkind2str(t->kind), t->value ? t->value : "(null)");
    }
    free(input);

    Token *cur = tokens;
    ASTNode *root = parse_program(&cur);

    print_ast(root, 0);
    printf("AST parsing completed.\n");

    char *output = codegen(root);
    
    if (!output) {
        fprintf(stderr, "Code generation failed.\n");
        return 1;
    }
    saveOutput(output_path, output);
    
    printf("Code generation completed. Output saved to %s\n", output_path);
    
    free(output);

    return 0;
}
