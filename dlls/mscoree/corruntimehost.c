/*
 *
 * Copyright 2008 Alistair Leslie-Hughes
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
#include "winreg.h"
#include "ole2.h"

#include "cor.h"
#include "mscoree.h"
#include "metahost.h"
#include "mscoree_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( mscoree );

struct RuntimeHost
{
    const struct ICorRuntimeHostVtbl *lpVtbl;
    const struct ICLRRuntimeHostVtbl *lpCLRHostVtbl;
    const CLRRuntimeInfo *version;
    const loaded_mono *mono;
    LONG ref;
};

static inline RuntimeHost *impl_from_ICLRRuntimeHost( ICLRRuntimeHost *iface )
{
    return (RuntimeHost *)((char*)iface - FIELD_OFFSET(RuntimeHost, lpCLRHostVtbl));
}

static inline RuntimeHost *impl_from_ICorRuntimeHost( ICorRuntimeHost *iface )
{
    return (RuntimeHost *)((char*)iface - FIELD_OFFSET(RuntimeHost, lpVtbl));
}

/*** IUnknown methods ***/
static HRESULT WINAPI corruntimehost_QueryInterface(ICorRuntimeHost* iface,
        REFIID riid,
        void **ppvObject)
{
    RuntimeHost *This = impl_from_ICorRuntimeHost( iface );
    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_ICorRuntimeHost ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ICorRuntimeHost_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI corruntimehost_AddRef(ICorRuntimeHost* iface)
{
    RuntimeHost *This = impl_from_ICorRuntimeHost( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI corruntimehost_Release(ICorRuntimeHost* iface)
{
    RuntimeHost *This = impl_from_ICorRuntimeHost( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );

    return ref;
}

/*** ICorRuntimeHost methods ***/
static HRESULT WINAPI corruntimehost_CreateLogicalThreadState(
                    ICorRuntimeHost* iface)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_DeleteLogicalThreadState(
                    ICorRuntimeHost* iface)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_SwitchInLogicalThreadState(
                    ICorRuntimeHost* iface,
                    DWORD *fiberCookie)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_SwitchOutLogicalThreadState(
                    ICorRuntimeHost* iface,
                    DWORD **fiberCookie)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_LocksHeldByLogicalThread(
                    ICorRuntimeHost* iface,
                    DWORD *pCount)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_MapFile(
    ICorRuntimeHost* iface,
    HANDLE hFile,
    HMODULE *mapAddress)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_GetConfiguration(
    ICorRuntimeHost* iface,
    ICorConfiguration **pConfiguration)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_Start(
    ICorRuntimeHost* iface)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_Stop(
    ICorRuntimeHost* iface)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_CreateDomain(
    ICorRuntimeHost* iface,
    LPCWSTR friendlyName,
    IUnknown *identityArray,
    IUnknown **appDomain)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_GetDefaultDomain(
    ICorRuntimeHost* iface,
    IUnknown **pAppDomain)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_EnumDomains(
    ICorRuntimeHost* iface,
    HDOMAINENUM *hEnum)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_NextDomain(
    ICorRuntimeHost* iface,
    HDOMAINENUM hEnum,
    IUnknown **appDomain)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_CloseEnum(
    ICorRuntimeHost* iface,
    HDOMAINENUM hEnum)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_CreateDomainEx(
    ICorRuntimeHost* iface,
    LPCWSTR friendlyName,
    IUnknown *setup,
    IUnknown *evidence,
    IUnknown **appDomain)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_CreateDomainSetup(
    ICorRuntimeHost* iface,
    IUnknown **appDomainSetup)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_CreateEvidence(
    ICorRuntimeHost* iface,
    IUnknown **evidence)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_UnloadDomain(
    ICorRuntimeHost* iface,
    IUnknown *appDomain)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_CurrentDomain(
    ICorRuntimeHost* iface,
    IUnknown **appDomain)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static const struct ICorRuntimeHostVtbl corruntimehost_vtbl =
{
    corruntimehost_QueryInterface,
    corruntimehost_AddRef,
    corruntimehost_Release,
    corruntimehost_CreateLogicalThreadState,
    corruntimehost_DeleteLogicalThreadState,
    corruntimehost_SwitchInLogicalThreadState,
    corruntimehost_SwitchOutLogicalThreadState,
    corruntimehost_LocksHeldByLogicalThread,
    corruntimehost_MapFile,
    corruntimehost_GetConfiguration,
    corruntimehost_Start,
    corruntimehost_Stop,
    corruntimehost_CreateDomain,
    corruntimehost_GetDefaultDomain,
    corruntimehost_EnumDomains,
    corruntimehost_NextDomain,
    corruntimehost_CloseEnum,
    corruntimehost_CreateDomainEx,
    corruntimehost_CreateDomainSetup,
    corruntimehost_CreateEvidence,
    corruntimehost_UnloadDomain,
    corruntimehost_CurrentDomain
};

static HRESULT WINAPI CLRRuntimeHost_QueryInterface(ICLRRuntimeHost* iface,
        REFIID riid,
        void **ppvObject)
{
    RuntimeHost *This = impl_from_ICLRRuntimeHost( iface );
    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_ICLRRuntimeHost ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ICLRRuntimeHost_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI CLRRuntimeHost_AddRef(ICLRRuntimeHost* iface)
{
    RuntimeHost *This = impl_from_ICLRRuntimeHost( iface );
    return IUnknown_AddRef((IUnknown*)&This->lpVtbl);
}

static ULONG WINAPI CLRRuntimeHost_Release(ICLRRuntimeHost* iface)
{
    RuntimeHost *This = impl_from_ICLRRuntimeHost( iface );
    return IUnknown_Release((IUnknown*)&This->lpVtbl);
}

static HRESULT WINAPI CLRRuntimeHost_Start(ICLRRuntimeHost* iface)
{
    FIXME("(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_Stop(ICLRRuntimeHost* iface)
{
    FIXME("(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_SetHostControl(ICLRRuntimeHost* iface,
    IHostControl *pHostControl)
{
    FIXME("(%p,%p)\n", iface, pHostControl);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_GetCLRControl(ICLRRuntimeHost* iface,
    ICLRControl **pCLRControl)
{
    FIXME("(%p,%p)\n", iface, pCLRControl);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_UnloadAppDomain(ICLRRuntimeHost* iface,
    DWORD dwAppDomainId, BOOL fWaitUntilDone)
{
    FIXME("(%p,%u,%i)\n", iface, dwAppDomainId, fWaitUntilDone);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_ExecuteInAppDomain(ICLRRuntimeHost* iface,
    DWORD dwAppDomainId, FExecuteInAppDomainCallback pCallback, void *cookie)
{
    FIXME("(%p,%u,%p,%p)\n", iface, dwAppDomainId, pCallback, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_GetCurrentAppDomainId(ICLRRuntimeHost* iface,
    DWORD *pdwAppDomainId)
{
    FIXME("(%p,%p)\n", iface, pdwAppDomainId);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_ExecuteApplication(ICLRRuntimeHost* iface,
    LPCWSTR pwzAppFullName, DWORD dwManifestPaths, LPCWSTR *ppwzManifestPaths,
    DWORD dwActivationData, LPCWSTR *ppwzActivationData, int *pReturnValue)
{
    FIXME("(%p,%s,%u,%u)\n", iface, debugstr_w(pwzAppFullName), dwManifestPaths, dwActivationData);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_ExecuteInDefaultAppDomain(ICLRRuntimeHost* iface,
    LPCWSTR pwzAssemblyPath, LPCWSTR pwzTypeName, LPCWSTR pwzMethodName,
    LPCWSTR pwzArgument, DWORD *pReturnValue)
{
    FIXME("(%p,%s,%s,%s,%s)\n", iface, debugstr_w(pwzAssemblyPath),
        debugstr_w(pwzTypeName), debugstr_w(pwzMethodName), debugstr_w(pwzArgument));
    return E_NOTIMPL;
}

static const struct ICLRRuntimeHostVtbl CLRHostVtbl =
{
    CLRRuntimeHost_QueryInterface,
    CLRRuntimeHost_AddRef,
    CLRRuntimeHost_Release,
    CLRRuntimeHost_Start,
    CLRRuntimeHost_Stop,
    CLRRuntimeHost_SetHostControl,
    CLRRuntimeHost_GetCLRControl,
    CLRRuntimeHost_UnloadAppDomain,
    CLRRuntimeHost_ExecuteInAppDomain,
    CLRRuntimeHost_GetCurrentAppDomainId,
    CLRRuntimeHost_ExecuteApplication,
    CLRRuntimeHost_ExecuteInDefaultAppDomain
};

HRESULT RuntimeHost_Construct(const CLRRuntimeInfo *runtime_version,
    const loaded_mono *loaded_mono, RuntimeHost** result)
{
    RuntimeHost *This;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof *This );
    if ( !This )
        return E_OUTOFMEMORY;

    This->lpVtbl = &corruntimehost_vtbl;
    This->lpCLRHostVtbl = &CLRHostVtbl;
    This->ref = 1;
    This->version = runtime_version;
    This->mono = loaded_mono;

    *result = This;

    return S_OK;
}

HRESULT RuntimeHost_GetInterface(RuntimeHost *This, REFCLSID clsid, REFIID riid, void **ppv)
{
    IUnknown *unk;

    if (IsEqualGUID(clsid, &CLSID_CorRuntimeHost))
        unk = (IUnknown*)&This->lpVtbl;
    else if (IsEqualGUID(clsid, &CLSID_CLRRuntimeHost))
        unk = (IUnknown*)&This->lpCLRHostVtbl;
    else
        unk = NULL;

    if (unk)
        return IUnknown_QueryInterface(unk, riid, ppv);
    else
        FIXME("not implemented for class %s\n", debugstr_guid(clsid));

    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT RuntimeHost_Destroy(RuntimeHost *This)
{
    HeapFree( GetProcessHeap(), 0, This );
    return S_OK;
}
