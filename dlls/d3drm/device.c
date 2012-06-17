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
#include "initguid.h"
#include "d3drmwin.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3drm);

typedef struct {
    IDirect3DRMDevice2 IDirect3DRMDevice2_iface;
    IDirect3DRMDevice3 IDirect3DRMDevice3_iface;
    IDirect3DRMWinDevice IDirect3DRMWinDevice_iface;
    LONG ref;
    BOOL dither;
    D3DRMRENDERQUALITY quality;
    DWORD rendermode;
    DWORD height;
    DWORD width;
} IDirect3DRMDeviceImpl;

static inline IDirect3DRMDeviceImpl *impl_from_IDirect3DRMDevice2(IDirect3DRMDevice2 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMDeviceImpl, IDirect3DRMDevice2_iface);
}

static inline IDirect3DRMDeviceImpl *impl_from_IDirect3DRMDevice3(IDirect3DRMDevice3 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMDeviceImpl, IDirect3DRMDevice3_iface);
}

static inline IDirect3DRMDeviceImpl *impl_from_IDirect3DRMWinDevice(IDirect3DRMWinDevice *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMDeviceImpl, IDirect3DRMWinDevice_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMDevice2Impl_QueryInterface(IDirect3DRMDevice2* iface,
                                                            REFIID riid, void** object)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), object);

    *object = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirect3DRMDevice) ||
        IsEqualGUID(riid, &IID_IDirect3DRMDevice2))
    {
        *object = &This->IDirect3DRMDevice2_iface;
    }
    else if(IsEqualGUID(riid, &IID_IDirect3DRMDevice3))
    {
        *object = &This->IDirect3DRMDevice3_iface;
    }
    else if(IsEqualGUID(riid, &IID_IDirect3DRMWinDevice))
    {
        *object = &This->IDirect3DRMWinDevice_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDirect3DRMDevice2_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI IDirect3DRMDevice2Impl_AddRef(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirect3DRMDevice2Impl_Release(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", iface, ref);

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

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, size, name);

    return IDirect3DRMDevice3_GetClassName(&This->IDirect3DRMDevice3_iface, size, name);
}

/*** IDirect3DRMDevice methods ***/
static HRESULT WINAPI IDirect3DRMDevice2Impl_Init(IDirect3DRMDevice2* iface, ULONG width,
                                                  ULONG height)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    TRACE("(%p/%p)->(%u, %u)\n", iface, This, width, height);

    return IDirect3DRMDevice3_Init(&This->IDirect3DRMDevice3_iface, width, height);
}

/*** IDirect3DRMDevice2 methods ***/
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

    TRACE("(%p/%p)->(%p, %p, %u, %u)\n", iface, This, lpDDClipper, lpGUID, width, height);

    return IDirect3DRMDevice3_InitFromClipper(&This->IDirect3DRMDevice3_iface, lpDDClipper, lpGUID,
                                              width, height);
}

static HRESULT WINAPI IDirect3DRMDevice2Impl_Update(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return D3DRM_OK;
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

    TRACE("(%p/%p)->(%d)\n", iface, This, enable);

    return IDirect3DRMDevice3_SetDither(&This->IDirect3DRMDevice3_iface, enable);
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

    TRACE("(%p/%p)->(%u)\n", iface, This, quality);

    return IDirect3DRMDevice3_SetQuality(&This->IDirect3DRMDevice3_iface, quality);
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

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirect3DRMDevice3_GetDither(&This->IDirect3DRMDevice3_iface);
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

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirect3DRMDevice3_GetHeight(&This->IDirect3DRMDevice3_iface);
}

static DWORD WINAPI IDirect3DRMDevice2Impl_GetWidth(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirect3DRMDevice3_GetWidth(&This->IDirect3DRMDevice3_iface);
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

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirect3DRMDevice3_GetQuality(&This->IDirect3DRMDevice3_iface);
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

    TRACE("(%p/%p)->(%u)\n", iface, This, dwFlags);

    return IDirect3DRMDevice3_SetRenderMode(&This->IDirect3DRMDevice3_iface, dwFlags);
}

