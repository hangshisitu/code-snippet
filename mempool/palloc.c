
/*
 * Copyright (c) Qiaojun Xiao
 */
#include <stdio.h>
#include "palloc.h"

static void *palloc_block(pool_t *pool, size_t size);
static void *palloc_large(pool_t *pool, size_t size);
 
pool_t *
create_pool(size_t size)
{
  pool_t *p;
  int err;
  err =  posix_memalign(&p,POOL_ALIGNMENT,size);
  if(err)
    {
      return NULL;
    }

  p->d.last = (u_char *) p + sizeof(pool_t);
  p->d.end = (u_char *) p + size;
  p->d.next = NULL;
  p->d.failed = 0;

  size = size - sizeof(pool_t);
  p->max = (size < MAX_ALLOC_FROM_POOL)? size : MAX_ALLOC_FROM_POOL;
  p->current = p;
  p->large = NULL;
  p->cleanup = NULL;
  
  return p;
}

void
destroy_pool(pool_t *pool)
{
  pool_t *p, *n;
  pool_cleanup_t *c;
  pool_large_t *l;

  for (c = pool->cleanup;c;c=c->next)
    {
      if (c->handler)
	{
	  c->handler(c->data);
	}
    }

  for (l = pool->large;l;l=l->next)
    {
      if (l->alloc)
	{
	  free(l->alloc);
	}
    }

  for ( p = pool, n = pool->d.next;;p=n, n = n->d.next)
    {
      free(p);
      if(n == NULL)
	{
	  break;
	}
    }

}

void
reset_pool(pool_t *pool)
{
  pool_t *p;
  pool_large_t *l;
  
  for (l = pool->large;l;l=l->next)
    {
      if (l->alloc)
	{
	  free(l->alloc);
	}
    }

  for (p = pool; p ; p = p->d.next)
    {
      p->d.failed = 0;
      p->d.last = (u_char *)p + sizeof(pool_t);
    }

  pool->current = p;
  pool->large = NULL;

}

void *
palloc(pool_t *pool, size_t size)
{
  u_char *m;
  pool_t *p, *prev, *newp;
  size_t psize;
  int err;

  if( size <= pool->max)
    {
      p = pool->current;
      do
	{
	  m = align_ptr(p->d.last,ALIGNMENT);
	  if( (size_t)(p->d.end - size) >= m)
	    {
	      p->d.last = m + size;
	      return m;
	    } 
	  if (p->d.failed++ > 4 && p->d.next)
	    {
	      pool->current = p->d.next;
	    }
	  prev = p;
	  p = p->d.next;
	}while(p);
      //      return palloc_block(pool, size);
      psize = (size_t)(pool->d.end - (u_char *)pool);
      err = posix_memalign(&m,POOL_ALIGNMENT,psize);
      if(err)
	{
	  return NULL;
	}
      newp = (pool_t *)m;
      newp->d.end = m + psize;
      newp->d.next = NULL;
      newp->d.failed = 0;
      prev->d.next = newp;

      m += sizeof(pool_data_t);
      m = align_ptr(m,ALIGNMENT);
      newp->d.last = m + size;
      return m;
    }
  return palloc_large(pool, size);
}

void *
palloc_block(pool_t *pool, size_t size)
{
  return NULL; 
}

void *
palloc_large(pool_t *pool, size_t size)
{
  void *p;
  uint_t n;
  pool_large_t *large;

  p = malloc(size);
  if ( p == NULL)
    {
      return NULL;
    }
  
  for (large = pool->large;large;large = large->next)
    {
      if (large->alloc == NULL)
	{
	  large->alloc = p;
	  return p;
	}
      if (n++ >3)
	{
	  break;
	}
    }

  large = palloc(pool,sizeof(pool_large_t));
  if (large == NULL)
    {
      free(p);
      return NULL;
    }
  
  large->alloc = p;
  large->next = pool->large;
  pool->large = large;

  return p;
}

void *
pnalloc(pool_t *pool, size_t size)
{
  u_char *m;
  pool_t *p, *prev, *newp;
  size_t psize;
  int err;

  if( size <= pool->max)
    {
      p = pool->current;
      do
	{
	  m = p->d.last;
	  if( (size_t)(p->d.end - size) >= m)
	    {
	      p->d.last = m + size;
	      return m;
	    } 
	  if (p->d.failed++ > 4 && p->d.next)
	    {
	      pool->current = p->d.next;
	    }
	  prev = p;
	  p = p->d.next;
	}while(p);
      psize = (size_t)(pool->d.end - (u_char *)pool);
      err = posix_memalign(&m,POOL_ALIGNMENT,psize);
      if(err)
	{
	  return NULL;
	}
      newp = (pool_t *)m;
      newp->d.end = m + psize;
      newp->d.next = NULL;
      newp->d.failed = 0;
      prev->d.next = newp;

      m += sizeof(pool_data_t);
      //m = align_ptr(m,ALIGNMENT);
      newp->d.last = m + size;
      return m;
    }
  return palloc_large(pool, size);
}

void *
pcalloc(pool_t *pool, size_t size)
{
  void *p;
  p = palloc(pool,size);
  if(p)
    {
      memset(p,0,size);
    }
  return p;
}

void *
pmemalign(pool_t *pool, size_t size, size_t alignment)
{
  void *p;
  pool_large_t * large;
  int err;
  err = posix_memalign(&p,alignment,size);
  if (err)
    {
      return NULL;
    }

  large = palloc(pool,sizeof(pool_large_t));
  if (large == NULL)
    {
      free(p);
      return NULL;
    }
  large->alloc = p;
  large->next = pool->large;
  pool->large = large;

  return p;
}

int pfree(pool_t *pool,void* p)
{
  pool_large_t *large;
  
  for (large = pool->large;large;large = large->next)
    {
      if ( p == large->alloc )
	{
	  free(p);
	  large->alloc = NULL;
	  
	  return 0;
	}
    }
  return -1;
}

pool_cleanup_t *
pool_cleanup_add(pool_t *pool, size_t size)
{
  pool_cleanup_t *c;
  
  c = palloc(pool,sizeof(pool_cleanup_t));
  if (c == NULL)
    {
      return NULL;
    }

  if (size)
    {
      c->data = palloc(pool,size);
      if (c->data == NULL)
	{
	  return NULL;
	}
    }
  else
    {
      c->data = NULL;
    }

  c->handler = NULL;
  c->next = pool->cleanup;
  pool->cleanup = c;

  return c;
}
