#ifndef PROGRAMMING_LANGUAGE_H_
#define PROGRAMMING_LANGUAGE_H_

#include "Tree.h"

enum ErrorCode
{
    NO_ERROR,
    SYNTAX_ERROR,
    REALLOC_ERROR
};

union Token
{
    double value;
    NodeType keyword;
    char id[STR_MAX_SIZE];
};

struct Lexer
{
    Token *tokens;
    size_t capacity;
    size_t curToken;
    ErrorCode errorCode;
};

void Tokenizer(char *str, Lexer *lexer);

#endif // PROGRAMMING_LANGUAGE_H_

