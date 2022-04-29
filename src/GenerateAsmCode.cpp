#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../include/GenerateAsmCode.h"

const char *NAME_OUTPUT_FILE          = "./res/asm.txt";
const int   NO_VARIABLE_IN_TABLE_NAME = -1;
const int   NO_FUNCTION_IN_TABLE_NAME = -1;

TableGlobalNames globalNames = { {}, 0 };
TableFunctions   functions   = {};

int  curLabel = 0;

static int GenerateLabel()
{
    return curLabel++;
}

static int NodeVisitorForSearchMain(Node_t *node)
{
    assert(node != nullptr);

    if (node->nodeType == MAIN) { return 1; }

    if (node->leftChild  != nullptr) { if (NodeVisitorForSearchMain(node->leftChild )) { return 1; } }
    if (node->rightChild != nullptr) { if (NodeVisitorForSearchMain(node->rightChild)) { return 1; } }

    return 0;
}

static int FindMain(Tree_t *tree)
{
    assert(tree != nullptr);

    return NodeVisitorForSearchMain(tree->root);
}

static void NodeVisitorForFindAndPrintGlobalVar(Tree_t *tree, Node_t *node, FILE *code)
{
    assert(tree != nullptr);
    assert(node != nullptr);
    assert(code != nullptr);

    Node_t *ptrNode = nullptr;

    if (node->leftChild != nullptr)
    {
        if (node->leftChild->nodeType == STATEMENT && node->leftChild->rightChild->nodeType == ASSIGN)
        {
            strcpy(globalNames.globalNames[globalNames.curName].str, node->leftChild->rightChild->leftChild->str);
            globalNames.globalNames[globalNames.curName].curOffset = globalNames.curName;
            if (node->leftChild->rightChild->rightChild->nodeType == CONST)
            {
                fprintf(code, "\n\tmov rax, %g\n"
                                "\tmov [rsi + %d * 8], rax\n\n", node->leftChild->rightChild->rightChild->value, globalNames.curName);
            }
            else
            {
                printf("The global variable is not assigned a constant!\n");
                return;
            }
            globalNames.curName++;
            if (node->leftChild->leftChild != nullptr)
            {
                ptrNode = node->leftChild;
                node->leftChild = ptrNode->leftChild;
                ptrNode->leftChild->parent = node;
                ptrNode->leftChild = nullptr;
                ptrNode->parent = nullptr;
                SubtreeDtor(ptrNode);
                node = node->leftChild;
                NodeVisitorForFindAndPrintGlobalVar(tree, node, code);
            }
            else
            {
                ptrNode = node->leftChild;
                node->leftChild = ptrNode->leftChild;
                ptrNode->parent = nullptr;
                SubtreeDtor(ptrNode);
            }
        }
        else
        {
            NodeVisitorForFindAndPrintGlobalVar(tree, node->leftChild, code);
        }
    }
    else
    {
        return ;
    }
}

//! Only a constant can be assigned to a global variable, otherwise it will lead to an error
static void FindAndPrintGlobalVar(Tree_t *tree, FILE *code)
{
    assert(tree != nullptr);
    assert(code != nullptr);

    Node_t *node = tree->root;

    while (node->nodeType == STATEMENT && node->rightChild->nodeType == ASSIGN)
    {
        if (globalNames.curName < NUMBERS_VARIABLE)
        {
            strcpy(globalNames.globalNames[globalNames.curName].str, node->rightChild->leftChild->str);
            globalNames.globalNames[globalNames.curName].curOffset = globalNames.curName;
            if (node->rightChild->rightChild->nodeType == CONST)
            {
                fprintf(code, "\n\tmov rax, %g\n"
                                "\tmov [rsi + %d * 8], rax\n\n", node->rightChild->rightChild->value, globalNames.curName);
            }
            else
            {
                printf("The global variable is not assigned a constant!\n");
                return;
            }
            globalNames.curName++;
            node = node->leftChild;
            if (node->leftChild == nullptr) { break; }
        }
        else
        {
            printf("Invalid number of global variables!\n");
            return;
        }
    }

    tree->root = node;
    tree->root->parent = nullptr;

    NodeVisitorForFindAndPrintGlobalVar(tree, tree->root, code);

    TreeDump(tree);
}