static DWORD WINAPI IDirect3DRMDevice2Impl_GetRenderMode(IDirect3DRMDevice2* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice2(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirect3DRMDevice3_GetRenderMode(&This->IDirect3DRMDevice3_iface);
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


/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMDevice3Impl_QueryInterface(IDirect3DRMDevice3* iface,
                                                            REFIID riid, void** object)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);
    return IDirect3DRMDevice2_QueryInterface(&This->IDirect3DRMDevice2_iface, riid, object);
}

static ULONG WINAPI IDirect3DRMDevice3Impl_AddRef(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);
    return IDirect3DRMDevice2_AddRef(&This->IDirect3DRMDevice2_iface);
}

static ULONG WINAPI IDirect3DRMDevice3Impl_Release(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);
    return IDirect3DRMDevice2_Release(&This->IDirect3DRMDevice2_iface);
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMDevice3Impl_Clone(IDirect3DRMDevice3* iface,
                                                        LPUNKNOWN unkwn, REFIID riid,
                                                        LPVOID* object)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%p, %s, %p): stub\n", iface, This, unkwn, debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_AddDestroyCallback(IDirect3DRMDevice3* iface,
                                                                     D3DRMOBJECTCALLBACK cb,
                                                                     LPVOID argument)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_DeleteDestroyCallback(IDirect3DRMDevice3* iface,
                                                                        D3DRMOBJECTCALLBACK cb,
                                                                        LPVOID argument)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_SetAppData(IDirect3DRMDevice3* iface,
                                                             DWORD data)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMDevice3Impl_GetAppData(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_SetName(IDirect3DRMDevice3* iface, LPCSTR name)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_GetName(IDirect3DRMDevice3* iface,
                                                          LPDWORD size, LPSTR name)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_GetClassName(IDirect3DRMDevice3* iface,
                                                          LPDWORD size, LPSTR name)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, size, name);

    if (!size || *size < strlen("Device") || !name)
        return E_INVALIDARG;

    strcpy(name, "Device");
    *size = sizeof("Device");

    return D3DRM_OK;
}

/*** IDirect3DRMDevice methods ***/
static HRESULT WINAPI IDirect3DRMDevice3Impl_Init(IDirect3DRMDevice3* iface, ULONG width,
                                                  ULONG height)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%u, %u): stub\n", iface, This, width, height);

    This->height = height;
    This->width = width;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_InitFromD3D(IDirect3DRMDevice3* iface,
                                                         LPDIRECT3D lpD3D,
                                                         LPDIRECT3DDEVICE lpD3DDev)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, lpD3D, lpD3DDev);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_InitFromClipper(IDirect3DRMDevice3* iface,
                                                             LPDIRECTDRAWCLIPPER lpDDClipper,
                                                             LPGUID lpGUID, int width, int height)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%p, %p, %u, %u): stub\n", iface, This, lpDDClipper, lpGUID, width, height);

    This->height = height;
    This->width = width;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_Update(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_AddUpdateCallback(IDirect3DRMDevice3* iface,
                                                               D3DRMUPDATECALLBACK cb, LPVOID arg)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, arg);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_DeleteUpdateCallback(IDirect3DRMDevice3* iface,
                                                                  D3DRMUPDATECALLBACK cb,
                                                                  LPVOID arg)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, arg);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_SetBufferCount(IDirect3DRMDevice3* iface, DWORD count)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, count);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMDevice3Impl_GetBufferCount(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_SetDither(IDirect3DRMDevice3* iface, BOOL enable)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    TRACE("(%p/%p)->(%d)\n", iface, This, enable);

    This->dither = enable;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_SetShades(IDirect3DRMDevice3* iface, DWORD count)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_SetQuality(IDirect3DRMDevice3* iface,
                                                        D3DRMRENDERQUALITY quality)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    TRACE("(%p/%p)->(%u)\n", iface, This, quality);

    This->quality = quality;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_SetTextureQuality(IDirect3DRMDevice3* iface,
                                                               D3DRMTEXTUREQUALITY quality)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, quality);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_GetViewports(IDirect3DRMDevice3* iface,
                                                          LPDIRECT3DRMVIEWPORTARRAY *return_views)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, return_views);

    return E_NOTIMPL;
}

