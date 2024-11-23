/*
 * propsys main
 *
 * Copyright 1997, 2002 Alexandre Julliard
 * Copyright 2008 James Hawkins
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
#include "objbase.h"
#include "rpcproxy.h"
#include "propsys.h"
#include "propkey.h"
#include "wine/debug.h"

#include "propsys_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(propsys);

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

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
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

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);

    return S_OK;
}

static HRESULT WINAPI InMemoryPropertyStoreFactory_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    return PropertyStore_CreateInstance(outer, riid, ppv);
}

static const IClassFactoryVtbl InMemoryPropertyStoreFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    InMemoryPropertyStoreFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory InMemoryPropertyStoreFactory = { &InMemoryPropertyStoreFactoryVtbl };

static IPropertySystem propsys;

static HRESULT WINAPI propsys_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID iid, void **out)
{
    TRACE("%p, %p, %s, %p\n", iface, outer, debugstr_guid(iid), out);

    *out = NULL;
    if (outer)
        return CLASS_E_NOAGGREGATION;
    return IPropertySystem_QueryInterface(&propsys, iid, out);
}

static const IClassFactoryVtbl PropertySystemFactoryVtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    propsys_factory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory PropertySystemFactory = { &PropertySystemFactoryVtbl };

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(&CLSID_InMemoryPropertyStore, rclsid)) {
        TRACE("(CLSID_InMemoryPropertyStore %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&InMemoryPropertyStoreFactory, riid, ppv);
    }
    if (IsEqualGUID(&CLSID_PropertySystem, rclsid))
    {
        TRACE("(CLSID_PropertySystem %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&PropertySystemFactory, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

static HRESULT WINAPI propsys_QueryInterface(IPropertySystem *iface, REFIID riid, void **obj)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(riid), obj);
    *obj = NULL;

    if (IsEqualIID(riid, &IID_IPropertySystem) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IPropertySystem_AddRef(iface);
        return S_OK;
    }

    FIXME("%s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI propsys_AddRef(IPropertySystem *iface)
{
    return 2;
}

static ULONG WINAPI propsys_Release(IPropertySystem *iface)
{
    return 1;
}

static HRESULT propdesc_get_by_name( const WCHAR *name, IPropertyDescription **desc );
static HRESULT propdesc_get_by_key( const PROPERTYKEY *key, IPropertyDescription **desc );

static HRESULT WINAPI propsys_GetPropertyDescription(IPropertySystem *iface,
    REFPROPERTYKEY propkey, REFIID riid, void **ppv)
{
    HRESULT hr;
    IPropertyDescription *desc;

    FIXME("(%p, %s, %s, %p): semi-stub!\n", iface, debugstr_propkey(propkey), debugstr_guid(riid), ppv);

    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;
    hr = propdesc_get_by_key( propkey, &desc );
    if (FAILED( hr ))
        return hr;

    hr = IPropertyDescription_QueryInterface( desc, riid, ppv );
    IPropertyDescription_Release( desc );
    return hr;
}

static HRESULT WINAPI propsys_GetPropertyDescriptionByName(IPropertySystem *iface,
    LPCWSTR canonical_name, REFIID riid, void **ppv)
{
    HRESULT hr;
    IPropertyDescription *desc;

    FIXME("(%p, %s, %s, %p): stub\n", iface, debugstr_w(canonical_name), debugstr_guid(riid), ppv);

    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;
    hr = propdesc_get_by_name( canonical_name, &desc );
    if (FAILED( hr ))
        return hr;
    hr = IPropertyDescription_QueryInterface( desc, riid, ppv );
    IPropertyDescription_Release( desc );
    return hr;
}

static HRESULT WINAPI propsys_GetPropertyDescriptionListFromString(IPropertySystem *iface,
    LPCWSTR proplist, REFIID riid, void **ppv)
{
    return PSGetPropertyDescriptionListFromString(proplist, riid, ppv);
}

static HRESULT WINAPI propsys_EnumeratePropertyDescriptions(IPropertySystem *iface,
    PROPDESC_ENUMFILTER filter, REFIID riid, void **ppv)
{
    FIXME("%d %s %p: stub\n", filter, debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI propsys_FormatForDisplay(IPropertySystem *iface,
    REFPROPERTYKEY key, REFPROPVARIANT propvar, PROPDESC_FORMAT_FLAGS flags,
    LPWSTR dest, DWORD destlen)
{
    FIXME("%p %p %x %p %ld: stub\n", key, propvar, flags, dest, destlen);
    return E_NOTIMPL;
}

static HRESULT WINAPI propsys_FormatForDisplayAlloc(IPropertySystem *iface,
    REFPROPERTYKEY key, REFPROPVARIANT propvar, PROPDESC_FORMAT_FLAGS flags,
    LPWSTR *text)
{
    FIXME("%p %p %x %p: stub\n", key, propvar, flags, text);
    return E_NOTIMPL;
}

static HRESULT WINAPI propsys_RegisterPropertySchema(IPropertySystem *iface, LPCWSTR path)
{
    return PSRegisterPropertySchema(path);
}

static HRESULT WINAPI propsys_UnregisterPropertySchema(IPropertySystem *iface, LPCWSTR path)
{
    return PSUnregisterPropertySchema(path);
}

static HRESULT WINAPI propsys_RefreshPropertySchema(IPropertySystem *iface)
{
    return PSRefreshPropertySchema();
}

static const IPropertySystemVtbl propsysvtbl = {
    propsys_QueryInterface,
    propsys_AddRef,
    propsys_Release,
    propsys_GetPropertyDescription,
    propsys_GetPropertyDescriptionByName,
    propsys_GetPropertyDescriptionListFromString,
    propsys_EnumeratePropertyDescriptions,
    propsys_FormatForDisplay,
    propsys_FormatForDisplayAlloc,
    propsys_RegisterPropertySchema,
    propsys_UnregisterPropertySchema,
    propsys_RefreshPropertySchema
};

static IPropertySystem propsys = { &propsysvtbl };

HRESULT WINAPI PSGetPropertySystem(REFIID riid, void **obj)
{
    return IPropertySystem_QueryInterface(&propsys, riid, obj);
}

HRESULT WINAPI PSRegisterPropertySchema(PCWSTR path)
{
    FIXME("%s stub\n", debugstr_w(path));

    return S_OK;
}

HRESULT WINAPI PSUnregisterPropertySchema(PCWSTR path)
{
    FIXME("%s stub\n", debugstr_w(path));
    return E_NOTIMPL;
}

HRESULT WINAPI PSGetPropertyDescription(REFPROPERTYKEY propkey, REFIID riid, void **ppv)
{
    HRESULT hr;
    IPropertySystem *system;

    TRACE("%p, %p, %p\n", propkey, riid, ppv);
    hr = IPropertySystem_QueryInterface(&propsys, &IID_IPropertySystem, (void **)&system);
    if (SUCCEEDED(hr))
    {
        hr = IPropertySystem_GetPropertyDescription(system, propkey, riid, ppv);
        IPropertySystem_Release(system);
    }
    return hr;
}

HRESULT WINAPI PSGetPropertyDescriptionListFromString(LPCWSTR proplist, REFIID riid, void **ppv)
{
    FIXME("%s, %p, %p\n", debugstr_w(proplist), riid, ppv);
    return E_NOTIMPL;
}

HRESULT WINAPI PSGetNameFromPropertyKey(REFPROPERTYKEY key, LPWSTR *name)
{
    HRESULT hr;
    IPropertySystem *system;

    TRACE("(%s, %p)\n", debugstr_propkey(key), name);
    hr = IPropertySystem_QueryInterface(&propsys, &IID_IPropertySystem, (void **)&system);
    if (SUCCEEDED(hr))
    {
        IPropertyDescription *desc;

        hr = IPropertySystem_GetPropertyDescription(system, key, &IID_IPropertyDescription, (void **)&desc);
        if (SUCCEEDED(hr))
        {
            hr = IPropertyDescription_GetCanonicalName(desc, name);
            IPropertyDescription_Release(desc);
        }
        IPropertySystem_Release(system);
    }

    return hr;
}

HRESULT WINAPI PSGetPropertyKeyFromName(PCWSTR name, PROPERTYKEY *key)
{
    HRESULT hr;
    IPropertySystem *system;

    TRACE("%s, %p\n", debugstr_w(name), debugstr_propkey(key));
    hr = IPropertySystem_QueryInterface(&propsys, &IID_IPropertySystem, (void **)&system);
    if (SUCCEEDED(hr))
    {
        IPropertyDescription *desc;
        hr = IPropertySystem_GetPropertyDescriptionByName(system, name, &IID_IPropertyDescription, (void **)&desc);
        if (SUCCEEDED(hr))
        {
            hr = IPropertyDescription_GetPropertyKey(desc, key);
            IPropertyDescription_Release(desc);
        }
        IPropertySystem_Release(system);
    }

    return hr;
}

HRESULT WINAPI PSRefreshPropertySchema(void)
{
    FIXME("\n");
    return S_OK;
}

HRESULT WINAPI PSStringFromPropertyKey(REFPROPERTYKEY pkey, LPWSTR psz, UINT cch)
{
    WCHAR pidW[PKEY_PIDSTR_MAX + 1];
    LPWSTR p = psz;
    int len;

    TRACE("(%p, %p, %u)\n", pkey, psz, cch);

    if (!psz)
        return E_POINTER;

    /* GUIDSTRING_MAX accounts for null terminator, +1 for space character. */
    if (cch <= GUIDSTRING_MAX + 1)
        return E_NOT_SUFFICIENT_BUFFER;

    if (!pkey)
    {
        psz[0] = '\0';
        return E_NOT_SUFFICIENT_BUFFER;
    }

    swprintf(psz, cch, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", pkey->fmtid.Data1,
             pkey->fmtid.Data2, pkey->fmtid.Data3, pkey->fmtid.Data4[0], pkey->fmtid.Data4[1],
             pkey->fmtid.Data4[2], pkey->fmtid.Data4[3], pkey->fmtid.Data4[4],
             pkey->fmtid.Data4[5], pkey->fmtid.Data4[6], pkey->fmtid.Data4[7]);

    /* Overwrite the null terminator with the space character. */
    p += GUIDSTRING_MAX - 1;
    *p++ = ' ';
    cch -= GUIDSTRING_MAX - 1 + 1;

    len = swprintf(pidW, ARRAY_SIZE(pidW), L"%u", pkey->pid);

    if (cch >= len + 1)
    {
        lstrcpyW(p, pidW);
        return S_OK;
    }
    else
    {
        WCHAR *ptr = pidW + len - 1;

        psz[0] = '\0';
        *p++ = '\0';
        cch--;

        /* Replicate a quirk of the native implementation where the contents
         * of the property ID string are written backwards to the output
         * buffer, skipping the rightmost digit. */
        if (cch)
        {
            ptr--;
            while (cch--)
                *p++ = *ptr--;
        }

        return E_NOT_SUFFICIENT_BUFFER;
    }
}

