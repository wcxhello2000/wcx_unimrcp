#include "xz_link.h"

//单向链表尾部添加节点
int xz_insert_node_tail(NODE_H* head, NODE_T* node)
{
    if(head->next == NULL){
        head->next = node;
    }else{
        NODE_T *tmp_node = head->next;
        while(1){
            if(tmp_node->next == NULL){
                tmp_node->next = node;
                break;
            }else {
                tmp_node = tmp_node->next;
            }
        }
    }
    return 0;
}

//销毁链表(节点类型NODE_T)
void xz_destory_link(NODE_H* head)
{
    if(head->next){
        while(1){
            NODE_T *tmp_node = head->next;
            if(tmp_node != NULL){
                head->next = tmp_node->next;
                free(tmp_node);
            }else {
              break;
            }
        }
        free(head);
    }
}

/***************************************/
//创建链表栈
LINK_STACK* xz_creat_link_stack(int max_size)
{
    LINK_STACK* xz_link_stack = (LINK_STACK*)malloc(sizeof(LINK_STACK));
    xz_link_stack->cur_size = 0;
    xz_link_stack->max_size = max_size;
    PREV_DATA_NODE_H* head = (PREV_DATA_NODE_H*)malloc(sizeof(PREV_DATA_NODE_H));
    head->next = NULL;
    xz_link_stack->link_head = head;

    return xz_link_stack;
}

//销毁链表栈
int xz_destory_link_stack(LINK_STACK* stack)
{
    if(stack){
        if(stack->cur_size > 0){
            PREV_DATA_NODE_H* head = stack->link_head;
        }
        free(stack);
        stack = NULL;
    }
    return 0;
}

//销毁链表(节点类型PREV_DATA_NODE_T)
void xz_destory_prev_data_link(PREV_DATA_NODE_H* head)
{
    if(head->next){
        while(1){
            PREV_DATA_NODE_T *tmp_node = head->next;
            if(tmp_node != NULL){
                head->next = tmp_node->next;
                if(tmp_node->data) free(tmp_node->data);
                free(tmp_node);
            }else {
                break;
            }
        }
        free(head);
    }   
}

//插入一段音频数据
void xz_insert_voice_data_into_link_stack(LINK_STACK* stack, const void *voice_data, unsigned int voice_len)
{
    PREV_DATA_NODE_T* node = (PREV_DATA_NODE_T*)malloc(sizeof(PREV_DATA_NODE_T));
    node->next = NULL;
    node->len = voice_len;
    node->data = malloc(voice_len);
    memset(node->data, 0, voice_len);
    memcpy(node->data, voice_data, voice_len);
    
    if(stack->cur_size < stack->max_size){
        xz_push_link_stack(stack, node);
        stack->cur_size++;
    }else if(stack->cur_size >= stack->max_size){
        PREV_DATA_NODE_T* tmp_node = xz_pop_link_stack(stack);
        xz_push_link_stack(stack, node);
        if(tmp_node){
            if(tmp_node->data) free(tmp_node->data);
            free(tmp_node);
        }
    }
}

//入栈
void xz_push_link_stack(LINK_STACK* stack, PREV_DATA_NODE_T* node)
{
    PREV_DATA_NODE_H* head = stack->link_head;
    if(head->next == NULL){
        head->next = node;
    }else {
        PREV_DATA_NODE_T *tmp_node = head->next;
        while(1){
            if(tmp_node->next == NULL){
                tmp_node->next = node;
                break;
            }else {
                tmp_node = tmp_node->next;
            }
        }
    }
}

//出栈
PREV_DATA_NODE_T* xz_pop_link_stack(LINK_STACK* stack)
{
    PREV_DATA_NODE_H* head = stack->link_head; 
    PREV_DATA_NODE_T* tmp_node = head->next;   
    if(tmp_node){
        head->next = tmp_node->next;
    }
    return tmp_node;
}

