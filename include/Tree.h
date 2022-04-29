#ifndef TREE_H_
#define TREE_H_

#include <fstream>

typedef char elem_t;

const size_t STR_MAX_SIZE = 400;

enum NodeChild
{
    RIGHT_CHILD,
    LEFT_CHILD
};

enum TreeStatus
{
    TREE_NOT_CONSTRUCTED,
    TREE_CONSTRUCTED,
    TREE_DESTRUCTED
};

enum TreeErrorCode
{
    TREE_NO_ERROR,
    TREE_CONSTRUCTED_ERROR,
    TREE_INSERT_ERROR,
    TREE_DESTRUCTED_ERROR,
    TREE_FILL_ERROR,
    TREE_OBJECT_DEFINITION_MODE_ERROR
};

enum NodeType
{
    CONST     , // 0
    VARIABLE  , // 1
    FUNC      , // 2
    STATEMENT , // 3
    DEFINE    , // 4
    FUNCTION  , // 5
    PARAMETR  , // 6
    CALL      , // 7
    MAIN      , // 8
    SCAN      , // 9
    PRINT     , // 10
    P_CDOT    , // 11
    P_SPACE   , // 12
    NEW_LINE  , // 13
    SQRT      , // 14
    RETURN    , // 15
    IF        , // 16
    ELSE      , // 17
    WHILE     , // 18
    DECISION  , // 19
    ASSIGN    , // 20
    LSB       , // 21
    RSB       , // 22
    LB        , // 23
    RB        , // 24
    LAB       , // 25
    RAB       , // 26
    COMMA     , // 27
    SEMICOLON , // 28
    ADD       , // 29
    SUB       , // 30
    MUL       , // 31
    DIV       , // 32
    POW       , // 33
    JA        , // 34
    JB        , // 35
    JE        , // 36
    JAE       , // 37
    JBE       , // 38
    JNE         // 39
};

struct Node_t
{
    Node_t *parent;
    Node_t *leftChild;
    Node_t *rightChild;
    NodeType nodeType;
    double value;
    char *str;
};

struct Tree_t
{
    Node_t *root;
    size_t size;
    TreeStatus status;
};

void TreeDump(Tree_t *tree);

TreeErrorCode TreeCtor(Tree_t *tree);

Node_t* TreeInsert(Tree_t *tree, Node_t *node, const NodeChild child, const NodeType tNodeType, const double tValue, const char *tStr);

void SubtreeDtor(Node_t *node);

TreeErrorCode TreeDtor(Tree_t *tree);

void SetNodeTypeValueStr(Node_t *node, const NodeType nodeType, const double value, char *str);

#endif // TREE_H_