static const BYTE hex2bin[] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x00 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x10 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x20 */
    0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,        /* 0x30 */
    0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,  /* 0x40 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x50 */
    0,10,11,12,13,14,15                     /* 0x60 */
};

static BOOL validate_indices(LPCWSTR s, int min, int max)
{
    int i;

    for (i = min; i <= max; i++)
    {
        if (!s[i])
            return FALSE;

        if (i == 0)
        {
            if (s[i] != '{')
                return FALSE;
        }
        else if (i == 9 || i == 14 || i == 19 || i == 24)
        {
            if (s[i] != '-')
                return FALSE;
        }
        else if (i == 37)
        {
            if (s[i] != '}')
                return FALSE;
        }
        else
        {
            if (s[i] > 'f' || (!hex2bin[s[i]] && s[i] != '0'))
                return FALSE;
        }
    }

    return TRUE;
}

/* Adapted from CLSIDFromString helper in dlls/ole32/compobj.c and
 * UuidFromString in dlls/rpcrt4/rpcrt4_main.c. */
static BOOL string_to_guid(LPCWSTR s, LPGUID id)
{
    /* in form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} */

    if (!validate_indices(s, 0, 8)) return FALSE;
    id->Data1 = (hex2bin[s[1]] << 28 | hex2bin[s[2]] << 24 | hex2bin[s[3]] << 20 | hex2bin[s[4]] << 16 |
                 hex2bin[s[5]] << 12 | hex2bin[s[6]] << 8  | hex2bin[s[7]] << 4  | hex2bin[s[8]]);
    if (!validate_indices(s, 9, 14)) return FALSE;
    id->Data2 = hex2bin[s[10]] << 12 | hex2bin[s[11]] << 8 | hex2bin[s[12]] << 4 | hex2bin[s[13]];
    if (!validate_indices(s, 15, 19)) return FALSE;
    id->Data3 = hex2bin[s[15]] << 12 | hex2bin[s[16]] << 8 | hex2bin[s[17]] << 4 | hex2bin[s[18]];

    /* these are just sequential bytes */

    if (!validate_indices(s, 20, 21)) return FALSE;
    id->Data4[0] = hex2bin[s[20]] << 4 | hex2bin[s[21]];
    if (!validate_indices(s, 22, 24)) return FALSE;
    id->Data4[1] = hex2bin[s[22]] << 4 | hex2bin[s[23]];

    if (!validate_indices(s, 25, 26)) return FALSE;
    id->Data4[2] = hex2bin[s[25]] << 4 | hex2bin[s[26]];
    if (!validate_indices(s, 27, 28)) return FALSE;
    id->Data4[3] = hex2bin[s[27]] << 4 | hex2bin[s[28]];
    if (!validate_indices(s, 29, 30)) return FALSE;
    id->Data4[4] = hex2bin[s[29]] << 4 | hex2bin[s[30]];
    if (!validate_indices(s, 31, 32)) return FALSE;
    id->Data4[5] = hex2bin[s[31]] << 4 | hex2bin[s[32]];
    if (!validate_indices(s, 33, 34)) return FALSE;
    id->Data4[6] = hex2bin[s[33]] << 4 | hex2bin[s[34]];
    if (!validate_indices(s, 35, 37)) return FALSE;
    id->Data4[7] = hex2bin[s[35]] << 4 | hex2bin[s[36]];

    return TRUE;
}

