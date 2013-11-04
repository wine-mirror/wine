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

#define COBJMACROS
#define NONAMELESSUNION

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#include "ddraw.h"
#include "d3d.h"

#include "ddrawex_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddrawex);

/******************************************************************************
 * Helper functions for COM management
 ******************************************************************************/
static IDirectDrawImpl *impl_from_IDirectDraw(IDirectDraw *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawImpl, IDirectDraw_iface);
}

static IDirectDrawImpl *impl_from_IDirectDraw2(IDirectDraw2 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawImpl, IDirectDraw2_iface);
}

static IDirectDrawImpl *impl_from_IDirectDraw3(IDirectDraw3 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawImpl, IDirectDraw3_iface);
}

static IDirectDrawImpl *impl_from_IDirectDraw4(IDirectDraw4 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawImpl, IDirectDraw4_iface);
}

/******************************************************************************
 * IDirectDraw4 -> ddraw.dll wrappers
 ******************************************************************************/
static HRESULT WINAPI IDirectDraw4Impl_QueryInterface(IDirectDraw4 *iface, REFIID refiid,
        void **obj)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(refiid), obj);
    *obj = NULL;

    if(!refiid)
    {
        return DDERR_INVALIDPARAMS;
    }

    if (IsEqualGUID( &IID_IDirectDraw7, refiid ) )
    {
        WARN("IDirectDraw7 not allowed in ddrawex.dll\n");
        return E_NOINTERFACE;
    }
    else if ( IsEqualGUID( &IID_IUnknown, refiid ) ||
              IsEqualGUID( &IID_IDirectDraw4, refiid ) )
    {
        *obj = &This->IDirectDraw4_iface;
        TRACE("(%p) Returning IDirectDraw4 interface at %p\n", This, *obj);
        IDirectDraw4_AddRef(&This->IDirectDraw4_iface);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw3, refiid ) )
    {
        *obj = &This->IDirectDraw3_iface;
        TRACE("(%p) Returning IDirectDraw3 interface at %p\n", This, *obj);
        IDirectDraw3_AddRef(&This->IDirectDraw3_iface);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw2, refiid ) )
    {
        *obj = &This->IDirectDraw2_iface;
        TRACE("(%p) Returning IDirectDraw2 interface at %p\n", This, *obj);
        IDirectDraw2_AddRef(&This->IDirectDraw2_iface);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw, refiid ) )
    {
        *obj = &This->IDirectDraw_iface;
        TRACE("(%p) Returning IDirectDraw interface at %p\n", This, *obj);
        IDirectDraw_AddRef(&This->IDirectDraw_iface);
    }
    else if ( IsEqualGUID( &IID_IDirect3D  , refiid ) ||
              IsEqualGUID( &IID_IDirect3D2 , refiid ) ||
              IsEqualGUID( &IID_IDirect3D3 , refiid ) ||
              IsEqualGUID( &IID_IDirect3D7 , refiid ) )
    {
        WARN("Direct3D not allowed in ddrawex.dll\n");
        return E_NOINTERFACE;
    }
    /* Unknown interface */
    else
    {
        ERR("(%p)->(%s, %p): No interface found\n", This, debugstr_guid(refiid), obj);
        return E_NOINTERFACE;
    }
    TRACE("Returning S_OK\n");
    return S_OK;
}

static HRESULT WINAPI IDirectDraw3Impl_QueryInterface(IDirectDraw3 *iface, REFIID refiid,
        void **obj)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_QueryInterface(&This->IDirectDraw4_iface, refiid, obj);
}

static HRESULT WINAPI IDirectDraw2Impl_QueryInterface(IDirectDraw2 *iface, REFIID refiid,
        void **obj)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_QueryInterface(&This->IDirectDraw4_iface, refiid, obj);
}

static HRESULT WINAPI IDirectDrawImpl_QueryInterface(IDirectDraw *iface, REFIID refiid, void **obj)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_QueryInterface(&This->IDirectDraw4_iface, refiid, obj);
}

static ULONG WINAPI IDirectDraw4Impl_AddRef(IDirectDraw4 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : incrementing refcount from %u.\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirectDraw3Impl_AddRef(IDirectDraw3 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_AddRef(&This->IDirectDraw4_iface);
}

static ULONG WINAPI IDirectDraw2Impl_AddRef(IDirectDraw2 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_AddRef(&This->IDirectDraw4_iface);
}

static ULONG WINAPI IDirectDrawImpl_AddRef(IDirectDraw *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_AddRef(&This->IDirectDraw4_iface);
}

static ULONG WINAPI IDirectDraw4Impl_Release(IDirectDraw4 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : decrementing refcount to %u.\n", This, ref);

    if(ref == 0)
    {
        TRACE("Destroying object\n");
        IDirectDraw4_Release(This->parent);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static ULONG WINAPI IDirectDraw3Impl_Release(IDirectDraw3 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_Release(&This->IDirectDraw4_iface);
}

static ULONG WINAPI IDirectDraw2Impl_Release(IDirectDraw2 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_Release(&This->IDirectDraw4_iface);
}

static ULONG WINAPI IDirectDrawImpl_Release(IDirectDraw *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_Release(&This->IDirectDraw4_iface);
}

static HRESULT WINAPI IDirectDraw4Impl_Compact(IDirectDraw4 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)\n", This);

    return IDirectDraw4_Compact(This->parent);
}

static HRESULT WINAPI IDirectDraw3Impl_Compact(IDirectDraw3 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_Compact(&This->IDirectDraw4_iface);
}

static HRESULT WINAPI IDirectDraw2Impl_Compact(IDirectDraw2 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_Compact(&This->IDirectDraw4_iface);
}

static HRESULT WINAPI IDirectDrawImpl_Compact(IDirectDraw *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_Compact(&This->IDirectDraw4_iface);
}

static HRESULT WINAPI IDirectDraw4Impl_CreateClipper(IDirectDraw4 *iface, DWORD Flags,
        IDirectDrawClipper **clipper, IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)->(0x%08x, %p, %p)\n", This, Flags, clipper, UnkOuter);

    if(UnkOuter != NULL)
    {
        /* This may require a wrapper interface for clippers too which handles this */
        FIXME("Test and implement Aggregation for ddrawex clippers\n");
    }

    return IDirectDraw4_CreateClipper(This->parent, Flags, clipper, UnkOuter);
}

static HRESULT WINAPI IDirectDraw3Impl_CreateClipper(IDirectDraw3 *iface, DWORD Flags,
        IDirectDrawClipper **clipper, IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_CreateClipper(&This->IDirectDraw4_iface, Flags, clipper, UnkOuter);
}

static HRESULT WINAPI IDirectDraw2Impl_CreateClipper(IDirectDraw2 *iface, DWORD Flags,
        IDirectDrawClipper **clipper, IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_CreateClipper(&This->IDirectDraw4_iface, Flags, clipper, UnkOuter);
}

static HRESULT WINAPI IDirectDrawImpl_CreateClipper(IDirectDraw *iface, DWORD Flags,
        IDirectDrawClipper **clipper, IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_CreateClipper(&This->IDirectDraw4_iface, Flags, clipper, UnkOuter);
}

static HRESULT WINAPI IDirectDraw4Impl_CreatePalette(IDirectDraw4 *iface, DWORD Flags,
        PALETTEENTRY *ColorTable, IDirectDrawPalette **Palette, IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)(0x%08x,%p,%p,%p)\n", This, Flags, ColorTable, Palette, UnkOuter);

    if(UnkOuter != NULL)
    {
        /* This may require a wrapper interface for palettes too which handles this */
        FIXME("Test and implement Aggregation for ddrawex palettes\n");
    }

    return IDirectDraw4_CreatePalette(This->parent, Flags, ColorTable, Palette, UnkOuter);
}

