#ifndef FILE_OPERATIONS_H_
#define FILE_OPERATIONS_H_

#include <stdio.h>

int GetFileSize(FILE *finput);

void *ReadFile(FILE *finput, char *str, const int numberBytesSize);

#endif // FILE_OPERATIONS_H_
