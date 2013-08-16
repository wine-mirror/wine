/*
 * Implementation of IDirect3DRM Interface
 *
 * Copyright 2010, 2012 Christian Costa
 * Copyright 2011 André Hentschel
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
#include "dxfile.h"
#include "rmxfguid.h"

#include "d3drm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3drm);

static const char* get_IID_string(const GUID* guid)
{
    if (IsEqualGUID(guid, &IID_IDirect3DRMFrame))
        return "IID_IDirect3DRMFrame";
    else if (IsEqualGUID(guid, &IID_IDirect3DRMFrame2))
        return "IID_IDirect3DRMFrame2";
    else if (IsEqualGUID(guid, &IID_IDirect3DRMFrame3))
        return "IID_IDirect3DRMFrame3";
    else if (IsEqualGUID(guid, &IID_IDirect3DRMMeshBuilder))
        return "IID_IDirect3DRMMeshBuilder";
    else if (IsEqualGUID(guid, &IID_IDirect3DRMMeshBuilder2))
        return "IID_IDirect3DRMMeshBuilder2";
    else if (IsEqualGUID(guid, &IID_IDirect3DRMMeshBuilder3))
        return "IID_IDirect3DRMMeshBuilder3";

    return "?";
}

typedef struct {
    IDirect3DRM IDirect3DRM_iface;
    IDirect3DRM2 IDirect3DRM2_iface;
    IDirect3DRM3 IDirect3DRM3_iface;
    LONG ref;
} IDirect3DRMImpl;

static inline IDirect3DRMImpl *impl_from_IDirect3DRM(IDirect3DRM *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMImpl, IDirect3DRM_iface);
}

static inline IDirect3DRMImpl *impl_from_IDirect3DRM2(IDirect3DRM2 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMImpl, IDirect3DRM2_iface);
}

static inline IDirect3DRMImpl *impl_from_IDirect3DRM3(IDirect3DRM3 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMImpl, IDirect3DRM3_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMImpl_QueryInterface(IDirect3DRM* iface, REFIID riid, void** ppvObject)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IDirect3DRM))
    {
        *ppvObject = &This->IDirect3DRM_iface;
    }
    else if(IsEqualGUID(riid, &IID_IDirect3DRM2))
    {
        *ppvObject = &This->IDirect3DRM2_iface;
    }
    else if(IsEqualGUID(riid, &IID_IDirect3DRM3))
    {
        *ppvObject = &This->IDirect3DRM3_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDirect3DRM_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI IDirect3DRMImpl_AddRef(IDirect3DRM* iface)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p)->(): new ref = %d\n", iface, This, ref);

    return ref;
}

static ULONG WINAPI IDirect3DRMImpl_Release(IDirect3DRM* iface)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)->(): new ref = %d\n", iface, This, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IDirect3DRM methods ***/
static HRESULT WINAPI IDirect3DRMImpl_CreateObject(IDirect3DRM* iface, REFCLSID rclsid, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);

    FIXME("(%p/%p)->(%s,%p,%s,%p): stub\n", iface, This, debugstr_guid(rclsid), pUnkOuter, debugstr_guid(riid), ppvObj);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateFrame(IDirect3DRM *iface,
        IDirect3DRMFrame *parent_frame, IDirect3DRMFrame **frame)
{
    TRACE("iface %p, parent_frame %p, frame %p.\n", iface, parent_frame, frame);

    return Direct3DRMFrame_create(&IID_IDirect3DRMFrame, (IUnknown *)parent_frame, (IUnknown **)frame);
}

static HRESULT WINAPI IDirect3DRMImpl_CreateMesh(IDirect3DRM *iface, IDirect3DRMMesh **mesh)
{
    IDirect3DRMImpl *d3drm = impl_from_IDirect3DRM(iface);

    TRACE("iface %p, mesh %p.\n", iface, mesh);

    return IDirect3DRM3_CreateMesh(&d3drm->IDirect3DRM3_iface, mesh);
}

static HRESULT WINAPI IDirect3DRMImpl_CreateMeshBuilder(IDirect3DRM *iface, IDirect3DRMMeshBuilder **mesh_builder)
{
    TRACE("iface %p, mesh_builder %p.\n", iface, mesh_builder);

    return Direct3DRMMeshBuilder_create(&IID_IDirect3DRMMeshBuilder, (IUnknown **)mesh_builder);
}

static HRESULT WINAPI IDirect3DRMImpl_CreateFace(IDirect3DRM* iface, IDirect3DRMFace **face)
{
    TRACE("iface %p, face %p.\n", iface, face);

    return Direct3DRMFace_create(&IID_IDirect3DRMFace, (IUnknown **)face);
}

static HRESULT WINAPI IDirect3DRMImpl_CreateAnimation(IDirect3DRM *iface, IDirect3DRMAnimation **animation)
{
    FIXME("iface %p, animation %p stub!\n", iface, animation);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateAnimationSet(IDirect3DRM *iface, IDirect3DRMAnimationSet **set)
{
    FIXME("iface %p, set %p stub!\n", iface, set);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateTexture(IDirect3DRM *iface,
        D3DRMIMAGE *image, IDirect3DRMTexture **texture)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);

    FIXME("(%p/%p)->(%p,%p): partial stub\n", iface, This, image, texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture, (IUnknown **)texture);
}

static HRESULT WINAPI IDirect3DRMImpl_CreateLight(IDirect3DRM *iface,
        D3DRMLIGHTTYPE type, D3DCOLOR color, IDirect3DRMLight **light)
{
    IDirect3DRMImpl *d3drm = impl_from_IDirect3DRM(iface);

    TRACE("iface %p, type %#x, color 0x%08x, light %p.\n", iface, type, color, light);

    return IDirect3DRM3_CreateLight(&d3drm->IDirect3DRM3_iface, type, color, light);
}

static HRESULT WINAPI IDirect3DRMImpl_CreateLightRGB(IDirect3DRM *iface, D3DRMLIGHTTYPE type,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue, IDirect3DRMLight **light)
{
    IDirect3DRMImpl *d3drm = impl_from_IDirect3DRM(iface);

    TRACE("iface %p, type %#x, red %.8e, green %.8e, blue %.8e, light %p.\n",
            iface, type, red, green, blue, light);

    return IDirect3DRM3_CreateLightRGB(&d3drm->IDirect3DRM3_iface, type, red, green, blue, light);
}

static HRESULT WINAPI IDirect3DRMImpl_CreateMaterial(IDirect3DRM *iface,
        D3DVALUE power, IDirect3DRMMaterial **material)
{
    IDirect3DRMImpl *d3drm = impl_from_IDirect3DRM(iface);

    TRACE("iface %p, power %.8e, material %p.\n", iface, power, material);

    return IDirect3DRM3_CreateMaterial(&d3drm->IDirect3DRM3_iface, power, (IDirect3DRMMaterial2 **)material);
}

