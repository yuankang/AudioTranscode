#include "AvAac.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
 
#define AAC_ENC_MAX_OUTPUT_SIZE		(768 * 8)	// @8ch
 
AvAac::AvAac() {
 
	h_aac_encoder_ = NULL;
	pcm_frame_len_ = 0;
}
 
AvAac::~AvAac() {
 
 
}
 
int AvAac::open_encoder(AvAacEncConfig aac_config, int &pcm_frame_len) {
 
	aac_enc_config_ = aac_config;
 
	AACENC_ERROR e;
	e = aacEncOpen((HANDLE_AACENCODER *)&h_aac_encoder_, 0, aac_config.in_channels);
	if (e != AACENC_OK) {
 
		return e;
	}
	// MPEG AOT
	int aot;
	switch (aac_config.format) {
 
		default:
		case AvAacEncFmt_AACLC:		aot = AOT_AAC_LC;		break;
		case AvAacEncFmt_EAAC:		aot = AOT_SBR;			break;
		case AvAacEncFmt_EAACPLUS:	aot = AOT_PS;			break;
		case AvAacEncFmt_AACLD:		aot = AOT_ER_AAC_LD;	break;
		case AvAacEncFmt_AACELD:	aot = AOT_ER_AAC_ELD;	break;
	}
	e = aacEncoder_SetParam(h_aac_encoder_, AACENC_AOT, aot);
	if (e != AACENC_OK) {
 
		return e;
	}
	int eld_sbr = 0;
	if (aot == AOT_ER_AAC_ELD && eld_sbr) {
 
		e = aacEncoder_SetParam(h_aac_encoder_, AACENC_SBR_MODE, 1);
		if (e != AACENC_OK) {
 
			return e;
		}
	}
	e = aacEncoder_SetParam(h_aac_encoder_, AACENC_SAMPLERATE, aac_config.sample_rate);
	if (e != AACENC_OK) {
 
		return e;
	}
	// MPEG channel
	int mode;
	switch (aac_config.in_channels) {
 
		case 1: mode = MODE_1;       break;
		case 2: mode = MODE_2;       break;
		case 3: mode = MODE_1_2;     break;
		case 4: mode = MODE_1_2_1;   break;
		case 5: mode = MODE_1_2_2;   break;
		case 6: mode = MODE_1_2_2_1; break;
		default: return -1;
	}
	e = aacEncoder_SetParam(h_aac_encoder_, AACENC_CHANNELMODE, mode);
	if (e != AACENC_OK) {
 
		return e;
	}
	int vbr = 0;
	if (vbr) {
 
		e = aacEncoder_SetParam(h_aac_encoder_, AACENC_BITRATEMODE, vbr);
		if (e != AACENC_OK) {
 
			return e;
		}
	}
	else {
 
		e = aacEncoder_SetParam(h_aac_encoder_, AACENC_BITRATEMODE, 0);
		if (e != AACENC_OK) {
 
			return e;
		}
		e = aacEncoder_SetParam(h_aac_encoder_, AACENC_BITRATE, aac_config.bit_rate);
		if (e != AACENC_OK) {
 
			return e;
		}
	}
	// trans type
	TRANSPORT_TYPE tt;
	switch (aac_config.trans_type) {
 
		default:
		case AvAacEncTransType_ADTS: tt = TT_MP4_ADTS; break;
		case AvAacEncTransType_LOAS: tt = TT_MP4_LOAS; break;
		case AvAacEncTransType_LATM_MCP1: tt = TT_MP4_LATM_MCP1; break;
	}
	e = aacEncoder_SetParam(h_aac_encoder_, AACENC_TRANSMUX, tt);
	if (e != AACENC_OK) {
 
		return e;
	}
	// band width
	if (aac_config.band_width > 0) {
 
		int min_bw = (aac_config.sample_rate + 255) >> 8;
		int max_bw = 20000;
		if (aac_config.band_width < min_bw) aac_config.band_width = min_bw;
		if (aac_config.band_width > max_bw) aac_config.band_width = max_bw;
		e = aacEncoder_SetParam(h_aac_encoder_, AACENC_BANDWIDTH, aac_config.band_width);
		if (e != AACENC_OK) {
 
			return e;
		}
	}
	// initialize
	e = aacEncEncode(h_aac_encoder_, NULL, NULL, NULL, NULL);
	if (e != AACENC_OK) {
 
		return e;
	}
	AACENC_InfoStruct enc_info = { 0 };
	e = aacEncInfo(h_aac_encoder_, &enc_info);
	if (e != AACENC_OK) {
 
		return e;
	}
	pcm_frame_len_ = enc_info.inputChannels * enc_info.frameLength * 2;
	pcm_frame_len = pcm_frame_len_;
	return AACENC_OK;
}
 
