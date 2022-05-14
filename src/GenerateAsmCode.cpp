#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../include/GenerateAsmCode.h"
#include "../include/DSL.h"

const char     *NAME_OUTPUT_FILE          = "res/prog";
const char     *NAME_RESIZE_FILE          = "asmTests/prog.asm";
const u_int32_t START_POSITION_IN_FILE    = 4448;
      u_int32_t NUMBERS_NOP               = 3344;
const u_int32_t NUMBERS_NOP_FOR_RESIZE_IP = 1000;
      u_int64_t ADDRESS_DATA_BUFFER       = 0x404048;
      u_int64_t ADDRESS_DATA_PRINTF       = 0x404368;
      u_int64_t ADDRESS_DATA_SCANF        = 0x40436c;
const u_int16_t START_ADDRESS_FUNC_PRINTF = 0xfedc;
const u_int16_t START_ADDRESS_FUNC_SCANF  = 0xfeec;
const u_int16_t START_ADDRESS_FUNC_POW    = 0xfecc;
const u_int16_t START_ADDRESS_FUNC_SQRT   = 0xfefc;
const int32_t   NO_VARIABLE_IN_TABLE_NAME = -1;
const int32_t   NO_FUNCTION_IN_TABLE_NAME = -1;

TableGlobalNames globalNames = { {}, 0 };
TableFunctions   functions   = {};

u_int32_t curLabel       = 0;
u_int32_t numBytesInFile = 0;
u_int32_t numTreeCrawl   = 0;

int32_t labelsAddress[100] = {};

#define myFwrite( address, sizeof_elem, num_elems, file )                             \
    do                                                                                \
    {                                                                                 \
        if ( numTreeCrawl == 2 ) { fwrite( address, sizeof_elem, num_elems, file ); } \
    } while ( 0 )

static int32_t GenerateLabel()
{
    return curLabel++;
}

static int NodeVisitorForSearchMain( Node_t *node )
{
    assert( node != nullptr );

    if ( node->nodeType == MAIN ) { return 1; }

    if ( node->leftChild  != nullptr ) { if ( NodeVisitorForSearchMain( node->leftChild  ) ) { return 1; } }
    if ( node->rightChild != nullptr ) { if ( NodeVisitorForSearchMain( node->rightChild ) ) { return 1; } }

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
                u_int8_t bufNumOper = 0xb8;                                                       // mov rax, C
                myFwrite( &bufNumOper, sizeof( u_int8_t ), 1, code );                             //          |
                numBytesInFile += 1;                                                              //          |
                                                                                                  //          |
                int32_t regVal = (int32_t)( node->leftChild->rightChild->rightChild->value * 8 ); //<---------|            
                myFwrite( &regVal, sizeof( int32_t ), 1, code );                                  // C - constant value
                numBytesInFile += 4;

                u_int8_t bufNumOpers[3] = { 0x48, 0x89, 0x86 };                   // mov [rsi + x * 8], rax
                myFwrite( bufNumOpers, sizeof( u_int8_t ), 3, code );             //          |______|
                numBytesInFile += 3;                                              //              |
                                                                                  //              |
                int32_t curOffsetInMemory = (int32_t)( globalNames.curName * 8 ); //<-------------|
                myFwrite( &curOffsetInMemory, sizeof( int32_t ), 1, code );       // x - number of global variables
                numBytesInFile += 4;
            }
            else
            {
                printf( "Error: the global variable is not assigned a constant!\n" );
                return ;
            }
            globalNames.curName++;
            if ( node->leftChild->leftChild != nullptr )
            {
                node = node->leftChild->leftChild;
                NodeVisitorForFindAndPrintGlobalVar( tree, node, code );
            }
        }
        else
        {
            NodeVisitorForFindAndPrintGlobalVar( tree, node->leftChild, code );
        }
    }
}

