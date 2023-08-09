#!/bin/bash

#set -x
cd audio; rm -rf 000.pcm 111.pcm 222.aac 333.pcm 444.pcm 555.aac; cd -;

echo "---------- 编译运行 ----------" 
echo "gcc AudioTranscode.c g711.c -lsoxr -lfdk-aac -I . -o AudioTranscode"
gcc AudioTranscode.c g711.c -lsoxr -lfdk-aac -I . -o AudioTranscode
./AudioTranscode

echo "---------- play cmd ----------" 
echo "ffplay -f alaw -ar 8000 -ac 1 audio/dang.g711a"
echo "ffplay -f mulaw -ar 8000 -ac 1 audio/dang.g711u"
echo "ffplay -f s16le -ar 8000 -ac 1 audio/000.pcm"
echo "ffplay -f s16le -ar 11025 -ac 1 audio/111.pcm"
echo "ffplay audio/222.aac"
echo "ffplay audio/audio22050.aac"
echo "ffplay -f s16le -ar 22050 -ac 1 audio/333.pcm"
echo "ffplay -f s16le -ar 11025 -ac 1 audio/444.pcm"
echo "ffplay audio/555.aac"

echo "---------- 动态库 ----------" 
echo "gcc AudioTranscodeSo.c g711.c -lsoxr -lfdk-aac -I . -o libAudioTranscode.so -fPIC -shared"
gcc AudioTranscodeSo.c g711.c -lsoxr -lfdk-aac -I . -o libAudioTranscode.so -fPIC -shared
