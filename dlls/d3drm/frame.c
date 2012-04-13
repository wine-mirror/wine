/*
 * Implementation of IDirect3DRMFrame Interface
 *
 * Copyright 2011, 2012 André Hentschel
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

#include <assert.h>
#include "wine/debug.h"

#define COBJMACROS

#include "winbase.h"
#include "wingdi.h"

#include "d3drm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3drm);

typedef struct {
    IDirect3DRMFrame2 IDirect3DRMFrame2_iface;
    IDirect3DRMFrame3 IDirect3DRMFrame3_iface;
    LONG ref;
    ULONG nb_children;
    ULONG children_capacity;
    IDirect3DRMFrame3** children;
} IDirect3DRMFrameImpl;

static inline IDirect3DRMFrameImpl *impl_from_IDirect3DRMFrame2(IDirect3DRMFrame2 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMFrameImpl, IDirect3DRMFrame2_iface);
}

static inline IDirect3DRMFrameImpl *impl_from_IDirect3DRMFrame3(IDirect3DRMFrame3 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMFrameImpl, IDirect3DRMFrame3_iface);
}

static inline IDirect3DRMFrameImpl *unsafe_impl_from_IDirect3DRMFrame2(IDirect3DRMFrame2 *iface);

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
    else if(IsEqualGUID(riid, &IID_IDirect3DRMFrame3))
    {
        *object = &This->IDirect3DRMFrame3_iface;
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
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirect3DRMFrame2Impl_Release(IDirect3DRMFrame2* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    ULONG i;

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    if (!ref)
    {
        for (i = 0; i < This->nb_children; i++)
            IDirect3DRMFrame3_Release(This->children[i]);
        HeapFree(GetProcessHeap(), 0, This->children);
        HeapFree(GetProcessHeap(), 0, This);
    }

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
    IDirect3DRMFrameImpl *frame;

    TRACE("(%p/%p)->(%p)\n", iface, This, child);

    frame = unsafe_impl_from_IDirect3DRMFrame2((LPDIRECT3DRMFRAME2)child);

    if (!frame)
        return D3DRMERR_BADOBJECT;

    return IDirect3DRMFrame3_AddChild(&This->IDirect3DRMFrame3_iface, &frame->IDirect3DRMFrame3_iface);
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
    IDirect3DRMFrameImpl *child;

    TRACE("(%p/%p)->(%p)\n", iface, This, frame);

    child = unsafe_impl_from_IDirect3DRMFrame2((LPDIRECT3DRMFRAME2)frame);

    if (!child)
        return D3DRMERR_BADOBJECT;

    return IDirect3DRMFrame3_DeleteChild(&This->IDirect3DRMFrame3_iface, &child->IDirect3DRMFrame3_iface);
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

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMFrame3Impl_QueryInterface(IDirect3DRMFrame3* iface,
                                                           REFIID riid, void** object)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);
    return IDirect3DRMFrame_QueryInterface(&This->IDirect3DRMFrame2_iface, riid, object);
}

static ULONG WINAPI IDirect3DRMFrame3Impl_AddRef(IDirect3DRMFrame3* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);
    return IDirect3DRMFrame2_AddRef(&This->IDirect3DRMFrame2_iface);
}

static ULONG WINAPI IDirect3DRMFrame3Impl_Release(IDirect3DRMFrame3* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);
    return IDirect3DRMFrame2_Release(&This->IDirect3DRMFrame2_iface);
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMFrame3Impl_Clone(IDirect3DRMFrame3* iface,
                                                  LPUNKNOWN unkwn, REFIID riid,
                                                  LPVOID* object)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p, %s, %p): stub\n", iface, This, unkwn, debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_AddDestroyCallback(IDirect3DRMFrame3* iface,
                                                               D3DRMOBJECTCALLBACK cb,
                                                               LPVOID argument)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_DeleteDestroyCallback(IDirect3DRMFrame3* iface,
                                                                  D3DRMOBJECTCALLBACK cb,
                                                                  LPVOID argument)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetAppData(IDirect3DRMFrame3* iface,
                                                       DWORD data)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMFrame3Impl_GetAppData(IDirect3DRMFrame3* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetName(IDirect3DRMFrame3* iface, LPCSTR name)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%s): stub\n", iface, This, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetName(IDirect3DRMFrame3* iface,
                                                    LPDWORD size, LPSTR name)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetClassName(IDirect3DRMFrame3* iface,
                                                         LPDWORD size, LPSTR name)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p, %p): stub\n", iface, This, size, name);

    return E_NOTIMPL;
}

/*** IDirect3DRMFrame methods ***/
static HRESULT WINAPI IDirect3DRMFrame3Impl_AddChild(IDirect3DRMFrame3* iface,
                                                     LPDIRECT3DRMFRAME3 child)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);
    ULONG i;
    IDirect3DRMFrame3** children;

    TRACE("(%p/%p)->(%p)\n", iface, This, child);

    if (!child)
        return D3DRMERR_BADOBJECT;

    /* Check if already existing and return gracefully without increasing ref count */
    for (i = 0; i < This->nb_children; i++)
        if (This->children[i] == child)
            return D3DRM_OK;

    if ((This->nb_children + 1) > This->children_capacity)
    {
        ULONG new_capacity;

        if (!This->children_capacity)
        {
            new_capacity = 16;
            children = HeapAlloc(GetProcessHeap(), 0, new_capacity * sizeof(IDirect3DRMFrame3*));
        }
        else
        {
            new_capacity = This->children_capacity * 2;
            children = HeapReAlloc(GetProcessHeap(), 0, This->children, new_capacity * sizeof(IDirect3DRMFrame3*));
        }

        if (!children)
            return E_OUTOFMEMORY;

        This->children_capacity = new_capacity;
        This->children = children;
    }

    This->children[This->nb_children++] = child;
    IDirect3DRMFrame3_AddRef(child);

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_AddLight(IDirect3DRMFrame3* iface,
                                                     LPDIRECT3DRMLIGHT light)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, light);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_AddMoveCallback(IDirect3DRMFrame3* iface,
                                                            D3DRMFRAME3MOVECALLBACK cb, VOID *arg,
                                                            DWORD flags)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p,%u): stub\n", iface, This, cb, arg, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_AddTransform(IDirect3DRMFrame3* iface,
                                                         D3DRMCOMBINETYPE type,
                                                         D3DRMMATRIX4D matrix)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u,%p): stub\n", iface, This, type, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_AddTranslation(IDirect3DRMFrame3* iface,
                                                           D3DRMCOMBINETYPE type,
                                                           D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u,%f,%f,%f): stub\n", iface, This, type, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_AddScale(IDirect3DRMFrame3* iface,
                                                     D3DRMCOMBINETYPE type,
                                                     D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u,%f,%f,%f): stub\n", iface, This, type, sx, sy, sz);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_AddRotation(IDirect3DRMFrame3* iface,
                                                        D3DRMCOMBINETYPE type,
                                                        D3DVALUE x, D3DVALUE y, D3DVALUE z,
                                                        D3DVALUE theta)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u,%f,%f,%f,%f): stub\n", iface, This, type, x, y, z, theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_AddVisual(IDirect3DRMFrame3* iface, LPUNKNOWN vis)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, vis);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetChildren(IDirect3DRMFrame3* iface,
                                                        LPDIRECT3DRMFRAMEARRAY *children)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, children);

    return E_NOTIMPL;
}

