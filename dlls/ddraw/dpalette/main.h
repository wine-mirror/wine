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
