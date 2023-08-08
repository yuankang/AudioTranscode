#!/bin/bash

cd audio; rm -rf 000.pcm 111.pcm 222.aac 333.pcm; cd -;

gcc AudioTranscode.c g711.c -lsoxr -lfdk-aac -I . -o AudioTranscode
./AudioTranscode

ffplay -f alaw -ar 8000 -ac 1 audio/000.g711a
#ffplay -f mulaw -ar 8000 -ac 1 audio/000.g711u
ffplay -f s16le -ar 8000 -ac 1 audio/000.pcm
ffplay -f s16le -ar 11025 -ac 1 audio/111.pcm
ffplay audio/222.aac
ffplay -f s16le -ar 11025 -ac 1 audio/333.pcm
