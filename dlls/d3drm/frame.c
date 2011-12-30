/*
 * Implementation of IDirect3DRMFrame Interface
 *
 * Copyright 2011 André Hentschel
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
    IDirect3DRMFrame2 IDirect3DRMFrame2_iface;
    LONG ref;
} IDirect3DRMFrameImpl;

static const struct IDirect3DRMFrame2Vtbl Direct3DRMFrame2_Vtbl;

static inline IDirect3DRMFrameImpl *impl_from_IDirect3DRMFrame2(IDirect3DRMFrame2 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMFrameImpl, IDirect3DRMFrame2_iface);
}

HRESULT Direct3DRMFrame_create(IUnknown** ppObj)
{
    IDirect3DRMFrameImpl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DRMFrameImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->IDirect3DRMFrame2_iface.lpVtbl = &Direct3DRMFrame2_Vtbl;
    object->ref = 1;

    *ppObj = (IUnknown*)object;

    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMFrame2Impl_QueryInterface(IDirect3DRMFrame2* iface,
                                                           REFIID riid, void** object)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    TRACE("(%p/%p)->(%s, %p)\n", iface, This, debugstr_guid(riid), object);

    *object = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IDirect3DRMFrame) ||
       IsEqualGUID(riid, &IID_IDirect3DRMFrame2))
    {
        *object = &This->IDirect3DRMFrame2_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDirect3DRMFrame2_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI IDirect3DRMFrame2Impl_AddRef(IDirect3DRMFrame2* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IDirect3DRMFrame2Impl_Release(IDirect3DRMFrame2* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)\n", This);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMFrame2Impl_Clone(IDirect3DRMFrame2* iface,
                                                  LPUNKNOWN unkwn, REFIID riid,
                                                  LPVOID* object)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p, %s, %p): stub\n", iface, This, unkwn, debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_AddDestroyCallback(IDirect3DRMFrame2* iface,
                                                               D3DRMOBJECTCALLBACK cb,
                                                               LPVOID argument)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_DeleteDestroyCallback(IDirect3DRMFrame2* iface,
                                                                  D3DRMOBJECTCALLBACK cb,
                                                                  LPVOID argument)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetAppData(IDirect3DRMFrame2* iface,
                                                       DWORD data)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMFrame2Impl_GetAppData(IDirect3DRMFrame2* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetName(IDirect3DRMFrame2* iface, LPCSTR name)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetName(IDirect3DRMFrame2* iface,
                                                    LPDWORD size, LPSTR name)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetClassName(IDirect3DRMFrame2* iface,
                                                         LPDWORD size, LPSTR name)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, size, name);

    return E_NOTIMPL;
}

/*** IDirect3DRMFrame methods ***/
static HRESULT WINAPI IDirect3DRMFrame2Impl_AddChild(IDirect3DRMFrame2* iface,
                                                     LPDIRECT3DRMFRAME child)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, child);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_AddLight(IDirect3DRMFrame2* iface,
                                                     LPDIRECT3DRMLIGHT light)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, light);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_AddMoveCallback(IDirect3DRMFrame2* iface,
                                                            D3DRMFRAMEMOVECALLBACK cb, VOID *arg)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, cb, arg);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_AddTransform(IDirect3DRMFrame2* iface,
                                                         D3DRMCOMBINETYPE type,
                                                         D3DRMMATRIX4D matrix)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%u,%p): stub\n", iface, This, type, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_AddTranslation(IDirect3DRMFrame2* iface,
                                                           D3DRMCOMBINETYPE type,
                                                           D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%u,%f,%f,%f): stub\n", iface, This, type, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_AddScale(IDirect3DRMFrame2* iface,
                                                     D3DRMCOMBINETYPE type,
                                                     D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%u,%f,%f,%f): stub\n", iface, This, type, sx, sy, sz);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_AddRotation(IDirect3DRMFrame2* iface,
                                                        D3DRMCOMBINETYPE type,
                                                        D3DVALUE x, D3DVALUE y, D3DVALUE z,
                                                        D3DVALUE theta)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%u,%f,%f,%f,%f): stub\n", iface, This, type, x, y, z, theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_AddVisual(IDirect3DRMFrame2* iface,
                                                      LPDIRECT3DRMVISUAL vis)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, vis);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetChildren(IDirect3DRMFrame2* iface,
                                                        LPDIRECT3DRMFRAMEARRAY *children)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, children);

    return E_NOTIMPL;
}

