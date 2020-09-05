#include "slink_list.h"
#include <malloc.h>
#include "../csapp.h"
#include "../log.h"
#include <assert.h>
#include <semaphore.h>

/* global vars */
static sem_t mutex;

/**
 * @brief initialize list 
 * 
 * @param list 
 * @return int 0 sucess
 *             -1 failed
 */
int slist_init(slist *list) {
    if (!list) {
        log_debug("init list failed\n");
        return -1;
    }
    /* allocate a header for this list */
    list->head = (list_node *)malloc(sizeof(list_node));
    list->head->itemp = NULL;
    list->head->next = NULL;
    list->tail = list->head;
    list->node_num = 0;

    /* mutex init */
    sem_init(&mutex, 0, 1);
    return 0;
}

/**
 * @brief  deinitialize 
 * 
 * @param list 
 * @return int 0 sucess
 *             -1 failed
 */
int slist_deinit(slist *list) {
    if (!list) return -1;

    if (list->node_num > 0) {
        /* iterate the list and free*/
        list_node *p1 = list->head, *p2;
        while (p1) {
            p2 = p1;
            p1 = p1->next;
            free(p2);
        }
    }
    return 0;
}

/**
 * @brief push back 
 * 
 * @param list 
 * @param node 
 * @return int  0 sucess
 *              -1 failed
 */
int slist_push_back(slist *list, list_node *node){
    return slist_insert(list, list->node_num+1, node);
}

/**
 * @brief insert node to list
 *        idx range [1,list->node_num+1]
 * 
 * @param list 
 * @param idx 
 * @param node 
 * @return int 
 */
int slist_insert(slist *list, size_t idx, list_node *node)
{
    list_node *p;
    int passby;

    if(!list){
        log_debug("list is empty\n");
        return -1;
    }

    P(&mutex);
    /* check idx is over the list node num + 1? */
    if(idx < 1 || idx > list->node_num + 1){
        log_debug("idx is out of range\n");
        V(&mutex);
        return -1;
    }

    node->next = NULL;
    /* is push back? */
    if(idx == list->node_num + 1){
        list->tail->next = node;
        list->tail = node;
    } else {
        /* normal insert */
        node->next = p->next;
        p->next = node;
    }

     list->node_num++;
     V(&mutex);
     return 0;
}

/**
 * @brief pop front
 * 
 * @param list 
 * @return list_node*  sucess
 *          NULL       failed
 */
list_node *slist_pop(slist *list)
{
    return slist_remove(list, 1);
}

/**
 * @brief remove node index "idx"
 *        idx range[1,list_node_num] 
 * @param list 
 * @param idx 
 * @return list_node* 
 */
list_node *slist_remove(slist *list, int idx)
{
    int passby;
    list_node *p;
    list_node *q;

    P(&mutex);
    if (!list || list->node_num == 0) {
        log_debug("list is empty\n");
        return NULL;
    }
    if (idx < 1 || idx > list->node_num) {
        log_debug("idx is out of range\n");
        return NULL;
    }

    passby = 0;
    p = list->head;
    while(passby != idx){
        q = p;
        p = p->next;
        passby++;
    }

    if(idx == list->node_num){  /* remove final node */
        list->tail = q;
    }
    q->next = p->next;
    list->node_num--;
    V(&mutex);
    return p;
}