/*
 * Implementation of IDirect3DRMViewport Interface
 *
 * Copyright 2012 André Hentschel
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
    IDirect3DRMViewport IDirect3DRMViewport_iface;
    LONG ref;
} IDirect3DRMViewportImpl;

static inline IDirect3DRMViewportImpl *impl_from_IDirect3DRMViewport(IDirect3DRMViewport *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMViewportImpl, IDirect3DRMViewport_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMViewportImpl_QueryInterface(IDirect3DRMViewport* iface,
                                                             REFIID riid, void** object)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirect3DRMViewport))
    {
        IClassFactory_AddRef(iface);
        *object = This;
        return S_OK;
    }

    ERR("(%p/%p)->(%s, %p),not found\n", iface, This, debugstr_guid(riid), object);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DRMViewportImpl_AddRef(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IDirect3DRMViewportImpl_Release(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)\n", This);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMViewportImpl_Clone(IDirect3DRMViewport* iface,
                                                        LPUNKNOWN unkwn, REFIID riid,
                                                        LPVOID* object)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p, %s, %p): stub\n", iface, This, unkwn, debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_AddDestroyCallback(IDirect3DRMViewport* iface,
                                                                     D3DRMOBJECTCALLBACK cb,
                                                                     LPVOID argument)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_DeleteDestroyCallback(IDirect3DRMViewport* iface,
                                                                        D3DRMOBJECTCALLBACK cb,
                                                                        LPVOID argument)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_SetAppData(IDirect3DRMViewport* iface,
                                                             DWORD data)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMViewportImpl_GetAppData(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_SetName(IDirect3DRMViewport* iface, LPCSTR name)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_GetName(IDirect3DRMViewport* iface,
                                                          LPDWORD size, LPSTR name)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_GetClassName(IDirect3DRMViewport* iface,
                                                               LPDWORD size, LPSTR name)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, size, name);

    return E_NOTIMPL;
}

/*** IDirect3DRMViewport methods ***/
static HRESULT WINAPI IDirect3DRMViewportImpl_Init(IDirect3DRMViewport* iface,
                                                   LPDIRECT3DRMDEVICE dev, LPDIRECT3DRMFRAME camera,
                                                   DWORD xpos, DWORD ypos,
                                                   DWORD width, DWORD height)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p, %p, %u, %u, %u, %u): stub\n", iface, This, dev, camera,
          xpos, ypos, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_Clear(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_Render(IDirect3DRMViewport* iface,
                                                     LPDIRECT3DRMFRAME frame)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, frame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_SetFront(IDirect3DRMViewport* iface, D3DVALUE front)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%f): stub\n", iface, This, front);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_SetBack(IDirect3DRMViewport* iface, D3DVALUE back)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%f): stub\n", iface, This, back);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_SetField(IDirect3DRMViewport* iface, D3DVALUE field)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%f): stub\n", iface, This, field);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_SetUniformScaling(IDirect3DRMViewport* iface, BOOL b)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%d): stub\n", iface, This, b);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_SetCamera(IDirect3DRMViewport* iface,
                                                        LPDIRECT3DRMFRAME frame)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, frame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_SetProjection(IDirect3DRMViewport* iface,
                                                            D3DRMPROJECTIONTYPE type)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, type);

    return S_OK;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_Transform(IDirect3DRMViewport* iface,
                                                        D3DRMVECTOR4D *d, D3DVECTOR *s)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_InverseTransform(IDirect3DRMViewport* iface,
                                                               D3DVECTOR *d, D3DRMVECTOR4D *s)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_Configure(IDirect3DRMViewport* iface, LONG x, LONG y,
                                                        DWORD width, DWORD height)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%u, %u, %u, %u): stub\n", iface, This, x, y, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_ForceUpdate(IDirect3DRMViewport* iface,
                                                          DWORD x1, DWORD y1, DWORD x2, DWORD y2)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%u, %u, %u, %u): stub\n", iface, This, x1, y1, x2, y2);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_SetPlane(IDirect3DRMViewport* iface,
                                                       D3DVALUE left, D3DVALUE right,
                                                       D3DVALUE bottom, D3DVALUE top)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%f, %f, %f, %f): stub\n", iface, This, left, right, bottom, top);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_GetCamera(IDirect3DRMViewport* iface,
                                                        LPDIRECT3DRMFRAME * frame)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, frame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_GetDevice(IDirect3DRMViewport* iface,
                                                        LPDIRECT3DRMDEVICE * device)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, device);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_GetPlane(IDirect3DRMViewport* iface,
                                                       D3DVALUE *left, D3DVALUE *right,
                                                       D3DVALUE *bottom, D3DVALUE *top)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p, %p, %p, %p): stub\n", iface, This, left, right, bottom, top);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_Pick(IDirect3DRMViewport* iface, LONG x, LONG y,
                                                   LPDIRECT3DRMPICKEDARRAY *return_visuals)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%u, %u, %p): stub\n", iface, This, x, y, return_visuals);

    return E_NOTIMPL;
}