static HRESULT WINAPI IDirectDraw3Impl_CreatePalette(IDirectDraw3 *iface, DWORD Flags,
        PALETTEENTRY *ColorTable, IDirectDrawPalette **Palette, IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_CreatePalette(&This->IDirectDraw4_iface, Flags, ColorTable, Palette,
            UnkOuter);
}

static HRESULT WINAPI IDirectDraw2Impl_CreatePalette(IDirectDraw2 *iface, DWORD Flags,
        PALETTEENTRY *ColorTable, IDirectDrawPalette **Palette, IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_CreatePalette(&This->IDirectDraw4_iface, Flags, ColorTable, Palette,
            UnkOuter);
}

static HRESULT WINAPI IDirectDrawImpl_CreatePalette(IDirectDraw *iface, DWORD Flags,
        PALETTEENTRY *ColorTable, IDirectDrawPalette **Palette, IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("Thunking to IDirectDraw4\n");
    return IDirectDraw4_CreatePalette(&This->IDirectDraw4_iface, Flags, ColorTable, Palette,
            UnkOuter);
}

static HRESULT WINAPI IDirectDraw4Impl_CreateSurface(IDirectDraw4 *iface, DDSURFACEDESC2 *DDSD,
        IDirectDrawSurface4 **Surf, IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    HRESULT hr;
    const DWORD perm_dc_flags = DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY;
    BOOL permanent_dc;
    TRACE("(%p)(%p, %p, %p)\n", This, DDSD, Surf, UnkOuter);

    if(UnkOuter != NULL)
    {
        /* Handle this in this dll. Don't forward the UnkOuter to ddraw.dll */
        FIXME("Implement aggregation for ddrawex surfaces\n");
    }

    /* plain ddraw.dll refuses to create a surface that has both VIDMEM and SYSMEM flags
     * set. In ddrawex this succeeds, and the GetDC() call changes the behavior. The DC
     * is permanently valid, and the surface can be locked between GetDC() and ReleaseDC()
     * calls. GetDC() can be called more than once too
     */
    if((DDSD->ddsCaps.dwCaps & perm_dc_flags) == perm_dc_flags)
    {
        permanent_dc = TRUE;
        DDSD->ddsCaps.dwCaps &= ~DDSCAPS_VIDEOMEMORY;
        DDSD->ddsCaps.dwCaps |= DDSCAPS_OWNDC;
    }
    else
    {
        permanent_dc = FALSE;
    }

    hr = IDirectDraw4_CreateSurface(This->parent, DDSD, Surf, UnkOuter);
    *Surf = dds_get_outer(*Surf);
    if(permanent_dc) prepare_permanent_dc(*Surf);
    return hr;
}

void DDSD_to_DDSD2(const DDSURFACEDESC *in, DDSURFACEDESC2 *out)
{
    memset(out, 0, sizeof(*out));
    out->dwSize = sizeof(*out);
    out->dwFlags = in->dwFlags;
    if(in->dwFlags & DDSD_WIDTH) out->dwWidth = in->dwWidth;
    if(in->dwFlags & DDSD_HEIGHT) out->dwHeight = in->dwHeight;
    if(in->dwFlags & DDSD_PIXELFORMAT) out->u4.ddpfPixelFormat = in->ddpfPixelFormat;
    if(in->dwFlags & DDSD_CAPS) out->ddsCaps.dwCaps = in->ddsCaps.dwCaps;
    if(in->dwFlags & DDSD_PITCH) out->u1.lPitch = in->u1.lPitch;
    if(in->dwFlags & DDSD_BACKBUFFERCOUNT) out->dwBackBufferCount = in->dwBackBufferCount;
    if(in->dwFlags & DDSD_ZBUFFERBITDEPTH) out->u2.dwMipMapCount = in->u2.dwZBufferBitDepth; /* same union */
    if(in->dwFlags & DDSD_ALPHABITDEPTH) out->dwAlphaBitDepth = in->dwAlphaBitDepth;
    /* DDraw(native, and wine) does not set the DDSD_LPSURFACE, so always copy */
    out->lpSurface = in->lpSurface;
    if(in->dwFlags & DDSD_CKDESTOVERLAY) out->u3.ddckCKDestOverlay = in->ddckCKDestOverlay;
    if(in->dwFlags & DDSD_CKDESTBLT) out->ddckCKDestBlt = in->ddckCKDestBlt;
    if(in->dwFlags & DDSD_CKSRCOVERLAY) out->ddckCKSrcOverlay = in->ddckCKSrcOverlay;
    if(in->dwFlags & DDSD_CKSRCBLT) out->ddckCKSrcBlt = in->ddckCKSrcBlt;
    if(in->dwFlags & DDSD_MIPMAPCOUNT) out->u2.dwMipMapCount = in->u2.dwMipMapCount;
    if(in->dwFlags & DDSD_REFRESHRATE) out->u2.dwRefreshRate = in->u2.dwRefreshRate;
    if(in->dwFlags & DDSD_LINEARSIZE) out->u1.dwLinearSize = in->u1.dwLinearSize;
    /* Does not exist in DDSURFACEDESC:
     * DDSD_TEXTURESTAGE, DDSD_FVF, DDSD_SRCVBHANDLE,
     */
}

