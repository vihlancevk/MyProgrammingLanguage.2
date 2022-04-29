#ifndef PARSER_H_
#define PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include "Tokenizer.h"
#include "Tree.h"

enum ParserErrorCode
{
    PARSER_NO_ERROR,
    PARSER_SYNTAX_ERROR
};

struct UnaryOperation
{
    const NodeType nodeType;
    char *str;
};

struct BoolOperation
{
    const NodeType nodeType;
    char *str;
};

struct Parser
{
    Token *tokens;
    size_t curToken;
    ParserErrorCode parserError;
    Tree_t *tree;
};

Tree_t* SyntacticAnalysis(Parser *parser);

#endif // PARSER_H_