static D3DCOLOR WINAPI IDirect3DRMFrame3Impl_GetColor(IDirect3DRMFrame3* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetLights(IDirect3DRMFrame3* iface,
                                                      LPDIRECT3DRMLIGHTARRAY *lights)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, lights);

    return E_NOTIMPL;
}

static D3DRMMATERIALMODE WINAPI IDirect3DRMFrame3Impl_GetMaterialMode(IDirect3DRMFrame3* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return D3DRMMATERIAL_FROMPARENT;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetParent(IDirect3DRMFrame3* iface,
                                                      LPDIRECT3DRMFRAME3 * frame)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, frame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetPosition(IDirect3DRMFrame3* iface,
                                                        LPDIRECT3DRMFRAME3 reference,
                                                        LPD3DVECTOR return_position)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, reference, return_position);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetRotation(IDirect3DRMFrame3* iface,
                                                        LPDIRECT3DRMFRAME3 reference,
                                                        LPD3DVECTOR axis, LPD3DVALUE return_theta)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p,%p): stub\n", iface, This, reference, axis, return_theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetScene(IDirect3DRMFrame3* iface,
                                                     LPDIRECT3DRMFRAME3 * frame)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, frame);

    return E_NOTIMPL;
}

static D3DRMSORTMODE WINAPI IDirect3DRMFrame3Impl_GetSortMode(IDirect3DRMFrame3* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return D3DRMSORT_FROMPARENT;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetTexture(IDirect3DRMFrame3* iface,
                                                       LPDIRECT3DRMTEXTURE3 * tex)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, tex);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetTransform(IDirect3DRMFrame3* iface,
                                                         LPDIRECT3DRMFRAME3 reference,
                                                         D3DRMMATRIX4D return_matrix)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, reference, return_matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetVelocity(IDirect3DRMFrame3* iface,
                                                        LPDIRECT3DRMFRAME3 reference,
                                                        LPD3DVECTOR return_velocity,
                                                        BOOL with_rotation)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p,%d): stub\n", iface, This, reference, return_velocity, with_rotation);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetOrientation(IDirect3DRMFrame3* iface,
                                                           LPDIRECT3DRMFRAME3 reference,
                                                           LPD3DVECTOR dir, LPD3DVECTOR up)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p,%p): stub\n", iface, This, reference, dir, up);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetVisuals(IDirect3DRMFrame3* iface, LPDWORD num,
                                                       LPUNKNOWN *visuals)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, num, visuals);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_InverseTransform(IDirect3DRMFrame3* iface,
                                                             D3DVECTOR *d, D3DVECTOR *s)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_Load(IDirect3DRMFrame3* iface, LPVOID filename,
                                                 LPVOID name, D3DRMLOADOPTIONS loadflags,
                                                 D3DRMLOADTEXTURE3CALLBACK cb, LPVOID lpArg)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p,%u,%p,%p): stub\n", iface, This, filename, name, loadflags, cb, lpArg);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_LookAt(IDirect3DRMFrame3* iface,
                                                   LPDIRECT3DRMFRAME3 target,
                                                   LPDIRECT3DRMFRAME3 reference,
                                                   D3DRMFRAMECONSTRAINT constraint)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p,%u): stub\n", iface, This, target, reference, constraint);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_Move(IDirect3DRMFrame3* iface, D3DVALUE delta)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%f): stub\n", iface, This, delta);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_DeleteChild(IDirect3DRMFrame3* iface,
                                                        LPDIRECT3DRMFRAME3 frame)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);
    ULONG i;

    TRACE("(%p/%p)->(%p)\n", iface, This, frame);

    if (!frame)
        return D3DRMERR_BADOBJECT;

    /* Check if child exists */
    for (i = 0; i < This->nb_children; i++)
        if (This->children[i] == frame)
            break;

    if (i == This->nb_children)
        return D3DRMERR_BADVALUE;

    memmove(This->children + i, This->children + i + 1, sizeof(IDirect3DRMFrame3*) * (This->nb_children - 1 - i));
    IDirect3DRMFrame3_Release(frame);
    This->nb_children--;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_DeleteLight(IDirect3DRMFrame3* iface,
                                                        LPDIRECT3DRMLIGHT light)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, light);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_DeleteMoveCallback(IDirect3DRMFrame3* iface,
                                                               D3DRMFRAME3MOVECALLBACK cb,
                                                               VOID *arg)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, cb, arg);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_DeleteVisual(IDirect3DRMFrame3* iface, LPUNKNOWN vis)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, vis);

    return E_NOTIMPL;
}

