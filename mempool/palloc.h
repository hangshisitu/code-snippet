
/*
 * Copyright (C) Qiaojun Xiao
 */

#ifndef _PALLOC_H_INCLUDED_
#define _PALLOC_H_INCLUDED_

#include <stdint.h>
//#include <sys/types.h>

#define align(d,a)  (((d)+(a-1))&~(a-1))

/*
 * MAX_ALLOC_FROM_POOL should be (pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */

#define MAX_ALLOC_FROM_POOL (ngx_pagesize - 1)

#define DEFAULT_POOL_SIZE (16 * 1024)

#define POOL_ALIGNMENT 16

#define MIN_POOL_SIZE   align((sizeof(pool_t)+2 * sizeof(pool_large_t)),POOL_ALIGNMENT)

typedef intptr_t  int_t
typedef uintptr_t uint_t

typedef struct {
  u_char    *last;
  u_char    *end;
  pool_t    *next;
  uint_t    failed;
} pool_data_t;

typedef struct {
  
} pool_t;

#endif
