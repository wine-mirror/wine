/* Copyright 2000-2001 TransGaming Technologies, Inc. */
#ifndef WINE_DDRAW_DDRAW_XVIDMODE_H_INCLUDED
#define WINE_DDRAW_DDRAW_XVIDMODE_H_INCLUDED

#include <X11/extensions/xf86vmode.h>

#define XVIDMODE_DDRAW_PRIV(ddraw) \
	((XVidMode_DirectDrawImpl*)((ddraw)->private))
#define XVIDMODE_DDRAW_PRIV_VAR(name,ddraw) \
	XVidMode_DirectDrawImpl* name = XVIDMODE_DDRAW_PRIV(ddraw)

typedef struct
{
    XF86VidModeModeInfo* original_mode;
    XF86VidModeModeInfo* current_mode;
} XVidMode_DirectDrawImpl_Part;

typedef struct
{
    User_DirectDrawImpl_Part user;
    XVidMode_DirectDrawImpl_Part xvidmode;
} XVidMode_DirectDrawImpl;

void XVidMode_DirectDraw_final_release(IDirectDrawImpl* This);
HRESULT XVidMode_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex);
HRESULT XVidMode_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
				   IUnknown* pUnkOuter, BOOL ex);
HRESULT WINAPI
XVidMode_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
					LPDDDEVICEIDENTIFIER2 pDDDI,
					DWORD dwFlags);
HRESULT WINAPI
XVidMode_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
				   DWORD dwHeight, DWORD dwBPP,
				   DWORD dwRefreshRate, DWORD dwFlags);
HRESULT WINAPI
XVidMode_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface);

#endif
