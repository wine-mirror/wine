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

#ifndef DDRAW_DSURFACE_HAL_H_INCLUDED
#define DDRAW_DSURFACE_HAL_H_INCLUDED

#define HAL_PRIV(surf) ((HAL_DirectDrawSurfaceImpl*)((surf)->private))

#define HAL_PRIV_VAR(name,surf) \
	HAL_DirectDrawSurfaceImpl* name = HAL_PRIV(surf)

struct HAL_DirectDrawSurfaceImpl_Part
{
    DWORD need_late;
    LPVOID fb_addr;
    DWORD fb_pitch, fb_vofs;
};

typedef struct
{
    struct DIB_DirectDrawSurfaceImpl_Part dib;
    struct User_DirectDrawSurfaceImpl_Part user;
    struct HAL_DirectDrawSurfaceImpl_Part hal;
} HAL_DirectDrawSurfaceImpl;

HRESULT
HAL_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl* This,
				IDirectDrawImpl* pDD,
				const DDSURFACEDESC2* pDDSD);

HRESULT
HAL_DirectDrawSurface_Create(IDirectDrawImpl *pDD,
			     const DDSURFACEDESC2 *pDDSD,
			     LPDIRECTDRAWSURFACE7 *ppSurf,
			     IUnknown *pUnkOuter);

void HAL_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This);
HRESULT HAL_DirectDrawSurface_late_allocate(IDirectDrawSurfaceImpl* This);

void HAL_DirectDrawSurface_set_palette(IDirectDrawSurfaceImpl* This,
				       IDirectDrawPaletteImpl* pal);
void HAL_DirectDrawSurface_update_palette(IDirectDrawSurfaceImpl* This,
					  IDirectDrawPaletteImpl* pal,
					  DWORD dwStart, DWORD dwCount,
					  LPPALETTEENTRY palent);
HRESULT HAL_DirectDrawSurface_duplicate_surface(IDirectDrawSurfaceImpl* This,
						LPDIRECTDRAWSURFACE7* ppDup);
void HAL_DirectDrawSurface_lock_update(IDirectDrawSurfaceImpl* This,
				       LPCRECT pRect, DWORD dwFlags);
void HAL_DirectDrawSurface_unlock_update(IDirectDrawSurfaceImpl* This,
					 LPCRECT pRect);
BOOL HAL_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
				     IDirectDrawSurfaceImpl* back,
				     DWORD dwFlags);
void HAL_DirectDrawSurface_flip_update(IDirectDrawSurfaceImpl* This,
				       DWORD dwFlags);
HWND HAL_DirectDrawSurface_get_display_window(IDirectDrawSurfaceImpl* This);

#endif
