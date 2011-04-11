/*
 * Copyright (C) 2002 Raphael Junqueira
 * Copyright (C) 2008 David Adam
 * Copyright (C) 2008 Tony Wasserka
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

#ifndef __WINE_D3DX9_36_PRIVATE_H
#define __WINE_D3DX9_36_PRIVATE_H

#include <stdarg.h>

#define COBJMACROS
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "d3dx9.h"

/* for internal use */
typedef enum _FormatType {
    FORMAT_ARGB,   /* unsigned */
    FORMAT_UNKNOWN
} FormatType;

typedef struct _PixelFormatDesc {
    D3DFORMAT format;
    BYTE bits[4];
    BYTE shift[4];
    UINT bytes_per_pixel;
    FormatType type;
} PixelFormatDesc;

HRESULT map_view_of_file(LPCWSTR filename, LPVOID *buffer, DWORD *length);
HRESULT load_resource_into_memory(HMODULE module, HRSRC resinfo, LPVOID *buffer, DWORD *length);

const PixelFormatDesc *get_format_info(D3DFORMAT format);
const PixelFormatDesc *get_format_info_idx(int idx);

/* debug helpers */
const char *debug_d3dxparameter_class(D3DXPARAMETER_CLASS c);
const char *debug_d3dxparameter_type(D3DXPARAMETER_TYPE t);

#endif /* __WINE_D3DX9_36_PRIVATE_H */