//! Only a constant can be assigned to a global variable, otherwise it will lead to an error
static void FindAndPrintGlobalVar( Tree_t *tree, FILE *code )
{
    assert( tree != nullptr );
    assert( code != nullptr );

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
        
        u_int8_t bufNumOpers[3] = { 0x49, 0xff, 0xc7 }; // inc r15
        myFwrite( bufNumOpers, sizeof( u_int8_t ), 3, code );
        numBytesInFile += 3;
    }
    else
    {
        printf( "Error: invalid number of local variables!\n" );
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
            u_int32_t curFunCall = CheckTableFunctions( node->leftChild->leftChild );
            if ( numTreeCrawl == 1 ) { functions.functions[curFunCall].addressCall += numBytesInFile; }
            u_int8_t bufNumOpers1[4] = { 0x59,               // pop rcx
                                         0x4d, 0x31, 0xff }; // xor r15, r15
            myFwrite( bufNumOpers1, sizeof( u_int8_t ), 4, code );
            numBytesInFile += 4;

            if ( node->leftChild->rightChild != nullptr )
            {
                if ( node->leftChild->rightChild->leftChild != nullptr )
                {
                    u_int8_t bufNumOpers2[2] = { 0x58,   // pop rax
                                                 0x5b }; // pop rbx
                    myFwrite( bufNumOpers2, sizeof( u_int8_t ), 2, code );
                    numBytesInFile += 2;

                    Node_t *node1 = node->leftChild->rightChild;

                    int32_t curOffsetInMemory = (int32_t)( ( globalNames.curName + CheckTableLocalNames( node1->rightChild, code, &newLocalNames ) ) * 8 );
                                                        // |_____________________________________________________________________________________________|
                                                                           //                       ^
                                                                           //                       |
                                                                           //                 |-----------|
                    u_int8_t bufNumOpers3[4] = { 0x4a, 0x89, 0x9c, 0xf6 }; // mov [rsi + (r14 + x + y) * 8], rbx
                    myFwrite( bufNumOpers3, sizeof( u_int8_t ), 4, code ); // x - number of global variables
                    numBytesInFile += 4;                                   // y - number of local variables

                    myFwrite( &curOffsetInMemory, sizeof( int32_t ), 1, code );
                    numBytesInFile += 4;

                    while ( node1->leftChild != nullptr )
                    {
                        node1 = node1->leftChild;

                        curOffsetInMemory = (int32_t)( ( globalNames.curName + CheckTableLocalNames( node1->rightChild, code, &newLocalNames ) ) * 8 );
                                                    // |_____________________________________________________________________________________________|
                                                                               //                       ^
                                                                               //                       |
                                                                               //                 |-----------|
                        u_int8_t bufNumOpers4[4] = { 0x4a, 0x89, 0x84, 0xf6 }; // mov [rsi + (r14 + x + y) * 8], rax
                        myFwrite( bufNumOpers4, sizeof( u_int8_t ), 4, code ); // x - number of global variables
                        numBytesInFile += 4;                                   // y - number of local variables

                        myFwrite( &curOffsetInMemory, sizeof( int32_t ), 1, code );
                        numBytesInFile += 4;
                    }
                }
                else
                {
                    u_int8_t bufNumOper = 0x58;
                    myFwrite( &bufNumOper, sizeof( u_int8_t ), 1, code );
                    numBytesInFile += 1;

                    Node_t *node1 = node->leftChild->rightChild;
                    
                    int32_t curOffsetInMemory = (int32_t)( ( globalNames.curName + CheckTableLocalNames( node1->rightChild, code, &newLocalNames ) ) * 8 );
                                                      // |_______________________________________________________________________________________________|
                                                                           //                       ^
                                                                           //                       |
                                                                           //                 |-----------|
                    u_int8_t bufNumOpers3[4] = { 0x4a, 0x89, 0x84, 0xf6 }; // mov [rsi + (r14 + x + y) * 8], rax
                    myFwrite( bufNumOpers3, sizeof( u_int8_t ), 4, code ); // x - number of global variables
                    numBytesInFile += 4;                                   // y - number of local variables

                    myFwrite( &curOffsetInMemory, sizeof( int32_t ), 1, code );
                    numBytesInFile += 4;
                }
            }

            u_int8_t bufNumOper = 0x51; // push rcx
            myFwrite( &bufNumOper, sizeof( u_int8_t ), 1, code );
            numBytesInFile += 1;

            ConvertSubtreeInCode( node->rightChild, code, &newLocalNames );
        }
        else
        {
            printf( "Error: invalid number of local variables!\n" );
            return ;
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
        
        u_int8_t bufNumOpers[2] = { 0x58,   // pop rax
                                    0xc3 }; // ret
        myFwrite( bufNumOpers, sizeof( u_int8_t ), 2, code );
        numBytesInFile += 2;
    }
    else
    {
        u_int8_t bufNumOper = 0xc3; // ret
        myFwrite( &bufNumOper, sizeof( u_int8_t ), 1, code );
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
            u_int8_t bufNumOpers1[20] = { 0x48, 0x83, 0xec, 0x18,                               // sub rsp, 24
                                          0x48, 0xc7, 0x44, 0x24, 0x08, 0x00, 0x00, 0x00, 0x00, // mov qword [rsp + 8], 0
                                          0x48, 0x8d, 0x74, 0x24, 0x08,                         // lea rsi, [rsp + 8]
                                          0x48, 0xbf };                                         // mov rdi, SCAN
            myFwrite( bufNumOpers1, sizeof( u_int8_t ), 20, code );
            numBytesInFile += 20;

            myFwrite( &ADDRESS_DATA_SCANF, sizeof( u_int64_t ), 1, code );
            numBytesInFile += 8;

            u_int8_t bufNumOpers2[6] = { 0xb8, 0x00, 0x00, 0x00, 0x00, // mov rax, 0
                                         0xe8 };                       // call scanf
            myFwrite( bufNumOpers2, sizeof( u_int8_t ), 6, code );
            numBytesInFile += 6;

            u_int16_t curAddressScanf = START_ADDRESS_FUNC_SCANF - numBytesInFile;
            myFwrite( &curAddressScanf, sizeof( u_int16_t ), 1, code ); // call address ( 0xXX 0xXX ____ ____ )
            numBytesInFile += 2;

            u_int8_t bufNumOpers3[4] = { 0xff, 0xff,   // call address ( ____ ____ 0xXX 0xXX )
                                         0x48, 0xbe }; // mov rsi, BUFFER
            myFwrite( bufNumOpers3, sizeof( u_int8_t ), 4, code );
            numBytesInFile += 4;

            myFwrite( &ADDRESS_DATA_BUFFER, sizeof( u_int64_t ), 1, code );
            numBytesInFile += 8;

            u_int8_t bufNumOpers4[10] = { 0x48, 0x8b, 0x44, 0x24, 0x08, // mov rax, qword [rsp + 8]
                                          0x48, 0x83, 0xc4, 0x18,       // add rsp, 24
                                          0x50 };                       // push rax
            myFwrite( bufNumOpers4, sizeof( u_int8_t ), 10, code );
            numBytesInFile += 10;
        }
        else if (node->leftChild->nodeType == PRINT)    
        {
            u_int8_t bufNumOpers1[2] = { 0x48, 0xbf }; // mov rdi, PRINT
            myFwrite( bufNumOpers1, sizeof( u_int8_t ), 2, code );
            numBytesInFile += 2;
            
            myFwrite( &ADDRESS_DATA_PRINTF, sizeof( u_int64_t ), 1, code );
            numBytesInFile += 8;

            ConvertSubtreeInCode( node->rightChild, code, localNames );
            
            u_int8_t bufNumOpers2[10] = { 0x58,                         // pop rax
                                          0x48, 0x89, 0xc6,             // mov rsi, rax
                                          0xb8, 0x00, 0x00, 0x00, 0x00, // mov rax, 0
                                          0xe8 };                       // call printf
            myFwrite( bufNumOpers2, sizeof( u_int8_t ), 10, code );
            numBytesInFile += 10;

            u_int16_t curAddressPrintf = START_ADDRESS_FUNC_PRINTF - numBytesInFile; // call address ( 0xXX 0xXX ____ ____ )
            myFwrite( &curAddressPrintf, sizeof( u_int16_t ), 1, code );
            numBytesInFile += 2;

            u_int8_t bufNumOpers3[4] = { 0xff, 0xff,   // call address ( ____ ____ 0xXX 0xXX )
                                          0x48, 0xbe }; // mov rsi, BUFFER
            myFwrite( bufNumOpers3, sizeof( u_int8_t ), 4, code );
            numBytesInFile += 4;

            myFwrite( &ADDRESS_DATA_BUFFER, sizeof( u_int64_t ), 1, code );
            numBytesInFile += 8;
        }
        else
        {
            ConvertSubtreeInCode(node->rightChild, code, localNames);
            
            u_int8_t bufNumOpers1[6] = { 0x58,                   // pop rax
                                         0xf2, 0x0f, 0x2a, 0xc0, // cvtsi2sd xmm0, eax
                                         0xe8 };                 // call sqrt
            myFwrite( bufNumOpers1, sizeof( u_int8_t ), 6, code );
            numBytesInFile += 6;

            u_int16_t curAddressSqrt = START_ADDRESS_FUNC_SQRT - numBytesInFile; // call address ( 0xXX 0xXX ____ ____ )
            myFwrite( &curAddressSqrt, sizeof( u_int16_t ), 1, code );
            numBytesInFile += 2;

            u_int8_t bufNumOpers2[7] = { 0xff, 0xff,             // call address ( ____ ____ 0xXX 0xXX )
                                         0xf2, 0x0f, 0x2c, 0xc0, // cvttsd2si eax, xmm0
                                         0x50 };                 // push rax
            myFwrite( bufNumOpers2, sizeof( u_int8_t ), 7, code );
            numBytesInFile += 7;
        }
    }
    else
    {
        int isFunctionInTableFunctions = CheckTableFunctions( node->leftChild );

        if (isFunctionInTableFunctions != NO_FUNCTION_IN_TABLE_NAME)
        {
            u_int8_t bufNumOpers1[4] = { 0x41, 0x56,   // push r14
                                         0x41, 0x57 }; // push r15
            myFwrite( bufNumOpers1, sizeof( u_int8_t ), 4, code );
            numBytesInFile += 4;

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

            u_int8_t bufNumOpers2[4] = { 0x4d, 0x01, 0xfe, // add r14, r15
                                         0xe8 };           // call function
            myFwrite( bufNumOpers2, sizeof( u_int8_t ), 4, code );
            numBytesInFile += 4;

            numBytesInFile      += 4;
            u_int32_t curFunCall = CheckTableFunctions( node->leftChild );
            int32_t addressCall  = functions.functions[curFunCall].addressCall - numBytesInFile; // call address ( 0xXX 0xXX 0xXX 0xXX )
            myFwrite( &addressCall, sizeof( int32_t ), 1, code );

            u_int8_t bufNumOpers3[5] = { 0x41, 0x5f, // pop r15
                                         0x41, 0x5e, // pop r14
                                         0x50 };     // push rax
            myFwrite( bufNumOpers3, sizeof( u_int8_t ), 5, code );
            numBytesInFile += 5;
        }
        else
        {
            printf("Invalid function call!\n");
            return ;
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
    
    u_int8_t bufNumOper = 0xe9;                                             // jmp x
    myFwrite( &bufNumOper, sizeof( u_int8_t ), 1, code  );                  //     |
    numBytesInFile += 1;                                                    //     |
                                                                            //     |
    numBytesInFile += 4;                                                    //     |
    u_int32_t numLabel = label;                                             //     |
    if ( numTreeCrawl == 1 ) { labelsAddress[numLabel] -= numBytesInFile; } //     |
    int32_t addressJmp = labelsAddress[numLabel];                           //<----|
    myFwrite( &addressJmp, sizeof( int32_t ), 1, code );                    // x- jump address

    if ( numTreeCrawl == 1 ) { labelsAddress[label - 1] += numBytesInFile; }
    
    ConvertSubtreeInCode( node->rightChild->leftChild, code, localNames );
    
    if ( numTreeCrawl == 1 ) { labelsAddress[label] += numBytesInFile; }
}

static void ConvertWhileNodeInCode( Node_t *node, FILE *code, TableLocalNames *localNames )
{
    assert( node       != nullptr );
    assert( code       != nullptr );
    assert( localNames != nullptr );

    int32_t label = GenerateLabel();

    if ( numTreeCrawl == 1 ) { labelsAddress[label] += numBytesInFile; }
    
    ConvertSubtreeInCode( node->leftChild, code, localNames );
    
    label = GenerateLabel();
    
    u_int8_t bufNumOpers = 0xe9;                                            // jmp x1
    myFwrite( &bufNumOpers, sizeof( u_int8_t ), 1, code );                  //     |
    numBytesInFile += 1;                                                    //     |
                                                                            //     |
    numBytesInFile += 4;                                                    //     |
    u_int32_t numLabel = label;                                             //     |
    if ( numTreeCrawl == 1 ) { labelsAddress[numLabel] -= numBytesInFile; } //     |
    int32_t addressJmp = labelsAddress[numLabel];                           //<----|
    myFwrite( &addressJmp, sizeof( int32_t ), 1, code );                    // x1 - jump address
    
    if ( numTreeCrawl == 1 ) { labelsAddress[label - 1] += numBytesInFile; }
    
    ConvertSubtreeInCode( node->rightChild, code, localNames );
    
    myFwrite( &bufNumOpers, sizeof( u_int8_t ), 1, code  );                     // jmp x2
    numBytesInFile += 1;                                                        //     |
                                                                                //     |
    numBytesInFile += 4;                                                        //     |
    numLabel = label;                                                           //     |
    if ( numTreeCrawl == 1 ) { labelsAddress[numLabel - 2] -= numBytesInFile; } //     |
    addressJmp = labelsAddress[numLabel - 2];                                   //<----|
    myFwrite( &addressJmp, sizeof( int32_t ), 1, code );                        // x2 - jump address
    
    if ( numTreeCrawl == 1 ) { labelsAddress[label] += numBytesInFile; }
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

        u_int8_t bufNumOpers[5] = { 0x58,                         // pop rax
                                    0x4a, 0x89, 0x84, 0xf6 };     // mov [rsi + (r14 + x + y) * 8], rax
                                                                  //                 |           |
        myFwrite( bufNumOpers, sizeof( u_int8_t ), 5, code  );    //                 +-----+-----+
        numBytesInFile += 5;                                      //                       |
                                                                  //                       |
        int32_t regVal = ( globalNames.curName + curOffset ) * 8; //<----------------------+
        myFwrite( &regVal, sizeof( int32_t ), 1, code );          // r14 - counter of local variables in the function 
        numBytesInFile += 4;                                      // x - number of global variables
                                                                  // y - number of local variables
    }
    else
    {
        ConvertSubtreeInCode( node->rightChild, code, localNames );

        u_int8_t bufNumOpers[4] = { 0x58,                       // pop rax
                                    0x48, 0x89, 0x86 };         // mov [rsi + x * 8], rax
        myFwrite( bufNumOpers, sizeof( u_int8_t ), 4, code  );  //          |______|
        numBytesInFile += 4;                                    //              |
                                                                //              |
        int32_t regVal = curOffset * 8;                         //<------------/
        myFwrite( &regVal, sizeof( int32_t ), 1, code );        // x - number of global variables
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

        u_int8_t bufNumOpers[4] = { 0x4a, 0x8b, 0x84, 0xf6 };     // mov rax, [rsi + (r14 + x + y) * 8]
        myFwrite( bufNumOpers, sizeof( u_int8_t ), 4, code  );    //                      |___________|
        numBytesInFile += 4;                                      //                            |
                                                                  //                            |
        int32_t regVal = ( globalNames.curName + curOffset ) * 8; //<---------------------------|
        myFwrite( &regVal, sizeof( int32_t ), 1, code );          // r14 - counter of local variables in the function
        numBytesInFile += 4;                                      // x - number of global variables
                                                                  // y - number of local variables
        
        u_int8_t bufNumOper = 0x50;                               // push rax
        myFwrite( &bufNumOper, sizeof( u_int8_t ), 1, code );
        numBytesInFile += 1;
    }
    else
    {
        u_int8_t bufNumOpers[3] = { 0x48, 0x8b, 0x86 };         // mov rax, [rsi + x * 8]
        myFwrite( bufNumOpers, sizeof( u_int8_t ), 3, code  );  //               |______|
        numBytesInFile += 3;                                    //                   |
                                                                //                   |
        int32_t regVal = curOffset * 8;                         //<------------------|
        myFwrite( &regVal, sizeof( int32_t ), 1, code );        // x - number of global variables
        numBytesInFile += 4;
        
        u_int8_t bufNumOper = 0x50;                             // push rax
        myFwrite( &bufNumOper, sizeof( u_int8_t ), 1, code );
        numBytesInFile += 1;
    }
}