static BOOL WINAPI IDirect3DRMDevice3Impl_GetDither(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->dither;
}

static DWORD WINAPI IDirect3DRMDevice3Impl_GetShades(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMDevice3Impl_GetHeight(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->height;
}

static DWORD WINAPI IDirect3DRMDevice3Impl_GetWidth(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->width;
}

static DWORD WINAPI IDirect3DRMDevice3Impl_GetTrianglesDrawn(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMDevice3Impl_GetWireframeOptions(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static D3DRMRENDERQUALITY WINAPI IDirect3DRMDevice3Impl_GetQuality(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->quality;
}

static D3DCOLORMODEL WINAPI IDirect3DRMDevice3Impl_GetColorModel(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static D3DRMTEXTUREQUALITY WINAPI IDirect3DRMDevice3Impl_GetTextureQuality(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_GetDirect3DDevice(IDirect3DRMDevice3* iface,
                                                               LPDIRECT3DDEVICE * dev)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, dev);

    return E_NOTIMPL;
}

/*** IDirect3DRMDevice2 methods ***/
static HRESULT WINAPI IDirect3DRMDevice3Impl_InitFromD3D2(IDirect3DRMDevice3* iface,
                                                          LPDIRECT3D2 lpD3D,
                                                          LPDIRECT3DDEVICE2 lpD3DDev)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, lpD3D, lpD3DDev);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_InitFromSurface(IDirect3DRMDevice3* iface,
                                                             LPGUID lpGUID, LPDIRECTDRAW lpDD,
                                                             LPDIRECTDRAWSURFACE lpDDSBack)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%p, %p, %p): stub\n", iface, This, lpGUID, lpDD, lpDDSBack);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_SetRenderMode(IDirect3DRMDevice3* iface, DWORD dwFlags)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    TRACE("(%p/%p)->(%u)\n", iface, This, dwFlags);

    This->rendermode = dwFlags;

    return D3DRM_OK;
}

static DWORD WINAPI IDirect3DRMDevice3Impl_GetRenderMode(IDirect3DRMDevice3* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->rendermode;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_GetDirect3DDevice2(IDirect3DRMDevice3* iface,
                                                                LPDIRECT3DDEVICE2 * dev)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, dev);

    return E_NOTIMPL;
}

/*** IDirect3DRMDevice3 methods ***/
static HRESULT WINAPI IDirect3DRMDevice3Impl_FindPreferredTextureFormat(IDirect3DRMDevice3* iface,
                                                                        DWORD bitdepths,
                                                                        DWORD flags,
                                                                        LPDDPIXELFORMAT lpDDPF)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%u, %u, %p): stub\n", iface, This, bitdepths, flags, lpDDPF);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_RenderStateChange(IDirect3DRMDevice3* iface,
                                                               D3DRENDERSTATETYPE type, DWORD val,
                                                               DWORD flags)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%u, %u, %u): stub\n", iface, This, type, val, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_LightStateChange(IDirect3DRMDevice3* iface,
                                                              D3DLIGHTSTATETYPE type, DWORD val,
                                                              DWORD flags)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%u, %u, %u): stub\n", iface, This, type, val, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_GetStateChangeOptions(IDirect3DRMDevice3* iface,
                                                                   DWORD stateclass, DWORD statenum,
                                                                   LPDWORD flags)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%u, %u, %p): stub\n", iface, This, stateclass, statenum, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMDevice3Impl_SetStateChangeOptions(IDirect3DRMDevice3* iface,
                                                                   DWORD stateclass, DWORD statenum,
                                                                   DWORD flags)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMDevice3(iface);

    FIXME("(%p/%p)->(%u, %u, %u): stub\n", iface, This, stateclass, statenum, flags);

    return E_NOTIMPL;
}

