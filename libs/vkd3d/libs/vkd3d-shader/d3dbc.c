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
 * Copyright 2019-2020 Zebediah Figura for CodeWeavers
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

#include "hlsl.h"

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

struct vkd3d_sm1_opcode_info
{
    enum vkd3d_sm1_opcode sm1_opcode;
    unsigned int dst_count;
    unsigned int src_count;
    enum vkd3d_shader_opcode vkd3d_opcode;
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

    struct vkd3d_shader_parser p;

#define MAX_CONSTANT_COUNT 8192
    uint32_t constant_def_mask[3][MAX_CONSTANT_COUNT / 32];
};

/* This table is not order or position dependent. */
static const struct vkd3d_sm1_opcode_info vs_opcode_table[] =
{
    /* Arithmetic */
    {VKD3D_SM1_OP_NOP,          0, 0, VKD3DSIH_NOP},
    {VKD3D_SM1_OP_MOV,          1, 1, VKD3DSIH_MOV},
    {VKD3D_SM1_OP_MOVA,         1, 1, VKD3DSIH_MOVA,         {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ADD,          1, 2, VKD3DSIH_ADD},
    {VKD3D_SM1_OP_SUB,          1, 2, VKD3DSIH_SUB},
    {VKD3D_SM1_OP_MAD,          1, 3, VKD3DSIH_MAD},
    {VKD3D_SM1_OP_MUL,          1, 2, VKD3DSIH_MUL},
    {VKD3D_SM1_OP_RCP,          1, 1, VKD3DSIH_RCP},
    {VKD3D_SM1_OP_RSQ,          1, 1, VKD3DSIH_RSQ},
    {VKD3D_SM1_OP_DP3,          1, 2, VKD3DSIH_DP3},
    {VKD3D_SM1_OP_DP4,          1, 2, VKD3DSIH_DP4},
    {VKD3D_SM1_OP_MIN,          1, 2, VKD3DSIH_MIN},
    {VKD3D_SM1_OP_MAX,          1, 2, VKD3DSIH_MAX},
    {VKD3D_SM1_OP_SLT,          1, 2, VKD3DSIH_SLT},
    {VKD3D_SM1_OP_SGE,          1, 2, VKD3DSIH_SGE},
    {VKD3D_SM1_OP_ABS,          1, 1, VKD3DSIH_ABS},
    {VKD3D_SM1_OP_EXP,          1, 1, VKD3DSIH_EXP},
    {VKD3D_SM1_OP_LOG,          1, 1, VKD3DSIH_LOG},
    {VKD3D_SM1_OP_EXPP,         1, 1, VKD3DSIH_EXPP},
    {VKD3D_SM1_OP_LOGP,         1, 1, VKD3DSIH_LOGP},
    {VKD3D_SM1_OP_LIT,          1, 1, VKD3DSIH_LIT},
    {VKD3D_SM1_OP_DST,          1, 2, VKD3DSIH_DST},
    {VKD3D_SM1_OP_LRP,          1, 3, VKD3DSIH_LRP},
    {VKD3D_SM1_OP_FRC,          1, 1, VKD3DSIH_FRC},
    {VKD3D_SM1_OP_POW,          1, 2, VKD3DSIH_POW},
    {VKD3D_SM1_OP_CRS,          1, 2, VKD3DSIH_CRS},
    {VKD3D_SM1_OP_SGN,          1, 3, VKD3DSIH_SGN,          {2, 0}, {  2,   1}},
    {VKD3D_SM1_OP_SGN,          1, 1, VKD3DSIH_SGN,          {3, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_NRM,          1, 1, VKD3DSIH_NRM,},
    {VKD3D_SM1_OP_SINCOS,       1, 3, VKD3DSIH_SINCOS,       {2, 0}, {  2,   1}},
    {VKD3D_SM1_OP_SINCOS,       1, 1, VKD3DSIH_SINCOS,       {3, 0}, {~0u, ~0u}},
    /* Matrix */
    {VKD3D_SM1_OP_M4x4,         1, 2, VKD3DSIH_M4x4},
    {VKD3D_SM1_OP_M4x3,         1, 2, VKD3DSIH_M4x3},
    {VKD3D_SM1_OP_M3x4,         1, 2, VKD3DSIH_M3x4},
    {VKD3D_SM1_OP_M3x3,         1, 2, VKD3DSIH_M3x3},
    {VKD3D_SM1_OP_M3x2,         1, 2, VKD3DSIH_M3x2},
    /* Declarations */
    {VKD3D_SM1_OP_DCL,          0, 0, VKD3DSIH_DCL},
    /* Constant definitions */
    {VKD3D_SM1_OP_DEF,          1, 1, VKD3DSIH_DEF},
    {VKD3D_SM1_OP_DEFB,         1, 1, VKD3DSIH_DEFB},
    {VKD3D_SM1_OP_DEFI,         1, 1, VKD3DSIH_DEFI},
    /* Control flow */
    {VKD3D_SM1_OP_REP,          0, 1, VKD3DSIH_REP,          {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ENDREP,       0, 0, VKD3DSIH_ENDREP,       {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_IF,           0, 1, VKD3DSIH_IF,           {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_IFC,          0, 2, VKD3DSIH_IFC,          {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ELSE,         0, 0, VKD3DSIH_ELSE,         {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ENDIF,        0, 0, VKD3DSIH_ENDIF,        {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_BREAK,        0, 0, VKD3DSIH_BREAK,        {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_BREAKC,       0, 2, VKD3DSIH_BREAKC,       {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_BREAKP,       0, 1, VKD3DSIH_BREAKP},
    {VKD3D_SM1_OP_CALL,         0, 1, VKD3DSIH_CALL,         {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_CALLNZ,       0, 2, VKD3DSIH_CALLNZ,       {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_LOOP,         0, 2, VKD3DSIH_LOOP,         {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_RET,          0, 0, VKD3DSIH_RET,          {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ENDLOOP,      0, 0, VKD3DSIH_ENDLOOP,      {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_LABEL,        0, 1, VKD3DSIH_LABEL,        {2, 0}, {~0u, ~0u}},

    {VKD3D_SM1_OP_SETP,         1, 2, VKD3DSIH_SETP},
    {VKD3D_SM1_OP_TEXLDL,       1, 2, VKD3DSIH_TEXLDL,       {3, 0}, {~0u, ~0u}},
    {0,                         0, 0, VKD3DSIH_INVALID},
};

static const struct vkd3d_sm1_opcode_info ps_opcode_table[] =
{
    /* Arithmetic */
    {VKD3D_SM1_OP_NOP,          0, 0, VKD3DSIH_NOP},
    {VKD3D_SM1_OP_MOV,          1, 1, VKD3DSIH_MOV},
    {VKD3D_SM1_OP_ADD,          1, 2, VKD3DSIH_ADD},
    {VKD3D_SM1_OP_SUB,          1, 2, VKD3DSIH_SUB},
    {VKD3D_SM1_OP_MAD,          1, 3, VKD3DSIH_MAD},
    {VKD3D_SM1_OP_MUL,          1, 2, VKD3DSIH_MUL},
    {VKD3D_SM1_OP_RCP,          1, 1, VKD3DSIH_RCP},
    {VKD3D_SM1_OP_RSQ,          1, 1, VKD3DSIH_RSQ},
    {VKD3D_SM1_OP_DP3,          1, 2, VKD3DSIH_DP3},
    {VKD3D_SM1_OP_DP4,          1, 2, VKD3DSIH_DP4},
    {VKD3D_SM1_OP_MIN,          1, 2, VKD3DSIH_MIN},
    {VKD3D_SM1_OP_MAX,          1, 2, VKD3DSIH_MAX},
    {VKD3D_SM1_OP_SLT,          1, 2, VKD3DSIH_SLT},
    {VKD3D_SM1_OP_SGE,          1, 2, VKD3DSIH_SGE},
    {VKD3D_SM1_OP_ABS,          1, 1, VKD3DSIH_ABS},
    {VKD3D_SM1_OP_EXP,          1, 1, VKD3DSIH_EXP},
    {VKD3D_SM1_OP_LOG,          1, 1, VKD3DSIH_LOG},
    {VKD3D_SM1_OP_EXPP,         1, 1, VKD3DSIH_EXPP},
    {VKD3D_SM1_OP_LOGP,         1, 1, VKD3DSIH_LOGP},
    {VKD3D_SM1_OP_DST,          1, 2, VKD3DSIH_DST},
    {VKD3D_SM1_OP_LRP,          1, 3, VKD3DSIH_LRP},
    {VKD3D_SM1_OP_FRC,          1, 1, VKD3DSIH_FRC},
    {VKD3D_SM1_OP_CND,          1, 3, VKD3DSIH_CND,          {1, 0}, {  1,   4}},
    {VKD3D_SM1_OP_CMP,          1, 3, VKD3DSIH_CMP,          {1, 2}, {  3,   0}},
    {VKD3D_SM1_OP_POW,          1, 2, VKD3DSIH_POW},
    {VKD3D_SM1_OP_CRS,          1, 2, VKD3DSIH_CRS},
    {VKD3D_SM1_OP_NRM,          1, 1, VKD3DSIH_NRM},
    {VKD3D_SM1_OP_SINCOS,       1, 3, VKD3DSIH_SINCOS,       {2, 0}, {  2,   1}},
    {VKD3D_SM1_OP_SINCOS,       1, 1, VKD3DSIH_SINCOS,       {3, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_DP2ADD,       1, 3, VKD3DSIH_DP2ADD,       {2, 0}, {~0u, ~0u}},
    /* Matrix */
    {VKD3D_SM1_OP_M4x4,         1, 2, VKD3DSIH_M4x4},
    {VKD3D_SM1_OP_M4x3,         1, 2, VKD3DSIH_M4x3},
    {VKD3D_SM1_OP_M3x4,         1, 2, VKD3DSIH_M3x4},
    {VKD3D_SM1_OP_M3x3,         1, 2, VKD3DSIH_M3x3},
    {VKD3D_SM1_OP_M3x2,         1, 2, VKD3DSIH_M3x2},
    /* Declarations */
    {VKD3D_SM1_OP_DCL,          0, 0, VKD3DSIH_DCL},
    /* Constant definitions */
    {VKD3D_SM1_OP_DEF,          1, 1, VKD3DSIH_DEF},
    {VKD3D_SM1_OP_DEFB,         1, 1, VKD3DSIH_DEFB},
    {VKD3D_SM1_OP_DEFI,         1, 1, VKD3DSIH_DEFI},
    /* Control flow */
    {VKD3D_SM1_OP_REP,          0, 1, VKD3DSIH_REP,          {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ENDREP,       0, 0, VKD3DSIH_ENDREP,       {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_IF,           0, 1, VKD3DSIH_IF,           {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_IFC,          0, 2, VKD3DSIH_IFC,          {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ELSE,         0, 0, VKD3DSIH_ELSE,         {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ENDIF,        0, 0, VKD3DSIH_ENDIF,        {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_BREAK,        0, 0, VKD3DSIH_BREAK,        {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_BREAKC,       0, 2, VKD3DSIH_BREAKC,       {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_BREAKP,       0, 1, VKD3DSIH_BREAKP},
    {VKD3D_SM1_OP_CALL,         0, 1, VKD3DSIH_CALL,         {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_CALLNZ,       0, 2, VKD3DSIH_CALLNZ,       {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_LOOP,         0, 2, VKD3DSIH_LOOP,         {3, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_RET,          0, 0, VKD3DSIH_RET,          {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_ENDLOOP,      0, 0, VKD3DSIH_ENDLOOP,      {3, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_LABEL,        0, 1, VKD3DSIH_LABEL,        {2, 1}, {~0u, ~0u}},
    /* Texture */
    {VKD3D_SM1_OP_TEXCOORD,     1, 0, VKD3DSIH_TEXCOORD,     {0, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXCOORD,     1, 1, VKD3DSIH_TEXCOORD,     {1 ,4}, {  1,   4}},
    {VKD3D_SM1_OP_TEXKILL,      1, 0, VKD3DSIH_TEXKILL,      {1 ,0}, {  3,   0}},
    {VKD3D_SM1_OP_TEX,          1, 0, VKD3DSIH_TEX,          {0, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEX,          1, 1, VKD3DSIH_TEX,          {1, 4}, {  1,   4}},
    {VKD3D_SM1_OP_TEX,          1, 2, VKD3DSIH_TEX,          {2, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_TEXBEM,       1, 1, VKD3DSIH_TEXBEM,       {0, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXBEML,      1, 1, VKD3DSIH_TEXBEML,      {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXREG2AR,    1, 1, VKD3DSIH_TEXREG2AR,    {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXREG2GB,    1, 1, VKD3DSIH_TEXREG2GB,    {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXREG2RGB,   1, 1, VKD3DSIH_TEXREG2RGB,   {1, 2}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x2PAD,   1, 1, VKD3DSIH_TEXM3x2PAD,   {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x2TEX,   1, 1, VKD3DSIH_TEXM3x2TEX,   {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x3PAD,   1, 1, VKD3DSIH_TEXM3x3PAD,   {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x3DIFF,  1, 1, VKD3DSIH_TEXM3x3DIFF,  {0, 0}, {  0,   0}},
    {VKD3D_SM1_OP_TEXM3x3SPEC,  1, 2, VKD3DSIH_TEXM3x3SPEC,  {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x3VSPEC, 1, 1, VKD3DSIH_TEXM3x3VSPEC, {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x3TEX,   1, 1, VKD3DSIH_TEXM3x3TEX,   {1, 0}, {  1,   3}},
    {VKD3D_SM1_OP_TEXDP3TEX,    1, 1, VKD3DSIH_TEXDP3TEX,    {1, 2}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x2DEPTH, 1, 1, VKD3DSIH_TEXM3x2DEPTH, {1, 3}, {  1,   3}},
    {VKD3D_SM1_OP_TEXDP3,       1, 1, VKD3DSIH_TEXDP3,       {1, 2}, {  1,   3}},
    {VKD3D_SM1_OP_TEXM3x3,      1, 1, VKD3DSIH_TEXM3x3,      {1, 2}, {  1,   3}},
    {VKD3D_SM1_OP_TEXDEPTH,     1, 0, VKD3DSIH_TEXDEPTH,     {1, 4}, {  1,   4}},
    {VKD3D_SM1_OP_BEM,          1, 2, VKD3DSIH_BEM,          {1, 4}, {  1,   4}},
    {VKD3D_SM1_OP_DSX,          1, 1, VKD3DSIH_DSX,          {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_DSY,          1, 1, VKD3DSIH_DSY,          {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_TEXLDD,       1, 4, VKD3DSIH_TEXLDD,       {2, 1}, {~0u, ~0u}},
    {VKD3D_SM1_OP_SETP,         1, 2, VKD3DSIH_SETP},
    {VKD3D_SM1_OP_TEXLDL,       1, 2, VKD3DSIH_TEXLDL,       {3, 0}, {~0u, ~0u}},
    {VKD3D_SM1_OP_PHASE,        0, 0, VKD3DSIH_PHASE},
    {0,                         0, 0, VKD3DSIH_INVALID},
};

static const enum vkd3d_shader_resource_type resource_type_table[] =
{
    /* VKD3D_SM1_RESOURCE_UNKNOWN */      VKD3D_SHADER_RESOURCE_NONE,
    /* VKD3D_SM1_RESOURCE_TEXTURE_1D */   VKD3D_SHADER_RESOURCE_TEXTURE_1D,
    /* VKD3D_SM1_RESOURCE_TEXTURE_2D */   VKD3D_SHADER_RESOURCE_TEXTURE_2D,
    /* VKD3D_SM1_RESOURCE_TEXTURE_CUBE */ VKD3D_SHADER_RESOURCE_TEXTURE_CUBE,
    /* VKD3D_SM1_RESOURCE_TEXTURE_3D */   VKD3D_SHADER_RESOURCE_TEXTURE_3D,
};

static struct vkd3d_shader_sm1_parser *vkd3d_shader_sm1_parser(struct vkd3d_shader_parser *parser)
{
    return CONTAINING_RECORD(parser, struct vkd3d_shader_sm1_parser, p);
}

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
    const struct vkd3d_sm1_opcode_info *info;
    unsigned int i = 0;

    for (;;)
    {
        info = &sm1->opcode_table[i++];
        if (info->vkd3d_opcode == VKD3DSIH_INVALID)
            return NULL;

        if (opcode == info->sm1_opcode
                && vkd3d_shader_ver_ge(&sm1->p.shader_version, info->min_version.major, info->min_version.minor)
                && (vkd3d_shader_ver_le(&sm1->p.shader_version, info->max_version.major, info->max_version.minor)
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

static void shader_sm1_parse_src_param(uint32_t param, const struct vkd3d_shader_src_param *rel_addr,
        struct vkd3d_shader_src_param *src)
{
    enum vkd3d_shader_register_type reg_type = ((param & VKD3D_SM1_REGISTER_TYPE_MASK) >> VKD3D_SM1_REGISTER_TYPE_SHIFT)
            | ((param & VKD3D_SM1_REGISTER_TYPE_MASK2) >> VKD3D_SM1_REGISTER_TYPE_SHIFT2);

    vsir_register_init(&src->reg, reg_type, VKD3D_DATA_FLOAT, 1);
    src->reg.precision = VKD3D_SHADER_REGISTER_PRECISION_DEFAULT;
    src->reg.non_uniform = false;
    src->reg.idx[0].offset = param & VKD3D_SM1_REGISTER_NUMBER_MASK;
    src->reg.idx[0].rel_addr = rel_addr;
    if (src->reg.type == VKD3DSPR_SAMPLER)
        src->reg.dimension = VSIR_DIMENSION_NONE;
    else if (src->reg.type == VKD3DSPR_DEPTHOUT)
        src->reg.dimension = VSIR_DIMENSION_SCALAR;
    else
        src->reg.dimension = VSIR_DIMENSION_VEC4;
    src->swizzle = swizzle_from_sm1((param & VKD3D_SM1_SWIZZLE_MASK) >> VKD3D_SM1_SWIZZLE_SHIFT);
    src->modifiers = (param & VKD3D_SM1_SRC_MODIFIER_MASK) >> VKD3D_SM1_SRC_MODIFIER_SHIFT;
}

static void shader_sm1_parse_dst_param(uint32_t param, const struct vkd3d_shader_src_param *rel_addr,
        struct vkd3d_shader_dst_param *dst)
{
    enum vkd3d_shader_register_type reg_type = ((param & VKD3D_SM1_REGISTER_TYPE_MASK) >> VKD3D_SM1_REGISTER_TYPE_SHIFT)
            | ((param & VKD3D_SM1_REGISTER_TYPE_MASK2) >> VKD3D_SM1_REGISTER_TYPE_SHIFT2);

    vsir_register_init(&dst->reg, reg_type, VKD3D_DATA_FLOAT, 1);
    dst->reg.precision = VKD3D_SHADER_REGISTER_PRECISION_DEFAULT;
    dst->reg.non_uniform = false;
    dst->reg.idx[0].offset = param & VKD3D_SM1_REGISTER_NUMBER_MASK;
    dst->reg.idx[0].rel_addr = rel_addr;
    if (dst->reg.type == VKD3DSPR_SAMPLER)
        dst->reg.dimension = VSIR_DIMENSION_NONE;
    else if (dst->reg.type == VKD3DSPR_DEPTHOUT)
        dst->reg.dimension = VSIR_DIMENSION_SCALAR;
    else
        dst->reg.dimension = VSIR_DIMENSION_VEC4;
    dst->write_mask = (param & VKD3D_SM1_WRITEMASK_MASK) >> VKD3D_SM1_WRITEMASK_SHIFT;
    dst->modifiers = (param & VKD3D_SM1_DST_MODIFIER_MASK) >> VKD3D_SM1_DST_MODIFIER_SHIFT;
    dst->shift = (param & VKD3D_SM1_DSTSHIFT_MASK) >> VKD3D_SM1_DSTSHIFT_SHIFT;
}

static struct signature_element *find_signature_element(const struct shader_signature *signature,
        const char *semantic_name, unsigned int semantic_index)
{
    struct signature_element *e = signature->elements;
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        if (!ascii_strcasecmp(e[i].semantic_name, semantic_name)
                && e[i].semantic_index == semantic_index)
            return &e[i];
    }

    return NULL;
}

static struct signature_element *find_signature_element_by_register_index(
        const struct shader_signature *signature, unsigned int register_index)
{
    struct signature_element *e = signature->elements;
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        if (e[i].register_index == register_index)
            return &e[i];
    }

    return NULL;
}

#define SM1_COLOR_REGISTER_OFFSET 8

static bool add_signature_element(struct vkd3d_shader_sm1_parser *sm1, bool output,
        const char *name, unsigned int index, enum vkd3d_shader_sysval_semantic sysval,
        unsigned int register_index, bool is_dcl, unsigned int mask)
{
    struct shader_signature *signature;
    struct signature_element *element;

    if (output)
        signature = &sm1->p.shader_desc.output_signature;
    else
        signature = &sm1->p.shader_desc.input_signature;

    if ((element = find_signature_element(signature, name, index)))
    {
        element->mask |= mask;
        if (!is_dcl)
            element->used_mask |= mask;
        return true;
    }

    if (!vkd3d_array_reserve((void **)&signature->elements, &signature->elements_capacity,
            signature->element_count + 1, sizeof(*signature->elements)))
        return false;
    element = &signature->elements[signature->element_count++];

    memset(element, 0, sizeof(*element));
    element->semantic_name = name;
    element->semantic_index = index;
    element->sysval_semantic = sysval;
    element->component_type = VKD3D_SHADER_COMPONENT_FLOAT;
    element->register_index = register_index;
    element->target_location = register_index;
    element->register_count = 1;
    element->mask = mask;
    element->used_mask = is_dcl ? 0 : mask;
    if (sm1->p.shader_version.type == VKD3D_SHADER_TYPE_PIXEL && !output)
        element->interpolation_mode = VKD3DSIM_LINEAR;

    return true;
}

static void add_signature_mask(struct vkd3d_shader_sm1_parser *sm1, bool output,
        unsigned int register_index, unsigned int mask)
{
    struct shader_signature *signature;
    struct signature_element *element;

    if (output)
        signature = &sm1->p.shader_desc.output_signature;
    else
        signature = &sm1->p.shader_desc.input_signature;

    if (!(element = find_signature_element_by_register_index(signature, register_index)))
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNDECLARED_SEMANTIC,
                "%s register %u was used without being declared.", output ? "Output" : "Input", register_index);
        return;
    }

    element->used_mask |= mask;
}

static bool add_signature_element_from_register(struct vkd3d_shader_sm1_parser *sm1,
        const struct vkd3d_shader_register *reg, bool is_dcl, unsigned int mask)
{
    unsigned int register_index = reg->idx[0].offset;

    switch (reg->type)
    {
        case VKD3DSPR_TEMP:
            if (sm1->p.shader_version.type == VKD3D_SHADER_TYPE_PIXEL
                    && sm1->p.shader_version.major == 1 && !register_index)
                return add_signature_element(sm1, true, "COLOR", 0, VKD3D_SHADER_SV_TARGET, 0, is_dcl, mask);
            return true;

        case VKD3DSPR_INPUT:
            /* For vertex shaders or sm3 pixel shaders, we should have already
             * had a DCL instruction. Otherwise, this is a colour input. */
            if (sm1->p.shader_version.type == VKD3D_SHADER_TYPE_VERTEX || sm1->p.shader_version.major == 3)
            {
                add_signature_mask(sm1, false, register_index, mask);
                return true;
            }
            return add_signature_element(sm1, false, "COLOR", register_index,
                    VKD3D_SHADER_SV_NONE, SM1_COLOR_REGISTER_OFFSET + register_index, is_dcl, mask);

        case VKD3DSPR_TEXTURE:
            /* For vertex shaders, this is ADDR. */
            if (sm1->p.shader_version.type == VKD3D_SHADER_TYPE_VERTEX)
                return true;
            return add_signature_element(sm1, false, "TEXCOORD", register_index,
                    VKD3D_SHADER_SV_NONE, register_index, is_dcl, mask);

        case VKD3DSPR_OUTPUT:
            if (sm1->p.shader_version.type == VKD3D_SHADER_TYPE_VERTEX)
            {
                /* For sm < 2 vertex shaders, this is TEXCRDOUT.
                 *
                 * For sm3 vertex shaders, this is OUTPUT, but we already
                 * should have had a DCL instruction. */
                if (sm1->p.shader_version.major == 3)
                {
                    add_signature_mask(sm1, true, register_index, mask);
                    return true;
                }
                return add_signature_element(sm1, true, "TEXCOORD", register_index,
                        VKD3D_SHADER_SV_NONE, register_index, is_dcl, mask);
            }
            /* fall through */

        case VKD3DSPR_ATTROUT:
            return add_signature_element(sm1, true, "COLOR", register_index,
                    VKD3D_SHADER_SV_NONE, SM1_COLOR_REGISTER_OFFSET + register_index, is_dcl, mask);

        case VKD3DSPR_COLOROUT:
            return add_signature_element(sm1, true, "COLOR", register_index,
                    VKD3D_SHADER_SV_TARGET, register_index, is_dcl, mask);

        case VKD3DSPR_DEPTHOUT:
            return add_signature_element(sm1, true, "DEPTH", 0,
                    VKD3D_SHADER_SV_DEPTH, register_index, is_dcl, 0x1);

        case VKD3DSPR_RASTOUT:
            switch (register_index)
            {
                case 0:
                    return add_signature_element(sm1, true, "POSITION", 0,
                            VKD3D_SHADER_SV_POSITION, register_index, is_dcl, mask);

                case 1:
                    return add_signature_element(sm1, true, "FOG", 0,
                            VKD3D_SHADER_SV_NONE, register_index, is_dcl, 0x1);

                case 2:
                    return add_signature_element(sm1, true, "PSIZE", 0,
                            VKD3D_SHADER_SV_NONE, register_index, is_dcl, 0x1);

                default:
                    vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_REGISTER_INDEX,
                            "Invalid rasterizer output index %u.", register_index);
                    return true;
            }

        case VKD3DSPR_MISCTYPE:
            switch (register_index)
            {
                case 0:
                    return add_signature_element(sm1, false, "VPOS", 0,
                            VKD3D_SHADER_SV_POSITION, register_index, is_dcl, mask);

                case 1:
                    return add_signature_element(sm1, false, "VFACE", 0,
                            VKD3D_SHADER_SV_IS_FRONT_FACE, register_index, is_dcl, 0x1);

                default:
                    vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_REGISTER_INDEX,
                            "Invalid miscellaneous fragment input index %u.", register_index);
                    return true;
            }

        default:
            return true;
    }
}

static bool add_signature_element_from_semantic(struct vkd3d_shader_sm1_parser *sm1,
        const struct vkd3d_shader_semantic *semantic)
{
    const struct vkd3d_shader_register *reg = &semantic->resource.reg.reg;
    enum vkd3d_shader_sysval_semantic sysval = VKD3D_SHADER_SV_NONE;
    unsigned int mask = semantic->resource.reg.write_mask;
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

    if (reg->type == VKD3DSPR_OUTPUT)
        output = true;
    else if (reg->type == VKD3DSPR_INPUT || reg->type == VKD3DSPR_TEXTURE)
        output = false;
    else /* vpos and vface don't have a semantic. */
        return add_signature_element_from_register(sm1, reg, true, mask);

    /* sm2 pixel shaders use DCL but don't provide a semantic. */
    if (sm1->p.shader_version.type == VKD3D_SHADER_TYPE_PIXEL && sm1->p.shader_version.major == 2)
        return add_signature_element_from_register(sm1, reg, true, mask);

    /* With the exception of vertex POSITION output, none of these are system
     * values. Pixel POSITION input is not equivalent to SV_Position; the closer
     * equivalent is VPOS, which is not declared as a semantic. */
    if (sm1->p.shader_version.type == VKD3D_SHADER_TYPE_VERTEX
            && output && semantic->usage == VKD3D_DECL_USAGE_POSITION)
        sysval = VKD3D_SHADER_SV_POSITION;

    return add_signature_element(sm1, output, sm1_semantic_names[semantic->usage],
            semantic->usage_idx, sysval, reg->idx[0].offset, true, mask);
}

static void record_constant_register(struct vkd3d_shader_sm1_parser *sm1,
        enum vkd3d_shader_d3dbc_constant_register set, uint32_t index, bool from_def)
{
    struct vkd3d_shader_desc *desc = &sm1->p.shader_desc;

    desc->flat_constant_count[set].used = max(desc->flat_constant_count[set].used, index + 1);
    if (from_def)
    {
        /* d3d shaders have a maximum of 8192 constants; we should not overrun
         * this array. */
        assert((index / 32) <= ARRAY_SIZE(sm1->constant_def_mask[set]));
        bitmap_set(sm1->constant_def_mask[set], index);
    }
}

static void shader_sm1_scan_register(struct vkd3d_shader_sm1_parser *sm1,
        const struct vkd3d_shader_register *reg, unsigned int mask, bool from_def)
{
    struct vkd3d_shader_desc *desc = &sm1->p.shader_desc;
    uint32_t register_index = reg->idx[0].offset;

    switch (reg->type)
    {
        case VKD3DSPR_TEMP:
            desc->temp_count = max(desc->temp_count, register_index + 1);
            break;

        case VKD3DSPR_CONST:
            record_constant_register(sm1, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER, register_index, from_def);
            break;

        case VKD3DSPR_CONST2:
            record_constant_register(sm1, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER, 2048 + register_index, from_def);
            break;

        case VKD3DSPR_CONST3:
            record_constant_register(sm1, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER, 4096 + register_index, from_def);
            break;

        case VKD3DSPR_CONST4:
            record_constant_register(sm1, VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER, 6144 + register_index, from_def);
            break;

        case VKD3DSPR_CONSTINT:
            record_constant_register(sm1, VKD3D_SHADER_D3DBC_INT_CONSTANT_REGISTER, register_index, from_def);
            break;

        case VKD3DSPR_CONSTBOOL:
            record_constant_register(sm1, VKD3D_SHADER_D3DBC_BOOL_CONSTANT_REGISTER, register_index, from_def);
            break;

        default:
            break;
    }

    add_signature_element_from_register(sm1, reg, false, mask);
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
    if (sm1->p.shader_version.major < 2)
    {
        *addr_token = (1u << 31)
                | ((VKD3DSPR_ADDR << VKD3D_SM1_REGISTER_TYPE_SHIFT2) & VKD3D_SM1_REGISTER_TYPE_MASK2)
                | ((VKD3DSPR_ADDR << VKD3D_SM1_REGISTER_TYPE_SHIFT) & VKD3D_SM1_REGISTER_TYPE_MASK)
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
    if (sm1->p.shader_version.major >= 2)
    {
        length = (opcode_token & VKD3D_SM1_INSTRUCTION_LENGTH_MASK) >> VKD3D_SM1_INSTRUCTION_LENGTH_SHIFT;
        *ptr += length;
        return;
    }

    /* DCL instructions do not have sources or destinations, but they
     * read two tokens to a semantic. See
     * shader_sm1_read_semantic(). */
    if (opcode_info->vkd3d_opcode == VKD3DSIH_DCL)
    {
        *ptr += 2;
    }
    /* Somewhat similarly, DEF and DEFI have a single source, but need to read
     * four tokens for that source. See shader_sm1_read_immconst().
     * Technically shader model 1 doesn't have integer registers or DEFI; we
     * handle it here anyway because it's easy. */
    else if (opcode_info->vkd3d_opcode == VKD3DSIH_DEF || opcode_info->vkd3d_opcode == VKD3DSIH_DEFI)
    {
        *ptr += 3;
    }

    *ptr += (opcode_info->dst_count + opcode_info->src_count);
}

static void shader_sm1_destroy(struct vkd3d_shader_parser *parser)
{
    struct vkd3d_shader_sm1_parser *sm1 = vkd3d_shader_sm1_parser(parser);

    shader_instruction_array_destroy(&parser->instructions);
    free_shader_desc(&sm1->p.shader_desc);
    vkd3d_free(sm1);
}

static void shader_sm1_read_src_param(struct vkd3d_shader_sm1_parser *sm1, const uint32_t **ptr,
        struct vkd3d_shader_src_param *src_param)
{
    struct vkd3d_shader_src_param *src_rel_addr = NULL;
    uint32_t token, addr_token;

    shader_sm1_read_param(sm1, ptr, &token, &addr_token);
    if (has_relative_address(token))
    {
        if (!(src_rel_addr = shader_parser_get_src_params(&sm1->p, 1)))
        {
            vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_OUT_OF_MEMORY,
                    "Out of memory.");
            sm1->abort = true;
            return;
        }
        shader_sm1_parse_src_param(addr_token, NULL, src_rel_addr);
    }
    shader_sm1_parse_src_param(token, src_rel_addr, src_param);
}

static void shader_sm1_read_dst_param(struct vkd3d_shader_sm1_parser *sm1, const uint32_t **ptr,
        struct vkd3d_shader_dst_param *dst_param)
{
    struct vkd3d_shader_src_param *dst_rel_addr = NULL;
    uint32_t token, addr_token;

    shader_sm1_read_param(sm1, ptr, &token, &addr_token);
    if (has_relative_address(token))
    {
        if (!(dst_rel_addr = shader_parser_get_src_params(&sm1->p, 1)))
        {
            vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_OUT_OF_MEMORY,
                    "Out of memory.");
            sm1->abort = true;
            return;
        }
        shader_sm1_parse_src_param(addr_token, NULL, dst_rel_addr);
    }
    shader_sm1_parse_dst_param(token, dst_rel_addr, dst_param);
}

static void shader_sm1_read_semantic(struct vkd3d_shader_sm1_parser *sm1,
        const uint32_t **ptr, struct vkd3d_shader_semantic *semantic)
{
    enum vkd3d_sm1_resource_type resource_type;
    struct vkd3d_shader_register_range *range;
    uint32_t usage_token, dst_token;

    if (*ptr >= sm1->end || sm1->end - *ptr < 2)
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "Attempted to read a declaration instruction, but not enough tokens are remaining.");
        sm1->abort = true;
        return;
    }

    usage_token = read_u32(ptr);
    dst_token = read_u32(ptr);

    semantic->usage = (usage_token & VKD3D_SM1_DCL_USAGE_MASK) >> VKD3D_SM1_DCL_USAGE_SHIFT;
    semantic->usage_idx = (usage_token & VKD3D_SM1_DCL_USAGE_INDEX_MASK) >> VKD3D_SM1_DCL_USAGE_INDEX_SHIFT;
    resource_type = (usage_token & VKD3D_SM1_RESOURCE_TYPE_MASK) >> VKD3D_SM1_RESOURCE_TYPE_SHIFT;
    if (resource_type >= ARRAY_SIZE(resource_type_table))
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_RESOURCE_TYPE,
                "Invalid resource type %#x.", resource_type);
        semantic->resource_type = VKD3D_SHADER_RESOURCE_NONE;
    }
    else
    {
        semantic->resource_type = resource_type_table[resource_type];
    }
    semantic->resource_data_type[0] = VKD3D_DATA_FLOAT;
    semantic->resource_data_type[1] = VKD3D_DATA_FLOAT;
    semantic->resource_data_type[2] = VKD3D_DATA_FLOAT;
    semantic->resource_data_type[3] = VKD3D_DATA_FLOAT;
    shader_sm1_parse_dst_param(dst_token, NULL, &semantic->resource.reg);
    range = &semantic->resource.range;
    range->space = 0;
    range->first = range->last = semantic->resource.reg.reg.idx[0].offset;

    add_signature_element_from_semantic(sm1, semantic);
}

static void shader_sm1_read_immconst(struct vkd3d_shader_sm1_parser *sm1, const uint32_t **ptr,
        struct vkd3d_shader_src_param *src_param, enum vsir_dimension dimension, enum vkd3d_data_type data_type)
{
    unsigned int count = dimension == VSIR_DIMENSION_VEC4 ? 4 : 1;

    if (*ptr >= sm1->end || sm1->end - *ptr < count)
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "Attempted to read a constant definition, but not enough tokens are remaining. "
                "%zu token(s) available, %u required.", sm1->end - *ptr, count);
        sm1->abort = true;
        return;
    }

    src_param->reg.type = VKD3DSPR_IMMCONST;
    src_param->reg.precision = VKD3D_SHADER_REGISTER_PRECISION_DEFAULT;
    src_param->reg.non_uniform = false;
    src_param->reg.data_type = data_type;
    src_param->reg.idx[0].offset = ~0u;
    src_param->reg.idx[0].rel_addr = NULL;
    src_param->reg.idx[1].offset = ~0u;
    src_param->reg.idx[1].rel_addr = NULL;
    src_param->reg.idx[2].offset = ~0u;
    src_param->reg.idx[2].rel_addr = NULL;
    src_param->reg.idx_count = 0;
    src_param->reg.dimension = dimension;
    memcpy(src_param->reg.u.immconst_uint, *ptr, count * sizeof(uint32_t));
    src_param->swizzle = VKD3D_SHADER_NO_SWIZZLE;
    src_param->modifiers = 0;

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

static void shader_sm1_validate_instruction(struct vkd3d_shader_sm1_parser *sm1, struct vkd3d_shader_instruction *ins)
{
    if ((ins->handler_idx == VKD3DSIH_BREAKP || ins->handler_idx == VKD3DSIH_IF) && ins->flags)
    {
        vkd3d_shader_parser_warning(&sm1->p, VKD3D_SHADER_WARNING_D3DBC_IGNORED_INSTRUCTION_FLAGS,
                "Ignoring unexpected instruction flags %#x.", ins->flags);
        ins->flags = 0;
    }
}

static unsigned int mask_from_swizzle(unsigned int swizzle)
{
    return (1u << vkd3d_swizzle_get_component(swizzle, 0))
            | (1u << vkd3d_swizzle_get_component(swizzle, 1))
            | (1u << vkd3d_swizzle_get_component(swizzle, 2))
            | (1u << vkd3d_swizzle_get_component(swizzle, 3));
}

static void shader_sm1_read_instruction(struct vkd3d_shader_sm1_parser *sm1, struct vkd3d_shader_instruction *ins)
{
    struct vkd3d_shader_src_param *src_params, *predicate;
    const struct vkd3d_sm1_opcode_info *opcode_info;
    struct vkd3d_shader_dst_param *dst_param;
    const uint32_t **ptr = &sm1->ptr;
    uint32_t opcode_token;
    const uint32_t *p;
    bool predicated;
    unsigned int i;

    shader_sm1_read_comment(sm1);

    if (*ptr >= sm1->end)
    {
        WARN("End of byte-code, failed to read opcode.\n");
        goto fail;
    }

    ++sm1->p.location.line;
    opcode_token = read_u32(ptr);
    if (!(opcode_info = shader_sm1_get_opcode_info(sm1, opcode_token & VKD3D_SM1_OPCODE_MASK)))
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_INVALID_OPCODE,
                "Invalid opcode %#x (token 0x%08x, shader version %u.%u).",
                opcode_token & VKD3D_SM1_OPCODE_MASK, opcode_token,
                sm1->p.shader_version.major, sm1->p.shader_version.minor);
        goto fail;
    }

    vsir_instruction_init(ins, &sm1->p.location, opcode_info->vkd3d_opcode);
    ins->flags = (opcode_token & VKD3D_SM1_INSTRUCTION_FLAGS_MASK) >> VKD3D_SM1_INSTRUCTION_FLAGS_SHIFT;
    ins->coissue = opcode_token & VKD3D_SM1_COISSUE;
    ins->raw = false;
    ins->structured = false;
    predicated = !!(opcode_token & VKD3D_SM1_INSTRUCTION_PREDICATED);
    ins->predicate = predicate = predicated ? shader_parser_get_src_params(&sm1->p, 1) : NULL;
    ins->dst_count = opcode_info->dst_count;
    ins->dst = dst_param = shader_parser_get_dst_params(&sm1->p, ins->dst_count);
    ins->src_count = opcode_info->src_count;
    ins->src = src_params = shader_parser_get_src_params(&sm1->p, ins->src_count);
    if ((!predicate && predicated) || (!src_params && ins->src_count) || (!dst_param && ins->dst_count))
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_OUT_OF_MEMORY, "Out of memory.");
        goto fail;
    }

    ins->resource_type = VKD3D_SHADER_RESOURCE_NONE;
    ins->resource_stride = 0;
    ins->resource_data_type[0] = VKD3D_DATA_FLOAT;
    ins->resource_data_type[1] = VKD3D_DATA_FLOAT;
    ins->resource_data_type[2] = VKD3D_DATA_FLOAT;
    ins->resource_data_type[3] = VKD3D_DATA_FLOAT;
    memset(&ins->texel_offset, 0, sizeof(ins->texel_offset));

    p = *ptr;
    shader_sm1_skip_opcode(sm1, ptr, opcode_info, opcode_token);
    if (*ptr > sm1->end)
    {
        vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_UNEXPECTED_EOF,
                "The current instruction ends %zu token(s) past the end of the shader.",
                *ptr - sm1->end);
        goto fail;
    }

    if (ins->handler_idx == VKD3DSIH_DCL)
    {
        shader_sm1_read_semantic(sm1, &p, &ins->declaration.semantic);
    }
    else if (ins->handler_idx == VKD3DSIH_DEF)
    {
        shader_sm1_read_dst_param(sm1, &p, dst_param);
        shader_sm1_read_immconst(sm1, &p, &src_params[0], VSIR_DIMENSION_VEC4, VKD3D_DATA_FLOAT);
        shader_sm1_scan_register(sm1, &dst_param->reg, dst_param->write_mask, true);
    }
    else if (ins->handler_idx == VKD3DSIH_DEFB)
    {
        shader_sm1_read_dst_param(sm1, &p, dst_param);
        shader_sm1_read_immconst(sm1, &p, &src_params[0], VSIR_DIMENSION_SCALAR, VKD3D_DATA_UINT);
        shader_sm1_scan_register(sm1, &dst_param->reg, dst_param->write_mask, true);
    }
    else if (ins->handler_idx == VKD3DSIH_DEFI)
    {
        shader_sm1_read_dst_param(sm1, &p, dst_param);
        shader_sm1_read_immconst(sm1, &p, &src_params[0], VSIR_DIMENSION_VEC4, VKD3D_DATA_INT);
        shader_sm1_scan_register(sm1, &dst_param->reg, dst_param->write_mask, true);
    }
    else
    {
        /* Destination token */
        if (ins->dst_count)
        {
            shader_sm1_read_dst_param(sm1, &p, dst_param);
            shader_sm1_scan_register(sm1, &dst_param->reg, dst_param->write_mask, false);
        }

        /* Predication token */
        if (ins->predicate)
            shader_sm1_read_src_param(sm1, &p, predicate);

        /* Other source tokens */
        for (i = 0; i < ins->src_count; ++i)
        {
            shader_sm1_read_src_param(sm1, &p, &src_params[i]);
            shader_sm1_scan_register(sm1, &src_params[i].reg, mask_from_swizzle(src_params[i].swizzle), false);
        }
    }

    if (sm1->abort)
    {
        sm1->abort = false;
        goto fail;
    }

    shader_sm1_validate_instruction(sm1, ins);
    return;

fail:
    ins->handler_idx = VKD3DSIH_INVALID;
    *ptr = sm1->end;
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

const struct vkd3d_shader_parser_ops shader_sm1_parser_ops =
{
    .parser_destroy = shader_sm1_destroy,
};

static enum vkd3d_result shader_sm1_init(struct vkd3d_shader_sm1_parser *sm1,
        const struct vkd3d_shader_compile_info *compile_info, struct vkd3d_shader_message_context *message_context)
{
    const struct vkd3d_shader_location location = {.source_name = compile_info->source_name};
    const uint32_t *code = compile_info->source.code;
    size_t code_size = compile_info->source.size;
    struct vkd3d_shader_desc *shader_desc;
    struct vkd3d_shader_version version;
    uint16_t shader_type;
    size_t token_count;

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

    /* Estimate instruction count to avoid reallocation in most shaders. */
    if (!vkd3d_shader_parser_init(&sm1->p, message_context, compile_info->source_name, &version, &shader_sm1_parser_ops,
            code_size != ~(size_t)0 ? token_count / 4u + 4 : 16))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    shader_desc = &sm1->p.shader_desc;
    shader_desc->byte_code = code;
    shader_desc->byte_code_size = code_size;
    sm1->ptr = sm1->start;

    return VKD3D_OK;
}

static uint32_t get_external_constant_count(struct vkd3d_shader_sm1_parser *sm1,
        enum vkd3d_shader_d3dbc_constant_register set)
{
    unsigned int j;

    /* Find the highest constant index which is not written by a DEF
     * instruction. We can't (easily) use an FFZ function for this since it
     * needs to be limited by the highest used register index. */
    for (j = sm1->p.shader_desc.flat_constant_count[set].used; j > 0; --j)
    {
        if (!bitmap_is_set(sm1->constant_def_mask[set], j - 1))
            return j;
    }

    return 0;
}

int vkd3d_shader_sm1_parser_create(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_message_context *message_context, struct vkd3d_shader_parser **parser)
{
    struct vkd3d_shader_instruction_array *instructions;
    struct vkd3d_shader_instruction *ins;
    struct vkd3d_shader_sm1_parser *sm1;
    unsigned int i;
    int ret;

    if (!(sm1 = vkd3d_calloc(1, sizeof(*sm1))))
    {
        ERR("Failed to allocate parser.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    if ((ret = shader_sm1_init(sm1, compile_info, message_context)) < 0)
    {
        WARN("Failed to initialise shader parser, ret %d.\n", ret);
        vkd3d_free(sm1);
        return ret;
    }

    instructions = &sm1->p.instructions;
    while (!shader_sm1_is_end(sm1))
    {
        if (!shader_instruction_array_reserve(instructions, instructions->count + 1))
        {
            ERR("Failed to allocate instructions.\n");
            vkd3d_shader_parser_error(&sm1->p, VKD3D_SHADER_ERROR_D3DBC_OUT_OF_MEMORY, "Out of memory.");
            shader_sm1_destroy(&sm1->p);
            return VKD3D_ERROR_OUT_OF_MEMORY;
        }
        ins = &instructions->elements[instructions->count];
        shader_sm1_read_instruction(sm1, ins);

        if (ins->handler_idx == VKD3DSIH_INVALID)
        {
            WARN("Encountered unrecognized or invalid instruction.\n");
            shader_sm1_destroy(&sm1->p);
            return VKD3D_ERROR_INVALID_SHADER;
        }
        ++instructions->count;
    }

    for (i = 0; i < ARRAY_SIZE(sm1->p.shader_desc.flat_constant_count); ++i)
        sm1->p.shader_desc.flat_constant_count[i].external = get_external_constant_count(sm1, i);

    if (!sm1->p.failed)
        vsir_validate(&sm1->p);

    if (sm1->p.failed)
    {
        WARN("Failed to parse shader.\n");
        shader_sm1_destroy(&sm1->p);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    *parser = &sm1->p;

    return VKD3D_OK;
}

bool hlsl_sm1_register_from_semantic(struct hlsl_ctx *ctx, const struct hlsl_semantic *semantic,
        bool output, D3DSHADER_PARAM_REGISTER_TYPE *type, unsigned int *reg)
{
    unsigned int i;

    static const struct
    {
        const char *semantic;
        bool output;
        enum vkd3d_shader_type shader_type;
        unsigned int major_version;
        D3DSHADER_PARAM_REGISTER_TYPE type;
        DWORD offset;
    }
    register_table[] =
    {
        {"color",       false, VKD3D_SHADER_TYPE_PIXEL, 1, D3DSPR_INPUT},
        {"texcoord",    false, VKD3D_SHADER_TYPE_PIXEL, 1, D3DSPR_TEXTURE},

        {"color",       true,  VKD3D_SHADER_TYPE_PIXEL, 2, D3DSPR_COLOROUT},
        {"depth",       true,  VKD3D_SHADER_TYPE_PIXEL, 2, D3DSPR_DEPTHOUT},
        {"sv_depth",    true,  VKD3D_SHADER_TYPE_PIXEL, 2, D3DSPR_DEPTHOUT},
        {"sv_target",   true,  VKD3D_SHADER_TYPE_PIXEL, 2, D3DSPR_COLOROUT},
        {"color",       false, VKD3D_SHADER_TYPE_PIXEL, 2, D3DSPR_INPUT},
        {"texcoord",    false, VKD3D_SHADER_TYPE_PIXEL, 2, D3DSPR_TEXTURE},

        {"color",       true,  VKD3D_SHADER_TYPE_PIXEL, 3, D3DSPR_COLOROUT},
        {"depth",       true,  VKD3D_SHADER_TYPE_PIXEL, 3, D3DSPR_DEPTHOUT},
        {"sv_depth",    true,  VKD3D_SHADER_TYPE_PIXEL, 3, D3DSPR_DEPTHOUT},
        {"sv_target",   true,  VKD3D_SHADER_TYPE_PIXEL, 3, D3DSPR_COLOROUT},
        {"sv_position", false, VKD3D_SHADER_TYPE_PIXEL, 3, D3DSPR_MISCTYPE,    D3DSMO_POSITION},
        {"vface",       false, VKD3D_SHADER_TYPE_PIXEL, 3, D3DSPR_MISCTYPE,    D3DSMO_FACE},
        {"vpos",        false, VKD3D_SHADER_TYPE_PIXEL, 3, D3DSPR_MISCTYPE,    D3DSMO_POSITION},

        {"color",       true,  VKD3D_SHADER_TYPE_VERTEX, 1, D3DSPR_ATTROUT},
        {"fog",         true,  VKD3D_SHADER_TYPE_VERTEX, 1, D3DSPR_RASTOUT,     D3DSRO_FOG},
        {"position",    true,  VKD3D_SHADER_TYPE_VERTEX, 1, D3DSPR_RASTOUT,     D3DSRO_POSITION},
        {"psize",       true,  VKD3D_SHADER_TYPE_VERTEX, 1, D3DSPR_RASTOUT,     D3DSRO_POINT_SIZE},
        {"sv_position", true,  VKD3D_SHADER_TYPE_VERTEX, 1, D3DSPR_RASTOUT,     D3DSRO_POSITION},
        {"texcoord",    true,  VKD3D_SHADER_TYPE_VERTEX, 1, D3DSPR_TEXCRDOUT},

        {"color",       true,  VKD3D_SHADER_TYPE_VERTEX, 2, D3DSPR_ATTROUT},
        {"fog",         true,  VKD3D_SHADER_TYPE_VERTEX, 2, D3DSPR_RASTOUT,     D3DSRO_FOG},
        {"position",    true,  VKD3D_SHADER_TYPE_VERTEX, 2, D3DSPR_RASTOUT,     D3DSRO_POSITION},
        {"psize",       true,  VKD3D_SHADER_TYPE_VERTEX, 2, D3DSPR_RASTOUT,     D3DSRO_POINT_SIZE},
        {"sv_position", true,  VKD3D_SHADER_TYPE_VERTEX, 2, D3DSPR_RASTOUT,     D3DSRO_POSITION},
        {"texcoord",    true,  VKD3D_SHADER_TYPE_VERTEX, 2, D3DSPR_TEXCRDOUT},
    };

    for (i = 0; i < ARRAY_SIZE(register_table); ++i)
    {
        if (!ascii_strcasecmp(semantic->name, register_table[i].semantic)
                && output == register_table[i].output
                && ctx->profile->type == register_table[i].shader_type
                && ctx->profile->major_version == register_table[i].major_version)
        {
            *type = register_table[i].type;
            if (register_table[i].type == D3DSPR_MISCTYPE || register_table[i].type == D3DSPR_RASTOUT)
                *reg = register_table[i].offset;
            else
                *reg = semantic->index;
            return true;
        }
    }

    return false;
}

bool hlsl_sm1_usage_from_semantic(const struct hlsl_semantic *semantic, D3DDECLUSAGE *usage, uint32_t *usage_idx)
{
    static const struct
    {
        const char *name;
        D3DDECLUSAGE usage;
    }
    semantics[] =
    {
        {"binormal",        D3DDECLUSAGE_BINORMAL},
        {"blendindices",    D3DDECLUSAGE_BLENDINDICES},
        {"blendweight",     D3DDECLUSAGE_BLENDWEIGHT},
        {"color",           D3DDECLUSAGE_COLOR},
        {"depth",           D3DDECLUSAGE_DEPTH},
        {"fog",             D3DDECLUSAGE_FOG},
        {"normal",          D3DDECLUSAGE_NORMAL},
        {"position",        D3DDECLUSAGE_POSITION},
        {"positiont",       D3DDECLUSAGE_POSITIONT},
        {"psize",           D3DDECLUSAGE_PSIZE},
        {"sample",          D3DDECLUSAGE_SAMPLE},
        {"sv_depth",        D3DDECLUSAGE_DEPTH},
        {"sv_position",     D3DDECLUSAGE_POSITION},
        {"sv_target",       D3DDECLUSAGE_COLOR},
        {"tangent",         D3DDECLUSAGE_TANGENT},
        {"tessfactor",      D3DDECLUSAGE_TESSFACTOR},
        {"texcoord",        D3DDECLUSAGE_TEXCOORD},
    };

    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(semantics); ++i)
    {
        if (!ascii_strcasecmp(semantic->name, semantics[i].name))
        {
            *usage = semantics[i].usage;
            *usage_idx = semantic->index;
            return true;
        }
    }

    return false;
}

static uint32_t sm1_version(enum vkd3d_shader_type type, unsigned int major, unsigned int minor)
{
    if (type == VKD3D_SHADER_TYPE_VERTEX)
        return D3DVS_VERSION(major, minor);
    else
        return D3DPS_VERSION(major, minor);
}

static D3DXPARAMETER_CLASS sm1_class(const struct hlsl_type *type)
{
    switch (type->class)
    {
        case HLSL_CLASS_ARRAY:
            return sm1_class(type->e.array.type);
        case HLSL_CLASS_MATRIX:
            assert(type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK);
            if (type->modifiers & HLSL_MODIFIER_COLUMN_MAJOR)
                return D3DXPC_MATRIX_COLUMNS;
            else
                return D3DXPC_MATRIX_ROWS;
        case HLSL_CLASS_OBJECT:
            return D3DXPC_OBJECT;
        case HLSL_CLASS_SCALAR:
            return D3DXPC_SCALAR;
        case HLSL_CLASS_STRUCT:
            return D3DXPC_STRUCT;
        case HLSL_CLASS_VECTOR:
            return D3DXPC_VECTOR;
        default:
            ERR("Invalid class %#x.\n", type->class);
            vkd3d_unreachable();
    }
}

static D3DXPARAMETER_TYPE sm1_base_type(const struct hlsl_type *type)
{
    switch (type->base_type)
    {
        case HLSL_TYPE_BOOL:
            return D3DXPT_BOOL;
        case HLSL_TYPE_FLOAT:
        case HLSL_TYPE_HALF:
            return D3DXPT_FLOAT;
        case HLSL_TYPE_INT:
        case HLSL_TYPE_UINT:
            return D3DXPT_INT;
        case HLSL_TYPE_PIXELSHADER:
            return D3DXPT_PIXELSHADER;
        case HLSL_TYPE_SAMPLER:
            switch (type->sampler_dim)
            {
                case HLSL_SAMPLER_DIM_1D:
                    return D3DXPT_SAMPLER1D;
                case HLSL_SAMPLER_DIM_2D:
                    return D3DXPT_SAMPLER2D;
                case HLSL_SAMPLER_DIM_3D:
                    return D3DXPT_SAMPLER3D;
                case HLSL_SAMPLER_DIM_CUBE:
                    return D3DXPT_SAMPLERCUBE;
                case HLSL_SAMPLER_DIM_GENERIC:
                    return D3DXPT_SAMPLER;
                default:
                    ERR("Invalid dimension %#x.\n", type->sampler_dim);
                    vkd3d_unreachable();
            }
            break;
        case HLSL_TYPE_STRING:
            return D3DXPT_STRING;
        case HLSL_TYPE_TEXTURE:
            switch (type->sampler_dim)
            {
                case HLSL_SAMPLER_DIM_1D:
                    return D3DXPT_TEXTURE1D;
                case HLSL_SAMPLER_DIM_2D:
                    return D3DXPT_TEXTURE2D;
                case HLSL_SAMPLER_DIM_3D:
                    return D3DXPT_TEXTURE3D;
                case HLSL_SAMPLER_DIM_CUBE:
                    return D3DXPT_TEXTURECUBE;
                case HLSL_SAMPLER_DIM_GENERIC:
                    return D3DXPT_TEXTURE;
                default:
                    ERR("Invalid dimension %#x.\n", type->sampler_dim);
                    vkd3d_unreachable();
            }
            break;
        case HLSL_TYPE_VERTEXSHADER:
            return D3DXPT_VERTEXSHADER;
        case HLSL_TYPE_VOID:
            return D3DXPT_VOID;
        default:
            vkd3d_unreachable();
    }
}

static void write_sm1_type(struct vkd3d_bytecode_buffer *buffer, struct hlsl_type *type, unsigned int ctab_start)
{
    const struct hlsl_type *array_type = hlsl_get_multiarray_element_type(type);
    unsigned int array_size = hlsl_get_multiarray_size(type);
    unsigned int field_count = 0;
    size_t fields_offset = 0;
    size_t i;

    if (type->bytecode_offset)
        return;

    if (array_type->class == HLSL_CLASS_STRUCT)
    {
        field_count = array_type->e.record.field_count;

        for (i = 0; i < field_count; ++i)
        {
            struct hlsl_struct_field *field = &array_type->e.record.fields[i];

            field->name_bytecode_offset = put_string(buffer, field->name);
            write_sm1_type(buffer, field->type, ctab_start);
        }

        fields_offset = bytecode_align(buffer) - ctab_start;

        for (i = 0; i < field_count; ++i)
        {
            struct hlsl_struct_field *field = &array_type->e.record.fields[i];

            put_u32(buffer, field->name_bytecode_offset - ctab_start);
            put_u32(buffer, field->type->bytecode_offset - ctab_start);
        }
    }

    type->bytecode_offset = put_u32(buffer, vkd3d_make_u32(sm1_class(type), sm1_base_type(array_type)));
    put_u32(buffer, vkd3d_make_u32(type->dimy, type->dimx));
    put_u32(buffer, vkd3d_make_u32(array_size, field_count));
    put_u32(buffer, fields_offset);
}

static void sm1_sort_extern(struct list *sorted, struct hlsl_ir_var *to_sort)
{
    struct hlsl_ir_var *var;

    list_remove(&to_sort->extern_entry);

    LIST_FOR_EACH_ENTRY(var, sorted, struct hlsl_ir_var, extern_entry)
    {
        if (strcmp(to_sort->name, var->name) < 0)
        {
            list_add_before(&var->extern_entry, &to_sort->extern_entry);
            return;
        }
    }

    list_add_tail(sorted, &to_sort->extern_entry);
}

static void sm1_sort_externs(struct hlsl_ctx *ctx)
{
    struct list sorted = LIST_INIT(sorted);
    struct hlsl_ir_var *var, *next;

    LIST_FOR_EACH_ENTRY_SAFE(var, next, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_uniform)
            sm1_sort_extern(&sorted, var);
    }
    list_move_tail(&ctx->extern_vars, &sorted);
}

static void write_sm1_uniforms(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        struct hlsl_ir_function_decl *entry_func)
{
    size_t ctab_offset, ctab_start, ctab_end, vars_start, size_offset, creator_offset, offset;
    unsigned int uniform_count = 0;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        unsigned int r;

        for (r = 0; r <= HLSL_REGSET_LAST; ++r)
        {
            if (var->semantic.name || !var->regs[r].allocated)
                continue;

            ++uniform_count;

            if (var->is_param && var->is_uniform)
            {
                char *new_name;

                if (!(new_name = hlsl_sprintf_alloc(ctx, "$%s", var->name)))
                    return;
                vkd3d_free((char *)var->name);
                var->name = new_name;
            }
        }
    }

    sm1_sort_externs(ctx);

    size_offset = put_u32(buffer, 0);
    ctab_offset = put_u32(buffer, VKD3D_MAKE_TAG('C','T','A','B'));

    ctab_start = put_u32(buffer, sizeof(D3DXSHADER_CONSTANTTABLE));
    creator_offset = put_u32(buffer, 0);
    put_u32(buffer, sm1_version(ctx->profile->type, ctx->profile->major_version, ctx->profile->minor_version));
    put_u32(buffer, uniform_count);
    put_u32(buffer, sizeof(D3DXSHADER_CONSTANTTABLE)); /* offset of constants */
    put_u32(buffer, 0); /* FIXME: flags */
    put_u32(buffer, 0); /* FIXME: target string */

    vars_start = bytecode_align(buffer);

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        unsigned int r;

        for (r = 0; r <= HLSL_REGSET_LAST; ++r)
        {
            if (var->semantic.name || !var->regs[r].allocated)
                continue;

            put_u32(buffer, 0); /* name */
            if (r == HLSL_REGSET_NUMERIC)
            {
                put_u32(buffer, vkd3d_make_u32(D3DXRS_FLOAT4, var->regs[r].id));
                put_u32(buffer, var->data_type->reg_size[r] / 4);
            }
            else
            {
                put_u32(buffer, vkd3d_make_u32(D3DXRS_SAMPLER, var->regs[r].id));
                put_u32(buffer, var->bind_count[r]);
            }
            put_u32(buffer, 0); /* type */
            put_u32(buffer, 0); /* FIXME: default value */
        }
    }

    uniform_count = 0;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        unsigned int r;

        for (r = 0; r <= HLSL_REGSET_LAST; ++r)
        {
            size_t var_offset, name_offset;

            if (var->semantic.name || !var->regs[r].allocated)
                continue;

            var_offset = vars_start + (uniform_count * 5 * sizeof(uint32_t));

            name_offset = put_string(buffer, var->name);
            set_u32(buffer, var_offset, name_offset - ctab_start);

            write_sm1_type(buffer, var->data_type, ctab_start);
            set_u32(buffer, var_offset + 3 * sizeof(uint32_t), var->data_type->bytecode_offset - ctab_start);
            ++uniform_count;
        }
    }

    offset = put_string(buffer, vkd3d_shader_get_version(NULL, NULL));
    set_u32(buffer, creator_offset, offset - ctab_start);

    ctab_end = bytecode_align(buffer);
    set_u32(buffer, size_offset, vkd3d_make_u32(D3DSIO_COMMENT, (ctab_end - ctab_offset) / sizeof(uint32_t)));
}

static uint32_t sm1_encode_register_type(D3DSHADER_PARAM_REGISTER_TYPE type)
{
    return ((type << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK)
            | ((type << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2);
}

struct sm1_instruction
{
    D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode;

    struct sm1_dst_register
    {
        D3DSHADER_PARAM_REGISTER_TYPE type;
        D3DSHADER_PARAM_DSTMOD_TYPE mod;
        unsigned int writemask;
        uint32_t reg;
    } dst;

    struct sm1_src_register
    {
        D3DSHADER_PARAM_REGISTER_TYPE type;
        D3DSHADER_PARAM_SRCMOD_TYPE mod;
        unsigned int swizzle;
        uint32_t reg;
    } srcs[3];
    unsigned int src_count;

    unsigned int has_dst;
};

static void write_sm1_dst_register(struct vkd3d_bytecode_buffer *buffer, const struct sm1_dst_register *reg)
{
    assert(reg->writemask);
    put_u32(buffer, (1u << 31) | sm1_encode_register_type(reg->type) | reg->mod | (reg->writemask << 16) | reg->reg);
}

static void write_sm1_src_register(struct vkd3d_bytecode_buffer *buffer,
        const struct sm1_src_register *reg)
{
    put_u32(buffer, (1u << 31) | sm1_encode_register_type(reg->type) | reg->mod | (reg->swizzle << 16) | reg->reg);
}

static void write_sm1_instruction(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct sm1_instruction *instr)
{
    uint32_t token = instr->opcode;
    unsigned int i;

    if (ctx->profile->major_version > 1)
        token |= (instr->has_dst + instr->src_count) << D3DSI_INSTLENGTH_SHIFT;
    put_u32(buffer, token);

    if (instr->has_dst)
        write_sm1_dst_register(buffer, &instr->dst);

    for (i = 0; i < instr->src_count; ++i)
        write_sm1_src_register(buffer, &instr->srcs[i]);
};

static void sm1_map_src_swizzle(struct sm1_src_register *src, unsigned int map_writemask)
{
    src->swizzle = hlsl_map_swizzle(src->swizzle, map_writemask);
}

static void write_sm1_dp2add(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_reg *dst, const struct hlsl_reg *src1, const struct hlsl_reg *src2,
        const struct hlsl_reg *src3)
{
    struct sm1_instruction instr =
    {
        .opcode = D3DSIO_DP2ADD,

        .dst.type = D3DSPR_TEMP,
        .dst.writemask = dst->writemask,
        .dst.reg = dst->id,
        .has_dst = 1,

        .srcs[0].type = D3DSPR_TEMP,
        .srcs[0].swizzle = hlsl_swizzle_from_writemask(src1->writemask),
        .srcs[0].reg = src1->id,
        .srcs[1].type = D3DSPR_TEMP,
        .srcs[1].swizzle = hlsl_swizzle_from_writemask(src2->writemask),
        .srcs[1].reg = src2->id,
        .srcs[2].type = D3DSPR_TEMP,
        .srcs[2].swizzle = hlsl_swizzle_from_writemask(src3->writemask),
        .srcs[2].reg = src3->id,
        .src_count = 3,
    };

    write_sm1_instruction(ctx, buffer, &instr);
}

static void write_sm1_ternary_op(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode, const struct hlsl_reg *dst, const struct hlsl_reg *src1,
        const struct hlsl_reg *src2, const struct hlsl_reg *src3)
{
    struct sm1_instruction instr =
    {
        .opcode = opcode,

        .dst.type = D3DSPR_TEMP,
        .dst.writemask = dst->writemask,
        .dst.reg = dst->id,
        .has_dst = 1,

        .srcs[0].type = D3DSPR_TEMP,
        .srcs[0].swizzle = hlsl_swizzle_from_writemask(src1->writemask),
        .srcs[0].reg = src1->id,
        .srcs[1].type = D3DSPR_TEMP,
        .srcs[1].swizzle = hlsl_swizzle_from_writemask(src2->writemask),
        .srcs[1].reg = src2->id,
        .srcs[2].type = D3DSPR_TEMP,
        .srcs[2].swizzle = hlsl_swizzle_from_writemask(src3->writemask),
        .srcs[2].reg = src3->id,
        .src_count = 3,
    };

    sm1_map_src_swizzle(&instr.srcs[0], instr.dst.writemask);
    sm1_map_src_swizzle(&instr.srcs[1], instr.dst.writemask);
    sm1_map_src_swizzle(&instr.srcs[2], instr.dst.writemask);
    write_sm1_instruction(ctx, buffer, &instr);
}

static void write_sm1_binary_op(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode, const struct hlsl_reg *dst,
        const struct hlsl_reg *src1, const struct hlsl_reg *src2)
{
    struct sm1_instruction instr =
    {
        .opcode = opcode,

        .dst.type = D3DSPR_TEMP,
        .dst.writemask = dst->writemask,
        .dst.reg = dst->id,
        .has_dst = 1,

        .srcs[0].type = D3DSPR_TEMP,
        .srcs[0].swizzle = hlsl_swizzle_from_writemask(src1->writemask),
        .srcs[0].reg = src1->id,
        .srcs[1].type = D3DSPR_TEMP,
        .srcs[1].swizzle = hlsl_swizzle_from_writemask(src2->writemask),
        .srcs[1].reg = src2->id,
        .src_count = 2,
    };

    sm1_map_src_swizzle(&instr.srcs[0], instr.dst.writemask);
    sm1_map_src_swizzle(&instr.srcs[1], instr.dst.writemask);
    write_sm1_instruction(ctx, buffer, &instr);
}

static void write_sm1_binary_op_dot(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode, const struct hlsl_reg *dst,
        const struct hlsl_reg *src1, const struct hlsl_reg *src2)
{
    struct sm1_instruction instr =
    {
        .opcode = opcode,

        .dst.type = D3DSPR_TEMP,
        .dst.writemask = dst->writemask,
        .dst.reg = dst->id,
        .has_dst = 1,

        .srcs[0].type = D3DSPR_TEMP,
        .srcs[0].swizzle = hlsl_swizzle_from_writemask(src1->writemask),
        .srcs[0].reg = src1->id,
        .srcs[1].type = D3DSPR_TEMP,
        .srcs[1].swizzle = hlsl_swizzle_from_writemask(src2->writemask),
        .srcs[1].reg = src2->id,
        .src_count = 2,
    };

    write_sm1_instruction(ctx, buffer, &instr);
}

static void write_sm1_unary_op(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode, const struct hlsl_reg *dst,
        const struct hlsl_reg *src, D3DSHADER_PARAM_SRCMOD_TYPE src_mod, D3DSHADER_PARAM_DSTMOD_TYPE dst_mod)
{
    struct sm1_instruction instr =
    {
        .opcode = opcode,

        .dst.type = D3DSPR_TEMP,
        .dst.mod = dst_mod,
        .dst.writemask = dst->writemask,
        .dst.reg = dst->id,
        .has_dst = 1,

        .srcs[0].type = D3DSPR_TEMP,
        .srcs[0].swizzle = hlsl_swizzle_from_writemask(src->writemask),
        .srcs[0].reg = src->id,
        .srcs[0].mod = src_mod,
        .src_count = 1,
    };

    sm1_map_src_swizzle(&instr.srcs[0], instr.dst.writemask);
    write_sm1_instruction(ctx, buffer, &instr);
}

static void write_sm1_constant_defs(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer)
{
    unsigned int i, x;

    for (i = 0; i < ctx->constant_defs.count; ++i)
    {
        const struct hlsl_constant_register *constant_reg = &ctx->constant_defs.regs[i];
        uint32_t token = D3DSIO_DEF;
        const struct sm1_dst_register reg =
        {
            .type = D3DSPR_CONST,
            .writemask = VKD3DSP_WRITEMASK_ALL,
            .reg = constant_reg->index,
        };

        if (ctx->profile->major_version > 1)
            token |= 5 << D3DSI_INSTLENGTH_SHIFT;
        put_u32(buffer, token);

        write_sm1_dst_register(buffer, &reg);
        for (x = 0; x < 4; ++x)
            put_f32(buffer, constant_reg->value.f[x]);
    }
}

static void write_sm1_semantic_dcl(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_ir_var *var, bool output)
{
    struct sm1_dst_register reg = {0};
    uint32_t token, usage_idx;
    D3DDECLUSAGE usage;
    bool ret;

    if (hlsl_sm1_register_from_semantic(ctx, &var->semantic, output, &reg.type, &reg.reg))
    {
        usage = 0;
        usage_idx = 0;
    }
    else
    {
        ret = hlsl_sm1_usage_from_semantic(&var->semantic, &usage, &usage_idx);
        assert(ret);
        reg.type = output ? D3DSPR_OUTPUT : D3DSPR_INPUT;
        reg.reg = var->regs[HLSL_REGSET_NUMERIC].id;
    }

    token = D3DSIO_DCL;
    if (ctx->profile->major_version > 1)
        token |= 2 << D3DSI_INSTLENGTH_SHIFT;
    put_u32(buffer, token);

    token = (1u << 31);
    token |= usage << D3DSP_DCL_USAGE_SHIFT;
    token |= usage_idx << D3DSP_DCL_USAGEINDEX_SHIFT;
    put_u32(buffer, token);

    reg.writemask = (1 << var->data_type->dimx) - 1;
    write_sm1_dst_register(buffer, &reg);
}

static void write_sm1_semantic_dcls(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer)
{
    bool write_in = false, write_out = false;
    struct hlsl_ir_var *var;

    if (ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL && ctx->profile->major_version >= 2)
        write_in = true;
    else if (ctx->profile->type == VKD3D_SHADER_TYPE_VERTEX && ctx->profile->major_version == 3)
        write_in = write_out = true;
    else if (ctx->profile->type == VKD3D_SHADER_TYPE_VERTEX && ctx->profile->major_version < 3)
        write_in = true;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (write_in && var->is_input_semantic)
            write_sm1_semantic_dcl(ctx, buffer, var, false);
        if (write_out && var->is_output_semantic)
            write_sm1_semantic_dcl(ctx, buffer, var, true);
    }
}

static void write_sm1_sampler_dcl(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        unsigned int reg_id, enum hlsl_sampler_dim sampler_dim)
{
    struct sm1_dst_register reg = {0};
    uint32_t token, res_type = 0;

    token = D3DSIO_DCL;
    if (ctx->profile->major_version > 1)
        token |= 2 << D3DSI_INSTLENGTH_SHIFT;
    put_u32(buffer, token);

    switch (sampler_dim)
    {
        case HLSL_SAMPLER_DIM_2D:
            res_type = VKD3D_SM1_RESOURCE_TEXTURE_2D;
            break;

        case HLSL_SAMPLER_DIM_CUBE:
            res_type = VKD3D_SM1_RESOURCE_TEXTURE_CUBE;
            break;

        case HLSL_SAMPLER_DIM_3D:
            res_type = VKD3D_SM1_RESOURCE_TEXTURE_3D;
            break;

        default:
            vkd3d_unreachable();
            break;
    }

    token = (1u << 31);
    token |= res_type << VKD3D_SM1_RESOURCE_TYPE_SHIFT;
    put_u32(buffer, token);

    reg.type = D3DSPR_SAMPLER;
    reg.writemask = VKD3DSP_WRITEMASK_ALL;
    reg.reg = reg_id;

    write_sm1_dst_register(buffer, &reg);
}

static void write_sm1_sampler_dcls(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer)
{
    enum hlsl_sampler_dim sampler_dim;
    unsigned int i, count, reg_id;
    struct hlsl_ir_var *var;

    if (ctx->profile->major_version < 2)
        return;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->regs[HLSL_REGSET_SAMPLERS].allocated)
            continue;

        count = var->bind_count[HLSL_REGSET_SAMPLERS];

        for (i = 0; i < count; ++i)
        {
            if (var->objects_usage[HLSL_REGSET_SAMPLERS][i].used)
            {
                sampler_dim = var->objects_usage[HLSL_REGSET_SAMPLERS][i].sampler_dim;
                if (sampler_dim == HLSL_SAMPLER_DIM_GENERIC)
                {
                    /* These can appear in sm4-style combined sample instructions. */
                    hlsl_fixme(ctx, &var->loc, "Generic samplers need to be lowered.");
                    continue;
                }

                reg_id = var->regs[HLSL_REGSET_SAMPLERS].id + i;
                write_sm1_sampler_dcl(ctx, buffer, reg_id, sampler_dim);
            }
        }
    }
}

static void write_sm1_constant(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_ir_node *instr)
{
    const struct hlsl_ir_constant *constant = hlsl_ir_constant(instr);
    struct sm1_instruction sm1_instr =
    {
        .opcode = D3DSIO_MOV,

        .dst.type = D3DSPR_TEMP,
        .dst.reg = instr->reg.id,
        .dst.writemask = instr->reg.writemask,
        .has_dst = 1,

        .srcs[0].type = D3DSPR_CONST,
        .srcs[0].reg = constant->reg.id,
        .srcs[0].swizzle = hlsl_swizzle_from_writemask(constant->reg.writemask),
        .src_count = 1,
    };

    assert(instr->reg.allocated);
    assert(constant->reg.allocated);
    sm1_map_src_swizzle(&sm1_instr.srcs[0], sm1_instr.dst.writemask);
    write_sm1_instruction(ctx, buffer, &sm1_instr);
}

static void write_sm1_per_component_unary_op(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_ir_node *instr, D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode)
{
    struct hlsl_ir_expr *expr = hlsl_ir_expr(instr);
    struct hlsl_ir_node *arg1 = expr->operands[0].node;
    unsigned int i;

    for (i = 0; i < instr->data_type->dimx; ++i)
    {
        struct hlsl_reg src = arg1->reg, dst = instr->reg;

        src.writemask = hlsl_combine_writemasks(src.writemask, 1u << i);
        dst.writemask = hlsl_combine_writemasks(dst.writemask, 1u << i);
        write_sm1_unary_op(ctx, buffer, opcode, &dst, &src, 0, 0);
    }
}

static void write_sm1_expr(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_node *instr)
{
    struct hlsl_ir_expr *expr = hlsl_ir_expr(instr);
    struct hlsl_ir_node *arg1 = expr->operands[0].node;
    struct hlsl_ir_node *arg2 = expr->operands[1].node;
    struct hlsl_ir_node *arg3 = expr->operands[2].node;

    assert(instr->reg.allocated);

    if (instr->data_type->base_type != HLSL_TYPE_FLOAT)
    {
        /* These need to be lowered. */
        hlsl_fixme(ctx, &instr->loc, "SM1 non-float expression.");
        return;
    }

    switch (expr->op)
    {
        case HLSL_OP1_ABS:
            write_sm1_unary_op(ctx, buffer, D3DSIO_ABS, &instr->reg, &arg1->reg, 0, 0);
            break;

        case HLSL_OP1_DSX:
            write_sm1_unary_op(ctx, buffer, D3DSIO_DSX, &instr->reg, &arg1->reg, 0, 0);
            break;

        case HLSL_OP1_DSY:
            write_sm1_unary_op(ctx, buffer, D3DSIO_DSY, &instr->reg, &arg1->reg, 0, 0);
            break;

        case HLSL_OP1_EXP2:
            write_sm1_per_component_unary_op(ctx, buffer, instr, D3DSIO_EXP);
            break;

        case HLSL_OP1_LOG2:
            write_sm1_per_component_unary_op(ctx, buffer, instr, D3DSIO_LOG);
            break;

        case HLSL_OP1_NEG:
            write_sm1_unary_op(ctx, buffer, D3DSIO_MOV, &instr->reg, &arg1->reg, D3DSPSM_NEG, 0);
            break;

        case HLSL_OP1_SAT:
            write_sm1_unary_op(ctx, buffer, D3DSIO_MOV, &instr->reg, &arg1->reg, 0, D3DSPDM_SATURATE);
            break;

        case HLSL_OP1_RCP:
            write_sm1_per_component_unary_op(ctx, buffer, instr, D3DSIO_RCP);
            break;

        case HLSL_OP1_RSQ:
            write_sm1_per_component_unary_op(ctx, buffer, instr, D3DSIO_RSQ);
            break;

        case HLSL_OP2_ADD:
            write_sm1_binary_op(ctx, buffer, D3DSIO_ADD, &instr->reg, &arg1->reg, &arg2->reg);
            break;

        case HLSL_OP2_MAX:
            write_sm1_binary_op(ctx, buffer, D3DSIO_MAX, &instr->reg, &arg1->reg, &arg2->reg);
            break;

        case HLSL_OP2_MIN:
            write_sm1_binary_op(ctx, buffer, D3DSIO_MIN, &instr->reg, &arg1->reg, &arg2->reg);
            break;

        case HLSL_OP2_MUL:
            write_sm1_binary_op(ctx, buffer, D3DSIO_MUL, &instr->reg, &arg1->reg, &arg2->reg);
            break;

        case HLSL_OP1_FRACT:
            write_sm1_unary_op(ctx, buffer, D3DSIO_FRC, &instr->reg, &arg1->reg, D3DSPSM_NONE, 0);
            break;

        case HLSL_OP2_DOT:
            switch (arg1->data_type->dimx)
            {
                case 4:
                    write_sm1_binary_op_dot(ctx, buffer, D3DSIO_DP4, &instr->reg, &arg1->reg, &arg2->reg);
                    break;

                case 3:
                    write_sm1_binary_op_dot(ctx, buffer, D3DSIO_DP3, &instr->reg, &arg1->reg, &arg2->reg);
                    break;

                default:
                    vkd3d_unreachable();
            }
            break;

        case HLSL_OP3_CMP:
            write_sm1_ternary_op(ctx, buffer, D3DSIO_CMP, &instr->reg, &arg1->reg, &arg2->reg, &arg3->reg);
            break;

        case HLSL_OP3_DP2ADD:
            write_sm1_dp2add(ctx, buffer, &instr->reg, &arg1->reg, &arg2->reg, &arg3->reg);
            break;

        default:
            hlsl_fixme(ctx, &instr->loc, "SM1 \"%s\" expression.", debug_hlsl_expr_op(expr->op));
            break;
    }
}

static void write_sm1_jump(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_node *instr)
{
    const struct hlsl_ir_jump *jump = hlsl_ir_jump(instr);

    switch (jump->type)
    {
        case HLSL_IR_JUMP_DISCARD_NEG:
        {
            struct hlsl_reg *reg = &jump->condition.node->reg;

            struct sm1_instruction instr =
            {
                .opcode = D3DSIO_TEXKILL,

                .dst.type = D3DSPR_TEMP,
                .dst.reg = reg->id,
                .dst.writemask = reg->writemask,
                .has_dst = 1,
            };

            write_sm1_instruction(ctx, buffer, &instr);
            break;
        }

        default:
            hlsl_fixme(ctx, &jump->node.loc, "Jump type %s.\n", hlsl_jump_type_to_string(jump->type));
    }
}

static void write_sm1_load(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer, const struct hlsl_ir_node *instr)
{
    const struct hlsl_ir_load *load = hlsl_ir_load(instr);
    const struct hlsl_reg reg = hlsl_reg_from_deref(ctx, &load->src);
    struct sm1_instruction sm1_instr =
    {
        .opcode = D3DSIO_MOV,

        .dst.type = D3DSPR_TEMP,
        .dst.reg = instr->reg.id,
        .dst.writemask = instr->reg.writemask,
        .has_dst = 1,

        .srcs[0].type = D3DSPR_TEMP,
        .srcs[0].reg = reg.id,
        .srcs[0].swizzle = hlsl_swizzle_from_writemask(reg.writemask),
        .src_count = 1,
    };

    assert(instr->reg.allocated);

    if (load->src.var->is_uniform)
    {
        assert(reg.allocated);
        sm1_instr.srcs[0].type = D3DSPR_CONST;
    }
    else if (load->src.var->is_input_semantic)
    {
        if (!hlsl_sm1_register_from_semantic(ctx, &load->src.var->semantic,
                false, &sm1_instr.srcs[0].type, &sm1_instr.srcs[0].reg))
        {
            assert(reg.allocated);
            sm1_instr.srcs[0].type = D3DSPR_INPUT;
            sm1_instr.srcs[0].reg = reg.id;
        }
        else
            sm1_instr.srcs[0].swizzle = hlsl_swizzle_from_writemask((1 << load->src.var->data_type->dimx) - 1);
    }

    sm1_map_src_swizzle(&sm1_instr.srcs[0], sm1_instr.dst.writemask);
    write_sm1_instruction(ctx, buffer, &sm1_instr);
}

static void write_sm1_resource_load(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_ir_node *instr)
{
    const struct hlsl_ir_resource_load *load = hlsl_ir_resource_load(instr);
    struct hlsl_ir_node *coords = load->coords.node;
    unsigned int sampler_offset, reg_id;
    struct sm1_instruction sm1_instr;

    sampler_offset = hlsl_offset_from_deref_safe(ctx, &load->resource);
    reg_id = load->resource.var->regs[HLSL_REGSET_SAMPLERS].id + sampler_offset;

    sm1_instr = (struct sm1_instruction)
    {
        .opcode = D3DSIO_TEX,

        .dst.type = D3DSPR_TEMP,
        .dst.reg = instr->reg.id,
        .dst.writemask = instr->reg.writemask,
        .has_dst = 1,

        .srcs[0].type = D3DSPR_TEMP,
        .srcs[0].reg = coords->reg.id,
        .srcs[0].swizzle = hlsl_swizzle_from_writemask(VKD3DSP_WRITEMASK_ALL),

        .srcs[1].type = D3DSPR_SAMPLER,
        .srcs[1].reg = reg_id,
        .srcs[1].swizzle = hlsl_swizzle_from_writemask(VKD3DSP_WRITEMASK_ALL),

        .src_count = 2,
    };
    if (load->load_type == HLSL_RESOURCE_SAMPLE_PROJ)
        sm1_instr.opcode |= VKD3DSI_TEXLD_PROJECT << VKD3D_SM1_INSTRUCTION_FLAGS_SHIFT;

    assert(instr->reg.allocated);

    write_sm1_instruction(ctx, buffer, &sm1_instr);
}

static void write_sm1_store(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_ir_node *instr)
{
    const struct hlsl_ir_store *store = hlsl_ir_store(instr);
    const struct hlsl_ir_node *rhs = store->rhs.node;
    const struct hlsl_reg reg = hlsl_reg_from_deref(ctx, &store->lhs);
    struct sm1_instruction sm1_instr =
    {
        .opcode = D3DSIO_MOV,

        .dst.type = D3DSPR_TEMP,
        .dst.reg = reg.id,
        .dst.writemask = hlsl_combine_writemasks(reg.writemask, store->writemask),
        .has_dst = 1,

        .srcs[0].type = D3DSPR_TEMP,
        .srcs[0].reg = rhs->reg.id,
        .srcs[0].swizzle = hlsl_swizzle_from_writemask(rhs->reg.writemask),
        .src_count = 1,
    };

    if (store->lhs.var->data_type->class == HLSL_CLASS_MATRIX)
    {
        FIXME("Matrix writemasks need to be lowered.\n");
        return;
    }

    if (store->lhs.var->is_output_semantic)
    {
        if (ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL && ctx->profile->major_version == 1)
        {
            sm1_instr.dst.type = D3DSPR_TEMP;
            sm1_instr.dst.reg = 0;
        }
        else if (!hlsl_sm1_register_from_semantic(ctx, &store->lhs.var->semantic,
                true, &sm1_instr.dst.type, &sm1_instr.dst.reg))
        {
            assert(reg.allocated);
            sm1_instr.dst.type = D3DSPR_OUTPUT;
            sm1_instr.dst.reg = reg.id;
        }
        else
            sm1_instr.dst.writemask = (1u << store->lhs.var->data_type->dimx) - 1;
    }
    else
        assert(reg.allocated);

    sm1_map_src_swizzle(&sm1_instr.srcs[0], sm1_instr.dst.writemask);
    write_sm1_instruction(ctx, buffer, &sm1_instr);
}

static void write_sm1_swizzle(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_ir_node *instr)
{
    const struct hlsl_ir_swizzle *swizzle = hlsl_ir_swizzle(instr);
    const struct hlsl_ir_node *val = swizzle->val.node;
    struct sm1_instruction sm1_instr =
    {
        .opcode = D3DSIO_MOV,

        .dst.type = D3DSPR_TEMP,
        .dst.reg = instr->reg.id,
        .dst.writemask = instr->reg.writemask,
        .has_dst = 1,

        .srcs[0].type = D3DSPR_TEMP,
        .srcs[0].reg = val->reg.id,
        .srcs[0].swizzle = hlsl_combine_swizzles(hlsl_swizzle_from_writemask(val->reg.writemask),
                swizzle->swizzle, instr->data_type->dimx),
        .src_count = 1,
    };

    assert(instr->reg.allocated);
    assert(val->reg.allocated);
    sm1_map_src_swizzle(&sm1_instr.srcs[0], sm1_instr.dst.writemask);
    write_sm1_instruction(ctx, buffer, &sm1_instr);
}

static void write_sm1_instructions(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer,
        const struct hlsl_ir_function_decl *entry_func)
{
    const struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &entry_func->body.instrs, struct hlsl_ir_node, entry)
    {
        if (instr->data_type)
        {
            if (instr->data_type->class == HLSL_CLASS_MATRIX)
            {
                /* These need to be lowered. */
                hlsl_fixme(ctx, &instr->loc, "SM1 matrix expression.");
                continue;
            }
            else if (instr->data_type->class == HLSL_CLASS_OBJECT)
            {
                hlsl_fixme(ctx, &instr->loc, "Object copy.");
                break;
            }

            assert(instr->data_type->class == HLSL_CLASS_SCALAR || instr->data_type->class == HLSL_CLASS_VECTOR);
        }

        switch (instr->type)
        {
            case HLSL_IR_CALL:
                vkd3d_unreachable();

            case HLSL_IR_CONSTANT:
                write_sm1_constant(ctx, buffer, instr);
                break;

            case HLSL_IR_EXPR:
                write_sm1_expr(ctx, buffer, instr);
                break;

            case HLSL_IR_JUMP:
                write_sm1_jump(ctx, buffer, instr);
                break;

            case HLSL_IR_LOAD:
                write_sm1_load(ctx, buffer, instr);
                break;

            case HLSL_IR_RESOURCE_LOAD:
                write_sm1_resource_load(ctx, buffer, instr);
                break;

            case HLSL_IR_STORE:
                write_sm1_store(ctx, buffer, instr);
                break;

            case HLSL_IR_SWIZZLE:
                write_sm1_swizzle(ctx, buffer, instr);
                break;

            default:
                hlsl_fixme(ctx, &instr->loc, "Instruction type %s.", hlsl_node_type_to_string(instr->type));
        }
    }
}

int hlsl_sm1_write(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func, struct vkd3d_shader_code *out)
{
    struct vkd3d_bytecode_buffer buffer = {0};

    put_u32(&buffer, sm1_version(ctx->profile->type, ctx->profile->major_version, ctx->profile->minor_version));

    write_sm1_uniforms(ctx, &buffer, entry_func);

    write_sm1_constant_defs(ctx, &buffer);
    write_sm1_semantic_dcls(ctx, &buffer);
    write_sm1_sampler_dcls(ctx, &buffer);
    write_sm1_instructions(ctx, &buffer, entry_func);

    put_u32(&buffer, D3DSIO_END);

    if (buffer.status)
        ctx->result = buffer.status;

    if (!ctx->result)
    {
        out->code = buffer.data;
        out->size = buffer.size;
    }
    else
    {
        vkd3d_free(buffer.data);
    }
    return ctx->result;
}
