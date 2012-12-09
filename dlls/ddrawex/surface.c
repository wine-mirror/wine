/*
 * Copyright 2008 Stefan Dösinger for CodeWeavers
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

#include "ddraw.h"
#include "d3d.h"

#include "ddrawex_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddrawex);

/******************************************************************************
 * Helper functions for COM management
 ******************************************************************************/
static IDirectDrawSurfaceImpl *impl_from_IDirectDrawSurface3(IDirectDrawSurface3 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirectDrawSurface3_iface);
}

static IDirectDrawSurfaceImpl *unsafe_impl_from_IDirectDrawSurface3(IDirectDrawSurface3 *iface);

static IDirectDrawSurfaceImpl *impl_from_IDirectDrawSurface4(IDirectDrawSurface4 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirectDrawSurface4_iface);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_QueryInterface(IDirectDrawSurface4 *iface,
        REFIID riid, void **obj)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);

    *obj = NULL;

    if(!riid)
        return DDERR_INVALIDPARAMS;

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),obj);
    if (IsEqualGUID(riid, &IID_IUnknown)
     || IsEqualGUID(riid, &IID_IDirectDrawSurface4) )
    {
        *obj = &This->IDirectDrawSurface4_iface;
        IDirectDrawSurface4_AddRef(&This->IDirectDrawSurface4_iface);
        TRACE("(%p) returning IDirectDrawSurface4 interface at %p\n", This, *obj);
        return S_OK;
    }
    else if( IsEqualGUID(riid, &IID_IDirectDrawSurface3)
          || IsEqualGUID(riid, &IID_IDirectDrawSurface2)
          || IsEqualGUID(riid, &IID_IDirectDrawSurface) )
    {
        *obj = &This->IDirectDrawSurface3_iface;
        IDirectDrawSurface3_AddRef(&This->IDirectDrawSurface3_iface);
        TRACE("(%p) returning IDirectDrawSurface3 interface at %p\n", This, *obj);
        return S_OK;
    }
    else if( IsEqualGUID(riid, &IID_IDirectDrawGammaControl) )
    {
        FIXME("Implement IDirectDrawGammaControl in ddrawex\n");
    }
    else if( IsEqualGUID(riid, &IID_IDirect3DHALDevice)||
             IsEqualGUID(riid, &IID_IDirect3DRGBDevice) )
    {
        /* Most likely not supported */
        FIXME("Test IDirect3DDevice in ddrawex\n");
    }
    else if (IsEqualGUID( &IID_IDirect3DTexture, riid ) ||
             IsEqualGUID( &IID_IDirect3DTexture2, riid ))
    {
        FIXME("Implement IDirect3dTexture in ddrawex\n");
    }
    else
    {
        WARN("No interface\n");
    }

    return E_NOINTERFACE;
}

static HRESULT WINAPI IDirectDrawSurface3Impl_QueryInterface(IDirectDrawSurface3 *iface,
        REFIID riid, void **obj)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%s,%p): Thunking to IDirectDrawSurface4\n",This,debugstr_guid(riid),obj);
    return IDirectDrawSurface4_QueryInterface(&This->IDirectDrawSurface4_iface, riid, obj);
}

static ULONG WINAPI IDirectDrawSurface4Impl_AddRef(IDirectDrawSurface4 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : incrementing refcount from %u.\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirectDrawSurface3Impl_AddRef(IDirectDrawSurface3 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p): Thunking to IDirectDrawSurface4\n", This);
    return IDirectDrawSurface4_AddRef(&This->IDirectDrawSurface4_iface);
}

