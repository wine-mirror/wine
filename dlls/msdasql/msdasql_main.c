/*
 * Copyright 2019 Alistair Leslie-Hughes
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
#define DBINITCONSTANTS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "oledb.h"
#include "rpcproxy.h"
#include "wine/debug.h"
#include "oledberr.h"

#include "initguid.h"
#include "msdasql.h"
#include "odbcinst.h"
#include "sqlext.h"
#include "sqlucode.h"

#include "msdasql_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msdasql);

void dump_sql_diag_records(SQLSMALLINT type, SQLHANDLE handle)
{
    SQLCHAR state[6], msg[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER native;
    SQLSMALLINT i = 1, len;

    if (!TRACE_ON(msdasql))
        return;

    while(SQLGetDiagRec(type, handle, i, state, &native, msg, sizeof(msg), &len) == SQL_SUCCESS)
    {
        WARN("%d: %s: %s\n", i, state, msg);
        i++;
    }
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT create_msdasql_provider(REFIID riid, void **ppv);
static HRESULT create_msdasql_enumerator(REFIID riid, void **ppv);

HRESULT WINAPI msdasql_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    return create_msdasql_provider(riid, ppv);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static const IClassFactoryVtbl cfmsdasqlVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    msdasql_CreateInstance,
    ClassFactory_LockServer
};

HRESULT WINAPI enumerationcf_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    return create_msdasql_enumerator(riid, ppv);
}

static const IClassFactoryVtbl enumfactoryVtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    enumerationcf_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory cfmsdasql = { &cfmsdasqlVtbl };
static IClassFactory enumfactory = { &enumfactoryVtbl };

HRESULT WINAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, void **ppv )
{
    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (IsEqualGUID(&CLSID_MSDASQL, rclsid))
        return IClassFactory_QueryInterface(&cfmsdasql, riid, ppv);
    else if (IsEqualGUID(&CLSID_MSDASQL_ENUMERATOR, rclsid))
        return IClassFactory_QueryInterface(&enumfactory, riid, ppv);

    return CLASS_E_CLASSNOTAVAILABLE;
}

struct dbproperty
{
    const WCHAR *name;
    DBPROPID id;
    DBPROPOPTIONS options;
    VARTYPE type;
    HRESULT (*convert_dbproperty)(const WCHAR *src, VARIANT *dest);
};

struct mode_propval
{
    const WCHAR *name;
    DWORD value;
};

static int __cdecl dbmodeprop_compare(const void *a, const void *b)
{
    const WCHAR *src = a;
    const struct mode_propval *propval = b;
    return wcsicmp(src, propval->name);
}

static HRESULT convert_dbproperty_mode(const WCHAR *src, VARIANT *dest)
{
    struct mode_propval mode_propvals[] =
    {
        { L"Read", DB_MODE_READ },
        { L"ReadWrite", DB_MODE_READWRITE },
        { L"Share Deny None", DB_MODE_SHARE_DENY_NONE },
        { L"Share Deny Read", DB_MODE_SHARE_DENY_READ },
        { L"Share Deny Write", DB_MODE_SHARE_DENY_WRITE },
        { L"Share Exclusive", DB_MODE_SHARE_EXCLUSIVE },
        { L"Write", DB_MODE_WRITE },
    };
    struct mode_propval *prop;

    if ((prop = bsearch(src, mode_propvals, ARRAY_SIZE(mode_propvals),
                        sizeof(struct mode_propval), dbmodeprop_compare)))
    {
        V_VT(dest) = VT_I4;
        V_I4(dest) = prop->value;
        TRACE("%s = %#lx\n", debugstr_w(src), prop->value);
        return S_OK;
    }

    return E_FAIL;
}

static const struct dbproperty dbproperties[] =
{
    { L"Password",                 DBPROP_AUTH_PASSWORD,                   DBPROPOPTIONS_OPTIONAL, VT_BSTR },
    { L"Persist Security Info",    DBPROP_AUTH_PERSIST_SENSITIVE_AUTHINFO, DBPROPOPTIONS_OPTIONAL, VT_BOOL },
    { L"User ID",                  DBPROP_AUTH_USERID,                     DBPROPOPTIONS_OPTIONAL, VT_BSTR },
    { L"Data Source",              DBPROP_INIT_DATASOURCE,                 DBPROPOPTIONS_REQUIRED, VT_BSTR },
    { L"Window Handle",            DBPROP_INIT_HWND,                       DBPROPOPTIONS_OPTIONAL, sizeof(void *) == 8 ? VT_I8 : VT_I4 },
    { L"Location",                 DBPROP_INIT_LOCATION,                   DBPROPOPTIONS_OPTIONAL, VT_BSTR },
    { L"Mode",                     DBPROP_INIT_MODE,                       DBPROPOPTIONS_OPTIONAL, VT_I4, convert_dbproperty_mode },
    { L"Prompt",                   DBPROP_INIT_PROMPT,                     DBPROPOPTIONS_OPTIONAL, VT_I2 },
    { L"Connect Timeout",          DBPROP_INIT_TIMEOUT,                    DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { L"Extended Properties",      DBPROP_INIT_PROVIDERSTRING,             DBPROPOPTIONS_REQUIRED, VT_BSTR },
    { L"Locale Identifier",        DBPROP_INIT_LCID,                       DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { L"Initial Catalog",          DBPROP_INIT_CATALOG,                    DBPROPOPTIONS_OPTIONAL, VT_BSTR },
    { L"OLE DB Services",          DBPROP_INIT_OLEDBSERVICES,              DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { L"General Timeout",          DBPROP_INIT_GENERALTIMEOUT,             DBPROPOPTIONS_OPTIONAL, VT_I4 },
};

struct msdasql_prop
{
    VARTYPE id;
    VARIANT value;
};

struct msdasql
{
    IUnknown         MSDASQL_iface;
    IDBProperties    IDBProperties_iface;
    IDBInitialize    IDBInitialize_iface;
    IDBCreateSession IDBCreateSession_iface;
    IPersist         IPersist_iface;

    LONG     ref;
    struct msdasql_prop properties[14];

    /* ODBC Support */
    HENV henv;
    HDBC hdbc;
};

