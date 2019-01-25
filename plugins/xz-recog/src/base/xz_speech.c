#include "xz_speech.h"

char* xz_get_access_token(const char* url, char* ak, char* sk)
{
    NODE_H* head = (NODE_H*)malloc(sizeof(NODE_H));//need free after finshed 
    head->next = NULL;
    
    NODE_T* grant_type_node = (NODE_T*)malloc(sizeof(NODE_T));
    grant_type_node->next = NULL;
    grant_type_node->key = "grant_type";
    grant_type_node->value = "client_credentials";
    xz_insert_node_tail(head, grant_type_node);

    NODE_T* client_id_node = (NODE_T*)malloc(sizeof(NODE_T));
    client_id_node->next = NULL;
    client_id_node->key = "client_id";
    client_id_node->value = ak;
    xz_insert_node_tail(head, client_id_node);

    NODE_T* client_secret_node = (NODE_T*)malloc(sizeof(NODE_T));
    client_secret_node->next = NULL;
    client_secret_node->key = "client_secret";
    client_secret_node->value = sk;
    xz_insert_node_tail(head, client_secret_node);

    char* response = (char*)malloc(1024);
    memset(response, 0, 1024);
    xz_get(url, head, NULL, response);
    printf("response=%s\n", response);
    xz_destory_link(head);

    return response;
}

static cJSON* xz_request_asr(char* url, cJSON* data, NODE_H* headers)
{
    cJSON *obj;
    char *body = cJSON_Print(data);
    char *response = (char*)malloc(100*1024);
    memset(response, 0, 100*1024);
    
    int status_code = xz_post(url, NULL, body, headers, response);
    printf("response=%s\n", response);
    obj = cJSON_Parse(response);
    free(response);
    free(body);

    return obj;
}

static cJSON* xz_request_asr01(char* url, const char* data, NODE_H* headers)
{
    char *response = (char*)malloc(100*1024);
    memset(response, 0, 100*1024);
    
    int status_code = xz_post(url, NULL, data, headers, response);
    // printf("response=%s\n", response);
    cJSON* obj = cJSON_Parse(response);
    free(response);

    return obj;
}


cJSON* xz_bd_recognize(char* url, const char* voice_binary, int len, const char* format, int rate, const NODE_H* options)
{
    cJSON* data = cJSON_CreateObject();

    char* token = "24.5808c68707466b4b86b6f6d0856de324.2592000.1549698960.282335-15213493";
    char* voice_data = xz_base64_encode(voice_binary, len);
    char rate_data[10] = {0};
    snprintf(rate_data, 10, "%d", rate); 
    
    cJSON_AddStringToObject(data, "speech", voice_data);
    cJSON_AddStringToObject(data, "format", format);
    cJSON_AddStringToObject(data, "rate", rate_data);
    cJSON_AddStringToObject(data, "channel", "1");
    cJSON_AddStringToObject(data, "token", token);
    cJSON_AddStringToObject(data, "cuid", "KDsZBGiKBQS6AXxGlaSRdsvF");
    cJSON_AddNumberToObject(data, "len", len);

    NODE_H* headers = (NODE_H*)malloc(sizeof(NODE_H));
    headers->next = NULL;
    NODE_T* node = (NODE_T*)malloc(sizeof(NODE_T));
    node->next = NULL;
    node->key = "Content-Type";
    node->value = "application/json";
    headers->next = node;

    cJSON* obj = xz_request_asr(url, data, headers);

    cJSON_Delete(data);
    xz_destory_link(headers);
    if(voice_data){
        free(voice_data);
    }
    
    return obj;
}

cJSON* xz_xf_recognize(char* url, const char* voice_binary, int len, const char* format, int rate)
{
    char* appid = "5c236634";
    char* apikey = "3189dd39a2929af967807753ecaab6a4";
    char* curtime = xz_current_time();//need free after finshed 
    char *param = NULL;
    unsigned char* checksum = NULL;

    //X-Param
    cJSON* param_obj = cJSON_CreateObject();//need free after finshed 
    char rate_str[10] = {0};
    snprintf(rate_str, 10, "sms%dk", rate/1000);
    cJSON_AddStringToObject(param_obj, "engine_type", rate_str);
    cJSON_AddStringToObject(param_obj, "aue", format);
    char *param_json_str = cJSON_Print(param_obj);
    param = xz_base64_encode(param_json_str, strlen(param_json_str));//need free after finshed 
    cJSON_Delete(param_obj);
    free(param_json_str);

    //X-CheckSum
    char checksum_str[1024] = {0};
    snprintf(checksum_str, 1024, "%s%s%s", apikey, curtime, param);
    checksum = xz_md5(checksum_str); //need free after finshed 

    NODE_H* headers = (NODE_H*)malloc(sizeof(NODE_H));//need free after finshed 
    headers->next = NULL;

    NODE_T* content_type_node = (NODE_T*)malloc(sizeof(NODE_T));
    content_type_node->next = NULL;
    content_type_node->key = "Content-Type";
    content_type_node->value = "application/x-www-form-urlencoded; charset=utf-8";
    xz_insert_node_tail(headers, content_type_node);

    NODE_T* appid_node = (NODE_T*)malloc(sizeof(NODE_T));
    appid_node->next = NULL;
    appid_node->key = "X-Appid";
    appid_node->value = appid;
    xz_insert_node_tail(headers, appid_node);

    NODE_T* curtime_node = (NODE_T*)malloc(sizeof(NODE_T));
    curtime_node->next = NULL;
    curtime_node->key = "X-CurTime";
    curtime_node->value = curtime;
    xz_insert_node_tail(headers, curtime_node);

    NODE_T* param_node = (NODE_T*)malloc(sizeof(NODE_T));
    param_node->next = NULL;
    param_node->key = "X-Param";
    param_node->value = param;
    xz_insert_node_tail(headers, param_node);

    NODE_T* checksum_node = (NODE_T*)malloc(sizeof(NODE_T));
    checksum_node->next = NULL;
    checksum_node->key = "X-CheckSum";
    checksum_node->value = checksum;
    xz_insert_node_tail(headers, checksum_node);

    char* data = xz_base64_encode(voice_binary, len);//need free after finshed 
    int body_length = strlen(data)+10;
    char *body = (char*)malloc(body_length);
    memset(body,0,body_length);
    strcpy(body,"audio=");
    strcat(body,data);
    free(data);

    cJSON* obj = xz_request_asr01(url, body, headers);
    xz_destory_link(headers);
    free(body);
    free(param);
    free(curtime);
    if(checksum){
        free(checksum);
    }

    return obj;
}

