/*
 * Copyright (C) 2002 Raphael Junqueira
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

#ifndef __WINE_D3DX8CORE_H
#define __WINE_D3DX8CORE_H

#include "objbase.h"

#include "d3d8.h"
#include "d3d8types.h"
#include "d3d8caps.h"

/*****************************************************************************
 * #defines and error codes
 */
#define D3DXASM_DEBUG           1
#define D3DXASM_SKIPVALIDATION  2

#define _FACD3D  0x876
#define MAKE_D3DXHRESULT( code )  MAKE_HRESULT( 1, _FACD3D, code )

/*
 * Direct3D Errors
 */
#define D3DXERR_CANNOTATTRSORT                  MAKE_D3DXHRESULT(2158)
#define D3DXERR_CANNOTMODIFYINDEXBUFFER         MAKE_D3DXHRESULT(2159)
#define D3DXERR_INVALIDMESH                     MAKE_D3DXHRESULT(2160)
#define D3DXERR_SKINNINGNOTSUPPORTED            MAKE_D3DXHRESULT(2161)
#define D3DXERR_TOOMANYINFLUENCES               MAKE_D3DXHRESULT(2162)
#define D3DXERR_INVALIDDATA                     MAKE_D3DXHRESULT(2163)

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_ID3DXBuffer,             0x1,0x1,0x4,0xB0,0xCF,0x98,0xFE,0xFD,0xFF,0x95,0x12);/* FIXME */
typedef struct ID3DXBuffer              ID3DXBuffer, *LPD3DXBUFFER;
DEFINE_GUID(IID_ID3DXFont,               0x1,0x1,0x4,0xB0,0xCF,0x98,0xFE,0xFD,0xFF,0x95,0x13);/* FIXME */
typedef struct ID3DXFont                ID3DXFont, *LPD3DXFONT;

/*****************************************************************************
 * ID3DXBuffer interface
 */
#undef INTERFACE
#define INTERFACE ID3DXBuffer
#define ID3DXBuffer_METHODS \
    /*** ID3DXBuffer methods ***/ \
    STDMETHOD_(LPVOID,GetBufferPointer)(THIS) PURE; \
    STDMETHOD_(DWORD,GetBufferSize)(THIS) PURE;

    /*** ID3DXBuffer methods ***/
#define ID3DXBuffer_IMETHODS \
    IUnknown_IMETHODS \
    ID3DXBuffer_METHODS
ICOM_DEFINE(ID3DXBuffer,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define ID3DXBuffer_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define ID3DXBuffer_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define ID3DXBuffer_Release(p)            (p)->lpVtbl->Release(p)
/*** ID3DXBuffer methods ***/
#define ID3DXBuffer_GetBufferPointer(p)   (p)->lpVtbl->GetBufferPointer(p)
#define ID3DXBuffer_GetBufferSize(p)      (p)->lpVtbl->GetBufferSize(p)
#endif

/*****************************************************************************
 * ID3DXFont interface
 */
#undef INTERFACE
#define INTERFACE ID3DXFont
#define ID3DXFont_METHODS \
    /*** ID3DXFont methods ***/ \
    STDMETHOD(Begin)(THIS) PURE; \
    STDMETHOD(DrawTextA)(THIS) PURE; \
    STDMETHOD(End)(THIS) PURE;

    /*** ID3DXFont methods ***/
#define ID3DXFont_IMETHODS \
    IUnknown_IMETHODS \
    ID3DXFont_METHODS
ICOM_DEFINE(ID3DXFont,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define ID3DXFont_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define ID3DXFont_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define ID3DXFont_Release(p)            (p)->lpVtbl->Release(p)
/*** ID3DXFont methods ***/
#define ID3DXFont_Begin(p)              (p)->lpVtbl->Begin(p)
#define ID3DXFont_DrawTextA(p,a,b,c,d,e)(p)->lpVtbl->DrawText(p,a,b,c,d,e)
#define ID3DXFont_End(p)                (p)->lpVtbl->End(p)
#endif

/*************************************************************************************
 * Define entrypoints 
 */
HRESULT WINAPI D3DXCreateBuffer(DWORD NumBytes, LPD3DXBUFFER* ppBuffer);
HRESULT WINAPI D3DXCreateFont(LPDIRECT3DDEVICE8 pDevice, HFONT hFont, LPD3DXFONT* ppFont);
UINT WINAPI D3DXGetFVFVertexSize(DWORD FVF);
HRESULT WINAPI D3DXAssembleShader(LPCVOID pSrcData, UINT SrcDataLen, DWORD Flags, 
			   LPD3DXBUFFER* ppConstants, 
			   LPD3DXBUFFER* ppCompiledShader,
			   LPD3DXBUFFER* ppCompilationErrors);
HRESULT WINAPI D3DXAssembleShaderFromFileA(LPSTR pSrcFile, DWORD Flags,
				    LPD3DXBUFFER* ppConstants,
				    LPD3DXBUFFER* ppCompiledShader,
				    LPD3DXBUFFER* ppCompilationErrors);
HRESULT WINAPI D3DXAssembleShaderFromFileW(LPSTR pSrcFile, DWORD Flags,
				    LPD3DXBUFFER* ppConstants,
				    LPD3DXBUFFER* ppCompiledShader,
				    LPD3DXBUFFER* ppCompilationErrors);

#endif /* __WINE_D3DX8CORE_H */
