/*
 * Copyright 2001 TransGaming Technologies, Inc.
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

#ifndef WINE_DDRAW_DDRAW_HAL_H_INCLUDED
#define WINE_DDRAW_DDRAW_HAL_H_INCLUDED

#define HAL_DDRAW_PRIV(ddraw) \
	((HAL_DirectDrawImpl*)((ddraw)->private))
#define HAL_DDRAW_PRIV_VAR(name,ddraw) \
	HAL_DirectDrawImpl* name = HAL_DDRAW_PRIV(ddraw)

typedef struct
{
    DWORD next_vofs;
} HAL_DirectDrawImpl_Part;

typedef struct
{
    User_DirectDrawImpl_Part user;
    HAL_DirectDrawImpl_Part hal;
} HAL_DirectDrawImpl;

void HAL_DirectDraw_final_release(IDirectDrawImpl* This);
HRESULT HAL_DirectDraw_create_primary(IDirectDrawImpl* This,
				      const DDSURFACEDESC2* pDDSD,
				      LPDIRECTDRAWSURFACE7* ppSurf,
				      LPUNKNOWN pOuter);
HRESULT HAL_DirectDraw_create_backbuffer(IDirectDrawImpl* This,
					 const DDSURFACEDESC2* pDDSD,
					 LPDIRECTDRAWSURFACE7* ppSurf,
					 LPUNKNOWN pOuter,
					 IDirectDrawSurfaceImpl* primary);
HRESULT HAL_DirectDraw_create_texture(IDirectDrawImpl* This,
				      const DDSURFACEDESC2* pDDSD,
				      LPDIRECTDRAWSURFACE7* ppSurf,
				      LPUNKNOWN pOuter,
				      DWORD dwMipMapLevel);

HRESULT HAL_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex);
HRESULT HAL_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
				   IUnknown* pUnkOuter, BOOL ex);


HRESULT WINAPI
HAL_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
					LPDDDEVICEIDENTIFIER2 pDDDI,
					DWORD dwFlags);
HRESULT WINAPI
HAL_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
				   DWORD dwHeight, DWORD dwBPP,
				   DWORD dwRefreshRate, DWORD dwFlags);
HRESULT WINAPI
HAL_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface);

#endif
