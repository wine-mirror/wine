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

#ifndef __VKD3D_SM4_H
#define __VKD3D_SM4_H

#define VKD3D_SM4_PS  0x0000u
#define VKD3D_SM4_VS  0x0001u
#define VKD3D_SM4_GS  0x0002u
#define VKD3D_SM5_HS  0x0003u
#define VKD3D_SM5_DS  0x0004u
#define VKD3D_SM5_CS  0x0005u
#define VKD3D_SM4_LIB 0xfff0u

#define VKD3D_SM4_INSTRUCTION_MODIFIER        (0x1u << 31)

#define VKD3D_SM4_MODIFIER_MASK               0x3fu

#define VKD3D_SM5_MODIFIER_DATA_TYPE_SHIFT    6
#define VKD3D_SM5_MODIFIER_DATA_TYPE_MASK     (0xffffu << VKD3D_SM5_MODIFIER_DATA_TYPE_SHIFT)

#define VKD3D_SM5_MODIFIER_RESOURCE_TYPE_SHIFT 6
#define VKD3D_SM5_MODIFIER_RESOURCE_TYPE_MASK (0xfu << VKD3D_SM5_MODIFIER_RESOURCE_TYPE_SHIFT)

#define VKD3D_SM5_MODIFIER_RESOURCE_STRIDE_SHIFT 11
#define VKD3D_SM5_MODIFIER_RESOURCE_STRIDE_MASK  (0xfffu << VKD3D_SM5_MODIFIER_RESOURCE_STRIDE_SHIFT)

#define VKD3D_SM4_AOFFIMMI_U_SHIFT            9
#define VKD3D_SM4_AOFFIMMI_U_MASK             (0xfu << VKD3D_SM4_AOFFIMMI_U_SHIFT)
#define VKD3D_SM4_AOFFIMMI_V_SHIFT            13
#define VKD3D_SM4_AOFFIMMI_V_MASK             (0xfu << VKD3D_SM4_AOFFIMMI_V_SHIFT)
#define VKD3D_SM4_AOFFIMMI_W_SHIFT            17
#define VKD3D_SM4_AOFFIMMI_W_MASK             (0xfu << VKD3D_SM4_AOFFIMMI_W_SHIFT)

#define VKD3D_SM4_INSTRUCTION_LENGTH_SHIFT    24
#define VKD3D_SM4_INSTRUCTION_LENGTH_MASK     (0x1fu << VKD3D_SM4_INSTRUCTION_LENGTH_SHIFT)

#define VKD3D_SM4_INSTRUCTION_FLAGS_SHIFT     11
#define VKD3D_SM4_INSTRUCTION_FLAGS_MASK      (0x7u << VKD3D_SM4_INSTRUCTION_FLAGS_SHIFT)

#define VKD3D_SM4_RESOURCE_TYPE_SHIFT         11
#define VKD3D_SM4_RESOURCE_TYPE_MASK          (0xfu << VKD3D_SM4_RESOURCE_TYPE_SHIFT)

#define VKD3D_SM4_PRIMITIVE_TYPE_SHIFT        11
#define VKD3D_SM4_PRIMITIVE_TYPE_MASK         (0x3fu << VKD3D_SM4_PRIMITIVE_TYPE_SHIFT)

#define VKD3D_SM4_INDEX_TYPE_SHIFT            11
#define VKD3D_SM4_INDEX_TYPE_MASK             (0x1u << VKD3D_SM4_INDEX_TYPE_SHIFT)

#define VKD3D_SM4_SAMPLER_MODE_SHIFT          11
#define VKD3D_SM4_SAMPLER_MODE_MASK           (0xfu << VKD3D_SM4_SAMPLER_MODE_SHIFT)

#define VKD3D_SM4_SHADER_DATA_TYPE_SHIFT      11
#define VKD3D_SM4_SHADER_DATA_TYPE_MASK       (0xfu << VKD3D_SM4_SHADER_DATA_TYPE_SHIFT)

#define VKD3D_SM4_INTERPOLATION_MODE_SHIFT    11
#define VKD3D_SM4_INTERPOLATION_MODE_MASK     (0xfu << VKD3D_SM4_INTERPOLATION_MODE_SHIFT)

#define VKD3D_SM4_GLOBAL_FLAGS_SHIFT          11
#define VKD3D_SM4_GLOBAL_FLAGS_MASK           (0xffu << VKD3D_SM4_GLOBAL_FLAGS_SHIFT)

#define VKD3D_SM5_PRECISE_SHIFT               19
#define VKD3D_SM5_PRECISE_MASK                (0xfu << VKD3D_SM5_PRECISE_SHIFT)

