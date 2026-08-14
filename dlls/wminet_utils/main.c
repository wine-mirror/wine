/*
 * Copyright 2026 Esme Povirk for CodeWeavers
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

#include "windef.h"
#include "winbase.h"

#define COBJMACROS

#include "initguid.h"
#include "objidl.h"
#include "wbemcli.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wminet_utils);

HRESULT WINAPI CloneEnumWbemClassObject(IEnumWbemClassObject **ppEnum, DWORD authLevel,
    DWORD impLevel, IEnumWbemClassObject *pCurrentEnumWbemClassObject, BSTR strUser,
    BSTR strPassword, BSTR strAuthority)
{
    TRACE("\n");

    if (!ppEnum)
        return E_POINTER;

    *ppEnum = NULL;

    return IEnumWbemClassObject_Clone(pCurrentEnumWbemClassObject, ppEnum);
}

HRESULT WINAPI ConnectServerWmi(BSTR strNetworkResource, BSTR strUser, BSTR strPassword,
    BSTR strLocale, long lSecurityFlags, BSTR strAuthority, IWbemContext *pCtx,
    IWbemServices** ppNamespace, DWORD impLevel, DWORD authLevel)
{
    HRESULT hr;
    IWbemLocator *locator;

    TRACE("%li %li\n", impLevel, authLevel);

    if (!ppNamespace)
        return E_POINTER;

    *ppNamespace = NULL;

    hr = CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator,
        (void**)&locator);

    if (SUCCEEDED(hr))
    {
        hr = IWbemLocator_ConnectServer(locator, strNetworkResource, strUser, strPassword,
            strLocale, lSecurityFlags, strAuthority, pCtx, ppNamespace);

        IWbemLocator_Release(locator);
    }

    return hr;
}

HRESULT WINAPI CreateInstanceEnumWmi(BSTR strFilter, long lFlags, IWbemContext *pCtx,
    IEnumWbemClassObject **ppEnum, DWORD authLevel, DWORD impLevel, IWbemServices *pCurrentNamespace,
    BSTR strUser, BSTR strPassword, BSTR strAuthority)
{
    TRACE("%s %lx %p %p %p\n", debugstr_w(strFilter), lFlags, pCtx, ppEnum, pCurrentNamespace);

    if (!ppEnum)
        return E_POINTER;

    *ppEnum = NULL;

    return IWbemServices_CreateInstanceEnum(pCurrentNamespace, strFilter, lFlags, pCtx, ppEnum);
}

HRESULT WINAPI ExecQueryWmi(BSTR strQueryLanguage, BSTR strQuery, long lFlags, IWbemContext *pCtx,
    IEnumWbemClassObject **ppEnum, DWORD authLevel, DWORD impLevel, IWbemServices *pCurrentNamespace,
    BSTR strUser, BSTR strPassword, BSTR strAuthority)
{
    TRACE("%s %s %lx\n", debugstr_w(strQueryLanguage), debugstr_w(strQuery), lFlags);

    if (!ppEnum)
        return E_POINTER;

    *ppEnum = NULL;

    return IWbemServices_ExecQuery(pCurrentNamespace, strQueryLanguage, strQuery, lFlags, pCtx, ppEnum);
}

HRESULT WINAPI Get(int vFunc, IWbemClassObject *ptr, LPCWSTR wszName, LONG lFlags, VARIANT *pVal,
    CIMTYPE *pvtType, LONG *plFlavor)
{
    TRACE("%i %p %s %lx %p %p %p\n", vFunc, ptr, debugstr_w(wszName), lFlags, pVal, pvtType, plFlavor);

    return IWbemClassObject_Get(ptr, wszName, lFlags, pVal, pvtType, plFlavor);
}

HRESULT WINAPI GetCurrentApartmentType(int vFunc, IComThreadingInfo *ptr, APTTYPE *aptType)
{
    TRACE("%i %p %p\n", vFunc, ptr, aptType);
    return IComThreadingInfo_GetCurrentApartmentType(ptr, aptType);
}

HRESULT WINAPI Initialize(BOOLEAN bAllowIManagementObjectQI)
{
    TRACE("%i\n", bAllowIManagementObjectQI);
    return S_OK;
}

HRESULT WINAPI GetNames(int vFunc, IWbemClassObject *ptr, LPCWSTR wszQualifierName, LONG lFlags,
    VARIANT *pQualifierVal, SAFEARRAY **pNames)
{
    TRACE("%i %p %s %lx %p %p\n", vFunc, ptr, debugstr_w(wszQualifierName), lFlags, pQualifierVal, pNames);

    return IWbemClassObject_GetNames(ptr, wszQualifierName, lFlags, pQualifierVal, pNames);
}

HRESULT WINAPI BeginMethodEnumeration(int vFunc, IWbemClassObject *ptr, LONG lEnumFlags)
{
    TRACE("%i %p %lx\n", vFunc, ptr, lEnumFlags);

    return IWbemClassObject_BeginMethodEnumeration(ptr, lEnumFlags);
}

HRESULT WINAPI NextMethod(int vFunc, IWbemClassObject *ptr, LONG lFlags, BSTR *pstrName,
    IWbemClassObject **ppInSignature, IWbemClassObject **ppOutSignature)
{
    TRACE("%i %p, %lx, %p, %p, %p\n", vFunc, ptr, lFlags, pstrName, ppInSignature, ppOutSignature);

    return IWbemClassObject_NextMethod(ptr, lFlags, pstrName, ppInSignature, ppOutSignature);
}

HRESULT WINAPI EndMethodEnumeration(int vFunc, IWbemClassObject *ptr)
{
    TRACE("%i %p\n", vFunc, ptr);

    return IWbemClassObject_EndMethodEnumeration(ptr);
}

HRESULT WINAPI GetMethod(int vFunc, IWbemClassObject *ptr, LPCWSTR wszName, LONG lFlags,
    IWbemClassObject **ppInSignature, IWbemClassObject **ppOutSignature)
{
	TRACE("%i %p %s %lx %p %p\n", vFunc, ptr, debugstr_w(wszName), lFlags, ppInSignature, ppOutSignature);

	return IWbemClassObject_GetMethod(ptr, wszName, lFlags, ppInSignature, ppOutSignature);
}

HRESULT WINAPI GetMethodQualifierSet(int vFunc, IWbemClassObject *ptr, LPCWSTR wszMethod, IWbemQualifierSet **ppQualSet)
{
    TRACE("%i %p %s %p\n", vFunc, ptr, debugstr_w(wszMethod), ppQualSet);

    return IWbemClassObject_GetMethodQualifierSet(ptr, wszMethod, ppQualSet);
}

HRESULT WINAPI QualifierSet_Get(int vFunc, IWbemQualifierSet* ptr, LPCWSTR wszName, LONG lFlags, VARIANT *pVal, LONG *plFlavor)
{
    TRACE("%i %p %s %lx %p %p\n", vFunc, ptr, debugstr_w(wszName), lFlags, pVal, plFlavor);

    return IWbemQualifierSet_Get(ptr, wszName, lFlags, pVal, plFlavor);
}

HRESULT WINAPI GetPropertyQualifierSet(int vFunc, IWbemClassObject *ptr, LPCWSTR wszProperty, IWbemQualifierSet **ppQualSet)
{
    TRACE("%i %p %s %p\n", vFunc, ptr, debugstr_w(wszProperty), ppQualSet);

    return IWbemClassObject_GetPropertyQualifierSet(ptr, wszProperty, ppQualSet);
}

IErrorInfo* WINAPI wminet_utils_GetErrorInfo(void)
{
    IErrorInfo *error_info = NULL;
    HRESULT hr;

    hr = GetErrorInfo(0, &error_info);
    TRACE("returning %p, hr %#lx\n", error_info, hr);

    if (FAILED(hr))
        return NULL;

    return error_info;
}
