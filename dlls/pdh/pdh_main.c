/*
 * Performance Data Helper (pdh.dll)
 *
 * Copyright 2007 Andrey Turkin
 * Copyright 2007 Hans Leidekker
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
#include <math.h>

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

static struct counter *create_counter( void )
{
    struct counter *counter;

    if ((counter = pdh_alloc_zero( sizeof(struct counter) ))) return counter;
    return NULL;
}

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

static const WCHAR path_processor_time[] =
    {'\\','P','r','o','c','e','s','s','o','r','(','_','T','o','t','a','l',')',
     '\\','%',' ','P','r','o','c','e','s','s','o','r',' ','T','i','m','e',0};
static const WCHAR path_uptime[] =
    {'\\','S','y','s','t','e','m', '\\', 'S','y','s','t','e','m',' ','U','p',' ','T','i','m','e',0};

static void CALLBACK collect_processor_time( struct counter *counter )
{
    counter->two.largevalue = 500000; /* FIXME */
    counter->status = PDH_CSTATUS_VALID_DATA;
}

static void CALLBACK collect_uptime( struct counter *counter )
{
    counter->two.largevalue = GetTickCount64();
    counter->status = PDH_CSTATUS_VALID_DATA;
}

#define TYPE_PROCESSOR_TIME \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE | PERF_TIMER_100NS | PERF_DELTA_COUNTER | \
     PERF_INVERSE_COUNTER | PERF_DISPLAY_PERCENT)

#define TYPE_UPTIME \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_ELAPSED | PERF_OBJECT_TIMER | PERF_DISPLAY_SECONDS)

/* counter source registry */
static const struct source counter_sources[] =
{
    { path_processor_time, collect_processor_time, TYPE_PROCESSOR_TIME, -5, 10000000 },
    { path_uptime, collect_uptime, TYPE_UPTIME, -3, 1000 }
};

/***********************************************************************
 *              PdhAddCounterA   (PDH.@)
 */
PDH_STATUS WINAPI PdhAddCounterA( PDH_HQUERY query, LPCSTR path,
                                  DWORD_PTR userdata, PDH_HCOUNTER *counter )
{
    PDH_STATUS ret;
    WCHAR *pathW;

    TRACE("%p %s %lx %p\n", query, debugstr_a(path), userdata, counter);

    if (!path) return PDH_INVALID_ARGUMENT;

    if (!(pathW = pdh_strdup_aw( path )))
        return PDH_MEMORY_ALLOCATION_FAILURE;

    ret = PdhAddCounterW( query, pathW, userdata, counter );

    pdh_free( pathW );
    return ret;
}

/***********************************************************************
 *              PdhAddCounterW   (PDH.@)
 */
PDH_STATUS WINAPI PdhAddCounterW( PDH_HQUERY hquery, LPCWSTR path,
                                  DWORD_PTR userdata, PDH_HCOUNTER *hcounter )
{
    struct query *query = hquery;
    struct counter *counter;
    unsigned int i;

    TRACE("%p %s %lx %p\n", hquery, debugstr_w(path), userdata, hcounter);

    if (!path  || !hcounter) return PDH_INVALID_ARGUMENT;
    if (!query || (query->magic != PDH_MAGIC_QUERY)) return PDH_INVALID_HANDLE;

    *hcounter = NULL;
    for (i = 0; i < sizeof(counter_sources) / sizeof(counter_sources[0]); i++)
    {
        if (strstrW( path, counter_sources[i].path ))
        {
            if ((counter = create_counter()))
            {
                counter->path         = pdh_strdup( counter_sources[i].path );
                counter->collect      = counter_sources[i].collect;
                counter->type         = counter_sources[i].type;
                counter->defaultscale = counter_sources[i].scale;
                counter->base         = counter_sources[i].base;
                counter->queryuser    = query->user;
                counter->user         = userdata;

                list_add_tail( &query->counters, &counter->entry );

                *hcounter = counter;
                return ERROR_SUCCESS;
            }
            return PDH_MEMORY_ALLOCATION_FAILURE;
        }
    }
    return PDH_CSTATUS_NO_COUNTER;
}

/***********************************************************************
 *              PdhCloseQuery   (PDH.@)
 */
