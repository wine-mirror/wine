/*
 * shaders implementation
 *
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
#include <string.h>
#include <stdio.h>
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d);

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

typedef enum _WINED3DSHADER_ADDRESSMODE_TYPE
{
  WINED3DSHADER_ADDRMODE_ABSOLUTE = 0 << WINED3DSHADER_ADDRESSMODE_SHIFT,
  WINED3DSHADER_ADDRMODE_RELATIVE = 1 << WINED3DSHADER_ADDRESSMODE_SHIFT,
  WINED3DSHADER_ADDRMODE_FORCE_DWORD = 0x7fffffff,
} WINED3DSHADER_ADDRESSMODE_TYPE;

static void shader_dump_param(const DWORD param, const DWORD addr_token, BOOL is_src, DWORD shader_version);

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
        tokens_read += shader_get_param(ptr, shader_version, &token, &addr_token);
        ptr += tokens_read;

        FIXME("Unrecognized opcode param: token=0x%08x addr_token=0x%08x name=", token, addr_token);
        shader_dump_param(token, addr_token, i, shader_version);
        FIXME("\n");
        ++i;
    }
    return tokens_read;
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

static void shader_sm1_read_opcode(const DWORD **ptr, struct wined3d_shader_instruction *ins, UINT *param_size,
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

static void shader_sm1_read_src_param(const DWORD **ptr, struct wined3d_shader_src_param *src_param,
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

static void shader_sm1_read_dst_param(const DWORD **ptr, struct wined3d_shader_dst_param *dst_param,
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

static void shader_sm1_read_semantic(const DWORD **ptr, struct wined3d_shader_semantic *semantic)
{
    DWORD usage_token = *(*ptr)++;
    DWORD dst_token = *(*ptr)++;

    semantic->usage = (usage_token & WINED3DSP_DCL_USAGE_MASK) >> WINED3DSP_DCL_USAGE_SHIFT;
    semantic->usage_idx = (usage_token & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;
    semantic->sampler_type = (usage_token & WINED3DSP_TEXTURETYPE_MASK) >> WINED3DSP_TEXTURETYPE_SHIFT;
    shader_parse_dst_param(dst_token, NULL, &semantic->reg);
}

static const char *shader_opcode_names[] =
{
    /* WINED3DSIH_ABS           */ "abs",
    /* WINED3DSIH_ADD           */ "add",
    /* WINED3DSIH_BEM           */ "bem",
    /* WINED3DSIH_BREAK         */ "break",
    /* WINED3DSIH_BREAKC        */ "breakc",
    /* WINED3DSIH_BREAKP        */ "breakp",
    /* WINED3DSIH_CALL          */ "call",
    /* WINED3DSIH_CALLNZ        */ "callnz",
    /* WINED3DSIH_CMP           */ "cmp",
    /* WINED3DSIH_CND           */ "cnd",
    /* WINED3DSIH_CRS           */ "crs",
    /* WINED3DSIH_DCL           */ "dcl",
    /* WINED3DSIH_DEF           */ "def",
    /* WINED3DSIH_DEFB          */ "defb",
    /* WINED3DSIH_DEFI          */ "defi",
    /* WINED3DSIH_DP2ADD        */ "dp2add",
    /* WINED3DSIH_DP3           */ "dp3",
    /* WINED3DSIH_DP4           */ "dp4",
    /* WINED3DSIH_DST           */ "dst",
    /* WINED3DSIH_DSX           */ "dsx",
    /* WINED3DSIH_DSY           */ "dsy",
    /* WINED3DSIH_ELSE          */ "else",
    /* WINED3DSIH_ENDIF         */ "endif",
    /* WINED3DSIH_ENDLOOP       */ "endloop",
    /* WINED3DSIH_ENDREP        */ "endrep",
    /* WINED3DSIH_EXP           */ "exp",
    /* WINED3DSIH_EXPP          */ "expp",
    /* WINED3DSIH_FRC           */ "frc",
    /* WINED3DSIH_IF            */ "if",
    /* WINED3DSIH_IFC           */ "ifc",
    /* WINED3DSIH_LABEL         */ "label",
    /* WINED3DSIH_LIT           */ "lit",
    /* WINED3DSIH_LOG           */ "log",
    /* WINED3DSIH_LOGP          */ "logp",
    /* WINED3DSIH_LOOP          */ "loop",
    /* WINED3DSIH_LRP           */ "lrp",
    /* WINED3DSIH_M3x2          */ "m3x2",
    /* WINED3DSIH_M3x3          */ "m3x3",
    /* WINED3DSIH_M3x4          */ "m3x4",
    /* WINED3DSIH_M4x3          */ "m4x3",
    /* WINED3DSIH_M4x4          */ "m4x4",
    /* WINED3DSIH_MAD           */ "mad",
    /* WINED3DSIH_MAX           */ "max",
    /* WINED3DSIH_MIN           */ "min",
    /* WINED3DSIH_MOV           */ "mov",
    /* WINED3DSIH_MOVA          */ "mova",
    /* WINED3DSIH_MUL           */ "mul",
    /* WINED3DSIH_NOP           */ "nop",
    /* WINED3DSIH_NRM           */ "nrm",
    /* WINED3DSIH_PHASE         */ "phase",
    /* WINED3DSIH_POW           */ "pow",
    /* WINED3DSIH_RCP           */ "rcp",
    /* WINED3DSIH_REP           */ "rep",
    /* WINED3DSIH_RET           */ "ret",
    /* WINED3DSIH_RSQ           */ "rsq",
    /* WINED3DSIH_SETP          */ "setp",
    /* WINED3DSIH_SGE           */ "sge",
    /* WINED3DSIH_SGN           */ "sgn",
    /* WINED3DSIH_SINCOS        */ "sincos",
    /* WINED3DSIH_SLT           */ "slt",
    /* WINED3DSIH_SUB           */ "sub",
    /* WINED3DSIH_TEX           */ "texld",
    /* WINED3DSIH_TEXBEM        */ "texbem",
    /* WINED3DSIH_TEXBEML       */ "texbeml",
    /* WINED3DSIH_TEXCOORD      */ "texcrd",
    /* WINED3DSIH_TEXDEPTH      */ "texdepth",
    /* WINED3DSIH_TEXDP3        */ "texdp3",
    /* WINED3DSIH_TEXDP3TEX     */ "texdp3tex",
    /* WINED3DSIH_TEXKILL       */ "texkill",
    /* WINED3DSIH_TEXLDD        */ "texldd",
    /* WINED3DSIH_TEXLDL        */ "texldl",
    /* WINED3DSIH_TEXM3x2DEPTH  */ "texm3x2depth",
    /* WINED3DSIH_TEXM3x2PAD    */ "texm3x2pad",
    /* WINED3DSIH_TEXM3x2TEX    */ "texm3x2tex",
    /* WINED3DSIH_TEXM3x3       */ "texm3x3",
    /* WINED3DSIH_TEXM3x3DIFF   */ "texm3x3diff",
    /* WINED3DSIH_TEXM3x3PAD    */ "texm3x3pad",
    /* WINED3DSIH_TEXM3x3SPEC   */ "texm3x3spec",
    /* WINED3DSIH_TEXM3x3TEX    */ "texm3x3tex",
    /* WINED3DSIH_TEXM3x3VSPEC  */ "texm3x3vspec",
    /* WINED3DSIH_TEXREG2AR     */ "texreg2ar",
    /* WINED3DSIH_TEXREG2GB     */ "texreg2gb",
    /* WINED3DSIH_TEXREG2RGB    */ "texreg2rgb",
};

