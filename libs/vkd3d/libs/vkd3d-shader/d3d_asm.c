/*
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
 * Copyright 2007-2008, 2013 Stefan Dösinger for CodeWeavers
 * Copyright 2009-2011 Henri Verbeet for CodeWeavers
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

#include <stdio.h>
#include <math.h>

static const char * const shader_opcode_names[] =
{
    [VKD3DSIH_ABS                             ] = "abs",
    [VKD3DSIH_ADD                             ] = "add",
    [VKD3DSIH_AND                             ] = "and",
    [VKD3DSIH_ATOMIC_AND                      ] = "atomic_and",
    [VKD3DSIH_ATOMIC_CMP_STORE                ] = "atomic_cmp_store",
    [VKD3DSIH_ATOMIC_IADD                     ] = "atomic_iadd",
    [VKD3DSIH_ATOMIC_IMAX                     ] = "atomic_imax",
    [VKD3DSIH_ATOMIC_IMIN                     ] = "atomic_imin",
    [VKD3DSIH_ATOMIC_OR                       ] = "atomic_or",
    [VKD3DSIH_ATOMIC_UMAX                     ] = "atomic_umax",
    [VKD3DSIH_ATOMIC_UMIN                     ] = "atomic_umin",
    [VKD3DSIH_ATOMIC_XOR                      ] = "atomic_xor",
    [VKD3DSIH_BEM                             ] = "bem",
    [VKD3DSIH_BFI                             ] = "bfi",
    [VKD3DSIH_BFREV                           ] = "bfrev",
    [VKD3DSIH_BREAK                           ] = "break",
    [VKD3DSIH_BREAKC                          ] = "breakc",
    [VKD3DSIH_BREAKP                          ] = "breakp",
    [VKD3DSIH_BUFINFO                         ] = "bufinfo",
    [VKD3DSIH_CALL                            ] = "call",
    [VKD3DSIH_CALLNZ                          ] = "callnz",
    [VKD3DSIH_CASE                            ] = "case",
    [VKD3DSIH_CHECK_ACCESS_FULLY_MAPPED       ] = "check_access_fully_mapped",
    [VKD3DSIH_CMP                             ] = "cmp",
    [VKD3DSIH_CND                             ] = "cnd",
    [VKD3DSIH_CONTINUE                        ] = "continue",
    [VKD3DSIH_CONTINUEP                       ] = "continuec",
    [VKD3DSIH_COUNTBITS                       ] = "countbits",
    [VKD3DSIH_CRS                             ] = "crs",
    [VKD3DSIH_CUT                             ] = "cut",
    [VKD3DSIH_CUT_STREAM                      ] = "cut_stream",
    [VKD3DSIH_DADD                            ] = "dadd",
    [VKD3DSIH_DCL                             ] = "dcl",
    [VKD3DSIH_DCL_CONSTANT_BUFFER             ] = "dcl_constantBuffer",
    [VKD3DSIH_DCL_FUNCTION_BODY               ] = "dcl_function_body",
    [VKD3DSIH_DCL_FUNCTION_TABLE              ] = "dcl_function_table",
    [VKD3DSIH_DCL_GLOBAL_FLAGS                ] = "dcl_globalFlags",
    [VKD3DSIH_DCL_GS_INSTANCES                ] = "dcl_gs_instances",
    [VKD3DSIH_DCL_HS_FORK_PHASE_INSTANCE_COUNT] = "dcl_hs_fork_phase_instance_count",
    [VKD3DSIH_DCL_HS_JOIN_PHASE_INSTANCE_COUNT] = "dcl_hs_join_phase_instance_count",
    [VKD3DSIH_DCL_HS_MAX_TESSFACTOR           ] = "dcl_hs_max_tessfactor",
    [VKD3DSIH_DCL_IMMEDIATE_CONSTANT_BUFFER   ] = "dcl_immediateConstantBuffer",
    [VKD3DSIH_DCL_INDEX_RANGE                 ] = "dcl_index_range",
    [VKD3DSIH_DCL_INDEXABLE_TEMP              ] = "dcl_indexableTemp",
    [VKD3DSIH_DCL_INPUT                       ] = "dcl_input",
    [VKD3DSIH_DCL_INPUT_CONTROL_POINT_COUNT   ] = "dcl_input_control_point_count",
    [VKD3DSIH_DCL_INPUT_PRIMITIVE             ] = "dcl_inputPrimitive",
    [VKD3DSIH_DCL_INPUT_PS                    ] = "dcl_input_ps",
    [VKD3DSIH_DCL_INPUT_PS_SGV                ] = "dcl_input_ps_sgv",
    [VKD3DSIH_DCL_INPUT_PS_SIV                ] = "dcl_input_ps_siv",
    [VKD3DSIH_DCL_INPUT_SGV                   ] = "dcl_input_sgv",
    [VKD3DSIH_DCL_INPUT_SIV                   ] = "dcl_input_siv",
    [VKD3DSIH_DCL_INTERFACE                   ] = "dcl_interface",
    [VKD3DSIH_DCL_OUTPUT                      ] = "dcl_output",
    [VKD3DSIH_DCL_OUTPUT_CONTROL_POINT_COUNT  ] = "dcl_output_control_point_count",
    [VKD3DSIH_DCL_OUTPUT_SIV                  ] = "dcl_output_siv",
    [VKD3DSIH_DCL_OUTPUT_TOPOLOGY             ] = "dcl_outputTopology",
    [VKD3DSIH_DCL_RESOURCE_RAW                ] = "dcl_resource_raw",
    [VKD3DSIH_DCL_RESOURCE_STRUCTURED         ] = "dcl_resource_structured",
    [VKD3DSIH_DCL_SAMPLER                     ] = "dcl_sampler",
    [VKD3DSIH_DCL_STREAM                      ] = "dcl_stream",
    [VKD3DSIH_DCL_TEMPS                       ] = "dcl_temps",
    [VKD3DSIH_DCL_TESSELLATOR_DOMAIN          ] = "dcl_tessellator_domain",
    [VKD3DSIH_DCL_TESSELLATOR_OUTPUT_PRIMITIVE] = "dcl_tessellator_output_primitive",
    [VKD3DSIH_DCL_TESSELLATOR_PARTITIONING    ] = "dcl_tessellator_partitioning",
    [VKD3DSIH_DCL_TGSM_RAW                    ] = "dcl_tgsm_raw",
    [VKD3DSIH_DCL_TGSM_STRUCTURED             ] = "dcl_tgsm_structured",
    [VKD3DSIH_DCL_THREAD_GROUP                ] = "dcl_thread_group",
    [VKD3DSIH_DCL_UAV_RAW                     ] = "dcl_uav_raw",
    [VKD3DSIH_DCL_UAV_STRUCTURED              ] = "dcl_uav_structured",
    [VKD3DSIH_DCL_UAV_TYPED                   ] = "dcl_uav_typed",
    [VKD3DSIH_DCL_VERTICES_OUT                ] = "dcl_maxOutputVertexCount",
    [VKD3DSIH_DDIV                            ] = "ddiv",
    [VKD3DSIH_DEF                             ] = "def",
    [VKD3DSIH_DEFAULT                         ] = "default",
    [VKD3DSIH_DEFB                            ] = "defb",
    [VKD3DSIH_DEFI                            ] = "defi",
    [VKD3DSIH_DEQO                            ] = "deq",
    [VKD3DSIH_DFMA                            ] = "dfma",
    [VKD3DSIH_DGEO                            ] = "dge",
    [VKD3DSIH_DISCARD                         ] = "discard",
    [VKD3DSIH_DIV                             ] = "div",
    [VKD3DSIH_DLT                             ] = "dlt",
    [VKD3DSIH_DMAX                            ] = "dmax",
    [VKD3DSIH_DMIN                            ] = "dmin",
    [VKD3DSIH_DMOV                            ] = "dmov",
    [VKD3DSIH_DMOVC                           ] = "dmovc",
    [VKD3DSIH_DMUL                            ] = "dmul",
    [VKD3DSIH_DNE                             ] = "dne",
    [VKD3DSIH_DP2                             ] = "dp2",
    [VKD3DSIH_DP2ADD                          ] = "dp2add",
    [VKD3DSIH_DP3                             ] = "dp3",
    [VKD3DSIH_DP4                             ] = "dp4",
    [VKD3DSIH_DRCP                            ] = "drcp",
    [VKD3DSIH_DST                             ] = "dst",
    [VKD3DSIH_DSX                             ] = "dsx",
    [VKD3DSIH_DSX_COARSE                      ] = "deriv_rtx_coarse",
    [VKD3DSIH_DSX_FINE                        ] = "deriv_rtx_fine",
    [VKD3DSIH_DSY                             ] = "dsy",
    [VKD3DSIH_DSY_COARSE                      ] = "deriv_rty_coarse",
    [VKD3DSIH_DSY_FINE                        ] = "deriv_rty_fine",
    [VKD3DSIH_DTOF                            ] = "dtof",
    [VKD3DSIH_DTOI                            ] = "dtoi",
    [VKD3DSIH_DTOU                            ] = "dtou",
    [VKD3DSIH_ELSE                            ] = "else",
    [VKD3DSIH_EMIT                            ] = "emit",
    [VKD3DSIH_EMIT_STREAM                     ] = "emit_stream",
    [VKD3DSIH_ENDIF                           ] = "endif",
    [VKD3DSIH_ENDLOOP                         ] = "endloop",
    [VKD3DSIH_ENDREP                          ] = "endrep",
    [VKD3DSIH_ENDSWITCH                       ] = "endswitch",
    [VKD3DSIH_EQO                             ] = "eq",
    [VKD3DSIH_EQU                             ] = "eq_unord",
    [VKD3DSIH_EVAL_CENTROID                   ] = "eval_centroid",
    [VKD3DSIH_EVAL_SAMPLE_INDEX               ] = "eval_sample_index",
    [VKD3DSIH_EXP                             ] = "exp",
    [VKD3DSIH_EXPP                            ] = "expp",
    [VKD3DSIH_F16TOF32                        ] = "f16tof32",
    [VKD3DSIH_F32TOF16                        ] = "f32tof16",
    [VKD3DSIH_FCALL                           ] = "fcall",
    [VKD3DSIH_FIRSTBIT_HI                     ] = "firstbit_hi",
    [VKD3DSIH_FIRSTBIT_LO                     ] = "firstbit_lo",
    [VKD3DSIH_FIRSTBIT_SHI                    ] = "firstbit_shi",
    [VKD3DSIH_FRC                             ] = "frc",
    [VKD3DSIH_FREM                            ] = "frem",
    [VKD3DSIH_FTOD                            ] = "ftod",
    [VKD3DSIH_FTOI                            ] = "ftoi",
    [VKD3DSIH_FTOU                            ] = "ftou",
    [VKD3DSIH_GATHER4                         ] = "gather4",
    [VKD3DSIH_GATHER4_C                       ] = "gather4_c",
    [VKD3DSIH_GATHER4_C_S                     ] = "gather4_c_s",
    [VKD3DSIH_GATHER4_PO                      ] = "gather4_po",
    [VKD3DSIH_GATHER4_PO_C                    ] = "gather4_po_c",
    [VKD3DSIH_GATHER4_PO_C_S                  ] = "gather4_po_c_s",
    [VKD3DSIH_GATHER4_PO_S                    ] = "gather4_po_s",
    [VKD3DSIH_GATHER4_S                       ] = "gather4_s",
    [VKD3DSIH_GEO                             ] = "ge",
    [VKD3DSIH_GEU                             ] = "ge_unord",
    [VKD3DSIH_HS_CONTROL_POINT_PHASE          ] = "hs_control_point_phase",
    [VKD3DSIH_HS_DECLS                        ] = "hs_decls",
    [VKD3DSIH_HS_FORK_PHASE                   ] = "hs_fork_phase",
    [VKD3DSIH_HS_JOIN_PHASE                   ] = "hs_join_phase",
    [VKD3DSIH_IADD                            ] = "iadd",
    [VKD3DSIH_IBFE                            ] = "ibfe",
    [VKD3DSIH_IDIV                            ] = "idiv",
    [VKD3DSIH_IEQ                             ] = "ieq",
    [VKD3DSIH_IF                              ] = "if",
    [VKD3DSIH_IFC                             ] = "ifc",
    [VKD3DSIH_IGE                             ] = "ige",
    [VKD3DSIH_ILT                             ] = "ilt",
    [VKD3DSIH_IMAD                            ] = "imad",
    [VKD3DSIH_IMAX                            ] = "imax",
    [VKD3DSIH_IMIN                            ] = "imin",
    [VKD3DSIH_IMM_ATOMIC_ALLOC                ] = "imm_atomic_alloc",
    [VKD3DSIH_IMM_ATOMIC_AND                  ] = "imm_atomic_and",
    [VKD3DSIH_IMM_ATOMIC_CMP_EXCH             ] = "imm_atomic_cmp_exch",
    [VKD3DSIH_IMM_ATOMIC_CONSUME              ] = "imm_atomic_consume",
    [VKD3DSIH_IMM_ATOMIC_EXCH                 ] = "imm_atomic_exch",
    [VKD3DSIH_IMM_ATOMIC_IADD                 ] = "imm_atomic_iadd",
    [VKD3DSIH_IMM_ATOMIC_IMAX                 ] = "imm_atomic_imax",
    [VKD3DSIH_IMM_ATOMIC_IMIN                 ] = "imm_atomic_imin",
    [VKD3DSIH_IMM_ATOMIC_OR                   ] = "imm_atomic_or",
    [VKD3DSIH_IMM_ATOMIC_UMAX                 ] = "imm_atomic_umax",
    [VKD3DSIH_IMM_ATOMIC_UMIN                 ] = "imm_atomic_umin",
    [VKD3DSIH_IMM_ATOMIC_XOR                  ] = "imm_atomic_xor",
    [VKD3DSIH_IMUL                            ] = "imul",
    [VKD3DSIH_INE                             ] = "ine",
    [VKD3DSIH_INEG                            ] = "ineg",
    [VKD3DSIH_ISHL                            ] = "ishl",
    [VKD3DSIH_ISHR                            ] = "ishr",
    [VKD3DSIH_ITOD                            ] = "itod",
    [VKD3DSIH_ITOF                            ] = "itof",
    [VKD3DSIH_ITOI                            ] = "itoi",
    [VKD3DSIH_LABEL                           ] = "label",
    [VKD3DSIH_LD                              ] = "ld",
    [VKD3DSIH_LD2DMS                          ] = "ld2dms",
    [VKD3DSIH_LD2DMS_S                        ] = "ld2dms_s",
    [VKD3DSIH_LD_RAW                          ] = "ld_raw",
    [VKD3DSIH_LD_RAW_S                        ] = "ld_raw_s",
    [VKD3DSIH_LD_S                            ] = "ld_s",
    [VKD3DSIH_LD_STRUCTURED                   ] = "ld_structured",
    [VKD3DSIH_LD_STRUCTURED_S                 ] = "ld_structured_s",
    [VKD3DSIH_LD_UAV_TYPED                    ] = "ld_uav_typed",
    [VKD3DSIH_LD_UAV_TYPED_S                  ] = "ld_uav_typed_s",
    [VKD3DSIH_LIT                             ] = "lit",
    [VKD3DSIH_LOD                             ] = "lod",
    [VKD3DSIH_LOG                             ] = "log",
    [VKD3DSIH_LOGP                            ] = "logp",
    [VKD3DSIH_LOOP                            ] = "loop",
    [VKD3DSIH_LRP                             ] = "lrp",
    [VKD3DSIH_LTO                             ] = "lt",
    [VKD3DSIH_LTU                             ] = "lt_unord",
    [VKD3DSIH_M3x2                            ] = "m3x2",
    [VKD3DSIH_M3x3                            ] = "m3x3",
    [VKD3DSIH_M3x4                            ] = "m3x4",
    [VKD3DSIH_M4x3                            ] = "m4x3",
    [VKD3DSIH_M4x4                            ] = "m4x4",
    [VKD3DSIH_MAD                             ] = "mad",
    [VKD3DSIH_MAX                             ] = "max",
    [VKD3DSIH_MIN                             ] = "min",
    [VKD3DSIH_MOV                             ] = "mov",
    [VKD3DSIH_MOVA                            ] = "mova",
    [VKD3DSIH_MOVC                            ] = "movc",
    [VKD3DSIH_MSAD                            ] = "msad",
    [VKD3DSIH_MUL                             ] = "mul",
    [VKD3DSIH_NEO                             ] = "ne_ord",
    [VKD3DSIH_NEU                             ] = "ne",
    [VKD3DSIH_NOP                             ] = "nop",
    [VKD3DSIH_NOT                             ] = "not",
    [VKD3DSIH_NRM                             ] = "nrm",
    [VKD3DSIH_OR                              ] = "or",
    [VKD3DSIH_PHASE                           ] = "phase",
    [VKD3DSIH_POW                             ] = "pow",
    [VKD3DSIH_RCP                             ] = "rcp",
    [VKD3DSIH_REP                             ] = "rep",
    [VKD3DSIH_RESINFO                         ] = "resinfo",
    [VKD3DSIH_RET                             ] = "ret",
    [VKD3DSIH_RETP                            ] = "retp",
    [VKD3DSIH_ROUND_NE                        ] = "round_ne",
    [VKD3DSIH_ROUND_NI                        ] = "round_ni",
    [VKD3DSIH_ROUND_PI                        ] = "round_pi",
    [VKD3DSIH_ROUND_Z                         ] = "round_z",
    [VKD3DSIH_RSQ                             ] = "rsq",
    [VKD3DSIH_SAMPLE                          ] = "sample",
    [VKD3DSIH_SAMPLE_B                        ] = "sample_b",
    [VKD3DSIH_SAMPLE_B_CL_S                   ] = "sample_b_cl_s",
    [VKD3DSIH_SAMPLE_C                        ] = "sample_c",
    [VKD3DSIH_SAMPLE_C_CL_S                   ] = "sample_c_cl_s",
    [VKD3DSIH_SAMPLE_C_LZ                     ] = "sample_c_lz",
    [VKD3DSIH_SAMPLE_C_LZ_S                   ] = "sample_c_lz_s",
    [VKD3DSIH_SAMPLE_CL_S                     ] = "sample_cl_s",
    [VKD3DSIH_SAMPLE_GRAD                     ] = "sample_d",
    [VKD3DSIH_SAMPLE_GRAD_CL_S                ] = "sample_d_cl_s",
    [VKD3DSIH_SAMPLE_INFO                     ] = "sample_info",
    [VKD3DSIH_SAMPLE_LOD                      ] = "sample_l",
    [VKD3DSIH_SAMPLE_LOD_S                    ] = "sample_l_s",
    [VKD3DSIH_SAMPLE_POS                      ] = "sample_pos",
    [VKD3DSIH_SETP                            ] = "setp",
    [VKD3DSIH_SGE                             ] = "sge",
    [VKD3DSIH_SGN                             ] = "sgn",
    [VKD3DSIH_SINCOS                          ] = "sincos",
    [VKD3DSIH_SLT                             ] = "slt",
    [VKD3DSIH_SQRT                            ] = "sqrt",
    [VKD3DSIH_STORE_RAW                       ] = "store_raw",
    [VKD3DSIH_STORE_STRUCTURED                ] = "store_structured",
    [VKD3DSIH_STORE_UAV_TYPED                 ] = "store_uav_typed",
    [VKD3DSIH_SUB                             ] = "sub",
    [VKD3DSIH_SWAPC                           ] = "swapc",
    [VKD3DSIH_SWITCH                          ] = "switch",
    [VKD3DSIH_SYNC                            ] = "sync",
    [VKD3DSIH_TEX                             ] = "texld",
    [VKD3DSIH_TEXBEM                          ] = "texbem",
    [VKD3DSIH_TEXBEML                         ] = "texbeml",
    [VKD3DSIH_TEXCOORD                        ] = "texcrd",
    [VKD3DSIH_TEXDEPTH                        ] = "texdepth",
    [VKD3DSIH_TEXDP3                          ] = "texdp3",
    [VKD3DSIH_TEXDP3TEX                       ] = "texdp3tex",
    [VKD3DSIH_TEXKILL                         ] = "texkill",
    [VKD3DSIH_TEXLDD                          ] = "texldd",
    [VKD3DSIH_TEXLDL                          ] = "texldl",
    [VKD3DSIH_TEXM3x2DEPTH                    ] = "texm3x2depth",
    [VKD3DSIH_TEXM3x2PAD                      ] = "texm3x2pad",
    [VKD3DSIH_TEXM3x2TEX                      ] = "texm3x2tex",
    [VKD3DSIH_TEXM3x3                         ] = "texm3x3",
    [VKD3DSIH_TEXM3x3DIFF                     ] = "texm3x3diff",
    [VKD3DSIH_TEXM3x3PAD                      ] = "texm3x3pad",
    [VKD3DSIH_TEXM3x3SPEC                     ] = "texm3x3spec",
    [VKD3DSIH_TEXM3x3TEX                      ] = "texm3x3tex",
    [VKD3DSIH_TEXM3x3VSPEC                    ] = "texm3x3vspec",
    [VKD3DSIH_TEXREG2AR                       ] = "texreg2ar",
    [VKD3DSIH_TEXREG2GB                       ] = "texreg2gb",
    [VKD3DSIH_TEXREG2RGB                      ] = "texreg2rgb",
    [VKD3DSIH_UBFE                            ] = "ubfe",
    [VKD3DSIH_UDIV                            ] = "udiv",
    [VKD3DSIH_UGE                             ] = "uge",
    [VKD3DSIH_ULT                             ] = "ult",
    [VKD3DSIH_UMAX                            ] = "umax",
    [VKD3DSIH_UMIN                            ] = "umin",
    [VKD3DSIH_UMUL                            ] = "umul",
    [VKD3DSIH_USHR                            ] = "ushr",
    [VKD3DSIH_UTOD                            ] = "utod",
    [VKD3DSIH_UTOF                            ] = "utof",
    [VKD3DSIH_UTOU                            ] = "utou",
    [VKD3DSIH_XOR                             ] = "xor",
};

static const struct
{
    enum vkd3d_shader_input_sysval_semantic sysval_semantic;
    const char *sysval_name;
}
shader_input_sysval_semantic_names[] =
{
    {VKD3D_SIV_POSITION,                   "position"},
    {VKD3D_SIV_CLIP_DISTANCE,              "clip_distance"},
    {VKD3D_SIV_CULL_DISTANCE,              "cull_distance"},
    {VKD3D_SIV_RENDER_TARGET_ARRAY_INDEX,  "render_target_array_index"},
    {VKD3D_SIV_VIEWPORT_ARRAY_INDEX,       "viewport_array_index"},
    {VKD3D_SIV_VERTEX_ID,                  "vertex_id"},
    {VKD3D_SIV_INSTANCE_ID,                "instance_id"},
    {VKD3D_SIV_PRIMITIVE_ID,               "primitive_id"},
    {VKD3D_SIV_IS_FRONT_FACE,              "is_front_face"},
    {VKD3D_SIV_SAMPLE_INDEX,               "sample_index"},
    {VKD3D_SIV_QUAD_U0_TESS_FACTOR,        "finalQuadUeq0EdgeTessFactor"},
    {VKD3D_SIV_QUAD_V0_TESS_FACTOR,        "finalQuadVeq0EdgeTessFactor"},
    {VKD3D_SIV_QUAD_U1_TESS_FACTOR,        "finalQuadUeq1EdgeTessFactor"},
    {VKD3D_SIV_QUAD_V1_TESS_FACTOR,        "finalQuadVeq1EdgeTessFactor"},
    {VKD3D_SIV_QUAD_U_INNER_TESS_FACTOR,   "finalQuadUInsideTessFactor"},
    {VKD3D_SIV_QUAD_V_INNER_TESS_FACTOR,   "finalQuadVInsideTessFactor"},
    {VKD3D_SIV_TRIANGLE_U_TESS_FACTOR,     "finalTriUeq0EdgeTessFactor"},
    {VKD3D_SIV_TRIANGLE_V_TESS_FACTOR,     "finalTriVeq0EdgeTessFactor"},
    {VKD3D_SIV_TRIANGLE_W_TESS_FACTOR,     "finalTriWeq0EdgeTessFactor"},
    {VKD3D_SIV_TRIANGLE_INNER_TESS_FACTOR, "finalTriInsideTessFactor"},
    {VKD3D_SIV_LINE_DETAIL_TESS_FACTOR,    "finalLineDetailTessFactor"},
    {VKD3D_SIV_LINE_DENSITY_TESS_FACTOR,   "finalLineDensityTessFactor"},
};

struct vkd3d_d3d_asm_colours
{
    const char *reset;
    const char *error;
    const char *literal;
    const char *modifier;
    const char *opcode;
    const char *reg;
    const char *swizzle;
    const char *version;
    const char *write_mask;
};

struct vkd3d_d3d_asm_compiler
{
    struct vkd3d_string_buffer buffer;
    struct vkd3d_shader_version shader_version;
    struct vkd3d_d3d_asm_colours colours;
};

static int VKD3D_PRINTF_FUNC(2, 3) shader_addline(struct vkd3d_string_buffer *buffer, const char *format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = vkd3d_string_buffer_vprintf(buffer, format, args);
    va_end(args);

    return ret;
}

/* Convert floating point offset relative to a register file to an absolute
 * offset for float constants. */