#define VKD3D_SM5_CONTROL_POINT_COUNT_SHIFT   11
#define VKD3D_SM5_CONTROL_POINT_COUNT_MASK    (0xffu << VKD3D_SM5_CONTROL_POINT_COUNT_SHIFT)

#define VKD3D_SM5_FP_ARRAY_SIZE_SHIFT         16
#define VKD3D_SM5_FP_TABLE_COUNT_MASK         0xffffu

#define VKD3D_SM5_UAV_FLAGS_SHIFT             15
#define VKD3D_SM5_UAV_FLAGS_MASK              (0x1ffu << VKD3D_SM5_UAV_FLAGS_SHIFT)

#define VKD3D_SM5_SYNC_FLAGS_SHIFT            11
#define VKD3D_SM5_SYNC_FLAGS_MASK             (0xffu << VKD3D_SM5_SYNC_FLAGS_SHIFT)

#define VKD3D_SM5_TESSELLATOR_SHIFT           11
#define VKD3D_SM5_TESSELLATOR_MASK            (0xfu << VKD3D_SM5_TESSELLATOR_SHIFT)

#define VKD3D_SM4_OPCODE_MASK                 0xff

#define VKD3D_SM4_EXTENDED_OPERAND            (0x1u << 31)

#define VKD3D_SM4_EXTENDED_OPERAND_TYPE_MASK  0x3fu

#define VKD3D_SM4_REGISTER_MODIFIER_SHIFT     6
#define VKD3D_SM4_REGISTER_MODIFIER_MASK      (0xffu << VKD3D_SM4_REGISTER_MODIFIER_SHIFT)

#define VKD3D_SM4_REGISTER_PRECISION_SHIFT    14
#define VKD3D_SM4_REGISTER_PRECISION_MASK     (0x7u << VKD3D_SM4_REGISTER_PRECISION_SHIFT)

#define VKD3D_SM4_REGISTER_NON_UNIFORM_SHIFT  17
#define VKD3D_SM4_REGISTER_NON_UNIFORM_MASK   (0x1u << VKD3D_SM4_REGISTER_NON_UNIFORM_SHIFT)

#define VKD3D_SM4_ADDRESSING_SHIFT2           28
#define VKD3D_SM4_ADDRESSING_MASK2            (0x3u << VKD3D_SM4_ADDRESSING_SHIFT2)

#define VKD3D_SM4_ADDRESSING_SHIFT1           25
#define VKD3D_SM4_ADDRESSING_MASK1            (0x3u << VKD3D_SM4_ADDRESSING_SHIFT1)

#define VKD3D_SM4_ADDRESSING_SHIFT0           22
#define VKD3D_SM4_ADDRESSING_MASK0            (0x3u << VKD3D_SM4_ADDRESSING_SHIFT0)

#define VKD3D_SM4_REGISTER_ORDER_SHIFT        20
#define VKD3D_SM4_REGISTER_ORDER_MASK         (0x3u << VKD3D_SM4_REGISTER_ORDER_SHIFT)

#define VKD3D_SM4_REGISTER_TYPE_SHIFT         12
#define VKD3D_SM4_REGISTER_TYPE_MASK          (0xffu << VKD3D_SM4_REGISTER_TYPE_SHIFT)

#define VKD3D_SM4_SWIZZLE_TYPE_SHIFT          2
#define VKD3D_SM4_SWIZZLE_TYPE_MASK           (0x3u << VKD3D_SM4_SWIZZLE_TYPE_SHIFT)

#define VKD3D_SM4_DIMENSION_SHIFT             0
#define VKD3D_SM4_DIMENSION_MASK              (0x3u << VKD3D_SM4_DIMENSION_SHIFT)

#define VKD3D_SM4_WRITEMASK_SHIFT             4
#define VKD3D_SM4_WRITEMASK_MASK              (0xfu << VKD3D_SM4_WRITEMASK_SHIFT)

#define VKD3D_SM4_SWIZZLE_SHIFT               4
#define VKD3D_SM4_SWIZZLE_MASK                (0xffu << VKD3D_SM4_SWIZZLE_SHIFT)

#define VKD3D_SM4_VERSION_MAJOR(version)      (((version) >> 4) & 0xf)
#define VKD3D_SM4_VERSION_MINOR(version)      (((version) >> 0) & 0xf)

#define VKD3D_SM4_ADDRESSING_RELATIVE         0x2
#define VKD3D_SM4_ADDRESSING_OFFSET           0x1

#define VKD3D_SM4_INSTRUCTION_FLAG_SATURATE   0x4

#define VKD3D_SM4_CONDITIONAL_NZ              (0x1u << 18)

#define VKD3D_SM4_TYPE_COMPONENT(com, i)      (((com) >> (4 * (i))) & 0xfu)

/* The shift that corresponds to the D3D_SIF_TEXTURE_COMPONENTS mask. */
#define VKD3D_SM4_SIF_TEXTURE_COMPONENTS_SHIFT 2

