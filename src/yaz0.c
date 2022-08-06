#include <stdio.h>
#include <string.h>
#include <yaz0.h>

#define BUFSIZE 0x1000

static int decompress(const char* inPath, const char* outPath)
{
    int ret;
    int err;
    FILE* in;
    FILE* out;
    Yaz0Stream* stream;
    char bufferIn[BUFSIZE];
    char bufferOut[BUFSIZE];

    in = NULL;
    out = NULL;
    stream = NULL;

    err = 0;
    in = fopen(inPath, "rb");
    if (!in)
    {
        fprintf(stderr, "Could not open `%s'\n", inPath);
        err = -1;
        goto end;
    };
    out = fopen(outPath, "wb");
    if (!out)
    {
        fprintf(stderr, "Could not open `%s'\n", outPath);
        err = -1;
        goto end;
    }
    ret = yaz0InitDecompress(&stream);
    if (ret != YAZ0_OK)
    {
        fprintf(stderr, "Could not init libyaz0\n");
        err = -1;
        goto end;
    }
    yaz0Output(stream, bufferOut, BUFSIZE);
    size_t size = fread(bufferIn, 1, BUFSIZE, in);
    yaz0Input(stream, bufferIn, size);
    for (;;)
    {
        ret = yaz0Run(stream);
        switch (ret)
        {
        case YAZ0_BAD_MAGIC:
            fprintf(stderr, "%s: Bad magic\n", inPath);
            err = -1;
            goto end;
        case YAZ0_OK:
            goto last;
            break;
        case YAZ0_NEED_AVAIL_IN:
            size = fread(bufferIn, 1, BUFSIZE, in);
            if (size == 0)
            {
                fprintf(stderr, "%s: Abrupt end of file\n", inPath);
                err = -1;
                goto end;
            }
            yaz0Input(stream, bufferIn, size);
            break;
        case YAZ0_NEED_AVAIL_OUT:
            fwrite(bufferOut, 1, yaz0OutputChunkSize(stream), out);
            yaz0Output(stream, bufferOut, BUFSIZE);
            break;
        }
    }
last:
    fwrite(bufferOut, 1, yaz0OutputChunkSize(stream), out);
end:
    if (stream)
        yaz0Destroy(stream);
    if (in)
        fclose(in);
    if (out)
        fclose(out);
    return err;
}

static void usage(const char* program)
{
    printf("usage: %s [-d] [-l level] [-o output] input\n", program);
}

int main(int argc, char** argv)
{
    char namebuf[4096];

    if (argc != 2)
    {
        usage(argv[0]);
        return 0;
    }

    strcpy(namebuf, argv[1]);
    char* ext = strrchr(namebuf, '.');
    if (ext && !strcmp(ext, ".yaz0"))
        *ext = 0;
    else
        strcat(namebuf, ".out");
    decompress(argv[1], namebuf);
    return 0;
}
