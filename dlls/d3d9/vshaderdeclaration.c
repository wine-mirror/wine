/*
 * vertex declaration implementation
 *
 * Copyright 2002-2003 Raphael Junqueira
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

#include "config.h"
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

/**
 * DirectX9 SDK download
 *  http://msdn.microsoft.com/library/default.asp?url=/downloads/list/directx.asp
 *
 * Exploring D3DX
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dndrive/html/directx07162002.asp
 *
 * Using Vertex Shaders
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dndrive/html/directx02192001.asp
 *
 * Dx9 New
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/whatsnew.asp
 *
 * Dx9 Shaders
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/VertexShader2_0/VertexShader2_0.asp
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/VertexShader2_0/Instructions/Instructions.asp
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/GettingStarted/VertexDeclaration/VertexDeclaration.asp
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/Shaders/VertexShader3_0/VertexShader3_0.asp
 *
 * Dx9 D3DX
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/advancedtopics/VertexPipe/matrixstack/matrixstack.asp
 *
 * FVF
 *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/GettingStarted/VertexFormats/vformats.asp
 *
 * NVIDIA: DX8 Vertex Shader to NV Vertex Program
 *  http://developer.nvidia.com/view.asp?IO=vstovp
 *
 * NVIDIA: Memory Management with VAR
 *  http://developer.nvidia.com/view.asp?IO=var_memory_management 
 */

HRESULT  WINAPI  IDirect3DDevice9Impl_CreateVertexDeclaration(LPDIRECT3DDEVICE9 iface, CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_SetVertexDeclaration(LPDIRECT3DDEVICE9 iface, IDirect3DVertexDeclaration9* pDecl) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_GetVertexDeclaration(LPDIRECT3DDEVICE9 iface, IDirect3DVertexDeclaration9** ppDecl) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}
