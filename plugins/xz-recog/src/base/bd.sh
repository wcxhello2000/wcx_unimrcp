#! /bin/bash

gcc xz_main.c cJSON.c xz_base64.c xz_link.c xz_http.c xz_speech.c xz_tool.c xz_recog.c -o test -lcurl -lm -lcrypto
