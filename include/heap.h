/*
 * Win32 heap definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_HEAP_H
#define __WINE_HEAP_H

#include <string.h>

#include "winbase.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "wine/windef16.h"  /* for SEGPTR */

/* SEGPTR helper macros */

#define SEGPTR_ALLOC(size) \
     (HeapAlloc( GetProcessHeap(), 0, (size) ))
#define SEGPTR_NEW(type) \
     ((type *)HeapAlloc( GetProcessHeap(), 0, sizeof(type) ))
#define SEGPTR_STRDUP_WtoA(str) \
     (HIWORD(str) ? HEAP_strdupWtoA( GetProcessHeap(), 0, (str) ) : (LPSTR)(str))
#define SEGPTR_FREE(ptr) \
     (HIWORD(ptr) ? HeapFree( GetProcessHeap(), 0, (ptr) ) : 0)
#define SEGPTR_GET(ptr) MapLS(ptr)

inline static LPSTR SEGPTR_STRDUP( LPCSTR str )
{
    if (HIWORD(str))
    {
        INT len = strlen(str) + 1;
        LPSTR p = HeapAlloc( GetProcessHeap(), 0, len );
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
