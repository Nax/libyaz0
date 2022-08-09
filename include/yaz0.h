#ifndef YAZ0_H
#define YAZ0_H

#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
# define YAZ0_API extern "C"
#else
# define YAZ0_API
#endif

#define YAZ0_OK             0
#define YAZ0_NEED_AVAIL_IN  1
#define YAZ0_NEED_AVAIL_OUT 2
#define YAZ0_BAD_MAGIC      (-1)
#define YAZ0_OUT_OF_MEMORY  (-2)

typedef struct Yaz0Stream Yaz0Stream;

YAZ0_API int yaz0InitDecompress(Yaz0Stream** stream);
YAZ0_API int yaz0InitCompress(Yaz0Stream** stream, uint32_t size);
YAZ0_API int yaz0Destroy(Yaz0Stream* stream);
YAZ0_API int yaz0Run(Yaz0Stream* stream);
YAZ0_API int yaz0Input(Yaz0Stream* stream, const void* data, size_t size);
YAZ0_API int yaz0Output(Yaz0Stream* stream, void* data, size_t size);

YAZ0_API size_t yaz0OutputChunkSize(const Yaz0Stream* stream);

#endif /* YAZ0_H */
