/* Copyright 2000-2001 TransGaming Technologies Inc. */

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