static const struct IDirect3DRMDevice3Vtbl Direct3DRMDevice3_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMDevice3Impl_QueryInterface,
    IDirect3DRMDevice3Impl_AddRef,
    IDirect3DRMDevice3Impl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMDevice3Impl_Clone,
    IDirect3DRMDevice3Impl_AddDestroyCallback,
    IDirect3DRMDevice3Impl_DeleteDestroyCallback,
    IDirect3DRMDevice3Impl_SetAppData,
    IDirect3DRMDevice3Impl_GetAppData,
    IDirect3DRMDevice3Impl_SetName,
    IDirect3DRMDevice3Impl_GetName,
    IDirect3DRMDevice3Impl_GetClassName,
    /*** IDirect3DRMDevice methods ***/
    IDirect3DRMDevice3Impl_Init,
    IDirect3DRMDevice3Impl_InitFromD3D,
    IDirect3DRMDevice3Impl_InitFromClipper,
    IDirect3DRMDevice3Impl_Update,
    IDirect3DRMDevice3Impl_AddUpdateCallback,
    IDirect3DRMDevice3Impl_DeleteUpdateCallback,
    IDirect3DRMDevice3Impl_SetBufferCount,
    IDirect3DRMDevice3Impl_GetBufferCount,
    IDirect3DRMDevice3Impl_SetDither,
    IDirect3DRMDevice3Impl_SetShades,
    IDirect3DRMDevice3Impl_SetQuality,
    IDirect3DRMDevice3Impl_SetTextureQuality,
    IDirect3DRMDevice3Impl_GetViewports,
    IDirect3DRMDevice3Impl_GetDither,
    IDirect3DRMDevice3Impl_GetShades,
    IDirect3DRMDevice3Impl_GetHeight,
    IDirect3DRMDevice3Impl_GetWidth,
    IDirect3DRMDevice3Impl_GetTrianglesDrawn,
    IDirect3DRMDevice3Impl_GetWireframeOptions,
    IDirect3DRMDevice3Impl_GetQuality,
    IDirect3DRMDevice3Impl_GetColorModel,
    IDirect3DRMDevice3Impl_GetTextureQuality,
    IDirect3DRMDevice3Impl_GetDirect3DDevice,
    /*** IDirect3DRMDevice2 methods ***/
    IDirect3DRMDevice3Impl_InitFromD3D2,
    IDirect3DRMDevice3Impl_InitFromSurface,
    IDirect3DRMDevice3Impl_SetRenderMode,
    IDirect3DRMDevice3Impl_GetRenderMode,
    IDirect3DRMDevice3Impl_GetDirect3DDevice2,
    /*** IDirect3DRMDevice3 methods ***/
    IDirect3DRMDevice3Impl_FindPreferredTextureFormat,
    IDirect3DRMDevice3Impl_RenderStateChange,
    IDirect3DRMDevice3Impl_LightStateChange,
    IDirect3DRMDevice3Impl_GetStateChangeOptions,
    IDirect3DRMDevice3Impl_SetStateChangeOptions
};


/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMWinDeviceImpl_QueryInterface(IDirect3DRMWinDevice* iface,
                                                              REFIID riid, void** object)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMWinDevice(iface);
    return IDirect3DRMDevice2_QueryInterface(&This->IDirect3DRMDevice2_iface, riid, object);
}

static ULONG WINAPI IDirect3DRMWinDeviceImpl_AddRef(IDirect3DRMWinDevice* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMWinDevice(iface);
    return IDirect3DRMDevice2_AddRef(&This->IDirect3DRMDevice2_iface);
}

