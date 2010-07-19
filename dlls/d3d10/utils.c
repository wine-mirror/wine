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

void skip_dword_unknown(const char **ptr, unsigned int count)
{
    unsigned int i;
    DWORD d;

    FIXME("Skipping %u unknown DWORDs:\n", count);
    for (i = 0; i < count; ++i)
    {
        read_dword(ptr, &d);
        FIXME("\t0x%08x\n", d);
    }
}

void write_dword_unknown(char **ptr, DWORD d)
{
    FIXME("Writing unknown DWORD 0x%08x\n", d);
    write_dword(ptr, d);
}

HRESULT parse_dxbc(const char *data, SIZE_T data_size,
        HRESULT (*chunk_handler)(const char *data, DWORD data_size, DWORD tag, void *ctx), void *ctx)
{
    const char *ptr = data;
    HRESULT hr = S_OK;
    DWORD chunk_count;
    DWORD total_size;
    unsigned int i;
    DWORD tag;

    if (!data)
    {
        WARN("No data supplied.\n");
        return E_FAIL;
    }

    read_dword(&ptr, &tag);
    TRACE("tag: %s.\n", debugstr_an((const char *)&tag, 4));

    if (tag != TAG_DXBC)
    {
        WARN("Wrong tag.\n");
        return E_FAIL;
    }

    /* checksum? */
    skip_dword_unknown(&ptr, 4);

    skip_dword_unknown(&ptr, 1);

    read_dword(&ptr, &total_size);
    TRACE("total size: %#x\n", total_size);

    if (data_size != total_size)
    {
        WARN("Wrong size supplied.\n");
        return E_FAIL;
    }

    read_dword(&ptr, &chunk_count);
    TRACE("chunk count: %#x\n", chunk_count);

    for (i = 0; i < chunk_count; ++i)
    {
        DWORD chunk_tag, chunk_size;
        const char *chunk_ptr;
        DWORD chunk_offset;

        read_dword(&ptr, &chunk_offset);
        TRACE("chunk %u at offset %#x\n", i, chunk_offset);

        chunk_ptr = data + chunk_offset;

        read_dword(&chunk_ptr, &chunk_tag);
        read_dword(&chunk_ptr, &chunk_size);

        hr = chunk_handler(chunk_ptr, chunk_size, chunk_tag, ctx);
        if (FAILED(hr)) break;
    }

    return hr;
}
