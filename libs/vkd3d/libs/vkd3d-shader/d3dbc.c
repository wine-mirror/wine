/*
 * d3dbc (Direct3D shader models 1-3 bytecode) support
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009, 2021 Henri Verbeet for CodeWeavers
 * Copyright 2019-2020, 2023-2026 Elizabeth Figura for CodeWeavers
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

#include "vkd3d_shader_private.h"
#include <math.h>

#define VKD3D_SM1_VS  0xfffeu
#define VKD3D_SM1_PS  0xffffu

#define VKD3D_SM1_DCL_USAGE_SHIFT              0u
#define VKD3D_SM1_DCL_USAGE_MASK               (0xfu << VKD3D_SM1_DCL_USAGE_SHIFT)
#define VKD3D_SM1_DCL_USAGE_INDEX_SHIFT        16u
#define VKD3D_SM1_DCL_USAGE_INDEX_MASK         (0xfu << VKD3D_SM1_DCL_USAGE_INDEX_SHIFT)

#define VKD3D_SM1_RESOURCE_TYPE_SHIFT          27u
#define VKD3D_SM1_RESOURCE_TYPE_MASK           (0xfu << VKD3D_SM1_RESOURCE_TYPE_SHIFT)

#define VKD3D_SM1_OPCODE_MASK                  0x0000ffffu

#define VKD3D_SM1_INSTRUCTION_FLAGS_SHIFT      16u
#define VKD3D_SM1_INSTRUCTION_FLAGS_MASK       (0xffu << VKD3D_SM1_INSTRUCTION_FLAGS_SHIFT)

#define VKD3D_SM1_INSTRUCTION_LENGTH_SHIFT     24u
#define VKD3D_SM1_INSTRUCTION_LENGTH_MASK      (0xfu << VKD3D_SM1_INSTRUCTION_LENGTH_SHIFT)

#define VKD3D_SM1_COISSUE                      (0x1u << 30u)

#define VKD3D_SM1_COMMENT_SIZE_SHIFT           16u
#define VKD3D_SM1_COMMENT_SIZE_MASK            (0x7fffu << VKD3D_SM1_COMMENT_SIZE_SHIFT)

#define VKD3D_SM1_INSTRUCTION_PREDICATED       (0x1u << 28u)

#define VKD3D_SM1_INSTRUCTION_PARAMETER        (0x1u << 31u)

#define VKD3D_SM1_REGISTER_NUMBER_MASK         0x000007ffu

#define VKD3D_SM1_REGISTER_TYPE_SHIFT          28u
#define VKD3D_SM1_REGISTER_TYPE_MASK           (0x7u << VKD3D_SM1_REGISTER_TYPE_SHIFT)
#define VKD3D_SM1_REGISTER_TYPE_SHIFT2         8u
#define VKD3D_SM1_REGISTER_TYPE_MASK2          (0x18u << VKD3D_SM1_REGISTER_TYPE_SHIFT2)

#define VKD3D_SM1_ADDRESS_MODE_SHIFT           13u
#define VKD3D_SM1_ADDRESS_MODE_MASK            (0x1u << VKD3D_SM1_ADDRESS_MODE_SHIFT)

#define VKD3D_SM1_DST_MODIFIER_SHIFT           20u
#define VKD3D_SM1_DST_MODIFIER_MASK            (0xfu << VKD3D_SM1_DST_MODIFIER_SHIFT)

#define VKD3D_SM1_DSTSHIFT_SHIFT               24u
#define VKD3D_SM1_DSTSHIFT_MASK                (0xfu << VKD3D_SM1_DSTSHIFT_SHIFT)

#define VKD3D_SM1_WRITEMASK_SHIFT              16u
#define VKD3D_SM1_WRITEMASK_MASK               (0xfu << VKD3D_SM1_WRITEMASK_SHIFT)

#define VKD3D_SM1_SWIZZLE_SHIFT                16u
#define VKD3D_SM1_SWIZZLE_MASK                 (0xffu << VKD3D_SM1_SWIZZLE_SHIFT)
#define VKD3D_SM1_SWIZZLE_DEFAULT              (0u | (1u << 2) | (2u << 4) | (3u << 6))

#define VKD3D_SM1_SWIZZLE_COMPONENT_SHIFT(idx) (2u * (idx))
#define VKD3D_SM1_SWIZZLE_COMPONENT_MASK(idx)  (0x3u << VKD3D_SM1_SWIZZLE_COMPONENT_SHIFT(idx))

#define VKD3D_SM1_SRC_MODIFIER_SHIFT           24u
#define VKD3D_SM1_SRC_MODIFIER_MASK            (0xfu << VKD3D_SM1_SRC_MODIFIER_SHIFT)

#define VKD3D_SM1_END                          0x0000ffffu

#define VKD3D_SM1_VERSION_MAJOR(version)       (((version) >> 8u) & 0xffu)
#define VKD3D_SM1_VERSION_MINOR(version)       (((version) >> 0u) & 0xffu)

enum vkd3d_sm1_register_type
{
    VKD3D_SM1_REG_TEMP              = 0x00,
    VKD3D_SM1_REG_INPUT             = 0x01,
    VKD3D_SM1_REG_CONST             = 0x02,
    VKD3D_SM1_REG_ADDR              = 0x03,
    VKD3D_SM1_REG_TEXTURE           = 0x03,
    VKD3D_SM1_REG_RASTOUT           = 0x04,
    VKD3D_SM1_REG_ATTROUT           = 0x05,
    VKD3D_SM1_REG_TEXCRDOUT         = 0x06,
    VKD3D_SM1_REG_OUTPUT            = 0x06,
    VKD3D_SM1_REG_CONSTINT          = 0x07,
    VKD3D_SM1_REG_COLOROUT          = 0x08,
    VKD3D_SM1_REG_DEPTHOUT          = 0x09,
    VKD3D_SM1_REG_SAMPLER           = 0x0a,
    VKD3D_SM1_REG_CONST2            = 0x0b,
    VKD3D_SM1_REG_CONST3            = 0x0c,
    VKD3D_SM1_REG_CONST4            = 0x0d,
    VKD3D_SM1_REG_CONSTBOOL         = 0x0e,
    VKD3D_SM1_REG_LOOP              = 0x0f,
    VKD3D_SM1_REG_TEMPFLOAT16       = 0x10,
    VKD3D_SM1_REG_MISCTYPE          = 0x11,
    VKD3D_SM1_REG_LABEL             = 0x12,
    VKD3D_SM1_REG_PREDICATE         = 0x13,
};

enum vkd3d_sm1_address_mode_type
{
    VKD3D_SM1_ADDRESS_MODE_ABSOLUTE = 0x0,
    VKD3D_SM1_ADDRESS_MODE_RELATIVE = 0x1,
};

enum vkd3d_sm1_resource_type
{
    VKD3D_SM1_RESOURCE_UNKNOWN      = 0x0,
    VKD3D_SM1_RESOURCE_TEXTURE_1D   = 0x1,
    VKD3D_SM1_RESOURCE_TEXTURE_2D   = 0x2,
    VKD3D_SM1_RESOURCE_TEXTURE_CUBE = 0x3,
    VKD3D_SM1_RESOURCE_TEXTURE_3D   = 0x4,
    VKD3D_SM1_RESOURCE_COUNT
};

enum vkd3d_sm1_misc_register
{
    VKD3D_SM1_MISC_POSITION         = 0x0,
    VKD3D_SM1_MISC_FACE             = 0x1,
};

enum vkd3d_sm1_opcode
{
    VKD3D_SM1_OP_NOP          = 0x00,
    VKD3D_SM1_OP_MOV          = 0x01,
    VKD3D_SM1_OP_ADD          = 0x02,
    VKD3D_SM1_OP_SUB          = 0x03,
    VKD3D_SM1_OP_MAD          = 0x04,
    VKD3D_SM1_OP_MUL          = 0x05,
    VKD3D_SM1_OP_RCP          = 0x06,
    VKD3D_SM1_OP_RSQ          = 0x07,
    VKD3D_SM1_OP_DP3          = 0x08,
    VKD3D_SM1_OP_DP4          = 0x09,
    VKD3D_SM1_OP_MIN          = 0x0a,
    VKD3D_SM1_OP_MAX          = 0x0b,
    VKD3D_SM1_OP_SLT          = 0x0c,
    VKD3D_SM1_OP_SGE          = 0x0d,
    VKD3D_SM1_OP_EXP          = 0x0e,
    VKD3D_SM1_OP_LOG          = 0x0f,
    VKD3D_SM1_OP_LIT          = 0x10,
    VKD3D_SM1_OP_DST          = 0x11,
    VKD3D_SM1_OP_LRP          = 0x12,
    VKD3D_SM1_OP_FRC          = 0x13,
    VKD3D_SM1_OP_M4x4         = 0x14,
    VKD3D_SM1_OP_M4x3         = 0x15,
    VKD3D_SM1_OP_M3x4         = 0x16,
    VKD3D_SM1_OP_M3x3         = 0x17,
    VKD3D_SM1_OP_M3x2         = 0x18,
    VKD3D_SM1_OP_CALL         = 0x19,
    VKD3D_SM1_OP_CALLNZ       = 0x1a,
    VKD3D_SM1_OP_LOOP         = 0x1b,
    VKD3D_SM1_OP_RET          = 0x1c,
    VKD3D_SM1_OP_ENDLOOP      = 0x1d,
    VKD3D_SM1_OP_LABEL        = 0x1e,
    VKD3D_SM1_OP_DCL          = 0x1f,
    VKD3D_SM1_OP_POW          = 0x20,
    VKD3D_SM1_OP_CRS          = 0x21,
    VKD3D_SM1_OP_SGN          = 0x22,
    VKD3D_SM1_OP_ABS          = 0x23,
    VKD3D_SM1_OP_NRM          = 0x24,
    VKD3D_SM1_OP_SINCOS       = 0x25,
    VKD3D_SM1_OP_REP          = 0x26,
    VKD3D_SM1_OP_ENDREP       = 0x27,
    VKD3D_SM1_OP_IF           = 0x28,
    VKD3D_SM1_OP_IFC          = 0x29,
    VKD3D_SM1_OP_ELSE         = 0x2a,
    VKD3D_SM1_OP_ENDIF        = 0x2b,
    VKD3D_SM1_OP_BREAK        = 0x2c,
    VKD3D_SM1_OP_BREAKC       = 0x2d,
    VKD3D_SM1_OP_MOVA         = 0x2e,
    VKD3D_SM1_OP_DEFB         = 0x2f,
    VKD3D_SM1_OP_DEFI         = 0x30,

    VKD3D_SM1_OP_TEXCOORD     = 0x40,
    VKD3D_SM1_OP_TEXKILL      = 0x41,
    VKD3D_SM1_OP_TEX          = 0x42,
    VKD3D_SM1_OP_TEXBEM       = 0x43,
    VKD3D_SM1_OP_TEXBEML      = 0x44,
    VKD3D_SM1_OP_TEXREG2AR    = 0x45,
    VKD3D_SM1_OP_TEXREG2GB    = 0x46,
    VKD3D_SM1_OP_TEXM3x2PAD   = 0x47,
    VKD3D_SM1_OP_TEXM3x2TEX   = 0x48,
    VKD3D_SM1_OP_TEXM3x3PAD   = 0x49,
    VKD3D_SM1_OP_TEXM3x3TEX   = 0x4a,
    VKD3D_SM1_OP_TEXM3x3DIFF  = 0x4b,
    VKD3D_SM1_OP_TEXM3x3SPEC  = 0x4c,
    VKD3D_SM1_OP_TEXM3x3VSPEC = 0x4d,
    VKD3D_SM1_OP_EXPP         = 0x4e,
    VKD3D_SM1_OP_LOGP         = 0x4f,
    VKD3D_SM1_OP_CND          = 0x50,
    VKD3D_SM1_OP_DEF          = 0x51,
    VKD3D_SM1_OP_TEXREG2RGB   = 0x52,
    VKD3D_SM1_OP_TEXDP3TEX    = 0x53,
    VKD3D_SM1_OP_TEXM3x2DEPTH = 0x54,
    VKD3D_SM1_OP_TEXDP3       = 0x55,
    VKD3D_SM1_OP_TEXM3x3      = 0x56,
    VKD3D_SM1_OP_TEXDEPTH     = 0x57,
    VKD3D_SM1_OP_CMP          = 0x58,
    VKD3D_SM1_OP_BEM          = 0x59,
    VKD3D_SM1_OP_DP2ADD       = 0x5a,
    VKD3D_SM1_OP_DSX          = 0x5b,
    VKD3D_SM1_OP_DSY          = 0x5c,
    VKD3D_SM1_OP_TEXLDD       = 0x5d,
    VKD3D_SM1_OP_SETP         = 0x5e,
    VKD3D_SM1_OP_TEXLDL       = 0x5f,
    VKD3D_SM1_OP_BREAKP       = 0x60,

    VKD3D_SM1_OP_PHASE        = 0xfffd,
    VKD3D_SM1_OP_COMMENT      = 0xfffe,
    VKD3D_SM1_OP_END          = 0Xffff,
};

enum d3dbc_register_type
{
    D3DBC_REGISTER_INVALID = 0,
    D3DBC_REGISTER_TEMP,
    D3DBC_REGISTER_INPUT,
    D3DBC_REGISTER_CONST,
    D3DBC_REGISTER_ADDR,
    D3DBC_REGISTER_TEXTURE,
    D3DBC_REGISTER_RASTOUT,
    D3DBC_REGISTER_ATTROUT,
    D3DBC_REGISTER_TEXCRDOUT,
    D3DBC_REGISTER_OUTPUT,
    D3DBC_REGISTER_CONSTINT,
    D3DBC_REGISTER_COLOROUT,
    D3DBC_REGISTER_DEPTHOUT,
    D3DBC_REGISTER_SAMPLER,
    D3DBC_REGISTER_CONSTBOOL,
    D3DBC_REGISTER_LOOP,
    D3DBC_REGISTER_MISCTYPE,
    D3DBC_REGISTER_LABEL,
    D3DBC_REGISTER_PREDICATE,
    D3DBC_REGISTER_SSA,
};

struct d3dbc_operand
{
    enum d3dbc_register_type type;
    unsigned int index;
    bool has_rel_addr;
};

struct d3dbc_dst_operand
{
    struct d3dbc_operand o;
    uint32_t write_mask;
    enum vkd3d_shader_dst_modifier modifiers;
    unsigned int shift;
};

struct d3dbc_src_operand
{
    struct d3dbc_operand o;
    uint32_t swizzle;
    enum vkd3d_shader_src_modifier modifiers;
};

struct d3dbc_instruction
{
    enum vkd3d_sm1_opcode opcode;
    uint32_t flags;
    bool coissue;

    struct d3dbc_semantic
    {
        enum vkd3d_decl_usage usage;
        unsigned int usage_idx;
        enum vkd3d_sm1_resource_type resource_type;
    } semantic;

    union d3dbc_immediate_constant
    {
        float f32[4];
        int32_t i32[4];
        uint32_t b32;
    } immconst;

    bool predicated;
    struct d3dbc_src_operand predicate;

    struct d3dbc_dst_operand dst[1];
    struct d3dbc_src_operand dst_rel_addr[1];
    unsigned int dst_count;
    struct d3dbc_src_operand src[4];
    struct d3dbc_src_operand src_rel_addr[4];
    unsigned int src_count;
};

struct vkd3d_sm1_opcode_info
{
    enum vkd3d_sm1_opcode sm1_opcode;
    unsigned int dst_count;
    unsigned int src_count;
    const char *name;
    enum vsir_opcode vsir_opcode;
    struct
    {
        unsigned int major, minor;
    } min_version, max_version;
};

struct vkd3d_shader_sm1_parser
{
    const struct vkd3d_sm1_opcode_info *opcode_table;
    const uint32_t *start, *end, *ptr;
    bool abort;

    struct vkd3d_shader_version version;

    struct vkd3d_shader_parser p;
    struct vsir_program *program;

    const struct vkd3d_shader_d3dbc_source_info *d3dbc_source_info;

    struct flat_constant_def
    {
        enum vkd3d_shader_d3dbc_constant_register set;
        uint32_t index;
        union d3dbc_immediate_constant value;
    } *defs;
    size_t def_count, defs_capacity;

    /* Index of the next temporary register to be allocated for internal use. */
    uint32_t internal_temp_id;

    /* Temporary registers representing aL, indexed by loop depth. */
    uint32_t *aL_ids;
    size_t aL_count, aL_ids_capacity;

    uint16_t texture_descriptors;

    struct
    {
#define MAX_CONSTANT_COUNT 8192
        uint32_t def_mask[VKD3D_BITMAP_SIZE(MAX_CONSTANT_COUNT)];
        uint32_t count;
    } constants[3];

    struct vkd3d_shader_sm1_texmpad_state
    {
        uint32_t components_read;
        uint32_t ssa_ids[VKD3D_VEC4_SIZE];
    } texm3x3pad_state, texm3x2pad_state;
};

/* This table is not order or position dependent. */
static const struct vkd3d_sm1_opcode_info vs_opcode_table[] =
{
    /* Arithmetic */
    {VKD3D_SM1_OP_NOP,          0, 0, "nop",            VSIR_OP_NOP},
    {VKD3D_SM1_OP_MOV,          1, 1, "mov",            VSIR_OP_MOV},
    {VKD3D_SM1_OP_MOVA,         1, 1, "mova",           VSIR_OP_MOVA,         {2, 0}},
    {VKD3D_SM1_OP_ADD,          1, 2, "add",            VSIR_OP_ADD},
    {VKD3D_SM1_OP_SUB,          1, 2, "sub",            VSIR_OP_SUB},
    {VKD3D_SM1_OP_MAD,          1, 3, "mad",            VSIR_OP_MAD},
    {VKD3D_SM1_OP_MUL,          1, 2, "mul",            VSIR_OP_MUL},
    {VKD3D_SM1_OP_RCP,          1, 1, "rcp",            VSIR_OP_RCP},
    {VKD3D_SM1_OP_RSQ,          1, 1, "rsq",            VSIR_OP_RSQ},
    {VKD3D_SM1_OP_DP3,          1, 2, "dp3",            VSIR_OP_DP3},
    {VKD3D_SM1_OP_DP4,          1, 2, "dp4",            VSIR_OP_DP4},
    {VKD3D_SM1_OP_MIN,          1, 2, "min",            VSIR_OP_MIN},
    {VKD3D_SM1_OP_MAX,          1, 2, "max",            VSIR_OP_MAX},
    {VKD3D_SM1_OP_SLT,          1, 2, "slt",            VSIR_OP_SLT},
    {VKD3D_SM1_OP_SGE,          1, 2, "sge",            VSIR_OP_SGE},
    {VKD3D_SM1_OP_ABS,          1, 1, "abs",            VSIR_OP_ABS,          {2, 0}},
    {VKD3D_SM1_OP_EXP,          1, 1, "exp",            VSIR_OP_EXP},
    {VKD3D_SM1_OP_LOG,          1, 1, "log",            VSIR_OP_LOG},
    {VKD3D_SM1_OP_EXPP,         1, 1, "expp",           VSIR_OP_EXPP},
    {VKD3D_SM1_OP_LOGP,         1, 1, "logp",           VSIR_OP_LOGP},
    {VKD3D_SM1_OP_LIT,          1, 1, "lit",            VSIR_OP_LIT},
    {VKD3D_SM1_OP_DST,          1, 2, "dst",            VSIR_OP_NOP},
    {VKD3D_SM1_OP_LRP,          1, 3, "lrp",            VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_FRC,          1, 1, "frc",            VSIR_OP_FRC},
    {VKD3D_SM1_OP_POW,          1, 2, "pow",            VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_CRS,          1, 2, "crs",            VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_SGN,          1, 3, "sgn",            VSIR_OP_NOP,          {2, 0}, {2, 1}},
    {VKD3D_SM1_OP_SGN,          1, 1, "sgn",            VSIR_OP_NOP,          {3, 0}},
    {VKD3D_SM1_OP_NRM,          1, 1, "nrm",            VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_SINCOS,       1, 3, "sincos",         VSIR_OP_SINCOS,       {2, 0}, {2, 1}},
    {VKD3D_SM1_OP_SINCOS,       1, 1, "sincos",         VSIR_OP_SINCOS,       {3, 0}},
    /* Matrix */
    {VKD3D_SM1_OP_M4x4,         1, 2, "m4x4",           VSIR_OP_NOP},
    {VKD3D_SM1_OP_M4x3,         1, 2, "m4x3",           VSIR_OP_NOP},
    {VKD3D_SM1_OP_M3x4,         1, 2, "m3x4",           VSIR_OP_NOP},
    {VKD3D_SM1_OP_M3x3,         1, 2, "m3x3",           VSIR_OP_NOP},
    {VKD3D_SM1_OP_M3x2,         1, 2, "m3x2",           VSIR_OP_NOP},
    /* Declarations */
    {VKD3D_SM1_OP_DCL,          1, 0, "dcl",            VSIR_OP_DCL},
    /* Constant definitions */
    {VKD3D_SM1_OP_DEF,          1, 0, "def",            VSIR_OP_NOP},
    {VKD3D_SM1_OP_DEFB,         1, 0, "defb",           VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_DEFI,         1, 0, "defi",           VSIR_OP_NOP,          {2, 0}},
    /* Control flow */
    {VKD3D_SM1_OP_REP,          0, 1, "rep",            VSIR_OP_REP,          {2, 0}},
    {VKD3D_SM1_OP_ENDREP,       0, 0, "endrep",         VSIR_OP_ENDREP,       {2, 0}},
    {VKD3D_SM1_OP_IF,           0, 1, "if",             VSIR_OP_IF,           {2, 0}},
    {VKD3D_SM1_OP_IFC,          0, 2, "if",             VSIR_OP_IFC,          {2, 1}},
    {VKD3D_SM1_OP_ELSE,         0, 0, "else",           VSIR_OP_ELSE,         {2, 0}},
    {VKD3D_SM1_OP_ENDIF,        0, 0, "endif",          VSIR_OP_ENDIF,        {2, 0}},
    {VKD3D_SM1_OP_BREAK,        0, 0, "break",          VSIR_OP_BREAK,        {2, 1}},
    {VKD3D_SM1_OP_BREAKC,       0, 2, "break",          VSIR_OP_BREAKC,       {2, 1}},
    {VKD3D_SM1_OP_BREAKP,       0, 1, "break",          VSIR_OP_BREAKP,       {2, 1}},
    {VKD3D_SM1_OP_CALL,         0, 1, "call",           VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_CALLNZ,       0, 2, "callnz",         VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_LOOP,         0, 2, "loop",           VSIR_OP_LOOP,         {2, 0}},
    {VKD3D_SM1_OP_RET,          0, 0, "ret",            VSIR_OP_RET,          {2, 0}},
    {VKD3D_SM1_OP_ENDLOOP,      0, 0, "endloop",        VSIR_OP_ENDLOOP,      {2, 0}},
    {VKD3D_SM1_OP_LABEL,        0, 1, "label",          VSIR_OP_LABEL,        {2, 0}},

    {VKD3D_SM1_OP_SETP,         1, 2, "setp",           VSIR_OP_NOP,          {2, 1}},
    {VKD3D_SM1_OP_TEXLDL,       1, 2, "texldl",         VSIR_OP_TEXLDL,       {3, 0}},
    {0,                         0, 0, "???",            VSIR_OP_INVALID},
};

