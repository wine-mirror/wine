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

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "ole2.h"
#include "msdasc.h"
#include "oledberr.h"
#include "initguid.h"
#include "oledb_private.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

DEFINE_GUID(DBGUID_SESSION,    0xc8b522f5, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBGUID_ROWSET,     0xc8b522f6, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBGUID_ROW,        0xc8b522f7, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBGUID_STREAM,     0xc8b522f9, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);

DEFINE_GUID(DBPROPSET_COLUMN,            0xc8b522b9, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_DATASOURCE,        0xc8b522ba, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_DATASOURCEINFO,    0xc8b522bb, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_DBINIT,            0xc8b522bc, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_INDEX,             0xc8b522bd, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_ROWSET,            0xc8b522be, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_TABLE,             0xc8b522bf, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_DATASOURCEALL,     0xc8b522c0, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_DATASOURCEINFOALL, 0xc8b522c1, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_ROWSETALL,         0xc8b522c2, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_SESSION,           0xc8b522c6, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_SESSIONALL,        0xc8b522c7, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_DBINITALL,         0xc8b522ca, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(DBPROPSET_PROPERTIESINERROR, 0xc8b522d4, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);

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

    FIXME("(%p)->(%ld %p %p %p)\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets);

    return E_NOTIMPL;
}

static HRESULT WINAPI dbprops_GetPropertyInfo(IDBProperties *iface, ULONG cPropertyIDSets,
            const DBPROPIDSET rgPropertyIDSets[], ULONG *pcPropertyInfoSets,
            DBPROPINFOSET **prgPropertyInfoSets, OLECHAR **ppDescBuffer)
{
    dbinit *This = impl_from_IDBProperties(iface);

    FIXME("(%p)->(%ld %p %p %p %p)\n", This, cPropertyIDSets, rgPropertyIDSets, pcPropertyInfoSets,
                prgPropertyInfoSets, ppDescBuffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI dbprops_SetProperties(IDBProperties *iface, ULONG cPropertySets,
            DBPROPSET rgPropertySets[])
{
    dbinit *This = impl_from_IDBProperties(iface);

    FIXME("(%p)->(%ld %p)\n", This, cPropertySets, rgPropertySets);

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
        free(This);

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

    This = malloc(sizeof(*This));
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
        free(This);

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
    static const struct mode_propval mode_propvals[] =
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
    WCHAR mode[64];
    WCHAR *pos = NULL;
    const WCHAR *lastpos = src;

    V_VT(dest) = VT_I4;
    V_I4(dest) = 0;

    pos = wcschr(src, '|');
    while (pos != NULL)
    {
        lstrcpynW(mode, lastpos, pos - lastpos + 1);

        if (!(prop = bsearch(mode, mode_propvals, ARRAY_SIZE(mode_propvals),
                            sizeof(struct mode_propval), dbmodeprop_compare)))
            goto done;

        V_I4(dest) |= prop->value;

        lastpos = pos + 1;
        pos = wcschr(lastpos, '|');
    }

    if (lastpos)
    {
        lstrcpyW(mode, lastpos);
        if (!(prop = bsearch(mode, mode_propvals, ARRAY_SIZE(mode_propvals),
                            sizeof(struct mode_propval), dbmodeprop_compare)))
            goto done;

        V_I4(dest) |= prop->value;
    }

    TRACE("%s = %#lx\n", debugstr_w(src), V_I4(dest));
    return S_OK;

done:
    FIXME("Failed to parse Mode (%s)\n", debugstr_w(src));

    return E_FAIL;
}

static const struct dbproperty dbproperties[] =
{
    { L"Asynchronous Processing", DBPROP_INIT_ASYNCH,                     DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { L"Bind Flags",              DBPROP_INIT_BINDFLAGS,                  DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { L"Cache Authentication",    DBPROP_AUTH_CACHE_AUTHINFO,             DBPROPOPTIONS_OPTIONAL, VT_BOOL },
    { L"Connect Timeout",         DBPROP_INIT_TIMEOUT,                    DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { L"Data Source",             DBPROP_INIT_DATASOURCE,                 DBPROPOPTIONS_REQUIRED, VT_BSTR },
    { L"Extended Properties",     DBPROP_INIT_PROVIDERSTRING,             DBPROPOPTIONS_REQUIRED, VT_BSTR },
    { L"Encrypt Password",        DBPROP_AUTH_ENCRYPT_PASSWORD,           DBPROPOPTIONS_REQUIRED, VT_BOOL },
    { L"General Timeout",         DBPROP_INIT_GENERALTIMEOUT,             DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { L"Impersonation Level",     DBPROP_INIT_IMPERSONATION_LEVEL,        DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { L"Initial Catalog",         DBPROP_INIT_CATALOG,                    DBPROPOPTIONS_REQUIRED, VT_BSTR },
    { L"Integrated Security",     DBPROP_AUTH_INTEGRATED,                 DBPROPOPTIONS_OPTIONAL, VT_BSTR },
    { L"Locale Identifier",       DBPROP_INIT_LCID,                       DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { L"Location",                DBPROP_INIT_LOCATION,                   DBPROPOPTIONS_OPTIONAL, VT_BSTR },
    { L"Lock Owner",              DBPROP_INIT_LOCKOWNER,                  DBPROPOPTIONS_OPTIONAL, VT_BSTR },
    { L"Mask Password",           DBPROP_AUTH_MASK_PASSWORD,              DBPROPOPTIONS_OPTIONAL, VT_BOOL },
    { L"Mode",                    DBPROP_INIT_MODE,                       DBPROPOPTIONS_OPTIONAL, VT_I4, convert_dbproperty_mode },
    { L"OLE DB Services",         DBPROP_INIT_OLEDBSERVICES,              DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { L"Password",                DBPROP_AUTH_PASSWORD,                   DBPROPOPTIONS_OPTIONAL, VT_BSTR },
    { L"Persist Encrypted",       DBPROP_AUTH_PERSIST_ENCRYPTED,          DBPROPOPTIONS_OPTIONAL, VT_BOOL },
    { L"Persist Security Info",   DBPROP_AUTH_PERSIST_SENSITIVE_AUTHINFO, DBPROPOPTIONS_OPTIONAL, VT_BOOL },
    { L"Prompt",                  DBPROP_INIT_PROMPT,                     DBPROPOPTIONS_OPTIONAL, VT_I2 },
    { L"Protection level",        DBPROP_INIT_PROTECTION_LEVEL,           DBPROPOPTIONS_OPTIONAL, VT_I4 },
    { L"User ID",                 DBPROP_AUTH_USERID,                     DBPROPOPTIONS_OPTIONAL, VT_BSTR },
#ifndef _WIN64
    { L"Window Handle",           DBPROP_INIT_HWND,                       DBPROPOPTIONS_OPTIONAL, VT_I4 },
#else
    { L"Window Handle",           DBPROP_INIT_HWND,                       DBPROPOPTIONS_OPTIONAL, VT_I8 },
#endif
};

struct dbprop_pair
{
    struct list entry;
    BSTR name;
    BSTR value;
};

struct dbprops
{
    struct list props;
    unsigned int count;
};

/* name/value strings are reused, so they shouldn't be freed after this call */
static HRESULT add_dbprop_to_list(struct dbprops *props, BSTR name, BSTR value)
{
    struct dbprop_pair *pair;

    pair = malloc(sizeof(*pair));
    if (!pair)
        return E_OUTOFMEMORY;

    pair->name = name;
    pair->value = value;
    list_add_tail(&props->props, &pair->entry);
    props->count++;
    return S_OK;
}

static void free_dbprop_list(struct dbprops *props)
{
    struct dbprop_pair *p, *p2;
    LIST_FOR_EACH_ENTRY_SAFE(p, p2, &props->props, struct dbprop_pair, entry)
    {
        list_remove(&p->entry);
        SysFreeString(p->name);
        SysFreeString(p->value);
        free(p);
    }
}

static HRESULT parse_init_string(const WCHAR *initstring, struct dbprops *props)
{
    const WCHAR *start;
    WCHAR *eq;
    HRESULT hr = S_OK;

    list_init(&props->props);
    props->count = 0;

    start = initstring;
    while (start && (eq = wcschr(start, '=')))
    {
        WCHAR *delim, quote;
        BSTR value, name;

        while (iswspace(*start)) start++;
        name = SysAllocStringLen(start, eq - start);
        /* skip equal sign to get value */
        eq++;

        quote = (*eq == '"' || *eq == '\'') ? *eq : 0;
        if (quote)
        {
            /* for quoted value string, skip opening mark, look for terminating one */
            eq++;
            delim = wcschr(eq, quote);
        }
        else
            delim = wcschr(eq, ';');

        if (delim)
            value = SysAllocStringLen(eq, delim - eq);
        else
            value = SysAllocString(eq);

        /* skip semicolon if present */
        if (delim)
        {
            if (*delim == quote)
               delim++;
            if (*delim == ';')
               delim++;
        }
        start = delim;

        if (!wcsicmp(name, L"Provider"))
        {
            SysFreeString(name);
            SysFreeString(value);
            continue;
        }

        TRACE("property (name=%s, value=%s)\n", debugstr_w(name), debugstr_w(value));

        hr = add_dbprop_to_list(props, name, value);
        if (FAILED(hr))
        {
            SysFreeString(name);
            SysFreeString(value);
            break;
        }
    }

    if (FAILED(hr))
        free_dbprop_list(props);

    return hr;
}

static const struct dbproperty *get_known_dprop_descr(BSTR name)
{
    int min, max, n;

    min = 0;
    max = ARRAY_SIZE(dbproperties) - 1;

    while (min <= max)
    {
        int r;

        n = (min+max)/2;

        r = wcsicmp(dbproperties[n].name, name);
        if (!r)
            break;

        if (r < 0)
            min = n+1;
        else
            max = n-1;
    }

    return (min <= max) ? &dbproperties[n] : NULL;
}

static HRESULT get_dbpropset_from_proplist(struct dbprops *props, DBPROPSET **propset)
{
    BSTR provider_string = NULL;
    struct dbprop_pair *pair;
    int i = 0;
    HRESULT hr;

    *propset = CoTaskMemAlloc(sizeof(DBPROPSET));
    if (!*propset)
        return E_OUTOFMEMORY;

    (*propset)->rgProperties = CoTaskMemAlloc(props->count*sizeof(DBPROP));
    if (!(*propset)->rgProperties)
    {
        CoTaskMemFree(*propset);
        *propset = NULL;
        return E_OUTOFMEMORY;
    }

    (*propset)->cProperties = 0;
    (*propset)->guidPropertySet = DBPROPSET_DBINIT;
    LIST_FOR_EACH_ENTRY(pair, &props->props, struct dbprop_pair, entry)
    {
        const struct dbproperty *descr = get_known_dprop_descr(pair->name);
        VARIANT dest, src;

        if (!descr)
        {
            int len;

            len = SysStringLen(pair->name) + SysStringLen(pair->value) + 1; /* for '=' */
            if (!provider_string)
            {
                provider_string = SysAllocStringLen(NULL, len);
            }
            else
            {
                BSTR old_string = provider_string;

                len += SysStringLen(provider_string) + 1; /* for ';' separator */
                provider_string = SysAllocStringLen(NULL, len);
                lstrcpyW(provider_string, old_string);
                lstrcatW(provider_string, L";");
                SysFreeString(old_string);
            }
            lstrcatW(provider_string, pair->name);
            lstrcatW(provider_string, L"=");
            lstrcatW(provider_string, pair->value);
            continue;
        }

        V_VT(&src) = VT_BSTR;
        V_BSTR(&src) = pair->value;

        VariantInit(&dest);
        hr = VariantChangeType(&dest, &src, 0, descr->type);
        if (FAILED(hr) && descr->convert_dbproperty)
            hr = descr->convert_dbproperty(pair->value, &dest);

        if (FAILED(hr))
        {
            ERR("failed to init property %s value as type %d\n", debugstr_w(pair->name), descr->type);
            SysFreeString(provider_string);
            free_dbpropset(1, *propset);
            *propset = NULL;
            return hr;
        }

        (*propset)->cProperties++;
        (*propset)->rgProperties[i].dwPropertyID = descr->id;
        (*propset)->rgProperties[i].dwOptions = descr->options;
        (*propset)->rgProperties[i].dwStatus = 0;
        memset(&(*propset)->rgProperties[i].colid, 0, sizeof(DBID));
        (*propset)->rgProperties[i].vValue = dest;
        i++;
    }

    /* Provider specific property is always VT_BSTR */
    /* DBPROP_INIT_PROVIDERSTRING should be specified only once. Otherwise, it will be considered as
     * provider-specific rather than an initialization property */
    if (provider_string)
    {
        (*propset)->cProperties++;
        (*propset)->rgProperties[i].dwPropertyID = DBPROP_INIT_PROVIDERSTRING;
        (*propset)->rgProperties[i].dwOptions = DBPROPOPTIONS_REQUIRED;
        (*propset)->rgProperties[i].dwStatus = 0;
        memset(&(*propset)->rgProperties[i].colid, 0, sizeof(DBID));
        V_VT(&(*propset)->rgProperties[i].vValue) = VT_BSTR;
        V_BSTR(&(*propset)->rgProperties[i].vValue) = provider_string;
    }

    return S_OK;
}

/*** IDataInitialize methods ***/
static void datasource_release(BOOL created, IUnknown **datasource)
{
    if (!created)
        return;

    IUnknown_Release(*datasource);
    *datasource = NULL;
}

static WCHAR *strstriW(const WCHAR *str, const WCHAR *sub)
{
    LPWSTR strlower, sublower, r;
    strlower = CharLowerW(wcsdup(str));
    sublower = CharLowerW(wcsdup(sub));
    r = wcsstr(strlower, sublower);
    if (r)
        r = (LPWSTR)str + (r - strlower);
    free(strlower);
    free(sublower);
    return r;
}

HRESULT get_data_source(IUnknown *outer, DWORD clsctx, LPCOLESTR initstring, REFIID riid, IUnknown **datasource)
{
    static const WCHAR providerW[] = L"Provider=";
    BOOL datasource_created = FALSE;
    IDBProperties *dbprops;
    DBPROPSET *propset;
    WCHAR *prov = NULL;
    CLSID provclsid;
    HRESULT hr;


    /* first get provider name */
    provclsid = IID_NULL;
    if (initstring && (prov = strstriW(initstring, providerW)))
    {
        WCHAR *start, *progid;
        int len;

        prov += ARRAY_SIZE(providerW)-1;
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
        hr = CLSIDFromProgID(L"MSDASQL", &provclsid);
        if (FAILED(hr))
            ERR("ODBC provider for OLE DB not registered\n");
    }

    /* check for provider mismatch if it was specified in init string */
    if (*datasource && prov)
    {
        DBPROPIDSET propidset;
        DWORD prop;
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
            WARN("GetProperties failed for datasource, 0x%08lx\n", hr);
            return hr;
        }

        TRACE("initial data source provider %s\n", debugstr_w(V_BSTR(&propset->rgProperties[0].vValue)));
        initprov = IID_NULL;
        hr = CLSIDFromProgID(V_BSTR(&propset->rgProperties[0].vValue), &initprov);
        free_dbpropset(count, propset);

        if (FAILED(hr) || !IsEqualIID(&provclsid, &initprov)) return DB_E_MISMATCHEDPROVIDER;
    }

    if (!*datasource)
    {
        if (!IsEqualIID(&provclsid, &IID_NULL))
            hr = CoCreateInstance(&provclsid, outer, clsctx, riid, (void**)datasource);

        if (FAILED(hr) && IsEqualIID(riid, &IID_IDBInitialize))
            hr = create_db_init(datasource);

        datasource_created = *datasource != NULL;
    }

    /* now set properties */
    if (initstring)
    {
        struct dbprops props;

        hr = IUnknown_QueryInterface(*datasource, &IID_IDBProperties, (void**)&dbprops);
        if (FAILED(hr))
        {
            ERR("provider doesn't support IDBProperties\n");
            datasource_release(datasource_created, datasource);
            return hr;
        }

        hr = parse_init_string(initstring, &props);
        if (FAILED(hr))
        {
            datasource_release(datasource_created, datasource);
            return hr;
        }

        hr = get_dbpropset_from_proplist(&props, &propset);
        free_dbprop_list(&props);
        if (FAILED(hr))
        {
            datasource_release(datasource_created, datasource);
            return hr;
        }

        hr = IDBProperties_SetProperties(dbprops, 1, propset);
        /* Return S_OK for any successful code.  */
        if (SUCCEEDED(hr))
        {
            hr = S_OK;
        }
        IDBProperties_Release(dbprops);
        free_dbpropset(1, propset);
        if (FAILED(hr))
        {
            ERR("SetProperties failed, 0x%08lx\n", hr);
            datasource_release(datasource_created, datasource);
            return hr;
        }
    }

    return hr;
}

static HRESULT WINAPI datainit_GetDataSource(IDataInitialize *iface, IUnknown *outer, DWORD clsctx,
                                LPCOLESTR initstring, REFIID riid, IUnknown **datasource)
{
    datainit *This = impl_from_IDataInitialize(iface);

    TRACE("(%p)->(%p 0x%lx %s %s %p)\n", This, outer, clsctx, debugstr_w(initstring), debugstr_guid(riid), datasource);

    return get_data_source(outer, clsctx, initstring, riid, datasource);
}

/* returns character length of string representation */
static int get_propvalue_length(DBPROP *prop)
{
    VARIANT *v = &prop->vValue;
    VARIANT str;
    HRESULT hr;
    int length;

    if (V_VT(v) == VT_BSTR)
    {
        length = SysStringLen(V_BSTR(v));
        /* Quotes values with '\"' if the value contains semicolons */
        if (wcsstr(V_BSTR(v), L";"))
            length += 2;
        return length;
    }

    VariantInit(&str);
    hr = VariantChangeType(&str, v, 0, VT_BSTR);
    if (hr == S_OK)
    {
        length = SysStringLen(V_BSTR(&str));
        /* Quotes values with '\"' if the value contains semicolons */
        if (wcsstr(V_BSTR(&str), L";"))
            length += 2;
        VariantClear(&str);
        return length;
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
        if (wcsstr(V_BSTR(v), L";"))
        {
            lstrcatW(str, L"\"");
            lstrcatW(str, V_BSTR(v));
            lstrcatW(str, L"\"");
        }
        else
        {
            lstrcatW(str, V_BSTR(v));
        }
        return;
    }

    VariantInit(&vstr);
    hr = VariantChangeType(&vstr, v, VARIANT_ALPHABOOL, VT_BSTR);
    if (hr == S_OK)
    {
        if (wcsstr(V_BSTR(&vstr), L";"))
        {
            lstrcatW(str, L"\"");
            lstrcatW(str, V_BSTR(&vstr));
            lstrcatW(str, L"\"");
        }
        else
        {
            lstrcatW(str, V_BSTR(&vstr));
        }
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

/* Whether a property should be skipped in datainit_GetInitializationString() */
static BOOL skip_property(const DBPROPSET *propset, unsigned int prop_index, boolean include_pass)
{
    DWORD prop_id = propset->rgProperties[prop_index].dwPropertyID;
    unsigned int i;

    /* Skip password if include_pass is FALSE */
    if (!include_pass && prop_id == DBPROP_AUTH_PASSWORD)
        return TRUE;

    /* Skip these properties according to the function spec */
    if (prop_id == DBPROP_INIT_ASYNCH
        || prop_id == DBPROP_INIT_HWND
        || prop_id == DBPROP_INIT_PROMPT
        || prop_id == DBPROP_INIT_TIMEOUT
        || prop_id == DBPROP_INIT_GENERALTIMEOUT
        || prop_id == DBPROP_INIT_OLEDBSERVICES
        || prop_id == DBPROP_INIT_LCID) /* DBPROP_INIT_LCID should also be ignored according to tests */
        return TRUE;

    /* Skip duplicate properties */
    for (i = prop_index + 1; i < propset->cProperties; i++)
    {
        if (propset->rgProperties[i].dwPropertyID == prop_id)
            return TRUE;
    }

    return FALSE;
}

static HRESULT WINAPI datainit_GetInitializationString(IDataInitialize *iface, IUnknown *datasource,
                                boolean include_pass, LPWSTR *init_string)
{
    static const WCHAR providerW[] = L"Provider=";
    datainit *This = impl_from_IDataInitialize(iface);
    DBPROPINFOSET *propinfoset;
    IDBProperties *props;
    DBPROPIDSET propidset;
    ULONG propset_count, infocount;
    ULONG i, len, propvalue_length;
    WCHAR *progid, *desc;
    DBPROPSET *propset;
    IPersist *persist;
    HRESULT hr;
    CLSID clsid;

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
    propset_count = 0;
    hr = IDBProperties_GetProperties(props, 1, &propidset, &propset_count, &propset);
    if (FAILED(hr))
    {
        WARN("failed to get data source properties, 0x%08lx\n", hr);
        CoTaskMemFree(progid);
        return hr;
    }

    infocount = 0;
    IDBProperties_GetPropertyInfo(props, 1, &propidset, &infocount, &propinfoset, &desc);
    IDBProperties_Release(props);

    /* check if we need to skip password */
    len = lstrlenW(progid) + lstrlenW(providerW) + 1; /* including '\0' */
    for (i = 0; i < propset->cProperties; i++)
    {
        WCHAR *descr;

        if (skip_property(propset, i, TRUE))
            continue;

        descr = get_propinfo_descr(&propset->rgProperties[i], propinfoset);
        if (descr)
        {
            propvalue_length = get_propvalue_length(&propset->rgProperties[i]);
            if (propvalue_length)
                len += lstrlenW(descr) + propvalue_length + 2; /* include ';' and '=' */
        }

        if ((propset->rgProperties[i].dwPropertyID == DBPROP_AUTH_PERSIST_SENSITIVE_AUTHINFO) &&
            (V_BOOL(&propset->rgProperties[i].vValue) == VARIANT_FALSE))
           include_pass = FALSE;
    }

    len *= sizeof(WCHAR);
    *init_string = CoTaskMemAlloc(len);
    *init_string[0] = 0;

    /* provider name */
    lstrcatW(*init_string, providerW);
    lstrcatW(*init_string, progid);
    CoTaskMemFree(progid);

    for (i = 0; i < propset->cProperties; i++)
    {
        WCHAR *descr;

        if (skip_property(propset, i, include_pass))
            continue;

        descr = get_propinfo_descr(&propset->rgProperties[i], propinfoset);
        if (descr)
        {
            if (!get_propvalue_length(&propset->rgProperties[i]))
                continue;

            lstrcatW(*init_string, L";");
            lstrcatW(*init_string, descr);
            lstrcatW(*init_string, L"=");
            write_propvalue_str(*init_string, &propset->rgProperties[i]);
        }
    }

    free_dbpropset(propset_count, propset);
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

    TRACE("(%p)->(%s %p 0x%08lx %s %s %p)\n", This, debugstr_guid(provider), outer, clsctx, debugstr_w(reserved),
        debugstr_guid(riid), datasource);

    return CoCreateInstance(provider, outer, clsctx, riid, (void**)datasource);
}

static HRESULT WINAPI datainit_CreateDBInstanceEx(IDataInitialize *iface, REFCLSID provider, IUnknown *outer,
    DWORD clsctx, LPWSTR reserved, COSERVERINFO *server_info, DWORD cmq, MULTI_QI *results)
{
    datainit *This = impl_from_IDataInitialize(iface);

    FIXME("(%p)->(%s %p %#lx %s %p %lu %p)\n", This, debugstr_guid(provider), outer, clsctx,
        debugstr_w(reserved), server_info, cmq, results);

    return E_NOTIMPL;
}

static HRESULT WINAPI datainit_LoadStringFromStorage(IDataInitialize *iface, LPCOLESTR pwszFileName,
                                LPWSTR *ppwszInitializationString)
{
    datainit *This = impl_from_IDataInitialize(iface);

    FIXME("(%p)->(%s %p)\n", This, debugstr_w(pwszFileName), ppwszInitializationString);

    return E_NOTIMPL;
}

static HRESULT WINAPI datainit_WriteStringToStorage(IDataInitialize *iface, LPCOLESTR pwszFileName,
                                LPCOLESTR pwszInitializationString, DWORD dwCreationDisposition)
{
    datainit *This = impl_from_IDataInitialize(iface);

    FIXME("(%p)->(%s %s %ld)\n", This, debugstr_w(pwszFileName), debugstr_w(pwszInitializationString), dwCreationDisposition);

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
    datainit_CreateDBInstanceEx,
    datainit_LoadStringFromStorage,
    datainit_WriteStringToStorage
};

HRESULT create_data_init(IUnknown *outer, void **obj)
{
    datainit *This;

    TRACE("(%p)\n", obj);

    if(outer) return CLASS_E_NOAGGREGATION;

    *obj = NULL;

    This = malloc(sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->IDataInitialize_iface.lpVtbl = &datainit_vtbl;
    This->ref = 1;

    *obj = &This->IDataInitialize_iface;

    return S_OK;
}
