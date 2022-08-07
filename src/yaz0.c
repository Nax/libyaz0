#include <stdio.h>
#include <string.h>
#include <yaz0.h>

#define BUFSIZE 0x1000

static int run(const char* inPath, const char* outPath, int compress)
{
    int ret;
    int err;
    size_t size;
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
        err = 1;
        goto end;
    };
    out = fopen(outPath, "wb");
    if (!out)
    {
        fprintf(stderr, "Could not open `%s'\n", outPath);
        err = 1;
        goto end;
    }
    if (compress)
    {
        fseek(in, 0, SEEK_END);
        size = ftell(in);
        fseek(in, 0, SEEK_SET);
        if (size > 0xffffffff)
        {
            fprintf(stderr, "%s: file too large\n", inPath);
            err = 1;
            goto end;
        }
        ret = yaz0InitCompress(&stream, (uint32_t)size);
    }
    else
        ret = yaz0InitDecompress(&stream);
    if (ret != YAZ0_OK)
    {
        fprintf(stderr, "Could not init libyaz0\n");
        err = 1;
        goto end;
    }
    yaz0Output(stream, bufferOut, BUFSIZE);
    size = fread(bufferIn, 1, BUFSIZE, in);
    yaz0Input(stream, bufferIn, size);
    for (;;)
    {
        ret = yaz0Run(stream);
        switch (ret)
        {
        case YAZ0_BAD_MAGIC:
            fprintf(stderr, "%s: Bad magic\n", inPath);
            err = 1;
            goto end;
        case YAZ0_OK:
            goto last;
            break;
        case YAZ0_NEED_AVAIL_IN:
            size = fread(bufferIn, 1, BUFSIZE, in);
            if (size == 0)
            {
                fprintf(stderr, "%s: Abrupt end of file\n", inPath);
                err = 1;
                goto end;
            }
            yaz0Input(stream, bufferIn, size);
            break;
        case YAZ0_NEED_AVAIL_OUT:
            fwrite(bufferOut, yaz0OutputChunkSize(stream), 1, out);
            yaz0Output(stream, bufferOut, BUFSIZE);
            break;
        }
    }
last:
    fwrite(bufferOut, yaz0OutputChunkSize(stream), 1, out);
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
    char outFile[4096];
    const char* inFile;
    int compress;
    int autoOutFile;

    inFile = NULL;
    compress = 1;
    autoOutFile = 1;

    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '-')
        {
            if (strcmp(argv[i], "-d") == 0)
            {
                compress = 0;
            }
            else if (strcmp(argv[i], "-o") == 0)
            {
                i++;
                if (argc == i || (strlen(argv[i]) == 0))
                {
                    fprintf(stderr, "Missing argument for -o\n");
                    return 1;
                }
                strcpy(outFile, argv[i]);
                autoOutFile = 0;
            }
            else
            {
                usage(argv[0]);
                return 1;
            }
        }
        else
        {
            if (!inFile)
                inFile = argv[i];
            else
            {
                usage(argv[0]);
                return 1;
            }
        }
    }

    if (!inFile)
    {
        usage(argv[0]);
        return 0;
    }

    if (autoOutFile)
    {
        strcpy(outFile, inFile);
        if (compress)
            strcat(outFile, ".yaz0");
        else
        {
            char* ext = strrchr(outFile, '.');
            if (ext && !strcmp(ext, ".yaz0"))
                *ext = 0;
            else
                strcat(outFile, ".out");
        }
    }
    return run(inFile, outFile, compress);
}