//百度，json解析
BD_RESULT* xz_bd_parse(cJSON* json)
{
    BD_RESULT* ret = (BD_RESULT*)malloc(sizeof(XF_RESULT));
    ret->status = -1;
    ret->corpus_no = NULL;
    ret->err_msg = NULL;
    ret->err_no = -1;
    ret->result = NULL;
    ret->sn = NULL;
    if(!json){
        return ret;
    }

    cJSON *json_err_no = cJSON_GetObjectItem(json, "err_no");
    if(json_err_no){
        if(json_err_no->type == cJSON_Number){
            printf("err_no=%d\n", json_err_no->valueint);
            ret->err_no = json_err_no->valueint;
            if(ret->err_no == 0){
                ret->status = 0;
            }
        }
    }
    cJSON *json_corpus_no = cJSON_GetObjectItem(json, "corpus_no");
    if(json_corpus_no){
        if(json_corpus_no->type == cJSON_String){
            printf("json_corpus_no=%s\n", json_corpus_no->valuestring);
            int length = strlen(json_corpus_no->valuestring) + 1;
            ret->corpus_no = (char*)malloc(length);
            memset(ret->corpus_no, 0, length);
            strcpy(ret->corpus_no, json_corpus_no->valuestring);
        }
    }   
    cJSON *json_err_msg = cJSON_GetObjectItem(json, "err_msg");
    if(json_err_msg){
        if(json_err_msg->type == cJSON_String){
            printf("err_msg=%s\n", json_err_msg->valuestring);
            int length = strlen(json_err_msg->valuestring) + 1;
            ret->err_msg = (char*)malloc(length);
            memset(ret->err_msg, 0, length);
            strcpy(ret->err_msg, json_err_msg->valuestring);
        }
    }
    cJSON *json_result = cJSON_GetObjectItem(json, "result");
    if(json_result){
        if(json_result->type == cJSON_String){
            printf("result=%s\n", json_result->valuestring);
            int length = strlen(json_result->valuestring) + 1;
            ret->result = (char*)malloc(length);
            memset(ret->result, 0, length);
            strcpy(ret->result, json_result->valuestring);
        }
    }  
    cJSON *json_sn = cJSON_GetObjectItem(json, "sn");
    if(json_sn){
        if(json_sn->type == cJSON_String){
            printf("sn=%s\n", json_sn->valuestring);
            int length = strlen(json_sn->valuestring) + 1;
            ret->sn = (char*)malloc(length);
            memset(ret->sn, 0, length);
            strcpy(ret->sn, json_sn->valuestring);
        }
    }
    
    cJSON_Delete(json);
    return ret;
}

void xz_BD_RESULT_destory(BD_RESULT* ret)
{
    if(ret){
        if(ret->corpus_no) free(ret->corpus_no);
        if(ret->err_msg) free(ret->err_msg);
        if(ret->result) free(ret->result);
        if(ret->sn) free(ret->sn);
        free(ret);
    }
}

//讯飞，json解析
XF_RESULT* xz_xf_parse(cJSON* json)
{
    XF_RESULT* ret = (XF_RESULT*)malloc(sizeof(XF_RESULT));
    ret->status = -1;
    ret->code = -1;
    ret->data = NULL;
    ret->desc = NULL;
    ret->sid = NULL;
    if(!json){
        return ret;
    }
    cJSON *json_code = cJSON_GetObjectItem(json, "code");
    if(json_code){
        if(json_code->type == cJSON_String){
            ret->code = atoi(json_code->valuestring);
            if(ret->code == 0){
                ret->status = 0;
            }
        }
    }

    cJSON *json_data = cJSON_GetObjectItem(json, "data");
    if(json_data){
        if(json_data->type == cJSON_String){
            int length = strlen(json_data->valuestring) + 1;
            ret->data = (char*)malloc(length);
            memset(ret->data, 0, length);
            strcpy(ret->data, json_data->valuestring);
        }
    }

    cJSON *json_desc = cJSON_GetObjectItem(json, "desc");
    if(json_desc){
        if(json_desc->type == cJSON_String){
            int length = strlen(json_desc->valuestring) + 1;
            ret->desc = (char*)malloc(length);
            memset(ret->desc, 0, length);
            strcpy(ret->desc, json_desc->valuestring);
        }
    }

    cJSON *json_sid = cJSON_GetObjectItem(json, "sid");
    if(json_sid){
        if(json_sid->type == cJSON_String){
            int length = strlen(json_sid->valuestring) + 1;
            ret->sid = (char*)malloc(length);
            memset(ret->sid, 0, length);
            strcpy(ret->sid, json_sid->valuestring);
        }
    }

    return ret;
}

void xz_XF_RESULT_destory(XF_RESULT* ret)
{
    if(ret){
        if(ret->data) free(ret->data);
        if(ret->desc) free(ret->desc);
        if(ret->sid) free(ret->sid);
        free(ret);
    }
}