static unsigned int shader_get_float_offset(enum vkd3d_shader_register_type register_type, UINT register_idx)
{
    switch (register_type)
    {
        case VKD3DSPR_CONST: return register_idx;
        case VKD3DSPR_CONST2: return 2048 + register_idx;
        case VKD3DSPR_CONST3: return 4096 + register_idx;
        case VKD3DSPR_CONST4: return 6144 + register_idx;
        default:
            FIXME("Unsupported register type: %u.\n", register_type);
            return register_idx;
    }
}

static void shader_dump_global_flags(struct vkd3d_d3d_asm_compiler *compiler,
        enum vkd3d_shader_global_flags global_flags)
{
    unsigned int i;

    static const struct
    {
        enum vkd3d_shader_global_flags flag;
        const char *name;
    }
    global_flag_info[] =
    {
        {VKD3DSGF_REFACTORING_ALLOWED,               "refactoringAllowed"},
        {VKD3DSGF_FORCE_EARLY_DEPTH_STENCIL,         "forceEarlyDepthStencil"},
        {VKD3DSGF_ENABLE_RAW_AND_STRUCTURED_BUFFERS, "enableRawAndStructuredBuffers"},
        {VKD3DSGF_ENABLE_MINIMUM_PRECISION,          "enableMinimumPrecision"},
        {VKD3DSGF_SKIP_OPTIMIZATION,                 "skipOptimization"},
        {VKD3DSGF_ENABLE_DOUBLE_PRECISION_FLOAT_OPS, "enableDoublePrecisionFloatOps"},
        {VKD3DSGF_ENABLE_11_1_DOUBLE_EXTENSIONS,     "enable11_1DoubleExtensions"},
    };

