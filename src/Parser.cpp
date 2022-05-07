#include <assert.h>
#include <math.h>
#include <string.h>
#include "../include/Parser.h"

#define IS_PARSER_ERROR()                             \
    do                                                \
    {                                                 \
        if (parser->parserError != PARSER_NO_ERROR)   \
        {                                             \
            printf("Syntaxis error, %d\n", __LINE__); \
            return nullptr;                           \
        }                                             \
    } while(0)

static Node_t* GetG    (Parser *parser);
static Node_t* GetF    (Parser *parser);
static Node_t* GetPar  (Parser *parser);
static Node_t* GetA    (Parser *parser);
static Node_t* GetIf   (Parser *parser);
static Node_t* GetWhile(Parser *parser);
static Node_t* GetRet  (Parser *parser);
static Node_t* GetBool (Parser *parser);
static Node_t* GetE    (Parser *parser);
static Node_t* GetT    (Parser *parser);
static Node_t* GetPow  (Parser *parser);
static Node_t* GetUnary(Parser *parser);
static Node_t* GetP    (Parser *parser);
static Node_t* GetN    (Parser *parser);
static Node_t* GetCall (Parser *parser);
static Node_t* GetArray(Parser *parser);
static Node_t* GetId   (Parser *parser);

const int NUMBER_UNARY_OPERATIONS = 0;
const int NUMBER_BOOL_OPERATIONS  = 8;
const double NO_VALUE = -1.0;

UnaryOperation unaryOperation[NUMBER_UNARY_OPERATIONS] = {};

BoolOperation boolOperation[NUMBER_BOOL_OPERATIONS] = {{JG , (char*)">" },
                                                       {JL , (char*)"<" },
                                                       {JE , (char*)"=="},
                                                       {JGE, (char*)">="},
                                                       {JLE, (char*)"<="},
                                                       {JNE, (char*)"!="}};

static Node_t* NodeCtor(Node_t *thisNode)
{
    assert(thisNode != nullptr);

    Node_t *node = (Node_t*)calloc(1, sizeof(Node_t));
    node->parent = thisNode->parent;
    node->leftChild  = thisNode->leftChild;
    node->rightChild = thisNode->rightChild;
    node->nodeType = thisNode->nodeType;
    node->value = thisNode->value;
    if (thisNode->str != nullptr)
    {
        node->str = (char*)calloc(STR_MAX_SIZE, sizeof(char));
        strcpy(node->str, thisNode->str);
    }
    else
    {
        node->str = thisNode->str;
    }

    if (thisNode->leftChild  != nullptr) { thisNode->leftChild->parent  = node; }
    if (thisNode->rightChild != nullptr) { thisNode->rightChild->parent = node; }

    return node;
}

static void Require(Parser *parser, const NodeType nodeType)
{
    assert(parser != nullptr);

    if (parser->tokens[parser->curToken].keyword == nodeType) parser->curToken++;
    else parser->parserError = PARSER_SYNTAX_ERROR;
}

static Node_t* GetG(Parser *parser)
{
    assert(parser != nullptr);

    Node_t *node = nullptr;

    while (((strcmp(parser->tokens[parser->curToken].id, (char*)"\0") != 0 ||  parser->tokens[parser->curToken].keyword == MAIN)
            && parser->tokens[parser->curToken + 1].keyword == LB) || parser->tokens[parser->curToken + 1].keyword == ASSIGN)
    {
        if ((strcmp(parser->tokens[parser->curToken].id, (char*)"\0") != 0 ||  parser->tokens[parser->curToken].keyword == MAIN)
        && parser->tokens[parser->curToken + 1].keyword == LB)
        {
            Node_t *node1 = GetF(parser);
            IS_PARSER_ERROR();

            if (node1 != nullptr)
            {
                if (node != nullptr)
                {
                    node1->leftChild = node;
                    node->parent = node1;
                    node = node1;
                }
                else
                {
                    node = node1;
                }
            }
        }
        else if (parser->tokens[parser->curToken + 1].keyword == ASSIGN)
        {
            Node_t *node1 = (Node_t*)calloc(1, sizeof(Node_t));
            Node_t *node2 = TreeInsert(parser->tree, node1, RIGHT_CHILD, ASSIGN, NO_VALUE, (char*)"=");
            node2->leftChild = GetId(parser);
            IS_PARSER_ERROR();
            node2->leftChild->parent = node2;

            parser->curToken = parser->curToken + 1;

            node2->rightChild = GetBool(parser);
            IS_PARSER_ERROR();
            node2->rightChild->parent = node2;
            SetNodeTypeValueStr(node1, STATEMENT, NO_VALUE, (char*)"statement");
            Require(parser, SEMICOLON);

            if (node1 != nullptr)
            {
                if (node != nullptr)
                {
                    Node_t *node2 = node1;
                    while (node2->leftChild != nullptr && node2->nodeType == STATEMENT)
                    {
                        node2 = node2->leftChild;
                    }
                    node2->leftChild = node;
                    node->parent = node2;
                    node = node1;
                }
                else
                {
                    node = node1;
                }
            }
        }
    }

    return node;

}