static D3DCOLOR WINAPI IDirect3DRMFrame2Impl_GetColor(IDirect3DRMFrame2* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetLights(IDirect3DRMFrame2* iface,
                                                      LPDIRECT3DRMLIGHTARRAY *lights)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, lights);

    return E_NOTIMPL;
}

static D3DRMMATERIALMODE WINAPI IDirect3DRMFrame2Impl_GetMaterialMode(IDirect3DRMFrame2* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return D3DRMMATERIAL_FROMPARENT;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetParent(IDirect3DRMFrame2* iface,
                                                      LPDIRECT3DRMFRAME * frame)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, frame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetPosition(IDirect3DRMFrame2* iface,
                                                        LPDIRECT3DRMFRAME reference,
                                                        LPD3DVECTOR return_position)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, reference, return_position);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetRotation(IDirect3DRMFrame2* iface,
                                                        LPDIRECT3DRMFRAME reference,
                                                        LPD3DVECTOR axis, LPD3DVALUE return_theta)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%p,%p): stub\n", iface, This, reference, axis, return_theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetScene(IDirect3DRMFrame2* iface,
                                                     LPDIRECT3DRMFRAME * frame)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, frame);

    return E_NOTIMPL;
}

static D3DRMSORTMODE WINAPI IDirect3DRMFrame2Impl_GetSortMode(IDirect3DRMFrame2* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return D3DRMSORT_FROMPARENT;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetTexture(IDirect3DRMFrame2* iface,
                                                       LPDIRECT3DRMTEXTURE * tex)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, tex);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetTransform(IDirect3DRMFrame2* iface,
                                                         D3DRMMATRIX4D return_matrix)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, return_matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetVelocity(IDirect3DRMFrame2* iface,
                                                        LPDIRECT3DRMFRAME reference,
                                                        LPD3DVECTOR return_velocity,
                                                        BOOL with_rotation)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%p,%d): stub\n", iface, This, reference, return_velocity, with_rotation);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetOrientation(IDirect3DRMFrame2* iface,
                                                           LPDIRECT3DRMFRAME reference,
                                                           LPD3DVECTOR dir, LPD3DVECTOR up)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%p,%p): stub\n", iface, This, reference, dir, up);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetVisuals(IDirect3DRMFrame2* iface,
                                                       LPDIRECT3DRMVISUALARRAY *visuals)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, visuals);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetTextureTopology(IDirect3DRMFrame2* iface,
                                                               BOOL *wrap_u, BOOL *wrap_v)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_InverseTransform(IDirect3DRMFrame2* iface,
                                                             D3DVECTOR *d, D3DVECTOR *s)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_Load(IDirect3DRMFrame2* iface, LPVOID filename,
                                                 LPVOID name, D3DRMLOADOPTIONS loadflags,
                                                 D3DRMLOADTEXTURECALLBACK cb, LPVOID lpArg)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%p,%u,%p,%p): stub\n", iface, This, filename, name, loadflags, cb, lpArg);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_LookAt(IDirect3DRMFrame2* iface,
                                                   LPDIRECT3DRMFRAME target,
                                                   LPDIRECT3DRMFRAME reference,
                                                   D3DRMFRAMECONSTRAINT constraint)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%p,%u): stub\n", iface, This, target, reference, constraint);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_Move(IDirect3DRMFrame2* iface, D3DVALUE delta)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%f): stub\n", iface, This, delta);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_DeleteChild(IDirect3DRMFrame2* iface,
                                                        LPDIRECT3DRMFRAME frame)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, frame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_DeleteLight(IDirect3DRMFrame2* iface,
                                                        LPDIRECT3DRMLIGHT light)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, light);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_DeleteMoveCallback(IDirect3DRMFrame2* iface,
                                                               D3DRMFRAMEMOVECALLBACK cb, VOID *arg)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, cb, arg);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_DeleteVisual(IDirect3DRMFrame2* iface,
                                                         LPDIRECT3DRMVISUAL vis)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, vis);

    return E_NOTIMPL;
}

