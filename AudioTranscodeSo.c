#include <stdint.h> //linux uint8_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <soxr.h>
#include <fdk-aac/aacenc_lib.h>
#include <fdk-aac/aacdecoder_lib.h>
#include "g711.h"

#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#define AAC_ENC_MAX_OUTPUT_SIZE (768 * 8) //@8ch
#define G711_FRAME_SIZE 320

HANDLE_AACENCODER AacEncoder = NULL;

char *ReadAllFile(char *file, int *fsize) {
    int file_size;
    char *tmp;
    FILE *fp = fopen(file, "r");
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);

    tmp = (char*)malloc(file_size * sizeof(char));
    memset(tmp, '\0', file_size * sizeof(char));

    fseek(fp, 0, SEEK_SET);
    fread(tmp, sizeof(char), file_size, fp);
    *fsize = file_size;
    fclose(fp);
    return tmp;
}

//format 0:alaw 1:ulaw
int g711x2pcm(char *ifile, char *ofile, int format) {
    FILE *infp = NULL;
    FILE *outfp = NULL;
    int ret = -1;
    char in_buf[G711_FRAME_SIZE] = {0};
    char out_buf[G711_FRAME_SIZE*2] = {0};

    if(1 != format && 0 != format) {
        printf("unknow g771a or g771u\n");
        return -1;
    }

    infp = fopen(ifile, "rb");
    if(NULL == infp) {
        printf("fopen %s fail! error\n", ifile);
        return -1;
    }
    outfp = fopen(ofile, "wb");
    if(NULL == outfp) {
        printf("fopen %s fail! error\n", ofile);
        return -1;
    }

    while((ret = fread(in_buf,1,G711_FRAME_SIZE,infp))) {
        //G711有两种格式：G711A(alaw)和G711U(ulaw)
        if(0 == format)
            G711_alaw2linear((long)ret,(unsigned char *)in_buf,(short *)out_buf);
        else
            G711_ulaw2linear((long)ret,(unsigned char *)in_buf,(short *)out_buf);
        fwrite(out_buf,ret*2,1,outfp);
    }

    fclose(infp);
    fclose(outfp);
    return 0;
}

typedef enum PcmQ_E {
    Quick     = 0, // Quick cubic interpolation
    LowQ      = 1, // LowQ 16-bit with larger rolloff
    MediumQ   = 2, // MediumQ 16-bit with medium rolloff
    HighQ     = 4, // High quality
    VeryHighQ = 6, // Very high quality
}PcmQ;

typedef enum PcmFmt_E {
    F32LE = 0, // 32-bit floating point PCM
    F64LE = 1, // 64-bit floating point PCM
    S32LE = 2, // 32-bit signed linear PCM
    S16LE = 3, // 16-bit signed linear PCM
}PcmFmt;

//输入文件, 输出文件, 输入采样率, 输出采样率, 通道数, 数据格式, 质量
int resample(char *ifile, char *ofile, double isr, double osr, int chan, int fmt, int q) {
    int ids;
    char *inData = ReadAllFile(ifile, &ids);
    printf("filesize = %d \n", ids);

    FILE *outfp = fopen(ofile,"wb");
    if(NULL == outfp) {
        printf("fopen %s fail! error\n", ofile);
        return -1;
    }

    soxr_error_t err;
    soxr_io_spec_t io_spec = soxr_io_spec(fmt, fmt);
    soxr_quality_spec_t q_spec = soxr_quality_spec(q, 0);

    /* Create a stream resampler: */
    soxr_t soxr = soxr_create(isr, osr, 1, &err, &io_spec, &q_spec, NULL);
    if (err) {
        printf("error\n");
        return -1;
    }

    size_t frameSize = 16 / 8; //s16le
    size_t fragment = ids % (frameSize * chan);
    size_t framesIn = ids / frameSize / chan;
    size_t framesOut = framesIn * (osr / isr);
    printf("fragment=%ld, framesIn=%ld, framesOut=%ld \n", fragment, framesIn, framesOut);

    int ods = framesOut * frameSize * chan;
    char *outData = malloc(ods);

    size_t read = 0, done = 0, d = 0;
    do {
        err = soxr_process(soxr, inData, framesIn, &read, outData, framesOut, &done);
        if (err) {
            printf("error\n");
            return -1;
        }

        if (read == framesIn && done < framesOut) {
            err = soxr_process(soxr, NULL, 0, NULL, outData, framesOut, &d);
            if (err) {
                printf("error\n");
                return -1;
            }
            done += d;
            break;
        }
    } while (done < framesOut);
    printf("done=%ld, d=%ld, framesOut=%ld\n", done, d, framesOut);

    fwrite(outData, done*frameSize*chan, 1, outfp);

    soxr_delete(soxr);
    free(inData);
    free(outData);
    fclose(outfp);
    return 0;
}