static void ConvertConstNodeInCode( Node_t *node, FILE *code, TableLocalNames *localNames )
{
    assert( node       != nullptr );
    assert( code       != nullptr );
    assert( localNames != nullptr );

    u_int8_t bufNumOper = 0xb8;                           // mov rax, C
    myFwrite( &bufNumOper, sizeof( u_int8_t ), 1, code ); //          |
    numBytesInFile += 1;                                  //          |
                                                          //          |
    int32_t regVal = (int32_t)node->value;                //<---------|
    myFwrite( &regVal, sizeof( int32_t ), 1, code );      // C - constant value
    numBytesInFile += 4;

    bufNumOper = 0x50;                                     // push rax
    myFwrite( &bufNumOper, sizeof( u_int8_t ), 1, code );
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
            u_int8_t bufNumOpers[6] = { 0x5b,             // pop rbx
                                        0x58,             // pop rax
                                        0x48, 0x01, 0xd8, // add rax, rbx
                                        0x50 };           // push rax
            myFwrite( bufNumOpers, sizeof( u_int8_t ), 6, code );
            numBytesInFile += 6;

            break;                               
        }
        case (int32_t)SUB:
        {
            u_int8_t bufNumOpers[6] = { 0x5b,             // pop rbx
                                        0x58,             // pop rax
                                        0x48, 0x29, 0xd8, // sub rax, rbx
                                        0x50 };           // push rax
            myFwrite( bufNumOpers, sizeof( u_int8_t ), 6, code );
            numBytesInFile += 6;

            break;
        }
        case (int32_t)MUL:
        {
            u_int8_t bufNumOpers[5] = { 0x5b,       // pop rbx
                                        0x58,       // pop rax
                                        0xf7, 0xe3, // mul ebx
                                        0x50 };     // push rax
            myFwrite( bufNumOpers, sizeof( u_int8_t ), 5, code );
            numBytesInFile += 5;
        
            break;
        }
        case (int32_t)DIV:
        {
            u_int8_t bufNumOpers[6] = { 0x5b,       // pop rbx
                                        0x58,       // pop rax
                                        0x99,       // cdq
                                        0xf7, 0xfb, // idiv ebx
                                        0x50 };     // push rax
            myFwrite( bufNumOpers, sizeof( u_int8_t ), 6, code );
            numBytesInFile += 6;
        
            break;
        }
        case (int32_t)POW:
        {                                                                                                                               
            u_int8_t bufNumOpers1[11] = { 0x5b,                   // pop rbx
                                          0x58,                   // pop rax
                                          0xf2, 0x0f, 0x2a, 0xc0, // cvtsi2sd xmm0, eax
                                          0xf2, 0x0f, 0x2a, 0xcb, // cvtsi2sd xmm1, ebx
                                          0xe8 };                 // call pow                 
            myFwrite( bufNumOpers1, sizeof( u_int8_t ), 11, code );                                                                         
            numBytesInFile += 11;                                                                                                         
                                                                                                                                      
            u_int16_t curAddressPow = START_ADDRESS_FUNC_POW - numBytesInFile; // call address ( 0xXX 0xXX ____ ____ )                                                         
            myFwrite( &curAddressPow, sizeof( u_int16_t ), 1, code );                                                                       
            numBytesInFile += 2;                                                                                                          
                                                                                                                                      
            u_int8_t bufNumOpers2[4] = { 0xff, 0xff,    // call address ( ____ ____ 0xXX 0xXX )
                                          0x48, 0xbe }; // mov rsi, BUFFER
            myFwrite( bufNumOpers2, sizeof( u_int8_t ), 4, code );                               
            numBytesInFile += 4;

            myFwrite( &ADDRESS_DATA_BUFFER, sizeof( u_int64_t ), 1, code );
            numBytesInFile += 8;

            u_int8_t bufNumOpers3[5] = { 0xf2, 0x0f, 0x2c, 0xc0, // cvttsd2si eax, xmm0
                                          0x50 };                // push rax                                 
            myFwrite( bufNumOpers3, sizeof( u_int8_t ), 5, code );                               
            numBytesInFile += 5;

            break;
        }
        case (int32_t)JG:  { CND_JMP( 0x8f ); }
        case (int32_t)JL:  { CND_JMP( 0x8c ); }
        case (int32_t)JE:  { CND_JMP( 0x84 ); }
        case (int32_t)JGE: { CND_JMP( 0x8d ); }
        case (int32_t)JLE: { CND_JMP( 0x8e ); }
        case (int32_t)JNE: { CND_JMP( 0x85 ); }
        default:           { printf( "Error: invalid binary operations!\n" ); break; }
    }
}

