#ifndef __C_TOOL_H__
#define __C_TOOL_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "openssl/md5.h"
#include "xz_base64.h"

//base64加密
char* xz_base64_encode(const unsigned char* data, int len);

//base64解密
unsigned char* xz_base64_decode(const char* data);

//二进制转十六进制
unsigned char* xz_bin_to_hex(unsigned char *bin, int len);

//md5加密
unsigned char* xz_md5(unsigned char* src);

//获取当前时间
char* xz_current_time();

#endif