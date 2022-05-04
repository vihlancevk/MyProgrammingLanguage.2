#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../include/GenerateAsmCode.h"

const char     *NAME_OUTPUT_FILE          = "./res/prog";
const u_int32_t START_POSITION_IN_FILE    = 4448;
const u_int16_t START_ADDRESS_PRINTF      = 0xfedc;
const u_int16_t START_ADDRESS_SCANF       = 0xfeec;
const u_int16_t START_ADDRESS_POW         = 0xfecc;
const u_int16_t START_ADDRESS_SQRT        = 0xfefc;
const int32_t   NO_VARIABLE_IN_TABLE_NAME = -1;
const int32_t   NO_FUNCTION_IN_TABLE_NAME = -1;

TableGlobalNames globalNames = { {}, 0 };
TableFunctions   functions   = {};

u_int32_t curLabel       = 0;
u_int16_t numBytesInFile = 0;

static int32_t GenerateLabel()
{
    return curLabel++;
}

static int NodeVisitorForSearchMain( Node_t *node )
{
    assert( node != nullptr );

    if ( node->nodeType == MAIN ) { return 1; }

    if ( node->leftChild  != nullptr ) { if ( NodeVisitorForSearchMain( node->leftChild  )) { return 1; } }
    if ( node->rightChild != nullptr ) { if ( NodeVisitorForSearchMain( node->rightChild )) { return 1; } }

    return 0;
}

static int32_t FindMain( Tree_t *tree )
{
    assert( tree != nullptr );

    return NodeVisitorForSearchMain( tree->root );
}

static void NodeVisitorForFindAndPrintGlobalVar( Tree_t *tree, Node_t *node, FILE *code )
{
    assert( tree != nullptr );
    assert( node != nullptr );
    assert( code != nullptr );

    Node_t *ptrNode = nullptr;

    if ( node->leftChild != nullptr )
    {
        if ( node->leftChild->nodeType == STATEMENT && node->leftChild->rightChild->nodeType == ASSIGN )
        {
            strcpy( globalNames.globalNames[globalNames.curName].str, node->leftChild->rightChild->leftChild->str );
            globalNames.globalNames[globalNames.curName].curOffset = globalNames.curName;
            if ( node->leftChild->rightChild->rightChild->nodeType == CONST )
            {
                fprintf( code, "\n\tmov rax, %g\n"
                                "\tmov [rsi + %d * 8], rax\n\n", node->leftChild->rightChild->rightChild->value, globalNames.curName );
            }
            else
            {
                printf( "The global variable is not assigned a constant!\n" );
                return;
            }
            globalNames.curName++;
            if ( node->leftChild->leftChild != nullptr )
            {
                ptrNode = node->leftChild;
                node->leftChild = ptrNode->leftChild;
                ptrNode->leftChild->parent = node;
                ptrNode->leftChild = nullptr;
                ptrNode->parent = nullptr;
                SubtreeDtor( ptrNode );
                node = node->leftChild;
                NodeVisitorForFindAndPrintGlobalVar( tree, node, code );
            }
            else
            {
                ptrNode = node->leftChild;
                node->leftChild = ptrNode->leftChild;
                ptrNode->parent = nullptr;
                SubtreeDtor( ptrNode );
            }
        }
        else
        {
            NodeVisitorForFindAndPrintGlobalVar( tree, node->leftChild, code );
        }
    }
    else
    {
        return ;
    }
}

