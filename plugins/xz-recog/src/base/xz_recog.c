#include "xz_recog.h"

//百度接口，获取token
void xz_bd_get_token()
{
    xz_get_access_token("https://aip.baidubce.com/oauth/2.0/token", "PGXMsVhIiQtEeVjlzbsjNwLY", "EalejZCyzK6lFlE8afi6hYIuHvb3Nv99");
}

//百度接口，wav格式
void xz_bd_recog_wav(const char* filename, int rate)
{
    int length = 0;
    int  n = 0;
    FILE *fd = fopen(filename, "rb+");
    if(fd == NULL){
        perror("opening file err ");
        return;
    }

    fseek(fd, 0, SEEK_END);
    length = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    char *buffer = (char*)malloc(length+1);
    memset(buffer, 0, length+1);
    
    n = fread(buffer, 1, length, fd);;
    char *url = "http://vop.baidu.com/server_api";
    
    cJSON* obj = xz_bd_recognize(url, buffer, n, "wav", rate, NULL);
    if(obj){
        cJSON_Delete(obj);
    }
    
    fclose(fd);
    free(buffer);
}

//百度接口，pcm格式
void xz_bd_recog_pcm(const char* filename, int rate)
{
    FILE* fd = fopen(filename, "rb+");
    int length = 100*1024;
    char *buffer = (char*)malloc(length);
    int n = 0;
    while(!feof(fd)){
        memset(buffer, 0, length);
        n = fread(buffer, 1, length, fd);
        char *url = "http://vop.baidu.com/server_api";
        cJSON* obj = xz_bd_recognize(url, buffer, n, "pcm", rate, NULL);
        if(obj){
            cJSON_Delete(obj);
        }
    }
    
    fclose(fd);
    free(buffer);
}

//讯飞接口，wav格式
char* xz_xf_recog_wav(const char* filename, int rate)
{
    //cJSON* xf_recognize(char* url, const char* voice_binary, int len, const char* format, int rate)
    char *result = NULL;
    int length = 0;
    int  n = 0;
    FILE *fd = fopen(filename, "rb+");
    if(fd == NULL){
        perror("opening file err ");
        return NULL;
    }

    fseek(fd, 0, SEEK_END);
    length = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    char *buffer = (char*)malloc(length+1);
    memset(buffer, 0, length+1);
    
    n = fread(buffer, 1, length, fd);;
    char *url = "http://api.xfyun.cn/v1/service/v1/iat";
    
    cJSON* obj = xz_xf_recognize(url, buffer, n, "raw", rate);
    if(obj){
        XF_RESULT* ret = xz_xf_parse(obj);
        if(ret){
            if(ret->status == 0){
                if(ret->code == 0){
                    int len = strlen(ret->data) + 1;
                    result = (char*)malloc(len);
                    memset(result, 0, len);
                    strcpy(result, ret->data);
                }
            }
            xz_XF_RESULT_destory(ret);
        }

        cJSON_Delete(obj); 
    }

    fclose(fd);
    free(buffer);

    return result;
}
// XF_RESULT* xz_xf_parse(cJSON* json);
//讯飞接口，pcm格式
char* xz_xf_recog_pcm(const char* filename, int rate)
{
    char *result = NULL;
    result = (char*)malloc(1024);
    memset(result, 0, 1024);
    FILE* fd = fopen(filename, "rb+");
    int length = 100*1024;
    char *buffer = (char*)malloc(length);
    int n = 0;
    while(!feof(fd)){
        memset(buffer, 0, length);
        n = fread(buffer, 1, length, fd);
        char *url = "http://api.xfyun.cn/v1/service/v1/iat";
        cJSON* obj = xz_xf_recognize(url, buffer, n, "raw", rate);
        if(obj){
            XF_RESULT* ret = xz_xf_parse(obj);
            if(ret){
                if(ret->status == 0){
                    if(ret->code == 0){
                        // printf("xf_recog_data=%s\n", ret->data);
                        strcat(result, ret->data);
                    }
                }
                xz_XF_RESULT_destory(ret);
            }

            cJSON_Delete(obj);
        }
    }
    
    fclose(fd);
    free(buffer);

    return result;
}