static HRESULT WINAPI IDirect3DRMImpl_CreateDevice(IDirect3DRM *iface,
        DWORD width, DWORD height, IDirect3DRMDevice **device)
{
    FIXME("iface %p, width %u, height %u, device %p partial stub!\n", iface, width, height, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice, (IUnknown **)device);
}

static HRESULT WINAPI IDirect3DRMImpl_CreateDeviceFromSurface(IDirect3DRM *iface, GUID *pGUID,
        IDirectDraw *pDD, IDirectDrawSurface *pDDSBack, IDirect3DRMDevice **ppDevice)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);

    FIXME("(%p/%p)->(%s,%p,%p,%p): partial stub\n", iface, This, debugstr_guid(pGUID), pDD,
          pDDSBack, ppDevice);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice, (IUnknown**)ppDevice);
}

static HRESULT WINAPI IDirect3DRMImpl_CreateDeviceFromD3D(IDirect3DRM *iface,
        IDirect3D *pD3D, IDirect3DDevice *pD3DDev, IDirect3DRMDevice **ppDevice)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);

    FIXME("(%p/%p)->(%p,%p,%p): partial stub\n", iface, This, pD3D, pD3DDev, ppDevice);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice, (IUnknown**)ppDevice);
}

static HRESULT WINAPI IDirect3DRMImpl_CreateDeviceFromClipper(IDirect3DRM *iface,
        IDirectDrawClipper *pDDClipper, GUID *pGUID, int width, int height,
        IDirect3DRMDevice **ppDevice)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);

    FIXME("(%p/%p)->(%p,%s,%d,%d,%p): partial stub\n", iface, This, pDDClipper,
          debugstr_guid(pGUID), width, height, ppDevice);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice, (IUnknown**)ppDevice);
}

static HRESULT WINAPI IDirect3DRMImpl_CreateTextureFromSurface(IDirect3DRM *iface,
        IDirectDrawSurface *pDDS, IDirect3DRMTexture **ppTexture)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, pDDS, ppTexture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateShadow(IDirect3DRM *iface, IDirect3DRMVisual *visual,
        IDirect3DRMLight *light, D3DVALUE px, D3DVALUE py, D3DVALUE pz, D3DVALUE nx, D3DVALUE ny, D3DVALUE nz,
        IDirect3DRMVisual **shadow)
{
    FIXME("iface %p, visual %p, light %p, px %.8e, py %.8e, pz %.8e, nx %.8e, ny %.8e, nz %.8e, shadow %p stub!\n",
            iface, visual, light, px, py, pz, nx, ny, nz, shadow);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateViewport(IDirect3DRM *iface, IDirect3DRMDevice *device,
        IDirect3DRMFrame *camera, DWORD x, DWORD y, DWORD width, DWORD height, IDirect3DRMViewport **viewport)
{
    FIXME("iface %p, device %p, camera %p, x %u, y %u, width %u, height %u, viewport %p partial stub!\n",
            iface, device, camera, x, y, width, height, viewport);

    return Direct3DRMViewport_create(&IID_IDirect3DRMViewport, (IUnknown **)viewport);
}

static HRESULT WINAPI IDirect3DRMImpl_CreateWrap(IDirect3DRM *iface, D3DRMWRAPTYPE type, IDirect3DRMFrame *frame,
        D3DVALUE ox, D3DVALUE oy, D3DVALUE oz, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz, D3DVALUE ou, D3DVALUE ov, D3DVALUE su, D3DVALUE sv,
        IDirect3DRMWrap **wrap)
{
    FIXME("iface %p, type %#x, frame %p, ox %.8e, oy %.8e, oz %.8e, dx %.8e, dy %.8e, dz %.8e, "
            "ux %.8e, uy %.8e, uz %.8e, ou %.8e, ov %.8e, su %.8e, sv %.8e, wrap %p stub!\n",
            iface, type, frame, ox, oy, oz, dx, dy, dz, ux, uy, uz, ou, ov, su, sv, wrap);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_CreateUserVisual(IDirect3DRM *iface,
        D3DRMUSERVISUALCALLBACK cb, void *ctx, IDirect3DRMUserVisual **visual)
{
    FIXME("iface %p, cb %p, ctx %p visual %p stub!\n", iface, cb, ctx, visual);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_LoadTexture(IDirect3DRM *iface,
        const char *filename, IDirect3DRMTexture **texture)
{
    FIXME("iface %p, filename %s, texture %p stub!\n", iface, debugstr_a(filename), texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture, (IUnknown **)texture);
}

static HRESULT WINAPI IDirect3DRMImpl_LoadTextureFromResource(IDirect3DRM *iface,
        HRSRC resource, IDirect3DRMTexture **texture)
{
    FIXME("iface %p, resource %p, texture %p stub!\n", iface, resource, texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture, (IUnknown **)texture);
}

static HRESULT WINAPI IDirect3DRMImpl_SetSearchPath(IDirect3DRM* iface, LPCSTR path)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_AddSearchPath(IDirect3DRM* iface, LPCSTR path)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_GetSearchPath(IDirect3DRM* iface, DWORD *size_return, LPSTR path_return)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);

    FIXME("(%p/%p)->(%p,%s): stub\n", iface, This, size_return, path_return);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_SetDefaultTextureColors(IDirect3DRM* iface, DWORD nb_colors)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);

    FIXME("(%p/%p)->(%d): stub\n", iface, This, nb_colors);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_SetDefaultTextureShades(IDirect3DRM* iface, DWORD nb_shades)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);

    FIXME("(%p/%p)->(%d): stub\n", iface, This, nb_shades);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_GetDevices(IDirect3DRM *iface, IDirect3DRMDeviceArray **array)
{
    FIXME("iface %p, array %p stub!\n", iface, array);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_GetNamedObject(IDirect3DRM *iface,
        const char *name, IDirect3DRMObject **object)
{
    FIXME("iface %p, name %s, object %p stub!\n", iface, debugstr_a(name), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_EnumerateObjects(IDirect3DRM* iface, D3DRMOBJECTCALLBACK cb, LPVOID pArg)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, cb, pArg);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMImpl_Load(IDirect3DRM *iface, void *source, void *object_id, IID **iids,
        DWORD iid_count, D3DRMLOADOPTIONS flags, D3DRMLOADCALLBACK load_cb, void *load_ctx,
        D3DRMLOADTEXTURECALLBACK load_tex_cb, void *load_tex_ctx, IDirect3DRMFrame *parent_frame)
{
    IDirect3DRMImpl *d3drm = impl_from_IDirect3DRM(iface);
    IDirect3DRMFrame3 *parent_frame3 = NULL;
    HRESULT hr = D3DRM_OK;

    TRACE("iface %p, source %p, object_id %p, iids %p, iid_count %u, flags %#x, "
            "load_cb %p, load_ctx %p, load_tex_cb %p, load_tex_ctx %p, parent_frame %p.\n",
            iface, source, object_id, iids, iid_count, flags,
            load_cb, load_ctx, load_tex_cb, load_tex_ctx, parent_frame);

    if (parent_frame)
        hr = IDirect3DRMFrame_QueryInterface(parent_frame, &IID_IDirect3DRMFrame3, (void **)&parent_frame3);
    if (SUCCEEDED(hr))
        hr = IDirect3DRM3_Load(&d3drm->IDirect3DRM3_iface, source, object_id, iids, iid_count,
                flags, load_cb, load_ctx, load_tex_cb, load_tex_ctx, parent_frame3);
    if (parent_frame3)
        IDirect3DRMFrame3_Release(parent_frame3);

    return hr;
}

static HRESULT WINAPI IDirect3DRMImpl_Tick(IDirect3DRM* iface, D3DVALUE tick)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM(iface);

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
    IDirect3DRMImpl_CreateMaterial,
    IDirect3DRMImpl_CreateDevice,
    IDirect3DRMImpl_CreateDeviceFromSurface,
    IDirect3DRMImpl_CreateDeviceFromD3D,
    IDirect3DRMImpl_CreateDeviceFromClipper,
    IDirect3DRMImpl_CreateTextureFromSurface,
    IDirect3DRMImpl_CreateShadow,
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


/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRM2Impl_QueryInterface(IDirect3DRM2* iface, REFIID riid,
                                                      void** ppvObject)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);
    return IDirect3DRM_QueryInterface(&This->IDirect3DRM_iface, riid, ppvObject);
}

