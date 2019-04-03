/*
 * Support for Microsoft Debugging Extension API
 *
 * Copyright (C) 2010 Volodymyr Shcherbyna
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winternl.h"

#include "initguid.h"
#include "dbgeng.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbgeng);

struct debug_client
{
    IDebugClient IDebugClient_iface;
    IDebugDataSpaces IDebugDataSpaces_iface;
    IDebugSymbols IDebugSymbols_iface;
    LONG refcount;
};

static struct debug_client *impl_from_IDebugClient(IDebugClient *iface)
{
    return CONTAINING_RECORD(iface, struct debug_client, IDebugClient_iface);
}

static struct debug_client *impl_from_IDebugDataSpaces(IDebugDataSpaces *iface)
{
    return CONTAINING_RECORD(iface, struct debug_client, IDebugDataSpaces_iface);
}

static struct debug_client *impl_from_IDebugSymbols(IDebugSymbols *iface)
{
    return CONTAINING_RECORD(iface, struct debug_client, IDebugSymbols_iface);
}

static HRESULT STDMETHODCALLTYPE debugclient_QueryInterface(IDebugClient *iface, REFIID riid, void **obj)
{
    struct debug_client *debug_client = impl_from_IDebugClient(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDebugClient) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IDebugDataSpaces))
    {
        *obj = &debug_client->IDebugDataSpaces_iface;
    }
    else if (IsEqualIID(riid, &IID_IDebugSymbols))
    {
        *obj = &debug_client->IDebugSymbols_iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE debugclient_AddRef(IDebugClient *iface)
{
    struct debug_client *debug_client = impl_from_IDebugClient(iface);
    ULONG refcount = InterlockedIncrement(&debug_client->refcount);

    TRACE("%p, %d.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE debugclient_Release(IDebugClient *iface)
{
    struct debug_client *debug_client = impl_from_IDebugClient(iface);
    ULONG refcount = InterlockedDecrement(&debug_client->refcount);

    TRACE("%p, %d.\n", debug_client, refcount);

    if (!refcount)
        heap_free(debug_client);

    return refcount;
}

static HRESULT STDMETHODCALLTYPE debugclient_AttachKernel(IDebugClient *iface, ULONG flags, const char *options)
{
    FIXME("%p, %#x, %s stub.\n", iface, flags, debugstr_a(options));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetKernelConnectionOptions(IDebugClient *iface, char *buffer,
        ULONG buffer_size, ULONG *options_size)
{
    FIXME("%p, %p, %u, %p stub.\n", iface, buffer, buffer_size, options_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetKernelConnectionOptions(IDebugClient *iface, const char *options)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(options));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_StartProcessServer(IDebugClient *iface, ULONG flags, const char *options,
        void *reserved)
{
    FIXME("%p, %#x, %s, %p stub.\n", iface, flags, debugstr_a(options), reserved);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_ConnectProcessServer(IDebugClient *iface, const char *remote_options,
        ULONG64 *server)
{
    FIXME("%p, %s, %p stub.\n", iface, debugstr_a(remote_options), server);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_DisconnectProcessServer(IDebugClient *iface, ULONG64 server)
{
    FIXME("%p, %s stub.\n", iface, wine_dbgstr_longlong(server));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetRunningProcessSystemIds(IDebugClient *iface, ULONG64 server,
        ULONG *ids, ULONG count, ULONG *actual_count)
{
    FIXME("%p, %s, %p, %u, %p stub.\n", iface, wine_dbgstr_longlong(server), ids, count, actual_count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetRunningProcessSystemIdByExecutableName(IDebugClient *iface,
        ULONG64 server, const char *exe_name, ULONG flags, ULONG *id)
{
    FIXME("%p, %s, %s, %#x, %p stub.\n", iface, wine_dbgstr_longlong(server), debugstr_a(exe_name), flags, id);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetRunningProcessDescription(IDebugClient *iface, ULONG64 server,
        ULONG systemid, ULONG flags, char *exe_name, ULONG exe_name_size, ULONG *actual_exe_name_size,
        char *description, ULONG description_size, ULONG *actual_description_size)
{
    FIXME("%p, %s, %u, %#x, %p, %u, %p, %p, %u, %p stub.\n", iface, wine_dbgstr_longlong(server), systemid, flags,
            exe_name, exe_name_size, actual_exe_name_size, description, description_size, actual_description_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_AttachProcess(IDebugClient *iface, ULONG64 server, ULONG pid, ULONG flags)
{
    FIXME("%p, %s, %u, %#x stub.\n", iface, wine_dbgstr_longlong(server), pid, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_CreateProcess(IDebugClient *iface, ULONG64 server, char *cmdline,
        ULONG flags)
{
    FIXME("%p, %s, %s, %#x stub.\n", iface, wine_dbgstr_longlong(server), debugstr_a(cmdline), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_CreateProcessAndAttach(IDebugClient *iface, ULONG64 server, char *cmdline,
        ULONG create_flags, ULONG pid, ULONG attach_flags)
{
    FIXME("%p, %s, %s, %#x, %u, %#x stub.\n", iface, wine_dbgstr_longlong(server), debugstr_a(cmdline), create_flags,
            pid, attach_flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetProcessOptions(IDebugClient *iface, ULONG *options)
{
    FIXME("%p, %p stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_AddProcessOptions(IDebugClient *iface, ULONG options)
{
    FIXME("%p, %#x stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_RemoveProcessOptions(IDebugClient *iface, ULONG options)
{
    FIXME("%p, %#x stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetProcessOptions(IDebugClient *iface, ULONG options)
{
    FIXME("%p, %#x stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_OpenDumpFile(IDebugClient *iface, const char *filename)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(filename));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_WriteDumpFile(IDebugClient *iface, const char *filename, ULONG qualifier)
{
    FIXME("%p, %s, %u stub.\n", iface, debugstr_a(filename), qualifier);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_ConnectSession(IDebugClient *iface, ULONG flags, ULONG history_limit)
{
    FIXME("%p, %#x, %u stub.\n", iface, flags, history_limit);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_StartServer(IDebugClient *iface, const char *options)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(options));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_OutputServers(IDebugClient *iface, ULONG output_control,
        const char *machine, ULONG flags)
{
    FIXME("%p, %u, %s, %#x stub.\n", iface, output_control, debugstr_a(machine), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_TerminateProcesses(IDebugClient *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_DetachProcesses(IDebugClient *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_EndSession(IDebugClient *iface, ULONG flags)
{
    FIXME("%p, %#x stub.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetExitCode(IDebugClient *iface, ULONG *code)
{
    FIXME("%p, %p stub.\n", iface, code);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_DispatchCallbacks(IDebugClient *iface, ULONG timeout)
{
    FIXME("%p, %u stub.\n", iface, timeout);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_ExitDispatch(IDebugClient *iface, IDebugClient *client)
{
    FIXME("%p, %p stub.\n", iface, client);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_CreateClient(IDebugClient *iface, IDebugClient **client)
{
    FIXME("%p, %p stub.\n", iface, client);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetInputCallbacks(IDebugClient *iface, IDebugInputCallbacks **callbacks)
{
    FIXME("%p, %p stub.\n", iface, callbacks);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetInputCallbacks(IDebugClient *iface, IDebugInputCallbacks *callbacks)
{
    FIXME("%p, %p stub.\n", iface, callbacks);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetOutputCallbacks(IDebugClient *iface, IDebugOutputCallbacks **callbacks)
{
    FIXME("%p, %p stub.\n", iface, callbacks);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetOutputCallbacks(IDebugClient *iface, IDebugOutputCallbacks *callbacks)
{
    FIXME("%p, %p stub.\n", iface, callbacks);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetOutputMask(IDebugClient *iface, ULONG *mask)
{
    FIXME("%p, %p stub.\n", iface, mask);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetOutputMask(IDebugClient *iface, ULONG mask)
{
    FIXME("%p, %#x stub.\n", iface, mask);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetOtherOutputMask(IDebugClient *iface, IDebugClient *client, ULONG *mask)
{
    FIXME("%p, %p, %p stub.\n", iface, client, mask);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetOtherOutputMask(IDebugClient *iface, IDebugClient *client, ULONG mask)
{
    FIXME("%p, %p, %#x stub.\n", iface, client, mask);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetOutputWidth(IDebugClient *iface, ULONG *columns)
{
    FIXME("%p, %p stub.\n", iface, columns);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetOutputWidth(IDebugClient *iface, ULONG columns)
{
    FIXME("%p, %u stub.\n", iface, columns);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetOutputLinePrefix(IDebugClient *iface, char *buffer, ULONG buffer_size,
        ULONG *prefix_size)
{
    FIXME("%p, %p, %u, %p stub.\n", iface, buffer, buffer_size, prefix_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetOutputLinePrefix(IDebugClient *iface, const char *prefix)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(prefix));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetIdentity(IDebugClient *iface, char *buffer, ULONG buffer_size,
        ULONG *identity_size)
{
    FIXME("%p, %p, %u, %p stub.\n", iface, buffer, buffer_size, identity_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_OutputIdentity(IDebugClient *iface, ULONG output_control, ULONG flags,
        const char *format)
{
    FIXME("%p, %u, %#x, %s stub.\n", iface, output_control, flags, debugstr_a(format));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_GetEventCallbacks(IDebugClient *iface, IDebugEventCallbacks **callbacks)
{
    FIXME("%p, %p stub.\n", iface, callbacks);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_SetEventCallbacks(IDebugClient *iface, IDebugEventCallbacks *callbacks)
{
    FIXME("%p, %p stub.\n", iface, callbacks);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugclient_FlushCallbacks(IDebugClient *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static const IDebugClientVtbl debugclientvtbl =
{
    debugclient_QueryInterface,
    debugclient_AddRef,
    debugclient_Release,
    debugclient_AttachKernel,
    debugclient_GetKernelConnectionOptions,
    debugclient_SetKernelConnectionOptions,
    debugclient_StartProcessServer,
    debugclient_ConnectProcessServer,
    debugclient_DisconnectProcessServer,
    debugclient_GetRunningProcessSystemIds,
    debugclient_GetRunningProcessSystemIdByExecutableName,
    debugclient_GetRunningProcessDescription,
    debugclient_AttachProcess,
    debugclient_CreateProcess,
    debugclient_CreateProcessAndAttach,
    debugclient_GetProcessOptions,
    debugclient_AddProcessOptions,
    debugclient_RemoveProcessOptions,
    debugclient_SetProcessOptions,
    debugclient_OpenDumpFile,
    debugclient_WriteDumpFile,
    debugclient_ConnectSession,
    debugclient_StartServer,
    debugclient_OutputServers,
    debugclient_TerminateProcesses,
    debugclient_DetachProcesses,
    debugclient_EndSession,
    debugclient_GetExitCode,
    debugclient_DispatchCallbacks,
    debugclient_ExitDispatch,
    debugclient_CreateClient,
    debugclient_GetInputCallbacks,
    debugclient_SetInputCallbacks,
    debugclient_GetOutputCallbacks,
    debugclient_SetOutputCallbacks,
    debugclient_GetOutputMask,
    debugclient_SetOutputMask,
    debugclient_GetOtherOutputMask,
    debugclient_SetOtherOutputMask,
    debugclient_GetOutputWidth,
    debugclient_SetOutputWidth,
    debugclient_GetOutputLinePrefix,
    debugclient_SetOutputLinePrefix,
    debugclient_GetIdentity,
    debugclient_OutputIdentity,
    debugclient_GetEventCallbacks,
    debugclient_SetEventCallbacks,
    debugclient_FlushCallbacks,
};

static HRESULT STDMETHODCALLTYPE debugdataspaces_QueryInterface(IDebugDataSpaces *iface, REFIID riid, void **obj)
{
    struct debug_client *debug_client = impl_from_IDebugDataSpaces(iface);
    IUnknown *unk = (IUnknown *)&debug_client->IDebugClient_iface;
    return IUnknown_QueryInterface(unk, riid, obj);
}

static ULONG STDMETHODCALLTYPE debugdataspaces_AddRef(IDebugDataSpaces *iface)
{
    struct debug_client *debug_client = impl_from_IDebugDataSpaces(iface);
    IUnknown *unk = (IUnknown *)&debug_client->IDebugClient_iface;
    return IUnknown_AddRef(unk);
}

static ULONG STDMETHODCALLTYPE debugdataspaces_Release(IDebugDataSpaces *iface)
{
    struct debug_client *debug_client = impl_from_IDebugDataSpaces(iface);
    IUnknown *unk = (IUnknown *)&debug_client->IDebugClient_iface;
    return IUnknown_Release(unk);
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadVirtual(IDebugDataSpaces *iface, ULONG64 offset, void *buffer,
        ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %s, %p, %u, %p stub.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WriteVirtual(IDebugDataSpaces *iface, ULONG64 offset, void *buffer,
        ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %s, %p, %u, %p stub.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_SearchVirtual(IDebugDataSpaces *iface, ULONG64 offset, ULONG64 length,
        void *pattern, ULONG pattern_size, ULONG pattern_granularity, ULONG64 *ret_offset)
{
    FIXME("%p, %s, %s, %p, %u, %u, %p stub.\n", iface, wine_dbgstr_longlong(offset), wine_dbgstr_longlong(length),
            pattern, pattern_size, pattern_granularity, ret_offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadVirtualUncached(IDebugDataSpaces *iface, ULONG64 offset,
        void *buffer, ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %s, %p, %u, %p stub.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WriteVirtualUncached(IDebugDataSpaces *iface, ULONG64 offset,
        void *buffer, ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %s, %p, %u, %p stub.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadPointersVirtual(IDebugDataSpaces *iface, ULONG count,
        ULONG64 offset, ULONG64 *pointers)
{
    FIXME("%p, %u, %s, %p stub.\n", iface, count, wine_dbgstr_longlong(offset), pointers);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WritePointersVirtual(IDebugDataSpaces *iface, ULONG count,
        ULONG64 offset, ULONG64 *pointers)
{
    FIXME("%p, %u, %s, %p stub.\n", iface, count, wine_dbgstr_longlong(offset), pointers);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadPhysical(IDebugDataSpaces *iface, ULONG64 offset, void *buffer,
        ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %s, %p, %u, %p stub.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WritePhysical(IDebugDataSpaces *iface, ULONG64 offset, void *buffer,
        ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %s, %p, %u, %p stub.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadControl(IDebugDataSpaces *iface, ULONG processor, ULONG64 offset,
        void *buffer, ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %u, %s, %p, %u, %p stub.\n", iface, processor, wine_dbgstr_longlong(offset), buffer, buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WriteControl(IDebugDataSpaces *iface, ULONG processor, ULONG64 offset,
        void *buffer, ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %u, %s, %p, %u, %p stub.\n", iface, processor, wine_dbgstr_longlong(offset), buffer, buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadIo(IDebugDataSpaces *iface, ULONG type, ULONG bus_number,
        ULONG address_space, ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %u, %u, %u, %s, %p, %u, %p stub.\n", iface, type, bus_number, address_space, wine_dbgstr_longlong(offset),
            buffer, buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WriteIo(IDebugDataSpaces *iface, ULONG type, ULONG bus_number,
        ULONG address_space, ULONG64 offset, void *buffer, ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %u, %u, %u, %s, %p, %u, %p stub.\n", iface, type, bus_number, address_space, wine_dbgstr_longlong(offset),
            buffer, buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadMsr(IDebugDataSpaces *iface, ULONG msr, ULONG64 *value)
{
    FIXME("%p, %u, %p stub.\n", iface, msr, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WriteMsr(IDebugDataSpaces *iface, ULONG msr, ULONG64 value)
{
    FIXME("%p, %u, %s stub.\n", iface, msr, wine_dbgstr_longlong(value));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadBusData(IDebugDataSpaces *iface, ULONG data_type,
        ULONG bus_number, ULONG slot_number, ULONG offset, void *buffer, ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %u, %u, %u, %u, %p, %u, %p stub.\n", iface, data_type, bus_number, slot_number, offset, buffer,
            buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_WriteBusData(IDebugDataSpaces *iface, ULONG data_type,
        ULONG bus_number, ULONG slot_number, ULONG offset, void *buffer, ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %u, %u, %u, %u, %p, %u, %p stub.\n", iface, data_type, bus_number, slot_number, offset, buffer,
            buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_CheckLowMemory(IDebugDataSpaces *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadDebuggerData(IDebugDataSpaces *iface, ULONG index, void *buffer,
        ULONG buffer_size, ULONG *data_size)
{
    FIXME("%p, %u, %p, %u, %p stub.\n", iface, index, buffer, buffer_size, data_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugdataspaces_ReadProcessorSystemData(IDebugDataSpaces *iface, ULONG processor,
        ULONG index, void *buffer, ULONG buffer_size, ULONG *data_size)
{
    FIXME("%p, %u, %u, %p, %u, %p stub.\n", iface, processor, index, buffer, buffer_size, data_size);

    return E_NOTIMPL;
}

static const IDebugDataSpacesVtbl debugdataspacesvtbl =
{
    debugdataspaces_QueryInterface,
    debugdataspaces_AddRef,
    debugdataspaces_Release,
    debugdataspaces_ReadVirtual,
    debugdataspaces_WriteVirtual,
    debugdataspaces_SearchVirtual,
    debugdataspaces_ReadVirtualUncached,
    debugdataspaces_WriteVirtualUncached,
    debugdataspaces_ReadPointersVirtual,
    debugdataspaces_WritePointersVirtual,
    debugdataspaces_ReadPhysical,
    debugdataspaces_WritePhysical,
    debugdataspaces_ReadControl,
    debugdataspaces_WriteControl,
    debugdataspaces_ReadIo,
    debugdataspaces_WriteIo,
    debugdataspaces_ReadMsr,
    debugdataspaces_WriteMsr,
    debugdataspaces_ReadBusData,
    debugdataspaces_WriteBusData,
    debugdataspaces_CheckLowMemory,
    debugdataspaces_ReadDebuggerData,
    debugdataspaces_ReadProcessorSystemData,
};

static HRESULT STDMETHODCALLTYPE debugsymbols_QueryInterface(IDebugSymbols *iface, REFIID riid, void **obj)
{
    struct debug_client *debug_client = impl_from_IDebugSymbols(iface);
    IUnknown *unk = (IUnknown *)&debug_client->IDebugClient_iface;
    return IUnknown_QueryInterface(unk, riid, obj);
}

static ULONG STDMETHODCALLTYPE debugsymbols_AddRef(IDebugSymbols *iface)
{
    struct debug_client *debug_client = impl_from_IDebugSymbols(iface);
    IUnknown *unk = (IUnknown *)&debug_client->IDebugClient_iface;
    return IUnknown_AddRef(unk);
}

static ULONG STDMETHODCALLTYPE debugsymbols_Release(IDebugSymbols *iface)
{
    struct debug_client *debug_client = impl_from_IDebugSymbols(iface);
    IUnknown *unk = (IUnknown *)&debug_client->IDebugClient_iface;
    return IUnknown_Release(unk);
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolOptions(IDebugSymbols *iface, ULONG *options)
{
    FIXME("%p, %p stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AddSymbolOptions(IDebugSymbols *iface, ULONG options)
{
    FIXME("%p, %#x stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_RemoveSymbolOptions(IDebugSymbols *iface, ULONG options)
{
    FIXME("%p, %#x stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetSymbolOptions(IDebugSymbols *iface, ULONG options)
{
    FIXME("%p, %#x stub.\n", iface, options);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetNameByOffset(IDebugSymbols *iface, ULONG64 offset, char *buffer,
        ULONG buffer_size, ULONG *name_size, ULONG64 *displacement)
{
    FIXME("%p, %s, %p, %u, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), buffer, buffer_size,
            name_size, displacement);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetOffsetByName(IDebugSymbols *iface, const char *symbol,
        ULONG64 *offset)
{
    FIXME("%p, %s, %p stub.\n", iface, debugstr_a(symbol), offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetNearNameByOffset(IDebugSymbols *iface, ULONG64 offset, LONG delta,
        char *buffer, ULONG buffer_size, ULONG *name_size, ULONG64 *displacement)
{
    FIXME("%p, %s, %d, %p, %u, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), delta, buffer, buffer_size,
            name_size, displacement);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetLineByOffset(IDebugSymbols *iface, ULONG64 offset, ULONG *line,
        char *buffer, ULONG buffer_size, ULONG *file_size, ULONG64 *displacement)
{
    FIXME("%p, %s, %p, %p, %u, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), line, buffer, buffer_size,
            file_size, displacement);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetOffsetByLine(IDebugSymbols *iface, ULONG line, const char *file,
        ULONG64 *offset)
{
    FIXME("%p, %u, %s, %p stub.\n", iface, line, debugstr_a(file), offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetNumberModules(IDebugSymbols *iface, ULONG *loaded, ULONG *unloaded)
{
    FIXME("%p, %p, %p stub.\n", iface, loaded, unloaded);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleByIndex(IDebugSymbols *iface, ULONG index, ULONG64 *base)
{
    FIXME("%p, %u, %p stub.\n", iface, index, base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleByModuleName(IDebugSymbols *iface, const char *name,
        ULONG start_index, ULONG *index, ULONG64 *base)
{
    FIXME("%p, %s, %u, %p, %p stub.\n", iface, debugstr_a(name), start_index, index, base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleByOffset(IDebugSymbols *iface, ULONG64 offset,
        ULONG start_index, ULONG *index, ULONG64 *base)
{
    FIXME("%p, %s, %u, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), start_index, index, base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleNames(IDebugSymbols *iface, ULONG index, ULONG64 base,
        char *image_name, ULONG image_name_buffer_size, ULONG *image_name_size, char *module_name,
        ULONG module_name_buffer_size, ULONG *module_name_size, char *loaded_image_name,
        ULONG loaded_image_name_buffer_size, ULONG *loaded_image_size)
{
    FIXME("%p, %u, %s, %p, %u, %p, %p, %u, %p, %p, %u, %p stub.\n", iface, index, wine_dbgstr_longlong(base),
            image_name, image_name_buffer_size, image_name_size, module_name, module_name_buffer_size,
            module_name_size, loaded_image_name, loaded_image_name_buffer_size, loaded_image_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetModuleParameters(IDebugSymbols *iface, ULONG count, ULONG64 *bases,
        ULONG start, DEBUG_MODULE_PARAMETERS *parameters)
{
    FIXME("%p, %u, %p, %u, %p stub.\n", iface, count, bases, start, parameters);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolModule(IDebugSymbols *iface, const char *symbol, ULONG64 *base)
{
    FIXME("%p, %s, %p stub.\n", iface, debugstr_a(symbol), base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetTypeName(IDebugSymbols *iface, ULONG64 base, ULONG type_id,
        char *buffer, ULONG buffer_size, ULONG *name_size)
{
    FIXME("%p, %s, %u, %p, %u, %p stub.\n", iface, wine_dbgstr_longlong(base), type_id, buffer,
            buffer_size, name_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetTypeId(IDebugSymbols *iface, ULONG64 base, ULONG type_id, ULONG *size)
{
    FIXME("%p, %s, %u, %p stub.\n", iface, wine_dbgstr_longlong(base), type_id, size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetFieldOffset(IDebugSymbols *iface, ULONG64 base, ULONG type_id,
        const char *field, ULONG *offset)
{
    FIXME("%p, %s, %u, %s, %p stub.\n", iface, wine_dbgstr_longlong(base), type_id, debugstr_a(field), offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolTypeId(IDebugSymbols *iface, const char *symbol, ULONG *type_id,
        ULONG64 *base)
{
    FIXME("%p, %s, %p, %p stub.\n", iface, debugstr_a(symbol), type_id, base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetOffsetTypeId(IDebugSymbols *iface, ULONG64 offset, ULONG *type_id,
        ULONG64 *base)
{
    FIXME("%p, %s, %p, %p stub.\n", iface, wine_dbgstr_longlong(offset), type_id, base);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_ReadTypedDataVirtual(IDebugSymbols *iface, ULONG64 offset, ULONG64 base,
        ULONG type_id, void *buffer, ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %s, %s, %u, %p, %u, %p stub.\n", iface, wine_dbgstr_longlong(offset), wine_dbgstr_longlong(base),
            type_id, buffer, buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_WriteTypedDataVirtual(IDebugSymbols *iface, ULONG64 offset, ULONG64 base,
        ULONG type_id, void *buffer, ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %s, %s, %u, %p, %u, %p stub.\n", iface, wine_dbgstr_longlong(offset), wine_dbgstr_longlong(base),
            type_id, buffer, buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_OutputTypedDataVirtual(IDebugSymbols *iface, ULONG output_control,
        ULONG64 offset, ULONG64 base, ULONG type_id, ULONG flags)
{
    FIXME("%p, %#x, %s, %s, %u, %#x stub.\n", iface, output_control, wine_dbgstr_longlong(offset),
            wine_dbgstr_longlong(base), type_id, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_ReadTypedDataPhysical(IDebugSymbols *iface, ULONG64 offset, ULONG64 base,
        ULONG type_id, void *buffer, ULONG buffer_size, ULONG *read_len)
{
    FIXME("%p, %s, %s, %u, %p, %u, %p stub.\n", iface, wine_dbgstr_longlong(offset), wine_dbgstr_longlong(base),
            type_id, buffer, buffer_size, read_len);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_WriteTypedDataPhysical(IDebugSymbols *iface, ULONG64 offset,
        ULONG64 base, ULONG type_id, void *buffer, ULONG buffer_size, ULONG *written)
{
    FIXME("%p, %s, %s, %u, %p, %u, %p stub.\n", iface, wine_dbgstr_longlong(offset), wine_dbgstr_longlong(base),
            type_id, buffer, buffer_size, written);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_OutputTypedDataPhysical(IDebugSymbols *iface, ULONG output_control,
        ULONG64 offset, ULONG64 base, ULONG type_id, ULONG flags)
{
    FIXME("%p, %#x, %s, %s, %u, %#x stub.\n", iface, output_control, wine_dbgstr_longlong(offset),
            wine_dbgstr_longlong(base), type_id, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetScope(IDebugSymbols *iface, ULONG64 *instr_offset,
        DEBUG_STACK_FRAME *frame, void *scope_context, ULONG scope_context_size)
{
    FIXME("%p, %p, %p, %p, %u stub.\n", iface, instr_offset, frame, scope_context, scope_context_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetScope(IDebugSymbols *iface, ULONG64 instr_offset,
        DEBUG_STACK_FRAME *frame, void *scope_context, ULONG scope_context_size)
{
    FIXME("%p, %s, %p, %p, %u stub.\n", iface, wine_dbgstr_longlong(instr_offset), frame, scope_context,
            scope_context_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_ResetScope(IDebugSymbols *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetScopeSymbolGroup(IDebugSymbols *iface, ULONG flags,
        IDebugSymbolGroup *update, IDebugSymbolGroup **symbols)
{
    FIXME("%p, %#x, %p, %p stub.\n", iface, flags, update, symbols);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_CreateSymbolGroup(IDebugSymbols *iface, IDebugSymbolGroup **group)
{
    FIXME("%p, %p stub.\n", iface, group);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_StartSymbolMatch(IDebugSymbols *iface, const char *pattern,
        ULONG64 *handle)
{
    FIXME("%p, %s, %p stub.\n", iface, pattern, handle);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetNextSymbolMatch(IDebugSymbols *iface, ULONG64 handle, char *buffer,
        ULONG buffer_size, ULONG *match_size, ULONG64 *offset)
{
    FIXME("%p, %s, %p, %u, %p, %p stub.\n", iface, wine_dbgstr_longlong(handle), buffer, buffer_size, match_size, offset);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_EndSymbolMatch(IDebugSymbols *iface, ULONG64 handle)
{
    FIXME("%p, %s stub.\n", iface, wine_dbgstr_longlong(handle));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_Reload(IDebugSymbols *iface, const char *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSymbolPath(IDebugSymbols *iface, char *buffer, ULONG buffer_size,
        ULONG *path_size)
{
    FIXME("%p, %p, %u, %p stub.\n", iface, buffer, buffer_size, path_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetSymbolPath(IDebugSymbols *iface, const char *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AppendSymbolPath(IDebugSymbols *iface, const char *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetImagePath(IDebugSymbols *iface, char *buffer, ULONG buffer_size,
        ULONG *path_size)
{
    FIXME("%p, %p, %u, %p stub.\n", iface, buffer, buffer_size, path_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetImagePath(IDebugSymbols *iface, const char *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AppendImagePath(IDebugSymbols *iface, const char *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourcePath(IDebugSymbols *iface, char *buffer, ULONG buffer_size,
        ULONG *path_size)
{
    FIXME("%p, %p, %u, %p stub.\n", iface, buffer, buffer_size, path_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourcePathElement(IDebugSymbols *iface, ULONG index, char *buffer,
        ULONG buffer_size, ULONG *element_size)
{
    FIXME("%p, %u, %p, %u, %p stub.\n", iface, index, buffer, buffer_size, element_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_SetSourcePath(IDebugSymbols *iface, const char *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_AppendSourcePath(IDebugSymbols *iface, const char *path)
{
    FIXME("%p, %s stub.\n", iface, debugstr_a(path));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_FindSourceFile(IDebugSymbols *iface, ULONG start, const char *file,
        ULONG flags, ULONG *found_element, char *buffer, ULONG buffer_size, ULONG *found_size)
{
    FIXME("%p, %u, %s, %#x, %p, %p, %u, %p stub.\n", iface, start, debugstr_a(file), flags, found_element, buffer,
            buffer_size, found_size);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE debugsymbols_GetSourceFileLineOffsets(IDebugSymbols *iface, const char *file,
        ULONG64 *buffer, ULONG buffer_lines, ULONG *file_lines)
{
    FIXME("%p, %s, %p, %u, %p stub.\n", iface, debugstr_a(file), buffer, buffer_lines, file_lines);

    return E_NOTIMPL;
}

static const IDebugSymbolsVtbl debugsymbolsvtbl =
{
    debugsymbols_QueryInterface,
    debugsymbols_AddRef,
    debugsymbols_Release,
    debugsymbols_GetSymbolOptions,
    debugsymbols_AddSymbolOptions,
    debugsymbols_RemoveSymbolOptions,
    debugsymbols_SetSymbolOptions,
    debugsymbols_GetNameByOffset,
    debugsymbols_GetOffsetByName,
    debugsymbols_GetNearNameByOffset,
    debugsymbols_GetLineByOffset,
    debugsymbols_GetOffsetByLine,
    debugsymbols_GetNumberModules,
    debugsymbols_GetModuleByIndex,
    debugsymbols_GetModuleByModuleName,
    debugsymbols_GetModuleByOffset,
    debugsymbols_GetModuleNames,
    debugsymbols_GetModuleParameters,
    debugsymbols_GetSymbolModule,
    debugsymbols_GetTypeName,
    debugsymbols_GetTypeId,
    debugsymbols_GetFieldOffset,
    debugsymbols_GetSymbolTypeId,
    debugsymbols_GetOffsetTypeId,
    debugsymbols_ReadTypedDataVirtual,
    debugsymbols_WriteTypedDataVirtual,
    debugsymbols_OutputTypedDataVirtual,
    debugsymbols_ReadTypedDataPhysical,
    debugsymbols_WriteTypedDataPhysical,
    debugsymbols_OutputTypedDataPhysical,
    debugsymbols_GetScope,
    debugsymbols_SetScope,
    debugsymbols_ResetScope,
    debugsymbols_GetScopeSymbolGroup,
    debugsymbols_CreateSymbolGroup,
    debugsymbols_StartSymbolMatch,
    debugsymbols_GetNextSymbolMatch,
    debugsymbols_EndSymbolMatch,
    debugsymbols_Reload,
    debugsymbols_GetSymbolPath,
    debugsymbols_SetSymbolPath,
    debugsymbols_AppendSymbolPath,
    debugsymbols_GetImagePath,
    debugsymbols_SetImagePath,
    debugsymbols_AppendImagePath,
    debugsymbols_GetSourcePath,
    debugsymbols_GetSourcePathElement,
    debugsymbols_SetSourcePath,
    debugsymbols_AppendSourcePath,
    debugsymbols_FindSourceFile,
    debugsymbols_GetSourceFileLineOffsets,
};

/************************************************************
*                    DebugExtensionInitialize   (DBGENG.@)
*
* Initializing Debug Engine
*
* PARAMS
*   pVersion  [O] Receiving the version of extension
*   pFlags    [O] Reserved
*
* RETURNS
*   Success: S_OK
*   Failure: Anything other than S_OK
*
* BUGS
*   Unimplemented
*/
HRESULT WINAPI DebugExtensionInitialize(ULONG * pVersion, ULONG * pFlags)
{
    FIXME("(%p,%p): stub\n", pVersion, pFlags);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return E_NOTIMPL;
}