    for (i = 0; i < ARRAY_SIZE(global_flag_info); ++i)
    {
        if (global_flags & global_flag_info[i].flag)
        {
            vkd3d_string_buffer_printf(&compiler->buffer, "%s", global_flag_info[i].name);
            global_flags &= ~global_flag_info[i].flag;
            if (global_flags)
                vkd3d_string_buffer_printf(&compiler->buffer, " | ");
        }
    }

    if (global_flags)
        vkd3d_string_buffer_printf(&compiler->buffer, "unknown_flags(%#"PRIx64")", (uint64_t)global_flags);
}

static void shader_dump_sync_flags(struct vkd3d_d3d_asm_compiler *compiler, uint32_t sync_flags)
{
    if (sync_flags & VKD3DSSF_GLOBAL_UAV)
    {
        vkd3d_string_buffer_printf(&compiler->buffer, "_uglobal");
        sync_flags &= ~VKD3DSSF_GLOBAL_UAV;
    }
    if (sync_flags & VKD3DSSF_GROUP_SHARED_MEMORY)
    {
        vkd3d_string_buffer_printf(&compiler->buffer, "_g");
        sync_flags &= ~VKD3DSSF_GROUP_SHARED_MEMORY;
    }
    if (sync_flags & VKD3DSSF_THREAD_GROUP)
    {
        vkd3d_string_buffer_printf(&compiler->buffer, "_t");
        sync_flags &= ~VKD3DSSF_THREAD_GROUP;
    }

    if (sync_flags)
        vkd3d_string_buffer_printf(&compiler->buffer, "_unknown_flags(%#x)", sync_flags);
}

static void shader_dump_precise_flags(struct vkd3d_d3d_asm_compiler *compiler, uint32_t flags)
{
    if (!(flags & VKD3DSI_PRECISE_XYZW))
        return;

    vkd3d_string_buffer_printf(&compiler->buffer, " [precise");
    if (flags != VKD3DSI_PRECISE_XYZW)
    {
        vkd3d_string_buffer_printf(&compiler->buffer, "(%s%s%s%s)",
                flags & VKD3DSI_PRECISE_X ? "x" : "",
                flags & VKD3DSI_PRECISE_Y ? "y" : "",
                flags & VKD3DSI_PRECISE_Z ? "z" : "",
                flags & VKD3DSI_PRECISE_W ? "w" : "");
    }
    vkd3d_string_buffer_printf(&compiler->buffer, "]");
}

static void shader_dump_uav_flags(struct vkd3d_d3d_asm_compiler *compiler, uint32_t uav_flags)
{
    if (uav_flags & VKD3DSUF_GLOBALLY_COHERENT)
    {
        vkd3d_string_buffer_printf(&compiler->buffer, "_glc");
        uav_flags &= ~VKD3DSUF_GLOBALLY_COHERENT;
    }
    if (uav_flags & VKD3DSUF_ORDER_PRESERVING_COUNTER)
    {
        vkd3d_string_buffer_printf(&compiler->buffer, "_opc");
        uav_flags &= ~VKD3DSUF_ORDER_PRESERVING_COUNTER;
    }
    if (uav_flags & VKD3DSUF_RASTERISER_ORDERED_VIEW)
    {
        vkd3d_string_buffer_printf(&compiler->buffer, "_rov");
        uav_flags &= ~VKD3DSUF_RASTERISER_ORDERED_VIEW;
    }

    if (uav_flags)
        vkd3d_string_buffer_printf(&compiler->buffer, "_unknown_flags(%#x)", uav_flags);
}

static void shader_dump_tessellator_domain(struct vkd3d_d3d_asm_compiler *compiler,
        enum vkd3d_tessellator_domain domain)
{
    struct vkd3d_string_buffer *buffer = &compiler->buffer;

    shader_addline(buffer, "domain_");
    switch (domain)
    {
        case VKD3D_TESSELLATOR_DOMAIN_LINE:
            shader_addline(buffer, "isoline");
            break;
        case VKD3D_TESSELLATOR_DOMAIN_TRIANGLE:
            shader_addline(buffer, "tri");
            break;
        case VKD3D_TESSELLATOR_DOMAIN_QUAD:
            shader_addline(buffer, "quad");
            break;
        default:
            shader_addline(buffer, "unknown_tessellator_domain(%#x)", domain);
            break;
    }
}

static void shader_dump_tessellator_output_primitive(struct vkd3d_d3d_asm_compiler *compiler,
        enum vkd3d_shader_tessellator_output_primitive output_primitive)
{
    struct vkd3d_string_buffer *buffer = &compiler->buffer;

    shader_addline(buffer, "output_");
    switch (output_primitive)
    {
        case VKD3D_SHADER_TESSELLATOR_OUTPUT_POINT:
            shader_addline(buffer, "point");
            break;
        case VKD3D_SHADER_TESSELLATOR_OUTPUT_LINE:
            shader_addline(buffer, "line");
            break;
        case VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CW:
            shader_addline(buffer, "triangle_cw");
            break;
        case VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CCW:
            shader_addline(buffer, "triangle_ccw");
            break;
        default:
            shader_addline(buffer, "unknown_tessellator_output_primitive(%#x)", output_primitive);
            break;
    }
}

static void shader_dump_tessellator_partitioning(struct vkd3d_d3d_asm_compiler *compiler,
        enum vkd3d_shader_tessellator_partitioning partitioning)
{
    struct vkd3d_string_buffer *buffer = &compiler->buffer;

    shader_addline(buffer, "partitioning_");
    switch (partitioning)
    {
        case VKD3D_SHADER_TESSELLATOR_PARTITIONING_INTEGER:
            shader_addline(buffer, "integer");
            break;
        case VKD3D_SHADER_TESSELLATOR_PARTITIONING_POW2:
            shader_addline(buffer, "pow2");
            break;
        case VKD3D_SHADER_TESSELLATOR_PARTITIONING_FRACTIONAL_ODD:
            shader_addline(buffer, "fractional_odd");
            break;
        case VKD3D_SHADER_TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN:
            shader_addline(buffer, "fractional_even");
            break;
        default:
            shader_addline(buffer, "unknown_tessellator_partitioning(%#x)", partitioning);
            break;
    }
}

static void shader_dump_shader_input_sysval_semantic(struct vkd3d_d3d_asm_compiler *compiler,
        enum vkd3d_shader_input_sysval_semantic semantic)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(shader_input_sysval_semantic_names); ++i)
    {
        if (shader_input_sysval_semantic_names[i].sysval_semantic == semantic)
        {
            vkd3d_string_buffer_printf(&compiler->buffer, "%s", shader_input_sysval_semantic_names[i].sysval_name);
            return;
        }
    }

    vkd3d_string_buffer_printf(&compiler->buffer, "unknown_shader_input_sysval_semantic(%#x)", semantic);
}

