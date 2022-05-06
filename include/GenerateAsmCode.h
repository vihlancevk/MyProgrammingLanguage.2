#ifndef GENERATE_ASM_CODE_H_
#define GENERATE_ASM_CODE_H_

#include "Tree.h"

const int32_t INITIAL_CAPACITY = 20;
const int32_t REG_NAME_SIZE    = 2;
const int32_t NUMBERS_VARIABLE = 10;
const int32_t NUMBERS_LABEL    = 10;
const int32_t NUMBERS_FUNCTION = 10;

struct Name
{
    char str[STR_MAX_SIZE];
    int32_t curOffset;
    int32_t addressCall;
};

struct TableLocalNames
{
    Name localNames[NUMBERS_VARIABLE];
    int32_t curName;
};

struct TableGlobalNames
{
    Name globalNames[NUMBERS_VARIABLE];
    int32_t curName;
};

struct TableFunctions
{
    Name functions[NUMBERS_FUNCTION];
    int32_t curName;
};

void GenerateAsmCode(Tree_t *tree);

#endif // GENERATE_ASM_CODE_H_
