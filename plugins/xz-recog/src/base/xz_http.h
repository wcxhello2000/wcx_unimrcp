#ifndef __C_HTTP_H__
#define __C_HTTP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdbool.h>
#include "xz_link.h"

inline size_t xz_on_write_data(void * buffer, size_t size, size_t nmemb, void * userp);

void xz_make_urlencoded_form(const NODE_H* head, char * content);

void xz_append_headers(const NODE_H* head, struct curl_slist ** slist);

void xz_append_url_params(const NODE_H* head, char* url);

int xz_post(const char* url, const NODE_H* params, const char* body, const NODE_H* headers, char* response);

int xz_get(const char* url, const NODE_H* params, const NODE_H* headers, char* response);


#endif