static int CheckTableGlobalNames(Node_t *node)
{
    assert(node != nullptr);

    int curOffset = NO_VARIABLE_IN_TABLE_NAME;

    for (int i = 0; i < NUMBERS_VARIABLE; i++)
    {
        if (strcmp(node->str, globalNames.globalNames[i].str) == 0)
        {
            curOffset = globalNames.globalNames[i].curOffset;
            return curOffset;
        }
    }

    return curOffset;
}

static int CheckTableLocalNames(Node_t *node, FILE *code, TableLocalNames *localNames)
{
    assert(node       != nullptr);
    assert(code       != nullptr);
    assert(localNames != nullptr);

    int curOffset = NO_VARIABLE_IN_TABLE_NAME;

    for (int i = 0; i < NUMBERS_VARIABLE; i++)
    {
        if (strcmp(node->str, localNames->localNames[i].str) == 0)
        {
            curOffset = localNames->localNames[i].curOffset;
            return curOffset;
        }
    }

    if (localNames->curName < NUMBERS_VARIABLE)
    {
        strcpy(localNames->localNames[localNames->curName].str, node->str);
        curOffset = localNames->curName;
        localNames->localNames[localNames->curName].curOffset = curOffset;
        localNames->curName++;
        fprintf(code, "\n\tinc r15\n\n");
    }
    else
    {
        printf("Invalid number of local variables!\n");
    }

    return curOffset;
}

static void FillTableFunctions(Node_t *node)
{
    assert(node != nullptr);

    if (node->rightChild->nodeType == DEFINE && node->rightChild->leftChild->leftChild->nodeType != MAIN)
    {
        strcpy(functions.functions[functions.curName].str, node->rightChild->leftChild->leftChild->str);
        functions.functions[functions.curName].curOffset = functions.curName;
        functions.curName++;
    }

    if (node->leftChild != nullptr) { FillTableFunctions(node->leftChild); }
}

static int CheckTableFunctions(Node_t *node)
{
    assert(node != nullptr);

    int curOffset = NO_FUNCTION_IN_TABLE_NAME;

    for (int i = 0; i < NUMBERS_FUNCTION; i++)
    {
        if (strcmp(node->str, functions.functions[i].str) == 0)
        {
            curOffset = functions.functions[i].curOffset;
            return curOffset;
        }
    }

    return curOffset;
}

static void ConvertSubtreeInCode            (Node_t *node, FILE *code, TableLocalNames *localNames);
static void ConvertDefineNodeInCode         (Node_t *node, FILE *code, TableLocalNames *localNames);
static void ConvertReturnNodeInCode         (Node_t *node, FILE *code, TableLocalNames *localNames);
static void ConvertCallNodeInCode           (Node_t *node, FILE *code, TableLocalNames *localNames);
static void ConvertIfNodeInCode             (Node_t *node, FILE *code, TableLocalNames *localNames);
static void ConvertWhileNodeInCode          (Node_t *node, FILE *code, TableLocalNames *localNames);
static void ConvertAssignNodeInCode         (Node_t *node, FILE *code, TableLocalNames *localNames);
static void ConvertConstNodeInCode          (Node_t *node, FILE *code, TableLocalNames *localNames);
static void ConvertLocVariableNodeIncode    (Node_t *node, FILE *code, TableLocalNames *localNames);
static void ConvertBinaryOperationNodeInCode(Node_t *node, FILE *code, TableLocalNames *localNames);