static inline BOOL shader_is_version_token(DWORD token) {
    return shader_is_pshader_version(token) ||
           shader_is_vshader_version(token);
}

void shader_buffer_init(struct SHADER_BUFFER *buffer)
{
    buffer->buffer = HeapAlloc(GetProcessHeap(), 0, SHADER_PGMSIZE);
    buffer->buffer[0] = '\0';
    buffer->bsize = 0;
    buffer->lineNo = 0;
    buffer->newline = TRUE;
}

void shader_buffer_free(struct SHADER_BUFFER *buffer)
{
    HeapFree(GetProcessHeap(), 0, buffer->buffer);
}

int shader_vaddline(SHADER_BUFFER* buffer, const char *format, va_list args)
{
    char* base = buffer->buffer + buffer->bsize;
    int rc;

    rc = vsnprintf(base, SHADER_PGMSIZE - 1 - buffer->bsize, format, args);

    if (rc < 0 ||                                   /* C89 */ 
        rc > SHADER_PGMSIZE - 1 - buffer->bsize) {  /* C99 */

        ERR("The buffer allocated for the shader program string "
            "is too small at %d bytes.\n", SHADER_PGMSIZE);
        buffer->bsize = SHADER_PGMSIZE - 1;
        return -1;
    }

    if (buffer->newline) {
        TRACE("GL HW (%u, %u) : %s", buffer->lineNo + 1, buffer->bsize, base);
        buffer->newline = FALSE;
    } else {
        TRACE("%s", base);
    }

    buffer->bsize += rc;
    if (buffer->buffer[buffer->bsize-1] == '\n') {
        buffer->lineNo++;
        buffer->newline = TRUE;
    }
    return 0;
}

int shader_addline(SHADER_BUFFER* buffer, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = shader_vaddline(buffer, format, args);
    va_end(args);

    return ret;
}

void shader_init(struct IWineD3DBaseShaderClass *shader,
        IWineD3DDevice *device, const SHADER_OPCODE *instruction_table)
{
    shader->ref = 1;
    shader->device = device;
    shader->shader_ins = instruction_table;
    list_init(&shader->linked_programs);
}

static inline WINED3DSHADER_PARAM_REGISTER_TYPE shader_get_regtype(DWORD param)
{
    return ((param & WINED3DSP_REGTYPE_MASK) >> WINED3DSP_REGTYPE_SHIFT)
            | ((param & WINED3DSP_REGTYPE_MASK2) >> WINED3DSP_REGTYPE_SHIFT2);
}

static inline DWORD shader_get_writemask(DWORD param)
{
    return param & WINED3DSP_WRITEMASK_ALL;
}

static inline BOOL shader_is_comment(DWORD token)
{
    return WINED3DSIO_COMMENT == (token & WINED3DSI_OPCODE_MASK);
}

/* Convert floating point offset relative
 * to a register file to an absolute offset for float constants */
static unsigned int shader_get_float_offset(WINED3DSHADER_PARAM_REGISTER_TYPE register_type, UINT register_idx)
{
    switch (register_type)
    {
        case WINED3DSPR_CONST: return register_idx;
        case WINED3DSPR_CONST2: return 2048 + register_idx;
        case WINED3DSPR_CONST3: return 4096 + register_idx;
        case WINED3DSPR_CONST4: return 6144 + register_idx;
        default:
            FIXME("Unsupported register type: %d\n", register_type);
            return register_idx;
    }
}

static void shader_delete_constant_list(struct list* clist) {

    struct list *ptr;
    struct local_constant* constant;

    ptr = list_head(clist);
    while (ptr) {
        constant = LIST_ENTRY(ptr, struct local_constant, entry);
        ptr = list_next(clist, ptr);
        HeapFree(GetProcessHeap(), 0, constant);
    }
    list_init(clist);
}