int AvAac::encode(unsigned char *aac, unsigned char *pcm) {
 
	AACENC_BufDesc in_buf = { 0 };
	AACENC_BufDesc out_buf = { 0 };
	AACENC_InArgs in_args = { 0 };
	AACENC_OutArgs out_args = { 0 };
	AACENC_ERROR e;
 
	// config in_bug and out_buf
	unsigned char ancillary_buf[64];
	AACENC_MetaData meta_data_setup;
	void *ibufs[] = { pcm, ancillary_buf, &meta_data_setup };
	int ibuf_ids[] = { IN_AUDIO_DATA, IN_ANCILLRY_DATA, IN_METADATA_SETUP };
	int ibuf_sizes[] = { pcm_frame_len_, sizeof(ancillary_buf), sizeof(meta_data_setup) };
	int ibuf_element_sizes[] = { sizeof(unsigned char), sizeof(unsigned char), sizeof(AACENC_MetaData) };
	in_buf.numBufs = sizeof(ibufs) / sizeof(void *);
	in_buf.bufs = (void **)&ibufs;
	in_buf.bufferIdentifiers = ibuf_ids;
	in_buf.bufSizes = ibuf_sizes;
	in_buf.bufElSizes = ibuf_element_sizes;
 
	void *obufs[] = { aac };
	int obuf_ids[] = { OUT_BITSTREAM_DATA };
	int obuf_sizes[] = { AAC_ENC_MAX_OUTPUT_SIZE };
	int obuf_element_sizes[] = { sizeof(unsigned char) };
	out_buf.numBufs = sizeof(obufs) / sizeof(void *);
	out_buf.bufs = (void **)&aac;
	out_buf.bufferIdentifiers = obuf_ids;
	out_buf.bufSizes = obuf_sizes;
	out_buf.bufElSizes = obuf_element_sizes;
 
	in_args.numAncBytes = 0;
	in_args.numInSamples = AAC_ENC_MAX_OUTPUT_SIZE;
 
	// start encode
	e = aacEncEncode(h_aac_encoder_, &in_buf, &out_buf, &in_args, &out_args);
	if (e != AACENC_OK) {
 
		if (e == AACENC_ENCODE_EOF) {
 
			return 0; // eof
		}
		return -1;
	}
	return out_args.numOutBytes;
}
 
int AvAac::encode_file(const char *aac_file, const char *pcm_file) {
 
	FILE *ifile = fopen(pcm_file, "rb");
	FILE *ofile = fopen(aac_file, "wb");
	if (ifile && ofile) {
 
		int r = -1;
		do {
 
			unsigned char *ibuf = (unsigned char *)malloc(pcm_frame_len_);
			memset(ibuf, 0, pcm_frame_len_);
			r = fread(ibuf, 1, pcm_frame_len_, ifile);
			if (r > 0) {
 
				unsigned char obuf[AAC_ENC_MAX_OUTPUT_SIZE];
				memset(obuf, 0, sizeof(obuf));
				int olen = encode(obuf, ibuf);
				fwrite(obuf, 1, olen, ofile);
			}
			free(ibuf);
		} while (r > 0);
		fclose(ifile);
		fclose(ofile);
		return 0;
	}
	return -1;
}
 
void AvAac::close_encoder() {
 
	aacEncClose(&h_aac_encoder_);
}
 
const char *AvAac::fdkaac_error2string(AACENC_ERROR e) {
 
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
