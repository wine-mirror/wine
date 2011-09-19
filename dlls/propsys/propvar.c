/*
 * PropVariant implementation
 *
 * Copyright 2008 James Hawkins for CodeWeavers
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
#include <stdio.h>

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winuser.h"
#include "shlobj.h"
#include "propvarutil.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(propsys);

static HRESULT PROPVAR_ConvertFILETIME(PROPVARIANT *ppropvarDest,
                                       REFPROPVARIANT propvarSrc, VARTYPE vt)
{
    SYSTEMTIME time;

    FileTimeToSystemTime(&propvarSrc->u.filetime, &time);

    switch (vt)
    {
        case VT_LPSTR:
        {
            static const char format[] = "%04d/%02d/%02d:%02d:%02d:%02d.%03d";

            ppropvarDest->u.pszVal = HeapAlloc(GetProcessHeap(), 0,
                                             lstrlenA(format) + 1);
            if (!ppropvarDest->u.pszVal)
                return E_OUTOFMEMORY;

            sprintf(ppropvarDest->u.pszVal, format, time.wYear, time.wMonth,
                    time.wDay, time.wHour, time.wMinute,
                    time.wSecond, time.wMilliseconds);

            return S_OK;
        }

        default:
            FIXME("Unhandled target type: %d\n", vt);
    }

    return E_FAIL;
}

/******************************************************************
 *  PropVariantChangeType   (PROPSYS.@)
 */
HRESULT WINAPI PropVariantChangeType(PROPVARIANT *ppropvarDest, REFPROPVARIANT propvarSrc,
                                     PROPVAR_CHANGE_FLAGS flags, VARTYPE vt)
{
    FIXME("(%p, %p, %d, %d, %d): semi-stub!\n", ppropvarDest, propvarSrc,
          propvarSrc->vt, flags, vt);

    switch (propvarSrc->vt)
    {
        case VT_FILETIME:
            return PROPVAR_ConvertFILETIME(ppropvarDest, propvarSrc, vt);
        default:
            FIXME("Unhandled source type: %d\n", propvarSrc->vt);
    }

    return E_FAIL;
}