static D3DCOLOR WINAPI IDirect3DRMFrame2Impl_GetSceneBackground(IDirect3DRMFrame2* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetSceneBackgroundDepth(IDirect3DRMFrame2* iface,
                                                                    LPDIRECTDRAWSURFACE * surface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, surface);

    return E_NOTIMPL;
}

static D3DCOLOR WINAPI IDirect3DRMFrame2Impl_GetSceneFogColor(IDirect3DRMFrame2* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static BOOL WINAPI IDirect3DRMFrame2Impl_GetSceneFogEnable(IDirect3DRMFrame2* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return FALSE;
}

static D3DRMFOGMODE WINAPI IDirect3DRMFrame2Impl_GetSceneFogMode(IDirect3DRMFrame2* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return D3DRMFOG_LINEAR;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetSceneFogParams(IDirect3DRMFrame2* iface,
                                                              D3DVALUE *return_start,
                                                              D3DVALUE *return_end,
                                                              D3DVALUE *return_density)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%p,%p): stub\n", iface, This, return_start, return_end, return_density);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetSceneBackground(IDirect3DRMFrame2* iface,
                                                               D3DCOLOR color)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetSceneBackgroundRGB(IDirect3DRMFrame2* iface,
                                                                  D3DVALUE red, D3DVALUE green,
                                                                  D3DVALUE blue)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%f,%f,%f): stub\n", iface, This, red, green, blue);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetSceneBackgroundDepth(IDirect3DRMFrame2* iface,
                                                                    LPDIRECTDRAWSURFACE surface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, surface);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetSceneBackgroundImage(IDirect3DRMFrame2* iface,
                                                                    LPDIRECT3DRMTEXTURE texture)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetSceneFogEnable(IDirect3DRMFrame2* iface, BOOL enable)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%d): stub\n", iface, This, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetSceneFogColor(IDirect3DRMFrame2* iface,
                                                             D3DCOLOR color)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetSceneFogMode(IDirect3DRMFrame2* iface,
                                                            D3DRMFOGMODE mode)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetSceneFogParams(IDirect3DRMFrame2* iface,
                                                              D3DVALUE start, D3DVALUE end,
                                                              D3DVALUE density)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%f,%f,%f): stub\n", iface, This, start, end, density);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetColor(IDirect3DRMFrame2* iface, D3DCOLOR color)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetColorRGB(IDirect3DRMFrame2* iface, D3DVALUE red,
                                                        D3DVALUE green, D3DVALUE blue)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%f,%f,%f): stub\n", iface, This, red, green, blue);

    return E_NOTIMPL;
}