static const struct vkd3d_sm1_opcode_info ps_opcode_table[] =
{
    /* Arithmetic */
    {VKD3D_SM1_OP_NOP,          0, 0, "nop",            VSIR_OP_NOP},
    {VKD3D_SM1_OP_MOV,          1, 1, "mov",            VSIR_OP_MOV},
    {VKD3D_SM1_OP_ADD,          1, 2, "add",            VSIR_OP_ADD},
    {VKD3D_SM1_OP_SUB,          1, 2, "sub",            VSIR_OP_SUB},
    {VKD3D_SM1_OP_MAD,          1, 3, "mad",            VSIR_OP_MAD},
    {VKD3D_SM1_OP_MUL,          1, 2, "mul",            VSIR_OP_MUL},
    {VKD3D_SM1_OP_RCP,          1, 1, "rcp",            VSIR_OP_RCP,          {2, 0}},
    {VKD3D_SM1_OP_RSQ,          1, 1, "rsq",            VSIR_OP_RSQ,          {2, 0}},
    {VKD3D_SM1_OP_DP3,          1, 2, "dp3",            VSIR_OP_DP3},
    {VKD3D_SM1_OP_DP4,          1, 2, "dp4",            VSIR_OP_DP4,          {1, 2}},
    {VKD3D_SM1_OP_MIN,          1, 2, "min",            VSIR_OP_MIN,          {2, 0}},
    {VKD3D_SM1_OP_MAX,          1, 2, "max",            VSIR_OP_MAX,          {2, 0}},
    {VKD3D_SM1_OP_ABS,          1, 1, "abs",            VSIR_OP_ABS,          {2, 0}},
    {VKD3D_SM1_OP_EXP,          1, 1, "exp",            VSIR_OP_EXP,          {2, 0}},
    {VKD3D_SM1_OP_LOG,          1, 1, "log",            VSIR_OP_LOG,          {2, 0}},
    {VKD3D_SM1_OP_LRP,          1, 3, "lrp",            VSIR_OP_NOP},
    {VKD3D_SM1_OP_FRC,          1, 1, "frc",            VSIR_OP_FRC,          {2, 0}},
    {VKD3D_SM1_OP_CND,          1, 3, "cnd",            VSIR_OP_NOP,          {1, 0}, {1, 4}},
    {VKD3D_SM1_OP_CMP,          1, 3, "cmp",            VSIR_OP_CMP,          {1, 2}},
    {VKD3D_SM1_OP_POW,          1, 2, "pow",            VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_CRS,          1, 2, "crs",            VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_NRM,          1, 1, "nrm",            VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_SINCOS,       1, 3, "sincos",         VSIR_OP_SINCOS,       {2, 0}, {2, 1}},
    {VKD3D_SM1_OP_SINCOS,       1, 1, "sincos",         VSIR_OP_SINCOS,       {3, 0}},
    {VKD3D_SM1_OP_DP2ADD,       1, 3, "dp2add",         VSIR_OP_DP2ADD,       {2, 0}},
    /* Matrix */
    {VKD3D_SM1_OP_M4x4,         1, 2, "m4x4",           VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_M4x3,         1, 2, "m4x3",           VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_M3x4,         1, 2, "m3x4",           VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_M3x3,         1, 2, "m3x3",           VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_M3x2,         1, 2, "m3x2",           VSIR_OP_NOP,          {2, 0}},
    /* Declarations */
    {VKD3D_SM1_OP_DCL,          1, 0, "dcl",            VSIR_OP_DCL,          {2, 0}},
    /* Constant definitions */
    {VKD3D_SM1_OP_DEF,          1, 0, "def",            VSIR_OP_NOP},
    {VKD3D_SM1_OP_DEFB,         1, 0, "defb",           VSIR_OP_NOP,          {2, 0}},
    {VKD3D_SM1_OP_DEFI,         1, 0, "defi",           VSIR_OP_NOP,          {2, 1}},
    /* Control flow */
    {VKD3D_SM1_OP_REP,          0, 1, "rep",            VSIR_OP_REP,          {2, 1}},
    {VKD3D_SM1_OP_ENDREP,       0, 0, "endrep",         VSIR_OP_ENDREP,       {2, 1}},
    {VKD3D_SM1_OP_IF,           0, 1, "if",             VSIR_OP_IF,           {2, 1}},
    {VKD3D_SM1_OP_IFC,          0, 2, "if",             VSIR_OP_IFC,          {2, 1}},
    {VKD3D_SM1_OP_ELSE,         0, 0, "else",           VSIR_OP_ELSE,         {2, 1}},
    {VKD3D_SM1_OP_ENDIF,        0, 0, "endif",          VSIR_OP_ENDIF,        {2, 1}},
    {VKD3D_SM1_OP_BREAK,        0, 0, "break",          VSIR_OP_BREAK,        {2, 1}},
    {VKD3D_SM1_OP_BREAKC,       0, 2, "break",          VSIR_OP_BREAKC,       {2, 1}},
    {VKD3D_SM1_OP_BREAKP,       0, 1, "break",          VSIR_OP_BREAKP,       {2, 1}},
    {VKD3D_SM1_OP_CALL,         0, 1, "call",           VSIR_OP_NOP,          {2, 1}},
    {VKD3D_SM1_OP_CALLNZ,       0, 2, "callnz",         VSIR_OP_NOP,          {2, 1}},
    {VKD3D_SM1_OP_LOOP,         0, 2, "loop",           VSIR_OP_LOOP,         {3, 0}},
    {VKD3D_SM1_OP_RET,          0, 0, "ret",            VSIR_OP_RET,          {2, 1}},
    {VKD3D_SM1_OP_ENDLOOP,      0, 0, "endloop",        VSIR_OP_ENDLOOP,      {3, 0}},
    {VKD3D_SM1_OP_LABEL,        0, 1, "label",          VSIR_OP_LABEL,        {2, 1}},
    /* Texture */
    {VKD3D_SM1_OP_TEXCOORD,     1, 0, "texcoord",       VSIR_OP_NOP,          {0, 0}, {1, 3}},
    {VKD3D_SM1_OP_TEXCOORD,     1, 1, "texcrd",         VSIR_OP_NOP,          {1, 4}, {1, 4}},
    {VKD3D_SM1_OP_TEXKILL,      1, 0, "texkill",        VSIR_OP_TEXKILL,      {1, 0}},
    {VKD3D_SM1_OP_TEX,          1, 0, "tex",            VSIR_OP_TEX,          {0, 0}, {1, 3}},
    {VKD3D_SM1_OP_TEX,          1, 1, "texld",          VSIR_OP_TEXLD,        {1, 4}, {1, 4}},
    {VKD3D_SM1_OP_TEX,          1, 2, "texld",          VSIR_OP_TEXLD,        {2, 0}},
    {VKD3D_SM1_OP_TEXBEM,       1, 1, "texbem",         VSIR_OP_NOP,          {0, 0}, {1, 3}},
    {VKD3D_SM1_OP_TEXBEML,      1, 1, "texbeml",        VSIR_OP_NOP,          {1, 0}, {1, 3}},
    {VKD3D_SM1_OP_TEXREG2AR,    1, 1, "texreg2ar",      VSIR_OP_TEXREG2AR,    {1, 0}, {1, 3}},
    {VKD3D_SM1_OP_TEXREG2GB,    1, 1, "texreg2gb",      VSIR_OP_TEXREG2GB,    {1, 0}, {1, 3}},
    {VKD3D_SM1_OP_TEXREG2RGB,   1, 1, "texreg2rgb",     VSIR_OP_TEXREG2RGB,   {1, 2}, {1, 3}},
    {VKD3D_SM1_OP_TEXM3x2PAD,   1, 1, "texm3x2pad",     VSIR_OP_NOP,          {1, 0}, {1, 3}},
    {VKD3D_SM1_OP_TEXM3x2TEX,   1, 1, "texm3x2tex",     VSIR_OP_NOP,          {1, 0}, {1, 3}},
    {VKD3D_SM1_OP_TEXM3x3PAD,   1, 1, "texm3x3pad",     VSIR_OP_NOP,          {1, 0}, {1, 3}},
    {VKD3D_SM1_OP_TEXM3x3DIFF,  1, 1, "texm3x3diff",    VSIR_OP_NOP,          {0, 0}, {0, 0}},
    {VKD3D_SM1_OP_TEXM3x3SPEC,  1, 2, "texm3x3spec",    VSIR_OP_TEXM3x3SPEC,  {1, 0}, {1, 3}},
    {VKD3D_SM1_OP_TEXM3x3VSPEC, 1, 1, "texm3x3vspec",   VSIR_OP_TEXM3x3VSPEC, {1, 0}, {1, 3}},
    {VKD3D_SM1_OP_TEXM3x3TEX,   1, 1, "texm3x3tex",     VSIR_OP_NOP,          {1, 0}, {1, 3}},
    {VKD3D_SM1_OP_TEXDP3TEX,    1, 1, "texdp3tex",      VSIR_OP_TEXDP3TEX,    {1, 2}, {1, 3}},
    {VKD3D_SM1_OP_TEXM3x2DEPTH, 1, 1, "texm3x2depth",   VSIR_OP_NOP,          {1, 3}, {1, 3}},
    {VKD3D_SM1_OP_TEXDP3,       1, 1, "texdp3",         VSIR_OP_NOP,          {1, 2}, {1, 3}},
    {VKD3D_SM1_OP_TEXM3x3,      1, 1, "texm3x3",        VSIR_OP_NOP,          {1, 2}, {1, 3}},
    {VKD3D_SM1_OP_TEXDEPTH,     1, 0, "texdepth",       VSIR_OP_NOP,          {1, 4}, {1, 4}},
    {VKD3D_SM1_OP_BEM,          1, 2, "bem",            VSIR_OP_NOP,          {1, 4}, {1, 4}},
    {VKD3D_SM1_OP_DSX,          1, 1, "dsx",            VSIR_OP_DSX,          {2, 1}},
    {VKD3D_SM1_OP_DSY,          1, 1, "dsy",            VSIR_OP_DSY,          {2, 1}},
    {VKD3D_SM1_OP_TEXLDD,       1, 4, "texldd",         VSIR_OP_TEXLDD,       {2, 1}},
    {VKD3D_SM1_OP_SETP,         1, 2, "setp",           VSIR_OP_NOP,          {2, 1}},
    {VKD3D_SM1_OP_TEXLDL,       1, 2, "texldl",         VSIR_OP_TEXLDL,       {3, 0}},
    {VKD3D_SM1_OP_PHASE,        0, 0, "phase",          VSIR_OP_NOP,          {1, 4}, {1, 4}},
    {0,                         0, 0, "???",            VSIR_OP_INVALID},
};

static const struct
{
    enum vkd3d_sm1_register_type d3dbc_type;
    enum d3dbc_register_type vsir_type;
}
register_types[] =
{
    {VKD3D_SM1_REG_TEMP,        D3DBC_REGISTER_TEMP},
    {VKD3D_SM1_REG_INPUT,       D3DBC_REGISTER_INPUT},
    {VKD3D_SM1_REG_CONST,       D3DBC_REGISTER_CONST},
    {VKD3D_SM1_REG_ADDR,        D3DBC_REGISTER_ADDR},
    {VKD3D_SM1_REG_TEXTURE,     D3DBC_REGISTER_TEXTURE},
    {VKD3D_SM1_REG_RASTOUT,     D3DBC_REGISTER_RASTOUT},
    {VKD3D_SM1_REG_ATTROUT,     D3DBC_REGISTER_ATTROUT},
    {VKD3D_SM1_REG_OUTPUT,      D3DBC_REGISTER_OUTPUT},
    {VKD3D_SM1_REG_TEXCRDOUT,   D3DBC_REGISTER_TEXCRDOUT},
    {VKD3D_SM1_REG_CONSTINT,    D3DBC_REGISTER_CONSTINT},
    {VKD3D_SM1_REG_COLOROUT,    D3DBC_REGISTER_COLOROUT},
    {VKD3D_SM1_REG_DEPTHOUT,    D3DBC_REGISTER_DEPTHOUT},
    {VKD3D_SM1_REG_SAMPLER,     D3DBC_REGISTER_SAMPLER},
    {VKD3D_SM1_REG_CONSTBOOL,   D3DBC_REGISTER_CONSTBOOL},
    {VKD3D_SM1_REG_LOOP,        D3DBC_REGISTER_LOOP},
    {VKD3D_SM1_REG_MISCTYPE,    D3DBC_REGISTER_MISCTYPE},
    {VKD3D_SM1_REG_LABEL,       D3DBC_REGISTER_LABEL},
    {VKD3D_SM1_REG_PREDICATE,   D3DBC_REGISTER_PREDICATE},
};

static const enum vkd3d_shader_resource_type resource_type_table[] =
{
    /* VKD3D_SM1_RESOURCE_UNKNOWN */      VKD3D_SHADER_RESOURCE_NONE,
    /* VKD3D_SM1_RESOURCE_TEXTURE_1D */   VKD3D_SHADER_RESOURCE_TEXTURE_1D,
    /* VKD3D_SM1_RESOURCE_TEXTURE_2D */   VKD3D_SHADER_RESOURCE_TEXTURE_2D,
    /* VKD3D_SM1_RESOURCE_TEXTURE_CUBE */ VKD3D_SHADER_RESOURCE_TEXTURE_CUBE,
    /* VKD3D_SM1_RESOURCE_TEXTURE_3D */   VKD3D_SHADER_RESOURCE_TEXTURE_3D,
};

static uint32_t read_u32(const uint32_t **ptr)
{
    return *(*ptr)++;
}

static bool has_relative_address(uint32_t param)
{
    enum vkd3d_sm1_address_mode_type address_mode;

    address_mode = (param & VKD3D_SM1_ADDRESS_MODE_MASK) >> VKD3D_SM1_ADDRESS_MODE_SHIFT;

    return address_mode == VKD3D_SM1_ADDRESS_MODE_RELATIVE;
}

static const struct vkd3d_sm1_opcode_info *shader_sm1_get_opcode_info(
        const struct vkd3d_shader_sm1_parser *sm1, enum vkd3d_sm1_opcode opcode)
{
    const struct vkd3d_shader_version *version = &sm1->version;
    const struct vkd3d_sm1_opcode_info *info;
    unsigned int i = 0;

    for (;;)
    {
        info = &sm1->opcode_table[i++];
        if (info->vsir_opcode == VSIR_OP_INVALID)
            return NULL;

        if (opcode == info->sm1_opcode
                && vkd3d_shader_ver_ge(version, info->min_version.major, info->min_version.minor)
                && (vkd3d_shader_ver_le(version, info->max_version.major, info->max_version.minor)
                        || !info->max_version.major))
            return info;
    }
}

static unsigned int shader_sm1_get_swizzle_component(uint32_t swizzle, unsigned int idx)
{
    return (swizzle & VKD3D_SM1_SWIZZLE_COMPONENT_MASK(idx)) >> VKD3D_SM1_SWIZZLE_COMPONENT_SHIFT(idx);
}

static uint32_t swizzle_from_sm1(uint32_t swizzle)
{
    return vkd3d_shader_create_swizzle(shader_sm1_get_swizzle_component(swizzle, 0),
            shader_sm1_get_swizzle_component(swizzle, 1),
            shader_sm1_get_swizzle_component(swizzle, 2),
            shader_sm1_get_swizzle_component(swizzle, 3));
}

/* D3DBC doesn't have the concept of index count. All registers implicitly have
 * exactly one index. However for some register types the index doesn't make
 * sense, so we remove it. */
static unsigned int idx_count_from_reg_type(enum d3dbc_register_type reg_type)
{
    switch (reg_type)
    {
        case D3DBC_REGISTER_DEPTHOUT:
        case D3DBC_REGISTER_ADDR:
            return 0;

        default:
            return 1;
    }
}

static enum d3dbc_register_type parse_register_type(
        struct vkd3d_shader_sm1_parser *sm1, uint32_t param, unsigned int *index_offset)
{
    enum vkd3d_sm1_register_type d3dbc_type = ((param & VKD3D_SM1_REGISTER_TYPE_MASK) >> VKD3D_SM1_REGISTER_TYPE_SHIFT)
            | ((param & VKD3D_SM1_REGISTER_TYPE_MASK2) >> VKD3D_SM1_REGISTER_TYPE_SHIFT2);

    static const enum d3dbc_register_type types[] =
    {
        [VKD3D_SM1_REG_TEMP]        = D3DBC_REGISTER_TEMP,
        [VKD3D_SM1_REG_INPUT]       = D3DBC_REGISTER_INPUT,
        [VKD3D_SM1_REG_CONST]       = D3DBC_REGISTER_CONST,
        [VKD3D_SM1_REG_RASTOUT]     = D3DBC_REGISTER_RASTOUT,
        [VKD3D_SM1_REG_ATTROUT]     = D3DBC_REGISTER_ATTROUT,
        [VKD3D_SM1_REG_CONSTINT]    = D3DBC_REGISTER_CONSTINT,
        [VKD3D_SM1_REG_COLOROUT]    = D3DBC_REGISTER_COLOROUT,
        [VKD3D_SM1_REG_DEPTHOUT]    = D3DBC_REGISTER_DEPTHOUT,
        [VKD3D_SM1_REG_SAMPLER]     = D3DBC_REGISTER_SAMPLER,
        [VKD3D_SM1_REG_CONSTBOOL]   = D3DBC_REGISTER_CONSTBOOL,
        [VKD3D_SM1_REG_LOOP]        = D3DBC_REGISTER_LOOP,
        [VKD3D_SM1_REG_MISCTYPE]    = D3DBC_REGISTER_MISCTYPE,
        [VKD3D_SM1_REG_LABEL]       = D3DBC_REGISTER_LABEL,
        [VKD3D_SM1_REG_PREDICATE]   = D3DBC_REGISTER_PREDICATE,
    };

    *index_offset = 0;

    if (d3dbc_type == VKD3D_SM1_REG_CONST2)
    {
        *index_offset = 2048;
        return D3DBC_REGISTER_CONST;
    }

    if (d3dbc_type == VKD3D_SM1_REG_CONST3)
    {
        *index_offset = 4096;
        return D3DBC_REGISTER_CONST;
    }

    if (d3dbc_type == VKD3D_SM1_REG_CONST4)
    {
        *index_offset = 6144;
        return D3DBC_REGISTER_CONST;
    }

    if (d3dbc_type == VKD3D_SM1_REG_ADDR)
        return (sm1->version.type == VKD3D_SHADER_TYPE_PIXEL) ? D3DBC_REGISTER_TEXTURE : D3DBC_REGISTER_ADDR;
    if (d3dbc_type == VKD3D_SM1_REG_TEXCRDOUT)
        return vkd3d_shader_ver_ge(&sm1->version, 3, 0) ? D3DBC_REGISTER_OUTPUT : D3DBC_REGISTER_TEXCRDOUT;

    if ((uint32_t)d3dbc_type < ARRAY_SIZE(types))
        return types[d3dbc_type];
    vkd3d_shader_parser_warning(&sm1->p, VKD3D_SHADER_WARNING_D3DBC_INVALID_REGISTER_TYPE,
            "Invalid register type %#x.", d3dbc_type);
    return D3DBC_REGISTER_INVALID;
}

static void d3dbc_parse_operand(struct vkd3d_shader_sm1_parser *parser,
        struct d3dbc_operand *operand, uint32_t token)
{
    unsigned int index_offset;

    operand->type = parse_register_type(parser, token, &index_offset);
    operand->index = index_offset + (token & VKD3D_SM1_REGISTER_NUMBER_MASK);
    operand->has_rel_addr = has_relative_address(token);
}

static void d3dbc_parse_src_operand(struct vkd3d_shader_sm1_parser *parser,
        struct d3dbc_src_operand *src, uint32_t token)
{
    d3dbc_parse_operand(parser, &src->o, token);
    src->swizzle = swizzle_from_sm1((token & VKD3D_SM1_SWIZZLE_MASK) >> VKD3D_SM1_SWIZZLE_SHIFT);
    src->modifiers = (token & VKD3D_SM1_SRC_MODIFIER_MASK) >> VKD3D_SM1_SRC_MODIFIER_SHIFT;
    if (src->modifiers >= VKD3DSPSM_COUNT)
        vkd3d_shader_parser_warning(&parser->p, VKD3D_SHADER_WARNING_D3DBC_INVALID_SOURCE_MODIFIER,
                "Invalid source modifier %#x.", src->modifiers);
}

static void d3dbc_parse_dst_operand(struct vkd3d_shader_sm1_parser *parser,
        struct d3dbc_dst_operand *dst, uint32_t token)
{
    d3dbc_parse_operand(parser, &dst->o, token);
    dst->write_mask = (token & VKD3D_SM1_WRITEMASK_MASK) >> VKD3D_SM1_WRITEMASK_SHIFT;
    dst->modifiers = (token & VKD3D_SM1_DST_MODIFIER_MASK) >> VKD3D_SM1_DST_MODIFIER_SHIFT;
    dst->shift = (token & VKD3D_SM1_DSTSHIFT_MASK) >> VKD3D_SM1_DSTSHIFT_SHIFT;
    if (dst->modifiers & ~VKD3DSPDM_MASK)
        vkd3d_shader_parser_warning(&parser->p, VKD3D_SHADER_WARNING_D3DBC_INVALID_DESTINATION_MODIFIER,
                "Invalid destination modifiers %#x.", dst->modifiers);
}

static struct vsir_signature_element *find_signature_element(const struct vsir_signature *signature,
        const char *semantic_name, unsigned int semantic_index)
{
    struct vsir_signature_element *e = signature->elements;
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        if (!ascii_strcasecmp(e[i].semantic_name, semantic_name)
                && e[i].semantic_index == semantic_index)
            return &e[i];
    }

    return NULL;
}

static struct vsir_signature_element *find_signature_element_by_register_index(
        const struct vsir_signature *signature, unsigned int register_index)
{
    struct vsir_signature_element *e = signature->elements;
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        if (e[i].register_index == register_index)
            return &e[i];
    }

    return NULL;
}

/* Add missing bits to a mask to make it contiguous. */
static unsigned int make_mask_contiguous(unsigned int mask)
{
    static const unsigned int table[] =
    {
        0x0, 0x1, 0x2, 0x3,
        0x4, 0x7, 0x6, 0x7,
        0x8, 0xf, 0xe, 0xf,
        0xc, 0xf, 0xe, 0xf,
    };

    VKD3D_ASSERT(mask < ARRAY_SIZE(table));
    return table[mask];
}

static bool add_signature_element(struct vkd3d_shader_sm1_parser *sm1, bool output,
        const char *name, unsigned int index, enum vkd3d_shader_sysval_semantic sysval,
        unsigned int register_index, bool is_dcl, unsigned int mask, uint32_t dst_modifiers)
{
    struct vsir_program *program = sm1->program;
    struct vsir_signature_element *element;
    struct vsir_signature *signature;

    if (output)
        signature = &program->output_signature;
    else
        signature = &program->input_signature;

    if ((element = find_signature_element(signature, name, index)))
    {
        element->mask = make_mask_contiguous(element->mask | mask);
        if (!is_dcl)
            element->used_mask |= mask;
        return true;
    }

    if (!vkd3d_array_reserve((void **)&signature->elements, &signature->elements_capacity,
            signature->element_count + 1, sizeof(*signature->elements)))
        return false;
    element = &signature->elements[signature->element_count++];

    memset(element, 0, sizeof(*element));
    if (!(element->semantic_name = vkd3d_strdup(name)))
        return false;
    element->semantic_index = index;
    element->sysval_semantic = sysval;
    element->component_type = VKD3D_SHADER_COMPONENT_FLOAT;
    element->register_index = register_index;
    element->target_location = register_index;
    element->register_count = 1;
    element->mask = make_mask_contiguous(mask);
    element->used_mask = is_dcl ? 0 : mask;
    if (program->shader_version.type == VKD3D_SHADER_TYPE_PIXEL && !output)
        element->interpolation_mode = (dst_modifiers & VKD3DSPDM_MSAMPCENTROID)
                ? VKD3DSIM_LINEAR_CENTROID : VKD3DSIM_LINEAR;

    return true;
}

static void add_signature_mask(struct vkd3d_shader_sm1_parser *sm1, bool output,
        unsigned int register_index, unsigned int mask)
{
    struct vsir_program *program = sm1->program;
    struct vsir_signature_element *element;
    struct vsir_signature *signature;

    if (output)
        signature = &program->output_signature;
    else
        signature = &program->input_signature;

    if (!(element = find_signature_element_by_register_index(signature, register_index)))
    {
        vkd3d_shader_parser_warning(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNDECLARED_SEMANTIC,
                "%s register %u was used without being declared.", output ? "Output" : "Input", register_index);
        return;
    }

    /* Normally VSIR mandates that the register mask is a subset of the usage
     * mask, and the usage mask is a subset of the signature mask. This is
     * doesn't always happen with SM1-3 registers, because of the limited
     * flexibility with expressing swizzles.
     *
     * For example it's easy to find shaders like this:
     *   ps_3_0
     *   [...]
     *   dcl_texcoord0 v0
     *   [...]
     *   texld r2.xyzw, v0.xyzw, s1.xyzw
     *   [...]
     *
     * The dcl_textcoord0 instruction secretly has a .xy mask, which is used to
     * compute the signature mask, but the texld instruction apparently uses all
     * the components. Of course the last two components are ignored, but
     * formally they seem to be used. So we end up with a signature element with
     * mask .xy and usage mask .xyzw.
     *
     * In order to avoid this problem, when generating VSIR code with SM4
     * normalisation level we remove the unused components in the write mask. We
     * don't do that when targetting the SM1 normalisation level (i.e., when
     * disassembling) so as to generate the same disassembly code as native. */
    element->used_mask |= mask;
    if (program->normalisation_level >= VSIR_NORMALISED_SM4)
        element->used_mask &= element->mask;
}

static bool add_signature_element_from_operand(struct vkd3d_shader_sm1_parser *sm1,
        const struct d3dbc_operand *reg, bool is_dcl, uint32_t mask, uint32_t dst_modifiers)
{
    const struct vkd3d_shader_version *version = &sm1->program->shader_version;
    unsigned int register_index = reg->index;
    struct vsir_signature_element *e;

    switch (reg->type)
    {
        case D3DBC_REGISTER_TEMP:
            if (version->type == VKD3D_SHADER_TYPE_PIXEL && version->major == 1 && !register_index)
                return add_signature_element(sm1, true, "COLOR", 0, VKD3D_SHADER_SV_TARGET,
                        0, is_dcl, mask, dst_modifiers);
            return true;

        case D3DBC_REGISTER_INPUT:
            /* For vertex shaders or sm3 pixel shaders, we should have already
             * had a DCL instruction. Otherwise, this is a colour input. */
            if (version->type == VKD3D_SHADER_TYPE_PIXEL && version->major < 3)
                return add_signature_element(sm1, false, "COLOR", register_index,
                        VKD3D_SHADER_SV_NONE, SM1_COLOR_REGISTER_OFFSET + register_index, is_dcl, mask, dst_modifiers);

            if (reg->has_rel_addr)
            {
                WARN("Indirect addressing detected, adding used_mask %#x to all input elements.\n", mask);
                for (unsigned int i = 0; i < sm1->program->input_signature.element_count; ++i)
                {
                    e = &sm1->program->input_signature.elements[i];
                    e->used_mask |= mask & e->mask;
                }
                return true;
            }

            add_signature_mask(sm1, false, register_index, mask);
            return true;

        case D3DBC_REGISTER_TEXTURE:
            return add_signature_element(sm1, false, "TEXCOORD", register_index,
                    VKD3D_SHADER_SV_NONE, register_index, is_dcl, mask, dst_modifiers);

        case D3DBC_REGISTER_TEXCRDOUT:
            return add_signature_element(sm1, true, "TEXCOORD", register_index,
                    VKD3D_SHADER_SV_NONE, register_index, is_dcl, mask, dst_modifiers);

        case D3DBC_REGISTER_OUTPUT:
            if (version->type == VKD3D_SHADER_TYPE_VERTEX)
            {
                add_signature_mask(sm1, true, register_index, mask);
                return true;
            }
            /* fall through */

        case D3DBC_REGISTER_ATTROUT:
            return add_signature_element(sm1, true, "COLOR", register_index,
                    VKD3D_SHADER_SV_NONE, SM1_COLOR_REGISTER_OFFSET + register_index, is_dcl, mask, dst_modifiers);

        case D3DBC_REGISTER_COLOROUT:
            return add_signature_element(sm1, true, "COLOR", register_index,
                    VKD3D_SHADER_SV_TARGET, register_index, is_dcl, mask, dst_modifiers);

        case D3DBC_REGISTER_DEPTHOUT:
            return add_signature_element(sm1, true, "DEPTH", 0,
                    VKD3D_SHADER_SV_DEPTH, register_index, is_dcl, 0x1, dst_modifiers);

        case D3DBC_REGISTER_RASTOUT:
            switch (register_index)
            {
                case 0:
                    return add_signature_element(sm1, true, "POSITION", 0, VKD3D_SHADER_SV_POSITION,
                            SM1_RASTOUT_REGISTER_OFFSET + register_index, is_dcl, mask, dst_modifiers);

                case 1:
                    return add_signature_element(sm1, true, "FOG", 0, VKD3D_SHADER_SV_NONE,
                            SM1_RASTOUT_REGISTER_OFFSET + register_index, is_dcl, 0x1, dst_modifiers);

                case 2:
                    return add_signature_element(sm1, true, "PSIZE", 0, VKD3D_SHADER_SV_NONE,
                            SM1_RASTOUT_REGISTER_OFFSET + register_index, is_dcl, 0x1, dst_modifiers);

                default:
                    vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_REGISTER_INDEX,
                            "Invalid rasterizer output index %u.", register_index);
                    return true;
            }

        case D3DBC_REGISTER_MISCTYPE:
            switch (register_index)
            {
                case 0:
                    return add_signature_element(sm1, false, "VPOS", 0,
                            VKD3D_SHADER_SV_POSITION, register_index, is_dcl, mask, dst_modifiers);

                case 1:
                    return add_signature_element(sm1, false, "VFACE", 0,
                            VKD3D_SHADER_SV_IS_FRONT_FACE, register_index, is_dcl, 0x1, dst_modifiers);

                default:
                    vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_REGISTER_INDEX,
                            "Invalid miscellaneous fragment input index %u.", register_index);
                    return true;
            }

        default:
            return true;
    }
}