static void ConvertDefineNodeInCode(Node_t *node, FILE *code, TableLocalNames *localNames)
{
    assert(node != nullptr);
    assert(code != nullptr);

    TableLocalNames newLocalNames = {{}, 0};

    if (node->leftChild->leftChild->nodeType == MAIN)
    {
        ConvertSubtreeInCode(node->rightChild, code, &newLocalNames);
        memcpy(localNames, &newLocalNames, sizeof(TableLocalNames));
    }
    else
    {
        if (functions.curName < NUMBERS_FUNCTION)
        {

            node->leftChild->leftChild->str[strlen(node->leftChild->leftChild->str) - 1] = '\0';
            fprintf(code, "\n%s:\n\n"
                          "\n\tpop rcx\n"
                            "\txor r15, r15\n\n", node->leftChild->leftChild->str + 1);

            if (node->leftChild->rightChild != nullptr)
            {
                if (node->leftChild->rightChild->leftChild != nullptr)
                {
                    fprintf(code, "\n\tpop rax\n"
                                    "\tpop rbx\n\n");

                    Node_t *node1 = node->leftChild->rightChild;
                    fprintf(code, "\n\tmov [rsi + (r14 + %d + %d) * 8], rbx\n\n",
                                  globalNames.curName, CheckTableLocalNames(node1->rightChild, code, &newLocalNames));
                    while (node1->leftChild != nullptr)
                    {
                        node1 = node1->leftChild;
                        fprintf(code, "\n\tmov [rsi + (r14 + %d + %d) * 8], rax\n\n",
                                      globalNames.curName, CheckTableLocalNames(node1->rightChild, code, &newLocalNames));
                        //! ToDo other register (extensibility)
                    }
                }
                else
                {
                    fprintf(code, "\n\tpop rax\n\n");

                    Node_t *node1 = node->leftChild->rightChild;
                    fprintf(code, "\n\tmov [rsi + (r14 + %d + %d) * 8], rax\n\n",
                                  globalNames.curName, CheckTableLocalNames(node1->rightChild, code, &newLocalNames));
                }
            }

            fprintf(code, "\n\tpush rcx\n\n");

            ConvertSubtreeInCode(node->rightChild, code, &newLocalNames);
        }
        else
        {
            printf("Invalid number of local variables!\n");
            return;
        }
    }
}

static void ConvertReturnNodeInCode(Node_t *node, FILE *code, TableLocalNames *localNames)
{
    assert(node       != nullptr);
    assert(code       != nullptr);
    assert(localNames != nullptr);

    if (node->rightChild != nullptr)
    {
        ConvertSubtreeInCode(node->rightChild, code, localNames);
        fprintf(code, "\n\tpop rax\n" 
                        "\tret\n\n");
    }
    else
    {
        fprintf(code, "\n\tret\n\n");
    }
}

