/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */
/*
** Author: Eric Veach, July 1994.
**
*/

#include <stdarg.h>
#include <assert.h>
#include <limits.h>		/* LONG_MAX */

#include "windef.h"
#include "winbase.h"
#include "tess.h"

/* Include all the code for the regular heap-based queue here. */

typedef struct PriorityQHeap PriorityQHeap;
typedef struct { PQhandle handle; } PQnode;
typedef struct { PQkey key; PQhandle node; } PQhandleElem;

struct PriorityQHeap {
  PQnode	*nodes;
  PQhandleElem	*handles;
  long		size, max;
  PQhandle	freeList;
  int		initialized;
  int		(*leq)(PQkey key1, PQkey key2);
};

#define __gl_pqHeapMinimum(pq)	((pq)->handles[(pq)->nodes[1].handle].key)
#define __gl_pqHeapIsEmpty(pq)	((pq)->size == 0)

#define INIT_SIZE	32

/* Violates modularity, but a little faster */
#define LEQ(x,y)	VertLeq((GLUvertex *)x, (GLUvertex *)y)

static PriorityQHeap *__gl_pqHeapNewPriorityQ( int (*leq)(PQkey key1, PQkey key2) )
{
  PriorityQHeap *pq = HeapAlloc( GetProcessHeap(), 0, sizeof( PriorityQHeap ));
  if (pq == NULL) return NULL;

  pq->size = 0;
  pq->max = INIT_SIZE;
  pq->nodes = HeapAlloc( GetProcessHeap(), 0, (INIT_SIZE + 1) * sizeof(pq->nodes[0]) );
  if (pq->nodes == NULL) {
     HeapFree( GetProcessHeap(), 0, pq );
     return NULL;
  }

  pq->handles = HeapAlloc( GetProcessHeap(), 0, (INIT_SIZE + 1) * sizeof(pq->handles[0]) );
  if (pq->handles == NULL) {
     HeapFree( GetProcessHeap(), 0, pq->nodes );
     HeapFree( GetProcessHeap(), 0, pq );
     return NULL;
  }

  pq->initialized = FALSE;
  pq->freeList = 0;
  pq->leq = leq;

  pq->nodes[1].handle = 1;	/* so that Minimum() returns NULL */
  pq->handles[1].key = NULL;
  return pq;
}

static void __gl_pqHeapDeletePriorityQ( PriorityQHeap *pq )
{
  HeapFree( GetProcessHeap(), 0, pq->handles );
  HeapFree( GetProcessHeap(), 0, pq->nodes );
  HeapFree( GetProcessHeap(), 0, pq );
}


static void FloatDown( PriorityQHeap *pq, long curr )
{
  PQnode *n = pq->nodes;
  PQhandleElem *h = pq->handles;
  PQhandle hCurr, hChild;
  long child;

  hCurr = n[curr].handle;
  for( ;; ) {
    child = curr << 1;
    if( child < pq->size && LEQ( h[n[child+1].handle].key,
				 h[n[child].handle].key )) {
      ++child;
    }

    assert(child <= pq->max);

    hChild = n[child].handle;
    if( child > pq->size || LEQ( h[hCurr].key, h[hChild].key )) {
      n[curr].handle = hCurr;
      h[hCurr].node = curr;
      break;
    }
    n[curr].handle = hChild;
    h[hChild].node = curr;
    curr = child;
  }
}


static void FloatUp( PriorityQHeap *pq, long curr )
{
  PQnode *n = pq->nodes;
  PQhandleElem *h = pq->handles;
  PQhandle hCurr, hParent;
  long parent;

  hCurr = n[curr].handle;
  for( ;; ) {
    parent = curr >> 1;
    hParent = n[parent].handle;
    if( parent == 0 || LEQ( h[hParent].key, h[hCurr].key )) {
      n[curr].handle = hCurr;
      h[hCurr].node = curr;
      break;
    }
    n[curr].handle = hParent;
    h[hParent].node = curr;
    curr = parent;
  }
}

static void __gl_pqHeapInit( PriorityQHeap *pq )
{
  long i;

  /* This method of building a heap is O(n), rather than O(n lg n). */

  for( i = pq->size; i >= 1; --i ) {
    FloatDown( pq, i );
  }
  pq->initialized = TRUE;
}