static void shader_dump_resource_type(struct vkd3d_d3d_asm_compiler *compiler, enum vkd3d_shader_resource_type type)
{
    static const char *const resource_type_names[] =
    {
        [VKD3D_SHADER_RESOURCE_NONE             ] = "none",
        [VKD3D_SHADER_RESOURCE_BUFFER           ] = "buffer",
        [VKD3D_SHADER_RESOURCE_TEXTURE_1D       ] = "texture1d",
        [VKD3D_SHADER_RESOURCE_TEXTURE_2D       ] = "texture2d",
        [VKD3D_SHADER_RESOURCE_TEXTURE_2DMS     ] = "texture2dms",
        [VKD3D_SHADER_RESOURCE_TEXTURE_3D       ] = "texture3d",
        [VKD3D_SHADER_RESOURCE_TEXTURE_CUBE     ] = "texturecube",
        [VKD3D_SHADER_RESOURCE_TEXTURE_1DARRAY  ] = "texture1darray",
        [VKD3D_SHADER_RESOURCE_TEXTURE_2DARRAY  ] = "texture2darray",
        [VKD3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY] = "texture2dmsarray",
        [VKD3D_SHADER_RESOURCE_TEXTURE_CUBEARRAY] = "texturecubearray",
    };

    if (type < ARRAY_SIZE(resource_type_names))
        vkd3d_string_buffer_printf(&compiler->buffer, "%s", resource_type_names[type]);
    else
        vkd3d_string_buffer_printf(&compiler->buffer, "unknown");
}

static void shader_dump_data_type(struct vkd3d_d3d_asm_compiler *compiler, const enum vkd3d_data_type *type)
{
    static const char *const data_type_names[] =
    {
        [VKD3D_DATA_FLOAT    ] = "float",
        [VKD3D_DATA_INT      ] = "int",
        [VKD3D_DATA_RESOURCE ] = "resource",
        [VKD3D_DATA_SAMPLER  ] = "sampler",
        [VKD3D_DATA_UAV      ] = "uav",
        [VKD3D_DATA_UINT     ] = "uint",
        [VKD3D_DATA_UNORM    ] = "unorm",
        [VKD3D_DATA_SNORM    ] = "snorm",
        [VKD3D_DATA_OPAQUE   ] = "opaque",
        [VKD3D_DATA_MIXED    ] = "mixed",
        [VKD3D_DATA_DOUBLE   ] = "double",
        [VKD3D_DATA_CONTINUED] = "<continued>",
        [VKD3D_DATA_UNUSED   ] = "<unused>",
    };
    const char *name;
    int i;

    vkd3d_string_buffer_printf(&compiler->buffer, "(");

    for (i = 0; i < 4; i++)
    {
        if (type[i] < ARRAY_SIZE(data_type_names))
            name = data_type_names[type[i]];
        else
            name = "unknown";
        vkd3d_string_buffer_printf(&compiler->buffer, "%s%s", i == 0 ? "" : ",", name);
    }

    vkd3d_string_buffer_printf(&compiler->buffer, ")");
}

static void shader_dump_decl_usage(struct vkd3d_d3d_asm_compiler *compiler,
        const struct vkd3d_shader_semantic *semantic, uint32_t flags)
{
    struct vkd3d_string_buffer *buffer = &compiler->buffer;

    if (semantic->resource.reg.reg.type == VKD3DSPR_COMBINED_SAMPLER)
    {
        switch (semantic->resource_type)
        {
            case VKD3D_SHADER_RESOURCE_TEXTURE_2D:
                shader_addline(buffer, "_2d");
                break;

            case VKD3D_SHADER_RESOURCE_TEXTURE_3D:
                shader_addline(buffer, "_volume");
                break;

            case VKD3D_SHADER_RESOURCE_TEXTURE_CUBE:
                shader_addline(buffer, "_cube");
                break;

            default:
                shader_addline(buffer, "_unknown_resource_type(%#x)", semantic->resource_type);
                break;
        }
    }
    else if (semantic->resource.reg.reg.type == VKD3DSPR_RESOURCE || semantic->resource.reg.reg.type == VKD3DSPR_UAV)
    {
        if (semantic->resource.reg.reg.type == VKD3DSPR_RESOURCE)
            shader_addline(buffer, "_resource");

        shader_addline(buffer, "_");
        shader_dump_resource_type(compiler, semantic->resource_type);
        if (semantic->resource_type == VKD3D_SHADER_RESOURCE_TEXTURE_2DMS
                || semantic->resource_type == VKD3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY)
        {
            shader_addline(buffer, "(%u)", semantic->sample_count);
        }
        if (semantic->resource.reg.reg.type == VKD3DSPR_UAV)
            shader_dump_uav_flags(compiler, flags);
        shader_addline(buffer, " ");
        shader_dump_data_type(compiler, semantic->resource_data_type);
    }
    else
    {
        /* Pixel shaders 3.0 don't have usage semantics. */
        if (!vkd3d_shader_ver_ge(&compiler->shader_version, 3, 0)
                && compiler->shader_version.type == VKD3D_SHADER_TYPE_PIXEL)
            return;
        else
            shader_addline(buffer, "_");

        switch (semantic->usage)
        {
            case VKD3D_DECL_USAGE_POSITION:
                shader_addline(buffer, "position%u", semantic->usage_idx);
                break;

            case VKD3D_DECL_USAGE_BLEND_INDICES:
                shader_addline(buffer, "blend");
                break;

            case VKD3D_DECL_USAGE_BLEND_WEIGHT:
                shader_addline(buffer, "weight");
                break;

            case VKD3D_DECL_USAGE_NORMAL:
                shader_addline(buffer, "normal%u", semantic->usage_idx);
                break;

            case VKD3D_DECL_USAGE_PSIZE:
                shader_addline(buffer, "psize");
                break;

            case VKD3D_DECL_USAGE_COLOR:
                if (!semantic->usage_idx)
                    shader_addline(buffer, "color");
                else
                    shader_addline(buffer, "specular%u", (semantic->usage_idx - 1));
                break;

            case VKD3D_DECL_USAGE_TEXCOORD:
                shader_addline(buffer, "texcoord%u", semantic->usage_idx);
                break;

            case VKD3D_DECL_USAGE_TANGENT:
                shader_addline(buffer, "tangent");
                break;

            case VKD3D_DECL_USAGE_BINORMAL:
                shader_addline(buffer, "binormal");
                break;

            case VKD3D_DECL_USAGE_TESS_FACTOR:
                shader_addline(buffer, "tessfactor");
                break;

            case VKD3D_DECL_USAGE_POSITIONT:
                shader_addline(buffer, "positionT%u", semantic->usage_idx);
                break;

            case VKD3D_DECL_USAGE_FOG:
                shader_addline(buffer, "fog");
                break;

            case VKD3D_DECL_USAGE_DEPTH:
                shader_addline(buffer, "depth");
                break;

            case VKD3D_DECL_USAGE_SAMPLE:
                shader_addline(buffer, "sample");
                break;

            default:
                shader_addline(buffer, "<unknown_semantic(%#x)>", semantic->usage);
                FIXME("Unrecognised semantic usage %#x.\n", semantic->usage);
        }
    }
}

static void shader_dump_src_param(struct vkd3d_d3d_asm_compiler *compiler,
        const struct vkd3d_shader_src_param *param);

static void shader_print_float_literal(struct vkd3d_d3d_asm_compiler *compiler,
        const char *prefix, float f, const char *suffix)
{
    const char *sign = "";

    if (isfinite(f) && signbit(f))
    {
        sign = "-";
        f = -f;
    }

    vkd3d_string_buffer_printf(&compiler->buffer, "%s%s%s", prefix, sign, compiler->colours.literal);
    vkd3d_string_buffer_print_f32(&compiler->buffer, f);
    vkd3d_string_buffer_printf(&compiler->buffer, "%s%s", compiler->colours.reset, suffix);
}

static void shader_print_double_literal(struct vkd3d_d3d_asm_compiler *compiler,
        const char *prefix, double d, const char *suffix)
{
    const char *sign = "";

    if (isfinite(d) && signbit(d))
    {
        sign = "-";
        d = -d;
    }

    vkd3d_string_buffer_printf(&compiler->buffer, "%s%s%s", prefix, sign, compiler->colours.literal);
    vkd3d_string_buffer_print_f64(&compiler->buffer, d);
    vkd3d_string_buffer_printf(&compiler->buffer, "l%s%s", compiler->colours.reset, suffix);
}

static void shader_print_int_literal(struct vkd3d_d3d_asm_compiler *compiler,
        const char *prefix, int i, const char *suffix)
{
    if (i < 0)
        vkd3d_string_buffer_printf(&compiler->buffer, "%s-%s%d%s%s",
                prefix, compiler->colours.literal, -i, compiler->colours.reset, suffix);
    else
        vkd3d_string_buffer_printf(&compiler->buffer, "%s%s%d%s%s",
                prefix, compiler->colours.literal, i, compiler->colours.reset, suffix);
}

static void shader_print_uint_literal(struct vkd3d_d3d_asm_compiler *compiler,
        const char *prefix, unsigned int i, const char *suffix)
{
    vkd3d_string_buffer_printf(&compiler->buffer, "%s%s%u%s%s",
            prefix, compiler->colours.literal, i, compiler->colours.reset, suffix);
}

static void shader_print_hex_literal(struct vkd3d_d3d_asm_compiler *compiler,
        const char *prefix, unsigned int i, const char *suffix)
{
    vkd3d_string_buffer_printf(&compiler->buffer, "%s%s0x%08x%s%s",
            prefix, compiler->colours.literal, i, compiler->colours.reset, suffix);
}

static void shader_print_bool_literal(struct vkd3d_d3d_asm_compiler *compiler,
        const char *prefix, unsigned int b, const char *suffix)
{
    vkd3d_string_buffer_printf(&compiler->buffer, "%s%s%s%s%s", prefix,
            compiler->colours.literal, b ? "true" : "false", compiler->colours.reset, suffix);
}

static void shader_print_subscript(struct vkd3d_d3d_asm_compiler *compiler,
        unsigned int offset, const struct vkd3d_shader_src_param *rel_addr)
{
    vkd3d_string_buffer_printf(&compiler->buffer, "[");
    if (rel_addr)
    {
        shader_dump_src_param(compiler, rel_addr);
        vkd3d_string_buffer_printf(&compiler->buffer, " + ");
    }
    shader_print_uint_literal(compiler, "", offset, "]");
}

static void shader_print_subscript_range(struct vkd3d_d3d_asm_compiler *compiler,
        unsigned int offset_first, unsigned int offset_last)
{
    shader_print_uint_literal(compiler, "[", offset_first, ":");
    if (offset_last != ~0u)
        shader_print_uint_literal(compiler, "", offset_last, "]");
    else
        vkd3d_string_buffer_printf(&compiler->buffer, "*]");
}

