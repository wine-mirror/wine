/*
 * Copyright 2000-2001 TransGaming Technologies Inc.
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

#ifndef DDRAW_DSURFACE_MAIN_H_INCLUDED
#define DDRAW_DSURFACE_MAIN_H_INCLUDED

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ddraw_private.h"

/* Support for IDirectDrawSurface7::Set/Get/FreePrivateData. I don't think
 * anybody uses it for much so a good implementation is optional. */
typedef struct PrivateData
{
    struct PrivateData* next;
    struct PrivateData* prev;

    GUID tag;
    DWORD flags; /* DDSPD_* */
    DWORD uniqueness_value;

    union
    {
	LPVOID data;
	LPUNKNOWN object;
    } ptr;

    DWORD size;
} PrivateData;

extern ICOM_VTABLE(IDirectDrawGammaControl) DDRAW_IDDGC_VTable;

/* Non-interface functions */
HRESULT
Main_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl* This,
				 IDirectDrawImpl* pDD,
				 const DDSURFACEDESC2* pDDSD);
void Main_DirectDrawSurface_ForceDestroy(IDirectDrawSurfaceImpl* This);

void
Main_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This);
HRESULT
Main_DirectDrawSurface_late_allocate(IDirectDrawSurfaceImpl* This);
BOOL
Main_DirectDrawSurface_attach(IDirectDrawSurfaceImpl *This,
			      IDirectDrawSurfaceImpl *to);
BOOL Main_DirectDrawSurface_detach(IDirectDrawSurfaceImpl *This);
void
Main_DirectDrawSurface_lock_update(IDirectDrawSurfaceImpl* This,
				   LPCRECT pRect, DWORD dwFlags);
void
Main_DirectDrawSurface_unlock_update(IDirectDrawSurfaceImpl* This,
				     LPCRECT pRect);
void
Main_DirectDrawSurface_lose_surface(IDirectDrawSurfaceImpl* This);
void
Main_DirectDrawSurface_set_palette(IDirectDrawSurfaceImpl* This,
				   IDirectDrawPaletteImpl* pal);
void
Main_DirectDrawSurface_update_palette(IDirectDrawSurfaceImpl* This,
				      IDirectDrawPaletteImpl* pal,
				      DWORD dwStart, DWORD dwCount,
				      LPPALETTEENTRY palent);
HWND
Main_DirectDrawSurface_get_display_window(IDirectDrawSurfaceImpl* This);

HRESULT
Main_DirectDrawSurface_get_gamma_ramp(IDirectDrawSurfaceImpl* This,
				      DWORD dwFlags,
				      LPDDGAMMARAMP lpGammaRamp);
HRESULT
Main_DirectDrawSurface_set_gamma_ramp(IDirectDrawSurfaceImpl* This,
				      DWORD dwFlags,
				      LPDDGAMMARAMP lpGammaRamp);

BOOL Main_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
				      IDirectDrawSurfaceImpl* back,
				      DWORD dwFlags);

#define CHECK_LOST(This)					\
	do {							\
		if (This->lost) return DDERR_SURFACELOST;	\
	} while (0)

#define CHECK_TEXTURE(This)					\
	do {							\
		if (!(This->surface_desc.ddsCaps.dwCaps2	\
		      & DDSCAPS2_TEXTUREMANAGE))		\
			return DDERR_INVALIDOBJECT;		\
	} while (0)

#define LOCK_OBJECT(This) do { } while (0)
#define UNLOCK_OBJECT(This) do { } while (0)

/* IDirectDrawSurface7 (partial) implementation */
ULONG WINAPI
Main_DirectDrawSurface_AddRef(LPDIRECTDRAWSURFACE7 iface);
ULONG WINAPI
Main_DirectDrawSurface_Release(LPDIRECTDRAWSURFACE7 iface);
HRESULT WINAPI
Main_DirectDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE7 iface, REFIID riid,
				      LPVOID* ppObj);
HRESULT WINAPI
Main_DirectDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDIRECTDRAWSURFACE7 pAttach);
HRESULT WINAPI
Main_DirectDrawSurface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE7 iface,
					   LPRECT pRect);
HRESULT WINAPI
Main_DirectDrawSurface_BltBatch(LPDIRECTDRAWSURFACE7 iface,
				LPDDBLTBATCH pBatch, DWORD dwCount,
				DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_ChangeUniquenessValue(LPDIRECTDRAWSURFACE7 iface);
HRESULT WINAPI
Main_DirectDrawSurface_DeleteAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					     DWORD dwFlags,
					     LPDIRECTDRAWSURFACE7 pAttach);
HRESULT WINAPI
Main_DirectDrawSurface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE7 iface,
					    LPVOID context,
					    LPDDENUMSURFACESCALLBACK7 cb);
HRESULT WINAPI
Main_DirectDrawSurface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE7 iface,
					  DWORD dwFlags, LPVOID context,
					  LPDDENUMSURFACESCALLBACK7 cb);
