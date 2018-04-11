#ifndef BOOL_H
#define BOOL_H

typedef int bool;
#define true 1
#define false 0

#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <zstd.h>

void handle_err(char * err)
{
    fputs(err, stderr);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

bool do_compress = true;

int compression_level = 1;

static void compress(int infd, int outfd)
{
    size_t const inbuff_size = ZSTD_CStreamInSize();
    void * inbuff = (void *)malloc(inbuff_size);

    if(inbuff == NULL)
        handle_err("Could not allocate in buffer");

    size_t const outbuff_size = ZSTD_CStreamOutSize();
    void * outbuff = (void *)malloc(outbuff_size);

    if(outbuff == NULL)
        handle_err("Could not allocate out buffer");

    ZSTD_CStream * const cstream = ZSTD_createCStream();

    if(cstream == NULL)
        handle_err("ZSTD_createCStream error");

    size_t const initresult = ZSTD_initCStream(cstream, compression_level);
    if(ZSTD_isError(initresult)) handle_err("ZSTD_initCStream() err" /* ZSTD_getErrorName(initresult) */);

    size_t readlen, toread = inbuff_size, totalread = 0;
    size_t writelen, totalwrite = 0; 

    for(;;totalread += readlen){
        readlen = pread(infd, inbuff, toread, totalread);

        if(readlen == 0) break; //We have finished reading the input

        ZSTD_inBuffer input = {inbuff, readlen, 0};
        for(;input.pos < input.size; totalwrite += writelen){
            ZSTD_outBuffer output = {outbuff, outbuff_size, 0};
            toread = ZSTD_compressStream(cstream, &output, &input);
            if(ZSTD_isError(toread)) handle_err("ZSTD_compressStream() err" /* ZSTD_getErrorName(toread) */);

            if(toread > inbuff_size) toread = inbuff_size;

            writelen = pwrite(outfd, outbuff, output.pos, totalwrite);
        }
    }

    ZSTD_outBuffer output = {outbuff, outbuff_size, 0};
    size_t const remaining = ZSTD_endStream(cstream, &output);
    if(remaining) handle_err("not fully flushed");

    writelen = pwrite(outfd, outbuff, output.pos, totalwrite);

    totalwrite += writelen;

    ZSTD_freeCStream(cstream);

    free(inbuff);
    free(outbuff);
}

static void decompress(int infd, int outfd)
{
    size_t const inbuff_size = ZSTD_DStreamInSize();
    void * inbuff = (void *)malloc(inbuff_size);

    if(inbuff == NULL)
        handle_err("Could not allocate in buffer");

    size_t const outbuff_size = ZSTD_DStreamOutSize();
    void * outbuff = (void *)malloc(outbuff_size);

    if(outbuff == NULL)
        handle_err("Could not allocate out buffer");

    ZSTD_DStream* const dstream = ZSTD_createDStream();
    if(dstream == NULL)
        handle_err("ZSTD_createDStream() error");

    size_t readlen, toread = inbuff_size, totalread = 0;
    size_t writelen, totalwrite = 0; 

    for(;;totalread += readlen){
        readlen = pread(infd, inbuff, toread, totalread);

        if(readlen == 0) break; //We have finished reading the input

        ZSTD_inBuffer input = {inbuff, readlen, 0};
        for(;input.pos < input.size; totalwrite += writelen){
            ZSTD_outBuffer output = {outbuff, outbuff_size, 0};
            toread = ZSTD_decompressStream(dstream, &output, &input);
            if(ZSTD_isError(toread)) handle_err("ZSTD_compressStream() err" /* ZSTD_getErrorName(toread) */);

            if(toread > inbuff_size) toread = inbuff_size;

            writelen = pwrite(outfd, outbuff, output.pos, totalwrite);
        }
    }

    ZSTD_freeDStream(dstream);

    free(inbuff);
    free(outbuff);
}

int main(int argc, char * argv[])
{
    int opt;
    int long_index = 0;
    static struct option long_options[] = {
        {"c",       no_argument, &do_compress, true},
        {"d",       no_argument, &do_compress, false},
        {"in",      required_argument,  0,  'i'}, 
        {"out",     required_argument,  0,  'o'}, 
        {0, 0, 0, 0}
    };

    char * in_path = NULL, * out_path = NULL;

    while ((opt = getopt_long_only(argc, argv, "i:o:", long_options, &long_index)) != -1) 
    {
        switch (opt) 
        {
            case 0: break;
            case 'i': in_path = optarg; break;
            case 'o': out_path = optarg; break;
            case '?': break;
            default:
                fprintf(stderr, "Usage: %s -in <input file path> -out <output file path>", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    int infd = (in_path == NULL) ? STDIN_FILENO : open(in_path, O_RDONLY);
    if(infd == -1)
        handle_err("failed to open read file");

    int outfd = (out_path == NULL) ? STDOUT_FILENO : open(out_path, O_RDWR|O_CREAT, S_IRUSR|S_IRGRP|S_IROTH);
    if(outfd == -1)
        handle_err("failed to open write file");

    if(do_compress)
        compress(infd, outfd);
    else
        decompress(infd, outfd);

    close(infd);
    close(outfd);
}
