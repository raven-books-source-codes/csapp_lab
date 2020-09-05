/**
 * @file proxy_single_cache.c proxy single thread with cache
 * @author raven (zhang.xingrui@foxmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-09-04
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "csapp.h"
#include "log.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/* macros defination */
#define BUF_SIZE 200
#define HTTP_PREFIX "http://"

/* global vars defination */

/* function declaration */
static void doit(int fd);
static int parse_client_request(int connfd, char *uri, char *versoin,
                                char *headers, char *hostname, char *port);
static int convert_request(char *src_uri, char *src_headers, char *src_version,
                           char *dest_uri, char *dest_headers,
                           char *dest_version);
static int read_requesthdrs(rio_t *rp, char *headers);
static int read_hostname_port_from_uri(char *uri, char *hostname, char *port);
static int fetch_data_from_server(char *request, size_t size, char *hostname,
                                  char *port, char *datap);
static int send_data_to_client(int fd, char *datap, size_t size);

int main(int argc, char const *argv[]) {
    int listenfd, connfd;
    char proxy_port[10];
    char client_hostname[BUF_SIZE];
    char client_port[10];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* log setting */
    log_set_quiet(false);
    /* parse port from cmd */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        log_error("need specify proxy port\n");
        exit(1);
    } else {
        strcpy(proxy_port, argv[1]);
    }

    /* start listen */
    listenfd = Open_listenfd(proxy_port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE,
                    client_port, MAXLINE, 0);
        log_info("Accepted connection from (%s, %s)\n", client_hostname,
                 client_port);
        doit(connfd);
        Close(connfd);
    }

    return 0;
}

/**
 * @brief
 *
 * @param fd
 */
static void doit(int fd) {
    char src_uri[BUF_SIZE], dest_uri[BUF_SIZE], src_version[BUF_SIZE],
        dest_version[BUF_SIZE], src_headers[BUF_SIZE], dest_headers[BUF_SIZE],
        hostname[BUF_SIZE], server_port[BUF_SIZE];
    char *buf = (char *)malloc(sizeof(char) * MAX_OBJECT_SIZE);
    char *request_to_server = (char *)malloc(sizeof(char) * MAXBUF);
    if (!buf || !request_to_server) {
        log_error(
            "allocate memory for request which is sending to server faild\n");
        return;
    }

    /* init */
    memset(src_uri, 0, BUF_SIZE);
    memset(src_version, 0, BUF_SIZE);
    memset(src_headers, 0, BUF_SIZE);
    memset(hostname, 0, BUF_SIZE);
    memset(buf, 0, BUF_SIZE);
    memset(request_to_server, 0, BUF_SIZE);
    memset(server_port, 0, BUF_SIZE);

    /* parse request from client */
    if (parse_client_request(fd, src_uri, src_version, src_headers, hostname,
                             server_port)) {
        log_error("parse client request error\n");
        return;
    }
    log_info("request: %s\n", src_uri);
    log_info("http version: %s\n", src_version);
    log_info("headers: %s\n", src_headers);
    log_info("hostname: %s\n", hostname);

    /* convert uri , headers, version to the uri headers version that will be
     * send to server */
    /* append HOSTNAME first */
    sprintf(buf, "Host: %s\r\n", hostname);
    strcat(src_headers, buf);
    if (convert_request(src_uri, src_headers, src_version, dest_uri,
                        dest_headers, dest_version)) {
        log_error("convert request failed\n");
        return;
    }

    /* constrcut request sending to server */
    sprintf(buf, "GET %s %s\r\n", dest_uri, dest_version);
    strcat(request_to_server, buf);
    strcat(request_to_server, dest_headers);
    strcat(request_to_server, "\r\n"); /* end of requst */
    log_info("coneverted request to server:\n%s\n", request_to_server);

    /* fetch data from server */
    int read_len = -1;
    if ((read_len = fetch_data_from_server(request_to_server,
                                           strlen(request_to_server) + 1,
                                           hostname, server_port, buf)) == -1) {
        log_error("fetch data from server failed\n");
        return;
    }
    log_info("fetch data from server sucess\n");

    /* send data to client */
    int write_len = -1;
    if ((write_len = send_data_to_client(fd, buf, read_len)) == -1) {
        log_error("send data to client failed\n");
        return;
    }
    log_info("send data to client sucess\n");

    /* free resource */
    free(request_to_server);
    free(buf);
}

