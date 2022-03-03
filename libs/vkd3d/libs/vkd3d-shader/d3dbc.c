/*
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009, 2021 Henri Verbeet for CodeWeavers
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
    const uint32_t *start, *end;
    bool abort;

    struct vkd3d_shader_src_param src_rel_addr[4];
    struct vkd3d_shader_src_param pred_rel_addr;
    struct vkd3d_shader_src_param dst_rel_addr;
    struct vkd3d_shader_src_param src_param[4];
    struct vkd3d_shader_src_param pred_param;
    struct vkd3d_shader_dst_param dst_param;

    struct vkd3d_shader_parser p;
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
    {VKD3D_SM1_OP_DCL,          0, 2, VKD3DSIH_DCL},
    /* Constant definitions */
    {VKD3D_SM1_OP_DEF,          1, 4, VKD3DSIH_DEF},
    {VKD3D_SM1_OP_DEFB,         1, 1, VKD3DSIH_DEFB},
    {VKD3D_SM1_OP_DEFI,         1, 4, VKD3DSIH_DEFI},
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
    {VKD3D_SM1_OP_DCL,          0, 2, VKD3DSIH_DCL},
    /* Constant definitions */
    {VKD3D_SM1_OP_DEF,          1, 4, VKD3DSIH_DEF},
    {VKD3D_SM1_OP_DEFB,         1, 1, VKD3DSIH_DEFB},
    {VKD3D_SM1_OP_DEFI,         1, 4, VKD3DSIH_DEFI},
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

static bool shader_ver_ge(const struct vkd3d_shader_version *v, unsigned int major, unsigned int minor)
{
    return v->major > major || (v->major == major && v->minor >= minor);
}