static void shader_record_register_usage(IWineD3DBaseShaderImpl *This, struct shader_reg_maps *reg_maps,
        DWORD register_type, UINT register_idx, BOOL has_rel_addr, BOOL pshader)
{
    switch (register_type)
    {
        case WINED3DSPR_TEXTURE: /* WINED3DSPR_ADDR */
            if (pshader) reg_maps->texcoord[register_idx] = 1;
            else reg_maps->address[register_idx] = 1;
            break;

        case WINED3DSPR_TEMP:
            reg_maps->temporary[register_idx] = 1;
            break;

        case WINED3DSPR_INPUT:
            if (!pshader) reg_maps->attributes[register_idx] = 1;
            else
            {
                if (has_rel_addr)
                {
                    /* If relative addressing is used, we must assume that all registers
                     * are used. Even if it is a construct like v3[aL], we can't assume
                     * that v0, v1 and v2 aren't read because aL can be negative */
                    unsigned int i;
                    for (i = 0; i < MAX_REG_INPUT; ++i)
                    {
                        ((IWineD3DPixelShaderImpl *)This)->input_reg_used[i] = TRUE;
                    }
                }
                else
                {
                    ((IWineD3DPixelShaderImpl *)This)->input_reg_used[register_idx] = TRUE;
                }
            }
            break;

        case WINED3DSPR_RASTOUT:
            if (register_idx == 1) reg_maps->fog = 1;
            break;

        case WINED3DSPR_MISCTYPE:
            if (pshader && register_idx == 0) reg_maps->vpos = 1;
            break;

        case WINED3DSPR_CONST:
            if (has_rel_addr)
            {
                if (!pshader)
                {
                    if (register_idx <= ((IWineD3DVertexShaderImpl *)This)->min_rel_offset)
                        ((IWineD3DVertexShaderImpl *)This)->min_rel_offset = register_idx;
                    else if (register_idx >= ((IWineD3DVertexShaderImpl *)This)->max_rel_offset)
                        ((IWineD3DVertexShaderImpl *)This)->max_rel_offset = register_idx;
                }
                reg_maps->usesrelconstF = TRUE;
            }
            break;

        case WINED3DSPR_CONSTINT:
            reg_maps->integer_constants |= (1 << register_idx);
            break;

        case WINED3DSPR_CONSTBOOL:
            reg_maps->boolean_constants |= (1 << register_idx);
            break;

        default:
            TRACE("Not recording register of type %#x and idx %u\n", register_type, register_idx);
            break;
    }
}

/* Note that this does not count the loop register
 * as an address register. */