static ULONG WINAPI IDirectDrawSurface4Impl_Release(IDirectDrawSurface4 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : decrementing refcount to %u.\n", This, ref);

    if(ref == 0)
    {
        TRACE("Destroying object\n");
        IDirectDrawSurface4_FreePrivateData(This->parent, &IID_DDrawexPriv);
        IDirectDrawSurface4_Release(This->parent);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static ULONG WINAPI IDirectDrawSurface3Impl_Release(IDirectDrawSurface3 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p): Thunking to IDirectDrawSurface4\n", This);
    return IDirectDrawSurface4_Release(&This->IDirectDrawSurface4_iface);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_AddAttachedSurface(IDirectDrawSurface4 *iface,
        IDirectDrawSurface4 *Attach_iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurfaceImpl *attach = unsafe_impl_from_IDirectDrawSurface4(Attach_iface);
    TRACE("(%p)->(%p)\n", This, attach);
    return IDirectDrawSurface4_AddAttachedSurface(This->parent, attach->parent);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_AddAttachedSurface(IDirectDrawSurface3 *iface,
        IDirectDrawSurface3 *Attach_iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurfaceImpl *attach = unsafe_impl_from_IDirectDrawSurface3(Attach_iface);
    TRACE("(%p)->(%p): Thunking to IDirectDrawSurface4\n", This, attach);
    return IDirectDrawSurface4_AddAttachedSurface(&This->IDirectDrawSurface4_iface,
            attach ? &attach->IDirectDrawSurface4_iface : NULL);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_AddOverlayDirtyRect(IDirectDrawSurface4 *iface,
        RECT *Rect)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%p)\n", This, Rect);
    return IDirectDrawSurface4_AddOverlayDirtyRect(This->parent, Rect);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_AddOverlayDirtyRect(IDirectDrawSurface3 *iface,
        RECT *Rect)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDrawSurface4\n", This, Rect);
    return IDirectDrawSurface4_AddOverlayDirtyRect(&This->IDirectDrawSurface4_iface, Rect);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_Blt(IDirectDrawSurface4 *iface, RECT *DestRect,
        IDirectDrawSurface4 *SrcSurface, RECT *SrcRect, DWORD Flags, DDBLTFX *DDBltFx)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurfaceImpl *Src = unsafe_impl_from_IDirectDrawSurface4(SrcSurface);
    TRACE("(%p)->(%p,%p,%p,0x%08x,%p)\n", This, DestRect, Src, SrcRect, Flags, DDBltFx);
    return IDirectDrawSurface4_Blt(This->parent, DestRect, Src ? Src->parent : NULL,
                                   SrcRect, Flags, DDBltFx);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_Blt(IDirectDrawSurface3 *iface, RECT *DestRect,
        IDirectDrawSurface3 *SrcSurface, RECT *SrcRect, DWORD Flags, DDBLTFX *DDBltFx)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurfaceImpl *Src = unsafe_impl_from_IDirectDrawSurface3(SrcSurface);
    TRACE("(%p)->(%p,%p,%p,0x%08x,%p): Thunking to IDirectDrawSurface4\n", This, DestRect, Src, SrcRect, Flags, DDBltFx);
    return IDirectDrawSurface4_Blt(&This->IDirectDrawSurface4_iface, DestRect,
            Src ? &Src->IDirectDrawSurface4_iface : NULL, SrcRect, Flags, DDBltFx);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_BltBatch(IDirectDrawSurface4 *iface,
        DDBLTBATCH *Batch, DWORD Count, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%p,%u,0x%08x)\n", This, Batch, Count, Flags);
    return IDirectDrawSurface4_BltBatch(This->parent, Batch, Count, Flags);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_BltBatch(IDirectDrawSurface3 *iface,
        DDBLTBATCH *Batch, DWORD Count, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%p,%u,0x%08x): Thunking to IDirectDrawSurface4\n", This, Batch, Count, Flags);
    return IDirectDrawSurface4_BltBatch(&This->IDirectDrawSurface4_iface, Batch, Count, Flags);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_BltFast(IDirectDrawSurface4 *iface, DWORD dstx,
        DWORD dsty, IDirectDrawSurface4 *Source, RECT *rsrc, DWORD trans)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurfaceImpl *Src = unsafe_impl_from_IDirectDrawSurface4(Source);
    TRACE("(%p)->(%u,%u,%p,%p,0x%08x)\n", This, dstx, dsty, Src, rsrc, trans);
    return IDirectDrawSurface4_BltFast(This->parent, dstx, dsty, Src ? Src->parent : NULL,
                                       rsrc, trans);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_BltFast(IDirectDrawSurface3 *iface, DWORD dstx,
        DWORD dsty, IDirectDrawSurface3 *Source, RECT *rsrc, DWORD trans)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurfaceImpl *Src = unsafe_impl_from_IDirectDrawSurface3(Source);
    TRACE("(%p)->(%u,%u,%p,%p,0x%08x): Thunking to IDirectDrawSurface4\n", This, dstx, dsty, Src, rsrc, trans);
    return IDirectDrawSurface4_BltFast(&This->IDirectDrawSurface4_iface, dstx, dsty,
            Src ? &Src->IDirectDrawSurface4_iface : NULL, rsrc, trans);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_DeleteAttachedSurface(IDirectDrawSurface4 *iface,
        DWORD Flags, IDirectDrawSurface4 *Attach)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurfaceImpl *Att = unsafe_impl_from_IDirectDrawSurface4(Attach);
    TRACE("(%p)->(0x%08x,%p)\n", This, Flags, Att);
    return IDirectDrawSurface4_DeleteAttachedSurface(This->parent, Flags,
                                                     Att ? Att->parent : NULL);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_DeleteAttachedSurface(IDirectDrawSurface3 *iface,
        DWORD Flags, IDirectDrawSurface3 *Attach)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurfaceImpl *Att = unsafe_impl_from_IDirectDrawSurface3(Attach);
    TRACE("(%p)->(0x%08x,%p): Thunking to IDirectDrawSurface4\n", This, Flags, Att);
    return IDirectDrawSurface4_DeleteAttachedSurface(&This->IDirectDrawSurface4_iface, Flags,
            Att ? &Att->IDirectDrawSurface4_iface : NULL);
}

struct enumsurfaces_wrap
{
    LPDDENUMSURFACESCALLBACK2 orig_cb;
    void *orig_ctx;
};

static HRESULT WINAPI
enumsurfaces_wrap_cb(IDirectDrawSurface4 *surf, DDSURFACEDESC2 *desc, void *vctx)
{
    struct enumsurfaces_wrap *ctx = vctx;
    IDirectDrawSurface4 *outer = dds_get_outer(surf);

    TRACE("Returning outer surface %p for inner surface %p\n", outer, surf);
    IDirectDrawSurface4_AddRef(outer);
    IDirectDrawSurface4_Release(surf);
    return ctx->orig_cb(outer, desc, vctx);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_EnumAttachedSurfaces(IDirectDrawSurface4 *iface,
        void *context, LPDDENUMSURFACESCALLBACK2 cb)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    struct enumsurfaces_wrap ctx;
    TRACE("(%p)->(%p,%p)\n", This, context, cb);

    ctx.orig_cb = cb;
    ctx.orig_ctx = context;
    return IDirectDrawSurface4_EnumAttachedSurfaces(This->parent, &ctx, enumsurfaces_wrap_cb);
}

struct enumsurfaces_thunk
{
    LPDDENUMSURFACESCALLBACK orig_cb;
    void *orig_ctx;
};

static HRESULT WINAPI enumsurfaces_thunk_cb(IDirectDrawSurface4 *surf, DDSURFACEDESC2 *desc2,
        void *vctx)
{
    IDirectDrawSurfaceImpl *This = unsafe_impl_from_IDirectDrawSurface4(surf);
    struct enumsurfaces_thunk *ctx = vctx;
    DDSURFACEDESC desc;

    TRACE("Thunking back to IDirectDrawSurface3\n");
    IDirectDrawSurface3_AddRef(&This->IDirectDrawSurface3_iface);
    IDirectDrawSurface3_Release(surf);
    DDSD2_to_DDSD(desc2, &desc);
    return ctx->orig_cb((IDirectDrawSurface *)&This->IDirectDrawSurface3_iface, &desc,
            ctx->orig_ctx);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_EnumAttachedSurfaces(IDirectDrawSurface3 *iface,
        void *context, LPDDENUMSURFACESCALLBACK cb)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    struct enumsurfaces_thunk ctx;
    TRACE("(%p)->(%p,%p): Thunking to IDirectDraw4\n", This, context, cb);

    ctx.orig_cb = cb;
    ctx.orig_ctx = context;
    return IDirectDrawSurface4_EnumAttachedSurfaces(&This->IDirectDrawSurface4_iface, &ctx,
            enumsurfaces_thunk_cb);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_EnumOverlayZOrders(IDirectDrawSurface4 *iface,
        DWORD Flags, void *context, LPDDENUMSURFACESCALLBACK2 cb)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    struct enumsurfaces_wrap ctx;
    TRACE("(%p)->(0x%08x,%p,%p)\n", This, Flags, context, cb);

    ctx.orig_cb = cb;
    ctx.orig_ctx = context;
    return IDirectDrawSurface4_EnumOverlayZOrders(This->parent, Flags, &ctx, enumsurfaces_wrap_cb);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_EnumOverlayZOrders(IDirectDrawSurface3 *iface,
        DWORD Flags, void *context, LPDDENUMSURFACESCALLBACK cb)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    struct enumsurfaces_thunk ctx;
    TRACE("(%p)->(0x%08x,%p,%p): Thunking to IDirectDraw4\n", This, Flags, context, cb);

    ctx.orig_cb = cb;
    ctx.orig_ctx = context;
    return IDirectDrawSurface4_EnumOverlayZOrders(&This->IDirectDrawSurface4_iface, Flags, &ctx,
            enumsurfaces_thunk_cb);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_Flip(IDirectDrawSurface4 *iface,
        IDirectDrawSurface4 *DestOverride, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurfaceImpl *Dest = unsafe_impl_from_IDirectDrawSurface4(DestOverride);
    TRACE("(%p)->(%p,0x%08x)\n", This, Dest, Flags);
    return IDirectDrawSurface4_Flip(This->parent, Dest ? Dest->parent : NULL, Flags);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_Flip(IDirectDrawSurface3 *iface,
        IDirectDrawSurface3 *DestOverride, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurfaceImpl *Dest = unsafe_impl_from_IDirectDrawSurface3(DestOverride);
    TRACE("(%p)->(%p,0x%08x): Thunking to IDirectDrawSurface4\n", This, Dest, Flags);
    return IDirectDrawSurface4_Flip(&This->IDirectDrawSurface4_iface,
            Dest ? &Dest->IDirectDrawSurface4_iface : NULL, Flags);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetAttachedSurface(IDirectDrawSurface4 *iface,
        DDSCAPS2 *Caps, IDirectDrawSurface4 **Surface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurface4 *inner = NULL;
    HRESULT hr;
    TRACE("(%p)->(%p,%p)\n", This, Caps, Surface);

    hr = IDirectDrawSurface4_GetAttachedSurface(This->parent, Caps, &inner);
    if(SUCCEEDED(hr))
    {
        *Surface = dds_get_outer(inner);
        IDirectDrawSurface4_AddRef(*Surface);
        IDirectDrawSurface4_Release(inner);
    }
    else
    {
        *Surface = NULL;
    }
    return hr;
}

static HRESULT WINAPI IDirectDrawSurface3Impl_GetAttachedSurface(IDirectDrawSurface3 *iface,
        DDSCAPS *Caps, IDirectDrawSurface3 **Surface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurface4 *surf4;
    DDSCAPS2 caps2;
    HRESULT hr;
    TRACE("(%p)->(%p,%p): Thunking to IDirectDrawSurface4\n", This, Caps, Surface);

    memset(&caps2, 0, sizeof(caps2));
    caps2.dwCaps = Caps->dwCaps;
    hr = IDirectDrawSurface4_GetAttachedSurface(&This->IDirectDrawSurface4_iface, &caps2, &surf4);
    if(SUCCEEDED(hr))
    {
        IDirectDrawSurface4_QueryInterface(surf4, &IID_IDirectDrawSurface3, (void **) Surface);
        IDirectDrawSurface4_Release(surf4);
    }
    else
    {
        *Surface = NULL;
    }
    return hr;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetBltStatus(IDirectDrawSurface4 *iface, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(0x%08x)\n", This, Flags);
    return IDirectDrawSurface4_GetBltStatus(This->parent, Flags);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_GetBltStatus(IDirectDrawSurface3 *iface, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(0x%08x): Thunking to IDirectDrawSurface4\n", This, Flags);
    return IDirectDrawSurface4_GetBltStatus(&This->IDirectDrawSurface4_iface, Flags);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetCaps(IDirectDrawSurface4 *iface, DDSCAPS2 *Caps)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%p)\n", This, Caps);
    return IDirectDrawSurface4_GetCaps(This->parent, Caps);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_GetCaps(IDirectDrawSurface3 *iface, DDSCAPS *Caps)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    DDSCAPS2 caps2;
    HRESULT hr;
    TRACE("(%p)->(%p): Thunking to IDirectDrawSurface4\n", This, Caps);

    memset(&caps2, 0, sizeof(caps2));
    memset(Caps, 0, sizeof(*Caps));
    hr = IDirectDrawSurface4_GetCaps(&This->IDirectDrawSurface4_iface, &caps2);
    Caps->dwCaps = caps2.dwCaps;
    return hr;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetClipper(IDirectDrawSurface4 *iface,
        IDirectDrawClipper **Clipper)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%p)\n", This, Clipper);
    return IDirectDrawSurface4_GetClipper(This->parent, Clipper);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_GetClipper(IDirectDrawSurface3 *iface,
        IDirectDrawClipper **Clipper)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDrawSurface4\n", This, Clipper);
    return IDirectDrawSurface4_GetClipper(&This->IDirectDrawSurface4_iface, Clipper);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetColorKey(IDirectDrawSurface4 *iface, DWORD Flags,
        DDCOLORKEY *CKey)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(0x%08x,%p)\n", This, Flags, CKey);
    return IDirectDrawSurface4_GetColorKey(This->parent, Flags, CKey);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_GetColorKey(IDirectDrawSurface3 *iface, DWORD Flags,
        DDCOLORKEY *CKey)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(0x%08x,%p): Thunking to IDirectDrawSurface4\n", This, Flags, CKey);
    return IDirectDrawSurface4_GetColorKey(&This->IDirectDrawSurface4_iface, Flags, CKey);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetDC(IDirectDrawSurface4 *iface, HDC *hdc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%p)\n", This, hdc);
    if(This->permanent_dc)
    {
        TRACE("Returning stored dc %p\n", This->hdc);
        *hdc = This->hdc;
        return DD_OK;
    }
    else
    {
        return IDirectDrawSurface4_GetDC(This->parent, hdc);
    }
}

static HRESULT WINAPI IDirectDrawSurface3Impl_GetDC(IDirectDrawSurface3 *iface, HDC *hdc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDrawSurface4\n", This, hdc);
    return IDirectDrawSurface4_GetDC(&This->IDirectDrawSurface4_iface, hdc);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetFlipStatus(IDirectDrawSurface4 *iface, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(0x%08x)\n", This, Flags);
    return IDirectDrawSurface4_GetFlipStatus(This->parent, Flags);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_GetFlipStatus(IDirectDrawSurface3 *iface, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(0x%08x): Thunking to IDirectDrawSurface4\n", This, Flags);
    return IDirectDrawSurface4_GetFlipStatus(&This->IDirectDrawSurface4_iface, Flags);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetOverlayPosition(IDirectDrawSurface4 *iface,
        LONG *X, LONG *Y)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%p,%p)\n", This, X, Y);
    return IDirectDrawSurface4_GetOverlayPosition(This->parent, X, Y);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_GetOverlayPosition(IDirectDrawSurface3 *iface,
        LONG *X, LONG *Y)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%p,%p): Thunking to IDirectDrawSurface4\n", This, X, Y);
    return IDirectDrawSurface4_GetOverlayPosition(&This->IDirectDrawSurface4_iface, X, Y);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetPalette(IDirectDrawSurface4 *iface,
        IDirectDrawPalette **Pal)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%p)\n", This, Pal);
    return IDirectDrawSurface4_GetPalette(This->parent, Pal);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_GetPalette(IDirectDrawSurface3 *iface,
        IDirectDrawPalette **Pal)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDrawSurface4\n", This, Pal);
    return IDirectDrawSurface4_GetPalette(&This->IDirectDrawSurface4_iface, Pal);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetPixelFormat(IDirectDrawSurface4 *iface,
        DDPIXELFORMAT *PixelFormat)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%p)\n", This, PixelFormat);
    return IDirectDrawSurface4_GetPixelFormat(This->parent, PixelFormat);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_GetPixelFormat(IDirectDrawSurface3 *iface,
        DDPIXELFORMAT *PixelFormat)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDrawSurface4\n", This, PixelFormat);
    return IDirectDrawSurface4_GetPixelFormat(&This->IDirectDrawSurface4_iface, PixelFormat);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetSurfaceDesc(IDirectDrawSurface4 *iface,
        DDSURFACEDESC2 *DDSD)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, DDSD);
    hr = IDirectDrawSurface4_GetSurfaceDesc(This->parent, DDSD);

    if(SUCCEEDED(hr) && This->permanent_dc)
    {
        DDSD->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
        DDSD->ddsCaps.dwCaps &= ~DDSCAPS_OWNDC;
    }

    return hr;
}

static HRESULT WINAPI IDirectDrawSurface3Impl_GetSurfaceDesc(IDirectDrawSurface3 *iface,
        DDSURFACEDESC *DDSD)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    DDSURFACEDESC2 ddsd2;
    HRESULT hr;
    TRACE("(%p)->(%p): Thunking to IDirectDrawSurface4\n", This, DDSD);

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    hr = IDirectDrawSurface4_GetSurfaceDesc(&This->IDirectDrawSurface4_iface, &ddsd2);
    DDSD2_to_DDSD(&ddsd2, DDSD);
    return hr;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_Initialize(IDirectDrawSurface4 *iface,
        IDirectDraw *DD, DDSURFACEDESC2 *DDSD)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDraw4 *outer_DD4;
    IDirectDraw4 *inner_DD4;
    IDirectDraw *inner_DD;
    HRESULT hr;
    TRACE("(%p)->(%p,%p)\n", This, DD, DDSD);

    IDirectDraw_QueryInterface(DD, &IID_IDirectDraw4, (void **) &outer_DD4);
    inner_DD4 = dd_get_inner(outer_DD4);
    IDirectDraw4_Release(outer_DD4);
    IDirectDraw4_QueryInterface(inner_DD4, &IID_IDirectDraw4, (void **) &inner_DD);
    hr = IDirectDrawSurface4_Initialize(This->parent, inner_DD, DDSD);
    IDirectDraw_Release(inner_DD);
    return hr;
}

static HRESULT WINAPI IDirectDrawSurface3Impl_Initialize(IDirectDrawSurface3 *iface,
        IDirectDraw *DD, DDSURFACEDESC *DDSD)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    DDSURFACEDESC2 ddsd2;
    TRACE("(%p)->(%p,%p): Thunking to IDirectDrawSurface4\n", This, DD, DDSD);
    DDSD_to_DDSD2(DDSD, &ddsd2);
    return IDirectDrawSurface4_Initialize(&This->IDirectDrawSurface4_iface, DD, &ddsd2);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_IsLost(IDirectDrawSurface4 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)\n", This);
    return IDirectDrawSurface4_IsLost(This->parent);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_IsLost(IDirectDrawSurface3 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p): Thunking to IDirectDrawSurface4\n", This);
    return IDirectDrawSurface4_IsLost(&This->IDirectDrawSurface4_iface);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_Lock(IDirectDrawSurface4 *iface, RECT *Rect,
        DDSURFACEDESC2 *DDSD, DWORD Flags, HANDLE h)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    HRESULT hr;
    TRACE("(%p)->(%p,%p,0x%08x,%p)\n", This, Rect, DDSD, Flags, h);
    hr = IDirectDrawSurface4_Lock(This->parent, Rect, DDSD, Flags, h);

    if(SUCCEEDED(hr) && This->permanent_dc)
    {
        DDSD->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
        DDSD->ddsCaps.dwCaps &= ~DDSCAPS_OWNDC;
    }

    return hr;
}

