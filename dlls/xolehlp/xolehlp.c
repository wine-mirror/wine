/*
 * Copyright 2011 Hans Leidekker for CodeWeavers
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
#include "transact.h"
#include "initguid.h"
#include "txdtc.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xolehlp);

/* Resource manager start */

typedef struct {
    IResourceManager IResourceManager_iface;
    LONG ref;
} ResourceManager;

static inline ResourceManager *impl_from_IResourceManager(IResourceManager *iface)
{
    return CONTAINING_RECORD(iface, ResourceManager, IResourceManager_iface);
}

static HRESULT WINAPI ResourceManager_QueryInterface(IResourceManager *iface, REFIID iid,
    void **ppv)
{
    ResourceManager *This = impl_from_IResourceManager(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IResourceManager, iid))
    {
        *ppv = &This->IResourceManager_iface;
    }
    else
    {
        FIXME("(%s): not implemented\n", debugstr_guid(iid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ResourceManager_AddRef(IResourceManager *iface)
{
    ResourceManager *This = impl_from_IResourceManager(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI ResourceManager_Release(IResourceManager *iface)
{
    ResourceManager *This = impl_from_IResourceManager(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}
static HRESULT WINAPI ResourceManager_Enlist(IResourceManager *iface,
	ITransaction *pTransaction,ITransactionResourceAsync *pRes,XACTUOW *pUOW,
	LONG *pisoLevel,ITransactionEnlistmentAsync **ppEnlist)
{
    FIXME("(%p, %p, %p, %p, %p, %p): stub\n", iface, pTransaction,pRes,pUOW,
        pisoLevel,ppEnlist);
    return E_NOTIMPL;
}
static HRESULT WINAPI ResourceManager_Reenlist(IResourceManager *iface,
        byte *pPrepInfo,ULONG cbPrepInfo,DWORD lTimeout,XACTSTAT *pXactStat)
{
    FIXME("(%p, %p, %u, %u, %p): stub\n", iface, pPrepInfo, cbPrepInfo, lTimeout, pXactStat);
    return E_NOTIMPL;
}
static HRESULT WINAPI ResourceManager_ReenlistmentComplete(IResourceManager *iface)
{
    FIXME("(%p): stub\n", iface);
    return S_OK;
}
static HRESULT WINAPI ResourceManager_GetDistributedTransactionManager(IResourceManager *iface,
        REFIID iid,void **ppvObject)
{
    FIXME("(%p, %s, %p): stub\n", iface, debugstr_guid(iid), ppvObject);
    return E_NOTIMPL;
}

static const IResourceManagerVtbl ResourceManager_Vtbl = {
    ResourceManager_QueryInterface,
    ResourceManager_AddRef,
    ResourceManager_Release,
    ResourceManager_Enlist,
    ResourceManager_Reenlist,
    ResourceManager_ReenlistmentComplete,
    ResourceManager_GetDistributedTransactionManager
};

static HRESULT ResourceManager_Create(REFIID riid, void **ppv)
{
    ResourceManager *This;
    HRESULT ret;

    if (!ppv) return E_INVALIDARG;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(ResourceManager));
    if (!This) return E_OUTOFMEMORY;

    This->IResourceManager_iface.lpVtbl = &ResourceManager_Vtbl;
    This->ref = 1;

    ret = IResourceManager_QueryInterface(&This->IResourceManager_iface, riid, ppv);
    IResourceManager_Release(&This->IResourceManager_iface);

    return ret;
}

/* Resource manager end */


/* DTC Proxy Core Object start */

typedef struct {
    ITransactionDispenser ITransactionDispenser_iface;
    LONG ref;
    IResourceManagerFactory2 IResourceManagerFactory2_iface;
    ITransactionImportWhereabouts ITransactionImportWhereabouts_iface;
} TransactionManager;

static inline TransactionManager *impl_from_ITransactionDispenser(ITransactionDispenser *iface)
{
    return CONTAINING_RECORD(iface, TransactionManager, ITransactionDispenser_iface);
}

static HRESULT WINAPI TransactionDispenser_QueryInterface(ITransactionDispenser *iface, REFIID iid,
    void **ppv)
{
    TransactionManager *This = impl_from_ITransactionDispenser(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_ITransactionDispenser, iid))
    {
        *ppv = &This->ITransactionDispenser_iface;
    }
    else if (IsEqualIID(&IID_IResourceManagerFactory, iid) ||
        IsEqualIID(&IID_IResourceManagerFactory2, iid))
    {
        *ppv = &This->IResourceManagerFactory2_iface;
    }
    else if (IsEqualIID(&IID_ITransactionImportWhereabouts, iid))
    {
        *ppv = &This->ITransactionImportWhereabouts_iface;
    }
    else
    {
        FIXME("(%s): not implemented\n", debugstr_guid(iid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI TransactionDispenser_AddRef(ITransactionDispenser *iface)
{
    TransactionManager *This = impl_from_ITransactionDispenser(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI TransactionDispenser_Release(ITransactionDispenser *iface)
{
    TransactionManager *This = impl_from_ITransactionDispenser(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI TransactionDispenser_GetOptionsObject(ITransactionDispenser *iface,
        ITransactionOptions **ppOptions)
{
    FIXME("(%p, %p): stub\n", iface, ppOptions);
    if (!ppOptions) return E_INVALIDARG;
    *ppOptions = NULL;
    return E_NOTIMPL;
}
static HRESULT WINAPI TransactionDispenser_BeginTransaction(ITransactionDispenser *iface,
        IUnknown *punkOuter,
        ISOLEVEL isoLevel,
        ULONG isoFlags,
        ITransactionOptions *pOptions,
        ITransaction **ppTransaction)
{
    FIXME("(%p, %p, %08x, %08x, %p, %p): stub\n", iface, punkOuter,
        isoLevel, isoFlags, pOptions, ppTransaction);

    if (!ppTransaction) return E_INVALIDARG;
    *ppTransaction = NULL;
    if (punkOuter) return CLASS_E_NOAGGREGATION;
    return E_NOTIMPL;
}
static const ITransactionDispenserVtbl TransactionDispenser_Vtbl = {
    TransactionDispenser_QueryInterface,
    TransactionDispenser_AddRef,
    TransactionDispenser_Release,
    TransactionDispenser_GetOptionsObject,
    TransactionDispenser_BeginTransaction
};

static inline TransactionManager *impl_from_IResourceManagerFactory2(IResourceManagerFactory2 *iface)
{
    return CONTAINING_RECORD(iface, TransactionManager, IResourceManagerFactory2_iface);
}

static HRESULT WINAPI ResourceManagerFactory2_QueryInterface(IResourceManagerFactory2 *iface, REFIID iid,
    void **ppv)
{
    TransactionManager *This = impl_from_IResourceManagerFactory2(iface);
    return TransactionDispenser_QueryInterface(&This->ITransactionDispenser_iface, iid, ppv);
}

static ULONG WINAPI ResourceManagerFactory2_AddRef(IResourceManagerFactory2 *iface)
{
    TransactionManager *This = impl_from_IResourceManagerFactory2(iface);
    return TransactionDispenser_AddRef(&This->ITransactionDispenser_iface);
}

static ULONG WINAPI ResourceManagerFactory2_Release(IResourceManagerFactory2 *iface)
{
    TransactionManager *This = impl_from_IResourceManagerFactory2(iface);
    return TransactionDispenser_Release(&This->ITransactionDispenser_iface);
}
static HRESULT WINAPI ResourceManagerFactory2_Create(IResourceManagerFactory2 *iface,
        GUID *pguidRM, CHAR *pszRMName, IResourceManagerSink *pIResMgrSink, IResourceManager **ppResMgr)
{
    FIXME("(%p, %s, %s, %p, %p): semi-stub\n", iface, debugstr_guid(pguidRM),
        debugstr_a(pszRMName), pIResMgrSink, ppResMgr);
    return ResourceManager_Create(&IID_IResourceManager, (void**)ppResMgr);
}
static HRESULT WINAPI ResourceManagerFactory2_CreateEx(IResourceManagerFactory2 *iface,
        GUID *pguidRM, CHAR *pszRMName, IResourceManagerSink *pIResMgrSink, REFIID riidRequested, void **ppResMgr)
{
    FIXME("(%p, %s, %s, %p, %s, %p): semi-stub\n", iface, debugstr_guid(pguidRM),
        debugstr_a(pszRMName), pIResMgrSink, debugstr_guid(riidRequested), ppResMgr);

    return ResourceManager_Create(riidRequested, ppResMgr);
}
static const IResourceManagerFactory2Vtbl ResourceManagerFactory2_Vtbl = {
    ResourceManagerFactory2_QueryInterface,
    ResourceManagerFactory2_AddRef,
    ResourceManagerFactory2_Release,
    ResourceManagerFactory2_Create,
    ResourceManagerFactory2_CreateEx
};

static inline TransactionManager *impl_from_ITransactionImportWhereabouts(ITransactionImportWhereabouts *iface)
{
    return CONTAINING_RECORD(iface, TransactionManager, ITransactionImportWhereabouts_iface);
}

static HRESULT WINAPI TransactionImportWhereabouts_QueryInterface(ITransactionImportWhereabouts *iface, REFIID iid,
    void **ppv)
{
    TransactionManager *This = impl_from_ITransactionImportWhereabouts(iface);
    return TransactionDispenser_QueryInterface(&This->ITransactionDispenser_iface, iid, ppv);
}

static ULONG WINAPI TransactionImportWhereabouts_AddRef(ITransactionImportWhereabouts *iface)
{
    TransactionManager *This = impl_from_ITransactionImportWhereabouts(iface);
    return TransactionDispenser_AddRef(&This->ITransactionDispenser_iface);
}

static ULONG WINAPI TransactionImportWhereabouts_Release(ITransactionImportWhereabouts *iface)
{
    TransactionManager *This = impl_from_ITransactionImportWhereabouts(iface);
    return TransactionDispenser_Release(&This->ITransactionDispenser_iface);
}
static HRESULT WINAPI TransactionImportWhereabouts_GetWhereaboutsSize(ITransactionImportWhereabouts *iface,
        ULONG *pcbWhereabouts)
{
    FIXME("(%p, %p): stub\n", iface, pcbWhereabouts);

    return E_NOTIMPL;
}
static HRESULT WINAPI TransactionImportWhereabouts_GetWhereabouts(ITransactionImportWhereabouts *iface,
        ULONG cbWhereabouts, BYTE *rgbWhereabouts,ULONG *pcbUsed)
{
    FIXME("(%p, %u, %p, %p): stub\n", iface, cbWhereabouts, rgbWhereabouts, pcbUsed);

    return E_NOTIMPL;
}
static const ITransactionImportWhereaboutsVtbl TransactionImportWhereabouts_Vtbl = {
    TransactionImportWhereabouts_QueryInterface,
    TransactionImportWhereabouts_AddRef,
    TransactionImportWhereabouts_Release,
    TransactionImportWhereabouts_GetWhereaboutsSize,
    TransactionImportWhereabouts_GetWhereabouts
};

static HRESULT TransactionManager_Create(REFIID riid, void **ppv)
{
    TransactionManager *This;
    HRESULT ret;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(TransactionManager));
    if (!This) return E_OUTOFMEMORY;

    This->ITransactionDispenser_iface.lpVtbl = &TransactionDispenser_Vtbl;
    This->IResourceManagerFactory2_iface.lpVtbl = &ResourceManagerFactory2_Vtbl;
    This->ITransactionImportWhereabouts_iface.lpVtbl = &TransactionImportWhereabouts_Vtbl;
    This->ref = 1;

    ret = ITransactionDispenser_QueryInterface(&This->ITransactionDispenser_iface, riid, ppv);
    ITransactionDispenser_Release(&This->ITransactionDispenser_iface);

    return ret;
}
/* DTC Proxy Core Object end */

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE("%p, %u, %p\n", hinst, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hinst );
            break;
    }
    return TRUE;
}

static BOOL is_local_machineA( const CHAR *server )
{
    static const CHAR dot[] = ".";
    CHAR buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD len = sizeof(buffer) / sizeof(buffer[0]);

    if (!server || !strcmp( server, dot )) return TRUE;
    if (GetComputerNameA( buffer, &len ) && !lstrcmpiA( server, buffer )) return TRUE;
    return FALSE;
}
static BOOL is_local_machineW( const WCHAR *server )
{
    static const WCHAR dotW[] = {'.',0};
    WCHAR buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD len = sizeof(buffer) / sizeof(buffer[0]);

    if (!server || !strcmpW( server, dotW )) return TRUE;
    if (GetComputerNameW( buffer, &len ) && !strcmpiW( server, buffer )) return TRUE;
    return FALSE;
}

HRESULT CDECL DtcGetTransactionManager(char *host, char *tm_name, REFIID riid,
        DWORD dwReserved1, WORD wcbReserved2, void *pvReserved2, void **ppv)
{
    TRACE("(%s, %s, %s, %d, %d, %p, %p)\n", debugstr_a(host), debugstr_a(tm_name),
          debugstr_guid(riid), dwReserved1, wcbReserved2, pvReserved2, ppv);

    if (!is_local_machineA(host))
    {
        FIXME("remote computer not supported\n");
        return E_NOTIMPL;
    }
    return TransactionManager_Create(riid, ppv);
}

HRESULT CDECL DtcGetTransactionManagerExA(CHAR *host, CHAR *tm_name, REFIID riid,
        DWORD options, void *config, void **ppv)
{
    TRACE("(%s, %s, %s, %d, %p, %p)\n", debugstr_a(host), debugstr_a(tm_name),
          debugstr_guid(riid), options, config, ppv);

    if (!is_local_machineA(host))
    {
        FIXME("remote computer not supported\n");
        return E_NOTIMPL;
    }
    return TransactionManager_Create(riid, ppv);
}

HRESULT CDECL DtcGetTransactionManagerExW(WCHAR *host, WCHAR *tm_name, REFIID riid,
        DWORD options, void *config, void **ppv)
{
    TRACE("(%s, %s, %s, %d, %p, %p)\n", debugstr_w(host), debugstr_w(tm_name),
            debugstr_guid(riid), options, config, ppv);

    if (!is_local_machineW(host))
    {
        FIXME("remote computer not supported\n");
        return E_NOTIMPL;
    }
    return TransactionManager_Create(riid, ppv);
}
