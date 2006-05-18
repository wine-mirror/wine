/*	DirectDrawSurface HAL driver
 *
 * Copyright 2001 TransGaming Technologies Inc.
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

#include <assert.h>
#include <stdlib.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "wine/debug.h"
#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static const IDirectDrawSurface7Vtbl HAL_IDirectDrawSurface7_VTable;

static HRESULT HAL_DirectDrawSurface_create_surface(IDirectDrawSurfaceImpl* This,
						    IDirectDrawImpl* pDD)
{
    HAL_PRIV_VAR(priv, This);
    HAL_DDRAW_PRIV_VAR(ddpriv, pDD);
    LPDDRAWI_DIRECTDRAW_GBL dd_gbl = pDD->local.lpGbl;
    LPDDRAWI_DDRAWSURFACE_LCL local = &This->local;
    DDHAL_CREATESURFACEDATA data;
    HRESULT hr;

    data.lpDD = dd_gbl;
    data.lpDDSurfaceDesc = (LPDDSURFACEDESC)&This->surface_desc;
    data.lplpSList = &local;
    data.dwSCnt = 1;
    data.ddRVal = 0;
    data.CreateSurface = dd_gbl->lpDDCBtmp->HALDD.CreateSurface;
    hr = data.CreateSurface(&data);

    if (hr == DDHAL_DRIVER_HANDLED) {
	if (This->global.fpVidMem < 4) {
	    /* grab framebuffer data from current_mode */
	    priv->hal.fb_pitch = dd_gbl->vmiData.lDisplayPitch;
	    priv->hal.fb_vofs  = ddpriv->hal.next_vofs;
	    priv->hal.fb_addr  = ((LPBYTE)dd_gbl->vmiData.fpPrimary) +
				 dd_gbl->vmiData.lDisplayPitch * priv->hal.fb_vofs;
	    TRACE("vofs=%ld, addr=%p\n", priv->hal.fb_vofs, priv->hal.fb_addr);
	    ddpriv->hal.next_vofs += This->surface_desc.dwHeight;

	    This->global.fpVidMem = (FLATPTR)priv->hal.fb_addr;
	    This->global.u4.lPitch = priv->hal.fb_pitch;
	}
	This->surface_desc.lpSurface = (LPVOID)This->global.fpVidMem;
	This->surface_desc.dwFlags |= DDSD_LPSURFACE;
	if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER) {
	    This->surface_desc.u1.dwLinearSize = This->global.u4.dwLinearSize;
	    This->surface_desc.dwFlags |= DDSD_LINEARSIZE;
	} else {
	    This->surface_desc.u1.lPitch = This->global.u4.lPitch;
	    This->surface_desc.dwFlags |= DDSD_PITCH;
	}
    }
    else priv->hal.need_late = TRUE;

    return data.ddRVal;
}

static inline BOOL HAL_IsUser(IDirectDrawSurfaceImpl* This)
{
    HAL_PRIV_VAR(priv, This);
    if (This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_TEXTURE | DDSCAPS_EXECUTEBUFFER))
	return FALSE;
    if (priv->hal.fb_addr)
	return FALSE;
    return TRUE;
}