static void ConvertSubtreeInCode( Node_t *node, FILE *code, TableLocalNames *localNames )
{
    assert( node       != nullptr );
    assert( code       != nullptr );
    assert( localNames != nullptr );

    switch ( (int32_t)node->nodeType )
    {
        case (int32_t)DEFINE   : { ConvertDefineNodeInCode         ( node, code, localNames ); return ; }
        case (int32_t)RETURN   : { ConvertReturnNodeInCode         ( node, code, localNames ); return ; }
        case (int32_t)CALL     : { ConvertCallNodeInCode           ( node, code, localNames ); return ; }
        case (int32_t)IF       : { ConvertIfNodeInCode             ( node, code, localNames ); return ; }
        case (int32_t)WHILE    : { ConvertWhileNodeInCode          ( node, code, localNames ); return ; }
        case (int32_t)ASSIGN   : { ConvertAssignNodeInCode         ( node, code, localNames ); return ; }
        case (int32_t)CONST    : { ConvertConstNodeInCode          ( node, code, localNames ); return ; }
        case (int32_t)VARIABLE : { ConvertLocVariableNodeIncode    ( node, code, localNames ); return ; }
        case (int32_t)ADD      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return ; }
        case (int32_t)SUB      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return ; }
        case (int32_t)MUL      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return ; }
        case (int32_t)DIV      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return ; }
        case (int32_t)POW      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return ; }
        case (int32_t)JG       : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return ; }
        case (int32_t)JL       : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return ; }
        case (int32_t)JE       : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return ; }
        case (int32_t)JGE      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return ; }
        case (int32_t)JLE      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return ; }
        case (int32_t)JNE      : { ConvertBinaryOperationNodeInCode( node, code, localNames ); return ; }
        default                : { break; }
    }

    if ( node->leftChild  != nullptr ) { ConvertSubtreeInCode( node->leftChild , code, localNames ); }
    if ( node->rightChild != nullptr ) { ConvertSubtreeInCode( node->rightChild, code, localNames ); }
}

