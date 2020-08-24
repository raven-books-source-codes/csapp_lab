/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * 隐式链表分配器(csapp书上有源码，这里作为测试用)
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
// GET_ALLOC 很容易理解， 至于 GET_SIZE 也是类似，因为我们的 block size 总是双字对齐，所以它的大小总是后3位为0
// 我们这样做完全 GET(p) & (1111 ... 1000) 完全是 ok 的

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                     
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 
// HDRP header address 很容易理解，看图，bp - WSIZE 就是 header 的地址，同时也可以注意，这里我们也做了类型转换，把bp转成char * ，这样来计算才对。
// FTRP footer address 同样看图，是 bp + 整个块的大小 - DSIZE，就是我们先把bp移到普通快2的hdr， 然后减去两个WSIZE，也就是DSIZE，得到 footer address
// 这里我们同时也知道了GET_SIZE 是计算普通块1的大小，而不是普通块1的payload


/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) 
// 这两个计算方法同样也看图，next block pointer，实际上 ((char *)(bp) - WSIZE) 就是 HDRP，header address，我们得到整个块的大小，然后bp 加上整块大小，这里也知道我们的 address 总是指向 payload
// prev block pointer, ((char *)(bp) - DSIZE) 得到 previous block footer，计算之前的块的大小，然后bp - 之前的块大小，指向 payload

//---------------global var-------------------
static void *heap_listp = NULL;


static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void place(void *bp, size_t size);
/**
 * @brief  打印分配情况
 * example:
 * payload 1 : start address : end address 
 */
static void print_allocated_info();

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
   // Create the inital empty heap
    if( (heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1 ){
        return -1;
    }

    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);

    // Extend the empty heap with a free block of CHUNKSIZE bytes
    if(extend_heap(CHUNKSIZE/WSIZE) == NULL){
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

    // 初始化free block的header/footer和epilogue header
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    // Coalesce if the previous block was free
    return coalesce(bp);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;      // Adjusted block size
    size_t extendsize; // Amount to extend heap if no fit
    char *bp;

    if (size == 0)
        return NULL;

    // Ajust block size to include overhea and alignment reqs;
    if (size <= DSIZE)
    {
        asize = 2 * DSIZE;
    }
    else
    {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE); // 超过8字节，加上header/footer块开销，向上取整保证是8的倍数
    }

    // Search the free list for a fit
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
    }
    else
    {
        // No fit found. Get more memory and place the block
        extendsize = MAX(asize, CHUNKSIZE);
        if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
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
        // TODO: 其余两个tag不用清空？ 正常情况确实不用清空。
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

    for (bp = NEXT_BLKP(heap_listp); GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
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
    size_t remain_size;
    size_t origin_size;

    origin_size = GET_SIZE(HDRP(bp));
    remain_size = origin_size - size;
    if(remain_size >= 2*DSIZE)  // 剩下的块，最少需要一个double word (header/footer占用一个double word, pyaload不为空，再加上对齐要求)
    {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remain_size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(remain_size, 0));
    }else{
        // 不足一个双字，保留内部碎片
        PUT(HDRP(bp), PACK(origin_size, 1));
        PUT(FTRP(bp), PACK(origin_size, 1));
    }
}

static void print_allocated_info()
{
    void *bp;
    size_t idx;

    printf("\n=============start allocated info===========\n");
    idx = 0;
    for (bp = NEXT_BLKP(heap_listp); GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
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