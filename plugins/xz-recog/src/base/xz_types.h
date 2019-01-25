#include <stdio.h>

typedef struct BD_RESULT{
    int  status;
    char *corpus_no;//成功才有值
    char *err_msg;
    int  err_no;//0 成功， 非0 对应的错误码
    char *result;//成功才有值
    char *sn;
}BD_RESULT;

typedef struct XF_RESULT{
    int  status;
    int  code;//0 成功， 非0 对应的错误码
    char *data;
    char *desc;
    char *sid;  
}XF_RESULT;
