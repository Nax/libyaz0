#include <string.h>
#include <stdio.h>
#include "libyaz0.h"

static int maxSize(Yaz0Stream* stream)
{
    uint32_t max;
    max = stream->decompSize - stream->totalOut;
    if (max > 0x888)
        max = 0x888;
    return (int)max;
}

/* start: start of avail data */
/* end: end of avail data */
/* We need to write more data at the end of the window */

static int feed(Yaz0Stream* s)
{
    size_t avail;
    size_t min;
    size_t max;
    size_t size;
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

static int matchSize(Yaz0Stream* s, int pos)
{
    int size = 0;
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
    return size;
}

static void findBestMatch(Yaz0Stream* s, int* size, int* pos)
{
    int bestSize;
    int bestPos;

    bestSize = 0;
    for (int i = 0x1000; i > 0; --i)
    {
        int tmpSize = matchSize(s, i);
        if (tmpSize < 3)
            continue;
        if (tmpSize > bestSize)
        {
            bestSize = tmpSize;
            bestPos = i;
        }
        i -= (tmpSize - 1);
    }

    if (bestSize)
    {
        *size = bestSize;
        *pos = bestPos;
        s->window_start = (s->window_start + bestSize) % WINDOW_SIZE;
        s->totalOut += bestSize;
    }
    else
    {
        *size = 0;
        *pos = (uint8_t)s->window[s->window_start++];
        s->window_start %= WINDOW_SIZE;
        s->totalOut++;
    }
}

static void emitGroup(Yaz0Stream* s, int count, const int* arrSize, const int* arrPos)
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
    int arrSize[8];
    int arrPos[8];

    for (groupCount = 0; groupCount < 8; ++groupCount)
    {
        findBestMatch(s, &arrSize[groupCount], &arrPos[groupCount]);
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
