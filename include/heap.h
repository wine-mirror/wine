/*
 * Win32 heap definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_HEAP_H
#define __WINE_HEAP_H

#include "config.h"

#include <string.h>
#include "winbase.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "wine/windef16.h"  /* for SEGPTR */

extern SEGPTR HEAP_GetSegptr( HANDLE heap, DWORD flags, LPCVOID ptr );

/* SEGPTR helper macros */

#define SEGPTR_ALLOC(size) \
     (HeapAlloc( GetProcessHeap(), HEAP_WINE_SEGPTR, (size) ))
#define SEGPTR_NEW(type) \
     ((type *)HeapAlloc( GetProcessHeap(), HEAP_WINE_SEGPTR, sizeof(type) ))
#define SEGPTR_STRDUP_WtoA(str) \
     (HIWORD(str) ? HEAP_strdupWtoA( GetProcessHeap(), HEAP_WINE_SEGPTR, (str) ) : (LPSTR)(str))
#define SEGPTR_FREE(ptr) \
     (HIWORD(ptr) ? HeapFree( GetProcessHeap(), HEAP_WINE_SEGPTR, (ptr) ) : 0)
#define SEGPTR_GET(ptr) MapLS(ptr)

inline static LPSTR SEGPTR_STRDUP( LPCSTR str )
{
    if (HIWORD(str))
    {
        INT len = strlen(str) + 1;
        LPSTR p = HeapAlloc( GetProcessHeap(), HEAP_WINE_SEGPTR, len );
        if (p) memcpy( p, str, len );
        return p;
    }
    return (LPSTR)str;
}

/* strdup macros */
/* DO NOT USE THEM!!  they will go away soon */

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

#endif  /* __WINE_HEAP_H */
