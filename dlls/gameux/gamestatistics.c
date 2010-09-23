/*
 *    Gameux library coclass GameStatistics implementation
 *
 * Copyright (C) 2010 Mariusz Pluciński
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

#include "config.h"

#include "ole2.h"
#include "winreg.h"

#include "gameux.h"
#include "gameux_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gameux);

/*
 * IGameStatisticsMgr implementation
 */

typedef struct _GameStatisticsMgrImpl
{
    const struct IGameStatisticsMgrVtbl *lpVtbl;
    LONG ref;
} GameStatisticsMgrImpl;

static inline GameStatisticsMgrImpl *impl_from_IGameStatisticsMgr( IGameStatisticsMgr *iface )
{
    return (GameStatisticsMgrImpl *)((char*)iface - FIELD_OFFSET(GameStatisticsMgrImpl, lpVtbl));
}


static HRESULT WINAPI GameStatisticsMgrImpl_QueryInterface(
        IGameStatisticsMgr *iface,
        REFIID riid,
        void **ppvObject)
{
    GameStatisticsMgrImpl *This = impl_from_IGameStatisticsMgr( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    *ppvObject = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IGameStatisticsMgr) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IGameStatisticsMgr_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI GameStatisticsMgrImpl_AddRef(IGameStatisticsMgr *iface)
{
    GameStatisticsMgrImpl *This = impl_from_IGameStatisticsMgr( iface );
    LONG ref;

    ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI GameStatisticsMgrImpl_Release(IGameStatisticsMgr *iface)
{
    GameStatisticsMgrImpl *This = impl_from_IGameStatisticsMgr( iface );
    LONG ref;

    ref = InterlockedDecrement(&This->ref);
    TRACE("(%p): ref=%d\n", This, ref);

    if ( ref == 0 )
    {
        TRACE("freeing GameStatistics object\n");
        HeapFree( GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT STDMETHODCALLTYPE GameStatisticsMgrImpl_GetGameStatistics(
        IGameStatisticsMgr* iface,
        LPCWSTR GDFBinaryPath,
        GAMESTATS_OPEN_TYPE openType,
        GAMESTATS_OPEN_RESULT *pOpenResult,
        IGameStatistics **ppiStats)
{
    static const WCHAR sApplicationId[] =
            {'A','p','p','l','i','c','a','t','i','o','n','I','d',0};

    HRESULT hr;
    GUID instanceId;
    LPWSTR lpRegistryPath = NULL;
    WCHAR lpApplicationId[49];
    DWORD dwLength = sizeof(lpApplicationId);
    GAME_INSTALL_SCOPE installScope;

    TRACE("(%p, %s, 0x%x, %p, %p)\n", iface, debugstr_w(GDFBinaryPath), openType, pOpenResult, ppiStats);

    if(!GDFBinaryPath)
        return E_INVALIDARG;

    installScope = GIS_CURRENT_USER;
    hr = GAMEUX_FindGameInstanceId(GDFBinaryPath, installScope, &instanceId);

    if(hr == S_FALSE)
    {
        installScope = GIS_ALL_USERS;
        hr = GAMEUX_FindGameInstanceId(GDFBinaryPath, installScope, &instanceId);
    }

    if(hr == S_FALSE)
        /* game not registered, so statistics cannot be used */
        hr = E_FAIL;

    if(SUCCEEDED(hr))
        /* game is registered, let's read it's application id from registry */
        hr = GAMEUX_buildGameRegistryPath(installScope, &instanceId, &lpRegistryPath);

    if(SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE,
                lpRegistryPath, sApplicationId, RRF_RT_REG_SZ,
                NULL, lpApplicationId, &dwLength));
    }

    if(SUCCEEDED(hr))
    {
        TRACE("found app id: %s\n", debugstr_w(lpApplicationId));
        FIXME("creating instance of IGameStatistics not yet implemented\n");
        hr = E_NOTIMPL;
    }

    HeapFree(GetProcessHeap(), 0, lpRegistryPath);

    return hr;
}

static HRESULT STDMETHODCALLTYPE GameStatisticsMgrImpl_RemoveGameStatistics(
        IGameStatisticsMgr* iface,
        LPCWSTR GDFBinaryPath)
{
    FIXME("stub (%p, %s)\n", iface, debugstr_w(GDFBinaryPath));
    return E_NOTIMPL;
}

static const struct IGameStatisticsMgrVtbl GameStatisticsMgrImplVtbl =
{
    GameStatisticsMgrImpl_QueryInterface,
    GameStatisticsMgrImpl_AddRef,
    GameStatisticsMgrImpl_Release,
    GameStatisticsMgrImpl_GetGameStatistics,
    GameStatisticsMgrImpl_RemoveGameStatistics,
};

HRESULT GameStatistics_create(
        IUnknown *pUnkOuter,
        IUnknown **ppObj)
{
    GameStatisticsMgrImpl *pGameStatistics;

    TRACE("(%p, %p)\n", pUnkOuter, ppObj);

    pGameStatistics = HeapAlloc( GetProcessHeap(), 0, sizeof (*pGameStatistics) );

    if( !pGameStatistics )
        return E_OUTOFMEMORY;

    pGameStatistics->lpVtbl = &GameStatisticsMgrImplVtbl;
    pGameStatistics->ref = 1;

    *ppObj = (IUnknown*)(&pGameStatistics->lpVtbl);

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
