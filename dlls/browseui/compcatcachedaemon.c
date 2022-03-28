/*
 * Component Category Cache 'Daemon'
 *
 * Copyright (C) 2008 Maarten Lankhorst
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

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"

#include "shlguid.h"
#include "shlobj.h"

#include "browseui.h"
#include "resids.h"

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

typedef struct tagCCCD {
    IRunnableTask IRunnableTask_iface;
    LONG refCount;
    CRITICAL_SECTION cs;
} CompCatCacheDaemon;

static inline CompCatCacheDaemon *impl_from_IRunnableTask(IRunnableTask *iface)
{
    return CONTAINING_RECORD(iface, CompCatCacheDaemon, IRunnableTask_iface);
}

static void CompCatCacheDaemon_Destructor(CompCatCacheDaemon *This)
{
    TRACE("destroying %p\n", This);
    This->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->cs);
    free(This);
    InterlockedDecrement(&BROWSEUI_refCount);
}

static HRESULT WINAPI CompCatCacheDaemon_QueryInterface(IRunnableTask *iface, REFIID iid, LPVOID *ppvOut)
{
    CompCatCacheDaemon *This = impl_from_IRunnableTask(iface);
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IRunnableTask) || IsEqualIID(iid, &IID_IUnknown))
    {
        *ppvOut = &This->IRunnableTask_iface;
    }

    if (*ppvOut)
    {
        IRunnableTask_AddRef(iface);
        return S_OK;
    }

    FIXME("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI CompCatCacheDaemon_AddRef(IRunnableTask *iface)
{
    CompCatCacheDaemon *This = impl_from_IRunnableTask(iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI CompCatCacheDaemon_Release(IRunnableTask *iface)
{
    CompCatCacheDaemon *This = impl_from_IRunnableTask(iface);
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        CompCatCacheDaemon_Destructor(This);
    return ret;
}

static HRESULT WINAPI CompCatCacheDaemon_Run(IRunnableTask *iface)
{
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI CompCatCacheDaemon_Kill(IRunnableTask *iface, BOOL fWait)
{
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI CompCatCacheDaemon_Suspend(IRunnableTask *iface)
{
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI CompCatCacheDaemon_Resume(IRunnableTask *iface)
{
    FIXME("stub\n");
    return S_OK;
}

static ULONG WINAPI CompCatCacheDaemon_IsRunning(IRunnableTask *iface)
{
    FIXME("stub\n");
    return 0;
}

static const IRunnableTaskVtbl CompCatCacheDaemonVtbl =
{
    CompCatCacheDaemon_QueryInterface,
    CompCatCacheDaemon_AddRef,
    CompCatCacheDaemon_Release,
    CompCatCacheDaemon_Run,
    CompCatCacheDaemon_Kill,
    CompCatCacheDaemon_Suspend,
    CompCatCacheDaemon_Resume,
    CompCatCacheDaemon_IsRunning
};

HRESULT CompCatCacheDaemon_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    CompCatCacheDaemon *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (!(This = calloc(1, sizeof(*This))))
        return E_OUTOFMEMORY;

    This->IRunnableTask_iface.lpVtbl = &CompCatCacheDaemonVtbl;
    This->refCount = 1;
    InitializeCriticalSection(&This->cs);
    This->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": CompCatCacheDaemon.cs");

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)&This->IRunnableTask_iface;
    InterlockedIncrement(&BROWSEUI_refCount);
    return S_OK;
}
