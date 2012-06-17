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
    IDirect3DRMViewport2 IDirect3DRMViewport2_iface;
    LONG ref;
    D3DVALUE back;
    D3DVALUE front;
    D3DVALUE field;
    D3DRMPROJECTIONTYPE projection;
} IDirect3DRMViewportImpl;

static inline IDirect3DRMViewportImpl *impl_from_IDirect3DRMViewport(IDirect3DRMViewport *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMViewportImpl, IDirect3DRMViewport_iface);
}

static inline IDirect3DRMViewportImpl *impl_from_IDirect3DRMViewport2(IDirect3DRMViewport2 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMViewportImpl, IDirect3DRMViewport2_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMViewportImpl_QueryInterface(IDirect3DRMViewport* iface,
                                                             REFIID riid, void** object)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), object);

    *object = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirect3DRMViewport))
    {
        *object = &This->IDirect3DRMViewport_iface;
    }
    else if(IsEqualGUID(riid, &IID_IDirect3DRMViewport2))
    {
        *object = &This->IDirect3DRMViewport2_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDirect3DRMViewport_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI IDirect3DRMViewportImpl_AddRef(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirect3DRMViewportImpl_Release(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

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

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, size, name);

    return IDirect3DRMViewport2_GetClassName(&This->IDirect3DRMViewport2_iface, size, name);
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

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_Render(IDirect3DRMViewport* iface,
                                                     LPDIRECT3DRMFRAME frame)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, frame);

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMViewportImpl_SetFront(IDirect3DRMViewport* iface, D3DVALUE front)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    TRACE("(%p/%p)->(%f)\n", iface, This, front);

    return IDirect3DRMViewport2_SetFront(&This->IDirect3DRMViewport2_iface, front);
}

static HRESULT WINAPI IDirect3DRMViewportImpl_SetBack(IDirect3DRMViewport* iface, D3DVALUE back)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    TRACE("(%p/%p)->(%f)\n", iface, This, back);

    return IDirect3DRMViewport2_SetBack(&This->IDirect3DRMViewport2_iface, back);
}

static HRESULT WINAPI IDirect3DRMViewportImpl_SetField(IDirect3DRMViewport* iface, D3DVALUE field)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    TRACE("(%p/%p)->(%f)\n", iface, This, field);

    return IDirect3DRMViewport2_SetField(&This->IDirect3DRMViewport2_iface, field);
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

    TRACE("(%p/%p)->(%u)\n", iface, This, type);

    return IDirect3DRMViewport2_SetProjection(&This->IDirect3DRMViewport2_iface, type);
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

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirect3DRMViewport2_GetField(&This->IDirect3DRMViewport2_iface);
}

static D3DVALUE WINAPI IDirect3DRMViewportImpl_GetBack(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirect3DRMViewport2_GetBack(&This->IDirect3DRMViewport2_iface);
}

static D3DVALUE WINAPI IDirect3DRMViewportImpl_GetFront(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirect3DRMViewport2_GetFront(&This->IDirect3DRMViewport2_iface);
}

static D3DRMPROJECTIONTYPE WINAPI IDirect3DRMViewportImpl_GetProjection(IDirect3DRMViewport* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return IDirect3DRMViewport2_GetProjection(&This->IDirect3DRMViewport2_iface);
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


/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMViewport2Impl_QueryInterface(IDirect3DRMViewport2* iface,
                                                             REFIID riid, void** object)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);
    return IDirect3DRMViewport_QueryInterface(&This->IDirect3DRMViewport_iface, riid, object);
}

static ULONG WINAPI IDirect3DRMViewport2Impl_AddRef(IDirect3DRMViewport2* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);
    return IDirect3DRMViewport_AddRef(&This->IDirect3DRMViewport_iface);
}

static ULONG WINAPI IDirect3DRMViewport2Impl_Release(IDirect3DRMViewport2* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);
    return IDirect3DRMViewport_Release(&This->IDirect3DRMViewport_iface);
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMViewport2Impl_Clone(IDirect3DRMViewport2* iface,
                                                        LPUNKNOWN unkwn, REFIID riid,
                                                        LPVOID* object)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%p, %s, %p): stub\n", iface, This, unkwn, debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_AddDestroyCallback(IDirect3DRMViewport2* iface,
                                                                     D3DRMOBJECTCALLBACK cb,
                                                                     LPVOID argument)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_DeleteDestroyCallback(IDirect3DRMViewport2* iface,
                                                                        D3DRMOBJECTCALLBACK cb,
                                                                        LPVOID argument)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_SetAppData(IDirect3DRMViewport2* iface,
                                                             DWORD data)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMViewport2Impl_GetAppData(IDirect3DRMViewport2* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_SetName(IDirect3DRMViewport2* iface, LPCSTR name)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_GetName(IDirect3DRMViewport2* iface,
                                                          LPDWORD size, LPSTR name)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_GetClassName(IDirect3DRMViewport2* iface,
                                                            LPDWORD size, LPSTR name)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, size, name);

    if (!size || *size < strlen("Viewport") || !name)
        return E_INVALIDARG;

    strcpy(name, "Viewport");
    *size = sizeof("Viewport");

    return D3DRM_OK;
}