//! Only a constant can be assigned to a global variable, otherwise it will lead to an error
static void FindAndPrintGlobalVar( Tree_t *tree, FILE *code )
{
    assert( tree != nullptr );
    assert( code != nullptr );

    Node_t *node = tree->root;

    while ( node->nodeType == STATEMENT && node->rightChild->nodeType == ASSIGN )
    {
        if ( globalNames.curName < NUMBERS_VARIABLE )
        {
            strcpy( globalNames.globalNames[globalNames.curName].str, node->rightChild->leftChild->str );
            globalNames.globalNames[globalNames.curName].curOffset = globalNames.curName;
            if ( node->rightChild->rightChild->nodeType == CONST )
            {
                u_int8_t bufNumOper = 0xb8;
                fwrite( &bufNumOper, sizeof( u_int8_t ), 1, code );
                numBytesInFile += 1;

                int32_t regVal = (int32_t)node->rightChild->rightChild->value;
                fwrite( &regVal, sizeof( int32_t ), 1, code );
                numBytesInFile += 4;

                u_int8_t bufNumOpers[3] = { 0x48, 0x89, 0x86 };
                fwrite( bufNumOpers, sizeof( u_int8_t ), 3, code );
                numBytesInFile += 3;

                int32_t constVal = (int32_t)( globalNames.curName * 8 );
                fwrite( &constVal, sizeof( int32_t ), 1, code );
                numBytesInFile += 4;
            }
            else
            {
                printf( "The global variable is not assigned a constant!\n" );
                return;
            }
            globalNames.curName++;
            node = node->leftChild;
            if ( node->leftChild == nullptr ) { break; }
        }
        else
        {
            printf( "Invalid number of global variables!\n" );
            return;
        }
    }

    tree->root = node;
    tree->root->parent = nullptr;

    NodeVisitorForFindAndPrintGlobalVar( tree, tree->root, code );

    TreeDump( tree );
}

static int CheckTableGlobalNames( Node_t *node )
{
    assert( node != nullptr );

    int32_t curOffset = NO_VARIABLE_IN_TABLE_NAME;

    for ( int32_t i = 0; i < NUMBERS_VARIABLE; i++ )
    {
        if ( strcmp( node->str, globalNames.globalNames[i].str ) == 0 )
        {
            curOffset = globalNames.globalNames[i].curOffset;
            return curOffset;
        }
    }

    return curOffset;
}

static int CheckTableLocalNames( Node_t *node, FILE *code, TableLocalNames *localNames )
{
    assert( node       != nullptr );
    assert( code       != nullptr );
    assert( localNames != nullptr );

    int curOffset = NO_VARIABLE_IN_TABLE_NAME;

    for ( int32_t i = 0; i < NUMBERS_VARIABLE; i++ )
    {
        if ( strcmp( node->str, localNames->localNames[i].str ) == 0 )
        {
            curOffset = localNames->localNames[i].curOffset;
            return curOffset;
        }
    }

    if ( localNames->curName < NUMBERS_VARIABLE )
    {
        strcpy( localNames->localNames[localNames->curName].str, node->str );
        curOffset = localNames->curName;
        localNames->localNames[localNames->curName].curOffset = curOffset;
        localNames->curName++;
        
        u_int8_t bufNumOpers[3] = { 0x49, 0xff, 0xc7 };
        fwrite( bufNumOpers, sizeof( u_int8_t ), 3, code );
        numBytesInFile += 3;
    }
    else
    {
        printf( "Invalid number of local variables!\n" );
    }

    return curOffset;
}

static void FillTableFunctions( Node_t *node )
{
    assert( node != nullptr );

    if ( node->rightChild->nodeType == DEFINE && node->rightChild->leftChild->leftChild->nodeType != MAIN )
    {
        strcpy( functions.functions[functions.curName].str, node->rightChild->leftChild->leftChild->str );
        functions.functions[functions.curName].curOffset = functions.curName;
        functions.curName++;
    }

    if ( node->leftChild != nullptr ) { FillTableFunctions(node->leftChild); }
}

static int CheckTableFunctions( Node_t *node )
{
    assert( node != nullptr );

    int32_t curOffset = NO_FUNCTION_IN_TABLE_NAME;

    for ( int32_t i = 0; i < NUMBERS_FUNCTION; i++ )
    {
        if ( strcmp(node->str, functions.functions[i].str) == 0 )
        {
            curOffset = functions.functions[i].curOffset;
            return curOffset;
        }
    }

    return curOffset;
}

