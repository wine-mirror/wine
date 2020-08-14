/*
 *    Copyright 2005 Juan Lang
 *    Copyright 2005-2006 Robert Shearman (for CodeWeavers)
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
#define NONAMELESSUNION

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define USE_COM_CONTEXT_DEF
#include "objbase.h"
#include "oleauto.h"
#include "winternl.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define CHARS_IN_GUID 39

struct comclassredirect_data
{
    ULONG size;
    ULONG flags;
    DWORD model;
    GUID  clsid;
    GUID  alias;
    GUID  clsid2;
    GUID  tlbid;
    ULONG name_len;
    ULONG name_offset;
    ULONG progid_len;
    ULONG progid_offset;
    ULONG clrdata_len;
    ULONG clrdata_offset;
    DWORD miscstatus;
    DWORD miscstatuscontent;
    DWORD miscstatusthumbnail;
    DWORD miscstatusicon;
    DWORD miscstatusdocprint;
};

struct progidredirect_data
{
    ULONG size;
    DWORD reserved;
    ULONG clsid_offset;
};

static NTSTATUS create_key(HKEY *retkey, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr)
{
    NTSTATUS status = NtCreateKey((HANDLE *)retkey, access, attr, 0, NULL, 0, NULL);

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        HANDLE subkey, root = attr->RootDirectory;
        WCHAR *buffer = attr->ObjectName->Buffer;
        DWORD attrs, pos = 0, i = 0, len = attr->ObjectName->Length / sizeof(WCHAR);
        UNICODE_STRING str;

        while (i < len && buffer[i] != '\\') i++;
        if (i == len) return status;

        attrs = attr->Attributes;
        attr->ObjectName = &str;

        while (i < len)
        {
            str.Buffer = buffer + pos;
            str.Length = (i - pos) * sizeof(WCHAR);
            status = NtCreateKey(&subkey, access, attr, 0, NULL, 0, NULL);
            if (attr->RootDirectory != root) NtClose(attr->RootDirectory);
            if (status) return status;
            attr->RootDirectory = subkey;
            while (i < len && buffer[i] == '\\') i++;
            pos = i;
            while (i < len && buffer[i] != '\\') i++;
        }
        str.Buffer = buffer + pos;
        str.Length = (i - pos) * sizeof(WCHAR);
        attr->Attributes = attrs;
        status = NtCreateKey((HANDLE *)retkey, access, attr, 0, NULL, 0, NULL);
        if (attr->RootDirectory != root) NtClose(attr->RootDirectory);
    }
    return status;
}

static HKEY classes_root_hkey;

static HKEY create_classes_root_hkey(DWORD access)
{
    HKEY hkey, ret = 0;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &name;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString(&name, L"\\Registry\\Machine\\Software\\Classes");

    if (create_key( &hkey, access, &attr )) return 0;
    TRACE( "%s -> %p\n", debugstr_w(attr.ObjectName->Buffer), hkey );

    if (!(access & KEY_WOW64_64KEY))
    {
        if (!(ret = InterlockedCompareExchangePointer( (void **)&classes_root_hkey, hkey, 0 )))
            ret = hkey;
        else
            NtClose( hkey );  /* somebody beat us to it */
    }
    else
        ret = hkey;
    return ret;
}

static HKEY get_classes_root_hkey(HKEY hkey, REGSAM access);

static LSTATUS create_classes_key(HKEY hkey, const WCHAR *name, REGSAM access, HKEY *retkey)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    if (!(hkey = get_classes_root_hkey(hkey, access)))
        return ERROR_INVALID_HANDLE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, name );

    return RtlNtStatusToDosError(create_key(retkey, access, &attr));
}

static HKEY get_classes_root_hkey(HKEY hkey, REGSAM access)
{
    HKEY ret = hkey;
    const BOOL is_win64 = sizeof(void*) > sizeof(int);
    const BOOL force_wow32 = is_win64 && (access & KEY_WOW64_32KEY);

    if (hkey == HKEY_CLASSES_ROOT &&
        ((access & KEY_WOW64_64KEY) || !(ret = classes_root_hkey)))
        ret = create_classes_root_hkey(MAXIMUM_ALLOWED | (access & KEY_WOW64_64KEY));
    if (force_wow32 && ret && ret == classes_root_hkey)
    {
        access &= ~KEY_WOW64_32KEY;
        if (create_classes_key(classes_root_hkey, L"Wow6432Node", access, &hkey))
            return 0;
        ret = hkey;
    }

    return ret;
}

static LSTATUS open_classes_key(HKEY hkey, const WCHAR *name, REGSAM access, HKEY *retkey)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    if (!(hkey = get_classes_root_hkey(hkey, access)))
        return ERROR_INVALID_HANDLE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, name );

    return RtlNtStatusToDosError(NtOpenKey((HANDLE *)retkey, access, &attr));
}

static HRESULT open_key_for_clsid(REFCLSID clsid, const WCHAR *keyname, REGSAM access, HKEY *subkey)
{
    static const WCHAR clsidW[] = L"CLSID\\";
    WCHAR path[CHARS_IN_GUID + ARRAY_SIZE(clsidW) - 1];
    LONG res;
    HKEY key;

    lstrcpyW(path, clsidW);
    StringFromGUID2(clsid, path + lstrlenW(clsidW), CHARS_IN_GUID);
    res = open_classes_key(HKEY_CLASSES_ROOT, path, keyname ? KEY_READ : access, &key);
    if (res == ERROR_FILE_NOT_FOUND)
        return REGDB_E_CLASSNOTREG;
    else if (res != ERROR_SUCCESS)
        return REGDB_E_READREGDB;

    if (!keyname)
    {
        *subkey = key;
        return S_OK;
    }

    res = open_classes_key(key, keyname, access, subkey);
    RegCloseKey(key);
    if (res == ERROR_FILE_NOT_FOUND)
        return REGDB_E_KEYMISSING;
    else if (res != ERROR_SUCCESS)
        return REGDB_E_READREGDB;

    return S_OK;
}

