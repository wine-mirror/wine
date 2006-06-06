/*
 * Implementation of hyperlinking (hlink.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
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

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "unknwn.h"

#include "wine/debug.h"
#include "hlink.h"

WINE_DEFAULT_DEBUG_CHANNEL(hlink);


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p %ld %p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

HRESULT WINAPI DllCanUnloadNow( void )
{
    FIXME("\n");
    return S_OK;
}

HRESULT WINAPI HlinkCreateFromMoniker( IMoniker *pimkTrgt, LPCWSTR pwzLocation,
        LPCWSTR pwzFriendlyName, IHlinkSite* pihlsite, DWORD dwSiteData,
        IUnknown* piunkOuter, REFIID riid, void** ppvObj)
{
    FIXME("%p %s %s %p %li %p %s %p\n", pimkTrgt, debugstr_w(pwzLocation),
          debugstr_w(pwzFriendlyName), pihlsite, dwSiteData, piunkOuter,
          debugstr_guid(riid), ppvObj);

    return E_NOTIMPL;
}

HRESULT WINAPI HlinkCreateFromString( LPCWSTR pwzTarget, LPCWSTR pwzLocation,
        LPCWSTR pwzFriendlyName, IHlinkSite* pihlsite, DWORD dwSiteData,
        IUnknown* piunkOuter, REFIID riid, void** ppvObj)
{
    FIXME("%s %s %s %p %li %p %s %p\n", debugstr_w(pwzTarget),
          debugstr_w(pwzLocation), debugstr_w(pwzFriendlyName), pihlsite,
          dwSiteData, piunkOuter, debugstr_guid(riid), ppvObj);

    return E_NOTIMPL;
}


HRESULT WINAPI HlinkCreateBrowseContext( IUnknown* piunkOuter, REFIID riid, void** ppvObj)
{
    FIXME("%p %s %p\n", piunkOuter, debugstr_guid(riid), ppvObj);

    return E_NOTIMPL;
}

HRESULT WINAPI HlinkNavigate(IHlink *phl, IHlinkFrame *phlFrame,
        DWORD grfHLNF, LPBC pbc, IBindStatusCallback *pbsc,
        IHlinkBrowseContext *phlbc)
{
    FIXME("%p %p %li %p %p %p\n", phl, phlFrame, grfHLNF, pbc, pbsc, phlbc);

    return E_NOTIMPL;
}

HRESULT WINAPI HlinkOnNavigate( IHlinkFrame *phlFrame,
        IHlinkBrowseContext* phlbc, DWORD grfHLNF, IMoniker *pmkTarget,
        LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, ULONG* puHLID)
{
    FIXME("%p %p %li %p %s %s %p\n",phlFrame, phlbc, grfHLNF, pmkTarget,
          debugstr_w(pwzLocation), debugstr_w(pwzFriendlyName), puHLID);

    return E_NOTIMPL;
}

HRESULT WINAPI HlinkCreateFromData(IDataObject *piDataObj,
        IHlinkSite *pihlsite, DWORD dwSiteData, IUnknown *piunkOuter,
        REFIID riid, void **ppvObj)
{
    FIXME("%p %p %ld %p %p %p\n",
          piDataObj, pihlsite, dwSiteData, piunkOuter, riid, ppvObj);

    return E_NOTIMPL;
}

HRESULT WINAPI HlinkCreateExtensionServices(LPCWSTR pwzAdditionalHeaders,
        HWND phwnd, LPCWSTR pszUsername, LPCWSTR pszPassword,
        IUnknown *punkOuter, REFIID riid, void** ppvObj)
{
    FIXME("%s %p %s %s %p %s %p\n",debugstr_w(pwzAdditionalHeaders),
            phwnd, debugstr_w(pszUsername), debugstr_w(pszPassword),
            punkOuter, debugstr_guid(riid), ppvObj);

    return E_NOTIMPL;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;

    return CLASS_E_CLASSNOTAVAILABLE;
}