static void shader_dump_register(struct vkd3d_d3d_asm_compiler *compiler, const struct vkd3d_shader_register *reg,
        bool is_declaration)
{
    struct vkd3d_string_buffer *buffer = &compiler->buffer;
    unsigned int offset = reg->idx[0].offset;
    bool is_descriptor = false;

    static const char * const rastout_reg_names[] = {"oPos", "oFog", "oPts"};
    static const char * const misctype_reg_names[] = {"vPos", "vFace"};

    shader_addline(buffer, "%s", compiler->colours.reg);
    switch (reg->type)
    {
        case VKD3DSPR_TEMP:
            shader_addline(buffer, "r");
            break;

        case VKD3DSPR_INPUT:
            shader_addline(buffer, "v");
            break;

        case VKD3DSPR_CONST:
        case VKD3DSPR_CONST2:
        case VKD3DSPR_CONST3:
        case VKD3DSPR_CONST4:
            shader_addline(buffer, "c");
            offset = shader_get_float_offset(reg->type, offset);
            break;

        case VKD3DSPR_TEXTURE: /* vs: case VKD3DSPR_ADDR */
            vkd3d_string_buffer_printf(buffer, "%c",
                    compiler->shader_version.type == VKD3D_SHADER_TYPE_PIXEL ? 't' : 'a');
            break;

        case VKD3DSPR_RASTOUT:
            shader_addline(buffer, "%s", rastout_reg_names[offset]);
            break;

        case VKD3DSPR_COLOROUT:
            shader_addline(buffer, "oC");
            break;

        case VKD3DSPR_DEPTHOUT:
            shader_addline(buffer, "oDepth");
            break;

        case VKD3DSPR_DEPTHOUTGE:
            shader_addline(buffer, "oDepthGE");
            break;

        case VKD3DSPR_DEPTHOUTLE:
            shader_addline(buffer, "oDepthLE");
            break;

        case VKD3DSPR_ATTROUT:
            shader_addline(buffer, "oD");
            break;

        case VKD3DSPR_TEXCRDOUT:
            /* Vertex shaders >= 3.0 use general purpose output registers
             * (VKD3DSPR_OUTPUT), which can include an address token. */
            if (vkd3d_shader_ver_ge(&compiler->shader_version, 3, 0))
                shader_addline(buffer, "o");
            else
                shader_addline(buffer, "oT");
            break;

        case VKD3DSPR_CONSTINT:
            shader_addline(buffer, "i");
            break;

        case VKD3DSPR_CONSTBOOL:
            shader_addline(buffer, "b");
            break;

        case VKD3DSPR_LABEL:
            shader_addline(buffer, "l");
            break;

        case VKD3DSPR_LOOP:
            shader_addline(buffer, "aL");
            break;

        case VKD3DSPR_COMBINED_SAMPLER:
        case VKD3DSPR_SAMPLER:
            shader_addline(buffer, "s");
            is_descriptor = true;
            break;

        case VKD3DSPR_MISCTYPE:
            if (offset > 1)
            {
                FIXME("Unhandled misctype register %u.\n", offset);
                shader_addline(buffer, "<unhandled misctype %#x>", offset);
            }
            else
            {
                shader_addline(buffer, "%s", misctype_reg_names[offset]);
            }
            break;

        case VKD3DSPR_PREDICATE:
            shader_addline(buffer, "p");
            break;

        case VKD3DSPR_IMMCONST:
            shader_addline(buffer, "l");
            break;

        case VKD3DSPR_IMMCONST64:
            shader_addline(buffer, "d");
            break;

        case VKD3DSPR_CONSTBUFFER:
            shader_addline(buffer, "cb");
            is_descriptor = true;
            break;

        case VKD3DSPR_IMMCONSTBUFFER:
            shader_addline(buffer, "icb");
            break;

        case VKD3DSPR_PRIMID:
            shader_addline(buffer, "primID");
            break;

        case VKD3DSPR_NULL:
            shader_addline(buffer, "null");
            break;

        case VKD3DSPR_RASTERIZER:
            shader_addline(buffer, "rasterizer");
            break;

        case VKD3DSPR_RESOURCE:
            shader_addline(buffer, "t");
            is_descriptor = true;
            break;

        case VKD3DSPR_UAV:
            shader_addline(buffer, "u");
            is_descriptor = true;
            break;

        case VKD3DSPR_OUTPOINTID:
            shader_addline(buffer, "vOutputControlPointID");
            break;

        case VKD3DSPR_FORKINSTID:
            shader_addline(buffer, "vForkInstanceId");
            break;

        case VKD3DSPR_JOININSTID:
            shader_addline(buffer, "vJoinInstanceId");
            break;

        case VKD3DSPR_INCONTROLPOINT:
            shader_addline(buffer, "vicp");
            break;

        case VKD3DSPR_OUTCONTROLPOINT:
            shader_addline(buffer, "vocp");
            break;

        case VKD3DSPR_PATCHCONST:
            shader_addline(buffer, "vpc");
            break;

        case VKD3DSPR_TESSCOORD:
            shader_addline(buffer, "vDomainLocation");
            break;

        case VKD3DSPR_GROUPSHAREDMEM:
            shader_addline(buffer, "g");
            break;

        case VKD3DSPR_THREADID:
            shader_addline(buffer, "vThreadID");
            break;

        case VKD3DSPR_THREADGROUPID:
            shader_addline(buffer, "vThreadGroupID");
            break;

        case VKD3DSPR_LOCALTHREADID:
            shader_addline(buffer, "vThreadIDInGroup");
            break;

        case VKD3DSPR_LOCALTHREADINDEX:
            shader_addline(buffer, "vThreadIDInGroupFlattened");
            break;

        case VKD3DSPR_IDXTEMP:
            shader_addline(buffer, "x");
            break;

        case VKD3DSPR_STREAM:
            shader_addline(buffer, "m");
            break;

        case VKD3DSPR_FUNCTIONBODY:
            shader_addline(buffer, "fb");
            break;

        case VKD3DSPR_FUNCTIONPOINTER:
            shader_addline(buffer, "fp");
            break;

        case VKD3DSPR_COVERAGE:
            shader_addline(buffer, "vCoverage");
            break;

        case VKD3DSPR_SAMPLEMASK:
            shader_addline(buffer, "oMask");
            break;

        case VKD3DSPR_GSINSTID:
            shader_addline(buffer, "vGSInstanceID");
            break;

        case VKD3DSPR_OUTSTENCILREF:
            shader_addline(buffer, "oStencilRef");
            break;

        case VKD3DSPR_UNDEF:
            shader_addline(buffer, "undef");
            break;

        case VKD3DSPR_SSA:
            shader_addline(buffer, "sr");
            break;

        default:
            shader_addline(buffer, "<unhandled_rtype(%#x)>", reg->type);
            break;
    }

    if (reg->type == VKD3DSPR_IMMCONST)
    {
        shader_addline(buffer, "%s(", compiler->colours.reset);
        switch (reg->dimension)
        {
            case VSIR_DIMENSION_SCALAR:
                switch (reg->data_type)
                {
                    case VKD3D_DATA_FLOAT:
                        shader_print_float_literal(compiler, "", reg->u.immconst_float[0], "");
                        break;
                    case VKD3D_DATA_INT:
                        shader_print_int_literal(compiler, "", reg->u.immconst_uint[0], "");
                        break;
                    case VKD3D_DATA_RESOURCE:
                    case VKD3D_DATA_SAMPLER:
                    case VKD3D_DATA_UINT:
                        shader_print_uint_literal(compiler, "", reg->u.immconst_uint[0], "");
                        break;
                    default:
                        shader_addline(buffer, "<unhandled data type %#x>", reg->data_type);
                        break;
                }
                break;

            case VSIR_DIMENSION_VEC4:
                switch (reg->data_type)
                {
                    case VKD3D_DATA_FLOAT:
                        shader_print_float_literal(compiler, "", reg->u.immconst_float[0], "");
                        shader_print_float_literal(compiler, ", ", reg->u.immconst_float[1], "");
                        shader_print_float_literal(compiler, ", ", reg->u.immconst_float[2], "");
                        shader_print_float_literal(compiler, ", ", reg->u.immconst_float[3], "");
                        break;
                    case VKD3D_DATA_INT:
                        shader_print_int_literal(compiler, "", reg->u.immconst_uint[0], "");
                        shader_print_int_literal(compiler, ", ", reg->u.immconst_uint[1], "");
                        shader_print_int_literal(compiler, ", ", reg->u.immconst_uint[2], "");
                        shader_print_int_literal(compiler, ", ", reg->u.immconst_uint[3], "");
                        break;
                    case VKD3D_DATA_RESOURCE:
                    case VKD3D_DATA_SAMPLER:
                    case VKD3D_DATA_UINT:
                        shader_print_uint_literal(compiler, "", reg->u.immconst_uint[0], "");
                        shader_print_uint_literal(compiler, ", ", reg->u.immconst_uint[1], "");
                        shader_print_uint_literal(compiler, ", ", reg->u.immconst_uint[2], "");
                        shader_print_uint_literal(compiler, ", ", reg->u.immconst_uint[3], "");
                        break;
                    default:
                        shader_addline(buffer, "<unhandled data type %#x>", reg->data_type);
                        break;
                }
                break;

            default:
                shader_addline(buffer, "<unhandled immconst dimension %#x>", reg->dimension);
                break;
        }
        shader_addline(buffer, ")");
    }
    else if (reg->type == VKD3DSPR_IMMCONST64)
    {
        shader_addline(buffer, "%s(", compiler->colours.reset);
        /* A double2 vector is treated as a float4 vector in enum vsir_dimension. */
        if (reg->dimension == VSIR_DIMENSION_SCALAR || reg->dimension == VSIR_DIMENSION_VEC4)
        {
            if (reg->data_type == VKD3D_DATA_DOUBLE)
            {
                shader_print_double_literal(compiler, "", reg->u.immconst_double[0], "");
                if (reg->dimension == VSIR_DIMENSION_VEC4)
                    shader_print_double_literal(compiler, ", ", reg->u.immconst_double[1], "");
            }
            else
            {
                shader_addline(buffer, "<unhandled data type %#x>", reg->data_type);
            }
        }
        else
        {
            shader_addline(buffer, "<unhandled immconst64 dimension %#x>", reg->dimension);
        }
        shader_addline(buffer, ")");
    }
    else if (reg->type != VKD3DSPR_RASTOUT
            && reg->type != VKD3DSPR_MISCTYPE
            && reg->type != VKD3DSPR_NULL
            && reg->type != VKD3DSPR_DEPTHOUT)
    {
        if (offset != ~0u)
        {
            bool is_sm_5_1 = vkd3d_shader_ver_ge(&compiler->shader_version, 5, 1);

            if (reg->idx[0].rel_addr || reg->type == VKD3DSPR_IMMCONSTBUFFER
                    || reg->type == VKD3DSPR_INCONTROLPOINT || (reg->type == VKD3DSPR_INPUT
                    && (compiler->shader_version.type == VKD3D_SHADER_TYPE_GEOMETRY
                    || compiler->shader_version.type == VKD3D_SHADER_TYPE_HULL)))
            {
                vkd3d_string_buffer_printf(buffer, "%s", compiler->colours.reset);
                shader_print_subscript(compiler, offset, reg->idx[0].rel_addr);
            }
            else
            {
                vkd3d_string_buffer_printf(buffer, "%u%s", offset, compiler->colours.reset);
            }

            /* For sm 5.1 descriptor declarations we need to print the register range instead of
             * a single register index. */
            if (is_descriptor && is_declaration && is_sm_5_1)
            {
                shader_print_subscript_range(compiler, reg->idx[1].offset, reg->idx[2].offset);
            }
            else if (reg->type != VKD3DSPR_SSA)
            {
                /* For descriptors in sm < 5.1 we move the reg->idx values up one slot
                 * to normalise with 5.1.
                 * Here we should ignore it if it's a descriptor in sm < 5.1. */
                if (reg->idx[1].offset != ~0u && (!is_descriptor || is_sm_5_1))
                    shader_print_subscript(compiler, reg->idx[1].offset, reg->idx[1].rel_addr);

                if (reg->idx[2].offset != ~0u)
                    shader_print_subscript(compiler, reg->idx[2].offset, reg->idx[2].rel_addr);
            }
        }
        else
        {
            shader_addline(buffer, "%s", compiler->colours.reset);
        }

        if (reg->type == VKD3DSPR_FUNCTIONPOINTER)
            shader_print_subscript(compiler, reg->u.fp_body_idx, NULL);
    }
    else
    {
        shader_addline(buffer, "%s", compiler->colours.reset);
    }
}

