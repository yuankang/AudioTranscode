//gcc resample.c -lsoxr -I . -o resample
//ffplay -f s16le -ar 8000 -ac 1 audio.pcm
//ffplay -f s16le -ar 11025 -ac 1 audioNew.pcm
#include <soxr.h>
#include "examples-common.h"

char *ReadAllFile(char *file, int *fsize) {
    int file_size;
    char *tmp;
    FILE *fp = fopen(file, "r");
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    printf("文件字符数为 %d\n", file_size);

    tmp = (char*)malloc(file_size * sizeof(char));
    memset(tmp, '\0', file_size * sizeof(char));

    fseek(fp, 0, SEEK_SET);
    fread(tmp, sizeof(char), file_size, fp);
    //printf("内容:%s\n", tmp);
    *fsize = file_size;
    return tmp;
}

//输入文件, 输出文件, 输入采样率, 输出采样率, 通道数, 数据格式, 质量
int resample(char *ifile, char *ofile, double isr, double osr, int chan, int fmt, int q)
{
    int ids;
    char *inData = ReadAllFile(ifile, &ids);
    printf("filesize = %d \n", ids);

    FILE *outfp = fopen(ofile,"wb");
    if(NULL == outfp) {
        printf("fopen argv[2] = %s fail! error\n", ofile);
        return -1;
    }

    soxr_error_t error;
    soxr_io_spec_t io_spec = soxr_io_spec(fmt, fmt);

    /* Create a stream resampler: */
    soxr_t soxr = soxr_create(isr, osr, 1, &error, &io_spec, NULL, NULL);
    if (error) {
        printf("error\n");
        return -1;
    }

    size_t frameSize = 16 / 8;
    size_t fragment = ids % (frameSize * chan);
    size_t framesIn = ids / frameSize / chan;
    size_t framesOut = framesIn * (osr / isr);
    printf("fragment=%ld, framesIn=%ld, framesOut=%ld \n", fragment, framesIn, framesOut);

    int ods = framesOut * chan * frameSize;
    char *outData = malloc(ods);

    size_t read = 0;
    size_t done = 0;
    do {
        error = soxr_process(soxr, inData, framesIn, &read, outData, framesOut, &done);
        if (error) {
            printf("error\n");
            return -1;
        }

        if (read == framesIn && done < framesOut) {
            printf("111");
            done = framesOut;
        }
    } while (done < framesOut);

    fwrite(outData, ods, 1, outfp);

    soxr_delete(soxr);
    free(inData), free(outData);
    fprintf(stderr, "%-26s %s; I/O: %s\n", "resample", soxr_strerror(error),
            ferror(stdin) || ferror(stdout)? strerror(errno) : "no error");
    return !!error;
}

int main(int argc, char const * arg[]) {
    resample("audio.pcm", "audioNew.pcm", 8000, 11025, 1, 3, 0);
}