static inline struct msdasql *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct msdasql, MSDASQL_iface);
}

static inline struct msdasql *impl_from_IDBProperties(IDBProperties *iface)
{
    return CONTAINING_RECORD(iface, struct msdasql, IDBProperties_iface);
}

static inline struct msdasql *impl_from_IDBInitialize(IDBInitialize *iface)
{
    return CONTAINING_RECORD(iface, struct msdasql, IDBInitialize_iface);
}

static inline struct msdasql *impl_from_IDBCreateSession(IDBCreateSession *iface)
{
    return CONTAINING_RECORD(iface, struct msdasql, IDBCreateSession_iface);
}

static inline struct msdasql *impl_from_IPersist( IPersist *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql, IPersist_iface );
}

static HRESULT WINAPI msdsql_QueryInterface(IUnknown *iface, REFIID riid, void **out)
{
    struct msdasql *provider = impl_from_IUnknown(iface);

    TRACE("(%p)->(%s %p)\n", provider, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &CLSID_MSDASQL))
    {
        *out = &provider->MSDASQL_iface;
    }
    else if(IsEqualGUID(riid, &IID_IDBProperties))
    {
        *out = &provider->IDBProperties_iface;
    }
    else if ( IsEqualGUID(riid, &IID_IDBInitialize))
    {
        *out = &provider->IDBInitialize_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDBCreateSession))
    {
        *out = &provider->IDBCreateSession_iface;
    }
    else if(IsEqualGUID(&IID_IPersist, riid))
    {
        *out = &provider->IPersist_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI msdsql_AddRef(IUnknown *iface)
{
    struct msdasql *provider = impl_from_IUnknown(iface);
    ULONG ref = InterlockedIncrement(&provider->ref);

    TRACE("(%p) ref=%lu\n", provider, ref);

    return ref;
}

static ULONG  WINAPI msdsql_Release(IUnknown *iface)
{
    struct msdasql *provider = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&provider->ref);

    TRACE("(%p) ref=%lu\n", provider, ref);

    if (!ref)
    {
        SQLDisconnect(provider->hdbc);

        SQLFreeHandle(SQL_HANDLE_DBC, provider->hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, provider->henv);
        free(provider);
    }

    return ref;
}

static const IUnknownVtbl msdsql_vtbl =
{
    msdsql_QueryInterface,
    msdsql_AddRef,
    msdsql_Release
};

static HRESULT WINAPI dbprops_QueryInterface(IDBProperties *iface, REFIID riid, void **ppvObject)
{
    struct msdasql *provider = impl_from_IDBProperties(iface);

    return IUnknown_QueryInterface(&provider->MSDASQL_iface, riid, ppvObject);
}

