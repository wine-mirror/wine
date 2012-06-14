/*
 * Implementation of IDirect3DRMLight Interface
 *
 * Copyright 2012 André Hentschel
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
    IDirect3DRMLight IDirect3DRMLight_iface;
    LONG ref;
    D3DRMLIGHTTYPE type;
    D3DCOLOR color;
    D3DVALUE range;
    D3DVALUE cattenuation;
    D3DVALUE lattenuation;
    D3DVALUE qattenuation;
    D3DVALUE umbra;
    D3DVALUE penumbra;
} IDirect3DRMLightImpl;

static inline IDirect3DRMLightImpl *impl_from_IDirect3DRMLight(IDirect3DRMLight *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMLightImpl, IDirect3DRMLight_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMLightImpl_QueryInterface(IDirect3DRMLight* iface,
                                                           REFIID riid, void** object)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), object);

    *object = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IDirect3DRMLight))
    {
        *object = &This->IDirect3DRMLight_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDirect3DRMLight_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI IDirect3DRMLightImpl_AddRef(IDirect3DRMLight* iface)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirect3DRMLightImpl_Release(IDirect3DRMLight* iface)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", iface, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMLightImpl_Clone(IDirect3DRMLight* iface,
                                                  LPUNKNOWN unkwn, REFIID riid,
                                                  LPVOID* object)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    FIXME("(%p/%p)->(%p, %s, %p): stub\n", iface, This, unkwn, debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMLightImpl_AddDestroyCallback(IDirect3DRMLight* iface,
                                                               D3DRMOBJECTCALLBACK cb,
                                                               LPVOID argument)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMLightImpl_DeleteDestroyCallback(IDirect3DRMLight* iface,
                                                                  D3DRMOBJECTCALLBACK cb,
                                                                  LPVOID argument)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMLightImpl_SetAppData(IDirect3DRMLight* iface,
                                                       DWORD data)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMLightImpl_GetAppData(IDirect3DRMLight* iface)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMLightImpl_SetName(IDirect3DRMLight* iface, LPCSTR name)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMLightImpl_GetName(IDirect3DRMLight* iface,
                                                    LPDWORD size, LPSTR name)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMLightImpl_GetClassName(IDirect3DRMLight* iface,
                                                         LPDWORD size, LPSTR name)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, size, name);

    if (!size || *size < strlen("Light") || !name)
        return E_INVALIDARG;

    strcpy(name, "Light");
    *size = sizeof("Light");

    return D3DRM_OK;
}

/*** IDirect3DRMLight methods ***/
static HRESULT WINAPI IDirect3DRMLightImpl_SetType(IDirect3DRMLight* iface, D3DRMLIGHTTYPE type)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->(%u)\n", iface, This, type);

    This->type = type;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMLightImpl_SetColor(IDirect3DRMLight* iface, D3DCOLOR color)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->(%u)\n", iface, This, color);

    This->color = color;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMLightImpl_SetColorRGB(IDirect3DRMLight* iface,
                                                         D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->(%f,%f,%f)\n", iface, This, red, green, blue);

    This->color = D3DCOLOR_ARGB(0xff, (BYTE)(red   * 255.0f),
                                      (BYTE)(green * 255.0f),
                                      (BYTE)(blue  * 255.0f));

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMLightImpl_SetRange(IDirect3DRMLight* iface, D3DVALUE range)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->(%f)\n", iface, This, range);

    This->range = range;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMLightImpl_SetUmbra(IDirect3DRMLight* iface, D3DVALUE umbra)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->(%f)\n", iface, This, umbra);

    This->umbra = umbra;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMLightImpl_SetPenumbra(IDirect3DRMLight* iface, D3DVALUE penumbra)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->(%f)\n", iface, This, penumbra);

    This->penumbra = penumbra;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMLightImpl_SetConstantAttenuation(IDirect3DRMLight* iface,
                                                                    D3DVALUE cattenuation)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->(%f)\n", iface, This, cattenuation);

    This->cattenuation = cattenuation;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMLightImpl_SetLinearAttenuation(IDirect3DRMLight* iface,
                                                                  D3DVALUE lattenuation)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->(%f)\n", iface, This, lattenuation);

    This->lattenuation = lattenuation;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMLightImpl_SetQuadraticAttenuation(IDirect3DRMLight* iface,
                                                                     D3DVALUE qattenuation)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->(%f)\n", iface, This, qattenuation);

    This->qattenuation = qattenuation;

    return D3DRM_OK;
}