/***********************************************************************
 *           FreePropVariantArray    (combase.@)
 */
HRESULT WINAPI FreePropVariantArray(ULONG count, PROPVARIANT *rgvars)
{
    ULONG i;

    TRACE("%u, %p.\n", count, rgvars);

    if (!rgvars)
        return E_INVALIDARG;

    for (i = 0; i < count; ++i)
        PropVariantClear(&rgvars[i]);

    return S_OK;
}

static HRESULT propvar_validatetype(VARTYPE vt)
{
    switch (vt)
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_I1:
    case VT_I2:
    case VT_I4:
    case VT_I8:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_BSTR:
    case VT_ERROR:
    case VT_BOOL:
    case VT_DECIMAL:
    case VT_UI1:
    case VT_UI2:
    case VT_UI4:
    case VT_UI8:
    case VT_INT:
    case VT_UINT:
    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_FILETIME:
    case VT_BLOB:
    case VT_DISPATCH:
    case VT_UNKNOWN:
    case VT_STREAM:
    case VT_STORAGE:
    case VT_STREAMED_OBJECT:
    case VT_STORED_OBJECT:
    case VT_BLOB_OBJECT:
    case VT_CF:
    case VT_CLSID:
    case VT_I1|VT_VECTOR:
    case VT_I2|VT_VECTOR:
    case VT_I4|VT_VECTOR:
    case VT_I8|VT_VECTOR:
    case VT_R4|VT_VECTOR:
    case VT_R8|VT_VECTOR:
    case VT_CY|VT_VECTOR:
    case VT_DATE|VT_VECTOR:
    case VT_BSTR|VT_VECTOR:
    case VT_ERROR|VT_VECTOR:
    case VT_BOOL|VT_VECTOR:
    case VT_VARIANT|VT_VECTOR:
    case VT_UI1|VT_VECTOR:
    case VT_UI2|VT_VECTOR:
    case VT_UI4|VT_VECTOR:
    case VT_UI8|VT_VECTOR:
    case VT_LPSTR|VT_VECTOR:
    case VT_LPWSTR|VT_VECTOR:
    case VT_FILETIME|VT_VECTOR:
    case VT_CF|VT_VECTOR:
    case VT_CLSID|VT_VECTOR:
    case VT_ARRAY|VT_I1:
    case VT_ARRAY|VT_UI1:
    case VT_ARRAY|VT_I2:
    case VT_ARRAY|VT_UI2:
    case VT_ARRAY|VT_I4:
    case VT_ARRAY|VT_UI4:
    case VT_ARRAY|VT_INT:
    case VT_ARRAY|VT_UINT:
    case VT_ARRAY|VT_R4:
    case VT_ARRAY|VT_R8:
    case VT_ARRAY|VT_CY:
    case VT_ARRAY|VT_DATE:
    case VT_ARRAY|VT_BSTR:
    case VT_ARRAY|VT_BOOL:
    case VT_ARRAY|VT_DECIMAL:
    case VT_ARRAY|VT_DISPATCH:
    case VT_ARRAY|VT_UNKNOWN:
    case VT_ARRAY|VT_ERROR:
    case VT_ARRAY|VT_VARIANT:
        return S_OK;
    }
    WARN("Bad type %d\n", vt);
    return STG_E_INVALIDPARAMETER;
}

static void propvar_free_cf_array(ULONG count, CLIPDATA *data)
{
    ULONG i;
    for (i = 0; i < count; ++i)
        CoTaskMemFree(data[i].pClipData);
}

/***********************************************************************
 *           PropVariantClear    (combase.@)
 */