typedef enum AvAacEncFmt_E {
    AvAacEncFmt_AACLC		= 0,
    AvAacEncFmt_EAAC		= 1,
    AvAacEncFmt_EAACPLUS	= 2,
    AvAacEncFmt_AACLD		= 3,
    AvAacEncFmt_AACELD		= 4,
}AvAacEncFmt;

typedef enum AvAacEncTransType_E {
    AvAacEncTransType_ADTS		= 0,
    AvAacEncTransType_LOAS		= 1,
    AvAacEncTransType_LATM_MCP1 = 2,
}AvAacEncTransType;

typedef struct AvAacEncConfig_T {
    AvAacEncFmt format;
    AvAacEncTransType trans_type;
    short bits_per_sample;
    int sample_rate;
    int bit_rate;
    short in_channels;
    short band_width;
} AvAacEncConfig;

const char *aacError2String(AACENC_ERROR e) {
    switch (e) {
        case AACENC_OK: return "No error";
        case AACENC_INVALID_HANDLE: return "Invalid handle";
        case AACENC_MEMORY_ERROR: return "Memory allocation error";
        case AACENC_UNSUPPORTED_PARAMETER: return "Unsupported parameter";
        case AACENC_INVALID_CONFIG: return "Invalid config";
        case AACENC_INIT_ERROR: return "Initialization error";
        case AACENC_INIT_AAC_ERROR: return "AAC library initialization error";
        case AACENC_INIT_SBR_ERROR: return "SBR library initialization error";
        case AACENC_INIT_TP_ERROR: return "Transport library initialization error";
        case AACENC_INIT_META_ERROR: return "Metadata library initialization error";
        case AACENC_ENCODE_ERROR: return "Encoding error";
        case AACENC_ENCODE_EOF: return "End of file";
        default: return "Unknown error";
    }
}

int aacOpenEncoder(AvAacEncConfig AacConf, int *PcmFrameLen) {
    AACENC_ERROR e = aacEncOpen(&AacEncoder, 0, AacConf.in_channels);
    if (e != AACENC_OK) {
        fprintf(stderr, "Unable to open encoder\n");
        return e;
    }

    int aot;
    switch (AacConf.format) {
        default:
        case AvAacEncFmt_AACLC:		aot = AOT_AAC_LC;		break;
        case AvAacEncFmt_EAAC:		aot = AOT_SBR;			break;
        case AvAacEncFmt_EAACPLUS:	aot = AOT_PS;			break;
        case AvAacEncFmt_AACLD:		aot = AOT_ER_AAC_LD;	break;
        case AvAacEncFmt_AACELD:	aot = AOT_ER_AAC_ELD;	break;
    }
    e = aacEncoder_SetParam(AacEncoder, AACENC_AOT, aot);
    if (e != AACENC_OK) {
        fprintf(stderr, "Unable to set the AOT\n");
        return e;
    }

    int eld_sbr = 0;
    if (aot == AOT_ER_AAC_ELD && eld_sbr) {
        e = aacEncoder_SetParam(AacEncoder, AACENC_SBR_MODE, 1);
        if (e != AACENC_OK) {
            fprintf(stderr, "Unable to set SBR mode for ELD\n");
            return e;
        }
    }

    e = aacEncoder_SetParam(AacEncoder, AACENC_SAMPLERATE, AacConf.sample_rate);
    if (e != AACENC_OK) {
        fprintf(stderr, "Unable to set the sample rate\n");
        return e;
    }

    int mode;
    switch (AacConf.in_channels) {
        case 1: mode = MODE_1;       break;
        case 2: mode = MODE_2;       break;
        case 3: mode = MODE_1_2;     break;
        case 4: mode = MODE_1_2_1;   break;
        case 5: mode = MODE_1_2_2;   break;
        case 6: mode = MODE_1_2_2_1; break;
        default: return -1;
    }
    e = aacEncoder_SetParam(AacEncoder, AACENC_CHANNELMODE, mode);
    if (e != AACENC_OK) {
        fprintf(stderr, "Unable to set the channel mode\n");
        return e;
    }

    int vbr = 0;
    if (vbr) {
        e = aacEncoder_SetParam(AacEncoder, AACENC_BITRATEMODE, vbr);
        if (e != AACENC_OK) {
            fprintf(stderr, "Unable to set the VBR bitrate mode\n");
            return e;
        }
    } else {
        e = aacEncoder_SetParam(AacEncoder, AACENC_BITRATEMODE, 0);
        if (e != AACENC_OK) {
            fprintf(stderr, "Unable to set the VBR bitrate mode\n");
            return e;
        }
        e = aacEncoder_SetParam(AacEncoder, AACENC_BITRATE, AacConf.bit_rate);
        if (e != AACENC_OK) {
            fprintf(stderr, "Unable to set the bitrate\n");
            return e;
        }
    }

    // trans type
    TRANSPORT_TYPE tt;
    switch (AacConf.trans_type) {
        default:
        case AvAacEncTransType_ADTS: tt = TT_MP4_ADTS; break;
        case AvAacEncTransType_LOAS: tt = TT_MP4_LOAS; break;
        case AvAacEncTransType_LATM_MCP1: tt = TT_MP4_LATM_MCP1; break;
    }
    e = aacEncoder_SetParam(AacEncoder, AACENC_TRANSMUX, tt);
    if (e != AACENC_OK) {
        fprintf(stderr, "Unable to set the ADTS transmux\n");
        return e;
    }

    e = aacEncoder_SetParam(AacEncoder, AACENC_AFTERBURNER, 1);
    if (e != AACENC_OK) {
        fprintf(stderr, "Unable to set the afterburner mode\n");
        return e;
    }

    // band width
    if (AacConf.band_width > 0) {
        int min_bw = (AacConf.sample_rate + 255) >> 8;
        int max_bw = 20000;
        if (AacConf.band_width < min_bw) AacConf.band_width = min_bw;
        if (AacConf.band_width > max_bw) AacConf.band_width = max_bw;
        e = aacEncoder_SetParam(AacEncoder, AACENC_BANDWIDTH, AacConf.band_width);
        if (e != AACENC_OK) {
            printf("set bandwidth fail\n");
            return e;
        }
    }

    // initialize
    e = aacEncEncode(AacEncoder, NULL, NULL, NULL, NULL);
    if (e != AACENC_OK) {
        fprintf(stderr, "Unable to initialize the encoder\n");
        return e;
    }

    AACENC_InfoStruct info = { 0 };
    e = aacEncInfo(AacEncoder, &info);
    if (e != AACENC_OK) {
        fprintf(stderr, "Unable to get the encoder info\n");
        return e;
    }

    *PcmFrameLen = info.inputChannels * info.frameLength * 2;
    return AACENC_OK;
}

