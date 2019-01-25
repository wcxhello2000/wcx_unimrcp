#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xz_recog.h"


int main(int argc, char* argv[])
{
    // xz_bd_get_token();
    // xz_bd_recog_wav("voice_file/iflytek02.wav", 16000);
    // xz_bd_recog_pcm("voice_file/weather.pcm", 16000);
    //讯飞接口，wav格式
    // xz_xf_recog_wav("voice_file/iflytek02.wav", 16000);

    if(argc == 3){
        int rate = atoi(argv[2]);
        char *filename = argv[1];
        // xz_bd_recog_pcm(argv[1], rate);
        // char *rslt = xz_xf_recog_pcm(argv[1], rate);
        int count = 100;
        while(count){
            char *rslt = xz_xf_recog_wav(filename, rate);
            printf("rslt:[%s]\n", rslt);
            if(rslt) {
                free(rslt);
            }
            count--;
        }
       
    }
    
    return 0;
}
