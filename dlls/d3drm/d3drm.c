/*
 * Implementation of IDirect3DRM Interface
 *
 * Copyright 2010 Christian Costa
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
    const IDirect3DRMVtbl *lpVtbl;
    LONG ref;
} IDirect3DRMImpl;

static const struct IDirect3DRMVtbl Direct3DRM_Vtbl;

HRESULT Direct3DRM_create(LPDIRECT3DRM* ppDirect3DRM)
{
    IDirect3DRMImpl* object;

    TRACE("(%p)\n", ppDirect3DRM);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DRMImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->lpVtbl = &Direct3DRM_Vtbl;
    object->ref = 1;

    *ppDirect3DRM = (IDirect3DRM*)object;

    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMImpl_QueryInterface(IDirect3DRM* iface, REFIID riid, void** ppvObject)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirect3DRM))
    {
        IUnknown_AddRef(iface);
        *ppvObject = This;
        return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DRMImpl_AddRef(IDirect3DRM* iface)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    TRACE("(%p/%p)\n", iface, This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IDirect3DRMImpl_Release(IDirect3DRM* iface)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)\n", iface, This);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IDirect3DRM methods ***/
static HRESULT WINAPI IDirect3DRMImpl_CreateObject(IDirect3DRM* iface, REFCLSID rclsid, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%s,%p,%s,%p): stub\n", iface, This, debugstr_guid(rclsid), pUnkOuter, debugstr_guid(riid), ppvObj);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateFrame(IDirect3DRM* iface, LPDIRECT3DRMFRAME pFrameParent, LPDIRECT3DRMFRAME * ppFrame)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, pFrameParent, ppFrame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateMesh(IDirect3DRM* iface, LPDIRECT3DRMMESH * ppMesh)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p): stub\n", iface, This, ppMesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateMeshBuilder(IDirect3DRM* iface, LPDIRECT3DRMMESHBUILDER * ppMeshBuilder)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", iface, This, ppMeshBuilder);

    return Direct3DRMMeshBuilder_create(ppMeshBuilder);
}

