/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * 显示链表分配器
 * 1.默认工作再32bit os, 指针大小为4字节
 * 2.对齐要求：8字节对齐
 * 3.采用LIFO的free策略
 * 4.find策略，首次适配
 * 5.sbrk无法收缩
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// #define DEBUG

// macros

#define WSIZE       4       /* Word and header/footer size (bytes) */ 
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) 

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))           
#define PUT(p, val)  (*(unsigned int *)(p) = (val))   
// 这里有很多强制类型转换，书上提到因为p是一个 void * 指针

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   
#define GET_ALLOC(p) (GET(p) & 0x1)                    

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - 3*WSIZE)                     
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - 3*WSIZE) 
// 当前block的“NEXT"指针域
#define NEXT_PTR(bp)    ((char *)(bp) - 2*WSIZE)
#define PREV_PTR(bp)    ((char *)(bp) - WSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  (*(NEXT_PTR(bp)))
#define PREV_BLKP(bp)  (*(PREV_PTR(bp))) 

//---------------global var-------------------
static void *free_list_head = NULL;
static void *free_list_tail = NULL;
static void *cibrk = NULL;      // chunk inside brk: 用于指向当前已分配的chunk的第一个可用地址, 从cibrk往后的内存空间均可分配

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void place(void *bp, size_t size);
static void *allocate_from_chunk(size_t size);
static void insert_to_free_list(void *bp);
static void delete_from_free_list(void *bp);

/**
 * @brief  打印分配情况
 * example:
 * payload 1 : start address : end address 
 */
static void print_allocated_info();

/* 
 * mm_init - initialize the malloc package.
 * 主要工作：
 * 1. 初始化root指针
 * 2. 通过sbrk在虚拟地址空间中，分配一个chunk
 */
int mm_init(void)
{
   // Create the inital empty heap
   // 申请一个块，用于存放root指针
    if( (free_list_head = mem_sbrk(WSIZE)) == (void *)-1 ){
        return -1;
    }

    free_list_tail = free_list_head;
    PUT(free_list_head, NULL); // 初始化root指针为NULL（0）

    // Extend the empty heap with a free block of CHUNKSIZE bytes
    if((cibrk = extend_heap(CHUNKSIZE/WSIZE)) == NULL){
        return -1;
    }
    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if( (long)(bp = mem_sbrk(size)) == -1 ){
        return NULL;
    }

    // Coalesce if the previous block was free
    // TODO: 合并操作检查, 分配一片chunk时，需要检查当前chunk的前面是否有free block，有就需要合并
    return coalesce(bp);
}

/* 
 * mm_malloc, 根据 size 返回一个指针，该指针指向这个block的payload首地址
 * 主要工作：
 * 1. size的round操作，满足最小块需求以及对齐限制
 * 2. 首先检查当前free list中是否有可以满足 asize(adjusted size) ，有则place，（place可能需要split)
 * 无则第3步
 * 3. 从当前heap中分配新的free block， 如果当前heap空间够用，则直接分配。不够用，则转第4步
 * 4. 通过sbrk分配一片chunk， 分配新的block给当前请求，最后返回
 * 第3/4步转至allocate_from_chunk函数
 * 
 */
void *mm_malloc(size_t size)
{
    size_t asize;      // Adjusted block size
    size_t extendsize; // Amount to extend heap if no fit
    char *bp;

    if (size == 0)
        return NULL;

    // step1: round size 满足最小块和对齐限制
    asize = ALIGN(2 * DSIZE + size);    // 2*DSIZE = header+ footer + next + prev

    // step2: 从free list 中找free block
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
    }
    //free list中找不到
    else
    {
        // step3: 从当前chunk中分配
        if((bp = allocate_from_chunk(asize)) == NULL)
        {
            return NULL;  
        }
        place(bp, asize);
    }

#ifdef DEBUG
    printf("malloc\n");
    print_allocated_info();
#endif
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);

#ifdef DEBUG
    printf("free\n");
    print_allocated_info();
#endif
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}


