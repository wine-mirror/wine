/* Copyright 2000-2001 TransGaming Technologies Inc. */

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
				       LPCRECT pRect);
void HAL_DirectDrawSurface_unlock_update(IDirectDrawSurfaceImpl* This,
					 LPCRECT pRect);
BOOL HAL_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
				     IDirectDrawSurfaceImpl* back,
				     DWORD dwFlags);
void HAL_DirectDrawSurface_flip_update(IDirectDrawSurfaceImpl* This,
				       DWORD dwFlags);
HWND HAL_DirectDrawSurface_get_display_window(IDirectDrawSurfaceImpl* This);

#endif
