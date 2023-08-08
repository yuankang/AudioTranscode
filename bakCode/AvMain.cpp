#include "AvAac.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define AAC_ENC_MAX_OUTPUT_SIZE		(768 * 8)	// @8ch

int main() {
    AvAac aac;
    int pcm_len;

    AvAacEncConfig aac_config;
    aac_config.format = AvAacEncFmt_AACLC;
    aac_config.bit_rate = 64000;

    aac.open_encoder(aac_config, pcm_len);
	aac.encode_file("111.aac", "audioNew.pcm");
    /*
    unsigned char *ibuf = (unsigned char *)malloc(pcm_len);

    // set pcm data to ibuf here···
    unsigned char obuf[AAC_ENC_MAX_OUTPUT_SIZE];
    int olen = aac.encode(obuf, ibuf);
    // do obuf what you want···
    */
}