int aacEncode(unsigned char *aac, unsigned char *pcm, int r, int PcmFrameLen) {
    AACENC_BufDesc in_buf = { 0 };
    AACENC_BufDesc out_buf = { 0 };
    AACENC_InArgs in_args = { 0 };
    AACENC_OutArgs out_args = { 0 };
    AACENC_ERROR e;

    //config in_buf and out_buf
    unsigned char ancillary_buf[64];
    AACENC_MetaData meta_data_setup;
    void *ibufs[] = { pcm, ancillary_buf, &meta_data_setup };
    int ibuf_ids[] = { IN_AUDIO_DATA, IN_ANCILLRY_DATA, IN_METADATA_SETUP };
    int ibuf_sizes[] = { PcmFrameLen, sizeof(ancillary_buf), sizeof(meta_data_setup) };
    int ibuf_element_sizes[] = { sizeof(unsigned char), sizeof(unsigned char), sizeof(AACENC_MetaData) };
    in_buf.numBufs = sizeof(ibufs) / sizeof(void *);
    in_buf.bufs = (void **)&ibufs;
    in_buf.bufSizes = ibuf_sizes;
    in_buf.bufElSizes = ibuf_element_sizes;
    in_buf.bufferIdentifiers = ibuf_ids;

    void *obufs[] = { aac };
    int obuf_ids[] = { OUT_BITSTREAM_DATA };
    int obuf_sizes[] = { AAC_ENC_MAX_OUTPUT_SIZE };
    int obuf_element_sizes[] = { sizeof(unsigned char) };
    out_buf.numBufs = sizeof(obufs) / sizeof(void *);
    out_buf.bufs = (void **)&aac;
    out_buf.bufSizes = obuf_sizes;
    out_buf.bufElSizes = obuf_element_sizes;
    out_buf.bufferIdentifiers = obuf_ids;

    in_args.numAncBytes = 0;
    in_args.numInSamples = AAC_ENC_MAX_OUTPUT_SIZE;

    // start encode
    e = aacEncEncode(AacEncoder, &in_buf, &out_buf, &in_args, &out_args);
    if (e != AACENC_OK) {
        if (e == AACENC_ENCODE_EOF) {
            return 0; // eof
        }
        return -1;
    }
    return out_args.numOutBytes;
}

