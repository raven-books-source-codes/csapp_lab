#include "cachelab.h"

#define _GNU_SOURCE         // getline函数不属于c标准, 需要开启GNU扩展

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <string.h>

/*--------------------------前置定义-----------------------*/
#define FILE_NAME_LEN 100
#define OS_PTR_LEN 64
typedef unsigned long long address;     // 64-bit address

#define DEBUG_HIT(verbose) do{if(verbose) printf("hit\n");}while(0)
#define DEBUG_MISS(verbose) do{if(verbose) printf("miss\n");}while(0)
#define DEBUG_EVICT(verbose) do{if(verbose) printf("evict\n");}while(0)
/*--------------------------数据结构定义-----------------------*/
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
    L = 0,                           // load
    M = 1,                           // modify
    S = 2                            // store
} op_mode;

typedef struct trace_item {
    op_mode mode;                 // 操作模式
    address addr;                 // 地址
//    unsigned int access_size;       // 访问的内存单元数(byte为单位), unsigned int 可以 typedef，但是没想到好名字
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

/*--------------------------全局变量定义----------------------*/
char *usage = "Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
              "Options:\n"
              "  -h         Print this help message.\n"
              "  -v         Optional verbose flag.\n"
              "  -s <num>   Number of set index bits.\n"
              "  -E <num>   Number of lines per set.\n"
              "  -b <num>   Number of block offset bits.\n"
              "  -t <file>  Trace file.\n";             // example 略...
// 结果集合
unsigned int hits, miss, evicts;

/*--------------------------操作function定义----------------------*/
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
    
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'v':
                p->v = 1;
                break;
            case 's':
                p->s = atoi(optarg);
                break;
            case 'E':
                p->E = atoi(optarg);
                break;
            case 'b':
                p->b = atoi(optarg);
                break;
            case 't':
                sprintf(p->t, "%s", optarg);
                break;
            case 'h':
            case '?':
            default:
                printf("%s\n", usage);
                return -1;
        }
    }
    return 0;
}


/**
 * 根据字符c_mode,返回对应的op_mode枚举类型
 * @param c_mode
 * @return
 */
op_mode parse_op_mode(char c_mode)
{
    switch (c_mode) {
        case 'L':
            return L;
        case 'M':
            return M;
        case 'S':
            return S;
        default:
            perror("can't get op mode: invalid char!");
            exit(-1);
    }
}

/**
 * 解析trace file
 * @param trace_item_ptr 解析结构放置的地方
 * @param trace_item_num trace数组长度
 * @param path 解析文件路径
 * @return  有效的trace item数量
 *          -1 解析失败
 */
size_t parse_trace_file(trace_item **trace_item_ptr, size_t *trace_item_num, const char *path)
{
    FILE *file = NULL;
    char *line = NULL;
    trace_item *trace_items = *trace_item_ptr;
    size_t len = 0;
    if (!(file = fopen(path, "r"))) {
        perror("trace file not exit");
        return -1;
    }
    
    // read entry line by line
    int i = 0;
    while (getline(&line, &len, file) != -1) {
        if (line[0] == 'I')  // I 指令不需要解析
            continue;
        
        if (*trace_item_num <= i) {
            // line 过多,超过trace_item_num, 需要重新分配
            int new_num = *trace_item_num;
            do {                    // 保证new_num 一定要比i大
                new_num *= 2;
            } while (new_num <= i);
            
            trace_items = realloc(trace_items, new_num * sizeof(trace_item));
            if (!trace_items) {
                perror("realloc memory for trace item failed\n");
                exit(-1);
            }
            *trace_item_ptr = trace_items;
            // initialize  new alloc memory
            memset(trace_items + (*trace_item_num), 0, (new_num - *trace_item_num) * sizeof(trace_item));
            // update item_num (包含了无效的项)
            *trace_item_num = (*trace_item_num) << 1;
        }
        // mode
        trace_items[i].mode = parse_op_mode(line[1]);
        
        // address
        char addr[OS_PTR_LEN + 1];
        int j = 3;      // 代表line中"address"在line中的索引位置
        int offset = 0;
        // 跳过所有
        while (!isdigit(line[j]) && isspace(line[j]))
            ++j;
        offset = j;
        // 设置address
        while (line[j] != ',') {
            addr[j - offset] = line[j];
            j++;
        }
        addr[j - offset] = '\0';
        trace_items[i].addr = strtol(addr, NULL, 16);
        
        // byte_nums note: 实际仿真中,这个字段是没用的
//        char buf[10];
//        j++;     // 更新至第一个byte num索引索引位置
//        offset = j;
//        while (line[j] != '\n') {
//            buf[j - offset] = line[j];
//            j++;
//        }
//        buf[j] = '\0';
//        trace_items[i].access_size = atoi(buf);
        
        // update idx
        i++;
    }
    
    // 删除无用项
    *trace_item_num = i;
    *trace_item_ptr = (trace_item *) realloc(*trace_item_ptr, sizeof(trace_item) * (*trace_item_num));
    
    fclose(file);
    return 0;
}