HRESULT shader_get_registers_used(IWineD3DBaseShader *iface, struct shader_reg_maps *reg_maps,
        struct wined3d_shader_semantic *semantics_in, struct wined3d_shader_semantic *semantics_out,
        const DWORD *byte_code)
{
    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    const SHADER_OPCODE *shader_ins = This->baseShader.shader_ins;
    DWORD shader_version;
    unsigned int cur_loop_depth = 0, max_loop_depth = 0;
    const DWORD* pToken = byte_code;
    char pshader;

    /* There are some minor differences between pixel and vertex shaders */

    memset(reg_maps, 0, sizeof(*reg_maps));

    /* get_registers_used is called on every compile on some 1.x shaders, which can result
     * in stacking up a collection of local constants. Delete the old constants if existing
     */
    shader_delete_constant_list(&This->baseShader.constantsF);
    shader_delete_constant_list(&This->baseShader.constantsB);
    shader_delete_constant_list(&This->baseShader.constantsI);

    /* The version token is supposed to be the first token */
    if (!shader_is_version_token(*pToken))
    {
        FIXME("First token is not a version token, invalid shader.\n");
        return WINED3DERR_INVALIDCALL;
    }
    reg_maps->shader_version = shader_version = *pToken++;
    pshader = shader_is_pshader_version(shader_version);

    while (WINED3DVS_END() != *pToken) {
        struct wined3d_shader_instruction ins;
        UINT param_size;

        /* Skip comments */
        if (shader_is_comment(*pToken))
        {
             DWORD comment_len = (*pToken & WINED3DSI_COMMENTSIZE_MASK) >> WINED3DSI_COMMENTSIZE_SHIFT;
             ++pToken;
             pToken += comment_len;
             continue;
        }

        /* Fetch opcode */
        shader_sm1_read_opcode(&pToken, &ins, &param_size, shader_ins, shader_version);

        /* Unhandled opcode, and its parameters */
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            TRACE("Skipping unrecognized instruction.\n");
            pToken += param_size;
            continue;
        }

        /* Handle declarations */
        if (ins.handler_idx == WINED3DSIH_DCL)
        {
            struct wined3d_shader_semantic semantic;

            shader_sm1_read_semantic(&pToken, &semantic);

            switch (semantic.reg.register_type)
            {
                /* Vshader: mark attributes used
                 * Pshader: mark 3.0 input registers used, save token */
                case WINED3DSPR_INPUT:
                    if (!pshader) reg_maps->attributes[semantic.reg.register_idx] = 1;
                    else reg_maps->packed_input[semantic.reg.register_idx] = 1;
                    semantics_in[semantic.reg.register_idx] = semantic;
                    break;

                /* Vshader: mark 3.0 output registers used, save token */
                case WINED3DSPR_OUTPUT:
                    reg_maps->packed_output[semantic.reg.register_idx] = 1;
                    semantics_out[semantic.reg.register_idx] = semantic;
                    if (semantic.usage == WINED3DDECLUSAGE_FOG) reg_maps->fog = 1;
                    break;

                /* Save sampler usage token */
                case WINED3DSPR_SAMPLER:
                    reg_maps->sampler_type[semantic.reg.register_idx] = semantic.sampler_type;
                    break;

                default:
                    TRACE("Not recording DCL register type %#x.\n", semantic.reg.register_type);
                    break;
            }
        }
        else if (ins.handler_idx == WINED3DSIH_DEF)
        {
            local_constant* lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(local_constant));
            if (!lconst) return E_OUTOFMEMORY;
            lconst->idx = *pToken & WINED3DSP_REGNUM_MASK;
            memcpy(lconst->value, pToken + 1, 4 * sizeof(DWORD));

            /* In pixel shader 1.X shaders, the constants are clamped between [-1;1] */
            if (WINED3DSHADER_VERSION_MAJOR(shader_version) == 1 && pshader)
            {
                float *value = (float *) lconst->value;
                if(value[0] < -1.0) value[0] = -1.0;
                else if(value[0] >  1.0) value[0] =  1.0;
                if(value[1] < -1.0) value[1] = -1.0;
                else if(value[1] >  1.0) value[1] =  1.0;
                if(value[2] < -1.0) value[2] = -1.0;
                else if(value[2] >  1.0) value[2] =  1.0;
                if(value[3] < -1.0) value[3] = -1.0;
                else if(value[3] >  1.0) value[3] =  1.0;
            }

            list_add_head(&This->baseShader.constantsF, &lconst->entry);
            pToken += param_size;
        }
        else if (ins.handler_idx == WINED3DSIH_DEFI)
        {
            local_constant* lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(local_constant));
            if (!lconst) return E_OUTOFMEMORY;
            lconst->idx = *pToken & WINED3DSP_REGNUM_MASK;
            memcpy(lconst->value, pToken + 1, 4 * sizeof(DWORD));
            list_add_head(&This->baseShader.constantsI, &lconst->entry);
            pToken += param_size;
        }
        else if (ins.handler_idx == WINED3DSIH_DEFB)
        {
            local_constant* lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(local_constant));
            if (!lconst) return E_OUTOFMEMORY;
            lconst->idx = *pToken & WINED3DSP_REGNUM_MASK;
            memcpy(lconst->value, pToken + 1, 1 * sizeof(DWORD));
            list_add_head(&This->baseShader.constantsB, &lconst->entry);
            pToken += param_size;
        }
        /* If there's a loop in the shader */
        else if (ins.handler_idx == WINED3DSIH_LOOP
                || ins.handler_idx == WINED3DSIH_REP)
        {
            DWORD reg;

            if(ins.handler_idx == WINED3DSIH_LOOP) {
                reg = pToken[1];
            } else {
                reg = pToken[0];
            }

            cur_loop_depth++;
            if(cur_loop_depth > max_loop_depth)
                max_loop_depth = cur_loop_depth;
            pToken += param_size;

            /* Rep and Loop always use an integer constant for the control parameters */
            reg_maps->integer_constants |= (1 << (reg & WINED3DSP_REGNUM_MASK));
        }
        else if (ins.handler_idx == WINED3DSIH_ENDLOOP
                || ins.handler_idx == WINED3DSIH_ENDREP)
        {
            cur_loop_depth--;
        }
        /* For subroutine prototypes */
        else if (ins.handler_idx == WINED3DSIH_LABEL)
        {

            DWORD snum = *pToken & WINED3DSP_REGNUM_MASK; 
            reg_maps->labels[snum] = 1;
            pToken += param_size;
        }
        /* Set texture, address, temporary registers */
        else
        {
            int i, limit;

            /* Declare 1.X samplers implicitly, based on the destination reg. number */
            if (WINED3DSHADER_VERSION_MAJOR(shader_version) == 1
                    && pshader /* Filter different instructions with the same enum values in VS */
                    && (ins.handler_idx == WINED3DSIH_TEX
                        || ins.handler_idx == WINED3DSIH_TEXBEM
                        || ins.handler_idx == WINED3DSIH_TEXBEML
                        || ins.handler_idx == WINED3DSIH_TEXDP3TEX
                        || ins.handler_idx == WINED3DSIH_TEXM3x2TEX
                        || ins.handler_idx == WINED3DSIH_TEXM3x3SPEC
                        || ins.handler_idx == WINED3DSIH_TEXM3x3TEX
                        || ins.handler_idx == WINED3DSIH_TEXM3x3VSPEC
                        || ins.handler_idx == WINED3DSIH_TEXREG2AR
                        || ins.handler_idx == WINED3DSIH_TEXREG2GB
                        || ins.handler_idx == WINED3DSIH_TEXREG2RGB))
            {
                /* Fake sampler usage, only set reserved bit and ttype */
                DWORD sampler_code = *pToken & WINED3DSP_REGNUM_MASK;

                TRACE("Setting fake 2D sampler for 1.x pixelshader\n");
                reg_maps->sampler_type[sampler_code] = WINED3DSTT_2D;

                /* texbem is only valid with < 1.4 pixel shaders */
                if (ins.handler_idx == WINED3DSIH_TEXBEM
                        || ins.handler_idx == WINED3DSIH_TEXBEML)
                {
                    reg_maps->bumpmat[sampler_code] = TRUE;
                    if (ins.handler_idx == WINED3DSIH_TEXBEML)
                    {
                        reg_maps->luminanceparams[sampler_code] = TRUE;
                    }
                }
            }
            if (ins.handler_idx == WINED3DSIH_NRM)
            {
                reg_maps->usesnrm = 1;
            }
            else if (pshader && ins.handler_idx == WINED3DSIH_BEM)
            {
                DWORD regnum = *pToken & WINED3DSP_REGNUM_MASK;
                reg_maps->bumpmat[regnum] = TRUE;
            }
            else if (ins.handler_idx == WINED3DSIH_DSY)
            {
                reg_maps->usesdsy = 1;
            }

            /* This will loop over all the registers and try to
             * make a bitmask of the ones we're interested in.
             *
             * Relative addressing tokens are ignored, but that's
             * okay, since we'll catch any address registers when
             * they are initialized (required by spec) */

            if (ins.dst_count)
            {
                struct wined3d_shader_dst_param dst_param;
                struct wined3d_shader_src_param dst_rel_addr;

                shader_sm1_read_dst_param(&pToken, &dst_param, &dst_rel_addr, shader_version);

                /* WINED3DSPR_TEXCRDOUT is the same as WINED3DSPR_OUTPUT. _OUTPUT can be > MAX_REG_TEXCRD and
                 * is used in >= 3.0 shaders. Filter 3.0 shaders to prevent overflows, and also filter pixel
                 * shaders because TECRDOUT isn't used in them, but future register types might cause issues */
                if (!pshader && WINED3DSHADER_VERSION_MAJOR(shader_version) < 3
                        && dst_param.register_type == WINED3DSPR_TEXCRDOUT)
                {
                    reg_maps->texcoord_mask[dst_param.register_type] |= dst_param.write_mask;
                }
                else
                {
                    shader_record_register_usage(This, reg_maps, dst_param.register_type,
                            dst_param.register_idx, !!dst_param.rel_addr, pshader);
                }
            }

            limit = ins.src_count + (ins.predicate ? 1 : 0);
            for (i = 0; i < limit; ++i)
            {
                struct wined3d_shader_src_param src_param, src_rel_addr;

                shader_sm1_read_src_param(&pToken, &src_param, &src_rel_addr, shader_version);
                shader_record_register_usage(This, reg_maps, src_param.register_type,
                        src_param.register_idx, !!src_param.rel_addr, pshader);
            }
        }
    }
    ++pToken;
    reg_maps->loop_depth = max_loop_depth;

    This->baseShader.functionLength = ((char *)pToken - (char *)byte_code);

    return WINED3D_OK;
}

