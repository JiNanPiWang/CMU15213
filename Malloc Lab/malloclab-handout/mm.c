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

// PACK作用是将size和alloc合并成一个整数，size占低29位，alloc占高3位
#define PACK(size, alloc)   ((size) | (alloc))

#define WSIZE			4 /* Word and header/footer size (bytes) */
#define DSIZE			8 /* Double word size (bytes) */

#define GET(p)			(*(unsigned int *)(p))
#define PUT(p, val)		(*(unsigned int *)(p) = val)

// 获取头部信息指针里面的前29位，也就是整个malloc数据块的大小
#define GET_SIZE(p)		(GET(p) & ~0x7)
// 这个数据块是否被使用，根据头部信息指针判断
#define GET_ALLOC(p)	(GET(p) & 0x1)

// (HDRP=header_pointer)header的地址由数据块的地址减去4
#define HDRP(bp)		((char *)(bp) - WSIZE)
// footer的地址由数据块的地址加上数据块的大小（包括header和footer还有有效载荷）减去8
#define FTRP(bp)		((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char* heap_listp;

static void free_single_block(void *ptr)
{

}

static void* coalesce(void* bp)
{
    // 如果前一个数据块是空的或者下一个数据块是空的，就需要合并
    // 返回合并后的第一个数据块的地址
	char* prev_head = HDRP(PREV_BLKP(bp));
	char* next_foot = FTRP(NEXT_BLKP(bp));
    int allocated_prev = GET_ALLOC(prev_head);
    int allocated_next = GET_ALLOC(next_foot);
	
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

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 创建一个初始的空堆，包括一个空的头部和一个空的尾部
    if ((heap_listp = mem_sbrk(2 * WSIZE)) == (void *)-1)
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

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
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
