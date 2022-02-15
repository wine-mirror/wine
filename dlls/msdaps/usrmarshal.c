/*
 *    Misc marshaling routines
 *
 * Copyright 2009 Huw Davies
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
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "objbase.h"
#include "oleauto.h"
#include "oledb.h"

#include "row_server.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

HRESULT CALLBACK IDBCreateCommand_CreateCommand_Proxy(IDBCreateCommand* This, IUnknown *pUnkOuter,
                                                      REFIID riid, IUnknown **ppCommand)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p, %p, %s, %p)\n", This, pUnkOuter, debugstr_guid(riid), ppCommand);
    hr = IDBCreateCommand_RemoteCreateCommand_Proxy(This, pUnkOuter, riid, ppCommand, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }
    return hr;
}

HRESULT __RPC_STUB IDBCreateCommand_CreateCommand_Stub(IDBCreateCommand* This, IUnknown *pUnkOuter,
                                                       REFIID riid, IUnknown **ppCommand, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p, %p, %s, %p, %p)\n", This, pUnkOuter, debugstr_guid(riid), ppCommand, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = IDBCreateCommand_CreateCommand(This, pUnkOuter, riid, ppCommand);
    if(FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IDBCreateSession_CreateSession_Proxy(IDBCreateSession* This, IUnknown *pUnkOuter,
                                                      REFIID riid, IUnknown **ppDBSession)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p, %p, %s, %p)\n", This, pUnkOuter, debugstr_guid(riid), ppDBSession);
    hr = IDBCreateSession_RemoteCreateSession_Proxy(This, pUnkOuter, riid, ppDBSession, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }
    return hr;
}

HRESULT __RPC_STUB IDBCreateSession_CreateSession_Stub(IDBCreateSession* This, IUnknown *pUnkOuter,
                                                       REFIID riid, IUnknown **ppDBSession, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;
    TRACE("(%p, %p, %s, %p, %p)\n", This, pUnkOuter, debugstr_guid(riid),
          ppDBSession, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = IDBCreateSession_CreateSession(This, pUnkOuter, riid, ppDBSession);
    if(FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IDBProperties_GetProperties_Proxy(IDBProperties* This, ULONG cPropertyIDSets, const DBPROPIDSET rgPropertyIDSets[],
                                                   ULONG *pcPropertySets, DBPROPSET **prgPropertySets)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p, %ld, %p, %p, %p)\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertySets,
          prgPropertySets);
    hr = IDBProperties_RemoteGetProperties_Proxy(This, cPropertyIDSets, rgPropertyIDSets,
                                                 pcPropertySets, prgPropertySets, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }
    return hr;
}

HRESULT __RPC_STUB IDBProperties_GetProperties_Stub(IDBProperties* This, ULONG cPropertyIDSets, const DBPROPIDSET *rgPropertyIDSets,
                                                    ULONG *pcPropertySets, DBPROPSET **prgPropertySets, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p, %ld, %p, %p, %p, %p)\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertySets,
          prgPropertySets, ppErrorInfoRem);
    *pcPropertySets = 0;
    *ppErrorInfoRem = NULL;
    hr = IDBProperties_GetProperties(This, cPropertyIDSets, rgPropertyIDSets,
        pcPropertySets, prgPropertySets);
    if(FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IDBProperties_GetPropertyInfo_Proxy(IDBProperties* This, ULONG cPropertyIDSets, const DBPROPIDSET rgPropertyIDSets[],
                                                     ULONG *pcPropertyInfoSets, DBPROPINFOSET **prgPropertyInfoSets,
                                                     OLECHAR **ppDescBuffer)
{
    FIXME("(%p, %ld, %p, %p, %p, %p): stub\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertyInfoSets,
          prgPropertyInfoSets, ppDescBuffer);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDBProperties_GetPropertyInfo_Stub(IDBProperties* This, ULONG cPropertyIDSets, const DBPROPIDSET *rgPropertyIDSets,
                                                      ULONG *pcPropertyInfoSets, DBPROPINFOSET **prgPropertyInfoSets,
                                                      ULONG *pcOffsets, DBBYTEOFFSET **prgDescOffsets, ULONG *pcbDescBuffer,
                                                      OLECHAR **ppDescBuffer, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %ld, %p, %p, %p, %p, %p, %p, %p, %p): stub\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertyInfoSets,
          prgPropertyInfoSets, pcOffsets, prgDescOffsets, pcbDescBuffer, ppDescBuffer, ppErrorInfoRem);
    return E_NOTIMPL;
}

HRESULT CALLBACK IDBProperties_SetProperties_Proxy(IDBProperties* This, ULONG cPropertySets, DBPROPSET rgPropertySets[])
{
    ULONG prop_set, prop, total_props = 0;
    IErrorInfo *error = NULL;
    HRESULT hr;
    DBPROPSTATUS *status;

    TRACE("(%p, %ld, %p)\n", This, cPropertySets, rgPropertySets);

    for(prop_set = 0; prop_set < cPropertySets; prop_set++)
        total_props += rgPropertySets[prop_set].cProperties;

    if(total_props == 0) return S_OK;

    status = CoTaskMemAlloc(total_props * sizeof(*status));
    if(!status) return E_OUTOFMEMORY;

    hr = IDBProperties_RemoteSetProperties_Proxy(This, cPropertySets, rgPropertySets,
                                                 total_props, status, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    total_props = 0;
    for(prop_set = 0; prop_set < cPropertySets; prop_set++)
        for(prop = 0; prop < rgPropertySets[prop_set].cProperties; prop++)
            rgPropertySets[prop_set].rgProperties[prop].dwStatus = status[total_props++];

    CoTaskMemFree(status);
    return hr;
}

HRESULT __RPC_STUB IDBProperties_SetProperties_Stub(IDBProperties* This, ULONG cPropertySets, DBPROPSET *rgPropertySets,
                                                    ULONG cTotalProps, DBPROPSTATUS *rgPropStatus, IErrorInfo **ppErrorInfoRem)
{
    ULONG prop_set, prop, total_props = 0;
    HRESULT hr;

    TRACE("(%p, %ld, %p, %ld, %p, %p)\n", This, cPropertySets, rgPropertySets, cTotalProps,
          rgPropStatus, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = IDBProperties_SetProperties(This, cPropertySets, rgPropertySets);
    if(FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    for(prop_set = 0; prop_set < cPropertySets; prop_set++)
        for(prop = 0; prop < rgPropertySets[prop_set].cProperties; prop++)
            rgPropStatus[total_props++] = rgPropertySets[prop_set].rgProperties[prop].dwStatus;

    return hr;
}

HRESULT CALLBACK IDBInitialize_Initialize_Proxy(IDBInitialize* This)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)\n", This);
    hr = IDBInitialize_RemoteInitialize_Proxy(This, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }
    return hr;
}

HRESULT __RPC_STUB IDBInitialize_Initialize_Stub(IDBInitialize* This, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;
    TRACE("(%p, %p)\n", This, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = IDBInitialize_Initialize(This);
    if(FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IDBInitialize_Uninitialize_Proxy(IDBInitialize* This)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)\n", This);
    hr = IDBInitialize_RemoteUninitialize_Proxy(This, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }
    return hr;
}

HRESULT __RPC_STUB IDBInitialize_Uninitialize_Stub(IDBInitialize* This, IErrorInfo **error)
{
    HRESULT hr;

    TRACE("(%p, %p)\n", This, error);
    *error = NULL;
    hr = IDBInitialize_Uninitialize(This);
    if(FAILED(hr)) GetErrorInfo(0, error);

    return hr;
}

HRESULT CALLBACK IDBDataSourceAdmin_CreateDataSource_Proxy(IDBDataSourceAdmin* This, ULONG cPropertySets,
                                                           DBPROPSET rgPropertySets[], IUnknown *pUnkOuter,
                                                           REFIID riid, IUnknown **ppDBSession)
{
    ULONG prop_set, prop, total_props = 0;
    IErrorInfo *error = NULL;
    HRESULT hr;
    DBPROPSTATUS *status;

    TRACE("(%p, %ld, %p, %p, %s, %p)\n", This, cPropertySets, rgPropertySets, pUnkOuter,
          debugstr_guid(riid), ppDBSession);

    for(prop_set = 0; prop_set < cPropertySets; prop_set++)
        total_props += rgPropertySets[prop_set].cProperties;

    if(total_props == 0) return S_OK;

    status = CoTaskMemAlloc(total_props * sizeof(*status));
    if(!status) return E_OUTOFMEMORY;

    hr = IDBDataSourceAdmin_RemoteCreateDataSource_Proxy(This, cPropertySets, rgPropertySets, pUnkOuter,
                                                         riid, ppDBSession, total_props, status, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    total_props = 0;
    for(prop_set = 0; prop_set < cPropertySets; prop_set++)
        for(prop = 0; prop < rgPropertySets[prop_set].cProperties; prop++)
            rgPropertySets[prop_set].rgProperties[prop].dwStatus = status[total_props++];

    CoTaskMemFree(status);
    return hr;
}

HRESULT __RPC_STUB IDBDataSourceAdmin_CreateDataSource_Stub(IDBDataSourceAdmin* This, ULONG cPropertySets,
                                                            DBPROPSET *rgPropertySets, IUnknown *pUnkOuter,
                                                            REFIID riid, IUnknown **ppDBSession, ULONG cTotalProps,
                                                            DBPROPSTATUS *rgPropStatus, IErrorInfo **ppErrorInfoRem)
{
    ULONG prop_set, prop, total_props = 0;
    HRESULT hr;

    TRACE("(%p, %ld, %p, %p, %s, %p, %ld, %p, %p)\n", This, cPropertySets, rgPropertySets, pUnkOuter,
          debugstr_guid(riid), ppDBSession, cTotalProps, rgPropStatus, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = IDBDataSourceAdmin_CreateDataSource(This, cPropertySets, rgPropertySets, pUnkOuter, riid, ppDBSession);
    if(FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    for(prop_set = 0; prop_set < cPropertySets; prop_set++)
        for(prop = 0; prop < rgPropertySets[prop_set].cProperties; prop++)
            rgPropStatus[total_props++] = rgPropertySets[prop_set].rgProperties[prop].dwStatus;

    return hr;
}

HRESULT CALLBACK IDBDataSourceAdmin_DestroyDataSource_Proxy(IDBDataSourceAdmin* This)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)\n", This);
    hr = IDBDataSourceAdmin_RemoteDestroyDataSource_Proxy(This, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }
    return hr;
}

HRESULT __RPC_STUB IDBDataSourceAdmin_DestroyDataSource_Stub(IDBDataSourceAdmin* This, IErrorInfo **error)
{
    HRESULT hr;

    TRACE("(%p, %p)\n", This, error);
    *error = NULL;
    hr = IDBDataSourceAdmin_DestroyDataSource(This);
    if(FAILED(hr)) GetErrorInfo(0, error);

    return hr;
}

HRESULT CALLBACK IDBDataSourceAdmin_GetCreationProperties_Proxy(IDBDataSourceAdmin* This, ULONG cPropertyIDSets,
                                                                const DBPROPIDSET rgPropertyIDSets[], ULONG *pcPropertyInfoSets,
                                                                DBPROPINFOSET **prgPropertyInfoSets, OLECHAR **ppDescBuffer)
{
    FIXME("(%p, %ld, %p, %p, %p, %p): stub\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertyInfoSets,
          prgPropertyInfoSets, ppDescBuffer);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDBDataSourceAdmin_GetCreationProperties_Stub(IDBDataSourceAdmin* This, ULONG cPropertyIDSets,
                                                                 const DBPROPIDSET *rgPropertyIDSets, ULONG *pcPropertyInfoSets,
                                                                 DBPROPINFOSET **prgPropertyInfoSets, DBCOUNTITEM *pcOffsets,
                                                                 DBBYTEOFFSET **prgDescOffsets, ULONG *pcbDescBuffer,
                                                                 OLECHAR **ppDescBuffer, IErrorInfo **error)
{
    HRESULT hr;

    TRACE("(%p, %ld, %p, %p, %p, %p, %p, %p, %p, %p)\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertyInfoSets,
          prgPropertyInfoSets, pcOffsets, prgDescOffsets, pcbDescBuffer, ppDescBuffer, error);
    *error = NULL;
    hr = IDBDataSourceAdmin_GetCreationProperties(This, cPropertyIDSets, rgPropertyIDSets, pcPropertyInfoSets,
        prgPropertyInfoSets, ppDescBuffer);
    if(FAILED(hr)) GetErrorInfo(0, error);

    return hr;
}

HRESULT CALLBACK IDBDataSourceAdmin_ModifyDataSource_Proxy(IDBDataSourceAdmin* This, ULONG cPropertySets, DBPROPSET rgPropertySets[])
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p, %ld, %p)\n", This, cPropertySets, rgPropertySets);
    hr = IDBDataSourceAdmin_RemoteModifyDataSource_Proxy(This, cPropertySets, rgPropertySets, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    return hr;
}

HRESULT __RPC_STUB IDBDataSourceAdmin_ModifyDataSource_Stub(IDBDataSourceAdmin* This, ULONG cPropertySets,
                                                            DBPROPSET *rgPropertySets, IErrorInfo **error)
{
    HRESULT hr;

    TRACE("(%p, %ld, %p, %p)\n", This, cPropertySets, rgPropertySets, error);
    *error = NULL;
    hr = IDBDataSourceAdmin_ModifyDataSource(This, cPropertySets, rgPropertySets);
    if(FAILED(hr)) GetErrorInfo(0, error);

    return hr;
}

HRESULT CALLBACK ISessionProperties_GetProperties_Proxy(ISessionProperties* This, ULONG cPropertyIDSets,
                                                        const DBPROPIDSET rgPropertyIDSets[], ULONG *pcPropertySets,
                                                        DBPROPSET **prgPropertySets)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p, %ld, %p, %p, %p)\n", This, cPropertyIDSets, rgPropertyIDSets,
          pcPropertySets, prgPropertySets);

    hr = ISessionProperties_RemoteGetProperties_Proxy(This, cPropertyIDSets, rgPropertyIDSets, pcPropertySets,
        prgPropertySets, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    return hr;
}

HRESULT __RPC_STUB ISessionProperties_GetProperties_Stub(ISessionProperties* This, ULONG cPropertyIDSets,
                                                         const DBPROPIDSET *rgPropertyIDSets, ULONG *pcPropertySets,
                                                         DBPROPSET **prgPropertySets, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p, %ld, %p, %p, %p, %p)\n", This, cPropertyIDSets, rgPropertyIDSets,
          pcPropertySets, prgPropertySets, ppErrorInfoRem);

    hr = ISessionProperties_GetProperties(This, cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets);
    if (FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK ISessionProperties_SetProperties_Proxy(ISessionProperties* This, ULONG cPropertySets, DBPROPSET rgPropertySets[])
{
    ULONG prop_set, prop, total_props = 0;
    IErrorInfo *error = NULL;
    HRESULT hr;
    DBPROPSTATUS *status;

    TRACE("(%p, %ld, %p)\n", This, cPropertySets, rgPropertySets);

    for(prop_set = 0; prop_set < cPropertySets; prop_set++)
        total_props += rgPropertySets[prop_set].cProperties;

    if(total_props == 0) return S_OK;

    status = CoTaskMemAlloc(total_props * sizeof(*status));
    if(!status) return E_OUTOFMEMORY;

    hr = ISessionProperties_RemoteSetProperties_Proxy(This, cPropertySets, rgPropertySets,
                                                      total_props, status, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    total_props = 0;
    for(prop_set = 0; prop_set < cPropertySets; prop_set++)
        for(prop = 0; prop < rgPropertySets[prop_set].cProperties; prop++)
            rgPropertySets[prop_set].rgProperties[prop].dwStatus = status[total_props++];

    CoTaskMemFree(status);
    return hr;
}

HRESULT __RPC_STUB ISessionProperties_SetProperties_Stub(ISessionProperties* This, ULONG cPropertySets, DBPROPSET *rgPropertySets,
                                                         ULONG cTotalProps, DBPROPSTATUS *rgPropStatus, IErrorInfo **ppErrorInfoRem)
{
    ULONG prop_set, prop, total_props = 0;
    HRESULT hr;

    TRACE("(%p, %ld, %p, %ld, %p, %p)\n", This, cPropertySets, rgPropertySets, cTotalProps,
          rgPropStatus, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = ISessionProperties_SetProperties(This, cPropertySets, rgPropertySets);
    if(FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    for(prop_set = 0; prop_set < cPropertySets; prop_set++)
        for(prop = 0; prop < rgPropertySets[prop_set].cProperties; prop++)
            rgPropStatus[total_props++] = rgPropertySets[prop_set].rgProperties[prop].dwStatus;

    return hr;
}

HRESULT CALLBACK IOpenRowset_OpenRowset_Proxy(IOpenRowset* This, IUnknown *pUnkOuter, DBID *pTableID, DBID *pIndexID,
                                              REFIID riid, ULONG cPropertySets, DBPROPSET rgPropertySets[], IUnknown **ppRowset)
{
    FIXME("(%p, %p, %p, %p, %s, %ld, %p, %p): stub\n", This, pUnkOuter, pTableID, pIndexID, debugstr_guid(riid),
          cPropertySets, rgPropertySets, ppRowset);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IOpenRowset_OpenRowset_Stub(IOpenRowset* This, IUnknown *pUnkOuter, DBID *pTableID, DBID *pIndexID,
                                               REFIID riid, ULONG cPropertySets, DBPROPSET *rgPropertySets,
                                               IUnknown **ppRowset, ULONG cTotalProps, DBPROPSTATUS *rgPropStatus,
                                               IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %p, %p, %p, %s, %ld, %p, %p, %ld, %p, %p): stub\n", This, pUnkOuter, pTableID, pIndexID, debugstr_guid(riid),
          cPropertySets, rgPropertySets, ppRowset, cTotalProps, rgPropStatus, ppErrorInfoRem);
    return E_NOTIMPL;
}

HRESULT CALLBACK IBindResource_Bind_Proxy(IBindResource* This, IUnknown *pUnkOuter, LPCOLESTR pwszURL, DBBINDURLFLAG dwBindURLFlags,
                                          REFGUID rguid, REFIID riid, IAuthenticate *pAuthenticate, DBIMPLICITSESSION *pImplSession,
                                          DBBINDURLSTATUS *pdwBindStatus, IUnknown **ppUnk)
{
    HRESULT hr;

    TRACE("(%p, %p, %s, %08lx, %s, %s, %p, %p, %p, %p)\n", This, pUnkOuter, debugstr_w(pwszURL), dwBindURLFlags,
          debugstr_guid(rguid), debugstr_guid(riid), pAuthenticate, pImplSession, pdwBindStatus, ppUnk);

    if(pUnkOuter)
    {
        FIXME("Aggregation not supported\n");
        return CLASS_E_NOAGGREGATION;
    }

    hr = IBindResource_RemoteBind_Proxy(This, pUnkOuter, pwszURL, dwBindURLFlags, rguid, riid, pAuthenticate,
                                        pImplSession ? pImplSession->pUnkOuter : NULL, pImplSession ? pImplSession->piid : NULL,
                                        pImplSession ? &pImplSession->pSession : NULL, pdwBindStatus, ppUnk);
    return hr;
}

HRESULT __RPC_STUB IBindResource_Bind_Stub(IBindResource* This, IUnknown *pUnkOuter, LPCOLESTR pwszURL, DBBINDURLFLAG dwBindURLFlags,
                                           REFGUID rguid, REFIID riid, IAuthenticate *pAuthenticate, IUnknown *pSessionUnkOuter,
                                           IID *piid, IUnknown **ppSession, DBBINDURLSTATUS *pdwBindStatus, IUnknown **ppUnk)
{
    HRESULT hr;
    DBIMPLICITSESSION impl_session;
    IWineRowServer *server;
    IMarshal *marshal;
    IUnknown *obj;

    TRACE("(%p, %p, %s, %08lx, %s, %s, %p, %p, %p, %p, %p, %p)\n", This, pUnkOuter, debugstr_w(pwszURL), dwBindURLFlags,
          debugstr_guid(rguid), debugstr_guid(riid), pAuthenticate, pSessionUnkOuter, piid, ppSession, pdwBindStatus, ppUnk);

    *ppUnk = NULL;

    if(IsEqualGUID(rguid, &DBGUID_ROWSET))
        hr = CoCreateInstance(&CLSID_wine_rowset_server, NULL, CLSCTX_INPROC_SERVER, &IID_IWineRowServer, (void**)&server);
    else if(IsEqualGUID(rguid, &DBGUID_ROW))
        hr = CoCreateInstance(&CLSID_wine_row_server, NULL, CLSCTX_INPROC_SERVER, &IID_IWineRowServer, (void**)&server);
    else
    {
        hr = E_NOTIMPL;
        FIXME("Unhandled object %s\n", debugstr_guid(rguid));
    }

    if(FAILED(hr)) return hr;

    impl_session.pUnkOuter = pSessionUnkOuter;
    impl_session.piid = piid;
    impl_session.pSession = NULL;

    IWineRowServer_GetMarshal(server, &marshal);

    hr = IBindResource_Bind(This, (IUnknown*)marshal, pwszURL, dwBindURLFlags, rguid, &IID_IUnknown, pAuthenticate,
                            ppSession ? &impl_session : NULL, pdwBindStatus, &obj);

    IMarshal_Release(marshal);
    if(FAILED(hr))
    {
        IWineRowServer_Release(server);
        return hr;
    }

    IWineRowServer_SetInnerUnk(server, obj);
    hr = IUnknown_QueryInterface(obj, riid, (void**)ppUnk);
    IUnknown_Release(obj);

    if(ppSession) *ppSession = impl_session.pSession;
    return hr;
}

HRESULT CALLBACK ICreateRow_CreateRow_Proxy(ICreateRow* This, IUnknown *pUnkOuter, LPCOLESTR pwszURL, DBBINDURLFLAG dwBindURLFlags,
                                            REFGUID rguid, REFIID riid, IAuthenticate *pAuthenticate, DBIMPLICITSESSION *pImplSession,
                                            DBBINDURLSTATUS *pdwBindStatus, LPOLESTR *ppwszNewURL, IUnknown **ppUnk)
{
    HRESULT hr;

    TRACE("(%p, %p, %s, %08lx, %s, %s, %p, %p, %p, %p, %p)\n", This, pUnkOuter, debugstr_w(pwszURL), dwBindURLFlags,
          debugstr_guid(rguid), debugstr_guid(riid), pAuthenticate, pImplSession, pdwBindStatus, ppwszNewURL, ppUnk);

    if(pUnkOuter)
    {
        FIXME("Aggregation not supported\n");
        return CLASS_E_NOAGGREGATION;
    }

    hr = ICreateRow_RemoteCreateRow_Proxy(This, pUnkOuter, pwszURL, dwBindURLFlags, rguid, riid, pAuthenticate,
                                          pImplSession ? pImplSession->pUnkOuter : NULL, pImplSession ? pImplSession->piid : NULL,
                                          pImplSession ? &pImplSession->pSession : NULL, pdwBindStatus, ppwszNewURL, ppUnk);
    return hr;
}

HRESULT __RPC_STUB ICreateRow_CreateRow_Stub(ICreateRow* This, IUnknown *pUnkOuter, LPCOLESTR pwszURL, DBBINDURLFLAG dwBindURLFlags,
                                             REFGUID rguid, REFIID riid, IAuthenticate *pAuthenticate, IUnknown *pSessionUnkOuter,
                                             IID *piid, IUnknown **ppSession, DBBINDURLSTATUS *pdwBindStatus,
                                             LPOLESTR *ppwszNewURL, IUnknown **ppUnk)
{
    HRESULT hr;
    DBIMPLICITSESSION impl_session;
    IWineRowServer *row_server;
    IMarshal *marshal;
    IUnknown *obj;

    TRACE("(%p, %p, %s, %08lx, %s, %s, %p, %p, %p, %p, %p, %p, %p)\n", This, pUnkOuter, debugstr_w(pwszURL), dwBindURLFlags,
          debugstr_guid(rguid), debugstr_guid(riid), pAuthenticate, pSessionUnkOuter, piid, ppSession, pdwBindStatus, ppwszNewURL,
          ppUnk);

    *ppUnk = NULL;

    hr = CoCreateInstance(&CLSID_wine_row_server, NULL, CLSCTX_INPROC_SERVER, &IID_IWineRowServer, (void**)&row_server);
    if(FAILED(hr)) return hr;

    impl_session.pUnkOuter = pSessionUnkOuter;
    impl_session.piid = piid;
    impl_session.pSession = NULL;

    IWineRowServer_GetMarshal(row_server, &marshal);

    hr = ICreateRow_CreateRow(This, (IUnknown*) marshal, pwszURL, dwBindURLFlags, rguid, &IID_IUnknown, pAuthenticate,
                              ppSession ? &impl_session : NULL, pdwBindStatus, ppwszNewURL, &obj);
    IMarshal_Release(marshal);

    if(FAILED(hr))
    {
        IWineRowServer_Release(row_server);
        return hr;
    }

    IWineRowServer_SetInnerUnk(row_server, obj);
    hr = IUnknown_QueryInterface(obj, riid, (void**)ppUnk);
    IUnknown_Release(obj);

    if(ppSession) *ppSession = impl_session.pSession;
    return hr;
}

HRESULT CALLBACK IAccessor_AddRefAccessor_Proxy(IAccessor* This, HACCESSOR hAccessor, DBREFCOUNT *refcount)
{
    IErrorInfo *errorinfo;
    DBREFCOUNT ref;
    HRESULT hr;

    TRACE("(%p)->(%08Ix, %p)\n", This, hAccessor, refcount);

    if (!refcount)
        refcount = &ref;

    errorinfo = NULL;
    hr = IAccessor_RemoteAddRefAccessor_Proxy(This, hAccessor, refcount, &errorinfo);
    if (errorinfo)
    {
        SetErrorInfo(0, errorinfo);
        IErrorInfo_Release(errorinfo);
    }

    return hr;
}

HRESULT __RPC_STUB IAccessor_AddRefAccessor_Stub(IAccessor* This, HACCESSOR hAccessor, DBREFCOUNT *pcRefCount,
                                                 IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->(%08Ix, %p, %p)\n", This, hAccessor, pcRefCount, ppErrorInfoRem);

    hr = IAccessor_AddRefAccessor(This, hAccessor, pcRefCount);
    if (FAILED(hr))
        GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IAccessor_CreateAccessor_Proxy(IAccessor* This, DBACCESSORFLAGS dwAccessorFlags, DBCOUNTITEM cBindings,
                                                const DBBINDING rgBindings[], DBLENGTH cbRowSize, HACCESSOR *phAccessor,
                                                DBBINDSTATUS rgStatus[])
{
    IErrorInfo *error = NULL;
    HRESULT hr;
    DBCOUNTITEM i;

    TRACE("(%p)->(%08lx, %Id, %p, %Id, %p, %p)\n", This, dwAccessorFlags, cBindings, rgBindings,
          cbRowSize, phAccessor, rgStatus);

    for(i = 0; i < cBindings; i++)
    {
        TRACE("%Id: ord %Id val off %Id len off %Id stat off %Id part %04lx mem_owner %ld max_len %Id type %04x\n",
              i, rgBindings[i].iOrdinal, rgBindings[i].obValue, rgBindings[i].obLength, rgBindings[i].obStatus,
              rgBindings[i].dwPart, rgBindings[i].dwMemOwner, rgBindings[i].cbMaxLen, rgBindings[i].wType);
    }

    hr = IAccessor_RemoteCreateAccessor_Proxy(This, dwAccessorFlags, cBindings, (DBBINDING *)rgBindings,
                                              cbRowSize, phAccessor, rgStatus, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    TRACE("returning %08lx accessor %Ix\n", hr, *phAccessor);
    return hr;
}

HRESULT __RPC_STUB IAccessor_CreateAccessor_Stub(IAccessor* This, DBACCESSORFLAGS dwAccessorFlags, DBCOUNTITEM cBindings,
                                                 DBBINDING *rgBindings, DBLENGTH cbRowSize, HACCESSOR *phAccessor,
                                                 DBBINDSTATUS *rgStatus, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->(%08lx, %Id, %p, %Id, %p, %p, %p)\n", This, dwAccessorFlags, cBindings, rgBindings,
          cbRowSize, phAccessor, rgStatus, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = IAccessor_CreateAccessor(This, dwAccessorFlags, cBindings, rgBindings,
                                  cbRowSize, phAccessor, rgStatus);
    if(FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IAccessor_GetBindings_Proxy(IAccessor* This, HACCESSOR hAccessor, DBACCESSORFLAGS *pdwAccessorFlags,
                                             DBCOUNTITEM *pcBindings, DBBINDING **prgBindings)
{
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IAccessor_GetBindings_Stub(IAccessor* This, HACCESSOR hAccessor, DBACCESSORFLAGS *pdwAccessorFlags,
                                              DBCOUNTITEM *pcBindings, DBBINDING **prgBindings, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

HRESULT CALLBACK IAccessor_ReleaseAccessor_Proxy(IAccessor* This, HACCESSOR hAccessor, DBREFCOUNT *pcRefCount)
{
    IErrorInfo *error = NULL;
    HRESULT hr;
    DBREFCOUNT ref;

    TRACE("(%p)->(%Ix, %p)\n", This, hAccessor, pcRefCount);

    hr = IAccessor_RemoteReleaseAccessor_Proxy(This, hAccessor, &ref, &error);

    if(pcRefCount) *pcRefCount = ref;
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }
    return hr;
}

HRESULT __RPC_STUB IAccessor_ReleaseAccessor_Stub(IAccessor* This, HACCESSOR hAccessor, DBREFCOUNT *pcRefCount,
                                                  IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->(%Ix, %p, %p)\n", This, hAccessor, pcRefCount, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;

    hr = IAccessor_ReleaseAccessor(This, hAccessor, pcRefCount);

    if(FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);
    return hr;
}

HRESULT CALLBACK IRowsetInfo_GetProperties_Proxy(IRowsetInfo* This, const ULONG cPropertyIDSets, const DBPROPIDSET rgPropertyIDSets[],
                                                 ULONG *pcPropertySets, DBPROPSET **prgPropertySets)
{
    IErrorInfo *error = NULL;
    HRESULT hr;
    ULONG i;

    TRACE("(%p)->(%ld, %p, %p, %p)\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets);

    for(i = 0; i < cPropertyIDSets; i++)
    {
        unsigned int j;
        TRACE("%ld: %s %ld props\n", i, debugstr_guid(&rgPropertyIDSets[i].guidPropertySet), rgPropertyIDSets[i].cPropertyIDs);
        for(j = 0; j < rgPropertyIDSets[i].cPropertyIDs; j++)
            TRACE("\t%u: prop id %ld\n", j, rgPropertyIDSets[i].rgPropertyIDs[j]);
    }

    hr = IRowsetInfo_RemoteGetProperties_Proxy(This, cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets, &error);

    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }
    return hr;
}

HRESULT __RPC_STUB IRowsetInfo_GetProperties_Stub(IRowsetInfo* This, ULONG cPropertyIDSets, const DBPROPIDSET *rgPropertyIDSets,
                                                  ULONG *pcPropertySets, DBPROPSET **prgPropertySets, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->(%ld, %p, %p, %p, %p)\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;

    hr = IRowsetInfo_GetProperties(This, cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets);
    if(FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);
    TRACE("returning %08lx\n", hr);
    return hr;
}

HRESULT CALLBACK IRowsetInfo_GetReferencedRowset_Proxy(IRowsetInfo* This, DBORDINAL iOrdinal, REFIID riid, IUnknown **ppReferencedRowset)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)->(%Id, %s, %p)\n", This, iOrdinal, debugstr_guid(riid), ppReferencedRowset);

    hr = IRowsetInfo_RemoteGetReferencedRowset_Proxy(This, iOrdinal, riid, ppReferencedRowset, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    return hr;
}

HRESULT __RPC_STUB IRowsetInfo_GetReferencedRowset_Stub(IRowsetInfo* This, DBORDINAL iOrdinal, REFIID riid, IUnknown **ppReferencedRowset,
                                                        IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->(%Id, %s, %p, %p)\n", This, iOrdinal, debugstr_guid(riid), ppReferencedRowset, ppErrorInfoRem);

    hr = IRowsetInfo_GetReferencedRowset(This, iOrdinal, riid, ppReferencedRowset);
    if (FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IRowsetInfo_GetSpecification_Proxy(IRowsetInfo* This, REFIID riid, IUnknown **ppSpecification)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppSpecification);

    hr = IRowsetInfo_RemoteGetSpecification_Proxy(This, riid, ppSpecification, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    return hr;
}

HRESULT __RPC_STUB IRowsetInfo_GetSpecification_Stub(IRowsetInfo* This, REFIID riid, IUnknown **ppSpecification, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->(%s, %p, %p)\n", This, debugstr_guid(riid), ppSpecification, ppErrorInfoRem);

    hr = IRowsetInfo_GetSpecification(This, riid, ppSpecification);
    if (FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK ICommand_Cancel_Proxy(ICommand* This)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)\n", This);

    hr = ICommand_RemoteCancel_Proxy(This, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    return hr;
}

HRESULT __RPC_STUB ICommand_Cancel_Stub(ICommand* This, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, ppErrorInfoRem);

    hr = ICommand_Cancel(This);
    if (FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK ICommand_Execute_Proxy(ICommand* This, IUnknown *pUnkOuter, REFIID riid,
                                        DBPARAMS *pParams, DBROWCOUNT *pcRowsAffected, IUnknown **ppRowset)
{
    HRESULT hr;
    DBROWCOUNT affected;

    *ppRowset = NULL;

    TRACE("(%p)->(%p, %s, %p, %p, %p)\n", This, pUnkOuter, debugstr_guid(riid), pParams,
          pcRowsAffected, ppRowset);

    if(pParams)
    {
        FIXME("Unhandled params {%p, %Id, %08Ix}\n", pParams->pData, pParams->cParamSets, pParams->hAccessor);
        return E_NOTIMPL;
    }

    if(pUnkOuter)
    {
        FIXME("Aggregation not supported\n");
        return CLASS_E_NOAGGREGATION;
    }

    hr = ICommand_RemoteExecute_Proxy(This, pUnkOuter, riid, 0, 0, NULL, 0, NULL, NULL, 0, NULL, NULL, &affected,
                                      ppRowset);

    TRACE("Execute returns %08lx\n", hr);

    if(pcRowsAffected) *pcRowsAffected = affected;

    return hr;
}

HRESULT __RPC_STUB ICommand_Execute_Stub(ICommand* This, IUnknown *pUnkOuter, REFIID riid, HACCESSOR hAccessor,
                                         DB_UPARAMS cParamSets, GUID *pGuid, ULONG ulGuidOffset, RMTPACK *pInputParams,
                                         RMTPACK *pOutputParams, DBCOUNTITEM cBindings, DBBINDING *rgBindings,
                                         DBSTATUS *rgStatus, DBROWCOUNT *pcRowsAffected, IUnknown **ppRowset)
{
    IWineRowServer *rowset_server;
    IMarshal *marshal;
    IUnknown *obj = NULL;
    HRESULT hr;

    TRACE("(%p)->(%p, %s, %08Ix, %Id, %p, %ld, %p, %p, %Id, %p, %p, %p, %p)\n", This, pUnkOuter, debugstr_guid(riid),
          hAccessor, cParamSets, pGuid, ulGuidOffset, pInputParams, pOutputParams, cBindings, rgBindings, rgStatus,
          pcRowsAffected, ppRowset);

    *ppRowset = NULL;

    hr = CoCreateInstance(&CLSID_wine_rowset_server, NULL, CLSCTX_INPROC_SERVER, &IID_IWineRowServer, (void**)&rowset_server);
    if(FAILED(hr)) return hr;

    IWineRowServer_GetMarshal(rowset_server, &marshal);

    hr = ICommand_Execute(This, (IUnknown*)marshal, &IID_IUnknown, NULL, pcRowsAffected, &obj);

    IMarshal_Release(marshal);

    if(FAILED(hr))
    {
        IWineRowServer_Release(rowset_server);
        return hr;
    }

    IWineRowServer_SetInnerUnk(rowset_server, obj);
    hr = IUnknown_QueryInterface(obj, riid, (void**)ppRowset);
    IUnknown_Release(obj);

    return hr;
}

HRESULT CALLBACK ICommand_GetDBSession_Proxy(ICommand* This, REFIID riid, IUnknown **ppSession)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppSession);

    hr = ICommand_RemoteGetDBSession_Proxy(This, riid, ppSession, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    return hr;
}

HRESULT __RPC_STUB ICommand_GetDBSession_Stub(ICommand* This, REFIID riid, IUnknown **ppSession, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->(%s, %p, %p)\n", This, debugstr_guid(riid), ppSession, ppErrorInfoRem);

    hr = ICommand_GetDBSession(This, riid, ppSession);
    if (FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK ICommandText_GetCommandText_Proxy(ICommandText* This, GUID *pguidDialect, LPOLESTR *ppwszCommand)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)->(%p, %p)\n", This, pguidDialect, ppwszCommand);

    hr = ICommandText_RemoteGetCommandText_Proxy(This, pguidDialect, ppwszCommand, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    return hr;
}

HRESULT __RPC_STUB ICommandText_GetCommandText_Stub(ICommandText* This, GUID *pguidDialect,
                                                    LPOLESTR *ppwszCommand, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->(%p, %p, %p)\n", This, pguidDialect, ppwszCommand, ppErrorInfoRem);

    hr = ICommandText_GetCommandText(This, pguidDialect, ppwszCommand);
    if (FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK ICommandText_SetCommandText_Proxy(ICommandText* This, REFGUID rguidDialect, LPCOLESTR pwszCommand)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)->(%s, %s)\n", This, debugstr_guid(rguidDialect), debugstr_w(pwszCommand));

    hr = ICommandText_RemoteSetCommandText_Proxy(This, rguidDialect, pwszCommand, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }
    return hr;
}

HRESULT __RPC_STUB ICommandText_SetCommandText_Stub(ICommandText* This, REFGUID rguidDialect, LPCOLESTR pwszCommand,
                                                    IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->(%s, %s, %p)\n", This, debugstr_guid(rguidDialect), debugstr_w(pwszCommand),
          ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = ICommandText_SetCommandText(This, rguidDialect, pwszCommand);
    if(FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IDBAsynchNotify_OnLowResource_Proxy(IDBAsynchNotify* This, DB_DWRESERVE dwReserved)
{
    TRACE("(%p)->(%08Ix)\n", This, dwReserved);
    return IDBAsynchNotify_RemoteOnLowResource_Proxy(This, dwReserved);
}

HRESULT __RPC_STUB IDBAsynchNotify_OnLowResource_Stub(IDBAsynchNotify* This, DB_DWRESERVE dwReserved)
{
    TRACE("(%p)->(%08Ix)\n", This, dwReserved);
    return IDBAsynchNotify_OnLowResource(This, dwReserved);
}

HRESULT CALLBACK IDBAsynchNotify_OnProgress_Proxy(IDBAsynchNotify* This, HCHAPTER hChapter, DBASYNCHOP eOperation,
                                                  DBCOUNTITEM ulProgress, DBCOUNTITEM ulProgressMax, DBASYNCHPHASE eAsynchPhase,
                                                  LPOLESTR pwszStatusText)
{
    TRACE("(%p)->(%Ix, %ld, %Id, %Id, %ld, %s)\n", This, hChapter, eOperation, ulProgress, ulProgressMax,
          eAsynchPhase, debugstr_w(pwszStatusText));

    return IDBAsynchNotify_RemoteOnProgress_Proxy(This, hChapter, eOperation, ulProgress, ulProgressMax, eAsynchPhase,
                                                  pwszStatusText);
}

HRESULT __RPC_STUB IDBAsynchNotify_OnProgress_Stub(IDBAsynchNotify* This, HCHAPTER hChapter, DBASYNCHOP eOperation,
                                                   DBCOUNTITEM ulProgress, DBCOUNTITEM ulProgressMax, DBASYNCHPHASE eAsynchPhase,
                                                   LPOLESTR pwszStatusText)
{
    TRACE("(%p)->(%Ix, %ld, %Id, %Id, %ld, %s)\n", This, hChapter, eOperation, ulProgress, ulProgressMax,
          eAsynchPhase, debugstr_w(pwszStatusText));
    return IDBAsynchNotify_OnProgress(This, hChapter, eOperation, ulProgress, ulProgressMax, eAsynchPhase,
                                      pwszStatusText);
}

HRESULT CALLBACK IDBAsynchNotify_OnStop_Proxy(IDBAsynchNotify* This, HCHAPTER hChapter, DBASYNCHOP eOperation,
                                              HRESULT hrStatus, LPOLESTR pwszStatusText)
{
    TRACE("(%p)->(%Ix, %ld, %08lx, %s)\n", This, hChapter, eOperation, hrStatus, debugstr_w(pwszStatusText));
    return IDBAsynchNotify_RemoteOnStop_Proxy(This, hChapter, eOperation, hrStatus, pwszStatusText);
}

HRESULT __RPC_STUB IDBAsynchNotify_OnStop_Stub(IDBAsynchNotify* This, HCHAPTER hChapter, DBASYNCHOP eOperation,
                                               HRESULT hrStatus, LPOLESTR pwszStatusText)
{
    TRACE("(%p)->(%Ix, %ld, %08lx, %s)\n", This, hChapter, eOperation, hrStatus, debugstr_w(pwszStatusText));
    return IDBAsynchNotify_OnStop(This, hChapter, eOperation, hrStatus, pwszStatusText);
}

HRESULT CALLBACK IDBAsynchStatus_Abort_Proxy(IDBAsynchStatus* This, HCHAPTER hChapter, DBASYNCHOP eOperation)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)->(%Ix, %ld)\n", This, hChapter, eOperation);

    hr = IDBAsynchStatus_RemoteAbort_Proxy(This, hChapter, eOperation, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }
    return hr;
}

HRESULT __RPC_STUB IDBAsynchStatus_Abort_Stub(IDBAsynchStatus* This, HCHAPTER hChapter, DBASYNCHOP eOperation,
                                              IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->(%Ix, %ld, %p)\n", This, hChapter, eOperation, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = IDBAsynchStatus_Abort(This, hChapter, eOperation);
    if(FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IDBAsynchStatus_GetStatus_Proxy(IDBAsynchStatus* This, HCHAPTER hChapter, DBASYNCHOP eOperation,
                                                 DBCOUNTITEM *pulProgress, DBCOUNTITEM *pulProgressMax, DBASYNCHPHASE *peAsynchPhase,
                                                 LPOLESTR *ppwszStatusText)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)->(%Ix, %ld, %p, %p, %p, %p)\n", This, hChapter, eOperation, pulProgress, pulProgressMax,
          peAsynchPhase, ppwszStatusText);

    hr = IDBAsynchStatus_RemoteGetStatus_Proxy(This, hChapter, eOperation, pulProgress, pulProgressMax, peAsynchPhase,
        ppwszStatusText, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }
    return hr;
}

HRESULT __RPC_STUB IDBAsynchStatus_GetStatus_Stub(IDBAsynchStatus* This, HCHAPTER hChapter, DBASYNCHOP eOperation,
                                                  DBCOUNTITEM *pulProgress, DBCOUNTITEM *pulProgressMax, DBASYNCHPHASE *peAsynchPhase,
                                                  LPOLESTR *ppwszStatusText, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->(%Ix, %ld, %p, %p, %p, %p, %p)\n", This, hChapter, eOperation, pulProgress, pulProgressMax,
          peAsynchPhase, ppwszStatusText, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = IDBAsynchStatus_GetStatus(This, hChapter, eOperation, pulProgress, pulProgressMax, peAsynchPhase, ppwszStatusText);
    if(FAILED(hr)) GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IRowsetNotify_OnRowChange_Proxy(IRowsetNotify* This, IRowset *rowset, DBCOUNTITEM rows, HROW *hrows, DBREASON reason,
                                                 DBEVENTPHASE phase, BOOL cantdeny)
{
    TRACE("(%p)->(%p %Id %p %ld %ld %d)\n", This, rowset, rows, hrows, reason, phase, cantdeny);
    return IRowsetNotify_RemoteOnRowChange_Proxy(This, rowset, rows, hrows, reason, phase, cantdeny);
}

HRESULT __RPC_STUB IRowsetNotify_OnRowChange_Stub(IRowsetNotify* This, IRowset *rowset, DBCOUNTITEM rows, HROW *hrows, DBREASON reason,
                                                  DBEVENTPHASE phase, BOOL cantdeny)
{
    TRACE("(%p)->(%p %Id %p %ld %ld %d)\n", This, rowset, rows, hrows, reason, phase, cantdeny);
    return IRowsetNotify_OnRowChange(This, rowset, rows, hrows, reason, phase, cantdeny);
}

HRESULT CALLBACK IRowsetNotify_OnFieldChange_Proxy(IRowsetNotify* This, IRowset *rowset, HROW row, DBORDINAL ccols, DBORDINAL *columns,
                                                   DBREASON reason, DBEVENTPHASE phase, BOOL cantdeny)
{
    TRACE("(%p)->(%p %Ix %Id %p %ld %ld %d)\n", This, rowset, row, ccols, columns, reason, phase, cantdeny);
    return IRowsetNotify_RemoteOnFieldChange_Proxy(This, rowset, row, ccols, columns, reason, phase, cantdeny);
}

HRESULT __RPC_STUB IRowsetNotify_OnFieldChange_Stub(IRowsetNotify* This, IRowset *rowset, HROW row, DBORDINAL ccols, DBORDINAL *columns,
                                                    DBREASON reason, DBEVENTPHASE phase, BOOL cantdeny)
{
    TRACE("(%p)->(%p %Ix %Id %p %ld %ld %d)\n", This, rowset, row, ccols, columns, reason, phase, cantdeny);
    return IRowsetNotify_OnFieldChange(This, rowset, row, ccols, columns, reason, phase, cantdeny);
}

HRESULT CALLBACK IRowsetNotify_OnRowsetChange_Proxy(IRowsetNotify* This, IRowset *rowset, DBREASON reason, DBEVENTPHASE phase, BOOL cantdeny)
{
    TRACE("(%p)->(%p %ld %ld %d)\n", This, rowset, reason, phase, cantdeny);
    return IRowsetNotify_RemoteOnRowsetChange_Proxy(This, rowset, reason, phase, cantdeny);
}

HRESULT __RPC_STUB IRowsetNotify_OnRowsetChange_Stub(IRowsetNotify* This, IRowset *rowset, DBREASON reason, DBEVENTPHASE phase, BOOL cantdeny)
{
    TRACE("(%p)->(%p %ld %ld %d)\n", This, rowset, reason, phase, cantdeny);
    return IRowsetNotify_OnRowsetChange(This, rowset, reason, phase, cantdeny);
}


HRESULT CALLBACK ISourcesRowset_GetSourcesRowset_Proxy(ISourcesRowset* This, IUnknown *pUnkOuter, REFIID riid, ULONG cPropertySets,
                              DBPROPSET rgProperties[], IUnknown **ppSourcesRowset)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)->(%p %s %ld %p %p)\n", This, pUnkOuter, debugstr_guid(riid), cPropertySets, rgProperties, ppSourcesRowset);

    hr = ISourcesRowset_RemoteGetSourcesRowset_Proxy(This, pUnkOuter, riid, cPropertySets, rgProperties, ppSourcesRowset, 0, NULL, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    return hr;
}

HRESULT __RPC_STUB ISourcesRowset_GetSourcesRowset_Stub(ISourcesRowset* This, IUnknown *pUnkOuter, REFIID riid, ULONG cPropertySets,
                                 DBPROPSET *rgProperties, IUnknown **ppSourcesRowset, ULONG cTotalProps, DBPROPSTATUS *rgPropStatus,
                                 IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->(%p %s %ld %p %p %ld %p %p)\n", This, pUnkOuter, debugstr_guid(riid), cPropertySets, rgProperties, ppSourcesRowset,
                                cTotalProps, rgPropStatus, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = ISourcesRowset_GetSourcesRowset(This, pUnkOuter, riid, cPropertySets, rgProperties, ppSourcesRowset);
    if(FAILED(hr))
        GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IErrorRecords_GetRecordCount_Proxy(IErrorRecords* This, ULONG *records)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)->%p\n", This, records);

    hr = IErrorRecords_RemoteGetRecordCount_Proxy(This, records, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    return hr;
}

HRESULT __RPC_STUB IErrorRecords_GetRecordCount_Stub(IErrorRecords* This, ULONG *pcRecords, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->%p %p\n", This, pcRecords, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = IErrorRecords_GetRecordCount(This, pcRecords);
    if(FAILED(hr))
        GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IErrorRecords_GetErrorParameters_Proxy(IErrorRecords* This, ULONG ulRecordNum, DISPPARAMS *pdispparams)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)->%ld %p\n", This, ulRecordNum, pdispparams);

    hr = IErrorRecords_RemoteGetErrorParameters_Proxy(This, ulRecordNum, pdispparams, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    return hr;
}

HRESULT __RPC_STUB IErrorRecords_GetErrorParameters_Stub(IErrorRecords* This, ULONG ulRecordNum, DISPPARAMS *pdispparams,
    IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->%ld %p %p\n", This, ulRecordNum, pdispparams, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = IErrorRecords_GetErrorParameters(This, ulRecordNum, pdispparams);
    if(FAILED(hr))
        GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IErrorRecords_GetErrorInfo_Proxy(IErrorRecords* This, ULONG ulRecordNum, LCID lcid, IErrorInfo **ppErrorInfo)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)->%ld %ld %p\n", This, ulRecordNum, lcid, ppErrorInfo);

    hr = IErrorRecords_RemoteGetErrorInfo_Proxy(This, ulRecordNum, lcid, ppErrorInfo, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    return hr;
}

HRESULT __RPC_STUB IErrorRecords_GetErrorInfo_Stub(IErrorRecords* This, ULONG ulRecordNum, LCID lcid,
    IErrorInfo **ppErrorInfo, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->%ld %ld %p %p\n", This, ulRecordNum, lcid, ppErrorInfo, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = IErrorRecords_GetErrorInfo(This, ulRecordNum, lcid, ppErrorInfo);
    if(FAILED(hr))
        GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IErrorRecords_GetCustomErrorObject_Proxy(IErrorRecords* This, ULONG ulRecordNum, REFIID riid,
    IUnknown **ppObject)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)->%ld %s %p\n", This, ulRecordNum, debugstr_guid(riid), ppObject);

    hr = IErrorRecords_RemoteGetCustomErrorObject_Proxy(This, ulRecordNum, riid, ppObject, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }
    return hr;
}

HRESULT __RPC_STUB IErrorRecords_GetCustomErrorObject_Stub(IErrorRecords* This, ULONG ulRecordNum, REFIID riid,
    IUnknown **ppObject, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->%ld %s %p %p\n", This, ulRecordNum, debugstr_guid(riid), ppObject, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = IErrorRecords_GetCustomErrorObject(This, ulRecordNum, riid, ppObject);
    if(FAILED(hr))
        GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IErrorRecords_GetBasicErrorInfo_Proxy(IErrorRecords* This, ULONG ulRecordNum, ERRORINFO *pErrorInfo)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)->%ld %p\n", This, ulRecordNum, pErrorInfo);

    hr = IErrorRecords_RemoteGetBasicErrorInfo_Proxy(This, ulRecordNum, pErrorInfo, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }

    return hr;
}

HRESULT __RPC_STUB IErrorRecords_GetBasicErrorInfo_Stub(IErrorRecords* This, ULONG ulRecordNum, ERRORINFO *pErrorInfo,
    IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->%ld %p %p\n", This, ulRecordNum, pErrorInfo, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = IErrorRecords_GetBasicErrorInfo(This, ulRecordNum, pErrorInfo);
    if(FAILED(hr))
        GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT CALLBACK IErrorRecords_AddErrorRecord_Proxy(IErrorRecords* This, ERRORINFO *pErrorInfo, DWORD dwLookupID,
    DISPPARAMS *pdispparams, IUnknown *punkCustomError, DWORD dwDynamicErrorID)
{
    IErrorInfo *error = NULL;
    HRESULT hr;

    TRACE("(%p)->%p %ld %p %p %ld\n", This, pErrorInfo, dwLookupID, pdispparams, punkCustomError, dwDynamicErrorID);

    hr = IErrorRecords_RemoteAddErrorRecord_Proxy(This, pErrorInfo, dwLookupID, pdispparams, punkCustomError,
        dwDynamicErrorID, &error);
    if(error)
    {
        SetErrorInfo(0, error);
        IErrorInfo_Release(error);
    }
    return hr;
}

HRESULT __RPC_STUB IErrorRecords_AddErrorRecord_Stub(IErrorRecords* This, ERRORINFO *pErrorInfo, DWORD dwLookupID,
    DISPPARAMS *pdispparams, IUnknown *punkCustomError, DWORD dwDynamicErrorID, IErrorInfo **ppErrorInfoRem)
{
    HRESULT hr;

    TRACE("(%p)->%p %ld %p %p %ld %p\n", This, pErrorInfo, dwLookupID, pdispparams, punkCustomError,
                        dwDynamicErrorID, ppErrorInfoRem);

    *ppErrorInfoRem = NULL;
    hr = IErrorRecords_AddErrorRecord(This, pErrorInfo, dwLookupID, pdispparams, punkCustomError, dwDynamicErrorID);
    if(FAILED(hr))
        GetErrorInfo(0, ppErrorInfoRem);

    return hr;
}

HRESULT __RPC_STUB IRowPosition_Initialize_Stub(IRowPosition* This, IUnknown *rowset, IErrorInfo **errorinfo)
{
    FIXME("(%p)->(%p %p): stub\n", This, rowset, errorinfo);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IRowPosition_SetRowPosition_Stub(IRowPosition* This, HCHAPTER chapter, HROW row, DBPOSITIONFLAGS flags, IErrorInfo **errorinfo)
{
    FIXME("(%p)->(%Ix %Ix %ld %p): stub\n", This, chapter, row, flags, errorinfo);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IRowPosition_GetRowset_Stub(IRowPosition* This, REFIID riid, IUnknown **rowset, IErrorInfo **errorinfo)
{
    FIXME("(%p)->(%s %p %p): stub\n", This, debugstr_guid(riid), rowset, errorinfo);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IRowPosition_GetRowPosition_Stub(IRowPosition* This, HCHAPTER *chapter, HROW *row, DBPOSITIONFLAGS *flags, IErrorInfo **errorinfo)
{
    FIXME("(%p)->(%p %p %p %p): stub\n", This, chapter, row, flags, errorinfo);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IRowPosition_ClearRowPosition_Stub(IRowPosition* This, IErrorInfo **errorinfo)
{
    FIXME("(%p)->(%p): stub\n", This, errorinfo);
    return E_NOTIMPL;
}

HRESULT CALLBACK IRowPosition_ClearRowPosition_Proxy(IRowPosition* This)
{
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

HRESULT CALLBACK IRowPosition_GetRowPosition_Proxy(IRowPosition* This, HCHAPTER *chapter, HROW *row, DBPOSITIONFLAGS *flags)
{
    FIXME("(%p)->(%p %p %p): stub\n", This, chapter, row, flags);
    return E_NOTIMPL;
}

HRESULT CALLBACK IRowPosition_GetRowset_Proxy(IRowPosition* This, REFIID riid, IUnknown **rowset)
{
    FIXME("(%p)->(%s %p): stub\n", This, debugstr_guid(riid), rowset);
    return E_NOTIMPL;
}

HRESULT CALLBACK IRowPosition_Initialize_Proxy(IRowPosition* This, IUnknown *rowset)
{
    FIXME("(%p)->(%p): stub\n", This, rowset);
    return E_NOTIMPL;
}

HRESULT CALLBACK IRowPosition_SetRowPosition_Proxy(IRowPosition* This, HCHAPTER chapter, HROW row, DBPOSITIONFLAGS flags)
{
    FIXME("(%p)->(%Ix %Ix %ld): stub\n", This, chapter, row, flags);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IRowPositionChange_OnRowPositionChange_Stub(IRowPositionChange* This, DBREASON reason, DBEVENTPHASE phase,
    BOOL cant_deny, IErrorInfo **errorinfo)
{
    FIXME("(%p)->(0x%lx 0x%lx %d %p): stub\n", This, reason, phase, cant_deny, errorinfo);
    return E_NOTIMPL;
}

HRESULT CALLBACK IRowPositionChange_OnRowPositionChange_Proxy(IRowPositionChange* This, DBREASON reason, DBEVENTPHASE phase,
    BOOL cant_deny)
{
    FIXME("(%p)->(0x%lx 0x%lx %d): stub\n", This, reason, phase, cant_deny);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IChapteredRowset_ReleaseChapter_Stub(IChapteredRowset* This, HCHAPTER chapter, DBREFCOUNT *refcount, IErrorInfo **errorinfo)
{
    FIXME("(%p)->(%Ix %p %p): stub\n", This, chapter, refcount, errorinfo);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IChapteredRowset_AddRefChapter_Stub(IChapteredRowset* This, HCHAPTER chapter, DBREFCOUNT *refcount, IErrorInfo **errorinfo)
{
    FIXME("(%p)->(%Ix %p %p): stub\n", This, chapter, refcount, errorinfo);
    return E_NOTIMPL;
}

HRESULT CALLBACK IChapteredRowset_AddRefChapter_Proxy(IChapteredRowset* This, HCHAPTER chapter, DBREFCOUNT *refcount)
{
    FIXME("(%p)->(%Ix %p): stub\n", This, chapter, refcount);
    return E_NOTIMPL;
}

HRESULT CALLBACK IChapteredRowset_ReleaseChapter_Proxy(IChapteredRowset* This, HCHAPTER chapter, DBREFCOUNT *refcount)
{
    FIXME("(%p)->(%Ix %p): stub\n", This, chapter, refcount);
    return E_NOTIMPL;
}

HRESULT CALLBACK IColumnsInfo_GetColumnInfo_Proxy(IColumnsInfo* This, DBORDINAL *columns,
        DBCOLUMNINFO **colinfo, OLECHAR **stringsbuffer)
{
    FIXME("(%p)->(%p %p %p): stub\n", This, columns, colinfo, stringsbuffer);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IColumnsInfo_GetColumnInfo_Stub(IColumnsInfo* This, DBORDINAL *columns, DBCOLUMNINFO **col_info,
    DBBYTEOFFSET **name_offsets, DBBYTEOFFSET **columnid_offsets, DBLENGTH *string_len, OLECHAR **stringsbuffer,
    IErrorInfo **error)
{
    FIXME("(%p)->(%p %p %p %p %p %p %p): stub\n", This, columns, col_info, name_offsets, columnid_offsets,
            string_len, stringsbuffer, error);
    return E_NOTIMPL;
}

HRESULT CALLBACK IColumnsInfo_MapColumnIDs_Proxy(IColumnsInfo* This, DBORDINAL column_ids, const DBID *dbids,
    DBORDINAL *columns)
{
    FIXME("(%p)->(%Iu %p %p): stub\n", This, column_ids, dbids, columns);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IColumnsInfo_MapColumnIDs_Stub(IColumnsInfo* This, DBORDINAL column_ids, const DBID *dbids,
    DBORDINAL *columns, IErrorInfo **error)
{
    FIXME("(%p)->(%Iu %p %p %p): stub\n", This, column_ids, dbids, columns, error);
    return E_NOTIMPL;
}

HRESULT CALLBACK IGetDataSource_GetDataSource_Proxy(IGetDataSource* This, REFIID riid, IUnknown **datasource)
{
    FIXME("(%p)->(%s %p): stub\n", This, debugstr_guid(riid), datasource);
    return E_NOTIMPL;
}

HRESULT CALLBACK IColumnsRowset_GetAvailableColumns_Proxy(IColumnsRowset* This, DBORDINAL *count, DBID **columns)
{
    FIXME("(%p)->(%p %p): stub\n", This, count, columns);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IColumnsRowset_GetAvailableColumns_Stub(IColumnsRowset* This, DBORDINAL *count, DBID **columns,
    IErrorInfo **error)
{
    FIXME("(%p)->(%p %p %p): stub\n", This, count, columns, error);
    return E_NOTIMPL;
}

HRESULT CALLBACK IColumnsRowset_GetColumnsRowset_Proxy(IColumnsRowset* This, IUnknown *outer, DBORDINAL count,
    const DBID columns[], REFIID riid, ULONG property_cnt, DBPROPSET property_sets[], IUnknown **rowset)
{
    FIXME("(%p)->(%p %Id %p %s %ld %p %p): stub\n", This, outer, count, columns, debugstr_guid(riid),
          property_cnt, property_sets, rowset);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IColumnsRowset_GetColumnsRowset_Stub(IColumnsRowset* This, IUnknown *outer,
    DBORDINAL count, const DBID *columns, REFIID riid, ULONG property_cnt, DBPROPSET *property_sets,
    IUnknown **rowset, ULONG props_cnt, DBPROPSTATUS *prop_status, IErrorInfo **error)
{
    FIXME("(%p)->(%p %Id %p %s %ld %p %p %lu %p %p): stub\n", This, outer, count, columns, debugstr_guid(riid),
          property_cnt, property_sets, rowset, props_cnt, prop_status, error);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IGetDataSource_GetDataSource_Stub(IGetDataSource* This, REFIID riid, IUnknown **datasource,
    IErrorInfo **error)
{
    FIXME("(%p)->(%s %p %p): stub\n", This, debugstr_guid(riid), datasource, error);
    return E_NOTIMPL;
}

HRESULT CALLBACK ICommandProperties_GetProperties_Proxy(ICommandProperties* This, const ULONG count,
    const DBPROPIDSET propertyidsets[], ULONG *sets_cnt, DBPROPSET **propertyset)
{
    FIXME("(%p)->(%ld %p %p %p): stub\n", This, count, propertyidsets, sets_cnt, propertyset);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ICommandProperties_GetProperties_Stub(ICommandProperties* This, const ULONG count,
    const DBPROPIDSET *propertyidsets, ULONG *sets_cnt, DBPROPSET **propertyset, IErrorInfo **error)
{
    FIXME("(%p)->(%ld %p %p %p %p): stub\n", This, count, propertyidsets, sets_cnt, propertyset, error);
    return E_NOTIMPL;
}

HRESULT CALLBACK ICommandProperties_SetProperties_Proxy(ICommandProperties* This, ULONG count, DBPROPSET propertyset[])
{
    FIXME("(%p)->(%ld %p): stub\n", This, count, propertyset);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ICommandProperties_SetProperties_Stub(ICommandProperties* This, ULONG count,
    DBPROPSET *propertyset, ULONG total, DBPROPSTATUS *propstatus, IErrorInfo **error)
{
    FIXME("(%p)->(%ld %p %ld %p %p): stub\n", This, count, propertyset, total, propstatus, error);
    return E_NOTIMPL;
}

HRESULT CALLBACK IConvertType_CanConvert_Proxy(IConvertType* This, DBTYPE from, DBTYPE to, DBCONVERTFLAGS flags)
{
    FIXME("(%p)->(%d %d 0x%08lx): stub\n", This, from, to, flags);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IConvertType_CanConvert_Stub(IConvertType* This, DBTYPE from, DBTYPE to,
    DBCONVERTFLAGS flags, IErrorInfo **error)
{
    FIXME("(%p)->(%d %d 0x%08lx %p): stub\n", This, from, to, flags, error);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IMultipleResults_GetResult_Stub(IMultipleResults* This, IUnknown *outer,
    DBRESULTFLAG result, REFIID riid, DBROWCOUNT *affected, IUnknown **rowset, IErrorInfo **error)
{
    FIXME("(%p)->(%p %Id %s %p %p %p): stub\n", This, outer, result, debugstr_guid(riid), affected, rowset, error);
    return E_NOTIMPL;
}

HRESULT CALLBACK IMultipleResults_GetResult_Proxy(IMultipleResults* This, IUnknown *outer,
    DBRESULTFLAG result, REFIID riid, DBROWCOUNT *affected, IUnknown **rowset)
{
    FIXME("(%p)->(%p %Id %s %p %p): stub\n", This, outer, result, debugstr_guid(riid), affected, rowset);
    return E_NOTIMPL;
}

HRESULT CALLBACK ICommandPrepare_Prepare_Proxy(ICommandPrepare* This, ULONG runs)
{
    FIXME("(%p)->(%ld): stub\n", This, runs);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ICommandPrepare_Prepare_Stub(ICommandPrepare* This, ULONG runs, IErrorInfo **error)
{
    FIXME("(%p)->(%ld %p): stub\n", This, runs, error);
    return E_NOTIMPL;
}

HRESULT CALLBACK ICommandPrepare_Unprepare_Proxy(ICommandPrepare* This)
{
    FIXME("(%p)->(): stub\n", This);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ICommandPrepare_Unprepare_Stub(ICommandPrepare* This, IErrorInfo **error)
{
    FIXME("(%p)->(%p): stub\n", This, error);
    return E_NOTIMPL;
}

HRESULT CALLBACK ICommandWithParameters_GetParameterInfo_Proxy(ICommandWithParameters* This,
    DB_UPARAMS *uparams, DBPARAMINFO **info, OLECHAR **buffer)
{
    FIXME("(%p)->(%p %p %p): stub\n", This, uparams, info, buffer);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ICommandWithParameters_GetParameterInfo_Stub(ICommandWithParameters* This,
    DB_UPARAMS *uparams, DBPARAMINFO **param_info, DBBYTEOFFSET **offsets, DBLENGTH *buff_len,
    OLECHAR **buffer, IErrorInfo **error)
{
    FIXME("(%p)->(%p %p %p %p %p %p %p): stub\n", This, uparams, param_info, buffer, offsets, buff_len,
          buffer, error);
    return E_NOTIMPL;
}

HRESULT CALLBACK ICommandWithParameters_SetParameterInfo_Proxy(ICommandWithParameters* This,
    DB_UPARAMS params, const DB_UPARAMS ordinals[], const DBPARAMBINDINFO bindinfo[])
{
    FIXME("(%p)->(%Id %p %p): stub\n", This, params, ordinals, bindinfo);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ICommandWithParameters_SetParameterInfo_Stub(ICommandWithParameters* This,
    DB_UPARAMS params, const DB_UPARAMS *ordinals, const DBPARAMBINDINFO *bindinfo, IErrorInfo **error)
{
    FIXME("(%p)->(%Id %p %p %p): stub\n", This, params, ordinals, bindinfo, error);
    return E_NOTIMPL;
}

HRESULT CALLBACK ICommandWithParameters_MapParameterNames_Proxy(ICommandWithParameters* This,
    DB_UPARAMS count, LPCWSTR names[], DB_LPARAMS ordinals[])
{
    FIXME("(%p)->(%Id %p %p): stub\n", This, count, names, ordinals);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ICommandWithParameters_MapParameterNames_Stub(ICommandWithParameters* This,
    DB_UPARAMS count, LPCOLESTR *names, DB_LPARAMS *ordinals, IErrorInfo **error)
{
    FIXME("(%p)->(%Id %p %p %p): stub\n", This, count, names, ordinals, error);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ITransactionJoin_GetOptionsObject_Stub(ITransactionJoin* This,
    ITransactionOptions **options, IErrorInfo **error)
{
    FIXME("(%p)->(%p, %p): stub\n", This, options, error);
    return E_NOTIMPL;
}

HRESULT CALLBACK ITransactionJoin_GetOptionsObject_Proxy(ITransactionJoin *This,
        ITransactionOptions **options)
{
    FIXME("(%p)->(%p): stub\n", This, options);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ITransactionJoin_JoinTransaction_Stub(ITransactionJoin* This, IUnknown *unk,
    ISOLEVEL level, ULONG flags, ITransactionOptions *options, IErrorInfo **error)
{
    FIXME("(%p)->(%p, %ld, 0x%08lx, %p, %p): stub\n", This, unk, level, flags, options, error);
    return E_NOTIMPL;
}

HRESULT CALLBACK ITransactionJoin_JoinTransaction_Proxy(ITransactionJoin* This, IUnknown *unk,
    ISOLEVEL level, ULONG flags, ITransactionOptions *options)
{
    FIXME("(%p)->(%p, %ld, 0x%08lx, %p): stub\n", This, unk, level, flags, options);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ITransactionLocal_GetOptionsObject_Stub(ITransactionLocal* This,
    ITransactionOptions **options, IErrorInfo **info)
{
    FIXME("(%p)->(%p, %p): stub\n", This, options, info);
    return E_NOTIMPL;
}

HRESULT CALLBACK ITransactionLocal_GetOptionsObject_Proxy(ITransactionLocal* This, ITransactionOptions **options)
{
    FIXME("(%p)->(%p): stub\n", This, options);
    return E_NOTIMPL;
}

HRESULT CALLBACK ITransactionLocal_StartTransaction_Proxy(ITransactionLocal* This, ISOLEVEL level,
    ULONG flags, ITransactionOptions *options, ULONG *trans_level)
{
    FIXME("(%p)->(%ld, 0x%08lx, %p, %p): stub\n", This, level, flags, options, trans_level);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ITransactionLocal_StartTransaction_Stub(ITransactionLocal* This, ISOLEVEL level,
    ULONG flags, ITransactionOptions *options, ULONG *trans_level, IErrorInfo **info)
{
    FIXME("(%p)->(%ld, 0x%08lx, %p, %p, %p): stub\n", This, level, flags, options, trans_level, info);
    return E_NOTIMPL;
}

HRESULT CALLBACK ITransactionObject_GetTransactionObject_Proxy(ITransactionObject* This,
    ULONG level, ITransaction **transaction)
{
    FIXME("(%p)->(%ld, %p): stub\n", This, level, transaction);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ITransactionObject_GetTransactionObject_Stub(ITransactionObject* This,
    ULONG level, ITransaction **transaction, IErrorInfo **info)
{
    FIXME("(%p)->(%ld, %p, %p): stub\n", This, level, transaction, info);
    return E_NOTIMPL;
}
