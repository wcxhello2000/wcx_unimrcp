#ifndef __C_RECOG_H__
#define __C_RECOG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xz_speech.h"

//百度接口，获取token
void xz_bd_get_token();

//百度接口，wav格式
void xz_bd_recog_wav(const char* filename, int rate);

//百度接口，pcm格式
void xz_bd_recog_pcm(const char* filename, int rate);

//讯飞接口，wav格式
char* xz_xf_recog_wav(const char* filename, int rate);

//讯飞接口，pcm格式
char* xz_xf_recog_pcm(const char* filename, int rate);

#endif