void DDSD2_to_DDSD(const DDSURFACEDESC2 *in, DDSURFACEDESC *out)
{
    memset(out, 0, sizeof(*out));
    out->dwSize = sizeof(*out);
    out->dwFlags = in->dwFlags;
    if(in->dwFlags & DDSD_WIDTH) out->dwWidth = in->dwWidth;
    if(in->dwFlags & DDSD_HEIGHT) out->dwHeight = in->dwHeight;
    if(in->dwFlags & DDSD_PIXELFORMAT) out->ddpfPixelFormat = in->u4.ddpfPixelFormat;
    if(in->dwFlags & DDSD_CAPS) out->ddsCaps.dwCaps = in->ddsCaps.dwCaps;
    if(in->dwFlags & DDSD_PITCH) out->u1.lPitch = in->u1.lPitch;
    if(in->dwFlags & DDSD_BACKBUFFERCOUNT) out->dwBackBufferCount = in->dwBackBufferCount;
    if(in->dwFlags & DDSD_ZBUFFERBITDEPTH) out->u2.dwZBufferBitDepth = in->u2.dwMipMapCount; /* same union */
    if(in->dwFlags & DDSD_ALPHABITDEPTH) out->dwAlphaBitDepth = in->dwAlphaBitDepth;
    /* DDraw(native, and wine) does not set the DDSD_LPSURFACE, so always copy */
    out->lpSurface = in->lpSurface;
    if(in->dwFlags & DDSD_CKDESTOVERLAY) out->ddckCKDestOverlay = in->u3.ddckCKDestOverlay;
    if(in->dwFlags & DDSD_CKDESTBLT) out->ddckCKDestBlt = in->ddckCKDestBlt;
    if(in->dwFlags & DDSD_CKSRCOVERLAY) out->ddckCKSrcOverlay = in->ddckCKSrcOverlay;
    if(in->dwFlags & DDSD_CKSRCBLT) out->ddckCKSrcBlt = in->ddckCKSrcBlt;
    if(in->dwFlags & DDSD_MIPMAPCOUNT) out->u2.dwMipMapCount = in->u2.dwMipMapCount;
    if(in->dwFlags & DDSD_REFRESHRATE) out->u2.dwRefreshRate = in->u2.dwRefreshRate;
    if(in->dwFlags & DDSD_LINEARSIZE) out->u1.dwLinearSize = in->u1.dwLinearSize;
    /* Does not exist in DDSURFACEDESC:
     * DDSD_TEXTURESTAGE, DDSD_FVF, DDSD_SRCVBHANDLE,
     */
    if(in->dwFlags & DDSD_TEXTURESTAGE) WARN("Does not exist in DDSURFACEDESC: DDSD_TEXTURESTAGE\n");
    if(in->dwFlags & DDSD_FVF) WARN("Does not exist in DDSURFACEDESC: DDSD_FVF\n");
    if(in->dwFlags & DDSD_SRCVBHANDLE) WARN("Does not exist in DDSURFACEDESC: DDSD_SRCVBHANDLE\n");
    out->dwFlags &= ~(DDSD_TEXTURESTAGE | DDSD_FVF | DDSD_SRCVBHANDLE);
}

static HRESULT WINAPI IDirectDraw3Impl_CreateSurface(IDirectDraw3 *iface, DDSURFACEDESC *DDSD,
        IDirectDrawSurface **Surf, IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    DDSURFACEDESC2 ddsd2;
    IDirectDrawSurface4 *surf4 = NULL;
    HRESULT hr;
    TRACE("Thunking to IDirectDraw4\n");

    DDSD_to_DDSD2(DDSD, &ddsd2);

    hr = IDirectDraw4_CreateSurface(&This->IDirectDraw4_iface, &ddsd2, &surf4, UnkOuter);
    if(FAILED(hr))
    {
        *Surf = NULL;
        return hr;
    }

    TRACE("Got surface %p\n", surf4);
    IDirectDrawSurface4_QueryInterface(surf4, &IID_IDirectDrawSurface, (void **) Surf);
    IDirectDrawSurface4_Release(surf4);
    return hr;
}

static HRESULT WINAPI IDirectDraw2Impl_CreateSurface(IDirectDraw2 *iface, DDSURFACEDESC *DDSD,
        IDirectDrawSurface **Surf, IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("Thunking to IDirectDraw3\n");
    return IDirectDraw3_CreateSurface(&This->IDirectDraw3_iface, DDSD, Surf, UnkOuter);
}

static HRESULT WINAPI IDirectDrawImpl_CreateSurface(IDirectDraw *iface, DDSURFACEDESC *DDSD,
        IDirectDrawSurface **Surf, IUnknown *UnkOuter)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("Thunking to IDirectDraw3\n");
    return IDirectDraw3_CreateSurface(&This->IDirectDraw3_iface, DDSD, Surf, UnkOuter);
}

static HRESULT WINAPI IDirectDraw4Impl_DuplicateSurface(IDirectDraw4 *iface,
        IDirectDrawSurface4 *src, IDirectDrawSurface4 **dst)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    IDirectDrawSurfaceImpl *surf = unsafe_impl_from_IDirectDrawSurface4(src);

    FIXME("(%p)->(%p,%p). Create a wrapper surface\n", This, src, dst);

    return IDirectDraw4_DuplicateSurface(This->parent, surf ? surf->parent : NULL, dst);
}

static HRESULT WINAPI IDirectDraw3Impl_DuplicateSurface(IDirectDraw3 *iface,
        IDirectDrawSurface *src, IDirectDrawSurface **dst)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    IDirectDrawSurface4 *src_4;
    IDirectDrawSurface4 *dst_4;
    HRESULT hr;

    TRACE("Thunking to IDirectDraw4\n");
    IDirectDrawSurface_QueryInterface(src, &IID_IDirectDrawSurface4, (void **) &src_4);
    hr = IDirectDraw4_DuplicateSurface(&This->IDirectDraw4_iface, src_4, &dst_4);
    IDirectDrawSurface4_Release(src_4);

    if(FAILED(hr))
    {
        *dst = NULL;
        return hr;
    }
    IDirectDrawSurface4_QueryInterface(dst_4, &IID_IDirectDrawSurface, (void **) dst);
    IDirectDrawSurface4_Release(dst_4);
    return hr;
}

static HRESULT WINAPI IDirectDraw2Impl_DuplicateSurface(IDirectDraw2 *iface,
        IDirectDrawSurface *src, IDirectDrawSurface **dst)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("Thunking to IDirectDraw3\n");
    return IDirectDraw3_DuplicateSurface(&This->IDirectDraw3_iface, src, dst);
}

static HRESULT WINAPI IDirectDrawImpl_DuplicateSurface(IDirectDraw *iface, IDirectDrawSurface *src,
        IDirectDrawSurface **dst)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("Thunking to IDirectDraw3\n");
    return IDirectDraw3_DuplicateSurface(&This->IDirectDraw3_iface, src, dst);
}

static HRESULT WINAPI IDirectDraw4Impl_EnumDisplayModes(IDirectDraw4 *iface, DWORD Flags,
        DDSURFACEDESC2 *DDSD, void *Context, LPDDENUMMODESCALLBACK2 cb)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)->(0x%08x,%p,%p,%p)\n", This, Flags, DDSD, Context, cb);

    return IDirectDraw4_EnumDisplayModes(This->parent, Flags, DDSD, Context, cb);
}

struct enummodes_ctx
{
    LPDDENUMMODESCALLBACK orig_cb;
    void *orig_ctx;
};

static HRESULT WINAPI
enum_modes_cb2(DDSURFACEDESC2 *ddsd2, void *vctx)
{
    struct enummodes_ctx *ctx = vctx;
    DDSURFACEDESC ddsd;

    DDSD2_to_DDSD(ddsd2, &ddsd);
    return ctx->orig_cb(&ddsd, ctx->orig_ctx);
}

static HRESULT WINAPI IDirectDraw3Impl_EnumDisplayModes(IDirectDraw3 *iface, DWORD Flags,
        DDSURFACEDESC *DDSD, void *Context, LPDDENUMMODESCALLBACK cb)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    DDSURFACEDESC2 ddsd2;
    struct enummodes_ctx ctx;
    TRACE("(%p)->(0x%08x,%p,%p,%p): Thunking to IDirectDraw4\n", This, Flags, DDSD, Context, cb);

    DDSD_to_DDSD2(DDSD, &ddsd2);
    ctx.orig_cb = cb;
    ctx.orig_ctx = Context;
    return IDirectDraw4_EnumDisplayModes(&This->IDirectDraw4_iface, Flags, &ddsd2, &ctx,
            enum_modes_cb2);
}