enum vkd3d_sm4_opcode
{
    VKD3D_SM4_OP_ADD                              = 0x00,
    VKD3D_SM4_OP_AND                              = 0x01,
    VKD3D_SM4_OP_BREAK                            = 0x02,
    VKD3D_SM4_OP_BREAKC                           = 0x03,
    VKD3D_SM4_OP_CASE                             = 0x06,
    VKD3D_SM4_OP_CONTINUE                         = 0x07,
    VKD3D_SM4_OP_CONTINUEC                        = 0x08,
    VKD3D_SM4_OP_CUT                              = 0x09,
    VKD3D_SM4_OP_DEFAULT                          = 0x0a,
    VKD3D_SM4_OP_DERIV_RTX                        = 0x0b,
    VKD3D_SM4_OP_DERIV_RTY                        = 0x0c,
    VKD3D_SM4_OP_DISCARD                          = 0x0d,
    VKD3D_SM4_OP_DIV                              = 0x0e,
    VKD3D_SM4_OP_DP2                              = 0x0f,
    VKD3D_SM4_OP_DP3                              = 0x10,
    VKD3D_SM4_OP_DP4                              = 0x11,
    VKD3D_SM4_OP_ELSE                             = 0x12,
    VKD3D_SM4_OP_EMIT                             = 0x13,
    VKD3D_SM4_OP_ENDIF                            = 0x15,
    VKD3D_SM4_OP_ENDLOOP                          = 0x16,
    VKD3D_SM4_OP_ENDSWITCH                        = 0x17,
    VKD3D_SM4_OP_EQ                               = 0x18,
    VKD3D_SM4_OP_EXP                              = 0x19,
    VKD3D_SM4_OP_FRC                              = 0x1a,
    VKD3D_SM4_OP_FTOI                             = 0x1b,
    VKD3D_SM4_OP_FTOU                             = 0x1c,
    VKD3D_SM4_OP_GE                               = 0x1d,
    VKD3D_SM4_OP_IADD                             = 0x1e,
    VKD3D_SM4_OP_IF                               = 0x1f,
    VKD3D_SM4_OP_IEQ                              = 0x20,
    VKD3D_SM4_OP_IGE                              = 0x21,
    VKD3D_SM4_OP_ILT                              = 0x22,
    VKD3D_SM4_OP_IMAD                             = 0x23,
    VKD3D_SM4_OP_IMAX                             = 0x24,
    VKD3D_SM4_OP_IMIN                             = 0x25,
    VKD3D_SM4_OP_IMUL                             = 0x26,
    VKD3D_SM4_OP_INE                              = 0x27,
    VKD3D_SM4_OP_INEG                             = 0x28,
    VKD3D_SM4_OP_ISHL                             = 0x29,
    VKD3D_SM4_OP_ISHR                             = 0x2a,
    VKD3D_SM4_OP_ITOF                             = 0x2b,
    VKD3D_SM4_OP_LABEL                            = 0x2c,
    VKD3D_SM4_OP_LD                               = 0x2d,
    VKD3D_SM4_OP_LD2DMS                           = 0x2e,
    VKD3D_SM4_OP_LOG                              = 0x2f,
    VKD3D_SM4_OP_LOOP                             = 0x30,
    VKD3D_SM4_OP_LT                               = 0x31,
    VKD3D_SM4_OP_MAD                              = 0x32,
    VKD3D_SM4_OP_MIN                              = 0x33,
    VKD3D_SM4_OP_MAX                              = 0x34,
    VKD3D_SM4_OP_SHADER_DATA                      = 0x35,
    VKD3D_SM4_OP_MOV                              = 0x36,
    VKD3D_SM4_OP_MOVC                             = 0x37,
    VKD3D_SM4_OP_MUL                              = 0x38,
    VKD3D_SM4_OP_NE                               = 0x39,
    VKD3D_SM4_OP_NOP                              = 0x3a,
    VKD3D_SM4_OP_NOT                              = 0x3b,
    VKD3D_SM4_OP_OR                               = 0x3c,
    VKD3D_SM4_OP_RESINFO                          = 0x3d,
    VKD3D_SM4_OP_RET                              = 0x3e,
    VKD3D_SM4_OP_RETC                             = 0x3f,
    VKD3D_SM4_OP_ROUND_NE                         = 0x40,
    VKD3D_SM4_OP_ROUND_NI                         = 0x41,
    VKD3D_SM4_OP_ROUND_PI                         = 0x42,
    VKD3D_SM4_OP_ROUND_Z                          = 0x43,
    VKD3D_SM4_OP_RSQ                              = 0x44,
    VKD3D_SM4_OP_SAMPLE                           = 0x45,
    VKD3D_SM4_OP_SAMPLE_C                         = 0x46,
    VKD3D_SM4_OP_SAMPLE_C_LZ                      = 0x47,
    VKD3D_SM4_OP_SAMPLE_LOD                       = 0x48,
    VKD3D_SM4_OP_SAMPLE_GRAD                      = 0x49,
    VKD3D_SM4_OP_SAMPLE_B                         = 0x4a,
    VKD3D_SM4_OP_SQRT                             = 0x4b,
    VKD3D_SM4_OP_SWITCH                           = 0x4c,
    VKD3D_SM4_OP_SINCOS                           = 0x4d,
    VKD3D_SM4_OP_UDIV                             = 0x4e,
    VKD3D_SM4_OP_ULT                              = 0x4f,
    VKD3D_SM4_OP_UGE                              = 0x50,
    VKD3D_SM4_OP_UMUL                             = 0x51,
    VKD3D_SM4_OP_UMAX                             = 0x53,
    VKD3D_SM4_OP_UMIN                             = 0x54,
    VKD3D_SM4_OP_USHR                             = 0x55,
    VKD3D_SM4_OP_UTOF                             = 0x56,
    VKD3D_SM4_OP_XOR                              = 0x57,
    VKD3D_SM4_OP_DCL_RESOURCE                     = 0x58,
    VKD3D_SM4_OP_DCL_CONSTANT_BUFFER              = 0x59,
    VKD3D_SM4_OP_DCL_SAMPLER                      = 0x5a,
    VKD3D_SM4_OP_DCL_INDEX_RANGE                  = 0x5b,
    VKD3D_SM4_OP_DCL_OUTPUT_TOPOLOGY              = 0x5c,
    VKD3D_SM4_OP_DCL_INPUT_PRIMITIVE              = 0x5d,
    VKD3D_SM4_OP_DCL_VERTICES_OUT                 = 0x5e,
    VKD3D_SM4_OP_DCL_INPUT                        = 0x5f,
    VKD3D_SM4_OP_DCL_INPUT_SGV                    = 0x60,
    VKD3D_SM4_OP_DCL_INPUT_SIV                    = 0x61,
    VKD3D_SM4_OP_DCL_INPUT_PS                     = 0x62,
    VKD3D_SM4_OP_DCL_INPUT_PS_SGV                 = 0x63,
    VKD3D_SM4_OP_DCL_INPUT_PS_SIV                 = 0x64,
    VKD3D_SM4_OP_DCL_OUTPUT                       = 0x65,
    VKD3D_SM4_OP_DCL_OUTPUT_SIV                   = 0x67,
    VKD3D_SM4_OP_DCL_TEMPS                        = 0x68,
    VKD3D_SM4_OP_DCL_INDEXABLE_TEMP               = 0x69,
    VKD3D_SM4_OP_DCL_GLOBAL_FLAGS                 = 0x6a,
    VKD3D_SM4_OP_LOD                              = 0x6c,
    VKD3D_SM4_OP_GATHER4                          = 0x6d,
    VKD3D_SM4_OP_SAMPLE_POS                       = 0x6e,
    VKD3D_SM4_OP_SAMPLE_INFO                      = 0x6f,
    VKD3D_SM5_OP_HS_DECLS                         = 0x71,
    VKD3D_SM5_OP_HS_CONTROL_POINT_PHASE           = 0x72,
    VKD3D_SM5_OP_HS_FORK_PHASE                    = 0x73,
    VKD3D_SM5_OP_HS_JOIN_PHASE                    = 0x74,
    VKD3D_SM5_OP_EMIT_STREAM                      = 0x75,
    VKD3D_SM5_OP_CUT_STREAM                       = 0x76,
    VKD3D_SM5_OP_FCALL                            = 0x78,
    VKD3D_SM5_OP_BUFINFO                          = 0x79,
    VKD3D_SM5_OP_DERIV_RTX_COARSE                 = 0x7a,
    VKD3D_SM5_OP_DERIV_RTX_FINE                   = 0x7b,
    VKD3D_SM5_OP_DERIV_RTY_COARSE                 = 0x7c,
    VKD3D_SM5_OP_DERIV_RTY_FINE                   = 0x7d,
    VKD3D_SM5_OP_GATHER4_C                        = 0x7e,
    VKD3D_SM5_OP_GATHER4_PO                       = 0x7f,
    VKD3D_SM5_OP_GATHER4_PO_C                     = 0x80,
    VKD3D_SM5_OP_RCP                              = 0x81,
    VKD3D_SM5_OP_F32TOF16                         = 0x82,
    VKD3D_SM5_OP_F16TOF32                         = 0x83,
    VKD3D_SM5_OP_COUNTBITS                        = 0x86,
    VKD3D_SM5_OP_FIRSTBIT_HI                      = 0x87,
    VKD3D_SM5_OP_FIRSTBIT_LO                      = 0x88,
    VKD3D_SM5_OP_FIRSTBIT_SHI                     = 0x89,
    VKD3D_SM5_OP_UBFE                             = 0x8a,
    VKD3D_SM5_OP_IBFE                             = 0x8b,
    VKD3D_SM5_OP_BFI                              = 0x8c,
    VKD3D_SM5_OP_BFREV                            = 0x8d,
    VKD3D_SM5_OP_SWAPC                            = 0x8e,
    VKD3D_SM5_OP_DCL_STREAM                       = 0x8f,
    VKD3D_SM5_OP_DCL_FUNCTION_BODY                = 0x90,
    VKD3D_SM5_OP_DCL_FUNCTION_TABLE               = 0x91,
    VKD3D_SM5_OP_DCL_INTERFACE                    = 0x92,
    VKD3D_SM5_OP_DCL_INPUT_CONTROL_POINT_COUNT    = 0x93,
    VKD3D_SM5_OP_DCL_OUTPUT_CONTROL_POINT_COUNT   = 0x94,
    VKD3D_SM5_OP_DCL_TESSELLATOR_DOMAIN           = 0x95,
    VKD3D_SM5_OP_DCL_TESSELLATOR_PARTITIONING     = 0x96,
    VKD3D_SM5_OP_DCL_TESSELLATOR_OUTPUT_PRIMITIVE = 0x97,
    VKD3D_SM5_OP_DCL_HS_MAX_TESSFACTOR            = 0x98,
    VKD3D_SM5_OP_DCL_HS_FORK_PHASE_INSTANCE_COUNT = 0x99,
    VKD3D_SM5_OP_DCL_HS_JOIN_PHASE_INSTANCE_COUNT = 0x9a,
    VKD3D_SM5_OP_DCL_THREAD_GROUP                 = 0x9b,
    VKD3D_SM5_OP_DCL_UAV_TYPED                    = 0x9c,
    VKD3D_SM5_OP_DCL_UAV_RAW                      = 0x9d,
    VKD3D_SM5_OP_DCL_UAV_STRUCTURED               = 0x9e,
    VKD3D_SM5_OP_DCL_TGSM_RAW                     = 0x9f,
    VKD3D_SM5_OP_DCL_TGSM_STRUCTURED              = 0xa0,
    VKD3D_SM5_OP_DCL_RESOURCE_RAW                 = 0xa1,
    VKD3D_SM5_OP_DCL_RESOURCE_STRUCTURED          = 0xa2,
    VKD3D_SM5_OP_LD_UAV_TYPED                     = 0xa3,
    VKD3D_SM5_OP_STORE_UAV_TYPED                  = 0xa4,
    VKD3D_SM5_OP_LD_RAW                           = 0xa5,
    VKD3D_SM5_OP_STORE_RAW                        = 0xa6,
    VKD3D_SM5_OP_LD_STRUCTURED                    = 0xa7,
    VKD3D_SM5_OP_STORE_STRUCTURED                 = 0xa8,
    VKD3D_SM5_OP_ATOMIC_AND                       = 0xa9,
    VKD3D_SM5_OP_ATOMIC_OR                        = 0xaa,
    VKD3D_SM5_OP_ATOMIC_XOR                       = 0xab,
    VKD3D_SM5_OP_ATOMIC_CMP_STORE                 = 0xac,
    VKD3D_SM5_OP_ATOMIC_IADD                      = 0xad,
    VKD3D_SM5_OP_ATOMIC_IMAX                      = 0xae,
    VKD3D_SM5_OP_ATOMIC_IMIN                      = 0xaf,
    VKD3D_SM5_OP_ATOMIC_UMAX                      = 0xb0,
    VKD3D_SM5_OP_ATOMIC_UMIN                      = 0xb1,
    VKD3D_SM5_OP_IMM_ATOMIC_ALLOC                 = 0xb2,
    VKD3D_SM5_OP_IMM_ATOMIC_CONSUME               = 0xb3,
    VKD3D_SM5_OP_IMM_ATOMIC_IADD                  = 0xb4,
    VKD3D_SM5_OP_IMM_ATOMIC_AND                   = 0xb5,
    VKD3D_SM5_OP_IMM_ATOMIC_OR                    = 0xb6,
    VKD3D_SM5_OP_IMM_ATOMIC_XOR                   = 0xb7,
    VKD3D_SM5_OP_IMM_ATOMIC_EXCH                  = 0xb8,
    VKD3D_SM5_OP_IMM_ATOMIC_CMP_EXCH              = 0xb9,
    VKD3D_SM5_OP_IMM_ATOMIC_IMAX                  = 0xba,
    VKD3D_SM5_OP_IMM_ATOMIC_IMIN                  = 0xbb,
    VKD3D_SM5_OP_IMM_ATOMIC_UMAX                  = 0xbc,
    VKD3D_SM5_OP_IMM_ATOMIC_UMIN                  = 0xbd,
    VKD3D_SM5_OP_SYNC                             = 0xbe,
    VKD3D_SM5_OP_DADD                             = 0xbf,
    VKD3D_SM5_OP_DMAX                             = 0xc0,
    VKD3D_SM5_OP_DMIN                             = 0xc1,
    VKD3D_SM5_OP_DMUL                             = 0xc2,
    VKD3D_SM5_OP_DEQ                              = 0xc3,
    VKD3D_SM5_OP_DGE                              = 0xc4,
    VKD3D_SM5_OP_DLT                              = 0xc5,
    VKD3D_SM5_OP_DNE                              = 0xc6,
    VKD3D_SM5_OP_DMOV                             = 0xc7,
    VKD3D_SM5_OP_DMOVC                            = 0xc8,
    VKD3D_SM5_OP_DTOF                             = 0xc9,
    VKD3D_SM5_OP_FTOD                             = 0xca,
    VKD3D_SM5_OP_EVAL_SAMPLE_INDEX                = 0xcc,
    VKD3D_SM5_OP_EVAL_CENTROID                    = 0xcd,
    VKD3D_SM5_OP_DCL_GS_INSTANCES                 = 0xce,
    VKD3D_SM5_OP_DDIV                             = 0xd2,
    VKD3D_SM5_OP_DFMA                             = 0xd3,
    VKD3D_SM5_OP_DRCP                             = 0xd4,
    VKD3D_SM5_OP_DTOI                             = 0xd6,
    VKD3D_SM5_OP_DTOU                             = 0xd7,
    VKD3D_SM5_OP_ITOD                             = 0xd8,
    VKD3D_SM5_OP_UTOD                             = 0xd9,
    VKD3D_SM5_OP_GATHER4_S                        = 0xdb,
    VKD3D_SM5_OP_GATHER4_C_S                      = 0xdc,
    VKD3D_SM5_OP_GATHER4_PO_S                     = 0xdd,
    VKD3D_SM5_OP_GATHER4_PO_C_S                   = 0xde,
    VKD3D_SM5_OP_LD_S                             = 0xdf,
    VKD3D_SM5_OP_LD2DMS_S                         = 0xe0,
    VKD3D_SM5_OP_LD_UAV_TYPED_S                   = 0xe1,
    VKD3D_SM5_OP_LD_RAW_S                         = 0xe2,
    VKD3D_SM5_OP_LD_STRUCTURED_S                  = 0xe3,
    VKD3D_SM5_OP_SAMPLE_LOD_S                     = 0xe4,
    VKD3D_SM5_OP_SAMPLE_C_LZ_S                    = 0xe5,
    VKD3D_SM5_OP_SAMPLE_CL_S                      = 0xe6,
    VKD3D_SM5_OP_SAMPLE_B_CL_S                    = 0xe7,
    VKD3D_SM5_OP_SAMPLE_GRAD_CL_S                 = 0xe8,
    VKD3D_SM5_OP_SAMPLE_C_CL_S                    = 0xe9,
    VKD3D_SM5_OP_CHECK_ACCESS_FULLY_MAPPED        = 0xea,
};

