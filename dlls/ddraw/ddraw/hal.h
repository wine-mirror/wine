/* Copyright 2001 TransGaming Technologies, Inc. */
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
