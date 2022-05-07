#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <cstddef>
#include "../include/Tree.h"
#include "../include/FileOperations.h"

#define DEBUG

const size_t NODEVIEW_STR_MAX_SIZE = 20;
const int    NOT_EQUAL             = 0;
const int    EQUAL                 = 1;
const char  *TREE_GRAPHVIZ         = "res/graphviz.gv";

struct NodeView
{
    char shape[NODEVIEW_STR_MAX_SIZE];
    char color[NODEVIEW_STR_MAX_SIZE];
    char str[NODEVIEW_STR_MAX_SIZE];
};

static void NodeViewBuild(const Node_t *node, NodeView *nodeView)
{
    assert(node     != nullptr);
    assert(nodeView != nullptr);

    #define BUILD_NODEVIEW_(thisShape, thisColor)                                \
        if (node->str != nullptr) { sprintf(nodeView->str, "%s", node->str)  ; } \
        else                      { sprintf(nodeView->str, "%g", node->value); } \
        strcpy(nodeView->shape, thisShape);                                      \
        strcpy(nodeView->color, thisColor);                                      \
        break

    switch ((int)node->nodeType)
    {
        case (int)CONST    : { BUILD_NODEVIEW_("circle"       , "yellow" ); }
        case (int)VARIABLE : { BUILD_NODEVIEW_("rectangle"    , "cyan"   ); }
        case (int)FUNC     : { BUILD_NODEVIEW_("octagon"      , "cyan"   ); }
        case (int)STATEMENT: { BUILD_NODEVIEW_("parallelogram", "grey"   ); }
        case (int)DEFINE   : { BUILD_NODEVIEW_("parallelogram", "grey"   ); }
        case (int)FUNCTION : { BUILD_NODEVIEW_("parallelogram", "grey"   ); }
        case (int)PARAMETR : { BUILD_NODEVIEW_("parallelogram", "grey"   ); }
        case (int)CALL     : { BUILD_NODEVIEW_("parallelogram", "grey"   ); }
        case (int)MAIN     : { BUILD_NODEVIEW_("octagon"      , "cyan"   ); }
        case (int)SCAN     : { BUILD_NODEVIEW_("octagon"      , "cyan"   ); }
        case (int)PRINT    : { BUILD_NODEVIEW_("octagon"      , "cyan"   ); }
        case (int)P_CDOT   : { BUILD_NODEVIEW_("octagon"      , "cyan"   ); }
        case (int)P_SPACE  : { BUILD_NODEVIEW_("octagon"      , "cyan"   ); }
        case (int)NEW_LINE : { BUILD_NODEVIEW_("octagon"      , "cyan"   ); }
        case (int)SQRT     : { BUILD_NODEVIEW_("octagon"      , "cyan"   ); }
        case (int)RETURN   : { BUILD_NODEVIEW_("parallelogram", "grey"   ); }
        case (int)IF       : { BUILD_NODEVIEW_("parallelogram", "grey"   ); }
        case (int)WHILE    : { BUILD_NODEVIEW_("parallelogram", "grey"   ); }
        case (int)DECISION : { BUILD_NODEVIEW_("parallelogram", "grey"   ); }
        case (int)ASSIGN   : { BUILD_NODEVIEW_("diamond"      , "red"    ); }
        case (int)ADD      : { BUILD_NODEVIEW_("diamond"      , "red"    ); }
        case (int)SUB      : { BUILD_NODEVIEW_("diamond"      , "red"    ); }
        case (int)MUL      : { BUILD_NODEVIEW_("diamond"      , "red"    ); }
        case (int)DIV      : { BUILD_NODEVIEW_("diamond"      , "red"    ); }
        case (int)POW      : { BUILD_NODEVIEW_("diamond"      , "red"    ); }
        case (int)JG       : { BUILD_NODEVIEW_("diamond"      , "red"    ); }
        case (int)JL       : { BUILD_NODEVIEW_("diamond"      , "red"    ); }
        case (int)JE       : { BUILD_NODEVIEW_("diamond"      , "red"    ); }
        case (int)JGE      : { BUILD_NODEVIEW_("diamond"      , "red"    ); }
        case (int)JLE      : { BUILD_NODEVIEW_("diamond"      , "red"    ); }
        case (int)JNE      : { BUILD_NODEVIEW_("diamond"      , "red"    ); }
        default            : { BUILD_NODEVIEW_("rectangle"    , "green"  ); }
    }

    #undef BUILD_NODEVIEW_
}

static void TreeVisitPrintNodeInFile(const Node_t *node, FILE *foutput)
{
    assert(node    != nullptr);
    assert(foutput != nullptr);

    NodeView nodeView = {};
    NodeViewBuild(node, &nodeView);

    char str[STR_MAX_SIZE] = {};
    sprintf(str, "\t%lu[shape=record, shape=%s, style=\"filled\", fillcolor=%s, label=\"%s\"];\n",
                 (long unsigned int)node, nodeView.shape, nodeView.color, nodeView.str);
    fprintf(foutput, "%s", str);

    if (node->leftChild  != nullptr) TreeVisitPrintNodeInFile(node->leftChild, foutput);
    if (node->rightChild != nullptr) TreeVisitPrintNodeInFile(node->rightChild, foutput);
}

