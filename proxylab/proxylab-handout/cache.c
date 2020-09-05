#include <limits.h>
#include "cache.h"
#include "csapp.h"
#include "log.h"
#include <semaphore.h>

/**
 *  reader-writer lock
 *  writer priority
 */
static int rcounter = 0;
sem_t rcounter_mutex;
sem_t res_mutex;
sem_t w_mutex;     /* for writer-priority */


/**
 * LRU evict(a sucessc atually, this function do nothing but
 * just find the block that need to be evict
 * @param cp
 * @return empty block index
 */
static int evict(cache *cp)
{
    int min_age_idx = -1;
    unsigned long long age = LONG_MAX;
    for (size_t i = 0; i < cp->ci_num; i++) {
        if (age > cp->ci[i].age) {
            age = cp->ci[i].age;
            min_age_idx = i;
        }
    }
    return min_age_idx;
}


/**
 * cache init
 * @param cp
 * @return 0 sucess
 *         -1 failed
 */
int cache_init(cache *cp)
{
    /* cache_get cache item pair_num */
    const int ci_num = MAX_CACHE_SIZE / MAX_OBJECT_SIZE;
    cp->ci = (cache_item *) malloc(sizeof(cache_item) * ci_num);
    cp->ci_num = ci_num;
    /* init every cache block */
    for (size_t i = 0; i < cp->ci_num; i++) {
        cp->ci[i].effect_size = 0;
    }
    
    /* semaphore init */
    sem_init(&rcounter_mutex, 0, 1);
    sem_init(&res_mutex, 0, 1);
    sem_init(&w_mutex, 0, 1);
    return 0;
}

/**
 * cache_put data into cache
 * @param cp
 * @param key
 * @param data
 * @param size
 * @return  0 sucess
 */
int cache_put(cache *cp, char *key, char *data, size_t size)
{
    static unsigned long long age_counter = 0;
    int empty_block_idx = -1;
    cache_item *ci;
    
    /* write start */
    P(&w_mutex);
    P(&res_mutex);
    
    /* increment age counter */
    age_counter++;
    /* first try to an empty block */
    for (size_t i = 0; i < cp->ci_num; i++) {
        if (cp->ci[i].effect_size == 0) {
            empty_block_idx = i;
            break;
        }
    }
    
    /* need evict ?*/
    if (empty_block_idx == -1) {
        empty_block_idx = evict(cp);
    }
    
    ci = &cp->ci[empty_block_idx];
    memcpy(ci->key, key, strlen(key));
    memcpy(ci->data, data, size);
    ci->effect_size = size;
    ci->age = age_counter;
    
    /* write end*/
    V(&res_mutex);
    V(&w_mutex);
    return 0;
}

/**
 * cache_get data from cache in terms of key
 * @param cp
 * @param key
 * @return
 *         NULL not found
 */
cache_item *cache_get(cache *cp, char *key)
{
    cache_item *ci = NULL;
    
    /* read start*/
    P(&w_mutex);
    P(&rcounter_mutex);
    rcounter++;
    if (rcounter == 1)
        P(&res_mutex);
    V(&rcounter_mutex);
    V(&w_mutex);
    
    for (size_t i = 0; i < cp->ci_num; i++) {
        if (cp->ci[i].effect_size != 0 &&
            !strcmp(cp->ci[i].key, key)) {
            ci = &cp->ci[i];
            break;
        }
    }
    
    P(&rcounter_mutex);
    rcounter--;
    if (rcounter == 0)
        V(&res_mutex);
    V(&rcounter_mutex);
    return ci;
}

/**
 * deinit
 * @param cp
 * @return
 */
int cache_deinit(cache *cp)
{
    if (!cp) return -1;
    
    for (size_t i = 0; i < cp->ci_num; i++) {
        free(&cp->ci[i]);
    }
    return 0;
}
