#include <stdio.h>
#include <stdlib.h>
#include <fdk-aac/aacdecoder_lib.h>

uint32_t kMaxOutBufferSize = 2048;

typedef struct {
  int sampleRate;
  int numChannels;
} AudioSpecificConfig;

int readData(FILE *file, void *buffer, int length) {
    return fread(buffer, 1, length, file);
}

void decodeAudio(const char* inputFile) {
    FILE *file = fopen(inputFile, "rb"); // 打开要解码的文件
    if (!file) {
        fprintf(stderr, "Failed to open file\n");
        return;
    }
    FILE *ofile = fopen("111.pcm", "wb");

    // 创建AAC解码器实例
    HANDLE_AACDECODER handle = aacDecoder_Open(TT_MP4_RAW, 1);

    uint8_t* inputBuffer;
    unsigned int inputBufferSize = 4096;
    inputBuffer = (uint8_t*)calloc(inputBufferSize, sizeof(uint8_t));

    /*
    // 初始化AAC解码器
    AAC_DECODER_ERROR err = aacDecoder_Init(handle, inputBuffer, inputBufferSize, NULL);
    if (err != AAC_DEC_OK) {
        fprintf(stderr, "Failed to initialize AAC decoder\n");
        free(inputBuffer);
        fclose(file);
        aacDecoder_Close(handle);
        return;
    }
    */

    // 设置音频配置信息
    AudioSpecificConfig audioConfig;
    audioConfig.sampleRate = 11025;
    audioConfig.numChannels = 1;
    audioSpecificConfigType asc = { 0 };
    asc.samplingFrequencyIndex = audioConfig.sampleRate;
    asc.numChannels = audioConfig.numChannels;
    // 设置音频配置信息
    aacDecoder_ConfigRaw(handle, &asc);

    // 读取音频数据并进行解码
    while (!feof(file)) {
        int bytesRead = readData(file, inputBuffer, inputBufferSize);
        if (bytesRead > 0) {
            // 输入数据
            AAC_DECODER_ERROR decErr = aacDecoder_Fill(handle, &inputBuffer, &bytesRead, NULL);
            if (decErr != AAC_DEC_OK) {
                fprintf(stderr, "Failed to fill AAC decoder\n");
                break;
            }

            // 解码数据
            INT_PCM pcmBuffer[kMaxOutBufferSize];

            do {
                decErr = aacDecoder_DecodeFrame(handle, pcmBuffer, kMaxOutBufferSize, 0);
                fwrite(pcmBuffer, kMaxOutBufferSize, 1, ofile);
            } while (decErr == AAC_DEC_OK);
        }
    }

    // 释放资源
    free(inputBuffer);
    fclose(file);
    aacDecoder_Close(handle);
}

int main(int argc, char **argv) {
    decodeAudio("111.aac");
}
