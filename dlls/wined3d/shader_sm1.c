/*
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
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

/* DCL usage masks */
#define WINED3DSP_DCL_USAGE_SHIFT               0
#define WINED3DSP_DCL_USAGE_MASK                (0xf << WINED3DSP_DCL_USAGE_SHIFT)
#define WINED3DSP_DCL_USAGEINDEX_SHIFT          16
#define WINED3DSP_DCL_USAGEINDEX_MASK           (0xf << WINED3DSP_DCL_USAGEINDEX_SHIFT)

/* DCL sampler type */
#define WINED3DSP_TEXTURETYPE_SHIFT             27
#define WINED3DSP_TEXTURETYPE_MASK              (0xf << WINED3DSP_TEXTURETYPE_SHIFT)

/* Opcode-related masks */
#define WINED3DSI_OPCODE_MASK                   0x0000ffff

#define WINED3D_OPCODESPECIFICCONTROL_SHIFT     16
#define WINED3D_OPCODESPECIFICCONTROL_MASK      (0xff << WINED3D_OPCODESPECIFICCONTROL_SHIFT)

#define WINED3DSI_INSTLENGTH_SHIFT              24
#define WINED3DSI_INSTLENGTH_MASK               (0xf << WINED3DSI_INSTLENGTH_SHIFT)

#define WINED3DSI_COISSUE                       (1 << 30)

#define WINED3DSI_COMMENTSIZE_SHIFT             16
#define WINED3DSI_COMMENTSIZE_MASK              (0x7fff << WINED3DSI_COMMENTSIZE_SHIFT)

#define WINED3DSHADER_INSTRUCTION_PREDICATED    (1 << 28)

/* Register number mask */
#define WINED3DSP_REGNUM_MASK                   0x000007ff

/* Register type masks  */
#define WINED3DSP_REGTYPE_SHIFT                 28
#define WINED3DSP_REGTYPE_MASK                  (0x7 << WINED3DSP_REGTYPE_SHIFT)
#define WINED3DSP_REGTYPE_SHIFT2                8
#define WINED3DSP_REGTYPE_MASK2                 (0x18 << WINED3DSP_REGTYPE_SHIFT2)

/* Relative addressing mask */
#define WINED3DSHADER_ADDRESSMODE_SHIFT         13
#define WINED3DSHADER_ADDRESSMODE_MASK          (1 << WINED3DSHADER_ADDRESSMODE_SHIFT)

/* Destination modifier mask */
#define WINED3DSP_DSTMOD_SHIFT                  20
#define WINED3DSP_DSTMOD_MASK                   (0xf << WINED3DSP_DSTMOD_SHIFT)

/* Destination shift mask */
#define WINED3DSP_DSTSHIFT_SHIFT                24
#define WINED3DSP_DSTSHIFT_MASK                 (0xf << WINED3DSP_DSTSHIFT_SHIFT)

/* Swizzle mask */
#define WINED3DSP_SWIZZLE_SHIFT                 16
#define WINED3DSP_SWIZZLE_MASK                  (0xff << WINED3DSP_SWIZZLE_SHIFT)

/* Source modifier mask */
#define WINED3DSP_SRCMOD_SHIFT                  24
#define WINED3DSP_SRCMOD_MASK                   (0xf << WINED3DSP_SRCMOD_SHIFT)

enum WINED3DSHADER_ADDRESSMODE_TYPE
{
    WINED3DSHADER_ADDRMODE_ABSOLUTE = 0 << WINED3DSHADER_ADDRESSMODE_SHIFT,
    WINED3DSHADER_ADDRMODE_RELATIVE = 1 << WINED3DSHADER_ADDRESSMODE_SHIFT,
};

/* Read a parameter opcode from the input stream,
 * and possibly a relative addressing token.
 * Return the number of tokens read */
static int shader_get_param(const DWORD *ptr, DWORD shader_version, DWORD *token, DWORD *addr_token)
{
    UINT count = 1;

    *token = *ptr;

    /* PS >= 3.0 have relative addressing (with token)
     * VS >= 2.0 have relative addressing (with token)
     * VS >= 1.0 < 2.0 have relative addressing (without token)
     * The version check below should work in general */
    if (*ptr & WINED3DSHADER_ADDRMODE_RELATIVE)
    {
        if (WINED3DSHADER_VERSION_MAJOR(shader_version) < 2)
        {
            *addr_token = (1 << 31)
                    | ((WINED3DSPR_ADDR << WINED3DSP_REGTYPE_SHIFT2) & WINED3DSP_REGTYPE_MASK2)
                    | ((WINED3DSPR_ADDR << WINED3DSP_REGTYPE_SHIFT) & WINED3DSP_REGTYPE_MASK)
                    | (WINED3DSP_NOSWIZZLE << WINED3DSP_SWIZZLE_SHIFT);
        }
        else
        {
            *addr_token = *(ptr + 1);
            ++count;
        }
    }

    return count;
}