static bool add_signature_element_from_semantic(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *ins)
{
    const struct vkd3d_shader_version *version = &parser->program->shader_version;
    enum vkd3d_shader_sysval_semantic sysval = VKD3D_SHADER_SV_NONE;
    const struct d3dbc_operand *operand = &ins->dst[0].o;
    uint32_t modifiers = ins->dst[0].modifiers;
    uint32_t mask = ins->dst[0].write_mask;
    bool output;

    static const char sm1_semantic_names[][13] =
    {
        [VKD3D_DECL_USAGE_POSITION     ] = "POSITION",
        [VKD3D_DECL_USAGE_BLEND_WEIGHT ] = "BLENDWEIGHT",
        [VKD3D_DECL_USAGE_BLEND_INDICES] = "BLENDINDICES",
        [VKD3D_DECL_USAGE_NORMAL       ] = "NORMAL",
        [VKD3D_DECL_USAGE_PSIZE        ] = "PSIZE",
        [VKD3D_DECL_USAGE_TEXCOORD     ] = "TEXCOORD",
        [VKD3D_DECL_USAGE_TANGENT      ] = "TANGENT",
        [VKD3D_DECL_USAGE_BINORMAL     ] = "BINORMAL",
        [VKD3D_DECL_USAGE_TESS_FACTOR  ] = "TESSFACTOR",
        [VKD3D_DECL_USAGE_POSITIONT    ] = "POSITIONT",
        [VKD3D_DECL_USAGE_COLOR        ] = "COLOR",
        [VKD3D_DECL_USAGE_FOG          ] = "FOG",
        [VKD3D_DECL_USAGE_DEPTH        ] = "DEPTH",
        [VKD3D_DECL_USAGE_SAMPLE       ] = "SAMPLE",
    };

    if (operand->type == D3DBC_REGISTER_OUTPUT)
        output = true;
    else if (operand->type == D3DBC_REGISTER_INPUT || operand->type == D3DBC_REGISTER_TEXTURE)
        output = false;
    else /* vpos and vface don't have a semantic. */
        return add_signature_element_from_operand(parser, operand, true, mask, modifiers);

    /* sm2 pixel shaders use DCL but don't provide a semantic. */
    if (version->type == VKD3D_SHADER_TYPE_PIXEL && version->major == 2)
        return add_signature_element_from_operand(parser, operand, true, mask, modifiers);

    /* With the exception of vertex POSITION output, none of these are system
     * values. Pixel POSITION input is not equivalent to SV_Position; the closer
     * equivalent is VPOS, which is not declared as a semantic. */
    if (version->type == VKD3D_SHADER_TYPE_VERTEX
            && output && ins->semantic.usage == VKD3D_DECL_USAGE_POSITION)
        sysval = VKD3D_SHADER_SV_POSITION;

    return add_signature_element(parser, output, sm1_semantic_names[ins->semantic.usage],
            ins->semantic.usage_idx, sysval, operand->index, true, mask, modifiers);
}

static void record_constant_register(struct vkd3d_shader_sm1_parser *sm1,
        enum vkd3d_shader_d3dbc_constant_register set, uint32_t index, bool from_def)
{
    sm1->constants[set].count = max(sm1->constants[set].count, index + 1);
    if (from_def)
    {
        /* d3d shaders have a maximum of 8192 constants; we should not overrun
         * this array. */
        VKD3D_ASSERT((index / 32) <= ARRAY_SIZE(sm1->constants[set].def_mask));
        bitmap_set(sm1->constants[set].def_mask, index);
    }
}

static unsigned int d3dbc_get_src_register_count(enum vkd3d_sm1_opcode opcode, unsigned int src_idx)
{
    switch (opcode)
    {
        case VKD3D_SM1_OP_M3x2:
            return (src_idx == 1) ? 2 : 1;
        case VKD3D_SM1_OP_M3x3:
        case VKD3D_SM1_OP_M4x3:
            return (src_idx == 1) ? 3 : 1;
        case VKD3D_SM1_OP_M3x4:
        case VKD3D_SM1_OP_M4x4:
            return (src_idx == 1) ? 4 : 1;
        default:
            return 1;
    }
}

static void d3dbc_scan_operand(struct vkd3d_shader_sm1_parser *d3dbc,
        const struct d3dbc_operand *operand0, uint32_t mask, bool from_def, unsigned int count)
{
    struct vsir_program *program = d3dbc->program;
    struct d3dbc_operand operand = *operand0;

    for (unsigned int i = 0; i < count; ++i)
    {
        uint32_t register_index = operand.index;

        switch (operand.type)
        {
            case D3DBC_REGISTER_TEMP:
                program->temp_count = max(program->temp_count, register_index + 1);
                break;

            case D3DBC_REGISTER_CONST:
                record_constant_register(d3dbc, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER, register_index, from_def);
                break;

            case D3DBC_REGISTER_CONSTINT:
                record_constant_register(d3dbc, VKD3D_SHADER_D3DBC_INT_CONSTANT_REGISTER, register_index, from_def);
                break;

            case D3DBC_REGISTER_CONSTBOOL:
                record_constant_register(d3dbc, VKD3D_SHADER_D3DBC_BOOL_CONSTANT_REGISTER, register_index, from_def);
                break;

            case D3DBC_REGISTER_DEPTHOUT:
                bitmap_set(program->io_dcls, VSIR_REGISTER_DEPTHOUT);
                break;

            case D3DBC_REGISTER_RASTOUT:
                if (register_index == VSIR_RASTOUT_POINT_SIZE)
                    program->has_point_size = true;
                if (register_index == VSIR_RASTOUT_FOG)
                    program->has_fog = true;
                break;

            default:
                break;
        }

        add_signature_element_from_operand(d3dbc, &operand, false, mask, 0);

        ++operand.index;
    }
}

static void d3dbc_add_combined_sampler_descriptor(struct vkd3d_shader_sm1_parser *d3dbc,
        unsigned int sampler_idx, enum vkd3d_shader_resource_type resource_type)
{
    const struct vkd3d_shader_d3dbc_source_info *source_info = d3dbc->d3dbc_source_info;
    struct vsir_register_range range = {.first = sampler_idx, .last = sampler_idx};
    struct vsir_program *program = d3dbc->program;
    struct vsir_descriptor *d;

    if (!vsir_program_add_descriptor(program, VKD3D_SHADER_DESCRIPTOR_TYPE_SRV,
            sampler_idx, &range, resource_type, VSIR_DATA_F32))
        vkd3d_shader_parser_error(&d3dbc->p, VKD3D_SHADER_ERROR_D3DBC_OUT_OF_MEMORY,
                "Failed to create SRV descriptor for combined sampler %u.", sampler_idx);

    if (!(d = vsir_program_add_descriptor(program, VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER,
            sampler_idx, &range, VKD3D_SHADER_RESOURCE_NONE, VSIR_DATA_UNUSED)))
        vkd3d_shader_parser_error(&d3dbc->p, VKD3D_SHADER_ERROR_D3DBC_OUT_OF_MEMORY,
                "Failed to create sampler descriptor for combined sampler %u.", sampler_idx);
    else if (source_info && source_info->shadow_samplers & (1u << sampler_idx))
        d->flags |= VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_SAMPLER_COMPARISON_MODE;
}

/* Read a parameter token from the input stream, and possibly a relative
 * addressing token. */
static void shader_sm1_read_param(struct vkd3d_shader_sm1_parser *sm1,
        const uint32_t **ptr, uint32_t *token, uint32_t *addr_token)
{
    if (*ptr >= sm1->end)
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "Attempted to read a parameter token, but no more tokens are remaining.");
        sm1->abort = true;
        *token = 0;
        return;
    }
    *token = read_u32(ptr);
    if (!has_relative_address(*token))
        return;

    /* PS >= 3.0 have relative addressing (with token)
     * VS >= 2.0 have relative addressing (with token)
     * VS >= 1.0 < 2.0 have relative addressing (without token)
     * The version check below should work in general. */
    if (sm1->version.major < 2)
    {
        *addr_token = VKD3D_SM1_INSTRUCTION_PARAMETER
                | ((VKD3D_SM1_REG_ADDR << VKD3D_SM1_REGISTER_TYPE_SHIFT2) & VKD3D_SM1_REGISTER_TYPE_MASK2)
                | ((VKD3D_SM1_REG_ADDR << VKD3D_SM1_REGISTER_TYPE_SHIFT) & VKD3D_SM1_REGISTER_TYPE_MASK)
                | (VKD3D_SM1_SWIZZLE_DEFAULT << VKD3D_SM1_SWIZZLE_SHIFT);
        return;
    }

    if (*ptr >= sm1->end)
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "Attempted to read an indirect addressing token, but no more tokens are remaining.");
        sm1->abort = true;
        *addr_token = 0;
        return;
    }
    *addr_token = read_u32(ptr);
}

/* Skip the parameter tokens for an opcode. */
static void shader_sm1_skip_opcode(const struct vkd3d_shader_sm1_parser *sm1, const uint32_t **ptr,
        const struct vkd3d_sm1_opcode_info *opcode_info, uint32_t opcode_token)
{
    unsigned int length;

    /* Version 2.0+ shaders may contain address tokens, but fortunately they
     * have a useful length mask - use it here. Version 1.x shaders contain no
     * such tokens. */
    if (sm1->version.major >= 2)
    {
        length = (opcode_token & VKD3D_SM1_INSTRUCTION_LENGTH_MASK) >> VKD3D_SM1_INSTRUCTION_LENGTH_SHIFT;
        *ptr += length;
        return;
    }

    /* DCL instructions read an extra token for the semantic. */
    if (opcode_info->vsir_opcode == VSIR_OP_DCL)
        *ptr += 1;
    /* We don't count DEF instructions as having a source, but they do have
     * four tokens of float constants. */
    else if (opcode_info->sm1_opcode == VKD3D_SM1_OP_DEF)
        *ptr += 4;

    *ptr += (opcode_info->dst_count + opcode_info->src_count);
}

static void d3dbc_read_src_operand(struct vkd3d_shader_sm1_parser *parser,
        const uint32_t **ptr, struct d3dbc_src_operand *src, struct d3dbc_src_operand *rel_addr)
{
    uint32_t token, addr_token;

    shader_sm1_read_param(parser, ptr, &token, &addr_token);
    if (has_relative_address(token))
    {
        d3dbc_parse_src_operand(parser, rel_addr, addr_token);
        if (rel_addr->o.has_rel_addr)
        {
            vkd3d_shader_parser_error(&parser->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_INDIRECT_ADDRESS,
                    "An indirect address cannot itself have an indirect address.\n");
            rel_addr->o.has_rel_addr = false;
        }
    }
    d3dbc_parse_src_operand(parser, src, token);
}

static void d3dbc_read_dst_operand(struct vkd3d_shader_sm1_parser *parser,
        const uint32_t **ptr, struct d3dbc_dst_operand *dst, struct d3dbc_src_operand *rel_addr)
{
    uint32_t token, addr_token;

    shader_sm1_read_param(parser, ptr, &token, &addr_token);
    if (has_relative_address(token))
    {
        d3dbc_parse_src_operand(parser, rel_addr, addr_token);
        if (rel_addr->o.has_rel_addr)
        {
            vkd3d_shader_parser_error(&parser->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_INDIRECT_ADDRESS,
                    "An indirect address cannot itself have an indirect address.\n");
            rel_addr->o.has_rel_addr = false;
        }
    }
    d3dbc_parse_dst_operand(parser, dst, token);
}

static void shader_sm1_read_semantic(struct vkd3d_shader_sm1_parser *sm1,
        const uint32_t **ptr, struct d3dbc_semantic *semantic)
{
    uint32_t usage_token;

    if (*ptr >= sm1->end)
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "Attempted to read a declaration instruction, but not enough tokens are remaining.");
        sm1->abort = true;
        return;
    }

    usage_token = read_u32(ptr);

    semantic->usage = (usage_token & VKD3D_SM1_DCL_USAGE_MASK) >> VKD3D_SM1_DCL_USAGE_SHIFT;
    semantic->usage_idx = (usage_token & VKD3D_SM1_DCL_USAGE_INDEX_MASK) >> VKD3D_SM1_DCL_USAGE_INDEX_SHIFT;
    semantic->resource_type = (usage_token & VKD3D_SM1_RESOURCE_TYPE_MASK) >> VKD3D_SM1_RESOURCE_TYPE_SHIFT;

    if (semantic->usage >= VKD3D_DECL_USAGE_COUNT)
        vkd3d_shader_parser_warning(&sm1->p, VKD3D_SHADER_WARNING_D3DBC_INVALID_USAGE,
                "Invalid usage %#x.", semantic->usage);
    if (semantic->resource_type == VKD3D_SM1_RESOURCE_TEXTURE_1D || semantic->resource_type >= VKD3D_SM1_RESOURCE_COUNT)
        vkd3d_shader_parser_warning(&sm1->p, VKD3D_SHADER_WARNING_D3DBC_INVALID_RESOURCE_TYPE,
                "Invalid resource type %#x.", semantic->resource_type);
}

static void shader_sm1_read_immconst(struct vkd3d_shader_sm1_parser *sm1,
        const uint32_t **ptr, union d3dbc_immediate_constant *c, enum vkd3d_sm1_opcode opcode)
{
    unsigned int count = opcode == VKD3D_SM1_OP_DEFB ? 1 : 4;

    if (*ptr >= sm1->end || sm1->end - *ptr < count)
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "Attempted to read a constant definition, but not enough tokens are remaining. "
                "%zu token(s) available, %u required.", sm1->end - *ptr, count);
        sm1->abort = true;
        return;
    }

    memcpy(c, *ptr, count * sizeof(uint32_t));
    *ptr += count;
}

static void shader_sm1_read_comment(struct vkd3d_shader_sm1_parser *sm1)
{
    const uint32_t **ptr = &sm1->ptr;
    const char *comment;
    unsigned int size;
    size_t remaining;
    uint32_t token;

    if (*ptr >= sm1->end)
        return;

    remaining = sm1->end - *ptr;

    token = **ptr;
    while ((token & VKD3D_SM1_OPCODE_MASK) == VKD3D_SM1_OP_COMMENT)
    {
        size = (token & VKD3D_SM1_COMMENT_SIZE_MASK) >> VKD3D_SM1_COMMENT_SIZE_SHIFT;

        if (size > --remaining)
        {
            vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                    "Encountered a %u token comment, but only %zu token(s) is/are remaining.",
                    size, remaining);
            return;
        }

        comment = (const char *)++(*ptr);
        remaining -= size;
        *ptr += size;

        if (size > 1 && *(const uint32_t *)comment == TAG_TEXT)
        {
            const char *end = comment + size * sizeof(token);
            const char *p = comment + sizeof(token);
            const char *line;

            TRACE("// TEXT\n");
            for (line = p; line < end; line = p)
            {
                if (!(p = memchr(line, '\n', end - line)))
                    p = end;
                else
                    ++p;
                TRACE("// %s\n", debugstr_an(line, p - line));
            }
        }
        else if (size)
        {
            TRACE("// %s\n", debugstr_an(comment, size * sizeof(token)));
        }
        else
            break;

        if (!remaining)
            break;
        token = **ptr;
    }
}

static void d3dbc_update_descriptors(struct vkd3d_shader_sm1_parser *d3dbc, const struct d3dbc_instruction *ins)
{
    enum vkd3d_shader_resource_type type = VKD3D_SHADER_RESOURCE_TEXTURE_2D;
    const struct vkd3d_shader_d3dbc_source_info *source_info;
    unsigned int sampler_idx;

    switch (ins->opcode)
    {
        case VKD3D_SM1_OP_TEX:
        case VKD3D_SM1_OP_TEXBEM:
        case VKD3D_SM1_OP_TEXBEML:
        case VKD3D_SM1_OP_TEXDP3TEX:
        case VKD3D_SM1_OP_TEXM3x2TEX:
        case VKD3D_SM1_OP_TEXM3x3SPEC:
        case VKD3D_SM1_OP_TEXM3x3TEX:
        case VKD3D_SM1_OP_TEXM3x3VSPEC:
        case VKD3D_SM1_OP_TEXREG2AR:
        case VKD3D_SM1_OP_TEXREG2GB:
        case VKD3D_SM1_OP_TEXREG2RGB:
            sampler_idx = ins->dst[0].o.index;
            if ((d3dbc->texture_descriptors & (1u << sampler_idx)))
                break;

            if ((source_info = d3dbc->d3dbc_source_info)
                    && sampler_idx < ARRAY_SIZE(source_info->texture_dimensions))
                type = source_info->texture_dimensions[sampler_idx];
            d3dbc_add_combined_sampler_descriptor(d3dbc, sampler_idx, type);
            d3dbc->texture_descriptors |= (1u << sampler_idx);
            break;

        default:
            break;
    }
}

static void shader_sm1_validate_instruction(struct vkd3d_shader_sm1_parser *sm1, struct d3dbc_instruction *ins)
{
    if ((ins->opcode == VKD3D_SM1_OP_BREAKP || ins->opcode == VKD3D_SM1_OP_IF) && ins->flags)
    {
        vkd3d_shader_parser_warning(&sm1->p, VKD3D_SHADER_WARNING_D3DBC_IGNORED_INSTRUCTION_FLAGS,
                "Ignoring unexpected instruction flags %#x.", ins->flags);
        ins->flags = 0;
    }
}

static unsigned int mask_from_swizzle(uint32_t swizzle)
{
    return (1u << vsir_swizzle_get_component(swizzle, 0))
            | (1u << vsir_swizzle_get_component(swizzle, 1))
            | (1u << vsir_swizzle_get_component(swizzle, 2))
            | (1u << vsir_swizzle_get_component(swizzle, 3));
}

static bool d3dbc_parse_instruction(struct vkd3d_shader_sm1_parser *sm1,
        struct d3dbc_instruction *ins, const struct vkd3d_sm1_opcode_info **ret_opcode_info)
{
    const struct vkd3d_sm1_opcode_info *opcode_info;
    const uint32_t **ptr = &sm1->ptr;
    uint32_t opcode_token;
    const uint32_t *p;
    unsigned int i;

    shader_sm1_read_comment(sm1);

    if (*ptr >= sm1->end)
    {
        WARN("End of byte-code, failed to read opcode.\n");
        return false;
    }

    ++sm1->p.location.line;
    opcode_token = read_u32(ptr);

    ins->opcode = (opcode_token & VKD3D_SM1_OPCODE_MASK);

    if (!(opcode_info = shader_sm1_get_opcode_info(sm1, ins->opcode)))
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_OPCODE,
                "Invalid opcode %#x (token 0x%08x, shader version %u.%u).",
                opcode_token & VKD3D_SM1_OPCODE_MASK, opcode_token,
                sm1->version.major, sm1->version.minor);
        return false;
    }
    *ret_opcode_info = opcode_info;

    ins->flags = (opcode_token & VKD3D_SM1_INSTRUCTION_FLAGS_MASK) >> VKD3D_SM1_INSTRUCTION_FLAGS_SHIFT;
    ins->coissue = opcode_token & VKD3D_SM1_COISSUE;
    ins->predicated = (opcode_token & VKD3D_SM1_INSTRUCTION_PREDICATED);
    ins->dst_count = opcode_info->dst_count;
    ins->src_count = opcode_info->src_count;

    p = *ptr;
    shader_sm1_skip_opcode(sm1, ptr, opcode_info, opcode_token);
    if (*ptr > sm1->end)
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "The current instruction ends %zu token(s) past the end of the shader.",
                *ptr - sm1->end);
        return false;
    }

    if (ins->opcode == VKD3D_SM1_OP_DCL)
    {
        shader_sm1_read_semantic(sm1, &p, &ins->semantic);
        d3dbc_read_dst_operand(sm1, &p, &ins->dst[0], &ins->dst_rel_addr[0]);
    }
    else if (ins->opcode == VKD3D_SM1_OP_DEF
            || ins->opcode == VKD3D_SM1_OP_DEFI
            || ins->opcode == VKD3D_SM1_OP_DEFB)
    {
        d3dbc_read_dst_operand(sm1, &p, &ins->dst[0], &ins->dst_rel_addr[0]);
        shader_sm1_read_immconst(sm1, &p, &ins->immconst, ins->opcode);
    }
    else
    {
        /* Destination token */
        if (ins->dst_count)
            d3dbc_read_dst_operand(sm1, &p, &ins->dst[0], &ins->dst_rel_addr[0]);

        /* Predication token */
        if (ins->predicated)
        {
            struct d3dbc_src_operand rel_addr;

            d3dbc_read_src_operand(sm1, &p, &ins->predicate, &rel_addr);

            if (ins->predicate.o.has_rel_addr)
            {
                vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_INDIRECT_ADDRESS,
                        "Invalid indirect address on a predicate register.");
                ins->predicate.o.has_rel_addr = false;
            }
        }

        /* Other source tokens */
        for (i = 0; i < ins->src_count; ++i)
        {
            d3dbc_read_src_operand(sm1, &p, &ins->src[i], &ins->src_rel_addr[i]);
        }
    }

    if (sm1->abort)
    {
        sm1->abort = false;
        return false;
    }

    return true;
}

static void vsir_src_operand_from_d3dbc(struct vkd3d_shader_sm1_parser *parser, struct vsir_src_operand *vsir,
        const struct d3dbc_src_operand *d3dbc, const struct d3dbc_src_operand *rel_addr);

static void vsir_const_operand_from_d3dbc(struct vkd3d_shader_sm1_parser *parser, struct vsir_operand *vsir,
        const struct d3dbc_operand *d3dbc, const struct d3dbc_src_operand *rel_addr,
        enum vkd3d_shader_d3dbc_constant_register set, enum vsir_data_type type, enum vsir_dimension dimension)
{
    for (size_t i = 0; i < parser->def_count; ++i)
    {
        if (parser->defs[i].set == set && parser->defs[i].index == d3dbc->index)
        {
            vsir_operand_init(vsir, VSIR_REGISTER_IMMCONST, type, 0);
            vsir->dimension = dimension;
            for (unsigned int j = 0; j < 4; ++j)
                vsir->u.immconst_u32[j] = parser->defs[i].value.i32[j];
            return;
        }
    }

    vsir_operand_init(vsir, VSIR_REGISTER_CONSTBUFFER, type, 3);
    vsir->dimension = VSIR_DIMENSION_VEC4;
    vsir->idx[0].offset = set; /* register ID */
    vsir->idx[0].rel_addr = NULL;
    vsir->idx[1].offset = set; /* register index */
    vsir->idx[1].rel_addr = NULL;
    vsir->idx[2].offset = d3dbc->index; /* buffer index */
    vsir->idx[2].rel_addr = NULL;
    if (d3dbc->has_rel_addr)
    {
        if (!(vsir->idx[2].rel_addr = vsir_program_get_src_operands(parser->program, 1)))
        {
            vkd3d_shader_parser_error(&parser->p, VKD3D_SHADER_ERROR_D3DBC_OUT_OF_MEMORY, "Out of memory.");
            parser->abort = true;
            return;
        }

        vsir_src_operand_from_d3dbc(parser, vsir->idx[2].rel_addr, rel_addr, NULL);
    }
}