/*** IDirect3DRMViewport methods ***/
static HRESULT WINAPI IDirect3DRMViewport2Impl_Init(IDirect3DRMViewport2* iface,
                                                   LPDIRECT3DRMDEVICE3 dev, LPDIRECT3DRMFRAME3 camera,
                                                   DWORD xpos, DWORD ypos,
                                                   DWORD width, DWORD height)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%p, %p, %u, %u, %u, %u): stub\n", iface, This, dev, camera,
          xpos, ypos, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_Clear(IDirect3DRMViewport2* iface, DWORD flags)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_Render(IDirect3DRMViewport2* iface,
                                                     LPDIRECT3DRMFRAME3 frame)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, frame);

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_SetFront(IDirect3DRMViewport2* iface, D3DVALUE front)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    TRACE("(%p/%p)->(%f):\n", iface, This, front);

    This->front = front;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_SetBack(IDirect3DRMViewport2* iface, D3DVALUE back)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    TRACE("(%p/%p)->(%f)\n", iface, This, back);

    This->back = back;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_SetField(IDirect3DRMViewport2* iface, D3DVALUE field)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    TRACE("(%p/%p)->(%f)\n", iface, This, field);

    This->field = field;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_SetUniformScaling(IDirect3DRMViewport2* iface, BOOL b)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%d): stub\n", iface, This, b);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_SetCamera(IDirect3DRMViewport2* iface,
                                                        LPDIRECT3DRMFRAME3 frame)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, frame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_SetProjection(IDirect3DRMViewport2* iface,
                                                            D3DRMPROJECTIONTYPE type)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    TRACE("(%p/%p)->(%u)\n", iface, This, type);

    This->projection = type;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_Transform(IDirect3DRMViewport2* iface,
                                                        D3DRMVECTOR4D *d, D3DVECTOR *s)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_InverseTransform(IDirect3DRMViewport2* iface,
                                                               D3DVECTOR *d, D3DRMVECTOR4D *s)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_Configure(IDirect3DRMViewport2* iface, LONG x, LONG y,
                                                        DWORD width, DWORD height)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%u, %u, %u, %u): stub\n", iface, This, x, y, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_ForceUpdate(IDirect3DRMViewport2* iface,
                                                          DWORD x1, DWORD y1, DWORD x2, DWORD y2)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%u, %u, %u, %u): stub\n", iface, This, x1, y1, x2, y2);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_SetPlane(IDirect3DRMViewport2* iface,
                                                       D3DVALUE left, D3DVALUE right,
                                                       D3DVALUE bottom, D3DVALUE top)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%f, %f, %f, %f): stub\n", iface, This, left, right, bottom, top);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_GetCamera(IDirect3DRMViewport2* iface,
                                                        LPDIRECT3DRMFRAME3 * frame)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, frame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_GetDevice(IDirect3DRMViewport2* iface,
                                                        LPDIRECT3DRMDEVICE3 * device)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, device);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_GetPlane(IDirect3DRMViewport2* iface,
                                                       D3DVALUE *left, D3DVALUE *right,
                                                       D3DVALUE *bottom, D3DVALUE *top)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%p, %p, %p, %p): stub\n", iface, This, left, right, bottom, top);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_Pick(IDirect3DRMViewport2* iface, LONG x, LONG y,
                                                   LPDIRECT3DRMPICKEDARRAY *return_visuals)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%u, %u, %p): stub\n", iface, This, x, y, return_visuals);

    return E_NOTIMPL;
}