/**
 * @brief parse request from client
 *        1. get origin uri, and store it into "uri"
 *        2. get origin version, and store it into "version"
 *        3. get origin headers, and store t into "headers"
 *        4. extract hostname from uri, and store it into "hostname"
 *        5. extract port from uri, and store it into "port"
 *
 * @param connfd
 * @param uri
 * @param version
 * @param headers
 * @param hostname
 * @param port
 * @return 0 sucess
 *          -1 failed
 */
static int parse_client_request(int connfd, char *uri, char *version,
                                char *headers, char *hostname, char *port) {
    char buf[BUF_SIZE], method[BUF_SIZE];
    rio_t rio;

    if (!uri || !version || !headers || !hostname) {
        log_debug("ethier uri, version,headers or hostname is NULL\n");
        return -1;
    }

    /* init */
    memset(buf, 0, BUF_SIZE);
    memset(method, 0, BUF_SIZE);

    rio_readinitb(&rio, connfd);
    /* read request line and headers */
    /* GET : get uri, versoin field*/
    if (!rio_readlineb(&rio, buf, BUF_SIZE)) {
        return -1;
    }
    log_debug("request line: %s\n", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
        log_error("doest supoort other request method except GET\n");
        return -1;
    }
    /* get headers */
    if (read_requesthdrs(&rio, headers)) {
        log_error("request headers failed\n");
        return -1;
    }
    /* get hostname from uri*/
    if (read_hostname_port_from_uri(uri, hostname, port)) {
        log_error("get hostname port failed\n");
        return -1;
    }
    return 0;
}

/**
 * @brief get headers from rp
 *        only get headers which are not include in (Hostname, User-Agent,
 * Connection, Proxy-Connection)
 *
 * @param rp
 * @param headers
 * @return int 0 sucess
 *             -1 failed
 */

/* 1 pass, 0 not pass */
static inline int will_header_pass_by(char *msg) {
    if (!msg) return 1;

    static int len1 = strlen("Host");
    static int len2 = strlen("User-Agent");
    static int len3 = strlen("Connection");
    static int len4 = strlen("Proxy-Connection");

    if (!strncasecmp("Host", msg, len1) ||
        !strncasecmp("User-Agent", msg, len2) ||
        !strncasecmp("Connection", msg, len3) ||
        !strncasecmp("Proxy-Connection", msg, len4)) {
        return 1;
    }
    return 0;
}
static int read_requesthdrs(rio_t *rp, char *headers) {
    char buf[BUF_SIZE];
    int ret;

    if (headers == NULL) {
        log_debug("header pointer is null\n");
        return -1;
    }
    *headers = '\0';

    while ((ret = rio_readlineb(rp, buf, BUF_SIZE)) != 0) {
        if (ret == -1) {
            log_debug("read request headers error\n");
            return -1;
        }
        if (!strcmp(buf, "\r\n")) { /* End of request */
            break;
        }
        if (will_header_pass_by(buf)) continue;

        log_debug("%s\n", buf);
        strcat(headers, buf);
    }
    return 0;
}

/**
 * @brief convert src uri/headers/version to dest uri/headers/version
 *      src: client
 *      dest: server
 * for example:
 *  uri: GET http://www.cmu.edu/hub/index.html HTTP/1.1
 *  will be converted to:
 *       GET /hub/index.html HTTP/1.0
 *  headers:
 *       read the writeup of the proxylab
 *       the following four fileds are special:
 *       Host: www.cmu.edu
 *       User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3)
Gecko/20120305 Firefox/10.0.3
 *       Connection: close
 *      Proxy-Connection: close
 *      other fileds will be just forward, that means we change
 *      nothing but store it into "dest_headers"
 * @param src_uri
 * @param src_headers
 * @param src_version
 * @param dest_uri
 * @param dest_headers
 * @param dest_version
 * @return int 0 sucecss
 *              -1 failed
 */
