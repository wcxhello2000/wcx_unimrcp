#include "xz_tool.h"

//base64加密
char* xz_base64_encode(const unsigned char* data, int len)
{
    int buffer_len = BASE64_SIZE(len);
    char *buffer = (char*)malloc(buffer_len);
    memset(buffer, 0, buffer_len);
    xz_base64_encode_ex(buffer,buffer_len,data,len);
    return buffer;
}

//base64解密
unsigned char* xz_base64_decode(const char* data)
{
    char *out = (char*)malloc(1024);
    memset(out, 0, 1024);
    xz_base64_decode_ex(out, data, 1024);
    return out;
}

//二进制转十六进制
unsigned char* xz_bin_to_hex(unsigned char *bin, int len)
{
    //char *buf = new char[len*2+1]();
    char *buf = (char*)malloc(len*2+1);
    memset(buf, 0, len*2+1);
    int i;
    for (i=0;i<len;i++){
        sprintf(buf+2*i, "%02x", *(bin+i));
    }
    return buf;
}

//md5加密
unsigned char* xz_md5(unsigned char* src)
{
    unsigned char md[16] = {0};
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, src, strlen(src));
    MD5_Final(md, &ctx);
    return xz_bin_to_hex(md, 16);
}

//获取当前时间
char* xz_current_time()
{
    char *cur_time = (char*)malloc(128);
    memset(cur_time, 0, 128);
    long now = time(NULL);
    sprintf(cur_time, "%ld", now);
    return cur_time;
}

