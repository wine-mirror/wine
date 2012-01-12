/*
 * Implementation of IDirect3DRMDevice Interface
 *
 * Copyright 2011, 2012 André Hentschel
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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

#include "wine/debug.h"

#define COBJMACROS

#include "winbase.h"
#include "wingdi.h"

#include "d3drm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3drm);

typedef struct {
    IDirect3DRMDevice2 IDirect3DRMDevice2_iface;
    LONG ref;
} IDirect3DRMDeviceImpl;

static const struct IDirect3DRMDevice2Vtbl Direct3DRMDevice2_Vtbl;

static inline IDirect3DRMDeviceImpl *impl_from_IDirect3DRMDevice2(IDirect3DRMDevice2 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMDeviceImpl, IDirect3DRMDevice2_iface);
}

HRESULT Direct3DRMDevice_create(IUnknown** ppObj)
{
    IDirect3DRMDeviceImpl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DRMDeviceImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->IDirect3DRMDevice2_iface.lpVtbl = &Direct3DRMDevice2_Vtbl;
    object->ref = 1;

    *ppObj = (IUnknown*)object;

    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMDevice2Impl_QueryInterface(IDirect3DRMDevice2* iface,
                                                            REFIID riid, void** object)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirect3DRMDevice) ||
        IsEqualGUID(riid, &IID_IDirect3DRMDevice2))
    {
        IClassFactory_AddRef(iface);
        *object = This;
        return S_OK;
    }

    ERR("(%p/%p)->(%s, %p),not found\n", iface, This, debugstr_guid(riid), object);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DRMDevice2Impl_AddRef(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IDirect3DRMDevice2Impl_Release(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)\n", This);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMDevice2Impl_Clone(IDirect3DRMDevice2* iface,
                                                        LPUNKNOWN unkwn, REFIID riid,
                                                        LPVOID* object)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%p, %s, %p): stub\n", iface, This, unkwn, debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_AddDestroyCallback(IDirect3DRMDevice2* iface,
                                                                     D3DRMOBJECTCALLBACK cb,
                                                                     LPVOID argument)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_DeleteDestroyCallback(IDirect3DRMDevice2* iface,
                                                                        D3DRMOBJECTCALLBACK cb,
                                                                        LPVOID argument)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_SetAppData(IDirect3DRMDevice2* iface,
                                                             DWORD data)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMDevice2Impl_GetAppData(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_SetName(IDirect3DRMDevice2* iface, LPCSTR name)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_GetName(IDirect3DRMDevice2* iface,
                                                          LPDWORD size, LPSTR name)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_GetClassName(IDirect3DRMDevice2* iface,
                                                               LPDWORD size, LPSTR name)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, size, name);

    return E_NOTIMPL;
}

/*** IDirect3DRMDevice methods ***/
static HRESULT WINAPI IDirect3DRMDevice2Impl_Init(IDirect3DRMDevice2* iface, ULONG width,
                                                  ULONG height)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%u, %u): stub\n", iface, This, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_InitFromD3D(IDirect3DRMDevice2* iface,
                                                         LPDIRECT3D lpD3D,
                                                         LPDIRECT3DDEVICE lpD3DDev)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, lpD3D, lpD3DDev);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_InitFromClipper(IDirect3DRMDevice2* iface,
                                                             LPDIRECTDRAWCLIPPER lpDDClipper,
                                                             LPGUID lpGUID, int width, int height)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%p, %p, %u, %u): stub\n", iface, This, lpDDClipper, lpGUID, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_Update(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_AddUpdateCallback(IDirect3DRMDevice2* iface,
                                                               D3DRMUPDATECALLBACK cb, LPVOID arg)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, arg);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_DeleteUpdateCallback(IDirect3DRMDevice2* iface,
                                                                  D3DRMUPDATECALLBACK cb,
                                                                  LPVOID arg)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, arg);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_SetBufferCount(IDirect3DRMDevice2* iface, DWORD count)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, count);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMDevice2Impl_GetBufferCount(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_SetDither(IDirect3DRMDevice2* iface, BOOL enable)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%d): stub\n", iface, This, enable);

    return S_OK;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_SetShades(IDirect3DRMDevice2* iface, DWORD count)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_SetQuality(IDirect3DRMDevice2* iface,
                                                        D3DRMRENDERQUALITY quality)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, quality);

    return S_OK;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_SetTextureQuality(IDirect3DRMDevice2* iface,
                                                               D3DRMTEXTUREQUALITY quality)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, quality);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_GetViewports(IDirect3DRMDevice2* iface,
                                                          LPDIRECT3DRMVIEWPORTARRAY *return_views)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, return_views);

    return E_NOTIMPL;
}

