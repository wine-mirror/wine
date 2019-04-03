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
    LONG refcount;
};

static struct debug_client *impl_from_IDebugClient(IDebugClient *iface)
{
    return CONTAINING_RECORD(iface, struct debug_client, IDebugClient_iface);
}

static HRESULT STDMETHODCALLTYPE debugclient_QueryInterface(IDebugClient *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IDebugClient) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        iface->lpVtbl->AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
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