static Node_t* GetF(Parser *parser)
{
    assert(parser != nullptr);

    Node_t *node1 = (Node_t*)calloc(1, sizeof(Node_t));
    Node_t *node2 = TreeInsert(parser->tree, node1, RIGHT_CHILD, DEFINE, NO_VALUE, (char*)"define");
    Node_t *node3 = TreeInsert(parser->tree, node2, LEFT_CHILD, FUNCTION, NO_VALUE, (char*)"function");
    if (parser->tokens[parser->curToken].keyword == MAIN)
    {
        TreeInsert(parser->tree, node3, LEFT_CHILD, MAIN, NO_VALUE, (char*)"main");
        parser->curToken++;
    }
    else
    {
        node3->leftChild = GetId(parser);
        IS_PARSER_ERROR();
        node3->leftChild->parent = node3;
        node3->leftChild->nodeType = FUNC;
    }
    parser->curToken++;
    node3->rightChild = GetPar(parser);
    IS_PARSER_ERROR();
    if (node3->rightChild != nullptr) {node3->rightChild->parent = node3; }
    Require(parser, RB);

    if (parser->tokens[parser->curToken].keyword == LSB)
    {
        parser->curToken++;
        node2->rightChild = GetA(parser);
        IS_PARSER_ERROR();
        if (node2->rightChild != nullptr) { node2->rightChild->parent = node2; }
        Require(parser, RSB);
    }

    SetNodeTypeValueStr(node1, STATEMENT, NO_VALUE, (char*)"statement");

    return node1;
}

static Node_t* GetPar(Parser *parser)
{
    assert(parser != nullptr);

    Node_t *node = nullptr;

    while (parser->tokens[parser->curToken].keyword != RB)
    {
        Node_t *node1 = (Node_t*)calloc(1, sizeof(Node_t));
        node1->rightChild = GetBool(parser);
        IS_PARSER_ERROR();
        node1->rightChild->parent = node1;
        SetNodeTypeValueStr(node1, PARAMETR, NO_VALUE, (char*)"parametr");
        if (parser->tokens[parser->curToken].keyword == COMMA)
        {
            parser->curToken++;
        }

        if (node1 != nullptr)
        {
            if (node != nullptr)
            {
                node1->leftChild = node;
                node->parent = node1;
                node = node1;
            }
            else
            {
                node = node1;
            }
        }
    }

    return node;
}

static Node_t* GetA(Parser *parser)
{
    assert(parser != nullptr);

    Node_t *node = nullptr;
    while (parser->tokens[parser->curToken].keyword != RSB)
    {
        if (parser->tokens[parser->curToken + 1].keyword == ASSIGN)
        {
            Node_t *node1 = (Node_t*)calloc(1, sizeof(Node_t));
            Node_t *node2 = TreeInsert(parser->tree, node1, RIGHT_CHILD, ASSIGN, NO_VALUE, (char*)"=");
            node2->leftChild = GetId(parser);
            IS_PARSER_ERROR();
            node2->leftChild->parent = node2;

            parser->curToken = parser->curToken + 1;

            node2->rightChild = GetBool(parser);
            IS_PARSER_ERROR();
            node2->rightChild->parent = node2;
            SetNodeTypeValueStr(node1, STATEMENT, NO_VALUE, (char*)"statement");
            Require(parser, SEMICOLON);

            if (node != nullptr)
            {
                node1->leftChild = node;
                node->parent = node1;
                node = node1;
            }
            else
            {
                node = node1;
            }
        }
        else if (strcmp(parser->tokens[parser->curToken].id, (char*)"\0") != 0 && parser->tokens[parser->curToken + 1].keyword == LAB)
        {
            Node_t *node1 = GetArray(parser);
            IS_PARSER_ERROR();

            if (node1 != nullptr)
            {
                if (node != nullptr)
                {
                    node1->leftChild = node;
                    node->parent = node1;
                    node = node1;
                }
                else
                {
                    node = node1;
                }
            }
        }
        else
        {
            Node_t *node1 = GetIf(parser);
            IS_PARSER_ERROR();

            if (node1 != nullptr)
            {
                if (node != nullptr)
                {
                    node1->leftChild = node;
                    node->parent = node1;
                    node = node1;
                }
                else
                {
                    node = node1;
                }
            }
        }
    }

    return node;
}

