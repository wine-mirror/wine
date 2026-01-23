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
