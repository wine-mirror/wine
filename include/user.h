/*
 * USER definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef USER_H
#define USER_H

#include "segmem.h"
#include "heap.h"

/* USER local heap */

#ifdef WINELIB

#define USER_HEAP_ALLOC(f,size) LocalAlloc (f, size)
#define USER_HEAP_REALLOC(handle,size,f) LocalReAlloc (handle,size,f)
#define USER_HEAP_ADDR(handle) LocalLock (handle)
#define USER_HEAP_FREE(handle) LocalFree (handle)
#else

extern MDESC *USER_Heap;

#define USER_HEAP_ALLOC(f,size) ((int)HEAP_Alloc(&USER_Heap,f,size) & 0xffff)
#define USER_HEAP_REALLOC(handle,size,f) ((int)HEAP_ReAlloc(&USER_Heap, \
				       USER_HEAP_ADDR(handle),size,f) & 0xffff)
#define USER_HEAP_ADDR(handle) ((void *)((handle)|((int)USER_Heap&0xffff0000)))
#define USER_HEAP_FREE(handle) (HEAP_Free(&USER_Heap,USER_HEAP_ADDR(handle)))

#endif  /* WINELIB */

#endif  /* USER_H */
