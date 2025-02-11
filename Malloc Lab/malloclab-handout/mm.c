/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
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

// PACK作用是将size和alloc合并成一个整数，size占高29位，alloc占低3位
#define PACK(size, alloc)   ((size) | (alloc))

#define WSIZE			4			/* Word and header/footer size (bytes) */
#define DSIZE			8			/* Double word size (bytes) */
#define CHUNKSIZE		(1<<12)		/* Extend heap by this amount (bytes) */

#define GET(p)			(*(unsigned int *)(p))
#define PUT(p, val)		(*(unsigned int *)(p) = val)

// 获取头部信息指针里面的前29位，也就是整个malloc数据块的大小，包括头部和尾部还有有效载荷
#define GET_SIZE(p)		(GET(p) & ~0x7)
// 这个数据块是否被使用，根据头部信息指针判断
#define GET_ALLOC(p)	(GET(p) & 0x1)

// (HDRP=header_pointer)header的地址由数据块的地址减去4
#define HDRP(bp)		((char *)(bp) - WSIZE)
// footer的地址由数据块的地址加上数据块的大小（包括header和footer还有有效载荷）减去8
#define FTRP(bp)		((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char* heap_listp = NULL;

// #define DEBUG

static void* coalesce(void* bp)
{
    // 如果前一个数据块是空的或者下一个数据块是空的，就需要合并
    // 返回合并后的第一个数据块的地址
	char* prev_head = HDRP(PREV_BLKP(bp));
	char* next_foot = FTRP(NEXT_BLKP(bp));
	char* next_head = HDRP(NEXT_BLKP(bp));
    int allocated_prev = GET_ALLOC(prev_head);

	// 如果这是第一个块，prev_head就是heap_listp，这时候prev_head是不对的
	if (bp == heap_listp)
		allocated_prev = 1;
	// 需要用next_head，因为如果这是最后一个块，next_foot就不对了
    int allocated_next = GET_ALLOC(next_head);
	
#ifdef DEBUG
	printf("coalesce: bp: %p, prev_head: %p, next_foot: %p, allocated_prev: %d, allocated_next: %d\n", 
			bp, prev_head, next_foot, allocated_prev, allocated_next);
#endif

	if (allocated_prev && allocated_next)
    {
        return bp;
    }
    else if (!allocated_prev && allocated_next)
    {
        int new_size = GET_SIZE(HDRP(bp)) + GET_SIZE(prev_head);
    	PUT(prev_head, PACK(new_size, 0));
    	PUT(FTRP(bp), PACK(new_size, 0));
        return PREV_BLKP(bp);
    }
    else if (allocated_prev && !allocated_next)
    {
        int new_size = GET_SIZE(HDRP(bp)) + GET_SIZE(next_foot);
    	PUT(next_foot, PACK(new_size, 0));
    	PUT(HDRP(bp), PACK(new_size, 0));
        return bp;
    }
    else
    {
        int new_size = GET_SIZE(prev_head) + GET_SIZE(HDRP(bp)) + GET_SIZE(next_foot);
    	PUT(prev_head, PACK(new_size, 0));
    	PUT(next_foot, PACK(new_size, 0));
        return PREV_BLKP(bp);
    }
}

static void print_all()
{
	char* curp = heap_listp;
	printf("\n");
	while (GET_SIZE(HDRP(curp)) != 0)
	{
		printf("PRINTALL: curp: %p, size: %d, alloc: %d\n", curp, GET_SIZE(HDRP(curp)), GET_ALLOC(HDRP(curp)));
		curp = NEXT_BLKP(curp);
	}
	printf("\n");
}

