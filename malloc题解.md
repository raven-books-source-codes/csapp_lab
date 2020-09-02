---
title: Csapp-Malloclab题解
date: 2020-08-26 16:45:31
categories: Csapp
tags:
---

本次lab，malloclab，自己手写一个内存分配器。

## 1. 实验目的

malloclab，简单明了的说就是实现一个自己的 malloc,free,realloc函数。做完这个实验你能够加深对指针的理解，掌握一些内存分配中的核心概念，如：如何组织heap，如何找到可用free block，采用first-fit, next-fit,best-fit?  如何在吞吐量和内存利用率之间做trade-off等。

就我个人的感受来说，malloclab的基础知识不算难，但是代码中充斥了大量的指针运算，为了避免硬编码指针运算，会定义一些宏，而定义宏来操作则会加大debug的难度（当然了，诸如linus这样的大神会觉得，代码写好了，为什么还用debug？），debug基本只能靠gdb和print，所以整体还是有难度了。

## 2. 背景知识

这里简单说一下要做这个实验需要哪些背景知识。

首先，为了写一个alloctor,需要解决哪些问题。csapp本章的ppt中列出了一些关键问题：

![Implementation Issues  How do we know how much memory to free given just a  pointer?  How do we keep track of the free blocks?  What do we do with the extra space when allocating a  structure that is smaller than the free block it is placed in?  How do we pick a block to use for allocation — many  might fit?  How do we reinsert freed block? ](https://cdn.jsdelivr.net/gh/ravenxrz/PicBed/img/clip_image001.png)

第一个问题，free(ptr)这样的routine是如何知道本次释放的block的大小的？

很显然我们需要一些额外的信息来存放block的元信息。之类的具体做法是在block的前面添加一个word，存放分配的size和是否已分配状态。

![image-20200826170802474](https://cdn.jsdelivr.net/gh/ravenxrz/PicBed/img/image-20200826170802474.png)

**注意：这里只是给出了最简单的情况，实际操作中，额外的元数据不仅只有这些**

第二个问题，如何追踪free blocks?

![image-20200826170901374](https://cdn.jsdelivr.net/gh/ravenxrz/PicBed/img/image-20200826170901374.png)

csapp一共给出了4种方案。其中implicit list在书上给出了源码，我个人实现了implicit list和explicit list。segregated free list感觉利用OO思想，把explicit list封装一下也是能够实现的，红黑树同理。

第三个问题，拆分策略（详见代码的place函数）

第四个问题，一般来说有 first-fit, next-fit和best-fit策略，我这里采用了最简单的first-fit策略。（这其实是一个trade-off的问题，看你是想要吞吐量高还是内存利用率高了）

ok，下面就来看看implicit list(书上有）和explicit list两种方案是如何实现的。

## 3. Implicit list

下面是一个implicit list的组织方式和一个block的具体情况，一个block采用了双边tag，保证可以前向和后向索引。

![image-20200826171525409](https://cdn.jsdelivr.net/gh/ravenxrz/PicBed/img/image-20200826171525409.png)

这种方案的优点：实现简单。缺点：寻找free block的开销过大。

现在说说lab中给的一些代码把：

1. memlib，这个文件中，给出了heap扩展的方法，除此之外，我们还可以获取当前可用heap的第一个字节，最后一个字节，heap size等。具体实现是通过一个sbrk指针来操作的。
2. mdriver, 这个文件不用看，只用它编译出来的可执行文件即可，用于测试我们写的allocator是否正确。
3. mm.c， 这个就是我们要操作的文件了，主要实现三个函数 mm_malloc,mm_free,mm_realloc，我们再额外定义自己需要的函数。

好的，下面再说说具体代码，因为代码中涉及到很多指针操作，我们对这些操作做一个峰装，用宏定义来操作：

```c
#define WSIZE       4       /* Word and header/footer size (bytes) */ 
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) 

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))           
#define PUT(p, val)  (*(unsigned int *)(p) = (val))   

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   
#define GET_ALLOC(p) (GET(p) & 0x1)                    

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                     
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) 
```

注释给出了每个宏的意义。

一些额外的定义：

```c
static void *free_list_head = NULL;	 // 整个list的头部

static void *extend_heap(size_t words);	// heap 不够分配时，用于扩展heap大小
static void *coalesce(void *bp);	// free block时，可能存在一些前后也是free block的情况，这时需要做合并，不允许一条list上，同时存在两个连续的free block
static void *find_fit(size_t size);	// 在list上找到可满足本次malloc请求的block
static void place(void *bp, size_t size); // 放置当前块，如果size < 本次block size - MIN_BLOCK ，则需要做split操作
```

**mm_init**

```c
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
   // Create the inital empty heap
    if( (free_list_head = mem_sbrk(4 * WSIZE)) == (void *)-1 ){
        return -1;
    }

    PUT(free_list_head, 0);
    PUT(free_list_head + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(free_list_head + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(free_list_head + (3 * WSIZE), PACK(0, 1));
    free_list_head += (2 * WSIZE);

    // Extend the empty heap with a free block of CHUNKSIZE bytes
    if(extend_heap(CHUNKSIZE/WSIZE) == NULL){
        return -1;
    }
    return 0;
}
```

这个函数对mm做初始化，工作包括：

1. 分配4个字，第0个字为pad，为了后续分配的块payload首地址能够是8字节对齐。
2. 第1-2个字为序言块，free_list_head指向这里，相当于给list一个定义，不然我们从哪里开始search呢？
3. 第3个字，结尾块，主要用于确定尾边界。
4. extend_heap, 分配一大块heap，用于后续malloc请求时分配。

**extend_heap**

```c
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
```

工作：

1. size更新，保证size为偶数个word
2. 为当前分配的block添加元数据，即header和footer信息
3. 更新尾边界

**mm_malloc**

```c
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
```

mm_malloc也比较简单，首先更改请求size，满足8字节对齐+元数据的开销要求。接着尝试找到当前可用heap中是否有能够满足本次请求的block，有则直接place，无则需要扩展当前可用heap的大小，扩展后再place。

**fint_fit**

```c
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
```

遍历整个list，找到还未分配且满足当前请求大小的block，然后返回该block的首地址。

**place**

```c
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
```

place的工作也很简单：

1. 最小块大小（2*DSIZE) < 当前块的大小-检查当前请求的大小 ，则对当前block做split
2. 否则，直接place即可。

现在继续看看free：

**mm_free**

```c
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
```

可以看到，free也是相当简单的，将当前block的分配状态从1更新到0即可。然后做coalesce操作。

