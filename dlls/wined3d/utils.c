/*
 * Utility functions for the WineD3D Library
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
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

#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

const char* debug_d3dformat(D3DFORMAT fmt) {
  switch (fmt) {
#define FMT_TO_STR(fmt) case fmt: return #fmt
    FMT_TO_STR(D3DFMT_UNKNOWN);
    FMT_TO_STR(D3DFMT_R8G8B8);
    FMT_TO_STR(D3DFMT_A8R8G8B8);
    FMT_TO_STR(D3DFMT_X8R8G8B8);
    FMT_TO_STR(D3DFMT_R5G6B5);
    FMT_TO_STR(D3DFMT_X1R5G5B5);
    FMT_TO_STR(D3DFMT_A1R5G5B5);
    FMT_TO_STR(D3DFMT_A4R4G4B4);
    FMT_TO_STR(D3DFMT_R3G3B2);
    FMT_TO_STR(D3DFMT_A8);
    FMT_TO_STR(D3DFMT_A8R3G3B2);
    FMT_TO_STR(D3DFMT_X4R4G4B4);
    FMT_TO_STR(D3DFMT_A8P8);
    FMT_TO_STR(D3DFMT_P8);
    FMT_TO_STR(D3DFMT_L8);
    FMT_TO_STR(D3DFMT_A8L8);
    FMT_TO_STR(D3DFMT_A4L4);
    FMT_TO_STR(D3DFMT_V8U8);
    FMT_TO_STR(D3DFMT_L6V5U5);
    FMT_TO_STR(D3DFMT_X8L8V8U8);
    FMT_TO_STR(D3DFMT_Q8W8V8U8);
    FMT_TO_STR(D3DFMT_V16U16);
    FMT_TO_STR(D3DFMT_W11V11U10);
    FMT_TO_STR(D3DFMT_UYVY);
    FMT_TO_STR(D3DFMT_YUY2);
    FMT_TO_STR(D3DFMT_DXT1);
    FMT_TO_STR(D3DFMT_DXT2);
    FMT_TO_STR(D3DFMT_DXT3);
    FMT_TO_STR(D3DFMT_DXT4);
    FMT_TO_STR(D3DFMT_DXT5);
    FMT_TO_STR(D3DFMT_D16_LOCKABLE);
    FMT_TO_STR(D3DFMT_D32);
    FMT_TO_STR(D3DFMT_D15S1);
    FMT_TO_STR(D3DFMT_D24S8);
    FMT_TO_STR(D3DFMT_D16);
    FMT_TO_STR(D3DFMT_D24X8);
    FMT_TO_STR(D3DFMT_D24X4S4);
    FMT_TO_STR(D3DFMT_VERTEXDATA);
    FMT_TO_STR(D3DFMT_INDEX16);
    FMT_TO_STR(D3DFMT_INDEX32);
#undef FMT_TO_STR
  default:
    FIXME("Unrecognized %u D3DFORMAT!\n", fmt);
    return "unrecognized";
  }
}