static void vsir_operand_from_d3dbc(struct vkd3d_shader_sm1_parser *parser, struct vsir_operand *vsir,
        const struct d3dbc_operand *d3dbc, const struct d3dbc_src_operand *rel_addr)
{
    unsigned int index_count;

    static const enum vsir_register_type types[] =
    {
        [D3DBC_REGISTER_INVALID]      = VSIR_REGISTER_INVALID,
        [D3DBC_REGISTER_TEMP]         = VSIR_REGISTER_TEMP,
        [D3DBC_REGISTER_INPUT]        = VSIR_REGISTER_INPUT,
        [D3DBC_REGISTER_ADDR]         = VSIR_REGISTER_ADDR,
        [D3DBC_REGISTER_TEXTURE]      = VSIR_REGISTER_TEXTURE,
        [D3DBC_REGISTER_RASTOUT]      = VSIR_REGISTER_RASTOUT,
        [D3DBC_REGISTER_ATTROUT]      = VSIR_REGISTER_ATTROUT,
        [D3DBC_REGISTER_TEXCRDOUT]    = VSIR_REGISTER_TEXCRDOUT,
        [D3DBC_REGISTER_OUTPUT]       = VSIR_REGISTER_OUTPUT,
        [D3DBC_REGISTER_COLOROUT]     = VSIR_REGISTER_COLOROUT,
        [D3DBC_REGISTER_DEPTHOUT]     = VSIR_REGISTER_DEPTHOUT,
        [D3DBC_REGISTER_SAMPLER]      = VSIR_REGISTER_COMBINED_SAMPLER,
        [D3DBC_REGISTER_LOOP]         = VSIR_REGISTER_LOOP,
        [D3DBC_REGISTER_MISCTYPE]     = VSIR_REGISTER_MISCTYPE,
        [D3DBC_REGISTER_LABEL]        = VSIR_REGISTER_LABEL,
        [D3DBC_REGISTER_PREDICATE]    = VSIR_REGISTER_PREDICATE,
        [D3DBC_REGISTER_SSA]          = VSIR_REGISTER_SSA,
    };

    switch (d3dbc->type)
    {
        case D3DBC_REGISTER_CONST:
            return vsir_const_operand_from_d3dbc(parser, vsir, d3dbc, rel_addr,
                    VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER, VSIR_DATA_F32, VSIR_DIMENSION_VEC4);

        case D3DBC_REGISTER_CONSTBOOL:
            return vsir_const_operand_from_d3dbc(parser, vsir, d3dbc, rel_addr,
                    VKD3D_SHADER_D3DBC_BOOL_CONSTANT_REGISTER, VSIR_DATA_BOOL, VSIR_DIMENSION_SCALAR);

        case D3DBC_REGISTER_CONSTINT:
            return vsir_const_operand_from_d3dbc(parser, vsir, d3dbc, rel_addr,
                    VKD3D_SHADER_D3DBC_INT_CONSTANT_REGISTER, VSIR_DATA_U32, VSIR_DIMENSION_VEC4);

        case D3DBC_REGISTER_LOOP:
            if (parser->aL_count)
            {
                vsir_operand_init(vsir, VSIR_REGISTER_TEMP, VSIR_DATA_U32, 1);
                vsir->idx[0].offset = parser->aL_ids[parser->aL_count - 1];
            }
            else
            {
                vkd3d_shader_parser_error(&parser->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_REGISTER_TYPE,
                        "aL cannot be used outside of a loop.");
                vsir_operand_init(vsir, VSIR_REGISTER_NULL, VSIR_DATA_UNUSED, 0);
            }
            return;

        default:
            break;
    }

    index_count = idx_count_from_reg_type(d3dbc->type);
    vsir_operand_init(vsir, types[d3dbc->type], VSIR_DATA_F32, index_count);
    if (index_count == 1)
    {
        vsir->idx[0].offset = d3dbc->index;
        if (d3dbc->has_rel_addr)
        {
            VKD3D_ASSERT(rel_addr);
            if (!(vsir->idx[0].rel_addr = vsir_program_get_src_operands(parser->program, 1)))
            {
                vkd3d_shader_parser_error(&parser->p, VKD3D_SHADER_ERROR_D3DBC_OUT_OF_MEMORY, "Out of memory.");
                parser->p.status = VKD3D_ERROR_OUT_OF_MEMORY;
                return;
            }

            vsir_src_operand_from_d3dbc(parser, vsir->idx[0].rel_addr, rel_addr, NULL);
        }
    }

    if (d3dbc->type == D3DBC_REGISTER_SAMPLER)
        vsir->dimension = VSIR_DIMENSION_NONE;
    else if (d3dbc->type == D3DBC_REGISTER_DEPTHOUT)
        vsir->dimension = VSIR_DIMENSION_SCALAR;
    else
        vsir->dimension = VSIR_DIMENSION_VEC4;
}

static void vsir_src_operand_from_d3dbc(struct vkd3d_shader_sm1_parser *parser, struct vsir_src_operand *vsir,
        const struct d3dbc_src_operand *d3dbc, const struct d3dbc_src_operand *rel_addr)
{
    vsir_operand_from_d3dbc(parser, &vsir->reg, &d3dbc->o, rel_addr);
    vsir->modifiers = VKD3DSPSM_NONE;
    if (vsir->reg.type == VSIR_REGISTER_IMMCONST)
    {
        if (vsir->reg.dimension == VSIR_DIMENSION_VEC4)
        {
            union vsir_immediate_constant c = vsir->reg.u;

            for (unsigned int j = 0; j < 4; ++j)
                vsir->reg.u.immconst_u32[j] = c.immconst_u32[vsir_swizzle_get_component(d3dbc->swizzle, j)];
            vsir->swizzle = VKD3D_SHADER_NO_SWIZZLE;
        }
        else
        {
            vsir->swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);
        }
    }
    else if (vsir->reg.dimension == VSIR_DIMENSION_VEC4 && d3dbc->o.type != D3DBC_REGISTER_CONSTBOOL)
    {
        vsir->swizzle = d3dbc->swizzle;
    }
    else
    {
        vsir->swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);
    }
}

/* This helper exists partially to make sure that IMMCONST is handled correctly.
 * It is illegal to put a swizzle on an IMMCONST. */
static void vsir_src_operand_from_d3dbc_swizzled(struct vkd3d_shader_sm1_parser *parser, struct vsir_src_operand *vsir,
        const struct d3dbc_src_operand *d3dbc, const struct d3dbc_src_operand *rel_addr, uint32_t swizzle)
{
    struct d3dbc_src_operand swizzled = *d3dbc;
    swizzled.swizzle = vsir_combine_swizzles(d3dbc->swizzle, swizzle);
    vsir_src_operand_from_d3dbc(parser, vsir, &swizzled, rel_addr);
}

static void vsir_dst_operand_from_d3dbc(struct vkd3d_shader_sm1_parser *parser, struct vsir_dst_operand *vsir,
        const struct d3dbc_dst_operand *d3dbc, const struct d3dbc_src_operand *rel_addr)
{
    vsir_operand_from_d3dbc(parser, &vsir->reg, &d3dbc->o, rel_addr);
    vsir->modifiers = VKD3DSPDM_NONE;
    if (vsir->reg.dimension == VSIR_DIMENSION_SCALAR)
        vsir->write_mask = VKD3DSP_WRITEMASK_0;
    else if (vsir->reg.dimension == VSIR_DIMENSION_VEC4)
        vsir->write_mask = d3dbc->write_mask;
    else
        vsir->write_mask = 0;
}

static struct vsir_instruction *d3dbc_append_instruction(struct vkd3d_shader_sm1_parser *parser,
        enum vsir_opcode opcode, unsigned int dst_count, unsigned int src_count)
{
    struct vsir_instruction *ins;

    if (!(ins = vsir_program_append(parser->program)))
    {
        parser->p.status = VKD3D_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    if (!vsir_instruction_init_with_params(parser->program, ins, &parser->p.location, opcode, dst_count, src_count))
    {
        parser->p.status = VKD3D_ERROR_OUT_OF_MEMORY;
        vsir_instruction_init(ins, &parser->p.location, VSIR_OP_NOP);
        return NULL;
    }
    return ins;
}

static struct vsir_instruction *generate_bump_coords(struct vkd3d_shader_sm1_parser *parser,
        unsigned int idx, const struct vsir_src_operand *coords,
        const struct d3dbc_src_operand *perturbation, const struct d3dbc_src_operand *perturbation_rel_addr)
{
    unsigned int temp_id = parser->program->ssa_count++;
    struct vsir_instruction *ins;

    /* mad    srTMP.xy, PERTURBATION.xx, BUMP_MATRIX#.xy, COORDS.xy
     * mad    _____.xy, PERTURBATION.yy, BUMP_MATRIX#.zw, srTMP.xy
     *
     * The destination is left blank to be set by the caller. */

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MAD, 1, 3)))
        return NULL;
    vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], temp_id);
    ins->dst[0].write_mask = 0x3;
    vsir_src_operand_from_d3dbc_swizzled(parser, &ins->src[0],
            perturbation, perturbation_rel_addr, VKD3D_SHADER_SWIZZLE(X, X, X, X));
    vsir_src_operand_init_parameter_vec4(&ins->src[1], VKD3D_SHADER_PARAMETER_NAME_BUMP_MATRIX_0 + idx, VSIR_DATA_F32);
    ins->src[2] = *coords;

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MAD, 1, 3)))
        return NULL;
    vsir_src_operand_from_d3dbc_swizzled(parser, &ins->src[0],
            perturbation, perturbation_rel_addr, VKD3D_SHADER_SWIZZLE(Y, Y, Y, Y));
    vsir_src_operand_init_parameter_vec4(&ins->src[1], VKD3D_SHADER_PARAMETER_NAME_BUMP_MATRIX_0 + idx, VSIR_DATA_F32);
    ins->src[1].swizzle = VKD3D_SHADER_SWIZZLE(Z, W, W, W);
    vsir_src_operand_init_ssa_f32v4(&ins->src[2], temp_id);
    ins->src[2].swizzle = VKD3D_SHADER_SWIZZLE(X, Y, Y, Y);

    return ins;
}

static enum vkd3d_result d3dbc_read_bem(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins)
{
    struct vsir_src_operand coords;
    struct vsir_instruction *ins;

    /* bem DST.xy, SRC0, SRC1
     *      ->
     * mad srTMP.xy, SRC1.xx, BUMP_MATRIX#.xy, SRC0.xy
     * mad   DST.xy, SRC1.yy, BUMP_MATRIX#.zw, srTMP.xy */

    vsir_src_operand_from_d3dbc(parser, &coords, &d3dbc_ins->src[0], &d3dbc_ins->src_rel_addr[0]);

    if (!(ins = generate_bump_coords(parser, d3dbc_ins->dst[0].o.index,
            &coords, &d3dbc_ins->src[1], &d3dbc_ins->src_rel_addr[1])))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    vsir_dst_operand_from_d3dbc(parser, &ins->dst[0], &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);

    return VKD3D_OK;
}

static bool is_texture_projected(struct vkd3d_shader_sm1_parser *parser, unsigned int index)
{
    const struct vkd3d_shader_parameter1 *parameter;

    if (!(parameter = vsir_program_get_parameter(parser->program, VKD3D_SHADER_PARAMETER_NAME_PROJECTED_TEXTURE_MASK)))
        return false;

    if (parameter->type != VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT)
    {
        vkd3d_shader_parser_error(&parser->p, VKD3D_SHADER_ERROR_D3DBC_NOT_IMPLEMENTED,
                "Unsupported projected texture mask parameter type %#x.", parameter->type);
        return false;
    }

    if (parameter->data_type != VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32)
    {
        vkd3d_shader_parser_error(&parser->p, VKD3D_SHADER_ERROR_VSIR_INVALID_DATA_TYPE,
                "Invalid projected texture mask parameter data type %#x.", parameter->data_type);
        return false;
    }

    return parameter->u.immediate_constant.u.u32 & (1u << index);
}

static enum vkd3d_result d3dbc_read_texld_sm1(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins)
{
    unsigned int idx = d3dbc_ins->dst[0].o.index;
    const struct vsir_descriptor *descriptor;
    struct vsir_instruction *ins;

    /* texld DST#, SRC -> sample DST#, SRC, resource#, sampler# */

    descriptor = vkd3d_shader_find_descriptor(&parser->program->descriptors, VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER, idx);
    if (descriptor->flags & VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_SAMPLER_COMPARISON_MODE)
    {
        if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_SAMPLE_C, 1, 4)))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        vsir_src_operand_from_d3dbc_swizzled(parser, &ins->src[3], &d3dbc_ins->src[0], &d3dbc_ins->src_rel_addr[0],
                VKD3D_SHADER_SWIZZLE(Z, Z, Z, Z));
    }
    else
    {
        if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_SAMPLE, 1, 3)))
            return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    vsir_dst_operand_from_d3dbc(parser, &ins->dst[0], &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);
    vsir_src_operand_from_d3dbc(parser, &ins->src[0], &d3dbc_ins->src[0], &d3dbc_ins->src_rel_addr[0]);
    vsir_src_operand_init_resource(&ins->src[1], idx, idx);
    vsir_src_operand_init_sampler(&ins->src[2], idx, idx);

    if (descriptor->flags & VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_SAMPLER_COMPARISON_MODE)
        ins->src[1].swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);
    return VKD3D_OK;
}

static enum vkd3d_result d3dbc_read_texbem(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins)
{
    bool is_texbeml = (d3dbc_ins->opcode == VKD3D_SM1_OP_TEXBEML);
    struct vsir_program *program = parser->program;
    unsigned int idx = d3dbc_ins->dst[0].o.index;
    const struct vsir_descriptor *descriptor;
    struct vsir_src_operand coords;
    struct vsir_instruction *ins;
    unsigned int coords_id;

    /* texbem t#, SRC
     *      is equivalent to
     * bem srCOORDS.xy, t#, SRC
     * texld t#, srCOORDS
     *      ->
     * mad    srTMP.xy, SRC.xx, BUMP_MATRIX#.xy, t#.xy
     * mad srCOORDS.xy, SRC.yy, BUMP_MATRIX#.zw, srTMP.xy
     * sample t#, srCOORDS, resource#, sampler#
     *
     * Luminance then adds:
     *
     * mad srLUM.x, SRC.z, BUMP_LUMINANCE_SCALE#, BUMP_LUMINANCE_OFFSET#
     * mul t#, t#, srLUM.xxxx
     *
     * If projecting, we replace srCOORDS calculation with
     *
     * div srPROJ, t#, t#.w
     * bem srCOORDS.xy, srPROJ.xy, SRC
     *
     * Note that the t# destination will subsequently be turned into a temp. */

    descriptor = vkd3d_shader_find_descriptor(&program->descriptors, VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER, idx);
    if (descriptor->flags & VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_SAMPLER_COMPARISON_MODE)
    {
        vkd3d_shader_parser_error(&parser->p, VKD3D_SHADER_ERROR_D3DBC_NOT_IMPLEMENTED,
                "Unhandled TEXBEM(L) with a comparison sampler.");
        return VKD3D_ERROR_NOT_IMPLEMENTED;
    }

    descriptor = vkd3d_shader_find_descriptor(&program->descriptors, VKD3D_SHADER_DESCRIPTOR_TYPE_SRV, idx);
    if (descriptor->resource_type != VKD3D_SHADER_RESOURCE_TEXTURE_2D)
    {
        vkd3d_shader_parser_error(&parser->p, VKD3D_SHADER_ERROR_D3DBC_NOT_IMPLEMENTED,
                "Unhandled TEXBEM(L) with resource dimension %#x.", descriptor->resource_type);
        return VKD3D_ERROR_NOT_IMPLEMENTED;
    }

    if (is_texture_projected(parser, idx))
    {
        unsigned int ssa_proj = program->ssa_count++;

        if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_DIV, 1, 2)))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], ssa_proj);
        vsir_operand_from_d3dbc(parser, &ins->src[0].reg, &d3dbc_ins->dst[0].o, &d3dbc_ins->dst_rel_addr[0]);
        ins->src[0].swizzle = VKD3D_SHADER_NO_SWIZZLE;
        vsir_operand_from_d3dbc(parser, &ins->src[1].reg, &d3dbc_ins->dst[0].o, &d3dbc_ins->src_rel_addr[1]);
        ins->src[1].swizzle = VKD3D_SHADER_SWIZZLE(W, W, W, W);

        vsir_src_operand_init_ssa_f32v4(&coords, ssa_proj);
    }
    else
    {
        memset(&coords, 0, sizeof(coords));
        vsir_operand_from_d3dbc(parser, &coords.reg, &d3dbc_ins->dst[0].o, &d3dbc_ins->dst_rel_addr[0]);
        coords.swizzle = VKD3D_SHADER_NO_SWIZZLE;
    }

    if (!(ins = generate_bump_coords(parser, idx, &coords, &d3dbc_ins->src[0], &d3dbc_ins->src_rel_addr[0])))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    coords_id = program->ssa_count++;
    vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], coords_id);
    ins->dst[0].write_mask = 0x3;

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_SAMPLE, 1, 3)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    vsir_src_operand_init_ssa_f32v4(&ins->src[0], coords_id);
    ins->src[0].swizzle = VKD3D_SHADER_SWIZZLE(X, Y, Y, Y);
    vsir_src_operand_init_resource(&ins->src[1], idx, idx);
    vsir_src_operand_init_sampler(&ins->src[2], idx, idx);

    if (is_texbeml)
    {
        unsigned int luminance_id = program->ssa_count++;
        unsigned int sample_id = program->ssa_count++;

        vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], sample_id);

        if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MAD, 1, 3)))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], luminance_id);
        ins->dst[0].write_mask = 0x1;
        vsir_src_operand_from_d3dbc_swizzled(parser, &ins->src[0],
                &d3dbc_ins->src[0], &d3dbc_ins->src_rel_addr[0], VKD3D_SHADER_SWIZZLE(Z, Z, Z, Z));
        vsir_src_operand_init_parameter(&ins->src[1],
                VKD3D_SHADER_PARAMETER_NAME_BUMP_LUMINANCE_SCALE_0 + idx, VSIR_DATA_F32);
        vsir_src_operand_init_parameter(&ins->src[2],
                VKD3D_SHADER_PARAMETER_NAME_BUMP_LUMINANCE_OFFSET_0 + idx, VSIR_DATA_F32);

        if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MUL, 1, 2)))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        vsir_src_operand_init_ssa_f32v4(&ins->src[0], sample_id);
        vsir_src_operand_init_ssa_f32v4(&ins->src[1], luminance_id);
        ins->src[1].swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);
    }

    vsir_dst_operand_from_d3dbc(parser, &ins->dst[0], &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);

    return VKD3D_OK;
}

static void d3dbc_record_def(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *ins, enum vkd3d_shader_d3dbc_constant_register set)
{
    struct flat_constant_def *def;

    if (!vkd3d_array_reserve((void **)&parser->defs, &parser->defs_capacity,
            parser->def_count + 1, sizeof(*parser->defs)))
    {
        parser->abort = true;
        return;
    }
    def = &parser->defs[parser->def_count++];

    def->set = set;
    def->index = ins->dst[0].o.index;
    def->value = ins->immconst;
}

static enum vkd3d_result d3dbc_read_loop(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins)
{
    uint32_t iter_id, aL_id, neg_id;
    struct vsir_instruction *ins;

    /* loop aL, i#
     *  ->
     * mov rITER.x, i#.x
     * ineg srNEG, i#.z
     * iadd rVAL.x, i#.y, srNEG
     * loop
     *     breakp_z rITER.x
     *     iadd rITER.x, rITER.x, -1
     *     iadd rVAL.x, rVAL.x, i#.z
     */

    if (!vkd3d_array_reserve((void **)&parser->aL_ids, &parser->aL_ids_capacity,
            parser->aL_count + 1, sizeof(*parser->aL_ids)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    parser->aL_ids[parser->aL_count++] = aL_id = parser->internal_temp_id++;
    iter_id = parser->internal_temp_id++;
    parser->program->temp_count = parser->internal_temp_id;

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MOV, 1, 1)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    vsir_dst_operand_init_temp_u32(&ins->dst[0], iter_id);
    vsir_src_operand_from_d3dbc_swizzled(parser, &ins->src[0],
            &d3dbc_ins->src[1], &d3dbc_ins->src_rel_addr[1], VKD3D_SHADER_SWIZZLE(X, X, X, X));

    neg_id = parser->program->ssa_count++;
    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_INEG, 1, 1)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    vsir_dst_operand_init_ssa_scalar(&ins->dst[0], neg_id, VSIR_DATA_U32);
    vsir_src_operand_from_d3dbc_swizzled(parser, &ins->src[0],
            &d3dbc_ins->src[1], &d3dbc_ins->src_rel_addr[1], VKD3D_SHADER_SWIZZLE(Z, Z, Z, Z));

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_IADD, 1, 2)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    vsir_dst_operand_init_temp_u32(&ins->dst[0], aL_id);
    vsir_src_operand_from_d3dbc_swizzled(parser, &ins->src[0],
            &d3dbc_ins->src[1], &d3dbc_ins->src_rel_addr[1], VKD3D_SHADER_SWIZZLE(Y, Y, Y, Y));
    vsir_src_operand_init_ssa_scalar(&ins->src[1], neg_id, VSIR_DATA_U32);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_LOOP, 0, 0)))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_BREAKP, 0, 1)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    ins->flags = VKD3D_SHADER_CONDITIONAL_OP_Z;
    vsir_src_operand_init_temp_u32(&ins->src[0], iter_id);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_IADD, 1, 2)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    vsir_dst_operand_init_temp_u32(&ins->dst[0], iter_id);
    vsir_src_operand_init_temp_u32(&ins->src[0], iter_id);
    vsir_src_operand_init_const_u32(&ins->src[1], -1);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_IADD, 1, 2)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    vsir_dst_operand_init_temp_u32(&ins->dst[0], aL_id);
    vsir_src_operand_init_temp_u32(&ins->src[0], aL_id);
    vsir_src_operand_from_d3dbc_swizzled(parser, &ins->src[1],
            &d3dbc_ins->src[1], &d3dbc_ins->src_rel_addr[1], VKD3D_SHADER_SWIZZLE(Z, Z, Z, Z));

    return VKD3D_OK;
}

static enum vkd3d_result d3dbc_read_lrp(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins)
{
    struct vsir_instruction *ins;
    unsigned int neg_id, mad_id;

    /* lrp DST, SRC0, SRC1, SRC2
     *  ->
     * neg srNEG, SRC0
     * mad srMAD, srNEG, SRC2, SRC2
     * mad DST, SRC0, SRC1, srMAD */

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_NEG, 1, 1)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    neg_id = parser->program->ssa_count++;
    vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], neg_id);
    vsir_src_operand_from_d3dbc(parser, &ins->src[0], &d3dbc_ins->src[0], &d3dbc_ins->src_rel_addr[0]);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MAD, 1, 3)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    mad_id = parser->program->ssa_count++;
    vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], mad_id);
    vsir_src_operand_init_ssa_f32v4(&ins->src[0], neg_id);
    vsir_src_operand_from_d3dbc(parser, &ins->src[1], &d3dbc_ins->src[2], &d3dbc_ins->src_rel_addr[2]);
    vsir_src_operand_from_d3dbc(parser, &ins->src[2], &d3dbc_ins->src[2], &d3dbc_ins->src_rel_addr[2]);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MAD, 1, 3)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    vsir_dst_operand_from_d3dbc(parser, &ins->dst[0], &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);
    vsir_src_operand_from_d3dbc(parser, &ins->src[0], &d3dbc_ins->src[0], &d3dbc_ins->src_rel_addr[0]);
    vsir_src_operand_from_d3dbc(parser, &ins->src[1], &d3dbc_ins->src[1], &d3dbc_ins->src_rel_addr[1]);
    vsir_src_operand_init_ssa_f32v4(&ins->src[2], mad_id);

    return VKD3D_OK;
}

static enum vkd3d_result d3dbc_read_mnxn(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins, enum vsir_opcode opcode, unsigned int row_count)
{
    struct d3dbc_src_operand src = d3dbc_ins->src[1];
    struct vsir_instruction *ins;
    unsigned int i;

    /* mNxM DST, SRC0, SRC1
     *      ->
     * dpN DST.x, SRC0, SRC1
     * ...
     * dpN DST.(M-1), SRC0, SRC1[M-1] */

    VKD3D_ASSERT(d3dbc_ins->dst[0].write_mask == (1u << row_count) - 1);

    for (i = 0; i < row_count; ++i)
    {
        if (!(ins = d3dbc_append_instruction(parser, opcode, 1, 2)))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        vsir_dst_operand_from_d3dbc(parser, &ins->dst[0], &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);
        ins->dst[0].write_mask = (1u << i);
        vsir_src_operand_from_d3dbc(parser, &ins->src[0], &d3dbc_ins->src[0], &d3dbc_ins->src_rel_addr[0]);
        vsir_src_operand_from_d3dbc(parser, &ins->src[1], &src, &d3dbc_ins->src_rel_addr[1]);

        ++src.o.index;
    }

    return VKD3D_OK;
}

static enum vkd3d_result d3dbc_read_texm3xnpad(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins, unsigned int row_count)
{
    struct vkd3d_shader_sm1_texmpad_state *texmpad_state;
    struct vsir_instruction *ins;
    uint32_t texmk_id;

    /* texm3xMpad DST, SRC
     *      ->
     * dp3 srTEXM3xM(k++), DST, SRC
     *
     * where srTEXM3xM(k) is an SSA scalar reserved for the k-th component of
     * the destination of the texm3xM* instruction expected later.
     */
    texmpad_state = (row_count == 3) ? &parser->texm3x3pad_state : &parser->texm3x2pad_state;

    if (texmpad_state->components_read >= ARRAY_SIZE(texmpad_state->ssa_ids))
    {
        ++texmpad_state->components_read;
        return VKD3D_ERROR_INVALID_SHADER;
    }
    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_DP3, 1, 2)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    texmk_id = parser->program->ssa_count++;
    texmpad_state->ssa_ids[texmpad_state->components_read++] = texmk_id;
    vsir_dst_operand_init_ssa_f32(&ins->dst[0], texmk_id);
    vsir_operand_from_d3dbc(parser, &ins->src[0].reg, &d3dbc_ins->dst[0].o, &d3dbc_ins->dst_rel_addr[0]);
    ins->src[0].modifiers = VKD3DSPSM_NONE;
    ins->src[0].swizzle = VKD3D_SHADER_NO_SWIZZLE;
    vsir_src_operand_from_d3dbc(parser, &ins->src[1], &d3dbc_ins->src[0], &d3dbc_ins->src_rel_addr[0]);

    return VKD3D_OK;
}

static enum vkd3d_result d3dbc_read_texm3xntex(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins, unsigned int row_count,
        const char *op_name, const char *pad_op_name)
{
    struct vkd3d_shader_sm1_texmpad_state *texmpad_state;
    struct vsir_instruction *ins;
    enum vkd3d_result res;
    uint32_t texmk_id;
    unsigned int i;

    /* texm3x2tex DST, SRC
     *      ->
     * dp3 srTEXM3x2(1), DST, SRC
     * mov DST.x srTEXM3x2(0)
     * mov DST.y srTEXM3x2(1)
     * tex DST
     *
     * texm3x3tex DST, SRC
     *      ->
     * dp3 srTEXM3x3(2), DST, SRC
     * mov DST.x srTEXM3x3(0)
     * mov DST.y srTEXM3x3(1)
     * mov DST.z srTEXM3x3(2)
     * tex DST
     *
     * where each srTEXM3xM(k) is an SSA scalar initialized in the previous
     * texm3xMpad instructions, except for the one initialized here.
     */
    texmpad_state = (row_count == 3) ? &parser->texm3x3pad_state : &parser->texm3x2pad_state;

    if (texmpad_state->components_read != row_count - 1)
    {
        vkd3d_shader_parser_error(&parser->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_INSTRUCTION_SEQUENCE,
                "Invalid number of %s instructions before %s. Found %u, expected %u.\n",
                pad_op_name, op_name, texmpad_state->components_read, row_count - 1);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    if ((res = d3dbc_read_texm3xnpad(parser, d3dbc_ins, row_count)) < 0)
        return res;

    for (i = 0; i < row_count; ++i)
    {
        texmk_id = texmpad_state->ssa_ids[i];
        if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MOV, 1, 1)))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        vsir_dst_operand_from_d3dbc(parser, &ins->dst[0], &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);
        ins->dst[0].write_mask = (1u << i);
        vsir_src_operand_init_ssa_f32(&ins->src[0], texmk_id);
    }

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_TEX, 1, 0)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    vsir_dst_operand_from_d3dbc(parser, &ins->dst[0], &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);

    /* Reset for next instructions. */
    texmpad_state->components_read = 0;

    return VKD3D_OK;
}

