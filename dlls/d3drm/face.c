/*
 * Implementation of IDirect3DRMFace Interface
 *
 * Copyright 2013 André Hentschel
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
    IDirect3DRMFace IDirect3DRMFace_iface;
    LONG ref;

} IDirect3DRMFaceImpl;

static inline IDirect3DRMFaceImpl *impl_from_IDirect3DRMFace(IDirect3DRMFace *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMFaceImpl, IDirect3DRMFace_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMFaceImpl_QueryInterface(IDirect3DRMFace* iface,
                                                         REFIID riid, void** object)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), object);

    *object = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirect3DRMFace))
    {
        *object = &This->IDirect3DRMFace_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDirect3DRMFace_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI IDirect3DRMFaceImpl_AddRef(IDirect3DRMFace* iface)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirect3DRMFaceImpl_Release(IDirect3DRMFace* iface)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMFaceImpl_Clone(IDirect3DRMFace* iface,
                                                LPUNKNOWN unkwn, REFIID riid, LPVOID* object)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    FIXME("(%p/%p)->(%p, %s, %p): stub\n", iface, This, unkwn, debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_AddDestroyCallback(IDirect3DRMFace* iface,
                                                             D3DRMOBJECTCALLBACK cb,
                                                             LPVOID argument)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_DeleteDestroyCallback(IDirect3DRMFace* iface,
                                                                D3DRMOBJECTCALLBACK cb,
                                                                LPVOID argument)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_SetAppData(IDirect3DRMFace* iface, DWORD data)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMFaceImpl_GetAppData(IDirect3DRMFace* iface)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_SetName(IDirect3DRMFace* iface, LPCSTR name)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_GetName(IDirect3DRMFace* iface, LPDWORD size, LPSTR name)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_GetClassName(IDirect3DRMFace* iface,
                                                       LPDWORD size, LPSTR name)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, size, name);

    if (!size || *size < strlen("Face") || !name)
        return E_INVALIDARG;

    strcpy(name, "Face");
    *size = sizeof("Face");

    return D3DRM_OK;
}

/*** IDirect3DRMFace methods ***/
static HRESULT WINAPI IDirect3DRMFaceImpl_AddVertex(IDirect3DRMFace* iface,
                                                    D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    FIXME("(%p/%p)->(%f, %f, %f): stub\n", iface, This, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_AddVertexAndNormalIndexed(IDirect3DRMFace* iface,
                                                                    DWORD vertex, DWORD normal)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%u, %u): stub\n", iface, This, vertex, normal);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_SetColorRGB(IDirect3DRMFace* iface,
                                                      D3DVALUE r, D3DVALUE g, D3DVALUE b)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    FIXME("(%p/%p)->(%f, %f, %f): stub\n", iface, This, r, g, b);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_SetColor(IDirect3DRMFace* iface, D3DCOLOR color)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_SetTexture(IDirect3DRMFace* iface,
                                                     IDirect3DRMTexture *texture)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%p): stub\n", iface, This, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_SetTextureCoordinates(IDirect3DRMFace* iface,
                                                                DWORD vertex,
                                                                D3DVALUE u, D3DVALUE v)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%u, %f, %f): stub\n", iface, This, vertex, u, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_SetMaterial(IDirect3DRMFace* iface,
                                                     IDirect3DRMMaterial *material)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%p): stub\n", iface, This, material);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_SetTextureTopology(IDirect3DRMFace* iface,
                                                             BOOL wrap_u, BOOL wrap_v)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%d, %d): stub\n", iface, This, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_GetVertex(IDirect3DRMFace* iface, DWORD index,
                                                    D3DVECTOR *vertex, D3DVECTOR *normal)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%u, %p, %p): stub\n", iface, This, index, vertex, normal);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_GetVertices(IDirect3DRMFace* iface, DWORD *vertex_count,
                                                      D3DVECTOR *coords, D3DVECTOR *normals)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%p, %p, %p): stub\n", iface, This, vertex_count, coords, normals);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_GetTextureCoordinates(IDirect3DRMFace* iface,
                                                                DWORD vertex,
                                                                D3DVALUE *u, D3DVALUE *v)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%u, %p, %p): stub\n", iface, This, vertex, u, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_GetTextureTopology(IDirect3DRMFace* iface,
                                                             BOOL *wrap_u, BOOL *wrap_v)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%p, %p): stub\n", iface, This, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_GetNormal(IDirect3DRMFace* iface, D3DVECTOR *normal)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%p): stub\n", iface, This, normal);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_GetTexture(IDirect3DRMFace* iface,
                                                     IDirect3DRMTexture **texture)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%p): stub\n", iface, This, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFaceImpl_GetMaterial(IDirect3DRMFace* iface,
                                                      IDirect3DRMMaterial **material)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%p): stub\n", iface, This, material);

    return E_NOTIMPL;
}