HRESULT WINAPI
Main_DirectDrawSurface_Flip(LPDIRECTDRAWSURFACE7 iface,
			    LPDIRECTDRAWSURFACE7 override, DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_FreePrivateData(LPDIRECTDRAWSURFACE7 iface,
				       REFGUID tag);
HRESULT WINAPI
Main_DirectDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDDSCAPS2 pCaps,
					  LPDIRECTDRAWSURFACE7* ppSurface);
HRESULT WINAPI
Main_DirectDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE7 iface,
				    DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_GetCaps(LPDIRECTDRAWSURFACE7 iface,
			       LPDDSCAPS2 pCaps);
HRESULT WINAPI
Main_DirectDrawSurface_GetClipper(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER* ppClipper);
HRESULT WINAPI
Main_DirectDrawSurface_GetColorKey(LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwFlags, LPDDCOLORKEY pCKey);
HRESULT WINAPI
Main_DirectDrawSurface_GetDC(LPDIRECTDRAWSURFACE7 iface, HDC *phDC);
HRESULT WINAPI
Main_DirectDrawSurface_GetDDInterface(LPDIRECTDRAWSURFACE7 iface,
				      LPVOID* pDD);
HRESULT WINAPI
Main_DirectDrawSurface_GetFlipStatus(LPDIRECTDRAWSURFACE7 iface,
				     DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_GetLOD(LPDIRECTDRAWSURFACE7 iface,
			      LPDWORD pdwMaxLOD);
HRESULT WINAPI
Main_DirectDrawSurface_GetOverlayPosition(LPDIRECTDRAWSURFACE7 iface,
					  LPLONG pX, LPLONG pY);
HRESULT WINAPI
Main_DirectDrawSurface_GetPalette(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWPALETTE* ppPalette);
HRESULT WINAPI
Main_DirectDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE7 iface,
				      LPDDPIXELFORMAT pDDPixelFormat);
HRESULT WINAPI
Main_DirectDrawSurface_GetPriority(LPDIRECTDRAWSURFACE7 iface,
				   LPDWORD pdwPriority);
HRESULT WINAPI
Main_DirectDrawSurface_GetPrivateData(LPDIRECTDRAWSURFACE7 iface, REFGUID tag,
				      LPVOID pBuffer, LPDWORD pcbBufferSize);
HRESULT WINAPI
Main_DirectDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface,
				      LPDDSURFACEDESC2 pDDSD);
HRESULT WINAPI
Main_DirectDrawSurface_GetUniquenessValue(LPDIRECTDRAWSURFACE7 iface,
					  LPDWORD pValue);
HRESULT WINAPI
Main_DirectDrawSurface_Initialize(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAW pDD, LPDDSURFACEDESC2 pDDSD);
HRESULT WINAPI
Main_DirectDrawSurface_IsLost(LPDIRECTDRAWSURFACE7 iface);
HRESULT WINAPI
Main_DirectDrawSurface_Lock(LPDIRECTDRAWSURFACE7 iface, LPRECT prect,
			    LPDDSURFACEDESC2 pDDSD, DWORD flags, HANDLE h);
HRESULT WINAPI
Main_DirectDrawSurface_PageLock(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_PageUnlock(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7 iface, HDC hDC);
HRESULT WINAPI
Main_DirectDrawSurface_SetClipper(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER pDDClipper);
HRESULT WINAPI
Main_DirectDrawSurface_SetColorKey(LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwFlags, LPDDCOLORKEY pCKey);
HRESULT WINAPI
Main_DirectDrawSurface_SetLOD(LPDIRECTDRAWSURFACE7 iface, DWORD dwMaxLOD);
HRESULT WINAPI
Main_DirectDrawSurface_SetOverlayPosition(LPDIRECTDRAWSURFACE7 iface,
					  LONG X, LONG Y);
HRESULT WINAPI
Main_DirectDrawSurface_SetPalette(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWPALETTE pPalette);
HRESULT WINAPI
Main_DirectDrawSurface_SetPriority(LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwPriority);
HRESULT WINAPI
Main_DirectDrawSurface_SetPrivateData(LPDIRECTDRAWSURFACE7 iface,
				      REFGUID tag, LPVOID pData,
				      DWORD cbSize, DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_Unlock(LPDIRECTDRAWSURFACE7 iface, LPRECT pRect);
HRESULT WINAPI
Main_DirectDrawSurface_UpdateOverlay(LPDIRECTDRAWSURFACE7 iface,
				     LPRECT pSrcRect,
				     LPDIRECTDRAWSURFACE7 pDstSurface,
				     LPRECT pDstRect, DWORD dwFlags,
				     LPDDOVERLAYFX pFX);
HRESULT WINAPI
Main_DirectDrawSurface_UpdateOverlayDisplay(LPDIRECTDRAWSURFACE7 iface,
					    DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawSurface_UpdateOverlayZOrder(LPDIRECTDRAWSURFACE7 iface,
					   DWORD dwFlags,
					   LPDIRECTDRAWSURFACE7 pDDSRef);

#endif