static ULONG WINAPI dbprops_AddRef(IDBProperties *iface)
{
    struct msdasql *provider = impl_from_IDBProperties(iface);

    return IUnknown_AddRef(&provider->MSDASQL_iface);
}

static ULONG WINAPI dbprops_Release(IDBProperties *iface)
{
    struct msdasql *provider = impl_from_IDBProperties(iface);

    return IUnknown_Release(&provider->MSDASQL_iface);
}

static HRESULT WINAPI dbprops_GetProperties(IDBProperties *iface, ULONG cPropertyIDSets,
            const DBPROPIDSET rgPropertyIDSets[], ULONG *pcPropertySets, DBPROPSET **prgPropertySets)
{
    struct msdasql *provider = impl_from_IDBProperties(iface);
    int i, j, k;
    DBPROPSET *propset;
    BOOL no_prop_id;

    TRACE("(%p)->(%ld %p %p %p)\n", provider, cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets);

    *pcPropertySets = 1;

    if (cPropertyIDSets > 1)
    {
        FIXME("Currently only 0 or 1 property set are supported.\n");
        cPropertyIDSets = 1;
    }

    propset = CoTaskMemAlloc(max(cPropertyIDSets, 1) * sizeof(DBPROPSET));

    if (IsEqualGUID(&rgPropertyIDSets[0].guidPropertySet, &DBPROPSET_DATASOURCEINFO))
    {
        TRACE("Propertyset DBPROPSET_DATASOURCEINFO not supported\n");
        propset->guidPropertySet = rgPropertyIDSets[0].guidPropertySet;
        propset->cProperties = rgPropertyIDSets[0].cPropertyIDs;

        propset->rgProperties = CoTaskMemAlloc(propset->cProperties * sizeof(DBPROP));

        for (j=0; j < propset->cProperties; j++)
        {
            propset->rgProperties[j].dwPropertyID = rgPropertyIDSets[0].rgPropertyIDs[j];
            propset->rgProperties[j].dwStatus = DBPROPSTATUS_NOTSUPPORTED;
        }

        *prgPropertySets = propset;

        return DB_E_ERRORSOCCURRED;
    }

    propset->guidPropertySet = DBPROPSET_DBINIT;

    no_prop_id = TRUE;
    for (i = 0; i < cPropertyIDSets; i++)
    {
        if (rgPropertyIDSets[i].cPropertyIDs)
        {
            no_prop_id = FALSE;
            break;
        }
    }

    /* If no property ID is specified then return all currently set properties */
    if (no_prop_id)
    {
        propset->cProperties = ARRAY_SIZE(provider->properties);
        propset->rgProperties = CoTaskMemAlloc(propset->cProperties * sizeof(DBPROP));
        for(i = 0; i < ARRAY_SIZE(provider->properties); i++)
        {
            propset->rgProperties[i].dwPropertyID = provider->properties[i].id;
            V_VT(&propset->rgProperties[i].vValue) = VT_EMPTY;
            VariantCopy(&propset->rgProperties[i].vValue, &provider->properties[i].value);
        }

        *prgPropertySets = propset;
        return S_OK;
    }

    /* Return property info for properties in the specified property ID sets */
    for (i=0; i < cPropertyIDSets; i++)
    {
        TRACE("Property id %d (count %ld, set %s)\n", i, rgPropertyIDSets[i].cPropertyIDs,
                debugstr_guid(&rgPropertyIDSets[i].guidPropertySet));

        propset->cProperties = rgPropertyIDSets[i].cPropertyIDs;
        propset->rgProperties = CoTaskMemAlloc(propset->cProperties * sizeof(DBPROP));

        for (j=0; j < propset->cProperties; j++)
        {
            propset->rgProperties[j].dwPropertyID = rgPropertyIDSets[i].rgPropertyIDs[j];

            for(k = 0; k < ARRAY_SIZE(provider->properties); k++)
            {
                if (provider->properties[k].id == rgPropertyIDSets[i].rgPropertyIDs[j])
                {
                    V_VT(&propset->rgProperties[j].vValue) = VT_EMPTY;
                    VariantCopy(&propset->rgProperties[j].vValue, &provider->properties[k].value);
                    break;
                }
            }
        }
    }

    *prgPropertySets = propset;

    return S_OK;
}

