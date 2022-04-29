#include <assert.h>
#include <string.h>
#include "../include/FileOperations.h"

int GetFileSize(FILE *finput)
{
    assert(finput != nullptr);

    if (fseek(finput, 0, SEEK_END))
    {
        printf("Error fseek\n");
        return -1;
    }

    long numberBytesFile = ftell(finput);
    if (numberBytesFile == -1L)
    {
        printf("Error ftell\n");
        return -1;
    }

    rewind(finput);

    return (int)numberBytesFile;
}

void *ReadFile(FILE *finput, char *str, const int numberBytesFile)
{
    assert(finput != nullptr);
    assert(str != nullptr);
    assert(numberBytesFile > 0);

    if (((int)fread(str, sizeof(char), numberBytesFile + 1, finput) != numberBytesFile) && !feof(finput))
    {
        printf("Error fread\n");
        return nullptr;
    }
    str[strlen(str) - 1] = '\0';

    return str;
}