static D3DRMZBUFFERMODE WINAPI IDirect3DRMFrame2Impl_GetZbufferMode(IDirect3DRMFrame2* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return D3DRMZBUFFER_FROMPARENT;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetMaterialMode(IDirect3DRMFrame2* iface,
                                                            D3DRMMATERIALMODE mode)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetOrientation(IDirect3DRMFrame2* iface,
                                                           LPDIRECT3DRMFRAME reference,
                                                           D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
                                                           D3DVALUE ux, D3DVALUE uy, D3DVALUE uz )
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%f,%f,%f,%f,%f,%f): stub\n", iface, This, reference,
          dx, dy, dz, ux, uy, uz);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetPosition(IDirect3DRMFrame2* iface,
                                                        LPDIRECT3DRMFRAME reference,
                                                        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%f,%f,%f): stub\n", iface, This, reference, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetRotation(IDirect3DRMFrame2* iface,
                                                        LPDIRECT3DRMFRAME reference,
                                                        D3DVALUE x, D3DVALUE y, D3DVALUE z,
                                                        D3DVALUE theta)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%f,%f,%f,%f): stub\n", iface, This, reference, x, y, z, theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetSortMode(IDirect3DRMFrame2* iface,
                                                        D3DRMSORTMODE mode)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetTexture(IDirect3DRMFrame2* iface,
                                                       LPDIRECT3DRMTEXTURE texture)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetTextureTopology(IDirect3DRMFrame2* iface,
                                                               BOOL wrap_u, BOOL wrap_v)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%d,%d): stub\n", iface, This, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetVelocity(IDirect3DRMFrame2* iface,
                                                        LPDIRECT3DRMFRAME reference,
                                                        D3DVALUE x, D3DVALUE y, D3DVALUE z,
                                                        BOOL with_rotation)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%f,%f,%f,%d): stub\n", iface, This, reference, x, y, z, with_rotation);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_SetZbufferMode(IDirect3DRMFrame2* iface,
                                                           D3DRMZBUFFERMODE mode)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_Transform(IDirect3DRMFrame2* iface, D3DVECTOR *d,
                                                      D3DVECTOR *s)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, d, s);

    return E_NOTIMPL;
}

/*** IDirect3DRMFrame2 methods ***/
static HRESULT WINAPI IDirect3DRMFrame2Impl_AddMoveCallback2(IDirect3DRMFrame2* iface,
                                                             D3DRMFRAMEMOVECALLBACK cb, VOID *arg,
                                                             DWORD flags)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%p,%u): stub\n", iface, This, cb, arg, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetBox(IDirect3DRMFrame2* iface, LPD3DRMBOX box)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, box);

    return E_NOTIMPL;
}

static BOOL WINAPI IDirect3DRMFrame2Impl_GetBoxEnable(IDirect3DRMFrame2* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetAxes(IDirect3DRMFrame2* iface,
                                                    LPD3DVECTOR dir, LPD3DVECTOR up)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, dir, up);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetMaterial(IDirect3DRMFrame2* iface,
                                                        LPDIRECT3DRMMATERIAL *material)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, material);

    return E_NOTIMPL;
}

static BOOL WINAPI IDirect3DRMFrame2Impl_GetInheritAxes(IDirect3DRMFrame2* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame2Impl_GetHierarchyBox(IDirect3DRMFrame2* iface,
                                                            LPD3DRMBOX box)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, box);

    return E_NOTIMPL;
}

