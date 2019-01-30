#include "xz_http.h"

inline size_t xz_on_write_data(void * buffer, size_t size, size_t nmemb, void * userp)
{
    strcat((char*)userp, (char*)buffer);
    return nmemb;
}

void xz_make_urlencoded_form(const NODE_H* head, char * content)
{
    char str[1024] = {0};
    NODE_T *node = head->next;
    while(1){
        if(node != NULL){
            char *key = curl_escape(node->key, strlen(node->key));
            char *value = curl_escape(node->value, strlen(node->value));
            memset(str, 0, 1024);
            snprintf(str, 1024, "%s=%s&", key, value);
            strcat(content,str);
            curl_free(key);
            curl_free(value);
            node = node->next;
        }
        else{
             break;
        }
    }
}

void xz_append_headers(const NODE_H* head, struct curl_slist ** slist)
{
    if(head->next == NULL){
        printf("the list is empty...\n");
        return;
    }
    else{
        char str[1024] = {0};
        NODE_T *node = head->next;
        while(1){
            if(node != NULL){
                memset(str, 0, 1024);
                snprintf(str, sizeof(str), "%s:%s", node->key, node->value);
                *slist = curl_slist_append(*slist, str);
                node = node->next;
            }
            else{
                break;
            }
        }
    }
}


void xz_append_url_params(const NODE_H* head, char* url)
{
    if(head->next == NULL) {
        printf("the list is empty...\n");
        return;
    }
    char content[1024] = {0};
    xz_make_urlencoded_form(head, content);
    bool url_has_param = false;
    int i;
    for(i=0; i<strlen(url); i++){
        char ch = *(url+i);
        if(ch == '?'){
            url_has_param = true;
            break;
        }
    }
    if(url_has_param){
        strcat(url,"&");
        strcat(url,content);
    }else{
        strcat(url,"?");
        strcat(url,content);
    }
}


int xz_post(const char* url, const NODE_H* params, const char* body, const NODE_H* headers, char* response)
{
    char *url_tmp = (char*)malloc(10*1024);
    memset(url_tmp, 0, 10*1024);
    strcpy(url_tmp, url);
    struct curl_slist *slist = NULL;
    CURL *curl = curl_easy_init();
    if(headers){
        xz_append_headers(headers, &slist);
    }
    if(params){
        xz_append_url_params(params, url_tmp);
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url_tmp);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_POST, true);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(body));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, xz_on_write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) response);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, true);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 10000);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 10000);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, false);

    int status_code = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(slist);
    if(url_tmp){
        free(url_tmp);
        url_tmp = NULL;
    }

    return status_code;
}


int xz_get(const char* url, const NODE_H* params, const NODE_H* headers, char* response)
{
    char *url_tmp = (char*)malloc(10*1024);
    memset(url_tmp, 0, 10*1024);
    strcpy(url_tmp, url);
    CURL * curl = curl_easy_init();
    struct curl_slist * slist = NULL;
    if (headers) {
        xz_append_headers(headers, &slist);
    }
    if (params) {
        xz_append_url_params(params, url_tmp);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url_tmp);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, xz_on_write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) response);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, true);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 10000);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 10000);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, false);

    int status_code = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(slist);
    if(url_tmp){
        free(url_tmp);
        url_tmp = NULL;
    }
    return status_code;
}
