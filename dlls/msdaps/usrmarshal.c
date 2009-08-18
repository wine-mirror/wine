/*
 *    Misc marshaling routinues
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
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "objbase.h"
#include "oledb.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

HRESULT CALLBACK IDBCreateCommand_CreateCommand_Proxy(IDBCreateCommand* This, IUnknown *pUnkOuter,
                                                      REFIID riid, IUnknown **ppCommand)
{
    FIXME("(%p, %p, %s, %p): stub\n", This, pUnkOuter, debugstr_guid(riid),
          ppCommand);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDBCreateCommand_CreateCommand_Stub(IDBCreateCommand* This, IUnknown *pUnkOuter,
                                                       REFIID riid, IUnknown **ppCommand, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %p, %s, %p, %p): stub\n", This, pUnkOuter, debugstr_guid(riid),
          ppCommand, ppErrorInfoRem);
    return E_NOTIMPL;

}

HRESULT CALLBACK IDBCreateSession_CreateSession_Proxy(IDBCreateSession* This, IUnknown *pUnkOuter,
                                                      REFIID riid, IUnknown **ppDBSession)
{
    FIXME("(%p, %p, %s, %p): stub\n", This, pUnkOuter, debugstr_guid(riid),
          ppDBSession);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDBCreateSession_CreateSession_Stub(IDBCreateSession* This, IUnknown *pUnkOuter,
                                                       REFIID riid, IUnknown **ppDBSession, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %p, %s, %p, %p): stub\n", This, pUnkOuter, debugstr_guid(riid),
          ppDBSession, ppErrorInfoRem);
    return E_NOTIMPL;

}

HRESULT CALLBACK IDBProperties_GetProperties_Proxy(IDBProperties* This, ULONG cPropertyIDSets, const DBPROPIDSET rgPropertyIDSets[],
                                                   ULONG *pcPropertySets, DBPROPSET **prgPropertySets)
{
    FIXME("(%p, %d, %p, %p, %p): stub\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertySets,
          prgPropertySets);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDBProperties_GetProperties_Stub(IDBProperties* This, ULONG cPropertyIDSets, const DBPROPIDSET *rgPropertyIDSets,
                                                    ULONG *pcPropertySets, DBPROPSET **prgPropertySets, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %d, %p, %p, %p, %p): stub\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertySets,
          prgPropertySets, ppErrorInfoRem);
    return E_NOTIMPL;
}

HRESULT CALLBACK IDBProperties_GetPropertyInfo_Proxy(IDBProperties* This, ULONG cPropertyIDSets, const DBPROPIDSET rgPropertyIDSets[],
                                                     ULONG *pcPropertyInfoSets, DBPROPINFOSET **prgPropertyInfoSets,
                                                     OLECHAR **ppDescBuffer)
{
    FIXME("(%p, %d, %p, %p, %p, %p): stub\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertyInfoSets,
          prgPropertyInfoSets, ppDescBuffer);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDBProperties_GetPropertyInfo_Stub(IDBProperties* This, ULONG cPropertyIDSets, const DBPROPIDSET *rgPropertyIDSets,
                                                      ULONG *pcPropertyInfoSets, DBPROPINFOSET **prgPropertyInfoSets,
                                                      ULONG *pcOffsets, DBBYTEOFFSET **prgDescOffsets, ULONG *pcbDescBuffer,
                                                      OLECHAR **ppDescBuffer, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %d, %p, %p, %p, %p, %p, %p, %p, %p): stub\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertyInfoSets,
          prgPropertyInfoSets, pcOffsets, prgDescOffsets, pcbDescBuffer, ppDescBuffer, ppErrorInfoRem);
    return E_NOTIMPL;
}

HRESULT CALLBACK IDBProperties_SetProperties_Proxy(IDBProperties* This, ULONG cPropertySets, DBPROPSET rgPropertySets[])
{
    FIXME("(%p, %d, %p): stub\n", This, cPropertySets, rgPropertySets);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDBProperties_SetProperties_Stub(IDBProperties* This, ULONG cPropertySets, DBPROPSET *rgPropertySets,
                                                    ULONG cTotalProps, DBPROPSTATUS *rgPropStatus, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %d, %p, %d, %p, %p): stub\n", This, cPropertySets, rgPropertySets, cTotalProps,
          rgPropStatus, ppErrorInfoRem);
    return E_NOTIMPL;
}

HRESULT CALLBACK IDBInitialize_Initialize_Proxy(IDBInitialize* This)
{
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDBInitialize_Initialize_Stub(IDBInitialize* This, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %p): stub\n", This, ppErrorInfoRem);
    return E_NOTIMPL;
}

HRESULT CALLBACK IDBInitialize_Uninitialize_Proxy(IDBInitialize* This)
{
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDBInitialize_Uninitialize_Stub(IDBInitialize* This, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %p): stub\n", This, ppErrorInfoRem);
    return E_NOTIMPL;
}

HRESULT CALLBACK IDBDataSourceAdmin_CreateDataSource_Proxy(IDBDataSourceAdmin* This, ULONG cPropertySets,
                                                           DBPROPSET rgPropertySets[], IUnknown *pUnkOuter,
                                                           REFIID riid, IUnknown **ppDBSession)
{
    FIXME("(%p, %d, %p, %p, %s, %p): stub\n", This, cPropertySets, rgPropertySets, pUnkOuter,
          debugstr_guid(riid), ppDBSession);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDBDataSourceAdmin_CreateDataSource_Stub(IDBDataSourceAdmin* This, ULONG cPropertySets,
                                                            DBPROPSET *rgPropertySets, IUnknown *pUnkOuter,
                                                            REFIID riid, IUnknown **ppDBSession, ULONG cTotalProps,
                                                            DBPROPSTATUS *rgPropStatus, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %d, %p, %p, %s, %p, %d, %p, %p): stub\n", This, cPropertySets, rgPropertySets, pUnkOuter,
          debugstr_guid(riid), ppDBSession, cTotalProps, rgPropStatus, ppErrorInfoRem);
    return E_NOTIMPL;
}

HRESULT CALLBACK IDBDataSourceAdmin_DestroyDataSource_Proxy(IDBDataSourceAdmin* This)
{
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDBDataSourceAdmin_DestroyDataSource_Stub(IDBDataSourceAdmin* This, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %p): stub\n", This, ppErrorInfoRem);
    return E_NOTIMPL;
}

HRESULT CALLBACK IDBDataSourceAdmin_GetCreationProperties_Proxy(IDBDataSourceAdmin* This, ULONG cPropertyIDSets,
                                                                const DBPROPIDSET rgPropertyIDSets[], ULONG *pcPropertyInfoSets,
                                                                DBPROPINFOSET **prgPropertyInfoSets, OLECHAR **ppDescBuffer)
{
    FIXME("(%p, %d, %p, %p, %p, %p): stub\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertyInfoSets,
          prgPropertyInfoSets, ppDescBuffer);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDBDataSourceAdmin_GetCreationProperties_Stub(IDBDataSourceAdmin* This, ULONG cPropertyIDSets,
                                                                 const DBPROPIDSET *rgPropertyIDSets, ULONG *pcPropertyInfoSets,
                                                                 DBPROPINFOSET **prgPropertyInfoSets, DBCOUNTITEM *pcOffsets,
                                                                 DBBYTEOFFSET **prgDescOffsets, ULONG *pcbDescBuffer,
                                                                 OLECHAR **ppDescBuffer, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %d, %p, %p, %p, %p, %p, %p, %p, %p): stub\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertyInfoSets,
          prgPropertyInfoSets, pcOffsets, prgDescOffsets, pcbDescBuffer, ppDescBuffer, ppErrorInfoRem);
    return E_NOTIMPL;
}

HRESULT CALLBACK IDBDataSourceAdmin_ModifyDataSource_Proxy(IDBDataSourceAdmin* This, ULONG cPropertySets, DBPROPSET rgPropertySets[])
{
    FIXME("(%p, %d, %p): stub\n", This, cPropertySets, rgPropertySets);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDBDataSourceAdmin_ModifyDataSource_Stub(IDBDataSourceAdmin* This, ULONG cPropertySets,
                                                            DBPROPSET *rgPropertySets, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %d, %p, %p): stub\n", This, cPropertySets, rgPropertySets, ppErrorInfoRem);
    return E_NOTIMPL;
}

HRESULT CALLBACK ISessionProperties_GetProperties_Proxy(ISessionProperties* This, ULONG cPropertyIDSets,
                                                        const DBPROPIDSET rgPropertyIDSets[], ULONG *pcPropertySets,
                                                        DBPROPSET **prgPropertySets)
{
    FIXME("(%p, %d, %p, %p, %p): stub\n", This, cPropertyIDSets, rgPropertyIDSets,
          pcPropertySets, prgPropertySets);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ISessionProperties_GetProperties_Stub(ISessionProperties* This, ULONG cPropertyIDSets,
                                                         const DBPROPIDSET *rgPropertyIDSets, ULONG *pcPropertySets,
                                                         DBPROPSET **prgPropertySets, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %d, %p, %p, %p, %p): stub\n", This, cPropertyIDSets, rgPropertyIDSets,
          pcPropertySets, prgPropertySets, ppErrorInfoRem);
    return E_NOTIMPL;
}

HRESULT CALLBACK ISessionProperties_SetProperties_Proxy(ISessionProperties* This, ULONG cPropertySets, DBPROPSET rgPropertySets[])
{
    FIXME("(%p, %d, %p): stub\n", This, cPropertySets, rgPropertySets);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ISessionProperties_SetProperties_Stub(ISessionProperties* This, ULONG cPropertySets, DBPROPSET *rgPropertySets,
                                                         ULONG cTotalProps, DBPROPSTATUS *rgPropStatus, IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %d, %p, %d, %p, %p): stub\n", This, cPropertySets, rgPropertySets, cTotalProps,
          rgPropStatus, ppErrorInfoRem);
    return E_NOTIMPL;
}

HRESULT CALLBACK IOpenRowset_OpenRowset_Proxy(IOpenRowset* This, IUnknown *pUnkOuter, DBID *pTableID, DBID *pIndexID,
                                              REFIID riid, ULONG cPropertySets, DBPROPSET rgPropertySets[], IUnknown **ppRowset)
{
    FIXME("(%p, %p, %p, %p, %s, %d, %p, %p): stub\n", This, pUnkOuter, pTableID, pIndexID, debugstr_guid(riid),
          cPropertySets, rgPropertySets, ppRowset);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IOpenRowset_OpenRowset_Stub(IOpenRowset* This, IUnknown *pUnkOuter, DBID *pTableID, DBID *pIndexID,
                                               REFIID riid, ULONG cPropertySets, DBPROPSET *rgPropertySets,
                                               IUnknown **ppRowset, ULONG cTotalProps, DBPROPSTATUS *rgPropStatus,
                                               IErrorInfo **ppErrorInfoRem)
{
    FIXME("(%p, %p, %p, %p, %s, %d, %p, %p, %d, %p, %p): stub\n", This, pUnkOuter, pTableID, pIndexID, debugstr_guid(riid),
          cPropertySets, rgPropertySets, ppRowset, cTotalProps, rgPropStatus, ppErrorInfoRem);
    return E_NOTIMPL;
}

HRESULT CALLBACK IBindResource_Bind_Proxy(IBindResource* This, IUnknown *pUnkOuter, LPCOLESTR pwszURL, DBBINDURLFLAG dwBindURLFlags,
                                          REFGUID rguid, REFIID riid, IAuthenticate *pAuthenticate, DBIMPLICITSESSION *pImplSession,
                                          DBBINDURLSTATUS *pdwBindStatus, IUnknown **ppUnk)
{
    FIXME("(%p, %p, %s, %08x, %s, %s, %p, %p, %p, %p): stub\n", This, pUnkOuter, debugstr_w(pwszURL), dwBindURLFlags,
          debugstr_guid(rguid), debugstr_guid(riid), pAuthenticate, pImplSession, pdwBindStatus, ppUnk);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IBindResource_Bind_Stub(IBindResource* This, IUnknown *pUnkOuter, LPCOLESTR pwszURL, DBBINDURLFLAG dwBindURLFlags,
                                           REFGUID rguid, REFIID riid, IAuthenticate *pAuthenticate, IUnknown *pSessionUnkOuter,
                                           IID *piid, IUnknown **ppSession, DBBINDURLSTATUS *pdwBindStatus, IUnknown **ppUnk)
{
    FIXME("(%p, %p, %s, %08x, %s, %s, %p, %p, %p, %p, %p, %p): stub\n", This, pUnkOuter, debugstr_w(pwszURL), dwBindURLFlags,
          debugstr_guid(rguid), debugstr_guid(riid), pAuthenticate, pSessionUnkOuter, piid, ppSession, pdwBindStatus, ppUnk);
    return E_NOTIMPL;
}

HRESULT CALLBACK ICreateRow_CreateRow_Proxy(ICreateRow* This, IUnknown *pUnkOuter, LPCOLESTR pwszURL, DBBINDURLFLAG dwBindURLFlags,
                                            REFGUID rguid, REFIID riid, IAuthenticate *pAuthenticate, DBIMPLICITSESSION *pImplSession,
                                            DBBINDURLSTATUS *pdwBindStatus, LPOLESTR *ppwszNewURL, IUnknown **ppUnk)
{
    FIXME("(%p, %p, %s, %08x, %s, %s, %p, %p, %p, %p, %p): stub\n", This, pUnkOuter, debugstr_w(pwszURL), dwBindURLFlags,
          debugstr_guid(rguid), debugstr_guid(riid), pAuthenticate, pImplSession, pdwBindStatus, ppwszNewURL, ppUnk);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB ICreateRow_CreateRow_Stub(ICreateRow* This, IUnknown *pUnkOuter, LPCOLESTR pwszURL, DBBINDURLFLAG dwBindURLFlags,
                                             REFGUID rguid, REFIID riid, IAuthenticate *pAuthenticate, IUnknown *pSessionUnkOuter,
                                             IID *piid, IUnknown **ppSession, DBBINDURLSTATUS *pdwBindStatus,
                                             LPOLESTR *ppwszNewURL, IUnknown **ppUnk)
{
    FIXME("(%p, %p, %s, %08x, %s, %s, %p, %p, %p, %p, %p, %p, %p): stub\n", This, pUnkOuter, debugstr_w(pwszURL), dwBindURLFlags,
          debugstr_guid(rguid), debugstr_guid(riid), pAuthenticate, pSessionUnkOuter, piid, ppSession, pdwBindStatus, ppwszNewURL,
          ppUnk);
    return E_NOTIMPL;
}
