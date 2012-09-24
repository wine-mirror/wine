/*
 * Copyright (C) 2012 Alistair Leslie-Hughes
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
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "ole2.h"
#include "msdasc.h"
#include "oledberr.h"

#include "oledb_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);


typedef struct datainit
{
    IDataInitialize IDataInitialize_iface;

    LONG ref;
} datainit;

static inline datainit *impl_from_IDataInitialize(IDataInitialize *iface)
{
    return CONTAINING_RECORD(iface, datainit, IDataInitialize_iface);
}

static HRESULT WINAPI datainit_QueryInterface(IDataInitialize *iface, REFIID riid, void **obj)
{
    datainit *This = impl_from_IDataInitialize(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid, &IID_IDataInitialize))
    {
        *obj = &This->IDataInitialize_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDataInitialize_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI datainit_AddRef(IDataInitialize *iface)
{
    datainit *This = impl_from_IDataInitialize(iface);
    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI datainit_Release(IDataInitialize *iface)
{
    datainit *This = impl_from_IDataInitialize(iface);
    LONG ref;

    TRACE("(%p)\n", This);

    ref = InterlockedDecrement(&This->ref);
    if(ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** IDataInitialize methods ***/
static HRESULT WINAPI datainit_GetDataSource(IDataInitialize *iface, IUnknown *pUnkOuter, DWORD dwClsCtx,
                                LPWSTR pwszInitializationString, REFIID riid, IUnknown **ppDataSource)
{
    datainit *This = impl_from_IDataInitialize(iface);

    FIXME("(%p)->(%p %d %s %s %p)\n", This, pUnkOuter, dwClsCtx, debugstr_w(pwszInitializationString),
            debugstr_guid(riid), ppDataSource);

    return E_NOTIMPL;
}

static HRESULT WINAPI datainit_GetInitializationString(IDataInitialize *iface, IUnknown *pDataSource,
                                boolean fIncludePassword, LPWSTR *ppwszInitString)
{
    datainit *This = impl_from_IDataInitialize(iface);

    FIXME("(%p)->(%p %d %p)\n", This, pDataSource, fIncludePassword, ppwszInitString);

    return E_NOTIMPL;
}

static HRESULT WINAPI datainit_CreateDBInstance(IDataInitialize *iface, REFCLSID clsidProvider,
                                IUnknown *pUnkOuter, DWORD dwClsCtx, LPWSTR pwszReserved, REFIID riid,
                                IUnknown **ppDataSource)
{
    datainit *This = impl_from_IDataInitialize(iface);

    FIXME("(%p)->()\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI datainit_RemoteCreateDBInstanceEx(IDataInitialize *iface, REFCLSID clsidProvider,
                                IUnknown *pUnkOuter, DWORD dwClsCtx, LPWSTR pwszReserved, COSERVERINFO *pServerInfo,
                                DWORD cmq, GUID **rgpIID, IUnknown **rgpItf, HRESULT *rghr)
{
    datainit *This = impl_from_IDataInitialize(iface);

    FIXME("(%p)->()\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI datainit_LoadStringFromStorage(IDataInitialize *iface, LPWSTR pwszFileName,
                                LPWSTR *ppwszInitializationString)
{
    datainit *This = impl_from_IDataInitialize(iface);

    FIXME("(%p)->(%s %p)\n", This, debugstr_w(pwszFileName), ppwszInitializationString);

    return E_NOTIMPL;
}

static HRESULT WINAPI datainit_WriteStringToStorage(IDataInitialize *iface, LPWSTR pwszFileName,
                                LPWSTR pwszInitializationString, DWORD dwCreationDisposition)
{
    datainit *This = impl_from_IDataInitialize(iface);

    FIXME("(%p)->(%s %s %d)\n", This, debugstr_w(pwszFileName), debugstr_w(pwszInitializationString), dwCreationDisposition);

    return E_NOTIMPL;
}


static const struct IDataInitializeVtbl datainit_vtbl =
{
    datainit_QueryInterface,
    datainit_AddRef,
    datainit_Release,
    datainit_GetDataSource,
    datainit_GetInitializationString,
    datainit_CreateDBInstance,
    datainit_RemoteCreateDBInstanceEx,
    datainit_LoadStringFromStorage,
    datainit_WriteStringToStorage
};

HRESULT create_data_init(IUnknown *outer, void **obj)
{
    datainit *This;

    TRACE("(%p)\n", obj);

    if(outer) return CLASS_E_NOAGGREGATION;

    *obj = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->IDataInitialize_iface.lpVtbl = &datainit_vtbl;
    This->ref = 1;

    *obj = &This->IDataInitialize_iface;

    return S_OK;
}
