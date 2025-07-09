#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>

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

void saveOutput(const char *filePath, const char *content) {
    #ifdef _WIN32
        _mkdir("tests\\outputs");
    #else
        mkdir("tests/outputs", 0777);
    #endif

    FILE *f = fopen(filePath, "w");
    if (!f) {
        perror("Failed to open output file");
        return;
    }
    fputs(content, f);
    fclose(f);
}

#endif