static HRESULT WINAPI IDirectDrawSurface3Impl_Lock(IDirectDrawSurface3 *iface, RECT *Rect,
        DDSURFACEDESC *DDSD, DWORD Flags, HANDLE h)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    DDSURFACEDESC2 ddsd2;
    HRESULT hr;
    TRACE("(%p)->(%p,%p,0x%08x,%p): Thunking to IDirectDrawSurface4\n", This, Rect, DDSD, Flags, h);
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    hr = IDirectDrawSurface4_Lock(&This->IDirectDrawSurface4_iface, Rect, &ddsd2, Flags, h);
    DDSD2_to_DDSD(&ddsd2, DDSD);
    return hr;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_ReleaseDC(IDirectDrawSurface4 *iface, HDC hdc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%p)\n", This, hdc);
    if(This->permanent_dc)
    {
        TRACE("Surface has a permanent DC, not doing anything\n");
        return DD_OK;
    }
    else
    {
        return IDirectDrawSurface4_ReleaseDC(This->parent, hdc);
    }
}

static HRESULT WINAPI IDirectDrawSurface3Impl_ReleaseDC(IDirectDrawSurface3 *iface, HDC hdc)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDrawSurface4\n", This, hdc);
    return IDirectDrawSurface4_ReleaseDC(&This->IDirectDrawSurface4_iface, hdc);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_Restore(IDirectDrawSurface4 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)\n", This);
    return IDirectDrawSurface4_Restore(This->parent);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_Restore(IDirectDrawSurface3 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p): Thunking to IDirectDrawSurface4\n", This);
    return IDirectDrawSurface4_Restore(&This->IDirectDrawSurface4_iface);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetClipper(IDirectDrawSurface4 *iface,
        IDirectDrawClipper *Clipper)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%p)\n", This, Clipper);
    return IDirectDrawSurface4_SetClipper(This->parent, Clipper);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_SetClipper(IDirectDrawSurface3 *iface,
        IDirectDrawClipper *Clipper)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDrawSurface4\n", This, Clipper);
    return IDirectDrawSurface4_SetClipper(&This->IDirectDrawSurface4_iface, Clipper);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetColorKey(IDirectDrawSurface4 *iface, DWORD Flags,
        DDCOLORKEY *CKey)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(0x%08x,%p)\n", This, Flags, CKey);
    return IDirectDrawSurface4_SetColorKey(This->parent, Flags, CKey);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_SetColorKey(IDirectDrawSurface3 *iface, DWORD Flags,
        DDCOLORKEY *CKey)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(0x%08x,%p): Thunking to IDirectDrawSurface4\n", This, Flags, CKey);
    return IDirectDrawSurface4_SetColorKey(&This->IDirectDrawSurface4_iface, Flags, CKey);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetOverlayPosition(IDirectDrawSurface4 *iface,
        LONG X, LONG Y)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%u,%u)\n", This, X, Y);
    return IDirectDrawSurface4_SetOverlayPosition(This->parent, X, Y);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_SetOverlayPosition(IDirectDrawSurface3 *iface,
        LONG X, LONG Y)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%u,%u): Thunking to IDirectDrawSurface4\n", This, X, Y);
    return IDirectDrawSurface4_SetOverlayPosition(&This->IDirectDrawSurface4_iface, X, Y);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetPalette(IDirectDrawSurface4 *iface,
        IDirectDrawPalette *Pal)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%p)\n", This, Pal);
    return IDirectDrawSurface4_SetPalette(This->parent, Pal);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_SetPalette(IDirectDrawSurface3 *iface,
        IDirectDrawPalette *Pal)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDrawSurface4\n", This, Pal);
    return IDirectDrawSurface4_SetPalette(&This->IDirectDrawSurface4_iface, Pal);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_Unlock(IDirectDrawSurface4 *iface, RECT *pRect)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%p)\n", This, pRect);
    return IDirectDrawSurface4_Unlock(This->parent, pRect);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_Unlock(IDirectDrawSurface3 *iface, void *data)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDrawSurface4\n", This, data);
    return IDirectDrawSurface4_Unlock(&This->IDirectDrawSurface4_iface, NULL);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_UpdateOverlay(IDirectDrawSurface4 *iface,
        RECT *SrcRect, IDirectDrawSurface4 *DstSurface, RECT *DstRect, DWORD Flags,
        DDOVERLAYFX *FX)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurfaceImpl *Dst = unsafe_impl_from_IDirectDrawSurface4(DstSurface);
    TRACE("(%p)->(%p,%p,%p,0x%08x,%p)\n", This, SrcRect, Dst, DstRect, Flags, FX);
    return IDirectDrawSurface4_UpdateOverlay(This->parent, SrcRect, Dst ? Dst->parent : NULL,
                                             DstRect, Flags, FX);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_UpdateOverlay(IDirectDrawSurface3 *iface,
        RECT *SrcRect, IDirectDrawSurface3 *DstSurface, RECT *DstRect, DWORD Flags,
        DDOVERLAYFX *FX)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurfaceImpl *Dst = unsafe_impl_from_IDirectDrawSurface3(DstSurface);
    TRACE("(%p)->(%p,%p,%p,0x%08x,%p): Thunking to IDirectDrawSurface4\n", This, SrcRect, Dst, DstRect, Flags, FX);
    return IDirectDrawSurface4_UpdateOverlay(&This->IDirectDrawSurface4_iface, SrcRect,
            Dst ? &Dst->IDirectDrawSurface4_iface : NULL, DstRect, Flags, FX);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_UpdateOverlayDisplay(IDirectDrawSurface4 *iface,
        DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(0x%08x)\n", This, Flags);
    return IDirectDrawSurface4_UpdateOverlayDisplay(This->parent, Flags);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_UpdateOverlayDisplay(IDirectDrawSurface3 *iface,
        DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(0x%08x): Thunking to IDirectDrawSurface4\n", This, Flags);
    return IDirectDrawSurface4_UpdateOverlayDisplay(&This->IDirectDrawSurface4_iface, Flags);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_UpdateOverlayZOrder(IDirectDrawSurface4 *iface,
        DWORD Flags, IDirectDrawSurface4 *DDSRef)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurfaceImpl *Ref = unsafe_impl_from_IDirectDrawSurface4(DDSRef);
    TRACE("(%p)->(0x%08x,%p)\n", This, Flags, Ref);
    return IDirectDrawSurface4_UpdateOverlayZOrder(This->parent, Flags, Ref ? Ref->parent : NULL);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_UpdateOverlayZOrder(IDirectDrawSurface3 *iface,
        DWORD Flags, IDirectDrawSurface3 *DDSRef)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurfaceImpl *Ref = unsafe_impl_from_IDirectDrawSurface3(DDSRef);
    TRACE("(%p)->(0x%08x,%p): Thunking to IDirectDrawSurface4\n", This, Flags, Ref);
    return IDirectDrawSurface4_UpdateOverlayZOrder(&This->IDirectDrawSurface4_iface, Flags,
            Ref ? &Ref->IDirectDrawSurface4_iface : NULL);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetDDInterface(IDirectDrawSurface4 *iface, void **DD)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    FIXME("(%p)->(%p)\n", This, DD);
    /* This has to be implemented in ddrawex, DDraw's interface can't be used because it is pretty
     * hard to tell which version of the DD interface is returned
     */
    *DD = NULL;
    return E_FAIL;
}

static HRESULT WINAPI IDirectDrawSurface3Impl_GetDDInterface(IDirectDrawSurface3 *iface, void **DD)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    FIXME("(%p)->(%p)\n", This, DD);
    /* A thunk it pretty pointless because of the same reason relaying to ddraw.dll works badly
     */
    *DD = NULL;
    return E_FAIL;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_PageLock(IDirectDrawSurface4 *iface, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%x)\n", iface, Flags);
    return IDirectDrawSurface4_PageLock(This->parent, Flags);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_PageLock(IDirectDrawSurface3 *iface, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%x): Thunking to IDirectDrawSurface4\n", iface, Flags);
    return IDirectDrawSurface4_PageLock(&This->IDirectDrawSurface4_iface, Flags);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_PageUnlock(IDirectDrawSurface4 *iface, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%x)\n", iface, Flags);
    return IDirectDrawSurface4_PageUnlock(This->parent, Flags);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_PageUnlock(IDirectDrawSurface3 *iface, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    TRACE("(%p)->(%x): Thunking to IDirectDrawSurface4\n", iface, Flags);
    return IDirectDrawSurface4_PageUnlock(&This->IDirectDrawSurface4_iface, Flags);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetSurfaceDesc(IDirectDrawSurface4 *iface,
        DDSURFACEDESC2 *DDSD, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%p,0x%08x)\n", This, DDSD, Flags);
    return IDirectDrawSurface4_SetSurfaceDesc(This->parent, DDSD, Flags);
}

static HRESULT WINAPI IDirectDrawSurface3Impl_SetSurfaceDesc(IDirectDrawSurface3 *iface,
        DDSURFACEDESC *DDSD, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface3(iface);
    DDSURFACEDESC2 ddsd;
    TRACE("(%p)->(%p,0x%08x): Thunking to IDirectDrawSurface4\n", This, DDSD, Flags);

    DDSD_to_DDSD2(DDSD, &ddsd);
    return IDirectDrawSurface4_SetSurfaceDesc(&This->IDirectDrawSurface4_iface, &ddsd, Flags);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetPrivateData(IDirectDrawSurface4 *iface,
        REFGUID tag, void *Data, DWORD Size, DWORD Flags)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%s,%p,%u,0x%08x)\n", iface, debugstr_guid(tag), Data, Size, Flags);

    /* To completely avoid this we'd have to clone the private data API in ddrawex */
    if(IsEqualGUID(&IID_DDrawexPriv, tag)) {
        FIXME("Application uses ddrawex's private guid\n");
    }

    return IDirectDrawSurface4_SetPrivateData(This->parent, tag, Data, Size, Flags);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetPrivateData(IDirectDrawSurface4 *iface,
        REFGUID tag, void *Data, DWORD *Size)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%s,%p,%p)\n", iface, debugstr_guid(tag), Data, Size);

    /* To completely avoid this we'd have to clone the private data API in ddrawex */
    if(IsEqualGUID(&IID_DDrawexPriv, tag)) {
        FIXME("Application uses ddrawex's private guid\n");
    }

    return IDirectDrawSurface4_GetPrivateData(This->parent, tag, Data, Size);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_FreePrivateData(IDirectDrawSurface4 *iface,
        REFGUID tag)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%s)\n", iface, debugstr_guid(tag));

    /* To completely avoid this we'd have to clone the private data API in ddrawex */
    if(IsEqualGUID(&IID_DDrawexPriv, tag)) {
        FIXME("Application uses ddrawex's private guid\n");
    }

    return IDirectDrawSurface4_FreePrivateData(This->parent, tag);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetUniquenessValue(IDirectDrawSurface4 *iface,
        DWORD *pValue)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)->(%p)\n", This, pValue);
    return IDirectDrawSurface4_GetUniquenessValue(This->parent, pValue);
}