static Node_t* GetArray(Parser *parser)
{
    assert(parser != nullptr);

    Node_t *node1 = (Node_t*)calloc(1, sizeof(Node_t));
    Node_t *node2 = TreeInsert(parser->tree, node1, RIGHT_CHILD, ASSIGN, NO_VALUE, (char*)"=");
    node2->leftChild = GetId(parser);
    IS_PARSER_ERROR();
    node2->leftChild->parent = node2;
    parser->curToken = parser->curToken + 1;
    node2->leftChild->rightChild = GetBool(parser);
    IS_PARSER_ERROR();
    node2->leftChild->rightChild->parent = node2->leftChild;
    Require(parser, RAB);
    parser->curToken++;
    node2->rightChild = GetBool(parser);
    IS_PARSER_ERROR();
    node2->rightChild->parent = node2;
    Require(parser, SEMICOLON);
    SetNodeTypeValueStr(node1, STATEMENT, NO_VALUE, (char*)"statement");

    return node1;
}

static Node_t* GetIf(Parser *parser)
{
    assert(parser != nullptr);

    Node_t *node = nullptr;

    if (parser->tokens[parser->curToken].keyword == IF && parser->tokens[parser->curToken + 1].keyword == LB)
    {
        parser->curToken = parser->curToken + 2;

        Node_t *node1 = (Node_t*)calloc(1, sizeof(Node_t));
        Node_t *node2 = TreeInsert(parser->tree, node1, RIGHT_CHILD, IF, NO_VALUE, (char*)"if");
        node2->leftChild = GetBool(parser);
        IS_PARSER_ERROR();
        node2->leftChild->parent = node2;
        Require(parser, RB);

        Node_t *node3 = TreeInsert(parser->tree, node2, RIGHT_CHILD, DECISION, NO_VALUE, (char*)"decision");

        if (parser->tokens[parser->curToken].keyword == LSB)
        {
            parser->curToken++;

            node3->leftChild = GetA(parser);
            IS_PARSER_ERROR();
            node3->leftChild->parent = node3;
            Require(parser, RSB);
        }

        if (parser->tokens[parser->curToken].keyword == ELSE && parser->tokens[parser->curToken + 1].keyword == LSB)
        {
            parser->curToken = parser->curToken + 2;

            node3->rightChild = GetA(parser);
            IS_PARSER_ERROR();
            node3->rightChild->parent = node3;
            Require(parser, RSB);
        }

        SetNodeTypeValueStr(node1, STATEMENT, NO_VALUE, (char*)"statement");

        node = node1;
    }
    else
    {
        Node_t *node1 = GetWhile(parser);
        IS_PARSER_ERROR();

        if (node1 != nullptr)
        {
            if (node != nullptr)
            {
                node1->leftChild = node;
                node->parent = node1;
                node = node1;
            }
            else
            {
                node = node1;
            }
        }
    }

    return node;
}

static Node_t* GetWhile(Parser *parser)
{
    assert(parser != nullptr);

    Node_t *node = nullptr;

    if (parser->tokens[parser->curToken].keyword == WHILE && parser->tokens[parser->curToken + 1].keyword == LB)
    {
        parser->curToken = parser->curToken + 2;

        Node_t *node1 = (Node_t*)calloc(1, sizeof(Node_t));
        Node_t *node2 = TreeInsert(parser->tree, node1, RIGHT_CHILD, WHILE, NO_VALUE, (char*)"while");
        node2->leftChild = GetBool(parser);
        IS_PARSER_ERROR();
        node2->leftChild->parent = node2;
        Require(parser, RB);

        if (parser->tokens[parser->curToken].keyword == LSB)
        {
            parser->curToken++;

            node2->rightChild = GetA(parser);
            IS_PARSER_ERROR();
            node2->rightChild->parent = node2;
            Require(parser, RSB);
        }

        SetNodeTypeValueStr(node1, STATEMENT, NO_VALUE, (char*)"statement");

        node = node1;
    }
    else
    {
        Node_t *node1 = GetRet(parser);
        IS_PARSER_ERROR();

        if (node1 != nullptr)
        {
            if (node != nullptr)
            {
                node1->leftChild = node;
                node->parent = node1;
                node = node1;
            }
            else
            {
                node = node1;
            }
        }
    }

    return node;
}