static D3DCOLOR WINAPI IDirect3DRMFrame3Impl_GetSceneBackground(IDirect3DRMFrame3* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetSceneBackgroundDepth(IDirect3DRMFrame3* iface,
                                                                    LPDIRECTDRAWSURFACE * surface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, surface);

    return E_NOTIMPL;
}

static D3DCOLOR WINAPI IDirect3DRMFrame3Impl_GetSceneFogColor(IDirect3DRMFrame3* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return 0;
}

static BOOL WINAPI IDirect3DRMFrame3Impl_GetSceneFogEnable(IDirect3DRMFrame3* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return FALSE;
}

static D3DRMFOGMODE WINAPI IDirect3DRMFrame3Impl_GetSceneFogMode(IDirect3DRMFrame3* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return D3DRMFOG_LINEAR;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetSceneFogParams(IDirect3DRMFrame3* iface,
                                                              D3DVALUE *return_start,
                                                              D3DVALUE *return_end,
                                                              D3DVALUE *return_density)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p,%p): stub\n", iface, This, return_start, return_end, return_density);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetSceneBackground(IDirect3DRMFrame3* iface,
                                                               D3DCOLOR color)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetSceneBackgroundRGB(IDirect3DRMFrame3* iface,
                                                                  D3DVALUE red, D3DVALUE green,
                                                                  D3DVALUE blue)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%f,%f,%f): stub\n", iface, This, red, green, blue);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetSceneBackgroundDepth(IDirect3DRMFrame3* iface,
                                                                    LPDIRECTDRAWSURFACE surface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, surface);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetSceneBackgroundImage(IDirect3DRMFrame3* iface,
                                                                    LPDIRECT3DRMTEXTURE3 texture)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetSceneFogEnable(IDirect3DRMFrame3* iface, BOOL enable)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%d): stub\n", iface, This, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetSceneFogColor(IDirect3DRMFrame3* iface,
                                                             D3DCOLOR color)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetSceneFogMode(IDirect3DRMFrame3* iface,
                                                            D3DRMFOGMODE mode)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetSceneFogParams(IDirect3DRMFrame3* iface,
                                                              D3DVALUE start, D3DVALUE end,
                                                              D3DVALUE density)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%f,%f,%f): stub\n", iface, This, start, end, density);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetColor(IDirect3DRMFrame3* iface, D3DCOLOR color)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetColorRGB(IDirect3DRMFrame3* iface, D3DVALUE red,
                                                        D3DVALUE green, D3DVALUE blue)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%f,%f,%f): stub\n", iface, This, red, green, blue);

    return E_NOTIMPL;
}

