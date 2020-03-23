/*
 * Active Directory services LDAP Provider
 *
 * Copyright 2018 Dmitry Timoshkov
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
#include "initguid.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "rpc.h"
#include "iads.h"
#include "adshlp.h"
#include "adserr.h"
#define SECURITY_WIN32
#include "security.h"
#include "dsgetdc.h"
#include "lmcons.h"
#include "lmapibuf.h"
#include "winldap.h"

#include "adsldp_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(adsldp);

DEFINE_GUID(CLSID_LDAP,0x228d9a81,0xc302,0x11cf,0x9a,0xa4,0x00,0xaa,0x00,0x4a,0x56,0x91);
DEFINE_GUID(CLSID_LDAPNamespace,0x228d9a82,0xc302,0x11cf,0x9a,0xa4,0x00,0xaa,0x00,0x4a,0x56,0x91);

static HMODULE adsldp_hinst;

static HRESULT LDAPNamespace_create(REFIID riid, void **obj);

typedef struct
{
    IParseDisplayName IParseDisplayName_iface;
    LONG ref;
} LDAP_PARSE;

static inline LDAP_PARSE *impl_from_IParseDisplayName(IParseDisplayName *iface)
{
    return CONTAINING_RECORD(iface, LDAP_PARSE, IParseDisplayName_iface);
}

static HRESULT WINAPI ldap_QueryInterface(IParseDisplayName *iface, REFIID riid, void **obj)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IParseDisplayName))
    {
        IParseDisplayName_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ldap_AddRef(IParseDisplayName *iface)
{
    LDAP_PARSE *ldap = impl_from_IParseDisplayName(iface);
    return InterlockedIncrement(&ldap->ref);
}

static ULONG WINAPI ldap_Release(IParseDisplayName *iface)
{
    LDAP_PARSE *ldap = impl_from_IParseDisplayName(iface);
    LONG ref = InterlockedDecrement(&ldap->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        heap_free(ldap);
    }

    return ref;
}

static HRESULT WINAPI ldap_ParseDisplayName(IParseDisplayName *iface, IBindCtx *bc,
                                            LPOLESTR name, ULONG *eaten, IMoniker **mk)
{
    HRESULT hr;
    IADsOpenDSObject *ads_open;
    IDispatch *disp;

    TRACE("%p,%p,%s,%p,%p\n", iface, bc, debugstr_w(name), eaten, mk);

    hr = LDAPNamespace_create(&IID_IADsOpenDSObject, (void **)&ads_open);
    if (hr != S_OK) return hr;

    hr = IADsOpenDSObject_OpenDSObject(ads_open, name, NULL, NULL, ADS_SECURE_AUTHENTICATION, &disp);
    if (hr != S_OK)
        hr = IADsOpenDSObject_OpenDSObject(ads_open, name, NULL, NULL, 0, &disp);
    if (hr == S_OK)
    {
        hr = CreatePointerMoniker((IUnknown *)disp, mk);
        if (hr == S_OK)
            *eaten = wcslen(name);

        IDispatch_Release(disp);
    }

    IADsOpenDSObject_Release(ads_open);

    return hr;
}

static const IParseDisplayNameVtbl LDAP_PARSE_vtbl =
{
    ldap_QueryInterface,
    ldap_AddRef,
    ldap_Release,
    ldap_ParseDisplayName
};

static HRESULT LDAP_create(REFIID riid, void **obj)
{
    LDAP_PARSE *ldap;
    HRESULT hr;

    ldap = heap_alloc(sizeof(*ldap));
    if (!ldap) return E_OUTOFMEMORY;

    ldap->IParseDisplayName_iface.lpVtbl = &LDAP_PARSE_vtbl;
    ldap->ref = 1;

    hr = IParseDisplayName_QueryInterface(&ldap->IParseDisplayName_iface, riid, obj);
    IParseDisplayName_Release(&ldap->IParseDisplayName_iface);

    return hr;
}

typedef struct
{
    IADsADSystemInfo IADsADSystemInfo_iface;
    LONG ref;
} AD_sysinfo;

static inline AD_sysinfo *impl_from_IADsADSystemInfo(IADsADSystemInfo *iface)
{
    return CONTAINING_RECORD(iface, AD_sysinfo, IADsADSystemInfo_iface);
}

static HRESULT WINAPI sysinfo_QueryInterface(IADsADSystemInfo *iface, REFIID riid, void **obj)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    if (IsEqualGUID(riid, &IID_IADsADSystemInfo) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IADsADSystemInfo_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI sysinfo_AddRef(IADsADSystemInfo *iface)
{
    AD_sysinfo *sysinfo = impl_from_IADsADSystemInfo(iface);
    return InterlockedIncrement(&sysinfo->ref);
}

static ULONG WINAPI sysinfo_Release(IADsADSystemInfo *iface)
{
    AD_sysinfo *sysinfo = impl_from_IADsADSystemInfo(iface);
    LONG ref = InterlockedDecrement(&sysinfo->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        heap_free(sysinfo);
    }

    return ref;
}

static HRESULT WINAPI sysinfo_GetTypeInfoCount(IADsADSystemInfo *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_GetTypeInfo(IADsADSystemInfo *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%#x,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_GetIDsOfNames(IADsADSystemInfo *iface, REFIID riid, LPOLESTR *names,
                                            UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%u,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_Invoke(IADsADSystemInfo *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%d,%s,%04x,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_UserName(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_ComputerName(IADsADSystemInfo *iface, BSTR *retval)
{
    UINT size;
    WCHAR *name;

    TRACE("%p,%p\n", iface, retval);

    size = 0;
    GetComputerObjectNameW(NameFullyQualifiedDN, NULL, &size);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return HRESULT_FROM_WIN32(GetLastError());

    name = SysAllocStringLen(NULL, size);
    if (!name) return E_OUTOFMEMORY;

    if (!GetComputerObjectNameW(NameFullyQualifiedDN, name, &size))
    {
        SysFreeString(name);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *retval = name;
    return S_OK;
}

static HRESULT WINAPI sysinfo_get_SiteName(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_DomainShortName(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_DomainDNSName(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_ForestDNSName(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_PDCRoleOwner(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_SchemaRoleOwner(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_get_IsNativeMode(IADsADSystemInfo *iface, VARIANT_BOOL *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_GetAnyDCName(IADsADSystemInfo *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_GetDCSiteName(IADsADSystemInfo *iface, BSTR server, BSTR *retval)
{
    FIXME("%p,%s,%p: stub\n", iface, debugstr_w(server), retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_RefreshSchemaCache(IADsADSystemInfo *iface)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_GetTrees(IADsADSystemInfo *iface, VARIANT *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static const IADsADSystemInfoVtbl IADsADSystemInfo_vtbl =
{
    sysinfo_QueryInterface,
    sysinfo_AddRef,
    sysinfo_Release,
    sysinfo_GetTypeInfoCount,
    sysinfo_GetTypeInfo,
    sysinfo_GetIDsOfNames,
    sysinfo_Invoke,
    sysinfo_get_UserName,
    sysinfo_get_ComputerName,
    sysinfo_get_SiteName,
    sysinfo_get_DomainShortName,
    sysinfo_get_DomainDNSName,
    sysinfo_get_ForestDNSName,
    sysinfo_get_PDCRoleOwner,
    sysinfo_get_SchemaRoleOwner,
    sysinfo_get_IsNativeMode,
    sysinfo_GetAnyDCName,
    sysinfo_GetDCSiteName,
    sysinfo_RefreshSchemaCache,
    sysinfo_GetTrees
};

static HRESULT ADSystemInfo_create(REFIID riid, void **obj)
{
    AD_sysinfo *sysinfo;
    HRESULT hr;

    sysinfo = heap_alloc(sizeof(*sysinfo));
    if (!sysinfo) return E_OUTOFMEMORY;

    sysinfo->IADsADSystemInfo_iface.lpVtbl = &IADsADSystemInfo_vtbl;
    sysinfo->ref = 1;

    hr = IADsADSystemInfo_QueryInterface(&sysinfo->IADsADSystemInfo_iface, riid, obj);
    IADsADSystemInfo_Release(&sysinfo->IADsADSystemInfo_iface);

    return hr;
}

struct ldap_attribute
{
    WCHAR *name;
    WCHAR **values;
};

typedef struct
{
    IADs IADs_iface;
    IADsOpenDSObject IADsOpenDSObject_iface;
    LONG ref;
    LDAP *ld;
    BSTR host;
    BSTR object;
    ULONG port;
    ULONG attrs_count, attrs_count_allocated;
    struct ldap_attribute *attrs;
} LDAP_namespace;

static inline LDAP_namespace *impl_from_IADs(IADs *iface)
{
    return CONTAINING_RECORD(iface, LDAP_namespace, IADs_iface);
}

static HRESULT WINAPI ldapns_QueryInterface(IADs *iface, REFIID riid, void **obj)
{
    LDAP_namespace *ldap = impl_from_IADs(iface);

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IADs))
    {
        IADs_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IADsOpenDSObject))
    {
        IADs_AddRef(iface);
        *obj = &ldap->IADsOpenDSObject_iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ldapns_AddRef(IADs *iface)
{
    LDAP_namespace *ldap = impl_from_IADs(iface);
    return InterlockedIncrement(&ldap->ref);
}

static void free_attributes(LDAP_namespace *ldap)
{
    ULONG i;

    if (!ldap->attrs) return;

    for (i = 0; i < ldap->attrs_count; i++)
    {
        ldap_memfreeW(ldap->attrs[i].name);
        ldap_value_freeW(ldap->attrs[i].values);
    }

    heap_free(ldap->attrs);
    ldap->attrs = NULL;
    ldap->attrs_count = 0;
}

static ULONG WINAPI ldapns_Release(IADs *iface)
{
    LDAP_namespace *ldap = impl_from_IADs(iface);
    LONG ref = InterlockedDecrement(&ldap->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        if (ldap->ld) ldap_unbind(ldap->ld);
        SysFreeString(ldap->host);
        SysFreeString(ldap->object);
        free_attributes(ldap);
        heap_free(ldap);
    }

    return ref;
}

static HRESULT WINAPI ldapns_GetTypeInfoCount(IADs *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_GetTypeInfo(IADs *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%#x,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_GetIDsOfNames(IADs *iface, REFIID riid, LPOLESTR *names,
                                           UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%u,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_Invoke(IADs *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                    DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%d,%s,%04x,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_get_Name(IADs *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_get_Class(IADs *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_get_GUID(IADs *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_get_ADsPath(IADs *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_get_Parent(IADs *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_get_Schema(IADs *iface, BSTR *retval)
{
    FIXME("%p,%p: stub\n", iface, retval);
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_GetInfo(IADs *iface)
{
    HRESULT hr;
    VARIANT var;

    TRACE("%p\n", iface);

    hr = ADsBuildVarArrayStr(NULL, 0, &var);
    if (hr == S_OK)
    {
        hr = IADs_GetInfoEx(iface, var, 0);
        VariantClear(&var);
    }
    return hr;
}

static HRESULT WINAPI ldapns_SetInfo(IADs *iface)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_Get(IADs *iface, BSTR name, VARIANT *prop)
{
    LDAP_namespace *ldap = impl_from_IADs(iface);
    HRESULT hr;
    ULONG i;

    TRACE("%p,%s,%p\n", iface, debugstr_w(name), prop);

    if (!name || !prop) return E_ADS_BAD_PARAMETER;

    if (!ldap->attrs_count)
    {
        hr = IADs_GetInfo(iface);
        if (hr != S_OK) return hr;
    }

    for (i = 0; i < ldap->attrs_count; i++)
    {
        if (!wcsicmp(name, ldap->attrs[i].name))
        {
            LONG count = ldap_count_valuesW(ldap->attrs[i].values);
            if (!count)
            {
                V_BSTR(prop) = NULL;
                V_VT(prop) = VT_BSTR;
                return S_OK;
            }

            if (count > 1)
            {
                SAFEARRAY *sa;
                VARIANT item;
                LONG idx;

                TRACE("attr %s has %u values\n", debugstr_w(ldap->attrs[i].name), count);

                sa = SafeArrayCreateVector(VT_VARIANT, 0, count);
                if (!sa) return E_OUTOFMEMORY;

                for (idx = 0; idx < count; idx++)
                {
                    V_VT(&item) = VT_BSTR;
                    V_BSTR(&item) = SysAllocString(ldap->attrs[i].values[idx]);
                    if (!V_BSTR(&item))
                    {
                        hr = E_OUTOFMEMORY;
                        goto fail;
                    }

                    hr = SafeArrayPutElement(sa, &idx, &item);
                    SysFreeString(V_BSTR(&item));
                    if (hr != S_OK) goto fail;
                }

                V_VT(prop) = VT_ARRAY | VT_VARIANT;
                V_ARRAY(prop) = sa;
                return S_OK;
fail:
                SafeArrayDestroy(sa);
                return hr;
            }
            else
            {
                V_BSTR(prop) = SysAllocString(ldap->attrs[i].values[0]);
                if (!V_BSTR(prop)) return E_OUTOFMEMORY;
                V_VT(prop) = VT_BSTR;
                return S_OK;
            }
        }
    }

    return E_ADS_PROPERTY_NOT_FOUND;
}

static HRESULT WINAPI ldapns_Put(IADs *iface, BSTR name, VARIANT prop)
{
    FIXME("%p,%s,%s: stub\n", iface, debugstr_w(name), wine_dbgstr_variant(&prop));
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_GetEx(IADs *iface, BSTR name, VARIANT *prop)
{
    FIXME("%p,%s,%p: stub\n", iface, debugstr_w(name), prop);
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_PutEx(IADs *iface, LONG code, BSTR name, VARIANT prop)
{
    FIXME("%p,%d,%s,%s: stub\n", iface, code, debugstr_w(name), wine_dbgstr_variant(&prop));
    return E_NOTIMPL;
}

static HRESULT add_attribute(LDAP_namespace *ldap, WCHAR *name, WCHAR **values)
{
    struct ldap_attribute *new_attrs;

    if (!ldap->attrs)
    {
        ldap->attrs = heap_alloc(256 * sizeof(ldap->attrs[0]));
        if (!ldap->attrs) return E_OUTOFMEMORY;
        ldap->attrs_count_allocated = 256;
    }
    else if (ldap->attrs_count_allocated < ldap->attrs_count + 1)
    {
        new_attrs = heap_realloc(ldap->attrs, (ldap->attrs_count_allocated * 2) * sizeof(*new_attrs));
        if (!new_attrs) return E_OUTOFMEMORY;

        ldap->attrs_count_allocated *= 2;
        ldap->attrs = new_attrs;
    }

    ldap->attrs[ldap->attrs_count].name = name;
    ldap->attrs[ldap->attrs_count].values = values;
    ldap->attrs_count++;

    return S_OK;
}

static HRESULT WINAPI ldapns_GetInfoEx(IADs *iface, VARIANT prop, LONG reserved)
{
    LDAP_namespace *ldap = impl_from_IADs(iface);
    HRESULT hr;
    SAFEARRAY *sa;
    VARIANT *item;
    WCHAR **props = NULL, *attr, **values;
    DWORD i, count, err;
    LDAPMessage *res = NULL, *entry;
    BerElement *ber;

    TRACE("%p,%s,%d\n", iface, wine_dbgstr_variant(&prop), reserved);

    free_attributes(ldap);

    if (!ldap->ld) return E_NOTIMPL;

    if (V_VT(&prop) != (VT_ARRAY | VT_VARIANT))
        return E_ADS_BAD_PARAMETER;

    sa = V_ARRAY(&prop);
    if (sa->cDims != 1)
        return E_ADS_BAD_PARAMETER;

    hr = SafeArrayAccessData(sa, (void *)&item);
    if (hr != S_OK) return hr;

    count = sa->rgsabound[0].cElements;
    if (count)
    {
        props = heap_alloc((count + 1) * sizeof(props[0]));
        if (!props)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        for (i = 0; i < count; i++)
        {
            if (V_VT(&item[i]) != VT_BSTR)
            {
                hr = E_ADS_BAD_PARAMETER;
                goto exit;
            }
            props[i] = V_BSTR(&item[i]);
        }
        props[sa->rgsabound[0].cElements] = NULL;
    }

    err = ldap_search_sW(ldap->ld, NULL, LDAP_SCOPE_BASE, (WCHAR *)L"(objectClass=*)", props, FALSE, &res);
    if (err != LDAP_SUCCESS)
    {
        TRACE("ldap_search_sW error %#x\n", err);
        hr = HRESULT_FROM_WIN32(map_ldap_error(err));
        goto exit;
    }

    entry = ldap_first_entry(ldap->ld, res);
    while (entry)
    {
        attr = ldap_first_attributeW(ldap->ld, entry, &ber);
        while (attr)
        {
            TRACE("attr: %s\n", debugstr_w(attr));

            values = ldap_get_valuesW(ldap->ld, entry, attr);

            hr = add_attribute(ldap, attr, values);
            if (hr != S_OK)
            {
                ldap_value_freeW(values);
                ldap_memfreeW(attr);
                goto exit;
            }

            attr = ldap_next_attributeW(ldap->ld, entry, ber);
        }

        entry = ldap_next_entry(ldap->ld, res);
    }

exit:
    if (res) ldap_msgfree(res);
    heap_free(props);
    SafeArrayUnaccessData(sa);
    return hr;
}

static const IADsVtbl IADs_vtbl =
{
    ldapns_QueryInterface,
    ldapns_AddRef,
    ldapns_Release,
    ldapns_GetTypeInfoCount,
    ldapns_GetTypeInfo,
    ldapns_GetIDsOfNames,
    ldapns_Invoke,
    ldapns_get_Name,
    ldapns_get_Class,
    ldapns_get_GUID,
    ldapns_get_ADsPath,
    ldapns_get_Parent,
    ldapns_get_Schema,
    ldapns_GetInfo,
    ldapns_SetInfo,
    ldapns_Get,
    ldapns_Put,
    ldapns_GetEx,
    ldapns_PutEx,
    ldapns_GetInfoEx
};

static inline LDAP_namespace *impl_from_IADsOpenDSObject(IADsOpenDSObject *iface)
{
    return CONTAINING_RECORD(iface, LDAP_namespace, IADsOpenDSObject_iface);
}

static HRESULT WINAPI openobj_QueryInterface(IADsOpenDSObject *iface, REFIID riid, void **obj)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    if (IsEqualGUID(riid, &IID_IADsOpenDSObject) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IADsOpenDSObject_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI openobj_AddRef(IADsOpenDSObject *iface)
{
    LDAP_namespace *ldap = impl_from_IADsOpenDSObject(iface);
    return InterlockedIncrement(&ldap->ref);
}

static ULONG WINAPI openobj_Release(IADsOpenDSObject *iface)
{
    LDAP_namespace *ldap = impl_from_IADsOpenDSObject(iface);
    LONG ref = InterlockedDecrement(&ldap->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        HeapFree(GetProcessHeap(), 0, ldap);
    }

    return ref;
}

static HRESULT WINAPI openobj_GetTypeInfoCount(IADsOpenDSObject *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI openobj_GetTypeInfo(IADsOpenDSObject *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%#x,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI openobj_GetIDsOfNames(IADsOpenDSObject *iface, REFIID riid, LPOLESTR *names,
                                            UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%u,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI openobj_Invoke(IADsOpenDSObject *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%d,%s,%04x,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT parse_path(WCHAR *path, BSTR *host, ULONG *port, BSTR *object)
{
    WCHAR *p, *p_host;
    int host_len;

    if (host) *host = NULL;
    if (port) *port = 0;
    if (object) *object = NULL;

    if (wcsnicmp(path, L"LDAP:", 5) != 0)
        return E_ADS_BAD_PATHNAME;

    p = path + 5;
    if (!*p) return S_OK;

    if (*p++ != '/' || *p++ != '/' || !*p)
        return E_ADS_BAD_PATHNAME;

    p_host = p;
    host_len = 0;
    while (*p && *p != '/')
    {
        if (*p == ':')
        {
            ULONG dummy;
            if (!port) port = &dummy;
            *port = wcstol(p + 1, &p, 10);
            if (*p && *p != '/') return E_ADS_BAD_PATHNAME;
        }
        else
        {
            p++;
            host_len++;
        }
    }
    if (host_len == 0) return E_ADS_BAD_PATHNAME;

    if (host)
    {
        *host = SysAllocStringLen(p_host, host_len);
        if (!*host) return E_OUTOFMEMORY;
    }

    if (!*p) return S_OK;

    if (*p++ != '/' || !*p)
    {
        SysFreeString(*host);
        return E_ADS_BAD_PATHNAME;
    }

    if (object)
    {
        *object = SysAllocString(p);
        if (!*object)
        {
            SysFreeString(*host);
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}

static HRESULT WINAPI openobj_OpenDSObject(IADsOpenDSObject *iface, BSTR path, BSTR user, BSTR password,
                                           LONG flags, IDispatch **obj)
{
    BSTR host, object;
    ULONG port;
    IADs *ads;
    LDAP *ld = NULL;
    HRESULT hr;
    ULONG err;

    FIXME("%p,%s,%s,%p,%08x,%p: semi-stub\n", iface, debugstr_w(path), debugstr_w(user), password, flags, obj);

    hr = parse_path(path, &host, &port, &object);
    if (hr != S_OK) return hr;

    TRACE("host %s, port %u, object %s\n", debugstr_w(host), port, debugstr_w(object));

    if (host)
    {
        int version;

        if (!wcsicmp(host, L"rootDSE"))
        {
            DOMAIN_CONTROLLER_INFOW *dcinfo;

            if (object)
            {
                hr = E_ADS_BAD_PATHNAME;
                goto fail;
            }

            object = host;

            err = DsGetDcNameW(NULL, NULL, NULL, NULL, DS_RETURN_DNS_NAME, &dcinfo);
            if (err != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(LdapGetLastError());
                goto fail;
            }

            host = SysAllocString(dcinfo->DomainName);
            NetApiBufferFree(dcinfo);

            if (!host)
            {
                hr = E_OUTOFMEMORY;
                goto fail;
            }
        }

        ld = ldap_initW(host, port);
        if (!ld)
        {
            hr = HRESULT_FROM_WIN32(LdapGetLastError());
            goto fail;
        }

        version = LDAP_VERSION3;
        err = ldap_set_optionW(ld, LDAP_OPT_PROTOCOL_VERSION, &version);
        if (err != LDAP_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(map_ldap_error(err));
            ldap_unbind(ld);
            goto fail;
        }

        err = ldap_connect(ld, NULL);
        if (err != LDAP_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(map_ldap_error(err));
            ldap_unbind(ld);
            goto fail;
        }

        if (flags & ADS_SECURE_AUTHENTICATION)
        {
            SEC_WINNT_AUTH_IDENTITY_W id;

            id.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
            id.Domain = (unsigned short *)host;
            id.DomainLength = wcslen(host);
            id.User = (unsigned short *)user;
            id.UserLength = user ? wcslen(user) : 0;
            id.Password = (unsigned short *)password;
            id.PasswordLength = password ? wcslen(password) : 0;

            err = ldap_bind_sW(ld, NULL, (WCHAR *)&id, LDAP_AUTH_NEGOTIATE);
            if (err != LDAP_SUCCESS)
            {
                TRACE("ldap_bind_sW error %#x\n", err);
                hr = HRESULT_FROM_WIN32(map_ldap_error(err));
                ldap_unbind(ld);
                goto fail;
            }
        }
        else
        {
            err = ldap_simple_bind_sW(ld, user, password);
            if (err != LDAP_SUCCESS)
            {
                TRACE("ldap_simple_bind_sW error %#x\n", err);
                hr = HRESULT_FROM_WIN32(map_ldap_error(err));
                ldap_unbind(ld);
                goto fail;
            }
        }
    }

    hr = LDAPNamespace_create(&IID_IADs, (void **)&ads);
    if (hr == S_OK)
    {
        LDAP_namespace *ldap = impl_from_IADs(ads);
        ldap->ld = ld;
        ldap->host = host;
        ldap->port = port;
        ldap->object = object;
        hr = IADs_QueryInterface(ads, &IID_IDispatch, (void **)obj);
        IADs_Release(ads);
        return hr;
    }

fail:
    SysFreeString(host);
    SysFreeString(object);

    return hr;
}

static const IADsOpenDSObjectVtbl IADsOpenDSObject_vtbl =
{
    openobj_QueryInterface,
    openobj_AddRef,
    openobj_Release,
    openobj_GetTypeInfoCount,
    openobj_GetTypeInfo,
    openobj_GetIDsOfNames,
    openobj_Invoke,
    openobj_OpenDSObject
};

static HRESULT LDAPNamespace_create(REFIID riid, void **obj)
{
    LDAP_namespace *ldap;
    HRESULT hr;

    ldap = heap_alloc(sizeof(*ldap));
    if (!ldap) return E_OUTOFMEMORY;

    ldap->IADs_iface.lpVtbl = &IADs_vtbl;
    ldap->IADsOpenDSObject_iface.lpVtbl = &IADsOpenDSObject_vtbl;
    ldap->ref = 1;
    ldap->ld = NULL;
    ldap->host = NULL;
    ldap->object = NULL;
    ldap->attrs_count = 0;
    ldap->attrs_count_allocated = 0;
    ldap->attrs = NULL;

    hr = IADs_QueryInterface(&ldap->IADs_iface, riid, obj);
    IADs_Release(&ldap->IADs_iface);

    return hr;
}

static const struct class_info
{
    const CLSID *clsid;
    HRESULT (*constructor)(REFIID, void **);
} class_info[] =
{
    { &CLSID_ADSystemInfo, ADSystemInfo_create },
    { &CLSID_LDAP, LDAP_create },
    { &CLSID_LDAPNamespace, LDAPNamespace_create },
};

typedef struct
{
    IClassFactory IClassFactory_iface;
    LONG ref;
    const struct class_info *info;
} class_factory;

static inline class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, class_factory, IClassFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(IClassFactory *iface, REFIID riid, LPVOID *obj)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IClassFactory *iface)
{
    class_factory *factory = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&factory->ref);

    TRACE("(%p) ref %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI factory_Release(IClassFactory *iface)
{
    class_factory *factory = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&factory->ref);

    TRACE("(%p) ref %u\n", iface, ref);

    if (!ref)
        heap_free(factory);

    return ref;
}

static HRESULT WINAPI factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    class_factory *factory = impl_from_IClassFactory(iface);

    TRACE("%p,%s,%p\n", outer, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    *obj = NULL;
    if (outer) return CLASS_E_NOAGGREGATION;

    return factory->info->constructor(riid, obj);
}

static HRESULT WINAPI factory_LockServer(IClassFactory *iface, BOOL lock)
{
    FIXME("%p,%d: stub\n", iface, lock);
    return S_OK;
}

static const struct IClassFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    factory_CreateInstance,
    factory_LockServer
};

static HRESULT factory_constructor(const struct class_info *info, REFIID riid, void **obj)
{
    class_factory *factory;
    HRESULT hr;

    factory = heap_alloc(sizeof(*factory));
    if (!factory) return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &factory_vtbl;
    factory->ref = 1;
    factory->info = info;

    hr = IClassFactory_QueryInterface(&factory->IClassFactory_iface, riid, obj);
    IClassFactory_Release(&factory->IClassFactory_iface);

    return hr;
}

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *obj)
{
    int i;

    TRACE("%s,%s,%p\n", debugstr_guid(clsid), debugstr_guid(iid), obj);

    if (!clsid || !iid || !obj) return E_INVALIDARG;

    *obj = NULL;

    for (i = 0; i < ARRAY_SIZE(class_info); i++)
    {
        if (IsEqualCLSID(class_info[i].clsid, clsid))
            return factory_constructor(&class_info[i], iid, obj);
    }

    FIXME("class %s/%s is not implemented\n", debugstr_guid(clsid), debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources(adsldp_hinst);
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources(adsldp_hinst);
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, void *reserved)
{
    TRACE("%p,%u,%p\n", hinst, reason, reserved);

    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE; /* prefer native version */

    case DLL_PROCESS_ATTACH:
        adsldp_hinst = hinst;
        DisableThreadLibraryCalls(hinst);
        break;
    }

    return TRUE;
}
