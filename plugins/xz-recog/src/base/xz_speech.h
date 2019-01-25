#ifndef __C_SPEECH_H__
#define __C_SPEECH_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "xz_link.h"
#include "xz_tool.h"
#include "xz_http.h"
#include "xz_types.h"

char* xz_get_access_token(const char* url, char* ak, char* sk);

static cJSON* xz_request_asr(char* url, cJSON* data, NODE_H* headers);

static cJSON* xz_request_asr01(char* url, const char* data, NODE_H* headers);

cJSON* xz_bd_recognize(char* url, const char* voice_binary, int len, const char* format, int rate, const NODE_H* options);

cJSON* xz_xf_recognize(char* url, const char* voice_binary, int len, const char* format, int rate);

BD_RESULT* xz_bd_parse(cJSON* json);

void xz_BD_RESULT_destory(BD_RESULT* ret);

XF_RESULT* xz_xf_parse(cJSON* json);

void xz_XF_RESULT_destory(XF_RESULT* ret);

#endif