static void ConvertSubtreeInCode            ( Node_t *node, FILE *code, TableLocalNames *localNames );
static void ConvertDefineNodeInCode         ( Node_t *node, FILE *code, TableLocalNames *localNames );
static void ConvertReturnNodeInCode         ( Node_t *node, FILE *code, TableLocalNames *localNames );
static void ConvertCallNodeInCode           ( Node_t *node, FILE *code, TableLocalNames *localNames );
static void ConvertIfNodeInCode             ( Node_t *node, FILE *code, TableLocalNames *localNames );
static void ConvertWhileNodeInCode          ( Node_t *node, FILE *code, TableLocalNames *localNames );
static void ConvertAssignNodeInCode         ( Node_t *node, FILE *code, TableLocalNames *localNames );
static void ConvertConstNodeInCode          ( Node_t *node, FILE *code, TableLocalNames *localNames );
static void ConvertLocVariableNodeIncode    ( Node_t *node, FILE *code, TableLocalNames *localNames );
static void ConvertBinaryOperationNodeInCode( Node_t *node, FILE *code, TableLocalNames *localNames );

static void ConvertDefineNodeInCode( Node_t *node, FILE *code, TableLocalNames *localNames )
{
    assert( node != nullptr );
    assert( code != nullptr );

    TableLocalNames newLocalNames = { {}, 0 };

    if ( node->leftChild->leftChild->nodeType == MAIN )
    {
        ConvertSubtreeInCode( node->rightChild, code, &newLocalNames );
        memcpy( localNames, &newLocalNames, sizeof(TableLocalNames) );
    }
    else
    {
        if ( functions.curName < NUMBERS_FUNCTION )
        {

            node->leftChild->leftChild->str[strlen( node->leftChild->leftChild->str ) - 1] = '\0';
            fprintf(code, "\n%s:\n\n"
                          "\n\tpop rcx\n"
                            "\txor r15, r15\n\n", node->leftChild->leftChild->str + 1);

            if ( node->leftChild->rightChild != nullptr )
            {
                if ( node->leftChild->rightChild->leftChild != nullptr )
                {
                    fprintf( code, "\n\tpop rax\n"
                                    "\tpop rbx\n\n" );

                    Node_t *node1 = node->leftChild->rightChild;
                    fprintf( code, "\n\tmov [rsi + (r14 + %d + %d) * 8], rbx\n\n",
                                  globalNames.curName, CheckTableLocalNames( node1->rightChild, code, &newLocalNames ) );
                    while ( node1->leftChild != nullptr )
                    {
                        node1 = node1->leftChild;
                        fprintf( code, "\n\tmov [rsi + (r14 + %d + %d) * 8], rax\n\n",
                                      globalNames.curName, CheckTableLocalNames( node1->rightChild, code, &newLocalNames ) );
                        //! ToDo other register (extensibility)
                    }
                }
                else
                {
                    fprintf( code, "\n\tpop rax\n\n" );

                    Node_t *node1 = node->leftChild->rightChild;
                    fprintf( code, "\n\tmov [rsi + (r14 + %d + %d) * 8], rax\n\n",
                                  globalNames.curName, CheckTableLocalNames( node1->rightChild, code, &newLocalNames ) );
                }
            }

            fprintf( code, "\n\tpush rcx\n\n" );

            ConvertSubtreeInCode( node->rightChild, code, &newLocalNames );
        }
        else
        {
            printf( "Invalid number of local variables!\n" );
            return;
        }
    }
}