HRESULT WINAPI PropVariantClear(PROPVARIANT *pvar)
{
    HRESULT hr;

    TRACE("%p.\n", pvar);

    if (!pvar)
        return S_OK;

    hr = propvar_validatetype(pvar->vt);
    if (FAILED(hr))
    {
        memset(pvar, 0, sizeof(*pvar));
        return hr;
    }

    switch (pvar->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_I1:
    case VT_I2:
    case VT_I4:
    case VT_I8:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_ERROR:
    case VT_BOOL:
    case VT_DECIMAL:
    case VT_UI1:
    case VT_UI2:
    case VT_UI4:
    case VT_UI8:
    case VT_INT:
    case VT_UINT:
    case VT_FILETIME:
        break;
    case VT_DISPATCH:
    case VT_UNKNOWN:
    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:
        if (pvar->u.pStream)
            IStream_Release(pvar->u.pStream);
        break;
    case VT_CLSID:
    case VT_LPSTR:
    case VT_LPWSTR:
        /* pick an arbitrary typed pointer - we don't care about the type
         * as we are just freeing it */
        CoTaskMemFree(pvar->u.puuid);
        break;
    case VT_BLOB:
    case VT_BLOB_OBJECT:
        CoTaskMemFree(pvar->u.blob.pBlobData);
        break;
    case VT_BSTR:
        SysFreeString(pvar->u.bstrVal);
        break;
    case VT_CF:
        if (pvar->u.pclipdata)
        {
            propvar_free_cf_array(1, pvar->u.pclipdata);
            CoTaskMemFree(pvar->u.pclipdata);
        }
        break;
    default:
        if (pvar->vt & VT_VECTOR)
        {
            ULONG i;

            switch (pvar->vt & ~VT_VECTOR)
            {
            case VT_VARIANT:
                FreePropVariantArray(pvar->u.capropvar.cElems, pvar->u.capropvar.pElems);
                break;
            case VT_CF:
                propvar_free_cf_array(pvar->u.caclipdata.cElems, pvar->u.caclipdata.pElems);
                break;
            case VT_BSTR:
                for (i = 0; i < pvar->u.cabstr.cElems; i++)
                    SysFreeString(pvar->u.cabstr.pElems[i]);
                break;
            case VT_LPSTR:
                for (i = 0; i < pvar->u.calpstr.cElems; i++)
                    CoTaskMemFree(pvar->u.calpstr.pElems[i]);
                break;
            case VT_LPWSTR:
                for (i = 0; i < pvar->u.calpwstr.cElems; i++)
                    CoTaskMemFree(pvar->u.calpwstr.pElems[i]);
                break;
            }
            if (pvar->vt & ~VT_VECTOR)
            {
                /* pick an arbitrary VT_VECTOR structure - they all have the same
                 * memory layout */
                CoTaskMemFree(pvar->u.capropvar.pElems);
            }
        }
        else if (pvar->vt & VT_ARRAY)
            hr = SafeArrayDestroy(pvar->u.parray);
        else
        {
            WARN("Invalid/unsupported type %d\n", pvar->vt);
            hr = STG_E_INVALIDPARAMETER;
        }
    }

    memset(pvar, 0, sizeof(*pvar));
    return hr;
}

/***********************************************************************
 *           PropVariantCopy        (combase.@)
 */