enum vkd3d_sm4_instruction_modifier
{
    VKD3D_SM4_MODIFIER_AOFFIMMI         = 0x1,
    VKD3D_SM5_MODIFIER_RESOURCE_TYPE    = 0x2,
    VKD3D_SM5_MODIFIER_DATA_TYPE        = 0x3,
};

enum vkd3d_sm4_register_type
{
    VKD3D_SM4_RT_TEMP                    = 0x00,
    VKD3D_SM4_RT_INPUT                   = 0x01,
    VKD3D_SM4_RT_OUTPUT                  = 0x02,
    VKD3D_SM4_RT_INDEXABLE_TEMP          = 0x03,
    VKD3D_SM4_RT_IMMCONST                = 0x04,
    VKD3D_SM4_RT_IMMCONST64              = 0x05,
    VKD3D_SM4_RT_SAMPLER                 = 0x06,
    VKD3D_SM4_RT_RESOURCE                = 0x07,
    VKD3D_SM4_RT_CONSTBUFFER             = 0x08,
    VKD3D_SM4_RT_IMMCONSTBUFFER          = 0x09,
    VKD3D_SM4_RT_PRIMID                  = 0x0b,
    VKD3D_SM4_RT_DEPTHOUT                = 0x0c,
    VKD3D_SM4_RT_NULL                    = 0x0d,
    VKD3D_SM4_RT_RASTERIZER              = 0x0e,
    VKD3D_SM4_RT_OMASK                   = 0x0f,
    VKD3D_SM5_RT_STREAM                  = 0x10,
    VKD3D_SM5_RT_FUNCTION_BODY           = 0x11,
    VKD3D_SM5_RT_FUNCTION_POINTER        = 0x13,
    VKD3D_SM5_RT_OUTPUT_CONTROL_POINT_ID = 0x16,
    VKD3D_SM5_RT_FORK_INSTANCE_ID        = 0x17,
    VKD3D_SM5_RT_JOIN_INSTANCE_ID        = 0x18,
    VKD3D_SM5_RT_INPUT_CONTROL_POINT     = 0x19,
    VKD3D_SM5_RT_OUTPUT_CONTROL_POINT    = 0x1a,
    VKD3D_SM5_RT_PATCH_CONSTANT_DATA     = 0x1b,
    VKD3D_SM5_RT_DOMAIN_LOCATION         = 0x1c,
    VKD3D_SM5_RT_UAV                     = 0x1e,
    VKD3D_SM5_RT_SHARED_MEMORY           = 0x1f,
    VKD3D_SM5_RT_THREAD_ID               = 0x20,
    VKD3D_SM5_RT_THREAD_GROUP_ID         = 0x21,
    VKD3D_SM5_RT_LOCAL_THREAD_ID         = 0x22,
    VKD3D_SM5_RT_COVERAGE                = 0x23,
    VKD3D_SM5_RT_LOCAL_THREAD_INDEX      = 0x24,
    VKD3D_SM5_RT_GS_INSTANCE_ID          = 0x25,
    VKD3D_SM5_RT_DEPTHOUT_GREATER_EQUAL  = 0x26,
    VKD3D_SM5_RT_DEPTHOUT_LESS_EQUAL     = 0x27,
    VKD3D_SM5_RT_OUTPUT_STENCIL_REF      = 0x29,
};