static void shader_dump_decl_usage(DWORD decl, DWORD param, DWORD shader_version)
{
    DWORD regtype = shader_get_regtype(param);

    TRACE("dcl");

    if (regtype == WINED3DSPR_SAMPLER) {
        DWORD ttype = (decl & WINED3DSP_TEXTURETYPE_MASK) >> WINED3DSP_TEXTURETYPE_SHIFT;

        switch (ttype) {
            case WINED3DSTT_2D: TRACE("_2d"); break;
            case WINED3DSTT_CUBE: TRACE("_cube"); break;
            case WINED3DSTT_VOLUME: TRACE("_volume"); break;
            default: TRACE("_unknown_ttype(0x%08x)", ttype);
       }

    } else { 

        DWORD usage = decl & WINED3DSP_DCL_USAGE_MASK;
        DWORD idx = (decl & WINED3DSP_DCL_USAGEINDEX_MASK) >> WINED3DSP_DCL_USAGEINDEX_SHIFT;

        /* Pixel shaders 3.0 don't have usage semantics */
        if (shader_is_pshader_version(shader_version) && shader_version < WINED3DPS_VERSION(3,0))
            return;
        else
            TRACE("_");

        switch(usage) {
        case WINED3DDECLUSAGE_POSITION:
            TRACE("position%d", idx);
            break;
        case WINED3DDECLUSAGE_BLENDINDICES:
            TRACE("blend");
            break;
        case WINED3DDECLUSAGE_BLENDWEIGHT:
            TRACE("weight");
            break;
        case WINED3DDECLUSAGE_NORMAL:
            TRACE("normal%d", idx);
            break;
        case WINED3DDECLUSAGE_PSIZE:
            TRACE("psize");
            break;
        case WINED3DDECLUSAGE_COLOR:
            if(idx == 0)  {
                TRACE("color");
            } else {
                TRACE("specular%d", (idx - 1));
            }
            break;
        case WINED3DDECLUSAGE_TEXCOORD:
            TRACE("texture%d", idx);
            break;
        case WINED3DDECLUSAGE_TANGENT:
            TRACE("tangent");
            break;
        case WINED3DDECLUSAGE_BINORMAL:
            TRACE("binormal");
            break;
        case WINED3DDECLUSAGE_TESSFACTOR:
            TRACE("tessfactor");
            break;
        case WINED3DDECLUSAGE_POSITIONT:
            TRACE("positionT%d", idx);
            break;
        case WINED3DDECLUSAGE_FOG:
            TRACE("fog");
            break;
        case WINED3DDECLUSAGE_DEPTH:
            TRACE("depth");
            break;
        case WINED3DDECLUSAGE_SAMPLE:
            TRACE("sample");
            break;
        default:
            FIXME("unknown_semantics(0x%08x)", usage);
        }
    }
}

static void shader_dump_arr_entry(const DWORD param, const DWORD addr_token, unsigned int reg, DWORD shader_version)
{
    char relative =
        ((param & WINED3DSHADER_ADDRESSMODE_MASK) == WINED3DSHADER_ADDRMODE_RELATIVE);

    if (relative) {
        TRACE("[");
        if (addr_token)
            shader_dump_param(addr_token, 0, TRUE, shader_version);
        else
            TRACE("a0.x");
        TRACE(" + ");
     }
     TRACE("%u", reg);
     if (relative)
         TRACE("]");
}

