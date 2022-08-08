#include <string.h>
#include <stdio.h>
#include "libyaz0.h"

static uint32_t hash(uint8_t a, uint8_t b, uint8_t c)
{
    uint32_t x = (uint32_t)a | ((uint32_t)b << 8) | ((uint32_t)c << 16);
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

static uint32_t hashAt(Yaz0Stream* s, uint32_t offset)
{
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint32_t start;

    start = s->window_start + offset;
    a = s->window[(start + 0) % WINDOW_SIZE];
    b = s->window[(start + 1) % WINDOW_SIZE];
    c = s->window[(start + 2) % WINDOW_SIZE];
    return hash(a, b, c);
}

static void hashWrite(Yaz0Stream* s, uint32_t h, uint32_t offset)
{
    uint32_t bucket;
    uint32_t tmpBucket;
    uint32_t oldest;
    uint32_t entry;

    oldest = 0xffffffff;
    for (int i = 0; i < HASH_MAX_PROBES; ++i)
    {
        tmpBucket = (h + i) % HASH_MAX_ENTRIES;
        entry = s->htEntries[tmpBucket];
        //printf("ENTRY: 0x%04x\n", entry);
        if (entry == 0xffffffff)
        {
            bucket = tmpBucket;
            break;
        }
        if (entry < oldest)
        {
            oldest = entry;
            bucket = tmpBucket;
        }
    }
    if (entry != 0xffffffff)
    {
        //printf("Evicted oldest\n");
    }
    //printf("BUCKET: 0x%04x ENTRY: 0x%08x NEW: 0x%08x\n", bucket, entry, s->totalOut + offset);
    s->htEntries[bucket] = s->totalOut + offset;
    s->htHashes[bucket] = h;
}

static uint32_t maxSize(Yaz0Stream* stream)
{
    uint32_t max;
    max = stream->decompSize - stream->totalOut;
    if (max > 0x2000)
        max = 0x2000;
    return (int)max;
}

/* start: start of avail data */
/* end: end of avail data */
/* We need to write more data at the end of the window */

static int feed(Yaz0Stream* s)
{
    uint32_t avail;
    uint32_t min;
    uint32_t max;
    uint32_t size;
    int ret;

    /* Check how much data we have */
    if (s->window_start > s->window_end)
        avail = WINDOW_SIZE - s->window_start + s->window_end;
    else
        avail = s->window_end - s->window_start;
    if (avail >= maxSize(s))
        return YAZ0_OK;

    /* We need more data */
    min = maxSize(s) - avail;
    max = WINDOW_SIZE - 0x1000 - avail;
    if (max > s->sizeIn - s->cursorIn)
        max = s->sizeIn - s->cursorIn;
    if (max < min)
        ret = YAZ0_NEED_AVAIL_IN;
    else
        ret = YAZ0_OK;
    if (s->window_end + max >= WINDOW_SIZE)
    {
        /* We might need two copies */
        size = WINDOW_SIZE - s->window_end;
        memcpy(s->window + s->window_end, s->in + s->cursorIn, size);
        s->cursorIn += size;
        s->window_end = 0;
        max -= size;
    }
    memcpy(s->window + s->window_end, s->in + s->cursorIn, max);
    s->cursorIn += max;
    s->window_end += max;
    return ret;
}

static int matchSize(Yaz0Stream* s, uint32_t pos)
{
    uint32_t size = 0;
    uint32_t cursorA = (s->window_start + WINDOW_SIZE - pos) % WINDOW_SIZE;
    uint32_t cursorB = s->window_start;
    uint32_t maxSize;

    maxSize = s->decompSize - s->totalOut;
    if (maxSize > 0x111)
        maxSize = 0x111;
    for (;;)
    {
        if (s->window[cursorA] != s->window[cursorB])
            break;
        size++;
        if (size == maxSize)
            break;
        cursorA++;
        cursorB++;
        cursorA %= WINDOW_SIZE;
        cursorB %= WINDOW_SIZE;
    }
    //printf("MATCH END A:0x%04x B:0x%04x\n", cursorA, cursorB);
    return size;
}

static void findHashMatch(Yaz0Stream* s, uint32_t h, uint32_t* outSize, uint32_t* outPos)
{
    uint32_t bucket;
    uint32_t entry;
    uint32_t bestSize;
    uint32_t bestPos;
    uint32_t size;
    uint32_t pos;

    bestSize = 0;
    bestPos = 0;
    for (int i = 0; i < HASH_MAX_PROBES; ++i)
    {
        bucket = (h + i) % HASH_MAX_ENTRIES;
        entry = s->htEntries[bucket];
        if (entry == 0xffffffff)
            break;
        if (s->htHashes[bucket] == h)
        {
            pos = s->totalOut - entry;
            if (pos > 0x1000)
                continue;
            size = matchSize(s, pos);
            //printf("POS 0x%04x SIZE 0x%04x\n", pos, size);
            if (size > bestSize)
            {
                bestSize = size;
                bestPos = pos;
            }
        }
    }
    //printf("BS 0x%04x\n", bestSize);
    if (bestSize < 3)
    {
        bestSize = 0;
        bestPos = s->window[s->window_start];
    }
    else
    {
        //printf("O:0x%08x S:0x%04x P:0x%04x\n", s->totalOut, bestSize, bestPos);
    }
    *outSize = bestSize;
    *outPos = bestPos;
}

static void emitGroup(Yaz0Stream* s, int count, const uint32_t* arrSize, const uint32_t* arrPos)
{
    uint8_t header;
    int size;
    int pos;

    header = 0;
    for (int i = 0; i < count; ++i)
    {
        if (!arrSize[i])
            header |= (1 << (7 - i));
    }
    s->out[s->cursorOut++] = header;
    for (int i = 0; i < count; ++i)
    {
        size = arrSize[i];
        pos = arrPos[i];
        if (!size)
            s->out[s->cursorOut++] = (uint8_t)pos;
        else
        {
            pos--;
            if (size >= 0x12)
            {
                /* 3 bytes */
                s->out[s->cursorOut++] = (uint8_t)(pos >> 8);
                s->out[s->cursorOut++] = (uint8_t)pos;
                s->out[s->cursorOut++] = (uint8_t)(size - 0x12);
            }
            else
            {
                /* 2 bytes */
                s->out[s->cursorOut++] = (uint8_t)(pos >> 8) | ((size - 2) << 4);
                s->out[s->cursorOut++] = (uint8_t)pos;
            }
        }
    }
}

static void compressGroup(Yaz0Stream* s)
{
    int groupCount;
    uint32_t h;
    uint32_t size;
    uint32_t pos;
    uint32_t arrSize[8];
    uint32_t arrPos[8];

    for (groupCount = 0; groupCount < 8; ++groupCount)
    {
        h = hashAt(s, 0);
        findHashMatch(s, h, &size, &pos);
        hashWrite(s, h, 0);
        arrSize[groupCount] = size;
        arrPos[groupCount] = pos;
        if (!size)
        {
            s->window_start += 1;
            s->totalOut += 1;
        }
        else
        {
            for (uint32_t i = 1; i < size; ++i)
            {
                h = hashAt(s, i);
                hashWrite(s, h, i);
            }
            s->window_start += size;
            s->totalOut += size;
        }
        s->window_start %= WINDOW_SIZE;
        if (s->totalOut >= s->decompSize)
        {
            groupCount++;
            break;
        }
    }
    emitGroup(s, groupCount, arrSize, arrPos);
}

int yaz0InitCompress(Yaz0Stream** ptr, uint32_t size)
{
    int ret;

    ret = yaz0_Init(ptr);
    if (ret != YAZ0_OK)
        return ret;
    (*ptr)->flags = FLAG_COMPRESS;
    (*ptr)->decompSize = size;
    return YAZ0_OK;
}

int yaz0_RunCompress(Yaz0Stream* stream)
{
    uint32_t tmp;
    int ret;

    /* Write headers */
    if (!(stream->flags & FLAG_HEADERS_PARSED))
    {
        if (stream->sizeOut < 16)
            return YAZ0_NEED_AVAIL_OUT;
        memcpy(stream->out, "Yaz0", 4);
        tmp = swap32(stream->decompSize);
        memcpy(stream->out + 4, &tmp, 4);
        tmp = 0;
        memcpy(stream->out + 8, &tmp, 4);
        memcpy(stream->out + 12, &tmp, 4);
        stream->cursorOut += 16;
        stream->flags |= FLAG_HEADERS_PARSED;
    }

    /* Compress */
    for (;;)
    {
        /* Check EOF */
        if (stream->totalOut >= stream->decompSize)
            return YAZ0_OK;

        /* Check output space */
        if (stream->sizeOut - stream->cursorOut < 1 + 8 * 3)
            return YAZ0_NEED_AVAIL_OUT;

        /* Check that we have consumed enough input */
        ret = feed(stream);
        if (ret)
            return ret;

        /* Compress one chunk */
        compressGroup(stream);
    }
}
