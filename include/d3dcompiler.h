/*
 * Copyright 2010 Matteo Bruni for CodeWeavers
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

#ifndef __D3DCOMPILER_H__
#define __D3DCOMPILER_H__

#include "d3d11shader.h"

#define D3DCOMPILE_DEBUG                          0x0001
#define D3DCOMPILE_SKIP_VALIDATION                0x0002
#define D3DCOMPILE_SKIP_OPTIMIZATION              0x0004
#define D3DCOMPILE_PACK_MATRIX_ROW_MAJOR          0x0008
#define D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR       0x0010
#define D3DCOMPILE_PARTIAL_PRECISION              0x0020
#define D3DCOMPILE_FORCE_VS_SOFTWARE_NO_OPT       0x0040
#define D3DCOMPILE_FORCE_PS_SOFTWARE_NO_OPT       0x0080
#define D3DCOMPILE_NO_PRESHADER                   0x0100
#define D3DCOMPILE_AVOID_FLOW_CONTROL             0x0200
#define D3DCOMPILE_PREFER_FLOW_CONTROL            0x0400
#define D3DCOMPILE_ENABLE_STRICTNESS              0x0800
#define D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY 0x1000
#define D3DCOMPILE_IEEE_STRICTNESS                0x2000
#define D3DCOMPILE_OPTIMIZATION_LEVEL0            0x4000
#define D3DCOMPILE_OPTIMIZATION_LEVEL1            0x0000
#define D3DCOMPILE_OPTIMIZATION_LEVEL2            0xC000
#define D3DCOMPILE_OPTIMIZATION_LEVEL3            0x8000
#define D3DCOMPILE_WARNINGS_ARE_ERRORS           0x40000

HRESULT WINAPI D3DCompile(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT sflags, UINT eflags, ID3DBlob **shader, ID3DBlob **error_messages);

HRESULT WINAPI D3DCreateBlob(SIZE_T data_size, ID3DBlob **blob);

#endif