static void ConvertCallNodeInCode(Node_t *node, FILE *code, TableLocalNames *localNames)
{
    assert(node       != nullptr);
    assert(code       != nullptr);
    assert(localNames != nullptr);

    if (node->leftChild->nodeType == SCAN || node->leftChild->nodeType == PRINT
        || node->leftChild->nodeType == P_CDOT || node->leftChild->nodeType == P_SPACE
        || node->leftChild->nodeType == NEW_LINE || node->leftChild->nodeType == SQRT)
    {
        if (node->leftChild->nodeType == SCAN)
        {
            fprintf(code, "\n\tsub rsp, 24\n"
                            "\tmov qword [rsp + 8], 0\n"
                            "\tlea rsi, [rsp + 8]\n"
                            "\tmov rdi, SCAN\n"
                            "\tmov rax, 0\n"
                            "\tcall scanf\n"
                            "\tmov rsi, BUFFER\n"
                            "\tmov rax, qword [rsp + 8] \n"
                            "\tadd rsp, 24\n"
                            "\tpush rax\n\n");
        }
        else if (node->leftChild->nodeType == PRINT)    
        {
            fprintf(code, "\n\tmov rdi, PRINT\n\n");
            ConvertSubtreeInCode(node->rightChild, code, localNames);
            fprintf(code, "\n\tpop rax\n"
                            "\tmov rsi, rax\n"
                            "\tmov rax, 0\n"
                            "\tcall printf\n"
                            "\tmov rsi, BUFFER\n\n");
        }
        // else if (node->leftChild->nodeType == P_CDOT)   { fprintf(code, "P_CDOT\n")  ; }
        // else if (node->leftChild->nodeType == P_SPACE)  { fprintf(code, "P_SPACE\n") ; }
        // else if (node->leftChild->nodeType == NEW_LINE) { fprintf(code, "NEW_LINE\n"); }
        else
        {
            ConvertSubtreeInCode(node->rightChild, code, localNames);
            fprintf(code, "\n\tpop rax\n"
                            "\tcvtsi2sd xmm0, eax\n"
                            "\tcall sqrt\n"
                            "\tcvttsd2si eax, xmm0\n"
                            "\tpush rax\n\n");
        }
    }
    else
    {
        int isFunctionInTableFunctions = CheckTableFunctions(node->leftChild);

        if (isFunctionInTableFunctions != NO_FUNCTION_IN_TABLE_NAME)
        {
            fprintf(code, "\n\tpush r14\n"
                            "\tpush r15\n\n");

            if (node->rightChild != nullptr)
            {

                Node_t *node1 = node->rightChild;
                ConvertSubtreeInCode(node1->rightChild, code, localNames);
                while (node1->leftChild != nullptr)
                {
                    node1 = node1->leftChild;
                    ConvertSubtreeInCode(node1->rightChild, code, localNames);
                    //! ToDo other register (extensibility)
                }
            }
            node->leftChild->str[strlen(node->leftChild->str) - 1] = '\0';
            fprintf(code, "\n\tadd r14, r15\n"
                            "\tcall %s\n"
                            "\tpop r15\n"
                            "\tpop r14\n"
                            "\tpush rax\n\n", node->leftChild->str + 1);

        }
        else
        {
            printf("Invalid function call!\n");
            return;
        }
    }
}

static void ConvertIfNodeInCode(Node_t *node, FILE *code, TableLocalNames *localNames)
{
    assert(node       != nullptr);
    assert(code       != nullptr);
    assert(localNames != nullptr);

    size_t label = 0;

    ConvertSubtreeInCode(node->leftChild, code, localNames);
    label = GenerateLabel();
    if (node->rightChild->rightChild != nullptr) { ConvertSubtreeInCode(node->rightChild->rightChild, code, localNames); }
    fprintf(code, "\n\tjmp .next%zu\n\n", label);
    fprintf(code, "\n.next%zu:\n\n", label - 1);
    ConvertSubtreeInCode(node->rightChild->leftChild, code, localNames);
    fprintf(code, "\n.next%zu:\n\n", label);
}

static void ConvertWhileNodeInCode(Node_t *node, FILE *code, TableLocalNames *localNames)
{
    assert(node       != nullptr);
    assert(code       != nullptr);
    assert(localNames != nullptr);

    size_t label = GenerateLabel();

    fprintf(code, "\n.next%zu:\n\n", label);
    ConvertSubtreeInCode(node->leftChild, code, localNames);
    label = GenerateLabel();
    fprintf(code, "\n\tjmp .next%zu\n", label);
    fprintf(code, "\n.next%zu:\n\n", label - 1);
    ConvertSubtreeInCode(node->rightChild, code, localNames);
    fprintf(code, "\n\tjmp .next%zu\n", label - 2);
    fprintf(code, "\n.next%zu:\n\n", label);
}

static void ConvertAssignNodeInCode(Node_t *node, FILE *code, TableLocalNames *localNames)
{
    assert(node       != nullptr);
    assert(code       != nullptr);
    assert(localNames != nullptr);

    int curOffset = CheckTableGlobalNames(node->leftChild);
    if (curOffset == NO_VARIABLE_IN_TABLE_NAME)
    {
        ConvertSubtreeInCode(node->rightChild, code, localNames);
        curOffset = CheckTableLocalNames(node->leftChild, code, localNames);
        fprintf(code, "\n\tpop rax\n"
                        "\tmov [rsi + (r14 + %d + %d) * 8], rax\n\n", globalNames.curName, curOffset);
    }
    else
    {
        ConvertSubtreeInCode(node->rightChild, code, localNames);
        fprintf(code, "\tpop rax\n"
                      "\tmov [rsi + %d * 8], rax\n\n", curOffset);
    }
}

