/*
 * Copyright (C) 2008 Francois Gouget
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

#ifndef __WINE_D3DX8MESH_H
#define __WINE_D3DX8MESH_H

#include <d3dx8.h>
#include <dxfile.h>

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI D3DXCreateBuffer(DWORD,LPD3DXBUFFER*);
UINT    WINAPI D3DXGetFVFVertexSize(DWORD);
BOOL    WINAPI D3DXBoxBoundProbe(CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *);
BOOL    WINAPI D3DXSphereBoundProbe(CONST D3DXVECTOR3 *,FLOAT,CONST D3DXVECTOR3 *,CONST D3DXVECTOR3 *);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_D3DX8MESH_H */