HRESULT WINAPI PropVariantCopy(PROPVARIANT *pvarDest, const PROPVARIANT *pvarSrc)
{
    ULONG len;
    HRESULT hr;

    TRACE("%p, %p vt %04x.\n", pvarDest, pvarSrc, pvarSrc->vt);

    hr = propvar_validatetype(pvarSrc->vt);
    if (FAILED(hr))
        return DISP_E_BADVARTYPE;

    /* this will deal with most cases */
    *pvarDest = *pvarSrc;

    switch (pvarSrc->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_I1:
    case VT_UI1:
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
    case VT_DECIMAL:
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
    case VT_I8:
    case VT_UI8:
    case VT_INT:
    case VT_UINT:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
        break;
    case VT_DISPATCH:
    case VT_UNKNOWN:
    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:
        if (pvarDest->u.pStream)
            IStream_AddRef(pvarDest->u.pStream);
        break;
    case VT_CLSID:
        pvarDest->u.puuid = CoTaskMemAlloc(sizeof(CLSID));
        *pvarDest->u.puuid = *pvarSrc->u.puuid;
        break;
    case VT_LPSTR:
        if (pvarSrc->u.pszVal)
        {
            len = strlen(pvarSrc->u.pszVal);
            pvarDest->u.pszVal = CoTaskMemAlloc((len+1)*sizeof(CHAR));
            CopyMemory(pvarDest->u.pszVal, pvarSrc->u.pszVal, (len+1)*sizeof(CHAR));
        }
        break;
    case VT_LPWSTR:
        if (pvarSrc->u.pwszVal)
        {
            len = lstrlenW(pvarSrc->u.pwszVal);
            pvarDest->u.pwszVal = CoTaskMemAlloc((len+1)*sizeof(WCHAR));
            CopyMemory(pvarDest->u.pwszVal, pvarSrc->u.pwszVal, (len+1)*sizeof(WCHAR));
        }
        break;
    case VT_BLOB:
    case VT_BLOB_OBJECT:
        if (pvarSrc->u.blob.pBlobData)
        {
            len = pvarSrc->u.blob.cbSize;
            pvarDest->u.blob.pBlobData = CoTaskMemAlloc(len);
            CopyMemory(pvarDest->u.blob.pBlobData, pvarSrc->u.blob.pBlobData, len);
        }
        break;
    case VT_BSTR:
        pvarDest->u.bstrVal = SysAllocString(pvarSrc->u.bstrVal);
        break;
    case VT_CF:
        if (pvarSrc->u.pclipdata)
        {
            len = pvarSrc->u.pclipdata->cbSize - sizeof(pvarSrc->u.pclipdata->ulClipFmt);
            pvarDest->u.pclipdata = CoTaskMemAlloc(sizeof (CLIPDATA));
            pvarDest->u.pclipdata->cbSize = pvarSrc->u.pclipdata->cbSize;
            pvarDest->u.pclipdata->ulClipFmt = pvarSrc->u.pclipdata->ulClipFmt;
            pvarDest->u.pclipdata->pClipData = CoTaskMemAlloc(len);
            CopyMemory(pvarDest->u.pclipdata->pClipData, pvarSrc->u.pclipdata->pClipData, len);
        }
        break;
    default:
        if (pvarSrc->vt & VT_VECTOR)
        {
            int elemSize;
            ULONG i;

            switch (pvarSrc->vt & ~VT_VECTOR)
            {
            case VT_I1:       elemSize = sizeof(pvarSrc->u.cVal); break;
            case VT_UI1:      elemSize = sizeof(pvarSrc->u.bVal); break;
            case VT_I2:       elemSize = sizeof(pvarSrc->u.iVal); break;
            case VT_UI2:      elemSize = sizeof(pvarSrc->u.uiVal); break;
            case VT_BOOL:     elemSize = sizeof(pvarSrc->u.boolVal); break;
            case VT_I4:       elemSize = sizeof(pvarSrc->u.lVal); break;
            case VT_UI4:      elemSize = sizeof(pvarSrc->u.ulVal); break;
            case VT_R4:       elemSize = sizeof(pvarSrc->u.fltVal); break;
            case VT_R8:       elemSize = sizeof(pvarSrc->u.dblVal); break;
            case VT_ERROR:    elemSize = sizeof(pvarSrc->u.scode); break;
            case VT_I8:       elemSize = sizeof(pvarSrc->u.hVal); break;
            case VT_UI8:      elemSize = sizeof(pvarSrc->u.uhVal); break;
            case VT_CY:       elemSize = sizeof(pvarSrc->u.cyVal); break;
            case VT_DATE:     elemSize = sizeof(pvarSrc->u.date); break;
            case VT_FILETIME: elemSize = sizeof(pvarSrc->u.filetime); break;
            case VT_CLSID:    elemSize = sizeof(*pvarSrc->u.puuid); break;
            case VT_CF:       elemSize = sizeof(*pvarSrc->u.pclipdata); break;
            case VT_BSTR:     elemSize = sizeof(pvarSrc->u.bstrVal); break;
            case VT_LPSTR:    elemSize = sizeof(pvarSrc->u.pszVal); break;
            case VT_LPWSTR:   elemSize = sizeof(pvarSrc->u.pwszVal); break;
            case VT_VARIANT:  elemSize = sizeof(*pvarSrc->u.pvarVal); break;

            default:
                FIXME("Invalid element type: %ul\n", pvarSrc->vt & ~VT_VECTOR);
                return E_INVALIDARG;
            }
            len = pvarSrc->u.capropvar.cElems;
            pvarDest->u.capropvar.pElems = len ? CoTaskMemAlloc(len * elemSize) : NULL;
            if (pvarSrc->vt == (VT_VECTOR | VT_VARIANT))
            {
                for (i = 0; i < len; i++)
                    PropVariantCopy(&pvarDest->u.capropvar.pElems[i], &pvarSrc->u.capropvar.pElems[i]);
            }
            else if (pvarSrc->vt == (VT_VECTOR | VT_CF))
            {
                FIXME("Copy clipformats\n");
            }
            else if (pvarSrc->vt == (VT_VECTOR | VT_BSTR))
            {
                for (i = 0; i < len; i++)
                    pvarDest->u.cabstr.pElems[i] = SysAllocString(pvarSrc->u.cabstr.pElems[i]);
            }
            else if (pvarSrc->vt == (VT_VECTOR | VT_LPSTR))
            {
                size_t strLen;
                for (i = 0; i < len; i++)
                {
                    strLen = lstrlenA(pvarSrc->u.calpstr.pElems[i]) + 1;
                    pvarDest->u.calpstr.pElems[i] = CoTaskMemAlloc(strLen);
                    memcpy(pvarDest->u.calpstr.pElems[i],
                     pvarSrc->u.calpstr.pElems[i], strLen);
                }
            }
            else if (pvarSrc->vt == (VT_VECTOR | VT_LPWSTR))
            {
                size_t strLen;
                for (i = 0; i < len; i++)
                {
                    strLen = (lstrlenW(pvarSrc->u.calpwstr.pElems[i]) + 1) *
                     sizeof(WCHAR);
                    pvarDest->u.calpstr.pElems[i] = CoTaskMemAlloc(strLen);
                    memcpy(pvarDest->u.calpstr.pElems[i],
                     pvarSrc->u.calpstr.pElems[i], strLen);
                }
            }
            else
                CopyMemory(pvarDest->u.capropvar.pElems, pvarSrc->u.capropvar.pElems, len * elemSize);
        }
        else if (pvarSrc->vt & VT_ARRAY)
        {
            pvarDest->u.uhVal.QuadPart = 0;
            return SafeArrayCopy(pvarSrc->u.parray, &pvarDest->u.parray);
        }
        else
            WARN("Invalid/unsupported type %d\n", pvarSrc->vt);
    }

    return S_OK;
}

/***********************************************************************
 *           CoFileTimeNow        (combase.@)
 */
HRESULT WINAPI CoFileTimeNow(FILETIME *filetime)
{
    GetSystemTimeAsFileTime(filetime);
    return S_OK;
}

/******************************************************************************
 *            CoCreateGuid        (combase.@)
 */
HRESULT WINAPI CoCreateGuid(GUID *guid)
{
    RPC_STATUS status;

    if (!guid) return E_INVALIDARG;

    status = UuidCreate(guid);
    if (status == RPC_S_OK || status == RPC_S_UUID_LOCAL_ONLY) return S_OK;
    return HRESULT_FROM_WIN32(status);
}

/******************************************************************************
 *            CoQueryProxyBlanket        (combase.@)
 */
HRESULT WINAPI CoQueryProxyBlanket(IUnknown *proxy, DWORD *authn_service,
        DWORD *authz_service, OLECHAR **servername, DWORD *authn_level,
        DWORD *imp_level, void **auth_info, DWORD *capabilities)
{
    IClientSecurity *client_security;
    HRESULT hr;

    TRACE("%p, %p, %p, %p, %p, %p, %p, %p.\n", proxy, authn_service, authz_service, servername, authn_level, imp_level,
            auth_info, capabilities);

    hr = IUnknown_QueryInterface(proxy, &IID_IClientSecurity, (void **)&client_security);
    if (SUCCEEDED(hr))
    {
        hr = IClientSecurity_QueryBlanket(client_security, proxy, authn_service, authz_service, servername,
                authn_level, imp_level, auth_info, capabilities);
        IClientSecurity_Release(client_security);
    }

    if (FAILED(hr)) ERR("-- failed with %#x.\n", hr);
    return hr;
}

