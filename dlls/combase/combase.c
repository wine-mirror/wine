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

#include "oleauto.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

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