HRESULT WINAPI PSPropertyKeyFromString(LPCWSTR pszString, PROPERTYKEY *pkey)
{
    BOOL has_minus = FALSE, has_comma = FALSE;

    TRACE("(%s, %p)\n", debugstr_w(pszString), pkey);

    if (!pszString || !pkey)
        return E_POINTER;

    memset(pkey, 0, sizeof(PROPERTYKEY));

    if (!string_to_guid(pszString, &pkey->fmtid))
        return E_INVALIDARG;

    pszString += GUIDSTRING_MAX - 1;

    if (!*pszString)
        return E_INVALIDARG;

    /* Only the space seems to be recognized as whitespace. The comma is only
     * recognized once and processing terminates if another comma is found. */
    while (*pszString == ' ' || *pszString == ',')
    {
        if (*pszString == ',')
        {
            if (has_comma)
                return S_OK;
            else
                has_comma = TRUE;
        }
        pszString++;
    }

    if (!*pszString)
        return E_INVALIDARG;

    /* Only two minus signs are recognized if no comma is detected. The first
     * sign is ignored, and the second is interpreted. If a comma is detected
     * before the minus sign, then only one minus sign counts, and property ID
     * interpretation begins with the next character. */
    if (has_comma)
    {
        if (*pszString == '-')
        {
            has_minus = TRUE;
            pszString++;
        }
    }
    else
    {
        if (*pszString == '-')
            pszString++;

        /* Skip any intermediate spaces after the first minus sign. */
        while (*pszString == ' ')
            pszString++;

        if (*pszString == '-')
        {
            has_minus = TRUE;
            pszString++;
        }

        /* Skip any remaining spaces after minus sign. */
        while (*pszString == ' ')
            pszString++;
    }

    /* Overflow is not checked. */
    while ('0' <= *pszString && *pszString <= '9')
    {
        pkey->pid *= 10;
        pkey->pid += (*pszString - '0');
        pszString++;
    }

    if (has_minus)
        pkey->pid = ~pkey->pid + 1;

    return S_OK;
}