static void TreeVisitPrintArrowInFile(const Node_t *node, FILE *foutput)
{
    assert(node    != nullptr);
    assert(foutput != nullptr);

    if (node->parent != nullptr)
        fprintf(foutput, "\t%lu -> %lu[label = \"P\", color=red, fontsize=12]\n", (long unsigned int)node, (long unsigned int)node->parent);

    if (node->leftChild  != nullptr)
        fprintf(foutput, "\t%lu -> %lu[label = \"L\", fontsize=12]\n", (long unsigned int)node, (long unsigned int)node->leftChild);

    if (node->rightChild != nullptr)
        fprintf(foutput, "\t%lu -> %lu[label = \"R\", fontsize=12]\n", (long unsigned int)node, (long unsigned int)node->rightChild);

    if (node->leftChild  != nullptr)
        TreeVisitPrintArrowInFile(node->leftChild, foutput);
    if (node->rightChild != nullptr)
        TreeVisitPrintArrowInFile(node->rightChild, foutput);
}

void TreeDump(Tree_t *tree)
{
    FILE *graphViz = fopen(TREE_GRAPHVIZ, "w");

    fprintf(graphViz, "digraph Tree{\n\n");
    fprintf(graphViz, "\trankdir=UD;\n\n");
    fprintf(graphViz, "\tnode[fontsize=14];\n\n");

    TreeVisitPrintNodeInFile(tree->root, graphViz);

    fprintf(graphViz, "\n");

    TreeVisitPrintArrowInFile(tree->root, graphViz);

    fprintf(graphViz, "\n");

    fprintf(graphViz, "}");

    fclose(graphViz);

    system("dot -Tpng res/graphviz.gv -o res/graphviz.png");
}

TreeErrorCode TreeCtor(Tree_t *tree)
{
    assert(tree != nullptr);

    TreeErrorCode treeError = TREE_NO_ERROR;

    if (tree->status != TREE_NOT_CONSTRUCTED)
    {
        treeError = TREE_CONSTRUCTED_ERROR;
    }

    tree->root = (Node_t*)calloc(1, sizeof(Node_t));
    if (tree->root == nullptr)
    {
        treeError = TREE_CONSTRUCTED_ERROR;
    }

    tree->size = 1;
    tree->status = TREE_CONSTRUCTED;

    return treeError;
}

Node_t* TreeInsert(Tree_t *tree, Node_t *node, const NodeChild child, const NodeType thisNodeType, const double thisValue, const char *thisStr)
{
    assert(tree    != nullptr);
    assert(node    != nullptr);
    assert(thisStr != nullptr);

    Node_t *newNode = nullptr;

    #define TREE_INSERT_(branch)                                           \
        do                                                                 \
        {                                                                  \
        node->branch = (Node_t*)calloc(1, sizeof(Node_t));                 \
        node->branch->parent = node;                                       \
        node->branch->nodeType = thisNodeType;                             \
        node->branch->value = thisValue;                                   \
        node->branch->str = (elem_t*)calloc(STR_MAX_SIZE, sizeof(elem_t)); \
        strcpy(node->branch->str, thisStr);                                \
        newNode = node->branch;                                            \
        } while(0)

    if (child == 1)
    {
        TREE_INSERT_(leftChild);
    }
    else
    {
        TREE_INSERT_(rightChild);
    }

    #undef TREE_INSERT_

    tree->size = tree->size + 1;

    return newNode;
}

void SubtreeDtor(Node_t *node)
{
    assert(node != nullptr);

    Node_t *leftChild  = node->leftChild;
    Node_t *rightChild = node->rightChild;

    if (node->str != nullptr) { free(node->str); }
    free(node);

    if (leftChild  != nullptr) SubtreeDtor(leftChild);
    if (rightChild != nullptr) SubtreeDtor(rightChild);
}

TreeErrorCode TreeDtor(Tree_t *tree)
{
    assert(tree != nullptr);

    if (tree->status == TREE_DESTRUCTED)
    {
        return TREE_DESTRUCTED_ERROR;
    }

    SubtreeDtor(tree->root);
    tree->size = 0;
    tree->size = TREE_DESTRUCTED;

    return TREE_NO_ERROR;
}

void SetNodeTypeValueStr(Node_t *node, const NodeType nodeType, const double value, char *str)
{
    assert(node != nullptr);
    assert(str  != nullptr);

    node->nodeType = nodeType;
    node->value    = value;
    node->str = (char*)calloc(STR_MAX_SIZE, sizeof(char));
    strcpy(node->str, str);
}