HRESULT
HAL_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl* This,
				IDirectDrawImpl* pDD,
				const DDSURFACEDESC2* pDDSD)
{
    HAL_PRIV_VAR(priv, This);
    LPDDRAWI_DIRECTDRAW_GBL dd_gbl = pDD->local.lpGbl;
    HRESULT hr;

    TRACE("(%p,%p,%p)\n",This,pDD,pDDSD);

    /* copy surface_desc, we may want to modify it before DIB construction */
    This->surface_desc = *pDDSD;

    /* the driver may want to dereference these pointers */
    This->local.lpSurfMore = &This->more;
    This->local.lpGbl = &This->global;
    This->gmore = &This->global_more;

    if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_TEXTURE) {
	hr = HAL_DirectDrawSurface_create_surface(This, pDD);
	if (FAILED(hr)) return hr;

	hr = DIB_DirectDrawSurface_Construct(This, pDD, &This->surface_desc);
	if (FAILED(hr)) return hr;
    }
    else if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER) {
	FIXME("create execute buffer\n");
	return DDERR_GENERIC;
    }
    else {
	if (!(dd_gbl->dwFlags & DDRAWI_MODECHANGED)) {
	    /* force a mode set (HALs like DGA may need it) */
	    hr = HAL_DirectDraw_SetDisplayMode(ICOM_INTERFACE(pDD, IDirectDraw7),
					       pDD->width, pDD->height,
					       pDD->pixelformat.u1.dwRGBBitCount,
					       0, 0);
	    if (FAILED(hr)) return hr;
	}

	if (dd_gbl->vmiData.fpPrimary) {
	    hr = HAL_DirectDrawSurface_create_surface(This, pDD);
	    if (FAILED(hr)) return hr;

	    if (priv->hal.need_late) {
		/* this doesn't make sense... driver error? */
		ERR("driver failed to create framebuffer surface\n");
		return DDERR_GENERIC;
	    }

	    hr = DIB_DirectDrawSurface_Construct(This, pDD, &This->surface_desc);
	    if (FAILED(hr)) return hr;
	} else {
	    /* no framebuffer, construct User-based primary */
	    hr = User_DirectDrawSurface_Construct(This, pDD, pDDSD);
	    if (FAILED(hr)) return hr;

	    /* must notify HAL *after* creating User-based primary */
	    /* (or use CreateSurfaceEx, which we don't yet) */
	    hr = HAL_DirectDrawSurface_create_surface(This, pDD);
	    if (FAILED(hr)) return hr;

	    priv->hal.need_late = FALSE;
	}
    }

    ICOM_INIT_INTERFACE(This, IDirectDrawSurface7,
			HAL_IDirectDrawSurface7_VTable);

    This->final_release = HAL_DirectDrawSurface_final_release;
    This->late_allocate = HAL_DirectDrawSurface_late_allocate;
    This->duplicate_surface = HAL_DirectDrawSurface_duplicate_surface;

    This->flip_data   = HAL_DirectDrawSurface_flip_data;
    This->flip_update = HAL_DirectDrawSurface_flip_update;

    This->set_palette    = HAL_DirectDrawSurface_set_palette;

    This->get_display_window = HAL_DirectDrawSurface_get_display_window;

    return DD_OK;
}

HRESULT
HAL_DirectDrawSurface_Create(IDirectDrawImpl *pDD,
			     const DDSURFACEDESC2 *pDDSD,
			     LPDIRECTDRAWSURFACE7 *ppSurf,
			     IUnknown *pUnkOuter)
{
    IDirectDrawSurfaceImpl* This;
    HRESULT hr;
    assert(pUnkOuter == NULL);

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		     sizeof(*This) + sizeof(HAL_DirectDrawSurfaceImpl));
    if (This == NULL) return E_OUTOFMEMORY;

    This->private = (HAL_DirectDrawSurfaceImpl*)(This+1);

    hr = HAL_DirectDrawSurface_Construct(This, pDD, pDDSD);
    if (FAILED(hr))
	HeapFree(GetProcessHeap(), 0, This);
    else
	*ppSurf = ICOM_INTERFACE(This, IDirectDrawSurface7);

    return hr;
}

void HAL_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This)
{
    LPDDRAWI_DIRECTDRAW_GBL dd_gbl = This->more.lpDD_lcl->lpGbl;
    DDHAL_DESTROYSURFACEDATA data;

    /* destroy HAL surface */
    data.lpDD = dd_gbl;
    data.lpDDSurface = &This->local;
    data.ddRVal = 0;
    data.DestroySurface = dd_gbl->lpDDCBtmp->HALDDSurface.DestroySurface;
    data.DestroySurface(&data);

    if (HAL_IsUser(This)) {
	User_DirectDrawSurface_final_release(This);
    } else {
	DIB_DirectDrawSurface_final_release(This);
    }
}

