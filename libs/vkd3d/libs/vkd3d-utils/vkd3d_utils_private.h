/*
 * Copyright 2016 Józef Kucia for CodeWeavers
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

#ifndef __VKD3D_UTILS_PRIVATE_H
#define __VKD3D_UTILS_PRIVATE_H

#ifndef __MINGW32__
#define WIDL_C_INLINE_WRAPPERS
#endif
#define COBJMACROS
#define NONAMELESSUNION
#define VK_NO_PROTOTYPES
#define CONST_VTABLE

#include <vkd3d.h>
#include <vkd3d_shader.h>
#include <d3dcompiler.h>

#include "vkd3d_blob.h"
#include "vkd3d_memory.h"
#include <vkd3d_utils.h>

#include <inttypes.h>

#ifndef D3DERR_INVALIDCALL
#define D3DERR_INVALIDCALL _HRESULT_TYPEDEF_(0x8876086c)
#endif

#endif  /* __VKD3D_UTILS_PRIVATE_H */
