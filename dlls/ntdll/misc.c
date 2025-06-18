/*
 * Helper functions for ntdll
 *
 * Copyright 2000 Juergen Schmied
 * Copyright 2010 Marcus Meissner
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

#include <time.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/debug.h"
#include "ntdll_misc.h"
#include "wmistr.h"
#include "evntrace.h"
#include "evntprov.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

LPCSTR debugstr_us( const UNICODE_STRING *us )
{
    if (!us) return "<null>";
    return debugstr_wn(us->Buffer, us->Length / sizeof(WCHAR));
}

static int __cdecl compare_wrapper(void *ctx, const void *e1, const void *e2)
{
    int (__cdecl *compare)( const void *, const void * ) = ctx;
    return compare( e1, e2 );
}

static inline void swap(char *l, char *r, size_t size)
{
    char tmp;

    while(size--) {
        tmp = *l;
        *l++ = *r;
        *r++ = tmp;
    }
}

static void small_sort(void *base, size_t nmemb, size_t size,
        int (CDECL *compar)(void *, const void *, const void *), void *context)
{
    size_t e, i;
    char *max, *p;

    for(e=nmemb; e>1; e--) {
        max = base;
        for(i=1; i<e; i++) {
            p = (char*)base + i*size;
            if(compar(context, p, max) > 0)
                max = p;
        }

        if(p != max)
            swap(p, max, size);
    }
}

static void quick_sort(void *base, size_t nmemb, size_t size,
        int (CDECL *compar)(void *, const void *, const void *), void *context)
{
    size_t stack_lo[8*sizeof(size_t)], stack_hi[8*sizeof(size_t)];
    size_t beg, end, lo, hi, med;
    int stack_pos;

    stack_pos = 0;
    stack_lo[stack_pos] = 0;
    stack_hi[stack_pos] = nmemb-1;

#define X(i) ((char*)base+size*(i))
    while(stack_pos >= 0) {
        beg = stack_lo[stack_pos];
        end = stack_hi[stack_pos--];

        if(end-beg < 8) {
            small_sort(X(beg), end-beg+1, size, compar, context);
            continue;
        }

        lo = beg;
        hi = end;
        med = lo + (hi-lo+1)/2;
        if(compar(context, X(lo), X(med)) > 0)
            swap(X(lo), X(med), size);
        if(compar(context, X(lo), X(hi)) > 0)
            swap(X(lo), X(hi), size);
        if(compar(context, X(med), X(hi)) > 0)
            swap(X(med), X(hi), size);

        lo++;
        hi--;
        while(1) {
            while(lo <= hi) {
                if(lo!=med && compar(context, X(lo), X(med))>0)
                    break;
                lo++;
            }

            while(med != hi) {
                if(compar(context, X(hi), X(med)) <= 0)
                    break;
                hi--;
            }

            if(hi < lo)
                break;

            swap(X(lo), X(hi), size);
            if(hi == med)
                med = lo;
            lo++;
            hi--;
        }

        while(hi > beg) {
            if(hi!=med && compar(context, X(hi), X(med))!=0)
                break;
            hi--;
        }

        if(hi-beg >= end-lo) {
            stack_lo[++stack_pos] = beg;
            stack_hi[stack_pos] = hi;
            stack_lo[++stack_pos] = lo;
            stack_hi[stack_pos] = end;
        }else {
            stack_lo[++stack_pos] = lo;
            stack_hi[stack_pos] = end;
            stack_lo[++stack_pos] = beg;
            stack_hi[stack_pos] = hi;
        }
    }
#undef X
}


/*********************************************************************
 *                  qsort_s   (NTDLL.@)
 */
void __cdecl qsort_s( void *base, size_t nmemb, size_t size,
                      int (__cdecl *compar)(void *, const void *, const void *), void *context )
{
    const size_t total_size = nmemb * size;

    if (!base && nmemb) return;
    if (!size) return;
    if (!compar) return;
    if (total_size / size != nmemb) return;
    if (nmemb < 2) return;
    quick_sort( base, nmemb, size, compar, context );
}


