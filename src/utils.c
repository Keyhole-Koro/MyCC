#include "utils.h"

#include <errno.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

char *readSampleInput(const char *filePath) {
    FILE *file = fopen(filePath, "rb");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        perror("Failed to seek file");
        fclose(file);
        return NULL;
    }
    long fileSize = ftell(file);
    if (fileSize < 0) {
        perror("Failed to tell file size");
        fclose(file);
        return NULL;
    }
    rewind(file);

    char *buffer = (char*)malloc((size_t)fileSize + 1);
    if (!buffer) {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }
    size_t readn = fread(buffer, 1, (size_t)fileSize, file);
    if (readn != (size_t)fileSize) {
        perror("Failed to read entire file");
        free(buffer);
        fclose(file);
        return NULL;
    }
    buffer[fileSize] = '\0';
    fclose(file);
    return buffer;
}

static void ensure_outputs_dir(void) {
#ifdef _WIN32
    _mkdir("tests\\outputs");
#else
    // Attempt to create; ignore EEXIST
    if (mkdir("tests/outputs", 0777) != 0 && errno != EEXIST) {
        perror("mkdir tests/outputs");
    }
#endif
}

void saveOutput(const char *filePath, const char *content) {
    ensure_outputs_dir();

    FILE *f = fopen(filePath, "wb");
    if (!f) {
        perror("Failed to open output file");
        return;
    }
    if (content) {
        fputs(content, f);
    }
    fclose(f);
}