static void shader_dump_param(const DWORD param, const DWORD addr_token, BOOL is_src, DWORD shader_version)
{
    static const char * const rastout_reg_names[] = { "oPos", "oFog", "oPts" };
    static const char * const misctype_reg_names[] = { "vPos", "vFace"};
    const char *swizzle_reg_chars = "xyzw";

    DWORD reg = param & WINED3DSP_REGNUM_MASK;
    DWORD regtype = shader_get_regtype(param);
    DWORD modifier = param & WINED3DSP_SRCMOD_MASK;

    /* There are some minor differences between pixel and vertex shaders */
    char pshader = shader_is_pshader_version(shader_version);

    if (is_src)
    {
        if ( (modifier == WINED3DSPSM_NEG) ||
             (modifier == WINED3DSPSM_BIASNEG) ||
             (modifier == WINED3DSPSM_SIGNNEG) ||
             (modifier == WINED3DSPSM_X2NEG) ||
             (modifier == WINED3DSPSM_ABSNEG) )
            TRACE("-");
        else if (modifier == WINED3DSPSM_COMP)
            TRACE("1-");
        else if (modifier == WINED3DSPSM_NOT)
            TRACE("!");

        if (modifier == WINED3DSPSM_ABS || modifier == WINED3DSPSM_ABSNEG) 
            TRACE("abs(");
    }

    switch (regtype) {
        case WINED3DSPR_TEMP:
            TRACE("r%u", reg);
            break;
        case WINED3DSPR_INPUT:
            TRACE("v");
            shader_dump_arr_entry(param, addr_token, reg, shader_version);
            break;
        case WINED3DSPR_CONST:
        case WINED3DSPR_CONST2:
        case WINED3DSPR_CONST3:
        case WINED3DSPR_CONST4:
            TRACE("c");
            shader_dump_arr_entry(param, addr_token, shader_get_float_offset(shader_get_regtype(param),
                    param & WINED3DSP_REGNUM_MASK), shader_version);
            break;
        case WINED3DSPR_TEXTURE: /* vs: case D3DSPR_ADDR */
            TRACE("%c%u", (pshader? 't':'a'), reg);
            break;        
        case WINED3DSPR_RASTOUT:
            TRACE("%s", rastout_reg_names[reg]);
            break;
        case WINED3DSPR_COLOROUT:
            TRACE("oC%u", reg);
            break;
        case WINED3DSPR_DEPTHOUT:
            TRACE("oDepth");
            break;
        case WINED3DSPR_ATTROUT:
            TRACE("oD%u", reg);
            break;
        case WINED3DSPR_TEXCRDOUT: 

            /* Vertex shaders >= 3.0 use general purpose output registers
             * (WINED3DSPR_OUTPUT), which can include an address token */

            if (WINED3DSHADER_VERSION_MAJOR(shader_version) >= 3) {
                TRACE("o");
                shader_dump_arr_entry(param, addr_token, reg, shader_version);
            }
            else 
               TRACE("oT%u", reg);
            break;
        case WINED3DSPR_CONSTINT:
            TRACE("i");
            shader_dump_arr_entry(param, addr_token, reg, shader_version);
            break;
        case WINED3DSPR_CONSTBOOL:
            TRACE("b");
            shader_dump_arr_entry(param, addr_token, reg, shader_version);
            break;
        case WINED3DSPR_LABEL:
            TRACE("l%u", reg);
            break;
        case WINED3DSPR_LOOP:
            TRACE("aL");
            break;
        case WINED3DSPR_SAMPLER:
            TRACE("s%u", reg);
            break;
        case WINED3DSPR_MISCTYPE:
            if (reg > 1) {
                FIXME("Unhandled misctype register %d\n", reg);
            } else {
                TRACE("%s", misctype_reg_names[reg]);
            }
            break;
        case WINED3DSPR_PREDICATE:
            TRACE("p%u", reg);
            break;
        default:
            TRACE("unhandled_rtype(%#x)", regtype);
            break;
   }

   if (!is_src)
   {
       /* operand output (for modifiers and shift, see dump_ins_modifiers) */

       if ((param & WINED3DSP_WRITEMASK_ALL) != WINED3DSP_WRITEMASK_ALL) {
           TRACE(".");
           if (param & WINED3DSP_WRITEMASK_0) TRACE("%c", swizzle_reg_chars[0]);
           if (param & WINED3DSP_WRITEMASK_1) TRACE("%c", swizzle_reg_chars[1]);
           if (param & WINED3DSP_WRITEMASK_2) TRACE("%c", swizzle_reg_chars[2]);
           if (param & WINED3DSP_WRITEMASK_3) TRACE("%c", swizzle_reg_chars[3]);
       }

   } else {
        /** operand input */
        DWORD swizzle = (param & WINED3DSP_SWIZZLE_MASK) >> WINED3DSP_SWIZZLE_SHIFT;
        DWORD swizzle_x = swizzle & 0x03;
        DWORD swizzle_y = (swizzle >> 2) & 0x03;
        DWORD swizzle_z = (swizzle >> 4) & 0x03;
        DWORD swizzle_w = (swizzle >> 6) & 0x03;

        if (0 != modifier) {
            switch (modifier) {
                case WINED3DSPSM_NONE:    break;
                case WINED3DSPSM_NEG:     break;
                case WINED3DSPSM_NOT:     break;
                case WINED3DSPSM_BIAS:    TRACE("_bias"); break;
                case WINED3DSPSM_BIASNEG: TRACE("_bias"); break;
                case WINED3DSPSM_SIGN:    TRACE("_bx2"); break;
                case WINED3DSPSM_SIGNNEG: TRACE("_bx2"); break;
                case WINED3DSPSM_COMP:    break;
                case WINED3DSPSM_X2:      TRACE("_x2"); break;
                case WINED3DSPSM_X2NEG:   TRACE("_x2"); break;
                case WINED3DSPSM_DZ:      TRACE("_dz"); break;
                case WINED3DSPSM_DW:      TRACE("_dw"); break;
                case WINED3DSPSM_ABSNEG:  TRACE(")"); break;
                case WINED3DSPSM_ABS:     TRACE(")"); break;
                default:
                    TRACE("_unknown_modifier(%#x)", modifier);
            }
        }

        /**
        * swizzle bits fields:
        *  RRGGBBAA
        */
        if (swizzle != WINED3DSP_NOSWIZZLE)
        {
            if (swizzle_x == swizzle_y &&
                swizzle_x == swizzle_z &&
                swizzle_x == swizzle_w) {
                    TRACE(".%c", swizzle_reg_chars[swizzle_x]);
            } else {
                TRACE(".%c%c%c%c",
                swizzle_reg_chars[swizzle_x],
                swizzle_reg_chars[swizzle_y],
                swizzle_reg_chars[swizzle_z],
                swizzle_reg_chars[swizzle_w]);
            }
        }
    }
}

/* Shared code in order to generate the bulk of the shader string.
 * NOTE: A description of how to parse tokens can be found on msdn */
