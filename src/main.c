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
    free(input);

    Token *cur = tokens;
    ASTNode *root = parse_program(&cur);

    print_ast(root, 0);

    char *output = codegen(root);

    saveOutput(output_path, output);
    free(output);

    return 0;
}
