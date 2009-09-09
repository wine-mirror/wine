/*
 * Copyright 2008-2009 Henri Verbeet for CodeWeavers
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

#include "config.h"
#include "wine/port.h"

#include "d3d10_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10);

#define WINE_D3D10_TO_STR(x) case x: return #x

const char *debug_d3d10_driver_type(D3D10_DRIVER_TYPE driver_type)
{
    switch(driver_type)
    {
        WINE_D3D10_TO_STR(D3D10_DRIVER_TYPE_HARDWARE);
        WINE_D3D10_TO_STR(D3D10_DRIVER_TYPE_REFERENCE);
        WINE_D3D10_TO_STR(D3D10_DRIVER_TYPE_NULL);
        WINE_D3D10_TO_STR(D3D10_DRIVER_TYPE_SOFTWARE);
        default:
            FIXME("Unrecognized D3D10_DRIVER_TYPE %#x\n", driver_type);
            return "unrecognized";
    }
}

const char *debug_d3d10_shader_variable_class(D3D10_SHADER_VARIABLE_CLASS c)
{
    switch (c)
    {
        WINE_D3D10_TO_STR(D3D10_SVC_SCALAR);
        WINE_D3D10_TO_STR(D3D10_SVC_VECTOR);
        WINE_D3D10_TO_STR(D3D10_SVC_MATRIX_ROWS);
        WINE_D3D10_TO_STR(D3D10_SVC_MATRIX_COLUMNS);
        WINE_D3D10_TO_STR(D3D10_SVC_OBJECT);
        WINE_D3D10_TO_STR(D3D10_SVC_STRUCT);
        default:
            FIXME("Unrecognized D3D10_SHADER_VARIABLE_CLASS %#x.\n", c);
            return "unrecognized";
    }
}

const char *debug_d3d10_shader_variable_type(D3D10_SHADER_VARIABLE_TYPE t)
{
    switch (t)
    {
        WINE_D3D10_TO_STR(D3D10_SVT_VOID);
        WINE_D3D10_TO_STR(D3D10_SVT_BOOL);
        WINE_D3D10_TO_STR(D3D10_SVT_INT);
        WINE_D3D10_TO_STR(D3D10_SVT_FLOAT);
        WINE_D3D10_TO_STR(D3D10_SVT_STRING);
        WINE_D3D10_TO_STR(D3D10_SVT_TEXTURE);
        WINE_D3D10_TO_STR(D3D10_SVT_TEXTURE1D);
        WINE_D3D10_TO_STR(D3D10_SVT_TEXTURE2D);
        WINE_D3D10_TO_STR(D3D10_SVT_TEXTURE3D);
        WINE_D3D10_TO_STR(D3D10_SVT_TEXTURECUBE);
        WINE_D3D10_TO_STR(D3D10_SVT_SAMPLER);
        WINE_D3D10_TO_STR(D3D10_SVT_PIXELSHADER);
        WINE_D3D10_TO_STR(D3D10_SVT_VERTEXSHADER);
        WINE_D3D10_TO_STR(D3D10_SVT_UINT);
        WINE_D3D10_TO_STR(D3D10_SVT_UINT8);
        WINE_D3D10_TO_STR(D3D10_SVT_GEOMETRYSHADER);
        WINE_D3D10_TO_STR(D3D10_SVT_RASTERIZER);
        WINE_D3D10_TO_STR(D3D10_SVT_DEPTHSTENCIL);
        WINE_D3D10_TO_STR(D3D10_SVT_BLEND);
        WINE_D3D10_TO_STR(D3D10_SVT_BUFFER);
        WINE_D3D10_TO_STR(D3D10_SVT_CBUFFER);
        WINE_D3D10_TO_STR(D3D10_SVT_TBUFFER);
        WINE_D3D10_TO_STR(D3D10_SVT_TEXTURE1DARRAY);
        WINE_D3D10_TO_STR(D3D10_SVT_TEXTURE2DARRAY);
        WINE_D3D10_TO_STR(D3D10_SVT_RENDERTARGETVIEW);
        WINE_D3D10_TO_STR(D3D10_SVT_DEPTHSTENCILVIEW);
        WINE_D3D10_TO_STR(D3D10_SVT_TEXTURE2DMS);
        WINE_D3D10_TO_STR(D3D10_SVT_TEXTURE2DMSARRAY);
        WINE_D3D10_TO_STR(D3D10_SVT_TEXTURECUBEARRAY);
        default:
            FIXME("Unrecognized D3D10_SHADER_VARIABLE_TYPE %#x.\n", t);
            return "unrecognized";
    }
}

#undef WINE_D3D10_TO_STR

void *d3d10_rb_alloc(size_t size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void *d3d10_rb_realloc(void *ptr, size_t size)
{
    return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
}

void d3d10_rb_free(void *ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