static void ConvertLocVariableNodeIncode(Node_t *node, FILE *code, TableLocalNames *localNames)
{
    assert(node       != nullptr);
    assert(code       != nullptr);
    assert(localNames != nullptr);

    int  curOffset = CheckTableGlobalNames(node);
    if (curOffset == NO_VARIABLE_IN_TABLE_NAME)
    {
        curOffset = CheckTableLocalNames(node, code, localNames);
        fprintf(code, "\n\tmov rax, [rsi + (r14 + %d + %d) * 8]\n"
                        "\tpush rax\n\n", globalNames.curName, curOffset);
    }
    else
    {
        fprintf(code, "\n\tmov rax, [rsi + %d * 8]\n\n"
                        "\tpush rax\n\n", curOffset);
    }
}

static void ConvertConstNodeInCode(Node_t *node, FILE *code, TableLocalNames *localNames)
{
    assert(node       != nullptr);
    assert(code       != nullptr);
    assert(localNames != nullptr);

    fprintf(code, "\n\tmov rax, %g\n"
                    "\tpush rax\n\n", node->value);
}

static void ConvertBinaryOperationNodeInCode(Node_t *node, FILE *code, TableLocalNames *localNames)
{
    assert(node       != nullptr);
    assert(code       != nullptr);
    assert(localNames != nullptr);

    ConvertSubtreeInCode(node->leftChild , code, localNames);
    ConvertSubtreeInCode(node->rightChild, code, localNames);
    switch ((int)node->nodeType)
    {
        case (int)ADD:
        {
            fprintf(code, "\n\tpop rbx\n"
                            "\tpop rax\n"
                            "\tadd rax, rbx\n"
                            "\tpush rax\n\n");
            break;
        }
        case (int)SUB:
        {
            fprintf(code, "\n\tpop rbx\n"
                            "\tpop rax\n"
                            "\tsub rax, rbx\n"
                            "\tpush rax\n\n");
            break;
        }
        case (int)MUL:
        {
            fprintf(code, "\n\tpop rbx\n"
                            "\tpop rax\n"
                            "\tmul ebx\n"
                            "\tpush rax\n\n");
            break;
        }
        case (int)DIV:
        {
            fprintf(code, "\n\tpop rbx\n"
                            "\tpop rax\n"
                            "\tcdq\n"
                            "\tidiv ebx\n"
                            "\tpush rax\n\n");
            break;
        }
        case (int)POW:
        {
            fprintf(code, "\n\tpop rbx\n"
                            "\tpop rax\n"
                            "\tcvtsi2sd xmm0, eax\n"
                            "\tcvtsi2sd xmm1, ebx\n"
                            "\tcall pow\n"
                            "\tmov rsi, BUFFER\n"
                            "\tcvttsd2si eax, xmm0\n"
                            "\tpush rax\n\n");
            break;
        }
        case (int)JA:
        {
            fprintf(code, "\n\tpop rbx\n"
                            "\tpop rax\n"
                            "\tcmp rax, rbx\n"
                            "\tjg .next%d\n\n" , GenerateLabel());
            break;
        }
        case (int)JB:
        {
            fprintf(code, "\n\tpop rbx\n"
                            "\tpop rax\n"
                            "\tcmp rax, rbx\n"
                            "\tjl .next%d\n\n" , GenerateLabel());
            break;
        }
        case (int)JE:
        { 
            fprintf(code, "\n\tpop rbx\n"
                            "\tpop rax\n"
                            "\tcmp rax, rbx\n"
                            "\tje .next%d\n\n" , GenerateLabel());
            break;
        }
        case (int)JAE:
        {
            fprintf(code, "\n\tpop rbx\n"
                            "\tpop rax\n"
                            "\tcmp rax, rbx\n"
                            "\tjge .next%d\n\n" , GenerateLabel());
            break;
        }
        case (int)JBE:
        {
            fprintf(code, "\n\tpop rbx\n"
                            "\tpop rax\n"
                            "\tcmp rax, rbx\n"
                            "\tjle .next%d\n\n" , GenerateLabel());
            break;
        }
        case (int)JNE:
        {
            fprintf(code, "\n\tpop rbx\n"
                            "\tpop rax\n"
                            "\tcmp rax, rbx\n"
                            "\tjne .next%d\n\n" , GenerateLabel());
            break;
        }
        default:
        {
            printf("Invalid binary operations!\n");
            break;
        }
    }
}