static BOOL WINAPI IDirect3DRMViewportImpl_GetUniformScaling(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static LONG WINAPI IDirect3DRMViewportImpl_GetX(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static LONG WINAPI IDirect3DRMViewportImpl_GetY(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMViewportImpl_GetWidth(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMViewportImpl_GetHeight(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static D3DVALUE WINAPI IDirect3DRMViewportImpl_GetField(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static D3DVALUE WINAPI IDirect3DRMViewportImpl_GetBack(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static D3DVALUE WINAPI IDirect3DRMViewportImpl_GetFront(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static D3DRMPROJECTIONTYPE WINAPI IDirect3DRMViewportImpl_GetProjection(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_GetDirect3DViewport(IDirect3DRMViewport* iface,
                                                                  LPDIRECT3DVIEWPORT * viewport)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, This);

    return E_NOTIMPL;
}

static const struct IDirect3DRMViewportVtbl Direct3DRMViewport_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMViewportImpl_QueryInterface,
    IDirect3DRMViewportImpl_AddRef,
    IDirect3DRMViewportImpl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMViewportImpl_Clone,
    IDirect3DRMViewportImpl_AddDestroyCallback,
    IDirect3DRMViewportImpl_DeleteDestroyCallback,
    IDirect3DRMViewportImpl_SetAppData,
    IDirect3DRMViewportImpl_GetAppData,
    IDirect3DRMViewportImpl_SetName,
    IDirect3DRMViewportImpl_GetName,
    IDirect3DRMViewportImpl_GetClassName,
    /*** IDirect3DRMViewport methods ***/
    IDirect3DRMViewportImpl_Init,
    IDirect3DRMViewportImpl_Clear,
    IDirect3DRMViewportImpl_Render,
    IDirect3DRMViewportImpl_SetFront,
    IDirect3DRMViewportImpl_SetBack,
    IDirect3DRMViewportImpl_SetField,
    IDirect3DRMViewportImpl_SetUniformScaling,
    IDirect3DRMViewportImpl_SetCamera,
    IDirect3DRMViewportImpl_SetProjection,
    IDirect3DRMViewportImpl_Transform,
    IDirect3DRMViewportImpl_InverseTransform,
    IDirect3DRMViewportImpl_Configure,
    IDirect3DRMViewportImpl_ForceUpdate,
    IDirect3DRMViewportImpl_SetPlane,
    IDirect3DRMViewportImpl_GetCamera,
    IDirect3DRMViewportImpl_GetDevice,
    IDirect3DRMViewportImpl_GetPlane,
    IDirect3DRMViewportImpl_Pick,
    IDirect3DRMViewportImpl_GetUniformScaling,
    IDirect3DRMViewportImpl_GetX,
    IDirect3DRMViewportImpl_GetY,
    IDirect3DRMViewportImpl_GetWidth,
    IDirect3DRMViewportImpl_GetHeight,
    IDirect3DRMViewportImpl_GetField,
    IDirect3DRMViewportImpl_GetBack,
    IDirect3DRMViewportImpl_GetFront,
    IDirect3DRMViewportImpl_GetProjection,
    IDirect3DRMViewportImpl_GetDirect3DViewport
};

HRESULT Direct3DRMViewport_create(IUnknown** ppObj)
{
    IDirect3DRMViewportImpl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DRMViewportImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->IDirect3DRMViewport_iface.lpVtbl = &Direct3DRMViewport_Vtbl;
    object->ref = 1;

    *ppObj = (IUnknown*)object;

    return S_OK;
}