static const struct IDirect3DRMFrame2Vtbl Direct3DRMFrame2_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMFrame2Impl_QueryInterface,
    IDirect3DRMFrame2Impl_AddRef,
    IDirect3DRMFrame2Impl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMFrame2Impl_Clone,
    IDirect3DRMFrame2Impl_AddDestroyCallback,
    IDirect3DRMFrame2Impl_DeleteDestroyCallback,
    IDirect3DRMFrame2Impl_SetAppData,
    IDirect3DRMFrame2Impl_GetAppData,
    IDirect3DRMFrame2Impl_SetName,
    IDirect3DRMFrame2Impl_GetName,
    IDirect3DRMFrame2Impl_GetClassName,
    /*** IDirect3DRMFrame methods ***/
    IDirect3DRMFrame2Impl_AddChild,
    IDirect3DRMFrame2Impl_AddLight,
    IDirect3DRMFrame2Impl_AddMoveCallback,
    IDirect3DRMFrame2Impl_AddTransform,
    IDirect3DRMFrame2Impl_AddTranslation,
    IDirect3DRMFrame2Impl_AddScale,
    IDirect3DRMFrame2Impl_AddRotation,
    IDirect3DRMFrame2Impl_AddVisual,
    IDirect3DRMFrame2Impl_GetChildren,
    IDirect3DRMFrame2Impl_GetColor,
    IDirect3DRMFrame2Impl_GetLights,
    IDirect3DRMFrame2Impl_GetMaterialMode,
    IDirect3DRMFrame2Impl_GetParent,
    IDirect3DRMFrame2Impl_GetPosition,
    IDirect3DRMFrame2Impl_GetRotation,
    IDirect3DRMFrame2Impl_GetScene,
    IDirect3DRMFrame2Impl_GetSortMode,
    IDirect3DRMFrame2Impl_GetTexture,
    IDirect3DRMFrame2Impl_GetTransform,
    IDirect3DRMFrame2Impl_GetVelocity,
    IDirect3DRMFrame2Impl_GetOrientation,
    IDirect3DRMFrame2Impl_GetVisuals,
    IDirect3DRMFrame2Impl_GetTextureTopology,
    IDirect3DRMFrame2Impl_InverseTransform,
    IDirect3DRMFrame2Impl_Load,
    IDirect3DRMFrame2Impl_LookAt,
    IDirect3DRMFrame2Impl_Move,
    IDirect3DRMFrame2Impl_DeleteChild,
    IDirect3DRMFrame2Impl_DeleteLight,
    IDirect3DRMFrame2Impl_DeleteMoveCallback,
    IDirect3DRMFrame2Impl_DeleteVisual,
    IDirect3DRMFrame2Impl_GetSceneBackground,
    IDirect3DRMFrame2Impl_GetSceneBackgroundDepth,
    IDirect3DRMFrame2Impl_GetSceneFogColor,
    IDirect3DRMFrame2Impl_GetSceneFogEnable,
    IDirect3DRMFrame2Impl_GetSceneFogMode,
    IDirect3DRMFrame2Impl_GetSceneFogParams,
    IDirect3DRMFrame2Impl_SetSceneBackground,
    IDirect3DRMFrame2Impl_SetSceneBackgroundRGB,
    IDirect3DRMFrame2Impl_SetSceneBackgroundDepth,
    IDirect3DRMFrame2Impl_SetSceneBackgroundImage,
    IDirect3DRMFrame2Impl_SetSceneFogEnable,
    IDirect3DRMFrame2Impl_SetSceneFogColor,
    IDirect3DRMFrame2Impl_SetSceneFogMode,
    IDirect3DRMFrame2Impl_SetSceneFogParams,
    IDirect3DRMFrame2Impl_SetColor,
    IDirect3DRMFrame2Impl_SetColorRGB,
    IDirect3DRMFrame2Impl_GetZbufferMode,
    IDirect3DRMFrame2Impl_SetMaterialMode,
    IDirect3DRMFrame2Impl_SetOrientation,
    IDirect3DRMFrame2Impl_SetPosition,
    IDirect3DRMFrame2Impl_SetRotation,
    IDirect3DRMFrame2Impl_SetSortMode,
    IDirect3DRMFrame2Impl_SetTexture,
    IDirect3DRMFrame2Impl_SetTextureTopology,
    IDirect3DRMFrame2Impl_SetVelocity,
    IDirect3DRMFrame2Impl_SetZbufferMode,
    IDirect3DRMFrame2Impl_Transform,
    /*** IDirect3DRMFrame2 methods ***/
    IDirect3DRMFrame2Impl_AddMoveCallback2,
    IDirect3DRMFrame2Impl_GetBox,
    IDirect3DRMFrame2Impl_GetBoxEnable,
    IDirect3DRMFrame2Impl_GetAxes,
    IDirect3DRMFrame2Impl_GetMaterial,
    IDirect3DRMFrame2Impl_GetInheritAxes,
    IDirect3DRMFrame2Impl_GetHierarchyBox
};
