/*
 * Win32 heap definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_HEAP_H
#define __WINE_HEAP_H

#include "winbase.h"

extern HANDLE32 SystemHeap;
extern HANDLE32 SegptrHeap;
extern CRITICAL_SECTION *HEAP_SystemLock;

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
	/* define an inline function, a macro won't do */
static __inline__ SEGPTR SEGPTR_Get(LPCVOID ptr) {
         return (HIWORD(ptr) ? HEAP_GetSegptr( SegptrHeap, 0, ptr ) : (SEGPTR)ptr);
}
#define SEGPTR_GET(ptr) SEGPTR_Get(ptr)
#define SEGPTR_FREE(ptr) \
         (HIWORD(ptr) ? HeapFree( SegptrHeap, 0, (ptr) ) : 0)

/* System heap locking macros */

#define SYSTEM_LOCK()       (EnterCriticalSection(HEAP_SystemLock))
#define SYSTEM_UNLOCK()     (LeaveCriticalSection(HEAP_SystemLock))
/* Use this one only when you own the lock! */
#define SYSTEM_LOCK_COUNT() (HEAP_SystemLock->RecursionCount)


typedef struct
{
    LPVOID lpData;
    DWORD cbData;
    BYTE cbOverhead;
    BYTE iRegionIndex;
    WORD wFlags;
    union {
        struct {
            HANDLE32 hMem;
            DWORD dwReserved[3];
        } Block;
        struct {
            DWORD dwCommittedSize;
            DWORD dwUnCommittedSize;
            LPVOID lpFirstBlock;
            LPVOID lpLastBlock;
        } Region;
    } Foo;
} PROCESS_HEAP_ENTRY, *LPPROCESS_HEAP_ENTRY;

#endif  /* __WINE_HEAP_H */
