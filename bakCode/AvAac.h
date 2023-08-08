#ifndef _AV_AAC_H_
#define _AV_AAC_H_
 
/***********************************************************
**	Author:kaychan
**	Data:2019-11-21
**	Mail:1203375695@qq.com
**	Explain:a aac codec class base on fdk-aac
***********************************************************/
 
#include <fdk-aac/aacenc_lib.h>
 
typedef enum AvAacEncFmt_E {
 
	AvAacEncFmt_AACLC		= 0, // AAC LC 
	AvAacEncFmt_EAAC		= 1, // eAAC(HEAAC or AAC+  or aacPlusV1) 
	AvAacEncFmt_EAACPLUS	= 2, // eAAC+(AAC++ or aacPlusV2) 
	AvAacEncFmt_AACLD		= 3, // AAC LD(Low Delay) 
	AvAacEncFmt_AACELD		= 4, // AAC ELD(Low Delay) 
}AvAacEncFmt;
 
typedef enum AvAacEncTransType_E {
 
	AvAacEncTransType_ADTS		= 0,
	AvAacEncTransType_LOAS		= 1,
	AvAacEncTransType_LATM_MCP1 = 2,
}AvAacEncTransType;
 
typedef struct AvAacEncConfig_T{
 
	AvAacEncFmt format = AvAacEncFmt_AACLC;
	AvAacEncTransType trans_type = AvAacEncTransType_ADTS;
	short bits_per_sample = 16;		// bit
	int sample_rate = 11025;		// Hz
	int bit_rate = 64000;			// bit/second
	short in_channels = 1;
	short band_width = 10000;		// Hz
}AvAacEncConfig;
 
class AvAac {
 
public:
	AvAac();
	~AvAac();
	int open_encoder(AvAacEncConfig aac_config, int &pcm_frame_len);
	int encode(unsigned char *aac, unsigned char *pcm);
	int encode_file(const char *aac_file, const char *pcm_file);
	void close_encoder();
	const char *fdkaac_error2string(AACENC_ERROR e);
 
private:
	HANDLE_AACENCODER h_aac_encoder_;
	AvAacEncConfig aac_enc_config_;
	int pcm_frame_len_;
};
 
#endif
