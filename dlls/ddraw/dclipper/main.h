/*
 * Copyright 2000 TransGaming Technologies Inc.
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