enum vkd3d_sm4_extended_operand_type
{
    VKD3D_SM4_EXTENDED_OPERAND_NONE      = 0x0,
    VKD3D_SM4_EXTENDED_OPERAND_MODIFIER  = 0x1,
};

enum vkd3d_sm4_register_modifier
{
    VKD3D_SM4_REGISTER_MODIFIER_NONE       = 0x00,
    VKD3D_SM4_REGISTER_MODIFIER_NEGATE     = 0x01,
    VKD3D_SM4_REGISTER_MODIFIER_ABS        = 0x02,
    VKD3D_SM4_REGISTER_MODIFIER_ABS_NEGATE = 0x03,
};

enum vkd3d_sm4_register_precision
{
    VKD3D_SM4_REGISTER_PRECISION_DEFAULT      = 0x0,
    VKD3D_SM4_REGISTER_PRECISION_MIN_FLOAT_16 = 0x1,
    VKD3D_SM4_REGISTER_PRECISION_MIN_FLOAT_10 = 0x2,
    VKD3D_SM4_REGISTER_PRECISION_MIN_INT_16   = 0x4,
    VKD3D_SM4_REGISTER_PRECISION_MIN_UINT_16  = 0x5,
};

enum vkd3d_sm4_output_primitive_type
{
    VKD3D_SM4_OUTPUT_PT_POINTLIST     = 0x1,
    VKD3D_SM4_OUTPUT_PT_LINESTRIP     = 0x3,
    VKD3D_SM4_OUTPUT_PT_TRIANGLESTRIP = 0x5,
};