static void ConvertReturnNodeInCode( Node_t *node, FILE *code, TableLocalNames *localNames )
{
    assert( node       != nullptr );
    assert( code       != nullptr );
    assert( localNames != nullptr );

    if ( node->rightChild != nullptr )
    {
        ConvertSubtreeInCode( node->rightChild, code, localNames );
        
        u_int8_t bufNumOpers[2] = { 0x58, 0xc3 };
        fwrite( bufNumOpers, sizeof( u_int8_t ), 2, code );
        numBytesInFile += 2;
    }
    else
    {
        u_int8_t bufNumOper = 0xc3;
        fwrite( &bufNumOper, sizeof( u_int8_t ), 1, code );
        numBytesInFile += 1;
    }
}

static void ConvertCallNodeInCode( Node_t *node, FILE *code, TableLocalNames *localNames )
{
    assert( node       != nullptr );
    assert( code       != nullptr );
    assert( localNames != nullptr );

    if ( node->leftChild->nodeType == SCAN || node->leftChild->nodeType == PRINT || node->leftChild->nodeType == SQRT )
    {
        if ( node->leftChild->nodeType == SCAN )
        {
            u_int8_t bufNumOpers1[34] = { 0x48, 0x83, 0xec, 0x18, 0x48, 0xc7, 0x44, 0x24, 0x08, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8d, 0x74,
                                          0x24, 0x08, 0x48, 0xbf, 0x6c, 0x43, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00,
                                          0x00, 0xe8 };
            fwrite( bufNumOpers1, sizeof( u_int8_t ), 34, code );
            numBytesInFile += 34;

            u_int16_t curAddressScanf = START_ADDRESS_SCANF - numBytesInFile;
            fwrite( &curAddressScanf, sizeof( u_int16_t ), 1, code );
            numBytesInFile += 2;

            u_int8_t bufNumOpers2[22] = { 0xff, 0xff, 0x48, 0xbe, 0x48, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8b, 0x44, 0x24,
                                          0x08, 0x48, 0x83, 0xc4, 0x18, 0x50 };
            fwrite( bufNumOpers2, sizeof( u_int8_t ), 22, code );
            numBytesInFile += 22;
        }
        else if (node->leftChild->nodeType == PRINT)    
        {
            u_int8_t bufNumOpers1[10] = { 0x48, 0xbf, 0x68, 0x43, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00 };
            fwrite( bufNumOpers1, sizeof( u_int8_t ), 10, code );
            numBytesInFile += 10;
            
            ConvertSubtreeInCode( node->rightChild, code, localNames );
            
            u_int8_t bufNumOpers2[10] = { 0x58, 0x48, 0x89, 0xc6, 0xb8, 0x00, 0x00, 0x00, 0x00, 0xe8 };
            fwrite( bufNumOpers2, sizeof( u_int8_t ), 10, code );
            numBytesInFile += 10;

            u_int16_t curAddressPrintf = START_ADDRESS_PRINTF - numBytesInFile;
            fwrite( &curAddressPrintf, sizeof( u_int16_t ), 1, code );
            numBytesInFile += 2;

            u_int8_t bufNumOpers3[12] = { 0xff, 0xff, 0x48, 0xbe, 0x48, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00 };
            fwrite( bufNumOpers3, sizeof( u_int8_t ), 12, code );
            numBytesInFile += 12;
        }
        else
        {
            ConvertSubtreeInCode(node->rightChild, code, localNames);
            
            u_int8_t bufNumOpers1[6] = { 0x58, 0xf2, 0x0f, 0x2a, 0xc0, 0xe8 };
            fwrite( bufNumOpers1, sizeof( u_int8_t ), 6, code );
            numBytesInFile += 6;

            u_int16_t curAddressSqrt = START_ADDRESS_SQRT - numBytesInFile;
            fwrite( &curAddressSqrt, sizeof( u_int16_t ), 1, code );
            numBytesInFile += 2;

            u_int8_t bufNumOpers2[7] = { 0xff, 0xff, 0xf2, 0x0f, 0x2c, 0xc0, 0x50 };
            fwrite( bufNumOpers2, sizeof( u_int8_t ), 7, code );
            numBytesInFile += 7;
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

static void ConvertIfNodeInCode( Node_t *node, FILE *code, TableLocalNames *localNames )
{
    assert( node       != nullptr );
    assert( code       != nullptr );
    assert( localNames != nullptr );

    int32_t label = 0;

    ConvertSubtreeInCode( node->leftChild, code, localNames );
    
    label = GenerateLabel();
    
    if ( node->rightChild->rightChild != nullptr ) { ConvertSubtreeInCode( node->rightChild->rightChild, code, localNames ); }
    
    u_int8_t bufNumOpers[5] = { 0xe9, 0x00, 0x00, 0x00, 0x00 };
    fwrite( bufNumOpers, sizeof( u_int8_t ), 5, code  );
    numBytesInFile += 5;

    // fprintf( code, "\n\tjmp .next%d\n\n", label );
    // fprintf( code, "\n.next%d:\n\n", label - 1 );
    ConvertSubtreeInCode( node->rightChild->leftChild, code, localNames );
    // fprintf( code, "\n.next%d:\n\n", label );
}

static void ConvertWhileNodeInCode( Node_t *node, FILE *code, TableLocalNames *localNames )
{
    assert( node       != nullptr );
    assert( code       != nullptr );
    assert( localNames != nullptr );

    int32_t label = GenerateLabel();

    // fprintf( code, "\n.next%d:\n\n", label );
    ConvertSubtreeInCode( node->leftChild, code, localNames );
    
    label = GenerateLabel();
    
    u_int8_t bufNumOpers[5] = { 0xe9, 0x00, 0x00, 0x00, 0x00 };
    fwrite( bufNumOpers, sizeof( u_int8_t ), 5, code  );
    numBytesInFile += 5;
    
    // fprintf( code, "\n\tjmp .next%d\n", label     );
    // fprintf( code, "\n.next%d:\n\n"   , label - 1 );
    ConvertSubtreeInCode( node->rightChild, code, localNames );
    
    fwrite( bufNumOpers, sizeof( u_int8_t ), 5, code  );
    numBytesInFile += 5;
    // fprintf( code, "\n\tjmp .next%d\n", label - 2 );
    // fprintf( code, "\n.next%d:\n\n"   , label     );
}

static void ConvertAssignNodeInCode( Node_t *node, FILE *code, TableLocalNames *localNames )
{
    assert( node       != nullptr );
    assert( code       != nullptr );
    assert( localNames != nullptr );

    int32_t curOffset = CheckTableGlobalNames( node->leftChild );
    if (curOffset == NO_VARIABLE_IN_TABLE_NAME)
    {
        ConvertSubtreeInCode( node->rightChild, code, localNames );
        
        curOffset = CheckTableLocalNames( node->leftChild, code, localNames );

        u_int8_t bufNumOpers[5] = { 0x58, 0x4a, 0x89, 0x84, 0xf6 };
        fwrite( bufNumOpers, sizeof( u_int8_t ), 5, code  );
        numBytesInFile += 5;
        
        int32_t regVal = ( globalNames.curName + curOffset ) * 8;
        fwrite( &regVal, sizeof( int32_t ), 1, code );
        numBytesInFile += 4;
    }
    else
    {
        ConvertSubtreeInCode( node->rightChild, code, localNames );

        u_int8_t bufNumOpers[4] = { 0x58, 0x48, 0x89, 0x86 };
        fwrite( bufNumOpers, sizeof( u_int8_t ), 4, code  );
        numBytesInFile += 4;
        
        int32_t regVal = curOffset * 8;
        fwrite( &regVal, sizeof( int32_t ), 1, code );
        numBytesInFile += 4;
    }
}

static void ConvertLocVariableNodeIncode(Node_t *node, FILE *code, TableLocalNames *localNames)
{
    assert(node       != nullptr);
    assert(code       != nullptr);
    assert(localNames != nullptr);

    int32_t curOffset = CheckTableGlobalNames( node );
    if ( curOffset == NO_VARIABLE_IN_TABLE_NAME )
    {
        curOffset = CheckTableLocalNames( node, code, localNames );

        u_int8_t bufNumOpers[4] = { 0x4a, 0x8b, 0x84, 0xf6 };
        fwrite( bufNumOpers, sizeof( u_int8_t ), 4, code  );
        numBytesInFile += 4;
        
        int32_t regVal = ( globalNames.curName + curOffset ) * 8;
        fwrite( &regVal, sizeof( int32_t ), 1, code );
        numBytesInFile += 4;
        
        u_int8_t bufNumOper = 0x50;
        fwrite( &bufNumOper, sizeof( u_int8_t ), 1, code );
        numBytesInFile += 1;
    }
    else
    {
        u_int8_t bufNumOpers[3] = { 0x48, 0x8b, 0x86 };
        fwrite( bufNumOpers, sizeof( u_int8_t ), 3, code  );
        numBytesInFile += 3;
        
        int32_t regVal = curOffset * 8;
        fwrite( &regVal, sizeof( int32_t ), 1, code );
        numBytesInFile += 4;
        
        u_int8_t bufNumOper = 0x50;
        fwrite( &bufNumOper, sizeof( u_int8_t ), 1, code );
        numBytesInFile += 1;
    }
}

static void ConvertConstNodeInCode( Node_t *node, FILE *code, TableLocalNames *localNames )
{
    assert( node       != nullptr );
    assert( code       != nullptr );
    assert( localNames != nullptr );

    u_int8_t bufNumOper = 0xb8;
    fwrite( &bufNumOper, sizeof( u_int8_t ), 1, code );
    numBytesInFile += 1;
    
    int32_t regVal = (int32_t)node->value;
    fwrite( &regVal, sizeof( int32_t ), 1, code );
    numBytesInFile += 4;

    bufNumOper = 0x50;
    fwrite( &bufNumOper, sizeof( u_int8_t ), 1, code );
    numBytesInFile += 1;
}

static void ConvertBinaryOperationNodeInCode( Node_t *node, FILE *code, TableLocalNames *localNames )
{
    assert( node       != nullptr );
    assert( code       != nullptr );
    assert( localNames != nullptr );

    ConvertSubtreeInCode( node->leftChild , code, localNames );
    ConvertSubtreeInCode( node->rightChild, code, localNames );
    switch ( (int32_t)node->nodeType )
    {
        case (int32_t)ADD:
        {
            u_int8_t bufNumOpers[6] = { 0x5b, 0x58, 0x48, 0x01, 0xd8, 0x50 };
            fwrite( bufNumOpers, sizeof( u_int8_t ), 6, code );
            numBytesInFile += 6;

            break;
        }
        case (int32_t)SUB:
        {
            u_int8_t bufNumOpers[6] = { 0x5b, 0x58, 0x48, 0x29, 0xd8, 0x50 };
            fwrite( bufNumOpers, sizeof( u_int8_t ), 6, code );
            numBytesInFile += 6;

            break;
        }
        case (int32_t)MUL:
        {
            u_int8_t bufNumOpers[6] = { 0x5b, 0x58, 0xf7, 0xe3, 0x50 };
            fwrite( bufNumOpers, sizeof( u_int8_t ), 6, code );
            break;
        }
        case (int32_t)DIV:
        {
            u_int8_t bufNumOpers[6] = { 0x5b, 0x58, 0x99, 0xf7, 0xfb, 0x50 };
            fwrite( bufNumOpers, sizeof( u_int8_t ), 6, code );
            numBytesInFile += 6;

            break;
        }
        case (int32_t)POW:
        {
            u_int8_t bufNumOpers1[11] = { 0x5b, 0x58, 0xf2, 0x0f, 0x2a, 0xc0, 0xf2, 0x0f, 0x2a, 0xcb, 0xe8 };
            fwrite( bufNumOpers1, sizeof( u_int8_t ), 11, code );
            numBytesInFile += 11;

            u_int16_t curAddressPow = START_ADDRESS_POW - numBytesInFile;
            fwrite( &curAddressPow, sizeof( u_int16_t ), 1, code );
            numBytesInFile += 2;

            u_int8_t bufNumOpers2[17] = { 0xff, 0xff, 0x48, 0xbe, 0x48, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf2, 0x0f, 0x2c, 0xc0,
                                         0x50 };
            fwrite( bufNumOpers2, sizeof( u_int8_t ), 17, code );
            numBytesInFile += 17;

            break;
        }
        case (int32_t)JA:
        {
            u_int8_t bufNumOpers[11] = { 0x5b, 0x58, 0x48, 0x39, 0xd8, 0x0f, 0x8f, 0x00, 0x00, 0x00, 0x00 };
            fwrite( bufNumOpers, sizeof( u_int8_t ), 11, code );
            numBytesInFile += 11;

            // fprintf(code, "\n\tpop rbx\n"
            //                 "\tpop rax\n"
            //                 "\tcmp rax, rbx\n"
            //                 "\tjg .next%d\n\n" , GenerateLabel());
            break;
        }
        case (int32_t)JB:
        {
            u_int8_t bufNumOpers[11] = { 0x5b, 0x58, 0x48, 0x39, 0xd8, 0x0f, 0x8c, 0x00, 0x00, 0x00, 0x00 };
            fwrite( bufNumOpers, sizeof( u_int8_t ), 11, code );
            numBytesInFile += 11;

            // fprintf(code, "\n\tpop rbx\n"
            //                 "\tpop rax\n"
            //                 "\tcmp rax, rbx\n"
            //                 "\tjl .next%d\n\n" , GenerateLabel());
            break;
        }
        case (int32_t)JE:
        { 
            u_int8_t bufNumOpers[11] = { 0x5b, 0x58, 0x48, 0x39, 0xd8, 0x0f, 0x84, 0x00, 0x00, 0x00, 0x00 };
            fwrite( bufNumOpers, sizeof( u_int8_t ), 11, code );
            numBytesInFile += 11;

            // fprintf(code, "\n\tpop rbx\n"
            //                 "\tpop rax\n"
            //                 "\tcmp rax, rbx\n"
            //                 "\tje .next%d\n\n" , GenerateLabel());
            break;
        }
        case (int32_t)JAE:
        {
            u_int8_t bufNumOpers[11] = { 0x5b, 0x58, 0x48, 0x39, 0xd8, 0x0f, 0x8d, 0x00, 0x00, 0x00, 0x00 };
            fwrite( bufNumOpers, sizeof( u_int8_t ), 11, code );
            numBytesInFile += 11;

            // fprintf(code, "\n\tpop rbx\n"
            //                 "\tpop rax\n"
            //                 "\tcmp rax, rbx\n"
            //                 "\tjge .next%d\n\n" , GenerateLabel());
            break;
        }
        case (int32_t)JBE:
        {
            u_int8_t bufNumOpers[11] = { 0x5b, 0x58, 0x48, 0x39, 0xd8, 0x0f, 0x8e, 0x00, 0x00, 0x00, 0x00 };
            fwrite( bufNumOpers, sizeof( u_int8_t ), 11, code );
            numBytesInFile += 11;

            // fprintf(code, "\n\tpop rbx\n"
            //                 "\tpop rax\n"
            //                 "\tcmp rax, rbx\n"
            //                 "\tjle .next%d\n\n" , GenerateLabel());
            break;
        }
        case (int32_t)JNE:
        {
            u_int8_t bufNumOpers[11] = { 0x5b, 0x58, 0x48, 0x39, 0xd8, 0x0f, 0x85, 0x00, 0x00, 0x00, 0x00 };
            fwrite( bufNumOpers, sizeof( u_int8_t ), 11, code );
            numBytesInFile += 11;

            // fprintf(code, "\n\tpop rbx\n"
            //                 "\tpop rax\n"
            //                 "\tcmp rax, rbx\n"
            //                 "\tjne .next%d\n\n" , GenerateLabel());
            break;
        }
        default:
        {
            printf( "Invalid binary operations!\n" );
            break;
        }
    }
}

static void ConvertSubtreeInCode( Node_t *node, FILE *code, TableLocalNames *localNames )
{
    assert( node       != nullptr );
    assert( code       != nullptr );
    assert( localNames != nullptr );

    switch ( (int32_t)node->nodeType )
    {
        case (int32_t)DEFINE   : { ConvertDefineNodeInCode         ( node, code, localNames ); return; }
        case (int32_t)RETURN   : { ConvertReturnNodeInCode         ( node, code, localNames ); return; }
        case (int32_t)CALL     : { ConvertCallNodeInCode           ( node, code, localNames ); return; }
        case (int32_t)IF       : { ConvertIfNodeInCode             ( node, code, localNames ); return; }
        case (int32_t)WHILE    : { ConvertWhileNodeInCode          ( node, code, localNames ); return; }
        case (int32_t)ASSIGN   : { ConvertAssignNodeInCode         ( node, code, localNames ); return; }
        case (int32_t)CONST    : { ConvertConstNodeInCode          ( node, code, localNames ); return; }
        case (int32_t)VARIABLE : { ConvertLocVariableNodeIncode    ( node, code, localNames ); return; }
        case (int32_t)ADD      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return; }
        case (int32_t)SUB      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return; }
        case (int32_t)MUL      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return; }
        case (int32_t)DIV      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return; }
        case (int32_t)POW      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return; }
        case (int32_t)JA       : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return; }
        case (int32_t)JB       : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return; }
        case (int32_t)JE       : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return; }
        case (int32_t)JAE      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return; }
        case (int32_t)JBE      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return; }
        case (int32_t)JNE      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return; }
        default            : { break; }
    }

    if ( node->leftChild  != nullptr ) { ConvertSubtreeInCode( node->leftChild , code, localNames ); }
    if ( node->rightChild != nullptr ) { ConvertSubtreeInCode( node->rightChild, code, localNames ); }
}