static enum vkd3d_result d3dbc_read_texm3x3(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins)
{
    struct vkd3d_shader_sm1_texmpad_state *texmpad_state;
    struct vsir_instruction *ins;
    enum vkd3d_result res;
    uint32_t texmk_id;
    unsigned int i;

    /* texm3x3 DST, SRC
     *      ->
     * dp3 srTEXM3x3(k++), DST, SRC
     * mov DST.x srTEXM3x3(0)
     * mov DST.y srTEXM3x3(1)
     * mov DST.z srTEXM3x3(2)
     * mov DST.w 1.0f
     *
     * where srTEXM3x3(k) is an SSA scalar reserved for the the previous
     * texm3x3pad instructions.
     */
    texmpad_state = &parser->texm3x3pad_state;

    if (texmpad_state->components_read != 2)
    {
        vkd3d_shader_parser_error(&parser->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_INSTRUCTION_SEQUENCE,
                "Invalid number of %s instructions before %s. Found %u, expected %u.\n",
                "texm3x3pad", "texm3x3", texmpad_state->components_read, 2);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    if ((res = d3dbc_read_texm3xnpad(parser, d3dbc_ins, 3)) < 0)
        return res;

    for (i = 0; i < 3; ++i)
    {
        texmk_id = texmpad_state->ssa_ids[i];
        if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MOV, 1, 1)))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        vsir_dst_operand_from_d3dbc(parser, &ins->dst[0], &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);
        ins->dst[0].write_mask = (1u << i);
        vsir_src_operand_init_ssa_f32(&ins->src[0], texmk_id);
    }

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MOV, 1, 1)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    vsir_dst_operand_from_d3dbc(parser, &ins->dst[0], &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);
    ins->dst[0].write_mask = VKD3DSP_WRITEMASK_3;
    vsir_src_operand_init_const_f32(&ins->src[0], 1.0);

    /* Reset for next instructions. */
    texmpad_state->components_read = 0;

    return VKD3D_OK;
}

static enum vkd3d_result d3dbc_read_nrm(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins)
{
    unsigned int dot_id, rsq_id, mul_id;
    struct vsir_instruction *ins;

    /* nrm DST, SRC
     *      ->
     * dp3 srDOT, SRC, SRC
     * rsq srRSQ, srDOT
     * mul srMUL, srRSQ, SRC
     * movc DST, srDOT, srMUL, srDOT */

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_DP3, 1, 2)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    dot_id = parser->program->ssa_count++;
    vsir_dst_operand_init_ssa_f32(&ins->dst[0], dot_id);
    vsir_src_operand_from_d3dbc(parser, &ins->src[0], &d3dbc_ins->src[0], &d3dbc_ins->src_rel_addr[0]);
    vsir_src_operand_from_d3dbc(parser, &ins->src[1], &d3dbc_ins->src[0], &d3dbc_ins->src_rel_addr[0]);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_RSQ, 1, 1)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    rsq_id = parser->program->ssa_count++;
    vsir_dst_operand_init_ssa_f32(&ins->dst[0], rsq_id);
    vsir_src_operand_init_ssa_f32(&ins->src[0], dot_id);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MUL, 1, 2)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    mul_id = parser->program->ssa_count++;
    vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], mul_id);
    vsir_src_operand_init_ssa_f32(&ins->src[0], rsq_id);
    vsir_src_operand_from_d3dbc(parser, &ins->src[1], &d3dbc_ins->src[0], &d3dbc_ins->src_rel_addr[0]);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MOVC, 1, 3)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    vsir_dst_operand_from_d3dbc(parser, &ins->dst[0], &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);
    vsir_src_operand_init_ssa(&ins->src[0], dot_id, VSIR_DATA_U32, VSIR_DIMENSION_SCALAR);
    vsir_src_operand_init_ssa_f32v4(&ins->src[1], mul_id);
    vsir_src_operand_init_ssa_f32(&ins->src[2], dot_id);

    return VKD3D_OK;
}

static enum vkd3d_result d3dbc_read_pow(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins)
{
    unsigned int abs_id, log_id, ne_id, movc_id, mul_id;
    struct vsir_instruction *ins;

    /* pow DST, SRC0, SRC1
     *      ->
     * abs srABS, SRC0.x
     * log srLOG, srABS
     * ne srNE, SRC1.x, l(0.0f)
     * movc srMOVC, srNE, srLOG, srABS
     * mul srMUL, srMOVC, SRC1.x
     * exp DST, srMUL */

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_ABS, 1, 1)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    abs_id = parser->program->ssa_count++;
    vsir_dst_operand_init_ssa_f32(&ins->dst[0], abs_id);
    vsir_src_operand_from_d3dbc(parser, &ins->src[0], &d3dbc_ins->src[0], &d3dbc_ins->src_rel_addr[0]);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_LOG, 1, 1)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    log_id = parser->program->ssa_count++;
    vsir_dst_operand_init_ssa_f32(&ins->dst[0], log_id);
    vsir_src_operand_init_ssa_f32(&ins->src[0], abs_id);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_NEU, 1, 2)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    ne_id = parser->program->ssa_count++;
    vsir_dst_operand_init_ssa(&ins->dst[0], ne_id, VSIR_DATA_U32, VSIR_DIMENSION_SCALAR);
    vsir_src_operand_from_d3dbc(parser, &ins->src[0], &d3dbc_ins->src[1], &d3dbc_ins->src_rel_addr[1]);
    vsir_src_operand_init_const_f32(&ins->src[1], 0.0f);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MOVC, 1, 3)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    movc_id = parser->program->ssa_count++;
    vsir_dst_operand_init_ssa_f32(&ins->dst[0], movc_id);
    vsir_src_operand_init_ssa(&ins->src[0], ne_id, VSIR_DATA_U32, VSIR_DIMENSION_SCALAR);
    vsir_src_operand_init_ssa_f32(&ins->src[1], log_id);
    vsir_src_operand_init_ssa_f32(&ins->src[2], abs_id);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MUL, 1, 2)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    mul_id = parser->program->ssa_count++;
    vsir_dst_operand_init_ssa_f32(&ins->dst[0], mul_id);
    vsir_src_operand_init_ssa_f32(&ins->src[0], movc_id);
    vsir_src_operand_from_d3dbc(parser, &ins->src[1], &d3dbc_ins->src[1], &d3dbc_ins->src_rel_addr[1]);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_EXP, 1, 1)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    vsir_dst_operand_from_d3dbc(parser, &ins->dst[0], &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);
    vsir_src_operand_init_ssa_f32(&ins->src[0], mul_id);

    return VKD3D_OK;
}

static enum vkd3d_result d3dbc_read_texcoord(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins)
{
    struct vsir_instruction *ins;

    if (vkd3d_shader_ver_ge(&parser->program->shader_version, 1, 4))
    {
        /* texcrd DST, SRC -> mov DST, SRC */

        if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MOV, 1, 1)))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        vsir_dst_operand_from_d3dbc(parser, &ins->dst[0], &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);
        vsir_src_operand_from_d3dbc(parser, &ins->src[0], &d3dbc_ins->src[0], &d3dbc_ins->src_rel_addr[0]);
    }
    else
    {
        /* texcoord t# -> saturate t#, t# */

        if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_SATURATE, 1, 1)))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        vsir_dst_operand_from_d3dbc(parser, &ins->dst[0], &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);
        vsir_operand_from_d3dbc(parser, &ins->src[0].reg, &d3dbc_ins->dst[0].o, &d3dbc_ins->dst_rel_addr[0]);
        ins->src[0].swizzle = VKD3D_SHADER_NO_SWIZZLE;
    }
    return VKD3D_OK;
}

static enum vkd3d_result d3dbc_read_texdepth(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins)
{
    unsigned int ssa_eq, ssa_div, ssa_sat;
    struct vsir_instruction *ins;

    /* texdepth r5
     *      ->
     * eq srEQ, r5.y, 0.0
     * div srDIV, r5.x, r5.y
     * saturate srSAT, srDIV
     * movc oDepth, srEQ, 1.0, srSAT */

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_EQO, 1, 2)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    ssa_eq = parser->program->ssa_count++;
    vsir_dst_operand_init_ssa_bool(&ins->dst[0], ssa_eq);
    vsir_src_operand_init_temp_f32v4(&ins->src[0], 5);
    ins->src[0].swizzle = VKD3D_SHADER_SWIZZLE(Y, Y, Y, Y);
    vsir_src_operand_init_const_f32(&ins->src[1], 0.0f);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_DIV, 1, 2)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    ssa_div = parser->program->ssa_count++;
    vsir_dst_operand_init_ssa_f32(&ins->dst[0], ssa_div);
    vsir_src_operand_init_temp_f32v4(&ins->src[0], 5);
    ins->src[0].swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);
    vsir_src_operand_init_temp_f32v4(&ins->src[1], 5);
    ins->src[1].swizzle = VKD3D_SHADER_SWIZZLE(Y, Y, Y, Y);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_SATURATE, 1, 1)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    ssa_sat = parser->program->ssa_count++;
    vsir_dst_operand_init_ssa_f32(&ins->dst[0], ssa_sat);
    vsir_src_operand_init_ssa_f32(&ins->src[0], ssa_div);

    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MOVC, 1, 3)))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    vsir_dst_operand_init(&ins->dst[0], VSIR_REGISTER_DEPTHOUT, VSIR_DATA_F32, 0);
    ins->dst[0].reg.dimension = VSIR_DIMENSION_SCALAR;
    vsir_src_operand_init_ssa_bool(&ins->src[0], ssa_eq);
    vsir_src_operand_init_const_f32(&ins->src[1], 1.0f);
    vsir_src_operand_init_ssa_f32(&ins->src[2], ssa_sat);

    bitmap_set(parser->program->io_dcls, VSIR_REGISTER_DEPTHOUT);
    return VKD3D_OK;
}

static enum vkd3d_result d3dbc_read_simple_instruction(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins, enum vsir_opcode vsir_opcode)
{
    struct vsir_instruction *ins;

    if (!(ins = d3dbc_append_instruction(parser, vsir_opcode, d3dbc_ins->dst_count, d3dbc_ins->src_count)))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    if (d3dbc_ins->dst_count)
        vsir_dst_operand_from_d3dbc(parser, &ins->dst[0], &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);
    for (unsigned int i = 0; i < d3dbc_ins->src_count; ++i)
        vsir_src_operand_from_d3dbc(parser, &ins->src[i], &d3dbc_ins->src[i], &d3dbc_ins->src_rel_addr[i]);
    ins->flags = d3dbc_ins->flags;
    return VKD3D_OK;
}

static enum vkd3d_result shader_sm1_read_instruction(struct vkd3d_shader_sm1_parser *parser,
        const struct d3dbc_instruction *d3dbc_ins, const struct vkd3d_sm1_opcode_info *opcode_info)
{
    struct vsir_program *program = parser->program;
    struct vkd3d_shader_semantic *semantic;
    struct vsir_instruction *ins;
    unsigned int resource_idx, i;

    if (program->shader_version.major == 1)
        d3dbc_update_descriptors(parser, d3dbc_ins);

    if (d3dbc_ins->opcode != VKD3D_SM1_OP_DCL)
    {
        bool is_def = (d3dbc_ins->opcode == VKD3D_SM1_OP_DEF
                || d3dbc_ins->opcode == VKD3D_SM1_OP_DEFB
                || d3dbc_ins->opcode == VKD3D_SM1_OP_DEFI);

        if (d3dbc_ins->dst_count)
            d3dbc_scan_operand(parser, &d3dbc_ins->dst[0].o, d3dbc_ins->dst[0].write_mask, is_def, 1);

        for (i = 0; i < d3dbc_ins->src_count; ++i)
        {
            d3dbc_scan_operand(parser, &d3dbc_ins->src[i].o, mask_from_swizzle(d3dbc_ins->src[i].swizzle),
                    false, d3dbc_get_src_register_count(d3dbc_ins->opcode, i));
        }
    }

    switch (d3dbc_ins->opcode)
    {
        case VKD3D_SM1_OP_BEM:
            return d3dbc_read_bem(parser, d3dbc_ins);

        case VKD3D_SM1_OP_DCL:
            if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_DCL, 0, 0)))
                return VKD3D_ERROR_OUT_OF_MEMORY;

            semantic = &ins->declaration.semantic;
            semantic->usage = d3dbc_ins->semantic.usage;
            semantic->usage_idx = d3dbc_ins->semantic.usage_idx;

            if (d3dbc_ins->semantic.resource_type >= ARRAY_SIZE(resource_type_table))
            {
                vkd3d_shader_parser_error(&parser->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_RESOURCE_TYPE,
                        "Invalid resource type %#x.", d3dbc_ins->semantic.resource_type);
                semantic->resource_type = VKD3D_SHADER_RESOURCE_NONE;
            }
            else
            {
                semantic->resource_type = resource_type_table[d3dbc_ins->semantic.resource_type];
            }
            for (i = 0; i < 4; ++i)
            {
                semantic->resource_data_type[i] = VSIR_DATA_F32;
            }
            vsir_dst_operand_from_d3dbc(parser, &semantic->resource.reg,
                    &d3dbc_ins->dst[0], &d3dbc_ins->dst_rel_addr[0]);
            resource_idx = d3dbc_ins->dst[0].o.index;
            semantic->resource.range.first = semantic->resource.range.last = resource_idx;

            add_signature_element_from_semantic(parser, d3dbc_ins);
            if (semantic->resource_type)
            {
                d3dbc_add_combined_sampler_descriptor(parser, resource_idx, semantic->resource_type);
                parser->texture_descriptors |= (1u << resource_idx);
            }

            if (d3dbc_ins->semantic.usage == VKD3D_DECL_USAGE_PSIZE)
                parser->program->has_point_size = true;
            break;

        case VKD3D_SM1_OP_DEF:
            d3dbc_record_def(parser, d3dbc_ins, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER);
            break;

        case VKD3D_SM1_OP_DEFB:
            d3dbc_record_def(parser, d3dbc_ins, VKD3D_SHADER_D3DBC_BOOL_CONSTANT_REGISTER);
            break;

        case VKD3D_SM1_OP_DEFI:
            d3dbc_record_def(parser, d3dbc_ins, VKD3D_SHADER_D3DBC_INT_CONSTANT_REGISTER);
            break;

        case VKD3D_SM1_OP_LOOP:
            return d3dbc_read_loop(parser, d3dbc_ins);

        case VKD3D_SM1_OP_LRP:
            return d3dbc_read_lrp(parser, d3dbc_ins);

        case VKD3D_SM1_OP_TEXM3x2PAD:
            return d3dbc_read_texm3xnpad(parser, d3dbc_ins, 2);

        case VKD3D_SM1_OP_TEXM3x2TEX:
            return d3dbc_read_texm3xntex(parser, d3dbc_ins, 2, "texm3x2tex", "texm3x2pad");

        case VKD3D_SM1_OP_TEXM3x3PAD:
            return d3dbc_read_texm3xnpad(parser, d3dbc_ins, 3);

        case VKD3D_SM1_OP_TEXM3x3:
            return d3dbc_read_texm3x3(parser, d3dbc_ins);

        case VKD3D_SM1_OP_TEXM3x3TEX:
            return d3dbc_read_texm3xntex(parser, d3dbc_ins, 3, "texm3x3tex", "texm3x3pad");

        case VKD3D_SM1_OP_M3x2:
            return d3dbc_read_mnxn(parser, d3dbc_ins, VSIR_OP_DP3, 2);

        case VKD3D_SM1_OP_M3x3:
            return d3dbc_read_mnxn(parser, d3dbc_ins, VSIR_OP_DP3, 3);

        case VKD3D_SM1_OP_M3x4:
            return d3dbc_read_mnxn(parser, d3dbc_ins, VSIR_OP_DP3, 4);

        case VKD3D_SM1_OP_M4x3:
            return d3dbc_read_mnxn(parser, d3dbc_ins, VSIR_OP_DP4, 3);

        case VKD3D_SM1_OP_M4x4:
            return d3dbc_read_mnxn(parser, d3dbc_ins, VSIR_OP_DP4, 4);

        case VKD3D_SM1_OP_NRM:
            return d3dbc_read_nrm(parser, d3dbc_ins);

        case VKD3D_SM1_OP_POW:
            return d3dbc_read_pow(parser, d3dbc_ins);

        case VKD3D_SM1_OP_TEX:
            if (program->shader_version.major == 1 && program->shader_version.minor == 4)
                return d3dbc_read_texld_sm1(parser, d3dbc_ins);
            else
                return d3dbc_read_simple_instruction(parser, d3dbc_ins, opcode_info->vsir_opcode);

        case VKD3D_SM1_OP_TEXBEM:
        case VKD3D_SM1_OP_TEXBEML:
            return d3dbc_read_texbem(parser, d3dbc_ins);

        case VKD3D_SM1_OP_TEXCOORD:
            return d3dbc_read_texcoord(parser, d3dbc_ins);

        case VKD3D_SM1_OP_TEXDEPTH:
            return d3dbc_read_texdepth(parser, d3dbc_ins);

        /* TEXKILL, uniquely, encodes its argument as a destination, when it
         * is semantically a source. Since we have multiple passes which
         * operate generically on sources or destinations, normalise that. */
        case VKD3D_SM1_OP_TEXKILL:
            if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_TEXKILL, 0, 1)))
                return VKD3D_ERROR_OUT_OF_MEMORY;

            vsir_operand_from_d3dbc(parser, &ins->src[0].reg, &d3dbc_ins->dst[0].o, &d3dbc_ins->dst_rel_addr[0]);
            ins->src[0].swizzle = vsir_swizzle_from_writemask(d3dbc_ins->dst[0].write_mask);
            break;

        case VKD3D_SM1_OP_CALL:
        case VKD3D_SM1_OP_CALLNZ:
        case VKD3D_SM1_OP_CND:
        case VKD3D_SM1_OP_CRS:
        case VKD3D_SM1_OP_DST:
        case VKD3D_SM1_OP_SETP:
        case VKD3D_SM1_OP_SGN:
        case VKD3D_SM1_OP_TEXDP3:
        case VKD3D_SM1_OP_TEXM3x2DEPTH:
        case VKD3D_SM1_OP_TEXM3x3DIFF:
            vkd3d_shader_error(parser->p.message_context, &parser->p.location, VKD3D_SHADER_ERROR_VSIR_NOT_IMPLEMENTED,
                    "Unhandled instruction %#x.\n", d3dbc_ins->opcode);
            return VKD3D_ERROR_NOT_IMPLEMENTED;

        case VKD3D_SM1_OP_ENDLOOP:
            --parser->aL_count;
            return d3dbc_read_simple_instruction(parser, d3dbc_ins, VSIR_OP_ENDLOOP);

        default:
            return d3dbc_read_simple_instruction(parser, d3dbc_ins, opcode_info->vsir_opcode);
    }
    return VKD3D_OK;
}

static bool shader_sm1_is_end(struct vkd3d_shader_sm1_parser *sm1)
{
    const uint32_t **ptr = &sm1->ptr;

    shader_sm1_read_comment(sm1);

    if (*ptr >= sm1->end)
        return true;

    if (**ptr == VKD3D_SM1_END)
    {
        ++(*ptr);
        return true;
    }

    return false;
}

static void shader_sm1_cleanup(struct vkd3d_shader_sm1_parser *parser)
{
    vkd3d_free(parser->aL_ids);
    vkd3d_free(parser->defs);
}

static enum vkd3d_result shader_sm1_init(struct vkd3d_shader_sm1_parser *sm1, struct vsir_program *program,
        const struct vkd3d_shader_compile_info *compile_info, struct vkd3d_shader_message_context *message_context)
{
    const struct vkd3d_shader_location location = {.source_name = compile_info->source_name};
    const uint32_t *code = compile_info->source.code;
    size_t code_size = compile_info->source.size;
    struct vkd3d_shader_version version;
    uint16_t shader_type;
    size_t token_count;

    sm1->d3dbc_source_info = vkd3d_find_struct(compile_info->next, D3DBC_SOURCE_INFO);

    token_count = code_size / sizeof(*sm1->start);

    if (token_count < 2)
    {
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "Invalid shader size %zu (token count %zu). At least 2 tokens are required.",
                code_size, token_count);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    TRACE("Version: 0x%08x.\n", code[0]);

    shader_type = code[0] >> 16;
    version.major = VKD3D_SM1_VERSION_MAJOR(code[0]);
    version.minor = VKD3D_SM1_VERSION_MINOR(code[0]);

    switch (shader_type)
    {
        case VKD3D_SM1_VS:
            version.type = VKD3D_SHADER_TYPE_VERTEX;
            sm1->opcode_table = vs_opcode_table;
            break;

        case VKD3D_SM1_PS:
            version.type = VKD3D_SHADER_TYPE_PIXEL;
            sm1->opcode_table = ps_opcode_table;
            break;

        default:
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_D3DBC_INVALID_VERSION_TOKEN,
                    "Invalid shader type %#x (token 0x%08x).", shader_type, code[0]);
            return VKD3D_ERROR_INVALID_SHADER;
    }

    if (!vkd3d_shader_ver_le(&version, 3, 0))
    {
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_D3DBC_INVALID_VERSION_TOKEN,
                "Invalid shader version %u.%u (token 0x%08x).", version.major, version.minor, code[0]);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    sm1->start = &code[1];
    sm1->end = &code[token_count];

    vkd3d_shader_parser_init(&sm1->p, message_context, compile_info->source_name);
    sm1->program = program;
    sm1->ptr = sm1->start;
    sm1->version = version;

    /* Legal d3dbc programs can use a maximum of 32 registers. */
    sm1->internal_temp_id = 32;

    if (!program)
        return VKD3D_OK;

    /* Estimate instruction count to avoid reallocation in most shaders. */
    if (!vsir_program_init(program, compile_info, &version,
            code_size != ~(size_t)0 ? token_count / 4u + 4 : 16, VSIR_CF_STRUCTURED, VSIR_NORMALISED_SM4))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    /* D3dbc shaders do not (strictly) adhere to IEEE 754. */
    program->global_flags |= VKD3DSGF_REFACTORING_ALLOWED;
    program->f32_denormal_mode = VKD3D_SHADER_DENORMAL_MODE_FLUSH_TO_ZERO;

    return VKD3D_OK;
}

static uint32_t get_external_constant_count(struct vkd3d_shader_sm1_parser *sm1,
        enum vkd3d_shader_d3dbc_constant_register set)
{
    unsigned int j;

    /* Find the highest constant index which is not written by a DEF
     * instruction. We can't (easily) use an FFZ function for this since it
     * needs to be limited by the highest used register index. */
    for (j = sm1->constants[set].count; j > 0; --j)
    {
        if (!bitmap_is_set(sm1->constants[set].def_mask, j - 1))
            return j;
    }

    return 0;
}

static float scale_from_shift(struct vkd3d_shader_sm1_parser *parser, unsigned int shift)
{
    switch (shift)
    {
        case 1: /* _x2 */
            return 2.0f;
        case 2: /* _x4 */
            return 4.0f;
        case 3: /* _x8 */
            return 8.0f;
        case 13: /* _d8 */
            return 1.0f / 8.0f;
        case 14: /* _d4 */
            return 1.0f / 4.0f;
        case 15: /* _d2 */
            return 1.0f / 2.0f;
        default:
            vkd3d_shader_parser_error(&parser->p, VKD3D_SHADER_ERROR_D3DBC_NOT_IMPLEMENTED,
                    "Unhandled destination shift %#x.", shift);
            return 0.0f;
    }
}

static void d3dbc_lower_src_modifier(struct vkd3d_shader_sm1_parser *parser,
        struct d3dbc_src_operand *src, struct d3dbc_src_operand *src_rel_addr)
{
    bool bias = false, negate = false, abs = false, x2 = false;
    struct vsir_instruction *ins;
    unsigned int src_id;

    if (!src->modifiers)
        return;

    /* We can pass a count of 1 here; modifiers are not legal on sources that
     * read multiple consecutive registers. */
    d3dbc_scan_operand(parser, &src->o, mask_from_swizzle(src->swizzle), false, 1);

