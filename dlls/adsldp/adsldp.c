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
#include "winber.h"

#include "adsldp_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(adsldp);

#ifndef LDAP_OPT_SERVER_CONTROLS
#define LDAP_OPT_SERVER_CONTROLS 0x0012
#endif

DEFINE_GUID(CLSID_LDAP,0x228d9a81,0xc302,0x11cf,0x9a,0xa4,0x00,0xaa,0x00,0x4a,0x56,0x91);
DEFINE_GUID(CLSID_LDAPNamespace,0x228d9a82,0xc302,0x11cf,0x9a,0xa4,0x00,0xaa,0x00,0x4a,0x56,0x91);

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
        free(ldap);
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

    ldap = malloc(sizeof(*ldap));
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
        free(sysinfo);
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
    FIXME("%p,%u,%#lx,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_GetIDsOfNames(IADsADSystemInfo *iface, REFIID riid, LPOLESTR *names,
                                            UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysinfo_Invoke(IADsADSystemInfo *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
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
    ULONG size;
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

    sysinfo = malloc(sizeof(*sysinfo));
    if (!sysinfo) return E_OUTOFMEMORY;

    sysinfo->IADsADSystemInfo_iface.lpVtbl = &IADsADSystemInfo_vtbl;
    sysinfo->ref = 1;

    hr = IADsADSystemInfo_QueryInterface(&sysinfo->IADsADSystemInfo_iface, riid, obj);
    IADsADSystemInfo_Release(&sysinfo->IADsADSystemInfo_iface);

    return hr;
}

typedef struct
{
    IADs IADs_iface;
    IADsOpenDSObject IADsOpenDSObject_iface;
    IDirectorySearch IDirectorySearch_iface;
    IDirectoryObject IDirectoryObject_iface;
    LONG ref;
    LDAP *ld;
    BSTR host;
    BSTR object;
    ULONG port;
    ULONG attrs_count, attrs_count_allocated;
    ADS_SEARCH_COLUMN *attrs;
    struct attribute_type *at;
    ULONG at_single_count, at_multiple_count;
    struct
    {
        ADS_SCOPEENUM scope;
        int pagesize;
        int size_limit;
        BOOL cache_results;
        BOOL attribtypes_only;
        BOOL tombstone;
    } search;
} LDAP_namespace;

struct ldap_search_context
{
    LDAPSearch *page;
    LDAPMessage *res, *entry;
    BerElement *ber;
    ULONG count, pos;
    BOOL add_ADsPath;
};

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

    if (IsEqualGUID(riid, &IID_IDirectorySearch))
    {
        if (!ldap->ld || (ldap->object && !wcsicmp(ldap->object, L"rootDSE")))
            return E_NOINTERFACE;

        IADs_AddRef(iface);
        *obj = &ldap->IDirectorySearch_iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IDirectoryObject))
    {
        IADs_AddRef(iface);
        *obj = &ldap->IDirectoryObject_iface;
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
        IDirectorySearch_FreeColumn(&ldap->IDirectorySearch_iface, &ldap->attrs[i]);

    free(ldap->attrs);
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
        free_attribute_types(ldap->at, ldap->at_single_count + ldap->at_multiple_count);
        free(ldap);
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
    FIXME("%p,%u,%#lx,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_GetIDsOfNames(IADs *iface, REFIID riid, LPOLESTR *names,
                                           UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI ldapns_Invoke(IADs *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                    DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
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

static HRESULT adsvalue_to_variant(const ADSVALUE *value, VARIANT *var)
{
    HRESULT hr = S_OK;
    void *val;

    switch (value->dwType)
    {
    default:
        FIXME("no special handling for type %d\n", value->dwType);
        /* fall through */
    case ADSTYPE_DN_STRING:
    case ADSTYPE_CASE_EXACT_STRING:
    case ADSTYPE_CASE_IGNORE_STRING:
    case ADSTYPE_PRINTABLE_STRING:
        val = SysAllocString(value->CaseIgnoreString);
        if (!val) return E_OUTOFMEMORY;

        V_VT(var) = VT_BSTR;
        V_BSTR(var) = val;
        break;

    case ADSTYPE_BOOLEAN:
        V_VT(var) = VT_BOOL;
        V_BOOL(var) = value->Boolean ? VARIANT_TRUE : VARIANT_FALSE;
        break;

    case ADSTYPE_INTEGER:
        V_VT(var) = VT_I4;
        V_I4(var) = value->Integer;
        break;

    case ADSTYPE_LARGE_INTEGER:
        V_VT(var) = VT_I8;
        V_I8(var) = value->LargeInteger.QuadPart;
        break;

    case ADSTYPE_OCTET_STRING:
    case ADSTYPE_NT_SECURITY_DESCRIPTOR:
    {
        SAFEARRAY *sa;

        sa = SafeArrayCreateVector(VT_UI1, 0, value->OctetString.dwLength);
        if (!sa) return E_OUTOFMEMORY;

        memcpy(sa->pvData, value->OctetString.lpValue, value->OctetString.dwLength);
        V_VT(var) = VT_ARRAY | VT_UI1;
        V_ARRAY(var) = sa;
        break;
    }

    case ADSTYPE_UTC_TIME:
    {
        DOUBLE date;

        if (!SystemTimeToVariantTime((SYSTEMTIME *)&value->UTCTime, &date))
            return E_FAIL;

        V_VT(var) = VT_DATE;
        V_DATE(var) = date;
        break;
    }

    case ADSTYPE_DN_WITH_BINARY:
    {
        SAFEARRAY *sa;
        IADsDNWithBinary *dn = NULL;
        VARIANT data;

        sa = SafeArrayCreateVector(VT_UI1, 0, value->pDNWithBinary->dwLength);
        if (!sa) return E_OUTOFMEMORY;
        memcpy(sa->pvData, value->pDNWithBinary->lpBinaryValue, value->pDNWithBinary->dwLength);

        hr = CoCreateInstance(&CLSID_DNWithBinary, 0, CLSCTX_INPROC_SERVER, &IID_IADsDNWithBinary, (void **)&dn);
        if (hr != S_OK) goto fail;

        V_VT(&data) = VT_ARRAY | VT_UI1;
        V_ARRAY(&data) = sa;
        hr = IADsDNWithBinary_put_BinaryValue(dn, data);
        if (hr != S_OK) goto fail;

        hr = IADsDNWithBinary_put_DNString(dn, value->pDNWithBinary->pszDNString);
        if (hr != S_OK) goto fail;

        V_VT(var) = VT_DISPATCH;
        V_DISPATCH(var) = (IDispatch *)dn;
        break;
fail:
        SafeArrayDestroy(sa);
        if (dn) IADsDNWithBinary_Release(dn);
        return hr;

    }
    }

    TRACE("=> %s\n", wine_dbgstr_variant(var));
    return hr;
}

static HRESULT adsvalues_to_array(const ADSVALUE *values, LONG count, VARIANT *var)
{
    HRESULT hr;
    SAFEARRAY *sa;
    VARIANT item;
    LONG i;

    sa = SafeArrayCreateVector(VT_VARIANT, 0, count);
    if (!sa) return E_OUTOFMEMORY;

    for (i = 0; i < count; i++)
    {
        hr = adsvalue_to_variant(&values[i], &item);
        if (hr != S_OK) goto fail;

        hr = SafeArrayPutElement(sa, &i, &item);
        VariantClear(&item);
        if (hr != S_OK) goto fail;
    }

    V_VT(var) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(var) = sa;
    return S_OK;

fail:
    SafeArrayDestroy(sa);
    return hr;
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
        if (!wcsicmp(name, ldap->attrs[i].pszAttrName))
        {
            if (!ldap->attrs[i].dwNumValues)
            {
                V_BSTR(prop) = NULL;
                V_VT(prop) = VT_BSTR;
                return S_OK;
            }

            TRACE("attr %s has %lu values\n", debugstr_w(ldap->attrs[i].pszAttrName), ldap->attrs[i].dwNumValues);

            if (ldap->attrs[i].dwNumValues > 1)
                return adsvalues_to_array(ldap->attrs[i].pADsValues, ldap->attrs[i].dwNumValues, prop);
            else
                return adsvalue_to_variant(&ldap->attrs[i].pADsValues[0], prop);
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
        if (!wcsicmp(name, ldap->attrs[i].pszAttrName))
        {
            if (!ldap->attrs[i].dwNumValues)
            {
                V_BSTR(prop) = NULL;
                V_VT(prop) = VT_BSTR;
                return S_OK;
            }

            TRACE("attr %s has %lu values\n", debugstr_w(ldap->attrs[i].pszAttrName), ldap->attrs[i].dwNumValues);

            return adsvalues_to_array(ldap->attrs[i].pADsValues, ldap->attrs[i].dwNumValues, prop);
        }
    }

    return E_ADS_PROPERTY_NOT_FOUND;
}

static HRESULT WINAPI ldapns_PutEx(IADs *iface, LONG code, BSTR name, VARIANT prop)
{
    FIXME("%p,%ld,%s,%s: stub\n", iface, code, debugstr_w(name), wine_dbgstr_variant(&prop));
    return E_NOTIMPL;
}

static HRESULT add_attribute(LDAP_namespace *ldap, ADS_SEARCH_COLUMN *attr)
{
    ADS_SEARCH_COLUMN *new_attrs;

    if (!ldap->attrs)
    {
        ldap->attrs = malloc(256 * sizeof(ldap->attrs[0]));
        if (!ldap->attrs) return E_OUTOFMEMORY;
        ldap->attrs_count_allocated = 256;
    }
    else if (ldap->attrs_count_allocated < ldap->attrs_count + 1)
    {
        new_attrs = realloc(ldap->attrs, (ldap->attrs_count_allocated * 2) * sizeof(*new_attrs));
        if (!new_attrs) return E_OUTOFMEMORY;

        ldap->attrs_count_allocated *= 2;
        ldap->attrs = new_attrs;
    }

    ldap->attrs[ldap->attrs_count] = *attr;
    ldap->attrs_count++;

    return S_OK;
}

static HRESULT WINAPI ldapns_GetInfoEx(IADs *iface, VARIANT prop, LONG reserved)
{
    LDAP_namespace *ldap = impl_from_IADs(iface);
    HRESULT hr;
    SAFEARRAY *sa;
    VARIANT *item;
    WCHAR **props = NULL;
    DWORD i, count;
    ADS_SEARCHPREF_INFO pref[3];
    ADS_SEARCH_HANDLE sh;
    ADS_SEARCH_COLUMN col;

    TRACE("%p,%s,%ld\n", iface, wine_dbgstr_variant(&prop), reserved);

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
        props = malloc((count + 1) * sizeof(props[0]));
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
    }
    else
        count = ~0;

    pref[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    pref[0].vValue.dwType = ADSTYPE_INTEGER;
    pref[0].vValue.Integer = ADS_SCOPE_BASE;
    pref[0].dwStatus = 0;

    pref[1].dwSearchPref = ADS_SEARCHPREF_ATTRIBTYPES_ONLY;
    pref[1].vValue.dwType = ADSTYPE_BOOLEAN;
    pref[1].vValue.Boolean = FALSE;
    pref[1].dwStatus = 0;

    pref[2].dwSearchPref = ADS_SEARCHPREF_SIZE_LIMIT;
    pref[2].vValue.dwType = ADSTYPE_INTEGER;
    pref[2].vValue.Integer = 20;
    pref[2].dwStatus = 0;

    IDirectorySearch_SetSearchPreference(&ldap->IDirectorySearch_iface, pref, ARRAY_SIZE(pref));

    hr = IDirectorySearch_ExecuteSearch(&ldap->IDirectorySearch_iface, (WCHAR *)L"(objectClass=*)", props, count, &sh);
    if (hr != S_OK)
    {
        TRACE("ExecuteSearch error %#lx\n", hr);
        goto exit;
    }

    while (IDirectorySearch_GetNextRow(&ldap->IDirectorySearch_iface, sh) != S_ADS_NOMORE_ROWS)
    {
        WCHAR *name;
        while (IDirectorySearch_GetNextColumnName(&ldap->IDirectorySearch_iface, sh, &name) != S_ADS_NOMORE_COLUMNS)
        {
            hr = IDirectorySearch_GetColumn(&ldap->IDirectorySearch_iface, sh, name, &col);
            if (hr == S_OK)
                add_attribute(ldap, &col);

            FreeADsMem(name);
        }
    }

    IDirectorySearch_CloseSearchHandle(&ldap->IDirectorySearch_iface, sh);
exit:
    free(props);
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
    return IADs_AddRef(&ldap->IADs_iface);
}

static ULONG WINAPI openobj_Release(IADsOpenDSObject *iface)
{
    LDAP_namespace *ldap = impl_from_IADsOpenDSObject(iface);
    return IADs_Release(&ldap->IADs_iface);
}

static HRESULT WINAPI openobj_GetTypeInfoCount(IADsOpenDSObject *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI openobj_GetTypeInfo(IADsOpenDSObject *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%#lx,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI openobj_GetIDsOfNames(IADsOpenDSObject *iface, REFIID riid, LPOLESTR *names,
                                            UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI openobj_Invoke(IADsOpenDSObject *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
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
    ULONG err, at_single_count = 0, at_multiple_count = 0;
    struct attribute_type *at = NULL;

    TRACE("%p,%s,%s,%p,%08lx,%p\n", iface, debugstr_w(path), debugstr_w(user), password, flags, obj);

    hr = parse_path(path, &host, &port, &object);
    if (hr != S_OK) return hr;

    TRACE("host %s, port %lu, object %s\n", debugstr_w(host), port, debugstr_w(object));

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
                hr = HRESULT_FROM_WIN32(err);
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
                TRACE("ldap_bind_sW error %#lx\n", err);
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
                TRACE("ldap_simple_bind_sW error %#lx\n", err);
                hr = HRESULT_FROM_WIN32(map_ldap_error(err));
                ldap_unbind(ld);
                goto fail;
            }
        }

        at = load_schema(ld, &at_single_count, &at_multiple_count);
    }

    hr = LDAPNamespace_create(&IID_IADs, (void **)&ads);
    if (hr == S_OK)
    {
        LDAP_namespace *ldap = impl_from_IADs(ads);
        ldap->ld = ld;
        ldap->host = host;
        ldap->port = port;
        ldap->object = object;
        ldap->at = at;
        ldap->at_single_count = at_single_count;
        ldap->at_multiple_count = at_multiple_count;
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

static inline LDAP_namespace *impl_from_IDirectorySearch(IDirectorySearch *iface)
{
    return CONTAINING_RECORD(iface, LDAP_namespace, IDirectorySearch_iface);
}

static HRESULT WINAPI search_QueryInterface(IDirectorySearch *iface, REFIID riid, void **obj)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    if (IsEqualGUID(riid, &IID_IDirectorySearch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirectorySearch_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI search_AddRef(IDirectorySearch *iface)
{
    LDAP_namespace *ldap = impl_from_IDirectorySearch(iface);
    return IADs_AddRef(&ldap->IADs_iface);
}

static ULONG WINAPI search_Release(IDirectorySearch *iface)
{
    LDAP_namespace *ldap = impl_from_IDirectorySearch(iface);
    return IADs_Release(&ldap->IADs_iface);
}

static HRESULT WINAPI search_SetSearchPreference(IDirectorySearch *iface, PADS_SEARCHPREF_INFO prefs, DWORD count)
{
    LDAP_namespace *ldap = impl_from_IDirectorySearch(iface);
    HRESULT hr = S_OK;
    DWORD i;

    TRACE("%p,%p,%lu\n", iface, prefs, count);

    for (i = 0; i < count; i++)
    {
        switch (prefs[i].dwSearchPref)
        {
        case ADS_SEARCHPREF_SEARCH_SCOPE:
            if (prefs[i].vValue.dwType != ADSTYPE_INTEGER)
            {
                FIXME("ADS_SEARCHPREF_SEARCH_SCOPE: unsupported dwType %d\n", prefs[i].vValue.dwType);
                prefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                break;
            }

            switch (prefs[i].vValue.Integer)
            {
            case ADS_SCOPE_BASE:
            case ADS_SCOPE_ONELEVEL:
            case ADS_SCOPE_SUBTREE:
                TRACE("SEARCH_SCOPE: %ld\n", prefs[i].vValue.Integer);
                ldap->search.scope = prefs[i].vValue.Integer;
                prefs[i].dwStatus = ADS_STATUS_S_OK;
                break;

            default:
                prefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                break;
            }
            break;

        case ADS_SEARCHPREF_SECURITY_MASK:
        {
            int security_mask;
            ULONG err;
            BerElement *ber;
            struct berval *berval;
            LDAPControlW *ctrls[2], mask;

            if (prefs[i].vValue.dwType != ADSTYPE_INTEGER)
            {
                FIXME("ADS_SEARCHPREF_SECURITY_MASK: not supported dwType %d\n", prefs[i].vValue.dwType);
                prefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                break;
            }

            TRACE("SECURITY_MASK: %08lx\n", prefs[i].vValue.Integer);
            security_mask = prefs[i].vValue.Integer;
            if (!security_mask)
                security_mask = ADS_SECURITY_INFO_OWNER;

            ber = ber_alloc_t(LBER_USE_DER);
            if (!ber || ber_printf(ber, (char *)"{i}", security_mask) == -1 || ber_flatten(ber, &berval) == -1)
            {
                ber_free(ber, 1);
                prefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREF;
                break;
            }
            TRACE("ber: %s\n", debugstr_an(berval->bv_val, berval->bv_len));

            mask.ldctl_oid = (WCHAR *)L"1.2.840.113556.1.4.801";
            mask.ldctl_iscritical = TRUE;
            mask.ldctl_value.bv_val = berval->bv_val;
            mask.ldctl_value.bv_len = berval->bv_len;
            ctrls[0] = &mask;
            ctrls[1] = NULL;
            err = ldap_set_optionW(ldap->ld, LDAP_OPT_SERVER_CONTROLS, ctrls);
            if (err != LDAP_SUCCESS)
            {
                TRACE("ldap_set_option error %#lx\n", err);
                prefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREF;
                hr = S_ADS_ERRORSOCCURRED;
            }
            else
                prefs[i].dwStatus = ADS_STATUS_S_OK;

            ber_bvfree(berval);
            ber_free(ber, 1);
            break;
        }

        case ADS_SEARCHPREF_PAGESIZE:
            if (prefs[i].vValue.dwType != ADSTYPE_INTEGER)
            {
                FIXME("ADS_SEARCHPREF_PAGESIZE: unsupported dwType %d\n", prefs[i].vValue.dwType);
                prefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                break;
            }

            TRACE("PAGESIZE: %ld\n", prefs[i].vValue.Integer);
            ldap->search.pagesize = prefs[i].vValue.Integer;
            prefs[i].dwStatus = ADS_STATUS_S_OK;
            break;

        case ADS_SEARCHPREF_CACHE_RESULTS:
            if (prefs[i].vValue.dwType != ADSTYPE_BOOLEAN)
            {
                FIXME("ADS_SEARCHPREF_CACHE_RESULTS: unsupported dwType %d\n", prefs[i].vValue.dwType);
                prefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                break;
            }

            TRACE("CACHE_RESULTS: %ld\n", prefs[i].vValue.Boolean);
            ldap->search.cache_results = prefs[i].vValue.Boolean;
            prefs[i].dwStatus = ADS_STATUS_S_OK;
            break;

        case ADS_SEARCHPREF_ATTRIBTYPES_ONLY:
            if (prefs[i].vValue.dwType != ADSTYPE_BOOLEAN)
            {
                FIXME("ADS_SEARCHPREF_ATTRIBTYPES_ONLY: unsupported dwType %d\n", prefs[i].vValue.dwType);
                prefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                break;
            }

            TRACE("ATTRIBTYPES_ONLY: %ld\n", prefs[i].vValue.Boolean);
            ldap->search.attribtypes_only = prefs[i].vValue.Boolean;
            prefs[i].dwStatus = ADS_STATUS_S_OK;
            break;

        case ADS_SEARCHPREF_TOMBSTONE:
            if (prefs[i].vValue.dwType != ADSTYPE_BOOLEAN)
            {
                FIXME("ADS_SEARCHPREF_TOMBSTONE: unsupported dwType %d\n", prefs[i].vValue.dwType);
                prefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                break;
            }

            TRACE("TOMBSTONE: %ld\n", prefs[i].vValue.Boolean);
            ldap->search.tombstone = prefs[i].vValue.Boolean;
            prefs[i].dwStatus = ADS_STATUS_S_OK;
            break;

        case ADS_SEARCHPREF_SIZE_LIMIT:
            if (prefs[i].vValue.dwType != ADSTYPE_INTEGER)
            {
                FIXME("ADS_SEARCHPREF_SIZE_LIMIT: unsupported dwType %d\n", prefs[i].vValue.dwType);
                prefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                break;
            }

            TRACE("SIZE_LIMIT: %ld\n", prefs[i].vValue.Integer);
            ldap->search.size_limit = prefs[i].vValue.Integer;
            prefs[i].dwStatus = ADS_STATUS_S_OK;
            break;

        default:
            FIXME("pref %d, type %u: stub\n", prefs[i].dwSearchPref, prefs[i].vValue.dwType);
            prefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREF;
            break;
        }
    }

    return hr;
}

static HRESULT WINAPI search_ExecuteSearch(IDirectorySearch *iface, LPWSTR filter, LPWSTR *names,
                                           DWORD count, PADS_SEARCH_HANDLE res)
{
    LDAP_namespace *ldap = impl_from_IDirectorySearch(iface);
    ULONG err, i;
    WCHAR **props, *object;
    LDAPControlW **ctrls = NULL, *ctrls_a[2], tombstone;
    struct ldap_search_context *ldap_ctx;

    TRACE("%p,%s,%p,%lu,%p\n", iface, debugstr_w(filter), names, count, res);

    if (!res) return E_ADS_BAD_PARAMETER;

    ldap_ctx = calloc(1, sizeof(*ldap_ctx));
    if (!ldap_ctx) return E_OUTOFMEMORY;

    if (count == 0xffffffff)
        props = NULL;
    else
    {
        if (count && !names)
        {
            free(ldap_ctx);
            return E_ADS_BAD_PARAMETER;
        }

        props = malloc((count + 1) * sizeof(props[0]));
        if (!props)
        {
            free(ldap_ctx);
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < count; i++)
        {
            TRACE("=> %s\n", debugstr_w(names[i]));
            props[i] = names[i];
        }

        props[count] = NULL;
    }

    if (ldap->search.tombstone)
    {
        tombstone.ldctl_oid = (WCHAR *)L"1.2.840.113556.1.4.417";
        tombstone.ldctl_iscritical = TRUE;
        tombstone.ldctl_value.bv_val = NULL;
        tombstone.ldctl_value.bv_len = 0;
        ctrls_a[0] = &tombstone;
        ctrls_a[1] = NULL;
        ctrls = ctrls_a;
    }

    object = ldap->object;
    if (object && !wcsicmp(object, L"rootDSE"))
        object = NULL;

    if (ldap->search.pagesize)
    {
        ldap_ctx->page = ldap_search_init_pageW(ldap->ld, object, ldap->search.scope,
                                                filter, props, ldap->search.attribtypes_only,
                                                ctrls, NULL, 0, ldap->search.size_limit, NULL);
        if (ldap_ctx->page)
            err = ldap_get_next_page_s(ldap->ld, ldap_ctx->page, NULL,
                                       ldap->search.pagesize, &count, &ldap_ctx->res);
        else
            err = LDAP_NO_MEMORY;
    }
    else
        err = ldap_search_ext_sW(ldap->ld, object, ldap->search.scope, filter, props,
                                 ldap->search.attribtypes_only, ctrls, NULL, NULL, ldap->search.size_limit,
                                 &ldap_ctx->res);
    free(props);
    if (err != LDAP_SUCCESS)
    {
        TRACE("ldap_search_sW error %#lx\n", err);
        if (ldap_ctx->res)
            ldap_msgfree(ldap_ctx->res);
        if (ldap_ctx->page)
            ldap_search_abandon_page(ldap->ld, ldap_ctx->page);
        free(ldap_ctx);
        return HRESULT_FROM_WIN32(map_ldap_error(err));
    }

    *res = ldap_ctx;
    return S_OK;
}

static HRESULT WINAPI search_AbandonSearch(IDirectorySearch *iface, ADS_SEARCH_HANDLE res)
{
    FIXME("%p,%p: stub\n", iface, res);
    return E_NOTIMPL;
}

static HRESULT WINAPI search_GetFirstRow(IDirectorySearch *iface, ADS_SEARCH_HANDLE res)
{
    struct ldap_search_context *ldap_ctx = res;

    TRACE("%p,%p\n", iface, res);

    if (!res) return E_ADS_BAD_PARAMETER;

    ldap_ctx->entry = NULL;

    return IDirectorySearch_GetNextRow(iface, res);
}

static HRESULT WINAPI search_GetNextRow(IDirectorySearch *iface, ADS_SEARCH_HANDLE res)
{
    LDAP_namespace *ldap = impl_from_IDirectorySearch(iface);
    struct ldap_search_context *ldap_ctx = res;

    TRACE("%p,%p\n", iface, res);

    if (!res) return E_ADS_BAD_PARAMETER;

    if (!ldap_ctx->entry)
    {
        ldap_ctx->count = ldap_count_entries(ldap->ld, ldap_ctx->res);
        ldap_ctx->pos = 0;

        if (ldap_ctx->pos >= ldap_ctx->count)
            return S_ADS_NOMORE_ROWS;

        ldap_ctx->entry = ldap_first_entry(ldap->ld, ldap_ctx->res);
    }
    else
    {
        if (ldap_ctx->pos >= ldap_ctx->count)
        {
            if (ldap_ctx->page)
            {
                ULONG err, count;

                ldap_msgfree(ldap_ctx->res);
                ldap_ctx->res = NULL;

                err = ldap_get_next_page_s(ldap->ld, ldap_ctx->page, NULL, ldap->search.pagesize, &count, &ldap_ctx->res);
                if (err == LDAP_SUCCESS)
                {
                    ldap_ctx->count = ldap_count_entries(ldap->ld, ldap_ctx->res);
                    ldap_ctx->pos = 0;

                    if (ldap_ctx->pos >= ldap_ctx->count)
                        return S_ADS_NOMORE_ROWS;

                    ldap_ctx->entry = ldap_first_entry(ldap->ld, ldap_ctx->res);
                    goto exit;
                }

                if (err != LDAP_NO_RESULTS_RETURNED)
                {
                    TRACE("ldap_get_next_page_s error %#lx\n", err);
                    return HRESULT_FROM_WIN32(map_ldap_error(err));
                }
                /* fall through */
            }

            return S_ADS_NOMORE_ROWS;
        }

        ldap_ctx->entry = ldap_next_entry(ldap->ld, ldap_ctx->entry);
    }

exit:
    if (!ldap_ctx->entry)
        return S_ADS_NOMORE_ROWS;

    ldap_ctx->pos++;
    if (ldap_ctx->ber) ber_free(ldap_ctx->ber, 0);
    ldap_ctx->ber = NULL;

    return S_OK;
}

static HRESULT WINAPI search_GetPreviousRow(IDirectorySearch *iface, ADS_SEARCH_HANDLE res)
{
    FIXME("%p,%p: stub\n", iface, res);
    return E_NOTIMPL;
}

static HRESULT WINAPI search_GetNextColumnName(IDirectorySearch *iface, ADS_SEARCH_HANDLE res, LPWSTR *name)
{
    LDAP_namespace *ldap = impl_from_IDirectorySearch(iface);
    struct ldap_search_context *ldap_ctx = res;
    WCHAR *attr;

    TRACE("%p,%p,%p\n", iface, res, name);

    if (!name || !ldap_ctx || !ldap_ctx->entry) return E_ADS_BAD_PARAMETER;

    if (!ldap_ctx->ber)
    {
        attr = ldap_first_attributeW(ldap->ld, ldap_ctx->entry, &ldap_ctx->ber);
        ldap_ctx->add_ADsPath = TRUE;
    }
    else
        attr = ldap_next_attributeW(ldap->ld, ldap_ctx->entry, ldap_ctx->ber);

    if (attr)
    {
        TRACE("=> %s\n", debugstr_w(attr));
        *name = AllocADsStr(attr);
        ldap_memfreeW(attr);
        return *name ? S_OK : E_OUTOFMEMORY;
    }
    else if (ldap_ctx->add_ADsPath)
    {
        ldap_ctx->add_ADsPath = FALSE;
        *name = AllocADsStr((WCHAR *)L"ADsPath");
        TRACE("=> %s\n", debugstr_w(*name));
        return *name ? S_OK : E_OUTOFMEMORY;
    }

    *name = NULL;
    return S_ADS_NOMORE_COLUMNS;
}

static HRESULT add_column_values(LDAP_namespace *ldap, struct ldap_search_context *ldap_ctx,
                                 LPWSTR name, ADS_SEARCH_COLUMN *col)
{
    ADSTYPEENUM type;
    DWORD i, count;

    type = get_schema_type(name, ldap->at, ldap->at_single_count, ldap->at_multiple_count);
    TRACE("%s => type %d\n", debugstr_w(name), type);

    switch (type)
    {
    default:
        FIXME("no special handling for type %d\n", type);
        /* fall through */
    case ADSTYPE_DN_STRING:
    case ADSTYPE_CASE_EXACT_STRING:
    case ADSTYPE_CASE_IGNORE_STRING:
    case ADSTYPE_PRINTABLE_STRING:
    {
        WCHAR **values = ldap_get_valuesW(ldap->ld, ldap_ctx->entry, name);
        if (!values)
            return E_ADS_COLUMN_NOT_SET;
        count = ldap_count_valuesW(values);

        col->pADsValues = calloc(count, sizeof(col->pADsValues[0]));
        if (!col->pADsValues)
        {
            ldap_value_freeW(values);
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < count; i++)
        {
            TRACE("=> %s\n", debugstr_w(values[i]));
            col->pADsValues[i].dwType = type;
            col->pADsValues[i].CaseIgnoreString = values[i];
        }

        col->hReserved = values;
        break;
    }

    case ADSTYPE_BOOLEAN:
    {
        WCHAR **values = ldap_get_valuesW(ldap->ld, ldap_ctx->entry, name);
        if (!values)
            return E_ADS_COLUMN_NOT_SET;
        count = ldap_count_valuesW(values);

        col->pADsValues = calloc(count, sizeof(col->pADsValues[0]));
        if (!col->pADsValues)
        {
            ldap_value_freeW(values);
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < count; i++)
        {
            col->pADsValues[i].dwType = type;

            if (!wcsicmp(values[i], L"TRUE"))
                col->pADsValues[i].Boolean = 1;
            else if (!wcsicmp(values[i], L"FALSE"))
                col->pADsValues[i].Boolean = 0;
            else
            {
                FIXME("not recognized boolean value %s\n", debugstr_w(values[i]));
                col->pADsValues[i].Boolean = 0;
            }
            TRACE("%s => %ld\n", debugstr_w(values[i]), col->pADsValues[i].Boolean);
        }

        ldap_value_freeW(values);
        col->hReserved = NULL;
        break;
    }

    case ADSTYPE_INTEGER:
    case ADSTYPE_LARGE_INTEGER:
    {
        struct berval **values = ldap_get_values_lenW(ldap->ld, ldap_ctx->entry, name);
        if (!values)
            return E_ADS_COLUMN_NOT_SET;
        count = ldap_count_values_len(values);

        col->pADsValues = calloc(count, sizeof(col->pADsValues[0]));
        if (!col->pADsValues)
        {
            ldap_value_free_len(values);
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < count; i++)
        {
            col->pADsValues[i].dwType = type;

            if (type == ADSTYPE_LARGE_INTEGER)
            {
                col->pADsValues[i].LargeInteger.QuadPart = _atoi64(values[i]->bv_val);
                TRACE("%s => %s\n", debugstr_an(values[i]->bv_val, values[i]->bv_len), wine_dbgstr_longlong(col->pADsValues[i].LargeInteger.QuadPart));
            }
            else
            {
                col->pADsValues[i].Integer = atol(values[i]->bv_val);
                TRACE("%s => %ld\n", debugstr_an(values[i]->bv_val, values[i]->bv_len), col->pADsValues[i].Integer);
            }
        }

        ldap_value_free_len(values);
        col->hReserved = NULL;
        break;
    }

    case ADSTYPE_OCTET_STRING:
    case ADSTYPE_NT_SECURITY_DESCRIPTOR:
    {
        struct berval **values = ldap_get_values_lenW(ldap->ld, ldap_ctx->entry, name);
        if (!values)
            return E_ADS_COLUMN_NOT_SET;
        count = ldap_count_values_len(values);

        col->pADsValues = calloc(count, sizeof(col->pADsValues[0]));
        if (!col->pADsValues)
        {
            ldap_value_free_len(values);
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < count; i++)
        {
            TRACE("=> %s\n", debugstr_an(values[i]->bv_val, values[i]->bv_len));
            col->pADsValues[i].dwType = type;
            col->pADsValues[i].OctetString.dwLength = values[i]->bv_len;
            col->pADsValues[i].OctetString.lpValue = (BYTE *)values[i]->bv_val;
        }

        col->hReserved = values;
        break;
    }

    case ADSTYPE_UTC_TIME:
    {
        struct berval **values = ldap_get_values_lenW(ldap->ld, ldap_ctx->entry, name);
        if (!values)
            return E_ADS_COLUMN_NOT_SET;
        count = ldap_count_values_len(values);

        col->pADsValues = calloc(count, sizeof(col->pADsValues[0]));
        if (!col->pADsValues)
        {
            ldap_value_free_len(values);
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < count; i++)
        {
            col->pADsValues[i].dwType = type;
            if (values[i]->bv_len < 14 ||
                _snscanf_l(values[i]->bv_val, values[i]->bv_len, "%04hu%02hu%02hu%02hu%02hu%02hu", NULL,
                           &col->pADsValues[i].UTCTime.wYear, &col->pADsValues[i].UTCTime.wMonth,
                           &col->pADsValues[i].UTCTime.wDay, &col->pADsValues[i].UTCTime.wHour,
                           &col->pADsValues[i].UTCTime.wMinute, &col->pADsValues[i].UTCTime.wSecond) != 6)
            {
                FIXME("not recognized UTCTime: %s\n", debugstr_an(values[i]->bv_val, values[i]->bv_len));
                memset(&col->pADsValues[i].UTCTime, 0, sizeof(col->pADsValues[i].UTCTime));
                continue;
            }

            if ((values[i]->bv_val[14] != '.' && values[i]->bv_val[14] != ',') ||
                values[i]->bv_val[15] != '0' || values[i]->bv_val[16] != 'Z')
                    FIXME("not handled time zone: %s\n", debugstr_an(values[i]->bv_val + 14, values[i]->bv_len - 14));

            TRACE("%s => %02u.%02u.%04u %02u:%02u:%02u\n", debugstr_an(values[i]->bv_val, values[i]->bv_len),
                  col->pADsValues[i].UTCTime.wDay, col->pADsValues[i].UTCTime.wMonth,
                  col->pADsValues[i].UTCTime.wYear, col->pADsValues[i].UTCTime.wHour,
                  col->pADsValues[i].UTCTime.wMinute, col->pADsValues[i].UTCTime.wSecond);
        }

        ldap_value_free_len(values);
        col->hReserved = NULL;
        break;
    }

    case ADSTYPE_DN_WITH_BINARY:
    {
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
        ADS_DN_WITH_BINARY *dnb;
        WCHAR **values = ldap_get_valuesW(ldap->ld, ldap_ctx->entry, name);
        if (!values)
            return E_ADS_COLUMN_NOT_SET;
        count = ldap_count_valuesW(values);

        col->pADsValues = calloc(count, sizeof(col->pADsValues[0]) + sizeof(col->pADsValues[0].pDNWithBinary[0]));
        if (!col->pADsValues)
        {
            ldap_value_freeW(values);
            return E_OUTOFMEMORY;
        }

        dnb = (ADS_DN_WITH_BINARY *)(col->pADsValues + count);

        for (i = 0; i < count; i++)
        {
            WCHAR *p = values[i];
            DWORD n;

            col->pADsValues[i].dwType = type;
            col->pADsValues[i].pDNWithBinary = dnb++;

            if ((p[0] != 'b' && p[0] != 'B') || p[1] != ':')
                FIXME("wrong DN with binary tag '%c%c'\n", p[0], p[1]);
            p += 2;

            col->pADsValues[i].pDNWithBinary->dwLength = wcstol(p, &p, 10) / 2;
            if (*p != ':')
                FIXME("wrong DN with binary separator '%c'\n", *p);
            p++;
            col->pADsValues[i].pDNWithBinary->lpBinaryValue = (BYTE *)p;
            /* decode values in-place */
            for (n = 0; n < col->pADsValues[i].pDNWithBinary->dwLength; n++, p += 2)
            {
                BYTE b;

                if (p[0] > 'f' || (p[0] != '0' && !hex2bin[p[0]]) ||
                    p[1] > 'f' || (p[1] != '0' && !hex2bin[p[1]]))
                {
                    FIXME("bad hex encoding at %s\n", debugstr_w(p));
                    continue;
                }

                b = (hex2bin[p[0]] << 4) | hex2bin[p[1]];
                col->pADsValues[i].pDNWithBinary->lpBinaryValue[n] = b;
            }
            if (*p != ':')
                FIXME("wrong DN with binary separator '%c'\n", *p);
            col->pADsValues[i].pDNWithBinary->pszDNString = p + 1;

            TRACE("%s => %lu,%s,%s\n", debugstr_w(values[i]),
                  col->pADsValues[i].pDNWithBinary->dwLength,
                  debugstr_an((char *)col->pADsValues[i].pDNWithBinary->lpBinaryValue, col->pADsValues[i].pDNWithBinary->dwLength),
                  debugstr_w(col->pADsValues[i].pDNWithBinary->pszDNString));
        }

        col->hReserved = values;
        break;
    }
    }

    col->dwADsType = type;
    col->dwNumValues = count;
    col->pszAttrName = wcsdup(name);

    return S_OK;
}

static HRESULT WINAPI search_GetColumn(IDirectorySearch *iface, ADS_SEARCH_HANDLE res,
                                       LPWSTR name, PADS_SEARCH_COLUMN col)
{
    LDAP_namespace *ldap = impl_from_IDirectorySearch(iface);
    struct ldap_search_context *ldap_ctx = res;
    HRESULT hr;
    ULONG count;

    TRACE("%p,%p,%s,%p\n", iface, res, debugstr_w(name), col);

    if (!res || !name || !ldap_ctx->entry) return E_ADS_BAD_PARAMETER;

    memset(col, 0, sizeof(*col));

    if (!wcsicmp(name, L"ADsPath"))
    {
        WCHAR *dn = ldap_get_dnW(ldap->ld, ldap_ctx->entry);

        col->pADsValues = malloc(sizeof(col->pADsValues[0]));
        if (!col->pADsValues)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        count = sizeof(L"LDAP://") + (wcslen(ldap->host) + 1 /* '/' */) * sizeof(WCHAR);
        if (dn) count += wcslen(dn) * sizeof(WCHAR);

        col->pADsValues[0].CaseIgnoreString = malloc(count);
        if (!col->pADsValues[0].CaseIgnoreString)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        wcscpy(col->pADsValues[0].CaseIgnoreString, L"LDAP://");
        wcscat(col->pADsValues[0].CaseIgnoreString, ldap->host);
        wcscat(col->pADsValues[0].CaseIgnoreString, L"/");
        if (dn) wcscat(col->pADsValues[0].CaseIgnoreString, dn);
        col->pADsValues[0].dwType = ADSTYPE_CASE_IGNORE_STRING;
        col->dwADsType = ADSTYPE_CASE_IGNORE_STRING;
        col->dwNumValues = 1;
        col->pszAttrName = wcsdup(name);
        col->hReserved = NULL;

        TRACE("=> %s\n", debugstr_w(col->pADsValues[0].CaseIgnoreString));
        hr = S_OK;
exit:
        ldap_memfreeW(dn);
        return hr;
    }

    return add_column_values(ldap, ldap_ctx, name, col);
}

static HRESULT WINAPI search_FreeColumn(IDirectorySearch *iface, PADS_SEARCH_COLUMN col)
{
    TRACE("%p,%p\n", iface, col);

    if (!col) return E_ADS_BAD_PARAMETER;

    if (!wcsicmp(col->pszAttrName, L"ADsPath"))
        free(col->pADsValues[0].CaseIgnoreString);
    free(col->pADsValues);
    free(col->pszAttrName);

    if (col->hReserved)
    {
        if (col->dwADsType == ADSTYPE_OCTET_STRING || col->dwADsType == ADSTYPE_NT_SECURITY_DESCRIPTOR)
            ldap_value_free_len(col->hReserved);
        else
            ldap_value_freeW(col->hReserved);
    }

    return S_OK;
}

static HRESULT WINAPI search_CloseSearchHandle(IDirectorySearch *iface, ADS_SEARCH_HANDLE res)
{
    LDAP_namespace *ldap = impl_from_IDirectorySearch(iface);
    struct ldap_search_context *ldap_ctx = res;

    TRACE("%p,%p\n", iface, res);

    if (!res) return E_ADS_BAD_PARAMETER;

    if (ldap_ctx->page)
        ldap_search_abandon_page(ldap->ld, ldap_ctx->page);
    if (ldap_ctx->res)
        ldap_msgfree(ldap_ctx->res);
    if (ldap_ctx->ber)
        ber_free(ldap_ctx->ber, 0);
    free(ldap_ctx);

    return S_OK;
}

static const IDirectorySearchVtbl IDirectorySearch_vtbl =
{
    search_QueryInterface,
    search_AddRef,
    search_Release,
    search_SetSearchPreference,
    search_ExecuteSearch,
    search_AbandonSearch,
    search_GetFirstRow,
    search_GetNextRow,
    search_GetPreviousRow,
    search_GetNextColumnName,
    search_GetColumn,
    search_FreeColumn,
    search_CloseSearchHandle
};

static inline LDAP_namespace *impl_from_IDirectoryObject(IDirectoryObject *iface)
{
    return CONTAINING_RECORD(iface, LDAP_namespace, IDirectoryObject_iface);
}

static HRESULT WINAPI dirobj_QueryInterface(IDirectoryObject *iface, REFIID riid, void **obj)
{
    LDAP_namespace *ldap = impl_from_IDirectoryObject(iface);

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    if (IsEqualGUID(riid, &IID_IDirectoryObject) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirectoryObject_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    return IADs_QueryInterface(&ldap->IADs_iface, riid, obj);
}

static ULONG WINAPI dirobj_AddRef(IDirectoryObject *iface)
{
    LDAP_namespace *ldap = impl_from_IDirectoryObject(iface);
    return IADs_AddRef(&ldap->IADs_iface);
}

static ULONG WINAPI dirobj_Release(IDirectoryObject *iface)
{
    LDAP_namespace *ldap = impl_from_IDirectoryObject(iface);
    return IADs_Release(&ldap->IADs_iface);
}

static HRESULT WINAPI dirobj_GetObjectInformation(IDirectoryObject *iface, PADS_OBJECT_INFO *info)
{
    FIXME("%p,%p: stub\n", iface, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI dirobj_GetObjectAttributes(IDirectoryObject *iface, LPWSTR *names,
                                                 DWORD count, PADS_ATTR_INFO *attrs, DWORD *count_returned)
{
    FIXME("%p,%p,%lu,%p,%p: stub\n", iface, names, count, attrs, count_returned);
    return E_NOTIMPL;
}

static HRESULT WINAPI dirobj_SetObjectAttributes(IDirectoryObject *iface, PADS_ATTR_INFO attrs,
                                                 DWORD count, DWORD *count_set)
{
    FIXME("%p,%p,%lu,%p: stub\n", iface, attrs, count, count_set);
    return E_NOTIMPL;
}

static HRESULT WINAPI dirobj_CreateDSObject(IDirectoryObject *iface, LPWSTR name,
                                            PADS_ATTR_INFO attrs, DWORD count, IDispatch **obj)
{
    FIXME("%p,%s,%p,%lu,%p: stub\n", iface, debugstr_w(name), attrs, count, obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI dirobj_DeleteDSObject(IDirectoryObject *iface, LPWSTR name)
{
    FIXME("%p,%s: stub\n", iface, debugstr_w(name));
    return E_NOTIMPL;
}

static const IDirectoryObjectVtbl IDirectoryObject_vtbl =
{
    dirobj_QueryInterface,
    dirobj_AddRef,
    dirobj_Release,
    dirobj_GetObjectInformation,
    dirobj_GetObjectAttributes,
    dirobj_SetObjectAttributes,
    dirobj_CreateDSObject,
    dirobj_DeleteDSObject
};

static HRESULT LDAPNamespace_create(REFIID riid, void **obj)
{
    LDAP_namespace *ldap;
    HRESULT hr;

    ldap = malloc(sizeof(*ldap));
    if (!ldap) return E_OUTOFMEMORY;

    ldap->IADs_iface.lpVtbl = &IADs_vtbl;
    ldap->IADsOpenDSObject_iface.lpVtbl = &IADsOpenDSObject_vtbl;
    ldap->IDirectorySearch_iface.lpVtbl = &IDirectorySearch_vtbl;
    ldap->IDirectoryObject_iface.lpVtbl = &IDirectoryObject_vtbl;
    ldap->ref = 1;
    ldap->ld = NULL;
    ldap->host = NULL;
    ldap->object = NULL;
    ldap->attrs_count = 0;
    ldap->attrs_count_allocated = 0;
    ldap->attrs = NULL;
    ldap->search.scope = ADS_SCOPE_SUBTREE;
    ldap->search.pagesize = 0;
    ldap->search.size_limit = 0;
    ldap->search.cache_results = TRUE;
    ldap->search.attribtypes_only = FALSE;
    ldap->search.tombstone = FALSE;
    ldap->at = NULL;
    ldap->at_single_count = 0;
    ldap->at_multiple_count = 0;

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

    TRACE("(%p) ref %lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI factory_Release(IClassFactory *iface)
{
    class_factory *factory = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&factory->ref);

    TRACE("(%p) ref %lu\n", iface, ref);

    if (!ref)
        free(factory);

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

    factory = malloc(sizeof(*factory));
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