/******************************************************************************
 *            CoSetProxyBlanket        (combase.@)
 */
HRESULT WINAPI CoSetProxyBlanket(IUnknown *proxy, DWORD authn_service, DWORD authz_service,
        OLECHAR *servername, DWORD authn_level, DWORD imp_level, void *auth_info, DWORD capabilities)
{
    IClientSecurity *client_security;
    HRESULT hr;

    TRACE("%p, %u, %u, %p, %u, %u, %p, %#x.\n", proxy, authn_service, authz_service, servername,
            authn_level, imp_level, auth_info, capabilities);

    hr = IUnknown_QueryInterface(proxy, &IID_IClientSecurity, (void **)&client_security);
    if (SUCCEEDED(hr))
    {
        hr = IClientSecurity_SetBlanket(client_security, proxy, authn_service, authz_service, servername, authn_level,
                imp_level, auth_info, capabilities);
        IClientSecurity_Release(client_security);
    }

    if (FAILED(hr)) ERR("-- failed with %#x.\n", hr);
    return hr;
}

/***********************************************************************
 *           CoCopyProxy        (combase.@)
 */
HRESULT WINAPI CoCopyProxy(IUnknown *proxy, IUnknown **proxy_copy)
{
    IClientSecurity *client_security;
    HRESULT hr;

    TRACE("%p, %p.\n", proxy, proxy_copy);

    hr = IUnknown_QueryInterface(proxy, &IID_IClientSecurity, (void **)&client_security);
    if (SUCCEEDED(hr))
    {
        hr = IClientSecurity_CopyProxy(client_security, proxy, proxy_copy);
        IClientSecurity_Release(client_security);
    }

    if (FAILED(hr)) ERR("-- failed with %#x.\n", hr);
    return hr;
}

/***********************************************************************
 *           CoQueryClientBlanket        (combase.@)
 */
HRESULT WINAPI CoQueryClientBlanket(DWORD *authn_service, DWORD *authz_service, OLECHAR **servername,
        DWORD *authn_level, DWORD *imp_level, RPC_AUTHZ_HANDLE *privs, DWORD *capabilities)
{
    IServerSecurity *server_security;
    HRESULT hr;

    TRACE("%p, %p, %p, %p, %p, %p, %p.\n", authn_service, authz_service, servername, authn_level, imp_level,
            privs, capabilities);

    hr = CoGetCallContext(&IID_IServerSecurity, (void **)&server_security);
    if (SUCCEEDED(hr))
    {
        hr = IServerSecurity_QueryBlanket(server_security, authn_service, authz_service, servername, authn_level,
                imp_level, privs, capabilities);
        IServerSecurity_Release(server_security);
    }

    return hr;
}

/***********************************************************************
 *           CoImpersonateClient        (combase.@)
 */
HRESULT WINAPI CoImpersonateClient(void)
{
    IServerSecurity *server_security;
    HRESULT hr;

    TRACE("\n");

    hr = CoGetCallContext(&IID_IServerSecurity, (void **)&server_security);
    if (SUCCEEDED(hr))
    {
        hr = IServerSecurity_ImpersonateClient(server_security);
        IServerSecurity_Release(server_security);
    }

    return hr;
}

/***********************************************************************
 *           CoRevertToSelf        (combase.@)
 */
HRESULT WINAPI CoRevertToSelf(void)
{
    IServerSecurity *server_security;
    HRESULT hr;

    TRACE("\n");

    hr = CoGetCallContext(&IID_IServerSecurity, (void **)&server_security);
    if (SUCCEEDED(hr))
    {
        hr = IServerSecurity_RevertToSelf(server_security);
        IServerSecurity_Release(server_security);
    }

    return hr;
}

/***********************************************************************
 *           CoInitializeSecurity    (combase.@)
 */
HRESULT WINAPI CoInitializeSecurity(PSECURITY_DESCRIPTOR sd, LONG cAuthSvc,
        SOLE_AUTHENTICATION_SERVICE *asAuthSvc, void *reserved1, DWORD authn_level,
        DWORD imp_level, void *reserved2, DWORD capabilities, void *reserved3)
{
    FIXME("%p, %d, %p, %p, %d, %d, %p, %d, %p stub\n", sd, cAuthSvc, asAuthSvc, reserved1, authn_level,
            imp_level, reserved2, capabilities, reserved3);

    return S_OK;
}

/***********************************************************************
 *           CoGetObjectContext    (combase.@)
 */
HRESULT WINAPI CoGetObjectContext(REFIID riid, void **ppv)
{
    IObjContext *context;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_guid(riid), ppv);

    *ppv = NULL;
    hr = CoGetContextToken((ULONG_PTR *)&context);
    if (FAILED(hr))
        return hr;

    return IObjContext_QueryInterface(context, riid, ppv);
}

/***********************************************************************
 *           CoGetDefaultContext    (combase.@)
 */
HRESULT WINAPI CoGetDefaultContext(APTTYPE type, REFIID riid, void **obj)
{
    FIXME("%d, %s, %p stub\n", type, debugstr_guid(riid), obj);

    return E_NOINTERFACE;
}

/***********************************************************************
 *          CoGetCallState        (combase.@)
 */
HRESULT WINAPI CoGetCallState(int arg1, ULONG *arg2)
{
    FIXME("%d, %p.\n", arg1, arg2);

    return E_NOTIMPL;
}