    src_id = parser->program->ssa_count++;
    if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MOV, 1, 1)))
        return;
    vsir_src_operand_from_d3dbc(parser, &ins->src[0], src, src_rel_addr);
    vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], src_id);

    switch (src->modifiers)
    {
        case VKD3DSPSM_ABS:
            abs = true;
            break;

        case VKD3DSPSM_ABSNEG:
            abs = true;
            negate = true;
            break;

        case VKD3DSPSM_BIAS:
            bias = true;
            break;

        case VKD3DSPSM_BIASNEG:
            bias = true;
            negate = true;
            break;

        case VKD3DSPSM_COMP:
            if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_NEG, 1, 1)))
                return;
            vsir_src_operand_init_ssa_f32v4(&ins->src[0], src_id);
            src_id = parser->program->ssa_count++;
            vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], src_id);

            if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_ADD, 1, 2)))
                return;
            vsir_src_operand_init_const_f32(&ins->src[0], 1.0f);
            vsir_src_operand_init_ssa_f32v4(&ins->src[1], src_id);
            src_id = parser->program->ssa_count++;
            vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], src_id);
            break;

        case VKD3DSPSM_DW:
        case VKD3DSPSM_DZ:
            if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_DIV, 1, 2)))
                return;
            vsir_src_operand_init_ssa_f32v4(&ins->src[0], src_id);
            vsir_src_operand_init_ssa_f32v4(&ins->src[1], src_id);
            ins->src[0].swizzle = VKD3D_SHADER_NO_SWIZZLE;
            if (src->modifiers == VKD3DSPSM_DW)
                ins->src[1].swizzle = VKD3D_SHADER_SWIZZLE(W, W, W, W);
            else
                ins->src[1].swizzle = VKD3D_SHADER_SWIZZLE(Z, Z, Z, Z);
            src_id = parser->program->ssa_count++;
            vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], src_id);
            break;

        case VKD3DSPSM_NEG:
            negate = true;
            break;

        case VKD3DSPSM_SIGN:
            bias = true;
            x2 = true;
            break;

        case VKD3DSPSM_SIGNNEG:
            bias = true;
            x2 = true;
            negate = true;
            break;

        case VKD3DSPSM_X2:
            x2 = true;
            break;

        case VKD3DSPSM_X2NEG:
            x2 = true;
            negate = true;
            break;

        default:
            vkd3d_shader_parser_error(&parser->p, VKD3D_SHADER_ERROR_D3DBC_NOT_IMPLEMENTED,
                    "Unhandled source modifier %#x.", src->modifiers);
            break;
    }

    if (abs)
    {
        if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_ABS, 1, 1)))
            return;
        vsir_src_operand_init_ssa_f32v4(&ins->src[0], src_id);
        src_id = parser->program->ssa_count++;
        vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], src_id);
    }

    if (bias)
    {
        if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_ADD, 1, 2)))
            return;
        vsir_src_operand_init_ssa_f32v4(&ins->src[0], src_id);
        vsir_src_operand_init_const_f32(&ins->src[1], -0.5f);
        src_id = parser->program->ssa_count++;
        vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], src_id);
    }

    if (x2)
    {
        if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_MUL, 1, 2)))
            return;
        vsir_src_operand_init_ssa_f32v4(&ins->src[0], src_id);
        vsir_src_operand_init_const_f32(&ins->src[1], 2.0f);
        src_id = parser->program->ssa_count++;
        vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], src_id);
    }

    if (negate)
    {
        if (!(ins = d3dbc_append_instruction(parser, VSIR_OP_NEG, 1, 1)))
            return;
        vsir_src_operand_init_ssa_f32v4(&ins->src[0], src_id);
        src_id = parser->program->ssa_count++;
        vsir_dst_operand_init_ssa_f32v4(&ins->dst[0], src_id);
    }

    src->o.type = D3DBC_REGISTER_SSA;
    src->o.index = src_id;
    src->o.has_rel_addr = false;
    src->modifiers = VKD3DSPSM_NONE;
    src->swizzle = VKD3D_SHADER_NO_SWIZZLE;
}

int d3dbc_parse(const struct vkd3d_shader_compile_info *compile_info, uint64_t config_flags,
        struct vkd3d_shader_message_context *message_context, struct vsir_program *program)
{
    struct vkd3d_shader_sm1_parser sm1 = {0};
    struct vsir_descriptor *d;
    unsigned int i;
    int ret;

    if ((ret = shader_sm1_init(&sm1, program, compile_info, message_context)) < 0)
    {
        WARN("Failed to initialise shader parser, ret %d.\n", ret);
        return ret;
    }

    while (!shader_sm1_is_end(&sm1))
    {
        const struct vkd3d_sm1_opcode_info *opcode_info;
        struct d3dbc_instruction d3dbc_ins;
        struct d3dbc_dst_operand orig_dst;
        uint32_t dst_temp_id = ~0u;

        if (!d3dbc_parse_instruction(&sm1, &d3dbc_ins, &opcode_info))
        {
            WARN("Encountered unrecognised or invalid instruction.\n");
            shader_sm1_cleanup(&sm1);
            vsir_program_cleanup(program);
            return VKD3D_ERROR_INVALID_SHADER;
        }
        orig_dst = d3dbc_ins.dst[0];

        shader_sm1_validate_instruction(&sm1, &d3dbc_ins);

        if (d3dbc_ins.dst_count)
        {
            /* It is always legitimate to ignore _pp. */
            d3dbc_ins.dst[0].modifiers &= ~VKD3DSPDM_PARTIALPRECISION;

            if ((d3dbc_ins.dst[0].modifiers & VKD3DSPDM_SATURATE) || d3dbc_ins.dst[0].shift)
            {
                /* Don't forget to scan the original. */
                d3dbc_scan_operand(&sm1, &orig_dst.o, orig_dst.write_mask, false, 1);

                /* Replace the output with a temporary.
                 * We will insert VSIR_OP_SATURATE and/or shift after
                 * translating the instruction. */
                dst_temp_id = sm1.internal_temp_id++;
                d3dbc_ins.dst[0].modifiers &= ~VKD3DSPDM_SATURATE;
                d3dbc_ins.dst[0].shift = 0;
                d3dbc_ins.dst[0].o.type = D3DBC_REGISTER_TEMP;
                d3dbc_ins.dst[0].o.index = dst_temp_id;
                d3dbc_ins.dst[0].o.has_rel_addr = false;
            }

            /* We handle _centroid in d3dbc_append_instruction(). */
            if (d3dbc_ins.dst[0].modifiers & ~VKD3DSPDM_MSAMPCENTROID)
                vkd3d_shader_parser_error(&sm1.p, VKD3D_SHADER_ERROR_D3DBC_NOT_IMPLEMENTED,
                        "Aborting due to unimplemented feature: Destination modifier %#x.", d3dbc_ins.dst[0].modifiers);
        }

        for (i = 0; i < d3dbc_ins.src_count; ++i)
            d3dbc_lower_src_modifier(&sm1, &d3dbc_ins.src[i], &d3dbc_ins.src_rel_addr[i]);

        if ((ret = shader_sm1_read_instruction(&sm1, &d3dbc_ins, opcode_info)))
        {
            shader_sm1_cleanup(&sm1);
            vsir_program_cleanup(program);
            return ret;
        }

        if (d3dbc_ins.dst_count && orig_dst.shift)
        {
            struct vsir_instruction *ins;

            if (!(ins = d3dbc_append_instruction(&sm1, VSIR_OP_MUL, 1, 2)))
                break;
            if (orig_dst.modifiers & VKD3DSPDM_SATURATE)
                vsir_dst_operand_init_temp_f32v4(&ins->dst[0], dst_temp_id);
            else
                vsir_dst_operand_from_d3dbc(&sm1, &ins->dst[0], &orig_dst, &d3dbc_ins.dst_rel_addr[0]);
            vsir_src_operand_init_temp_f32v4(&ins->src[0], dst_temp_id);
            vsir_src_operand_init_const_f32(&ins->src[1], scale_from_shift(&sm1, orig_dst.shift));
        }

        if (d3dbc_ins.dst_count && (orig_dst.modifiers & VKD3DSPDM_SATURATE))
        {
            struct vsir_instruction *ins;

            if (!(ins = d3dbc_append_instruction(&sm1, VSIR_OP_SATURATE, 1, 1)))
                break;
            vsir_dst_operand_from_d3dbc(&sm1, &ins->dst[0], &orig_dst, &d3dbc_ins.dst_rel_addr[0]);
            vsir_src_operand_init_temp_f32v4(&ins->src[0], dst_temp_id);
        }
    }

    for (i = 0; i < 3; ++i)
    {
        struct vsir_register_range range = {.space = 0, .first = i, .last = i};
        unsigned int size = get_external_constant_count(&sm1, i);

        if (size)
        {
            if (!(d = vsir_program_add_descriptor(program, VKD3D_SHADER_DESCRIPTOR_TYPE_CBV,
                    i, &range, VKD3D_SHADER_RESOURCE_BUFFER, VSIR_DATA_U32)))
                vkd3d_shader_parser_error(&sm1.p, VKD3D_SHADER_ERROR_D3DBC_OUT_OF_MEMORY,
                        "Failed to create CBV descriptor.");
            else
                d->buffer_size = size * 16;
        }
    }

    program->normalisation_flags.has_descriptor_info = true;

    if (TRACE_ON())
        vsir_program_trace(program);

    if (ret >= 0 && sm1.p.status < 0)
        ret = sm1.p.status;
    shader_sm1_cleanup(&sm1);

    if (ret < 0)
    {
        vsir_program_cleanup(program);
        return ret;
    }

    if (program->normalisation_level >= VSIR_NORMALISED_SM4)
        ret = vsir_program_lower_d3dbc(program, config_flags, compile_info, message_context);

    return ret;
}

struct d3dbc_asm_colours
{
    const char *reset;
    const char *literal;
    const char *opcode;
    const char *reg;
    const char *swizzle;
    const char *version;
    const char *write_mask;
};

static void d3dbc_print_usage(struct vkd3d_string_buffer *buffer, enum vkd3d_decl_usage usage,
        unsigned int usage_idx, const struct vkd3d_shader_version *version)
{
    static const char *const names[] =
    {
        [VKD3D_DECL_USAGE_POSITION]         = "position",
        [VKD3D_DECL_USAGE_BLEND_WEIGHT]     = "blendweight",
        [VKD3D_DECL_USAGE_BLEND_INDICES]    = "blendindices",
        [VKD3D_DECL_USAGE_NORMAL]           = "normal",
        [VKD3D_DECL_USAGE_PSIZE]            = "psize",
        [VKD3D_DECL_USAGE_TEXCOORD]         = "texcoord",
        [VKD3D_DECL_USAGE_TANGENT]          = "tangent",
        [VKD3D_DECL_USAGE_BINORMAL]         = "binormal",
        [VKD3D_DECL_USAGE_TESS_FACTOR]      = "tessfactor",
        [VKD3D_DECL_USAGE_POSITIONT]        = "positiont",
        [VKD3D_DECL_USAGE_COLOR]            = "color",
        [VKD3D_DECL_USAGE_FOG]              = "fog",
        [VKD3D_DECL_USAGE_DEPTH]            = "depth",
        [VKD3D_DECL_USAGE_SAMPLE]           = "sample",
    };

    if (!usage && !usage_idx && version->type == VKD3D_SHADER_TYPE_PIXEL)
        return;

    if ((uint32_t)usage < ARRAY_SIZE(names))
        vkd3d_string_buffer_printf(buffer, "_%s", names[usage]);
    else
        vkd3d_string_buffer_printf(buffer, "_unknown");
    if (usage_idx)
        vkd3d_string_buffer_printf(buffer, "%u", usage_idx);
}

static void d3dbc_print_instruction_flags(struct vkd3d_string_buffer *buffer, const struct d3dbc_instruction *ins,
        const struct vkd3d_shader_version *version, const struct d3dbc_asm_colours *colours)
{
    if (ins->opcode == VKD3D_SM1_OP_DCL)
    {
        vkd3d_string_buffer_printf(buffer, "%s", colours->opcode);
        if (ins->semantic.resource_type)
        {
            if (ins->semantic.resource_type == VKD3D_SM1_RESOURCE_TEXTURE_2D)
                vkd3d_string_buffer_printf(buffer, "_2d");
            else if (ins->semantic.resource_type == VKD3D_SM1_RESOURCE_TEXTURE_3D)
                vkd3d_string_buffer_printf(buffer, "_volume");
            else if (ins->semantic.resource_type == VKD3D_SM1_RESOURCE_TEXTURE_CUBE)
                vkd3d_string_buffer_printf(buffer, "_cube");
            else
                vkd3d_string_buffer_printf(buffer, "_unknown");
        }
        else
        {
            d3dbc_print_usage(buffer, ins->semantic.usage, ins->semantic.usage_idx, version);
        }
        vkd3d_string_buffer_printf(buffer, "%s", colours->reset);
    }
    else if (ins->opcode == VKD3D_SM1_OP_TEX && vkd3d_shader_ver_ge(version, 2, 0))
    {
        if (ins->flags == VKD3DSI_TEXLD_PROJECT)
            vkd3d_string_buffer_printf(buffer, "p");
        else if (ins->flags == VKD3DSI_TEXLD_BIAS)
            vkd3d_string_buffer_printf(buffer, "b");
    }
    else if (ins->opcode == VKD3D_SM1_OP_BREAKC || ins->opcode == VKD3D_SM1_OP_IFC)
    {
        static const char names[][2] =
        {
            [VKD3D_SHADER_REL_OP_GT] = "gt",
            [VKD3D_SHADER_REL_OP_EQ] = "eq",
            [VKD3D_SHADER_REL_OP_GE] = "ge",
            [VKD3D_SHADER_REL_OP_LT] = "lt",
            [VKD3D_SHADER_REL_OP_NE] = "ne",
            [VKD3D_SHADER_REL_OP_LE] = "le",
        };

        if (ins->flags && ins->flags < ARRAY_SIZE(names))
            vkd3d_string_buffer_printf(buffer, "_%.2s", names[ins->flags]);
    }

    if (ins->dst_count)
    {
        if (ins->dst[0].shift == 1)
            vkd3d_string_buffer_printf(buffer, "_x2");
        else if (ins->dst[0].shift == 2)
            vkd3d_string_buffer_printf(buffer, "_x4");
        else if (ins->dst[0].shift == 3)
            vkd3d_string_buffer_printf(buffer, "_x8");
        else if (ins->dst[0].shift == 15)
            vkd3d_string_buffer_printf(buffer, "_d2");
        else if (ins->dst[0].shift == 14)
            vkd3d_string_buffer_printf(buffer, "_d4");
        else if (ins->dst[0].shift == 13)
            vkd3d_string_buffer_printf(buffer, "_d8");

        if (ins->dst[0].modifiers & VKD3DSPDM_SATURATE)
            vkd3d_string_buffer_printf(buffer, "_sat");
        if (ins->dst[0].modifiers & VKD3DSPDM_PARTIALPRECISION)
            vkd3d_string_buffer_printf(buffer, "_pp");
        if (ins->dst[0].modifiers & VKD3DSPDM_MSAMPCENTROID)
            vkd3d_string_buffer_printf(buffer, "_centroid");
    }
}

static void d3dbc_print_src_operand(struct vkd3d_string_buffer *buffer,
        const struct d3dbc_src_operand *src, const struct d3dbc_src_operand *rel_addr,
        const struct vkd3d_shader_version *version, const struct d3dbc_asm_colours *colours);

static void d3dbc_print_operand(struct vkd3d_string_buffer *buffer,
        const struct d3dbc_operand *op, const struct d3dbc_src_operand *rel_addr,
        const struct vkd3d_shader_version *version, const struct d3dbc_asm_colours *colours)
{
    static const char names[][7] =
    {
        [D3DBC_REGISTER_INVALID]    = "x",
        [D3DBC_REGISTER_TEMP]       = "r",
        [D3DBC_REGISTER_INPUT]      = "v",
        [D3DBC_REGISTER_CONST]      = "c",
        [D3DBC_REGISTER_ADDR]       = "a",
        [D3DBC_REGISTER_TEXTURE]    = "t",
        [D3DBC_REGISTER_RASTOUT]    = "oX",
        [D3DBC_REGISTER_ATTROUT]    = "oD",
        [D3DBC_REGISTER_TEXCRDOUT]  = "oT",
        [D3DBC_REGISTER_OUTPUT]     = "o",
        [D3DBC_REGISTER_CONSTINT]   = "i",
        [D3DBC_REGISTER_COLOROUT]   = "oC",
        [D3DBC_REGISTER_DEPTHOUT]   = "oDepth",
        [D3DBC_REGISTER_SAMPLER]    = "s",
        [D3DBC_REGISTER_CONSTBOOL]  = "b",
        [D3DBC_REGISTER_LOOP]       = "aL",
        [D3DBC_REGISTER_MISCTYPE]   = "m",
        [D3DBC_REGISTER_LABEL]      = "l",
        [D3DBC_REGISTER_PREDICATE]  = "p",
    };

    vkd3d_string_buffer_printf(buffer, "%s", colours->reg);

    if (op->type == D3DBC_REGISTER_MISCTYPE && op->index == 0)
        vkd3d_string_buffer_printf(buffer, "vPos");
    else if (op->type == D3DBC_REGISTER_MISCTYPE && op->index == 1)
        vkd3d_string_buffer_printf(buffer, "vFace");
    else if (op->type == D3DBC_REGISTER_RASTOUT && op->index == 0)
        vkd3d_string_buffer_printf(buffer, "oPos");
    else if (op->type == D3DBC_REGISTER_RASTOUT && op->index == 1)
        vkd3d_string_buffer_printf(buffer, "oFog");
    else if (op->type == D3DBC_REGISTER_RASTOUT && op->index == 2)
        vkd3d_string_buffer_printf(buffer, "oPts");
    else
    {
        vkd3d_string_buffer_printf(buffer, "%s", names[op->type]);

        if (op->type != D3DBC_REGISTER_DEPTHOUT && op->type != D3DBC_REGISTER_LOOP)
            vkd3d_string_buffer_printf(buffer, "%u", op->index);
    }

    if (op->has_rel_addr)
    {
        vkd3d_string_buffer_printf(buffer, "[");
        d3dbc_print_src_operand(buffer, rel_addr, NULL, version, colours);
        vkd3d_string_buffer_printf(buffer, "]");
    }

    vkd3d_string_buffer_printf(buffer, "%s", colours->reset);
}

static void d3dbc_print_src_operand(struct vkd3d_string_buffer *buffer,
        const struct d3dbc_src_operand *src, const struct d3dbc_src_operand *rel_addr,
        const struct vkd3d_shader_version *version, const struct d3dbc_asm_colours *colours)
{
    static const char swizzle_chars[4] = "xyzw";
    unsigned int s[4];

    static const struct
    {
        char prefix[3];
        char suffix[6];
    }
    modifiers[] =
    {
        [VKD3DSPSM_ABS]     = {"",   "_abs"},
        [VKD3DSPSM_ABSNEG]  = {"-",  "_abs"},
        [VKD3DSPSM_BIAS]    = {"",   "_bias"},
        [VKD3DSPSM_BIASNEG] = {"-",  "_bias"},
        [VKD3DSPSM_COMP]    = {"1-", ""},
        [VKD3DSPSM_DW]      = {"",   "_dw"},
        [VKD3DSPSM_DZ]      = {"",   "_dz"},
        [VKD3DSPSM_NEG]     = {"-",  ""},
        [VKD3DSPSM_NONE]    = {"",   ""},
        [VKD3DSPSM_NOT]     = {"!",  ""},
        [VKD3DSPSM_SIGN]    = {"",   "_bx2"},
        [VKD3DSPSM_SIGNNEG] = {"-",  "_bx2"},
        [VKD3DSPSM_X2]      = {"",   "_x2"},
        [VKD3DSPSM_X2NEG]   = {"-",  "_x2"},
    };

    if ((unsigned int)src->modifiers < ARRAY_SIZE(modifiers))
        vkd3d_string_buffer_printf(buffer, "%s", modifiers[src->modifiers].prefix);
    d3dbc_print_operand(buffer, &src->o, rel_addr, version, colours);
    if ((unsigned int)src->modifiers < ARRAY_SIZE(modifiers))
        vkd3d_string_buffer_printf(buffer, "%s", modifiers[src->modifiers].suffix);

    for (unsigned int i = 0; i < 4; ++i)
        s[i] = vsir_swizzle_get_component(src->swizzle, i);

    if (s[0] == s[1] && s[0] == s[2] && s[0] == s[3])
        vkd3d_string_buffer_printf(buffer, ".%s%c%s", colours->swizzle, swizzle_chars[s[0]], colours->reset);
    else if (src->swizzle != VKD3D_SHADER_NO_SWIZZLE)
        vkd3d_string_buffer_printf(buffer, ".%s%c%c%c%c%s", colours->swizzle, swizzle_chars[s[0]],
                swizzle_chars[s[1]], swizzle_chars[s[2]], swizzle_chars[s[3]], colours->reset);
}

static void d3dbc_print_dst_operand(struct vkd3d_string_buffer *buffer,
        const struct d3dbc_dst_operand *dst, const struct d3dbc_src_operand *rel_addr,
        const struct vkd3d_shader_version *version, const struct d3dbc_asm_colours *colours)
{
    static const char writemask_chars[4] = "xyzw";

    d3dbc_print_operand(buffer, &dst->o, rel_addr, version, colours);

    if (dst->write_mask == VKD3DSP_WRITEMASK_ALL)
        return;
    vkd3d_string_buffer_printf(buffer, ".%s", colours->write_mask);
    for (unsigned int i = 0; i < 4; ++i)
    {
        if (dst->write_mask & (1u << i))
            vkd3d_string_buffer_printf(buffer, "%c", writemask_chars[i]);
    }
    vkd3d_string_buffer_printf(buffer, "%s", colours->reset);
}

static void d3dbc_print_instruction(struct vkd3d_string_buffer *buffer, const struct d3dbc_instruction *ins,
        const struct vkd3d_shader_sm1_parser *sm1, const struct d3dbc_asm_colours *colours)
{
    const struct vkd3d_sm1_opcode_info *info = shader_sm1_get_opcode_info(sm1, ins->opcode);

    if (ins->coissue)
        vkd3d_string_buffer_printf(buffer, "+ ");

    if (ins->predicated)
    {
        vkd3d_string_buffer_printf(buffer, "(");
        d3dbc_print_src_operand(buffer, &ins->predicate, NULL, &sm1->version, colours);
        vkd3d_string_buffer_printf(buffer, ") ");
    }

    vkd3d_string_buffer_printf(buffer, "%s%s", colours->opcode, info->name);
    d3dbc_print_instruction_flags(buffer, ins, &sm1->version, colours);
    vkd3d_string_buffer_printf(buffer, "%s", colours->reset);

    if (ins->dst_count)
    {
        vkd3d_string_buffer_printf(buffer, " ");
        d3dbc_print_dst_operand(buffer, &ins->dst[0], &ins->dst_rel_addr[0], &sm1->version, colours);
    }

    for (unsigned int i = 0; i < ins->src_count; ++i)
    {
        if (ins->dst_count || i)
            vkd3d_string_buffer_printf(buffer, ",");
        vkd3d_string_buffer_printf(buffer, " ");
        d3dbc_print_src_operand(buffer, &ins->src[i], &ins->src_rel_addr[i], &sm1->version, colours);
    }

    if (ins->opcode == VKD3D_SM1_OP_DEF)
    {
        for (unsigned int i = 0; i < 4; ++i)
        {
            float f = ins->immconst.f32[i];

            vkd3d_string_buffer_printf(buffer, ", ");
            if (isfinite(f) && signbit(f))
            {
                f = -f;
                vkd3d_string_buffer_printf(buffer, "-");
            }
            vkd3d_string_buffer_printf(buffer, "%s", colours->literal);
            vkd3d_string_buffer_print_f32(buffer, f);
            vkd3d_string_buffer_printf(buffer, "%s", colours->reset);
        }
    }
    else if (ins->opcode == VKD3D_SM1_OP_DEFI)
    {
        for (unsigned int i = 0; i < 4; ++i)
        {
            /* Note that we need to handle INT_MIN here as well. */
            if (ins->immconst.i32[i] < 0)
                vkd3d_string_buffer_printf(buffer, ", -%s%u%s",
                        colours->literal, -(uint32_t)ins->immconst.i32[i], colours->reset);
            else
                vkd3d_string_buffer_printf(buffer, ", %s%d%s",
                        colours->literal, ins->immconst.i32[i], colours->reset);
        }
    }
    else if (ins->opcode == VKD3D_SM1_OP_DEFB)
    {
        if (ins->immconst.b32)
            vkd3d_string_buffer_printf(buffer, ", %strue%s", colours->literal, colours->reset);
        else
            vkd3d_string_buffer_printf(buffer, ", %sfalse%s", colours->literal, colours->reset);
    }

    vkd3d_string_buffer_printf(buffer, "\n");
}

enum vkd3d_result d3dbc_disassemble(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, struct vkd3d_shader_message_context *message_context)
{
    struct vsir_compile_info vsir_compile_info;
    struct vkd3d_shader_sm1_parser sm1 = {0};
    const struct d3dbc_asm_colours *colours;
    struct vkd3d_string_buffer buffer;
    unsigned int indent = 0;
    const char *indent_str;
    enum vkd3d_result ret;

    static const struct d3dbc_asm_colours no_colours =
    {
        .reset = "",
        .literal = "",
        .opcode = "",
        .reg = "",
        .swizzle = "",
        .version = "",
        .write_mask = "",
    };
    static const struct d3dbc_asm_colours yes_colours =
    {
        .reset = "\x1b[m",
        .literal = "\x1b[95m",
        .opcode = "\x1b[96;1m",
        .reg = "\x1b[96m",
        .swizzle = "\x1b[93m",
        .version = "\x1b[36m",
        .write_mask = "\x1b[93m",
    };

    vsir_compile_info_init(&vsir_compile_info, compile_info);

    if (vsir_compile_info.formatting & VKD3D_SHADER_COMPILE_OPTION_FORMATTING_COLOUR)
        colours = &yes_colours;
    else
        colours = &no_colours;
    if (vsir_compile_info.formatting & VKD3D_SHADER_COMPILE_OPTION_FORMATTING_INDENT)
        indent_str = "    ";
    else
        indent_str = "";

    if ((ret = shader_sm1_init(&sm1, NULL, compile_info, message_context)) < 0)
        return ret;

    vkd3d_string_buffer_init(&buffer);

    vkd3d_string_buffer_printf(&buffer, "%s%cs_%u_%u%s\n", colours->version,
            sm1.version.type == VKD3D_SHADER_TYPE_PIXEL ? 'p' : 'v',
            sm1.version.major, sm1.version.minor, colours->reset);

    while (!shader_sm1_is_end(&sm1))
    {
        const struct vkd3d_sm1_opcode_info *opcode_info;
        struct d3dbc_instruction ins;

        if (!d3dbc_parse_instruction(&sm1, &ins, &opcode_info))
        {
            vkd3d_string_buffer_cleanup(&buffer);
            shader_sm1_cleanup(&sm1);
            return VKD3D_ERROR_INVALID_SHADER;
        }

        switch (ins.opcode)
        {
            case VKD3D_SM1_OP_ELSE:
            case VKD3D_SM1_OP_ENDIF:
            case VKD3D_SM1_OP_ENDLOOP:
            case VKD3D_SM1_OP_ENDREP:
                if (indent)
                    --indent;
                break;

            default:
                break;
        }

        for (unsigned int j = 0; j < indent; ++j)
            vkd3d_string_buffer_printf(&buffer, "%s", indent_str);

        d3dbc_print_instruction(&buffer, &ins, &sm1, colours);

        switch (ins.opcode)
        {
            case VKD3D_SM1_OP_ELSE:
            case VKD3D_SM1_OP_IF:
            case VKD3D_SM1_OP_IFC:
            case VKD3D_SM1_OP_LOOP:
            case VKD3D_SM1_OP_REP:
                ++indent;
                break;

            default:
                break;
        }

    }

