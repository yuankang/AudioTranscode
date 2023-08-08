#include <stdio.h>
#include <stdlib.h>
#include <fdk-aac/aacdecoder_lib.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s input.aac output.pcm\n", argv[0]);
        return 1;
    }

    /* Open input file */
    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        fprintf(stderr, "Failed to open input file: %s\n", argv[1]);
        return 1;
    }
	FILE *ofile = fopen(argv[2], "wb");

    /* Initialize decoder */
    HANDLE_AACDECODER handle = aacDecoder_Open(TT_MP4_ADTS, /* Transport type */
            1 /* Number of channels */);
    if (!handle) {
        fprintf(stderr, "Failed to open AAC decoder\n");
        return 1;
    }

    /* Decode input file */
    const int INPUT_BUFFER_SIZE = 1024;
    uint8_t inputBuffer[INPUT_BUFFER_SIZE];
    uint8_t *inputData = inputBuffer;
    int inputSize = INPUT_BUFFER_SIZE;
    AAC_DECODER_ERROR err;

    while (1) {
        /* Read input data */
        int bytesRead = fread(inputData, 1, inputSize, file);
        if (bytesRead == 0) {
            break; /* End of file */
        }
        /* Submit input data to decoder */
        err = aacDecoder_Fill(handle, &inputData, &bytesRead, &inputSize);
        if (err != AAC_DEC_OK) {
            fprintf(stderr, "Failed to submit input data (%d)\n", err);
            return 1;
        }

        /* Decode frame */
        while (1) {
            fprintf(stderr, "xxx\n", err);
            err = aacDecoder_DecodeFrame(handle, NULL, 0, 0);
            if (err == AAC_DEC_NOT_ENOUGH_BITS) {
                break; /* Need more input data */
            } else if (err != AAC_DEC_OK) {
                fprintf(stderr, "Failed to decode frame (%d)\n", err);
                return 1;
            }
            /* Get decoded output */
            const void *pcmBuffer;
            uint8_t *pcmData;
            UINT pcmBufferSize;
            //int numSamples = aacDecoder_GetSamplesPerFrame(handle);
            //int numChannels = aacDecoder_GetChannelCount(handle);
            int numSamples = 11025;
            int numChannels = 1;
            //COLORATION pcmFormat = CA_PCM_16BIT_INTERLEAVED;
            err = aacDecoder_DecodeFrame(handle, &pcmBuffer, &pcmBufferSize, 0);
            if (err != AAC_DEC_OK) {
                fprintf(stderr, "Failed to get decoded output (%d)\n", err);
                return 1;
            }
            pcmData = (uint8_t *)pcmBuffer;
            /* Write decoded output to stdout */
            fwrite(pcmData, numSamples * numChannels * 2, 1, ofile);
        }
    }
    /* Close input file */
    fclose(file);
    /* Close decoder */
    aacDecoder_Close(handle);
    return 0;
}