HRESULT HAL_DirectDrawSurface_late_allocate(IDirectDrawSurfaceImpl* This)
{
    HAL_PRIV_VAR(priv, This);
    if (priv->hal.need_late) {
	priv->hal.need_late = FALSE;
	return HAL_DirectDrawSurface_create_surface(This, This->ddraw_owner);
    }
    return DD_OK;
}

void HAL_DirectDrawSurface_set_palette(IDirectDrawSurfaceImpl* This,
				       IDirectDrawPaletteImpl* pal)
{
    LPDDRAWI_DIRECTDRAW_GBL dd_gbl = This->more.lpDD_lcl->lpGbl;
    DDHAL_SETPALETTEDATA data;

    DIB_DirectDrawSurface_set_palette(This, pal);
    data.lpDD = dd_gbl;
    data.lpDDSurface = &This->local;
    data.lpDDPalette = (pal != NULL ? &pal->global : NULL);
    data.ddRVal = 0;
    data.Attach = TRUE; /* what's this? */
    data.SetPalette = dd_gbl->lpDDCBtmp->HALDDSurface.SetPalette;
    if (data.SetPalette)
	data.SetPalette(&data);
}

HRESULT HAL_DirectDrawSurface_duplicate_surface(IDirectDrawSurfaceImpl* This,
						LPDIRECTDRAWSURFACE7* ppDup)
{
    return HAL_DirectDrawSurface_Create(This->ddraw_owner,
					     &This->surface_desc, ppDup, NULL);
}

void HAL_DirectDrawSurface_lock_update(IDirectDrawSurfaceImpl* This,
				       LPCRECT pRect, DWORD dwFlags)
{
    LPDDRAWI_DIRECTDRAW_GBL dd_gbl = This->more.lpDD_lcl->lpGbl;
    DDHAL_LOCKDATA	data;

    data.lpDD		= dd_gbl;
    data.lpDDSurface	= &This->local;
    data.ddRVal		= 0;
    data.lpSurfData	= This->surface_desc.lpSurface; /* FIXME: correct? */
    if (pRect) {
	data.rArea.top	= pRect->top;
	data.rArea.bottom	= pRect->bottom;
	data.rArea.left	= pRect->left;
	data.rArea.right	= pRect->right;
	data.bHasRect 	= TRUE;
    } else {
	data.bHasRect 	= FALSE;
    }
    data.dwFlags	= dwFlags;

    data.Lock		= dd_gbl->lpDDCBtmp->HALDDSurface.Lock;
    if (data.Lock && (data.Lock(&data) == DDHAL_DRIVER_HANDLED))
	return;

    if (HAL_IsUser(This)) {
	User_DirectDrawSurface_lock_update(This, pRect, dwFlags);
    } else {
	Main_DirectDrawSurface_lock_update(This, pRect, dwFlags);
    }
}

void HAL_DirectDrawSurface_unlock_update(IDirectDrawSurfaceImpl* This,
					 LPCRECT pRect)
{
    LPDDRAWI_DIRECTDRAW_GBL dd_gbl = This->more.lpDD_lcl->lpGbl;
    DDHAL_UNLOCKDATA	data;

    data.lpDD		= dd_gbl;
    data.lpDDSurface	= &This->local;
    data.ddRVal		= 0;
    data.Unlock		= dd_gbl->lpDDCBtmp->HALDDSurface.Unlock;
    if (data.Unlock && (data.Unlock(&data) == DDHAL_DRIVER_HANDLED))
	return;

    if (HAL_IsUser(This)) {
	User_DirectDrawSurface_unlock_update(This, pRect);
    } else {
	Main_DirectDrawSurface_unlock_update(This, pRect);
    }
}

