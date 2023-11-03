/*
 * Event tracing API
 *
 * Copyright 1995 Sven Verdoolaege
 * Copyright 1998 Juergen Schmied
 * Copyright 2003 Mike Hearn
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
#include "windef.h"
#include "winbase.h"
#include "wmistr.h"
#define _WMI_SOURCE_
#include "evntrace.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(eventlog);

/******************************************************************************
 *     ControlTraceA   (sechost.@)
 */
ULONG WINAPI ControlTraceA( TRACEHANDLE handle, const char *session,
                            EVENT_TRACE_PROPERTIES *properties, ULONG control )
{
    FIXME("(%s, %s, %p, %ld) stub\n", wine_dbgstr_longlong(handle), debugstr_a(session), properties, control);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *     ControlTraceW   (sechost.@)
 */
ULONG WINAPI ControlTraceW( TRACEHANDLE handle, const WCHAR *session,
                            EVENT_TRACE_PROPERTIES *properties, ULONG control )
{
    FIXME("(%s, %s, %p, %ld) stub\n", wine_dbgstr_longlong(handle), debugstr_w(session), properties, control);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *     EnableTraceEx2   (sechost.@)
 */
ULONG WINAPI EnableTraceEx2( TRACEHANDLE handle, const GUID *provider, ULONG control, UCHAR level,
                             ULONGLONG match_any, ULONGLONG match_all, ULONG timeout,
                             ENABLE_TRACE_PARAMETERS *params )
{
    FIXME("(%s, %s, %lu, %u, %s, %s, %lu, %p): stub\n", wine_dbgstr_longlong(handle),
          debugstr_guid(provider), control, level, wine_dbgstr_longlong(match_any),
          wine_dbgstr_longlong(match_all), timeout, params);

    return ERROR_SUCCESS;
}

/******************************************************************************
 *     QueryAllTracesA   (sechost.@)
 */
ULONG WINAPI QueryAllTracesA( EVENT_TRACE_PROPERTIES **properties, ULONG count, ULONG *ret_count )
{
    FIXME("(%p, %ld, %p) stub\n", properties, count, ret_count);

    if (ret_count) *ret_count = 0;
    return ERROR_SUCCESS;
}

/******************************************************************************
 *     QueryAllTracesW   (sechost.@)
 */
ULONG WINAPI QueryAllTracesW( EVENT_TRACE_PROPERTIES **properties, ULONG count, ULONG *ret_count )
{
    FIXME("(%p, %ld, %p) stub\n", properties, count, ret_count);

    if (ret_count) *ret_count = 0;
    return ERROR_SUCCESS;
}

/******************************************************************************
 *     StartTraceA   (sechost.@)
 */
ULONG WINAPI StartTraceA( TRACEHANDLE *handle, const char *session, EVENT_TRACE_PROPERTIES *properties )
{
    FIXME("(%p, %s, %p) stub\n", handle, debugstr_a(session), properties);
    if (handle) *handle = 0xcafe4242;
    return ERROR_SUCCESS;
}

/******************************************************************************
 *     StartTraceW   (sechost.@)
 */
ULONG WINAPI StartTraceW( TRACEHANDLE *handle, const WCHAR *session, EVENT_TRACE_PROPERTIES *properties )
{
    FIXME("(%p, %s, %p) stub\n", handle, debugstr_w(session), properties);
    if (handle) *handle = 0xcafe4242;
    return ERROR_SUCCESS;
}

/******************************************************************************
 *     StopTraceW   (sechost.@)
 */
ULONG WINAPI StopTraceW( TRACEHANDLE handle, const WCHAR *session, EVENT_TRACE_PROPERTIES *properties )
{
    FIXME("(%s, %s, %p) stub\n", wine_dbgstr_longlong(handle), debugstr_w(session), properties);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *     OpenTraceW   (sechost.@)
 */
TRACEHANDLE WINAPI OpenTraceW( EVENT_TRACE_LOGFILEW *logfile )
{
    static int once;

    if (!once++) FIXME("%p: stub\n", logfile);
    SetLastError(ERROR_ACCESS_DENIED);
    return INVALID_PROCESSTRACE_HANDLE;
}

/******************************************************************************
 *     ProcessTrace   (sechost.@)
 */
ULONG WINAPI ProcessTrace( TRACEHANDLE *handles, ULONG count, FILETIME *start_time, FILETIME *end_time )
{
    FIXME("%p %lu %p %p: stub\n", handles, count, start_time, end_time);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 *     CloseTrace   (sechost.@)
 */
ULONG WINAPI CloseTrace( TRACEHANDLE handle )
{
    FIXME("%s: stub\n", wine_dbgstr_longlong(handle));
    return ERROR_INVALID_HANDLE;
}

/******************************************************************************
 *     TraceSetInformation   (sechost.@)
 */
ULONG WINAPI TraceSetInformation( TRACEHANDLE handle, TRACE_INFO_CLASS class, void *info, ULONG len )
{
    FIXME("%s %d %p %ld: stub\n", wine_dbgstr_longlong(handle), class, info, len);
    return ERROR_CALL_NOT_IMPLEMENTED;
}