static HRESULT WINAPI IDirectDraw2Impl_EnumDisplayModes(IDirectDraw2 *iface, DWORD Flags,
        DDSURFACEDESC *DDSD, void *Context, LPDDENUMMODESCALLBACK cb)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p)->(0x%08x,%p,%p,%p): Thunking to IDirectDraw3\n", This, Flags, DDSD, Context, cb);
    return IDirectDraw3_EnumDisplayModes(&This->IDirectDraw3_iface, Flags, DDSD, Context, cb);
}

static HRESULT WINAPI IDirectDrawImpl_EnumDisplayModes(IDirectDraw *iface, DWORD Flags,
        DDSURFACEDESC *DDSD, void *Context, LPDDENUMMODESCALLBACK cb)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p)->(0x%08x,%p,%p,%p): Thunking to IDirectDraw3\n", This, Flags, DDSD, Context, cb);
    return IDirectDraw3_EnumDisplayModes(&This->IDirectDraw3_iface, Flags, DDSD, Context, cb);
}

struct enumsurfaces4_ctx
{
    LPDDENUMSURFACESCALLBACK2 orig_cb;
    void *orig_ctx;
};

static HRESULT WINAPI
enum_surfaces_wrapper(IDirectDrawSurface4 *surf4, DDSURFACEDESC2 *ddsd2, void *vctx)
{
    struct enumsurfaces4_ctx *ctx = vctx;
    IDirectDrawSurface4 *outer = dds_get_outer(surf4);
    IDirectDrawSurface4_AddRef(outer);
    IDirectDrawSurface4_Release(surf4);
    TRACE("Returning wrapper surface %p for enumerated inner surface %p\n", outer, surf4);
    return ctx->orig_cb(outer, ddsd2, ctx->orig_ctx);
}

static HRESULT WINAPI IDirectDraw4Impl_EnumSurfaces(IDirectDraw4 *iface, DWORD Flags,
        DDSURFACEDESC2 *DDSD, void *Context, LPDDENUMSURFACESCALLBACK2 Callback)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    struct enumsurfaces4_ctx ctx;
    TRACE("(%p)->(0x%08x,%p,%p,%p)\n", This, Flags, DDSD, Context, Callback);

    ctx.orig_cb = Callback;
    ctx.orig_ctx = Context;
    return IDirectDraw4Impl_EnumSurfaces(This->parent, Flags, DDSD, &ctx, enum_surfaces_wrapper);
}

struct enumsurfaces_ctx
{
    LPDDENUMSURFACESCALLBACK orig_cb;
    void *orig_ctx;
};

static HRESULT WINAPI
enum_surfaces_cb2(IDirectDrawSurface4 *surf4, DDSURFACEDESC2 *ddsd2, void *vctx)
{
    struct enumsurfaces_ctx *ctx = vctx;
    IDirectDrawSurface *surf1;
    DDSURFACEDESC ddsd;

    /* Keep the reference, it goes to the application */
    IDirectDrawSurface4_QueryInterface(surf4, &IID_IDirectDrawSurface, (void **) &surf1);
    /* Release the reference this function got */
    IDirectDrawSurface4_Release(surf4);

    DDSD2_to_DDSD(ddsd2, &ddsd);
    return ctx->orig_cb(surf1, &ddsd, ctx->orig_ctx);
}

static HRESULT WINAPI IDirectDraw3Impl_EnumSurfaces(IDirectDraw3 *iface, DWORD Flags,
        DDSURFACEDESC *DDSD, void *Context, LPDDENUMSURFACESCALLBACK Callback)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    DDSURFACEDESC2 ddsd2;
    struct enumsurfaces_ctx ctx;
    TRACE("(%p)->(0x%08x,%p,%p,%p): Thunking to IDirectDraw4\n", This, Flags, DDSD, Context, Callback);

    DDSD_to_DDSD2(DDSD, &ddsd2);
    ctx.orig_cb = Callback;
    ctx.orig_ctx = Context;
    return IDirectDraw4_EnumSurfaces(&This->IDirectDraw4_iface, Flags, &ddsd2, &ctx,
            enum_surfaces_cb2);
}

static HRESULT WINAPI IDirectDraw2Impl_EnumSurfaces(IDirectDraw2 *iface, DWORD Flags,
        DDSURFACEDESC *DDSD, void *Context, LPDDENUMSURFACESCALLBACK Callback)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p)->(0x%08x,%p,%p,%p): Thunking to IDirectDraw3\n", This, Flags, DDSD, Context, Callback);
    return IDirectDraw3_EnumSurfaces(&This->IDirectDraw3_iface, Flags, DDSD, Context, Callback);
}

static HRESULT WINAPI IDirectDrawImpl_EnumSurfaces(IDirectDraw *iface, DWORD Flags,
        DDSURFACEDESC *DDSD, void *Context, LPDDENUMSURFACESCALLBACK Callback)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p)->(0x%08x,%p,%p,%p): Thunking to IDirectDraw3\n", This, Flags, DDSD, Context, Callback);
    return IDirectDraw3_EnumSurfaces(&This->IDirectDraw3_iface, Flags, DDSD, Context, Callback);
}

static HRESULT WINAPI IDirectDraw4Impl_FlipToGDISurface(IDirectDraw4 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)\n", This);

    return IDirectDraw4_FlipToGDISurface(This->parent);
}

static HRESULT WINAPI IDirectDraw3Impl_FlipToGDISurface(IDirectDraw3 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("(%p). Thunking to IDirectDraw4\n", This);
    return IDirectDraw4_FlipToGDISurface(&This->IDirectDraw4_iface);
}

static HRESULT WINAPI IDirectDraw2Impl_FlipToGDISurface(IDirectDraw2 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p). Thunking to IDirectDraw4\n", This);
    return IDirectDraw4_FlipToGDISurface(&This->IDirectDraw4_iface);
}

static HRESULT WINAPI IDirectDrawImpl_FlipToGDISurface(IDirectDraw *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p). Thunking to IDirectDraw4\n", This);
    return IDirectDraw4_FlipToGDISurface(&This->IDirectDraw4_iface);
}

static HRESULT WINAPI IDirectDraw4Impl_GetCaps(IDirectDraw4 *iface, DDCAPS *DriverCaps,
        DDCAPS *HELCaps)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)->(%p,%p)\n", This, DriverCaps, HELCaps);
    return IDirectDraw4_GetCaps(This->parent, DriverCaps, HELCaps);
}

static HRESULT WINAPI IDirectDraw3Impl_GetCaps(IDirectDraw3 *iface, DDCAPS *DriverCaps,
        DDCAPS *HELCaps)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("(%p)->(%p,%p). Thunking to IDirectDraw4\n", This, DriverCaps, HELCaps);
    return IDirectDraw4_GetCaps(&This->IDirectDraw4_iface, DriverCaps, HELCaps);
}