/**
 * 初始化系统cache
 * @param sys_cache
 */
void init_cache(cache *sys_cache, param *p)
{
    const int S = 1 << p->s;
    const int E = p->E;
//    const int B = 1 << p->b;
    
    set *sets = (set *) malloc(sizeof(set) * S);
    for (size_t i = 0; i < S; i++) {
        cache_line *cls = (cache_line *) malloc(sizeof(cache_line) * E);
        // initialize
        memset(cls, 0, sizeof(cache_line) * E);
        sets[i].lines = cls;
    }
    sys_cache->sets = sets;
}

/**
 * check是否命中
 * @param cache_set 需要检测的set
 * @param p    系统参数
 * @param tag  地址中的tag
 * @param cl_idx   如果命中,cl_idx表示当前命中的cacheline在set的中位置(从0开始索引)
 *                 如果未命中,cl_idx = -1
 */
void check_if_hit(set *cache_set, param *p, int tag, size_t *cl_idx)
{
    const int E = p->E;
    for (size_t i = 0; i < E; i++) {
        if (!cache_set->lines[i].valid) continue;
        if (cache_set->lines[i].tag == tag) {
            *cl_idx = i;
            return;
        }
    }
    *cl_idx = -1;
}
//TODO: check_if_hit和check_has_non_block可以做成一个函数,不过这里不考虑效率问题,所以就这样分了
/**
 * 检验 这组set中,是否还有空block
 * @param cache_set  待检验的set
 * @param p  系统参数
 * @param cl_idx  如果有空块,则cl_idx=找到的第一个空块
 *                如果没有空块,cl_idx=-1
 */
void check_hash_empty_block(set *cache_set, param *p, size_t *cl_idx)
{
    const int E = p->E;
    for (size_t i = 0; i < E; i++) {
        if (!cache_set->lines[i].valid) {
            *cl_idx = i;
            return;
        }
    }
    *cl_idx = -1;
}

/**
 * 采用LRU算法找到需要evict的cache line索引
 * @param cache_set 需要检查的set
 * @param p     系统参数
 * @param cl_idx cl_idx = 找到的cache line在set中的索引
 */
void find_evict_cache_line(set *cache_set, param *p, size_t *cl_idx)
{
    const int E = p->E;
    int min_age_idx = 0;
    for (size_t i = 1; i < E; i++) {
        if (cache_set->lines[min_age_idx].age > cache_set->lines[i].age) {
            min_age_idx = i;
        }
    }
    *cl_idx = min_age_idx;
}

/**
 * load 操作
 * @param cache_set
 * @param tag
 * @param age
 */
void do_load(set *cache_set, param *p, int tag, int age)
{
    const int verbose = p->v;
    // 首先看能否hit
    size_t cl_idx;
    check_if_hit(cache_set, p, tag, &cl_idx);
    if (cl_idx != -1) // 命中
    {
        hits++;
        cache_line *cl = &cache_set->lines[cl_idx];
        cl->tag = tag;
        cl->age = age;
        DEBUG_HIT(verbose);
        return;
    }
    
    // 未命中,查看是否有空块
    check_hash_empty_block(cache_set, p, &cl_idx);
    if (cl_idx != -1) // 有空块
    {
        miss++;
        cache_line *cl = &cache_set->lines[cl_idx];
        cl->valid = 1;
        cl->tag = tag;
        cl->age = age;
        DEBUG_MISS(verbose);
        return;
    }
    
    // 未命中,没有空块
    find_evict_cache_line(cache_set, p, &cl_idx);
    if (cl_idx != -1) // ecvit
    {
        miss++;
        DEBUG_MISS(verbose);
        cache_line *cl = &cache_set->lines[cl_idx];
        cl->tag = tag;
        cl->age = age;
        evicts++;
        DEBUG_EVICT(verbose);
        return;
    }
}

