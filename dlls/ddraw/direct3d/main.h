/*
 * Copyright 2002 Lionel Ulmer
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

#include "config.h"

/* This is defined here so as to be able to put them in 'drivers' */

HRESULT WINAPI
Main_IDirect3DImpl_7_3T_2T_1T_QueryInterface(LPDIRECT3D7 iface,
                                             REFIID riid,
                                             LPVOID* obp);

ULONG WINAPI
Main_IDirect3DImpl_7_3T_2T_1T_AddRef(LPDIRECT3D7 iface);

ULONG WINAPI
Main_IDirect3DImpl_7_3T_2T_1T_Release(LPDIRECT3D7 iface);

HRESULT WINAPI
Main_IDirect3DImpl_7_EnumDevices(LPDIRECT3D7 iface,
                                 LPD3DENUMDEVICESCALLBACK7 lpEnumDevicesCallback,
                                 LPVOID lpUserArg);

HRESULT WINAPI
Main_IDirect3DImpl_7_CreateDevice(LPDIRECT3D7 iface,
                                  REFCLSID rclsid,
                                  LPDIRECTDRAWSURFACE7 lpDDS,
                                  LPDIRECT3DDEVICE7* lplpD3DDevice);

HRESULT WINAPI
Main_IDirect3DImpl_7_3T_CreateVertexBuffer(LPDIRECT3D7 iface,
					   LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc,
					   LPDIRECT3DVERTEXBUFFER7* lplpD3DVertBuf,
					   DWORD dwFlags);

HRESULT WINAPI
Main_IDirect3DImpl_7_3T_EnumZBufferFormats(LPDIRECT3D7 iface,
                                           REFCLSID riidDevice,
                                           LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback,
                                           LPVOID lpContext);

HRESULT WINAPI
Main_IDirect3DImpl_7_3T_EvictManagedTextures(LPDIRECT3D7 iface);

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_EnumDevices(LPDIRECT3D3 iface,
                                       LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback,
                                       LPVOID lpUserArg);

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_CreateLight(LPDIRECT3D3 iface,
                                       LPDIRECT3DLIGHT* lplpDirect3DLight,
                                       IUnknown* pUnkOuter);

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_CreateMaterial(LPDIRECT3D3 iface,
					  LPDIRECT3DMATERIAL3* lplpDirect3DMaterial3,
					  IUnknown* pUnkOuter);

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_CreateViewport(LPDIRECT3D3 iface,
					  LPDIRECT3DVIEWPORT3* lplpD3DViewport3,
					  IUnknown* pUnkOuter);

HRESULT WINAPI
Main_IDirect3DImpl_3_2T_1T_FindDevice(LPDIRECT3D3 iface,
				      LPD3DFINDDEVICESEARCH lpD3DDFS,
				      LPD3DFINDDEVICERESULT lpD3DFDR);

HRESULT WINAPI
Main_IDirect3DImpl_3_CreateDevice(LPDIRECT3D3 iface,
                                  REFCLSID rclsid,
                                  LPDIRECTDRAWSURFACE4 lpDDS,
                                  LPDIRECT3DDEVICE3* lplpD3DDevice3,
                                  LPUNKNOWN lpUnk);

HRESULT WINAPI
Thunk_IDirect3DImpl_3_CreateVertexBuffer(LPDIRECT3D3 iface,
					 LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc,
					 LPDIRECT3DVERTEXBUFFER* lplpD3DVertBuf,
					 DWORD dwFlags,
					 LPUNKNOWN lpUnk);

HRESULT WINAPI
Thunk_IDirect3DImpl_2_CreateMaterial(LPDIRECT3D2 iface,
				     LPDIRECT3DMATERIAL2* lplpDirect3DMaterial2,
				     IUnknown* pUnkOuter);

HRESULT WINAPI
Thunk_IDirect3DImpl_2_CreateViewport(LPDIRECT3D2 iface,
				     LPDIRECT3DVIEWPORT2* lplpD3DViewport2,
				     IUnknown* pUnkOuter);

