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
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xolehlp);

typedef struct {
    ITransactionDispenser ITransactionDispenser_iface;
    LONG ref;
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

static HRESULT TransactionManager_Create(REFIID riid, void **ppv)
{
    TransactionManager *This;
    HRESULT ret;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(TransactionManager));
    if (!This) return E_OUTOFMEMORY;

    This->ITransactionDispenser_iface.lpVtbl = &TransactionDispenser_Vtbl;
    This->ref = 1;

    ret = ITransactionDispenser_QueryInterface(&This->ITransactionDispenser_iface, riid, ppv);
    ITransactionDispenser_Release(&This->ITransactionDispenser_iface);

    return ret;
}

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
