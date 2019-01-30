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

/******************队列*********************/
//创建队列
LINK_QUEUE* xz_creat_queue(int max_size)
{
    LINK_QUEUE* queue = (LINK_QUEUE*)malloc(sizeof(LINK_QUEUE));
    queue->cur_size = 0;
    queue->max_size = max_size;
    PREV_DATA_NODE_H* head = (PREV_DATA_NODE_H*)malloc(sizeof(PREV_DATA_NODE_H));
    head->next = NULL;
    queue->link_head = head;

    return queue;
}

//销毁队列
int xz_destory_queue(LINK_QUEUE* queue)
{
    if(queue){
        if(queue->cur_size > 0){
            PREV_DATA_NODE_H* head = queue->link_head;
            xz_destory_prev_data_link(head);
        }
        free(queue);
        queue = NULL;
    }
    return 0;
}

//插入一段音频数据
void xz_insert_voice_data_into_queue(LINK_QUEUE* queue, const void *voice_data, unsigned int voice_len)
{
    PREV_DATA_NODE_T* node = (PREV_DATA_NODE_T*)malloc(sizeof(PREV_DATA_NODE_T));
    node->next = NULL;
    node->len = voice_len;
    node->data = malloc(voice_len);
    memset(node->data, 0, voice_len);
    memcpy(node->data, voice_data, voice_len);
    
    if(queue->cur_size < queue->max_size){
        xz_push_queue(queue, node);
        queue->cur_size++;
    }else if(queue->cur_size >= queue->max_size){
        PREV_DATA_NODE_T* tmp_node = xz_pop_queue(queue);
        xz_push_queue(queue, node);
        if(tmp_node){
            if(tmp_node->data) free(tmp_node->data);
            free(tmp_node);
        }
    }
}

//入队
void xz_push_queue(LINK_QUEUE* queue, PREV_DATA_NODE_T* node)
{
    PREV_DATA_NODE_H* head = queue->link_head;
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

//出队
PREV_DATA_NODE_T* xz_pop_queue(LINK_QUEUE* queue)
{
    PREV_DATA_NODE_H* head = queue->link_head; 
    PREV_DATA_NODE_T* node = head->next;   
    if(node){
        head->next = node->next;
    }
    return node;
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
        head = NULL;
    }   
}