// words应该包括头部和尾部
static void *extend_heap(int words)
{
	char* bp = NULL;
	int alloca_size = words * WSIZE;
	// 需要保证分配的内存是8的倍数
	if (words % 2 == 1)
		alloca_size += WSIZE;
	
	if ((bp = mem_sbrk(alloca_size)) == (void *)-1)
	{
		return NULL;
	}

#ifdef DEBUG
	printf("\nextend_heap: %d\n", alloca_size);
#endif

	// bp前面有上一个堆结尾，所以可以直接用HDRP(bp)和FTRP(bp)来设置新的头部和尾部
	// 这里直接传入alloca_size，不用加其他运算，因为最小的单位是8，有点绕，可以仔细想一下
	PUT(HDRP(bp), PACK(alloca_size, 0));
	PUT(FTRP(bp), PACK(alloca_size, 0));
	// 新的堆结尾
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

#ifdef DEBUG
	print_all();
#endif

	/* Coalesce if the previous block was free */
	return coalesce(bp);
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 创建一个初始的空堆，包括一个空的头部和一个空的尾部
	// 使用 mem_sbrk 扩展堆，将内存空间增加 size 字节，返回分配的内存的起始地址
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
    {
        return -1;
    }
    // 堆的第一个字需要是0
    PUT(heap_listp, 0);
    PUT(heap_listp + WSIZE, PACK(8, 0));
    PUT(heap_listp + 2 * WSIZE, PACK(8, 0));
    // 堆的结尾需要是1，这个标记的作用是防止内存分配器在遍历堆时越界
    PUT(heap_listp + 3 * WSIZE, PACK(0, 1));

    heap_listp += 2 * WSIZE;

	if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
		return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
#ifdef DEBUG
	printf("\nmm_malloc: %d\n", size);
#endif
	if (size == 0)
	{
		return NULL;
	}
	if (heap_listp == NULL)
	{
		mm_init();
	}

	// curp是当前遍历的数据块的地址
	char* curp = heap_listp;
	int alloca_size = ALIGN(size) + DSIZE;

#ifdef DEBUG
	print_all();
#endif

	while (GET(HDRP(curp)) != 1)
	{
		if (!GET_ALLOC(HDRP(curp)) && GET_SIZE(HDRP(curp)) >= alloca_size)
			break;
		curp = NEXT_BLKP(curp);
	}

	// 找到了空闲块
	if (GET(HDRP(curp)) != 1)
	{
		// 如果空闲块大小大于需要的大小加上多出的头部尾部的大小，就需要分割
		if (GET_SIZE(HDRP(curp)) >= alloca_size + 2 * WSIZE)
		{
			// 分割后的下一个空闲块的大小
			int next_size = GET_SIZE(HDRP(curp)) - alloca_size;

			// 先改变当前头部的alloc标记和size
			PUT(HDRP(curp), PACK(alloca_size, 1));
			// 这时候当前尾部的地址已经改变了，直接改变尾部的alloc标记即可
			PUT(FTRP(curp), PACK(alloca_size, 1));

			// 然后再新建下一个的头部
			char* nextp = NEXT_BLKP(curp);
			PUT(HDRP(nextp), PACK(next_size, 0));
			// 因为尾部的地址是用头部的地址加上size得到的，所以这里直接改变尾部的alloc标记即可
			PUT(FTRP(nextp), PACK(next_size, 0));
		}
		else
		{
			// 如果空闲块大小和需要的大小一样或者大一点点，就不需要再分割了，直接改变尾部的alloc标记
			PUT(HDRP(curp), PACK(GET_SIZE(HDRP(curp)), 1));
			PUT(FTRP(curp), PACK(GET_SIZE(HDRP(curp)), 1));
		}

#ifdef DEBUG
		printf("mm_malloc done: %p\n", curp);
		print_all();
#endif
		return curp;
	}
	else // 没有找到空闲块，需要扩展堆
	{
		// 扩展的大小至少是CHUNKSIZE
		alloca_size = alloca_size > CHUNKSIZE ? alloca_size : CHUNKSIZE;
		if ((curp = extend_heap(alloca_size / WSIZE)) == NULL)
			return NULL;
		return mm_malloc(size);
	}
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
#ifdef DEBUG
	printf("\nmm_free: %p\n", ptr);
#endif
    if (ptr == NULL)
    {
        return;
    }
    if (heap_listp == NULL)
    {
        mm_init();
    }

    PUT(HDRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0));
    PUT(FTRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 0));

    coalesce(ptr);

#ifdef DEBUG
	print_all();
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
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
