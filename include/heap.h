/*
 * Win32 heap definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_HEAP_H
#define __WINE_HEAP_H

#include "config.h"

#include "winbase.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "wine/windef16.h"  /* for SEGPTR */

extern HANDLE SystemHeap;
extern HANDLE SegptrHeap;

extern int HEAP_IsInsideHeap( HANDLE heap, DWORD flags, LPCVOID ptr );
extern SEGPTR HEAP_GetSegptr( HANDLE heap, DWORD flags, LPCVOID ptr );
extern BOOL HEAP_CreateSystemHeap(void);

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
static inline SEGPTR WINE_UNUSED SEGPTR_Get(LPCVOID ptr) {
         return (HIWORD(ptr) ? HEAP_GetSegptr( SegptrHeap, 0, ptr ) : (SEGPTR)ptr);
}
#define SEGPTR_GET(ptr) SEGPTR_Get(ptr)
#define SEGPTR_FREE(ptr) \
         (HIWORD(ptr) ? HeapFree( SegptrHeap, 0, (ptr) ) : 0)


/* strdup macros */
/* DO NOT USE THEM!!  they will go away soon */

inline static LPSTR HEAP_strdupA( HANDLE heap, DWORD flags, LPCSTR str )
{
    INT len = strlen(str) + 1;
    LPSTR p = HeapAlloc( heap, flags, len );
    if (p) memcpy( p, str, len );
    return p;
}

inline static LPWSTR HEAP_strdupW( HANDLE heap, DWORD flags, LPCWSTR str )
{
    INT len = strlenW(str) + 1;
    LPWSTR p = HeapAlloc( heap, flags, len * sizeof(WCHAR) );
    if (p) memcpy( p, str, len * sizeof(WCHAR) );
    return p;
}

inline static LPWSTR HEAP_strdupAtoW( HANDLE heap, DWORD flags, LPCSTR str )
{
    LPWSTR ret;
    INT len;

    if (!str) return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    ret = HeapAlloc( heap, flags, len * sizeof(WCHAR) );
    if (ret) MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    return ret;
}

inline static LPSTR HEAP_strdupWtoA( HANDLE heap, DWORD flags, LPCWSTR str )
{
    LPSTR ret;
    INT len;

    if (!str) return NULL;
    len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
    ret = HeapAlloc( heap, flags, len );
    if(ret) WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    return ret;
}

/* system heap private data */
/* you must lock the system heap before using this structure */
typedef struct
{
    void     *gdi;        /* GDI heap */
    void     *user;       /* USER handle table */
    void     *cursor;     /* cursor information */
    void     *queue;      /* message queues descriptor */
    void     *win;        /* windows descriptor */
    void     *root;       /* X11 root window */
} SYSTEM_HEAP_DESCR;

extern SYSTEM_HEAP_DESCR *SystemHeapDescr;

#endif  /* __WINE_HEAP_H */
