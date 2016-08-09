
/*
 * Copyright (C) Qiaojun Xiao
 */

#ifndef _PALLOC_H_INCLUDED_
#define _PALLOC_H_INCLUDED_

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

typedef intptr_t  int_t;
typedef uintptr_t uint_t;

#define align(d,a)  (((d)+(a-1))&~(a-1))

#define align_ptr(p,a)   \
  (u_char *)(((uint_t)(p) + ((uint_t)a-1))&~((uint_t)a-1))
/*
 * MAX_ALLOC_FROM_POOL should be (pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */

#define MAX_ALLOC_FROM_POOL (getpagesize() - 1)

#define DEFAULT_POOL_SIZE (16 * 1024)

#define POOL_ALIGNMENT 16

#define ALIGNMENT   sizeof(unsigned long)   /* platform word */
#define MIN_POOL_SIZE   align((sizeof(pool_t)+2 * sizeof(pool_large_t)),POOL_ALIGNMENT)



typedef void (*pool_cleanup_pt)(void *data);

/*
 * 管理内存池中需要清理额外资源的内存块
 */
typedef struct pool_cleanup_s pool_cleanup_t;
struct pool_cleanup_s {
  pool_cleanup_pt   handler;  /* 清理额外资源的回调函数，在内存池销毁是被调用 */
  void             *data;     /* 指向分配到的内存空间 */
  pool_cleanup_t   *next;     /* 维护一个链表结构 */
};


/*
 * 管理大内存块(大于MAX_ALLOC_FROM_POOL)
 * 这样的内存块是在申请时分配的，内存池回收内存时会被释放
 */
typedef struct pool_large_s pool_large_t;
struct pool_large_s {
  pool_large_t  *next;       /* 维护一个链表结构 */
  void          *alloc;      /* 指向分配的内存空间 */
};

/*
 * 管理内存池预分配的内存块
 * 该结构体对应内存池中一块预分配的内存
 */
typedef struct pool_s pool_t;
typedef struct {
  u_char    *last;           /* 指向空闲内存的起始地址 */
  u_char    *end;            /* 指向内存块的末尾 */
  pool_t    *next;           /* 维护一个链表结构*/
  uint_t    failed;          /* 向该内存块申请空间失败次数*/
} pool_data_t;

/*
 * 管理内存池
 */ 
struct pool_s{
  pool_data_t   d;           /* */
  size_t        max;         /* pool_data_t中能分配的最大内存块，更大的内存块有pool_large_t分配*/
  pool_t        *current;    /* 当前 pool_data_t ,failed<4 */
  pool_large_t  *large;      /* 大内存块链表 */
  pool_cleanup_t *cleanup;   /* 需要额外清理的内存块链表 */
};

/*
 * 创建内存池 
 */
pool_t  *create_pool(size_t size);
/*
 * 销毁内存池
 */
void destroy_pool(pool_t *pool);
/*
 * 回收内存池中已经分配出去的空间
 */
void reset_pool(pool_t *pool);

/*
 * 向内存池申请size大小的空间，ALIGNMENT粒度对齐
 */
void *palloc(pool_t *pool, size_t size);
/*
 * 同palloc,区别是返回地址不一定是按ALIGNMENT对齐
 */
void *pnalloc(pool_t *pool, size_t size);
/*
 * 是palloc包装，将申请到的空间填0
 */
void *pcalloc(pool_t *pool, size_t size);
/*
 * 分配大内存块,按alignment对齐
 */
void *pmemalign(pool_t *pool, size_t size, size_t alignment);
/*
 * 释放大内存，
 */
int pfree(pool_t *pool, void *p);
/*
 * 申请size大小内存，该内存块由pool_cleanup_t结构管理
 */
pool_cleanup_t *pool_cleanup_add(pool_t *pool, size_t size);

#endif
