/*
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2009-2011 Henri Verbeet for CodeWeavers
 * Copyright 2013 Stefan Dösinger for CodeWeavers
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

#include "config.h"
#include "wine/port.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* This call just uploads data, the caller is responsible for binding the
 * correct texture. */
/* Context activation is done by the caller. */
void wined3d_volume_upload_data(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        const struct wined3d_context *context, const struct wined3d_const_bo_address *data)
{
    const struct wined3d_format *format = texture->resource.format;
    unsigned int level = sub_resource_idx % texture->level_count;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int width, height, depth;
    const void *mem = data->addr;
    void *converted_mem = NULL;

    TRACE("texture %p, sub_resource_idx %u, context %p, format %s (%#x).\n",
            texture, level, context, debug_d3dformat(format->id), format->id);

    width = wined3d_texture_get_level_width(texture, level);
    height = wined3d_texture_get_level_height(texture, level);
    depth = wined3d_texture_get_level_depth(texture, level);

    if (format->convert)
    {
        UINT dst_row_pitch, dst_slice_pitch;
        UINT src_row_pitch, src_slice_pitch;

        if (data->buffer_object)
            ERR("Loading a converted texture from a PBO.\n");
        if (texture->resource.format_flags & WINED3DFMT_FLAG_BLOCKS)
            ERR("Converting a block-based format.\n");

        dst_row_pitch = width * format->conv_byte_count;
        dst_slice_pitch = dst_row_pitch * height;

        wined3d_texture_get_pitch(texture, level, &src_row_pitch, &src_slice_pitch);

        converted_mem = wined3d_calloc(depth, dst_slice_pitch);
        format->convert(data->addr, converted_mem, src_row_pitch, src_slice_pitch,
                dst_row_pitch, dst_slice_pitch, width, height, depth);
        mem = converted_mem;
    }

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data->buffer_object));
        checkGLcall("glBindBuffer");
    }

    GL_EXTCALL(glTexSubImage3D(GL_TEXTURE_3D, level, 0, 0, 0,
            width, height, depth, format->glFormat, format->glType, mem));
    checkGLcall("glTexSubImage3D");

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

    HeapFree(GetProcessHeap(), 0, converted_mem);
}