static Node_t* GetRet(Parser *parser)
{
    assert(parser != nullptr);

    Node_t *node = nullptr;

    if (parser->tokens[parser->curToken].keyword == RETURN)
    {
        parser->curToken++;

        Node_t *node1 = (Node_t*)calloc(1, sizeof(Node_t));
        Node_t *node2 = TreeInsert(parser->tree, node1, RIGHT_CHILD, RETURN, NO_VALUE, (char*)"return");
        node2->rightChild = GetBool(parser);
        IS_PARSER_ERROR();
        if (node2->rightChild != nullptr) { node2->rightChild->parent = node2; }
        else                              { parser->curToken--;                }
        SetNodeTypeValueStr(node1, STATEMENT, NO_VALUE, (char*)"statement");
        Require(parser, SEMICOLON);

        node = node1;
    }
    else
    {
        Node_t *node1 = GetBool(parser);
        IS_PARSER_ERROR();

        node = node1;
    }

    return node;
}

static size_t IsBoolOperation(NodeType nodeType)
{
    for (size_t i = 0; i < NUMBER_BOOL_OPERATIONS; i++)
    {
        if (nodeType == boolOperation[i].nodeType) { return i + 1; }
    }

    return 0;
}

static Node_t* GetBool (Parser *parser)
{
    assert(parser != nullptr);

    Node_t *val = GetE(parser);
    IS_PARSER_ERROR();

    size_t numberBoolOperation = IsBoolOperation(parser->tokens[parser->curToken].keyword);
    while (numberBoolOperation)
    {
        NodeType op = parser->tokens[parser->curToken].keyword;
        parser->curToken++;
        Node_t *val2 = GetE(parser);
        IS_PARSER_ERROR();
        Node_t node = {nullptr, val, val2, op, NO_VALUE, boolOperation[numberBoolOperation - 1].str};
        val =  NodeCtor(&node);
        numberBoolOperation = IsBoolOperation(parser->tokens[parser->curToken].keyword);
    }

    return val;
}

static Node_t* GetE(Parser *parser)
{
    assert(parser != nullptr);

    Node_t *val = GetT(parser);
    IS_PARSER_ERROR();

    while (parser->tokens[parser->curToken].keyword == ADD || parser->tokens[parser->curToken].keyword == SUB)
    {
        NodeType op = parser->tokens[parser->curToken].keyword;
        parser->curToken++;
        Node_t *val2 = GetT(parser);
        IS_PARSER_ERROR();
        if (op == ADD)
        {
            Node_t node = {nullptr, val, val2, ADD, NO_VALUE, (char*)"+"};
            val =  NodeCtor(&node);
        }
        else
        {
            Node_t node = {nullptr, val, val2, SUB, NO_VALUE, (char*)"-"};
            val = NodeCtor(&node);
        }
    }

    return val;
}

static Node_t* GetT(Parser *parser)
{
    assert(parser != nullptr);

    Node_t *val = GetPow(parser);
    IS_PARSER_ERROR();

    while (parser->tokens[parser->curToken].keyword == MUL || parser->tokens[parser->curToken].keyword == DIV)
    {
        NodeType op = parser->tokens[parser->curToken].keyword;
        parser->curToken++;
        Node_t *val2 = GetPow(parser);
        IS_PARSER_ERROR();
        if (op == MUL)
        {
            Node_t node = {nullptr, val, val2, MUL, NO_VALUE, (char*)"*"};
            val =  NodeCtor(&node);
        }
        else
        {
            Node_t node = {nullptr, val, val2, DIV, NO_VALUE, (char*)"/"};
            val =  NodeCtor(&node);
        }
    }

    return val;
}

static Node_t* GetPow(Parser *parser)
{
    assert(parser != nullptr);

    Node_t* val = GetP(parser);

    while (parser->tokens[parser->curToken].keyword == POW)
    {
        parser->curToken++;
        Node_t *val2 = GetP(parser);
        IS_PARSER_ERROR();

        Node_t node = {nullptr, val, val2, POW, NO_VALUE, (char*)"^"};

        val =  NodeCtor(&node);
    }

    return val;
}

static Node_t* GetP(Parser *parser)
{
    assert(parser != nullptr);

    Node_t *val = nullptr;

    if (parser->tokens[parser->curToken].keyword == LB)
    {
        parser->curToken++;
        val = GetBool(parser);
        IS_PARSER_ERROR();
        Require(parser, RB);
    }
    else
    {
        size_t curToken = parser->curToken;
        val = GetUnary(parser);
        if (curToken == parser->curToken) { val = GetN(parser)    ; }
        if (curToken == parser->curToken) { val = GetCall(parser) ; }
        if (curToken == parser->curToken) { val = GetId(parser)   ; }
        IS_PARSER_ERROR();
    }
    return val;
}

