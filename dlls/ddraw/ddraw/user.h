/* Copyright 2000-2001 TransGaming Technologies Inc. */

#ifndef WINE_DDRAW_DDRAW_USER_H_INCLUDED
#define WINE_DDRAW_DDRAW_USER_H_INCLUDED

#define USER_DDRAW_PRIV(ddraw) ((User_DirectDrawImpl*)((ddraw)->private))
#define USER_DDRAW_PRIV_VAR(name,ddraw) \
	User_DirectDrawImpl* name = USER_DDRAW_PRIV(ddraw)

typedef struct
{
    /* empty */
} User_DirectDrawImpl_Part;

typedef struct
{
    User_DirectDrawImpl_Part user;
} User_DirectDrawImpl;

void User_DirectDraw_final_release(IDirectDrawImpl* This);
HRESULT User_DirectDraw_create_primary(IDirectDrawImpl* This,
				       const DDSURFACEDESC2* pDDSD,
				       LPDIRECTDRAWSURFACE7* ppSurf,
				       LPUNKNOWN pOuter);
HRESULT User_DirectDraw_create_backbuffer(IDirectDrawImpl* This,
					  const DDSURFACEDESC2* pDDSD,
					  LPDIRECTDRAWSURFACE7* ppSurf,
					  LPUNKNOWN pOuter,
					  IDirectDrawSurfaceImpl* primary);
HRESULT User_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex);
HRESULT User_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
			       IUnknown* pUnkOuter, BOOL ex);

HRESULT WINAPI
User_DirectDraw_EnumDisplayModes(LPDIRECTDRAW7 iface, DWORD dwFlags,
				 LPDDSURFACEDESC2 pDDSD, LPVOID context,
				 LPDDENUMMODESCALLBACK2 callback);
HRESULT WINAPI
User_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
				    LPDDDEVICEIDENTIFIER2 pDDDI,
				    DWORD dwFlags);
HRESULT WINAPI
User_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
			       DWORD dwHeight, DWORD dwBPP,
			       DWORD dwRefreshRate, DWORD dwFlags);

#endif
