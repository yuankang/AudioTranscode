#ifndef AudioTranscodeH
#define AudioTranscodeH

//g711x -> aac 11025Hz
int G711xToAac(void const *iData, void *oData, int iType);

//aac -> aac 11025Hz
int AacToAac(void const *iData, void *oData, int iSampleRate);

#endif /* AudioTranscodeH */