static D3DRMZBUFFERMODE WINAPI IDirect3DRMFrame3Impl_GetZbufferMode(IDirect3DRMFrame3* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return D3DRMZBUFFER_FROMPARENT;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetMaterialMode(IDirect3DRMFrame3* iface,
                                                            D3DRMMATERIALMODE mode)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetOrientation(IDirect3DRMFrame3* iface,
                                                           LPDIRECT3DRMFRAME3 reference,
                                                           D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
                                                           D3DVALUE ux, D3DVALUE uy, D3DVALUE uz )
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%f,%f,%f,%f,%f,%f): stub\n", iface, This, reference,
          dx, dy, dz, ux, uy, uz);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetPosition(IDirect3DRMFrame3* iface,
                                                        LPDIRECT3DRMFRAME3 reference,
                                                        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%f,%f,%f): stub\n", iface, This, reference, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetRotation(IDirect3DRMFrame3* iface,
                                                        LPDIRECT3DRMFRAME3 reference,
                                                        D3DVALUE x, D3DVALUE y, D3DVALUE z,
                                                        D3DVALUE theta)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%f,%f,%f,%f): stub\n", iface, This, reference, x, y, z, theta);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetSortMode(IDirect3DRMFrame3* iface,
                                                        D3DRMSORTMODE mode)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetTexture(IDirect3DRMFrame3* iface,
                                                       LPDIRECT3DRMTEXTURE3 texture)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetVelocity(IDirect3DRMFrame3* iface,
                                                        LPDIRECT3DRMFRAME3 reference,
                                                        D3DVALUE x, D3DVALUE y, D3DVALUE z,
                                                        BOOL with_rotation)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%f,%f,%f,%d): stub\n", iface, This, reference, x, y, z, with_rotation);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetZbufferMode(IDirect3DRMFrame3* iface,
                                                           D3DRMZBUFFERMODE mode)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_Transform(IDirect3DRMFrame3* iface, D3DVECTOR *d,
                                                      D3DVECTOR *s)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, d, s);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetBox(IDirect3DRMFrame3* iface, LPD3DRMBOX box)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, box);

    return E_NOTIMPL;
}

