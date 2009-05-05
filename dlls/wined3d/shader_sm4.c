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
 */

#include "config.h"
#include "wine/port.h"

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

static void shader_sm4_read_header(const DWORD **ptr, DWORD *shader_version)
{
    TRACE("version: 0x%08x\n", **ptr);
    *shader_version = *(*ptr)++;
    TRACE("token count: %u\n", **ptr);
    ++(*ptr);
}

static void shader_sm4_read_opcode(const DWORD **ptr, struct wined3d_shader_instruction *ins,
        UINT *param_size, const SHADER_OPCODE *opcode_table, DWORD shader_version)
{
    FIXME("ptr %p, ins %p, param_size %p, opcode_table %p, shader_version %#x stub!\n",
            ptr, ins, param_size, opcode_table, shader_version);
}

static void shader_sm4_read_src_param(const DWORD **ptr, struct wined3d_shader_src_param *src_param,
        struct wined3d_shader_src_param *src_rel_addr, DWORD shader_version)
{
    FIXME("ptr %p, src_param %p, src_rel_addr %p, shader_version %#x stub!\n",
            ptr, src_param, src_rel_addr, shader_version);
}

static void shader_sm4_read_dst_param(const DWORD **ptr, struct wined3d_shader_dst_param *dst_param,
        struct wined3d_shader_src_param *dst_rel_addr, DWORD shader_version)
{
    FIXME("ptr %p, dst_param %p, dst_rel_addr %p, shader_version %#x stub!\n",
            ptr, dst_param, dst_rel_addr, shader_version);
}

static void shader_sm4_read_semantic(const DWORD **ptr, struct wined3d_shader_semantic *semantic)
{
    FIXME("ptr %p, semantic %p stub!\n", ptr, semantic);
}

static void shader_sm4_read_comment(const DWORD **ptr, const char **comment)
{
    FIXME("ptr %p, comment %p stub!\n", ptr, comment);
    *comment = NULL;
}

static BOOL shader_sm4_is_end(const DWORD **ptr)
{
    FIXME("ptr %p stub!\n", ptr);
    return TRUE;
}

const struct wined3d_shader_frontend sm4_shader_frontend =
{
    shader_sm4_read_header,
    shader_sm4_read_opcode,
    shader_sm4_read_src_param,
    shader_sm4_read_dst_param,
    shader_sm4_read_semantic,
    shader_sm4_read_comment,
    shader_sm4_is_end,
};
