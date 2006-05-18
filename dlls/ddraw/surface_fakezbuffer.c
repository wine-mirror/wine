/*		DirectDraw/Direct3D Z-Buffer stand in
 *
 * Copyright 2000 TransGaming Technologies Inc.
 *
 * This class provides a DirectDrawSurface implementation that represents
 * a Z-Buffer surface. However it does not store an image and does not
 * support Lock/Unlock or GetDC. It is merely a placeholder required by the
 * Direct3D architecture.
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

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ddraw.h"
#include "d3d.h"

#include "wine/debug.h"

#include "ddcomimpl.h"
#include "ddraw_private.h"
#include "d3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static const IDirectDrawSurface7Vtbl FakeZBuffer_IDirectDrawSurface7_VTable;

#ifdef HAVE_OPENGL
static void zbuffer_lock_update(IDirectDrawSurfaceImpl* This, LPCRECT pRect, DWORD dwFlags)
{
    /* Note that this does not do anything for now... At least it's not needed for Grim Fandango :-) */
}

static void zbuffer_unlock_update(IDirectDrawSurfaceImpl* This, LPCRECT pRect)
{
    ((FakeZBuffer_DirectDrawSurfaceImpl *) This->private)->in_memory = TRUE;
}

static BOOLEAN zbuffer_get_dirty_status(IDirectDrawSurfaceImpl* This, LPCRECT pRect)
{
    if (((FakeZBuffer_DirectDrawSurfaceImpl *) This->private)->in_memory) {
	((FakeZBuffer_DirectDrawSurfaceImpl *) This->private)->in_memory = FALSE;
	return TRUE;
    }
    return FALSE;
}
#endif

HRESULT FakeZBuffer_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl *This,
						IDirectDrawImpl *pDD,
						const DDSURFACEDESC2 *pDDSD)
{
    HRESULT hr;
    BYTE zdepth = 16; /* Default value.. Should use the one from GL */
    
    assert(pDDSD->ddsCaps.dwCaps & DDSCAPS_ZBUFFER);

    hr = Main_DirectDrawSurface_Construct(This, pDD, pDDSD);
    if (FAILED(hr)) return hr;

    ICOM_INIT_INTERFACE(This, IDirectDrawSurface7,
			FakeZBuffer_IDirectDrawSurface7_VTable);

    This->final_release = FakeZBuffer_DirectDrawSurface_final_release;
    This->duplicate_surface = FakeZBuffer_DirectDrawSurface_duplicate_surface;

#ifdef HAVE_OPENGL     
    if (opengl_initialized) {
	This->lock_update = zbuffer_lock_update;
	This->unlock_update = zbuffer_unlock_update;
	This->get_dirty_status = zbuffer_get_dirty_status;
    }
#endif
	
    
    /* Beginning of some D3D hacks :-) */
    if (This->surface_desc.dwFlags & DDSD_ZBUFFERBITDEPTH) {
	zdepth = This->surface_desc.u2.dwMipMapCount; /* This is where the Z buffer depth is stored in 'old' versions */
    }
    
    if ((This->surface_desc.dwFlags & DDSD_PIXELFORMAT) == 0) {
	This->surface_desc.dwFlags |= DDSD_PIXELFORMAT;
	This->surface_desc.u4.ddpfPixelFormat.dwSize = sizeof(This->surface_desc.u4.ddpfPixelFormat);
	This->surface_desc.u4.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
	This->surface_desc.u4.ddpfPixelFormat.u1.dwZBufferBitDepth = zdepth;
    }
    if ((This->surface_desc.dwFlags & DDSD_PITCH) == 0) {
	This->surface_desc.dwFlags |= DDSD_PITCH;
	This->surface_desc.u1.lPitch = ((zdepth + 7) / 8) * This->surface_desc.dwWidth;
    }
    This->surface_desc.lpSurface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					     This->surface_desc.u1.lPitch * This->surface_desc.dwHeight);
    
    return DD_OK;
}

