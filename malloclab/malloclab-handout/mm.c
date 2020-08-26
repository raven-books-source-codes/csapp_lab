/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * 显示链表分配器
 * 1.默认工作在32bit os, 指针大小为4字节
 * 2.对齐要求：8字节对齐
 * 3.采用LIFO的free策略
 * 4.find策略，首次适配
 * 5.sbrk无法收缩
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

// #define DEBUG

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
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (unsigned int)(val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-3 * WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - 4 * WSIZE)

// free block运算：计算当前block的“NEXT"指针域
// bp:当前block的payload首地址
#define NEXT_PTR(bp) ((char *)(bp)-2 * WSIZE)
#define PREV_PTR(bp) ((char *)(bp)-WSIZE)

// free block运算： 计算下一个free block的payload首地址
// bp:当前block的payload首地址
#define NEXT_FREE_BLKP(bp) ((char *)(*(unsigned int *)(NEXT_PTR(bp))))
#define PREV_FREE_BLKP(bp) ((char *)(*(unsigned int *)(PREV_PTR(bp))))

// virtual address计算：计算下一个block的payload首地址
// bp: 当前block的payload首地址
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(HDRP(bp) - WSIZE))

// 最小块
#define MIN_BLOCK ALIGN(4 * WSIZE + 1)

//---------------global declaration-------------------
static void *free_list_head = NULL;

static void *extend_heap(size_t words);
static void coalesce(void *bp);
static void *find_fit(size_t size);
static void place(void *bp, size_t size);
static void *allocate_from_chunk(size_t size);
static void insert_to_free_list(void *bp);
static void delete_from_free_list(void *bp);
static void *case1(void *bp);
static void *case2(void *bp);
static void *case3(void *bp);
static void *case4(void *bp);
static void debug();
static void print_allocated_info();
static void print_free_blocks_info();
static void consistent_check();
static int is_in_free_list(void *bp);

/* 
 * mm_init - initialize the malloc package.
 * 主要工作：
 * 1. 初始化root指针
 * 2. 通过sbrk在虚拟地址空间中，分配一个chunk
 */
int mm_init(void)
{
    // 申请一个块，用于存放root指针
    char *init_block_p;
    if ((init_block_p = mem_sbrk(MIN_BLOCK + WSIZE)) == (void *)-1)
    {
        return -1;
    }
    init_block_p = (char *)(init_block_p) + WSIZE; // 跳过首个对齐块

    free_list_head = init_block_p + 3 * WSIZE;
    PUT(PREV_PTR(free_list_head), NULL);
    PUT(NEXT_PTR(free_list_head), NULL); // 初始化root指针为NULL（0）
    PUT(HDRP(free_list_head), PACK(MIN_BLOCK, 1));
    PUT(FTRP(free_list_head), PACK(MIN_BLOCK, 1));

    // Extend the empty heap with a free block of CHUNKSIZE bytes
    if ((allocate_from_chunk(CHUNKSIZE)) == NULL)
    {
        return -1;
    }
    return 0;
}

/* 
 * mm_malloc, 根据 size 返回一个指针，该指针指向这个block的payload首地址
 * 主要工作：
 * 1. size的round操作，满足最小块需求以及对齐限制
 * 2. 首先检查当前free list中是否有可以满足 asize(adjusted size) ，有则place，（place可能需要split)
 * 无则第3步
 * 3. 从当前heap中分配新的free block， 插入到free list中，然后place
 * 
 */
void *mm_malloc(size_t size)
{
    size_t asize;      // Adjusted block size
    char *bp;

    if (size == 0)
        return NULL;

    // step1: round size 满足最小块和对齐限制
    asize = ALIGN(2 * DSIZE + size); // 2*DSIZE = header+ footer + next + prev

    // step2: 从free list 中找free block
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
    }
    //free list中找不到
    else
    {
        // step3: 从当前chunk中分配
        if ((bp = allocate_from_chunk(asize)) == NULL)
        {
            return NULL;
        }
        place(bp, asize);
    }

