#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fdk-aac/aacdecoder_lib.h>
#include <iostream>
using namespace std;

int main() {
    printf("111\n");
    HANDLE_AACDECODER handle;
    //handle = aacDecoder_Open(TT_MP4_RAW , 1);
    handle = aacDecoder_Open(TT_MP4_ADTS, 1);
    int conceal_method=2;//0 muting 1 noise 2 interpolation
//UCHAR eld_conf[] = { 0xF8, 0xE8, 0x50, 0x00 };//ste eld 44.1k
//11111 000111 0100 0010 10000 0000 0000
//32    7      4    2    其他
//UCHAR ld_conf[] = { 0xBA, 0x88, 0x00, 0x00 };//mono ld 32k
//UCHAR ld_conf[] = { 0xBD, 0x10, 0x00, 0x00 };//mono ld 32k
//10111 0101 0001 000 00000000 00000000
//23    5    1   其他
//10111 1010 0001 000 00000000 00000000
//23    a    1   其他
//UCHAR *conf[] = { ld_conf };
//static UINT conf_len = sizeof(ld_conf);
//AAC_DECODER_ERROR err = aacDecoder_ConfigRaw(handle, conf, &conf_len);
//if(err>0)
//cout<<"conf err:"<<err<<endl;
    AAC_DECODER_ERROR err;
    aacDecoder_SetParam(handle, AAC_CONCEAL_METHOD,conceal_method);
    aacDecoder_SetParam(handle,  AAC_PCM_MAX_OUTPUT_CHANNELS,1); //MONO:1
    aacDecoder_SetParam(handle,  AAC_PCM_MIN_OUTPUT_CHANNELS,1); //MONO:1
    printf("222\n");


    const char *lengthfile,*aacfile,*pcmfile;
    lengthfile="length.txt";
    aacfile="111.aac";
    pcmfile="111.pcm";
    unsigned int size=4096;
    unsigned int valid;
    FILE* lengthHandle;
    FILE* aacHandle;
    FILE* pcmHandle;
    lengthHandle = fopen(lengthfile, "rb");
    aacHandle = fopen(aacfile, "rb");
    pcmHandle = fopen(pcmfile, "wb");
    unsigned char* data=(unsigned char*)malloc(size);
    unsigned int decsize=2048 * sizeof(INT_PCM);
    unsigned char* decdata=(unsigned char*)malloc(decsize);
    printf("333\n");

    int frameCnt=0;
    do{
        printf("444\n");
        /*
           int  ll = fread(&size, 1,2, lengthHandle);
           if(ll <1)
           break;
           */
        printf("size %d\n",size);
        fread(data, 1,size, aacHandle);
        valid=size;
        err=aacDecoder_Fill(handle, &data, &size, &valid);
        if(err > 0)
            cout<<"fill err:"<<err<<endl;
        // aacDecoder_SetParam(handle, AAC_TPDEC_CLEAR_BUFFER, 1);
        err = aacDecoder_DecodeFrame(handle, (INT_PCM *)decdata,decsize / sizeof(INT_PCM), 0);
        if(err> 0)
            cout<<"dec err:"<<err<<endl;
        if(err != 0)
            break;
        CStreamInfo *info = aacDecoder_GetStreamInfo(handle);
        /* cout<<"channels"<<info->numChannels<<endl;
           cout<<"sampleRate"<<info->sampleRate<<endl;
           cout<<"frameSize"<<info->frameSize<<endl;
           cout<<"decsize"<<decsize<<endl;
           cout<<"decdata"<<decdata[0]<<endl;*/

        fwrite(decdata,1,info->frameSize*2,pcmHandle);
        frameCnt++;
    }while(true);
    cout<<"frame count:"<<frameCnt<<endl;
    fflush(pcmHandle);
    fclose(lengthHandle);
    fclose(aacHandle);
    fclose(pcmHandle);
    aacDecoder_Close(handle);
    return 0;
}