enum vkd3d_sm4_input_primitive_type
{
    VKD3D_SM4_INPUT_PT_POINT          = 0x01,
    VKD3D_SM4_INPUT_PT_LINE           = 0x02,
    VKD3D_SM4_INPUT_PT_TRIANGLE       = 0x03,
    VKD3D_SM4_INPUT_PT_LINEADJ        = 0x06,
    VKD3D_SM4_INPUT_PT_TRIANGLEADJ    = 0x07,
    VKD3D_SM5_INPUT_PT_PATCH1         = 0x08,
    VKD3D_SM5_INPUT_PT_PATCH2         = 0x09,
    VKD3D_SM5_INPUT_PT_PATCH3         = 0x0a,
    VKD3D_SM5_INPUT_PT_PATCH4         = 0x0b,
    VKD3D_SM5_INPUT_PT_PATCH5         = 0x0c,
    VKD3D_SM5_INPUT_PT_PATCH6         = 0x0d,
    VKD3D_SM5_INPUT_PT_PATCH7         = 0x0e,
    VKD3D_SM5_INPUT_PT_PATCH8         = 0x0f,
    VKD3D_SM5_INPUT_PT_PATCH9         = 0x10,
    VKD3D_SM5_INPUT_PT_PATCH10        = 0x11,
    VKD3D_SM5_INPUT_PT_PATCH11        = 0x12,
    VKD3D_SM5_INPUT_PT_PATCH12        = 0x13,
    VKD3D_SM5_INPUT_PT_PATCH13        = 0x14,
    VKD3D_SM5_INPUT_PT_PATCH14        = 0x15,
    VKD3D_SM5_INPUT_PT_PATCH15        = 0x16,
    VKD3D_SM5_INPUT_PT_PATCH16        = 0x17,
    VKD3D_SM5_INPUT_PT_PATCH17        = 0x18,
    VKD3D_SM5_INPUT_PT_PATCH18        = 0x19,
    VKD3D_SM5_INPUT_PT_PATCH19        = 0x1a,
    VKD3D_SM5_INPUT_PT_PATCH20        = 0x1b,
    VKD3D_SM5_INPUT_PT_PATCH21        = 0x1c,
    VKD3D_SM5_INPUT_PT_PATCH22        = 0x1d,
    VKD3D_SM5_INPUT_PT_PATCH23        = 0x1e,
    VKD3D_SM5_INPUT_PT_PATCH24        = 0x1f,
    VKD3D_SM5_INPUT_PT_PATCH25        = 0x20,
    VKD3D_SM5_INPUT_PT_PATCH26        = 0x21,
    VKD3D_SM5_INPUT_PT_PATCH27        = 0x22,
    VKD3D_SM5_INPUT_PT_PATCH28        = 0x23,
    VKD3D_SM5_INPUT_PT_PATCH29        = 0x24,
    VKD3D_SM5_INPUT_PT_PATCH30        = 0x25,
    VKD3D_SM5_INPUT_PT_PATCH31        = 0x26,
    VKD3D_SM5_INPUT_PT_PATCH32        = 0x27,
};