PDH_STATUS WINAPI PdhCloseQuery( PDH_HQUERY handle )
{
    struct query *query = handle;
    struct list *item, *next;

    TRACE("%p\n", handle);

    if (!query || (query->magic != PDH_MAGIC_QUERY)) return PDH_INVALID_HANDLE;

    LIST_FOR_EACH_SAFE( item, next, &query->counters )
    {
        struct counter *counter = LIST_ENTRY( item, struct counter, entry );

        list_remove( &counter->entry );

        pdh_free( counter->path );
        pdh_free( counter );
    }

    query->magic = 0;
    pdh_free( query );

    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhCollectQueryData   (PDH.@)
 */
PDH_STATUS WINAPI PdhCollectQueryData( PDH_HQUERY handle )
{
    struct query *query = handle;
    struct list *item;

    TRACE("%p\n", handle);

    if (!query || (query->magic != PDH_MAGIC_QUERY)) return PDH_INVALID_HANDLE;

    LIST_FOR_EACH( item, &query->counters )
    {
        SYSTEMTIME time;
        struct counter *counter = LIST_ENTRY( item, struct counter, entry );

        counter->collect( counter );

        GetLocalTime( &time );
        SystemTimeToFileTime( &time, &counter->stamp );
    }
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhGetCounterInfoA   (PDH.@)
 */
PDH_STATUS WINAPI PdhGetCounterInfoA( PDH_HCOUNTER handle, BOOLEAN text, LPDWORD size, PPDH_COUNTER_INFO_A info )
{
    struct counter *counter = handle;

    TRACE("%p %d %p %p\n", handle, text, size, info);

    if (!counter) return PDH_INVALID_HANDLE;
    if (!size)    return PDH_INVALID_ARGUMENT;

    if (*size < sizeof(PDH_COUNTER_INFO_A))
    {
        *size = sizeof(PDH_COUNTER_INFO_A);
        return PDH_MORE_DATA;
    }

    memset( info, 0, sizeof(PDH_COUNTER_INFO_A) );

    info->dwType          = counter->type;
    info->CStatus         = counter->status;
    info->lScale          = counter->scale;
    info->lDefaultScale   = counter->defaultscale;
    info->dwUserData      = counter->user;
    info->dwQueryUserData = counter->queryuser;

    *size = sizeof(PDH_COUNTER_INFO_A);
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhGetCounterInfoW   (PDH.@)
 */
PDH_STATUS WINAPI PdhGetCounterInfoW( PDH_HCOUNTER handle, BOOLEAN text, LPDWORD size, PPDH_COUNTER_INFO_W info )
{
    struct counter *counter = handle;

    TRACE("%p %d %p %p\n", handle, text, size, info);

    if (!counter) return PDH_INVALID_HANDLE;
    if (!size)    return PDH_INVALID_ARGUMENT;

    if (*size < sizeof(PDH_COUNTER_INFO_W))
    {
        *size = sizeof(PDH_COUNTER_INFO_W);
        return PDH_MORE_DATA;
    }

    memset( info, 0, sizeof(PDH_COUNTER_INFO_W) );

    info->dwType          = counter->type;
    info->CStatus         = counter->status;
    info->lScale          = counter->scale;
    info->lDefaultScale   = counter->defaultscale;
    info->dwUserData      = counter->user;
    info->dwQueryUserData = counter->queryuser;

    *size = sizeof(PDH_COUNTER_INFO_W);
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhGetCounterTimeBase   (PDH.@)
 */
PDH_STATUS WINAPI PdhGetCounterTimeBase( PDH_HCOUNTER handle, LONGLONG *base )
{
    struct counter *counter = handle;

    TRACE("%p %p\n", handle, base);

    if (!base)    return PDH_INVALID_ARGUMENT;
    if (!counter) return PDH_INVALID_HANDLE;

    *base = counter->base;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhGetFormattedCounterValue   (PDH.@)
 */
PDH_STATUS WINAPI PdhGetFormattedCounterValue( PDH_HCOUNTER handle, DWORD format,
                                               LPDWORD type, PPDH_FMT_COUNTERVALUE value )
{
    LONG factor;
    struct counter *counter = handle;

    TRACE("%p %x %p %p\n", handle, format, type, value);

    if (!value)   return PDH_INVALID_ARGUMENT;
    if (!counter) return PDH_INVALID_HANDLE;

    if (counter->status) return PDH_INVALID_DATA;

    factor = counter->scale ? counter->scale : counter->defaultscale;
    if (format & PDH_FMT_LONG)
    {
        if (format & PDH_FMT_1000) value->u.longValue = counter->two.longvalue * 1000;
        else value->u.longValue = counter->two.longvalue * pow( 10, factor );
    }
    else if (format & PDH_FMT_LARGE)
    {
        if (format & PDH_FMT_1000) value->u.largeValue = counter->two.largevalue * 1000;
        else value->u.largeValue = counter->two.largevalue * pow( 10, factor );
    }
    else if (format & PDH_FMT_DOUBLE)
    {
        if (format & PDH_FMT_1000) value->u.doubleValue = counter->two.doublevalue * 1000;
        else value->u.doubleValue = counter->two.doublevalue * pow( 10, factor );
    }
    else
    {
        WARN("unknown format %x\n", format);
        return PDH_INVALID_ARGUMENT;
    }
    value->CStatus = ERROR_SUCCESS;

    if (type) *type = counter->type;
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhGetRawCounterValue   (PDH.@)
 */
PDH_STATUS WINAPI PdhGetRawCounterValue( PDH_HCOUNTER handle, LPDWORD type,
                                         PPDH_RAW_COUNTER value )
{
    struct counter *counter = handle;

    TRACE("%p %p %p\n", handle, type, value);

    if (!value)   return PDH_INVALID_ARGUMENT;
    if (!counter) return PDH_INVALID_HANDLE;

    value->CStatus                  = counter->status;
    value->TimeStamp.dwLowDateTime  = counter->stamp.dwLowDateTime;
    value->TimeStamp.dwHighDateTime = counter->stamp.dwHighDateTime;
    value->FirstValue               = counter->one.largevalue;
    value->SecondValue              = counter->two.largevalue;
    value->MultiCount               = 1; /* FIXME */

    if (type) *type = counter->type;
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

/***********************************************************************
 *              PdhRemoveCounter   (PDH.@)
 */
PDH_STATUS WINAPI PdhRemoveCounter( PDH_HCOUNTER handle )
{
    struct counter *counter = handle;

    TRACE("%p\n", handle);

    if (!counter) return PDH_INVALID_HANDLE;

    list_remove( &counter->entry );

    pdh_free( counter->path );
    pdh_free( counter );

    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhSetCounterScaleFactor   (PDH.@)
 */
PDH_STATUS WINAPI PdhSetCounterScaleFactor( PDH_HCOUNTER handle, LONG factor )
{
    struct counter *counter = handle;

    TRACE("%p\n", handle);

    if (!counter) return PDH_INVALID_HANDLE;
    if (factor < PDH_MIN_SCALE || factor > PDH_MAX_SCALE) return PDH_INVALID_ARGUMENT;

    counter->scale = factor;
    return ERROR_SUCCESS;
}