// 由于存在序言块和尾块，避免了一些边界检查。
static void *coalesce(void *bp)
{
    size_t pre_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(pre_alloc && next_alloc){    // case1: 前后都分配
        return bp;
    }

    else if(pre_alloc && !next_alloc){  // case 2： 前分配，后free
        void *next_block = NEXT_BLKP(bp);
        size += GET_SIZE(HDRP(next_block));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(next_block), PACK(size, 0));
    }

    else if(!pre_alloc && next_alloc){  // case 3: 前free，后分配
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else {      // 前后两个都是free
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;

}

/**
 * @brief 使用first-fit policy
 * 
 * @param size 
 * @return void* 成功，返回可用块的首地址
 *               失败，返回NULL
 */
static void *find_fit(size_t size)
{
    void *bp ;      

    for (bp = NEXT_BLKP(free_list_head); GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if(GET_ALLOC(HDRP(bp)) == 0 && size <= GET_SIZE(HDRP(bp)))
        {
            return bp;
        }
    }
    return NULL;
}

/**
 * @brief place block
 * 
 * @param bp 
 * @param size 
 */
static void place(void *bp, size_t size)
{
    size_t origin_size;
    size_t remain_size;
    size_t min_block_size = ALIGN(4 * WSIZE + 1);
    
    origin_size = GET_SIZE(HDRP(bp));
    remain_size = origin_size - size;
    if(remain_size >= min_block_size){
        // 可拆分
    }else{
        // 不可拆分
        // 更新header和footer
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        // 移除free list
        delete_from_free_list(bp);
    }
}

/**
 * @brief 从chunk中分配满足当前需求的块
 * 
 * @param size  需求size
 * @return void*  成功：当前能够分配的块的首地址
 *                失败： NULL， 一般只在run out out memory时才会NULL 
 */
static void *allocate_from_chunk(size_t size)
{
    char *cur_bp = (char *)cibrk;
    // mem_max_addr指向当前sbrk，即第一个不可用块，当前可用chunk后的第一个字节
    char *mem_max_addr = (char *)mem_heap_hi() + 1;
    size_t remain_size = (size_t)(mem_max_addr - cur_bp);
    size_t extend_size;

    // 满足需求：直接分配
    if(size > remain_size){
        // 不满足需求，需要扩展chunk
        // No fit found. Get more memory and place the block
        extend_size = MAX(size, CHUNKSIZE);
        if ((cur_bp = extend_heap(extend_size / WSIZE)) == NULL)
        {
            return NULL;
        }
    }

    // set block的基本信息 
    cur_bp += 3 * WSIZE;    // point to payload
    PUT(HDRP(cur_bp), PACK(size, 1));   // set header
    PUT(FTRP(cur_bp), PUT(size, 1));    // set footer

    // 插入到free list中
    insert_to_free_list(cur_bp);
    return cur_bp;
}

/**
 * @brief 将free block插入到free list中
 * 
 * @param bp free block的payload的首个地址
 */
static void insert_to_free_list(void *bp)
{
    void *head = free_list_head;
    void *p = GET(head);    // 当前首个有效节点 或者 NULL
    
    if(p == NULL){
        PUT(head, bp);
        free_list_tail = bp;
        PUT(NEXT_PTR(bp), NULL);
        PUT(PREV_PTR(bp), head);
    }
    else
    {
        // 更新当前要插入的节点
        PUT(NEXT_PTR(bp), p);
        PUT(PREV_PTR(bp), head);
        // 更新head
        PUT(head, bp);
        // 更新p节点(原首有效节点)
        PUT(PREV_PTR(p), bp);
    }
}

/**
 * @brief 从free list中删除 bp 所在节点
 * 
 * @param bp 
 */
static void delete_from_free_list(void *bp)
{
    void *prev_block = PREV_BLKP(bp);
    void *next_block = NEXXT_BLKP(bp);

    if(free_list_tail == next_block){
        PUT(NEXT_PTR(prev_block), NULL);
        free_list_tail = prev_block;
    }
    else
    {
        PUT(NEXT_PTR(prev_block), next_block);
        PUT(PREV_BLKP(next_block), prev_block);
    }
}

static void print_allocated_info()
{
    void *bp;
    size_t idx;

    printf("\n=============start allocated info===========\n");
    idx = 0;
    for (bp = NEXT_BLKP(free_list_head); GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if(GET_ALLOC(HDRP(bp)) == 1)
        {
            ++idx;
            size_t size = GET_SIZE(HDRP(bp));
            printf("payload %d %p:%p\n", idx, (char *)bp, (char *)(bp) + size - DSIZE);
        }
    }
    printf("=============end allocated info===========\n\n");
}