void ClearCodeFile( FILE* code )
{
    assert( code != nullptr );

    if ( fseek ( code, START_POSITION_IN_FILE, SEEK_SET ) )
    {
        printf( "The pointer to the file position has not been moved!\n" );
        return;
    }

    u_int8_t numberOper = 0x90;
    for ( size_t i = 0; i < 3344; i++ )
    {
        fwrite( &numberOper, sizeof( u_int8_t ), 1, code );
    }
}

void GenerateAsmCode( Tree_t *tree )
{
    assert( tree != nullptr );

    FILE *code = fopen( NAME_OUTPUT_FILE, "r+b" );

    ClearCodeFile( code );

    if ( fseek ( code, START_POSITION_IN_FILE, SEEK_SET ) )
    {
        printf( "The pointer to the file position has not been moved!\n" );
        return;
    }

    if ( !FindMain(tree) )
    {
        printf( "No main declaration!\n" );
        return;
    }

    u_int8_t bufNumOpers[16] = { 0x48, 0xbe, 0x48, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x31, 0xf6, 0x4d, 0x31, 0xff };
    fwrite( &bufNumOpers, sizeof( u_int8_t ), 16, code );
    numBytesInFile += 16;

    // FillTableFunctions   ( tree->root );
    // FindAndPrintGlobalVar( tree, code );

    TableLocalNames localNames = { {}, 0 };

    ConvertSubtreeInCode( tree->root, code, &localNames );

    fclose( code );
}
