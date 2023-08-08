#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fdk-aac/aacenc_lib.h>

static HANDLE_AACENCODER init_fdk_aac(int sample_rate,int channels,int bitrate)
{
    HANDLE_AACENCODER handle;
	CHANNEL_MODE mode;
    int aot = 2; //lc 低复杂度，算法速度快
    switch (channels) {
	case 1: mode = MODE_1;       break;
	case 2: mode = MODE_2;       break;
	case 3: mode = MODE_1_2;     break;
	case 4: mode = MODE_1_2_1;   break;
	case 5: mode = MODE_1_2_2;   break;
	case 6: mode = MODE_1_2_2_1; break;
	default:
		fprintf(stderr, "Unsupported WAV channels %d\n", channels);
		return NULL;
	}
    //打开aac 编码器
    if (aacEncOpen(&handle, 0, channels) != AACENC_OK) {
		fprintf(stderr, "Unable to open encoder\n");
		return NULL;
	}
    //设置算法模式 lc等
	if (aacEncoder_SetParam(handle, AACENC_AOT, aot) != AACENC_OK) {
		fprintf(stderr, "Unable to set the AOT\n");
		return NULL;
	}
    //设置编码输入帧长
    if (aacEncoder_SetParam(handle, AACENC_GRANULE_LENGTH, 1024) != AACENC_OK) {
		fprintf(stderr, "Unable to set the audio frame length\n");
		return NULL;
	}
    //设置采样率
	if (aacEncoder_SetParam(handle, AACENC_SAMPLERATE, sample_rate) != AACENC_OK) {
		fprintf(stderr, "Unable to set the AOT\n");
		return NULL;
	}
    //声道模式
	if (aacEncoder_SetParam(handle, AACENC_CHANNELMODE, mode) != AACENC_OK) {
		fprintf(stderr, "Unable to set the channel mode\n");
		return NULL;
	}
    //设置pcm数据格式
	if (aacEncoder_SetParam(handle, AACENC_CHANNELORDER, 1) != AACENC_OK) {
		fprintf(stderr, "Unable to set the wav channel order\n");
		return NULL;
	}
    //设置编码码率
    if (aacEncoder_SetParam(handle, AACENC_BITRATE, bitrate) != AACENC_OK) {
        fprintf(stderr, "Unable to set the bitrate\n");
        return NULL;
    }
	//设置编码帧为ADTS格式
	if (aacEncoder_SetParam(handle, AACENC_TRANSMUX, TT_MP4_ADTS) != AACENC_OK) {
		fprintf(stderr, "Unable to set the ADTS transmux\n");
		return NULL;
	}
    //初始化 handle
	if (aacEncEncode(handle, NULL, NULL, NULL, NULL) != AACENC_OK) {
		fprintf(stderr, "Unable to initialize the encoder\n");
		return NULL;
	}
	//获取音频编码信息
    AACENC_InfoStruct g_aacInfo = {0};
	if (aacEncInfo(handle, &g_aacInfo) != AACENC_OK) {
		fprintf(stderr, "Unable to get the encoder info\n");
		return NULL;
	}
 
    return handle;
}

static void deInit_fdk_aac(HANDLE_AACENCODER *pHandle)
{
    aacEncClose(pHandle);
}

static int fdk_aac_enc(HANDLE_AACENCODER *pHandle,uint16_t *pInPcmData,int inputSize,uint8_t *pOutBuff,int outBuffsize)
{
    AACENC_ERROR err;
    AACENC_BufDesc in_buf = { 0 }, out_buf = { 0 };
	AACENC_InArgs in_args = { 0 };
	AACENC_OutArgs out_args = { 0 };
    int in_identifier = IN_AUDIO_DATA;
    int out_identifier = OUT_BITSTREAM_DATA;
    int in_size = inputSize, in_elem_size = 2;
    while(1)
    {
        void *in_ptr = pInPcmData;
		//输入参数和输入buff配置
        in_args.numInSamples = inputSize <= 0 ? -1 : inputSize/in_elem_size;//输入数据中所有声道采样点个数
		in_buf.numBufs = 1;   //输入buff个数
		in_buf.bufs = &in_ptr; //输入PCM音频地址
		in_buf.bufferIdentifiers = &in_identifier;//输入
		in_buf.bufSizes = &in_size;  //输入PCM大小
		in_buf.bufElSizes = &in_elem_size;//每个采样点数据类型(大小)short
        //输出参数和输出buff配置
		void * out_ptr = pOutBuff; 
		int out_size = outBuffsize;
		int out_elem_size = 1;   //输出每个
		out_buf.numBufs = 1; //输出buff个数
		out_buf.bufs = &out_ptr;//输出buff的地址
		out_buf.bufferIdentifiers = &out_identifier;
		out_buf.bufSizes = &out_size;//输出buff的大小
		out_buf.bufElSizes = &out_elem_size;//输出数据类型(大小)char
        if ((err = aacEncEncode(*pHandle, &in_buf, &out_buf, &in_args, &out_args)) != AACENC_OK) {
			if (err == AACENC_ENCODE_EOF)
				break;
			fprintf(stderr, "Encoding failed\n");
			return 0;
		}
		if (out_args.numOutBytes == 0)
			continue;
        break;
    }
 
    return out_args.numOutBytes;
}

char *ReadAllFile(char *file, int *fsize) {
    int file_size;
    char *tmp;
    FILE *fp = fopen(file, "rb");
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

int main() {
    HANDLE_AACENCODER handle = init_fdk_aac(11025, 1, 16000);

    int ids;
    char *inData = ReadAllFile("audioNew.pcm", &ids);
    printf("filesize = %d \n", ids);

    FILE* outfp = fopen("audioNew.aac", "wb");
    if(NULL == outfp) {
        printf("fopen audioNew.aac fail!\n");
        return -1;
    }

    char *outData = (char*)malloc(ids * sizeof(char));
    memset(outData, '\0', ids * sizeof(char));

    //static int fdk_aac_enc(HANDLE_AACENCODER *pHandle,uint16_t *pInPcmData,int inputSize,uint8_t *pOutBuff,int outBuffsize)
    int ods = fdk_aac_enc(&handle, (uint16_t *)inData, ids, (uint8_t *)outData, ids);
    fwrite(outData, ids, 1, outfp);
    
    deInit_fdk_aac(&handle);
}