static BOOL WINAPI IDirect3DRMViewport2Impl_GetUniformScaling(IDirect3DRMViewport2* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static LONG WINAPI IDirect3DRMViewport2Impl_GetX(IDirect3DRMViewport2* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static LONG WINAPI IDirect3DRMViewport2Impl_GetY(IDirect3DRMViewport2* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMViewport2Impl_GetWidth(IDirect3DRMViewport2* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMViewport2Impl_GetHeight(IDirect3DRMViewport2* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static D3DVALUE WINAPI IDirect3DRMViewport2Impl_GetField(IDirect3DRMViewport2* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->field;
}

static D3DVALUE WINAPI IDirect3DRMViewport2Impl_GetBack(IDirect3DRMViewport2* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->back;
}

static D3DVALUE WINAPI IDirect3DRMViewport2Impl_GetFront(IDirect3DRMViewport2* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->front;
}

static D3DRMPROJECTIONTYPE WINAPI IDirect3DRMViewport2Impl_GetProjection(IDirect3DRMViewport2* iface)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->projection;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_GetDirect3DViewport(IDirect3DRMViewport2* iface,
                                                                  LPDIRECT3DVIEWPORT * viewport)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, This);

    return E_NOTIMPL;
}

/*** IDirect3DRMViewport2 methods ***/
static HRESULT WINAPI IDirect3DRMViewport2Impl_TransformVectors(IDirect3DRMViewport2* iface,
                                                                DWORD numvectors,
                                                                LPD3DRMVECTOR4D dstvectors,
                                                                LPD3DVECTOR srcvectors)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%u, %p, %p): stub\n", iface, This, numvectors, dstvectors, srcvectors);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMViewport2Impl_InverseTransformVectors(IDirect3DRMViewport2* iface,
                                                                DWORD numvectors,
                                                                LPD3DVECTOR dstvectors,
                                                                LPD3DRMVECTOR4D srcvectors)
{
    IDirect3DRMViewportImpl *This = impl_from_IDirect3DRMViewport2(iface);

    FIXME("(%p/%p)->(%u, %p, %p): stub\n", iface, This, numvectors, dstvectors, srcvectors);

    return E_NOTIMPL;
}

static const struct IDirect3DRMViewport2Vtbl Direct3DRMViewport2_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMViewport2Impl_QueryInterface,
    IDirect3DRMViewport2Impl_AddRef,
    IDirect3DRMViewport2Impl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMViewport2Impl_Clone,
    IDirect3DRMViewport2Impl_AddDestroyCallback,
    IDirect3DRMViewport2Impl_DeleteDestroyCallback,
    IDirect3DRMViewport2Impl_SetAppData,
    IDirect3DRMViewport2Impl_GetAppData,
    IDirect3DRMViewport2Impl_SetName,
    IDirect3DRMViewport2Impl_GetName,
    IDirect3DRMViewport2Impl_GetClassName,
    /*** IDirect3DRMViewport methods ***/
    IDirect3DRMViewport2Impl_Init,
    IDirect3DRMViewport2Impl_Clear,
    IDirect3DRMViewport2Impl_Render,
    IDirect3DRMViewport2Impl_SetFront,
    IDirect3DRMViewport2Impl_SetBack,
    IDirect3DRMViewport2Impl_SetField,
    IDirect3DRMViewport2Impl_SetUniformScaling,
    IDirect3DRMViewport2Impl_SetCamera,
    IDirect3DRMViewport2Impl_SetProjection,
    IDirect3DRMViewport2Impl_Transform,
    IDirect3DRMViewport2Impl_InverseTransform,
    IDirect3DRMViewport2Impl_Configure,
    IDirect3DRMViewport2Impl_ForceUpdate,
    IDirect3DRMViewport2Impl_SetPlane,
    IDirect3DRMViewport2Impl_GetCamera,
    IDirect3DRMViewport2Impl_GetDevice,
    IDirect3DRMViewport2Impl_GetPlane,
    IDirect3DRMViewport2Impl_Pick,
    IDirect3DRMViewport2Impl_GetUniformScaling,
    IDirect3DRMViewport2Impl_GetX,
    IDirect3DRMViewport2Impl_GetY,
    IDirect3DRMViewport2Impl_GetWidth,
    IDirect3DRMViewport2Impl_GetHeight,
    IDirect3DRMViewport2Impl_GetField,
    IDirect3DRMViewport2Impl_GetBack,
    IDirect3DRMViewport2Impl_GetFront,
    IDirect3DRMViewport2Impl_GetProjection,
    IDirect3DRMViewport2Impl_GetDirect3DViewport,
    /*** IDirect3DRMViewport2 methods ***/
    IDirect3DRMViewport2Impl_TransformVectors,
    IDirect3DRMViewport2Impl_InverseTransformVectors
};

HRESULT Direct3DRMViewport_create(REFIID riid, IUnknown** ppObj)
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
    object->IDirect3DRMViewport2_iface.lpVtbl = &Direct3DRMViewport2_Vtbl;
    object->ref = 1;

    if (IsEqualGUID(riid, &IID_IDirect3DRMViewport2))
        *ppObj = (IUnknown*)&object->IDirect3DRMViewport2_iface;
    else
        *ppObj = (IUnknown*)&object->IDirect3DRMViewport_iface;

    return S_OK;
}
