/*
 * Direct3D wine internal public interface file
 *
 * Copyright 2002-2003 The wine-d3d team
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINED3D_INTERFACE_H
#define __WINE_WINED3D_INTERFACE_H

#if !defined( __WINE_CONFIG_H )
# error You must include config.h to use this header
#endif

#if !defined( __WINE_D3D8_H ) && !defined( __WINE_D3D9_H )
# error You must include d3d8.h or d3d9.h header to use this header
#endif

/*****************************************************************
 * THIS FILE MUST NOT CONTAIN X11 or MESA DEFINES 
 * PLEASE USE wine/wined3d_gl.h INSTEAD
 */

/*****************************************************************
 * Some defines
 */

/* Device caps */
#define MAX_PALETTES      256
#define MAX_STREAMS       16
#define MAX_CLIPPLANES    D3DMAXUSERCLIPPLANES
#define MAX_LEVELS        256

/* Other useful values */
#define HIGHEST_RENDER_STATE 174
#define HIGHEST_TEXTURE_STATE 29
#define HIGHEST_TRANSFORMSTATE 512
#define D3DSBT_RECORDED 0xfffffffe

#define D3D_VSHADER_MAX_CONSTANTS 96
#define D3D_PSHADER_MAX_CONSTANTS 32

/*****************************************************************
 * Some includes
 */

#include "wine/wined3d_gl.h"
#include "wine/wined3d_types.h"

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>

/*****************************************************************
 * Some defines
 */

typedef struct IDirect3DImpl IDirect3DImpl;
typedef struct IDirect3DBaseTextureImpl IDirect3DBaseTextureImpl;
typedef struct IDirect3DVolumeTextureImpl IDirect3DVolumeTextureImpl;
typedef struct IDirect3DDeviceImpl IDirect3DDeviceImpl;
typedef struct IDirect3DTextureImpl IDirect3DTextureImpl;
typedef struct IDirect3DCubeTextureImpl IDirect3DCubeTextureImpl;
typedef struct IDirect3DIndexBufferImpl IDirect3DIndexBufferImpl;
typedef struct IDirect3DSurfaceImpl IDirect3DSurfaceImpl;
typedef struct IDirect3DSwapChainImpl IDirect3DSwapChainImpl;
typedef struct IDirect3DResourceImpl IDirect3DResourceImpl;
typedef struct IDirect3DVolumeImpl IDirect3DVolumeImpl;
typedef struct IDirect3DVertexBufferImpl IDirect3DVertexBufferImpl;
typedef struct IDirect3DStateBlockImpl IDirect3DStateBlockImpl;
typedef struct IDirect3DVertexShaderImpl IDirect3DVertexShaderImpl;
typedef struct IDirect3DPixelShaderImpl IDirect3DPixelShaderImpl;
typedef struct IDirect3DVertexShaderDeclarationImpl IDirect3DVertexShaderDeclarationImpl;


/*************************************************************
 * d3dcore interfaces defs
 */

/** Vertex Shader API */
extern HRESULT WINAPI IDirect3DVertexShaderImpl_ParseProgram(IDirect3DVertexShaderImpl* This, CONST DWORD* pFunction);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_GetFunction(IDirect3DVertexShaderImpl* This, VOID* pData, UINT* pSizeOfData);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_SetConstantB(IDirect3DVertexShaderImpl* This, UINT StartRegister, CONST BOOL*  pConstantData, UINT BoolCount);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_SetConstantI(IDirect3DVertexShaderImpl* This, UINT StartRegister, CONST INT*   pConstantData, UINT Vector4iCount);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_SetConstantF(IDirect3DVertexShaderImpl* This, UINT StartRegister, CONST FLOAT* pConstantData, UINT Vector4fCount);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_GetConstantB(IDirect3DVertexShaderImpl* This, UINT StartRegister, BOOL*  pConstantData, UINT BoolCount);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_GetConstantI(IDirect3DVertexShaderImpl* This, UINT StartRegister, INT*   pConstantData, UINT Vector4iCount);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_GetConstantF(IDirect3DVertexShaderImpl* This, UINT StartRegister, FLOAT* pConstantData, UINT Vector4fCount);
/* internal Interfaces */
extern DWORD   WINAPI IDirect3DVertexShaderImpl_GetVersion(IDirect3DVertexShaderImpl* This);
extern HRESULT WINAPI IDirect3DVertexShaderImpl_ExecuteSW(IDirect3DVertexShaderImpl* This, VSHADERINPUTDATA* input, VSHADEROUTPUTDATA* output);

#ifdef __cplusplus
extern "C" {
#endif  /* defined(__cplusplus) */

/* Define the main entrypoint as well */
IDirect3DImpl* WINAPI WineDirect3DCreate(UINT SDKVersion);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif
