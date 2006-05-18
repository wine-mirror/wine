/*
 * Direct3D X 8 private include file
 *
 * Copyright 2002 Raphael Junqueira
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_D3DX8CORE_PRIVATE_H
#define __WINE_D3DX8CORE_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "d3dx8core.h"

/* Interfaces */
typedef struct ID3DXBufferImpl ID3DXBufferImpl;
typedef struct ID3DXFontImpl   ID3DXFontImpl;

/* ----------- */
/* ID3DXBuffer */
/* ----------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const ID3DXBufferVtbl D3DXBuffer_Vtbl;

/*****************************************************************************
 * ID3DXBufferImpl implementation structure
 */
struct ID3DXBufferImpl
{
  /* IUnknown fields */
  const ID3DXBufferVtbl *lpVtbl;
  LONG           ref;

  /* ID3DXBuffer fields */
  DWORD         *buffer;
  DWORD          bufferSize;
};

/* IUnknown: */
extern HRESULT WINAPI ID3DXBufferImpl_QueryInterface(LPD3DXBUFFER iface, REFIID refiid, LPVOID *obj);
extern ULONG WINAPI   ID3DXBufferImpl_AddRef(LPD3DXBUFFER iface);
extern ULONG WINAPI   ID3DXBufferImpl_Release(LPD3DXBUFFER iface);

/* ID3DXBuffer: */
extern LPVOID WINAPI ID3DXBufferImpl_GetBufferPointer(LPD3DXBUFFER iface);
extern DWORD WINAPI  ID3DXBufferImpl_GetBufferSize(LPD3DXBUFFER iface);

/* --------- */
/* ID3DXFont */
/* --------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const ID3DXFontVtbl D3DXFont_Vtbl;

/*****************************************************************************
 * ID3DXFontImpl implementation structure
 */
struct ID3DXFontImpl
{
  /* IUnknown fields */
  const ID3DXFontVtbl *lpVtbl;
  LONG           ref;

  /* ID3DXFont fields */
};

/* IUnknown: */
extern HRESULT WINAPI ID3DXFontImpl_QueryInterface(LPD3DXFONT iface, REFIID refiid, LPVOID *obj);
extern ULONG WINAPI   ID3DXFontImpl_AddRef(LPD3DXFONT iface);
extern ULONG WINAPI   ID3DXFontImpl_Release(LPD3DXFONT iface);

/* ID3DXFont: */
extern INT WINAPI     ID3DXFontImpl_DrawTextA(LPCSTR pString, INT Count, LPRECT pRect, DWORD Format, D3DCOLOR Color); 

#endif /*__WINE_D3DX8CORE_PRIVATE_H */