static HRESULT WINAPI IDirectDrawSurface4Impl_ChangeUniquenessValue(IDirectDrawSurface4 *iface)
{
    IDirectDrawSurfaceImpl *This = impl_from_IDirectDrawSurface4(iface);
    TRACE("(%p)\n", This);
    return IDirectDrawSurface4_ChangeUniquenessValue(This->parent);
}

static const IDirectDrawSurface3Vtbl IDirectDrawSurface3_Vtbl =
{
    /* IUnknown */
    IDirectDrawSurface3Impl_QueryInterface,
    IDirectDrawSurface3Impl_AddRef,
    IDirectDrawSurface3Impl_Release,
    /* IDirectDrawSurface */
    IDirectDrawSurface3Impl_AddAttachedSurface,
    IDirectDrawSurface3Impl_AddOverlayDirtyRect,
    IDirectDrawSurface3Impl_Blt,
    IDirectDrawSurface3Impl_BltBatch,
    IDirectDrawSurface3Impl_BltFast,
    IDirectDrawSurface3Impl_DeleteAttachedSurface,
    IDirectDrawSurface3Impl_EnumAttachedSurfaces,
    IDirectDrawSurface3Impl_EnumOverlayZOrders,
    IDirectDrawSurface3Impl_Flip,
    IDirectDrawSurface3Impl_GetAttachedSurface,
    IDirectDrawSurface3Impl_GetBltStatus,
    IDirectDrawSurface3Impl_GetCaps,
    IDirectDrawSurface3Impl_GetClipper,
    IDirectDrawSurface3Impl_GetColorKey,
    IDirectDrawSurface3Impl_GetDC,
    IDirectDrawSurface3Impl_GetFlipStatus,
    IDirectDrawSurface3Impl_GetOverlayPosition,
    IDirectDrawSurface3Impl_GetPalette,
    IDirectDrawSurface3Impl_GetPixelFormat,
    IDirectDrawSurface3Impl_GetSurfaceDesc,
    IDirectDrawSurface3Impl_Initialize,
    IDirectDrawSurface3Impl_IsLost,
    IDirectDrawSurface3Impl_Lock,
    IDirectDrawSurface3Impl_ReleaseDC,
    IDirectDrawSurface3Impl_Restore,
    IDirectDrawSurface3Impl_SetClipper,
    IDirectDrawSurface3Impl_SetColorKey,
    IDirectDrawSurface3Impl_SetOverlayPosition,
    IDirectDrawSurface3Impl_SetPalette,
    IDirectDrawSurface3Impl_Unlock,
    IDirectDrawSurface3Impl_UpdateOverlay,
    IDirectDrawSurface3Impl_UpdateOverlayDisplay,
    IDirectDrawSurface3Impl_UpdateOverlayZOrder,
    /* IDirectDrawSurface 2 */
    IDirectDrawSurface3Impl_GetDDInterface,
    IDirectDrawSurface3Impl_PageLock,
    IDirectDrawSurface3Impl_PageUnlock,
    /* IDirectDrawSurface 3 */
    IDirectDrawSurface3Impl_SetSurfaceDesc
};