/***********************************************************************
 *          CoGetActivationState    (combase.@)
 */
HRESULT WINAPI CoGetActivationState(GUID guid, DWORD arg2, DWORD *arg3)
{
    FIXME("%s, %x, %p.\n", debugstr_guid(&guid), arg2, arg3);

    return E_NOTIMPL;
}

/******************************************************************************
 *          CoGetTreatAsClass       (combase.@)
 */
HRESULT WINAPI CoGetTreatAsClass(REFCLSID clsidOld, CLSID *clsidNew)
{
    WCHAR buffW[CHARS_IN_GUID];
    LONG len = sizeof(buffW);
    HRESULT hr = S_OK;
    HKEY hkey = NULL;

    TRACE("%s, %p.\n", debugstr_guid(clsidOld), clsidNew);

    if (!clsidOld || !clsidNew)
        return E_INVALIDARG;

    *clsidNew = *clsidOld;

    hr = open_key_for_clsid(clsidOld, L"TreatAs", KEY_READ, &hkey);
    if (FAILED(hr))
    {
        hr = S_FALSE;
        goto done;
    }

    if (RegQueryValueW(hkey, NULL, buffW, &len))
    {
        hr = S_FALSE;
        goto done;
    }

    hr = CLSIDFromString(buffW, clsidNew);
    if (FAILED(hr))
        ERR("Failed to get CLSID from string %s, hr %#x.\n", debugstr_w(buffW), hr);
done:
    if (hkey) RegCloseKey(hkey);
    return hr;
}

/******************************************************************************
 *               ProgIDFromCLSID        (combase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH ProgIDFromCLSID(REFCLSID clsid, LPOLESTR *progid)
{
    ACTCTX_SECTION_KEYED_DATA data;
    LONG progidlen = 0;
    HKEY hkey;
    HRESULT hr;

    if (!progid)
        return E_INVALIDARG;

    *progid = NULL;

    data.cbSize = sizeof(data);
    if (FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION,
            clsid, &data))
    {
        struct comclassredirect_data *comclass = (struct comclassredirect_data *)data.lpData;
        if (comclass->progid_len)
        {
            WCHAR *ptrW;

            *progid = CoTaskMemAlloc(comclass->progid_len + sizeof(WCHAR));
            if (!*progid) return E_OUTOFMEMORY;

            ptrW = (WCHAR *)((BYTE *)comclass + comclass->progid_offset);
            memcpy(*progid, ptrW, comclass->progid_len + sizeof(WCHAR));
            return S_OK;
        }
        else
            return REGDB_E_CLASSNOTREG;
    }

    hr = open_key_for_clsid(clsid, L"ProgID", KEY_READ, &hkey);
    if (FAILED(hr))
        return hr;

    if (RegQueryValueW(hkey, NULL, NULL, &progidlen))
        hr = REGDB_E_CLASSNOTREG;

    if (hr == S_OK)
    {
        *progid = CoTaskMemAlloc(progidlen * sizeof(WCHAR));
        if (*progid)
        {
            if (RegQueryValueW(hkey, NULL, *progid, &progidlen))
            {
                hr = REGDB_E_CLASSNOTREG;
                CoTaskMemFree(*progid);
                *progid = NULL;
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }

    RegCloseKey(hkey);
    return hr;
}

static inline BOOL is_valid_hex(WCHAR c)
{
    if (!(((c >= '0') && (c <= '9'))  ||
          ((c >= 'a') && (c <= 'f'))  ||
          ((c >= 'A') && (c <= 'F'))))
        return FALSE;
    return TRUE;
}

static const BYTE guid_conv_table[256] =
{
    0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x00 */
    0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 */
    0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x20 */
    0,   1,   2,   3,   4,   5,   6, 7, 8, 9, 0, 0, 0, 0, 0, 0, /* 0x30 */
    0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x40 */
    0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x50 */
    0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf                             /* 0x60 */
};

static BOOL guid_from_string(LPCWSTR s, GUID *id)
{
    int i;

    if (!s || s[0] != '{')
    {
        memset(id, 0, sizeof(*id));
        if (!s) return TRUE;
        return FALSE;
    }

    TRACE("%s -> %p\n", debugstr_w(s), id);

    /* In form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} */

    id->Data1 = 0;
    for (i = 1; i < 9; ++i)
    {
        if (!is_valid_hex(s[i])) return FALSE;
        id->Data1 = (id->Data1 << 4) | guid_conv_table[s[i]];
    }
    if (s[9] != '-') return FALSE;

    id->Data2 = 0;
    for (i = 10; i < 14; ++i)
    {
        if (!is_valid_hex(s[i])) return FALSE;
        id->Data2 = (id->Data2 << 4) | guid_conv_table[s[i]];
    }
    if (s[14] != '-') return FALSE;

    id->Data3 = 0;
    for (i = 15; i < 19; ++i)
    {
        if (!is_valid_hex(s[i])) return FALSE;
        id->Data3 = (id->Data3 << 4) | guid_conv_table[s[i]];
    }
    if (s[19] != '-') return FALSE;

    for (i = 20; i < 37; i += 2)
    {
        if (i == 24)
        {
            if (s[i] != '-') return FALSE;
            i++;
        }
        if (!is_valid_hex(s[i]) || !is_valid_hex(s[i + 1])) return FALSE;
        id->Data4[(i - 20) / 2] = guid_conv_table[s[i]] << 4 | guid_conv_table[s[i + 1]];
    }

    if (s[37] == '}' && s[38] == '\0')
        return TRUE;

    return FALSE;
}

