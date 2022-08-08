#ifndef LIBYAZ0_H
#define LIBYAZ0_H

#include <stdint.h>
#include <yaz0.h>

#define WINDOW_SIZE             0x4000
#define FLAG_COMPRESS           (1 << 0)
#define FLAG_HEADERS_PARSED     (1 << 1)
#define HASH_MAX_ENTRIES        0x8000
#define HASH_REBUILD            0x2000
#define HASH_MAX_PROBES         0x1000

struct Yaz0Stream
{
    int             flags;
    int             level;
    uint32_t        predictionDepth;
    uint32_t        decompSize;
    uint32_t        totalOut;
    const uint8_t*  in;
    uint8_t*        out;
    uint32_t        sizeIn;
    uint32_t        sizeOut;
    uint32_t        cursorIn;
    uint32_t        cursorOut;
    uint8_t         auxBuf[16];
    uint8_t         auxSize;
    uint8_t         groupHeader;
    uint8_t         groupCount;
    uint32_t        window_start;
    uint32_t        window_end;
    uint8_t         window[WINDOW_SIZE];
    uint32_t        htSize;
    uint32_t        htHashes[HASH_MAX_ENTRIES];
    uint32_t        htEntries[HASH_MAX_ENTRIES];
};

int yaz0_Init(Yaz0Stream** stream);
int yaz0_RunDecompress(Yaz0Stream* stream);
int yaz0_RunCompress(Yaz0Stream* stream);

uint32_t swap32(uint32_t v);

#endif /* LIBYAZ0_H */