static BOOL WINAPI IDirect3DRMFrame3Impl_GetBoxEnable(IDirect3DRMFrame3* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetAxes(IDirect3DRMFrame3* iface,
                                                    LPD3DVECTOR dir, LPD3DVECTOR up)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, dir, up);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetMaterial(IDirect3DRMFrame3* iface,
                                                        LPDIRECT3DRMMATERIAL2 *material)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, material);

    return E_NOTIMPL;
}

static BOOL WINAPI IDirect3DRMFrame3Impl_GetInheritAxes(IDirect3DRMFrame3* iface)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(): stub\n", iface, This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetHierarchyBox(IDirect3DRMFrame3* iface,
                                                            LPD3DRMBOX box)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, box);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetBox(IDirect3DRMFrame3* iface, LPD3DRMBOX box)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, box);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetBoxEnable(IDirect3DRMFrame3* iface, BOOL enable)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetAxes(IDirect3DRMFrame3* iface,
                                                    D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
                                                    D3DVALUE ux, D3DVALUE uy, D3DVALUE uz)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%f,%f,%f,%f,%f,%f): stub\n", iface, This, dx, dy, dz, ux, uy, uz);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetInheritAxes(IDirect3DRMFrame3* iface,
                                                           BOOL inherit_from_parent)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, inherit_from_parent);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetMaterial(IDirect3DRMFrame3* iface,
                                                        LPDIRECT3DRMMATERIAL2 material)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, material);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetQuaternion(IDirect3DRMFrame3* iface,
                                                          LPDIRECT3DRMFRAME3 reference,
                                                          D3DRMQUATERNION *q)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", iface, This, reference, q);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_RayPick(IDirect3DRMFrame3* iface,
                                                    LPDIRECT3DRMFRAME3 reference, LPD3DRMRAY ray,
                                                    DWORD flags,
                                                    LPDIRECT3DRMPICKED2ARRAY *return_visuals)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%p,%u,%p): stub\n", iface, This, reference, ray, flags, return_visuals);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_Save(IDirect3DRMFrame3* iface, LPCSTR filename,
                                                 D3DRMXOFFORMAT d3dFormat,
                                                 D3DRMSAVEOPTIONS d3dSaveFlags)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%u,%u): stub\n", iface, This, filename, d3dFormat, d3dSaveFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_TransformVectors(IDirect3DRMFrame3* iface,
                                                             LPDIRECT3DRMFRAME3 reference,
                                                             DWORD num, LPD3DVECTOR dst,
                                                             LPD3DVECTOR src)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%u,%p,%p): stub\n", iface, This, reference, num, dst, src);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_InverseTransformVectors(IDirect3DRMFrame3* iface,
                                                                    LPDIRECT3DRMFRAME3 reference,
                                                                    DWORD num, LPD3DVECTOR dst,
                                                                    LPD3DVECTOR src)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p,%u,%p,%p): stub\n", iface, This, reference, num, dst, src);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetTraversalOptions(IDirect3DRMFrame3* iface,
                                                                DWORD flags)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetTraversalOptions(IDirect3DRMFrame3* iface,
                                                                LPDWORD flags)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetSceneFogMethod(IDirect3DRMFrame3* iface,
                                                              DWORD flags)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%u): stub\n", iface, This, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetSceneFogMethod(IDirect3DRMFrame3* iface,
                                                              LPDWORD flags)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_SetMaterialOverride(IDirect3DRMFrame3* iface,
                                                                LPD3DRMMATERIALOVERRIDE override)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, override);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMFrame3Impl_GetMaterialOverride(IDirect3DRMFrame3* iface,
                                                                LPD3DRMMATERIALOVERRIDE override)
{
    IDirect3DRMFrameImpl *This = impl_from_IDirect3DRMFrame3(iface);

    FIXME("(%p/%p)->(%p): stub\n", iface, This, override);

    return E_NOTIMPL;
}

