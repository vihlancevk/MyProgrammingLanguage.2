#ifndef DSL_H_
#define DSL_H_ 

#define CND_JMP( byte )                                                         \
    do                                                                          \
    {                                                                           \
        u_int8_t bufNumOpers[7] = { 0x5b,             /* pop rbx      */        \
                                    0x58,             /* pop rax      */        \
                                    0x48, 0x39, 0xd8, /* cmp rax, rbx */        \
                                    0x0f, byte };     /* J_ or J__    */        \
        fwrite( bufNumOpers, sizeof( u_int8_t ), 7, code );                     \
        numBytesInFile += 7;                                                    \
                                                                                \
        numBytesInFile += 4;                                                    \
        u_int32_t numLabel = GenerateLabel();                                   \
        if ( numTreeCrawl == 1 ) { labelsAddress[numLabel] -= numBytesInFile; } \
        int32_t addressJmp = labelsAddress[numLabel]; /* jump address */        \
        myFwrite( &addressJmp, sizeof( int32_t ), 1, code );                    \
    } while( 0 );                                                               \
    break

#endif // DSL_H_
