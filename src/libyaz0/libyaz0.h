#ifndef LIBYAZ0_H
#define LIBYAZ0_H

#include <stdint.h>
#include <yaz0.h>

#define WINDOW_SIZE             0x4000
#define FLAG_COMPRESS           (1 << 0)
#define FLAG_HEADERS_PARSED     (1 << 1)

struct Yaz0Stream
{
    int         flags;
    int         level;
    uint32_t    decompSize;
    uint32_t    totalOut;
    const char* in;
    char*       out;
    size_t      sizeIn;
    size_t      sizeOut;
    size_t      cursorIn;
    size_t      cursorOut;
    char        auxBuf[16];
    uint8_t     auxSize;
    uint8_t     groupHeader;
    uint8_t     groupCount;
    int         window_cursor_read;
    int         window_cursor_write;
    char        window[WINDOW_SIZE];
};

int yaz0_Init(Yaz0Stream** stream);
int yaz0_RunDecompress(Yaz0Stream* stream);

#endif /* LIBYAZ0_H */
