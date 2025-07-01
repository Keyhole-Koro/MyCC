#include "unity.h"
#include "../inc/lexer.h"
#include <stdio.h>
#include <stdlib.h>

void setUp(void) {}
void tearDown(void) {}

char *readSampleInput(const char *filePath) {
    FILE *file = fopen(filePath, "r");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    char *buffer = malloc(fileSize + 1);
    if (!buffer) {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, fileSize, file);
    buffer[fileSize] = '\0';
    fclose(file);
    return buffer;
}

void test_lexer_processes_sample_input(void) {
    char *input = readSampleInput("tests/inputs/expr.c");

    Token *tokens = lexer(input);
    for (Token *cur = tokens; cur != NULL; cur = cur->next) {
        printf("Token kind: %d, value: %s\n", cur->kind, cur->value);
    }
    free(input);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_lexer_processes_sample_input);
    return UNITY_END();
}