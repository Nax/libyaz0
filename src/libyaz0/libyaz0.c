#include <stdlib.h>
#include "libyaz0.h"

int yaz0Init(Yaz0Stream** ptr)
{
    Yaz0Stream* s;

    s = malloc(sizeof(*s));
    if (!s)
        return YAZ0_OUT_OF_MEMORY;
    s->mode = MODE_NONE;
    s->cursorOut = 0;
    s->decompSize = 0;
    *ptr = s;
    return YAZ0_OK;
}

int yaz0Destroy(Yaz0Stream* stream)
{
    free(stream);
    return YAZ0_OK;
}

int yaz0Run(Yaz0Stream* s)
{
    switch (s->mode)
    {
    case MODE_NONE:
        return YAZ0_OK;
    case MODE_DECOMPRESS:
        return yaz0_RunDecompress(s);
    case MODE_COMPRESS:
        return yaz0_RunCompress(s);
    }
}

int yaz0Input(Yaz0Stream* stream, const void* data, size_t size)
{
    stream->in = data;
    stream->sizeIn = size;
    stream->cursorIn = 0;

    return YAZ0_OK;
}

int yaz0Output(Yaz0Stream* stream, void* data, size_t size)
{
    stream->out = data;
    stream->sizeOut = size;
    stream->cursorOut = 0;
    return YAZ0_OK;
}

uint32_t yaz0OutputChunkSize(const Yaz0Stream* stream)
{
    return stream->cursorOut;
}

uint32_t yaz0DecompressedSize(const Yaz0Stream* stream)
{
    return stream->decompSize;
}