/* returns LONG_MAX iff out of memory */
static PQhandle __gl_pqHeapInsert( PriorityQHeap *pq, PQkey keyNew )
{
  long curr;
  PQhandle free_handle;

  curr = ++ pq->size;
  if( (curr*2) > pq->max ) {
    PQnode *saveNodes= pq->nodes;
    PQhandleElem *saveHandles= pq->handles;

    /* If the heap overflows, double its size. */
    pq->max <<= 1;
    pq->nodes = HeapReAlloc( GetProcessHeap(), 0, pq->nodes,
				     (size_t)
				     ((pq->max + 1) * sizeof( pq->nodes[0] )));
    if (pq->nodes == NULL) {
       pq->nodes = saveNodes;	/* restore ptr to free upon return */
       return LONG_MAX;
    }
    pq->handles = HeapReAlloc( GetProcessHeap(), 0, pq->handles,
			                     (size_t)
			                      ((pq->max + 1) *
					       sizeof( pq->handles[0] )));
    if (pq->handles == NULL) {
       pq->handles = saveHandles; /* restore ptr to free upon return */
       return LONG_MAX;
    }
  }

  if( pq->freeList == 0 ) {
    free_handle = curr;
  } else {
    free_handle = pq->freeList;
    pq->freeList = pq->handles[free_handle].node;
  }

  pq->nodes[curr].handle = free_handle;
  pq->handles[free_handle].node = curr;
  pq->handles[free_handle].key = keyNew;

  if( pq->initialized ) {
    FloatUp( pq, curr );
  }
  assert(free_handle != LONG_MAX);
  return free_handle;
}

static PQkey __gl_pqHeapExtractMin( PriorityQHeap *pq )
{
  PQnode *n = pq->nodes;
  PQhandleElem *h = pq->handles;
  PQhandle hMin = n[1].handle;
  PQkey min = h[hMin].key;

  if( pq->size > 0 ) {
    n[1].handle = n[pq->size].handle;
    h[n[1].handle].node = 1;

    h[hMin].key = NULL;
    h[hMin].node = pq->freeList;
    pq->freeList = hMin;

    if( -- pq->size > 0 ) {
      FloatDown( pq, 1 );
    }
  }
  return min;
}

static void __gl_pqHeapDelete( PriorityQHeap *pq, PQhandle hCurr )
{
  PQnode *n = pq->nodes;
  PQhandleElem *h = pq->handles;
  long curr;

  assert( hCurr >= 1 && hCurr <= pq->max && h[hCurr].key != NULL );

  curr = h[hCurr].node;
  n[curr].handle = n[pq->size].handle;
  h[n[curr].handle].node = curr;

  if( curr <= -- pq->size ) {
    if( curr <= 1 || LEQ( h[n[curr>>1].handle].key, h[n[curr].handle].key )) {
      FloatDown( pq, curr );
    } else {
      FloatUp( pq, curr );
    }
  }
  h[hCurr].key = NULL;
  h[hCurr].node = pq->freeList;
  pq->freeList = hCurr;
}

/* Now redefine all the function names to map to their "Sort" versions. */

struct PriorityQSort {
  PriorityQHeap	*heap;
  PQkey		*keys;
  PQkey		**order;
  PQhandle	size, max;
  int		initialized;
  int		(*leq)(PQkey key1, PQkey key2);
};

PriorityQSort *__gl_pqSortNewPriorityQ( int (*leq)(PQkey key1, PQkey key2) )
{
  PriorityQSort *pq = HeapAlloc( GetProcessHeap(), 0, sizeof( PriorityQSort ));
  if (pq == NULL) return NULL;

  pq->heap = __gl_pqHeapNewPriorityQ( leq );
  if (pq->heap == NULL) {
     HeapFree( GetProcessHeap(), 0, pq );
     return NULL;
  }

  pq->keys = HeapAlloc( GetProcessHeap(), 0, INIT_SIZE * sizeof(pq->keys[0]) );
  if (pq->keys == NULL) {
     __gl_pqHeapDeletePriorityQ(pq->heap);
     HeapFree( GetProcessHeap(), 0, pq );
     return NULL;
  }

  pq->size = 0;
  pq->max = INIT_SIZE;
  pq->initialized = FALSE;
  pq->leq = leq;
  return pq;
}

void __gl_pqSortDeletePriorityQ( PriorityQSort *pq )
{
  assert(pq != NULL);
  if (pq->heap != NULL) __gl_pqHeapDeletePriorityQ( pq->heap );
  HeapFree( GetProcessHeap(), 0, pq->order );
  HeapFree( GetProcessHeap(), 0, pq->keys );
  HeapFree( GetProcessHeap(), 0, pq );
}


#define LT(x,y)		(! LEQ(y,x))
#define GT(x,y)		(! LEQ(x,y))
#define Swap(a,b)	do{PQkey *tmp = *a; *a = *b; *b = tmp;}while(0)