static HRESULT WINAPI IDirectDraw2Impl_GetCaps(IDirectDraw2 *iface, DDCAPS *DriverCaps,
        DDCAPS *HELCaps)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p)->(%p,%p). Thunking to IDirectDraw4\n", This, DriverCaps, HELCaps);
    return IDirectDraw4_GetCaps(&This->IDirectDraw4_iface, DriverCaps, HELCaps);
}

static HRESULT WINAPI IDirectDrawImpl_GetCaps(IDirectDraw *iface, DDCAPS *DriverCaps,
        DDCAPS *HELCaps)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p)->(%p,%p). Thunking to IDirectDraw4\n", This, DriverCaps, HELCaps);
    return IDirectDraw4_GetCaps(&This->IDirectDraw4_iface, DriverCaps, HELCaps);
}

static HRESULT WINAPI IDirectDraw4Impl_GetDisplayMode(IDirectDraw4 *iface, DDSURFACEDESC2 *DDSD)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)->(%p)\n", This, DDSD);
    return IDirectDraw4_GetDisplayMode(This->parent, DDSD);
}

static HRESULT WINAPI IDirectDraw3Impl_GetDisplayMode(IDirectDraw3 *iface, DDSURFACEDESC *DDSD)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    DDSURFACEDESC2 ddsd2;
    HRESULT hr;

    TRACE("(%p)->(%p): Thunking to IDirectDraw4\n", This, DDSD);
    hr = IDirectDraw4_GetDisplayMode(&This->IDirectDraw4_iface, &ddsd2);
    DDSD2_to_DDSD(&ddsd2, DDSD);
    return hr;
}

static HRESULT WINAPI IDirectDraw2Impl_GetDisplayMode(IDirectDraw2 *iface, DDSURFACEDESC *DDSD)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDraw3\n", This, DDSD);
    return IDirectDraw3_GetDisplayMode(&This->IDirectDraw3_iface, DDSD);
}

static HRESULT WINAPI IDirectDrawImpl_GetDisplayMode(IDirectDraw *iface, DDSURFACEDESC *DDSD)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDraw3\n", This, DDSD);
    return IDirectDraw3_GetDisplayMode(&This->IDirectDraw3_iface, DDSD);
}

static HRESULT WINAPI IDirectDraw4Impl_GetFourCCCodes(IDirectDraw4 *iface, DWORD *NumCodes,
        DWORD *Codes)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)->(%p, %p):\n", This, NumCodes, Codes);
    return IDirectDraw4_GetFourCCCodes(This->parent, NumCodes, Codes);
}

static HRESULT WINAPI IDirectDraw3Impl_GetFourCCCodes(IDirectDraw3 *iface, DWORD *NumCodes,
        DWORD *Codes)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("(%p)->(%p, %p): Thunking to IDirectDraw4\n", This, NumCodes, Codes);
    return IDirectDraw4_GetFourCCCodes(&This->IDirectDraw4_iface, NumCodes, Codes);
}

static HRESULT WINAPI IDirectDraw2Impl_GetFourCCCodes(IDirectDraw2 *iface, DWORD *NumCodes,
        DWORD *Codes)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p)->(%p, %p): Thunking to IDirectDraw4\n", This, NumCodes, Codes);
    return IDirectDraw4_GetFourCCCodes(&This->IDirectDraw4_iface, NumCodes, Codes);
}

static HRESULT WINAPI IDirectDrawImpl_GetFourCCCodes(IDirectDraw *iface, DWORD *NumCodes,
        DWORD *Codes)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p)->(%p, %p): Thunking to IDirectDraw4\n", This, NumCodes, Codes);
    return IDirectDraw4_GetFourCCCodes(&This->IDirectDraw4_iface, NumCodes, Codes);
}

static HRESULT WINAPI IDirectDraw4Impl_GetGDISurface(IDirectDraw4 *iface,
        IDirectDrawSurface4 **GDISurface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    IDirectDrawSurface4 *inner = NULL;
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, GDISurface);

    hr = IDirectDraw4_GetGDISurface(This->parent, &inner);
    if(SUCCEEDED(hr))
    {
        *GDISurface = dds_get_outer(inner);
        IDirectDrawSurface4_AddRef(*GDISurface);
        IDirectDrawSurface4_Release(inner);
    }
    else
    {
        *GDISurface = NULL;
    }
    return hr;
}

static HRESULT WINAPI IDirectDraw3Impl_GetGDISurface(IDirectDraw3 *iface,
        IDirectDrawSurface **GDISurface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    IDirectDrawSurface4 *surf4;
    HRESULT hr;
    TRACE("(%p)->(%p): Thunking to IDirectDraw4\n", This, GDISurface);

    hr = IDirectDraw4_GetGDISurface(&This->IDirectDraw4_iface, &surf4);
    if(FAILED(hr)) {
        *GDISurface = NULL;
        return hr;
    }

    /* Release the reference we got from the DDraw4 call, and pass a reference to the caller */
    IDirectDrawSurface4_QueryInterface(surf4, &IID_IDirectDrawSurface, (void **) GDISurface);
    IDirectDrawSurface4_Release(surf4);
    return hr;
}

static HRESULT WINAPI IDirectDraw2Impl_GetGDISurface(IDirectDraw2 *iface,
        IDirectDrawSurface **GDISurface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDraw3\n", This, GDISurface);
    return IDirectDraw3_GetGDISurface(&This->IDirectDraw3_iface, GDISurface);
}

static HRESULT WINAPI IDirectDrawImpl_GetGDISurface(IDirectDraw *iface,
        IDirectDrawSurface **GDISurface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDraw3\n", This, GDISurface);
    return IDirectDraw3_GetGDISurface(&This->IDirectDraw3_iface, GDISurface);
}

static HRESULT WINAPI IDirectDraw4Impl_GetMonitorFrequency(IDirectDraw4 *iface, DWORD *Freq)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)->(%p)\n", This, Freq);
    return IDirectDraw4_GetMonitorFrequency(This->parent, Freq);
}

static HRESULT WINAPI IDirectDraw3Impl_GetMonitorFrequency(IDirectDraw3 *iface, DWORD *Freq)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDraw4\n", This, Freq);
    return IDirectDraw4_GetMonitorFrequency(&This->IDirectDraw4_iface, Freq);
}

static HRESULT WINAPI IDirectDraw2Impl_GetMonitorFrequency(IDirectDraw2 *iface, DWORD *Freq)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDraw4\n", This, Freq);
    return IDirectDraw4_GetMonitorFrequency(&This->IDirectDraw4_iface, Freq);
}

static HRESULT WINAPI IDirectDrawImpl_GetMonitorFrequency(IDirectDraw *iface, DWORD *Freq)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDraw4\n", This, Freq);
    return IDirectDraw4_GetMonitorFrequency(&This->IDirectDraw4_iface, Freq);
}

static HRESULT WINAPI IDirectDraw4Impl_GetScanLine(IDirectDraw4 *iface, DWORD *Scanline)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)->(%p)\n", This, Scanline);
    return IDirectDraw4_GetScanLine(This->parent, Scanline);
}