HRESULT WINAPI PSCreateMemoryPropertyStore(REFIID riid, void **ppv)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    return PropertyStore_CreateInstance(NULL, riid, ppv);
}

struct property_description
{
    IPropertyDescription IPropertyDescription_iface;
    LONG ref;

    PROPERTYKEY key;
    VARTYPE type;
    PROPDESC_TYPE_FLAGS type_flags;
    WCHAR *canonical_name;
};

static inline struct property_description *
impl_from_IPropertyDescription( IPropertyDescription *iface )
{
    return CONTAINING_RECORD( iface, struct property_description, IPropertyDescription_iface );
}

static HRESULT WINAPI propdesc_QueryInterface( IPropertyDescription *iface, REFIID iid, void **out )
{
    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );
    *out = NULL;

    if (IsEqualGUID( &IID_IUnknown, iid ) ||
        IsEqualGUID( &IID_IPropertyDescription, iid ))
    {
        *out = iface;
        IUnknown_AddRef( iface );
        return S_OK;
    }

    FIXME( "%s not implemented\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI propdesc_AddRef( IPropertyDescription *iface )
{
    struct property_description *propdesc = impl_from_IPropertyDescription( iface );
    TRACE( "(%p)\n", iface );
    return InterlockedIncrement( &propdesc->ref );
}

static ULONG WINAPI propdesc_Release( IPropertyDescription *iface )
{
    struct property_description *propdesc = impl_from_IPropertyDescription( iface );
    ULONG ref;

    TRACE( "(%p)\n", iface );
    ref = InterlockedDecrement( &propdesc->ref );
    if (!ref)
    {
        free( propdesc->canonical_name );
        free( propdesc );
    }

    return ref;
}

static HRESULT WINAPI propdesc_GetPropertyKey( IPropertyDescription *iface, PROPERTYKEY *pkey )
{
    struct property_description *propdesc = impl_from_IPropertyDescription( iface );

    TRACE( "(%p, %p)\n", iface, pkey );
    *pkey = propdesc->key;
    return S_OK;
}

static HRESULT WINAPI propdesc_GetCanonicalName( IPropertyDescription *iface, LPWSTR *name )
{
    struct property_description *propdesc = impl_from_IPropertyDescription( iface );

    TRACE( "(%p, %p)\n", iface, name );
    *name = CoTaskMemAlloc( (wcslen( propdesc->canonical_name ) + 1) * sizeof( WCHAR ) );
    if (!*name)
        return E_OUTOFMEMORY;
    wcscpy( *name, propdesc->canonical_name );
    return S_OK;
}

static HRESULT WINAPI propdesc_GetPropertyType( IPropertyDescription *iface, VARTYPE *vt )
{
    struct property_description *propdesc = impl_from_IPropertyDescription( iface );

    TRACE( "(%p, %p)\n", iface, vt );
    *vt = propdesc->type;
    return S_OK;
}

static HRESULT WINAPI propdesc_GetDisplayName( IPropertyDescription *iface, LPWSTR *name )
{
    FIXME( "(%p, %p) semi-stub!\n", iface, name );
    return IPropertyDescription_GetCanonicalName( iface, name );
}

static HRESULT WINAPI propdesc_GetEditInvitation( IPropertyDescription *iface, LPWSTR *invite )
{
    FIXME( "(%p, %p) stub!\n", iface, invite );
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_GetTypeFlags( IPropertyDescription *iface, PROPDESC_TYPE_FLAGS mask,
                                             PROPDESC_TYPE_FLAGS *flags )
{
    struct property_description *propdesc = impl_from_IPropertyDescription( iface );

    TRACE( "(%p, %#x, %p)\n", iface, mask, flags );
    *flags = mask & propdesc->type_flags;
    return S_OK;
}

static HRESULT WINAPI propdesc_GetViewFlags( IPropertyDescription *iface, PROPDESC_VIEW_FLAGS *flags )
{
    FIXME( "(%p, %p) stub!\n", iface, flags );
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_GetDefaultColumnWidth( IPropertyDescription *iface, UINT *chars )
{
    FIXME( "(%p, %p) stub!\n", iface, chars );
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_GetDisplayType( IPropertyDescription *iface, PROPDESC_DISPLAYTYPE *type )
{
    FIXME( "(%p, %p) stub!\n", iface, type );
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_GetColumnState( IPropertyDescription *iface, SHCOLSTATEF *flags )
{
    FIXME( "(%p, %p) stub!\n", iface, flags );
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_GetGroupingRange( IPropertyDescription *iface, PROPDESC_GROUPING_RANGE *range )
{
    FIXME( "(%p, %p) stub!\n", iface, range );
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_GetRelativeDescriptionType( IPropertyDescription *iface,
                                                           PROPDESC_RELATIVEDESCRIPTION_TYPE *type )
{
    FIXME( "(%p, %p) stub!\n", iface, type );
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_GetRelativeDescription( IPropertyDescription *iface, REFPROPVARIANT propvar1,
                                                       REFPROPVARIANT propvar2, LPWSTR *desc1, LPWSTR *desc2 )
{
    FIXME( "(%p, %p, %p, %p, %p) stub!\n", iface, propvar1, propvar2, desc1, desc2 );
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_GetSortDescription( IPropertyDescription *iface, PROPDESC_SORTDESCRIPTION *psd )
{
    FIXME( "(%p, %p) stub!\n", iface, psd );
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_GetSortDescriptionLabel( IPropertyDescription *iface, BOOL descending, LPWSTR *desc )
{
    FIXME( "(%p, %d, %p) stub!\n", iface, descending, desc );
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_GetAggregationType( IPropertyDescription *iface, PROPDESC_AGGREGATION_TYPE *type )
{
    FIXME( "(%p, %p) stub!\n", iface, type );
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_GetConditionType( IPropertyDescription *iface, PROPDESC_CONDITION_TYPE *cond_type,
                                                 CONDITION_OPERATION *op_default )
{
    FIXME( "(%p, %p, %p) stub!\n", iface, cond_type, op_default );
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_GetEnumTypeList( IPropertyDescription *iface, REFIID riid, void **out )
{
    FIXME( "(%p, %s, %p) stub!\n", iface, debugstr_guid( riid ), out );
    *out = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_CoerceToCanonicalValue( IPropertyDescription *iface, PROPVARIANT *propvar )
{
    FIXME( "(%p, %p) stub!\n", iface, propvar );
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_FormatForDisplay( IPropertyDescription *iface, REFPROPVARIANT propvar,
                                        PROPDESC_FORMAT_FLAGS flags, LPWSTR *display )
{
    FIXME( "(%p, %p, %#x, %p) stub!\n", iface, propvar, flags, display );
    return E_NOTIMPL;
}

static HRESULT WINAPI propdesc_IsValueCanonical( IPropertyDescription *iface, REFPROPVARIANT propvar )
{
    FIXME( "(%p, %p) stub!\n", iface, propvar );
    return E_NOTIMPL;
}

const static IPropertyDescriptionVtbl property_description_vtbl =
{
    /* IUnknown */
    propdesc_QueryInterface,
    propdesc_AddRef,
    propdesc_Release,
    /* IPropertyDescription */
    propdesc_GetPropertyKey,
    propdesc_GetCanonicalName,
    propdesc_GetPropertyType,
    propdesc_GetDisplayName,
    propdesc_GetEditInvitation,
    propdesc_GetTypeFlags,
    propdesc_GetViewFlags,
    propdesc_GetDefaultColumnWidth,
    propdesc_GetDisplayType,
    propdesc_GetColumnState,
    propdesc_GetGroupingRange,
    propdesc_GetRelativeDescriptionType,
    propdesc_GetRelativeDescription,
    propdesc_GetSortDescription,
    propdesc_GetSortDescriptionLabel,
    propdesc_GetAggregationType,
    propdesc_GetConditionType,
    propdesc_GetEnumTypeList,
    propdesc_CoerceToCanonicalValue,
    propdesc_FormatForDisplay,
    propdesc_IsValueCanonical
};

struct system_property_description
{
    const WCHAR *canonical_name;
    const PROPERTYKEY *key;

    VARTYPE type;
};

/* It may be ideal to construct rb_trees for looking up property descriptions by name and key if this array gets large
 * enough. */
static struct system_property_description system_properties[] =
{
    {L"System.Devices.ContainerId", &PKEY_Devices_ContainerId, VT_CLSID},
    {L"System.Devices.InterfaceClassGuid", &PKEY_Devices_InterfaceClassGuid, VT_CLSID},
    {L"System.Devices.DeviceInstanceId", &PKEY_Devices_DeviceInstanceId, VT_CLSID},
    {L"System.Devices.InterfaceEnabled", &PKEY_Devices_InterfaceEnabled, VT_BOOL},
    {L"System.Devices.ClassGuid", &PKEY_Devices_ClassGuid, VT_CLSID},
    {L"System.Devices.CompatibleIds", &PKEY_Devices_CompatibleIds, VT_VECTOR | VT_LPWSTR},
    {L"System.Devices.DeviceCapabilities", &PKEY_Devices_DeviceCapabilities, VT_UI2},
    {L"System.Devices.DeviceHasProblem", &PKEY_Devices_DeviceHasProblem, VT_BOOL},
    {L"System.Devices.DeviceManufacturer", &PKEY_Devices_DeviceManufacturer, VT_LPWSTR},
    {L"System.Devices.HardwareIds", &PKEY_Devices_HardwareIds, VT_VECTOR | VT_LPWSTR},
    {L"System.Devices.Parent", &PKEY_Devices_Parent, VT_LPWSTR},
    {L"System.ItemNameDisplay", &PKEY_ItemNameDisplay, VT_LPWSTR},
    {L"System.Devices.Category", &PKEY_Devices_Category, VT_VECTOR | VT_LPWSTR},
    {L"System.Devices.CategoryIds", &PKEY_Devices_CategoryIds, VT_VECTOR | VT_LPWSTR},
    {L"System.Devices.CategoryPlural", &PKEY_Devices_CategoryPlural, VT_VECTOR | VT_LPWSTR},
    {L"System.Devices.Connected", &PKEY_Devices_Connected, VT_BOOL},
    {L"System.Devices.GlyphIcon", &PKEY_Devices_GlyphIcon, VT_LPWSTR},
    {L"System.Devices.DevObjectType", &PKEY_Devices_DevObjectType, VT_UI4},
    {L"System.DeviceInterface.Bluetooth.DeviceAddress", &PKEY_DeviceInterface_Bluetooth_DeviceAddress, VT_LPWSTR},
    {L"System.Devices.Aep.AepId", &PKEY_Devices_Aep_AepId, VT_LPWSTR},
    {L"System.Devices.Aep.ProtocolId", &PKEY_Devices_Aep_ProtocolId, VT_CLSID},
    {L"System.Devices.Aep.IsPaired", &PKEY_Devices_Aep_IsPaired, VT_BOOL},
    {L"System.Devices.Aep.IsConnected", &PKEY_Devices_Aep_IsConnected, VT_BOOL},
    {L"System.Devices.Aep.IsConnected", &PKEY_Devices_Aep_IsConnected, VT_BOOL},
    {L"System.Devices.Aep.Bluetooth.Le.AddressType", &PKEY_Devices_Aep_Bluetooth_Le_AddressType, VT_UI1},
    {L"System.Devices.Aep.DeviceAddress", &PKEY_Devices_Aep_DeviceAddress, VT_LPWSTR},
    {L"System.Devices.AepContainer.IsPaired", &PKEY_Devices_AepContainer_IsPaired, VT_BOOL},
    {L"System.Devices.AepService.ProtocolId", &PKEY_Devices_AepService_ProtocolId, VT_CLSID},
};

static HRESULT propdesc_from_system_property( const struct system_property_description *desc, IPropertyDescription **out )
{
    struct property_description *propdesc;

    propdesc = calloc( 1, sizeof( *propdesc ) );
    if (!propdesc)
        return E_OUTOFMEMORY;

    propdesc->IPropertyDescription_iface.lpVtbl = &property_description_vtbl;
    if (!(propdesc->canonical_name = wcsdup( desc->canonical_name )))
    {
        free( propdesc );
        return E_OUTOFMEMORY;
    }
    propdesc->key = *desc->key;
    propdesc->type = desc->type;
    propdesc->type_flags = PDTF_ISINNATE;
    if (propdesc->type & VT_VECTOR)
        propdesc->type_flags |= PDTF_MULTIPLEVALUES;
    propdesc->ref = 1;

    *out = &propdesc->IPropertyDescription_iface;
    return S_OK;
}

static HRESULT propdesc_get_by_name( const WCHAR *name, IPropertyDescription **desc )
{
    SIZE_T i;

    for (i = 0; i < ARRAY_SIZE( system_properties ); i++)
    {
        if (!wcscmp( name, system_properties[i].canonical_name ))
            return propdesc_from_system_property( &system_properties[i], desc );
    }

    return TYPE_E_ELEMENTNOTFOUND;
}

static HRESULT propdesc_get_by_key( const PROPERTYKEY *key, IPropertyDescription **desc )
{
    SIZE_T i;

    for (i = 0; i < ARRAY_SIZE( system_properties ); i++)
    {
        if (!memcmp( key, system_properties[i].key, sizeof( *key ) ))
            return propdesc_from_system_property( &system_properties[i], desc );
    }

    return TYPE_E_ELEMENTNOTFOUND;
}