HRESULT WINAPI
Main_IDirect3DImpl_2_CreateDevice(LPDIRECT3D2 iface,
                                  REFCLSID rclsid,
                                  LPDIRECTDRAWSURFACE lpDDS,
                                  LPDIRECT3DDEVICE2* lplpD3DDevice2);

HRESULT WINAPI
Main_IDirect3DImpl_1_Initialize(LPDIRECT3D iface,
                                REFIID riid);

HRESULT WINAPI
Thunk_IDirect3DImpl_1_CreateMaterial(LPDIRECT3D iface,
				     LPDIRECT3DMATERIAL* lplpDirect3DMaterial,
				     IUnknown* pUnkOuter);

HRESULT WINAPI
Thunk_IDirect3DImpl_1_CreateViewport(LPDIRECT3D iface,
				     LPDIRECT3DVIEWPORT* lplpD3DViewport,
				     IUnknown* pUnkOuter);

HRESULT WINAPI
Main_IDirect3DImpl_1_FindDevice(LPDIRECT3D iface,
                                LPD3DFINDDEVICESEARCH lpD3DDFS,
                                LPD3DFINDDEVICERESULT lplpD3DDevice);

HRESULT WINAPI
Thunk_IDirect3DImpl_3_QueryInterface(LPDIRECT3D3 iface,
                                     REFIID riid,
                                     LPVOID* obp);

HRESULT WINAPI
Thunk_IDirect3DImpl_2_QueryInterface(LPDIRECT3D2 iface,
                                     REFIID riid,
                                     LPVOID* obp);

HRESULT WINAPI
Thunk_IDirect3DImpl_1_QueryInterface(LPDIRECT3D iface,
                                     REFIID riid,
                                     LPVOID* obp);

ULONG WINAPI
Thunk_IDirect3DImpl_3_AddRef(LPDIRECT3D3 iface);

ULONG WINAPI
Thunk_IDirect3DImpl_2_AddRef(LPDIRECT3D2 iface);

ULONG WINAPI
Thunk_IDirect3DImpl_1_AddRef(LPDIRECT3D iface);

ULONG WINAPI
Thunk_IDirect3DImpl_3_Release(LPDIRECT3D3 iface);

ULONG WINAPI
Thunk_IDirect3DImpl_2_Release(LPDIRECT3D2 iface);

ULONG WINAPI
Thunk_IDirect3DImpl_1_Release(LPDIRECT3D iface);

HRESULT WINAPI
Thunk_IDirect3DImpl_3_EnumZBufferFormats(LPDIRECT3D3 iface,
                                         REFCLSID riidDevice,
                                         LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback,
                                         LPVOID lpContext);

HRESULT WINAPI
Thunk_IDirect3DImpl_3_EvictManagedTextures(LPDIRECT3D3 iface);

HRESULT WINAPI
Thunk_IDirect3DImpl_2_EnumDevices(LPDIRECT3D2 iface,
                                  LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback,
                                  LPVOID lpUserArg);

HRESULT WINAPI
Thunk_IDirect3DImpl_1_EnumDevices(LPDIRECT3D iface,
                                  LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback,
                                  LPVOID lpUserArg);

HRESULT WINAPI
Thunk_IDirect3DImpl_2_CreateLight(LPDIRECT3D2 iface,
                                  LPDIRECT3DLIGHT* lplpDirect3DLight,
                                  IUnknown* pUnkOuter);

HRESULT WINAPI
Thunk_IDirect3DImpl_1_CreateLight(LPDIRECT3D iface,
                                  LPDIRECT3DLIGHT* lplpDirect3DLight,
                                  IUnknown* pUnkOuter);

HRESULT WINAPI
Thunk_IDirect3DImpl_1_FindDevice(LPDIRECT3D iface,
				 LPD3DFINDDEVICESEARCH lpD3DDFS,
				 LPD3DFINDDEVICERESULT lplpD3DDevice);

HRESULT WINAPI
Thunk_IDirect3DImpl_2_FindDevice(LPDIRECT3D2 iface,
				 LPD3DFINDDEVICESEARCH lpD3DDFS,
				 LPD3DFINDDEVICERESULT lpD3DFDR);