enum vkd3d_sm4_swizzle_type
{
    VKD3D_SM4_SWIZZLE_NONE            = 0x0,
    VKD3D_SM4_SWIZZLE_VEC4            = 0x1,
    VKD3D_SM4_SWIZZLE_SCALAR          = 0x2,
};

enum vkd3d_sm4_dimension
{
    VKD3D_SM4_DIMENSION_NONE    = 0x0,
    VKD3D_SM4_DIMENSION_SCALAR  = 0x1,
    VKD3D_SM4_DIMENSION_VEC4    = 0x2,
};

enum vkd3d_sm4_resource_type
{
    VKD3D_SM4_RESOURCE_BUFFER             = 0x1,
    VKD3D_SM4_RESOURCE_TEXTURE_1D         = 0x2,
    VKD3D_SM4_RESOURCE_TEXTURE_2D         = 0x3,
    VKD3D_SM4_RESOURCE_TEXTURE_2DMS       = 0x4,
    VKD3D_SM4_RESOURCE_TEXTURE_3D         = 0x5,
    VKD3D_SM4_RESOURCE_TEXTURE_CUBE       = 0x6,
    VKD3D_SM4_RESOURCE_TEXTURE_1DARRAY    = 0x7,
    VKD3D_SM4_RESOURCE_TEXTURE_2DARRAY    = 0x8,
    VKD3D_SM4_RESOURCE_TEXTURE_2DMSARRAY  = 0x9,
    VKD3D_SM4_RESOURCE_TEXTURE_CUBEARRAY  = 0xa,
    VKD3D_SM4_RESOURCE_RAW_BUFFER         = 0xb,
    VKD3D_SM4_RESOURCE_STRUCTURED_BUFFER  = 0xc,
};

enum vkd3d_sm4_data_type
{
    VKD3D_SM4_DATA_UNORM     = 0x1,
    VKD3D_SM4_DATA_SNORM     = 0x2,
    VKD3D_SM4_DATA_INT       = 0x3,
    VKD3D_SM4_DATA_UINT      = 0x4,
    VKD3D_SM4_DATA_FLOAT     = 0x5,
    VKD3D_SM4_DATA_MIXED     = 0x6,
    VKD3D_SM4_DATA_DOUBLE    = 0x7,
    VKD3D_SM4_DATA_CONTINUED = 0x8,
    VKD3D_SM4_DATA_UNUSED    = 0x9,
};

enum vkd3d_sm4_sampler_mode
{
    VKD3D_SM4_SAMPLER_DEFAULT    = 0x0,
    VKD3D_SM4_SAMPLER_COMPARISON = 0x1,
};

enum vkd3d_sm4_shader_data_type
{
    VKD3D_SM4_SHADER_DATA_IMMEDIATE_CONSTANT_BUFFER = 0x3,
    VKD3D_SM4_SHADER_DATA_MESSAGE                   = 0x4,
};

#endif  /* __VKD3D_SM4_H */