static HRESULT WINAPI IDirectDraw3Impl_GetScanLine(IDirectDraw3 *iface, DWORD *Scanline)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDraw4\n", This, Scanline);
    return IDirectDraw4_GetScanLine(&This->IDirectDraw4_iface, Scanline);
}

static HRESULT WINAPI IDirectDraw2Impl_GetScanLine(IDirectDraw2 *iface, DWORD *Scanline)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDraw4\n", This, Scanline);
    return IDirectDraw4_GetScanLine(&This->IDirectDraw4_iface, Scanline);
}

static HRESULT WINAPI IDirectDrawImpl_GetScanLine(IDirectDraw *iface, DWORD *Scanline)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDraw4\n", This, Scanline);
    return IDirectDraw4_GetScanLine(&This->IDirectDraw4_iface, Scanline);
}

static HRESULT WINAPI IDirectDraw4Impl_GetVerticalBlankStatus(IDirectDraw4 *iface, BOOL *status)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)->(%p)\n", This, status);
    return IDirectDraw4_GetVerticalBlankStatus(This->parent, status);
}

static HRESULT WINAPI IDirectDraw3Impl_GetVerticalBlankStatus(IDirectDraw3 *iface, BOOL *status)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDraw4\n", This, status);
    return IDirectDraw4_GetVerticalBlankStatus(&This->IDirectDraw4_iface, status);
}

static HRESULT WINAPI IDirectDraw2Impl_GetVerticalBlankStatus(IDirectDraw2 *iface, BOOL *status)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDraw4\n", This, status);
    return IDirectDraw4_GetVerticalBlankStatus(&This->IDirectDraw4_iface, status);
}

static HRESULT WINAPI IDirectDrawImpl_GetVerticalBlankStatus(IDirectDraw *iface, BOOL *status)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p)->(%p): Thunking to IDirectDraw4\n", This, status);
    return IDirectDraw4_GetVerticalBlankStatus(&This->IDirectDraw4_iface, status);
}

static HRESULT WINAPI IDirectDraw4Impl_Initialize(IDirectDraw4 *iface, GUID *Guid)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_guid(Guid));
    return IDirectDraw4_Initialize(This->parent, Guid);
}

static HRESULT WINAPI IDirectDraw3Impl_Initialize(IDirectDraw3 *iface, GUID *Guid)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("(%p)->(%s): Thunking to IDirectDraw4\n", This, debugstr_guid(Guid));
    return IDirectDraw4_Initialize(&This->IDirectDraw4_iface, Guid);
}

static HRESULT WINAPI IDirectDraw2Impl_Initialize(IDirectDraw2 *iface, GUID *Guid)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p)->(%s): Thunking to IDirectDraw4\n", This, debugstr_guid(Guid));
    return IDirectDraw4_Initialize(&This->IDirectDraw4_iface, Guid);
}

static HRESULT WINAPI IDirectDrawImpl_Initialize(IDirectDraw *iface, GUID *Guid)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p)->(%s): Thunking to IDirectDraw4\n", This, debugstr_guid(Guid));
    return IDirectDraw4_Initialize(&This->IDirectDraw4_iface, Guid);
}

static HRESULT WINAPI IDirectDraw4Impl_RestoreDisplayMode(IDirectDraw4 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)\n", This);
    return IDirectDraw4_RestoreDisplayMode(This->parent);
}

static HRESULT WINAPI IDirectDraw3Impl_RestoreDisplayMode(IDirectDraw3 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("(%p): Thunking to IDirectDraw4\n", This);
    return IDirectDraw4_RestoreDisplayMode(&This->IDirectDraw4_iface);
}

static HRESULT WINAPI IDirectDraw2Impl_RestoreDisplayMode(IDirectDraw2 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p): Thunking to IDirectDraw4\n", This);
    return IDirectDraw4_RestoreDisplayMode(&This->IDirectDraw4_iface);
}

static HRESULT WINAPI IDirectDrawImpl_RestoreDisplayMode(IDirectDraw *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p): Thunking to IDirectDraw4\n", This);
    return IDirectDraw4_RestoreDisplayMode(&This->IDirectDraw4_iface);
}

static HRESULT WINAPI IDirectDraw4Impl_SetCooperativeLevel(IDirectDraw4 *iface, HWND hwnd,
        DWORD cooplevel)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)->(%p, 0x%08x)\n", This, hwnd, cooplevel);
    return IDirectDraw4_SetCooperativeLevel(This->parent, hwnd, cooplevel);
}

static HRESULT WINAPI IDirectDraw3Impl_SetCooperativeLevel(IDirectDraw3 *iface, HWND hwnd,
        DWORD cooplevel)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("(%p)->(%p, 0x%08x): Thunking to IDirectDraw4\n", This, hwnd, cooplevel);
    return IDirectDraw4_SetCooperativeLevel(&This->IDirectDraw4_iface, hwnd, cooplevel);
}

static HRESULT WINAPI IDirectDraw2Impl_SetCooperativeLevel(IDirectDraw2 *iface, HWND hwnd,
        DWORD cooplevel)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p)->(%p, 0x%08x): Thunking to IDirectDraw4\n", This, hwnd, cooplevel);
    return IDirectDraw4_SetCooperativeLevel(&This->IDirectDraw4_iface, hwnd, cooplevel);
}

static HRESULT WINAPI IDirectDrawImpl_SetCooperativeLevel(IDirectDraw *iface, HWND hwnd,
        DWORD cooplevel)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p)->(%p, 0x%08x): Thunking to IDirectDraw4\n", This, hwnd, cooplevel);
    return IDirectDraw4_SetCooperativeLevel(&This->IDirectDraw4_iface, hwnd, cooplevel);
}

static HRESULT WINAPI IDirectDraw4Impl_SetDisplayMode(IDirectDraw4 *iface, DWORD Width,
        DWORD Height, DWORD BPP, DWORD RefreshRate, DWORD Flags)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)->(%u, %u, %u, %u, 0x%08x)\n", This, Width, Height, BPP, RefreshRate, Flags);
    return IDirectDraw4_SetDisplayMode(This->parent, Width, Height, BPP, RefreshRate, Flags);
}

static HRESULT WINAPI IDirectDraw3Impl_SetDisplayMode(IDirectDraw3 *iface, DWORD Width,
        DWORD Height, DWORD BPP, DWORD RefreshRate, DWORD Flags)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("(%p)->(%u, %u, %u, %u, 0x%08x): Thunking to IDirectDraw4\n", This, Width, Height, BPP, RefreshRate, Flags);
    return IDirectDraw3_SetDisplayMode(&This->IDirectDraw4_iface, Width, Height, BPP, RefreshRate,
            Flags);
}

static HRESULT WINAPI IDirectDraw2Impl_SetDisplayMode(IDirectDraw2 *iface, DWORD Width,
        DWORD Height, DWORD BPP, DWORD RefreshRate, DWORD Flags)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p)->(%u, %u, %u, %u, 0x%08x): Thunking to IDirectDraw4\n", This, Width, Height, BPP, RefreshRate, Flags);
    return IDirectDraw3_SetDisplayMode(&This->IDirectDraw4_iface, Width, Height, BPP, RefreshRate,
            Flags);
}