void shader_generate_main(IWineD3DBaseShader *iface, SHADER_BUFFER* buffer,
        const shader_reg_maps* reg_maps, CONST DWORD* pFunction)
{
    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) This->baseShader.device; /* To access shader backend callbacks */
    const SHADER_OPCODE *opcode_table = This->baseShader.shader_ins;
    const SHADER_HANDLER *handler_table = device->shader_backend->shader_instruction_handler_table;
    DWORD shader_version = reg_maps->shader_version;
    struct wined3d_shader_src_param src_rel_addr[4];
    struct wined3d_shader_src_param src_param[4];
    struct wined3d_shader_src_param dst_rel_addr;
    struct wined3d_shader_dst_param dst_param;
    struct wined3d_shader_instruction ins;
    struct wined3d_shader_context ctx;
    const DWORD *pToken = pFunction;
    SHADER_HANDLER hw_fct;
    DWORD i;

    /* Initialize current parsing state */
    ctx.shader = iface;
    ctx.reg_maps = reg_maps;
    ctx.buffer = buffer;

    ins.ctx = &ctx;
    ins.dst = &dst_param;
    ins.src = src_param;
    This->baseShader.parse_state.current_row = 0;

    if (!shader_is_version_token(*pToken++))
    {
        ERR("First token is not a version token, invalid shader.\n");
        return;
    }

    while (WINED3DPS_END() != *pToken)
    {
        UINT param_size;

        /* Skip comment tokens */
        if (shader_is_comment(*pToken))
        {
            pToken += (*pToken & WINED3DSI_COMMENTSIZE_MASK) >> WINED3DSI_COMMENTSIZE_SHIFT;
            ++pToken;
            continue;
        }

        /* Read opcode */
        shader_sm1_read_opcode(&pToken, &ins, &param_size, opcode_table, shader_version);

        /* Unknown opcode and its parameters */
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            TRACE("Skipping unrecognized instruction.\n");
            pToken += param_size;
            continue;
        }

        /* Nothing to do */
        if (ins.handler_idx == WINED3DSIH_DCL
                || ins.handler_idx == WINED3DSIH_NOP
                || ins.handler_idx == WINED3DSIH_DEF
                || ins.handler_idx == WINED3DSIH_DEFI
                || ins.handler_idx == WINED3DSIH_DEFB
                || ins.handler_idx == WINED3DSIH_PHASE
                || ins.handler_idx == WINED3DSIH_RET)
        {
            pToken += param_size;
            continue;
        }

        /* Select handler */
        hw_fct = handler_table[ins.handler_idx];

        /* Unhandled opcode */
        if (!hw_fct)
        {
            FIXME("Backend can't handle opcode %#x\n", ins.handler_idx);
            pToken += param_size;
            continue;
        }

        /* Destination token */
        if (ins.dst_count) shader_sm1_read_dst_param(&pToken, &dst_param, &dst_rel_addr, shader_version);

        /* Predication token */
        if (ins.predicate) ins.predicate = *pToken++;

        /* Other source tokens */
        for (i = 0; i < ins.src_count; ++i)
        {
            shader_sm1_read_src_param(&pToken, &src_param[i], &src_rel_addr[i], shader_version);
        }

        /* Call appropriate function for output target */
        hw_fct(&ins);

        /* Process instruction modifiers for GLSL apps ( _sat, etc. ) */
        /* FIXME: This should be internal to the shader backend.
         * Also, right now this is the only reason "shader_mode" exists. */
        if (This->baseShader.shader_mode == SHADER_GLSL) shader_glsl_add_instruction_modifiers(&ins);
    }
}

static void shader_dump_ins_modifiers(const DWORD output)
{
    DWORD shift = (output & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;
    DWORD mmask = output & WINED3DSP_DSTMOD_MASK;

    switch (shift) {
        case 0: break;
        case 13: TRACE("_d8"); break;
        case 14: TRACE("_d4"); break;
        case 15: TRACE("_d2"); break;
        case 1: TRACE("_x2"); break;
        case 2: TRACE("_x4"); break;
        case 3: TRACE("_x8"); break;
        default: TRACE("_unhandled_shift(%d)", shift); break;
    }

    if (mmask & WINED3DSPDM_SATURATE)         TRACE("_sat");
    if (mmask & WINED3DSPDM_PARTIALPRECISION) TRACE("_pp");
    if (mmask & WINED3DSPDM_MSAMPCENTROID)    TRACE("_centroid");

    mmask &= ~(WINED3DSPDM_SATURATE | WINED3DSPDM_PARTIALPRECISION | WINED3DSPDM_MSAMPCENTROID);
    if (mmask)
        FIXME("_unrecognized_modifier(%#x)", mmask);
}

void shader_trace_init(const DWORD *pFunction, const SHADER_OPCODE *opcode_table)
{
    const DWORD* pToken = pFunction;
    DWORD shader_version;
    DWORD i;

    TRACE("Parsing %p\n", pFunction);

    /* The version token is supposed to be the first token */
    if (!shader_is_version_token(*pToken))
    {
        FIXME("First token is not a version token, invalid shader.\n");
        return;
    }
    shader_version = *pToken++;
    TRACE("%s_%u_%u\n", shader_is_pshader_version(shader_version) ? "ps": "vs",
            WINED3DSHADER_VERSION_MAJOR(shader_version), WINED3DSHADER_VERSION_MINOR(shader_version));

    while (WINED3DVS_END() != *pToken)
    {
        struct wined3d_shader_instruction ins;
        UINT param_size;

        if (shader_is_comment(*pToken)) /* comment */
        {
            DWORD comment_len = (*pToken & WINED3DSI_COMMENTSIZE_MASK) >> WINED3DSI_COMMENTSIZE_SHIFT;
            ++pToken;
            TRACE("//%s\n", (const char*)pToken);
            pToken += comment_len;
            continue;
        }

        shader_sm1_read_opcode(&pToken, &ins, &param_size, opcode_table, shader_version);
        if (ins.handler_idx == WINED3DSIH_TABLE_SIZE)
        {
            TRACE("Skipping unrecognized instruction.\n");
            pToken += param_size;
            continue;
        }

        if (ins.handler_idx == WINED3DSIH_DCL)
        {
            DWORD usage = *pToken;
            DWORD param = *(pToken + 1);

            shader_dump_decl_usage(usage, param, shader_version);
            shader_dump_ins_modifiers(param);
            TRACE(" ");
            shader_dump_param(param, 0, FALSE, shader_version);
            pToken += 2;
        }
        else if (ins.handler_idx == WINED3DSIH_DEF)
        {
            unsigned int offset = shader_get_float_offset(shader_get_regtype(*pToken),
                    *pToken & WINED3DSP_REGNUM_MASK);

            TRACE("def c%u = %f, %f, %f, %f", offset,
                    *(const float *)(pToken + 1),
                    *(const float *)(pToken + 2),
                    *(const float *)(pToken + 3),
                    *(const float *)(pToken + 4));
            pToken += 5;
        }
        else if (ins.handler_idx == WINED3DSIH_DEFI)
        {
            TRACE("defi i%u = %d, %d, %d, %d", *pToken & WINED3DSP_REGNUM_MASK,
                    *(pToken + 1),
                    *(pToken + 2),
                    *(pToken + 3),
                    *(pToken + 4));
            pToken += 5;
        }
        else if (ins.handler_idx == WINED3DSIH_DEFB)
        {
            TRACE("defb b%u = %s", *pToken & WINED3DSP_REGNUM_MASK,
                    *(pToken + 1)? "true": "false");
            pToken += 2;
        }
        else
        {
            DWORD param, addr_token = 0;
            int tokens_read;

            /* Print out predication source token first - it follows
             * the destination token. */
            if (ins.predicate)
            {
                TRACE("(");
                shader_dump_param(*(pToken + 2), 0, TRUE, shader_version);
                TRACE(") ");
            }

            /* PixWin marks instructions with the coissue flag with a '+' */
            if (ins.coissue) TRACE("+");

            TRACE("%s", shader_opcode_names[ins.handler_idx]);

            if (ins.handler_idx == WINED3DSIH_IFC
                    || ins.handler_idx == WINED3DSIH_BREAKC)
            {
                switch (ins.flags)
                {
                    case COMPARISON_GT: TRACE("_gt"); break;
                    case COMPARISON_EQ: TRACE("_eq"); break;
                    case COMPARISON_GE: TRACE("_ge"); break;
                    case COMPARISON_LT: TRACE("_lt"); break;
                    case COMPARISON_NE: TRACE("_ne"); break;
                    case COMPARISON_LE: TRACE("_le"); break;
                    default: TRACE("_(%u)", ins.flags);
                }
            }
            else if (ins.handler_idx == WINED3DSIH_TEX
                    && shader_version >= WINED3DPS_VERSION(2,0)
                    && (ins.flags & WINED3DSI_TEXLD_PROJECT))
            {
                TRACE("p");
            }

            /* Destination token */
            if (ins.dst_count)
            {
                tokens_read = shader_get_param(pToken, shader_version, &param, &addr_token);
                pToken += tokens_read;

                shader_dump_ins_modifiers(param);
                TRACE(" ");
                shader_dump_param(param, addr_token, FALSE, shader_version);
            }

            /* Predication token - already printed out, just skip it */
            if (ins.predicate) ++pToken;

            /* Other source tokens */
            for (i = ins.dst_count; i < (ins.dst_count + ins.src_count); ++i)
            {
                tokens_read = shader_get_param(pToken, shader_version, &param, &addr_token);
                pToken += tokens_read;

                TRACE((i == 0)? " " : ", ");
                shader_dump_param(param, addr_token, TRUE, shader_version);
            }
        }
        TRACE("\n");
    }
}

void shader_cleanup(IWineD3DBaseShader *iface)
{
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *)iface;

    ((IWineD3DDeviceImpl *)This->baseShader.device)->shader_backend->shader_destroy(iface);
    HeapFree(GetProcessHeap(), 0, This->baseShader.function);
    shader_delete_constant_list(&This->baseShader.constantsF);
    shader_delete_constant_list(&This->baseShader.constantsB);
    shader_delete_constant_list(&This->baseShader.constantsI);
    list_remove(&This->baseShader.shader_list_entry);
}

