#ifndef GENERATE_ASM_CODE_H_
#define GENERATE_ASM_CODE_H_

#include "Tree.h"

const int INITIAL_CAPACITY = 20;
const int REG_NAME_SIZE    = 2;
const int NUMBERS_VARIABLE = 10;
const int NUMBERS_LABEL    = 10;
const int NUMBERS_FUNCTION = 10;

struct Name
{
    char str[STR_MAX_SIZE];
    int curOffset;
};

struct TableLocalNames
{
    Name localNames[NUMBERS_VARIABLE];
    int curName;
};

struct TableGlobalNames
{
    Name globalNames[NUMBERS_VARIABLE];
    int curName;
};

struct TableFunctions
{
    Name functions[NUMBERS_FUNCTION];
    int curName;
};

void GenerateAsmCode(Tree_t *tree);

#endif // GENERATE_ASM_CODE_H_