/* Not an API */
HRESULT FakeZBuffer_DirectDrawSurface_Create(IDirectDrawImpl* pDD,
					     const DDSURFACEDESC2* pDDSD,
					     LPDIRECTDRAWSURFACE7* ppSurf,
					     IUnknown* pUnkOuter)
{
    IDirectDrawSurfaceImpl* This;
    HRESULT hr;
    assert(pUnkOuter == NULL);

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		     sizeof(*This)
		     + sizeof(FakeZBuffer_DirectDrawSurfaceImpl));
    if (This == NULL) return E_OUTOFMEMORY;

    This->private = (FakeZBuffer_DirectDrawSurfaceImpl*)(This+1);

    hr = FakeZBuffer_DirectDrawSurface_Construct(This, pDD, pDDSD);
    if (FAILED(hr))
	HeapFree(GetProcessHeap(), 0, This);
    else
	*ppSurf = ICOM_INTERFACE(This, IDirectDrawSurface7);

    return hr;
}

void
FakeZBuffer_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This)
{
    Main_DirectDrawSurface_final_release(This);
}

HRESULT
FakeZBuffer_DirectDrawSurface_duplicate_surface(IDirectDrawSurfaceImpl* This,
						LPDIRECTDRAWSURFACE7* ppDup)
{
    return FakeZBuffer_DirectDrawSurface_Create(This->ddraw_owner,
						&This->surface_desc, ppDup,
						NULL);
}

/* put your breakpoint/abort call here */
static HRESULT cant_do_that(const char *s)
{
    FIXME("attempt to %s fake z-buffer\n", s);
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
FakeZBuffer_DirectDrawSurface_Blt(LPDIRECTDRAWSURFACE7 iface, LPRECT rdst,
				  LPDIRECTDRAWSURFACE7 src, LPRECT rsrc,
				  DWORD dwFlags, LPDDBLTFX lpbltfx)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    if (TRACE_ON(ddraw)) {
        TRACE("(%p)->(%p,%p,%p,%08lx,%p)\n", This,rdst,src,rsrc,dwFlags,lpbltfx);
	if (rdst) TRACE("\tdestrect :%ldx%ld-%ldx%ld\n",rdst->left,rdst->top,rdst->right,rdst->bottom);
	if (rsrc) TRACE("\tsrcrect  :%ldx%ld-%ldx%ld\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	TRACE("\tflags: ");
	DDRAW_dump_DDBLT(dwFlags);
	if (dwFlags & DDBLT_DDFX) {
	    TRACE("\tblitfx: ");
	    DDRAW_dump_DDBLTFX(lpbltfx->dwDDFX);
	}
    }

    /* We only support the BLT with DEPTH_FILL for now */
    if ((dwFlags & DDBLT_DEPTHFILL) && (This->ddraw_owner->d3d_private != NULL)) {
        if (This->ddraw_owner->current_device != NULL) {
	    D3DRECT rect;
	    if (rdst) {
	        rect.u1.x1 = rdst->left;
		rect.u2.y1 = rdst->top;
		rect.u3.x2 = rdst->right;
		rect.u4.y2 = rdst->bottom;
	    }
	    This->ddraw_owner->current_device->clear(This->ddraw_owner->current_device,
						     (rdst == NULL ? 0 : 1), &rect,
						     D3DCLEAR_ZBUFFER,
						     0x00000000,
						     ((double) lpbltfx->u5.dwFillDepth) / 4294967295.0,
						     0x00000000);
	    return DD_OK;
	}
    }

    return cant_do_that("blt to a");
}

HRESULT WINAPI
FakeZBuffer_DirectDrawSurface_BltFast(LPDIRECTDRAWSURFACE7 iface, DWORD dstx,
				      DWORD dsty, LPDIRECTDRAWSURFACE7 src,
				      LPRECT rsrc, DWORD trans)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    if (TRACE_ON(ddraw)) {
	FIXME("(%p)->(%ld,%ld,%p,%p,%08lx)\n",
		This,dstx,dsty,src,rsrc,trans
	);
	FIXME("\ttrans:");
	if (FIXME_ON(ddraw))
	  DDRAW_dump_DDBLTFAST(trans);
	if (rsrc)
	  FIXME("\tsrcrect: %ldx%ld-%ldx%ld\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	else
	  FIXME(" srcrect: NULL\n");
    }

    return cant_do_that("bltfast to a");
}

HRESULT WINAPI
FakeZBuffer_DirectDrawSurface_GetDC(LPDIRECTDRAWSURFACE7 iface, HDC *phDC)
{
    return cant_do_that("get a DC for a");
}

HRESULT WINAPI
FakeZBuffer_DirectDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7 iface, HDC hDC)
{
    return cant_do_that("release a DC for a");
}