static void PROPVAR_GUIDToWSTR(REFGUID guid, WCHAR *str)
{
    static const WCHAR format[] = {'{','%','0','8','X','-','%','0','4','X','-','%','0','4','X',
        '-','%','0','2','X','%','0','2','X','-','%','0','2','X','%','0','2','X','%','0','2','X',
        '%','0','2','X','%','0','2','X','%','0','2','X','}',0};

    sprintfW(str, format, guid->Data1, guid->Data2, guid->Data3,
            guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
            guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

HRESULT WINAPI InitPropVariantFromGUIDAsString(REFGUID guid, PROPVARIANT *ppropvar)
{
    TRACE("(%p %p)\n", guid, ppropvar);

    if(!guid)
        return E_FAIL;

    ppropvar->vt = VT_LPWSTR;
    ppropvar->u.pwszVal = CoTaskMemAlloc(39*sizeof(WCHAR));
    if(!ppropvar->u.pwszVal)
        return E_OUTOFMEMORY;

    PROPVAR_GUIDToWSTR(guid, ppropvar->u.pwszVal);
    return S_OK;
}

HRESULT WINAPI InitVariantFromGUIDAsString(REFGUID guid, VARIANT *pvar)
{
    TRACE("(%p %p)\n", guid, pvar);

    if(!guid) {
        FIXME("guid == NULL\n");
        return E_FAIL;
    }

    V_VT(pvar) = VT_BSTR;
    V_BSTR(pvar) = SysAllocStringLen(NULL, 38);
    if(!V_BSTR(pvar))
        return E_OUTOFMEMORY;

    PROPVAR_GUIDToWSTR(guid, V_BSTR(pvar));
    return S_OK;
}

HRESULT WINAPI InitPropVariantFromBuffer(const VOID *pv, UINT cb, PROPVARIANT *ppropvar)
{
    TRACE("(%p %u %p)\n", pv, cb, ppropvar);

    ppropvar->u.caub.pElems = CoTaskMemAlloc(cb);
    if(!ppropvar->u.caub.pElems)
        return E_OUTOFMEMORY;

    ppropvar->vt = VT_VECTOR|VT_UI1;
    ppropvar->u.caub.cElems = cb;
    memcpy(ppropvar->u.caub.pElems, pv, cb);
    return S_OK;
}

HRESULT WINAPI InitVariantFromBuffer(const VOID *pv, UINT cb, VARIANT *pvar)
{
    SAFEARRAY *arr;
    void *data;
    HRESULT hres;

    TRACE("(%p %u %p)\n", pv, cb, pvar);

    arr = SafeArrayCreateVector(VT_UI1, 0, cb);
    if(!arr)
        return E_OUTOFMEMORY;

    hres = SafeArrayAccessData(arr, &data);
    if(FAILED(hres)) {
        SafeArrayDestroy(arr);
        return hres;
    }

    memcpy(data, pv, cb);

    hres = SafeArrayUnaccessData(arr);
    if(FAILED(hres)) {
        SafeArrayDestroy(arr);
        return hres;
    }

    V_VT(pvar) = VT_ARRAY|VT_UI1;
    V_ARRAY(pvar) = arr;
    return S_OK;
}

static inline DWORD PROPVAR_HexToNum(const WCHAR *hex)
{
    DWORD ret;

    if(hex[0]>='0' && hex[0]<='9')
        ret = hex[0]-'0';
    else if(hex[0]>='a' && hex[0]<='f')
        ret = hex[0]-'a'+10;
    else if(hex[0]>='A' && hex[0]<='F')
        ret = hex[0]-'A'+10;
    else
        return -1;

    ret <<= 4;
    if(hex[1]>='0' && hex[1]<='9')
        return ret + hex[1]-'0';
    else if(hex[1]>='a' && hex[1]<='f')
        return ret + hex[1]-'a'+10;
    else if(hex[1]>='A' && hex[1]<='F')
        return ret + hex[1]-'A'+10;
    else
        return -1;
}

static inline HRESULT PROPVAR_WCHARToGUID(const WCHAR *str, int len, GUID *guid)
{
    DWORD i, val=0;
    const WCHAR *p;

    memset(guid, 0, sizeof(GUID));

    if(len!=38 || str[0]!='{' || str[9]!='-' || str[14]!='-'
            || str[19]!='-' || str[24]!='-' || str[37]!='}') {
        WARN("Error parsing %s\n", debugstr_w(str));
        return E_INVALIDARG;
    }

    p = str+1;
    for(i=0; i<4 && val!=-1; i++) {
        val = PROPVAR_HexToNum(p);
        guid->Data1 = (guid->Data1<<8) + val;
        p += 2;
    }
    p++;
    for(i=0; i<2 && val!=-1; i++) {
        val = PROPVAR_HexToNum(p);
        guid->Data2 = (guid->Data2<<8) + val;
        p += 2;
    }
    p++;
    for(i=0; i<2 && val!=-1; i++) {
        val = PROPVAR_HexToNum(p);
        guid->Data3 = (guid->Data3<<8) + val;
        p += 2;
    }
    p++;
    for(i=0; i<8 && val!=-1; i++) {
        if(i == 2)
            p++;

        val = guid->Data4[i] = PROPVAR_HexToNum(p);
        p += 2;
    }

    if(val == -1) {
        WARN("Error parsing %s\n", debugstr_w(str));
        memset(guid, 0, sizeof(GUID));
        return E_INVALIDARG;
    }
    return S_OK;
}

HRESULT WINAPI PropVariantToGUID(const PROPVARIANT *ppropvar, GUID *guid)
{
    TRACE("%p %p)\n", ppropvar, guid);

    switch(ppropvar->vt) {
    case VT_BSTR:
        return PROPVAR_WCHARToGUID(ppropvar->u.bstrVal, SysStringLen(ppropvar->u.bstrVal), guid);
    case VT_LPWSTR:
        return PROPVAR_WCHARToGUID(ppropvar->u.pwszVal, strlenW(ppropvar->u.pwszVal), guid);

    default:
        FIXME("unsupported vt: %d\n", ppropvar->vt);
        return E_NOTIMPL;
    }
}

HRESULT WINAPI VariantToGUID(const VARIANT *pvar, GUID *guid)
{
    TRACE("(%p %p)\n", pvar, guid);

    switch(V_VT(pvar)) {
    case VT_BSTR: {
        HRESULT hres = PROPVAR_WCHARToGUID(V_BSTR(pvar), SysStringLen(V_BSTR(pvar)), guid);
        if(hres == E_INVALIDARG)
            return E_FAIL;
        return hres;
    }

    default:
        FIXME("unsupported vt: %d\n", V_VT(pvar));
        return E_NOTIMPL;
    }
}