static const SHADER_OPCODE *shader_get_opcode(const SHADER_OPCODE *opcode_table, DWORD shader_version, DWORD code)
{
    DWORD i = 0;

    while (opcode_table[i].handler_idx != WINED3DSIH_TABLE_SIZE)
    {
        if ((code & WINED3DSI_OPCODE_MASK) == opcode_table[i].opcode
                && shader_version >= opcode_table[i].min_version
                && (!opcode_table[i].max_version || shader_version <= opcode_table[i].max_version))
        {
            return &opcode_table[i];
        }
        ++i;
    }

    FIXME("Unsupported opcode %#x(%d) masked %#x, shader version %#x\n",
            code, code, code & WINED3DSI_OPCODE_MASK, shader_version);

    return NULL;
}

/* Return the number of parameters to skip for an opcode */
static inline int shader_skip_opcode(const SHADER_OPCODE *opcode_info, DWORD opcode_token, DWORD shader_version)
{
   /* Shaders >= 2.0 may contain address tokens, but fortunately they
    * have a useful length mask - use it here. Shaders 1.0 contain no such tokens */
    return (WINED3DSHADER_VERSION_MAJOR(shader_version) >= 2)
            ? ((opcode_token & WINED3DSI_INSTLENGTH_MASK) >> WINED3DSI_INSTLENGTH_SHIFT) : opcode_info->num_params;
}

static void shader_parse_src_param(DWORD param, const struct wined3d_shader_src_param *rel_addr,
        struct wined3d_shader_src_param *src)
{
    src->register_type = ((param & WINED3DSP_REGTYPE_MASK) >> WINED3DSP_REGTYPE_SHIFT)
            | ((param & WINED3DSP_REGTYPE_MASK2) >> WINED3DSP_REGTYPE_SHIFT2);
    src->register_idx = param & WINED3DSP_REGNUM_MASK;
    src->swizzle = (param & WINED3DSP_SWIZZLE_MASK) >> WINED3DSP_SWIZZLE_SHIFT;
    src->modifiers = (param & WINED3DSP_SRCMOD_MASK) >> WINED3DSP_SRCMOD_SHIFT;
    src->rel_addr = rel_addr;
}