#ifdef DEBUG
    printf("malloc\n");
    debug();
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
    debug();
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

    // if (ptr == NULL)
    // {
    //     return mm_malloc(size);
    // }
    // if (size == 0)
    // {
    //     mm_free(ptr);
    //     return NULL;
    // }

    // // 当前block是否能满足realloc需求
    // size_t origin_size = GET_SIZE(HDRP(ptr));
    // if (origin_size > ALIGN(size+4*WSIZE))
    // {
    //     // 重新初始化
    //     PUT(HDRP(ptr), PACK(origin_size, 0));
    //     PUT(FTRP(ptr), PACK(origin_size, 0));
    //     insert_to_free_list(ptr);
    //     place(ptr, ALIGN(size+4*WSIZE));
    //     return ptr;
    // }
    // else
    
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

/**
 * @brief 扩展当前heap
 * 
 * @param words  需要扩展的words, 字为单位
 * @return void* 当前可用块的payload首地址
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
    {
        return NULL;
    }
    bp = (char *)(bp) + 3 * WSIZE; // point to payload
    // set 本块信息
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    // 插入到free list中
    insert_to_free_list(bp);
    return bp;
}

/**
 * @brief 前后都分配
 * 
 * @param bp 
 * @return void* 
 */
static void *case1(void *bp)
{
    insert_to_free_list(bp);
    return bp;
}

/**
 * @brief 前分配后未分配
 * 
 * @param bp 
 * @return void* 
 */
static void *case2(void *bp)
{
    void *next_blockp = NEXT_BLKP(bp);
    void *prev_free_blockp;
    void *next_free_blockp;
    size_t size = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(next_blockp));

    // 更新块大小
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(next_blockp), PACK(size, 0));

    // 更新前后free block指针
    prev_free_blockp = PREV_FREE_BLKP(next_blockp);
    next_free_blockp = NEXT_FREE_BLKP(next_blockp);

    // 边界检查
    if (next_free_blockp == NULL)
    {
        PUT(NEXT_PTR(prev_free_blockp), NULL);
    }
    else
    {
        PUT(NEXT_PTR(prev_free_blockp), next_free_blockp);
        PUT(PREV_PTR(next_free_blockp), prev_free_blockp);
    }

    insert_to_free_list(bp);
    return bp;
}

/**
 * @brief case 3 前一个block未分配，后一个块已分配
 * 
 * @param bp 当前块的payload首地址
 * @return void* 合并后的payload首地址
 */
static void *case3(void *bp)
{
    char *prev_blockp = PREV_BLKP(bp);
    char *prev_free_blockp;
    char *next_free_blockp;
    size_t size = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(prev_blockp));

    // 更新块大小
    PUT(HDRP(prev_blockp), PACK(size, 0));
    PUT(FTRP(prev_blockp), PACK(size, 0));

    // 找到前后free块并更新
    next_free_blockp = NEXT_FREE_BLKP(prev_blockp);
    prev_free_blockp = PREV_FREE_BLKP(prev_blockp);

    // 边界检查
    if (next_free_blockp == NULL)
    {
        PUT(NEXT_PTR(prev_free_blockp), NULL);
    }
    else
    {
        PUT(NEXT_PTR(prev_free_blockp), next_free_blockp);
        PUT(PREV_PTR(next_free_blockp), prev_free_blockp);
    }

    // LIFO策略，插入到free list的头部
    insert_to_free_list(prev_blockp);
    return bp;
}

/**
 * @brief 前后都未分配
 * 
 * @param bp 
 * @return void* 
 */
