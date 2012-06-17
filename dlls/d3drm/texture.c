/*
 * Implementation of IDirect3DRMTextureX interfaces
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

typedef struct {
    IDirect3DRMTexture2 IDirect3DRMTexture2_iface;
    IDirect3DRMTexture3 IDirect3DRMTexture3_iface;
    LONG ref;
} IDirect3DRMTextureImpl;

static inline IDirect3DRMTextureImpl *impl_from_IDirect3DRMTexture2(IDirect3DRMTexture2 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMTextureImpl, IDirect3DRMTexture2_iface);
}

static inline IDirect3DRMTextureImpl *impl_from_IDirect3DRMTexture3(IDirect3DRMTexture3 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMTextureImpl, IDirect3DRMTexture3_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMTexture2Impl_QueryInterface(IDirect3DRMTexture2* iface,
                                                             REFIID riid, void** object)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), object);

    *object = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirect3DRMTexture) ||
        IsEqualGUID(riid, &IID_IDirect3DRMTexture2))
    {
        *object = &This->IDirect3DRMTexture2_iface;
    }
    else if IsEqualGUID(riid, &IID_IDirect3DRMTexture3)
    {
        *object = &This->IDirect3DRMTexture3_iface;
    }
    else
    {
        FIXME("Interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDirect3DRMTexture3_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI IDirect3DRMTexture2Impl_AddRef(IDirect3DRMTexture2* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirect3DRMTexture2Impl_Release(IDirect3DRMTexture2* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", iface, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMTexture2Impl_Clone(IDirect3DRMTexture2* iface,
                                                    LPUNKNOWN unknown, REFIID riid,
                                                    LPVOID* object)
{
    FIXME("(%p)->(%p, %s, %p): stub\n", iface, unknown, debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_AddDestroyCallback(IDirect3DRMTexture2* iface,
                                                                 D3DRMOBJECTCALLBACK cb,
                                                                 LPVOID argument)
{
    FIXME("(%p)->(%p, %p): stub\n", iface, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_DeleteDestroyCallback(IDirect3DRMTexture2* iface,
                                                                    D3DRMOBJECTCALLBACK cb,
                                                                    LPVOID argument)
{
    FIXME("(%p)->(%p, %p): stub\n", iface, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_SetAppData(IDirect3DRMTexture2* iface,
                                                          DWORD data)
{
    FIXME("(%p)->(%u): stub\n", iface, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMTexture2Impl_GetAppData(IDirect3DRMTexture2* iface)
{
    FIXME("(%p)->(): stub\n", iface);

    return 0;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_SetName(IDirect3DRMTexture2* iface, LPCSTR name)
{
    FIXME("(%p)->(%s): stub\n", iface, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_GetName(IDirect3DRMTexture2* iface,
                                                      LPDWORD size, LPSTR name)
{
    FIXME("(%p)->(%p, %p): stub\n", iface, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_GetClassName(IDirect3DRMTexture2* iface,
                                                           LPDWORD size, LPSTR name)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, size, name);

    return IDirect3DRMTexture3_GetClassName(&This->IDirect3DRMTexture3_iface, size, name);
}

/*** IDirect3DRMTexture3 methods ***/
static HRESULT WINAPI IDirect3DRMTexture2Impl_InitFromFile(IDirect3DRMTexture2* iface, const char *filename)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, debugstr_a(filename));

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_InitFromSurface(IDirect3DRMTexture2* iface, LPDIRECTDRAWSURFACE surface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, surface);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_InitFromResource(IDirect3DRMTexture2* iface, HRSRC resource)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, resource);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_Changed(IDirect3DRMTexture2* iface, BOOL pixels, BOOL palette)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%u, %u): stub\n", iface, This, pixels, palette);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_SetColors(IDirect3DRMTexture2* iface, DWORD max_colors)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, max_colors);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_SetShades(IDirect3DRMTexture2* iface, DWORD max_shades)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, max_shades);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_SetDecalSize(IDirect3DRMTexture2* iface, D3DVALUE width, D3DVALUE height)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%f, %f): stub\n", iface, This, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_SetDecalOrigin(IDirect3DRMTexture2* iface, LONG x, LONG y)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%d, %d): stub\n", iface, This, x, y);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_SetDecalScale(IDirect3DRMTexture2* iface, DWORD scale)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, scale);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_SetDecalTransparency(IDirect3DRMTexture2* iface, BOOL transparency)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, transparency);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_SetDecalTransparentColor(IDirect3DRMTexture2* iface, D3DCOLOR color)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%x): stub\n", iface, This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_GetDecalSize(IDirect3DRMTexture2* iface, D3DVALUE* width, D3DVALUE* height)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, height, width);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_GetDecalOrigin(IDirect3DRMTexture2* iface, LONG* x, LONG* y)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, x, y);

    return E_NOTIMPL;
}