//https://blog.csdn.net/KayChanGEEK/article/details/103355961
//https://github.com/WangLCG/LiveStream/blob/master/PCM2AAC/fdk-aac/main.cpp
//https://blog.csdn.net/u010140427/article/details/127765173
int pcm2aac(char *pcmfile, char *aacfile) {
    int PcmFrameLen;
    AvAacEncConfig AacConf;
    AacConf.format = AvAacEncFmt_AACLC;
    AacConf.trans_type = AvAacEncTransType_ADTS;
    AacConf.bits_per_sample = 16;
    AacConf.sample_rate = 11025;
    AacConf.bit_rate = 64000;
    AacConf.in_channels = 1;
    //AacConf.band_width = 10000;
    AacConf.band_width = 0;

    aacOpenEncoder(AacConf, &PcmFrameLen);

    FILE *ifile = fopen(pcmfile, "rb");
    FILE *ofile = fopen(aacfile, "wb");
    if (ifile == NULL || ofile == NULL) {
        printf("fopen fail\n");
        return -1;
    }

    int r = -1, olen = 0;
    unsigned char *ibuf = (unsigned char *)malloc(PcmFrameLen);
    unsigned char obuf[AAC_ENC_MAX_OUTPUT_SIZE];
    do {
        memset(ibuf, 0, PcmFrameLen);
        r = fread(ibuf, 1, PcmFrameLen, ifile);
        if (r > 0) {
            //memset(obuf, 0, sizeof(obuf));
            memset(obuf, 0, AAC_ENC_MAX_OUTPUT_SIZE);
            olen = aacEncode(obuf, ibuf, r, PcmFrameLen);
            fwrite(obuf, 1, olen, ofile);
        }
    } while (r > 0);

    free(ibuf);
    fclose(ifile);
    fclose(ofile);
    aacEncClose(&AacEncoder);
    return 0;
}