static void *case4(void *bp)
{
    void *prev_blockp;
    void *prev1_free_blockp;
    void *next1_free_blockp;
    void *next_blockp;
    void *prev2_free_blockp;
    void *next2_free_blockp;
    size_t size;

    prev_blockp = PREV_BLKP(bp);
    next_blockp = NEXT_BLKP(bp);

    // 更新size
    size_t size1 = GET_SIZE(HDRP(prev_blockp));
    size_t size2 = GET_SIZE(HDRP(bp));
    size_t size3 = GET_SIZE(HDRP(next_blockp));
    size = size1 + size2 + size3;
    PUT(HDRP(prev_blockp), PACK(size, 0));
    PUT(FTRP(next_blockp), PACK(size, 0));
    bp = prev_blockp;

    // 更新前半部 free block指针
    prev1_free_blockp = PREV_FREE_BLKP(prev_blockp);
    next1_free_blockp = NEXT_FREE_BLKP(prev_blockp);
    if (next1_free_blockp == NULL)
    {
        PUT(NEXT_PTR(prev1_free_blockp), NULL);
    }
    else
    {
        PUT(NEXT_PTR(prev1_free_blockp), next1_free_blockp);
        PUT(PREV_PTR(next1_free_blockp), prev1_free_blockp);
    }

    // 更新后半部 free block指针
    prev2_free_blockp = PREV_FREE_BLKP(next_blockp);
    next2_free_blockp = NEXT_FREE_BLKP(next_blockp);
    if (next2_free_blockp == NULL)
    {
        PUT(NEXT_PTR(prev2_free_blockp), NULL);
    }
    else
    {
        PUT(NEXT_PTR(prev2_free_blockp), next2_free_blockp);
        PUT(PREV_PTR(next2_free_blockp), prev2_free_blockp);
    }

    // 根据LIFO策略插入free list
    insert_to_free_list(bp);
    return bp;
}

/**
 * @brief 合并地址空间，并将可用free block插入到free list中
 * 
 * @param bp 当前block的payload首地址
 *           
 */