static void shader_print_precision(struct vkd3d_d3d_asm_compiler *compiler, const struct vkd3d_shader_register *reg)
{
    struct vkd3d_string_buffer *buffer = &compiler->buffer;
    const char *precision;

    if (reg->precision == VKD3D_SHADER_REGISTER_PRECISION_DEFAULT)
        return;

    switch (reg->precision)
    {
        case VKD3D_SHADER_REGISTER_PRECISION_MIN_FLOAT_16:
            precision = "min16f";
            break;

        case VKD3D_SHADER_REGISTER_PRECISION_MIN_FLOAT_10:
            precision = "min2_8f";
            break;

        case VKD3D_SHADER_REGISTER_PRECISION_MIN_INT_16:
            precision = "min16i";
            break;

        case VKD3D_SHADER_REGISTER_PRECISION_MIN_UINT_16:
            precision = "min16u";
            break;

        default:
            vkd3d_string_buffer_printf(buffer, " {%s<unhandled precision %#x>%s}",
                    compiler->colours.error, reg->precision, compiler->colours.reset);
            return;
    }
    vkd3d_string_buffer_printf(buffer, " {%s%s%s}", compiler->colours.modifier, precision, compiler->colours.reset);
}

static void shader_print_non_uniform(struct vkd3d_d3d_asm_compiler *compiler, const struct vkd3d_shader_register *reg)
{
    if (reg->non_uniform)
        vkd3d_string_buffer_printf(&compiler->buffer, " {%snonuniform%s}",
                compiler->colours.modifier, compiler->colours.reset);
}

static void shader_dump_dst_param(struct vkd3d_d3d_asm_compiler *compiler,
        const struct vkd3d_shader_dst_param *param, bool is_declaration)
{
    struct vkd3d_string_buffer *buffer = &compiler->buffer;
    uint32_t write_mask = param->write_mask;

    shader_dump_register(compiler, &param->reg, is_declaration);

    if (write_mask && param->reg.dimension == VSIR_DIMENSION_VEC4)
    {
        static const char write_mask_chars[] = "xyzw";

        if (param->reg.data_type == VKD3D_DATA_DOUBLE)
            write_mask = vkd3d_write_mask_32_from_64(write_mask);

        shader_addline(buffer, ".%s", compiler->colours.write_mask);
        if (write_mask & VKD3DSP_WRITEMASK_0)
            shader_addline(buffer, "%c", write_mask_chars[0]);
        if (write_mask & VKD3DSP_WRITEMASK_1)
            shader_addline(buffer, "%c", write_mask_chars[1]);
        if (write_mask & VKD3DSP_WRITEMASK_2)
            shader_addline(buffer, "%c", write_mask_chars[2]);
        if (write_mask & VKD3DSP_WRITEMASK_3)
            shader_addline(buffer, "%c", write_mask_chars[3]);
        shader_addline(buffer, "%s", compiler->colours.reset);
    }

    shader_print_precision(compiler, &param->reg);
    shader_print_non_uniform(compiler, &param->reg);
}

static void shader_dump_src_param(struct vkd3d_d3d_asm_compiler *compiler,
        const struct vkd3d_shader_src_param *param)
{
    enum vkd3d_shader_src_modifier src_modifier = param->modifiers;
    struct vkd3d_string_buffer *buffer = &compiler->buffer;
    uint32_t swizzle = param->swizzle;

    if (src_modifier == VKD3DSPSM_NEG
            || src_modifier == VKD3DSPSM_BIASNEG
            || src_modifier == VKD3DSPSM_SIGNNEG
            || src_modifier == VKD3DSPSM_X2NEG
            || src_modifier == VKD3DSPSM_ABSNEG)
        shader_addline(buffer, "-");
    else if (src_modifier == VKD3DSPSM_COMP)
        shader_addline(buffer, "1-");
    else if (src_modifier == VKD3DSPSM_NOT)
        shader_addline(buffer, "!");

    if (src_modifier == VKD3DSPSM_ABS || src_modifier == VKD3DSPSM_ABSNEG)
        shader_addline(buffer, "|");

    shader_dump_register(compiler, &param->reg, false);

    switch (src_modifier)
    {
        case VKD3DSPSM_NONE:    break;
        case VKD3DSPSM_NEG:     break;
        case VKD3DSPSM_NOT:     break;
        case VKD3DSPSM_BIAS:    shader_addline(buffer, "_bias"); break;
        case VKD3DSPSM_BIASNEG: shader_addline(buffer, "_bias"); break;
        case VKD3DSPSM_SIGN:    shader_addline(buffer, "_bx2"); break;
        case VKD3DSPSM_SIGNNEG: shader_addline(buffer, "_bx2"); break;
        case VKD3DSPSM_COMP:    break;
        case VKD3DSPSM_X2:      shader_addline(buffer, "_x2"); break;
        case VKD3DSPSM_X2NEG:   shader_addline(buffer, "_x2"); break;
        case VKD3DSPSM_DZ:      shader_addline(buffer, "_dz"); break;
        case VKD3DSPSM_DW:      shader_addline(buffer, "_dw"); break;
        case VKD3DSPSM_ABSNEG:
        case VKD3DSPSM_ABS:     /* handled later */ break;
        default:                  shader_addline(buffer, "_unknown_modifier(%#x)", src_modifier);
    }

    if (param->reg.type != VKD3DSPR_IMMCONST && param->reg.type != VKD3DSPR_IMMCONST64
            && param->reg.dimension == VSIR_DIMENSION_VEC4)
    {
        unsigned int swizzle_x = vkd3d_swizzle_get_component(swizzle, 0);
        unsigned int swizzle_y = vkd3d_swizzle_get_component(swizzle, 1);
        unsigned int swizzle_z = vkd3d_swizzle_get_component(swizzle, 2);
        unsigned int swizzle_w = vkd3d_swizzle_get_component(swizzle, 3);

        static const char swizzle_chars[] = "xyzw";

        if (swizzle_x == swizzle_y
                && swizzle_x == swizzle_z
                && swizzle_x == swizzle_w)
        {
            shader_addline(buffer, ".%s%c%s", compiler->colours.swizzle,
                    swizzle_chars[swizzle_x], compiler->colours.reset);
        }
        else
        {
            shader_addline(buffer, ".%s%c%c%c%c%s", compiler->colours.swizzle,
                    swizzle_chars[swizzle_x], swizzle_chars[swizzle_y],
                    swizzle_chars[swizzle_z], swizzle_chars[swizzle_w], compiler->colours.reset);
        }
    }
    if (src_modifier == VKD3DSPSM_ABS || src_modifier == VKD3DSPSM_ABSNEG)
        shader_addline(buffer, "|");

    shader_print_precision(compiler, &param->reg);
    shader_print_non_uniform(compiler, &param->reg);
}

static void shader_dump_ins_modifiers(struct vkd3d_d3d_asm_compiler *compiler,
        const struct vkd3d_shader_dst_param *dst)
{
    struct vkd3d_string_buffer *buffer = &compiler->buffer;
    uint32_t mmask = dst->modifiers;

    switch (dst->shift)
    {
        case 0: break;
        case 13: shader_addline(buffer, "_d8"); break;
        case 14: shader_addline(buffer, "_d4"); break;
        case 15: shader_addline(buffer, "_d2"); break;
        case 1: shader_addline(buffer, "_x2"); break;
        case 2: shader_addline(buffer, "_x4"); break;
        case 3: shader_addline(buffer, "_x8"); break;
        default: shader_addline(buffer, "_unhandled_shift(%d)", dst->shift); break;
    }

    if (mmask & VKD3DSPDM_SATURATE)         shader_addline(buffer, "_sat");
    if (mmask & VKD3DSPDM_PARTIALPRECISION) shader_addline(buffer, "_pp");
    if (mmask & VKD3DSPDM_MSAMPCENTROID)    shader_addline(buffer, "_centroid");

    mmask &= ~VKD3DSPDM_MASK;
    if (mmask) FIXME("Unrecognised modifier %#x.\n", mmask);
}

static void shader_dump_primitive_type(struct vkd3d_d3d_asm_compiler *compiler,
        const struct vkd3d_shader_primitive_type *primitive_type)
{
    struct vkd3d_string_buffer *buffer = &compiler->buffer;

    switch (primitive_type->type)
    {
        case VKD3D_PT_UNDEFINED:
            shader_addline(buffer, "undefined");
            break;
        case VKD3D_PT_POINTLIST:
            shader_addline(buffer, "pointlist");
            break;
        case VKD3D_PT_LINELIST:
            shader_addline(buffer, "linelist");
            break;
        case VKD3D_PT_LINESTRIP:
            shader_addline(buffer, "linestrip");
            break;
        case VKD3D_PT_TRIANGLELIST:
            shader_addline(buffer, "trianglelist");
            break;
        case VKD3D_PT_TRIANGLESTRIP:
            shader_addline(buffer, "trianglestrip");
            break;
        case VKD3D_PT_TRIANGLEFAN:
            shader_addline(buffer, "trianglefan");
            break;
        case VKD3D_PT_LINELIST_ADJ:
            shader_addline(buffer, "linelist_adj");
            break;
        case VKD3D_PT_LINESTRIP_ADJ:
            shader_addline(buffer, "linestrip_adj");
            break;
        case VKD3D_PT_TRIANGLELIST_ADJ:
            shader_addline(buffer, "trianglelist_adj");
            break;
        case VKD3D_PT_TRIANGLESTRIP_ADJ:
            shader_addline(buffer, "trianglestrip_adj");
            break;
        case VKD3D_PT_PATCH:
            shader_addline(buffer, "patch%u", primitive_type->patch_vertex_count);
            break;
        default:
            shader_addline(buffer, "<unrecognized_primitive_type %#x>", primitive_type->type);
            break;
    }
}

static void shader_dump_interpolation_mode(struct vkd3d_d3d_asm_compiler *compiler,
        enum vkd3d_shader_interpolation_mode interpolation_mode)
{
    struct vkd3d_string_buffer *buffer = &compiler->buffer;

    switch (interpolation_mode)
    {
        case VKD3DSIM_CONSTANT:
            shader_addline(buffer, "constant");
            break;
        case VKD3DSIM_LINEAR:
            shader_addline(buffer, "linear");
            break;
        case VKD3DSIM_LINEAR_CENTROID:
            shader_addline(buffer, "linear centroid");
            break;
        case VKD3DSIM_LINEAR_NOPERSPECTIVE:
            shader_addline(buffer, "linear noperspective");
            break;
        case VKD3DSIM_LINEAR_SAMPLE:
            shader_addline(buffer, "linear sample");
            break;
        case VKD3DSIM_LINEAR_NOPERSPECTIVE_CENTROID:
            shader_addline(buffer, "linear noperspective centroid");
            break;
        case VKD3DSIM_LINEAR_NOPERSPECTIVE_SAMPLE:
            shader_addline(buffer, "linear noperspective sample");
            break;
        default:
            shader_addline(buffer, "<unrecognized_interpolation_mode %#x>", interpolation_mode);
            break;
    }
}

