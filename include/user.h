/*
 * USER definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef USER_H
#define USER_H

#include "ldt.h"
#include "local.h"

extern WORD USER_HeapSel;

#define USER_HEAP_ALLOC(size) \
            LOCAL_Alloc( USER_HeapSel, LMEM_FIXED, (size) )
#define USER_HEAP_REALLOC(handle,size) \
            LOCAL_ReAlloc( USER_HeapSel, (handle), (size), LMEM_FIXED )
#define USER_HEAP_FREE(handle) \
            LOCAL_Free( USER_HeapSel, (handle) )
#define USER_HEAP_LIN_ADDR(handle)  \
         ((handle) ? PTR_SEG_OFF_TO_LIN(USER_HeapSel, (handle)) : NULL)
#define USER_HEAP_SEG_ADDR(handle)  \
         ((handle) ? PTR_SEG_OFF_TO_SEGPTR(USER_HeapSel, (handle)) : (SEGPTR)0)

#endif  /* USER_H */