    shader_sm1_cleanup(&sm1);
    vkd3d_shader_code_from_string_buffer(out, &buffer);
    return ret;
}

bool sm1_register_from_semantic_name(const struct vkd3d_shader_version *version, const char *semantic_name,
        unsigned int semantic_index, bool output, enum vkd3d_shader_sysval_semantic *sysval,
        enum vsir_register_type *type, unsigned int *reg)
{
    unsigned int i;

    static const struct
    {
        const char *semantic;
        bool output;
        enum vkd3d_shader_type shader_type;
        unsigned int major_version;
        enum vkd3d_shader_sysval_semantic sysval;
        enum vsir_register_type type;
        unsigned int offset;
    }
    register_table[] =
    {
        {"color",       true,  VKD3D_SHADER_TYPE_PIXEL, 1, VKD3D_SHADER_SV_TARGET,        VSIR_REGISTER_TEMP},
        {"color",       false, VKD3D_SHADER_TYPE_PIXEL, 1, VKD3D_SHADER_SV_NONE,          VSIR_REGISTER_INPUT},
        {"texcoord",    false, VKD3D_SHADER_TYPE_PIXEL, 1, VKD3D_SHADER_SV_NONE,          VSIR_REGISTER_TEXTURE},

        {"color",       true,  VKD3D_SHADER_TYPE_PIXEL, 2, VKD3D_SHADER_SV_TARGET,        VSIR_REGISTER_COLOROUT},
        {"depth",       true,  VKD3D_SHADER_TYPE_PIXEL, 2, VKD3D_SHADER_SV_DEPTH,         VSIR_REGISTER_DEPTHOUT},
        {"sv_depth",    true,  VKD3D_SHADER_TYPE_PIXEL, 2, VKD3D_SHADER_SV_DEPTH,         VSIR_REGISTER_DEPTHOUT},
        {"sv_target",   true,  VKD3D_SHADER_TYPE_PIXEL, 2, VKD3D_SHADER_SV_TARGET,        VSIR_REGISTER_COLOROUT},
        {"color",       false, VKD3D_SHADER_TYPE_PIXEL, 2, VKD3D_SHADER_SV_NONE,          VSIR_REGISTER_INPUT},
        {"texcoord",    false, VKD3D_SHADER_TYPE_PIXEL, 2, VKD3D_SHADER_SV_NONE,          VSIR_REGISTER_TEXTURE},

        {"color",       true,  VKD3D_SHADER_TYPE_PIXEL, 3, VKD3D_SHADER_SV_TARGET,        VSIR_REGISTER_COLOROUT},
        {"depth",       true,  VKD3D_SHADER_TYPE_PIXEL, 3, VKD3D_SHADER_SV_DEPTH,         VSIR_REGISTER_DEPTHOUT},
        {"sv_depth",    true,  VKD3D_SHADER_TYPE_PIXEL, 3, VKD3D_SHADER_SV_DEPTH,         VSIR_REGISTER_DEPTHOUT},
        {"sv_target",   true,  VKD3D_SHADER_TYPE_PIXEL, 3, VKD3D_SHADER_SV_TARGET,        VSIR_REGISTER_COLOROUT},
        {"sv_position", false, VKD3D_SHADER_TYPE_PIXEL, 3, VKD3D_SHADER_SV_POSITION,      VSIR_REGISTER_MISCTYPE, VKD3D_SM1_MISC_POSITION},
        {"vface",       false, VKD3D_SHADER_TYPE_PIXEL, 3, VKD3D_SHADER_SV_IS_FRONT_FACE, VSIR_REGISTER_MISCTYPE, VKD3D_SM1_MISC_FACE},
        {"vpos",        false, VKD3D_SHADER_TYPE_PIXEL, 3, VKD3D_SHADER_SV_POSITION,      VSIR_REGISTER_MISCTYPE, VKD3D_SM1_MISC_POSITION},

        {"color",       true,  VKD3D_SHADER_TYPE_VERTEX, 1, VKD3D_SHADER_SV_NONE,         VSIR_REGISTER_ATTROUT},
        {"fog",         true,  VKD3D_SHADER_TYPE_VERTEX, 1, VKD3D_SHADER_SV_NONE,         VSIR_REGISTER_RASTOUT, VSIR_RASTOUT_FOG},
        {"position",    true,  VKD3D_SHADER_TYPE_VERTEX, 1, VKD3D_SHADER_SV_POSITION,     VSIR_REGISTER_RASTOUT, VSIR_RASTOUT_POSITION},
        {"psize",       true,  VKD3D_SHADER_TYPE_VERTEX, 1, VKD3D_SHADER_SV_NONE,         VSIR_REGISTER_RASTOUT, VSIR_RASTOUT_POINT_SIZE},
        {"sv_position", true,  VKD3D_SHADER_TYPE_VERTEX, 1, VKD3D_SHADER_SV_POSITION,     VSIR_REGISTER_RASTOUT, VSIR_RASTOUT_POSITION},
        {"texcoord",    true,  VKD3D_SHADER_TYPE_VERTEX, 1, VKD3D_SHADER_SV_NONE,         VSIR_REGISTER_TEXCRDOUT},

        {"color",       true,  VKD3D_SHADER_TYPE_VERTEX, 2, VKD3D_SHADER_SV_NONE,         VSIR_REGISTER_ATTROUT},
        {"fog",         true,  VKD3D_SHADER_TYPE_VERTEX, 2, VKD3D_SHADER_SV_NONE,         VSIR_REGISTER_RASTOUT,     VSIR_RASTOUT_FOG},
        {"position",    true,  VKD3D_SHADER_TYPE_VERTEX, 2, VKD3D_SHADER_SV_POSITION,     VSIR_REGISTER_RASTOUT,     VSIR_RASTOUT_POSITION},
        {"psize",       true,  VKD3D_SHADER_TYPE_VERTEX, 2, VKD3D_SHADER_SV_NONE,         VSIR_REGISTER_RASTOUT,     VSIR_RASTOUT_POINT_SIZE},
        {"sv_position", true,  VKD3D_SHADER_TYPE_VERTEX, 2, VKD3D_SHADER_SV_POSITION,     VSIR_REGISTER_RASTOUT,     VSIR_RASTOUT_POSITION},
        {"texcoord",    true,  VKD3D_SHADER_TYPE_VERTEX, 2, VKD3D_SHADER_SV_NONE,         VSIR_REGISTER_TEXCRDOUT},
    };

    for (i = 0; i < ARRAY_SIZE(register_table); ++i)
    {
        if (!ascii_strcasecmp(semantic_name, register_table[i].semantic)
                && output == register_table[i].output
                && version->type == register_table[i].shader_type
                && version->major == register_table[i].major_version)
        {
            if (sysval)
                *sysval = register_table[i].sysval;
            *type = register_table[i].type;
            if (register_table[i].type == VSIR_REGISTER_MISCTYPE || register_table[i].type == VSIR_REGISTER_RASTOUT)
                *reg = register_table[i].offset;
            else
                *reg = semantic_index;
            return true;
        }
    }

    return false;
}

bool sm1_usage_from_semantic_name(const struct vkd3d_shader_version *version, const char *semantic_name,
        uint32_t semantic_index, bool output, enum vkd3d_decl_usage *usage, uint32_t *usage_idx)
{
    static const struct
    {
        const char *name;
        enum vkd3d_decl_usage usage;
    }
    semantics[] =
    {
        {"binormal",        VKD3D_DECL_USAGE_BINORMAL},
        {"blendindices",    VKD3D_DECL_USAGE_BLEND_INDICES},
        {"blendweight",     VKD3D_DECL_USAGE_BLEND_WEIGHT},
        {"color",           VKD3D_DECL_USAGE_COLOR},
        {"depth",           VKD3D_DECL_USAGE_DEPTH},
        {"fog",             VKD3D_DECL_USAGE_FOG},
        {"normal",          VKD3D_DECL_USAGE_NORMAL},
        {"position",        VKD3D_DECL_USAGE_POSITION},
        {"positiont",       VKD3D_DECL_USAGE_POSITIONT},
        {"psize",           VKD3D_DECL_USAGE_PSIZE},
        {"sample",          VKD3D_DECL_USAGE_SAMPLE},
        {"sv_depth",        VKD3D_DECL_USAGE_DEPTH},
        {"sv_position",     VKD3D_DECL_USAGE_POSITION},
        {"tangent",         VKD3D_DECL_USAGE_TANGENT},
        {"tessfactor",      VKD3D_DECL_USAGE_TESS_FACTOR},
        {"texcoord",        VKD3D_DECL_USAGE_TEXCOORD},
    };

    unsigned int i;

    /* Generic I/O registers are used for shader model 3 varyings, as well as
     * for vertex shader inputs in all versions. */
    if ((version->major != 3 || (version->type == VKD3D_SHADER_TYPE_PIXEL && output))
            && !(version->type == VKD3D_SHADER_TYPE_VERTEX && !output))
        return false;

    for (i = 0; i < ARRAY_SIZE(semantics); ++i)
    {
        if (!ascii_strcasecmp(semantic_name, semantics[i].name))
        {
            *usage = semantics[i].usage;
            *usage_idx = semantic_index;
            return true;
        }
    }

    return false;
}

enum ctab_register_set
{
    CTAB_REGISTER_SET_BOOL,
    CTAB_REGISTER_SET_INT4,
    CTAB_REGISTER_SET_FLOAT4,
    CTAB_REGISTER_SET_SAMPLER,
};

struct ctab_header
{
    uint32_t header_size;
    uint32_t creator_offset;
    uint32_t version;
    uint32_t var_count;
    uint32_t vars_offset;
    uint32_t flags;
    uint32_t target_string;
};

struct ctab_var
{
    uint32_t name_offset;
    uint16_t register_set;
    uint16_t index;
    uint32_t bind_count;
    uint32_t type_offset;
    uint32_t default_value_offset;
};

struct d3dbc_const_allocator
{
    struct const_reg
    {
        unsigned int index;
        bool uniform;
        uint8_t allocated_mask;
        float value[4];
    } *regs;
    size_t reg_count, regs_capacity;
};

static void d3dbc_const_allocator_cleanup(struct d3dbc_const_allocator *allocator)
{
    vkd3d_free(allocator->regs);
}

static bool d3dbc_const_allocator_init(struct d3dbc_const_allocator *allocator,
        const struct vkd3d_shader_code *ctab)
{
    const struct ctab_header *header;
    const struct ctab_var *vars;

    if (ctab->size < sizeof(*header))
        return false;
    header = ctab->code;

    if (header->vars_offset >= ctab->size || (ctab->size - header->vars_offset) / sizeof(*vars) < header->var_count)
        return false;
    vars = (const void *)&((const uint8_t *)ctab->code)[header->vars_offset];

    /* First block out all uniforms. Due to dynamic indexing and structure
     * holes, we don't know simply from the program how many registers we can't
     * use. Therefore we read them back from the CTAB. */
    for (uint32_t i = 0; i < header->var_count; ++i)
    {
        const struct ctab_var *var = &vars[i];

        if (var->register_set != CTAB_REGISTER_SET_FLOAT4)
            continue;

        if (SIZE_MAX - allocator->reg_count < var->bind_count || UINT32_MAX - var->index < var->bind_count
                || !vkd3d_array_reserve((void **)&allocator->regs, &allocator->regs_capacity,
                allocator->reg_count + var->bind_count, sizeof(*allocator->regs)))
        {
            vkd3d_free(allocator->regs);
            return false;
        }

        for (uint32_t j = 0; j < var->bind_count; ++j)
        {
            allocator->regs[allocator->reg_count].index = var->index + j;
            allocator->regs[allocator->reg_count].uniform = true;
            ++allocator->reg_count;
        }
    }

    return true;
}

static struct const_reg *d3dbc_const_allocator_find_register(const struct d3dbc_const_allocator *allocator,
        unsigned int index)
{
    for (size_t i = 0; i < allocator->reg_count; ++i)
    {
        if (allocator->regs[i].index == index)
            return &allocator->regs[i];
    }

    return NULL;
}

static bool d3dbc_const_allocator_allocate_register(struct d3dbc_const_allocator *allocator,
        unsigned int component_count, const float *value, unsigned int *ret_index, uint32_t *ret_mask)
{
    /* Try to find one we can use already first.
     * FIXME: We could be more sophisticated here,
     * looking for swizzled versions of the same constants. */
    for (size_t i = 0; i < allocator->reg_count; ++i)
    {
        uint32_t mask = vkd3d_write_mask_from_component_count(component_count);
        const struct const_reg *reg = &allocator->regs[i];

        if (reg->uniform)
            continue;

        for (size_t j = 0; j <= 4 - component_count; ++j)
        {
            if ((reg->allocated_mask & mask) == mask
                    && !memcmp(value, &reg->value[j], component_count * sizeof(float)))
            {
                TRACE("Reusing constant c%u%s.\n", reg->index, debug_vsir_writemask(mask));
                *ret_index = reg->index;
                *ret_mask = mask;
                return true;
            }

            mask <<= 1;
        }
    }

    *ret_mask = 0;

    for (unsigned int index = 0;; ++index)
    {
        uint32_t available_mask;
        struct const_reg *reg;
        uint32_t mask = 0;

        if ((reg = d3dbc_const_allocator_find_register(allocator, index)))
        {
            if (reg->uniform)
                continue;

            available_mask = VKD3DSP_WRITEMASK_ALL & ~reg->allocated_mask;
            if (vkd3d_popcount(mask) < component_count)
                continue;
        }
        else
        {
            available_mask = VKD3DSP_WRITEMASK_ALL;

            if (!vkd3d_array_reserve((void **)&allocator->regs, &allocator->regs_capacity,
                    allocator->reg_count + 1, sizeof(*allocator->regs)))
            {
                *ret_index = *ret_mask = 0;
                return false;
            }
            reg = &allocator->regs[allocator->reg_count++];
            reg->index = index;
            reg->uniform = false;
            reg->allocated_mask = 0;
            memset(reg->value, 0, sizeof(reg->value));
        }

        for (unsigned int j = 0; j < component_count; ++j)
        {
            unsigned int component = vkd3d_ctz(available_mask);

            available_mask &= ~(1u << component);
            *ret_mask |= (1u << component);
            reg->value[component] = value[j];
        }
        reg->allocated_mask |= *ret_mask;
        *ret_index = index;
        TRACE("Allocated constant c%u%s.\n", index, debug_vsir_writemask(*ret_mask));
        return true;
    }
}

struct d3dbc_compiler
{
    const struct vkd3d_sm1_opcode_info *opcode_table;
    struct vsir_program *program;
    struct vkd3d_bytecode_buffer buffer;
    struct vkd3d_shader_message_context *message_context;
    struct vkd3d_shader_location null_location;
    struct d3dbc_const_allocator const_allocator, constint_allocator;
    bool failed;

    struct
    {
        struct d3dbc_instruction ins;
        struct vkd3d_shader_location location;
    } *instructions;
    size_t instruction_count, instructions_capacity;
};

static uint32_t sm1_version(enum vkd3d_shader_type type, unsigned int major, unsigned int minor)
{
    return vkd3d_make_u32(vkd3d_make_u16(minor, major),
            type == VKD3D_SHADER_TYPE_VERTEX ? VKD3D_SM1_VS : VKD3D_SM1_PS);
}

static const struct vkd3d_sm1_opcode_info *shader_sm1_get_opcode_info_from_vsir(
        struct d3dbc_compiler *compiler, enum vsir_opcode vsir_opcode)
{
    const struct vkd3d_shader_version *version = &compiler->program->shader_version;
    const struct vkd3d_sm1_opcode_info *info;
    unsigned int i = 0;

    for (;;)
    {
        info = &compiler->opcode_table[i++];
        if (info->vsir_opcode == VSIR_OP_INVALID)
            return NULL;

        if (vsir_opcode == info->vsir_opcode
                && vkd3d_shader_ver_ge(version, info->min_version.major, info->min_version.minor)
                && (vkd3d_shader_ver_le(version, info->max_version.major, info->max_version.minor)
                        || !info->max_version.major))
            return info;
    }
}

static const struct vkd3d_sm1_opcode_info *shader_sm1_get_opcode_info_from_vsir_instruction(
        struct d3dbc_compiler *compiler, const struct vsir_instruction *ins)
{
    const struct vkd3d_sm1_opcode_info *info;

    if (!(info = shader_sm1_get_opcode_info_from_vsir(compiler, ins->opcode)))
    {
        vkd3d_shader_error(compiler->message_context, &ins->location, VKD3D_SHADER_ERROR_D3DBC_INVALID_OPCODE,
                "Instruction \"%s\" (%#x) is not supported for the current target.",
                vsir_opcode_get_name(ins->opcode, "<unknown>"), ins->opcode);
        compiler->failed = true;
        return NULL;
    }

    if (ins->dst_count != info->dst_count)
    {
        vkd3d_shader_error(compiler->message_context, &ins->location, VKD3D_SHADER_ERROR_D3DBC_INVALID_REGISTER_COUNT,
                "Invalid destination parameter count %zu for instruction \"%s\" (%#x); expected %u.",
                ins->dst_count, vsir_opcode_get_name(ins->opcode, "<unknown>"), ins->opcode, info->dst_count);
        compiler->failed = true;
        return NULL;
    }
    if (ins->src_count != info->src_count)
    {
        vkd3d_shader_error(compiler->message_context, &ins->location, VKD3D_SHADER_ERROR_D3DBC_INVALID_REGISTER_COUNT,
                "Invalid source parameter count %zu for instruction \"%s\" (%#x); expected %u.",
                ins->src_count, vsir_opcode_get_name(ins->opcode, "<unknown>"), ins->opcode, info->src_count);
        compiler->failed = true;
        return NULL;
    }

    return info;
}

static void d3dbc_write_comment(struct d3dbc_compiler *compiler,
        uint32_t tag, const struct vkd3d_shader_code *comment)
{
    struct vkd3d_bytecode_buffer *buffer = &compiler->buffer;
    size_t offset, start, end;

    offset = put_u32(buffer, 0);

    start = put_u32(buffer, tag);
    bytecode_put_bytes(buffer, comment->code, comment->size);
    end = bytecode_align(buffer);

    set_u32(buffer, offset, vkd3d_make_u32(VKD3D_SM1_OP_COMMENT, (end - start) / sizeof(uint32_t)));
}

static enum vkd3d_sm1_register_type get_bytecode_register_type(const struct d3dbc_operand *reg)
{
    if (reg->type == D3DBC_REGISTER_CONST)
    {
        if (reg->index >= 6144)
            return VKD3D_SM1_REG_CONST4;
        if (reg->index >= 4096)
            return VKD3D_SM1_REG_CONST3;
        if (reg->index >= 2048)
            return VKD3D_SM1_REG_CONST2;
    }

    for (unsigned int i = 0; i < ARRAY_SIZE(register_types); ++i)
    {
        if (register_types[i].vsir_type == reg->type)
            return register_types[i].d3dbc_type;
    }

    vkd3d_unreachable();
}

static uint32_t sm1_encode_register_type(const struct d3dbc_operand *reg)
{
    enum vkd3d_sm1_register_type sm1_type = get_bytecode_register_type(reg);

    return ((sm1_type << VKD3D_SM1_REGISTER_TYPE_SHIFT) & VKD3D_SM1_REGISTER_TYPE_MASK)
            | ((sm1_type << VKD3D_SM1_REGISTER_TYPE_SHIFT2) & VKD3D_SM1_REGISTER_TYPE_MASK2);
}

static uint32_t swizzle_from_vsir(uint32_t swizzle)
{
    uint32_t x = vsir_swizzle_get_component(swizzle, 0);
    uint32_t y = vsir_swizzle_get_component(swizzle, 1);
    uint32_t z = vsir_swizzle_get_component(swizzle, 2);
    uint32_t w = vsir_swizzle_get_component(swizzle, 3);

    if (x & ~0x3u || y & ~0x3u || z & ~0x3u || w & ~0x3u)
        ERR("Unexpected vsir swizzle: 0x%08x.\n", swizzle);

    return ((x & 0x3u) << VKD3D_SM1_SWIZZLE_COMPONENT_SHIFT(0))
            | ((y & 0x3) << VKD3D_SM1_SWIZZLE_COMPONENT_SHIFT(1))
            | ((z & 0x3) << VKD3D_SM1_SWIZZLE_COMPONENT_SHIFT(2))
            | ((w & 0x3) << VKD3D_SM1_SWIZZLE_COMPONENT_SHIFT(3));
}

static bool is_inconsequential_instr(const struct vsir_instruction *ins)
{
    const struct vsir_src_operand *src = &ins->src[0];
    const struct vsir_dst_operand *dst = &ins->dst[0];
    unsigned int i;

    if (ins->opcode != VSIR_OP_MOV)
        return false;
    if (dst->modifiers != VKD3DSPDM_NONE)
        return false;
    if (src->modifiers != VKD3DSPSM_NONE)
        return false;
    if (src->reg.type != dst->reg.type)
        return false;
    if (src->reg.idx[0].offset != dst->reg.idx[0].offset)
        return false;

    for (i = 0; i < 4; ++i)
    {
        if ((dst->write_mask & (1u << i)) && (vsir_swizzle_get_component(src->swizzle, i) != i))
            return false;
    }

    return true;
}

static void write_sm1_dst_register(struct vkd3d_bytecode_buffer *buffer, const struct d3dbc_dst_operand *reg)
{
    VKD3D_ASSERT(reg->write_mask);
    put_u32(buffer, VKD3D_SM1_INSTRUCTION_PARAMETER
            | sm1_encode_register_type(&reg->o)
            | (reg->modifiers << VKD3D_SM1_DST_MODIFIER_SHIFT)
            | (reg->write_mask << VKD3D_SM1_WRITEMASK_SHIFT)
            | (reg->o.index & VKD3D_SM1_REGISTER_NUMBER_MASK));
}

static void write_sm1_src_register(struct vkd3d_bytecode_buffer *buffer, const struct d3dbc_src_operand *reg)
{
    uint32_t address_mode = VKD3D_SM1_ADDRESS_MODE_ABSOLUTE;

    if (reg->o.has_rel_addr)
        address_mode = VKD3D_SM1_ADDRESS_MODE_RELATIVE;

    put_u32(buffer, VKD3D_SM1_INSTRUCTION_PARAMETER
            | sm1_encode_register_type(&reg->o)
            | (address_mode << VKD3D_SM1_ADDRESS_MODE_SHIFT)
            | (reg->modifiers << VKD3D_SM1_SRC_MODIFIER_SHIFT)
            | (swizzle_from_vsir(reg->swizzle) << VKD3D_SM1_SWIZZLE_SHIFT)
            | (reg->o.index & VKD3D_SM1_REGISTER_NUMBER_MASK));
}

static void validate_register_limits(struct d3dbc_compiler *compiler,
        const struct d3dbc_operand *reg, const struct vkd3d_shader_location *loc)
{
    const struct vkd3d_shader_version *version = &compiler->program->shader_version;
    uint32_t idx = reg->index;

    if (reg->type == D3DBC_REGISTER_TEMP)
    {
        if ((version->type == VKD3D_SHADER_TYPE_PIXEL && vkd3d_shader_ver_le(version, 1, 3) && idx >= 2)
                || (version->type == VKD3D_SHADER_TYPE_PIXEL && vkd3d_shader_ver_le(version, 1, 4) && idx >= 6)
                || (version->type == VKD3D_SHADER_TYPE_VERTEX && vkd3d_shader_ver_le(version, 2, 0) && idx >= 12)
                || idx >= 32)
        {
            vkd3d_shader_error(compiler->message_context, loc, VKD3D_SHADER_ERROR_VSIR_INVALID_INDEX,
                    "Register r%u exceeds limits for shader type %#x, version %u.%u.",
                    idx, version->type, version->major, version->minor);
            compiler->failed = true;
        }
    }
}

static void d3dbc_add_instruction(struct d3dbc_compiler *compiler,
        const struct d3dbc_instruction *ins, const struct vkd3d_shader_location *location)
{
    if (vkd3d_array_reserve((void **)&compiler->instructions, &compiler->instructions_capacity,
            compiler->instruction_count + 1, sizeof(*compiler->instructions)))
    {
        compiler->instructions[compiler->instruction_count].ins = *ins;
        compiler->instructions[compiler->instruction_count].location = *location;
        ++compiler->instruction_count;
    }
}

