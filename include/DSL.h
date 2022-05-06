#ifndef DSL_H_
#define DSL_H_ 

#define ADD( byte )                                                       \
    do                                                                    \
    {                                                                     \
        u_int8_t bufNumOpers[6] = { 0x5b, 0x58, 0x48, byte, 0xd8, 0x50 }; \
        fwrite( bufNumOpers, sizeof( u_int8_t ), 6, code );               \
        numBytesInFile += 6;                                              \
    } while( 0 );                                                         \
    break

#define SUB( byte ) \
    ADD( byte )

#define MUL()                                                       \
    do                                                              \
    {                                                               \
        u_int8_t bufNumOpers[5] = { 0x5b, 0x58, 0xf7, 0xe3, 0x50 }; \
        fwrite( bufNumOpers, sizeof( u_int8_t ), 5, code );         \
        numBytesInFile += 5;                                        \
    } while ( 0 );                                                  \
    break
    
#define DIV()                                                             \
    do                                                                    \
    {                                                                     \
        u_int8_t bufNumOpers[6] = { 0x5b, 0x58, 0x99, 0xf7, 0xfb, 0x50 }; \
        fwrite( bufNumOpers, sizeof( u_int8_t ), 6, code );               \
        numBytesInFile += 6;                                              \
    } while ( 0 );                                                        \
    break

#define POW()                                                                                                                         \
    do                                                                                                                                \
    {                                                                                                                                 \
        u_int8_t bufNumOpers1[11] = { 0x5b, 0x58, 0xf2, 0x0f, 0x2a, 0xc0, 0xf2, 0x0f, 0x2a, 0xcb, 0xe8 };                             \
        fwrite( bufNumOpers1, sizeof( u_int8_t ), 11, code );                                                                         \
        numBytesInFile += 11;                                                                                                         \
                                                                                                                                      \
        u_int16_t curAddressPow = START_ADDRESS_POW - numBytesInFile;                                                                 \
        fwrite( &curAddressPow, sizeof( u_int16_t ), 1, code );                                                                       \
        numBytesInFile += 2;                                                                                                          \
                                                                                                                                      \
        u_int8_t bufNumOpers2[17] = { 0xff, 0xff, 0x48, 0xbe, 0x48, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf2, 0x0f, 0x2c, 0xc0, \
                                      0x50 };                                                                                         \
        fwrite( bufNumOpers2, sizeof( u_int8_t ), 17, code );                                                                         \
        numBytesInFile += 17;                                                                                                         \
    } while ( 0 );                                                                                                                    \
    break

#define CND_JMP( byte )                                                         \
    do                                                                          \
    {                                                                           \
        u_int8_t bufNumOpers[7] = { 0x5b, 0x58, 0x48, 0x39, 0xd8, 0x0f, byte }; \
        fwrite( bufNumOpers, sizeof( u_int8_t ), 7, code );                     \
        numBytesInFile += 7;                                                    \
                                                                                \
        numBytesInFile += 4;                                                    \
        u_int32_t numLabel = GenerateLabel();                                   \
        if ( numTreeCrawl == 1 ) { labelsAddress[numLabel] -= numBytesInFile; } \
        int32_t addressJmp = labelsAddress[numLabel];                           \
        fwrite( &addressJmp, sizeof( int32_t ), 1, code );                      \
    } while( 0 );                                                               \
    break

#endif // DSL_H_
