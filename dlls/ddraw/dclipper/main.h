/* Copyright 2000 TransGaming Technologies Inc. */

#ifndef WINE_DDRAW_DCLIPPER_MAIN_H_INCLUDED
#define WINE_DDRAW_DCLIPPER_MAIN_H_INCLUDED

HRESULT WINAPI DirectDrawCreateClipper(DWORD dwFlags,
				       LPDIRECTDRAWCLIPPER* ppClipper,
				       LPUNKNOWN pUnkOuter);
HRESULT DDRAW_CreateClipper(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppObj);
void Main_DirectDrawClipper_ForceDestroy(IDirectDrawClipperImpl* This);

HRESULT WINAPI
Main_DirectDrawClipper_SetHwnd(LPDIRECTDRAWCLIPPER iface, DWORD dwFlags,
			       HWND hWnd);
ULONG WINAPI Main_DirectDrawClipper_Release(LPDIRECTDRAWCLIPPER iface);
HRESULT WINAPI
Main_DirectDrawClipper_GetClipList(LPDIRECTDRAWCLIPPER iface,LPRECT prcClip,
				   LPRGNDATA lprgn,LPDWORD pdwSize);
HRESULT WINAPI
Main_DirectDrawClipper_SetClipList(LPDIRECTDRAWCLIPPER iface,LPRGNDATA lprgn,
				   DWORD pdwSize);
HRESULT WINAPI
Main_DirectDrawClipper_QueryInterface(LPDIRECTDRAWCLIPPER iface, REFIID riid,
				      LPVOID* ppvObj);
ULONG WINAPI Main_DirectDrawClipper_AddRef( LPDIRECTDRAWCLIPPER iface );
HRESULT WINAPI
Main_DirectDrawClipper_GetHWnd(LPDIRECTDRAWCLIPPER iface, HWND* hWndPtr);
HRESULT WINAPI
Main_DirectDrawClipper_Initialize(LPDIRECTDRAWCLIPPER iface, LPDIRECTDRAW lpDD,
				  DWORD dwFlags);
HRESULT WINAPI
Main_DirectDrawClipper_IsClipListChanged(LPDIRECTDRAWCLIPPER iface,
					 BOOL* lpbChanged);

#endif