static const IDirectDrawSurface4Vtbl IDirectDrawSurface4_Vtbl =
{
    /*** IUnknown ***/
    IDirectDrawSurface4Impl_QueryInterface,
    IDirectDrawSurface4Impl_AddRef,
    IDirectDrawSurface4Impl_Release,
    /*** IDirectDrawSurface ***/
    IDirectDrawSurface4Impl_AddAttachedSurface,
    IDirectDrawSurface4Impl_AddOverlayDirtyRect,
    IDirectDrawSurface4Impl_Blt,
    IDirectDrawSurface4Impl_BltBatch,
    IDirectDrawSurface4Impl_BltFast,
    IDirectDrawSurface4Impl_DeleteAttachedSurface,
    IDirectDrawSurface4Impl_EnumAttachedSurfaces,
    IDirectDrawSurface4Impl_EnumOverlayZOrders,
    IDirectDrawSurface4Impl_Flip,
    IDirectDrawSurface4Impl_GetAttachedSurface,
    IDirectDrawSurface4Impl_GetBltStatus,
    IDirectDrawSurface4Impl_GetCaps,
    IDirectDrawSurface4Impl_GetClipper,
    IDirectDrawSurface4Impl_GetColorKey,
    IDirectDrawSurface4Impl_GetDC,
    IDirectDrawSurface4Impl_GetFlipStatus,
    IDirectDrawSurface4Impl_GetOverlayPosition,
    IDirectDrawSurface4Impl_GetPalette,
    IDirectDrawSurface4Impl_GetPixelFormat,
    IDirectDrawSurface4Impl_GetSurfaceDesc,
    IDirectDrawSurface4Impl_Initialize,
    IDirectDrawSurface4Impl_IsLost,
    IDirectDrawSurface4Impl_Lock,
    IDirectDrawSurface4Impl_ReleaseDC,
    IDirectDrawSurface4Impl_Restore,
    IDirectDrawSurface4Impl_SetClipper,
    IDirectDrawSurface4Impl_SetColorKey,
    IDirectDrawSurface4Impl_SetOverlayPosition,
    IDirectDrawSurface4Impl_SetPalette,
    IDirectDrawSurface4Impl_Unlock,
    IDirectDrawSurface4Impl_UpdateOverlay,
    IDirectDrawSurface4Impl_UpdateOverlayDisplay,
    IDirectDrawSurface4Impl_UpdateOverlayZOrder,
    /*** IDirectDrawSurface2 ***/
    IDirectDrawSurface4Impl_GetDDInterface,
    IDirectDrawSurface4Impl_PageLock,
    IDirectDrawSurface4Impl_PageUnlock,
    /*** IDirectDrawSurface3 ***/
    IDirectDrawSurface4Impl_SetSurfaceDesc,
    /*** IDirectDrawSurface4 ***/
    IDirectDrawSurface4Impl_SetPrivateData,
    IDirectDrawSurface4Impl_GetPrivateData,
    IDirectDrawSurface4Impl_FreePrivateData,
    IDirectDrawSurface4Impl_GetUniquenessValue,
    IDirectDrawSurface4Impl_ChangeUniquenessValue,
};