static void shader_parse_dst_param(DWORD param, const struct wined3d_shader_src_param *rel_addr,
        struct wined3d_shader_dst_param *dst)
{
    dst->register_type = ((param & WINED3DSP_REGTYPE_MASK) >> WINED3DSP_REGTYPE_SHIFT)
            | ((param & WINED3DSP_REGTYPE_MASK2) >> WINED3DSP_REGTYPE_SHIFT2);
    dst->register_idx = param & WINED3DSP_REGNUM_MASK;
    dst->write_mask = param & WINED3DSP_WRITEMASK_ALL;
    dst->modifiers = (param & WINED3DSP_DSTMOD_MASK) >> WINED3DSP_DSTMOD_SHIFT;
    dst->shift = (param & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;
    dst->rel_addr = rel_addr;
}

/* Read the parameters of an unrecognized opcode from the input stream
 * Return the number of tokens read.
 *
 * Note: This function assumes source or destination token format.
 * It will not work with specially-formatted tokens like DEF or DCL,
 * but hopefully those would be recognized */
static int shader_skip_unrecognized(const DWORD *ptr, DWORD shader_version)
{
    int tokens_read = 0;
    int i = 0;

    /* TODO: Think of a good name for 0x80000000 and replace it with a constant */
    while (*ptr & 0x80000000)
    {
        DWORD token, addr_token = 0;
        struct wined3d_shader_src_param rel_addr;

        tokens_read += shader_get_param(ptr, shader_version, &token, &addr_token);
        ptr += tokens_read;

        FIXME("Unrecognized opcode param: token=0x%08x addr_token=0x%08x name=", token, addr_token);

        if (token & WINED3DSHADER_ADDRMODE_RELATIVE) shader_parse_src_param(addr_token, NULL, &rel_addr);

        if (!i)
        {
            struct wined3d_shader_dst_param dst;

            shader_parse_dst_param(token, token & WINED3DSHADER_ADDRMODE_RELATIVE ? &rel_addr : NULL, &dst);
            shader_dump_dst_param(&dst, shader_version);
        }
        else
        {
            struct wined3d_shader_src_param src;

            shader_parse_src_param(token, token & WINED3DSHADER_ADDRMODE_RELATIVE ? &rel_addr : NULL, &src);
            shader_dump_src_param(&src, shader_version);
        }
        FIXME("\n");
        ++i;
    }
    return tokens_read;
}

void shader_sm1_read_opcode(const DWORD **ptr, struct wined3d_shader_instruction *ins, UINT *param_size,
        const SHADER_OPCODE *opcode_table, DWORD shader_version)
{
    const SHADER_OPCODE *opcode_info;
    DWORD opcode_token;

    opcode_token = *(*ptr)++;
    opcode_info = shader_get_opcode(opcode_table, shader_version, opcode_token);
    if (!opcode_info)
    {
        FIXME("Unrecognized opcode: token=0x%08x\n", opcode_token);
        ins->handler_idx = WINED3DSIH_TABLE_SIZE;
        *param_size = shader_skip_unrecognized(*ptr, shader_version);
        return;
    }

    ins->handler_idx = opcode_info->handler_idx;
    ins->flags = (opcode_token & WINED3D_OPCODESPECIFICCONTROL_MASK) >> WINED3D_OPCODESPECIFICCONTROL_SHIFT;
    ins->coissue = opcode_token & WINED3DSI_COISSUE;
    ins->predicate = opcode_token & WINED3DSHADER_INSTRUCTION_PREDICATED;
    ins->dst_count = opcode_info->dst_token ? 1 : 0;
    ins->src_count = opcode_info->num_params - opcode_info->dst_token;
    *param_size = shader_skip_opcode(opcode_info, opcode_token, shader_version);
}

void shader_sm1_read_src_param(const DWORD **ptr, struct wined3d_shader_src_param *src_param,
        struct wined3d_shader_src_param *src_rel_addr, DWORD shader_version)
{
    DWORD token, addr_token;

    *ptr += shader_get_param(*ptr, shader_version, &token, &addr_token);
    if (token & WINED3DSHADER_ADDRMODE_RELATIVE)
    {
        shader_parse_src_param(addr_token, NULL, src_rel_addr);
        shader_parse_src_param(token, src_rel_addr, src_param);
    }
    else
    {
        shader_parse_src_param(token, NULL, src_param);
    }
}

void shader_sm1_read_dst_param(const DWORD **ptr, struct wined3d_shader_dst_param *dst_param,
        struct wined3d_shader_src_param *dst_rel_addr, DWORD shader_version)
{
    DWORD token, addr_token;

    *ptr += shader_get_param(*ptr, shader_version, &token, &addr_token);
    if (token & WINED3DSHADER_ADDRMODE_RELATIVE)
    {
        shader_parse_src_param(addr_token, NULL, dst_rel_addr);
        shader_parse_dst_param(token, dst_rel_addr, dst_param);
    }
    else
    {
        shader_parse_dst_param(token, NULL, dst_param);
    }
}

void shader_sm1_read_semantic(const DWORD **ptr, struct wined3d_shader_semantic *semantic)
{
    DWORD usage_token = *(*ptr)++;
    DWORD dst_token = *(*ptr)++;

    semantic->usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
    semantic->usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;
    semantic->sampler_type = (usage_token & WINED3DSP_TEXTURETYPE_MASK) >> WINED3DSP_TEXTURETYPE_SHIFT;
    shader_parse_dst_param(dst_token, NULL, &semantic->reg);
}

void shader_sm1_read_comment(const DWORD **ptr, const char **comment)
{
    DWORD token = **ptr;

    if ((token & WINED3DSI_OPCODE_MASK) != WINED3DSIO_COMMENT)
    {
        *comment = NULL;
        return;
    }

    *comment = (const char *)++(*ptr);
    *ptr += (token & WINED3DSI_COMMENTSIZE_MASK) >> WINED3DSI_COMMENTSIZE_SHIFT;
}