/*********************************************************************
 *                  qsort   (NTDLL.@)
 */
void __cdecl qsort( void *base, size_t nmemb, size_t size,
                    int (__cdecl *compar)(const void *, const void *) )
{
    qsort_s( base, nmemb, size, compare_wrapper, compar );
}


/*********************************************************************
 *                  bsearch_s   (NTDLL.@)
 */
void * __cdecl bsearch_s( const void *key, const void *base, size_t nmemb, size_t size,
                          int (__cdecl *compare)(void *, const void *, const void *), void *ctx )
{
    ssize_t min = 0;
    ssize_t max = nmemb - 1;

    if (!size) return NULL;
    if (!compare) return NULL;

    while (min <= max)
    {
        ssize_t cursor = min + (max - min) / 2;
        int ret = compare(ctx, key,(const char *)base+(cursor*size));
        if (!ret)
            return (char*)base+(cursor*size);
        if (ret < 0)
            max = cursor - 1;
        else
            min = cursor + 1;
    }
    return NULL;
}


/*********************************************************************
 *                  bsearch   (NTDLL.@)
 */
void * __cdecl bsearch( const void *key, const void *base, size_t nmemb,
                        size_t size, int (__cdecl *compar)(const void *, const void *) )
{
    return bsearch_s( key, base, nmemb, size, compare_wrapper, compar );
}



/*********************************************************************
 *                  _lfind   (NTDLL.@)
 */
void * __cdecl _lfind( const void *key, const void *base, unsigned int *nmemb,
                       size_t size, int(__cdecl *compar)(const void *, const void *) )
{
    size_t i, n = *nmemb;

    for (i=0;i<n;i++)
        if (!compar(key,(char*)base+(size*i)))
            return (char*)base+(size*i);
    return NULL;
}

/******************************************************************************
 *                  WinSqmEndSession   (NTDLL.@)
 */
NTSTATUS WINAPI WinSqmEndSession(HANDLE session)
{
    FIXME("(%p): stub\n", session);
    return STATUS_NOT_IMPLEMENTED;
}

/*********************************************************************
 *          WinSqmIncrementDWORD (NTDLL.@)
 */
void WINAPI WinSqmIncrementDWORD(DWORD unk1, DWORD unk2, DWORD unk3)
{
    FIXME("(%ld, %ld, %ld): stub\n", unk1, unk2, unk3);
}

/*********************************************************************
 *                  WinSqmIsOptedIn   (NTDLL.@)
 */
BOOL WINAPI WinSqmIsOptedIn(void)
{
    FIXME("(): stub\n");
    return FALSE;
}

/******************************************************************************
 *                  WinSqmStartSession   (NTDLL.@)
 */
