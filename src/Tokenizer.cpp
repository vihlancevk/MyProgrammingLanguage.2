#include <assert.h>
#include <math.h>
#include <string.h>
#include "../include/Tokenizer.h"

const size_t EXPANSION_COEFFICIENT = 2   ;
const double NO_VALUE              = -1.0;

static void ReallocTokenArray(Lexer *lexer)
{
    assert(lexer != nullptr);

    if (lexer->curToken >= lexer->capacity)
    {
        lexer->capacity *= EXPANSION_COEFFICIENT;
        Token *tokens = (Token*)realloc((void*)lexer->tokens, sizeof(Token) * lexer->capacity);
        if (tokens == nullptr)
        {
            lexer->errorCode = REALLOC_ERROR;
            return;
        }
        lexer->tokens = tokens;
    }
}

static void AddElemInTokenArray(Lexer *lexer, Token token)
{
    assert(lexer != nullptr);

    ReallocTokenArray(lexer);

    lexer->tokens[lexer->curToken] = token;

    lexer->curToken++;
}

const char *WHITESPACE_CHARACTERS = " \n\t";

static size_t SkeapWhitespaceCharacters(char *str, size_t curOffset)
{
    assert(str != nullptr);

    while (strchr(WHITESPACE_CHARACTERS, *(str + curOffset)) != nullptr)
    {
        curOffset++;
    }

    return curOffset;
}

void Tokenizer(char *str, Lexer *lexer)
{
    assert(str   != nullptr);
    assert(lexer != nullptr);

    size_t curOffset = 0;
    Token token = {};

    while(*(str + curOffset) != '\0')
    {
        curOffset = SkeapWhitespaceCharacters(str, curOffset);

        #define KEY_WORD(word, count, replace, thisKeyword) \
            if (strncmp(str + curOffset, word, count) == 0) \
            {                                               \
                token.keyword = thisKeyword;                \
                AddElemInTokenArray(lexer, token);          \
                curOffset += count;                         \
                continue;                                   \
            }

        #include "../include/Keyworld.h"

        if (isdigit((int)(unsigned char)*(str + curOffset)))
        {
            char *strPtr = str + curOffset;
            double value = strtod(strPtr, &strPtr);
            curOffset = strPtr - str;

            token.value = value;
            AddElemInTokenArray(lexer, token);
            continue;
        }

        if (isalpha((int)(unsigned char)*(str + curOffset)))
        {
            char *strPtr = str + curOffset;

            while((isalpha((int)(unsigned char)*(str + curOffset))  ||
                   isdigit((int)(unsigned char)*(str + curOffset))  ||
                   (*(str + curOffset) == '_')))
            {
                curOffset++;
            }

            sprintf(token.id, "'%s'", strndup(strPtr, str + curOffset - strPtr));
            AddElemInTokenArray(lexer, token);
            continue;
        }

        if (*(str + curOffset) == '$')
        {
            strcpy(token.id, "$");
            AddElemInTokenArray(lexer, token);
            curOffset++;
            continue;
        }

        curOffset = SkeapWhitespaceCharacters(str, curOffset);
    }
}

#undef KEY_WORD
