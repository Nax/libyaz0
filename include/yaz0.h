#ifndef YAZ0_H
#define YAZ0_H

#include <stddef.h>
#include <stdint.h>

#define YAZ0_OK             0
#define YAZ0_NEED_AVAIL_IN  1
#define YAZ0_NEED_AVAIL_OUT 2
#define YAZ0_BAD_MAGIC      (-1)
#define YAZ0_OUT_OF_MEMORY  (-2)

typedef struct Yaz0Stream Yaz0Stream;

int yaz0InitDecompress(Yaz0Stream** stream);
int yaz0InitCompress(Yaz0Stream** stream, uint32_t size);
int yaz0Destroy(Yaz0Stream* stream);
int yaz0Run(Yaz0Stream* stream);
int yaz0Input(Yaz0Stream* stream, const void* data, size_t size);
int yaz0Output(Yaz0Stream* stream, void* data, size_t size);

size_t yaz0OutputChunkSize(const Yaz0Stream* stream);

#endif /* YAZ0_H */