static void ClearCodeFile( FILE *code )
{
    assert( code != nullptr );

    if ( fseek ( code, START_POSITION_IN_FILE, SEEK_SET ) ) {
        printf( "Error: the pointer to the file position has not been moved!\n" );
        return ;
    }

    u_int8_t nopOper = 0x90;
    printf( "%u\n", NUMBERS_NOP );
    for ( size_t i = 0; i < NUMBERS_NOP; i++ )
    {
        myFwrite( &nopOper, sizeof( u_int8_t ), 1, code );
    }
}

static void ResizeUpCodeFile( u_int32_t numberNopForResizeUp )
{   
    FILE *foutput = fopen( NAME_RESIZE_FILE, "a" );
    if ( foutput == nullptr )
    {
        printf( "Error: can`t open code file for resize up!\n" );
        return ;
    }

    fprintf( foutput, "\ntimes %u nop\n\n", numberNopForResizeUp );

    fclose( foutput );
}

void GenerateAsmCode( Tree_t *tree )
{
    assert( tree != nullptr );

    for ( numTreeCrawl = 1; numTreeCrawl <= 2; numTreeCrawl++ )
    {
        printf( "%u\n", numTreeCrawl );
        globalNames.curName = 0;
        curLabel            = 0;
        numBytesInFile      = 0;

        FILE *code = fopen( NAME_OUTPUT_FILE, "r+b" );

        if ( numTreeCrawl == 2 ) { printf( "OK2\n" ); ClearCodeFile( code ); }

        if ( fseek ( code, START_POSITION_IN_FILE, SEEK_SET ) )
        {
            printf( "Error: the pointer to the file position has not been moved!\n" );
            fclose( code );
            return ;
        }

        if ( !FindMain(tree) )
        {
            printf( "Error: no main declaration!\n" );
            fclose( code );
            return ;
        }

        // rsi - variables buffer
        u_int8_t bufNumOpers1[2] = { 0x48, 0xbe }; // mov rsi, BUFFER
        myFwrite( bufNumOpers1, sizeof( u_int8_t ), 2, code );
        numBytesInFile += 2;

        myFwrite( &ADDRESS_DATA_BUFFER, sizeof( u_int64_t ), 1, code );
        numBytesInFile += 8;

        // r14 - counter of local variables in the function
        // r15 - global variable counter for this function
        u_int8_t bufNumOpers2[6] = { 0x4d, 0x31, 0xf6,   // xor r14, r14
                                     0x4d, 0x31, 0xff }; // xor r15, r15
        myFwrite( bufNumOpers2, sizeof( u_int8_t ), 6, code );
        numBytesInFile += 6;

        FillTableFunctions   ( tree->root );
        FindAndPrintGlobalVar( tree, code );

        TableLocalNames localNames = { {}, 0 };

        ConvertSubtreeInCode( tree->root, code, &localNames );

        if ( numBytesInFile > NUMBERS_NOP )
        {   printf( "OK1\n" );
            ResizeUpCodeFile( numBytesInFile - NUMBERS_NOP );
            
            NUMBERS_NOP = numBytesInFile;
            printf( "%u\n", NUMBERS_NOP );

            system( "make -C asmTests/" );
        }

        fclose( code );
    }
}
