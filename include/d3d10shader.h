/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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
 *
 */

#ifndef __WINE_D3D10SHADER_H
#define __WINE_D3D10SHADER_H

#include "d3d10.h"

typedef enum _D3D10_SHADER_VARIABLE_CLASS
{
    D3D10_SVC_SCALAR,
    D3D10_SVC_VECTOR,
    D3D10_SVC_MATRIX_ROWS,
    D3D10_SVC_MATRIX_COLUMNS,
    D3D10_SVC_OBJECT,
    D3D10_SVC_STRUCT,
    D3D10_SVC_FORCE_DWORD = 0x7fffffff
} D3D10_SHADER_VARIABLE_CLASS, *LPD3D10_SHADER_VARIABLE_CLASS;

typedef enum _D3D10_SHADER_VARIABLE_TYPE
{
    D3D10_SVT_VOID = 0,
    D3D10_SVT_BOOL = 1,
    D3D10_SVT_INT = 2,
    D3D10_SVT_FLOAT = 3,
    D3D10_SVT_STRING = 4,
    D3D10_SVT_TEXTURE = 5,
    D3D10_SVT_TEXTURE1D = 6,
    D3D10_SVT_TEXTURE2D = 7,
    D3D10_SVT_TEXTURE3D = 8,
    D3D10_SVT_TEXTURECUBE = 9,
    D3D10_SVT_SAMPLER = 10,
    D3D10_SVT_PIXELSHADER = 15,
    D3D10_SVT_VERTEXSHADER = 16,
    D3D10_SVT_UINT = 19,
    D3D10_SVT_UINT8 = 20,
    D3D10_SVT_GEOMETRYSHADER = 21,
    D3D10_SVT_RASTERIZER = 22,
    D3D10_SVT_DEPTHSTENCIL = 23,
    D3D10_SVT_BLEND = 24,
    D3D10_SVT_BUFFER = 25,
    D3D10_SVT_CBUFFER = 26,
    D3D10_SVT_TBUFFER = 27,
    D3D10_SVT_TEXTURE1DARRAY = 28,
    D3D10_SVT_TEXTURE2DARRAY = 29,
    D3D10_SVT_RENDERTARGETVIEW = 30,
    D3D10_SVT_DEPTHSTENCILVIEW = 31,
    D3D10_SVT_TEXTURE2DMS = 32,
    D3D10_SVT_TEXTURE2DMSARRAY = 33,
    D3D10_SVT_TEXTURECUBEARRAY = 34,
    D3D10_SVT_FORCE_DWORD = 0x7fffffff
} D3D10_SHADER_VARIABLE_TYPE, *LPD3D10_SHADER_VARIABLE_TYPE;

#endif /* __WINE_D3D10SHADER_H */