static ULONG WINAPI IDirect3DRM2Impl_AddRef(IDirect3DRM2* iface)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);
    return IDirect3DRM_AddRef(&This->IDirect3DRM_iface);
}

static ULONG WINAPI IDirect3DRM2Impl_Release(IDirect3DRM2* iface)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);
    return IDirect3DRM_Release(&This->IDirect3DRM_iface);
}

/*** IDirect3DRM2 methods ***/
static HRESULT WINAPI IDirect3DRM2Impl_CreateObject(IDirect3DRM2* iface, REFCLSID rclsid,
                                                    LPUNKNOWN pUnkOuter, REFIID riid,
                                                    LPVOID *ppvObj)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);

    FIXME("(%p/%p)->(%s,%p,%s,%p): stub\n", iface, This, debugstr_guid(rclsid), pUnkOuter,
                                            debugstr_guid(riid), ppvObj);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateFrame(IDirect3DRM2 *iface,
        IDirect3DRMFrame *parent_frame, IDirect3DRMFrame2 **frame)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);

    TRACE("(%p/%p)->(%p,%p)\n", iface, This, parent_frame, frame);

    return Direct3DRMFrame_create(&IID_IDirect3DRMFrame2, (IUnknown*)parent_frame, (IUnknown**)frame);
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateMesh(IDirect3DRM2 *iface, IDirect3DRMMesh **mesh)
{
    IDirect3DRMImpl *d3drm = impl_from_IDirect3DRM2(iface);

    TRACE("iface %p, mesh %p.\n", iface, mesh);

    return IDirect3DRM3_CreateMesh(&d3drm->IDirect3DRM3_iface, mesh);
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateMeshBuilder(IDirect3DRM2 *iface, IDirect3DRMMeshBuilder2 **mesh_builder)
{
    TRACE("iface %p, mesh_builder %p.\n", iface, mesh_builder);

    return Direct3DRMMeshBuilder_create(&IID_IDirect3DRMMeshBuilder2, (IUnknown **)mesh_builder);
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateFace(IDirect3DRM2 *iface, IDirect3DRMFace **face)
{
    TRACE("iface %p, face %p.\n", iface, face);

    return Direct3DRMFace_create(&IID_IDirect3DRMFace, (IUnknown **)face);
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateAnimation(IDirect3DRM2 *iface, IDirect3DRMAnimation **animation)
{
    FIXME("iface %p, animation %p stub!\n", iface, animation);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateAnimationSet(IDirect3DRM2 *iface, IDirect3DRMAnimationSet **set)
{
    FIXME("iface %p, set %p stub!\n", iface, set);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateTexture(IDirect3DRM2 *iface,
        D3DRMIMAGE *image, IDirect3DRMTexture2 **texture)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);

    FIXME("(%p/%p)->(%p,%p): partial stub\n", iface, This, image, texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture2, (IUnknown **)texture);
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateLight(IDirect3DRM2 *iface,
        D3DRMLIGHTTYPE type, D3DCOLOR color, IDirect3DRMLight **light)
{
    IDirect3DRMImpl *d3drm = impl_from_IDirect3DRM2(iface);

    TRACE("iface %p, type %#x, color 0x%08x, light %p.\n", iface, type, color, light);

    return IDirect3DRM3_CreateLight(&d3drm->IDirect3DRM3_iface, type, color, light);
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateLightRGB(IDirect3DRM2 *iface, D3DRMLIGHTTYPE type,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue, IDirect3DRMLight **light)
{
    IDirect3DRMImpl *d3drm = impl_from_IDirect3DRM2(iface);

    TRACE("iface %p, type %#x, red %.8e, green %.8e, blue %.8e, light %p.\n",
            iface, type, red, green, blue, light);

    return IDirect3DRM3_CreateLightRGB(&d3drm->IDirect3DRM3_iface, type, red, green, blue, light);
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateMaterial(IDirect3DRM2 *iface,
        D3DVALUE power, IDirect3DRMMaterial **material)
{
    IDirect3DRMImpl *d3drm = impl_from_IDirect3DRM2(iface);

    TRACE("iface %p, power %.8e, material %p.\n", iface, power, material);

    return IDirect3DRM3_CreateMaterial(&d3drm->IDirect3DRM3_iface, power, (IDirect3DRMMaterial2 **)material);
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateDevice(IDirect3DRM2 *iface,
        DWORD width, DWORD height, IDirect3DRMDevice2 **device)
{
    FIXME("iface %p, width %u, height %u, device %p.\n", iface, width, height, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice2, (IUnknown **)device);
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateDeviceFromSurface(IDirect3DRM2 *iface, GUID *pGUID,
        IDirectDraw *pDD, IDirectDrawSurface *pDDSBack, IDirect3DRMDevice2 **ppDevice)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);

    FIXME("(%p/%p)->(%s,%p,%p,%p): partial stub\n", iface, This, debugstr_guid(pGUID),
          pDD, pDDSBack, ppDevice);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice2, (IUnknown**)ppDevice);
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateDeviceFromD3D(IDirect3DRM2 *iface,
        IDirect3D2 *pD3D, IDirect3DDevice2 *pD3DDev, IDirect3DRMDevice2 **ppDevice)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);

    FIXME("(%p/%p)->(%p,%p,%p): partial stub\n", iface, This, pD3D, pD3DDev, ppDevice);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice2, (IUnknown**)ppDevice);
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateDeviceFromClipper(IDirect3DRM2 *iface,
        IDirectDrawClipper *pDDClipper, GUID *pGUID, int width, int height,
        IDirect3DRMDevice2 **ppDevice)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);

    FIXME("(%p/%p)->(%p,%s,%d,%d,%p): partial stub\n", iface, This, pDDClipper,
          debugstr_guid(pGUID), width, height, ppDevice);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice2, (IUnknown**)ppDevice);
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateTextureFromSurface(IDirect3DRM2 *iface,
        IDirectDrawSurface *pDDS, IDirect3DRMTexture2 **ppTexture)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, pDDS, ppTexture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateShadow(IDirect3DRM2 *iface, IDirect3DRMVisual *visual,
        IDirect3DRMLight *light, D3DVALUE px, D3DVALUE py, D3DVALUE pz, D3DVALUE nx, D3DVALUE ny, D3DVALUE nz,
        IDirect3DRMVisual **shadow)
{
    FIXME("iface %p, visual %p, light %p, px %.8e, py %.8e, pz %.8e, nx %.8e, ny %.8e, nz %.8e, shadow %p stub!\n",
            iface, visual, light, px, py, pz, nx, ny, nz, shadow);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateViewport(IDirect3DRM2 *iface, IDirect3DRMDevice *device,
        IDirect3DRMFrame *camera, DWORD x, DWORD y, DWORD width, DWORD height, IDirect3DRMViewport **viewport)
{
    FIXME("iface %p, device %p, camera %p, x %u, y %u, width %u, height %u, viewport %p partial stub!\n",
            iface, device, camera, x, y, width, height, viewport);

    return Direct3DRMViewport_create(&IID_IDirect3DRMViewport, (IUnknown **)viewport);
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateWrap(IDirect3DRM2 *iface, D3DRMWRAPTYPE type, IDirect3DRMFrame *frame,
        D3DVALUE ox, D3DVALUE oy, D3DVALUE oz, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz, D3DVALUE ou, D3DVALUE ov, D3DVALUE su, D3DVALUE sv,
        IDirect3DRMWrap **wrap)
{
    FIXME("iface %p, type %#x, frame %p, ox %.8e, oy %.8e, oz %.8e, dx %.8e, dy %.8e, dz %.8e, "
            "ux %.8e, uy %.8e, uz %.8e, ou %.8e, ov %.8e, su %.8e, sv %.8e, wrap %p stub!\n",
            iface, type, frame, ox, oy, oz, dx, dy, dz, ux, uy, uz, ou, ov, su, sv, wrap);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateUserVisual(IDirect3DRM2 *iface,
        D3DRMUSERVISUALCALLBACK cb, void *ctx, IDirect3DRMUserVisual **visual)
{
    FIXME("iface %p, cb %p, ctx %p, visual %p stub!\n", iface, cb, ctx, visual);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_LoadTexture(IDirect3DRM2 *iface,
        const char *filename, IDirect3DRMTexture2 **texture)
{
    FIXME("iface %p, filename %s, texture %p stub!\n", iface, debugstr_a(filename), texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture2, (IUnknown **)texture);
}

static HRESULT WINAPI IDirect3DRM2Impl_LoadTextureFromResource(IDirect3DRM2 *iface, HMODULE module,
        const char *resource_name, const char *resource_type, IDirect3DRMTexture2 **texture)
{
    FIXME("iface %p, resource_name %s, resource_type %s, texture %p stub!\n",
            iface, debugstr_a(resource_name), debugstr_a(resource_type), texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture2, (IUnknown **)texture);
}

static HRESULT WINAPI IDirect3DRM2Impl_SetSearchPath(IDirect3DRM2* iface, LPCSTR path)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_AddSearchPath(IDirect3DRM2* iface, LPCSTR path)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_GetSearchPath(IDirect3DRM2* iface, DWORD *size_return,
                                                     LPSTR path_return)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);

    FIXME("(%p/%p)->(%p,%s): stub\n", iface, This, size_return, path_return);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_SetDefaultTextureColors(IDirect3DRM2* iface, DWORD nb_colors)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);

    FIXME("(%p/%p)->(%d): stub\n", iface, This, nb_colors);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_SetDefaultTextureShades(IDirect3DRM2* iface, DWORD nb_shades)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);

    FIXME("(%p/%p)->(%d): stub\n", iface, This, nb_shades);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_GetDevices(IDirect3DRM2 *iface, IDirect3DRMDeviceArray **array)
{
    FIXME("iface %p, array %p stub!\n", iface, array);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_GetNamedObject(IDirect3DRM2 *iface,
        const char *name, IDirect3DRMObject **object)
{
    FIXME("iface %p, name %s, object %p stub!\n", iface, debugstr_a(name), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_EnumerateObjects(IDirect3DRM2* iface, D3DRMOBJECTCALLBACK cb,
                                                        LPVOID pArg)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, cb, pArg);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_Load(IDirect3DRM2 *iface, void *source, void *object_id, IID **iids,
        DWORD iid_count, D3DRMLOADOPTIONS flags, D3DRMLOADCALLBACK load_cb, void *load_ctx,
        D3DRMLOADTEXTURECALLBACK load_tex_cb, void *load_tex_ctx, IDirect3DRMFrame *parent_frame)
{
    IDirect3DRMImpl *d3drm = impl_from_IDirect3DRM2(iface);
    IDirect3DRMFrame3 *parent_frame3 = NULL;
    HRESULT hr = D3DRM_OK;

    TRACE("iface %p, source %p, object_id %p, iids %p, iid_count %u, flags %#x, "
            "load_cb %p, load_ctx %p, load_tex_cb %p, load_tex_ctx %p, parent_frame %p.\n",
            iface, source, object_id, iids, iid_count, flags,
            load_cb, load_ctx, load_tex_cb, load_tex_ctx, parent_frame);

    if (parent_frame)
        hr = IDirect3DRMFrame_QueryInterface(parent_frame, &IID_IDirect3DRMFrame3, (void **)&parent_frame3);
    if (SUCCEEDED(hr))
        hr = IDirect3DRM3_Load(&d3drm->IDirect3DRM3_iface, source, object_id, iids, iid_count,
                flags, load_cb, load_ctx, load_tex_cb, load_tex_ctx, parent_frame3);
    if (parent_frame3)
        IDirect3DRMFrame3_Release(parent_frame3);

    return hr;
}

static HRESULT WINAPI IDirect3DRM2Impl_Tick(IDirect3DRM2* iface, D3DVALUE tick)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM2(iface);

    FIXME("(%p/%p)->(%f): stub\n", iface, This, tick);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM2Impl_CreateProgressiveMesh(IDirect3DRM2 *iface, IDirect3DRMProgressiveMesh **mesh)
{
    FIXME("iface %p, mesh %p stub!\n", iface, mesh);

    return E_NOTIMPL;
}

static const struct IDirect3DRM2Vtbl Direct3DRM2_Vtbl =
{
    IDirect3DRM2Impl_QueryInterface,
    IDirect3DRM2Impl_AddRef,
    IDirect3DRM2Impl_Release,
    IDirect3DRM2Impl_CreateObject,
    IDirect3DRM2Impl_CreateFrame,
    IDirect3DRM2Impl_CreateMesh,
    IDirect3DRM2Impl_CreateMeshBuilder,
    IDirect3DRM2Impl_CreateFace,
    IDirect3DRM2Impl_CreateAnimation,
    IDirect3DRM2Impl_CreateAnimationSet,
    IDirect3DRM2Impl_CreateTexture,
    IDirect3DRM2Impl_CreateLight,
    IDirect3DRM2Impl_CreateLightRGB,
    IDirect3DRM2Impl_CreateMaterial,
    IDirect3DRM2Impl_CreateDevice,
    IDirect3DRM2Impl_CreateDeviceFromSurface,
    IDirect3DRM2Impl_CreateDeviceFromD3D,
    IDirect3DRM2Impl_CreateDeviceFromClipper,
    IDirect3DRM2Impl_CreateTextureFromSurface,
    IDirect3DRM2Impl_CreateShadow,
    IDirect3DRM2Impl_CreateViewport,
    IDirect3DRM2Impl_CreateWrap,
    IDirect3DRM2Impl_CreateUserVisual,
    IDirect3DRM2Impl_LoadTexture,
    IDirect3DRM2Impl_LoadTextureFromResource,
    IDirect3DRM2Impl_SetSearchPath,
    IDirect3DRM2Impl_AddSearchPath,
    IDirect3DRM2Impl_GetSearchPath,
    IDirect3DRM2Impl_SetDefaultTextureColors,
    IDirect3DRM2Impl_SetDefaultTextureShades,
    IDirect3DRM2Impl_GetDevices,
    IDirect3DRM2Impl_GetNamedObject,
    IDirect3DRM2Impl_EnumerateObjects,
    IDirect3DRM2Impl_Load,
    IDirect3DRM2Impl_Tick,
    IDirect3DRM2Impl_CreateProgressiveMesh
};


/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRM3Impl_QueryInterface(IDirect3DRM3* iface, REFIID riid,
                                                      void** ppvObject)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);
    return IDirect3DRM_QueryInterface(&This->IDirect3DRM_iface, riid, ppvObject);
}

static ULONG WINAPI IDirect3DRM3Impl_AddRef(IDirect3DRM3* iface)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);
    return IDirect3DRM_AddRef(&This->IDirect3DRM_iface);
}

static ULONG WINAPI IDirect3DRM3Impl_Release(IDirect3DRM3* iface)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);
    return IDirect3DRM_Release(&This->IDirect3DRM_iface);
}

