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

#ifndef WINE_DDRAW_DDRAW_MAIN_H_INCLUDED
#define WINE_DDRAW_DDRAW_MAIN_H_INCLUDED

/* internal virtual functions */
void Main_DirectDraw_final_release(IDirectDrawImpl* This);
HRESULT
Main_create_offscreen(IDirectDrawImpl* This, const DDSURFACEDESC2 *pDDSD,
		      LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pOuter);
HRESULT
Main_create_texture(IDirectDrawImpl* This, const DDSURFACEDESC2 *pDDSD,
		    LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pOuter,
		    DWORD dwMipMapLevel);
HRESULT
Main_create_zbuffer(IDirectDrawImpl* This, const DDSURFACEDESC2 *pDDSD,
		    LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pOuter);

/* internal functions */
HRESULT Main_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex);
void Main_DirectDraw_AddSurface(IDirectDrawImpl* This,
				IDirectDrawSurfaceImpl* surface);
void Main_DirectDraw_RemoveSurface(IDirectDrawImpl* This,
				   IDirectDrawSurfaceImpl* surface);
void Main_DirectDraw_AddClipper(IDirectDrawImpl* This,
				IDirectDrawClipperImpl* clipper);
void Main_DirectDraw_RemoveClipper(IDirectDrawImpl* This,
				   IDirectDrawClipperImpl* clipper);
void Main_DirectDraw_AddPalette(IDirectDrawImpl* This,
				IDirectDrawPaletteImpl* palette);
void Main_DirectDraw_RemovePalette(IDirectDrawImpl* This,
				   IDirectDrawPaletteImpl* palette);


/* interface functions */

ULONG WINAPI Main_DirectDraw_AddRef(LPDIRECTDRAW7 iface);
ULONG WINAPI Main_DirectDraw_Release(LPDIRECTDRAW7 iface);
HRESULT WINAPI Main_DirectDraw_QueryInterface(LPDIRECTDRAW7 iface,
					      REFIID refiid,LPVOID *obj);
HRESULT WINAPI Main_DirectDraw_Compact(LPDIRECTDRAW7 iface);
HRESULT WINAPI Main_DirectDraw_CreateClipper(LPDIRECTDRAW7 iface,
					     DWORD dwFlags,
					     LPDIRECTDRAWCLIPPER *ppClipper,
					     IUnknown *pUnkOuter);
HRESULT WINAPI
Main_DirectDraw_CreatePalette(LPDIRECTDRAW7 iface, DWORD dwFlags,
			      LPPALETTEENTRY palent,
			      LPDIRECTDRAWPALETTE* ppPalette,
			      LPUNKNOWN pUnknown);
HRESULT WINAPI
Main_DirectDraw_CreateSurface(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
			      LPDIRECTDRAWSURFACE7 *ppSurf,
			      IUnknown *pUnkOuter);
HRESULT WINAPI
Main_DirectDraw_DuplicateSurface(LPDIRECTDRAW7 iface, LPDIRECTDRAWSURFACE7 src,
				 LPDIRECTDRAWSURFACE7* dst);
HRESULT WINAPI
Main_DirectDraw_EnumSurfaces(LPDIRECTDRAW7 iface, DWORD dwFlags,
			     LPDDSURFACEDESC2 lpDDSD2, LPVOID context,
			     LPDDENUMSURFACESCALLBACK7 callback);
HRESULT WINAPI
Main_DirectDraw_EvaluateMode(LPDIRECTDRAW7 iface,DWORD a,DWORD* b);
HRESULT WINAPI Main_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface);
HRESULT WINAPI
Main_DirectDraw_GetCaps(LPDIRECTDRAW7 iface, LPDDCAPS pDriverCaps,
			LPDDCAPS pHELCaps);
HRESULT WINAPI
Main_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7 iface, LPDWORD pNumCodes,
			       LPDWORD pCodes);
HRESULT WINAPI
Main_DirectDraw_GetGDISurface(LPDIRECTDRAW7 iface,
			      LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface);
HRESULT WINAPI
Main_DirectDraw_GetMonitorFrequency(LPDIRECTDRAW7 iface,LPDWORD freq);
HRESULT WINAPI
Main_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine);
HRESULT WINAPI
Main_DirectDraw_GetSurfaceFromDC(LPDIRECTDRAW7 iface, HDC hdc,
				 LPDIRECTDRAWSURFACE7 *lpDDS);
HRESULT WINAPI
Main_DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW7 iface, LPBOOL status);
HRESULT WINAPI
Main_DirectDraw_Initialize(LPDIRECTDRAW7 iface, LPGUID lpGuid);
HRESULT WINAPI Main_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW7 iface);
HRESULT WINAPI
Main_DirectDraw_SetCooperativeLevel(LPDIRECTDRAW7 iface, HWND hwnd,
				    DWORD cooplevel);
HRESULT WINAPI
Main_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
			       DWORD dwHeight, LONG lPitch,
			       DWORD dwRefreshRate, DWORD dwFlags,
			       const DDPIXELFORMAT* pixelformat);
HRESULT WINAPI Main_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface);
HRESULT WINAPI
Main_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,
				     HANDLE h);
HRESULT WINAPI
Main_DirectDraw_GetDisplayMode(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD);
HRESULT WINAPI
Main_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface,LPDDSCAPS2 ddscaps,
				   LPDWORD total, LPDWORD free);
HRESULT WINAPI Main_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW7 iface);
HRESULT WINAPI
Main_DirectDraw_StartModeTest(LPDIRECTDRAW7 iface, LPSIZE pModes,
			      DWORD dwNumModes, DWORD dwFlags);

#endif