static int convert_request(char *src_uri, char *src_headers, char *src_version,
                           char *dest_uri, char *dest_headers,
                           char *dest_version) {
    char *p = src_uri;

    if (!src_uri || !src_headers || !src_version || !dest_uri ||
        !dest_headers || !dest_version) {
        log_debug("convert request params have nullptr\n");
        return -1;
    }

    /* src uri --> dest uri */
    if (strncasecmp(HTTP_PREFIX, src_uri, strlen(HTTP_PREFIX))) {
        log_debug("uri not start with http://\n");
        return -1;
    }
    p += strlen("http://");
    while (*p != '\0' && *p != '/') { /* looking for next backslash */
        p++;
    }
    strcpy(dest_uri, p);
    p = NULL;

    /* src_headers --> dest_headers */
    strcpy(dest_headers, src_headers);
    /* append USER-AGENT CONNECTION PROXY-CONNECT */
    strcat(dest_headers, user_agent_hdr);
    strcat(dest_headers, "Connection: Close\r\n");
    strcat(dest_headers, "Proxy-Connection: close\r\n");

    /* src version --> dest verison HTTP/1.1 --> HTTP/1.0*/
    strcpy(dest_version, src_version);
    int version_len = strlen(dest_version);
    if (dest_version[version_len - 1] == '1') {
        dest_version[version_len - 1] = '0';
    }
    return 0;
}

/**
 * @brief read hostname  and portfrom uri, and store it to "hostname" "port"
 *
 * @param uri
 * @param hostname
 * @param port
 * @return int 0 sucess, -1 failed
 */
static int read_hostname_port_from_uri(char *uri, char *hostname, char *port) {
    char *start = NULL, *end = NULL;
    int len = strlen(HTTP_PREFIX);
    if (!uri || !hostname || !port) {
        log_debug("uri or hostname is NULL\n");
        return -1;
    }

    /* default port */
    strcpy(port, "80");
    /* do extract hoasname */
    if (strncasecmp(HTTP_PREFIX, uri, len)) {
        log_debug("uri not start with http://");
        return -1;
    }

    start = uri + len;
    end = start;
    while (*end != '/') {
        if (*end == ':') { /* set server port */
            char *p = NULL;
            p = strstr(end, "/");
            strncpy(port, end+1, p - end-1);
            port[p - end-1] = '\0';
            break;
        } /* in case of "http://www.baidu.com:8080/xxx" */
        end++;
    }
    strncpy(hostname, start, end - start);
    log_debug("hostname: %s\n", hostname);
    log_debug("server port: %s\n", port);
    return 0;
}

/**
 * @brief fetch data from server
 *
 * @param request request, containg request line(GET http://xx) and headers
 * @param size requst string len
 * @param hostname
 * @param port
 * @param datap data return from server
 * @return int sucess read len (>=0)
 *             failed -1
 */
static int fetch_data_from_server(char *request, size_t size, char *hostname,
                                  char *port, char *datap) {
    int clientfd;
    int ret, read_len;
    // char buf[MAX_OBJECT_SIZE];
    char *buf = (char *)malloc(sizeof(char) * MAX_OBJECT_SIZE);
    if (!buf) {
        log_error("allocate buffer error\n");
        return -1;
    }

    if (!request || !datap || !hostname || !port) {
        log_debug("request is null or datap is null\n");
        return -1;
    }

    clientfd = open_clientfd(hostname, port);
    if (clientfd == -1 || clientfd == -2) {
        log_debug("open clientfd failed\n");
        return -1;
    }
    ret = rio_writen(clientfd, request, size);
    if (ret == -1) {
        log_debug("request to server failed\n");
        return -1;
    }
    ret = rio_readn(clientfd, buf, MAX_OBJECT_SIZE);
    if (ret == -1) {
        log_debug("read from server failed\n");
        return -1;
    }
    read_len = ret;
    strncpy(datap, buf, read_len);

    free(buf);
    close(clientfd);
    return read_len;
}

/**
 * @brief send data which comes from server to client.
 *
 * @param fd connect fd to client
 * @param datap
 * @param size data size
 * @return int sucess write len
 *             failed -1
 */
static int send_data_to_client(int fd, char *datap, size_t size) {
    int ret;
    if (fd < 0 || !datap) {
        log_debug("params error\n");
        return -1;
    }

    ret = rio_writen(fd, datap, size);
    if (ret == -1) {
        log_debug("send to client error\n");
        return -1;
    }
    return ret;
}