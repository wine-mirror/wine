/*
 * Win32 heap definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_HEAP_H
#define __WINE_HEAP_H

#include "winbase.h"
#include "winnt.h"

extern HANDLE32 SystemHeap;
extern HANDLE32 SegptrHeap;

extern int HEAP_IsInsideHeap( HANDLE32 heap, DWORD flags, LPCVOID ptr );
extern SEGPTR HEAP_GetSegptr( HANDLE32 heap, DWORD flags, LPCVOID ptr );
extern LPVOID HEAP_xalloc( HANDLE32 heap, DWORD flags, DWORD size );
extern LPSTR HEAP_strdupA( HANDLE32 heap, DWORD flags, LPCSTR str );
extern LPWSTR HEAP_strdupW( HANDLE32 heap, DWORD flags, LPCWSTR str );
extern LPWSTR HEAP_strdupAtoW( HANDLE32 heap, DWORD flags, LPCSTR str );
extern LPSTR HEAP_strdupWtoA( HANDLE32 heap, DWORD flags, LPCWSTR str );

/* SEGPTR helper macros */

#define SEGPTR_ALLOC(size) \
         (HeapAlloc( SegptrHeap, 0, (size) ))
#define SEGPTR_NEW(type) \
         ((type *)HeapAlloc( SegptrHeap, 0, sizeof(type) ))
#define SEGPTR_STRDUP(str) \
         (HIWORD(str) ? HEAP_strdupA( SegptrHeap, 0, (str) ) : (LPSTR)(str))
#define SEGPTR_STRDUP_WtoA(str) \
         (HIWORD(str) ? HEAP_strdupWtoA( SegptrHeap, 0, (str) ) : (LPSTR)(str))
#define SEGPTR_GET(ptr) \
         (HIWORD(ptr) ? HEAP_GetSegptr( SegptrHeap, 0, (ptr) ) : (SEGPTR)(ptr))
#define SEGPTR_FREE(ptr) \
         (HIWORD(ptr) ? HeapFree( SegptrHeap, 0, (ptr) ) : 0)

#endif  /* __WINE_HEAP_H */
