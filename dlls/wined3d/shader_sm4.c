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

#define WINED3D_SM4_INSTRUCTION_LENGTH_SHIFT    24
#define WINED3D_SM4_INSTRUCTION_LENGTH_MASK     (0xf << WINED3D_SM4_INSTRUCTION_LENGTH_SHIFT)

#define WINED3D_SM4_OPCODE_MASK                 0xff

#define WINED3D_SM4_REGISTER_TYPE_SHIFT         12
#define WINED3D_SM4_REGISTER_TYPE_MASK          (0xf << WINED3D_SM4_REGISTER_TYPE_SHIFT)

#define WINED3D_SM4_WRITEMASK_SHIFT             4
#define WINED3D_SM4_WRITEMASK_MASK              (0xf << WINED3D_SM4_WRITEMASK_SHIFT)

enum wined3d_sm4_register_type
{
    WINED3D_SM4_RT_TEMP     = 0x0,
    WINED3D_SM4_RT_INPUT    = 0x1,
    WINED3D_SM4_RT_OUTPUT   = 0x2,
};

struct wined3d_sm4_data
{
    const DWORD *end;
};

static const WINED3DSHADER_PARAM_REGISTER_TYPE register_type_table[] =
{
    /* WINED3D_SM4_RT_TEMP */   WINED3DSPR_TEMP,
    /* WINED3D_SM4_RT_INPUT */  WINED3DSPR_INPUT,
    /* WINED3D_SM4_RT_OUTPUT */ WINED3DSPR_OUTPUT,
};

static void *shader_sm4_init(const DWORD *byte_code)
{
    struct wined3d_sm4_data *priv = HeapAlloc(GetProcessHeap(), 0, sizeof(*priv));
    if (!priv)
    {
        ERR("Failed to allocate private data\n");
        return NULL;
    }

    return priv;
}

static void shader_sm4_free(void *data)
{
    HeapFree(GetProcessHeap(), 0, data);
}

static void shader_sm4_read_header(void *data, const DWORD **ptr, DWORD *shader_version)
{
    struct wined3d_sm4_data *priv = data;
    priv->end = *ptr;

    TRACE("version: 0x%08x\n", **ptr);
    *shader_version = *(*ptr)++;
    TRACE("token count: %u\n", **ptr);
    priv->end += *(*ptr)++;
}

static void shader_sm4_read_opcode(void *data, const DWORD **ptr, struct wined3d_shader_instruction *ins,
        UINT *param_size, DWORD shader_version)
{
    DWORD token = *(*ptr)++;
    DWORD opcode = token & WINED3D_SM4_OPCODE_MASK;

    FIXME("Unrecognized opcode %#x, token 0x%08x\n", opcode, token);

    ins->handler_idx = WINED3DSIH_TABLE_SIZE;
    ins->flags = 0;
    ins->coissue = 0;
    ins->predicate = 0;
    ins->dst_count = 0;
    ins->src_count = 0;
    *param_size = ((token & WINED3D_SM4_INSTRUCTION_LENGTH_MASK) >> WINED3D_SM4_INSTRUCTION_LENGTH_SHIFT) - 1;
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
    DWORD token = *(*ptr)++;
    UINT register_idx = *(*ptr)++;
    enum wined3d_sm4_register_type register_type;

    register_type = (token & WINED3D_SM4_REGISTER_TYPE_MASK) >> WINED3D_SM4_REGISTER_TYPE_SHIFT;
    if (register_type >= sizeof(register_type_table) / sizeof(*register_type_table))
    {
        FIXME("Unhandled register type %#x\n", register_type);
        dst_param->register_type = WINED3DSPR_TEMP;
    }
    else
    {
        dst_param->register_type = register_type_table[register_type];
    }

    dst_param->register_idx = register_idx;
    dst_param->write_mask = (token & WINED3D_SM4_WRITEMASK_MASK) >> WINED3D_SM4_WRITEMASK_SHIFT;
    dst_param->modifiers = 0;
    dst_param->shift = 0;
    dst_param->rel_addr = NULL;
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

static BOOL shader_sm4_is_end(void *data, const DWORD **ptr)
{
    struct wined3d_sm4_data *priv = data;
    return *ptr == priv->end;
}

const struct wined3d_shader_frontend sm4_shader_frontend =
{
    shader_sm4_init,
    shader_sm4_free,
    shader_sm4_read_header,
    shader_sm4_read_opcode,
    shader_sm4_read_src_param,
    shader_sm4_read_dst_param,
    shader_sm4_read_semantic,
    shader_sm4_read_comment,
    shader_sm4_is_end,
};