/************************************************************
*                    DebugCreate   (dbgeng.@)
*/
HRESULT WINAPI DebugCreate(REFIID riid, void **obj)
{
    struct debug_client *debug_client;
    IUnknown *unk;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_guid(riid), obj);

    debug_client = heap_alloc(sizeof(*debug_client));
    if (!debug_client)
        return E_OUTOFMEMORY;

    debug_client->IDebugClient_iface.lpVtbl = &debugclientvtbl;
    debug_client->IDebugDataSpaces_iface.lpVtbl = &debugdataspacesvtbl;
    debug_client->IDebugSymbols_iface.lpVtbl = &debugsymbolsvtbl;
    debug_client->refcount = 1;

    unk = (IUnknown *)&debug_client->IDebugClient_iface;

    hr = IUnknown_QueryInterface(unk, riid, obj);
    IUnknown_Release(unk);

    return hr;
}

/************************************************************
*                    DebugCreateEx   (DBGENG.@)
*/
HRESULT WINAPI DebugCreateEx(REFIID riid, DWORD flags, void **obj)
{
    FIXME("(%s, %#x, %p): stub\n", debugstr_guid(riid), flags, obj);

    return E_NOTIMPL;
}

/************************************************************
*                    DebugConnect   (DBGENG.@)
*
* Creating Debug Engine client object and connecting it to remote host
*
* PARAMS
*   RemoteOptions [I] Options which define how debugger engine connects to remote host
*   InterfaceId   [I] Interface Id of debugger client
*   pInterface    [O] Pointer to interface as requested via InterfaceId
*
* RETURNS
*   Success: S_OK
*   Failure: Anything other than S_OK
*
* BUGS
*   Unimplemented
*/
HRESULT WINAPI DebugConnect(PCSTR RemoteOptions, REFIID InterfaceId, PVOID * pInterface)
{
    FIXME("(%p,%p,%p): stub\n", RemoteOptions, InterfaceId, pInterface);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return E_NOTIMPL;
}