const char *shader_get_type_prefix(enum vkd3d_shader_type type)
{
    switch (type)
    {
        case VKD3D_SHADER_TYPE_VERTEX:
            return "vs";

        case VKD3D_SHADER_TYPE_HULL:
            return "hs";

        case VKD3D_SHADER_TYPE_DOMAIN:
            return "ds";

        case VKD3D_SHADER_TYPE_GEOMETRY:
            return "gs";

        case VKD3D_SHADER_TYPE_PIXEL:
            return "ps";

        case VKD3D_SHADER_TYPE_COMPUTE:
            return "cs";

        case VKD3D_SHADER_TYPE_EFFECT:
            return "fx";

        case VKD3D_SHADER_TYPE_TEXTURE:
            return "tx";

        case VKD3D_SHADER_TYPE_LIBRARY:
            return "lib";

        default:
            FIXME("Unhandled shader type %#x.\n", type);
            return "unknown";
    }
}

static void shader_dump_instruction_flags(struct vkd3d_d3d_asm_compiler *compiler,
        const struct vkd3d_shader_instruction *ins)
{
    struct vkd3d_string_buffer *buffer = &compiler->buffer;

    switch (ins->handler_idx)
    {
        case VKD3DSIH_BREAKP:
        case VKD3DSIH_CONTINUEP:
        case VKD3DSIH_DISCARD:
        case VKD3DSIH_IF:
        case VKD3DSIH_RETP:
            switch (ins->flags)
            {
                case VKD3D_SHADER_CONDITIONAL_OP_NZ: shader_addline(buffer, "_nz"); break;
                case VKD3D_SHADER_CONDITIONAL_OP_Z:  shader_addline(buffer, "_z"); break;
                default: shader_addline(buffer, "_unrecognized(%#x)", ins->flags); break;
            }
            break;

        case VKD3DSIH_IFC:
        case VKD3DSIH_BREAKC:
            switch (ins->flags)
            {
                case VKD3D_SHADER_REL_OP_GT: shader_addline(buffer, "_gt"); break;
                case VKD3D_SHADER_REL_OP_EQ: shader_addline(buffer, "_eq"); break;
                case VKD3D_SHADER_REL_OP_GE: shader_addline(buffer, "_ge"); break;
                case VKD3D_SHADER_REL_OP_LT: shader_addline(buffer, "_lt"); break;
                case VKD3D_SHADER_REL_OP_NE: shader_addline(buffer, "_ne"); break;
                case VKD3D_SHADER_REL_OP_LE: shader_addline(buffer, "_le"); break;
                default: shader_addline(buffer, "_(%u)", ins->flags);
            }
            break;

        case VKD3DSIH_RESINFO:
            switch (ins->flags)
            {
                case VKD3DSI_NONE: break;
                case VKD3DSI_RESINFO_RCP_FLOAT: shader_addline(buffer, "_rcpFloat"); break;
                case VKD3DSI_RESINFO_UINT: shader_addline(buffer, "_uint"); break;
                default: shader_addline(buffer, "_unrecognized(%#x)", ins->flags);
            }
            break;

        case VKD3DSIH_SAMPLE_INFO:
            switch (ins->flags)
            {
                case VKD3DSI_NONE: break;
                case VKD3DSI_SAMPLE_INFO_UINT: shader_addline(buffer, "_uint"); break;
                default: shader_addline(buffer, "_unrecognized(%#x)", ins->flags);
            }
            break;

        case VKD3DSIH_SYNC:
            shader_dump_sync_flags(compiler, ins->flags);
            break;

        case VKD3DSIH_TEX:
            if (vkd3d_shader_ver_ge(&compiler->shader_version, 2, 0) && (ins->flags & VKD3DSI_TEXLD_PROJECT))
                shader_addline(buffer, "p");
            break;

        default:
            shader_dump_precise_flags(compiler, ins->flags);
            break;
    }
}

static void shader_dump_register_space(struct vkd3d_d3d_asm_compiler *compiler, unsigned int register_space)
{
    if (vkd3d_shader_ver_ge(&compiler->shader_version, 5, 1))
        shader_print_uint_literal(compiler, ", space=", register_space, "");
}

static void shader_print_opcode(struct vkd3d_d3d_asm_compiler *compiler, enum vkd3d_shader_opcode opcode)
{
    vkd3d_string_buffer_printf(&compiler->buffer, "%s%s%s", compiler->colours.opcode,
            shader_opcode_names[opcode], compiler->colours.reset);
}

static void shader_dump_icb(struct vkd3d_d3d_asm_compiler *compiler,
        const struct vkd3d_shader_immediate_constant_buffer *icb)
{
    struct vkd3d_string_buffer *buffer = &compiler->buffer;
    unsigned int i, j;

    vkd3d_string_buffer_printf(buffer, " {\n");
    if (icb->component_count == 1)
    {
        for (i = 0; i < icb->element_count; )
        {
            for (j = 0; i < icb->element_count && j < 4; ++i, ++j)
                shader_print_hex_literal(compiler, !j ? "    " : ", ", icb->data[i], "");
            vkd3d_string_buffer_printf(buffer, "\n");
        }
    }
    else
    {
        assert(icb->component_count == VKD3D_VEC4_SIZE);
        for (i = 0; i < icb->element_count; ++i)
        {
            shader_print_hex_literal(compiler, "    {", icb->data[4 * i + 0], "");
            shader_print_hex_literal(compiler, ", ", icb->data[4 * i + 1], "");
            shader_print_hex_literal(compiler, ", ", icb->data[4 * i + 2], "");
            shader_print_hex_literal(compiler, ", ", icb->data[4 * i + 3], "},\n");
        }
    }
    shader_addline(buffer, "}");
}