static D3DRMIMAGE* WINAPI IDirect3DRMTexture2Impl_GetImage(IDirect3DRMTexture2* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return NULL;
}

static DWORD WINAPI IDirect3DRMTexture2Impl_GetShades(IDirect3DRMTexture2* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static DWORD WINAPI IDirect3DRMTexture2Impl_GetColors(IDirect3DRMTexture2* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static DWORD WINAPI IDirect3DRMTexture2Impl_GetDecalScale(IDirect3DRMTexture2* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static BOOL WINAPI IDirect3DRMTexture2Impl_GetDecalTransparency(IDirect3DRMTexture2* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return FALSE;
}

static D3DCOLOR WINAPI IDirect3DRMTexture2Impl_GetDecalTransparentColor(IDirect3DRMTexture2* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

/*** IDirect3DRMTexture2 methods ***/
static HRESULT WINAPI IDirect3DRMTexture2Impl_InitFromImage(IDirect3DRMTexture2* iface, LPD3DRMIMAGE image)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, image);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_InitFromResource2(IDirect3DRMTexture2* iface, HMODULE module,
                                                               LPCSTR /* LPCTSTR */ name, LPCSTR /* LPCTSTR */ type)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%p, %s, %s): stub\n", iface, This, module, debugstr_a(name), debugstr_a(type));

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture2Impl_GenerateMIPMap(IDirect3DRMTexture2* iface, DWORD flags)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture2(iface);

    FIXME("(%p/%p)->(%x): stub\n", iface, This, flags);

    return E_NOTIMPL;
}

static const struct IDirect3DRMTexture2Vtbl Direct3DRMTexture2_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMTexture2Impl_QueryInterface,
    IDirect3DRMTexture2Impl_AddRef,
    IDirect3DRMTexture2Impl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMTexture2Impl_Clone,
    IDirect3DRMTexture2Impl_AddDestroyCallback,
    IDirect3DRMTexture2Impl_DeleteDestroyCallback,
    IDirect3DRMTexture2Impl_SetAppData,
    IDirect3DRMTexture2Impl_GetAppData,
    IDirect3DRMTexture2Impl_SetName,
    IDirect3DRMTexture2Impl_GetName,
    IDirect3DRMTexture2Impl_GetClassName,
    /*** IDirect3DRMTexture methods ***/
    IDirect3DRMTexture2Impl_InitFromFile,
    IDirect3DRMTexture2Impl_InitFromSurface,
    IDirect3DRMTexture2Impl_InitFromResource,
    IDirect3DRMTexture2Impl_Changed,
    IDirect3DRMTexture2Impl_SetColors,
    IDirect3DRMTexture2Impl_SetShades,
    IDirect3DRMTexture2Impl_SetDecalSize,
    IDirect3DRMTexture2Impl_SetDecalOrigin,
    IDirect3DRMTexture2Impl_SetDecalScale,
    IDirect3DRMTexture2Impl_SetDecalTransparency,
    IDirect3DRMTexture2Impl_SetDecalTransparentColor,
    IDirect3DRMTexture2Impl_GetDecalSize,
    IDirect3DRMTexture2Impl_GetDecalOrigin,
    IDirect3DRMTexture2Impl_GetImage,
    IDirect3DRMTexture2Impl_GetShades,
    IDirect3DRMTexture2Impl_GetColors,
    IDirect3DRMTexture2Impl_GetDecalScale,
    IDirect3DRMTexture2Impl_GetDecalTransparency,
    IDirect3DRMTexture2Impl_GetDecalTransparentColor,
    /*** IDirect3DRMTexture2 methods ***/
    IDirect3DRMTexture2Impl_InitFromImage,
    IDirect3DRMTexture2Impl_InitFromResource2,
    IDirect3DRMTexture2Impl_GenerateMIPMap
};

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMTexture3Impl_QueryInterface(IDirect3DRMTexture3* iface,
                                                           REFIID riid, void** object)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), object);

    *object = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirect3DRMTexture) ||
        IsEqualGUID(riid, &IID_IDirect3DRMTexture2))
    {
        *object = &This->IDirect3DRMTexture2_iface;
    }
    else if IsEqualGUID(riid, &IID_IDirect3DRMTexture3)
    {
        *object = &This->IDirect3DRMTexture3_iface;
    }
    else
    {
        FIXME("Interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDirect3DRMTexture3_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI IDirect3DRMTexture3Impl_AddRef(IDirect3DRMTexture3* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirect3DRMTexture3Impl_Release(IDirect3DRMTexture3* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", iface, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMTexture3Impl_Clone(IDirect3DRMTexture3* iface,
                                                     LPUNKNOWN unknown, REFIID riid,
                                                     LPVOID* object)
{
    FIXME("(%p)->(%p, %s, %p): stub\n", iface, unknown, debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_AddDestroyCallback(IDirect3DRMTexture3* iface,
                                                                  D3DRMOBJECTCALLBACK cb,
                                                                  LPVOID argument)
{
    FIXME("(%p)->(%p, %p): stub\n", iface, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_DeleteDestroyCallback(IDirect3DRMTexture3* iface,
                                                                     D3DRMOBJECTCALLBACK cb,
                                                                     LPVOID argument)
{
    FIXME("(%p)->(%p, %p): stub\n", iface, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_SetAppData(IDirect3DRMTexture3* iface,
                                                          DWORD data)
{
    FIXME("(%p)->(%u): stub\n", iface, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMTexture3Impl_GetAppData(IDirect3DRMTexture3* iface)
{
    FIXME("(%p)->(): stub\n", iface);

    return 0;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_SetName(IDirect3DRMTexture3* iface, LPCSTR name)
{
    FIXME("(%p)->(%s): stub\n", iface, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_GetName(IDirect3DRMTexture3* iface,
                                                       LPDWORD size, LPSTR name)
{
    FIXME("(%p)->(%p, %p): stub\n", iface, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_GetClassName(IDirect3DRMTexture3* iface,
                                                           LPDWORD size, LPSTR name)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, size, name);

    if (!size || *size < strlen("Texture") || !name)
        return E_INVALIDARG;

    strcpy(name, "Texture");
    *size = sizeof("Texture");

    return D3DRM_OK;
}

/*** IDirect3DRMTexture3 methods ***/
static HRESULT WINAPI IDirect3DRMTexture3Impl_InitFromFile(IDirect3DRMTexture3* iface, const char *filename)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, debugstr_a(filename));

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_InitFromSurface(IDirect3DRMTexture3* iface, LPDIRECTDRAWSURFACE surface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, surface);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_InitFromResource(IDirect3DRMTexture3* iface, HRSRC resource)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, resource);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_Changed(IDirect3DRMTexture3* iface, DWORD flags, DWORD nb_rects, LPRECT rects)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%x, %u, %p): stub\n", iface, This, flags, nb_rects, rects);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_SetColors(IDirect3DRMTexture3* iface, DWORD max_colors)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, max_colors);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_SetShades(IDirect3DRMTexture3* iface, DWORD max_shades)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, max_shades);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_SetDecalSize(IDirect3DRMTexture3* iface, D3DVALUE width, D3DVALUE height)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%f, %f): stub\n", iface, This, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_SetDecalOrigin(IDirect3DRMTexture3* iface, LONG x, LONG y)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%d, %d): stub\n", iface, This, x, y);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_SetDecalScale(IDirect3DRMTexture3* iface, DWORD scale)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, scale);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_SetDecalTransparency(IDirect3DRMTexture3* iface, BOOL transparency)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, transparency);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_SetDecalTransparentColor(IDirect3DRMTexture3* iface, D3DCOLOR color)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%x): stub\n", iface, This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_GetDecalSize(IDirect3DRMTexture3* iface, D3DVALUE* width, D3DVALUE* height)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, height, width);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_GetDecalOrigin(IDirect3DRMTexture3* iface, LONG* x, LONG* y)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, x, y);

    return E_NOTIMPL;
}

static D3DRMIMAGE* WINAPI IDirect3DRMTexture3Impl_GetImage(IDirect3DRMTexture3* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return NULL;
}

static DWORD WINAPI IDirect3DRMTexture3Impl_GetShades(IDirect3DRMTexture3* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static DWORD WINAPI IDirect3DRMTexture3Impl_GetColors(IDirect3DRMTexture3* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static DWORD WINAPI IDirect3DRMTexture3Impl_GetDecalScale(IDirect3DRMTexture3* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static BOOL WINAPI IDirect3DRMTexture3Impl_GetDecalTransparency(IDirect3DRMTexture3* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return FALSE;
}

static D3DCOLOR WINAPI IDirect3DRMTexture3Impl_GetDecalTransparentColor(IDirect3DRMTexture3* iface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_InitFromImage(IDirect3DRMTexture3* iface, LPD3DRMIMAGE image)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, image);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_InitFromResource2(IDirect3DRMTexture3* iface, HMODULE module,
                                                               LPCSTR /* LPCTSTR */ name, LPCSTR /* LPCTSTR */ type)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%p, %s, %s): stub\n", iface, This, module, debugstr_a(name), debugstr_a(type));

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_GenerateMIPMap(IDirect3DRMTexture3* iface, DWORD flags)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%x): stub\n", iface, This, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_GetSurface(IDirect3DRMTexture3* iface, DWORD flags, LPDIRECTDRAWSURFACE* surface)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%x, %p): stub\n", iface, This, flags, surface);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_SetCacheOptions(IDirect3DRMTexture3* iface, LONG importance, DWORD flags)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%d, %x): stub\n", iface, This, importance, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_GetCacheOptions(IDirect3DRMTexture3* iface, LPLONG importance, LPDWORD flags)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, importance, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_SetDownsampleCallback(IDirect3DRMTexture3* iface, D3DRMDOWNSAMPLECALLBACK callback, LPVOID arg)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, callback, arg);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMTexture3Impl_SetValidationCallback(IDirect3DRMTexture3* iface, D3DRMVALIDATIONCALLBACK callback, LPVOID arg)
{
    IDirect3DRMTextureImpl *This = impl_from_IDirect3DRMTexture3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, callback, arg);

    return E_NOTIMPL;
}

static const struct IDirect3DRMTexture3Vtbl Direct3DRMTexture3_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMTexture3Impl_QueryInterface,
    IDirect3DRMTexture3Impl_AddRef,
    IDirect3DRMTexture3Impl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMTexture3Impl_Clone,
    IDirect3DRMTexture3Impl_AddDestroyCallback,
    IDirect3DRMTexture3Impl_DeleteDestroyCallback,
    IDirect3DRMTexture3Impl_SetAppData,
    IDirect3DRMTexture3Impl_GetAppData,
    IDirect3DRMTexture3Impl_SetName,
    IDirect3DRMTexture3Impl_GetName,
    IDirect3DRMTexture3Impl_GetClassName,
    /*** IDirect3DRMTexture3 methods ***/
    IDirect3DRMTexture3Impl_InitFromFile,
    IDirect3DRMTexture3Impl_InitFromSurface,
    IDirect3DRMTexture3Impl_InitFromResource,
    IDirect3DRMTexture3Impl_Changed,
    IDirect3DRMTexture3Impl_SetColors,
    IDirect3DRMTexture3Impl_SetShades,
    IDirect3DRMTexture3Impl_SetDecalSize,
    IDirect3DRMTexture3Impl_SetDecalOrigin,
    IDirect3DRMTexture3Impl_SetDecalScale,
    IDirect3DRMTexture3Impl_SetDecalTransparency,
    IDirect3DRMTexture3Impl_SetDecalTransparentColor,
    IDirect3DRMTexture3Impl_GetDecalSize,
    IDirect3DRMTexture3Impl_GetDecalOrigin,
    IDirect3DRMTexture3Impl_GetImage,
    IDirect3DRMTexture3Impl_GetShades,
    IDirect3DRMTexture3Impl_GetColors,
    IDirect3DRMTexture3Impl_GetDecalScale,
    IDirect3DRMTexture3Impl_GetDecalTransparency,
    IDirect3DRMTexture3Impl_GetDecalTransparentColor,
    IDirect3DRMTexture3Impl_InitFromImage,
    IDirect3DRMTexture3Impl_InitFromResource2,
    IDirect3DRMTexture3Impl_GenerateMIPMap,
    IDirect3DRMTexture3Impl_GetSurface,
    IDirect3DRMTexture3Impl_SetCacheOptions,
    IDirect3DRMTexture3Impl_GetCacheOptions,
    IDirect3DRMTexture3Impl_SetDownsampleCallback,
    IDirect3DRMTexture3Impl_SetValidationCallback
};

HRESULT Direct3DRMTexture_create(REFIID riid, IUnknown** ret_iface)
{
    IDirect3DRMTextureImpl* object;

    TRACE("(%p)\n", ret_iface);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DRMTextureImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->IDirect3DRMTexture2_iface.lpVtbl = &Direct3DRMTexture2_Vtbl;
    object->IDirect3DRMTexture3_iface.lpVtbl = &Direct3DRMTexture3_Vtbl;
    object->ref = 1;

    if (IsEqualGUID(riid, &IID_IDirect3DRMTexture3))
        *ret_iface = (IUnknown*)&object->IDirect3DRMTexture3_iface;
    else
        *ret_iface = (IUnknown*)&object->IDirect3DRMTexture2_iface;

    return S_OK;
}
