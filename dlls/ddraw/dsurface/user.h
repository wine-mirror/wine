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

#ifndef DDRAW_DSURFACE_USER_H_INCLUDED
#define DDRAW_DSURFACE_USER_H_INCLUDED

#define USER_PRIV(surf) ((User_DirectDrawSurfaceImpl*)((surf)->private))

#define USER_PRIV_VAR(name,surf) \
	User_DirectDrawSurfaceImpl* name = USER_PRIV(surf)

struct User_DirectDrawSurfaceImpl_Part
{
    HWND window;
    HDC cached_dc;
    HANDLE update_thread, update_event;
};

typedef struct
{
    struct DIB_DirectDrawSurfaceImpl_Part dib;
    struct User_DirectDrawSurfaceImpl_Part user;
} User_DirectDrawSurfaceImpl;

HRESULT
User_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl* This,
				 IDirectDrawImpl* pDD,
				 const DDSURFACEDESC2* pDDSD);

HRESULT
User_DirectDrawSurface_Create(IDirectDrawImpl *pDD,
			      const DDSURFACEDESC2 *pDDSD,
			      LPDIRECTDRAWSURFACE7 *ppSurf,
			      IUnknown *pUnkOuter);

void User_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This);

void User_DirectDrawSurface_lock_update(IDirectDrawSurfaceImpl* This,
					LPCRECT pRect, DWORD dwFlags);
void User_DirectDrawSurface_unlock_update(IDirectDrawSurfaceImpl* This,
					  LPCRECT pRect);
void User_DirectDrawSurface_set_palette(IDirectDrawSurfaceImpl* This,
					IDirectDrawPaletteImpl* pal);
void User_DirectDrawSurface_update_palette(IDirectDrawSurfaceImpl* This,
					   IDirectDrawPaletteImpl* pal,
					   DWORD dwStart, DWORD dwCount,
					   LPPALETTEENTRY palent);
HRESULT User_DirectDrawSurface_duplicate_surface(IDirectDrawSurfaceImpl* This,
						 LPDIRECTDRAWSURFACE7* ppDup);
BOOL User_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
				      IDirectDrawSurfaceImpl* back,
				      DWORD dwFlags);
void User_DirectDrawSurface_flip_update(IDirectDrawSurfaceImpl* This,
					DWORD dwFlags);
HWND User_DirectDrawSurface_get_display_window(IDirectDrawSurfaceImpl* This);

HRESULT User_DirectDrawSurface_get_dc(IDirectDrawSurfaceImpl* This, HDC* phDC);
HRESULT User_DirectDrawSurface_release_dc(IDirectDrawSurfaceImpl* This,
					  HDC hDC);

HRESULT User_DirectDrawSurface_get_gamma_ramp(IDirectDrawSurfaceImpl* This,
					      DWORD dwFlags,
					      LPDDGAMMARAMP lpGammaRamp);
HRESULT User_DirectDrawSurface_set_gamma_ramp(IDirectDrawSurfaceImpl* This,
					      DWORD dwFlags,
					      LPDDGAMMARAMP lpGammaRamp);

#endif
