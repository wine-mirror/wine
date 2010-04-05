/*
 * Copyright 2010 Christian Costa
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

#include "d3dx9.h"

#ifndef __D3DX9SHAPE_H__
#define __D3DX9SHAPE_H__

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI D3DXCreateBox(LPDIRECT3DDEVICE9 device,
                             FLOAT width,
                             FLOAT height,
                             FLOAT depth,
                             LPD3DXMESH* mesh,
                             LPD3DXBUFFER* adjacency);

#ifdef __cplusplus
}
#endif

#endif /* __D3DX9SHAPE_H__ */
