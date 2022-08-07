#include <stdlib.h>
#include "libyaz0.h"

int yaz0Destroy(Yaz0Stream* stream)
{
    free(stream);
    return YAZ0_OK;
}

int yaz0Run(Yaz0Stream* stream)
{
    if (stream->flags & FLAG_COMPRESS)
        return yaz0_RunCompress(stream);
    else
        return yaz0_RunDecompress(stream);
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

int yaz0_Init(Yaz0Stream** ptr)
{
    Yaz0Stream* s;

    s = calloc(1, sizeof(*s));
    if (!s)
        return YAZ0_OUT_OF_MEMORY;
    *ptr = s;
    return YAZ0_OK;
}

size_t yaz0OutputChunkSize(const Yaz0Stream* stream)
{
    return stream->cursorOut;
}
