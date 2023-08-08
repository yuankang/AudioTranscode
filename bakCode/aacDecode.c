#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fdk-aac/aacdecoder_lib.h>
#define INPUT_BUF_SIZE  2048
#define MAX_CHANNEL_NUM 6       // 最大通道数
#define MAX_SAMPLE_RATE 48000   // 最大采样率
int main(int argc, char *argv[])
{
    uint8_t *input_buf = NULL;
    int      buf_size = 0;
    uint8_t *input_ptr = NULL;
    HANDLE_AACDECODER h_aac_decoder = NULL;
    CStreamInfo       stream_info;
    int              aac_frame_size = 0;
    int              ret = 0;
    // 读取AAC音频数据并存入input_buf
	FILE *ifile = fopen("111.aac", "rb");
    // 初始化AAC解码器
    h_aac_decoder = aacDecoder_Open(TT_MP4_ADTS, 1);
    if (!h_aac_decoder) {
        printf("Failed to open AAC decoder\n");
        return -1;
    }
    // 读取AAC音频数据并解码直到输入完整一帧
    input_buf = (uint8_t*)malloc(INPUT_BUF_SIZE);
    input_ptr = input_buf;
    buf_size = INPUT_BUF_SIZE;
	int copy_size = -1;
    while (buf_size > 0 && aac_frame_size == 0) {
        // 将一部分数据拷贝至input_buf中
		copy_size = fread(input_buf, 1, INPUT_BUF_SIZE, ifile);
        buf_size -= copy_size;
        // 向AAC解码器输入数据
        const unsigned char *in_data_ptrs[1] = { input_buf };
        int in_data_sizes[1] = { copy_size };
        ret = aacDecoder_Fill(h_aac_decoder, in_data_ptrs, in_data_sizes, buf_size);
        if (ret != AAC_DEC_OK) {
            printf("aacDecoder_Fill error: %d\n", ret);
            break;
        }
        // 解码获取stream_info和完整帧的大小
        ret = aacDecoder_DecodeFrame(h_aac_decoder, NULL, 0, 0);
        if (ret != AAC_DEC_OK && ret != AAC_DEC_NOT_ENOUGH_BITS) {
            printf("aacDecoder_DecodeFrame error: %d\n", ret);
            break;
        }
        // 获取当前帧的Size
        aac_frame_size = aacDecoder_AvailableSamples(h_aac_decoder);
        if (aac_frame_size > 0) {
            // 获取流信息
            aacDecoder_GetStreamInfo(h_aac_decoder, &stream_info);
        }
        // 移动输入指针到还未解码的数据位置
        input_ptr += copy_size - buf_size;
        memcpy(input_buf, input_ptr, buf_size);
        input_ptr = input_buf;
    }
    if (ret != AAC_DEC_OK) {
        printf("Decode error.\n");
        return -1;
    }
    // 解码并输出音频数据
    int16_t pcm_buf[MAX_CHANNEL_NUM * MAX_SAMPLE_RATE];
    memset(pcm_buf, 0, sizeof(pcm_buf));
    ret = aacDecoder_DecodeFrame(h_aac_decoder, (void*)pcm_buf, MAX_SAMPLE_RATE, 0);
    if (ret != AAC_DEC_OK) {
        printf("aacDecoder_DecodeFrame error: %d\n", ret);
        return -1;
    }
    // 输出解码后的音频数据至文件或音频设备
    // ...
    aacDecoder_Close(h_aac_decoder);
    free(input_buf);
    return 0;
}
