/* Copyright 2000-2001 TransGaming Technologies Inc. */

#ifndef WINE_DDRAW_DPALETTE_MAIN_H_INCLUDED
#define WINE_DDRAW_DPALETTE_MAIN_H_INCLUDED

HRESULT Main_DirectDrawPalette_Construct(IDirectDrawPaletteImpl* This,
					 IDirectDrawImpl* pDD, DWORD dwFlags);
void Main_DirectDrawPalette_final_release(IDirectDrawPaletteImpl* This);


HRESULT
Main_DirectDrawPalette_Create(IDirectDrawImpl* pDD, DWORD dwFlags,
			      LPDIRECTDRAWPALETTE* ppPalette,
			      LPUNKNOWN pUnkOuter);
void Main_DirectDrawPalette_ForceDestroy(IDirectDrawPaletteImpl* This);


DWORD Main_DirectDrawPalette_Size(DWORD dwFlags);



HRESULT WINAPI
Main_DirectDrawPalette_GetEntries(LPDIRECTDRAWPALETTE iface, DWORD dwFlags,
				  DWORD dwStart, DWORD dwCount,
				  LPPALETTEENTRY palent);
HRESULT WINAPI
Main_DirectDrawPalette_SetEntries(LPDIRECTDRAWPALETTE iface, DWORD dwFlags,
				  DWORD dwStart, DWORD dwCount,
				  LPPALETTEENTRY palent);
ULONG WINAPI
Main_DirectDrawPalette_Release(LPDIRECTDRAWPALETTE iface);
ULONG WINAPI Main_DirectDrawPalette_AddRef(LPDIRECTDRAWPALETTE iface);
HRESULT WINAPI
Main_DirectDrawPalette_Initialize(LPDIRECTDRAWPALETTE iface,
				  LPDIRECTDRAW ddraw, DWORD dwFlags,
				  LPPALETTEENTRY palent);
HRESULT WINAPI
Main_DirectDrawPalette_GetCaps(LPDIRECTDRAWPALETTE iface, LPDWORD lpdwCaps);
HRESULT WINAPI
Main_DirectDrawPalette_QueryInterface(LPDIRECTDRAWPALETTE iface,
				      REFIID refiid, LPVOID *obj);


#endif
