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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <assert.h>

#include "ddraw.h"
#include "d3d.h"

#include "wine/debug.h"

#include "ddcomimpl.h"
#include "ddraw_private.h"
#include "dsurface/main.h"
#include "dsurface/fakezbuffer.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static ICOM_VTABLE(IDirectDrawSurface7) FakeZBuffer_IDirectDrawSurface7_VTable;

HRESULT FakeZBuffer_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl *This,
						IDirectDrawImpl *pDD,
						const DDSURFACEDESC2 *pDDSD)
{
    HRESULT hr;

    assert(pDDSD->ddsCaps.dwCaps & DDSCAPS_ZBUFFER);

    hr = Main_DirectDrawSurface_Construct(This, pDD, pDDSD);
    if (FAILED(hr)) return hr;

    ICOM_INIT_INTERFACE(This, IDirectDrawSurface7,
			FakeZBuffer_IDirectDrawSurface7_VTable);

    This->final_release = FakeZBuffer_DirectDrawSurface_final_release;
    This->duplicate_surface = FakeZBuffer_DirectDrawSurface_duplicate_surface;

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
    return cant_do_that("blt to a");
}

HRESULT WINAPI
FakeZBuffer_DirectDrawSurface_BltFast(LPDIRECTDRAWSURFACE7 iface, DWORD dstx,
				      DWORD dsty, LPDIRECTDRAWSURFACE7 src,
				      LPRECT rsrc, DWORD trans)
{
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


static ICOM_VTABLE(IDirectDrawSurface7) FakeZBuffer_IDirectDrawSurface7_VTable=
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
