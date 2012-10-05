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

typedef struct
{
    IDBInitialize IDBInitialize_iface;
    IDBProperties IDBProperties_iface;

    LONG ref;
} dbinit;

static inline dbinit *impl_from_IDBInitialize(IDBInitialize *iface)
{
    return CONTAINING_RECORD(iface, dbinit, IDBInitialize_iface);
}

static inline dbinit *impl_from_IDBProperties(IDBProperties *iface)
{
    return CONTAINING_RECORD(iface, dbinit, IDBProperties_iface);
}

static HRESULT WINAPI dbprops_QueryInterface(IDBProperties *iface, REFIID riid, void **ppvObject)
{
    dbinit *This = impl_from_IDBProperties(iface);

    return IDBInitialize_QueryInterface(&This->IDBInitialize_iface, riid, ppvObject);
}

static ULONG WINAPI dbprops_AddRef(IDBProperties *iface)
{
    dbinit *This = impl_from_IDBProperties(iface);

    return IDBInitialize_AddRef(&This->IDBInitialize_iface);
}

static ULONG WINAPI dbprops_Release(IDBProperties *iface)
{
    dbinit *This = impl_from_IDBProperties(iface);

    return IDBInitialize_Release(&This->IDBInitialize_iface);
}

static HRESULT WINAPI dbprops_GetProperties(IDBProperties *iface, ULONG cPropertyIDSets,
            const DBPROPIDSET rgPropertyIDSets[], ULONG *pcPropertySets, DBPROPSET **prgPropertySets)
{
    dbinit *This = impl_from_IDBProperties(iface);

    FIXME("(%p)->(%d %p %p %p)\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets);

    return E_NOTIMPL;
}

static HRESULT WINAPI dbprops_GetPropertyInfo(IDBProperties *iface, ULONG cPropertyIDSets,
            const DBPROPIDSET rgPropertyIDSets[], ULONG *pcPropertyInfoSets,
            DBPROPINFOSET **prgPropertyInfoSets, OLECHAR **ppDescBuffer)
{
    dbinit *This = impl_from_IDBProperties(iface);

    FIXME("(%p)->(%d %p %p %p %p)\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertyInfoSets,
                prgPropertyInfoSets, ppDescBuffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI dbprops_SetProperties(IDBProperties *iface, ULONG cPropertySets,
            DBPROPSET rgPropertySets[])
{
    dbinit *This = impl_from_IDBProperties(iface);

    FIXME("(%p)->(%d %p)\n", This, cPropertySets, rgPropertySets);

    return E_NOTIMPL;
}

static const struct IDBPropertiesVtbl dbprops_vtbl =
{
    dbprops_QueryInterface,
    dbprops_AddRef,
    dbprops_Release,
    dbprops_GetProperties,
    dbprops_GetPropertyInfo,
    dbprops_SetProperties
};

static HRESULT WINAPI dbinit_QueryInterface(IDBInitialize *iface, REFIID riid, void **obj)
{
    dbinit *This = impl_from_IDBInitialize(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid, &IID_IDBInitialize))
    {
        *obj = iface;
    }
    else if(IsEqualIID(riid, &IID_IDBProperties))
    {
        *obj = &This->IDBProperties_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDBInitialize_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI dbinit_AddRef(IDBInitialize *iface)
{
    dbinit *This = impl_from_IDBInitialize(iface);
    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI dbinit_Release(IDBInitialize *iface)
{
    dbinit *This = impl_from_IDBInitialize(iface);
    LONG ref;

    TRACE("(%p)\n", This);

    ref = InterlockedDecrement(&This->ref);
    if(ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI dbinit_Initialize(IDBInitialize *iface)
{
    dbinit *This = impl_from_IDBInitialize(iface);

    FIXME("(%p) stub\n", This);

    return S_OK;
}

static HRESULT WINAPI dbinit_Uninitialize(IDBInitialize *iface)
{
    dbinit *This = impl_from_IDBInitialize(iface);

    FIXME("(%p) stub\n", This);

    return S_OK;
}

static const IDBInitializeVtbl dbinit_vtbl =
{
    dbinit_QueryInterface,
    dbinit_AddRef,
    dbinit_Release,
    dbinit_Initialize,
    dbinit_Uninitialize
};

static HRESULT create_db_init(void **obj)
{
    dbinit *This;

    TRACE("()\n");

    *obj = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->IDBInitialize_iface.lpVtbl = &dbinit_vtbl;
    This->IDBProperties_iface.lpVtbl = &dbprops_vtbl;
    This->ref = 1;

    *obj = &This->IDBInitialize_iface;

    return S_OK;
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

    if(IsEqualIID(riid, &IID_IDBInitialize))
    {
        return create_db_init( (LPVOID*)ppDataSource);
    }

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