static const struct IDirect3DRMFrame3Vtbl Direct3DRMFrame3_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMFrame3Impl_QueryInterface,
    IDirect3DRMFrame3Impl_AddRef,
    IDirect3DRMFrame3Impl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMFrame3Impl_Clone,
    IDirect3DRMFrame3Impl_AddDestroyCallback,
    IDirect3DRMFrame3Impl_DeleteDestroyCallback,
    IDirect3DRMFrame3Impl_SetAppData,
    IDirect3DRMFrame3Impl_GetAppData,
    IDirect3DRMFrame3Impl_SetName,
    IDirect3DRMFrame3Impl_GetName,
    IDirect3DRMFrame3Impl_GetClassName,
    /*** IDirect3DRMFrame3 methods ***/
    IDirect3DRMFrame3Impl_AddChild,
    IDirect3DRMFrame3Impl_AddLight,
    IDirect3DRMFrame3Impl_AddMoveCallback,
    IDirect3DRMFrame3Impl_AddTransform,
    IDirect3DRMFrame3Impl_AddTranslation,
    IDirect3DRMFrame3Impl_AddScale,
    IDirect3DRMFrame3Impl_AddRotation,
    IDirect3DRMFrame3Impl_AddVisual,
    IDirect3DRMFrame3Impl_GetChildren,
    IDirect3DRMFrame3Impl_GetColor,
    IDirect3DRMFrame3Impl_GetLights,
    IDirect3DRMFrame3Impl_GetMaterialMode,
    IDirect3DRMFrame3Impl_GetParent,
    IDirect3DRMFrame3Impl_GetPosition,
    IDirect3DRMFrame3Impl_GetRotation,
    IDirect3DRMFrame3Impl_GetScene,
    IDirect3DRMFrame3Impl_GetSortMode,
    IDirect3DRMFrame3Impl_GetTexture,
    IDirect3DRMFrame3Impl_GetTransform,
    IDirect3DRMFrame3Impl_GetVelocity,
    IDirect3DRMFrame3Impl_GetOrientation,
    IDirect3DRMFrame3Impl_GetVisuals,
    IDirect3DRMFrame3Impl_InverseTransform,
    IDirect3DRMFrame3Impl_Load,
    IDirect3DRMFrame3Impl_LookAt,
    IDirect3DRMFrame3Impl_Move,
    IDirect3DRMFrame3Impl_DeleteChild,
    IDirect3DRMFrame3Impl_DeleteLight,
    IDirect3DRMFrame3Impl_DeleteMoveCallback,
    IDirect3DRMFrame3Impl_DeleteVisual,
    IDirect3DRMFrame3Impl_GetSceneBackground,
    IDirect3DRMFrame3Impl_GetSceneBackgroundDepth,
    IDirect3DRMFrame3Impl_GetSceneFogColor,
    IDirect3DRMFrame3Impl_GetSceneFogEnable,
    IDirect3DRMFrame3Impl_GetSceneFogMode,
    IDirect3DRMFrame3Impl_GetSceneFogParams,
    IDirect3DRMFrame3Impl_SetSceneBackground,
    IDirect3DRMFrame3Impl_SetSceneBackgroundRGB,
    IDirect3DRMFrame3Impl_SetSceneBackgroundDepth,
    IDirect3DRMFrame3Impl_SetSceneBackgroundImage,
    IDirect3DRMFrame3Impl_SetSceneFogEnable,
    IDirect3DRMFrame3Impl_SetSceneFogColor,
    IDirect3DRMFrame3Impl_SetSceneFogMode,
    IDirect3DRMFrame3Impl_SetSceneFogParams,
    IDirect3DRMFrame3Impl_SetColor,
    IDirect3DRMFrame3Impl_SetColorRGB,
    IDirect3DRMFrame3Impl_GetZbufferMode,
    IDirect3DRMFrame3Impl_SetMaterialMode,
    IDirect3DRMFrame3Impl_SetOrientation,
    IDirect3DRMFrame3Impl_SetPosition,
    IDirect3DRMFrame3Impl_SetRotation,
    IDirect3DRMFrame3Impl_SetSortMode,
    IDirect3DRMFrame3Impl_SetTexture,
    IDirect3DRMFrame3Impl_SetVelocity,
    IDirect3DRMFrame3Impl_SetZbufferMode,
    IDirect3DRMFrame3Impl_Transform,
    IDirect3DRMFrame3Impl_GetBox,
    IDirect3DRMFrame3Impl_GetBoxEnable,
    IDirect3DRMFrame3Impl_GetAxes,
    IDirect3DRMFrame3Impl_GetMaterial,
    IDirect3DRMFrame3Impl_GetInheritAxes,
    IDirect3DRMFrame3Impl_GetHierarchyBox,
    IDirect3DRMFrame3Impl_SetBox,
    IDirect3DRMFrame3Impl_SetBoxEnable,
    IDirect3DRMFrame3Impl_SetAxes,
    IDirect3DRMFrame3Impl_SetInheritAxes,
    IDirect3DRMFrame3Impl_SetMaterial,
    IDirect3DRMFrame3Impl_SetQuaternion,
    IDirect3DRMFrame3Impl_RayPick,
    IDirect3DRMFrame3Impl_Save,
    IDirect3DRMFrame3Impl_TransformVectors,
    IDirect3DRMFrame3Impl_InverseTransformVectors,
    IDirect3DRMFrame3Impl_SetTraversalOptions,
    IDirect3DRMFrame3Impl_GetTraversalOptions,
    IDirect3DRMFrame3Impl_SetSceneFogMethod,
    IDirect3DRMFrame3Impl_GetSceneFogMethod,
    IDirect3DRMFrame3Impl_SetMaterialOverride,
    IDirect3DRMFrame3Impl_GetMaterialOverride
};

static inline IDirect3DRMFrameImpl *unsafe_impl_from_IDirect3DRMFrame2(IDirect3DRMFrame2 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &Direct3DRMFrame2_Vtbl);

    return impl_from_IDirect3DRMFrame2(iface);
}

HRESULT Direct3DRMFrame_create(REFIID riid, IUnknown** ppObj)
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
    object->IDirect3DRMFrame3_iface.lpVtbl = &Direct3DRMFrame3_Vtbl;
    object->ref = 1;

    if (IsEqualGUID(riid, &IID_IDirect3DRMFrame3))
        *ppObj = (IUnknown*)&object->IDirect3DRMFrame3_iface;
    else
        *ppObj = (IUnknown*)&object->IDirect3DRMFrame2_iface;

    return S_OK;
}
