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