static HRESULT WINAPI dbprops_GetPropertyInfo(IDBProperties *iface, ULONG cPropertyIDSets,
            const DBPROPIDSET rgPropertyIDSets[], ULONG *pcPropertyInfoSets,
            DBPROPINFOSET **prgPropertyInfoSets, OLECHAR **ppDescBuffer)
{
    struct msdasql *provider = impl_from_IDBProperties(iface);
    int i;
    DBPROPINFOSET *infoset;
    int size = 1;
    OLECHAR *ptr;

    TRACE("(%p)->(%ld %p %p %p %p)\n", provider, cPropertyIDSets, rgPropertyIDSets, pcPropertyInfoSets,
                prgPropertyInfoSets, ppDescBuffer);

    infoset = CoTaskMemAlloc(sizeof(DBPROPINFOSET));
    memcpy(&infoset->guidPropertySet, &DBPROPSET_DBINIT, sizeof(GUID));
    infoset->cPropertyInfos = ARRAY_SIZE(dbproperties);
    infoset->rgPropertyInfos = CoTaskMemAlloc(sizeof(DBPROPINFO) * ARRAY_SIZE(dbproperties));

    for(i=0; i < ARRAY_SIZE(dbproperties); i++)
    {
        size += lstrlenW(dbproperties[i].name) + 1;
    }

    ptr = *ppDescBuffer = CoTaskMemAlloc(size * sizeof(WCHAR));
    memset(*ppDescBuffer, 0, size * sizeof(WCHAR));

    for(i=0; i < ARRAY_SIZE(dbproperties); i++)
    {
        lstrcpyW(ptr, dbproperties[i].name);
        infoset->rgPropertyInfos[i].pwszDescription = ptr;
        infoset->rgPropertyInfos[i].dwPropertyID =  dbproperties[i].id;
        infoset->rgPropertyInfos[i].dwFlags = DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE;
        infoset->rgPropertyInfos[i].vtType =  dbproperties[i].type;
        V_VT(&infoset->rgPropertyInfos[i].vValues) =  VT_EMPTY;

        ptr += lstrlenW(dbproperties[i].name) + 1;
    }

    *pcPropertyInfoSets = 1;
    *prgPropertyInfoSets = infoset;

    return S_OK;
}

