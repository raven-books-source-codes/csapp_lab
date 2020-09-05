/**
 * @file slink_list.h thread_safe_link_list for proxy lab
 * @author raven (zhang.xingrui@foxmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-09-04
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef SLINK_LIST_H
#define SLINK_LIST_H
#include <stdio.h>
#include "cache.h"

struct map_item;
struct list_node;

/* link list node */
typedef struct list_node{
    map_item *itemp;
    struct list_node *next;
} list_node;

typedef struct slist{
    struct list_node *head;     /* list head */
    struct list_node *tail;
    size_t node_num; /* node num */
} slist;


int slist_init(slist *list);
int slist_deinit(slist *list);
int slist_push_back(slist *list, list_node *node);
int slist_insert(slist *list, size_t idx, list_node *node);
list_node *slist_pop(slist *list);
list_node *slist_remove(slist *list, int idx);

#endif