#include "cache.h"
#include <string.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

struct pair {
    char key[100];
    char data[MAX_OBJECT_SIZE];
};

static cache mycache;
const int pair_num = 10;

static void *thread(void *p)
{
    pthread_detach(pthread_self());
    struct pair *pairs = (struct pair *) malloc(sizeof(struct pair) * pair_num);
    cache_item *ci;
    
    for (size_t i = 0; i < pair_num; i++) {
        sprintf(pairs[i].key, "%lu", i);
        sprintf(pairs[i].data, "%lu", i);
    }
    
    /* cache init and cache_put data into cache */
    cache_init(&mycache);
    for (size_t i = 0; i < pair_num; i++) {
        printf("thread %lu put\n", pthread_self());
        cache_put(&mycache, pairs[i].key,
                  pairs[i].data, strlen(pairs[i].data) + 1);
    }
    
    /* get data from cache */
    for (size_t i = 0; i < pair_num; i++) {
        printf("thread %lu get\n", pthread_self());
        ci = cache_get(&mycache, pairs[i].key);
        if (ci) printf("key:%s, data:%s\n", ci->key, ci->data);
    }
}

int main(int argc, char const *argv[])
{
    const int thread_num = 2;
    cache_item *ci;
    pthread_t tids[thread_num];
    /* create bunch of threads */
    for (size_t i = 0; i < thread_num; i++) {
        pthread_create(&tids[i], NULL, thread, NULL);
    }
    
    /* wait for thread over*/
    for (size_t i = 0; i < thread_num; i++) {
        pthread_join(tids[i], NULL);
    }
    
    /* print cache info */
    printf("main thread\n");
    for (size_t i = 0; i < mycache.ci_num; i++) {
        printf("%s\n", mycache.ci[i].key);
    }
    return 0;
}
