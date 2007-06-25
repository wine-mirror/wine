/*
 * Performance Data Helper (pdh.dll)
 *
 * Copyright 2007 Andrey Turkin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"

#include "pdh.h"
#include "pdhmsg.h"
#include "winperf.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(pdh);

static inline void *pdh_alloc( SIZE_T size )
{
    return HeapAlloc( GetProcessHeap(), 0, size );
}

static inline void *pdh_alloc_zero( SIZE_T size )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );
}

static inline void pdh_free( LPVOID mem )
{
    HeapFree( GetProcessHeap(), 0, mem );
}

static inline WCHAR *pdh_strdup( const WCHAR *src )
{
    WCHAR *dst;

    if (!src) return NULL;
    if ((dst = pdh_alloc( (strlenW( src ) + 1) * sizeof(WCHAR) ))) strcpyW( dst, src );
    return dst;
}

static inline WCHAR *pdh_strdup_aw( const char *src )
{
    int len;
    WCHAR *dst;

    if (!src) return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
    if ((dst = pdh_alloc( len * sizeof(WCHAR) ))) MultiByteToWideChar( CP_ACP, 0, src, -1, dst, len );
    return dst;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n",hinstDLL,fdwReason,lpvReserved);

    if (fdwReason == DLL_WINE_PREATTACH) return FALSE;    /* prefer native version */

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls( hinstDLL );
    }

    return TRUE;
}

struct counter
{
    struct list     entry;
    WCHAR          *path;                           /* identifier */
    DWORD           type;                           /* counter type */
    DWORD           status;                         /* update status */
    LONG            scale;                          /* scale factor */
    LONG            defaultscale;                   /* default scale factor */
    DWORD_PTR       user;                           /* user data */
    DWORD_PTR       queryuser;                      /* query user data */
    LONGLONG        base;                           /* samples per second */
    FILETIME        stamp;                          /* time stamp */
    void (CALLBACK *collect)( struct counter * );   /* collect callback */
    union
    {
        LONG     longvalue;
        double   doublevalue;
        LONGLONG largevalue;
    } one;                                          /* first value */
    union
    {
        LONG     longvalue;
        double   doublevalue;
        LONGLONG largevalue;
    } two;                                          /* second value */
};

#define PDH_MAGIC_QUERY     0x50444830 /* 'PDH0' */

struct query
{
    DWORD       magic;      /* signature */
    DWORD_PTR   user;       /* user data */
    struct list counters;   /* counter list */
};

static struct query *create_query( void )
{
    struct query *query;

    if ((query = pdh_alloc_zero( sizeof(struct query) )))
    {
        query->magic = PDH_MAGIC_QUERY;
        list_init( &query->counters );
        return query;
    }
    return NULL;
}

struct source
{
    const WCHAR    *path;                           /* identifier */
    void (CALLBACK *collect)( struct counter * );   /* collect callback */
    DWORD           type;                           /* counter type */
    LONG            scale;                          /* default scale factor */
    LONGLONG        base;                           /* samples per second */
};


/***********************************************************************
 *              PdhCloseQuery   (PDH.@)
 */
PDH_STATUS WINAPI PdhCloseQuery( PDH_HQUERY handle )
{
    struct query *query = handle;
    struct list *item;

    TRACE("%p\n", handle);

    if (!query || (query->magic != PDH_MAGIC_QUERY)) return PDH_INVALID_HANDLE;

    LIST_FOR_EACH( item, &query->counters )
    {
        struct counter *counter = LIST_ENTRY( item, struct counter, entry );

        list_remove( &counter->entry );

        pdh_free( counter->path );
        pdh_free( counter );
    }

    pdh_free( query );
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhOpenQueryA   (PDH.@)
 */
PDH_STATUS WINAPI PdhOpenQueryA( LPCSTR source, DWORD_PTR userdata, PDH_HQUERY *query )
{
    PDH_STATUS ret;
    WCHAR *sourceW = NULL;

    TRACE("%s %lx %p\n", debugstr_a(source), userdata, query);

    if (source && !(sourceW = pdh_strdup_aw( source ))) return PDH_MEMORY_ALLOCATION_FAILURE;

    ret = PdhOpenQueryW( sourceW, userdata, query );
    pdh_free( sourceW );

    return ret;
}

/***********************************************************************
 *              PdhOpenQueryW   (PDH.@)
 */
PDH_STATUS WINAPI PdhOpenQueryW( LPCWSTR source, DWORD_PTR userdata, PDH_HQUERY *handle )
{
    struct query *query;

    TRACE("%s %lx %p\n", debugstr_w(source), userdata, handle);

    if (!handle) return PDH_INVALID_ARGUMENT;

    if (source)
    {
        FIXME("log file data source not supported\n");
        return PDH_INVALID_ARGUMENT;
    }
    if ((query = create_query()))
    {
        query->user = userdata;
        *handle = query;

        return ERROR_SUCCESS;
    }
    return PDH_MEMORY_ALLOCATION_FAILURE;
}