/*
解码库会分配一个过渡性的decoder-internal input buffer
其大小由RANSPORTDEC_INBUF_SIZE规定, 可以任意设定但必须满足两个条件:
1 each input channel requires 768 bytes
2 the whole buffer must be of size 2^n
So for example a stereo decoder:
  TRANSPORTDEC_INBUF_SIZE = 2 * 768 = 1536 => 2048（选择2048bytes）

aacDecoder_Fill() 就把输入数据拷贝到decoder-internal input buffer
返回的是输入数据中还剩下多少没有被拷贝的数据(pBytesValid)
aacDecoder_DecodeFrame() 用来解码decoder-internal input buffer数据
如果数据不足以解码 则返回AAC_DEC_NOT_ENOUGH_BITS, 
此时应该继续调用aacDecoder_Fill填充数据
*/
//TT_MP4_RAW
//https://blog.csdn.net/qq_41106435/article/details/128935705
//https://blog.csdn.net/dong_beijing/article/details/87939673
//TT_MP4_ADTS
//https://www.5axxw.com/questions/simple/9uj1en
//https://www.5axxw.com/questions/simple/ayroci
int aac2pcm(char *ifile, char *ofile) {
    HANDLE_AACDECODER AacDecoder;
    AAC_DECODER_ERROR err;
    FILE *infile, *outfile;
    int16_t *OutBuff;
    uint8_t *InBuff;
    int OutBuffSize, InBuffSize;
    uint32_t SampleRate = 11025, ChannelNum = 1;
    uint32_t n = 0, inputSize = 2048;
    uint32_t pBytesValid;

    infile = fopen(ifile, "rb");
    if (infile == NULL) {
        fprintf(stderr, "Failed to open input file: %s\n", ifile);
        return -1;
    }
    outfile = fopen(ofile, "wb");
    if (outfile == NULL) {
        fprintf(stderr, "Failed to open output file: %s\n", ofile);
        return -1;
    }

    //TT_MP4_RAW = 0, TT_MP4_ADIF = 1, TT_MP4_ADTS = 2,
    AacDecoder = aacDecoder_Open(TT_MP4_ADTS, 1);
    if (!AacDecoder) {
        fprintf(stderr, "Failed to open AAC decoder\n");
        return -1;
    }
    //0:muting, 1:noise, 2interpolation;
    err = aacDecoder_SetParam(AacDecoder, AAC_CONCEAL_METHOD, 2);
    if (err != AAC_DEC_OK) {
        fprintf(stderr, "Failed to set AAC_CONCEALMETHOD\n");
        return -1;
    }
	aacDecoder_SetParam(AacDecoder,  AAC_PCM_MAX_OUTPUT_CHANNELS,1);
	aacDecoder_SetParam(AacDecoder,  AAC_PCM_MIN_OUTPUT_CHANNELS,1);

    // Allocate input buffer
    InBuffSize = inputSize * sizeof(uint8_t);
    InBuff = (uint8_t *) malloc(InBuffSize);
    // Allocate output buffer
    OutBuffSize = inputSize * ChannelNum;
    OutBuff = (int16_t *) malloc(OutBuffSize * sizeof(int16_t));

    int WriteSize = SampleRate * ChannelNum * (16 / 8);
    CStreamInfo *info;
    while (1) {
        n = fread(InBuff, 1, inputSize, infile);
        //fprintf(stderr, "fread num %d\n", n);
        if (n <= 0) {
            //fprintf(stderr, "fread num %d, exit\n", n);
            break;
        }

        pBytesValid = n;
        //该函数是将输入的AAC数据拷贝到解码器内部的内存中
        //InBuff是输入数据的地址, n为输入数据的大小
        //pBytesValid是输入数据没有拷贝完剩余的大小
        //注意当pBytesValid不为0时, 剩余的数据要和下一次AAC帧一起组合再次拷贝到解码器内部的内存中。
        err = aacDecoder_Fill(AacDecoder, &InBuff, &n, &pBytesValid);
        if (err != AAC_DEC_OK) {
            fprintf(stderr, "aacDecoder_Fill failed: %d\n", err);
            return -1;
        }

        while (1) {
            //该函数用于获取解码后的音频数据(通常是PCM)
            //OutBuff为解码后输出的音频数据(PCM)的buff
            //OutBuffSize为参数OutBuff的大小
            err = aacDecoder_DecodeFrame(AacDecoder, OutBuff, OutBuffSize, 0);
            if (err == AAC_DEC_NOT_ENOUGH_BITS) {
                break;
            }
            if (err != AAC_DEC_OK) {
                fprintf(stderr, "Failed to decode AAC frame: %d\n", err);
                return -1;
            }

            //该函数用于获取解码的信息, 主要包含
            //解码后的音频数据的采样率 sampleRate
            //解码后的音频数据通道数 numChannels
            //解码后的帧大小(采样点个数) frameSize
            //info = aacDecoder_GetStreamInfo(AacDecoder);
            //SampleRate=11025, ChannelNum=1, FrameSize=1024
            //fprintf(stderr, "SampleRate=%d, ChannelNum=%d, FrameSize=%d\n", info->sampleRate, info->numChannels, info->frameSize);
            //fwrite(OutBuff, WriteSize, 1, outfile); //应该用这个才对啊
            fwrite(OutBuff, 1, OutBuffSize, outfile); //为什么用这个对?
            memset(OutBuff, '\0', OutBuffSize);
        }
        memset(InBuff, '\0', InBuffSize);
    }

    fclose(infile);
    fclose(outfile);
    free(InBuff);
    free(OutBuff);
    aacDecoder_Close(AacDecoder);
    return 0;
}

//g711x -> aac 11025Hz
int G711xToAac(void const *iData, int iType, void *oData) {
    return 0;
}

//aac -> aac 11025Hz
int AacToAac(void const *iData, int iSampleRate, void *oData) {
    return 0;
}

int Test() {
    struct timeval start , end;
    gettimeofday(&start , NULL);

    int ret = -1;
    //8000Hz g711 -> 8000Hz pcm, 都是单声道
    ret = g711x2pcm("audio/000.g711a", "audio/000.pcm", 0);
    //ret = g711x2pcm("audio.g711u", "666.pcm", 1);
    printf("=== g711x2pcm ret=%d\n", ret);

    //8000Hz pcm -> 11025Hz pcm
    ret = resample("audio/000.pcm", "audio/111.pcm", 8000, 11025, 1, S16LE, HighQ);
    printf("=== resample ret=%d\n", ret);

    //11025Hz pcm -> 11025Hz aac
    ret = pcm2aac("audio/111.pcm", "audio/222.aac");
    printf("=== pcm2aac ret=%d\n", ret);

    //11025Hz aac -> 11025Hz pcm
    ret = aac2pcm("audio/222.aac", "audio/333.pcm");
    printf("=== aac2pcm ret=%d\n", ret);

    gettimeofday(&end , NULL);
    long cost = (end.tv_sec - start.tv_sec) * 1000 * 1000 + (end.tv_usec - start.tv_usec) ;
    printf("cost time: %ld us\n", cost);
    return 0;
}