static IDirectDrawSurfaceImpl *unsafe_impl_from_IDirectDrawSurface3(IDirectDrawSurface3 *iface)
{
    if (!iface) return NULL;
    if (iface->lpVtbl != &IDirectDrawSurface3_Vtbl) return NULL;
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirectDrawSurface3_iface);
}

IDirectDrawSurfaceImpl *unsafe_impl_from_IDirectDrawSurface4(IDirectDrawSurface4 *iface)
{
    if (!iface) return NULL;
    if (iface->lpVtbl != &IDirectDrawSurface4_Vtbl) return NULL;
    return impl_from_IDirectDrawSurface4(iface);
}

/* dds_get_outer
 *
 * Given a surface from ddraw.dll it retrieves the pointer to the ddrawex.dll wrapper around it
 *
 * Parameters:
 *  inner: ddraw.dll surface to retrieve the outer surface from
 *
 * Returns:
 *  The surface wrapper. If there is none yet, a new one is created
 */
IDirectDrawSurface4 *dds_get_outer(IDirectDrawSurface4 *inner)
{
    IDirectDrawSurface4 *outer = NULL;
    DWORD size = sizeof(outer);
    HRESULT hr;
    if(!inner) return NULL;

    hr = IDirectDrawSurface4_GetPrivateData(inner,
                                            &IID_DDrawexPriv,
                                            &outer,
                                            &size);
    if(FAILED(hr) || outer == NULL)
    {
        IDirectDrawSurfaceImpl *impl;

        TRACE("Creating new ddrawex surface wrapper for surface %p\n", inner);
        impl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*impl));
        impl->ref = 1;
        impl->IDirectDrawSurface3_iface.lpVtbl = &IDirectDrawSurface3_Vtbl;
        impl->IDirectDrawSurface4_iface.lpVtbl = &IDirectDrawSurface4_Vtbl;
        IDirectDrawSurface4_AddRef(inner);
        impl->parent = inner;

        outer = &impl->IDirectDrawSurface4_iface;

        hr = IDirectDrawSurface4_SetPrivateData(inner,
                                                &IID_DDrawexPriv,
                                                &outer,
                                                sizeof(outer),
                                                0 /* Flags */);
        if(FAILED(hr))
        {
            ERR("IDirectDrawSurface4_SetPrivateData failed\n");
        }
    }

    return outer;
}

HRESULT prepare_permanent_dc(IDirectDrawSurface4 *iface)
{
    IDirectDrawSurfaceImpl *This = unsafe_impl_from_IDirectDrawSurface4(iface);
    HRESULT hr;
    This->permanent_dc = TRUE;

    hr = IDirectDrawSurface4_GetDC(This->parent, &This->hdc);
    if(FAILED(hr)) return hr;
    hr = IDirectDrawSurface4_ReleaseDC(This->parent, This->hdc);
    return hr;
}