HRESULT WINAPI
FakeZBuffer_DirectDrawSurface_Restore(LPDIRECTDRAWSURFACE7 iface)
{
    return DD_OK;
}

HRESULT WINAPI
FakeZBuffer_DirectDrawSurface_SetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface,
					     LPDDSURFACEDESC2 pDDSD,
					     DWORD dwFlags)
{
    /* XXX */
    abort();
    return E_FAIL;
}


static const IDirectDrawSurface7Vtbl FakeZBuffer_IDirectDrawSurface7_VTable=
{
    Main_DirectDrawSurface_QueryInterface,
    Main_DirectDrawSurface_AddRef,
    Main_DirectDrawSurface_Release,
    Main_DirectDrawSurface_AddAttachedSurface,
    Main_DirectDrawSurface_AddOverlayDirtyRect,
    FakeZBuffer_DirectDrawSurface_Blt,
    Main_DirectDrawSurface_BltBatch,
    FakeZBuffer_DirectDrawSurface_BltFast,
    Main_DirectDrawSurface_DeleteAttachedSurface,
    Main_DirectDrawSurface_EnumAttachedSurfaces,
    Main_DirectDrawSurface_EnumOverlayZOrders,
    Main_DirectDrawSurface_Flip,
    Main_DirectDrawSurface_GetAttachedSurface,
    Main_DirectDrawSurface_GetBltStatus,
    Main_DirectDrawSurface_GetCaps,
    Main_DirectDrawSurface_GetClipper,
    Main_DirectDrawSurface_GetColorKey,
    FakeZBuffer_DirectDrawSurface_GetDC,
    Main_DirectDrawSurface_GetFlipStatus,
    Main_DirectDrawSurface_GetOverlayPosition,
    Main_DirectDrawSurface_GetPalette,
    Main_DirectDrawSurface_GetPixelFormat,
    Main_DirectDrawSurface_GetSurfaceDesc,
    Main_DirectDrawSurface_Initialize,
    Main_DirectDrawSurface_IsLost,
    Main_DirectDrawSurface_Lock,
    FakeZBuffer_DirectDrawSurface_ReleaseDC,
    FakeZBuffer_DirectDrawSurface_Restore,
    Main_DirectDrawSurface_SetClipper,
    Main_DirectDrawSurface_SetColorKey,
    Main_DirectDrawSurface_SetOverlayPosition,
    Main_DirectDrawSurface_SetPalette,
    Main_DirectDrawSurface_Unlock,
    Main_DirectDrawSurface_UpdateOverlay,
    Main_DirectDrawSurface_UpdateOverlayDisplay,
    Main_DirectDrawSurface_UpdateOverlayZOrder,
    Main_DirectDrawSurface_GetDDInterface,
    Main_DirectDrawSurface_PageLock,
    Main_DirectDrawSurface_PageUnlock,
    FakeZBuffer_DirectDrawSurface_SetSurfaceDesc,
    Main_DirectDrawSurface_SetPrivateData,
    Main_DirectDrawSurface_GetPrivateData,
    Main_DirectDrawSurface_FreePrivateData,
    Main_DirectDrawSurface_GetUniquenessValue,
    Main_DirectDrawSurface_ChangeUniquenessValue,
    Main_DirectDrawSurface_SetPriority,
    Main_DirectDrawSurface_GetPriority,
    Main_DirectDrawSurface_SetLOD,
    Main_DirectDrawSurface_GetLOD
};