static void ConvertSubtreeInCode(Node_t *node, FILE *code, TableLocalNames *localNames)
{
    assert(node       != nullptr);
    assert(code       != nullptr);
    assert(localNames != nullptr);

    switch ((int)node->nodeType)
    {
        case (int)DEFINE   : { ConvertDefineNodeInCode         (node, code, localNames); return; }
        case (int)RETURN   : { ConvertReturnNodeInCode         (node, code, localNames); return; }
        case (int)CALL     : { ConvertCallNodeInCode           (node, code, localNames); return; }
        case (int)IF       : { ConvertIfNodeInCode             (node, code, localNames); return; }
        case (int)WHILE    : { ConvertWhileNodeInCode          (node, code, localNames); return; }
        case (int)ASSIGN   : { ConvertAssignNodeInCode         (node, code, localNames); return; }
        case (int)CONST    : { ConvertConstNodeInCode          (node, code, localNames); return; }
        case (int)VARIABLE : { ConvertLocVariableNodeIncode    (node, code, localNames); return; }
        case (int)ADD      : { ConvertBinaryOperationNodeInCode(node, code, localNames); return; }
        case (int)SUB      : { ConvertBinaryOperationNodeInCode(node, code, localNames); return; }
        case (int)MUL      : { ConvertBinaryOperationNodeInCode(node, code, localNames); return; }
        case (int)DIV      : { ConvertBinaryOperationNodeInCode(node, code, localNames); return; }
        case (int)POW      : { ConvertBinaryOperationNodeInCode(node, code, localNames); return; }
        case (int)JA       : { ConvertBinaryOperationNodeInCode(node, code, localNames); return; }
        case (int)JB       : { ConvertBinaryOperationNodeInCode(node, code, localNames); return; }
        case (int)JE       : { ConvertBinaryOperationNodeInCode(node, code, localNames); return; }
        case (int)JAE      : { ConvertBinaryOperationNodeInCode(node, code, localNames); return; }
        case (int)JBE      : { ConvertBinaryOperationNodeInCode(node, code, localNames); return; }
        case (int)JNE      : { ConvertBinaryOperationNodeInCode(node, code, localNames); return; }
        default            : { break; }
    }

    if (node->leftChild  != nullptr) { ConvertSubtreeInCode(node->leftChild , code, localNames); }
    if (node->rightChild != nullptr) { ConvertSubtreeInCode(node->rightChild, code, localNames); }
}

void GenerateAsmCode(Tree_t *tree)
{
    assert(tree != nullptr);

    FILE *code = fopen(NAME_OUTPUT_FILE, "w");

    if (!FindMain(tree))
    {
        printf("No main declaration!\n");
        return;
    }

    fprintf(code, "extern scanf\n"
                  "extern printf\n"
                  "extern pow\n"
                  "extern sqrt\n\n"
                  "\nsection .data\n\n"
                  "\nBUFFER: times 100 dq 0\n"
                  "PRINT: db \"%%d\", 10, 0\n"
                  "SCAN:  db \"%%d\"\n\n"
                  "\nsection .text\n\n"
                  "\nglobal main\n\n"
                  "\nmain:\n\n"
                  "\n\tmov rsi, BUFFER\n"
                    "\txor r14, r14\n"
                    "\txor r15, r15\n\n");

    FillTableFunctions   (tree->root);
    FindAndPrintGlobalVar(tree, code);

    TableLocalNames localNames = {{}, 0};

    ConvertSubtreeInCode(tree->root, code, &localNames);

    fclose(code);
}
