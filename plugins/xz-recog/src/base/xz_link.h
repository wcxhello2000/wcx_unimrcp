#ifndef __C_LINK_H__
#define __C_LINK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct NODE_T{
    char* key;
    char* value;
    struct NODE_T* next;
}NODE_T;

typedef struct NODE_H{
    NODE_T* next;
}NODE_H;

//单向链表尾部添加节点
int xz_insert_node_tail(NODE_H* head, NODE_T* node);

//释放链表
void xz_destory_link(NODE_H* head);

/******************************************/

typedef struct PREV_DATA_NODE_T{
     void* data;
     int   len;
     struct PREV_DATA_NODE_T* next;
}PREV_DATA_NODE_T;

typedef struct PREV_DATA_NODE_H{
    PREV_DATA_NODE_T* next;
}PREV_DATA_NODE_H;

typedef struct LINK_STACK{
    int cur_size;//当前链表栈元素数
    int max_size;//最大链表栈元素数
    PREV_DATA_NODE_H* link_head;//链表头
}LINK_STACK;

//创建链表栈
LINK_STACK* xz_creat_link_stack(int max_size);

//销毁链表栈
int xz_destory_link_stack(LINK_STACK* stack);

//销毁链表
void xz_destory_prev_data_link(PREV_DATA_NODE_H* head);

//插入一段音频数据
void xz_insert_voice_data_into_link_stack(LINK_STACK* stack, const void *voice_data, unsigned int voice_len);

//入栈
void xz_push_link_stack(LINK_STACK* stack, PREV_DATA_NODE_T* node);

//出栈
PREV_DATA_NODE_T* xz_pop_link_stack(LINK_STACK* stack);

#endif