static HRESULT clsid_from_string_reg(LPCOLESTR progid, CLSID *clsid)
{
    WCHAR buf2[CHARS_IN_GUID];
    LONG buf2len = sizeof(buf2);
    HKEY xhkey;
    WCHAR *buf;

    memset(clsid, 0, sizeof(*clsid));
    buf = heap_alloc((lstrlenW(progid) + 8) * sizeof(WCHAR));
    if (!buf) return E_OUTOFMEMORY;

    lstrcpyW(buf, progid);
    lstrcatW(buf, L"\\CLSID");
    if (open_classes_key(HKEY_CLASSES_ROOT, buf, MAXIMUM_ALLOWED, &xhkey))
    {
        heap_free(buf);
        WARN("couldn't open key for ProgID %s\n", debugstr_w(progid));
        return CO_E_CLASSSTRING;
    }
    heap_free(buf);

    if (RegQueryValueW(xhkey, NULL, buf2, &buf2len))
    {
        RegCloseKey(xhkey);
        WARN("couldn't query clsid value for ProgID %s\n", debugstr_w(progid));
        return CO_E_CLASSSTRING;
    }
    RegCloseKey(xhkey);
    return guid_from_string(buf2, clsid) ? S_OK : CO_E_CLASSSTRING;
}

/******************************************************************************
 *                CLSIDFromProgID        (combase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH CLSIDFromProgID(LPCOLESTR progid, CLSID *clsid)
{
    ACTCTX_SECTION_KEYED_DATA data;

    if (!progid || !clsid)
        return E_INVALIDARG;

    data.cbSize = sizeof(data);
    if (FindActCtxSectionStringW(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION,
            progid, &data))
    {
        struct progidredirect_data *progiddata = (struct progidredirect_data *)data.lpData;
        CLSID *alias = (CLSID *)((BYTE *)data.lpSectionBase + progiddata->clsid_offset);
        *clsid = *alias;
        return S_OK;
    }

    return clsid_from_string_reg(progid, clsid);
}

/******************************************************************************
 *              CLSIDFromProgIDEx        (combase.@)
 */
HRESULT WINAPI CLSIDFromProgIDEx(LPCOLESTR progid, CLSID *clsid)
{
    FIXME("%s, %p: semi-stub\n", debugstr_w(progid), clsid);

    return CLSIDFromProgID(progid, clsid);
}

/******************************************************************************
 *                CLSIDFromString        (combase.@)
 */
HRESULT WINAPI CLSIDFromString(LPCOLESTR str, LPCLSID clsid)
{
    CLSID tmp_id;
    HRESULT hr;

    if (!clsid)
        return E_INVALIDARG;

    if (guid_from_string(str, clsid))
        return S_OK;

    /* It appears a ProgID is also valid */
    hr = clsid_from_string_reg(str, &tmp_id);
    if (SUCCEEDED(hr))
        *clsid = tmp_id;

    return hr;
}

/******************************************************************************
 *                IIDFromString        (combase.@)
 */
HRESULT WINAPI IIDFromString(LPCOLESTR str, IID *iid)
{
    TRACE("%s, %p\n", debugstr_w(str), iid);

    if (!str)
    {
        memset(iid, 0, sizeof(*iid));
        return S_OK;
    }

    /* length mismatch is a special case */
    if (lstrlenW(str) + 1 != CHARS_IN_GUID)
        return E_INVALIDARG;

    if (str[0] != '{')
        return CO_E_IIDSTRING;

    return guid_from_string(str, iid) ? S_OK : CO_E_IIDSTRING;
}

/******************************************************************************
 *            StringFromCLSID        (combase.@)
 */
HRESULT WINAPI StringFromCLSID(REFCLSID clsid, LPOLESTR *str)
{
    if (!(*str = CoTaskMemAlloc(CHARS_IN_GUID * sizeof(WCHAR)))) return E_OUTOFMEMORY;
    StringFromGUID2(clsid, *str, CHARS_IN_GUID);
    return S_OK;
}

/******************************************************************************
 *            StringFromGUID2        (combase.@)
 */
INT WINAPI StringFromGUID2(REFGUID guid, LPOLESTR str, INT cmax)
{
    if (!guid || cmax < CHARS_IN_GUID) return 0;
    swprintf(str, CHARS_IN_GUID, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", guid->Data1,
            guid->Data2, guid->Data3, guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
            guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return CHARS_IN_GUID;
}

static void init_multi_qi(DWORD count, MULTI_QI *mqi, HRESULT hr)
{
    ULONG i;

    for (i = 0; i < count; i++)
    {
        mqi[i].pItf = NULL;
        mqi[i].hr = hr;
    }
}

static HRESULT return_multi_qi(IUnknown *unk, DWORD count, MULTI_QI *mqi, BOOL include_unk)
{
    ULONG index = 0, fetched = 0;

    if (include_unk)
    {
        mqi[0].hr = S_OK;
        mqi[0].pItf = unk;
        index = fetched = 1;
    }

    for (; index < count; index++)
    {
        mqi[index].hr = IUnknown_QueryInterface(unk, mqi[index].pIID, (void **)&mqi[index].pItf);
        if (mqi[index].hr == S_OK)
            fetched++;
    }

    if (!include_unk)
        IUnknown_Release(unk);

    if (fetched == 0)
        return E_NOINTERFACE;

    return fetched == count ? S_OK : CO_S_NOTALLINTERFACES;
}

/***********************************************************************
 *          CoGetInstanceFromFile    (combase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH CoGetInstanceFromFile(COSERVERINFO *server_info, CLSID *rclsid,
        IUnknown *outer, DWORD cls_context, DWORD grfmode, OLECHAR *filename, DWORD count,
        MULTI_QI *results)
{
    IPersistFile *pf = NULL;
    IUnknown *obj = NULL;
    CLSID clsid;
    HRESULT hr;

    if (!count || !results)
        return E_INVALIDARG;

    if (server_info)
        FIXME("() non-NULL server_info not supported\n");

    init_multi_qi(count, results, E_NOINTERFACE);

    if (!rclsid)
    {
        hr = GetClassFile(filename, &clsid);
        if (FAILED(hr))
        {
            ERR("Failed to get CLSID from a file.\n");
            return hr;
        }

        rclsid = &clsid;
    }

    hr = CoCreateInstance(rclsid, outer, cls_context, &IID_IUnknown, (void **)&obj);
    if (hr != S_OK)
    {
        init_multi_qi(count, results, hr);
        return hr;
    }

    /* Init from file */
    hr = IUnknown_QueryInterface(obj, &IID_IPersistFile, (void **)&pf);
    if (FAILED(hr))
    {
        init_multi_qi(count, results, hr);
        IUnknown_Release(obj);
        return hr;
    }

    hr = IPersistFile_Load(pf, filename, grfmode);
    IPersistFile_Release(pf);
    if (SUCCEEDED(hr))
        return return_multi_qi(obj, count, results, FALSE);
    else
    {
        init_multi_qi(count, results, hr);
        IUnknown_Release(obj);
        return hr;
    }
}