static bool shader_ver_le(const struct vkd3d_shader_version *v, unsigned int major, unsigned int minor)
{
    return v->major < major || (v->major == major && v->minor <= minor);
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
                && shader_ver_ge(&sm1->p.shader_version, info->min_version.major, info->min_version.minor)
                && (shader_ver_le(&sm1->p.shader_version, info->max_version.major, info->max_version.minor)
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
    src->reg.type = ((param & VKD3D_SM1_REGISTER_TYPE_MASK) >> VKD3D_SM1_REGISTER_TYPE_SHIFT)
            | ((param & VKD3D_SM1_REGISTER_TYPE_MASK2) >> VKD3D_SM1_REGISTER_TYPE_SHIFT2);
    src->reg.precision = VKD3D_SHADER_REGISTER_PRECISION_DEFAULT;
    src->reg.non_uniform = false;
    src->reg.data_type = VKD3D_DATA_FLOAT;
    src->reg.idx[0].offset = param & VKD3D_SM1_REGISTER_NUMBER_MASK;
    src->reg.idx[0].rel_addr = rel_addr;
    src->reg.idx[1].offset = ~0u;
    src->reg.idx[1].rel_addr = NULL;
    src->reg.idx[2].offset = ~0u;
    src->reg.idx[2].rel_addr = NULL;
    src->swizzle = swizzle_from_sm1((param & VKD3D_SM1_SWIZZLE_MASK) >> VKD3D_SM1_SWIZZLE_SHIFT);
    src->modifiers = (param & VKD3D_SM1_SRC_MODIFIER_MASK) >> VKD3D_SM1_SRC_MODIFIER_SHIFT;
}

static void shader_sm1_parse_dst_param(uint32_t param, const struct vkd3d_shader_src_param *rel_addr,
        struct vkd3d_shader_dst_param *dst)
{
    dst->reg.type = ((param & VKD3D_SM1_REGISTER_TYPE_MASK) >> VKD3D_SM1_REGISTER_TYPE_SHIFT)
            | ((param & VKD3D_SM1_REGISTER_TYPE_MASK2) >> VKD3D_SM1_REGISTER_TYPE_SHIFT2);
    dst->reg.precision = VKD3D_SHADER_REGISTER_PRECISION_DEFAULT;
    dst->reg.non_uniform = false;
    dst->reg.data_type = VKD3D_DATA_FLOAT;
    dst->reg.idx[0].offset = param & VKD3D_SM1_REGISTER_NUMBER_MASK;
    dst->reg.idx[0].rel_addr = rel_addr;
    dst->reg.idx[1].offset = ~0u;
    dst->reg.idx[1].rel_addr = NULL;
    dst->reg.idx[2].offset = ~0u;
    dst->reg.idx[2].rel_addr = NULL;
    dst->write_mask = (param & VKD3D_SM1_WRITEMASK_MASK) >> VKD3D_SM1_WRITEMASK_SHIFT;
    dst->modifiers = (param & VKD3D_SM1_DST_MODIFIER_MASK) >> VKD3D_SM1_DST_MODIFIER_SHIFT;
    dst->shift = (param & VKD3D_SM1_DSTSHIFT_MASK) >> VKD3D_SM1_DSTSHIFT_SHIFT;
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

    *ptr += (opcode_info->dst_count + opcode_info->src_count);
}

static void shader_sm1_destroy(struct vkd3d_shader_parser *parser)
{
    struct vkd3d_shader_sm1_parser *sm1 = vkd3d_shader_sm1_parser(parser);

    free_shader_desc(&sm1->p.shader_desc);
    vkd3d_free(sm1);
}

static void shader_sm1_read_src_param(struct vkd3d_shader_sm1_parser *sm1, const uint32_t **ptr,
        struct vkd3d_shader_src_param *src_param, struct vkd3d_shader_src_param *src_rel_addr)
{
    uint32_t token, addr_token;

    shader_sm1_read_param(sm1, ptr, &token, &addr_token);
    if (has_relative_address(token))
        shader_sm1_parse_src_param(addr_token, NULL, src_rel_addr);
    else
        src_rel_addr = NULL;
    shader_sm1_parse_src_param(token, src_rel_addr, src_param);
}

static void shader_sm1_read_dst_param(struct vkd3d_shader_sm1_parser *sm1, const uint32_t **ptr,
        struct vkd3d_shader_dst_param *dst_param, struct vkd3d_shader_src_param *dst_rel_addr)
{
    uint32_t token, addr_token;

    shader_sm1_read_param(sm1, ptr, &token, &addr_token);
    if (has_relative_address(token))
        shader_sm1_parse_src_param(addr_token, NULL, dst_rel_addr);
    else
        dst_rel_addr = NULL;
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
}

static void shader_sm1_read_immconst(struct vkd3d_shader_sm1_parser *sm1, const uint32_t **ptr,
        struct vkd3d_shader_src_param *src_param, enum vkd3d_immconst_type type, enum vkd3d_data_type data_type)
{
    unsigned int count = type == VKD3D_IMMCONST_VEC4 ? 4 : 1;

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
    src_param->reg.immconst_type = type;
    memcpy(src_param->reg.u.immconst_uint, *ptr, count * sizeof(uint32_t));
    src_param->swizzle = VKD3D_SHADER_NO_SWIZZLE;
    src_param->modifiers = 0;

    *ptr += count;
}

static void shader_sm1_read_comment(struct vkd3d_shader_sm1_parser *sm1)
{
    const uint32_t **ptr = &sm1->p.ptr;
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

static void shader_sm1_read_instruction(struct vkd3d_shader_parser *parser, struct vkd3d_shader_instruction *ins)
{
    struct vkd3d_shader_sm1_parser *sm1 = vkd3d_shader_sm1_parser(parser);
    const struct vkd3d_sm1_opcode_info *opcode_info;
    const uint32_t **ptr = &parser->ptr;
    uint32_t opcode_token;
    const uint32_t *p;
    unsigned int i;

    shader_sm1_read_comment(sm1);

    if (*ptr >= sm1->end)
    {
        WARN("End of byte-code, failed to read opcode.\n");
        goto fail;
    }

    ++parser->location.line;
    opcode_token = read_u32(ptr);
    if (!(opcode_info = shader_sm1_get_opcode_info(sm1, opcode_token & VKD3D_SM1_OPCODE_MASK)))
    {
        vkd3d_shader_parser_error(parser, VKD3D_SHADER_ERROR_D3DBC_INVALID_OPCODE,
                "Invalid opcode %#x (token 0x%08x, shader version %u.%u).",
                opcode_token & VKD3D_SM1_OPCODE_MASK, opcode_token,
                sm1->p.shader_version.major, sm1->p.shader_version.minor);
        goto fail;
    }

    ins->handler_idx = opcode_info->vkd3d_opcode;
    ins->flags = (opcode_token & VKD3D_SM1_INSTRUCTION_FLAGS_MASK) >> VKD3D_SM1_INSTRUCTION_FLAGS_SHIFT;
    ins->coissue = opcode_token & VKD3D_SM1_COISSUE;
    ins->raw = false;
    ins->structured = false;
    ins->predicate = opcode_token & VKD3D_SM1_INSTRUCTION_PREDICATED ? &sm1->pred_param : NULL;
    ins->dst_count = opcode_info->dst_count;
    ins->dst = &sm1->dst_param;
    ins->src_count = opcode_info->src_count;
    ins->src = sm1->src_param;
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
        shader_sm1_read_dst_param(sm1, &p, &sm1->dst_param, &sm1->dst_rel_addr);
        shader_sm1_read_immconst(sm1, &p, &sm1->src_param[0], VKD3D_IMMCONST_VEC4, VKD3D_DATA_FLOAT);
    }
    else if (ins->handler_idx == VKD3DSIH_DEFB)
    {
        shader_sm1_read_dst_param(sm1, &p, &sm1->dst_param, &sm1->dst_rel_addr);
        shader_sm1_read_immconst(sm1, &p, &sm1->src_param[0], VKD3D_IMMCONST_SCALAR, VKD3D_DATA_UINT);
    }
    else if (ins->handler_idx == VKD3DSIH_DEFI)
    {
        shader_sm1_read_dst_param(sm1, &p, &sm1->dst_param, &sm1->dst_rel_addr);
        shader_sm1_read_immconst(sm1, &p, &sm1->src_param[0], VKD3D_IMMCONST_VEC4, VKD3D_DATA_INT);
    }
    else
    {
        /* Destination token */
        if (ins->dst_count)
            shader_sm1_read_dst_param(sm1, &p, &sm1->dst_param, &sm1->dst_rel_addr);

        /* Predication token */
        if (ins->predicate)
            shader_sm1_read_src_param(sm1, &p, &sm1->pred_param, &sm1->pred_rel_addr);

        /* Other source tokens */
        for (i = 0; i < ins->src_count; ++i)
            shader_sm1_read_src_param(sm1, &p, &sm1->src_param[i], &sm1->src_rel_addr[i]);
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

static bool shader_sm1_is_end(struct vkd3d_shader_parser *parser)
{
    struct vkd3d_shader_sm1_parser *sm1 = vkd3d_shader_sm1_parser(parser);
    const uint32_t **ptr = &parser->ptr;

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

static void shader_sm1_reset(struct vkd3d_shader_parser *parser)
{
    struct vkd3d_shader_sm1_parser *sm1 = vkd3d_shader_sm1_parser(parser);

    parser->ptr = sm1->start;
    parser->failed = false;
}

const struct vkd3d_shader_parser_ops shader_sm1_parser_ops =
{
    .parser_reset = shader_sm1_reset,
    .parser_destroy = shader_sm1_destroy,
    .parser_read_instruction = shader_sm1_read_instruction,
    .parser_is_end = shader_sm1_is_end,
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

    if (!shader_ver_le(&version, 3, 0))
    {
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_D3DBC_INVALID_VERSION_TOKEN,
                "Invalid shader version %u.%u (token 0x%08x).", version.major, version.minor, code[0]);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    sm1->start = &code[1];
    sm1->end = &code[token_count];

    vkd3d_shader_parser_init(&sm1->p, message_context, compile_info->source_name, &version, &shader_sm1_parser_ops);
    shader_desc = &sm1->p.shader_desc;
    shader_desc->byte_code = code;
    shader_desc->byte_code_size = code_size;
    sm1->p.ptr = sm1->start;

    return VKD3D_OK;
}

int vkd3d_shader_sm1_parser_create(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_message_context *message_context, struct vkd3d_shader_parser **parser)
{
    struct vkd3d_shader_sm1_parser *sm1;
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

    *parser = &sm1->p;

    return VKD3D_OK;
}
