/*
 * Implementation of IDirect3DRMMaterial2 interface
 *
 * Copyright 2012 Christian Costa
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

typedef struct
{
    D3DVALUE r;
    D3DVALUE g;
    D3DVALUE b;
} color_rgb;

typedef struct {
    IDirect3DRMMaterial2 IDirect3DRMMaterial2_iface;
    LONG ref;
    color_rgb emissive;
    color_rgb specular;
    D3DVALUE power;
    color_rgb ambient;
} IDirect3DRMMaterialImpl;

static inline IDirect3DRMMaterialImpl *impl_from_IDirect3DRMMaterial2(IDirect3DRMMaterial2 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMMaterialImpl, IDirect3DRMMaterial2_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMMaterial2Impl_QueryInterface(IDirect3DRMMaterial2* iface,
                                                           REFIID riid, void** ret_iface)
{
    IDirect3DRMMaterialImpl *This = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirect3DRMMaterial) ||
        IsEqualGUID(riid, &IID_IDirect3DRMMaterial2))
    {
        *ret_iface = &This->IDirect3DRMMaterial2_iface;
    }
    else
    {
        *ret_iface = NULL;
        FIXME("Interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDirect3DRMMaterial_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI IDirect3DRMMaterial2Impl_AddRef(IDirect3DRMMaterial2* iface)
{
    IDirect3DRMMaterialImpl *This = impl_from_IDirect3DRMMaterial2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirect3DRMMaterial2Impl_Release(IDirect3DRMMaterial2* iface)
{
    IDirect3DRMMaterialImpl *This = impl_from_IDirect3DRMMaterial2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", iface, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMMaterial2Impl_Clone(IDirect3DRMMaterial2* iface,
                                                     LPUNKNOWN unknown, REFIID riid,
                                                     LPVOID* object)
{
    FIXME("(%p)->(%p, %s, %p): stub\n", iface, unknown, debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMaterial2Impl_AddDestroyCallback(IDirect3DRMMaterial2* iface,
                                                                  D3DRMOBJECTCALLBACK cb,
                                                                  LPVOID argument)
{
    FIXME("(%p)->(%p, %p): stub\n", iface, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMaterial2Impl_DeleteDestroyCallback(IDirect3DRMMaterial2* iface,
                                                                     D3DRMOBJECTCALLBACK cb,
                                                                     LPVOID argument)
{
    FIXME("(%p)->(%p, %p): stub\n", iface, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMaterial2Impl_SetAppData(IDirect3DRMMaterial2* iface,
                                                          DWORD data)
{
    FIXME("(%p)->(%u): stub\n", iface, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMMaterial2Impl_GetAppData(IDirect3DRMMaterial2* iface)
{
    FIXME("(%p)->(): stub\n", iface);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMaterial2Impl_SetName(IDirect3DRMMaterial2* iface, LPCSTR name)
{
    FIXME("(%p)->(%s): stub\n", iface, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMaterial2Impl_GetName(IDirect3DRMMaterial2* iface,
                                                       LPDWORD size, LPSTR name)
{
    FIXME("(%p)->(%p, %p): stub\n", iface, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMaterial2Impl_GetClassName(IDirect3DRMMaterial2* iface,
                                                           LPDWORD size, LPSTR name)
{
    TRACE("(%p)->(%p, %p)\n", iface, size, name);

    if (!size || *size < strlen("Material") || !name)
        return E_INVALIDARG;

    strcpy(name, "Material");
    *size = sizeof("Material");

    return D3DRM_OK;
}

/*** IDirect3DRMMaterial methods ***/
static HRESULT WINAPI IDirect3DRMMaterial2Impl_SetPower(IDirect3DRMMaterial2* iface, D3DVALUE power)
{
    IDirect3DRMMaterialImpl *This = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("(%p)->(%f)\n", iface, power);

    This->power = power;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMaterial2Impl_SetSpecular(IDirect3DRMMaterial2* iface,
                                                           D3DVALUE r, D3DVALUE g, D3DVALUE b)
{
    IDirect3DRMMaterialImpl *This = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("(%p)->(%f, %f, %f)\n", iface, r, g, b);

    This->specular.r = r;
    This->specular.g = g;
    This->specular.b = b;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMaterial2Impl_SetEmissive(IDirect3DRMMaterial2* iface,
                                                           D3DVALUE r, D3DVALUE g, D3DVALUE b)
{
    IDirect3DRMMaterialImpl *This = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("(%p)->(%f, %f, %f)\n", iface, r, g, b);

    This->emissive.r = r;
    This->emissive.g = g;
    This->emissive.b = b;

    return D3DRM_OK;
}

static D3DVALUE WINAPI IDirect3DRMMaterial2Impl_GetPower(IDirect3DRMMaterial2* iface)
{
    IDirect3DRMMaterialImpl *This = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("(%p)->()\n", iface);

    return This->power;
}

static HRESULT WINAPI IDirect3DRMMaterial2Impl_GetSpecular(IDirect3DRMMaterial2* iface,
                                                           D3DVALUE* r, D3DVALUE* g, D3DVALUE* b)
{
    IDirect3DRMMaterialImpl *This = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("(%p)->(%p, %p, %p)\n", iface, r, g, b);

    *r = This->specular.r;
    *g = This->specular.g;
    *b = This->specular.b;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMaterial2Impl_GetEmissive(IDirect3DRMMaterial2* iface,
                                                           D3DVALUE* r, D3DVALUE* g, D3DVALUE* b)
{
    IDirect3DRMMaterialImpl *This = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("(%p)->(%p, %p, %p)\n", iface, r, g, b);

    *r = This->emissive.r;
    *g = This->emissive.g;
    *b = This->emissive.b;

    return D3DRM_OK;
}

/*** IDirect3DRMMaterial2 methods ***/
static HRESULT WINAPI IDirect3DRMMaterial2Impl_GetAmbient(IDirect3DRMMaterial2* iface,
                                                          D3DVALUE* r, D3DVALUE* g, D3DVALUE* b)
{
    IDirect3DRMMaterialImpl *This = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("(%p)->(%p, %p, %p)\n", iface, r, g, b);

    *r = This->ambient.r;
    *g = This->ambient.g;
    *b = This->ambient.b;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMaterial2Impl_SetAmbient(IDirect3DRMMaterial2* iface,
                                                          D3DVALUE r, D3DVALUE g, D3DVALUE b)
{
    IDirect3DRMMaterialImpl *This = impl_from_IDirect3DRMMaterial2(iface);

    TRACE("(%p)->(%f, %f, %f)\n", iface, r, g, b);

    This->ambient.r = r;
    This->ambient.g = g;
    This->ambient.b = b;

    return D3DRM_OK;
}

static const struct IDirect3DRMMaterial2Vtbl Direct3DRMMaterial2_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMMaterial2Impl_QueryInterface,
    IDirect3DRMMaterial2Impl_AddRef,
    IDirect3DRMMaterial2Impl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMMaterial2Impl_Clone,
    IDirect3DRMMaterial2Impl_AddDestroyCallback,
    IDirect3DRMMaterial2Impl_DeleteDestroyCallback,
    IDirect3DRMMaterial2Impl_SetAppData,
    IDirect3DRMMaterial2Impl_GetAppData,
    IDirect3DRMMaterial2Impl_SetName,
    IDirect3DRMMaterial2Impl_GetName,
    IDirect3DRMMaterial2Impl_GetClassName,
    /*** IDirect3DRMMaterial methods ***/
    IDirect3DRMMaterial2Impl_SetPower,
    IDirect3DRMMaterial2Impl_SetSpecular,
    IDirect3DRMMaterial2Impl_SetEmissive,
    IDirect3DRMMaterial2Impl_GetPower,
    IDirect3DRMMaterial2Impl_GetSpecular,
    IDirect3DRMMaterial2Impl_GetEmissive,
    /*** IDirect3DRMMaterial2 methods ***/
    IDirect3DRMMaterial2Impl_GetAmbient,
    IDirect3DRMMaterial2Impl_SetAmbient
};

HRESULT Direct3DRMMaterial_create(IDirect3DRMMaterial2** ret_iface)
{
    IDirect3DRMMaterialImpl* object;

    TRACE("(%p)\n", ret_iface);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DRMMaterialImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->IDirect3DRMMaterial2_iface.lpVtbl = &Direct3DRMMaterial2_Vtbl;
    object->ref = 1;

    object->specular.r = 1.0f;
    object->specular.g = 1.0f;
    object->specular.b = 1.0f;

    *ret_iface = &object->IDirect3DRMMaterial2_iface;

    return S_OK;
}
