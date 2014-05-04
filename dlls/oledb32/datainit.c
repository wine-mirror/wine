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
#include "initguid.h"
#include "oledb_private.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

DEFINE_GUID(DBPROPSET_DBINIT, 0xc8b522bc, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);

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
        heap_free(This);

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

static HRESULT create_db_init(IUnknown **obj)
{
    dbinit *This;

    TRACE("()\n");

    *obj = NULL;

    This = heap_alloc(sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->IDBInitialize_iface.lpVtbl = &dbinit_vtbl;
    This->IDBProperties_iface.lpVtbl = &dbprops_vtbl;
    This->ref = 1;

    *obj = (IUnknown*)&This->IDBInitialize_iface;

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
        heap_free(This);

    return ref;
}

static void free_dbpropset(ULONG count, DBPROPSET *propset)
{
    ULONG i;

    for (i = 0; i < count; i++)
    {
        ULONG p;

        for (p = 0; p < propset[i].cProperties; p++)
            VariantClear(&propset[i].rgProperties[p].vValue);

        CoTaskMemFree(propset[i].rgProperties);
    }
    CoTaskMemFree(propset);
}

struct dbproperty {
    const WCHAR *name;
    DBPROPID id;
    DBPROPOPTIONS options;
    VARTYPE type;
};

static const WCHAR asyncW[]  = {'A','s','y','n','c','h','r','o','n','o','u','s',' ','P','r','o','c','e','s','s','i','n','g',0};
static const WCHAR bindW[]  = {'B','i','n','d',' ','F','l','a','g','s',0};
static const WCHAR cacheW[]  = {'C','a','c','h','e',' ','A','u','t','h','e','n','i','c','a','t','i','o','n',0};
static const WCHAR conn_timeout[] = {'C','o','n','n','e','c','t',' ','T','i','m','e','o','u','t',0};
static const WCHAR datasourceW[] = {'D','a','t','a',' ','S','o','u','r','c','e',0};
static const WCHAR encryptW[] = {'E','n','c','r','y','p','t',' ','P','a','s','s','w','o','r','d',0};
static const WCHAR extendedW[] = {'E','x','t','e','n','d','e','d',' ','P','r','o','p','e','r','t','i','e','s',0};
static const WCHAR gen_timeout[] = {'G','e','n','e','r','a','l',' ','T','i','m','e','o','u','t',0};
static const WCHAR impersonW[] = {'I','m','p','e','r','s','o','n','a','t','i','o','n',' ','L','e','v','e','l',0};
static const WCHAR initcatW[] = {'I','n','i','t','i','a','l',' ','C','a','t','a','l','o','g',0};
static const WCHAR integratedW[] = {'I','n','t','e','g','r','a','t','e','d',' ','S','e','c','u','r','i','t','y',0};
static const WCHAR localeIDW[] = {'L','o','c','a','l','e',' ','I','d','e','n','t','i','f','i','e','r',0};
static const WCHAR locationW[] = {'L','o','c','a','t','i','o','n',0};
static const WCHAR lockownerW[] = {'L','o','c','k',' ','O','w','n','e','r',0};
static const WCHAR maskpassW[] = {'M','a','s','k',' ','P','a','s','s','w','o','r','d',0};
static const WCHAR modeW[] = {'M','o','d','e',0};
static const WCHAR oledbservW[] = {'O','L','E',' ','D','B',' ','S','e','r','v','i','c','i','e','s',0};
static const WCHAR passwordW[] = {'P','a','s','s','w','o','r','d',0};
static const WCHAR persistW[]  = {'P','e','r','s','i','s','t',' ','S','e','c','u','r','i','t','y',' ','I','n','f','o',0};
static const WCHAR persistEncW[]  = {'P','e','r','s','i','s','t',' ','E','n','c','r','y','p','t','e','d',0};
static const WCHAR promptW[] = {'P','r','o','m','p','t',0};
static const WCHAR protectW[]  = {'P','r','o','t','e','c','t','i','o','n',' ','l','e','v','e','l',0};
static const WCHAR useridW[] = {'U','s','e','r',' ','I','D',0};
static const WCHAR winhandleW[] = {'W','i','n','d','o','w',' ','H','a','n','d','l','e',0};

static const struct dbproperty dbproperties[] = {
    { asyncW,      DBPROP_INIT_ASYNCH,                     DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { bindW,       DBPROP_INIT_BINDFLAGS,                  DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { cacheW,      DBPROP_AUTH_CACHE_AUTHINFO,             DBPROPOPTIONS_OPTIONAL, VT_BOOL },
    { conn_timeout,DBPROP_INIT_TIMEOUT,                    DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { datasourceW, DBPROP_INIT_DATASOURCE,                 DBPROPOPTIONS_REQUIRED, VT_BSTR },
    { extendedW,   DBPROP_INIT_PROVIDERSTRING,             DBPROPOPTIONS_REQUIRED, VT_BSTR },
    { encryptW,    DBPROP_AUTH_ENCRYPT_PASSWORD,           DBPROPOPTIONS_REQUIRED, VT_BOOL },
    { gen_timeout, DBPROP_INIT_GENERALTIMEOUT,             DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { impersonW,   DBPROP_INIT_IMPERSONATION_LEVEL,        DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { initcatW,    DBPROP_CATALOGLOCATION,                 DBPROPOPTIONS_OPTIONAL, VT_BSTR },
    { integratedW, DBPROP_AUTH_INTEGRATED,                 DBPROPOPTIONS_OPTIONAL, VT_BSTR },
    { localeIDW,   DBPROP_INIT_LCID,                       DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { locationW,   DBPROP_INIT_LOCATION,                   DBPROPOPTIONS_OPTIONAL, VT_BSTR },
    { lockownerW,  DBPROP_INIT_LOCKOWNER,                  DBPROPOPTIONS_OPTIONAL, VT_BSTR },
    { maskpassW,   DBPROP_AUTH_MASK_PASSWORD,              DBPROPOPTIONS_OPTIONAL, VT_BOOL },
    { modeW,       DBPROP_INIT_MODE,                       DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { oledbservW,  DBPROP_INIT_OLEDBSERVICES,              DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { passwordW,   DBPROP_AUTH_PASSWORD,                   DBPROPOPTIONS_OPTIONAL, VT_BSTR },
    { persistEncW, DBPROP_AUTH_PERSIST_ENCRYPTED,          DBPROPOPTIONS_OPTIONAL, VT_BOOL },
    { persistW,    DBPROP_AUTH_PERSIST_SENSITIVE_AUTHINFO, DBPROPOPTIONS_OPTIONAL, VT_BOOL },
    { promptW,     DBPROP_INIT_PROMPT,                     DBPROPOPTIONS_OPTIONAL, VT_I2 },
    { protectW,    DBPROP_INIT_PROTECTION_LEVEL,           DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { useridW,     DBPROP_AUTH_USERID,                     DBPROPOPTIONS_OPTIONAL, VT_BSTR },
#ifndef _WIN64
    { winhandleW,  DBPROP_INIT_HWND,                       DBPROPOPTIONS_OPTIONAL, VT_I4 },
#else
    { winhandleW,  DBPROP_INIT_HWND,                       DBPROPOPTIONS_OPTIONAL, VT_I8 },
#endif
};


static HRESULT set_dbpropset(BSTR name, BSTR value, DBPROPSET **propset)
{
    VARIANT src, dest;
    int min, max, n;
    HRESULT hr;

    min = 0;
    max = sizeof(dbproperties)/sizeof(struct dbproperty) - 1;

    while (min <= max)
    {
        int r;

        n = (min+max)/2;

        r = strcmpiW(dbproperties[n].name, name);
        if (!r)
            break;

        if (r < 0)
            min = n+1;
        else
            max = n-1;
    }

    if (min > max)
    {
        *propset = NULL;
        FIXME("unsupported property %s\n", debugstr_w(name));
        return E_FAIL;
    }

    VariantInit(&dest);

    V_VT(&src) = VT_BSTR;
    V_BSTR(&src) = value;

    hr = VariantChangeType(&dest, &src, 0, dbproperties[n].type);
    if (FAILED(hr))
    {
        ERR("failed to init property %s value as type %d\n", debugstr_w(name), dbproperties[n].type);
        return hr;
    }

    *propset = CoTaskMemAlloc(sizeof(DBPROPSET));
    if (!*propset)
    {
        VariantClear(&dest);
        return E_OUTOFMEMORY;
    }

    (*propset)->rgProperties = CoTaskMemAlloc(sizeof(DBPROP));
    if (!(*propset)->rgProperties)
    {
        VariantClear(&dest);
        CoTaskMemFree(*propset);
        return E_OUTOFMEMORY;
    }

    (*propset)->cProperties = 1;
    (*propset)->guidPropertySet = DBPROPSET_DBINIT;
    (*propset)->rgProperties[0].dwPropertyID = dbproperties[n].id;
    (*propset)->rgProperties[0].dwOptions = dbproperties[n].options;
    (*propset)->rgProperties[0].dwStatus = 0;
    memset(&(*propset)->rgProperties[0].colid, 0, sizeof(DBID));
    (*propset)->rgProperties[0].vValue = dest;

    return S_OK;
}

/*** IDataInitialize methods ***/
static HRESULT WINAPI datainit_GetDataSource(IDataInitialize *iface, IUnknown *outer, DWORD clsctx,
                                LPWSTR initstring, REFIID riid, IUnknown **datasource)
{
    static const WCHAR providerW[] = {'P','r','o','v','i','d','e','r','=',0};
    static const WCHAR msdasqlW[] = {'M','S','D','A','S','Q','L',0};
    datainit *This = impl_from_IDataInitialize(iface);
    IDBProperties *dbprops;
    DBPROPSET *propset;
    WCHAR *prov = NULL;
    CLSID provclsid;
    HRESULT hr;

    TRACE("(%p)->(%p 0x%x %s %s %p)\n", This, outer, clsctx, debugstr_w(initstring), debugstr_guid(riid), datasource);

    /* first get provider name */
    provclsid = IID_NULL;
    if (initstring && (prov = strstrW(initstring, providerW)))
    {
        WCHAR *start, *progid;
        int len;

        prov += sizeof(providerW)/sizeof(WCHAR)-1;
        start = prov;
        while (*prov && *prov != ';')
            ++prov;
        TRACE("got provider %s\n", debugstr_wn(start, prov-start));

        len = prov - start;
        progid = CoTaskMemAlloc((len+1)*sizeof(WCHAR));
        if (!progid) return E_OUTOFMEMORY;

        memcpy(progid, start, len*sizeof(WCHAR));
        progid[len] = 0;

        hr = CLSIDFromProgID(progid, &provclsid);
        CoTaskMemFree(progid);
        if (FAILED(hr))
        {
            ERR("provider %s not registered\n", debugstr_wn(start, prov-start));
            return hr;
        }
    }
    else
    {
        hr = CLSIDFromProgID(msdasqlW, &provclsid);
        if (FAILED(hr))
            ERR("ODBC provider for OLE DB not registered\n");
    }

    /* check for provider mismatch if it was specified in init string */
    if (*datasource && prov)
    {
        DBPROPIDSET propidset;
        enum DBPROPENUM prop;
        CLSID initprov;
        ULONG count;

        hr = IUnknown_QueryInterface(*datasource, &IID_IDBProperties, (void**)&dbprops);
        if (FAILED(hr))
        {
            WARN("provider doesn't support IDBProperties\n");
            return hr;
        }

        prop = DBPROP_INIT_DATASOURCE;
        propidset.rgPropertyIDs = &prop;
        propidset.cPropertyIDs = 1;
        propidset.guidPropertySet = DBPROPSET_DBINIT;
        count = 0;
        propset = NULL;
        hr = IDBProperties_GetProperties(dbprops, 1, &propidset, &count, &propset);
        IDBProperties_Release(dbprops);
        if (FAILED(hr))
        {
            WARN("GetProperties failed for datasource, 0x%08x\n", hr);
            return hr;
        }

        TRACE("initial data source provider %s\n", debugstr_w(V_BSTR(&propset->rgProperties[0].vValue)));
        initprov = IID_NULL;
        CLSIDFromProgID(V_BSTR(&propset->rgProperties[0].vValue), &initprov);
        free_dbpropset(count, propset);

        if (!IsEqualIID(&provclsid, &initprov)) return DB_E_MISMATCHEDPROVIDER;
    }

    if (!*datasource)
    {
        if (!IsEqualIID(&provclsid, &IID_NULL))
            hr = CoCreateInstance(&provclsid, outer, clsctx, riid, (void**)datasource);

        if (FAILED(hr) && IsEqualIID(riid, &IID_IDBInitialize))
            hr = create_db_init(datasource);
    }

    /* now set properties */
    if (initstring)
    {
        WCHAR *eq, *start;

        hr = IUnknown_QueryInterface(*datasource, &IID_IDBProperties, (void**)&dbprops);
        if (FAILED(hr))
        {
            WARN("provider doesn't support IDBProperties\n");
            return hr;
        }

        start = initstring;
        while (start && (eq = strchrW(start, '=')))
        {
            static const WCHAR providerW[] = {'P','r','o','v','i','d','e','r',0};
            WCHAR *scol = strchrW(eq+1, ';');
            BSTR value, name;

            name = SysAllocStringLen(start, eq - start);
            /* skip equal sign to get value */
            eq++;
            value = SysAllocStringLen(eq, scol ? scol - eq : -1);

            /* skip semicolon if present */
            if (scol) scol++;
            start = scol;

            if (!strcmpW(name, providerW))
            {
                SysFreeString(name);
                SysFreeString(value);
                continue;
            }

            TRACE("property (name=%s, value=%s)\n", debugstr_w(name), debugstr_w(value));

            hr = set_dbpropset(name, value, &propset);
            SysFreeString(name);
            SysFreeString(value);
            if (FAILED(hr)) break;

            hr = IDBProperties_SetProperties(dbprops, 1, propset);
            free_dbpropset(1, propset);
            TRACE("provider ret 0x%08x\n", hr);
        }

        IDBProperties_Release(dbprops);
    }

    return hr;
}

/* returns character length of string representation */
static int get_propvalue_length(DBPROP *prop)
{
    VARIANT str;
    HRESULT hr;

    if (V_VT(&prop->vValue) == VT_BSTR) return SysStringLen(V_BSTR(&prop->vValue));

    VariantInit(&str);
    hr = VariantChangeType(&str, &prop->vValue, 0, VT_BSTR);
    if (hr == S_OK)
    {
        int len = SysStringLen(V_BSTR(&str));
        VariantClear(&str);
        return len;
    }

    return 0;
}

static void write_propvalue_str(WCHAR *str, DBPROP *prop)
{
    VARIANT *v = &prop->vValue;
    VARIANT vstr;
    HRESULT hr;

    if (V_VT(v) == VT_BSTR)
    {
        strcatW(str, V_BSTR(v));
        return;
    }

    VariantInit(&vstr);
    hr = VariantChangeType(&vstr, v, VARIANT_ALPHABOOL, VT_BSTR);
    if (hr == S_OK)
    {
        strcatW(str, V_BSTR(&vstr));
        VariantClear(&vstr);
    }
}

static WCHAR *get_propinfo_descr(DBPROP *prop, DBPROPINFOSET *propinfoset)
{
    ULONG i;

    for (i = 0; i < propinfoset->cPropertyInfos; i++)
        if (propinfoset->rgPropertyInfos[i].dwPropertyID == prop->dwPropertyID)
            return propinfoset->rgPropertyInfos[i].pwszDescription;

    return NULL;
}

static void free_dbpropinfoset(ULONG count, DBPROPINFOSET *propinfoset)
{
    ULONG i;

    for (i = 0; i < count; i++)
    {
        ULONG p;

        for (p = 0; p < propinfoset[i].cPropertyInfos; p++)
            VariantClear(&propinfoset[i].rgPropertyInfos[p].vValues);

        CoTaskMemFree(propinfoset[i].rgPropertyInfos);
    }
    CoTaskMemFree(propinfoset);
}

static HRESULT WINAPI datainit_GetInitializationString(IDataInitialize *iface, IUnknown *datasource,
                                boolean include_pass, LPWSTR *init_string)
{
    static const WCHAR provW[] = {'P','r','o','v','i','d','e','r','=',0};
    static const WCHAR colW[] = {';',0};
    datainit *This = impl_from_IDataInitialize(iface);
    DBPROPINFOSET *propinfoset;
    IDBProperties *props;
    DBPROPIDSET propidset;
    ULONG count, infocount;
    WCHAR *progid, *desc;
    DBPROPSET *propset;
    IPersist *persist;
    HRESULT hr;
    CLSID clsid;
    ULONG i, len;

    TRACE("(%p)->(%p %d %p)\n", This, datasource, include_pass, init_string);

    /* IPersist support is mandatory for data sources */
    hr = IUnknown_QueryInterface(datasource, &IID_IPersist, (void**)&persist);
    if (FAILED(hr)) return hr;

    memset(&clsid, 0, sizeof(clsid));
    hr = IPersist_GetClassID(persist, &clsid);
    IPersist_Release(persist);
    if (FAILED(hr)) return hr;

    progid = NULL;
    ProgIDFromCLSID(&clsid, &progid);
    TRACE("clsid=%s, progid=%s\n", debugstr_guid(&clsid), debugstr_w(progid));

    /* now get initialization properties */
    hr = IUnknown_QueryInterface(datasource, &IID_IDBProperties, (void**)&props);
    if (FAILED(hr))
    {
        WARN("IDBProperties not supported\n");
        CoTaskMemFree(progid);
        return hr;
    }

    propidset.rgPropertyIDs = NULL;
    propidset.cPropertyIDs = 0;
    propidset.guidPropertySet = DBPROPSET_DBINIT;
    propset = NULL;
    count = 0;
    hr = IDBProperties_GetProperties(props, 1, &propidset, &count, &propset);
    if (FAILED(hr))
    {
        WARN("failed to get data source properties, 0x%08x\n", hr);
        CoTaskMemFree(progid);
        return hr;
    }

    infocount = 0;
    IDBProperties_GetPropertyInfo(props, 1, &propidset, &infocount, &propinfoset, &desc);
    IDBProperties_Release(props);

    /* check if we need to skip password */
    len = strlenW(progid) + strlenW(provW) + 1; /* including ';' */
    for (i = 0; i < count; i++)
    {
        WCHAR *descr = get_propinfo_descr(&propset->rgProperties[i], propinfoset);
        if (descr)
        {
            /* include '=' and ';' */
            len += strlenW(descr) + 2;
            len += get_propvalue_length(&propset->rgProperties[i]);
        }

        if ((propset->rgProperties[i].dwPropertyID == DBPROP_AUTH_PERSIST_SENSITIVE_AUTHINFO) &&
            (V_BOOL(&propset->rgProperties[i].vValue) == VARIANT_FALSE))
           include_pass = FALSE;
    }

    len *= sizeof(WCHAR);
    *init_string = CoTaskMemAlloc(len);
    *init_string[0] = 0;

    /* provider name */
    strcatW(*init_string, provW);
    strcatW(*init_string, progid);
    strcatW(*init_string, colW);
    CoTaskMemFree(progid);

    for (i = 0; i < count; i++)
    {
        WCHAR *descr;

        if (!include_pass && propset->rgProperties[i].dwPropertyID == DBPROP_AUTH_PASSWORD) continue;

        descr = get_propinfo_descr(&propset->rgProperties[i], propinfoset);
        if (descr)
        {
            static const WCHAR eqW[] = {'=',0};
            strcatW(*init_string, descr);
            strcatW(*init_string, eqW);
            write_propvalue_str(*init_string, &propset->rgProperties[i]);
            strcatW(*init_string, colW);
        }
    }

    free_dbpropset(count, propset);
    free_dbpropinfoset(infocount, propinfoset);
    CoTaskMemFree(desc);

    if (!include_pass)
        TRACE("%s\n", debugstr_w(*init_string));
    return S_OK;
}

static HRESULT WINAPI datainit_CreateDBInstance(IDataInitialize *iface, REFCLSID provider,
                                IUnknown *outer, DWORD clsctx, LPWSTR reserved, REFIID riid,
                                IUnknown **datasource)
{
    datainit *This = impl_from_IDataInitialize(iface);

    TRACE("(%p)->(%s %p 0x%08x %s %s %p)\n", This, debugstr_guid(provider), outer, clsctx, debugstr_w(reserved),
        debugstr_guid(riid), datasource);

    return CoCreateInstance(provider, outer, clsctx, riid, (void**)datasource);
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

    This = heap_alloc(sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->IDataInitialize_iface.lpVtbl = &datainit_vtbl;
    This->ref = 1;

    *obj = &This->IDataInitialize_iface;

    return S_OK;
}