BOOL HAL_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
				     IDirectDrawSurfaceImpl* back,
				     DWORD dwFlags)
{
    HAL_PRIV_VAR(front_priv, front);
    HAL_PRIV_VAR(back_priv, back);
    LPDDRAWI_DIRECTDRAW_GBL dd_gbl = front->more.lpDD_lcl->lpGbl;
    DDHAL_FLIPDATA data;
    BOOL ret;

    {
	DWORD tmp;
	tmp = front_priv->hal.fb_vofs;
	front_priv->hal.fb_vofs = back_priv->hal.fb_vofs;
	back_priv->hal.fb_vofs = tmp;
    }
    {
	LPVOID tmp;
	tmp = front_priv->hal.fb_addr;
	front_priv->hal.fb_addr = back_priv->hal.fb_addr;
	back_priv->hal.fb_addr = tmp;
    }

    if (HAL_IsUser(front)) {
	ret = User_DirectDrawSurface_flip_data(front, back, dwFlags);
    } else {
	ret = DIB_DirectDrawSurface_flip_data(front, back, dwFlags);
    }

    TRACE("(%p,%p)\n",front,back);
    data.lpDD = dd_gbl;
    data.lpSurfCurr = &front->local;
    data.lpSurfTarg = &back->local;
    data.lpSurfCurrLeft = NULL;
    data.lpSurfTargLeft = NULL;
    data.dwFlags = dwFlags;
    data.ddRVal = 0;
    data.Flip = dd_gbl->lpDDCBtmp->HALDDSurface.Flip;
    if (data.Flip)
	if (data.Flip(&data) == DDHAL_DRIVER_HANDLED) ret = FALSE;

    return ret;
}

void HAL_DirectDrawSurface_flip_update(IDirectDrawSurfaceImpl* This, DWORD dwFlags)
{
    if (HAL_IsUser(This)) {
	User_DirectDrawSurface_flip_update(This, dwFlags);
    }
}

HWND HAL_DirectDrawSurface_get_display_window(IDirectDrawSurfaceImpl* This)
{
    return 0;
}

static const IDirectDrawSurface7Vtbl HAL_IDirectDrawSurface7_VTable =
{
    Main_DirectDrawSurface_QueryInterface,
    Main_DirectDrawSurface_AddRef,
    Main_DirectDrawSurface_Release,
    Main_DirectDrawSurface_AddAttachedSurface,
    Main_DirectDrawSurface_AddOverlayDirtyRect,
    DIB_DirectDrawSurface_Blt,
    Main_DirectDrawSurface_BltBatch,
    DIB_DirectDrawSurface_BltFast,
    Main_DirectDrawSurface_DeleteAttachedSurface,
    Main_DirectDrawSurface_EnumAttachedSurfaces,
    Main_DirectDrawSurface_EnumOverlayZOrders,
    Main_DirectDrawSurface_Flip,
    Main_DirectDrawSurface_GetAttachedSurface,
    Main_DirectDrawSurface_GetBltStatus,
    Main_DirectDrawSurface_GetCaps,
    Main_DirectDrawSurface_GetClipper,
    Main_DirectDrawSurface_GetColorKey,
    Main_DirectDrawSurface_GetDC,
    Main_DirectDrawSurface_GetFlipStatus,
    Main_DirectDrawSurface_GetOverlayPosition,
    Main_DirectDrawSurface_GetPalette,
    Main_DirectDrawSurface_GetPixelFormat,
    Main_DirectDrawSurface_GetSurfaceDesc,
    Main_DirectDrawSurface_Initialize,
    Main_DirectDrawSurface_IsLost,
    Main_DirectDrawSurface_Lock,
    Main_DirectDrawSurface_ReleaseDC,
    DIB_DirectDrawSurface_Restore,
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
    DIB_DirectDrawSurface_SetSurfaceDesc,
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
