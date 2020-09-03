#include <stdio.h>

#include "csapp.h"
#include "sbuf.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/* 宏定义 */
#define DEBUG
#define PROT "12344"    /*  proxy端口，TODO: 端口改用解析的形式 */
#define THREAD_NUM 1    /*  线程池中的线程数 */
#define MAX_TASK_NUM 10 /* 最大接受的请求数 */
#define BUF_SIZE 200    /*  BUF SIZE */

/* 全局变量定义 */
static sbuf_t task_queue; // 任务队列

/* 函数声明 */
/**
 * @brief 线程池初始化，线程池中共 thread_num 个线程， 每个线程默认执行 routine
 *
 * @param thread_num
 * @param routine
 * @return int 0 启动成功
 *             -1 启动失败
 */
static int thread_pool_init(int thread_num, void *(*routine)(void *));

/**
 * @brief 线程执行routine
 *
 * @param vargs
 * @return void*
 */
static void *thread_routine(void *vargs);

/**
 * @brief proxy线程工作函数，完成所有代理线程的工作。包括：
 * 1. 客户请求字符参数获取
 * 2. 请求参数解析
 * 3. 从server中获取数据并发挥给client
 *
 * @param connd 与client连接后的fd
 * @return void*
 */
static void *proxy(int connd);

/**
 * @brief
 * 获取并解析请求参数字符串，忽略不必要headers，转换为可向server转发的请求参数，将结果保存到buf中
 *
 * @param connfd
 * @param bufp 将转换后的字符串存放到buf中
 *             转换成功： bufp存放正确字符串    return 0
 *             转换失败： return -1
 *             失败场景：args无效，如无GET关键字
 * @return 0 success
 *         -1 failed
 */
static int parse_request_args(int connfd, char *bufp);

/* 解析GET行，保存主机名到hostname
    return: 解析后的字符串。如:
    line = GET http://www.cmu.edu/hub/index.html HTTP/1.1
    返回: GET /hub/index.html HTTP/1.0
    发生错误时：返回NULL
 */
static char *parse_GET_url(char *line, char *hostname);

/**
 * @brief 传递参数args 给 server端(host:port) 并获取数据，结果存放于buf中
 *
 * @param host host name
 * @param port host port
 * @param args 传递给server的参数
 * @param bufp 结果保存
 *             取回数据失败时， *bufp=NULL
 */
static void fetch_data_from_server(char *host, char *port, char *args,
                                   char **bufp);

/* 缓存数据 */
static void cache(char *bufp);

int main(int argc, char const *argv[]) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    clientlen = sizeof(struct sockaddr_storage);
    thread_pool_init(THREAD_NUM, thread_routine);
    sbuf_init(&task_queue, MAX_TASK_NUM);
    listenfd = Open_listenfd(PROT);

    while (1) {
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
#ifdef DEBUG
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port,
                    MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
#endif // DEBUG
       sbuf_insert(&task_queue, connfd);
    }
    return 0;
}

static int thread_pool_init(int thread_num, void *(*routine)(void *)) {
    pthread_t tid = -1;
    for (size_t i = 0; i < thread_num; i++) {
        if (pthread_create(&tid, NULL, routine, NULL) != 0)
            return -1;
    }
    return 0;
}

static void *thread_routine(void *vargs) {
    /* 进入detach模式，防止内存泄漏 */
    pthread_detach(pthread_self());
    int connfd = -1;

    while (1) {
        connfd = sbuf_remove(&task_queue);
        proxy(connfd);
        Close(connfd);
    }
}

static void *proxy(int connd) {
    int ret;
    char serverbuf[BUF_SIZE];

    /* parse request args */
    ret = parse_request_args(connd, serverbuf);
    if (ret == -1) {
        printf("parse request args error\n");
        return NULL;
    }
#ifdef DEBUG
    printf("converted args\n%s\n", serverbuf);
#endif // DEBUG
}

static int parse_request_args(int connfd, char *serverbuf) {
    if(serverbuf == NULL || connfd == -1){
        return -1;
    }

    char line[BUF_SIZE];
    char tempbuf[BUF_SIZE];
    char hostname[40]; /* 主机名 */
    char *tempptr;
    rio_t rio;

    memset(line, 0, sizeof(line) / sizeof(char));  
    memset(tempbuf, 0, sizeof(tempbuf) / sizeof(char));
    memset(hostname, 0, sizeof(hostname) / sizeof(char));
    Rio_readinitb(&rio, connfd);
    /* 解析客户端字符串 */
    while ((Rio_readlineb(&rio, line, MAXLINE)) != 0) {
#ifdef DEBUG
        printf("%s\n", line);
#endif // DEBUG
        char *p = line;
        while (*p == ' ') /* skip head space */
            p++;

        if (!strncmp(p, "GET", 3)) { /* GET */
            tempptr = parse_GET_url(line, hostname);
            if (!tempptr) {           /* 解析GET字符串不成功 */
                return -1;
            } 
            break;
        }
    }

    /* 填充serverbuf, 指针tempptr存放着转换后的GET xxx */
    strcpy(serverbuf, tempptr); /* GET */
    strcat(serverbuf, "\r\n");
    free(tempptr);
    sprintf(tempbuf, "HOST: %s", hostname); /* HOST: */
    strcat(serverbuf, tempbuf);
    strcat(serverbuf, "\r\n");
    strcat(serverbuf, "Connection: close\r\n"); /* Connection: close */
    strcat(serverbuf, "Proxy-Connection: close\r\n");
    serverbuf[strlen(serverbuf)] = '\0';

    return 0;
}

/* /home/raven/Projects/csapp_lab/proxylab/proxylab-handout/proxy */
static char *parse_GET_url(char *line, char *hostname) {
    if(line == NULL || hostname == NULL){
        return NULL;
    }

    size_t len = strlen(line);
    char *cstr = (char *)malloc(sizeof(char) * len);

    line = line + 4;                   /* skip "GET+one space" */
    if (strncmp(line, "http://", 7)) { /* only support http */
        return NULL;
    }
    line = line + 7; /* 指向 "http://"的后一位 */

    /* parse host name */
    char *start = line;
    while (*line != '/') {
        line++;
    }
    strncpy(hostname, start, (size_t)(line - start));
    hostname[strlen(hostname)] = '\0';
#ifdef DEBUG
    printf("hostname: %s\n", hostname);
#endif // DEBUG

    /* 拼接转换后的字符串 */
    strcpy(cstr, "GET ");
    strcat(cstr, line);
    len = strlen(cstr);
    if (cstr[len - 3] == '1') { /* HTTP/1.1 --> HTTP/1.0 */
        cstr[len - 3] = '0';
    }
    cstr[len-2] = '\0';
    return cstr;
}