static void d3dbc_write_instruction(struct d3dbc_compiler *compiler,
        const struct d3dbc_instruction *ins, const struct vkd3d_shader_location *location)
{
    const struct vkd3d_shader_version *version = &compiler->program->shader_version;
    struct vkd3d_bytecode_buffer *buffer = &compiler->buffer;
    const struct d3dbc_src_operand *src;
    size_t size, token_position;
    unsigned int i;
    uint32_t token;

    token = ins->opcode;
    token |= VKD3D_SM1_INSTRUCTION_FLAGS_MASK & (ins->flags << VKD3D_SM1_INSTRUCTION_FLAGS_SHIFT);

    token_position = put_u32(buffer, 0);

    if (ins->opcode == VKD3D_SM1_OP_DCL)
    {
        put_u32(buffer, VKD3D_SM1_INSTRUCTION_PARAMETER
                | (ins->semantic.usage << VKD3D_SM1_DCL_USAGE_SHIFT)
                | (ins->semantic.usage_idx << VKD3D_SM1_DCL_USAGE_INDEX_SHIFT)
                | (ins->semantic.resource_type << VKD3D_SM1_RESOURCE_TYPE_SHIFT));

        write_sm1_dst_register(buffer, &ins->dst[0]);
    }
    else if (ins->opcode == VKD3D_SM1_OP_DEFB)
    {
        write_sm1_dst_register(buffer, &ins->dst[0]);
        put_u32(buffer, ins->immconst.b32);
    }
    else if (ins->opcode == VKD3D_SM1_OP_DEF || ins->opcode == VKD3D_SM1_OP_DEFI)
    {
        write_sm1_dst_register(buffer, &ins->dst[0]);
        bytecode_put_bytes(buffer, ins->immconst.f32, sizeof(ins->immconst.f32));
    }
    else
    {
        for (i = 0; i < ins->dst_count; ++i)
        {
            validate_register_limits(compiler, &ins->dst[i].o, location);

            if (ins->dst[i].o.has_rel_addr)
            {
                vkd3d_shader_error(compiler->message_context, location, VKD3D_SHADER_ERROR_D3DBC_NOT_IMPLEMENTED,
                        "Unhandled relative addressing on destination register.");
                compiler->failed = true;
            }
            write_sm1_dst_register(buffer, &ins->dst[i]);
        }

        for (i = 0; i < ins->src_count; ++i)
        {
            validate_register_limits(compiler, &ins->src[i].o, location);

            src = &ins->src[i];
            write_sm1_src_register(buffer, src);
            if (src->o.has_rel_addr)
                write_sm1_src_register(buffer, &ins->src_rel_addr[i]);
        }
    }

    if (version->major > 1)
    {
        size = (bytecode_get_size(buffer) - token_position) / sizeof(uint32_t);
        token |= ((size - 1) << VKD3D_SM1_INSTRUCTION_LENGTH_SHIFT);
    }

    set_u32(buffer, token_position, token);
};

static void d3dbc_src_operand_from_vsir(struct d3dbc_compiler *compiler, struct d3dbc_src_operand *d3dbc,
        struct d3dbc_src_operand *rel_addr, const struct vsir_src_operand *vsir);

static void d3dbc_operand_from_vsir(struct d3dbc_compiler *compiler, struct d3dbc_operand *d3dbc,
        struct d3dbc_src_operand *rel_addr, const struct vsir_operand *vsir)
{
    static const enum d3dbc_register_type types[] =
    {
        [VSIR_REGISTER_TEMP]             = D3DBC_REGISTER_TEMP,
        [VSIR_REGISTER_INPUT]            = D3DBC_REGISTER_INPUT,
        [VSIR_REGISTER_ADDR]             = D3DBC_REGISTER_ADDR,
        [VSIR_REGISTER_TEXTURE]          = D3DBC_REGISTER_TEXTURE,
        [VSIR_REGISTER_RASTOUT]          = D3DBC_REGISTER_RASTOUT,
        [VSIR_REGISTER_ATTROUT]          = D3DBC_REGISTER_ATTROUT,
        [VSIR_REGISTER_TEXCRDOUT]        = D3DBC_REGISTER_TEXCRDOUT,
        [VSIR_REGISTER_OUTPUT]           = D3DBC_REGISTER_OUTPUT,
        [VSIR_REGISTER_COLOROUT]         = D3DBC_REGISTER_COLOROUT,
        [VSIR_REGISTER_DEPTHOUT]         = D3DBC_REGISTER_DEPTHOUT,
        [VSIR_REGISTER_COMBINED_SAMPLER] = D3DBC_REGISTER_SAMPLER,
        [VSIR_REGISTER_LOOP]             = D3DBC_REGISTER_LOOP,
        [VSIR_REGISTER_MISCTYPE]         = D3DBC_REGISTER_MISCTYPE,
    };
    STATIC_ASSERT(ARRAY_SIZE(types) == 13); /* Ensure we don't accidentally make this array too big. */

    if (vsir->type == VSIR_REGISTER_CONSTBUFFER)
    {
        if (vsir->idx[1].offset == VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER)
            d3dbc->type = D3DBC_REGISTER_CONST;
        else if (vsir->idx[1].offset == VKD3D_SHADER_D3DBC_INT_CONSTANT_REGISTER)
            d3dbc->type = D3DBC_REGISTER_CONSTINT;
        else if (vsir->idx[1].offset == VKD3D_SHADER_D3DBC_BOOL_CONSTANT_REGISTER)
            d3dbc->type = D3DBC_REGISTER_CONSTBOOL;
        else
            vkd3d_unreachable();

        d3dbc->index = vsir->idx[2].offset;
        if ((d3dbc->has_rel_addr = vsir->idx[2].rel_addr))
            d3dbc_src_operand_from_vsir(compiler, rel_addr, NULL, vsir->idx[2].rel_addr);
        return;
    }

    VKD3D_ASSERT(vsir->type < ARRAY_SIZE(types));

    d3dbc->type = types[vsir->type];
    if (idx_count_from_reg_type(d3dbc->type))
    {
        d3dbc->index = vsir->idx[0].offset;
        if ((d3dbc->has_rel_addr = vsir->idx[0].rel_addr))
            d3dbc_src_operand_from_vsir(compiler, rel_addr, NULL, vsir->idx[0].rel_addr);
    }
    else
    {
        d3dbc->index = 0;
        d3dbc->has_rel_addr = false;
    }
}

static void d3dbc_src_operand_from_vsir(struct d3dbc_compiler *compiler, struct d3dbc_src_operand *d3dbc,
        struct d3dbc_src_operand *rel_addr, const struct vsir_src_operand *vsir)
{
    d3dbc_operand_from_vsir(compiler, &d3dbc->o, rel_addr, &vsir->reg);
    d3dbc->swizzle = vsir->swizzle;
    d3dbc->modifiers = vsir->modifiers;
}

static void d3dbc_dst_operand_from_vsir(struct d3dbc_compiler *compiler, struct d3dbc_dst_operand *d3dbc,
        struct d3dbc_src_operand *rel_addr, const struct vsir_dst_operand *vsir)
{
    d3dbc_operand_from_vsir(compiler, &d3dbc->o, rel_addr, &vsir->reg);
    d3dbc->write_mask = vsir->write_mask;
    d3dbc->modifiers = vsir->modifiers;
}

static void d3dbc_src_operand_from_immconst_f32(struct d3dbc_compiler *compiler,
        struct d3dbc_src_operand *d3dbc, const struct vsir_instruction *ins, unsigned int src_idx)
{
    uint32_t read_mask, mask;
    unsigned int component;
    float value[4];

    switch (ins->opcode)
    {
        /* Component-wise operations, determined by the dst write mask. */
        case VSIR_OP_ABS:
        case VSIR_OP_ADD:
        case VSIR_OP_CMP:
        case VSIR_OP_DSX:
        case VSIR_OP_DSY:
        case VSIR_OP_EXP:
        case VSIR_OP_FRC:
        case VSIR_OP_LOG:
        case VSIR_OP_MAD:
        case VSIR_OP_MAX:
        case VSIR_OP_MIN:
        case VSIR_OP_MOV:
        case VSIR_OP_MOVA:
        case VSIR_OP_MUL:
        case VSIR_OP_RCP:
        case VSIR_OP_RSQ:
        case VSIR_OP_SLT:
            read_mask = ins->dst[0].write_mask;
            break;

        case VSIR_OP_IFC:
        case VSIR_OP_SINCOS:
            /* d3dbc allows the arguments to be swizzled, but IMMCONST cannot
             * be swizzled, so they are .x or scalar. */
            read_mask = 0x1;
            break;

        case VSIR_OP_DP2ADD:
            /* Args 0 and 1 read .xy; arg 2 is scalar like IFC. */
            read_mask = (src_idx == 2 ? 0x1 : 0x3);
            break;

        case VSIR_OP_DP3:
            read_mask = 0x7;
            break;

        case VSIR_OP_DP4:
        case VSIR_OP_REP:
        case VSIR_OP_TEXLD:
        case VSIR_OP_TEXLDL:
            /* The number of coordinate components read by texture instructions
             * depends on the dimension of the resource, but may also depend on
             * whether the resource has a shadow format, which isn't even part
             * of the shader. In that case we probably want all four components
             * regardless. */
            read_mask = 0xf;
            break;

        case VSIR_OP_TEXLDD:
            /* Gradient components read the first 2 components. */
            read_mask = (src_idx >= 2 ? 0x3 : 0xf);
            break;

        default:
            vkd3d_unreachable();
    }

    component = 0;
    for (unsigned int i = 0; i < 4; ++i)
    {
        if (read_mask & (1u << i))
            value[component++] = ins->src[src_idx].reg.u.immconst_f32[i];
    }

    if (!d3dbc_const_allocator_allocate_register(&compiler->const_allocator,
            component, value, &d3dbc->o.index, &mask))
        compiler->failed = true;
    d3dbc->o.type = D3DBC_REGISTER_CONST;
    d3dbc->o.has_rel_addr = false;
    d3dbc->modifiers = 0;
    d3dbc->swizzle = vsir_map_swizzle(vsir_swizzle_from_writemask(mask), read_mask);
}

static void d3dbc_src_operand_from_immconst_i32(struct d3dbc_compiler *compiler,
        struct d3dbc_src_operand *d3dbc, const struct vsir_src_operand *vsir)
{
    float value[4];
    uint32_t mask; /* always ALL, so we don't really need this */

    for (unsigned int i = 0; i < 4; ++i)
    {
        value[i] = vsir->reg.u.immconst_f32[i];
    }
    if (!d3dbc_const_allocator_allocate_register(&compiler->constint_allocator,
            4, value, &d3dbc->o.index, &mask))
        compiler->failed = true;
    d3dbc->o.type = D3DBC_REGISTER_CONSTINT;
    d3dbc->o.has_rel_addr = false;
    /* The only instruction that can take IMMCONST is REP, which must have
     * default swizzle. */
    d3dbc->swizzle = VKD3D_SHADER_NO_SWIZZLE;
    d3dbc->modifiers = 0;
}

static void d3dbc_write_sincos(struct d3dbc_compiler *compiler, const struct vsir_instruction *vsir)
{
    struct d3dbc_instruction d3dbc = {0};

    d3dbc.opcode = VKD3D_SM1_OP_SINCOS;
    d3dbc.dst_count = 1;
    d3dbc.src_count = 1;
    d3dbc_dst_operand_from_vsir(compiler, &d3dbc.dst[0], &d3dbc.dst_rel_addr[0], &vsir->dst[0]);
    d3dbc_src_operand_from_vsir(compiler, &d3dbc.src[0], &d3dbc.src_rel_addr[0], &vsir->src[0]);

    if (compiler->program->shader_version.major < 3)
    {
        /* 2.x sincos requires some constants to be embedded in the program,
         * which will be passed as extra arguments to the instruction. */
        static const float const1[4] = {-1.55009923e-06f, -2.17013894e-05f, 2.60416674e-03f, 2.60416680e-04f};
        static const float const2[4] = {-2.08333340e-02f, -1.25000000e-01f, 1.00000000e+00f, 5.00000000e-01f};
        unsigned int mask; /* always ALL, so we don't really need this */

        d3dbc.src_count = 3;
        d3dbc.src[1].o.type = D3DBC_REGISTER_CONST;
        if (!d3dbc_const_allocator_allocate_register(&compiler->const_allocator,
                4, const1, &d3dbc.src[1].o.index, &mask))
            compiler->failed = true;
        d3dbc.src[1].swizzle = VKD3D_SHADER_NO_SWIZZLE;
        d3dbc.src[2].o.type = D3DBC_REGISTER_CONST;
        if (!d3dbc_const_allocator_allocate_register(&compiler->const_allocator,
                4, const2, &d3dbc.src[2].o.index, &mask))
            compiler->failed = true;
        d3dbc.src[2].swizzle = VKD3D_SHADER_NO_SWIZZLE;
    }

    d3dbc_add_instruction(compiler, &d3dbc, &vsir->location);
}

static void d3dbc_write_texkill(struct d3dbc_compiler *compiler, const struct vsir_instruction *ins)
{
    struct d3dbc_instruction d3dbc =
    {
        .opcode = VKD3D_SM1_OP_TEXKILL,
        .dst_count = 1,
    };

    /* TEXKILL, uniquely, encodes its argument as a destination, when it is
     * semantically a source. We store it as a source in vsir, so convert it. */

    d3dbc_operand_from_vsir(compiler, &d3dbc.dst[0].o, &d3dbc.dst_rel_addr[0], &ins->src[0].reg);
    d3dbc.dst[0].modifiers = 0;
    d3dbc.dst[0].write_mask = mask_from_swizzle(ins->src[0].swizzle);

    d3dbc_add_instruction(compiler, &d3dbc, &ins->location);
}

static void d3dbc_write_vsir_sampler_dcl(struct d3dbc_compiler *compiler,
        unsigned int reg_id, enum vkd3d_sm1_resource_type res_type, const struct vkd3d_shader_location *location)
{
    struct d3dbc_instruction ins = {0};

    ins.opcode = VKD3D_SM1_OP_DCL;
    ins.semantic.resource_type = res_type;
    ins.dst_count = 1;
    ins.dst[0].o.type = D3DBC_REGISTER_SAMPLER;
    ins.dst[0].o.index = reg_id;
    ins.dst[0].write_mask = VKD3DSP_WRITEMASK_ALL;
    d3dbc_add_instruction(compiler, &ins, location);
}

static void d3dbc_write_vsir_dcl(struct d3dbc_compiler *compiler, const struct vsir_instruction *ins)
{
    const struct vkd3d_shader_version *version = &compiler->program->shader_version;
    const struct vkd3d_shader_semantic *semantic = &ins->declaration.semantic;
    unsigned int reg_id;

    if (version->major < 2)
        return;

    reg_id = semantic->resource.reg.reg.idx[0].offset;

    if (semantic->resource.reg.reg.type != VSIR_REGISTER_COMBINED_SAMPLER)
    {
        vkd3d_shader_error(compiler->message_context, &ins->location, VKD3D_SHADER_ERROR_D3DBC_INVALID_REGISTER_TYPE,
                "dcl instruction with register type %u.", semantic->resource.reg.reg.type);
        compiler->failed = true;
        return;
    }

    switch (semantic->resource_type)
    {
        case VKD3D_SHADER_RESOURCE_TEXTURE_2D:
            d3dbc_write_vsir_sampler_dcl(compiler, reg_id, VKD3D_SM1_RESOURCE_TEXTURE_2D, &ins->location);
            break;

        case VKD3D_SHADER_RESOURCE_TEXTURE_CUBE:
            d3dbc_write_vsir_sampler_dcl(compiler, reg_id, VKD3D_SM1_RESOURCE_TEXTURE_CUBE, &ins->location);
            break;

        case VKD3D_SHADER_RESOURCE_TEXTURE_3D:
            d3dbc_write_vsir_sampler_dcl(compiler, reg_id, VKD3D_SM1_RESOURCE_TEXTURE_3D, &ins->location);
            break;

        default:
            vkd3d_shader_error(compiler->message_context, &ins->location, VKD3D_SHADER_ERROR_D3DBC_INVALID_RESOURCE_TYPE,
                   "dcl instruction with resource_type %u.", semantic->resource_type);
            compiler->failed = true;
            return;
    }
}

static void d3dbc_write_vsir_instruction(struct d3dbc_compiler *compiler, const struct vsir_instruction *ins)
{
    const struct vkd3d_sm1_opcode_info *info;
    struct d3dbc_instruction d3dbc = {0};
    uint32_t writemask;

    switch (ins->opcode)
    {
        case VSIR_OP_NOP:
            break;

        case VSIR_OP_DCL:
            d3dbc_write_vsir_dcl(compiler, ins);
            break;

        case VSIR_OP_SINCOS:
            d3dbc_write_sincos(compiler, ins);
            break;

        case VSIR_OP_TEXKILL:
            d3dbc_write_texkill(compiler, ins);
            break;

        case VSIR_OP_EXP:
        case VSIR_OP_LOG:
        case VSIR_OP_RCP:
        case VSIR_OP_RSQ:
            writemask = ins->dst->write_mask;
            if (writemask != VKD3DSP_WRITEMASK_0 && writemask != VKD3DSP_WRITEMASK_1
                    && writemask != VKD3DSP_WRITEMASK_2 && writemask != VKD3DSP_WRITEMASK_3)
            {
                vkd3d_shader_error(compiler->message_context, &ins->location,
                        VKD3D_SHADER_ERROR_D3DBC_INVALID_WRITEMASK,
                        "Writemask %#x for instruction \"%s\" (%#x) is not single component.",
                        writemask, vsir_opcode_get_name(ins->opcode, "<unknown>"), ins->opcode);
                compiler->failed = true;
            }
            /* fall through */
        case VSIR_OP_ABS:
        case VSIR_OP_ADD:
        case VSIR_OP_BREAK:
        case VSIR_OP_CMP:
        case VSIR_OP_DP2ADD:
        case VSIR_OP_DP3:
        case VSIR_OP_DP4:
        case VSIR_OP_DSX:
        case VSIR_OP_DSY:
        case VSIR_OP_ELSE:
        case VSIR_OP_ENDIF:
        case VSIR_OP_ENDREP:
        case VSIR_OP_FRC:
        case VSIR_OP_IFC:
        case VSIR_OP_MAD:
        case VSIR_OP_MAX:
        case VSIR_OP_MIN:
        case VSIR_OP_MOV:
        case VSIR_OP_MOVA:
        case VSIR_OP_MUL:
        case VSIR_OP_REP:
        case VSIR_OP_SLT:
        case VSIR_OP_TEXLD:
        case VSIR_OP_TEXLDL:
        case VSIR_OP_TEXLDD:
            if (is_inconsequential_instr(ins))
                return;

            if (!(info = shader_sm1_get_opcode_info_from_vsir_instruction(compiler, ins)))
                return;

            d3dbc.opcode = info->sm1_opcode;
            d3dbc.flags = ins->flags;
            d3dbc.src_count = ins->src_count;
            d3dbc.dst_count = ins->dst_count;

            if (ins->dst_count)
                d3dbc_dst_operand_from_vsir(compiler, &d3dbc.dst[0], &d3dbc.dst_rel_addr[0], &ins->dst[0]);
            for (unsigned int i = 0; i < ins->src_count; ++i)
            {
                if (ins->src[i].reg.type == VSIR_REGISTER_IMMCONST && ins->src[i].reg.data_type == VSIR_DATA_F32)
                    d3dbc_src_operand_from_immconst_f32(compiler, &d3dbc.src[i], ins, i);
                else if (ins->src[i].reg.type == VSIR_REGISTER_IMMCONST && ins->src[i].reg.data_type == VSIR_DATA_I32)
                    d3dbc_src_operand_from_immconst_i32(compiler, &d3dbc.src[i], &ins->src[i]);
                else
                    d3dbc_src_operand_from_vsir(compiler, &d3dbc.src[i], &d3dbc.src_rel_addr[i], &ins->src[i]);
            }

            d3dbc_add_instruction(compiler, &d3dbc, &ins->location);
            break;

        default:
            vkd3d_shader_error(compiler->message_context, &ins->location, VKD3D_SHADER_ERROR_D3DBC_INVALID_OPCODE,
                   "Internal compiler error: Unhandled instruction \"%s\" (%#x).",
                   vsir_opcode_get_name(ins->opcode, "<unknown>"), ins->opcode);
            compiler->failed = true;
            break;
    }
}

static void d3dbc_write_semantic_dcl(struct d3dbc_compiler *compiler,
        const struct vsir_signature_element *element, bool output)
{
    const struct vkd3d_shader_version *version = &compiler->program->shader_version;
    struct d3dbc_instruction ins = {0};
    struct vsir_dst_operand reg = {0};
    bool ret;

    ins.opcode = VKD3D_SM1_OP_DCL;
    ins.dst_count = 1;

    reg.reg.idx_count = 1;
    if (!sm1_register_from_semantic_name(version, element->semantic_name,
            element->semantic_index, output, NULL, &reg.reg.type, &reg.reg.idx[0].offset))
    {
        ret = sm1_usage_from_semantic_name(version, element->semantic_name,
                element->semantic_index, output, &ins.semantic.usage, &ins.semantic.usage_idx);
        VKD3D_ASSERT(ret);
        reg.reg.type = output ? VSIR_REGISTER_OUTPUT : VSIR_REGISTER_INPUT;
        reg.reg.idx[0].offset = element->register_index;
        if ((version->type == VKD3D_SHADER_TYPE_PIXEL || output) && !vkd3d_shader_ver_ge(version, 3, 0))
        {
            if (reg.reg.idx[0].offset >= SM1_RASTOUT_REGISTER_OFFSET)
                reg.reg.idx[0].offset -= SM1_RASTOUT_REGISTER_OFFSET;
            else if (reg.reg.idx[0].offset >= SM1_COLOR_REGISTER_OFFSET)
                reg.reg.idx[0].offset -= SM1_COLOR_REGISTER_OFFSET;
        }
    }

    reg.write_mask = element->mask;
    if (element->interpolation_mode == VKD3DSIM_LINEAR_CENTROID)
        reg.modifiers |= VKD3DSPDM_MSAMPCENTROID;
    d3dbc_dst_operand_from_vsir(compiler, &ins.dst[0], &ins.dst_rel_addr[0], &reg);
    d3dbc_write_instruction(compiler, &ins, &compiler->null_location);
}

static void d3dbc_write_semantic_dcls(struct d3dbc_compiler *compiler)
{
    struct vsir_program *program = compiler->program;
    const struct vkd3d_shader_version *version;
    bool write_in = false, write_out = false;

    version = &program->shader_version;
    if (version->type == VKD3D_SHADER_TYPE_PIXEL && version->major >= 2)
        write_in = true;
    else if (version->type == VKD3D_SHADER_TYPE_VERTEX && version->major == 3)
        write_in = write_out = true;
    else if (version->type == VKD3D_SHADER_TYPE_VERTEX && version->major < 3)
        write_in = true;

    if (write_in)
    {
        for (unsigned int i = 0; i < program->input_signature.element_count; ++i)
            d3dbc_write_semantic_dcl(compiler, &program->input_signature.elements[i], false);
    }

    if (write_out)
    {
        for (unsigned int i = 0; i < program->output_signature.element_count; ++i)
            d3dbc_write_semantic_dcl(compiler, &program->output_signature.elements[i], true);
    }
}

static void d3dbc_instructions_from_vsir(struct d3dbc_compiler *compiler)
{
    struct vsir_program_iterator it = vsir_program_iterator(&compiler->program->instructions);
    struct vsir_instruction *ins;

    /* Preallocate at least as many d3dbc instructions as there are vsir instructions. */
    vkd3d_array_reserve((void **)&compiler->instructions, &compiler->instructions_capacity,
            compiler->program->instructions.count, sizeof(*compiler->instructions));

    for (ins = vsir_program_iterator_head(&it); ins; ins = vsir_program_iterator_next(&it))
    {
        d3dbc_write_vsir_instruction(compiler, ins);
    }
}

static void d3dbc_write_const_defs(struct d3dbc_compiler *compiler, enum vkd3d_sm1_opcode opcode,
        enum d3dbc_register_type type, const struct d3dbc_const_allocator *allocator)
{
    struct d3dbc_instruction ins = {0};

    for (size_t i = 0; i < allocator->reg_count; ++i)
    {
        const struct const_reg *reg = &allocator->regs[i];

        if (reg->uniform)
            continue;

        ins.opcode = opcode;
        ins.dst_count = 1;
        ins.dst[0].o.type = type;
        ins.dst[0].o.index = reg->index;
        ins.dst[0].write_mask = VKD3DSP_WRITEMASK_ALL;
        memcpy(ins.immconst.f32, reg->value, sizeof(ins.immconst.f32));
        d3dbc_write_instruction(compiler, &ins, &compiler->null_location);
    }
}

int d3dbc_compile(struct vsir_program *program, uint64_t config_flags,
        const struct vkd3d_shader_compile_info *compile_info, const struct vkd3d_shader_code *ctab,
        struct vkd3d_shader_code *out, struct vkd3d_shader_message_context *message_context)
{
    const struct vkd3d_shader_version *version = &program->shader_version;
    struct vkd3d_bytecode_buffer *buffer;
    struct d3dbc_compiler compiler;
    int result;

    if ((result = vsir_program_optimize(program, config_flags, compile_info, message_context)))
        return result;

    if ((result = vsir_program_allocate_temp_registers(program, config_flags, compile_info, message_context)) < 0)
        return result;

    compiler = (struct d3dbc_compiler)
    {
        .program = program,
        .message_context = message_context,
        .null_location = {.source_name = compile_info->source_name},
    };
    buffer = &compiler.buffer;

    switch (version->type)
    {
        case VKD3D_SHADER_TYPE_VERTEX:
            compiler.opcode_table = vs_opcode_table;
            break;

        case VKD3D_SHADER_TYPE_PIXEL:
            compiler.opcode_table = ps_opcode_table;
            break;

        default:
            vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_D3DBC_INVALID_PROFILE,
                    "Invalid shader type %u.", version->type);
            return VKD3D_ERROR_INVALID_SHADER;
    }

    if (!d3dbc_const_allocator_init(&compiler.const_allocator, ctab))
        compiler.failed = true;

    d3dbc_instructions_from_vsir(&compiler);

    put_u32(buffer, sm1_version(version->type, version->major, version->minor));
    d3dbc_write_comment(&compiler, TAG_CTAB, ctab);
    d3dbc_write_const_defs(&compiler, VKD3D_SM1_OP_DEF, D3DBC_REGISTER_CONST, &compiler.const_allocator);
    d3dbc_write_const_defs(&compiler, VKD3D_SM1_OP_DEFI, D3DBC_REGISTER_CONSTINT, &compiler.constint_allocator);
    d3dbc_write_semantic_dcls(&compiler);
    for (size_t i = 0; i < compiler.instruction_count; ++i)
        d3dbc_write_instruction(&compiler, &compiler.instructions[i].ins, &compiler.instructions[i].location);

    put_u32(buffer, VKD3D_SM1_OP_END);

    result = VKD3D_OK;
    if (buffer->status)
        result = buffer->status;
    if (compiler.failed)
        result = VKD3D_ERROR_INVALID_SHADER;

    vkd3d_free(compiler.instructions);
    if (!result)
        vkd3d_shader_code_from_bytecode_buffer(out, buffer);
    vkd3d_bytecode_buffer_cleanup(buffer);
    d3dbc_const_allocator_cleanup(&compiler.constint_allocator);
    d3dbc_const_allocator_cleanup(&compiler.const_allocator);

    return result;
}