/***********************************************************************
 *           CoGetInstanceFromIStorage        (combase.@)
 */
HRESULT WINAPI CoGetInstanceFromIStorage(COSERVERINFO *server_info, CLSID *rclsid,
        IUnknown *outer, DWORD cls_context, IStorage *storage, DWORD count, MULTI_QI *results)
{
    IPersistStorage *ps = NULL;
    IUnknown *obj = NULL;
    STATSTG stat;
    HRESULT hr;

    if (!count || !results || !storage)
        return E_INVALIDARG;

    if (server_info)
        FIXME("() non-NULL server_info not supported\n");

    init_multi_qi(count, results, E_NOINTERFACE);

    if (!rclsid)
    {
        memset(&stat.clsid, 0, sizeof(stat.clsid));
        hr = IStorage_Stat(storage, &stat, STATFLAG_NONAME);
        if (FAILED(hr))
        {
            ERR("Failed to get CLSID from a storage.\n");
            return hr;
        }

        rclsid = &stat.clsid;
    }

    hr = CoCreateInstance(rclsid, outer, cls_context, &IID_IUnknown, (void **)&obj);
    if (hr != S_OK)
        return hr;

    /* Init from IStorage */
    hr = IUnknown_QueryInterface(obj, &IID_IPersistStorage, (void **)&ps);
    if (FAILED(hr))
        ERR("failed to get IPersistStorage\n");

    if (ps)
    {
        IPersistStorage_Load(ps, storage);
        IPersistStorage_Release(ps);
    }

    return return_multi_qi(obj, count, results, FALSE);
}

/***********************************************************************
 *           CoCreateInstance        (combase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH CoCreateInstance(REFCLSID rclsid, IUnknown *outer, DWORD cls_context,
        REFIID riid, void **obj)
{
    MULTI_QI multi_qi = { .pIID = riid };
    HRESULT hr;

    TRACE("%s, %p, %#x, %s, %p.\n", debugstr_guid(rclsid), outer, cls_context, debugstr_guid(riid), obj);

    if (!obj)
        return E_POINTER;

    hr = CoCreateInstanceEx(rclsid, outer, cls_context, NULL, 1, &multi_qi);
    *obj = multi_qi.pItf;
    return hr;
}

/***********************************************************************
 *           CoCreateInstanceEx    (combase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH CoCreateInstanceEx(REFCLSID rclsid, IUnknown *outer, DWORD cls_context,
        COSERVERINFO *server_info, ULONG count, MULTI_QI *results)
{
    IClassFactory *factory;
    IUnknown *unk = NULL;
    CLSID clsid;
    HRESULT hr;

    TRACE("%s, %p, %#x, %p, %u, %p\n", debugstr_guid(rclsid), outer, cls_context, server_info, count, results);

    if (!count || !results)
        return E_INVALIDARG;

    if (server_info)
        FIXME("Server info is not supported.\n");

    init_multi_qi(count, results, E_NOINTERFACE);

    hr = CoGetTreatAsClass(rclsid, &clsid);
    if (FAILED(hr))
        clsid = *rclsid;

    hr = CoGetClassObject(&clsid, cls_context, NULL, &IID_IClassFactory, (void **)&factory);
    if (FAILED(hr))
        return hr;

    hr = IClassFactory_CreateInstance(factory, outer, results[0].pIID, (void **)&unk);
    IClassFactory_Release(factory);
    if (FAILED(hr))
    {
        if (hr == CLASS_E_NOAGGREGATION && outer)
            FIXME("Class %s does not support aggregation\n", debugstr_guid(&clsid));
        else
            FIXME("no instance created for interface %s of class %s, hr %#x.\n",
                    debugstr_guid(results[0].pIID), debugstr_guid(&clsid), hr);
        return hr;
    }

    return return_multi_qi(unk, count, results, TRUE);
}

/***********************************************************************
 *           CoFreeUnusedLibraries    (combase.@)
 */
void WINAPI DECLSPEC_HOTPATCH CoFreeUnusedLibraries(void)
{
    CoFreeUnusedLibrariesEx(INFINITE, 0);
}
