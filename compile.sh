#!/bin/bash

#set -x
cd audio; rm -rf 000.pcm 111.pcm 222.aac 333.pcm; cd -;

echo "gcc AudioTranscode.c g711.c -lsoxr -lfdk-aac -I . -o AudioTranscode"
gcc AudioTranscode.c g711.c -lsoxr -lfdk-aac -I . -o AudioTranscode
./AudioTranscode

echo "---------- play cmd ----------" 
echo "ffplay -nodisp -f alaw -ar 8000 -ac 1 audio/000.g711a"
echo "ffplay -f mulaw -ar 8000 -ac 1 audio/000.g711u"
echo "ffplay -f s16le -ar 8000 -ac 1 audio/000.pcm"
echo "ffplay -f s16le -ar 11025 -ac 1 audio/111.pcm"
echo "ffplay audio/222.aac"
echo "ffplay -f s16le -ar 11025 -ac 1 audio/333.pcm"
