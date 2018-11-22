/*
 * Copyright 2016 Sebastian Lackner
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

#include "config.h"
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "initguid.h"
#include "ocidl.h"
#include "shellscalingapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shcore);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    TRACE("(%p, %u, %p)\n", instance, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;  /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

HRESULT WINAPI GetProcessDpiAwareness(HANDLE process, PROCESS_DPI_AWARENESS *value)
{
    if (GetProcessDpiAwarenessInternal( process, (DPI_AWARENESS *)value )) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
}

HRESULT WINAPI SetProcessDpiAwareness(PROCESS_DPI_AWARENESS value)
{
    if (SetProcessDpiAwarenessInternal( value )) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
}

HRESULT WINAPI GetDpiForMonitor(HMONITOR monitor, MONITOR_DPI_TYPE type, UINT *x, UINT *y)
{
    if (GetDpiForMonitorInternal( monitor, type, x, y )) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
}

HRESULT WINAPI _IStream_Read(IStream *stream, void *dest, ULONG size)
{
    ULONG read;
    HRESULT hr;

    TRACE("(%p, %p, %u)\n", stream, dest, size);

    hr = IStream_Read(stream, dest, size, &read);
    if (SUCCEEDED(hr) && read != size)
        hr = E_FAIL;
    return hr;
}

HRESULT WINAPI IStream_Reset(IStream *stream)
{
    static const LARGE_INTEGER zero;

    TRACE("(%p)\n", stream);

    return IStream_Seek(stream, zero, 0, NULL);
}

HRESULT WINAPI IStream_Size(IStream *stream, ULARGE_INTEGER *size)
{
    STATSTG statstg;
    HRESULT hr;

    TRACE("(%p, %p)\n", stream, size);

    memset(&statstg, 0, sizeof(statstg));

    hr = IStream_Stat(stream, &statstg, STATFLAG_NONAME);

    if (SUCCEEDED(hr) && size)
        *size = statstg.cbSize;
    return hr;
}

HRESULT WINAPI _IStream_Write(IStream *stream, const void *src, ULONG size)
{
    ULONG written;
    HRESULT hr;

    TRACE("(%p, %p, %u)\n", stream, src, size);

    hr = IStream_Write(stream, src, size, &written);
    if (SUCCEEDED(hr) && written != size)
        hr = E_FAIL;

    return hr;
}

void WINAPI IUnknown_AtomicRelease(IUnknown **obj)
{
    TRACE("(%p)\n", obj);

    if (!obj || !*obj)
        return;

    IUnknown_Release(*obj);
    *obj = NULL;
}

HRESULT WINAPI IUnknown_GetSite(IUnknown *unk, REFIID iid, void **site)
{
    IObjectWithSite *obj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p, %s, %p)\n", unk, debugstr_guid(iid), site);

    if (unk && iid && site)
    {
        hr = IUnknown_QueryInterface(unk, &IID_IObjectWithSite, (void **)&obj);
        if (SUCCEEDED(hr) && obj)
        {
            hr = IObjectWithSite_GetSite(obj, iid, site);
            IObjectWithSite_Release(obj);
        }
    }

    return hr;
}

HRESULT WINAPI IUnknown_QueryService(IUnknown *obj, REFGUID sid, REFIID iid, void **out)
{
    IServiceProvider *provider = NULL;
    HRESULT hr;

    if (!out)
        return E_FAIL;

    *out = NULL;

    if (!obj)
        return E_FAIL;

    hr = IUnknown_QueryInterface(obj, &IID_IServiceProvider, (void **)&provider);
    if (hr == S_OK && provider)
    {
        TRACE("Using provider %p.\n", provider);

        hr = IServiceProvider_QueryService(provider, sid, iid, out);

        TRACE("Provider %p returned %p.\n", provider, *out);

        IServiceProvider_Release(provider);
    }

    return hr;
}

void WINAPI IUnknown_Set(IUnknown **dest, IUnknown *src)
{
    TRACE("(%p, %p)\n", dest, src);

    IUnknown_AtomicRelease(dest);

    if (src)
    {
        IUnknown_AddRef(src);
        *dest = src;
    }
}

HRESULT WINAPI IUnknown_SetSite(IUnknown *obj, IUnknown *site)
{
    IInternetSecurityManager *sec_manager;
    IObjectWithSite *objwithsite;
    HRESULT hr;

    if (!obj)
        return E_FAIL;

    hr = IUnknown_QueryInterface(obj, &IID_IObjectWithSite, (void **)&objwithsite);
    TRACE("ObjectWithSite %p, hr %#x.\n", objwithsite, hr);
    if (SUCCEEDED(hr))
    {
        hr = IObjectWithSite_SetSite(objwithsite, site);
        TRACE("SetSite() hr %#x.\n", hr);
        IObjectWithSite_Release(objwithsite);
    }
    else
    {
        hr = IUnknown_QueryInterface(obj, &IID_IInternetSecurityManager, (void **)&sec_manager);
        TRACE("InternetSecurityManager %p, hr %#x.\n", sec_manager, hr);
        if (FAILED(hr))
            return hr;

        hr = IInternetSecurityManager_SetSecuritySite(sec_manager, (IInternetSecurityMgrSite *)site);
        TRACE("SetSecuritySite() hr %#x.\n", hr);
        IInternetSecurityManager_Release(sec_manager);
    }

    return hr;
}
