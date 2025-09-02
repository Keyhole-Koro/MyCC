#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>

// Reads entire file into a newly-allocated, NUL-terminated buffer.
// Caller is responsible for freeing the returned buffer.
char *readSampleInput(const char *filePath);

// Saves content to filePath, creating the tests/outputs directory if needed.
void saveOutput(const char *filePath, const char *content);

#endif