static int WINAPI IDirect3DRMFaceImpl_GetVertexCount(IDirect3DRMFace* iface)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static int WINAPI IDirect3DRMFaceImpl_GetVertexIndex(IDirect3DRMFace* iface, DWORD which)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%u): stub\n", iface, This, which);

    return 0;
}

static int WINAPI IDirect3DRMFaceImpl_GetTextureCoordinateIndex(IDirect3DRMFace* iface, DWORD which)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(%u): stub\n", iface, This, which);

    return 0;
}

static D3DCOLOR WINAPI IDirect3DRMFaceImpl_GetColor(IDirect3DRMFace* iface)
{
    IDirect3DRMFaceImpl *This = impl_from_IDirect3DRMFace(iface);

    TRACE("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static const struct IDirect3DRMFaceVtbl Direct3DRMFace_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMFaceImpl_QueryInterface,
    IDirect3DRMFaceImpl_AddRef,
    IDirect3DRMFaceImpl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMFaceImpl_Clone,
    IDirect3DRMFaceImpl_AddDestroyCallback,
    IDirect3DRMFaceImpl_DeleteDestroyCallback,
    IDirect3DRMFaceImpl_SetAppData,
    IDirect3DRMFaceImpl_GetAppData,
    IDirect3DRMFaceImpl_SetName,
    IDirect3DRMFaceImpl_GetName,
    IDirect3DRMFaceImpl_GetClassName,
    /*** IDirect3DRMFace methods ***/
    IDirect3DRMFaceImpl_AddVertex,
    IDirect3DRMFaceImpl_AddVertexAndNormalIndexed,
    IDirect3DRMFaceImpl_SetColorRGB,
    IDirect3DRMFaceImpl_SetColor,
    IDirect3DRMFaceImpl_SetTexture,
    IDirect3DRMFaceImpl_SetTextureCoordinates,
    IDirect3DRMFaceImpl_SetMaterial,
    IDirect3DRMFaceImpl_SetTextureTopology,
    IDirect3DRMFaceImpl_GetVertex,
    IDirect3DRMFaceImpl_GetVertices,
    IDirect3DRMFaceImpl_GetTextureCoordinates,
    IDirect3DRMFaceImpl_GetTextureTopology,
    IDirect3DRMFaceImpl_GetNormal,
    IDirect3DRMFaceImpl_GetTexture,
    IDirect3DRMFaceImpl_GetMaterial,
    IDirect3DRMFaceImpl_GetVertexCount,
    IDirect3DRMFaceImpl_GetVertexIndex,
    IDirect3DRMFaceImpl_GetTextureCoordinateIndex,
    IDirect3DRMFaceImpl_GetColor
};

HRESULT Direct3DRMFace_create(IUnknown** ret_iface)
{
    IDirect3DRMFaceImpl* object;

    TRACE("(%p)\n", ret_iface);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DRMFaceImpl));
    if (!object)
        return E_OUTOFMEMORY;

    object->IDirect3DRMFace_iface.lpVtbl = &Direct3DRMFace_Vtbl;
    object->ref = 1;

    *ret_iface = (IUnknown*)&object->IDirect3DRMFace_iface;

    return S_OK;
}
