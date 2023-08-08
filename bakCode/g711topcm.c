//gcc g711topcm.c g711.c -I . -o g711topcm
//./g711topcm audio.mp3.g711a audio.pcm 0
//ffplay -f s16le -ar 8000 -ac 1 audio.pcm 只能用s16le
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "g711.h"

#define G711_FRAME_SIZE 320

int main(int argc,char **argv)
{
    if(argc != 4)
    {
        printf("argc != 4 error!!!\n");
        return -1;
    }

    char in_buf[G711_FRAME_SIZE] = {0};
    char out_buf[G711_FRAME_SIZE*2] = {0};

    FILE *infp = NULL;
    FILE *outfp = NULL;
    int ret = -1;

    int format = atoi(argv[3]);
    if(1 != format && 0 != format)
    {
        printf("argv[3] format error!!!\n");
        return -1;
    }
    //输入G711文件
    infp = fopen(argv[1],"r+b");
    if(NULL == infp)
    {
        printf("fopen argv[1] = %s fail! error\n",argv[1]);
        return -1;
    }
    //输出PCM文件
    outfp = fopen(argv[2],"a+b");
    if(NULL == outfp)
    {
        printf("fopen argv[2] = %s fail! error\n",argv[2]);
        return -1;
    }

    while((ret = fread(in_buf,1,G711_FRAME_SIZE,infp)))
    {
        //G711有两种格式：G711A(alaw)和G711U(ulaw)
        //format 0:alaw 1:ulaw
        if(0 == format)
            G711_alaw2linear((long)ret,(unsigned char *)in_buf,(short *)out_buf);
        else
            G711_ulaw2linear((long)ret,(unsigned char *)in_buf,(short *)out_buf);
        fwrite(out_buf,ret*2,1,outfp);
    }

    printf("Decode success!\n");

    return 0;
}