/*** IDirect3DRM3 methods ***/
static HRESULT WINAPI IDirect3DRM3Impl_CreateObject(IDirect3DRM3* iface, REFCLSID rclsid,
                                                    LPUNKNOWN unkwn, REFIID riid, LPVOID* object)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%s,%p,%s,%p): stub\n", iface, This, debugstr_guid(rclsid), unkwn,
          debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateFrame(IDirect3DRM3 *iface,
        IDirect3DRMFrame3 *parent, IDirect3DRMFrame3 **frame)
{
    TRACE("iface %p, parent %p, frame %p.\n", iface, parent, frame);

    return Direct3DRMFrame_create(&IID_IDirect3DRMFrame3, (IUnknown *)parent, (IUnknown **)frame);
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateMesh(IDirect3DRM3 *iface, IDirect3DRMMesh **mesh)
{
    TRACE("iface %p, mesh %p.\n", iface, mesh);

    return Direct3DRMMesh_create(mesh);
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateMeshBuilder(IDirect3DRM3 *iface, IDirect3DRMMeshBuilder3 **mesh_builder)
{
    TRACE("iface %p, mesh_builder %p.\n", iface, mesh_builder);

    return Direct3DRMMeshBuilder_create(&IID_IDirect3DRMMeshBuilder3, (IUnknown **)mesh_builder);
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateFace(IDirect3DRM3 *iface, IDirect3DRMFace2 **face)
{
    TRACE("iface %p, face %p.\n", iface, face);

    return Direct3DRMFace_create(&IID_IDirect3DRMFace2, (IUnknown **)face);
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateAnimation(IDirect3DRM3 *iface, IDirect3DRMAnimation2 **animation)
{
    FIXME("iface %p, animation %p stub!\n", iface, animation);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateAnimationSet(IDirect3DRM3 *iface, IDirect3DRMAnimationSet2 **set)
{
    FIXME("iface %p, set %p stub!\n", iface, set);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateTexture(IDirect3DRM3 *iface,
        D3DRMIMAGE *image, IDirect3DRMTexture3 **texture)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%p,%p): partial stub\n", iface, This, image, texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture3, (IUnknown **)texture);
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateLight(IDirect3DRM3 *iface,
        D3DRMLIGHTTYPE type, D3DCOLOR color, IDirect3DRMLight **light)
{
    HRESULT hr;

    FIXME("iface %p, type %#x, color 0x%08x, light %p partial stub!\n", iface, type, color, light);

    if (SUCCEEDED(hr = Direct3DRMLight_create((IUnknown **)light)))
    {
        IDirect3DRMLight_SetType(*light, type);
        IDirect3DRMLight_SetColor(*light, color);
    }

    return hr;
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateLightRGB(IDirect3DRM3 *iface, D3DRMLIGHTTYPE type,
        D3DVALUE red, D3DVALUE green, D3DVALUE blue, IDirect3DRMLight **light)
{
    HRESULT hr;

    FIXME("iface %p, type %#x, red %.8e, green %.8e, blue %.8e, light %p partial stub!\n",
            iface, type, red, green, blue, light);

    if (SUCCEEDED(hr = Direct3DRMLight_create((IUnknown **)light)))
    {
        IDirect3DRMLight_SetType(*light, type);
        IDirect3DRMLight_SetColorRGB(*light, red, green, blue);
    }

    return hr;
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateMaterial(IDirect3DRM3 *iface,
        D3DVALUE power, IDirect3DRMMaterial2 **material)
{
    HRESULT hr;

    TRACE("iface %p, power %.8e, material %p.\n", iface, power, material);

    if (SUCCEEDED(hr = Direct3DRMMaterial_create(material)))
        IDirect3DRMMaterial2_SetPower(*material, power);

    return hr;
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateDevice(IDirect3DRM3 *iface,
        DWORD width, DWORD height, IDirect3DRMDevice3 **device)
{
    FIXME("iface %p, width %u, height %u, device %p partial stub!\n", iface, width, height, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice3, (IUnknown **)device);
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateDeviceFromSurface(IDirect3DRM3 *iface, GUID *pGUID,
        IDirectDraw *dd, IDirectDrawSurface *back, IDirect3DRMDevice3 **device)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%s,%p,%p,%p): partial stub\n", iface, This, debugstr_guid(pGUID), dd, back, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice3, (IUnknown**)device);
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateDeviceFromD3D(IDirect3DRM3 *iface,
        IDirect3D2 *d3d, IDirect3DDevice2 *d3ddev, IDirect3DRMDevice3 **device)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%p,%p,%p): partial stub\n", iface, This, d3d, d3ddev, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice3, (IUnknown**)device);
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateDeviceFromClipper(IDirect3DRM3 *iface,
        IDirectDrawClipper *clipper, GUID *guid, int width, int height,
        IDirect3DRMDevice3 **device)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%p,%s,%d,%d,%p): partial stub\n", iface, This, clipper, debugstr_guid(guid),
          width, height, device);

    return Direct3DRMDevice_create(&IID_IDirect3DRMDevice3, (IUnknown**)device);
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateShadow(IDirect3DRM3 *iface, IUnknown *object, IDirect3DRMLight *light,
        D3DVALUE px, D3DVALUE py, D3DVALUE pz, D3DVALUE nx, D3DVALUE ny, D3DVALUE nz, IDirect3DRMShadow2 **shadow)
{
    FIXME("iface %p, object %p, light %p, px %.8e, py %.8e, pz %.8e, nx %.8e, ny %.8e, nz %.8e, shadow %p stub!\n",
            iface, object, light, px, py, pz, nx, ny, nz, shadow);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateTextureFromSurface(IDirect3DRM3 *iface,
        IDirectDrawSurface *surface, IDirect3DRMTexture3 **texture)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, surface, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateViewport(IDirect3DRM3 *iface, IDirect3DRMDevice3 *device,
        IDirect3DRMFrame3 *camera, DWORD x, DWORD y, DWORD width, DWORD height, IDirect3DRMViewport2 **viewport)
{
    FIXME("iface %p, device %p, camera %p, x %u, y %u, width %u, height %u, viewport %p partial stub!\n",
            iface, device, camera, x, y, width, height, viewport);

    return Direct3DRMViewport_create(&IID_IDirect3DRMViewport2, (IUnknown **)viewport);
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateWrap(IDirect3DRM3 *iface, D3DRMWRAPTYPE type, IDirect3DRMFrame3 *frame,
        D3DVALUE ox, D3DVALUE oy, D3DVALUE oz, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz, D3DVALUE ou, D3DVALUE ov, D3DVALUE su, D3DVALUE sv,
        IDirect3DRMWrap **wrap)
{
    FIXME("iface %p, type %#x, frame %p, ox %.8e, oy %.8e, oz %.8e, dx %.8e, dy %.8e, dz %.8e, "
            "ux %.8e, uy %.8e, uz %.8e, ou %.8e, ov %.8e, su %.8e, sv %.8e, wrap %p stub!\n",
            iface, type, frame, ox, oy, oz, dx, dy, dz, ux, uy, uz, ou, ov, su, sv, wrap);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateUserVisual(IDirect3DRM3 *iface,
        D3DRMUSERVISUALCALLBACK cb, void *ctx, IDirect3DRMUserVisual **visual)
{
    FIXME("iface %p, cb %p, ctx %p, visual %p stub!\n", iface, cb, ctx, visual);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_LoadTexture(IDirect3DRM3 *iface,
        const char *filename, IDirect3DRMTexture3 **texture)
{
    FIXME("iface %p, filename %s, texture %p stub!\n", iface, debugstr_a(filename), texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture3, (IUnknown **)texture);
}

static HRESULT WINAPI IDirect3DRM3Impl_LoadTextureFromResource(IDirect3DRM3 *iface, HMODULE module,
        const char *resource_name, const char *resource_type, IDirect3DRMTexture3 **texture)
{
    FIXME("iface %p, module %p, resource_name %s, resource_type %s, texture %p stub!\n",
            iface, module, debugstr_a(resource_name), debugstr_a(resource_type), texture);

    return Direct3DRMTexture_create(&IID_IDirect3DRMTexture3, (IUnknown **)texture);
}

static HRESULT WINAPI IDirect3DRM3Impl_SetSearchPath(IDirect3DRM3* iface, LPCSTR path)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_AddSearchPath(IDirect3DRM3* iface, LPCSTR path)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_GetSearchPath(IDirect3DRM3* iface, DWORD* size_return,
                                                     LPSTR path_return)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%p,%s): stub\n", iface, This, size_return, path_return);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_SetDefaultTextureColors(IDirect3DRM3* iface, DWORD nb_colors)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%d): stub\n", iface, This, nb_colors);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_SetDefaultTextureShades(IDirect3DRM3* iface, DWORD nb_shades)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%d): stub\n", iface, This, nb_shades);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_GetDevices(IDirect3DRM3 *iface, IDirect3DRMDeviceArray **array)
{
    FIXME("iface %p, array %p stub!\n", iface, array);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_GetNamedObject(IDirect3DRM3 *iface,
        const char *name, IDirect3DRMObject **object)
{
    FIXME("iface %p, name %s, object %p stub!\n", iface, debugstr_a(name), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_EnumerateObjects(IDirect3DRM3* iface, D3DRMOBJECTCALLBACK cb,
                                                        LPVOID arg)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, cb, arg);

    return E_NOTIMPL;
}

static HRESULT load_data(IDirect3DRM3 *iface, IDirectXFileData *data_object, IID **GUIDs, DWORD nb_GUIDs, D3DRMLOADCALLBACK LoadProc,
                         void *ArgLP, D3DRMLOADTEXTURECALLBACK LoadTextureProc, void *ArgLTP, IDirect3DRMFrame3 *parent_frame)
{
    HRESULT ret = D3DRMERR_BADOBJECT;
    HRESULT hr;
    const GUID* guid;
    DWORD i;
    BOOL requested = FALSE;

    hr = IDirectXFileData_GetType(data_object, &guid);
    if (hr != DXFILE_OK)
        goto end;

    TRACE("Found object type whose GUID = %s\n", debugstr_guid(guid));

    /* Load object only if it is top level and requested or if it is part of another object */

    if (IsEqualGUID(guid, &TID_D3DRMMesh))
    {
        TRACE("Found TID_D3DRMMesh\n");

        for (i = 0; i < nb_GUIDs; i++)
            if (IsEqualGUID(GUIDs[i], &IID_IDirect3DRMMeshBuilder) ||
                IsEqualGUID(GUIDs[i], &IID_IDirect3DRMMeshBuilder2) ||
                IsEqualGUID(GUIDs[i], &IID_IDirect3DRMMeshBuilder3))
            {
                requested = TRUE;
                break;
            }

        if (requested || parent_frame)
        {
            IDirect3DRMMeshBuilder3 *meshbuilder;

            TRACE("Load mesh data\n");

            hr = IDirect3DRM3_CreateMeshBuilder(iface, &meshbuilder);
            if (SUCCEEDED(hr))
            {
                hr = load_mesh_data(meshbuilder, data_object, LoadTextureProc, ArgLTP);
                if (SUCCEEDED(hr))
                {
                    /* Only top level objects are notified */
                    if (!parent_frame)
                    {
                        IDirect3DRMObject *object;

                        hr = IDirect3DRMMeshBuilder3_QueryInterface(meshbuilder, GUIDs[i], (void**)&object);
                        if (SUCCEEDED(hr))
                        {
                            LoadProc(object, GUIDs[i], ArgLP);
                            IDirect3DRMObject_Release(object);
                        }
                    }
                    else
                    {
                        IDirect3DRMFrame3_AddVisual(parent_frame, (IUnknown*)meshbuilder);
                    }
                }
                IDirect3DRMMeshBuilder3_Release(meshbuilder);
            }

            if (FAILED(hr))
                ERR("Cannot process mesh\n");
        }
    }
    else if (IsEqualGUID(guid, &TID_D3DRMFrame))
    {
        TRACE("Found TID_D3DRMFrame\n");

        for (i = 0; i < nb_GUIDs; i++)
            if (IsEqualGUID(GUIDs[i], &IID_IDirect3DRMFrame) ||
                IsEqualGUID(GUIDs[i], &IID_IDirect3DRMFrame2) ||
                IsEqualGUID(GUIDs[i], &IID_IDirect3DRMFrame3))
            {
                requested = TRUE;
                break;
            }

        if (requested || parent_frame)
        {
            IDirect3DRMFrame3 *frame;

            TRACE("Load frame data\n");

            hr = IDirect3DRM3_CreateFrame(iface, parent_frame, &frame);
            if (SUCCEEDED(hr))
            {
                IDirectXFileObject *child;

                while (SUCCEEDED(hr = IDirectXFileData_GetNextObject(data_object, &child)))
                {
                    IDirectXFileData *data;
                    IDirectXFileDataReference *reference;
                    IDirectXFileBinary *binary;

                    hr = IDirectXFileObject_QueryInterface(child, &IID_IDirectXFileBinary, (void **)&binary);
                    if (SUCCEEDED(hr))
                    {
                        FIXME("Binary Object not supported yet\n");
                        IDirectXFileBinary_Release(binary);
                        continue;
                    }

                    hr = IDirectXFileObject_QueryInterface(child, &IID_IDirectXFileData, (void **)&data);
                    if (SUCCEEDED(hr))
                    {
                        TRACE("Found Data Object\n");
                        hr = load_data(iface, data, GUIDs, nb_GUIDs, LoadProc, ArgLP, LoadTextureProc, ArgLTP, frame);
                        IDirectXFileData_Release(data);
                        continue;
                    }
                    hr = IDirectXFileObject_QueryInterface(child, &IID_IDirectXFileDataReference, (void **)&reference);
                    if (SUCCEEDED(hr))
                    {
                        TRACE("Found Data Object Reference\n");
                        IDirectXFileDataReference_Resolve(reference, &data);
                        hr = load_data(iface, data, GUIDs, nb_GUIDs, LoadProc, ArgLP, LoadTextureProc, ArgLTP, frame);
                        IDirectXFileData_Release(data);
                        IDirectXFileDataReference_Release(reference);
                        continue;
                    }
                }

                if (hr != DXFILEERR_NOMOREOBJECTS)
                {
                    IDirect3DRMFrame3_Release(frame);
                    goto end;
                }
                hr = S_OK;

                /* Only top level objects are notified */
                if (!parent_frame)
                {
                    IDirect3DRMObject *object;

                    hr = IDirect3DRMFrame3_QueryInterface(frame, GUIDs[i], (void**)&object);
                    if (SUCCEEDED(hr))
                    {
                        LoadProc(object, GUIDs[i], ArgLP);
                        IDirect3DRMObject_Release(object);
                    }
                }
                IDirect3DRMFrame3_Release(frame);
            }

            if (FAILED(hr))
                ERR("Cannot process frame\n");
        }
    }
    else if (IsEqualGUID(guid, &TID_D3DRMMaterial))
    {
        TRACE("Found TID_D3DRMMaterial\n");

        /* Cannot be requested so nothing to do */
    }
    else if (IsEqualGUID(guid, &TID_D3DRMFrameTransformMatrix))
    {
        TRACE("Found TID_D3DRMFrameTransformMatrix\n");

        /* Cannot be requested */
        if (parent_frame)
        {
            D3DRMMATRIX4D matrix;
            DWORD size;

            TRACE("Load Frame Transform Matrix data\n");

            size = sizeof(matrix);
            hr = IDirectXFileData_GetData(data_object, NULL, &size, (void**)matrix);
            if ((hr != DXFILE_OK) || (size != sizeof(matrix)))
                goto end;

            hr = IDirect3DRMFrame3_AddTransform(parent_frame, D3DRMCOMBINE_REPLACE, matrix);
            if (FAILED(hr))
                goto end;
        }
    }
    else
    {
        FIXME("Found unknown TID %s\n", debugstr_guid(guid));
    }

    ret = D3DRM_OK;

end:

    return ret;
}

static HRESULT WINAPI IDirect3DRM3Impl_Load(IDirect3DRM3 *iface, void *source, void *object_id, IID **iids,
        DWORD iid_count, D3DRMLOADOPTIONS flags, D3DRMLOADCALLBACK load_cb, void *load_ctx,
        D3DRMLOADTEXTURECALLBACK load_tex_cb, void *load_tex_ctx, IDirect3DRMFrame3 *parent_frame)
{
    DXFILELOADOPTIONS load_options;
    LPDIRECTXFILE pDXFile = NULL;
    LPDIRECTXFILEENUMOBJECT pEnumObject = NULL;
    LPDIRECTXFILEDATA pData = NULL;
    HRESULT hr;
    const GUID* pGuid;
    DWORD size;
    Header* pHeader;
    HRESULT ret = D3DRMERR_BADOBJECT;
    DWORD i;

    TRACE("iface %p, source %p, object_id %p, iids %p, iid_count %u, flags %#x, "
            "load_cb %p, load_ctx %p, load_tex_cb %p, load_tex_ctx %p, parent_frame %p.\n",
            iface, source, object_id, iids, iid_count, flags,
            load_cb, load_ctx, load_tex_cb, load_tex_ctx, parent_frame);

    TRACE("Looking for GUIDs:\n");
    for (i = 0; i < iid_count; ++i)
        TRACE("- %s (%s)\n", debugstr_guid(iids[i]), get_IID_string(iids[i]));

    if (flags == D3DRMLOAD_FROMMEMORY)
    {
        load_options = DXFILELOAD_FROMMEMORY;
    }
    else if (flags == D3DRMLOAD_FROMFILE)
    {
        load_options = DXFILELOAD_FROMFILE;
        TRACE("Loading from file %s\n", debugstr_a(source));
    }
    else
    {
        FIXME("Load options %#x not supported yet.\n", flags);
        return E_NOTIMPL;
    }

    hr = DirectXFileCreate(&pDXFile);
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFile_RegisterTemplates(pDXFile, templates, strlen(templates));
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFile_CreateEnumObject(pDXFile, source, load_options, &pEnumObject);
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFileEnumObject_GetNextDataObject(pEnumObject, &pData);
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFileData_GetType(pData, &pGuid);
    if (hr != DXFILE_OK)
        goto end;

    TRACE("Found object type whose GUID = %s\n", debugstr_guid(pGuid));

    if (!IsEqualGUID(pGuid, &TID_DXFILEHeader))
    {
        ret = D3DRMERR_BADFILE;
        goto end;
    }

    hr = IDirectXFileData_GetData(pData, NULL, &size, (void**)&pHeader);
    if ((hr != DXFILE_OK) || (size != sizeof(Header)))
        goto end;

    TRACE("Version is %d %d %d\n", pHeader->major, pHeader->minor, pHeader->flags);

    /* Version must be 1.0.x */
    if ((pHeader->major != 1) || (pHeader->minor != 0))
    {
        ret = D3DRMERR_BADFILE;
        goto end;
    }

    IDirectXFileData_Release(pData);
    pData = NULL;

    while (1)
    {
        hr = IDirectXFileEnumObject_GetNextDataObject(pEnumObject, &pData);
        if (hr == DXFILEERR_NOMOREOBJECTS)
        {
            TRACE("No more object\n");
            break;
        }
        else if (hr != DXFILE_OK)
        {
            ret = D3DRMERR_BADFILE;
            goto end;
        }

        ret = load_data(iface, pData, iids, iid_count, load_cb, load_ctx, load_tex_cb, load_tex_ctx, parent_frame);
        if (ret != D3DRM_OK)
            goto end;

        IDirectXFileData_Release(pData);
        pData = NULL;
    }

    ret = D3DRM_OK;

end:
    if (pData)
        IDirectXFileData_Release(pData);
    if (pEnumObject)
        IDirectXFileEnumObject_Release(pEnumObject);
    if (pDXFile)
        IDirectXFile_Release(pDXFile);

    return ret;
}

static HRESULT WINAPI IDirect3DRM3Impl_Tick(IDirect3DRM3* iface, D3DVALUE tick)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%f): stub\n", iface, This, tick);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateProgressiveMesh(IDirect3DRM3 *iface, IDirect3DRMProgressiveMesh **mesh)
{
    FIXME("iface %p, mesh %p stub!\n", iface, mesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_RegisterClient(IDirect3DRM3* iface, REFGUID rguid,
                                                      LPDWORD id)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%s, %p): stub\n", iface, This, debugstr_guid(rguid), id);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_UnregisterClient(IDirect3DRM3* iface, REFGUID rguid)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, debugstr_guid(rguid));

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_CreateClippedVisual(IDirect3DRM3 *iface,
        IDirect3DRMVisual *visual, IDirect3DRMClippedVisual **clipped_visual)
{
    FIXME("iface %p, visual %p, clipped_visual %p stub!\n", iface, visual, clipped_visual);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_SetOptions(IDirect3DRM3* iface, DWORD opt)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%d): stub\n", iface, This, opt);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRM3Impl_GetOptions(IDirect3DRM3* iface, LPDWORD opt)
{
    IDirect3DRMImpl *This = impl_from_IDirect3DRM3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, opt);

    return E_NOTIMPL;
}

static const struct IDirect3DRM3Vtbl Direct3DRM3_Vtbl =
{
    IDirect3DRM3Impl_QueryInterface,
    IDirect3DRM3Impl_AddRef,
    IDirect3DRM3Impl_Release,
    IDirect3DRM3Impl_CreateObject,
    IDirect3DRM3Impl_CreateFrame,
    IDirect3DRM3Impl_CreateMesh,
    IDirect3DRM3Impl_CreateMeshBuilder,
    IDirect3DRM3Impl_CreateFace,
    IDirect3DRM3Impl_CreateAnimation,
    IDirect3DRM3Impl_CreateAnimationSet,
    IDirect3DRM3Impl_CreateTexture,
    IDirect3DRM3Impl_CreateLight,
    IDirect3DRM3Impl_CreateLightRGB,
    IDirect3DRM3Impl_CreateMaterial,
    IDirect3DRM3Impl_CreateDevice,
    IDirect3DRM3Impl_CreateDeviceFromSurface,
    IDirect3DRM3Impl_CreateDeviceFromD3D,
    IDirect3DRM3Impl_CreateDeviceFromClipper,
    IDirect3DRM3Impl_CreateTextureFromSurface,
    IDirect3DRM3Impl_CreateShadow,
    IDirect3DRM3Impl_CreateViewport,
    IDirect3DRM3Impl_CreateWrap,
    IDirect3DRM3Impl_CreateUserVisual,
    IDirect3DRM3Impl_LoadTexture,
    IDirect3DRM3Impl_LoadTextureFromResource,
    IDirect3DRM3Impl_SetSearchPath,
    IDirect3DRM3Impl_AddSearchPath,
    IDirect3DRM3Impl_GetSearchPath,
    IDirect3DRM3Impl_SetDefaultTextureColors,
    IDirect3DRM3Impl_SetDefaultTextureShades,
    IDirect3DRM3Impl_GetDevices,
    IDirect3DRM3Impl_GetNamedObject,
    IDirect3DRM3Impl_EnumerateObjects,
    IDirect3DRM3Impl_Load,
    IDirect3DRM3Impl_Tick,
    IDirect3DRM3Impl_CreateProgressiveMesh,
    IDirect3DRM3Impl_RegisterClient,
    IDirect3DRM3Impl_UnregisterClient,
    IDirect3DRM3Impl_CreateClippedVisual,
    IDirect3DRM3Impl_SetOptions,
    IDirect3DRM3Impl_GetOptions
};

HRESULT WINAPI Direct3DRMCreate(IDirect3DRM **d3drm)
{
    IDirect3DRMImpl *object;

    TRACE("d3drm %p.\n", d3drm);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRM_iface.lpVtbl = &Direct3DRM_Vtbl;
    object->IDirect3DRM2_iface.lpVtbl = &Direct3DRM2_Vtbl;
    object->IDirect3DRM3_iface.lpVtbl = &Direct3DRM3_Vtbl;
    object->ref = 1;

    *d3drm = &object->IDirect3DRM_iface;

    return S_OK;
}