static HRESULT WINAPI IDirectDrawImpl_SetDisplayMode(IDirectDraw *iface, DWORD Width, DWORD Height,
        DWORD BPP)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p)->(%u, %u, %u): Thunking to IDirectDraw4\n", This, Width, Height, BPP);
    return IDirectDraw3_SetDisplayMode(&This->IDirectDraw4_iface, Width, Height, BPP, 0, 0);
}

static HRESULT WINAPI IDirectDraw4Impl_WaitForVerticalBlank(IDirectDraw4 *iface, DWORD Flags,
        HANDLE h)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)->(0x%08x, %p)\n", This, Flags, h);
    return IDirectDraw4_WaitForVerticalBlank(This->parent, Flags, h);
}

static HRESULT WINAPI IDirectDraw3Impl_WaitForVerticalBlank(IDirectDraw3 *iface, DWORD Flags,
        HANDLE h)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    TRACE("(%p)->(0x%08x, %p): Thunking to IDirectDraw4\n", This, Flags, h);
    return IDirectDraw4_WaitForVerticalBlank(&This->IDirectDraw4_iface, Flags, h);
}

static HRESULT WINAPI IDirectDraw2Impl_WaitForVerticalBlank(IDirectDraw2 *iface, DWORD Flags,
        HANDLE h)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    TRACE("(%p)->(0x%08x, %p): Thunking to IDirectDraw4\n", This, Flags, h);
    return IDirectDraw4_WaitForVerticalBlank(&This->IDirectDraw4_iface, Flags, h);
}

static HRESULT WINAPI IDirectDrawImpl_WaitForVerticalBlank(IDirectDraw *iface, DWORD Flags,
        HANDLE h)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw(iface);
    TRACE("(%p)->(0x%08x, %p): Thunking to IDirectDraw4\n", This, Flags, h);
    return IDirectDraw4_WaitForVerticalBlank(&This->IDirectDraw4_iface, Flags, h);
}

static HRESULT WINAPI IDirectDraw4Impl_GetAvailableVidMem(IDirectDraw4 *iface, DDSCAPS2 *Caps,
        DWORD *total, DWORD *free)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)->(%p, %p, %p)\n", This, Caps, total, free);
    return IDirectDraw4_GetAvailableVidMem(This->parent, Caps, total, free);
}

static HRESULT WINAPI IDirectDraw3Impl_GetAvailableVidMem(IDirectDraw3 *iface, DDSCAPS *Caps,
        DWORD *total, DWORD *free)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    DDSCAPS2 caps2;
    TRACE("(%p)->(%p, %p, %p): Thunking to IDirectDraw4\n", This, Caps, total, free);
    memset(&caps2, 0, sizeof(caps2));
    caps2.dwCaps = Caps->dwCaps;
    return IDirectDraw4_GetAvailableVidMem(&This->IDirectDraw4_iface, &caps2, total, free);
}

static HRESULT WINAPI IDirectDraw2Impl_GetAvailableVidMem(IDirectDraw2 *iface, DDSCAPS *Caps,
        DWORD *total, DWORD *free)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw2(iface);
    DDSCAPS2 caps2;
    TRACE("(%p)->(%p, %p, %p): Thunking to IDirectDraw4\n", This, Caps, total, free);
    memset(&caps2, 0, sizeof(caps2));
    caps2.dwCaps = Caps->dwCaps;
    return IDirectDraw4_GetAvailableVidMem(&This->IDirectDraw4_iface, &caps2, total, free);
}

static HRESULT WINAPI IDirectDraw4Impl_GetSurfaceFromDC(IDirectDraw4 *iface, HDC hdc,
        IDirectDrawSurface4 **Surface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    HRESULT hr;
    TRACE("(%p)->(%p, %p)\n", This, hdc, Surface);
    hr = IDirectDraw4_GetSurfaceFromDC(This->parent,hdc, Surface);

    return hr;
}

static HRESULT WINAPI IDirectDraw3Impl_GetSurfaceFromDC(IDirectDraw3 *iface, HDC hdc,
        IDirectDrawSurface **Surface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw3(iface);
    IDirectDrawSurface4 *surf4, *outer;
    IDirectDrawSurface *inner;
    HRESULT hr;
    TRACE("(%p)->(%p, %p): Thunking to IDirectDraw4\n", This, hdc, Surface);

    if (!Surface) return E_POINTER;

    hr = IDirectDraw4_GetSurfaceFromDC(This->parent, hdc, (IDirectDrawSurface4 **)&inner);
    if(FAILED(hr))
    {
        *Surface = NULL;
        return hr;
    }

    hr = IDirectDrawSurface_QueryInterface(inner, &IID_IDirectDrawSurface4, (void **)&surf4);
    IDirectDrawSurface_Release(inner);
    if (FAILED(hr))
    {
        *Surface = NULL;
        return hr;
    }

    outer = dds_get_outer(surf4);
    hr = IDirectDrawSurface4_QueryInterface(outer, &IID_IDirectDrawSurface, (void **)Surface);
    IDirectDrawSurface4_Release(surf4);
    return hr;
}

static HRESULT WINAPI IDirectDraw4Impl_RestoreAllSurfaces(IDirectDraw4 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)\n", This);
    return IDirectDraw4_RestoreAllSurfaces(This->parent);
}

static HRESULT WINAPI IDirectDraw4Impl_TestCooperativeLevel(IDirectDraw4 *iface)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)\n", This);
    return IDirectDraw4_TestCooperativeLevel(This->parent);
}

static HRESULT WINAPI IDirectDraw4Impl_GetDeviceIdentifier(IDirectDraw4 *iface,
        DDDEVICEIDENTIFIER *DDDI, DWORD Flags)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(iface);
    TRACE("(%p)->(%p,0x%08x)\n", This, DDDI, Flags);
    return IDirectDraw4_GetDeviceIdentifier(This->parent, DDDI, Flags);
}

static const IDirectDrawVtbl IDirectDraw1_Vtbl =
{
    IDirectDrawImpl_QueryInterface,
    IDirectDrawImpl_AddRef,
    IDirectDrawImpl_Release,
    IDirectDrawImpl_Compact,
    IDirectDrawImpl_CreateClipper,
    IDirectDrawImpl_CreatePalette,
    IDirectDrawImpl_CreateSurface,
    IDirectDrawImpl_DuplicateSurface,
    IDirectDrawImpl_EnumDisplayModes,
    IDirectDrawImpl_EnumSurfaces,
    IDirectDrawImpl_FlipToGDISurface,
    IDirectDrawImpl_GetCaps,
    IDirectDrawImpl_GetDisplayMode,
    IDirectDrawImpl_GetFourCCCodes,
    IDirectDrawImpl_GetGDISurface,
    IDirectDrawImpl_GetMonitorFrequency,
    IDirectDrawImpl_GetScanLine,
    IDirectDrawImpl_GetVerticalBlankStatus,
    IDirectDrawImpl_Initialize,
    IDirectDrawImpl_RestoreDisplayMode,
    IDirectDrawImpl_SetCooperativeLevel,
    IDirectDrawImpl_SetDisplayMode,
    IDirectDrawImpl_WaitForVerticalBlank,
};

