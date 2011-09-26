/*
 *
 * Copyright 2011 Alistair Leslie-Hughes
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"
#include "shellapi.h"
#include "mscoree.h"
#include "corhdr.h"
#include "metahost.h"
#include "cordebug.h"
#include "wine/list.h"
#include "mscoree_private.h"
#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL( mscoree );


static inline RuntimeHost *impl_from_ICorDebug( ICorDebug *iface )
{
    return CONTAINING_RECORD(iface, RuntimeHost, ICorDebug_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI CorDebug_QueryInterface(ICorDebug *iface, REFIID riid, void **ppvObject)
{
    RuntimeHost *This = impl_from_ICorDebug( iface );

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_ICorDebug ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = &This->ICorDebug_iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ICorDebug_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI CorDebug_AddRef(ICorDebug *iface)
{
    RuntimeHost *This = impl_from_ICorDebug( iface );
    return ICorRuntimeHost_AddRef(&This->ICorRuntimeHost_iface);
}

static ULONG WINAPI CorDebug_Release(ICorDebug *iface)
{
    RuntimeHost *This = impl_from_ICorDebug( iface );
    return ICorRuntimeHost_Release(&This->ICorRuntimeHost_iface);
}

/*** ICorDebug methods ***/
static HRESULT WINAPI CorDebug_Initialize(ICorDebug *iface)
{
    RuntimeHost *This = impl_from_ICorDebug( iface );
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_Terminate(ICorDebug *iface)
{
    RuntimeHost *This = impl_from_ICorDebug( iface );
    FIXME("stub %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_SetManagedHandler(ICorDebug *iface, ICorDebugManagedCallback *pCallback)
{
    RuntimeHost *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %p\n", This, pCallback);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_SetUnmanagedHandler(ICorDebug *iface, ICorDebugUnmanagedCallback *pCallback)
{
    RuntimeHost *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %p\n", This, pCallback);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_CreateProcess(ICorDebug *iface, LPCWSTR lpApplicationName,
            LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,
            LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
            DWORD dwCreationFlags, PVOID lpEnvironment,LPCWSTR lpCurrentDirectory,
            LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
            CorDebugCreateProcessFlags debuggingFlags, ICorDebugProcess **ppProcess)
{
    RuntimeHost *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %s %s %p %p %d %d %p %s %p %p %d %p\n", This, debugstr_w(lpApplicationName),
            debugstr_w(lpCommandLine), lpProcessAttributes, lpThreadAttributes,
            bInheritHandles, dwCreationFlags, lpEnvironment, debugstr_w(lpCurrentDirectory),
            lpStartupInfo, lpProcessInformation, debuggingFlags, ppProcess);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_DebugActiveProcess(ICorDebug *iface, DWORD id, BOOL win32Attach,
            ICorDebugProcess **ppProcess)
{
    RuntimeHost *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %d %d %p\n", This, id, win32Attach, ppProcess);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_EnumerateProcesses( ICorDebug *iface, ICorDebugProcessEnum **ppProcess)
{
    RuntimeHost *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %p\n", This, ppProcess);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_GetProcess(ICorDebug *iface, DWORD dwProcessId, ICorDebugProcess **ppProcess)
{
    RuntimeHost *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %d %p\n", This, dwProcessId, ppProcess);
    return E_NOTIMPL;
}

static HRESULT WINAPI CorDebug_CanLaunchOrAttach(ICorDebug *iface, DWORD dwProcessId,
            BOOL win32DebuggingEnabled)
{
    RuntimeHost *This = impl_from_ICorDebug( iface );
    FIXME("stub %p %d %d\n", This, dwProcessId, win32DebuggingEnabled);
    return E_NOTIMPL;
}

static const struct ICorDebugVtbl cordebug_vtbl =
{
    CorDebug_QueryInterface,
    CorDebug_AddRef,
    CorDebug_Release,
    CorDebug_Initialize,
    CorDebug_Terminate,
    CorDebug_SetManagedHandler,
    CorDebug_SetUnmanagedHandler,
    CorDebug_CreateProcess,
    CorDebug_DebugActiveProcess,
    CorDebug_EnumerateProcesses,
    CorDebug_GetProcess,
    CorDebug_CanLaunchOrAttach
};

void cordebug_init(RuntimeHost *This)
{
    This->ICorDebug_iface.lpVtbl = &cordebug_vtbl;
}