static ULONG WINAPI IDirect3DRMWinDeviceImpl_Release(IDirect3DRMWinDevice* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMWinDevice(iface);
    return IDirect3DRMDevice2_Release(&This->IDirect3DRMDevice2_iface);
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMWinDeviceImpl_Clone(IDirect3DRMWinDevice* iface,
                                                        LPUNKNOWN unkwn, REFIID riid,
                                                        LPVOID* object)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMWinDevice(iface);

    FIXME("(%p/%p)->(%p, %s, %p): stub\n", iface, This, unkwn, debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMWinDeviceImpl_AddDestroyCallback(IDirect3DRMWinDevice* iface,
                                                                     D3DRMOBJECTCALLBACK cb,
                                                                     LPVOID argument)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMWinDevice(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMWinDeviceImpl_DeleteDestroyCallback(IDirect3DRMWinDevice* iface,
                                                                        D3DRMOBJECTCALLBACK cb,
                                                                        LPVOID argument)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMWinDevice(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMWinDeviceImpl_SetAppData(IDirect3DRMWinDevice* iface,
                                                             DWORD data)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMWinDevice(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMWinDeviceImpl_GetAppData(IDirect3DRMWinDevice* iface)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMWinDevice(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMWinDeviceImpl_SetName(IDirect3DRMWinDevice* iface, LPCSTR name)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMWinDevice(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMWinDeviceImpl_GetName(IDirect3DRMWinDevice* iface,
                                                          LPDWORD size, LPSTR name)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMWinDevice(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMWinDeviceImpl_GetClassName(IDirect3DRMWinDevice* iface,
                                                            LPDWORD size, LPSTR name)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMWinDevice(iface);

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, size, name);

    return IDirect3DRMDevice3_GetClassName(&This->IDirect3DRMDevice3_iface, size, name);
}

/*** IDirect3DRMWinDevice methods ***/
static HRESULT WINAPI IDirect3DRMWinDeviceImpl_HandlePaint(IDirect3DRMWinDevice* iface, HDC hdc)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMWinDevice(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, hdc);

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMWinDeviceImpl_HandleActivate(IDirect3DRMWinDevice* iface, WORD wparam)
{
    IDirect3DRMDeviceImpl *This = impl_from_IDirect3DRMWinDevice(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, wparam);

    return D3DRM_OK;
}

static const struct IDirect3DRMWinDeviceVtbl Direct3DRMWinDevice_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMWinDeviceImpl_QueryInterface,
    IDirect3DRMWinDeviceImpl_AddRef,
    IDirect3DRMWinDeviceImpl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMWinDeviceImpl_Clone,
    IDirect3DRMWinDeviceImpl_AddDestroyCallback,
    IDirect3DRMWinDeviceImpl_DeleteDestroyCallback,
    IDirect3DRMWinDeviceImpl_SetAppData,
    IDirect3DRMWinDeviceImpl_GetAppData,
    IDirect3DRMWinDeviceImpl_SetName,
    IDirect3DRMWinDeviceImpl_GetName,
    IDirect3DRMWinDeviceImpl_GetClassName,
    /*** IDirect3DRMWinDevice methods ***/
    IDirect3DRMWinDeviceImpl_HandlePaint,
    IDirect3DRMWinDeviceImpl_HandleActivate
};

HRESULT Direct3DRMDevice_create(REFIID riid, IUnknown** ppObj)
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
    object->IDirect3DRMDevice3_iface.lpVtbl = &Direct3DRMDevice3_Vtbl;
    object->IDirect3DRMWinDevice_iface.lpVtbl = &Direct3DRMWinDevice_Vtbl;
    object->ref = 1;

    if (IsEqualGUID(riid, &IID_IDirect3DRMFrame3))
        *ppObj = (IUnknown*)&object->IDirect3DRMDevice3_iface;
    else
        *ppObj = (IUnknown*)&object->IDirect3DRMDevice2_iface;

    return S_OK;
}
