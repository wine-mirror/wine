/* Copyright 2000-2001 TransGaming Technologies Inc. */

#ifndef WINE_DDRAW_DPALETTE_HAL_H_INCLUDED
#define WINE_DDRAW_DPALETTE_HAL_H_INCLUDED

HRESULT HAL_DirectDrawPalette_Construct(IDirectDrawPaletteImpl* This,
					 IDirectDrawImpl* pDD, DWORD dwFlags);
void HAL_DirectDrawPalette_final_release(IDirectDrawPaletteImpl* This);

HRESULT
HAL_DirectDrawPalette_Create(IDirectDrawImpl* pDD, DWORD dwFlags,
			      LPDIRECTDRAWPALETTE* ppPalette,
			      LPUNKNOWN pUnkOuter);

HRESULT WINAPI
HAL_DirectDrawPalette_SetEntries(LPDIRECTDRAWPALETTE iface, DWORD dwFlags,
				  DWORD dwStart, DWORD dwCount,
				  LPPALETTEENTRY palent);

#endif
