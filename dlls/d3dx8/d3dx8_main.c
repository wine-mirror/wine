/*
 * Direct3D X 8 main file
 *
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
 *
 */

#include "config.h"
#include "wine/port.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"

#include "d3dx8core.h"
#include "d3dx8core_private.h"


WINE_DEFAULT_DEBUG_CHANNEL(d3d);

HRESULT D3DXCreateBuffer(DWORD NumBytes, LPD3DXBUFFER* ppBuffer) {
  ID3DXBufferImpl *object;

  object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ID3DXBufferImpl));
  if (NULL == object) {
    *ppBuffer = (LPD3DXBUFFER)NULL;
    return E_OUTOFMEMORY;
  }
  object->lpVtbl = &D3DXBuffer_Vtbl;
  object->ref = 1;
  object->bufferSize = NumBytes;
  object->buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, NumBytes);
  if (NULL == object->buffer) {
    HeapFree(GetProcessHeap(), 0, object);
    *ppBuffer = (LPD3DXBUFFER)NULL;
    return E_OUTOFMEMORY;
  }
  *ppBuffer = (LPD3DXBUFFER)object;
  return D3D_OK;
}

HRESULT D3DXCreateFont(LPDIRECT3DDEVICE8 pDevice, HFONT hFont, LPD3DXFONT* ppFont) {
  FIXME("(void): stub\n");
  return D3D_OK;
}

UINT D3DXGetFVFVertexSize(DWORD FVF) {
  FIXME("(void): stub\n");
  return 0;
}

HRESULT D3DXAssembleShader(LPCVOID pSrcData, UINT SrcDataLen, DWORD Flags, 
			   LPD3DXBUFFER* ppConstants, 
			   LPD3DXBUFFER* ppCompiledShader,
			   LPD3DXBUFFER* ppCompilationErrors) {
  FIXME("(void): stub\n");
  return D3D_OK;
}

HRESULT D3DXAssembleShaderFromFileA(LPSTR pSrcFile, DWORD Flags,
				    LPD3DXBUFFER* ppConstants,
				    LPD3DXBUFFER* ppCompiledShader,
				    LPD3DXBUFFER* ppCompilationErrors) {
  FIXME("(void): stub\n");
  return D3D_OK;
}

HRESULT D3DXAssembleShaderFromFileW(LPSTR pSrcFile, DWORD Flags,
				    LPD3DXBUFFER* ppConstants,
				    LPD3DXBUFFER* ppCompiledShader,
				    LPD3DXBUFFER* ppCompilationErrors) {
  FIXME("(void): stub\n");
  return D3D_OK;
}
