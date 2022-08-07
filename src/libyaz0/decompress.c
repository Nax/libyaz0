#include <string.h>
#include <stdio.h>
#include "libyaz0.h"

int yaz0InitDecompress(Yaz0Stream** stream)
{
    int ret;

    ret = yaz0_Init(stream);
    if (ret)
        return ret;
    return YAZ0_OK;
}

void loadAux(Yaz0Stream* stream, uint32_t size)
{
    if (stream->sizeIn - stream->cursorIn < size)
        size = stream->sizeIn - stream->cursorIn;
    for (uint32_t i = 0; i < size; i++)
        stream->auxBuf[stream->auxSize++] = stream->in[stream->cursorIn++];
}

int flush(Yaz0Stream* stream)
{
    size_t chunkSize;
    size_t outSize;

    if (stream->window_start == stream->window_end)
        return YAZ0_OK;
    if (stream->cursorOut >= stream->sizeOut)
        return YAZ0_NEED_AVAIL_OUT;
    outSize = stream->sizeOut - stream->cursorOut;
    if (stream->window_start > stream->window_end)
    {
        /* Wrap around - we will need 2 copies */
        chunkSize = WINDOW_SIZE - stream->window_start;
        if (chunkSize > outSize)
            chunkSize = outSize;
        memcpy(stream->out + stream->cursorOut, stream->window + stream->window_start, chunkSize);
        stream->cursorOut += chunkSize;
        stream->window_start += chunkSize;
        stream->window_start %= WINDOW_SIZE;
        outSize -= chunkSize;
    }
    /* Copy the rest */
    chunkSize = stream->window_end - stream->window_start;
    if (chunkSize > outSize)
        chunkSize = outSize;
    memcpy(stream->out + stream->cursorOut, stream->window + stream->window_start, chunkSize);
    stream->cursorOut += chunkSize;
    stream->window_start += chunkSize;
    return YAZ0_OK;
}

static int yaz0_ReadHeaders(Yaz0Stream* stream)
{
    loadAux(stream, 16 - stream->auxSize);
    if (stream->auxSize < 16)
        return YAZ0_NEED_AVAIL_IN;
    if (memcmp(stream->auxBuf, "Yaz0", 4))
        return YAZ0_BAD_MAGIC;
    stream->decompSize = swap32(*(uint32_t*)&stream->auxBuf[4]);
    stream->auxSize = 0;
    return YAZ0_OK;
}

static uint32_t windowFreeSize(Yaz0Stream* stream)
{
    uint32_t size;
    if (stream->window_start > stream->window_end)
        size = WINDOW_SIZE - stream->window_start + stream->window_end;
    else
        size = stream->window_end - stream->window_start;
    return WINDOW_SIZE - size;
}

static int ensureWindowFree(Yaz0Stream* stream)
{
    static const uint32_t maxSize = 0x111 * 8;

    if (windowFreeSize(stream) < maxSize)
    {
        flush(stream);
        if (windowFreeSize(stream) < maxSize)
            return YAZ0_NEED_AVAIL_OUT;
    }
    return YAZ0_OK;
}

int yaz0_DoDecompress(Yaz0Stream* stream)
{
    uint8_t groupBit;
    char    byte;
    int     ret;
    int     n;
    int     r;
    for (;;)
    {
        /* No group and no EOF - We need to read */
        if (stream->groupCount == 0)
        {
            /* Before we read, we eant to ensure the window is large enough */
            /* This will avoid a lot of checks and let us write the whole group */
            ret = ensureWindowFree(stream);
            if (ret)
                return ret;
            if (stream->cursorIn >= stream->sizeIn)
                return YAZ0_NEED_AVAIL_IN;
            stream->groupHeader = stream->in[stream->cursorIn++];
            stream->groupCount = 8;
        }

        /* We have a group! */
        while (stream->groupCount)
        {
            /* Get the group bit */
            groupBit = stream->groupHeader & (1 << (stream->groupCount - 1));
            if (groupBit)
            {
                /* Group bit is 1 - direct write */
                if (stream->cursorIn >= stream->sizeIn)
                    return YAZ0_NEED_AVAIL_IN;

                /* Everything ok, copy the byte */
                byte = stream->in[stream->cursorIn++];
                stream->window[stream->window_end++] = byte;
                stream->window_end %= WINDOW_SIZE;
                stream->totalOut++;
            }
            else
            {
                if (stream->auxSize < 2)
                {
                    loadAux(stream, 2 - stream->auxSize);
                    if (stream->auxSize < 2)
                        return YAZ0_NEED_AVAIL_IN;
                }
                /* Do we need 2 or 3 bytes? */
                n = ((uint8_t)stream->auxBuf[0]) >> 4;
                if (!n)
                {
                    /* We have a large chunk */
                    loadAux(stream, 1);
                    if (stream->auxSize < 3)
                        return YAZ0_NEED_AVAIL_IN;
                    n = (uint8_t)stream->auxBuf[2] + 0x12;
                }
                else
                {
                    /* We have a small chunk */
                    n = ((uint8_t)stream->auxBuf[0] >> 4) + 2;
                }
                r = ((uint16_t)((uint8_t)stream->auxBuf[0] & 0x0f) << 8) | ((uint8_t)stream->auxBuf[1]);
                r++;
                /* Reset the aux buffer */
                uint32_t cursor = (stream->window_end + WINDOW_SIZE - r) % WINDOW_SIZE;
                for (int i = 0; i < n; ++i)
                {
                    stream->window[stream->window_end++] = stream->window[cursor++];
                    stream->window_end %= WINDOW_SIZE;
                    cursor %= WINDOW_SIZE;
                }
                stream->auxSize = 0;
                stream->totalOut += n;
            }
            stream->groupCount--;
            /* Check for EOF */
            if (stream->totalOut >= stream->decompSize)
                return YAZ0_OK;
        }
    }
}

int yaz0_RunDecompress(Yaz0Stream* stream)
{
    int ret;

    /* Check the headers */
    if (!(stream->flags & FLAG_HEADERS_PARSED))
    {
        ret = yaz0_ReadHeaders(stream);
        if (ret)
            return ret;
        stream->flags |= FLAG_HEADERS_PARSED;
    }

    if (stream->totalOut < stream->decompSize)
    {
        /* We did not decompress everything */
        ret = yaz0_DoDecompress(stream);
        if (ret)
            return ret;
    }

    /* We did decompress everything */
    flush(stream);
    if (stream->window_start != stream->window_end)
        return YAZ0_NEED_AVAIL_OUT;
    return YAZ0_OK;
}
