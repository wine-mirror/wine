/*
 * USER definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_USER_H
#define __WINE_USER_H

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

#define USUD_LOCALALLOC        0x0001
#define USUD_LOCALFREE         0x0002
#define USUD_LOCALCOMPACT      0x0003
#define USUD_LOCALHEAP         0x0004
#define USUD_FIRSTCLASS        0x0005

void WINAPI USER_SignalProc(HANDLE16, UINT16, UINT16, HINSTANCE16, HQUEUE16);
void USER_ExitWindows(void);
HGLOBAL16 USER_CallDefaultRsrcHandler( HGLOBAL16 hMemObj, HMODULE16 hModule,
				       HRSRC16 hRsrc );

#endif  /* __WINE_USER_H */