static HRESULT WINAPI dbprops_SetProperties(IDBProperties *iface, ULONG cPropertySets,
            DBPROPSET rgPropertySets[])
{
    struct msdasql *provider = impl_from_IDBProperties(iface);
    int i, j, k;

    TRACE("(%p)->(%ld %p)\n", provider, cPropertySets, rgPropertySets);

    for (i=0; i < cPropertySets; i++)
    {
        for (j=0; j < rgPropertySets[i].cProperties; j++)
        {
            for(k=0; k < ARRAY_SIZE(provider->properties); k++)
            {
                if (provider->properties[k].id == rgPropertySets[i].rgProperties[j].dwPropertyID)
                {
                    TRACE("Found property %d\n", provider->properties[k].id);
                    VariantCopy(&provider->properties[k].value, &rgPropertySets[i].rgProperties[j].vValue);
                    break;
                }
            }
        }
    }

    return S_OK;
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

static HRESULT WINAPI dbinit_QueryInterface(IDBInitialize *iface, REFIID riid, void **ppvObject)
{
    struct msdasql *provider = impl_from_IDBInitialize(iface);

    return IUnknown_QueryInterface(&provider->MSDASQL_iface, riid, ppvObject);
}

static ULONG WINAPI dbinit_AddRef(IDBInitialize *iface)
{
    struct msdasql *provider = impl_from_IDBInitialize(iface);

    return IUnknown_AddRef(&provider->MSDASQL_iface);
}

static ULONG WINAPI dbinit_Release(IDBInitialize *iface)
{
    struct msdasql *provider = impl_from_IDBInitialize(iface);

    return IUnknown_Release(&provider->MSDASQL_iface);
}

static HRESULT WINAPI dbinit_Initialize(IDBInitialize *iface)
{
    struct msdasql *provider = impl_from_IDBInitialize(iface);
    int i, len;
    SQLRETURN ret;
    WCHAR connection[1024], *p = connection, outstr[1024];

    FIXME("%p semi-stub\n", provider);

    *p = 0;
    for(i=0; i < ARRAY_SIZE(provider->properties); i++)
    {
        if (provider->properties[i].id == DBPROP_INIT_DATASOURCE)
        {
            len = swprintf(p, ARRAY_SIZE(connection) - (p - connection),
                           L"DSN=%s;", V_BSTR(&provider->properties[i].value));
            p+= len;
        }
        else if (provider->properties[i].id == DBPROP_INIT_PROVIDERSTRING)
        {
            if (V_VT(&provider->properties[i].value) == VT_BSTR && SysStringLen(V_BSTR(&provider->properties[i].value)) )
            {
                len = swprintf(p, ARRAY_SIZE(connection) - (p - connection),
                               L"%s;", V_BSTR(&provider->properties[i].value));
                p+= len;
            }
        }
    }

    ret = SQLDriverConnectW( provider->hdbc, NULL, connection, wcslen(connection),
                             outstr, ARRAY_SIZE(outstr), NULL, 0);
    TRACE("SQLDriverConnectW ret %d\n", ret);
    if (ret != SQL_SUCCESS)
    {
        dump_sql_diag_records(SQL_HANDLE_DBC, provider->hdbc);

        if (ret != SQL_SUCCESS_WITH_INFO)
           return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI dbinit_Uninitialize(IDBInitialize *iface)
{
    struct msdasql *provider = impl_from_IDBInitialize(iface);

    FIXME("%p stub\n", provider);

    return S_OK;
}


static const struct IDBInitializeVtbl dbinit_vtbl =
{
    dbinit_QueryInterface,
    dbinit_AddRef,
    dbinit_Release,
    dbinit_Initialize,
    dbinit_Uninitialize
};

static HRESULT WINAPI dbsess_QueryInterface(IDBCreateSession *iface, REFIID riid, void **ppvObject)
{
    struct msdasql *provider = impl_from_IDBCreateSession(iface);

    return IUnknown_QueryInterface(&provider->MSDASQL_iface, riid, ppvObject);
}

static ULONG WINAPI dbsess_AddRef(IDBCreateSession *iface)
{
    struct msdasql *provider = impl_from_IDBCreateSession(iface);

    return IUnknown_AddRef(&provider->MSDASQL_iface);
}

static ULONG WINAPI dbsess_Release(IDBCreateSession *iface)
{
    struct msdasql *provider = impl_from_IDBCreateSession(iface);

    return IUnknown_Release(&provider->MSDASQL_iface);
}

static HRESULT WINAPI dbsess_CreateSession(IDBCreateSession *iface, IUnknown *outer, REFIID riid,
        IUnknown **session)
{
    struct msdasql *provider = impl_from_IDBCreateSession(iface);
    HRESULT hr;

    TRACE("%p, outer %p, riid %s, session %p stub\n", provider, outer, debugstr_guid(riid), session);

    if (outer)
        FIXME("outer currently not supported.\n");

    hr = create_db_session(riid, &provider->MSDASQL_iface, provider->hdbc, (void**)session);

    return hr;
}

static const struct IDBCreateSessionVtbl dbsess_vtbl =
{
    dbsess_QueryInterface,
    dbsess_AddRef,
    dbsess_Release,
    dbsess_CreateSession
};

static HRESULT WINAPI persist_QueryInterface(IPersist *iface, REFIID riid, void **ppv)
{
    struct msdasql *provider = impl_from_IPersist( iface );
    return IUnknown_QueryInterface(&provider->MSDASQL_iface, riid, ppv);
}

static ULONG WINAPI persist_AddRef(IPersist *iface)
{
    struct msdasql *provider = impl_from_IPersist( iface );
    return IUnknown_AddRef(&provider->MSDASQL_iface);
}

static ULONG WINAPI persist_Release(IPersist *iface)
{
    struct msdasql *provider = impl_from_IPersist( iface );
    return IUnknown_Release(&provider->MSDASQL_iface);
}

static HRESULT WINAPI persist_GetClassID(IPersist *iface, CLSID *classid)
{
    struct msdasql *provider = impl_from_IPersist( iface );

    TRACE("(%p)->(%p)\n", provider, classid);

    if(!classid)
        return E_INVALIDARG;

    *classid = CLSID_MSDASQL;
    return S_OK;

}

static const IPersistVtbl persistVtbl = {
    persist_QueryInterface,
    persist_AddRef,
    persist_Release,
    persist_GetClassID
};

static HRESULT create_msdasql_provider(REFIID riid, void **ppv)
{
    struct msdasql *provider;
    HRESULT hr;
    int i;

    provider = malloc(sizeof(struct msdasql));
    if (!provider)
        return E_OUTOFMEMORY;

    provider->MSDASQL_iface.lpVtbl = &msdsql_vtbl;
    provider->IDBProperties_iface.lpVtbl = &dbprops_vtbl;
    provider->IDBInitialize_iface.lpVtbl = &dbinit_vtbl;
    provider->IDBCreateSession_iface.lpVtbl = &dbsess_vtbl;
    provider->IPersist_iface.lpVtbl = &persistVtbl;
    provider->ref = 1;

    for(i=0; i < ARRAY_SIZE(dbproperties); i++)
    {
        provider->properties[i].id = dbproperties[i].id;
        VariantInit(&provider->properties[i].value);

        /* Only the follow are initialized to a value */
        switch(dbproperties[i].id)
        {
            case DBPROP_INIT_PROMPT:
                V_VT(&provider->properties[i].value) = dbproperties[i].type;
                V_I2(&provider->properties[i].value) = 4;
                break;
            case DBPROP_INIT_LCID:
                V_VT(&provider->properties[i].value) = dbproperties[i].type;
                V_I4(&provider->properties[i].value) = GetUserDefaultLCID();
                break;
            case DBPROP_INIT_OLEDBSERVICES:
                V_VT(&provider->properties[i].value) = dbproperties[i].type;
                V_I4(&provider->properties[i].value) = -1;
                break;
            default:
                V_VT(&provider->properties[i].value) = VT_EMPTY;
        }
    }

    SQLAllocHandle(SQL_HANDLE_ENV, NULL, &provider->henv );
    if (SQLSetEnvAttr(provider->henv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3_80, 0) == SQL_ERROR)
    {
        WARN("Falling back to SQL_OV_ODBC3\n");
        SQLSetEnvAttr(provider->henv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
    }

    SQLAllocHandle(SQL_HANDLE_DBC, provider->henv, &provider->hdbc);

    hr = IUnknown_QueryInterface(&provider->MSDASQL_iface, riid, ppv);
    IUnknown_Release(&provider->MSDASQL_iface);
    return hr;
}

struct msdasql_enum
{
    ISourcesRowset ISourcesRowset_iface;
    LONG ref;
};

static inline struct msdasql_enum *msdasql_enum_from_ISourcesRowset(ISourcesRowset *iface)
{
    return CONTAINING_RECORD(iface, struct msdasql_enum, ISourcesRowset_iface);
}

static HRESULT WINAPI msdasql_enum_QueryInterface(ISourcesRowset *iface, REFIID riid, void **out)
{
    struct msdasql_enum *enumerator = msdasql_enum_from_ISourcesRowset(iface);

    TRACE("(%p)->(%s %p)\n", enumerator, debugstr_guid(riid), out);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ISourcesRowset ) )
    {
        *out = &enumerator->ISourcesRowset_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI msdasql_enum_AddRef(ISourcesRowset *iface)
{
    struct msdasql_enum *enumerator = msdasql_enum_from_ISourcesRowset(iface);
    ULONG ref = InterlockedIncrement(&enumerator->ref);

    TRACE("(%p) ref=%lu\n", enumerator, ref);

    return ref;
}

static ULONG WINAPI msdasql_enum_Release(ISourcesRowset *iface)
{
    struct msdasql_enum *enumerator = msdasql_enum_from_ISourcesRowset(iface);
    ULONG ref = InterlockedDecrement(&enumerator->ref);

    TRACE("(%p) ref=%lu\n", enumerator, ref);

    if (!ref)
    {
        free(enumerator);
    }

    return ref;
}

struct msdasql_enum_rowset
{
    IRowset IRowset_iface;
    IAccessor IAccessor_iface;
    LONG ref;
};

static inline struct msdasql_enum_rowset *msdasql_rs_from_IRowset(IRowset *iface)
{
    return CONTAINING_RECORD(iface, struct msdasql_enum_rowset, IRowset_iface);
}

static inline struct msdasql_enum_rowset *msdasql_enum_from_IAccessor ( IAccessor *iface )
{
    return CONTAINING_RECORD( iface, struct msdasql_enum_rowset, IAccessor_iface );
}

static HRESULT WINAPI enum_rowset_QueryInterface(IRowset *iface, REFIID riid,  void **ppv)
{
    struct msdasql_enum_rowset *rowset = msdasql_rs_from_IRowset( iface );

    TRACE( "%p, %s, %p\n", rowset, debugstr_guid(riid), ppv );
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) ||
       IsEqualGUID(&IID_IRowset, riid))
    {
        *ppv = &rowset->IRowset_iface;
    }
    else if(IsEqualGUID(&IID_IAccessor, riid))
    {
         *ppv = &rowset->IAccessor_iface;
    }

    if(*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI enum_rowset_AddRef(IRowset *iface)
{
    struct msdasql_enum_rowset *rowset = msdasql_rs_from_IRowset( iface );
    LONG refs = InterlockedIncrement( &rowset->ref );
    TRACE( "%p new refcount %ld\n", rowset, refs );
    return refs;
}

static ULONG WINAPI enum_rowset_Release(IRowset *iface)
{
    struct msdasql_enum_rowset *rowset = msdasql_rs_from_IRowset( iface );
    LONG refs = InterlockedDecrement( &rowset->ref );
    TRACE( "%p new refcount %ld\n", rowset, refs );
    if (!refs)
    {
        TRACE( "destroying %p\n", rowset );
        free( rowset );
    }
    return refs;
}

static HRESULT WINAPI enum_rowset_AddRefRows(IRowset *iface, DBCOUNTITEM count,
        const HROW rows[], DBREFCOUNT ref_counts[], DBROWSTATUS status[])
{
    struct msdasql_enum_rowset *rowset = msdasql_rs_from_IRowset( iface );

    FIXME("%p, %Iu, %p, %p, %p\n", rowset, count, rows, ref_counts, status);

    return E_NOTIMPL;
}

static HRESULT WINAPI enum_rowset_GetData(IRowset *iface, HROW row, HACCESSOR accessor, void *data)
{
    struct msdasql_enum_rowset *rowset = msdasql_rs_from_IRowset( iface );

    FIXME("%p, %Iu, %Iu, %p\n", rowset, row, accessor, data);

    return E_NOTIMPL;
}

static HRESULT WINAPI enum_rowset_GetNextRows(IRowset *iface, HCHAPTER reserved, DBROWOFFSET offset,
        DBROWCOUNT count, DBCOUNTITEM *obtained, HROW **rows)
{
    struct msdasql_enum_rowset *rowset = msdasql_rs_from_IRowset( iface );

    FIXME("%p, %Iu, %Iu, %Iu, %p, %p\n", rowset, reserved, offset, count, obtained, rows);

    if (!obtained || !rows)
        return E_INVALIDARG;

    *obtained = 0;

    if (!count)
        return S_OK;

    return DB_S_ENDOFROWSET;
}

static HRESULT WINAPI enum_rowset_ReleaseRows(IRowset *iface, DBCOUNTITEM count,
        const HROW rows[], DBROWOPTIONS options[], DBREFCOUNT ref_counts[], DBROWSTATUS status[])
{
    struct msdasql_enum_rowset *rowset = msdasql_rs_from_IRowset( iface );

    FIXME("%p, %Iu, %p, %p, %p, %p\n", rowset, count, rows, options, ref_counts, status);

    return S_OK;
}

static HRESULT WINAPI enum_rowset_RestartPosition(IRowset *iface, HCHAPTER reserved)
{
    struct msdasql_enum_rowset *rowset = msdasql_rs_from_IRowset( iface );

    FIXME("%p, %Iu\n", rowset, reserved);

    return S_OK;
}

static const struct IRowsetVtbl enum_rowset_vtbl =
{
    enum_rowset_QueryInterface,
    enum_rowset_AddRef,
    enum_rowset_Release,
    enum_rowset_AddRefRows,
    enum_rowset_GetData,
    enum_rowset_GetNextRows,
    enum_rowset_ReleaseRows,
    enum_rowset_RestartPosition
};

static HRESULT WINAPI enum_rs_accessor_QueryInterface(IAccessor *iface, REFIID riid, void **out)
{
    struct msdasql_enum_rowset *rowset = msdasql_enum_from_IAccessor( iface );
    return IRowset_QueryInterface(&rowset->IRowset_iface, riid, out);
}

static ULONG WINAPI enum_rs_accessor_AddRef(IAccessor *iface)
{
    struct msdasql_enum_rowset *rowset = msdasql_enum_from_IAccessor( iface );
    return IRowset_AddRef(&rowset->IRowset_iface);
}

static ULONG  WINAPI enum_rs_accessor_Release(IAccessor *iface)
{
    struct msdasql_enum_rowset *rowset = msdasql_enum_from_IAccessor( iface );
    return IRowset_Release(&rowset->IRowset_iface);
}

static HRESULT WINAPI enum_rs_accessor_AddRefAccessor(IAccessor *iface, HACCESSOR accessor, DBREFCOUNT *count)
{
    struct msdasql_enum_rowset *rowset = msdasql_enum_from_IAccessor( iface );
    FIXME("%p, %Iu, %p\n", rowset, accessor, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI enum_rs_accessor_CreateAccessor(IAccessor *iface, DBACCESSORFLAGS flags,
        DBCOUNTITEM count, const DBBINDING bindings[], DBLENGTH row_size, HACCESSOR *accessor,
        DBBINDSTATUS status[])
{
    struct msdasql_enum_rowset *rowset = msdasql_enum_from_IAccessor( iface );

    FIXME("%p 0x%08lx, %Iu, %p, %Iu, %p, %p\n", rowset, flags, count, bindings, row_size, accessor, status);

    if (accessor)
        *accessor = 0xdeadbeef;

    return S_OK;
}

static HRESULT WINAPI enum_rs_accessor_GetBindings(IAccessor *iface, HACCESSOR accessor,
        DBACCESSORFLAGS *flags, DBCOUNTITEM *count, DBBINDING **bindings)
{
    struct msdasql_enum_rowset *rowset = msdasql_enum_from_IAccessor( iface );
    FIXME("%p, %Iu, %p, %p, %p\n", rowset, accessor, flags, count, bindings);
    return E_NOTIMPL;
}

static HRESULT WINAPI enum_rs_accessor_ReleaseAccessor(IAccessor *iface, HACCESSOR accessor, DBREFCOUNT *count)
{
    struct msdasql_enum_rowset *rowset = msdasql_enum_from_IAccessor( iface );

    FIXME("%p, %Iu, %p\n", rowset, accessor, count);

    if (count)
        *count = 0;

    return S_OK;
}

struct IAccessorVtbl enum_accessor_vtbl =
{
    enum_rs_accessor_QueryInterface,
    enum_rs_accessor_AddRef,
    enum_rs_accessor_Release,
    enum_rs_accessor_AddRefAccessor,
    enum_rs_accessor_CreateAccessor,
    enum_rs_accessor_GetBindings,
    enum_rs_accessor_ReleaseAccessor
};

static HRESULT WINAPI msdasql_enum_GetSourcesRowset(ISourcesRowset *iface, IUnknown *outer, REFIID riid, ULONG sets,
                DBPROPSET properties[], IUnknown **rowset)
{
    struct msdasql_enum *enumerator = msdasql_enum_from_ISourcesRowset(iface);
    struct msdasql_enum_rowset *enum_rs;
    HRESULT hr;

    TRACE("(%p) %p, %s, %lu, %p, %p\n", enumerator, outer, debugstr_guid(riid), sets, properties, rowset);

    enum_rs = malloc(sizeof(*enum_rs));
    enum_rs->IRowset_iface.lpVtbl = &enum_rowset_vtbl;
    enum_rs->IAccessor_iface.lpVtbl = &enum_accessor_vtbl;
    enum_rs->ref = 1;

    hr = IRowset_QueryInterface(&enum_rs->IRowset_iface, riid, (void**)rowset);
    IRowset_Release(&enum_rs->IRowset_iface);
    return hr;
}

static const ISourcesRowsetVtbl msdsqlenum_vtbl =
{
    msdasql_enum_QueryInterface,
    msdasql_enum_AddRef,
    msdasql_enum_Release,
    msdasql_enum_GetSourcesRowset
};

static HRESULT create_msdasql_enumerator(REFIID riid, void **ppv)
{
    struct msdasql_enum *enumerator;
    HRESULT hr;

    enumerator = malloc(sizeof(*enumerator));
    if (!enumerator)
        return E_OUTOFMEMORY;

    enumerator->ISourcesRowset_iface.lpVtbl = &msdsqlenum_vtbl;
    enumerator->ref = 1;

    hr = IUnknown_QueryInterface(&enumerator->ISourcesRowset_iface, riid, ppv);
    IUnknown_Release(&enumerator->ISourcesRowset_iface);
    return hr;
}