static D3DVALUE WINAPI IDirect3DRMLightImpl_GetRange(IDirect3DRMLight* iface)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->range;
}

static D3DVALUE WINAPI IDirect3DRMLightImpl_GetUmbra(IDirect3DRMLight* iface)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->umbra;
}

static D3DVALUE WINAPI IDirect3DRMLightImpl_GetPenumbra(IDirect3DRMLight* iface)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->penumbra;
}

static D3DVALUE WINAPI IDirect3DRMLightImpl_GetConstantAttenuation(IDirect3DRMLight* iface)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->cattenuation;
}

static D3DVALUE WINAPI IDirect3DRMLightImpl_GetLinearAttenuation(IDirect3DRMLight* iface)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->lattenuation;
}

static D3DVALUE WINAPI IDirect3DRMLightImpl_GetQuadraticAttenuation(IDirect3DRMLight* iface)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->qattenuation;
}

static D3DCOLOR WINAPI IDirect3DRMLightImpl_GetColor(IDirect3DRMLight* iface)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->color;
}

static D3DRMLIGHTTYPE WINAPI IDirect3DRMLightImpl_GetType(IDirect3DRMLight* iface)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return This->type;
}

static HRESULT WINAPI IDirect3DRMLightImpl_SetEnableFrame(IDirect3DRMLight* iface,
                                                            LPDIRECT3DRMFRAME frame)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, frame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMLightImpl_GetEnableFrame(IDirect3DRMLight* iface,
                                                            LPDIRECT3DRMFRAME* frame)
{
    IDirect3DRMLightImpl *This = impl_from_IDirect3DRMLight(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, frame);

    return E_NOTIMPL;
}

static const struct IDirect3DRMLightVtbl Direct3DRMLight_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMLightImpl_QueryInterface,
    IDirect3DRMLightImpl_AddRef,
    IDirect3DRMLightImpl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMLightImpl_Clone,
    IDirect3DRMLightImpl_AddDestroyCallback,
    IDirect3DRMLightImpl_DeleteDestroyCallback,
    IDirect3DRMLightImpl_SetAppData,
    IDirect3DRMLightImpl_GetAppData,
    IDirect3DRMLightImpl_SetName,
    IDirect3DRMLightImpl_GetName,
    IDirect3DRMLightImpl_GetClassName,
    /*** IDirect3DRMLight methods ***/
    IDirect3DRMLightImpl_SetType,
    IDirect3DRMLightImpl_SetColor,
    IDirect3DRMLightImpl_SetColorRGB,
    IDirect3DRMLightImpl_SetRange,
    IDirect3DRMLightImpl_SetUmbra,
    IDirect3DRMLightImpl_SetPenumbra,
    IDirect3DRMLightImpl_SetConstantAttenuation,
    IDirect3DRMLightImpl_SetLinearAttenuation,
    IDirect3DRMLightImpl_SetQuadraticAttenuation,
    IDirect3DRMLightImpl_GetRange,
    IDirect3DRMLightImpl_GetUmbra,
    IDirect3DRMLightImpl_GetPenumbra,
    IDirect3DRMLightImpl_GetConstantAttenuation,
    IDirect3DRMLightImpl_GetLinearAttenuation,
    IDirect3DRMLightImpl_GetQuadraticAttenuation,
    IDirect3DRMLightImpl_GetColor,
    IDirect3DRMLightImpl_GetType,
    IDirect3DRMLightImpl_SetEnableFrame,
    IDirect3DRMLightImpl_GetEnableFrame
};

HRESULT Direct3DRMLight_create(IUnknown** ppObj)
{
    IDirect3DRMLightImpl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DRMLightImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->IDirect3DRMLight_iface.lpVtbl = &Direct3DRMLight_Vtbl;
    object->ref = 1;

    *ppObj = (IUnknown*)&object->IDirect3DRMLight_iface;

    return S_OK;
}
