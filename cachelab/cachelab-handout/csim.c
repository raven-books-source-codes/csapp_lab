#include "cachelab.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/*--------------------------数据结构定义start-----------------------*/
#define FILE_NAME_LEN 100
typedef unsigned long long address;     // 64-bit address
/**
 * 参数
 */
typedef struct param {
    int v;                     // verbose
    int s;                     // set bits 数
    int E;                     // E-way
    int b;                     // block offset bits
    char t[FILE_NAME_LEN];     // trace file path
} param;

/**
 * cache line 结构
 */
typedef struct cache_line {
    int valid;                  // 有效位
    int tag;                    // tag
    int block_data;             // 存储load到cache的数据，但是由于是模拟，实际这个filed是没用的
    int age;                    // age，用于实现LRU替换策略
} cache_line;

/**
 *  trace item结构： 存储trace file中的一行数据（不包括I开头的行)
 *  op_mode: 操作模式： L(load), M(modify), S(store)
 */
typedef enum op_mode {
    L,                           // load
    M,                           // modify
    S                            // store
} op_mode;

typedef struct trace_item {
    op_mode mode;                 // 操作模式
    address addr;                 // 地址
    unsigned int byte_nums;       // 访问的内存单元数(byte为单位), unsigned int 可以 typedef，但是没想到好名字
} trace_item;

/**
 * set: cache中的set， 一个set可以由多个line组成
 * cache: 模拟的cache table，cache由多个set组成
 */
typedef struct set {
    cache_line *lines;
} set;

typedef struct cache {
    set *sets;
} cache;
/*--------------------------数据结构定义end----------------------*/

/*--------------------------全局变量定义start----------------------*/
// 系统参数(v,s,E等)
param sys_param;
char *usage = "Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
              "Options:\n"
              "  -h         Print this help message.\n"
              "  -v         Optional verbose flag.\n"
              "  -s <num>   Number of set index bits.\n"
              "  -E <num>   Number of lines per set.\n"
              "  -b <num>   Number of block offset bits.\n"
              "  -t <file>  Trace file.\n"
;             // example 略...
// 结果集合
unsigned int hits, miss, evits;
/*--------------------------全局变量定义end----------------------*/

/*--------------------------操作function定义start----------------------*/

/**
 * 解析输入参数
 * @param argc
 * @param argv
 * @param p  解析结果放置在p引用的内存中
 * @return 0 success parse
 *         -1 failed
 */
int parse_input_params(int argc, char *argv[], param *p)
{
    int opt;
    
    // put ':' in the starting of the
    // string so that program can
    // distinguish between '?' and ':'
    while((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch(opt)
        {
            case 'v':
                p->v = 1;
                break;
            case 's':
                p->s= atoi(optarg);
                break;
            case 'E':
                p->E = atoi(optarg);
                break;
            case 'b':
                p->b = atoi(optarg);
                break;
            case 't':
                sprintf(p->t,"%s",optarg);
                break;
            case 'h':
            case '?':
            default:
                printf("%s\n",usage);
                return -1;
        }
    }
    return 0;
}

/**
 * 解析trace file
 * @param trace_item_ptr 解析结构放置的地方
 * @param path 解析文件路径
 * @return  有效的trace item数量
 *          -1 解析失败
 */
unsigned int parse_trace_file(trace_item *trace_item_ptr, char *path)
{

}

/**
 * 初始化系统cache
 * @param sys_cache
 */
void init_cache(cache *sys_cache)
{

}

/**
 * 模拟仿真
 * @param sys_cache
 * @param trace_item_ptr
 * @param item_num
 */
void simulate(cache *sys_cache, trace_item *trace_item_ptr, unsigned int item_num)
{
    
}
/*--------------------------操作function定义start----------------------*/


/*--------------------------test函数----------------------*/

void test_parse(int argc, char *argv[])
{
    int ret;
    if(!(ret = parse_input_params(argc, argv,&sys_param)))
    {
        // parse success
        printf("v = %d, s = %d, E=%d, b=%d, t=%s\n",
               sys_param.v,sys_param.s,sys_param.E,sys_param.b,sys_param.t);
    } else {
        printf("parse params failed\n");
    }
}


int main(int argc, char *argv[])
{
    test_parse(argc,argv);
    return 0;
}
