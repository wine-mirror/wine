/* Copyright 2000-2001 TransGaming Technologies, Inc. */
#ifndef WINE_DDRAW_DDRAW_DGA2_H_INCLUDED
#define WINE_DDRAW_DDRAW_DGA2_H_INCLUDED

#include <X11/extensions/xf86dga.h>

#define XF86DGA2_DDRAW_PRIV(ddraw) \
	((XF86DGA2_DirectDrawImpl*)((ddraw)->private))
#define XF86DGA2_DDRAW_PRIV_VAR(name,ddraw) \
	XF86DGA2_DirectDrawImpl* name = XF86DGA2_DDRAW_PRIV(ddraw)

typedef struct
{
    XDGADevice* current_mode;
    DWORD next_vofs;
} XF86DGA2_DirectDrawImpl_Part;

typedef struct
{
    User_DirectDrawImpl_Part user;
    XF86DGA2_DirectDrawImpl_Part xf86dga2;
} XF86DGA2_DirectDrawImpl;

void XF86DGA2_DirectDraw_final_release(IDirectDrawImpl* This);
HRESULT XF86DGA2_DirectDraw_create_primary(IDirectDrawImpl* This,
					   const DDSURFACEDESC2* pDDSD,
					   LPDIRECTDRAWSURFACE7* ppSurf,
					   LPUNKNOWN pOuter);
HRESULT XF86DGA2_DirectDraw_create_backbuffer(IDirectDrawImpl* This,
					      const DDSURFACEDESC2* pDDSD,
					      LPDIRECTDRAWSURFACE7* ppSurf,
					      LPUNKNOWN pOuter,
					      IDirectDrawSurfaceImpl* primary);
HRESULT XF86DGA2_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex);
HRESULT XF86DGA2_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
				   IUnknown* pUnkOuter, BOOL ex);
HRESULT WINAPI
XF86DGA2_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
					LPDDDEVICEIDENTIFIER2 pDDDI,
					DWORD dwFlags);
HRESULT WINAPI
XF86DGA2_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
				   DWORD dwHeight, DWORD dwBPP,
				   DWORD dwRefreshRate, DWORD dwFlags);
HRESULT WINAPI
XF86DGA2_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface);

#endif