static void shader_dump_instruction(struct vkd3d_d3d_asm_compiler *compiler,
        const struct vkd3d_shader_instruction *ins)
{
    struct vkd3d_string_buffer *buffer = &compiler->buffer;
    unsigned int i;

    if (ins->predicate)
    {
        vkd3d_string_buffer_printf(buffer, "(");
        shader_dump_src_param(compiler, ins->predicate);
        vkd3d_string_buffer_printf(buffer, ") ");
    }

    /* PixWin marks instructions with the coissue flag with a '+' */
    if (ins->coissue)
        vkd3d_string_buffer_printf(buffer, "+");

    shader_print_opcode(compiler, ins->handler_idx);

    switch (ins->handler_idx)
    {
        case VKD3DSIH_DCL:
        case VKD3DSIH_DCL_UAV_TYPED:
            vkd3d_string_buffer_printf(buffer, "%s", compiler->colours.opcode);
            shader_dump_decl_usage(compiler, &ins->declaration.semantic, ins->flags);
            shader_dump_ins_modifiers(compiler, &ins->declaration.semantic.resource.reg);
            vkd3d_string_buffer_printf(buffer, "%s ", compiler->colours.reset);
            shader_dump_register(compiler, &ins->declaration.semantic.resource.reg.reg, true);
            shader_dump_register_space(compiler, ins->declaration.semantic.resource.range.space);
            break;

        case VKD3DSIH_DCL_CONSTANT_BUFFER:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_register(compiler, &ins->declaration.cb.src.reg, true);
            if (vkd3d_shader_ver_ge(&compiler->shader_version, 6, 0))
                shader_print_subscript(compiler, ins->declaration.cb.size, NULL);
            else if (vkd3d_shader_ver_ge(&compiler->shader_version, 5, 1))
                shader_print_subscript(compiler, ins->declaration.cb.size / VKD3D_VEC4_SIZE / sizeof(float), NULL);
            shader_addline(buffer, ", %s",
                    ins->flags & VKD3DSI_INDEXED_DYNAMIC ? "dynamicIndexed" : "immediateIndexed");
            shader_dump_register_space(compiler, ins->declaration.cb.range.space);
            break;

        case VKD3DSIH_DCL_FUNCTION_BODY:
            vkd3d_string_buffer_printf(buffer, " fb%u", ins->declaration.index);
            break;

        case VKD3DSIH_DCL_FUNCTION_TABLE:
            vkd3d_string_buffer_printf(buffer, " ft%u = {...}", ins->declaration.index);
            break;

        case VKD3DSIH_DCL_GLOBAL_FLAGS:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_global_flags(compiler, ins->declaration.global_flags);
            break;

        case VKD3DSIH_DCL_HS_MAX_TESSFACTOR:
            shader_print_float_literal(compiler, " ", ins->declaration.max_tessellation_factor, "");
            break;

        case VKD3DSIH_DCL_IMMEDIATE_CONSTANT_BUFFER:
            shader_dump_icb(compiler, ins->declaration.icb);
            break;

        case VKD3DSIH_DCL_INDEX_RANGE:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_dst_param(compiler, &ins->declaration.index_range.dst, true);
            shader_print_uint_literal(compiler, " ", ins->declaration.index_range.register_count, "");
            break;

        case VKD3DSIH_DCL_INDEXABLE_TEMP:
            vkd3d_string_buffer_printf(buffer, " %sx%u%s", compiler->colours.reg,
                    ins->declaration.indexable_temp.register_idx, compiler->colours.reset);
            shader_print_subscript(compiler, ins->declaration.indexable_temp.register_size, NULL);
            shader_print_uint_literal(compiler, ", ", ins->declaration.indexable_temp.component_count, "");
            if (ins->declaration.indexable_temp.alignment)
                shader_print_uint_literal(compiler, ", align ", ins->declaration.indexable_temp.alignment, "");
            if (ins->declaration.indexable_temp.initialiser)
                shader_dump_icb(compiler, ins->declaration.indexable_temp.initialiser);
            break;

        case VKD3DSIH_DCL_INPUT_PS:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_interpolation_mode(compiler, ins->flags);
            shader_addline(buffer, " ");
            shader_dump_dst_param(compiler, &ins->declaration.dst, true);
            break;

        case VKD3DSIH_DCL_INPUT_PS_SGV:
        case VKD3DSIH_DCL_INPUT_SGV:
        case VKD3DSIH_DCL_INPUT_SIV:
        case VKD3DSIH_DCL_OUTPUT_SIV:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_dst_param(compiler, &ins->declaration.register_semantic.reg, true);
            shader_addline(buffer, ", ");
            shader_dump_shader_input_sysval_semantic(compiler, ins->declaration.register_semantic.sysval_semantic);
            break;

        case VKD3DSIH_DCL_INPUT_PS_SIV:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_interpolation_mode(compiler, ins->flags);
            shader_addline(buffer, " ");
            shader_dump_dst_param(compiler, &ins->declaration.register_semantic.reg, true);
            shader_addline(buffer, ", ");
            shader_dump_shader_input_sysval_semantic(compiler, ins->declaration.register_semantic.sysval_semantic);
            break;

        case VKD3DSIH_DCL_INPUT:
        case VKD3DSIH_DCL_OUTPUT:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_dst_param(compiler, &ins->declaration.dst, true);
            break;

        case VKD3DSIH_DCL_INPUT_PRIMITIVE:
        case VKD3DSIH_DCL_OUTPUT_TOPOLOGY:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_primitive_type(compiler, &ins->declaration.primitive_type);
            break;

        case VKD3DSIH_DCL_INTERFACE:
            vkd3d_string_buffer_printf(buffer, " fp%u", ins->declaration.fp.index);
            shader_print_subscript(compiler, ins->declaration.fp.array_size, NULL);
            shader_print_subscript(compiler, ins->declaration.fp.body_count, NULL);
            vkd3d_string_buffer_printf(buffer, " = {...}");
            break;

        case VKD3DSIH_DCL_RESOURCE_RAW:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_dst_param(compiler, &ins->declaration.raw_resource.resource.reg, true);
            shader_dump_register_space(compiler, ins->declaration.raw_resource.resource.range.space);
            break;

        case VKD3DSIH_DCL_RESOURCE_STRUCTURED:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_dst_param(compiler, &ins->declaration.structured_resource.resource.reg, true);
            shader_print_uint_literal(compiler, ", ", ins->declaration.structured_resource.byte_stride, "");
            shader_dump_register_space(compiler, ins->declaration.structured_resource.resource.range.space);
            break;

        case VKD3DSIH_DCL_SAMPLER:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_register(compiler, &ins->declaration.sampler.src.reg, true);
            if (ins->flags == VKD3DSI_SAMPLER_COMPARISON_MODE)
                shader_addline(buffer, ", comparisonMode");
            shader_dump_register_space(compiler, ins->declaration.sampler.range.space);
            break;

        case VKD3DSIH_DCL_TEMPS:
        case VKD3DSIH_DCL_GS_INSTANCES:
        case VKD3DSIH_DCL_HS_FORK_PHASE_INSTANCE_COUNT:
        case VKD3DSIH_DCL_HS_JOIN_PHASE_INSTANCE_COUNT:
        case VKD3DSIH_DCL_INPUT_CONTROL_POINT_COUNT:
        case VKD3DSIH_DCL_OUTPUT_CONTROL_POINT_COUNT:
        case VKD3DSIH_DCL_VERTICES_OUT:
            shader_print_uint_literal(compiler, " ", ins->declaration.count, "");
            break;

        case VKD3DSIH_DCL_TESSELLATOR_DOMAIN:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_tessellator_domain(compiler, ins->declaration.tessellator_domain);
            break;

        case VKD3DSIH_DCL_TESSELLATOR_OUTPUT_PRIMITIVE:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_tessellator_output_primitive(compiler, ins->declaration.tessellator_output_primitive);
            break;

        case VKD3DSIH_DCL_TESSELLATOR_PARTITIONING:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_tessellator_partitioning(compiler, ins->declaration.tessellator_partitioning);
            break;

        case VKD3DSIH_DCL_TGSM_RAW:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_dst_param(compiler, &ins->declaration.tgsm_raw.reg, true);
            shader_print_uint_literal(compiler, ", ", ins->declaration.tgsm_raw.byte_count, "");
            break;

        case VKD3DSIH_DCL_TGSM_STRUCTURED:
            vkd3d_string_buffer_printf(buffer, " ");
            shader_dump_dst_param(compiler, &ins->declaration.tgsm_structured.reg, true);
            shader_print_uint_literal(compiler, ", ", ins->declaration.tgsm_structured.byte_stride, "");
            shader_print_uint_literal(compiler, ", ", ins->declaration.tgsm_structured.structure_count, "");
            break;

        case VKD3DSIH_DCL_THREAD_GROUP:
            shader_print_uint_literal(compiler, " ", ins->declaration.thread_group_size.x, "");
            shader_print_uint_literal(compiler, ", ", ins->declaration.thread_group_size.y, "");
            shader_print_uint_literal(compiler, ", ", ins->declaration.thread_group_size.z, "");
            break;

        case VKD3DSIH_DCL_UAV_RAW:
            shader_dump_uav_flags(compiler, ins->flags);
            shader_addline(buffer, " ");
            shader_dump_dst_param(compiler, &ins->declaration.raw_resource.resource.reg, true);
            shader_dump_register_space(compiler, ins->declaration.raw_resource.resource.range.space);
            break;

        case VKD3DSIH_DCL_UAV_STRUCTURED:
            shader_dump_uav_flags(compiler, ins->flags);
            shader_addline(buffer, " ");
            shader_dump_dst_param(compiler, &ins->declaration.structured_resource.resource.reg, true);
            shader_print_uint_literal(compiler, ", ", ins->declaration.structured_resource.byte_stride, "");
            shader_dump_register_space(compiler, ins->declaration.structured_resource.resource.range.space);
            break;

        case VKD3DSIH_DEF:
            vkd3d_string_buffer_printf(buffer, " %sc%u%s", compiler->colours.reg,
                    shader_get_float_offset(ins->dst[0].reg.type, ins->dst[0].reg.idx[0].offset),
                    compiler->colours.reset);
            shader_print_float_literal(compiler, " = ", ins->src[0].reg.u.immconst_float[0], "");
            shader_print_float_literal(compiler, ", ", ins->src[0].reg.u.immconst_float[1], "");
            shader_print_float_literal(compiler, ", ", ins->src[0].reg.u.immconst_float[2], "");
            shader_print_float_literal(compiler, ", ", ins->src[0].reg.u.immconst_float[3], "");
            break;

        case VKD3DSIH_DEFI:
            vkd3d_string_buffer_printf(buffer, " %si%u%s", compiler->colours.reg,
                    ins->dst[0].reg.idx[0].offset, compiler->colours.reset);
            shader_print_int_literal(compiler, " = ", ins->src[0].reg.u.immconst_uint[0], "");
            shader_print_int_literal(compiler, ", ", ins->src[0].reg.u.immconst_uint[1], "");
            shader_print_int_literal(compiler, ", ", ins->src[0].reg.u.immconst_uint[2], "");
            shader_print_int_literal(compiler, ", ", ins->src[0].reg.u.immconst_uint[3], "");
            break;

        case VKD3DSIH_DEFB:
            vkd3d_string_buffer_printf(buffer, " %sb%u%s", compiler->colours.reg,
                    ins->dst[0].reg.idx[0].offset, compiler->colours.reset);
            shader_print_bool_literal(compiler, " = ", ins->src[0].reg.u.immconst_uint[0], "");
            break;

        default:
            shader_dump_instruction_flags(compiler, ins);

            if (ins->resource_type != VKD3D_SHADER_RESOURCE_NONE)
            {
                shader_addline(buffer, "_indexable(");
                if (ins->raw)
                    vkd3d_string_buffer_printf(buffer, "raw_");
                if (ins->structured)
                    vkd3d_string_buffer_printf(buffer, "structured_");
                shader_dump_resource_type(compiler, ins->resource_type);
                if (ins->resource_stride)
                    shader_print_uint_literal(compiler, ", stride=", ins->resource_stride, "");
                shader_addline(buffer, ")");
            }

            if (vkd3d_shader_instruction_has_texel_offset(ins))
            {
                shader_print_int_literal(compiler, "(", ins->texel_offset.u, "");
                shader_print_int_literal(compiler, ",", ins->texel_offset.v, "");
                shader_print_int_literal(compiler, ",", ins->texel_offset.w, ")");
            }

            if (ins->resource_data_type[0] != VKD3D_DATA_FLOAT
                    || ins->resource_data_type[1] != VKD3D_DATA_FLOAT
                    || ins->resource_data_type[2] != VKD3D_DATA_FLOAT
                    || ins->resource_data_type[3] != VKD3D_DATA_FLOAT)
                shader_dump_data_type(compiler, ins->resource_data_type);

            for (i = 0; i < ins->dst_count; ++i)
            {
                shader_dump_ins_modifiers(compiler, &ins->dst[i]);
                shader_addline(buffer, !i ? " " : ", ");
                shader_dump_dst_param(compiler, &ins->dst[i], false);
            }

            /* Other source tokens */
            for (i = ins->dst_count; i < (ins->dst_count + ins->src_count); ++i)
            {
                shader_addline(buffer, !i ? " " : ", ");
                shader_dump_src_param(compiler, &ins->src[i - ins->dst_count]);
            }
            break;
    }

    shader_addline(buffer, "\n");
}

enum vkd3d_result vkd3d_dxbc_binary_to_text(const struct vkd3d_shader_instruction_array *instructions,
        const struct vkd3d_shader_version *shader_version, const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out)
{
    enum vkd3d_shader_compile_option_formatting_flags formatting;
    struct vkd3d_d3d_asm_compiler compiler;
    enum vkd3d_result result = VKD3D_OK;
    struct vkd3d_string_buffer *buffer;
    unsigned int indent, i, j;
    const char *indent_str;
    void *code;

    static const struct vkd3d_d3d_asm_colours no_colours =
    {
        .reset = "",
        .error = "",
        .literal = "",
        .modifier = "",
        .opcode = "",
        .reg = "",
        .swizzle = "",
        .version = "",
        .write_mask = "",
    };
    static const struct vkd3d_d3d_asm_colours colours =
    {
        .reset = "\x1b[m",
        .error = "\x1b[97;41m",
        .literal = "\x1b[95m",
        .modifier = "\x1b[36m",
        .opcode = "\x1b[96;1m",
        .reg = "\x1b[96m",
        .swizzle = "\x1b[93m",
        .version = "\x1b[36m",
        .write_mask = "\x1b[93m",
    };

    formatting = VKD3D_SHADER_COMPILE_OPTION_FORMATTING_INDENT
            | VKD3D_SHADER_COMPILE_OPTION_FORMATTING_HEADER;
    if (compile_info)
    {
        for (i = 0; i < compile_info->option_count; ++i)
        {
            const struct vkd3d_shader_compile_option *option = &compile_info->options[i];

            if (option->name == VKD3D_SHADER_COMPILE_OPTION_FORMATTING)
                formatting = option->value;
        }
    }

    if (formatting & VKD3D_SHADER_COMPILE_OPTION_FORMATTING_COLOUR)
        compiler.colours = colours;
    else
        compiler.colours = no_colours;
    if (formatting & VKD3D_SHADER_COMPILE_OPTION_FORMATTING_INDENT)
        indent_str = "    ";
    else
        indent_str = "";

    buffer = &compiler.buffer;
    vkd3d_string_buffer_init(buffer);

    compiler.shader_version = *shader_version;
    shader_version = &compiler.shader_version;
    vkd3d_string_buffer_printf(buffer, "%s%s_%u_%u%s\n", compiler.colours.version,
            shader_get_type_prefix(shader_version->type), shader_version->major,
            shader_version->minor, compiler.colours.reset);

    indent = 0;
    for (i = 0; i < instructions->count; ++i)
    {
        struct vkd3d_shader_instruction *ins = &instructions->elements[i];

        switch (ins->handler_idx)
        {
            case VKD3DSIH_ELSE:
            case VKD3DSIH_ENDIF:
            case VKD3DSIH_ENDLOOP:
            case VKD3DSIH_ENDSWITCH:
                if (indent)
                    --indent;
                break;

            default:
                break;
        }

        for (j = 0; j < indent; ++j)
        {
            vkd3d_string_buffer_printf(buffer, "%s", indent_str);
        }

        shader_dump_instruction(&compiler, ins);

        switch (ins->handler_idx)
        {
            case VKD3DSIH_ELSE:
            case VKD3DSIH_IF:
            case VKD3DSIH_LOOP:
            case VKD3DSIH_SWITCH:
                ++indent;
                break;

            default:
                break;
        }
    }

    if ((code = vkd3d_malloc(buffer->content_size)))
    {
        memcpy(code, buffer->buffer, buffer->content_size);
        out->size = buffer->content_size;
        out->code = code;
    }
    else
    {
        result = VKD3D_ERROR_OUT_OF_MEMORY;
    }

    vkd3d_string_buffer_cleanup(buffer);

    return result;
}

void vkd3d_shader_trace(const struct vkd3d_shader_instruction_array *instructions,
        const struct vkd3d_shader_version *shader_version)
{
    const char *p, *q, *end;
    struct vkd3d_shader_code code;

    if (vkd3d_dxbc_binary_to_text(instructions, shader_version, NULL, &code) != VKD3D_OK)
        return;

    end = (const char *)code.code + code.size;
    for (p = code.code; p < end; p = q)
    {
        if (!(q = memchr(p, '\n', end - p)))
            q = end;
        else
            ++q;
        TRACE("    %.*s", (int)(q - p), p);
    }

    vkd3d_shader_free_shader_code(&code);
}
