/**
 * @file cache.h cache for proxylab
 * @author raven (zhang.xingrui@foxmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-09-04
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef CACHE_H
#define CACHE_H
#include <stdio.h>

/* data item in cache */
typedef struct {
    char *datap;        /* data address base pointer */
    size_t size;        /* data size */
    unsigned long long age; /* for LRU eviction */
}data_item;

/* record mapping request path to data_item base pointer */
typedef struct{
    char *key;      /* use request path as key */
    data_item *data_itemp;  /* data item pointer */
}map_item;



#endif