HANDLE WINAPI WinSqmStartSession(GUID *sessionguid, DWORD sessionid, DWORD unknown1)
{
    FIXME("(%p, 0x%lx, 0x%lx): stub\n", sessionguid, sessionid, unknown1);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *          WinSqmSetDWORD (NTDLL.@)
 */
void WINAPI WinSqmSetDWORD(HANDLE session, DWORD datapoint_id, DWORD datapoint_value)
{
    FIXME("(%p, %ld, %ld): stub\n", session, datapoint_id, datapoint_value);
}

/******************************************************************************
 *          WinSqmSetIfMaxDWORD (NTDLL.@)
 */
void WINAPI WinSqmSetIfMaxDWORD(DWORD unk1, DWORD unk2, DWORD unk3)
{
    FIXME("(0x%lx, 0x%lx, 0x%lx): stub\n", unk1, unk2, unk3);
}

/******************************************************************************
 *                  EtwEventActivityIdControl (NTDLL.@)
 */
ULONG WINAPI EtwEventActivityIdControl(ULONG code, GUID *guid)
{
    static int once;

    if (!once++) FIXME("0x%lx, %p: stub\n", code, guid);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwEventProviderEnabled (NTDLL.@)
 */
BOOLEAN WINAPI EtwEventProviderEnabled( REGHANDLE handle, UCHAR level, ULONGLONG keyword )
{
    WARN("%s, %u, %s: stub\n", wine_dbgstr_longlong(handle), level, wine_dbgstr_longlong(keyword));
    return FALSE;
}

/******************************************************************************
 *                  EtwEventRegister (NTDLL.@)
 */
ULONG WINAPI EtwEventRegister( LPCGUID provider, PENABLECALLBACK callback, PVOID context,
                PREGHANDLE handle )
{
    WARN("(%s, %p, %p, %p) stub.\n", debugstr_guid(provider), callback, context, handle);

    if (!handle) return ERROR_INVALID_PARAMETER;

    *handle = 0xdeadbeef;
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwEventUnregister (NTDLL.@)
 */
ULONG WINAPI EtwEventUnregister( REGHANDLE handle )
{
    WARN("(%s) stub.\n", wine_dbgstr_longlong(handle));
    return ERROR_SUCCESS;
}

/*********************************************************************
 *                  EtwEventSetInformation   (NTDLL.@)
 */
ULONG WINAPI EtwEventSetInformation( REGHANDLE handle, EVENT_INFO_CLASS class, void *info,
                                     ULONG length )
{
    FIXME("(%s, %u, %p, %lu) stub\n", wine_dbgstr_longlong(handle), class, info, length);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwEventWriteString   (NTDLL.@)
 */
ULONG WINAPI EtwEventWriteString( REGHANDLE handle, UCHAR level, ULONGLONG keyword, PCWSTR string )
{
    FIXME("%s, %u, %s, %s: stub\n", wine_dbgstr_longlong(handle), level,
          wine_dbgstr_longlong(keyword), debugstr_w(string));
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwEventWriteTransfer   (NTDLL.@)
 */
ULONG WINAPI EtwEventWriteTransfer( REGHANDLE handle, PCEVENT_DESCRIPTOR descriptor, LPCGUID activity,
                                    LPCGUID related, ULONG count, PEVENT_DATA_DESCRIPTOR data )
{
    FIXME("%s, %p, %s, %s, %lu, %p: stub\n", wine_dbgstr_longlong(handle), descriptor,
          debugstr_guid(activity), debugstr_guid(related), count, data);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwRegisterTraceGuidsW (NTDLL.@)
 *
 * Register an event trace provider and the event trace classes that it uses
 * to generate events.
 *
 * PARAMS
 *  RequestAddress     [I]   ControlCallback function
 *  RequestContext     [I]   Optional provider-defined context
 *  ControlGuid        [I]   GUID of the registering provider
 *  GuidCount          [I]   Number of elements in the TraceGuidReg array
 *  TraceGuidReg       [I/O] Array of TRACE_GUID_REGISTRATION structures
 *  MofImagePath       [I]   not supported, set to NULL
 *  MofResourceName    [I]   not supported, set to NULL
 *  RegistrationHandle [O]   Provider's registration handle
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: System error code
 */
ULONG WINAPI EtwRegisterTraceGuidsW( WMIDPREQUEST RequestAddress,
                void *RequestContext, const GUID *ControlGuid, ULONG GuidCount,
                TRACE_GUID_REGISTRATION *TraceGuidReg, const WCHAR *MofImagePath,
                const WCHAR *MofResourceName, TRACEHANDLE *RegistrationHandle )
{
    WARN("(%p, %p, %s, %lu, %p, %s, %s, %p): stub\n", RequestAddress, RequestContext,
          debugstr_guid(ControlGuid), GuidCount, TraceGuidReg, debugstr_w(MofImagePath),
          debugstr_w(MofResourceName), RegistrationHandle);

    if (TraceGuidReg)
    {
        ULONG i;
        for (i = 0; i < GuidCount; i++)
        {
            FIXME("  register trace class %s\n", debugstr_guid(TraceGuidReg[i].Guid));
            TraceGuidReg[i].RegHandle = (HANDLE)0xdeadbeef;
        }
    }
    *RegistrationHandle = (TRACEHANDLE)0xdeadbeef;
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwRegisterTraceGuidsA (NTDLL.@)
 */
ULONG WINAPI EtwRegisterTraceGuidsA( WMIDPREQUEST RequestAddress,
                void *RequestContext, const GUID *ControlGuid, ULONG GuidCount,
                TRACE_GUID_REGISTRATION *TraceGuidReg, const char *MofImagePath,
                const char *MofResourceName, TRACEHANDLE *RegistrationHandle )
{
    WARN("(%p, %p, %s, %lu, %p, %s, %s, %p): stub\n", RequestAddress, RequestContext,
          debugstr_guid(ControlGuid), GuidCount, TraceGuidReg, debugstr_a(MofImagePath),
          debugstr_a(MofResourceName), RegistrationHandle);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwUnregisterTraceGuids (NTDLL.@)
 */
ULONG WINAPI EtwUnregisterTraceGuids( TRACEHANDLE RegistrationHandle )
{
    if (!RegistrationHandle)
         return ERROR_INVALID_PARAMETER;

    WARN("%s: stub\n", wine_dbgstr_longlong(RegistrationHandle));
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwEventEnabled (NTDLL.@)
 */
BOOLEAN WINAPI EtwEventEnabled( REGHANDLE handle, const EVENT_DESCRIPTOR *descriptor )
{
    WARN("(%s, %p): stub\n", wine_dbgstr_longlong(handle), descriptor);
    return FALSE;
}

/******************************************************************************
 *                  EtwEventWrite (NTDLL.@)
 */
ULONG WINAPI EtwEventWrite( REGHANDLE handle, const EVENT_DESCRIPTOR *descriptor, ULONG count,
    EVENT_DATA_DESCRIPTOR *data )
{
    FIXME("(%s, %p, %lu, %p): stub\n", wine_dbgstr_longlong(handle), descriptor, count, data);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwGetTraceEnableFlags (NTDLL.@)
 */
ULONG WINAPI EtwGetTraceEnableFlags( TRACEHANDLE handle )
{
    FIXME("(%s) stub\n", wine_dbgstr_longlong(handle));
    return 0;
}

/******************************************************************************
 *                  EtwGetTraceEnableLevel (NTDLL.@)
 */
UCHAR WINAPI EtwGetTraceEnableLevel( TRACEHANDLE handle )
{
    FIXME("(%s) stub\n", wine_dbgstr_longlong(handle));
    return TRACE_LEVEL_VERBOSE;
}

/******************************************************************************
 *                  EtwGetTraceLoggerHandle (NTDLL.@)
 */
TRACEHANDLE WINAPI EtwGetTraceLoggerHandle( PVOID buf )
{
    FIXME("(%p) stub\n", buf);
    return INVALID_PROCESSTRACE_HANDLE;
}

/******************************************************************************
 *                  EtwLogTraceEvent (NTDLL.@)
 */
ULONG WINAPI EtwLogTraceEvent( TRACEHANDLE SessionHandle, PEVENT_TRACE_HEADER EventTrace )
{
    FIXME("%s %p\n", wine_dbgstr_longlong(SessionHandle), EventTrace);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 *                  EtwTraceMessageVa (NTDLL.@)
 */
ULONG WINAPI EtwTraceMessageVa( TRACEHANDLE handle, ULONG flags, LPGUID guid, USHORT number,
                                va_list args )
{
    FIXME("(%s %lx %s %d) : stub\n", wine_dbgstr_longlong(handle), flags, debugstr_guid(guid), number);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwTraceMessage (NTDLL.@)
 */
ULONG WINAPIV EtwTraceMessage( TRACEHANDLE handle, ULONG flags, LPGUID guid, /*USHORT*/ ULONG number, ... )
{
    va_list valist;
    ULONG ret;

    va_start( valist, number );
    ret = EtwTraceMessageVa( handle, flags, guid, number, valist );
    va_end( valist );
    return ret;
}