static Node_t* GetUnary(Parser *parser)
{
    assert(parser != nullptr);

    for (int i = 0; i < NUMBER_UNARY_OPERATIONS; i++)
    {
        if (parser->tokens[parser->curToken].keyword == unaryOperation[i].nodeType)
        {
            parser->curToken++;
            Node_t *val = GetP(parser);
            IS_PARSER_ERROR();
            Node_t node = {nullptr, nullptr, val, unaryOperation[i].nodeType, NO_VALUE, unaryOperation[i].str};

            return NodeCtor(&node);
        }
    }

    return nullptr;
}

static Node_t* GetN(Parser *parser)
{
    assert(parser != nullptr);
    if (strcmp(parser->tokens[parser->curToken].id, "\0") == 0)
    {
        Node_t node = {nullptr, nullptr, nullptr, CONST, parser->tokens[parser->curToken].value, nullptr};

        parser->curToken++;

        return NodeCtor(&node);
    }
    else {return nullptr; }
}

static Node_t* GetCall (Parser *parser)
{
    assert(parser != nullptr);

    Node_t *node = nullptr;

    if (strcmp(parser->tokens[parser->curToken].id, (char*)"\0") != 0 && parser->tokens[parser->curToken + 1].keyword == LB)
    {
        Node_t *node1 = (Node_t*)calloc(1, sizeof(Node_t));
        Node_t *node2 = TreeInsert(parser->tree, node1, RIGHT_CHILD, CALL, NO_VALUE, (char*)"call");
        if (parser->tokens[parser->curToken].keyword != SCAN && parser->tokens[parser->curToken].keyword != PRINT
            && parser->tokens[parser->curToken].keyword != P_CDOT && parser->tokens[parser->curToken].keyword != P_SPACE
            && parser->tokens[parser->curToken].keyword != NEW_LINE && parser->tokens[parser->curToken].keyword != SQRT)
        {
            TreeInsert(parser->tree, node2, LEFT_CHILD, FUNC, NO_VALUE, parser->tokens[parser->curToken].id);
        }
        else
        {
            if (parser->tokens[parser->curToken].keyword == SCAN)          { TreeInsert(parser->tree, node2, LEFT_CHILD, SCAN     , NO_VALUE, (char*)"scan")    ; }
            else if (parser->tokens[parser->curToken].keyword == PRINT)    { TreeInsert(parser->tree, node2, LEFT_CHILD, PRINT    , NO_VALUE, (char*)"print")   ; }
            else if (parser->tokens[parser->curToken].keyword == P_CDOT)   { TreeInsert(parser->tree, node2, LEFT_CHILD, P_CDOT, NO_VALUE, (char*)"pCdot")      ; }
            else if (parser->tokens[parser->curToken].keyword == P_SPACE)  { TreeInsert(parser->tree, node2, LEFT_CHILD, P_SPACE, NO_VALUE, (char*)"pSpace")    ; }
            else if (parser->tokens[parser->curToken].keyword == NEW_LINE) { TreeInsert(parser->tree, node2, LEFT_CHILD, NEW_LINE , NO_VALUE, (char*)"newLine") ; }
            else                                                           { TreeInsert(parser->tree, node2, LEFT_CHILD, SQRT     , NO_VALUE, (char*)"sqrt")    ; }
        }
        parser->curToken = parser->curToken + 2;
        node2->rightChild = GetPar(parser);
        IS_PARSER_ERROR();
        if (node2->rightChild != nullptr) { node2->rightChild->parent = node2; }
        Require(parser, RB);
        SetNodeTypeValueStr(node1, STATEMENT, NO_VALUE, (char*)"statement");

        node = node1;
    }

    return node;
}

static Node_t* GetId(Parser *parser)
{
    assert(parser != nullptr);

    Node_t *node = nullptr;

    if (parser->tokens[parser->curToken].keyword != SEMICOLON)
    {
        Node_t node1 = {nullptr, nullptr, nullptr, VARIABLE, NO_VALUE, parser->tokens[parser->curToken].id};
        node = NodeCtor(&node1);

        parser->curToken++;
    }
    else
    {
        parser->curToken++;
    }

    return node;
}

#undef IS_PARSER_ERROR

Tree_t* SyntacticAnalysis(Parser *parser)
{
    assert(parser != nullptr);

    parser->tree->root = GetG(parser);

    return parser->tree;
}