static const SHADER_HANDLER shader_none_instruction_handler_table[WINED3DSIH_TABLE_SIZE] = {0};
static void shader_none_select(IWineD3DDevice *iface, BOOL usePS, BOOL useVS) {}
static void shader_none_select_depth_blt(IWineD3DDevice *iface, enum tex_types tex_type) {}
static void shader_none_deselect_depth_blt(IWineD3DDevice *iface) {}
static void shader_none_update_float_vertex_constants(IWineD3DDevice *iface, UINT start, UINT count) {}
static void shader_none_update_float_pixel_constants(IWineD3DDevice *iface, UINT start, UINT count) {}
static void shader_none_load_constants(IWineD3DDevice *iface, char usePS, char useVS) {}
static void shader_none_load_np2fixup_constants(IWineD3DDevice *iface, char usePS, char useVS) {}
static void shader_none_destroy(IWineD3DBaseShader *iface) {}
static HRESULT shader_none_alloc(IWineD3DDevice *iface) {return WINED3D_OK;}
static void shader_none_free(IWineD3DDevice *iface) {}
static BOOL shader_none_dirty_const(IWineD3DDevice *iface) {return FALSE;}
static GLuint shader_none_generate_pshader(IWineD3DPixelShader *iface, SHADER_BUFFER *buffer, const struct ps_compile_args *args) {
    FIXME("NONE shader backend asked to generate a pixel shader\n");
    return 0;
}
static GLuint shader_none_generate_vshader(IWineD3DVertexShader *iface, SHADER_BUFFER *buffer, const struct vs_compile_args *args) {
    FIXME("NONE shader backend asked to generate a vertex shader\n");
    return 0;
}

#define GLINFO_LOCATION      (*gl_info)
static void shader_none_get_caps(WINED3DDEVTYPE devtype, const WineD3D_GL_Info *gl_info, struct shader_caps *pCaps)
{
    /* Set the shader caps to 0 for the none shader backend */
    pCaps->VertexShaderVersion  = 0;
    pCaps->PixelShaderVersion    = 0;
    pCaps->PixelShader1xMaxValue = 0.0;
}
#undef GLINFO_LOCATION
static BOOL shader_none_color_fixup_supported(struct color_fixup_desc fixup)
{
    if (TRACE_ON(d3d_shader) && TRACE_ON(d3d))
    {
        TRACE("Checking support for fixup:\n");
        dump_color_fixup_desc(fixup);
    }

    /* Faked to make some apps happy. */
    if (!is_yuv_fixup(fixup))
    {
        TRACE("[OK]\n");
        return TRUE;
    }

    TRACE("[FAILED]\n");
    return FALSE;
}

const shader_backend_t none_shader_backend = {
    shader_none_instruction_handler_table,
    shader_none_select,
    shader_none_select_depth_blt,
    shader_none_deselect_depth_blt,
    shader_none_update_float_vertex_constants,
    shader_none_update_float_pixel_constants,
    shader_none_load_constants,
    shader_none_load_np2fixup_constants,
    shader_none_destroy,
    shader_none_alloc,
    shader_none_free,
    shader_none_dirty_const,
    shader_none_generate_pshader,
    shader_none_generate_vshader,
    shader_none_get_caps,
    shader_none_color_fixup_supported,
};