static HRESULT WINAPI IDirect3DRMImpl_CreateFace(IDirect3DRM* iface, LPDIRECT3DRMFACE * ppFace)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p): stub\n", iface, This, ppFace);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateAnimation(IDirect3DRM* iface, LPDIRECT3DRMANIMATION * ppAnimation)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p): stub\n", iface, This, ppAnimation);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateAnimationSet(IDirect3DRM* iface, LPDIRECT3DRMANIMATIONSET * ppAnimationSet)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p): stub\n", iface, This, ppAnimationSet);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateTexture(IDirect3DRM* iface, LPD3DRMIMAGE pImage, LPDIRECT3DRMTEXTURE * ppTexture)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, pImage, ppTexture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateLight(IDirect3DRM* iface, D3DRMLIGHTTYPE type, D3DCOLOR color, LPDIRECT3DRMLIGHT * ppLight)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%d,%d,%p): stub\n", iface, This, type, color, ppLight);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateLightRGB(IDirect3DRM* iface, D3DRMLIGHTTYPE type,  D3DVALUE red, D3DVALUE green, D3DVALUE blue, LPDIRECT3DRMLIGHT * ppLight)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%d,%f,%f,%f,%p): stub\n", iface, This, type, red, green, blue, ppLight);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_Material(IDirect3DRM* iface, D3DVALUE m, LPDIRECT3DRMMATERIAL * ppMaterial)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%f,%p): stub\n", iface, This, m, ppMaterial);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateDevice(IDirect3DRM* iface, DWORD width, DWORD height, LPDIRECT3DRMDEVICE * ppDevice)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%d,%d,%p): stub\n", iface, This, width, height, ppDevice);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateDeviceFromSurface(IDirect3DRM* iface, LPGUID pGUID, LPDIRECTDRAW pDD, LPDIRECTDRAWSURFACE pDDSBack, LPDIRECT3DRMDEVICE * ppDevice)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%s,%p,%p,%p): stub\n", iface, This, debugstr_guid(pGUID), pDD, pDDSBack, ppDevice);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateDeviceFromD3D(IDirect3DRM* iface, LPDIRECT3D pD3D, LPDIRECT3DDEVICE pD3DDev, LPDIRECT3DRMDEVICE * ppDevice)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p,%p,%p): stub\n", iface, This, pD3D, pD3DDev, ppDevice);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateDeviceFromClipper(IDirect3DRM* iface, LPDIRECTDRAWCLIPPER pDDClipper, LPGUID pGUID, int width, int height, LPDIRECT3DRMDEVICE * ppDevice)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p,%s,%d,%d,%p): stub\n", iface, This, pDDClipper, debugstr_guid(pGUID), width, height, ppDevice);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateShadow(IDirect3DRM* iface, LPDIRECT3DRMVISUAL pVisual, LPDIRECT3DRMLIGHT pLight, D3DVALUE px, D3DVALUE py, D3DVALUE pz, D3DVALUE nx, D3DVALUE ny, D3DVALUE nz, LPDIRECT3DRMVISUAL * ppVisual)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p,%p,%f,%f,%f,%f,%f,%f,%p): stub\n", iface, This, pVisual, pLight, px, py, pz, nx, ny, nz, ppVisual);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateTextureFromSurface(IDirect3DRM* iface, LPDIRECTDRAWSURFACE pDDS, LPDIRECT3DRMTEXTURE * ppTexture)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, pDDS, ppTexture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateViewport(IDirect3DRM* iface, LPDIRECT3DRMDEVICE pDevice, LPDIRECT3DRMFRAME pFrame, DWORD xpos, DWORD ypos, DWORD width, DWORD height, LPDIRECT3DRMVIEWPORT * ppViewport)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p,%p,%d,%d,%d,%d,%p): stub\n", iface, This, pDevice, pFrame, xpos, ypos, width, height, ppViewport);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateWrap(IDirect3DRM* iface, D3DRMWRAPTYPE type, LPDIRECT3DRMFRAME pFrame, D3DVALUE ox, D3DVALUE oy, D3DVALUE oz, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz, D3DVALUE ux, D3DVALUE uy, D3DVALUE uz, D3DVALUE ou, D3DVALUE ov, D3DVALUE su, D3DVALUE sv, LPDIRECT3DRMWRAP * ppWrap)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%d,%p,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%p): stub\n", iface, This, type, pFrame, ox, oy, oz, dx, dy, dz, ux, uy, uz, ou, ov, su, sv, ppWrap);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateUserVisual(IDirect3DRM* iface, D3DRMUSERVISUALCALLBACK cb, LPVOID pArg, LPDIRECT3DRMUSERVISUAL * ppUserVisual)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p,%p,%p): stub\n", iface, This, cb, pArg, ppUserVisual);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_LoadTexture(IDirect3DRM* iface, const char * filename, LPDIRECT3DRMTEXTURE * ppTexture)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%s,%p): stub\n", iface, This, filename, ppTexture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_LoadTextureFromResource(IDirect3DRM* iface, HRSRC rs, LPDIRECT3DRMTEXTURE * ppTexture)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, rs, ppTexture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_SetSearchPath(IDirect3DRM* iface, LPCSTR path)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%s): stub\n", iface, This, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_AddSearchPath(IDirect3DRM* iface, LPCSTR path)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%s): stub\n", iface, This, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_GetSearchPath(IDirect3DRM* iface, DWORD *size_return, LPSTR path_return)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p,%s): stub\n", iface, This, size_return, path_return);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_SetDefaultTextureColors(IDirect3DRM* iface, DWORD nb_colors)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%d): stub\n", iface, This, nb_colors);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_SetDefaultTextureShades(IDirect3DRM* iface, DWORD nb_shades)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%d): stub\n", iface, This, nb_shades);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_GetDevices(IDirect3DRM* iface, LPDIRECT3DRMDEVICEARRAY * ppDeviceArray)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p): stub\n", iface, This, ppDeviceArray);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_GetNamedObject(IDirect3DRM* iface, const char * pName, LPDIRECT3DRMOBJECT * ppObject)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%s,%p): stub\n", iface, This, pName, ppObject);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_EnumerateObjects(IDirect3DRM* iface, D3DRMOBJECTCALLBACK cb, LPVOID pArg)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, cb, pArg);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_Load(IDirect3DRM* iface, LPVOID pObjSource, LPVOID pObjID, LPIID * ppGUIDs, DWORD nb_GUIDs, D3DRMLOADOPTIONS LOFlags, D3DRMLOADCALLBACK LoadProc, LPVOID pArgLP, D3DRMLOADTEXTURECALLBACK LoadTextureProc, LPVOID pArgLTP, LPDIRECT3DRMFRAME pParentFrame)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%p,%p,%p,%d,%d,%p,%p,%p,%p,%p): stub\n", iface, This, pObjSource, pObjID, ppGUIDs, nb_GUIDs, LOFlags, LoadProc, pArgLP, LoadTextureProc, pArgLTP, pParentFrame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_Tick(IDirect3DRM* iface, D3DVALUE tick)
{
    IDirect3DRMImpl *This = (IDirect3DRMImpl *)iface;

    FIXME("(%p/%p)->(%f): stub\n", iface, This, tick);

    return E_NOTIMPL;
}

static const struct IDirect3DRMVtbl Direct3DRM_Vtbl =
{
    IDirect3DRMImpl_QueryInterface,
    IDirect3DRMImpl_AddRef,
    IDirect3DRMImpl_Release,
    IDirect3DRMImpl_CreateObject,
    IDirect3DRMImpl_CreateFrame,
    IDirect3DRMImpl_CreateMesh,
    IDirect3DRMImpl_CreateMeshBuilder,
    IDirect3DRMImpl_CreateFace,
    IDirect3DRMImpl_CreateAnimation,
    IDirect3DRMImpl_CreateAnimationSet,
    IDirect3DRMImpl_CreateTexture,
    IDirect3DRMImpl_CreateLight,
    IDirect3DRMImpl_CreateLightRGB,
    IDirect3DRMImpl_Material,
    IDirect3DRMImpl_CreateDevice,
    IDirect3DRMImpl_CreateDeviceFromSurface,
    IDirect3DRMImpl_CreateDeviceFromD3D,
    IDirect3DRMImpl_CreateDeviceFromClipper,
    IDirect3DRMImpl_CreateShadow,
    IDirect3DRMImpl_CreateTextureFromSurface,
    IDirect3DRMImpl_CreateViewport,
    IDirect3DRMImpl_CreateWrap,
    IDirect3DRMImpl_CreateUserVisual,
    IDirect3DRMImpl_LoadTexture,
    IDirect3DRMImpl_LoadTextureFromResource,
    IDirect3DRMImpl_SetSearchPath,
    IDirect3DRMImpl_AddSearchPath,
    IDirect3DRMImpl_GetSearchPath,
    IDirect3DRMImpl_SetDefaultTextureColors,
    IDirect3DRMImpl_SetDefaultTextureShades,
    IDirect3DRMImpl_GetDevices,
    IDirect3DRMImpl_GetNamedObject,
    IDirect3DRMImpl_EnumerateObjects,
    IDirect3DRMImpl_Load,
    IDirect3DRMImpl_Tick
};
