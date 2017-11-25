/*
 * advapi32 perf functions
 *
 * Copyright 2017 Austin English
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "perflib.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

PPERF_COUNTERSET_INSTANCE WINAPI PerfCreateInstance(HANDLE handle, LPCGUID guid, const WCHAR *name, ULONG id)
{
    FIXME("%p %s %s %u: stub\n", handle, debugstr_guid(guid), debugstr_w(name), id);
    return NULL;
}

ULONG WINAPI PerfDeleteInstance(HANDLE provider, PPERF_COUNTERSET_INSTANCE block)
{
    FIXME("%p %p: stub\n", provider, block);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

ULONG WINAPI PerfSetCounterSetInfo(HANDLE handle, PPERF_COUNTERSET_INFO template, ULONG size)
{
    FIXME("%p %p %u: stub\n", handle, template, size);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

ULONG WINAPI PerfSetCounterRefValue(HANDLE provider, PPERF_COUNTERSET_INSTANCE instance, ULONG counterid, void *address)
{
    FIXME("%p %p %u %p: stub\n", provider, instance, counterid, address);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

ULONG WINAPI PerfStartProvider(GUID *guid, PERFLIBREQUEST callback, HANDLE *provider)
{
    FIXME("%s %p %p: stub\n", debugstr_guid(guid), callback, provider);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

ULONG WINAPI PerfStartProviderEx(GUID *guid, PPERF_PROVIDER_CONTEXT context, HANDLE *provider)
{
    FIXME("%s %p %p: stub\n", debugstr_guid(guid), context, provider);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

ULONG WINAPI PerfStopProvider(HANDLE handle)
{
    FIXME("%p: stub\n", handle);
    return ERROR_CALL_NOT_IMPLEMENTED;
}