void do_store(set *cache_set, param *p, int tag, int age)
{
    const int verbose = p->v;
    size_t cl_idx;
    // 是否hit
    check_if_hit(cache_set, p, tag, &cl_idx);
    if (cl_idx != -1) // 命中
    {
        hits++;
        cache_set->lines[cl_idx].age = age;
        DEBUG_HIT(verbose);
        return;
    }
    
    // 是否有空块
    check_hash_empty_block(cache_set, p, &cl_idx);
    if (cl_idx != -1) // 空块
    {
        miss++;
        cache_set->lines[cl_idx].valid = 1;
        cache_set->lines[cl_idx].tag = tag;
        cache_set->lines[cl_idx].age = age;
        DEBUG_MISS(verbose);
        return;
    }
    
    // evcit
    find_evict_cache_line(cache_set, p, &cl_idx);
    if (cl_idx != -1) // evcit
    {
        miss++;
        DEBUG_MISS(verbose);
        cache_set->lines[cl_idx].tag = tag;
        cache_set->lines[cl_idx].age = age;
        evicts++;
        DEBUG_HIT(verbose);
        return;
    }
    
}

void do_modify(set *cache_set, param *p, int tag, int age)
{
    const int verbose = p->v;
    size_t cl_idx = -1;
    // hit?
    check_if_hit(cache_set, p, tag, &cl_idx);
    if (cl_idx != -1) {
        // hit , 之后的store也会hit
        hits += 2;
        cache_set->lines[cl_idx].age = age;
        DEBUG_HIT(verbose);
        DEBUG_HIT(verbose);
        return;
    }
    
    // 空块
    check_hash_empty_block(cache_set, p, &cl_idx);
    if (cl_idx != -1) {
        miss++;
        DEBUG_MISS(verbose);
        // 首先load
        cache_set->lines[cl_idx].valid = 1;
        cache_set->lines[cl_idx].tag = tag;
        cache_set->lines[cl_idx].age = age;
        // 再store
        hits++;
        DEBUG_HIT(verbose);
        return;
    }
    
    // evict
    find_evict_cache_line(cache_set, p, &cl_idx);
    if (cl_idx != -1) {
        miss++;
        DEBUG_MISS(verbose);
        // evict
        DEBUG_EVICT(verbose);
        evicts++;
        cache_set->lines[cl_idx].tag = tag;
        cache_set->lines[cl_idx].age = age;
        // store
        hits++;
        DEBUG_HIT(verbose);
        return;
    }
}

/**
 * 模拟仿真
 * @param sys_cache
 * @param trace_item
 * @param item_num
 */
void simulate(cache *sys_cache, param *p, trace_item *trace_items, const unsigned int item_num)
{
    const int s = p->s;
    const int b = p->b;
//    const int E = p->E;
    const int tag_bit = OS_PTR_LEN - s - b;
    
    // 构造set tag mask
    // set mask
    address set_mask = 1;
    for (size_t i = 0; i < s - 1; i++) {
        set_mask = set_mask << 1 | 1;
    }
    // tag mask
    address tag_mask = 1;
    for (size_t i = 0; i < tag_bit - 1; i++) {
        tag_mask = tag_mask << 1 | 1;
    }
    
    for (size_t i = 0; i < item_num; i++) {
        trace_item item = trace_items[i];
        address addr = item.addr;
        op_mode mode = item.mode;
//        int access_size = item.access_size;     // useless
        
        // step1: 得到set 和 tag
        int set_idx = addr >> b & set_mask;
        int tag = addr >> (b + s);
        
        // step2: 根据操作码来决定具体实施什么样的操作
        switch (mode) {
            case L:
                do_load(&sys_cache->sets[set_idx], p, tag, i);
                break;
            case M:
                do_modify(&sys_cache->sets[set_idx], p, tag, i);
                break;
            case S:
                do_store(&sys_cache->sets[set_idx], p, tag, i);
                break;
        }
    }
}

/*--------------------------操作function定义----------------------*/
int main(int argc, char *argv[])
{
    // 系统输入参数
    param sys_param;
    // 解析trace file
    size_t trace_item_len = 10;
    trace_item *trace_items = (trace_item *) malloc(trace_item_len * sizeof(trace_item));
    // 系统cache
    cache sys_cache;
    
    int ret;
    // 解析参数
    ret = parse_input_params(argc, argv, &sys_param);
    if (ret) {
        printf("parse input params failed\n");
        printf("%s\n", usage);
        return -1;
    }
    
    // 解析trace file
    ret = parse_trace_file(&trace_items, &trace_item_len, sys_param.t);
    if (ret) {
        printf("parse trace file failed");
        return -1;
    }
    
    // 初始化系统cache
    init_cache(&sys_cache, &sys_param);
    
    // 开始仿真
    simulate(&sys_cache, &sys_param, trace_items, trace_item_len);
    
    // 打印输出
    printSummary(hits, miss, evicts);
    return 0;
}