static const IDirectDraw2Vtbl IDirectDraw2_Vtbl =
{
    IDirectDraw2Impl_QueryInterface,
    IDirectDraw2Impl_AddRef,
    IDirectDraw2Impl_Release,
    IDirectDraw2Impl_Compact,
    IDirectDraw2Impl_CreateClipper,
    IDirectDraw2Impl_CreatePalette,
    IDirectDraw2Impl_CreateSurface,
    IDirectDraw2Impl_DuplicateSurface,
    IDirectDraw2Impl_EnumDisplayModes,
    IDirectDraw2Impl_EnumSurfaces,
    IDirectDraw2Impl_FlipToGDISurface,
    IDirectDraw2Impl_GetCaps,
    IDirectDraw2Impl_GetDisplayMode,
    IDirectDraw2Impl_GetFourCCCodes,
    IDirectDraw2Impl_GetGDISurface,
    IDirectDraw2Impl_GetMonitorFrequency,
    IDirectDraw2Impl_GetScanLine,
    IDirectDraw2Impl_GetVerticalBlankStatus,
    IDirectDraw2Impl_Initialize,
    IDirectDraw2Impl_RestoreDisplayMode,
    IDirectDraw2Impl_SetCooperativeLevel,
    IDirectDraw2Impl_SetDisplayMode,
    IDirectDraw2Impl_WaitForVerticalBlank,
    IDirectDraw2Impl_GetAvailableVidMem
};

static const IDirectDraw3Vtbl IDirectDraw3_Vtbl =
{
    IDirectDraw3Impl_QueryInterface,
    IDirectDraw3Impl_AddRef,
    IDirectDraw3Impl_Release,
    IDirectDraw3Impl_Compact,
    IDirectDraw3Impl_CreateClipper,
    IDirectDraw3Impl_CreatePalette,
    IDirectDraw3Impl_CreateSurface,
    IDirectDraw3Impl_DuplicateSurface,
    IDirectDraw3Impl_EnumDisplayModes,
    IDirectDraw3Impl_EnumSurfaces,
    IDirectDraw3Impl_FlipToGDISurface,
    IDirectDraw3Impl_GetCaps,
    IDirectDraw3Impl_GetDisplayMode,
    IDirectDraw3Impl_GetFourCCCodes,
    IDirectDraw3Impl_GetGDISurface,
    IDirectDraw3Impl_GetMonitorFrequency,
    IDirectDraw3Impl_GetScanLine,
    IDirectDraw3Impl_GetVerticalBlankStatus,
    IDirectDraw3Impl_Initialize,
    IDirectDraw3Impl_RestoreDisplayMode,
    IDirectDraw3Impl_SetCooperativeLevel,
    IDirectDraw3Impl_SetDisplayMode,
    IDirectDraw3Impl_WaitForVerticalBlank,
    IDirectDraw3Impl_GetAvailableVidMem,
    IDirectDraw3Impl_GetSurfaceFromDC,
};

static const IDirectDraw4Vtbl IDirectDraw4_Vtbl =
{
    IDirectDraw4Impl_QueryInterface,
    IDirectDraw4Impl_AddRef,
    IDirectDraw4Impl_Release,
    IDirectDraw4Impl_Compact,
    IDirectDraw4Impl_CreateClipper,
    IDirectDraw4Impl_CreatePalette,
    IDirectDraw4Impl_CreateSurface,
    IDirectDraw4Impl_DuplicateSurface,
    IDirectDraw4Impl_EnumDisplayModes,
    IDirectDraw4Impl_EnumSurfaces,
    IDirectDraw4Impl_FlipToGDISurface,
    IDirectDraw4Impl_GetCaps,
    IDirectDraw4Impl_GetDisplayMode,
    IDirectDraw4Impl_GetFourCCCodes,
    IDirectDraw4Impl_GetGDISurface,
    IDirectDraw4Impl_GetMonitorFrequency,
    IDirectDraw4Impl_GetScanLine,
    IDirectDraw4Impl_GetVerticalBlankStatus,
    IDirectDraw4Impl_Initialize,
    IDirectDraw4Impl_RestoreDisplayMode,
    IDirectDraw4Impl_SetCooperativeLevel,
    IDirectDraw4Impl_SetDisplayMode,
    IDirectDraw4Impl_WaitForVerticalBlank,
    IDirectDraw4Impl_GetAvailableVidMem,
    IDirectDraw4Impl_GetSurfaceFromDC,
    IDirectDraw4Impl_RestoreAllSurfaces,
    IDirectDraw4Impl_TestCooperativeLevel,
    IDirectDraw4Impl_GetDeviceIdentifier
};

HRESULT WINAPI ddrawex_factory_CreateDirectDraw(IDirectDrawFactory *iface, GUID * pGUID, HWND hWnd,
        DWORD dwCoopLevelFlags, DWORD dwReserved, IUnknown *pUnkOuter, IDirectDraw **ppDirectDraw)
{
    HRESULT hr;
    IDirectDrawImpl *object = NULL;
    IDirectDraw *parent = NULL;

    TRACE("(%p)->(%s,%p,0x%08x,0x%08x,%p,%p)\n", iface, debugstr_guid(pGUID), hWnd, dwCoopLevelFlags,
          dwReserved, pUnkOuter, ppDirectDraw);

    if(pUnkOuter)
    {
        FIXME("Implement aggregation in ddrawex's IDirectDraw interface\n");
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if(!object)
    {
        ERR("Out of memory\n");
        hr = E_OUTOFMEMORY;
        goto err;
    }
    object->ref = 1;
    object->IDirectDraw_iface.lpVtbl = &IDirectDraw1_Vtbl;
    object->IDirectDraw2_iface.lpVtbl = &IDirectDraw2_Vtbl;
    object->IDirectDraw3_iface.lpVtbl = &IDirectDraw3_Vtbl;
    object->IDirectDraw4_iface.lpVtbl = &IDirectDraw4_Vtbl;

    hr = DirectDrawCreate(pGUID, &parent, NULL);
    if (FAILED(hr)) goto err;

    hr = IDirectDraw_QueryInterface(parent, &IID_IDirectDraw4, (void **) &object->parent);
    if(FAILED(hr)) goto err;

    hr = IDirectDraw_SetCooperativeLevel(&object->IDirectDraw_iface, hWnd, dwCoopLevelFlags);
    if (FAILED(hr)) goto err;

    *ppDirectDraw = &object->IDirectDraw_iface;
    IDirectDraw_Release(parent);
    return DD_OK;

err:
    if(object && object->parent) IDirectDraw4_Release(object->parent);
    if(parent) IDirectDraw_Release(parent);
    HeapFree(GetProcessHeap(), 0, object);
    *ppDirectDraw = NULL;
    return hr;
}

IDirectDraw4 *dd_get_inner(IDirectDraw4 *outer)
{
    IDirectDrawImpl *This = impl_from_IDirectDraw4(outer);

    if (outer->lpVtbl != &IDirectDraw4_Vtbl) return NULL;
    return This->parent;
}