int __gl_pqSortInit( PriorityQSort *pq )
{
  PQkey **p, **r, **i, **j, *piv;
  struct { PQkey **p, **r; } Stack[50], *top = Stack;
  unsigned long seed = 2016473283;

  /* Create an array of indirect pointers to the keys, so that we
   * the handles we have returned are still valid.
   */
  pq->order = HeapAlloc( GetProcessHeap(), 0, (size_t)
                                  (pq->size * sizeof(pq->order[0])) );
  if (pq->order == NULL) return 0;

  p = pq->order;
  r = p + pq->size - 1;
  for( piv = pq->keys, i = p; i <= r; ++piv, ++i ) {
    *i = piv;
  }

  /* Sort the indirect pointers in descending order,
   * using randomized Quicksort
   */
  top->p = p; top->r = r; ++top;
  while( --top >= Stack ) {
    p = top->p;
    r = top->r;
    while( r > p + 10 ) {
      seed = seed * 1539415821 + 1;
      i = p + seed % (r - p + 1);
      piv = *i;
      *i = *p;
      *p = piv;
      i = p - 1;
      j = r + 1;
      do {
	do { ++i; } while( GT( **i, *piv ));
	do { --j; } while( LT( **j, *piv ));
	Swap( i, j );
      } while( i < j );
      Swap( i, j );	/* Undo last swap */
      if( i - p < r - j ) {
	top->p = j+1; top->r = r; ++top;
	r = i-1;
      } else {
	top->p = p; top->r = i-1; ++top;
	p = j+1;
      }
    }
    /* Insertion sort small lists */
    for( i = p+1; i <= r; ++i ) {
      piv = *i;
      for( j = i; j > p && LT( **(j-1), *piv ); --j ) {
	*j = *(j-1);
      }
      *j = piv;
    }
  }
  pq->max = pq->size;
  pq->initialized = TRUE;
  __gl_pqHeapInit( pq->heap );	/* always succeeds */

#ifndef NDEBUG
  p = pq->order;
  r = p + pq->size - 1;
  for( i = p; i < r; ++i ) {
    assert( LEQ( **(i+1), **i ));
  }
#endif

  return 1;
}

/* returns LONG_MAX iff out of memory */
PQhandle __gl_pqSortInsert( PriorityQSort *pq, PQkey keyNew )
{
  long curr;

  if( pq->initialized ) {
    return __gl_pqHeapInsert( pq->heap, keyNew );
  }
  curr = pq->size;
  if( ++ pq->size >= pq->max ) {
    PQkey *saveKey= pq->keys;

    /* If the heap overflows, double its size. */
    pq->max <<= 1;
    pq->keys = HeapReAlloc( GetProcessHeap(), 0, pq->keys,
	 	                        (size_t)
	                                 (pq->max * sizeof( pq->keys[0] )));
    if (pq->keys == NULL) {
       pq->keys = saveKey;	/* restore ptr to free upon return */
       return LONG_MAX;
    }
  }
  assert(curr != LONG_MAX);
  pq->keys[curr] = keyNew;

  /* Negative handles index the sorted array. */
  return -(curr+1);
}

PQkey __gl_pqSortExtractMin( PriorityQSort *pq )
{
  PQkey sortMin, heapMin;

  if( pq->size == 0 ) {
    return __gl_pqHeapExtractMin( pq->heap );
  }
  sortMin = *(pq->order[pq->size-1]);
  if( ! __gl_pqHeapIsEmpty( pq->heap )) {
    heapMin = __gl_pqHeapMinimum( pq->heap );
    if( LEQ( heapMin, sortMin )) {
      return __gl_pqHeapExtractMin( pq->heap );
    }
  }
  do {
    -- pq->size;
  } while( pq->size > 0 && *(pq->order[pq->size-1]) == NULL );
  return sortMin;
}

PQkey __gl_pqSortMinimum( PriorityQSort *pq )
{
  PQkey sortMin, heapMin;

  if( pq->size == 0 ) {
    return __gl_pqHeapMinimum( pq->heap );
  }
  sortMin = *(pq->order[pq->size-1]);
  if( ! __gl_pqHeapIsEmpty( pq->heap )) {
    heapMin = __gl_pqHeapMinimum( pq->heap );
    if( LEQ( heapMin, sortMin )) {
      return heapMin;
    }
  }
  return sortMin;
}

int __gl_pqSortIsEmpty( PriorityQSort *pq )
{
  return (pq->size == 0) && __gl_pqHeapIsEmpty( pq->heap );
}

void __gl_pqSortDelete( PriorityQSort *pq, PQhandle curr )
{
  if( curr >= 0 ) {
    __gl_pqHeapDelete( pq->heap, curr );
    return;
  }
  curr = -(curr+1);
  assert( curr < pq->max && pq->keys[curr] != NULL );

  pq->keys[curr] = NULL;
  while( pq->size > 0 && *(pq->order[pq->size-1]) == NULL ) {
    -- pq->size;
  }
}
