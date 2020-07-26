#include "cachelab.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*--------------------------数据结构定义start-----------------------*/
#define FILE_NAME_LEN 100
#define OS_PTR_LEN 64
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
    L = 0,                           // load
    M = 1,                           // modify
    S = 2                            // store
} op_mode;

typedef struct trace_item {
    op_mode mode;                 // 操作模式
    address addr;                 // 地址
    unsigned int access_size;       // 访问的内存单元数(byte为单位), unsigned int 可以 typedef，但是没想到好名字
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
              "  -t <file>  Trace file.\n";             // example 略...
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
            // update item_num
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
        char buf[10];
        j++;     // 更新至第一个byte num索引索引位置
        offset = j;
        while (line[j] != '\n') {
            buf[j - offset] = line[j];
            j++;
        }
        buf[j] = '\0';
        trace_items[i].access_size = atoi(buf);
        
        // update idx
        i++;
    }
    
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
    const int B = 1 << p->b;
    
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
 * load 操作
 * @param cache_set
 * @param tag
 * @param age
 */
void do_load(set *cache_set, param *p, int tag, int age)
{
    // 第一次循环,检查是否有效
    const int E = p->E;
    int loaded = 0;
    for(int i = 0; i< E; i++)
    {
       if(!cache_set->lines[i].valid)
       {
           // 空快
           cache_set->lines[i].valid = 1;
           cache_set->lines[i].tag = tag;
           cache_set->lines[i].age = age;
           loaded = 1;
           break;
       }
    }
    // 空block已经loaded了
    if(loaded)
    {
        miss++;
        printf("miss\n");
        return;
    }
    
    // 第二次循环,没有空块, 比较tag是否相同
    for(size_t i = 0; i< E ; i++)
    {
        if(cache_set->lines[i].tag == tag)
        {
            // update age
            cache_set->lines[i].age = age;
            loaded = 1;
            break;
        }
    }
    // 如果tag有相同的
    if(loaded)
    {
        hits++;
        printf("hits\n");
        return;
    }
    
    // 第三次循环,没有空快,没有tag相同,使用LRU算法替换策略
    size_t min_age_idx = 0;
    for(size_t i = 1; i< E;i++)
    {
        if(cache_set->lines[min_age_idx].age > cache_set->lines[i].age)
        {
            min_age_idx = i;
        }
    }
    // 替换
    cache_set->lines[min_age_idx].tag = tag;
    cache_set->lines[min_age_idx].age = age;
    evits++;
    printf("evits\n");
}


void do_store(set *cache_set, param *p, int tag, int age)
{
    do_load(cache_set,p,tag,age);
}

void do_modify(set *cache_set, param *p, int tag, int age)
{
    do_load(cache_set,p,tag,age);
    do_store(cache_set,p,tag,age);
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
    const int E = p->E;
    const int tag_bit = OS_PTR_LEN - s - b;
    
    // 构造set tag mask
    // set mask
    int set_mask = 1;
    for (size_t i = 0; i < s; i++) {
        set_mask = set_mask << 1 | 1;
    }
    // tag mask
    int tag_mask = 1;
    for (size_t i = 0; i < tag_bit; i++) {
        tag_mask = tag_mask << 1 | 1;
    }
    
    for (size_t i = 0; i < item_num; i++) {
        trace_item item = trace_items[i];
        address addr = item.addr;
        op_mode mode = item.mode;
        int access_size = item.access_size;     // useless
        
        // step1: 得到set 和 tag
        int set_idx = addr >> b & set_mask;
        int tag = addr >> (b + s);
        
        // step2: 根据操作码来决定具体实施什么样的操作
        switch (mode) {
            case L:
                do_load(&sys_cache->sets[set_idx],p, tag, i);
                break;
            case M:
                do_modify(&sys_cache->sets[set_idx],p, tag, i);
                break;
            case S:
                do_store(&sys_cache->sets[set_idx],p, tag, i);
                break;
        }
    }
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
    int ret;
    // 解析参数
    ret = parse_input_params(argc,argv,&sys_param);
    if(ret)
    {
        printf("parse input params failed\n");
        printf("%s\n",usage);
        return -1;
    }
    // 解析trace file
    int trace_item_len = 20;
    trace_item *trace_items = (trace_item *)malloc(trace_item_len *sizeof(trace_item));
    ret = parse_trace_file(&trace_items,&trace_item_len, sys_param.t);
    if(ret)
    {
        printf("parse trace file failed");
        return -1;
    }
    
    // 初始化系统cache
    cache sys_cache;
    init_cache(&sys_cache,&sys_param);
    
    // 开始仿真
    simulate(&sys_cache,&sys_cache,trace_items, trace_item_len);
    
    // 打印输出
    printf("hist=%d, miss=%d, evits=%d",hits,miss,evits);
    return 0;
}