static void coalesce(void *bp)
{

    char *prev_blockp = PREV_BLKP(bp);
    char *next_blockp = NEXT_BLKP(bp);
    char *mem_max_addr = (char *)mem_heap_hi() + 1; // heap的上边界
    size_t prev_alloc = GET_ALLOC(HDRP(prev_blockp));
    size_t next_alloc;

    if (next_blockp >= mem_max_addr)
    { // next_block超过heap的上边界，只用考虑prev_blockp
        if (!prev_alloc)
        {
            case3(bp);
        }
        else
        {
            case1(bp);
        }
    }
    else
    {
        next_alloc = GET_ALLOC(HDRP(next_blockp));
        if (prev_alloc && next_alloc)
        { // case 1: 前后都已经分配
            case1(bp);
        }
        else if (!prev_alloc && next_alloc)
        { //case 3: 前未分配，后分配
            case3(bp);
        }
        else if (prev_alloc && !next_alloc)
        { // case 2: 前分配，后未分配
            case2(bp);
        }
        else
        { // case 4: 前后都未分配
            case4(bp);
        }
    }
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
    void *bp;

    for (bp = NEXT_FREE_BLKP(free_list_head); bp != NULL && GET_SIZE(HDRP(bp)) > 0; bp = NEXT_FREE_BLKP(bp))
    {
        if (GET_ALLOC(HDRP(bp)) == 0 && size <= GET_SIZE(HDRP(bp)))
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

    origin_size = GET_SIZE(HDRP(bp));
    remain_size = origin_size - size;
    if (remain_size >= MIN_BLOCK)
    {
        // 可拆分
        // 设置拆分后剩下的块的size和allocate情况
        char *remain_blockp = (char *)(bp) + size;
        PUT(HDRP(remain_blockp), PACK(remain_size, 0));
        PUT(FTRP(remain_blockp), PACK(remain_size, 0));
        // 更新指针，将剩下块加入到free list中
        char *prev_blockp = PREV_FREE_BLKP(bp);
        char *next_blockp = NEXT_FREE_BLKP(bp);
        PUT(NEXT_PTR(remain_blockp), next_blockp);
        PUT(PREV_PTR(remain_blockp), prev_blockp);
        PUT(NEXT_PTR(prev_blockp), remain_blockp);
        if (next_blockp != NULL)
        {
            PUT(PREV_PTR(next_blockp), remain_blockp);
        }

        // 设置分配的块
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        // 断开原block与free list的连接
        PUT(NEXT_PTR(bp), NULL);
        PUT(PREV_PTR(bp), NULL);
    }
    else
    {
        // 不可拆分
        // 更新header和footer
        PUT(HDRP(bp), PACK(origin_size, 1));
        PUT(FTRP(bp), PACK(origin_size, 1));
        // 移除free block from free list
        delete_from_free_list(bp);
    }
}

/**
 * @brief 从chunk中分配满足当前需求的块
 * 
 * @param size  需求size 字节为单位
 * @return void*  成功：当前能够分配的块的首地址
 *                失败： NULL， 一般只在run out out memory时才会NULL 
 */
static void *allocate_from_chunk(size_t size)
{
    void *cur_bp = NULL;
    size_t extend_size = MAX(size, CHUNKSIZE);
    if ((cur_bp = extend_heap(extend_size / WSIZE)) == NULL)
    {
        return NULL;
    }

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
    void *p = NEXT_FREE_BLKP(head); // 当前首个有效节点 或者 NULL

    if (p == NULL)
    {
        PUT(NEXT_PTR(head), bp);
        PUT(NEXT_PTR(bp), NULL);
        PUT(PREV_PTR(bp), head);
    }
    else
    {
        // 更新当前要插入的节点
        PUT(NEXT_PTR(bp), p);
        PUT(PREV_PTR(bp), head);
        // 更新head
        PUT(NEXT_PTR(head), bp);
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
    void *prev_free_block = PREV_FREE_BLKP(bp);
    void *next_free_block = NEXT_FREE_BLKP(bp);

    if (next_free_block == NULL)
    {
        PUT(NEXT_PTR(prev_free_block), NULL);
    }
    else
    {
        PUT(NEXT_PTR(prev_free_block), next_free_block);
        PUT(PREV_PTR(next_free_block), prev_free_block);
        // 断开连接
        PUT(NEXT_PTR(bp), NULL);
        PUT(PREV_PTR(bp), NULL);
    }
}

/**
 * @brief 打印free blocks信息
 * 
 */
static void print_free_blocks_info()
{
    void *bp;
    size_t idx = 0;

    printf("=============start free blocks info===========\n");
    for (bp = NEXT_FREE_BLKP(free_list_head); bp != NULL; bp = NEXT_FREE_BLKP(bp))
    {
        printf("free block%d range %p  %p size=%d, payload %p  %p block size=%d\n", idx, HDRP(bp), FTRP(bp) + WSIZE, FTRP(bp) - HDRP(bp) + WSIZE, (char *)bp, FTRP(bp), FTRP(bp) - (char *)(bp));
    }
    printf("=============end free blocks info===========\n\n");
}

/**
 * @brief  打印分配情况
 * example:
 * payload 1 : start address : end address 
 */

static void print_allocated_info()
{
    char *bp;
    size_t idx = 0;
    char *mem_max_addr = mem_heap_hi();

    printf("=============start allocated info===========\n");
    for (bp = NEXT_BLKP(free_list_head); bp < mem_max_addr && GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (GET_ALLOC(HDRP(bp)) == 1)
        {
            ++idx;
            printf("block%d range %p  %p size=%d, payload %p  %p block size=%d\n", idx, HDRP(bp), FTRP(bp) + WSIZE, FTRP(bp) - HDRP(bp) + WSIZE, (char *)bp, FTRP(bp), FTRP(bp) - (char *)(bp));
        }
    }
    printf("=============end allocated info===========\n\n");
}

static void consistent_check()
{
    // 检查free list中的所有block都为free
    char *bp;
    char *mem_max_heap = mem_heap_hi();

    for (bp = NEXT_FREE_BLKP(free_list_head); bp != NULL; bp = NEXT_FREE_BLKP(bp))
    {
        if (GET_ALLOC(HDRP(bp)))
        {
            printf("%d free list中存在块已分配\n", __LINE__);
        }
    }

    // 检查是否所有free block都在free list中
    for (bp = NEXT_BLKP(free_list_head); bp <= mem_max_heap; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && !is_in_free_list(bp))
        {
            printf("%d 存在free block %p 不在free list中\n", __LINE__, bp);
        }
    }
}

static int is_in_free_list(void *bp)
{
    void *p;
    for (p = NEXT_FREE_BLKP(free_list_head); p != NULL; p = NEXT_FREE_BLKP(p))
    {
        if (p == bp)
        {
            return 1;
        }
    }
    return 0;
}

static void debug()
{
    print_allocated_info();
    print_free_blocks_info();
    consistent_check();
}
