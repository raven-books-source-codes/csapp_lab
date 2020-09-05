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

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct{
   char key[100] ;     /* use request path as key */
   char data[MAX_OBJECT_SIZE];
   size_t effect_size;      /* effect size */
   unsigned long long age;      /* use to implement LRU */
}cache_item;

typedef struct{
    cache_item *ci;
    size_t ci_num;     /* cache item pair_num */
}cache;

int cache_init(cache *cp);
/* cache_put */
int cache_put(cache *cp, char *key, char *data, size_t size);
/* cache_get */
cache_item *cache_get(cache *cp, char *key);
int cache_deinit(cache *cp);




#endif