static BOOL WINAPI IDirect3DRMDevice2Impl_GetDither(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMDevice2Impl_GetShades(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMDevice2Impl_GetHeight(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMDevice2Impl_GetWidth(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMDevice2Impl_GetTrianglesDrawn(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMDevice2Impl_GetWireframeOptions(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static D3DRMRENDERQUALITY WINAPI IDirect3DRMDevice2Impl_GetQuality(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static D3DCOLORMODEL WINAPI IDirect3DRMDevice2Impl_GetColorModel(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static D3DRMTEXTUREQUALITY WINAPI IDirect3DRMDevice2Impl_GetTextureQuality(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_GetDirect3DDevice(IDirect3DRMDevice2* iface,
                                                               LPDIRECT3DDEVICE * dev)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, dev);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_InitFromD3D2(IDirect3DRMDevice2* iface,
                                                          LPDIRECT3D2 lpD3D,
                                                          LPDIRECT3DDEVICE2 lpD3DDev)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, lpD3D, lpD3DDev);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_InitFromSurface(IDirect3DRMDevice2* iface,
                                                             LPGUID lpGUID, LPDIRECTDRAW lpDD,
                                                             LPDIRECTDRAWSURFACE lpDDSBack)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%p, %p, %p): stub\n", iface, This, lpGUID, lpDD, lpDDSBack);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_SetRenderMode(IDirect3DRMDevice2* iface, DWORD dwFlags)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, dwFlags);

    return S_OK;
}

static DWORD WINAPI IDirect3DRMDevice2Impl_GetRenderMode(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_GetDirect3DDevice2(IDirect3DRMDevice2* iface,
                                                                LPDIRECT3DDEVICE2 * dev)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, dev);

    return E_NOTIMPL;
}

static const struct IDirect3DRMDevice2Vtbl Direct3DRMDevice2_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMDevice2Impl_QueryInterface,
    IDirect3DRMDevice2Impl_AddRef,
    IDirect3DRMDevice2Impl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMDevice2Impl_Clone,
    IDirect3DRMDevice2Impl_AddDestroyCallback,
    IDirect3DRMDevice2Impl_DeleteDestroyCallback,
    IDirect3DRMDevice2Impl_SetAppData,
    IDirect3DRMDevice2Impl_GetAppData,
    IDirect3DRMDevice2Impl_SetName,
    IDirect3DRMDevice2Impl_GetName,
    IDirect3DRMDevice2Impl_GetClassName,
    /*** IDirect3DRMDevice methods ***/
    IDirect3DRMDevice2Impl_Init,
    IDirect3DRMDevice2Impl_InitFromD3D,
    IDirect3DRMDevice2Impl_InitFromClipper,
    IDirect3DRMDevice2Impl_Update,
    IDirect3DRMDevice2Impl_AddUpdateCallback,
    IDirect3DRMDevice2Impl_DeleteUpdateCallback,
    IDirect3DRMDevice2Impl_SetBufferCount,
    IDirect3DRMDevice2Impl_GetBufferCount,
    IDirect3DRMDevice2Impl_SetDither,
    IDirect3DRMDevice2Impl_SetShades,
    IDirect3DRMDevice2Impl_SetQuality,
    IDirect3DRMDevice2Impl_SetTextureQuality,
    IDirect3DRMDevice2Impl_GetViewports,
    IDirect3DRMDevice2Impl_GetDither,
    IDirect3DRMDevice2Impl_GetShades,
    IDirect3DRMDevice2Impl_GetHeight,
    IDirect3DRMDevice2Impl_GetWidth,
    IDirect3DRMDevice2Impl_GetTrianglesDrawn,
    IDirect3DRMDevice2Impl_GetWireframeOptions,
    IDirect3DRMDevice2Impl_GetQuality,
    IDirect3DRMDevice2Impl_GetColorModel,
    IDirect3DRMDevice2Impl_GetTextureQuality,
    IDirect3DRMDevice2Impl_GetDirect3DDevice,
    /*** IDirect3DRMDevice2 methods ***/
    IDirect3DRMDevice2Impl_InitFromD3D2,
    IDirect3DRMDevice2Impl_InitFromSurface,
    IDirect3DRMDevice2Impl_SetRenderMode,
    IDirect3DRMDevice2Impl_GetRenderMode,
    IDirect3DRMDevice2Impl_GetDirect3DDevice2
};
