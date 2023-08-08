#include <stdio.h>
#include <string.h>
#include "fdk-aac/aacenc_lib.h"

typedef struct parameterDict{
	AACENC_PARAM param;
	UINT		 value;
}Dict;

typedef struct AacContext {
	HANDLE_AACENCODER hAacEncoder;
	int inBufferIdentifier;
	int inputbufElSizes;  
	#define readMaxLen  4096
	int outputbufElSizes;
	int outputIdentifier;
	int outputBufferSize;
	
	AACENC_BufDesc inputBufDesc;
	AACENC_BufDesc outputBufDesc;
	AACENC_InArgs  inputArgs;
	AACENC_OutArgs outputArgs;
}AacCtx;

int aacEncInit(AacCtx *ctx);
int aacEncEnd(AacCtx *ctx);
int aacEncEnd(AacCtx *ctx);

int aacEncInit(AacCtx *ctx){
	if (!ctx) 
		return -1;
	memset((void*)ctx, '\0', sizeof(AacCtx));
	AACENC_ERROR AAC_ERR = AACENC_OK;
	Dict m_Dict[] ={
		{AACENC_AOT, AOT_AAC_LC},
		{AACENC_SBR_MODE, AOT_NULL_OBJECT},
		{AACENC_SAMPLERATE, 44100},
		{AACENC_BITRATE, 44100*2*16},
		{AACENC_BITRATEMODE, 0},
		{AACENC_CHANNELMODE, MODE_2},
		{AACENC_TRANSMUX, TT_MP4_ADTS},
	};
	
	AAC_ERR = aacEncOpen( &ctx->hAacEncoder, 0x01, 0x00 );
	if (ctx->hAacEncoder == NULL || AAC_ERR != AACENC_OK) {
		printf("aacEncOpen ERROR\r\n");
		return -1;
	}
	
	int i = 0, j = sizeof(m_Dict)/sizeof(Dict);
	for ( i = 0; i < j; i++ ) {
		AAC_ERR = aacEncoder_SetParam(ctx->hAacEncoder, m_Dict[i].param, m_Dict[i].value);
		if ( AAC_ERR != AACENC_OK ){
			aacEncEnd(ctx);
			ctx->hAacEncoder = NULL;
			printf("aacEncoder_SetParam ERROR %d\r\n", i);
			return -1;
		}
	}
	
	if (aacEncEncode(ctx->hAacEncoder, NULL, NULL, NULL, NULL) != AACENC_OK) {
		aacEncEnd(ctx);
		ctx->hAacEncoder = NULL;
		printf("aacEncEncode test ERROR from aacEncInit\n");
		return -1;
	}
	
	
	ctx->inBufferIdentifier = IN_AUDIO_DATA;
	ctx->inputBufDesc.bufferIdentifiers = &ctx->inBufferIdentifier;
	ctx->inputbufElSizes = 2;  
	ctx->inputBufDesc.bufElSizes = &ctx->inputbufElSizes;
	
	ctx->outputbufElSizes = 1;
	ctx->outputBufDesc.bufElSizes = &ctx->outputbufElSizes;
	ctx->outputIdentifier = OUT_BITSTREAM_DATA;
	ctx->outputBufDesc.bufferIdentifiers = &ctx->outputIdentifier;
	ctx->outputBufferSize = readMaxLen;
	ctx->outputBufDesc.bufSizes = (INT*)&ctx->outputBufferSize;

	ctx->inputBufDesc.numBufs = 1;
	ctx->outputBufDesc.numBufs = 1;
	return 0;
}


int aacEncode(AacCtx *ctx, void *pPCMdata, unsigned int PCMdataSize, void *outputBuffer){
	/* input buffer info */
	ctx->inputBufDesc.bufs = &pPCMdata;
	ctx->inputBufDesc.bufSizes = &PCMdataSize;
	
	/* output buffer info */
	ctx->outputBufDesc.bufs = &outputBuffer;
	
	/* input arguments */
	ctx->inputArgs.numInSamples = PCMdataSize/2;
	
	/* output arguments */
	memset((void*)&ctx->outputArgs, (int)'\0', sizeof(ctx->outputArgs));
	/* encode */
	if (aacEncEncode(ctx->hAacEncoder, &ctx->inputBufDesc, &ctx->outputBufDesc, &ctx->inputArgs, &ctx->outputArgs) != AACENC_OK){
		printf("aacEncEncode AACENC CODE %d, output bytes : %d\r\n",ctx->outputArgs.numOutBytes);
		return 0;
	}
	return ctx->outputArgs.numOutBytes;
} 

int aacEncEnd(AacCtx *ctx){
	if (!ctx || !ctx->hAacEncoder) {
		return -1;
	}
	
	if (aacEncClose(&ctx->hAacEncoder) != AACENC_OK) {
		return -1;
	}
	
	ctx->hAacEncoder = NULL;
	return 0;
}


/* compile :  gcc pcm2aac.c -std=c11 -I./include -L./lib -lfdk-aac -lm -o pcm2aac */
int main(int argc, char *argv[])
{
	AacCtx ctx = { NULL };
	if (aacEncInit(&ctx) != 0 || !ctx.hAacEncoder ){
		printf("init Fail!\r\n");
	}
	
	unsigned char inputBuffer[readMaxLen] = {0};
	unsigned char outputBuffer[readMaxLen] = {0};
	
	FILE *inputFD = fopen("audio.pcm","r");
	FILE *outputFD = fopen("audio_dj.aac","w");
	if (inputFD == NULL || outputFD == NULL) {
		printf("fail to open file\r\n");
		return -1;
	}
	
	size_t readLen = 0;
	do{
		readLen = fread(inputBuffer, 1, readMaxLen, inputFD);
		fwrite(outputBuffer, aacEncode(&ctx, inputBuffer, readLen, outputBuffer), 1, outputFD);
	}while(readLen > 0);
	
	fclose(inputFD);
	fclose(outputFD);
 
	aacEncEnd(&ctx);
	return 0;
}

/*
-rw-r--r-- 1 13919 197609  1764000 Mar  3 21:34 audio.pcm

$ ./ffprobe -ar 44100 -ac 2 -f s16le -i audio.pcm
Input #0, s16le, from 'audio.pcm':
  Duration: 00:00:10.00, bitrate: 1411 kb/s
  Stream #0:0: Audio: pcm_s16le, 44100 Hz, 2 channels, s16, 1411 kb/s

PCM format:
	L R L R L R L R ....
	↓
	2Byte
	
|   1764000 Byte / ( 44100HZ * 2 channls * 2 Byte ) = 10秒   |



比特率：bps（bit per second，位/秒）；kbps（通俗地讲就是每秒钟1000比特）作为单位。
	比特率=采样率*采样声道数*采样位深
	1411200 b/s = 44100*2*16 即 1411.2 kb/s

*/

/* see fdk-aac project of documentation/aacEncoder.pdf from github with https://github.com/mstorsjo/fdk-aac.

	2.2 Calling Sequence

		1. Call aacEncOpen() to allocate encoder instance with required configuration.

		2. Call aacEncoder SetParam() for each parameter to be set. AOT, samplingrate, channelMode,
		bitrate and transport type are mandatory.

		3. Call aacEncEncode() with NULL parameters to initialize encoder instance with present parameter set.

		4. Call aacEncInfo() to retrieve a configuration data block to be transmitted out of band. This
		is required when using RFC3640 or RFC3016 like transport.

		5. Encode input audio data in loop.

		6. Call aacEncClose() and destroy encoder instance.
*/


