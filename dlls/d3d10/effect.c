/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 * Copyright 2009 Rico Schüller
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
 *
 */

#include "config.h"
#include "wine/port.h"

#include "d3d10_private.h"

#include <float.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3d10);

#define D3D10_FX10_TYPE_COLUMN_SHIFT    11
#define D3D10_FX10_TYPE_COLUMN_MASK     (0x7 << D3D10_FX10_TYPE_COLUMN_SHIFT)

#define D3D10_FX10_TYPE_ROW_SHIFT       8
#define D3D10_FX10_TYPE_ROW_MASK        (0x7 << D3D10_FX10_TYPE_ROW_SHIFT)

#define D3D10_FX10_TYPE_BASETYPE_SHIFT  3
#define D3D10_FX10_TYPE_BASETYPE_MASK   (0x1f << D3D10_FX10_TYPE_BASETYPE_SHIFT)

#define D3D10_FX10_TYPE_CLASS_SHIFT     0
#define D3D10_FX10_TYPE_CLASS_MASK      (0x7 << D3D10_FX10_TYPE_CLASS_SHIFT)

#define D3D10_FX10_TYPE_MATRIX_COLUMN_MAJOR_MASK 0x4000

static const struct ID3D10EffectTechniqueVtbl d3d10_effect_technique_vtbl;
static const struct ID3D10EffectPassVtbl d3d10_effect_pass_vtbl;
static const struct ID3D10EffectVariableVtbl d3d10_effect_variable_vtbl;
static const struct ID3D10EffectConstantBufferVtbl d3d10_effect_constant_buffer_vtbl;
static const struct ID3D10EffectScalarVariableVtbl d3d10_effect_scalar_variable_vtbl;
static const struct ID3D10EffectVectorVariableVtbl d3d10_effect_vector_variable_vtbl;
static const struct ID3D10EffectMatrixVariableVtbl d3d10_effect_matrix_variable_vtbl;
static const struct ID3D10EffectStringVariableVtbl d3d10_effect_string_variable_vtbl;
static const struct ID3D10EffectShaderResourceVariableVtbl d3d10_effect_shader_resource_variable_vtbl;
static const struct ID3D10EffectRenderTargetViewVariableVtbl d3d10_effect_render_target_view_variable_vtbl;
static const struct ID3D10EffectDepthStencilViewVariableVtbl d3d10_effect_depth_stencil_view_variable_vtbl;
static const struct ID3D10EffectShaderVariableVtbl d3d10_effect_shader_variable_vtbl;
static const struct ID3D10EffectBlendVariableVtbl d3d10_effect_blend_variable_vtbl;
static const struct ID3D10EffectDepthStencilVariableVtbl d3d10_effect_depth_stencil_variable_vtbl;
static const struct ID3D10EffectRasterizerVariableVtbl d3d10_effect_rasterizer_variable_vtbl;
static const struct ID3D10EffectSamplerVariableVtbl d3d10_effect_sampler_variable_vtbl;
static const struct ID3D10EffectTypeVtbl d3d10_effect_type_vtbl;

/* null objects - needed for invalid calls */
static struct d3d10_effect_technique null_technique = {{&d3d10_effect_technique_vtbl}};
static struct d3d10_effect_pass null_pass = {{&d3d10_effect_pass_vtbl}};
static struct d3d10_effect_type null_type = {{&d3d10_effect_type_vtbl}};
static struct d3d10_effect_variable null_local_buffer = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_constant_buffer_vtbl},
        &null_local_buffer, &null_type};
static struct d3d10_effect_variable null_variable = {{&d3d10_effect_variable_vtbl},
        &null_local_buffer, &null_type};
static struct d3d10_effect_variable null_scalar_variable = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_scalar_variable_vtbl},
        &null_local_buffer, &null_type};
static struct d3d10_effect_variable null_vector_variable = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_vector_variable_vtbl},
        &null_local_buffer, &null_type};
static struct d3d10_effect_variable null_matrix_variable = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_matrix_variable_vtbl},
        &null_local_buffer, &null_type};
static struct d3d10_effect_variable null_string_variable = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_string_variable_vtbl},
        &null_local_buffer, &null_type};
static struct d3d10_effect_variable null_shader_resource_variable = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_shader_resource_variable_vtbl},
        &null_local_buffer, &null_type};
static struct d3d10_effect_variable null_render_target_view_variable = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_render_target_view_variable_vtbl},
        &null_local_buffer, &null_type};
static struct d3d10_effect_variable null_depth_stencil_view_variable = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_depth_stencil_view_variable_vtbl},
        &null_local_buffer, &null_type};
static struct d3d10_effect_variable null_shader_variable = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_shader_variable_vtbl},
        &null_local_buffer, &null_type};
static struct d3d10_effect_variable null_blend_variable = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_blend_variable_vtbl},
        &null_local_buffer, &null_type};
static struct d3d10_effect_variable null_depth_stencil_variable = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_depth_stencil_variable_vtbl},
        &null_local_buffer, &null_type};
static struct d3d10_effect_variable null_rasterizer_variable = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_rasterizer_variable_vtbl},
        &null_local_buffer, &null_type};
static struct d3d10_effect_variable null_sampler_variable = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_sampler_variable_vtbl},
        &null_local_buffer, &null_type};

/* anonymous_shader_type and anonymous_shader */
static char anonymous_name[] = "$Anonymous";
static char anonymous_vertexshader_name[] = "vertexshader";
static char anonymous_pixelshader_name[] = "pixelshader";
static char anonymous_geometryshader_name[] = "geometryshader";
static struct d3d10_effect_type anonymous_vs_type = {{&d3d10_effect_type_vtbl},
        anonymous_vertexshader_name, D3D10_SVT_VERTEXSHADER, D3D10_SVC_OBJECT};
static struct d3d10_effect_type anonymous_ps_type = {{&d3d10_effect_type_vtbl},
        anonymous_pixelshader_name, D3D10_SVT_PIXELSHADER, D3D10_SVC_OBJECT};
static struct d3d10_effect_type anonymous_gs_type = {{&d3d10_effect_type_vtbl},
        anonymous_geometryshader_name, D3D10_SVT_GEOMETRYSHADER, D3D10_SVC_OBJECT};
static struct d3d10_effect_variable anonymous_vs = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_shader_variable_vtbl},
        &null_local_buffer, &anonymous_vs_type, &null_shader_variable, anonymous_name};
static struct d3d10_effect_variable anonymous_ps = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_shader_variable_vtbl},
        &null_local_buffer, &anonymous_ps_type, &null_shader_variable, anonymous_name};
static struct d3d10_effect_variable anonymous_gs = {{(const ID3D10EffectVariableVtbl *)&d3d10_effect_shader_variable_vtbl},
        &null_local_buffer, &anonymous_gs_type, &null_shader_variable, anonymous_name};

static struct d3d10_effect_type *get_fx10_type(struct d3d10_effect *effect, const char *data, DWORD offset);

static inline struct d3d10_effect_variable *impl_from_ID3D10EffectVariable(ID3D10EffectVariable *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_effect_variable, ID3D10EffectVariable_iface);
}

struct d3d10_effect_state_property_info
{
    UINT id;
    const char *name;
    D3D_SHADER_VARIABLE_TYPE type;
    UINT size;
    UINT count;
    D3D_SHADER_VARIABLE_TYPE container_type;
    LONG offset;
};

static const struct d3d10_effect_state_property_info property_info[] =
{
    {0x0c, "RasterizerState.FillMode",                    D3D10_SVT_INT,   1, 1, D3D10_SVT_RASTERIZER,   FIELD_OFFSET(D3D10_RASTERIZER_DESC, FillMode)                       },
    {0x0d, "RasterizerState.CullMode",                    D3D10_SVT_INT,   1, 1, D3D10_SVT_RASTERIZER,   FIELD_OFFSET(D3D10_RASTERIZER_DESC, CullMode)                       },
    {0x0e, "RasterizerState.FrontCounterClockwise",       D3D10_SVT_BOOL,  1, 1, D3D10_SVT_RASTERIZER,   FIELD_OFFSET(D3D10_RASTERIZER_DESC, FrontCounterClockwise)          },
    {0x0f, "RasterizerState.DepthBias",                   D3D10_SVT_INT,   1, 1, D3D10_SVT_RASTERIZER,   FIELD_OFFSET(D3D10_RASTERIZER_DESC, DepthBias)                      },
    {0x10, "RasterizerState.DepthBiasClamp",              D3D10_SVT_FLOAT, 1, 1, D3D10_SVT_RASTERIZER,   FIELD_OFFSET(D3D10_RASTERIZER_DESC, DepthBiasClamp)                 },
    {0x11, "RasterizerState.SlopeScaledDepthBias",        D3D10_SVT_FLOAT, 1, 1, D3D10_SVT_RASTERIZER,   FIELD_OFFSET(D3D10_RASTERIZER_DESC, SlopeScaledDepthBias)           },
    {0x12, "RasterizerState.DepthClipEnable",             D3D10_SVT_BOOL,  1, 1, D3D10_SVT_RASTERIZER,   FIELD_OFFSET(D3D10_RASTERIZER_DESC, DepthClipEnable)                },
    {0x13, "RasterizerState.ScissorEnable",               D3D10_SVT_BOOL,  1, 1, D3D10_SVT_RASTERIZER,   FIELD_OFFSET(D3D10_RASTERIZER_DESC, ScissorEnable)                  },
    {0x14, "RasterizerState.MultisampleEnable",           D3D10_SVT_BOOL,  1, 1, D3D10_SVT_RASTERIZER,   FIELD_OFFSET(D3D10_RASTERIZER_DESC, MultisampleEnable)              },
    {0x15, "RasterizerState.AntialiasedLineEnable",       D3D10_SVT_BOOL,  1, 1, D3D10_SVT_RASTERIZER,   FIELD_OFFSET(D3D10_RASTERIZER_DESC, AntialiasedLineEnable)          },
    {0x16, "DepthStencilState.DepthEnable",               D3D10_SVT_BOOL,  1, 1, D3D10_SVT_DEPTHSTENCIL, FIELD_OFFSET(D3D10_DEPTH_STENCIL_DESC, DepthEnable)                 },
    {0x17, "DepthStencilState.DepthWriteMask",            D3D10_SVT_INT,   1, 1, D3D10_SVT_DEPTHSTENCIL, FIELD_OFFSET(D3D10_DEPTH_STENCIL_DESC, DepthWriteMask)              },
    {0x18, "DepthStencilState.DepthFunc",                 D3D10_SVT_INT,   1, 1, D3D10_SVT_DEPTHSTENCIL, FIELD_OFFSET(D3D10_DEPTH_STENCIL_DESC, DepthFunc)                   },
    {0x19, "DepthStencilState.StencilEnable",             D3D10_SVT_BOOL,  1, 1, D3D10_SVT_DEPTHSTENCIL, FIELD_OFFSET(D3D10_DEPTH_STENCIL_DESC, StencilEnable)               },
    {0x1a, "DepthStencilState.StencilReadMask",           D3D10_SVT_UINT8, 1, 1, D3D10_SVT_DEPTHSTENCIL, FIELD_OFFSET(D3D10_DEPTH_STENCIL_DESC, StencilReadMask)             },
    {0x1b, "DepthStencilState.StencilWriteMask",          D3D10_SVT_UINT8, 1, 1, D3D10_SVT_DEPTHSTENCIL, FIELD_OFFSET(D3D10_DEPTH_STENCIL_DESC, StencilWriteMask)            },
    {0x1c, "DepthStencilState.FrontFaceStencilFail",      D3D10_SVT_INT,   1, 1, D3D10_SVT_DEPTHSTENCIL, FIELD_OFFSET(D3D10_DEPTH_STENCIL_DESC, FrontFace.StencilFailOp)     },
    {0x1d, "DepthStencilState.FrontFaceStencilDepthFail", D3D10_SVT_INT,   1, 1, D3D10_SVT_DEPTHSTENCIL, FIELD_OFFSET(D3D10_DEPTH_STENCIL_DESC, FrontFace.StencilDepthFailOp)},
    {0x1e, "DepthStencilState.FrontFaceStencilPass",      D3D10_SVT_INT,   1, 1, D3D10_SVT_DEPTHSTENCIL, FIELD_OFFSET(D3D10_DEPTH_STENCIL_DESC, FrontFace.StencilPassOp)     },
    {0x1f, "DepthStencilState.FrontFaceStencilFunc",      D3D10_SVT_INT,   1, 1, D3D10_SVT_DEPTHSTENCIL, FIELD_OFFSET(D3D10_DEPTH_STENCIL_DESC, FrontFace.StencilFunc)       },
    {0x20, "DepthStencilState.BackFaceStencilFail",       D3D10_SVT_INT,   1, 1, D3D10_SVT_DEPTHSTENCIL, FIELD_OFFSET(D3D10_DEPTH_STENCIL_DESC, BackFace.StencilFailOp)      },
    {0x21, "DepthStencilState.BackFaceStencilDepthFail",  D3D10_SVT_INT,   1, 1, D3D10_SVT_DEPTHSTENCIL, FIELD_OFFSET(D3D10_DEPTH_STENCIL_DESC, BackFace.StencilDepthFailOp) },
    {0x22, "DepthStencilState.BackFaceStencilPass",       D3D10_SVT_INT,   1, 1, D3D10_SVT_DEPTHSTENCIL, FIELD_OFFSET(D3D10_DEPTH_STENCIL_DESC, BackFace.StencilPassOp)      },
    {0x23, "DepthStencilState.BackFaceStencilFunc",       D3D10_SVT_INT,   1, 1, D3D10_SVT_DEPTHSTENCIL, FIELD_OFFSET(D3D10_DEPTH_STENCIL_DESC, BackFace.StencilFunc)        },
    {0x24, "BlendState.AlphaToCoverageEnable",            D3D10_SVT_BOOL,  1, 1, D3D10_SVT_BLEND,        FIELD_OFFSET(D3D10_BLEND_DESC,         AlphaToCoverageEnable)       },
    {0x25, "BlendState.BlendEnable",                      D3D10_SVT_BOOL,  1, 8, D3D10_SVT_BLEND,        FIELD_OFFSET(D3D10_BLEND_DESC,         BlendEnable)                 },
    {0x26, "BlendState.SrcBlend",                         D3D10_SVT_INT,   1, 1, D3D10_SVT_BLEND,        FIELD_OFFSET(D3D10_BLEND_DESC,         SrcBlend)                    },
    {0x27, "BlendState.DestBlend",                        D3D10_SVT_INT,   1, 1, D3D10_SVT_BLEND,        FIELD_OFFSET(D3D10_BLEND_DESC,         DestBlend)                   },
    {0x28, "BlendState.BlendOp",                          D3D10_SVT_INT,   1, 1, D3D10_SVT_BLEND,        FIELD_OFFSET(D3D10_BLEND_DESC,         BlendOp)                     },
    {0x29, "BlendState.SrcBlendAlpha",                    D3D10_SVT_INT,   1, 1, D3D10_SVT_BLEND,        FIELD_OFFSET(D3D10_BLEND_DESC,         SrcBlendAlpha)               },
    {0x2a, "BlendState.DestBlendAlpha",                   D3D10_SVT_INT,   1, 1, D3D10_SVT_BLEND,        FIELD_OFFSET(D3D10_BLEND_DESC,         DestBlendAlpha)              },
    {0x2b, "BlendState.BlendOpAlpha",                     D3D10_SVT_INT,   1, 1, D3D10_SVT_BLEND,        FIELD_OFFSET(D3D10_BLEND_DESC,         BlendOpAlpha)                },
    {0x2c, "BlendState.RenderTargetWriteMask",            D3D10_SVT_UINT8, 1, 8, D3D10_SVT_BLEND,        FIELD_OFFSET(D3D10_BLEND_DESC,         RenderTargetWriteMask)       },
    {0x2d, "SamplerState.Filter",                         D3D10_SVT_INT,   1, 1, D3D10_SVT_SAMPLER,      FIELD_OFFSET(D3D10_SAMPLER_DESC,       Filter)                      },
    {0x2e, "SamplerState.AddressU",                       D3D10_SVT_INT,   1, 1, D3D10_SVT_SAMPLER,      FIELD_OFFSET(D3D10_SAMPLER_DESC,       AddressU)                    },
    {0x2f, "SamplerState.AddressV",                       D3D10_SVT_INT,   1, 1, D3D10_SVT_SAMPLER,      FIELD_OFFSET(D3D10_SAMPLER_DESC,       AddressV)                    },
    {0x30, "SamplerState.AddressW",                       D3D10_SVT_INT,   1, 1, D3D10_SVT_SAMPLER,      FIELD_OFFSET(D3D10_SAMPLER_DESC,       AddressW)                    },
    {0x31, "SamplerState.MipMapLODBias",                  D3D10_SVT_FLOAT, 1, 1, D3D10_SVT_SAMPLER,      FIELD_OFFSET(D3D10_SAMPLER_DESC,       MipLODBias)                  },
    {0x32, "SamplerState.MaxAnisotropy",                  D3D10_SVT_UINT,  1, 1, D3D10_SVT_SAMPLER,      FIELD_OFFSET(D3D10_SAMPLER_DESC,       MaxAnisotropy)               },
    {0x33, "SamplerState.ComparisonFunc",                 D3D10_SVT_INT,   1, 1, D3D10_SVT_SAMPLER,      FIELD_OFFSET(D3D10_SAMPLER_DESC,       ComparisonFunc)              },
    {0x34, "SamplerState.BorderColor",                    D3D10_SVT_FLOAT, 4, 1, D3D10_SVT_SAMPLER,      FIELD_OFFSET(D3D10_SAMPLER_DESC,       BorderColor)                 },
    {0x35, "SamplerState.MinLOD",                         D3D10_SVT_FLOAT, 1, 1, D3D10_SVT_SAMPLER,      FIELD_OFFSET(D3D10_SAMPLER_DESC,       MinLOD)                      },
    {0x36, "SamplerState.MaxLOD",                         D3D10_SVT_FLOAT, 1, 1, D3D10_SVT_SAMPLER,      FIELD_OFFSET(D3D10_SAMPLER_DESC,       MaxLOD)                      },
};

static const D3D10_RASTERIZER_DESC default_rasterizer_desc =
{
    D3D10_FILL_SOLID,
    D3D10_CULL_BACK,
    FALSE,
    0,
    0.0f,
    0.0f,
    TRUE,
    FALSE,
    FALSE,
    FALSE,
};

static const D3D10_DEPTH_STENCIL_DESC default_depth_stencil_desc =
{
    TRUE,
    D3D10_DEPTH_WRITE_MASK_ALL,
    D3D10_COMPARISON_LESS,
    FALSE,
    D3D10_DEFAULT_STENCIL_READ_MASK,
    D3D10_DEFAULT_STENCIL_WRITE_MASK,
    {D3D10_STENCIL_OP_KEEP, D3D10_STENCIL_OP_KEEP, D3D10_STENCIL_OP_KEEP, D3D10_COMPARISON_ALWAYS},
    {D3D10_STENCIL_OP_KEEP, D3D10_STENCIL_OP_KEEP, D3D10_STENCIL_OP_KEEP, D3D10_COMPARISON_ALWAYS},
};

static const D3D10_BLEND_DESC default_blend_desc =
{
    FALSE,
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},
    D3D10_BLEND_SRC_ALPHA,
    D3D10_BLEND_INV_SRC_ALPHA,
    D3D10_BLEND_OP_ADD,
    D3D10_BLEND_SRC_ALPHA,
    D3D10_BLEND_INV_SRC_ALPHA,
    D3D10_BLEND_OP_ADD,
    {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf},
};

static const D3D10_SAMPLER_DESC default_sampler_desc =
{
    D3D10_FILTER_MIN_MAG_MIP_POINT,
    D3D10_TEXTURE_ADDRESS_WRAP,
    D3D10_TEXTURE_ADDRESS_WRAP,
    D3D10_TEXTURE_ADDRESS_WRAP,
    0.0f,
    16,
    D3D10_COMPARISON_NEVER,
    {0.0f, 0.0f, 0.0f, 0.0f},
    0.0f,
    FLT_MAX,
};

struct d3d10_effect_state_storage_info
{
    D3D_SHADER_VARIABLE_TYPE id;
    size_t size;
    const void *default_state;
};

static const struct d3d10_effect_state_storage_info d3d10_effect_state_storage_info[] =
{
    {D3D10_SVT_RASTERIZER,   sizeof(default_rasterizer_desc),    &default_rasterizer_desc   },
    {D3D10_SVT_DEPTHSTENCIL, sizeof(default_depth_stencil_desc), &default_depth_stencil_desc},
    {D3D10_SVT_BLEND,        sizeof(default_blend_desc),         &default_blend_desc        },
    {D3D10_SVT_SAMPLER,      sizeof(default_sampler_desc),       &default_sampler_desc      },
};

static BOOL copy_name(const char *ptr, char **name)
{
    size_t name_len;

    if (!ptr) return TRUE;

    name_len = strlen(ptr) + 1;
    if (name_len == 1)
    {
        return TRUE;
    }

    *name = HeapAlloc(GetProcessHeap(), 0, name_len);
    if (!*name)
    {
        ERR("Failed to allocate name memory.\n");
        return FALSE;
    }

    memcpy(*name, ptr, name_len);

    return TRUE;
}

static HRESULT shader_parse_signature(const char *data, DWORD data_size, struct d3d10_effect_shader_signature *s)
{
    D3D10_SIGNATURE_PARAMETER_DESC *e;
    const char *ptr = data;
    unsigned int i;
    DWORD count;

    read_dword(&ptr, &count);
    TRACE("%u elements\n", count);

    skip_dword_unknown("shader signature", &ptr, 1);

    e = HeapAlloc(GetProcessHeap(), 0, count * sizeof(*e));
    if (!e)
    {
        ERR("Failed to allocate signature memory.\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < count; ++i)
    {
        UINT name_offset;
        UINT mask;

        read_dword(&ptr, &name_offset);
        e[i].SemanticName = data + name_offset;
        read_dword(&ptr, &e[i].SemanticIndex);
        read_dword(&ptr, &e[i].SystemValueType);
        read_dword(&ptr, &e[i].ComponentType);
        read_dword(&ptr, &e[i].Register);
        read_dword(&ptr, &mask);

        e[i].ReadWriteMask = mask >> 8;
        e[i].Mask = mask & 0xff;

        TRACE("semantic: %s, semantic idx: %u, sysval_semantic %#x, "
                "type %u, register idx: %u, use_mask %#x, input_mask %#x\n",
                debugstr_a(e[i].SemanticName), e[i].SemanticIndex, e[i].SystemValueType,
                e[i].ComponentType, e[i].Register, e[i].Mask, e[i].ReadWriteMask);
    }

    s->elements = e;
    s->element_count = count;

    return S_OK;
}

static void shader_free_signature(struct d3d10_effect_shader_signature *s)
{
    HeapFree(GetProcessHeap(), 0, s->signature);
    HeapFree(GetProcessHeap(), 0, s->elements);
}

static HRESULT shader_chunk_handler(const char *data, DWORD data_size, DWORD tag, void *ctx)
{
    struct d3d10_effect_shader_variable *s = ctx;
    HRESULT hr;

    TRACE("tag: %s.\n", debugstr_an((const char *)&tag, 4));

    TRACE("chunk size: %#x\n", data_size);

    switch(tag)
    {
        case TAG_ISGN:
        case TAG_OSGN:
        {
            /* 32 (DXBC header) + 1 * 4 (chunk index) + 2 * 4 (chunk header) + data_size (chunk data) */
            UINT size = 44 + data_size;
            struct d3d10_effect_shader_signature *sig;
            char *ptr;

            if (tag == TAG_ISGN) sig = &s->input_signature;
            else sig = &s->output_signature;

            sig->signature = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
            if (!sig->signature)
            {
                ERR("Failed to allocate input signature data\n");
                return E_OUTOFMEMORY;
            }
            sig->signature_size = size;

            ptr = sig->signature;

            write_dword(&ptr, TAG_DXBC);

            /* signature(?) */
            write_dword_unknown(&ptr, 0);
            write_dword_unknown(&ptr, 0);
            write_dword_unknown(&ptr, 0);
            write_dword_unknown(&ptr, 0);

            /* seems to be always 1 */
            write_dword_unknown(&ptr, 1);

            /* DXBC size */
            write_dword(&ptr, size);

            /* chunk count */
            write_dword(&ptr, 1);

            /* chunk index */
            write_dword(&ptr, (ptr - sig->signature) + 4);

            /* chunk */
            write_dword(&ptr, tag);
            write_dword(&ptr, data_size);
            memcpy(ptr, data, data_size);

            hr = shader_parse_signature(ptr, data_size, sig);
            if (FAILED(hr))
            {
                ERR("Failed to parse shader, hr %#x\n", hr);
                shader_free_signature(sig);
            }

            break;
        }

        default:
            FIXME("Unhandled chunk %s.\n", debugstr_an((const char *)&tag, 4));
            break;
    }

    return S_OK;
}

static HRESULT parse_shader(struct d3d10_effect_variable *v, const char *data)
{
    ID3D10Device *device = v->effect->device;
    struct d3d10_effect_shader_variable *s;
    const char *ptr = data;
    DWORD dxbc_size;
    HRESULT hr;

    s = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*s));
    if (!s)
    {
        ERR("Failed to allocate shader variable memory\n");
        return E_OUTOFMEMORY;
    }

    v->data = s;

    if (v->effect->used_shader_current >= v->effect->used_shader_count)
    {
        WARN("Invalid shader? Used shader current(%u) >= used shader count(%u)\n", v->effect->used_shader_current, v->effect->used_shader_count);
        return E_FAIL;
    }

    v->effect->used_shaders[v->effect->used_shader_current] = v;
    ++v->effect->used_shader_current;

    if (!ptr) return S_OK;

    read_dword(&ptr, &dxbc_size);
    TRACE("dxbc size: %#x\n", dxbc_size);

    /* We got a shader VertexShader vs = NULL, so it is fine to skip this. */
    if (!dxbc_size) return S_OK;

    switch (v->type->basetype)
    {
        case D3D10_SVT_VERTEXSHADER:
            hr = ID3D10Device_CreateVertexShader(device, ptr, dxbc_size, &s->shader.vs);
            if (FAILED(hr)) return hr;
            break;

        case D3D10_SVT_PIXELSHADER:
            hr = ID3D10Device_CreatePixelShader(device, ptr, dxbc_size, &s->shader.ps);
            if (FAILED(hr)) return hr;
            break;

        case D3D10_SVT_GEOMETRYSHADER:
            hr = ID3D10Device_CreateGeometryShader(device, ptr, dxbc_size, &s->shader.gs);
            if (FAILED(hr)) return hr;
            break;

        default:
            ERR("This should not happen!\n");
            return E_FAIL;
    }

    return parse_dxbc(ptr, dxbc_size, shader_chunk_handler, s);
}

static D3D10_SHADER_VARIABLE_CLASS d3d10_variable_class(DWORD c, BOOL is_column_major)
{
    switch (c)
    {
        case 1: return D3D10_SVC_SCALAR;
        case 2: return D3D10_SVC_VECTOR;
        case 3: if (is_column_major) return D3D10_SVC_MATRIX_COLUMNS;
                else return D3D10_SVC_MATRIX_ROWS;
        default:
            FIXME("Unknown variable class %#x.\n", c);
            return 0;
    }
}

static D3D10_SHADER_VARIABLE_TYPE d3d10_variable_type(DWORD t, BOOL is_object)
{
    if(is_object)
    {
        switch (t)
        {
            case 1: return D3D10_SVT_STRING;
            case 2: return D3D10_SVT_BLEND;
            case 3: return D3D10_SVT_DEPTHSTENCIL;
            case 4: return D3D10_SVT_RASTERIZER;
            case 5: return D3D10_SVT_PIXELSHADER;
            case 6: return D3D10_SVT_VERTEXSHADER;
            case 7: return D3D10_SVT_GEOMETRYSHADER;

            case 10: return D3D10_SVT_TEXTURE1D;
            case 11: return D3D10_SVT_TEXTURE1DARRAY;
            case 12: return D3D10_SVT_TEXTURE2D;
            case 13: return D3D10_SVT_TEXTURE2DARRAY;
            case 14: return D3D10_SVT_TEXTURE2DMS;
            case 15: return D3D10_SVT_TEXTURE2DMSARRAY;
            case 16: return D3D10_SVT_TEXTURE3D;
            case 17: return D3D10_SVT_TEXTURECUBE;

            case 19: return D3D10_SVT_RENDERTARGETVIEW;
            case 20: return D3D10_SVT_DEPTHSTENCILVIEW;
            case 21: return D3D10_SVT_SAMPLER;
            case 22: return D3D10_SVT_BUFFER;
            default:
                FIXME("Unknown variable type %#x.\n", t);
                return D3D10_SVT_VOID;
        }
    }
    else
    {
        switch (t)
        {
            case 1: return D3D10_SVT_FLOAT;
            case 2: return D3D10_SVT_INT;
            case 3: return D3D10_SVT_UINT;
            case 4: return D3D10_SVT_BOOL;
            default:
                FIXME("Unknown variable type %#x.\n", t);
                return D3D10_SVT_VOID;
        }
    }
}

static HRESULT parse_fx10_type(struct d3d10_effect_type *t, const char *ptr, const char *data)
{
    DWORD unknown0;
    DWORD offset;
    DWORD typeinfo;
    unsigned int i;

    read_dword(&ptr, &offset);
    TRACE("Type name at offset %#x.\n", offset);

    if (!copy_name(data + offset, &t->name))
    {
        ERR("Failed to copy name.\n");
        return E_OUTOFMEMORY;
    }
    TRACE("Type name: %s.\n", debugstr_a(t->name));

    read_dword(&ptr, &unknown0);
    TRACE("Unknown 0: %u.\n", unknown0);

    read_dword(&ptr, &t->element_count);
    TRACE("Element count: %u.\n", t->element_count);

    read_dword(&ptr, &t->size_unpacked);
    TRACE("Unpacked size: %#x.\n", t->size_unpacked);

    read_dword(&ptr, &t->stride);
    TRACE("Stride: %#x.\n", t->stride);

    read_dword(&ptr, &t->size_packed);
    TRACE("Packed size %#x.\n", t->size_packed);

    switch (unknown0)
    {
        case 1:
            t->member_count = 0;

            read_dword(&ptr, &typeinfo);
            t->column_count = (typeinfo & D3D10_FX10_TYPE_COLUMN_MASK) >> D3D10_FX10_TYPE_COLUMN_SHIFT;
            t->row_count = (typeinfo & D3D10_FX10_TYPE_ROW_MASK) >> D3D10_FX10_TYPE_ROW_SHIFT;
            t->basetype = d3d10_variable_type((typeinfo & D3D10_FX10_TYPE_BASETYPE_MASK) >> D3D10_FX10_TYPE_BASETYPE_SHIFT, FALSE);
            t->type_class = d3d10_variable_class((typeinfo & D3D10_FX10_TYPE_CLASS_MASK) >> D3D10_FX10_TYPE_CLASS_SHIFT, typeinfo & D3D10_FX10_TYPE_MATRIX_COLUMN_MAJOR_MASK);

            TRACE("Type description: %#x.\n", typeinfo);
            TRACE("\tcolumns: %u.\n", t->column_count);
            TRACE("\trows: %u.\n", t->row_count);
            TRACE("\tbasetype: %s.\n", debug_d3d10_shader_variable_type(t->basetype));
            TRACE("\tclass: %s.\n", debug_d3d10_shader_variable_class(t->type_class));
            TRACE("\tunknown bits: %#x.\n", typeinfo & ~(D3D10_FX10_TYPE_COLUMN_MASK | D3D10_FX10_TYPE_ROW_MASK
                    | D3D10_FX10_TYPE_BASETYPE_MASK | D3D10_FX10_TYPE_CLASS_MASK | D3D10_FX10_TYPE_MATRIX_COLUMN_MAJOR_MASK));
            break;

        case 2:
            TRACE("Type is an object.\n");

            t->member_count = 0;
            t->column_count = 0;
            t->row_count = 0;
            t->type_class = D3D10_SVC_OBJECT;

            read_dword(&ptr, &typeinfo);
            t->basetype = d3d10_variable_type(typeinfo, TRUE);

            TRACE("Type description: %#x.\n", typeinfo);
            TRACE("\tbasetype: %s.\n", debug_d3d10_shader_variable_type(t->basetype));
            TRACE("\tclass: %s.\n", debug_d3d10_shader_variable_class(t->type_class));
            break;

         case 3:
            TRACE("Type is a structure.\n");

            read_dword(&ptr, &t->member_count);
            TRACE("Member count: %u.\n", t->member_count);

            t->column_count = 0;
            t->row_count = 0;
            t->basetype = 0;
            t->type_class = D3D10_SVC_STRUCT;

            t->members = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, t->member_count * sizeof(*t->members));
            if (!t->members)
            {
                ERR("Failed to allocate members memory.\n");
                return E_OUTOFMEMORY;
            }

            for (i = 0; i < t->member_count; ++i)
            {
                struct d3d10_effect_type_member *typem = &t->members[i];

                read_dword(&ptr, &offset);
                TRACE("Member name at offset %#x.\n", offset);

                if (!copy_name(data + offset, &typem->name))
                {
                    ERR("Failed to copy name.\n");
                    return E_OUTOFMEMORY;
                }
                TRACE("Member name: %s.\n", debugstr_a(typem->name));

                read_dword(&ptr, &offset);
                TRACE("Member semantic at offset %#x.\n", offset);

                if (!copy_name(data + offset, &typem->semantic))
                {
                    ERR("Failed to copy semantic.\n");
                    return E_OUTOFMEMORY;
                }
                TRACE("Member semantic: %s.\n", debugstr_a(typem->semantic));

                read_dword(&ptr, &typem->buffer_offset);
                TRACE("Member offset in struct: %#x.\n", typem->buffer_offset);

                read_dword(&ptr, &offset);
                TRACE("Member type info at offset %#x.\n", offset);

                typem->type = get_fx10_type(t->effect, data, offset);
                if (!typem->type)
                {
                    ERR("Failed to get variable type.\n");
                    return E_FAIL;
                }
            }
            break;

        default:
            FIXME("Unhandled case %#x.\n", unknown0);
            return E_FAIL;
    }

    if (t->element_count)
    {
        TRACE("Elementtype for type at offset: %#x\n", t->id);

        /* allocate elementtype - we need only one, because all elements have the same type */
        t->elementtype = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*t->elementtype));
        if (!t->elementtype)
        {
            ERR("Failed to allocate members memory.\n");
            return E_OUTOFMEMORY;
        }

        /* create a copy of the original type with some minor changes */
        t->elementtype->ID3D10EffectType_iface.lpVtbl = &d3d10_effect_type_vtbl;
        t->elementtype->effect = t->effect;

        if (!copy_name(t->name, &t->elementtype->name))
        {
             ERR("Failed to copy name.\n");
             return E_OUTOFMEMORY;
        }
        TRACE("\tType name: %s.\n", debugstr_a(t->elementtype->name));

        t->elementtype->element_count = 0;
        TRACE("\tElement count: %u.\n", t->elementtype->element_count);

        /*
         * Not sure if this calculation is 100% correct, but a test
         * shows that these values work.
         */
        t->elementtype->size_unpacked = t->size_packed / t->element_count;
        TRACE("\tUnpacked size: %#x.\n", t->elementtype->size_unpacked);

        t->elementtype->stride = t->stride;
        TRACE("\tStride: %#x.\n", t->elementtype->stride);

        t->elementtype->size_packed = t->size_packed / t->element_count;
        TRACE("\tPacked size: %#x.\n", t->elementtype->size_packed);

        t->elementtype->member_count = t->member_count;
        TRACE("\tMember count: %u.\n", t->elementtype->member_count);

        t->elementtype->column_count = t->column_count;
        TRACE("\tColumns: %u.\n", t->elementtype->column_count);

        t->elementtype->row_count = t->row_count;
        TRACE("\tRows: %u.\n", t->elementtype->row_count);

        t->elementtype->basetype = t->basetype;
        TRACE("\tBasetype: %s.\n", debug_d3d10_shader_variable_type(t->elementtype->basetype));

        t->elementtype->type_class = t->type_class;
        TRACE("\tClass: %s.\n", debug_d3d10_shader_variable_class(t->elementtype->type_class));

        t->elementtype->members = t->members;
    }

    return S_OK;
}

static struct d3d10_effect_type *get_fx10_type(struct d3d10_effect *effect, const char *data, DWORD offset)
{
    struct d3d10_effect_type *type;
    struct wine_rb_entry *entry;
    HRESULT hr;

    entry = wine_rb_get(&effect->types, &offset);
    if (entry)
    {
        TRACE("Returning existing type.\n");
        return WINE_RB_ENTRY_VALUE(entry, struct d3d10_effect_type, entry);
    }

    type = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*type));
    if (!type)
    {
        ERR("Failed to allocate type memory.\n");
        return NULL;
    }

    type->ID3D10EffectType_iface.lpVtbl = &d3d10_effect_type_vtbl;
    type->id = offset;
    type->effect = effect;
    hr = parse_fx10_type(type, data + offset, data);
    if (FAILED(hr))
    {
        ERR("Failed to parse type info, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, type);
        return NULL;
    }

    if (wine_rb_put(&effect->types, &offset, &type->entry) == -1)
    {
        ERR("Failed to insert type entry.\n");
        HeapFree(GetProcessHeap(), 0, type);
        return NULL;
    }

    return type;
}

static void set_variable_vtbl(struct d3d10_effect_variable *v)
{
    const ID3D10EffectVariableVtbl **vtbl = &v->ID3D10EffectVariable_iface.lpVtbl;

    switch (v->type->type_class)
    {
        case D3D10_SVC_SCALAR:
            *vtbl = (const ID3D10EffectVariableVtbl *)&d3d10_effect_scalar_variable_vtbl;
            break;

        case D3D10_SVC_VECTOR:
            *vtbl = (const ID3D10EffectVariableVtbl *)&d3d10_effect_vector_variable_vtbl;
            break;

        case D3D10_SVC_MATRIX_ROWS:
        case D3D10_SVC_MATRIX_COLUMNS:
            *vtbl = (const ID3D10EffectVariableVtbl *)&d3d10_effect_matrix_variable_vtbl;
            break;

        case D3D10_SVC_STRUCT:
            *vtbl = &d3d10_effect_variable_vtbl;
            break;

        case D3D10_SVC_OBJECT:
            switch(v->type->basetype)
            {
                case D3D10_SVT_STRING:
                    *vtbl = (const ID3D10EffectVariableVtbl *)&d3d10_effect_string_variable_vtbl;
                    break;

                case D3D10_SVT_TEXTURE1D:
                case D3D10_SVT_TEXTURE1DARRAY:
                case D3D10_SVT_TEXTURE2D:
                case D3D10_SVT_TEXTURE2DARRAY:
                case D3D10_SVT_TEXTURE2DMS:
                case D3D10_SVT_TEXTURE2DMSARRAY:
                case D3D10_SVT_TEXTURE3D:
                case D3D10_SVT_TEXTURECUBE:
                case D3D10_SVT_BUFFER: /* Either resource or constant buffer. */
                    *vtbl = (const ID3D10EffectVariableVtbl *)&d3d10_effect_shader_resource_variable_vtbl;
                    break;

                case D3D10_SVT_RENDERTARGETVIEW:
                    *vtbl = (const ID3D10EffectVariableVtbl *)&d3d10_effect_render_target_view_variable_vtbl;
                    break;

                case D3D10_SVT_DEPTHSTENCILVIEW:
                    *vtbl = (const ID3D10EffectVariableVtbl *)&d3d10_effect_depth_stencil_view_variable_vtbl;
                    break;

                case D3D10_SVT_DEPTHSTENCIL:
                    *vtbl = (const ID3D10EffectVariableVtbl *)&d3d10_effect_depth_stencil_variable_vtbl;
                    break;

                case D3D10_SVT_VERTEXSHADER:
                case D3D10_SVT_GEOMETRYSHADER:
                case D3D10_SVT_PIXELSHADER:
                    *vtbl = (const ID3D10EffectVariableVtbl *)&d3d10_effect_shader_variable_vtbl;
                    break;

                case D3D10_SVT_BLEND:
                    *vtbl = (const ID3D10EffectVariableVtbl *)&d3d10_effect_blend_variable_vtbl;
                    break;

                case D3D10_SVT_RASTERIZER:
                    *vtbl = (const ID3D10EffectVariableVtbl *)&d3d10_effect_rasterizer_variable_vtbl;
                    break;

                case D3D10_SVT_SAMPLER:
                    *vtbl = (const ID3D10EffectVariableVtbl *)&d3d10_effect_sampler_variable_vtbl;
                    break;

                default:
                    FIXME("Unhandled basetype %s.\n", debug_d3d10_shader_variable_type(v->type->basetype));
                    *vtbl = &d3d10_effect_variable_vtbl;
                    break;
            }
            break;

        default:
            FIXME("Unhandled type class %s.\n", debug_d3d10_shader_variable_class(v->type->type_class));
            *vtbl = &d3d10_effect_variable_vtbl;
            break;
    }
}

static HRESULT copy_variableinfo_from_type(struct d3d10_effect_variable *v)
{
    unsigned int i;
    HRESULT hr;

    if (v->type->member_count)
    {
        v->members = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, v->type->member_count * sizeof(*v->members));
        if (!v->members)
        {
            ERR("Failed to allocate members memory.\n");
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < v->type->member_count; ++i)
        {
            struct d3d10_effect_variable *var = &v->members[i];
            struct d3d10_effect_type_member *typem = &v->type->members[i];

            var->buffer = v->buffer;
            var->effect = v->effect;
            var->type = typem->type;
            set_variable_vtbl(var);

            if (!copy_name(typem->name, &var->name))
            {
                ERR("Failed to copy name.\n");
                return E_OUTOFMEMORY;
            }
            TRACE("Variable name: %s.\n", debugstr_a(var->name));

            if (!copy_name(typem->semantic, &var->semantic))
            {
                ERR("Failed to copy name.\n");
                return E_OUTOFMEMORY;
            }
            TRACE("Variable semantic: %s.\n", debugstr_a(var->semantic));

            var->buffer_offset = v->buffer_offset + typem->buffer_offset;
            TRACE("Variable buffer offset: %u.\n", var->buffer_offset);

            hr = copy_variableinfo_from_type(var);
            if (FAILED(hr)) return hr;
        }
    }

    if (v->type->element_count)
    {
        unsigned int bufferoffset = v->buffer_offset;

        v->elements = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, v->type->element_count * sizeof(*v->elements));
        if (!v->elements)
        {
            ERR("Failed to allocate elements memory.\n");
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < v->type->element_count; ++i)
        {
            struct d3d10_effect_variable *var = &v->elements[i];

            var->buffer = v->buffer;
            var->effect = v->effect;
            var->type = v->type->elementtype;
            set_variable_vtbl(var);

            if (!copy_name(v->name, &var->name))
            {
                ERR("Failed to copy name.\n");
                return E_OUTOFMEMORY;
            }
            TRACE("Variable name: %s.\n", debugstr_a(var->name));

            if (!copy_name(v->semantic, &var->semantic))
            {
                ERR("Failed to copy name.\n");
                return E_OUTOFMEMORY;
            }
            TRACE("Variable semantic: %s.\n", debugstr_a(var->semantic));

            if (i != 0)
            {
                bufferoffset += v->type->stride;
            }
            var->buffer_offset = bufferoffset;
            TRACE("Variable buffer offset: %u.\n", var->buffer_offset);

            hr = copy_variableinfo_from_type(var);
            if (FAILED(hr)) return hr;
        }
    }

    return S_OK;
}

static HRESULT parse_fx10_variable_head(struct d3d10_effect_variable *v, const char **ptr, const char *data)
{
    DWORD offset;

    read_dword(ptr, &offset);
    TRACE("Variable name at offset %#x.\n", offset);

    if (!copy_name(data + offset, &v->name))
    {
        ERR("Failed to copy name.\n");
        return E_OUTOFMEMORY;
    }
    TRACE("Variable name: %s.\n", debugstr_a(v->name));

    read_dword(ptr, &offset);
    TRACE("Variable type info at offset %#x.\n", offset);

    v->type = get_fx10_type(v->effect, data, offset);
    if (!v->type)
    {
        ERR("Failed to get variable type.\n");
        return E_FAIL;
    }
    set_variable_vtbl(v);

    return copy_variableinfo_from_type(v);
}

static HRESULT parse_fx10_annotation(struct d3d10_effect_variable *a, const char **ptr, const char *data)
{
    HRESULT hr;

    hr = parse_fx10_variable_head(a, ptr, data);
    if (FAILED(hr)) return hr;

    skip_dword_unknown("annotation", ptr, 1);

    /* mark the variable as annotation */
    a->flag = D3D10_EFFECT_VARIABLE_ANNOTATION;

    return S_OK;
}

static HRESULT parse_fx10_anonymous_shader(struct d3d10_effect *e, struct d3d10_effect_anonymous_shader *s,
    enum d3d10_effect_object_type otype)
{
    struct d3d10_effect_variable *v = &s->shader;
    struct d3d10_effect_type *t = &s->type;
    const char *shader = NULL;

    switch (otype)
    {
        case D3D10_EOT_VERTEXSHADER:
            shader = "vertexshader";
            t->basetype = D3D10_SVT_VERTEXSHADER;
            break;

        case D3D10_EOT_PIXELSHADER:
            shader = "pixelshader";
            t->basetype = D3D10_SVT_PIXELSHADER;
            break;

        case D3D10_EOT_GEOMETRYSHADER:
            shader = "geometryshader";
            t->basetype = D3D10_SVT_GEOMETRYSHADER;
            break;

        default:
            FIXME("Unhandled object type %#x.\n", otype);
            return E_FAIL;
    }

    if (!copy_name(shader, &t->name))
    {
        ERR("Failed to copy name.\n");
        return E_OUTOFMEMORY;
    }
    TRACE("Type name: %s.\n", debugstr_a(t->name));

    t->type_class = D3D10_SVC_OBJECT;

    t->ID3D10EffectType_iface.lpVtbl = &d3d10_effect_type_vtbl;

    v->type = t;
    v->effect = e;
    set_variable_vtbl(v);

    if (!copy_name("$Anonymous", &v->name))
    {
        ERR("Failed to copy semantic.\n");
        return E_OUTOFMEMORY;
    }
    TRACE("Variable name: %s.\n", debugstr_a(v->name));

    if (!copy_name(NULL, &v->semantic))
    {
        ERR("Failed to copy semantic.\n");
        return E_OUTOFMEMORY;
    }
    TRACE("Variable semantic: %s.\n", debugstr_a(v->semantic));

    return S_OK;
}

static const struct d3d10_effect_state_property_info *get_property_info(UINT id)
{
    unsigned int i;

    for (i = 0; i < sizeof(property_info) / sizeof(*property_info); ++i)
    {
        if (property_info[i].id == id)
            return &property_info[i];
    }

    return NULL;
}

static const struct d3d10_effect_state_storage_info *get_storage_info(D3D_SHADER_VARIABLE_TYPE id)
{
    unsigned int i;

    for (i = 0; i < sizeof(d3d10_effect_state_storage_info) / sizeof(*d3d10_effect_state_storage_info); ++i)
    {
        if (d3d10_effect_state_storage_info[i].id == id)
            return &d3d10_effect_state_storage_info[i];
    }

    return NULL;
}

static BOOL read_float_value(DWORD value, D3D_SHADER_VARIABLE_TYPE in_type, float *out_data, UINT idx)
{
    switch (in_type)
    {
        case D3D10_SVT_FLOAT:
            out_data[idx] = *(float *)&value;
            return TRUE;

        case D3D10_SVT_INT:
            out_data[idx] = (INT)value;
            return TRUE;

        default:
            FIXME("Unhandled in_type %#x.\n", in_type);
            return FALSE;
    }
}

static BOOL read_int32_value(DWORD value, D3D_SHADER_VARIABLE_TYPE in_type, INT *out_data, UINT idx)
{
    switch (in_type)
    {
        case D3D10_SVT_FLOAT:
            out_data[idx] = *(float *)&value;
            return TRUE;

        case D3D10_SVT_INT:
        case D3D10_SVT_BOOL:
            out_data[idx] = value;
            return TRUE;

        default:
            FIXME("Unhandled in_type %#x.\n", in_type);
            return FALSE;
    }
}

static BOOL read_int8_value(DWORD value, D3D_SHADER_VARIABLE_TYPE in_type, INT8 *out_data, UINT idx)
{
    switch (in_type)
    {
        case D3D10_SVT_INT:
            out_data[idx] = value;
            return TRUE;

        default:
            FIXME("Unhandled in_type %#x.\n", in_type);
            return FALSE;
    }
}

static BOOL read_value_list(const char *ptr, D3D_SHADER_VARIABLE_TYPE out_type,
        UINT out_base, UINT out_size, void *out_data)
{
    D3D_SHADER_VARIABLE_TYPE in_type;
    DWORD t, value;
    DWORD count, i;

    read_dword(&ptr, &count);
    if (count != out_size)
        return FALSE;

    TRACE("%u values:\n", count);
    for (i = 0; i < count; ++i)
    {
        UINT out_idx = out_base * out_size + i;

        read_dword(&ptr, &t);
        read_dword(&ptr, &value);

        in_type = d3d10_variable_type(t, FALSE);
        TRACE("\t%s: %#x.\n", debug_d3d10_shader_variable_type(in_type), value);

        switch (out_type)
        {
            case D3D10_SVT_FLOAT:
                if (!read_float_value(value, in_type, out_data, out_idx))
                    return FALSE;
                break;

            case D3D10_SVT_INT:
            case D3D10_SVT_UINT:
            case D3D10_SVT_BOOL:
                if (!read_int32_value(value, in_type, out_data, out_idx))
                    return FALSE;
                break;

            case D3D10_SVT_UINT8:
                if (!read_int8_value(value, in_type, out_data, out_idx))
                    return FALSE;
                break;

            default:
                FIXME("Unhandled out_type %#x.\n", out_type);
                return FALSE;
        }
    }

    return TRUE;
}

static BOOL parse_fx10_state_group(const char **ptr, const char *data,
        D3D_SHADER_VARIABLE_TYPE container_type, void *container)
{
    const struct d3d10_effect_state_property_info *property_info;
    UINT value_offset;
    unsigned int i;
    DWORD count;
    UINT idx;
    UINT id;

    read_dword(ptr, &count);
    TRACE("Property count: %#x.\n", count);

    for (i = 0; i < count; ++i)
    {
        read_dword(ptr, &id);
        read_dword(ptr, &idx);
        skip_dword_unknown("read property", ptr, 1);
        read_dword(ptr, &value_offset);

        if (!(property_info = get_property_info(id)))
        {
            FIXME("Failed to find property info for property %#x.\n", id);
            return FALSE;
        }

        TRACE("Property %s[%#x] = value list @ offset %#x.\n",
                property_info->name, idx, value_offset);

        if (property_info->container_type != container_type)
        {
            ERR("FAIL1\n");
            return FALSE;
        }

        if (idx >= property_info->count)
        {
            ERR("FAIL2\n");
            return FALSE;
        }

        if (!read_value_list(data + value_offset, property_info->type, idx,
                property_info->size, (char *)container + property_info->offset))
        {
            ERR("FAIL3\n");
            return FALSE;
        }
    }

    return TRUE;
}

static HRESULT parse_fx10_object(struct d3d10_effect_object *o, const char **ptr, const char *data)
{
    const char *data_ptr = NULL;
    DWORD offset;
    enum d3d10_effect_object_operation operation;
    HRESULT hr;
    struct d3d10_effect *effect = o->pass->technique->effect;
    ID3D10Effect *e = &effect->ID3D10Effect_iface;

    read_dword(ptr, &o->type);
    TRACE("Effect object is of type %#x.\n", o->type);

    read_dword(ptr, &o->index);
    TRACE("Effect object index %#x.\n", o->index);

    read_dword(ptr, &operation);
    TRACE("Effect object operation %#x.\n", operation);

    read_dword(ptr, &offset);
    TRACE("Effect object idx is at offset %#x.\n", offset);

    switch(operation)
    {
        case D3D10_EOO_VALUE:
            TRACE("Copy variable values\n");

            switch (o->type)
            {
                case D3D10_EOT_VERTEXSHADER:
                    TRACE("Vertex shader\n");
                    o->data = &anonymous_vs;
                    hr = S_OK;
                    break;

                case D3D10_EOT_PIXELSHADER:
                    TRACE("Pixel shader\n");
                    o->data = &anonymous_ps;
                    hr = S_OK;
                    break;

                case D3D10_EOT_GEOMETRYSHADER:
                    TRACE("Geometry shader\n");
                    o->data = &anonymous_gs;
                    hr = S_OK;
                    break;

                case D3D10_EOT_STENCIL_REF:
                    if (!read_value_list(data + offset, D3D10_SVT_UINT, 0, 1, &o->pass->stencil_ref))
                    {
                        ERR("Failed to read stencil ref.\n");
                        return E_FAIL;
                    }

                    hr = S_OK;
                    break;

                case D3D10_EOT_SAMPLE_MASK:
                    if (!read_value_list(data + offset, D3D10_SVT_UINT, 0, 1, &o->pass->sample_mask))
                    {
                        FIXME("Failed to read sample mask.\n");
                        return E_FAIL;
                    }

                    hr = S_OK;
                    break;

                case D3D10_EOT_BLEND_FACTOR:
                    if (!read_value_list(data + offset, D3D10_SVT_FLOAT, 0, 4, &o->pass->blend_factor[0]))
                    {
                        FIXME("Failed to read blend factor.\n");
                        return E_FAIL;
                    }

                    hr = S_OK;
                    break;

                default:
                    FIXME("Unhandled object type %#x\n", o->type);
                    hr = E_FAIL;
                    break;
            }
            break;

        case D3D10_EOO_PARSED_OBJECT:
            /* This is a local object, we've parsed in parse_fx10_local_object. */
            TRACE("Shader = %s.\n", data + offset);

            o->data = e->lpVtbl->GetVariableByName(e, data + offset);
            hr = S_OK;
            break;

        case D3D10_EOO_PARSED_OBJECT_INDEX:
            /* This is a local object, we've parsed in parse_fx10_local_object, which has an array index. */
            data_ptr = data + offset;
            read_dword(&data_ptr, &offset);
            read_dword(&data_ptr, &o->index);
            TRACE("Shader = %s[%u].\n", data + offset, o->index);

            o->data = e->lpVtbl->GetVariableByName(e, data + offset);
            hr = S_OK;
            break;

        case D3D10_EOO_ANONYMOUS_SHADER:
            TRACE("Anonymous shader\n");

            /* check anonymous_shader_current for validity */
            if (effect->anonymous_shader_current >= effect->anonymous_shader_count)
            {
                ERR("Anonymous shader count is wrong!\n");
                return E_FAIL;
            }

            data_ptr = data + offset;
            read_dword(&data_ptr, &offset);
            TRACE("Effect object starts at offset %#x.\n", offset);

            data_ptr = data + offset;

            hr = parse_fx10_anonymous_shader(effect, &effect->anonymous_shaders[effect->anonymous_shader_current], o->type);
            if (FAILED(hr)) return hr;

            o->data = &effect->anonymous_shaders[effect->anonymous_shader_current].shader;
            ++effect->anonymous_shader_current;

            switch (o->type)
            {
                case D3D10_EOT_VERTEXSHADER:
                    TRACE("Vertex shader\n");
                    hr = parse_shader(o->data, data_ptr);
                    break;

                case D3D10_EOT_PIXELSHADER:
                    TRACE("Pixel shader\n");
                    hr = parse_shader(o->data, data_ptr);
                    break;

                case D3D10_EOT_GEOMETRYSHADER:
                    TRACE("Geometry shader\n");
                    hr = parse_shader(o->data, data_ptr);
                    break;

                default:
                    FIXME("Unhandled object type %#x\n", o->type);
                    hr = E_FAIL;
                    break;
            }
            break;

        default:
            hr = E_FAIL;
            FIXME("Unhandled operation %#x.\n", operation);
            break;
    }

    return hr;
}

static HRESULT parse_fx10_pass(struct d3d10_effect_pass *p, const char **ptr, const char *data)
{
    HRESULT hr = S_OK;
    unsigned int i;
    DWORD offset;

    read_dword(ptr, &offset);
    TRACE("Pass name at offset %#x.\n", offset);

    if (!copy_name(data + offset, &p->name))
    {
        ERR("Failed to copy name.\n");
        return E_OUTOFMEMORY;
    }
    TRACE("Pass name: %s.\n", debugstr_a(p->name));

    read_dword(ptr, &p->object_count);
    TRACE("Pass has %u effect objects.\n", p->object_count);

    read_dword(ptr, &p->annotation_count);
    TRACE("Pass has %u annotations.\n", p->annotation_count);

    p->annotations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, p->annotation_count * sizeof(*p->annotations));
    if (!p->annotations)
    {
        ERR("Failed to allocate pass annotations memory.\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < p->annotation_count; ++i)
    {
        struct d3d10_effect_variable *a = &p->annotations[i];

        a->effect = p->technique->effect;
        a->buffer = &null_local_buffer;

        hr = parse_fx10_annotation(a, ptr, data);
        if (FAILED(hr)) return hr;
    }

    p->objects = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, p->object_count * sizeof(*p->objects));
    if (!p->objects)
    {
        ERR("Failed to allocate effect objects memory.\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < p->object_count; ++i)
    {
        struct d3d10_effect_object *o = &p->objects[i];

        o->pass = p;

        hr = parse_fx10_object(o, ptr, data);
        if (FAILED(hr)) return hr;
    }

    return hr;
}

static HRESULT parse_fx10_technique(struct d3d10_effect_technique *t, const char **ptr, const char *data)
{
    unsigned int i;
    DWORD offset;
    HRESULT hr;

    read_dword(ptr, &offset);
    TRACE("Technique name at offset %#x.\n", offset);

    if (!copy_name(data + offset, &t->name))
    {
        ERR("Failed to copy name.\n");
        return E_OUTOFMEMORY;
    }
    TRACE("Technique name: %s.\n", debugstr_a(t->name));

    read_dword(ptr, &t->pass_count);
    TRACE("Technique has %u passes\n", t->pass_count);

    read_dword(ptr, &t->annotation_count);
    TRACE("Technique has %u annotations.\n", t->annotation_count);

    t->annotations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, t->annotation_count * sizeof(*t->annotations));
    if (!t->annotations)
    {
        ERR("Failed to allocate technique annotations memory.\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < t->annotation_count; ++i)
    {
        struct d3d10_effect_variable *a = &t->annotations[i];

        a->effect = t->effect;
        a->buffer = &null_local_buffer;

        hr = parse_fx10_annotation(a, ptr, data);
        if (FAILED(hr)) return hr;
    }

    t->passes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, t->pass_count * sizeof(*t->passes));
    if (!t->passes)
    {
        ERR("Failed to allocate passes memory\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < t->pass_count; ++i)
    {
        struct d3d10_effect_pass *p = &t->passes[i];

        p->ID3D10EffectPass_iface.lpVtbl = &d3d10_effect_pass_vtbl;
        p->technique = t;

        hr = parse_fx10_pass(p, ptr, data);
        if (FAILED(hr)) return hr;
    }

    return S_OK;
}

static HRESULT parse_fx10_variable(struct d3d10_effect_variable *v, const char **ptr, const char *data)
{
    DWORD offset;
    unsigned int i;
    HRESULT hr;

    hr = parse_fx10_variable_head(v, ptr, data);
    if (FAILED(hr)) return hr;

    read_dword(ptr, &offset);
    TRACE("Variable semantic at offset %#x.\n", offset);

    if (!copy_name(data + offset, &v->semantic))
    {
        ERR("Failed to copy semantic.\n");
        return E_OUTOFMEMORY;
    }
    TRACE("Variable semantic: %s.\n", debugstr_a(v->semantic));

    read_dword(ptr, &v->buffer_offset);
    TRACE("Variable offset in buffer: %#x.\n", v->buffer_offset);

    skip_dword_unknown("variable", ptr, 1);

    read_dword(ptr, &v->flag);
    TRACE("Variable flag: %#x.\n", v->flag);

    read_dword(ptr, &v->annotation_count);
    TRACE("Variable has %u annotations.\n", v->annotation_count);

    v->annotations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, v->annotation_count * sizeof(*v->annotations));
    if (!v->annotations)
    {
        ERR("Failed to allocate variable annotations memory.\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < v->annotation_count; ++i)
    {
        struct d3d10_effect_variable *a = &v->annotations[i];

        a->effect = v->effect;
        a->buffer = &null_local_buffer;

        hr = parse_fx10_annotation(a, ptr, data);
        if (FAILED(hr)) return hr;
    }

    return S_OK;
}

static HRESULT parse_fx10_local_variable(struct d3d10_effect_variable *v, const char **ptr, const char *data)
{
    unsigned int i;
    HRESULT hr;
    DWORD offset;

    hr = parse_fx10_variable_head(v, ptr, data);
    if (FAILED(hr)) return hr;

    read_dword(ptr, &offset);
    TRACE("Variable semantic at offset %#x.\n", offset);

    if (!copy_name(data + offset, &v->semantic))
    {
        ERR("Failed to copy semantic.\n");
        return E_OUTOFMEMORY;
    }
    TRACE("Variable semantic: %s.\n", debugstr_a(v->semantic));

    skip_dword_unknown("local variable", ptr, 1);

    switch (v->type->basetype)
    {
        case D3D10_SVT_TEXTURE1D:
        case D3D10_SVT_TEXTURE1DARRAY:
        case D3D10_SVT_TEXTURE2D:
        case D3D10_SVT_TEXTURE2DARRAY:
        case D3D10_SVT_TEXTURE2DMS:
        case D3D10_SVT_TEXTURE2DMSARRAY:
        case D3D10_SVT_TEXTURE3D:
        case D3D10_SVT_TEXTURECUBE:
        case D3D10_SVT_RENDERTARGETVIEW:
        case D3D10_SVT_DEPTHSTENCILVIEW:
        case D3D10_SVT_BUFFER:
            TRACE("SVT could not have elements.\n");
            break;

        case D3D10_SVT_VERTEXSHADER:
        case D3D10_SVT_PIXELSHADER:
        case D3D10_SVT_GEOMETRYSHADER:
            TRACE("Shader type is %s\n", debug_d3d10_shader_variable_type(v->type->basetype));
            for (i = 0; i < max(v->type->element_count, 1); ++i)
            {
                DWORD shader_offset;
                struct d3d10_effect_variable *var;

                if (!v->type->element_count)
                {
                    var = v;
                }
                else
                {
                    var = &v->elements[i];
                }

                read_dword(ptr, &shader_offset);
                TRACE("Shader offset: %#x.\n", shader_offset);

                hr = parse_shader(var, data + shader_offset);
                if (FAILED(hr)) return hr;
            }
            break;

        case D3D10_SVT_DEPTHSTENCIL:
        case D3D10_SVT_BLEND:
        case D3D10_SVT_RASTERIZER:
        case D3D10_SVT_SAMPLER:
            {
                const struct d3d10_effect_state_storage_info *storage_info;
                unsigned int count = max(v->type->element_count, 1);
                unsigned char *desc;

                if (!(storage_info = get_storage_info(v->type->basetype)))
                {
                    FIXME("Failed to get backing store info for type %s.\n",
                            debug_d3d10_shader_variable_type(v->type->basetype));
                    return E_FAIL;
                }

                if (!(desc = HeapAlloc(GetProcessHeap(), 0, count * storage_info->size)))
                {
                    ERR("Failed to allocate backing store memory.\n");
                    return E_OUTOFMEMORY;
                }

                for (i = 0; i < count; ++i)
                {
                    memcpy(&desc[i * storage_info->size], storage_info->default_state, storage_info->size);

                    if (!parse_fx10_state_group(ptr, data, v->type->basetype, &desc[i * storage_info->size]))
                    {
                        ERR("Failed to read property list.\n");
                        HeapFree(GetProcessHeap(), 0, desc);
                        return E_FAIL;
                    }
                }

                v->data = desc;
            }
            break;

        default:
            FIXME("Unhandled case %s.\n", debug_d3d10_shader_variable_type(v->type->basetype));
            return E_FAIL;
    }

    read_dword(ptr, &v->annotation_count);
    TRACE("Variable has %u annotations.\n", v->annotation_count);

    v->annotations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, v->annotation_count * sizeof(*v->annotations));
    if (!v->annotations)
    {
        ERR("Failed to allocate variable annotations memory.\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < v->annotation_count; ++i)
    {
        struct d3d10_effect_variable *a = &v->annotations[i];

        a->effect = v->effect;
        a->buffer = &null_local_buffer;

        hr = parse_fx10_annotation(a, ptr, data);
        if (FAILED(hr)) return hr;
    }

    return S_OK;
}

static HRESULT parse_fx10_local_buffer(struct d3d10_effect_variable *l, const char **ptr, const char *data)
{
    unsigned int i;
    DWORD offset;
    D3D10_CBUFFER_TYPE d3d10_cbuffer_type;
    HRESULT hr;
    unsigned int stride = 0;

    /* Generate our own type, it isn't in the fx blob. */
    l->type = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*l->type));
    if (!l->type)
    {
        ERR("Failed to allocate local buffer type memory.\n");
        return E_OUTOFMEMORY;
    }
    l->type->ID3D10EffectType_iface.lpVtbl = &d3d10_effect_type_vtbl;
    l->type->type_class = D3D10_SVC_OBJECT;
    l->type->effect = l->effect;

    read_dword(ptr, &offset);
    TRACE("Local buffer name at offset %#x.\n", offset);

    if (!copy_name(data + offset, &l->name))
    {
        ERR("Failed to copy name.\n");
        return E_OUTOFMEMORY;
    }
    TRACE("Local buffer name: %s.\n", debugstr_a(l->name));

    read_dword(ptr, &l->data_size);
    TRACE("Local buffer data size: %#x.\n", l->data_size);

    read_dword(ptr, &d3d10_cbuffer_type);
    TRACE("Local buffer type: %#x.\n", d3d10_cbuffer_type);

    switch(d3d10_cbuffer_type)
    {
        case D3D10_CT_CBUFFER:
            l->type->basetype = D3D10_SVT_CBUFFER;
            if (!copy_name("cbuffer", &l->type->name))
            {
                ERR("Failed to copy name.\n");
                return E_OUTOFMEMORY;
            }
            break;

        case D3D10_CT_TBUFFER:
            l->type->basetype = D3D10_SVT_TBUFFER;
            if (!copy_name("tbuffer", &l->type->name))
            {
                ERR("Failed to copy name.\n");
                return E_OUTOFMEMORY;
            }
            break;

        default:
            ERR("Unexpected D3D10_CBUFFER_TYPE %#x!\n", d3d10_cbuffer_type);
            return E_FAIL;
    }

    read_dword(ptr, &l->type->member_count);
    TRACE("Local buffer member count: %#x.\n", l->type->member_count);

    skip_dword_unknown("local buffer", ptr, 1);

    read_dword(ptr, &l->annotation_count);
    TRACE("Local buffer has %u annotations.\n", l->annotation_count);

    l->annotations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, l->annotation_count * sizeof(*l->annotations));
    if (!l->annotations)
    {
        ERR("Failed to allocate local buffer annotations memory.\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < l->annotation_count; ++i)
    {
        struct d3d10_effect_variable *a = &l->annotations[i];

        a->effect = l->effect;
        a->buffer = &null_local_buffer;

        hr = parse_fx10_annotation(a, ptr, data);
        if (FAILED(hr)) return hr;
    }

    l->members = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, l->type->member_count * sizeof(*l->members));
    if (!l->members)
    {
        ERR("Failed to allocate members memory.\n");
        return E_OUTOFMEMORY;
    }

    l->type->members = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, l->type->member_count * sizeof(*l->type->members));
    if (!l->type->members)
    {
        ERR("Failed to allocate type members memory.\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < l->type->member_count; ++i)
    {
        struct d3d10_effect_variable *v = &l->members[i];
        struct d3d10_effect_type_member *typem = &l->type->members[i];

        v->buffer = l;
        v->effect = l->effect;

        hr = parse_fx10_variable(v, ptr, data);
        if (FAILED(hr)) return hr;

        /*
         * Copy the values from the variable type to the constant buffers type
         * members structure, because it is our own generated type.
         */
        typem->type = v->type;

        if (!copy_name(v->name, &typem->name))
        {
            ERR("Failed to copy name.\n");
            return E_OUTOFMEMORY;
        }
        TRACE("Variable name: %s.\n", debugstr_a(typem->name));

        if (!copy_name(v->semantic, &typem->semantic))
        {
            ERR("Failed to copy name.\n");
            return E_OUTOFMEMORY;
        }
        TRACE("Variable semantic: %s.\n", debugstr_a(typem->semantic));

        typem->buffer_offset = v->buffer_offset;
        TRACE("Variable buffer offset: %u.\n", typem->buffer_offset);

        l->type->size_packed += v->type->size_packed;

        /*
         * For the complete constantbuffer the size_unpacked = stride,
         * the stride is calculated like this:
         *
         * 1) if the constant buffer variables are packed with packoffset
         *    - stride = the highest used constant
         *    - the complete stride has to be a multiple of 0x10
         *
         * 2) if the constant buffer variables are NOT packed with packoffset
         *    - sum of unpacked size for all variables which fit in a 0x10 part
         *    - if the size exceeds a 0x10 part, the rest of the old part is skipped
         *      and a new part is started
         *    - if the variable is a struct it is always used a new part
         *    - the complete stride has to be a multiple of 0x10
         *
         *    e.g.:
         *             0x4, 0x4, 0x4, 0x8, 0x4, 0x14, 0x4
         *        part 0x10           0x10      0x20     -> 0x40
         */
        if (v->flag & D3D10_EFFECT_VARIABLE_EXPLICIT_BIND_POINT)
        {
            if ((v->type->size_unpacked + v->buffer_offset) > stride)
            {
                stride = v->type->size_unpacked + v->buffer_offset;
            }
        }
        else
        {
            if (v->type->type_class == D3D10_SVC_STRUCT)
            {
                stride = (stride + 0xf) & ~0xf;
            }

            if ( ((stride & 0xf) + v->type->size_unpacked) > 0x10)
            {
                stride = (stride + 0xf) & ~0xf;
            }

            stride += v->type->size_unpacked;
        }
    }
    l->type->stride = l->type->size_unpacked = (stride + 0xf) & ~0xf;

    TRACE("Constant buffer:\n");
    TRACE("\tType name: %s.\n", debugstr_a(l->type->name));
    TRACE("\tElement count: %u.\n", l->type->element_count);
    TRACE("\tMember count: %u.\n", l->type->member_count);
    TRACE("\tUnpacked size: %#x.\n", l->type->size_unpacked);
    TRACE("\tStride: %#x.\n", l->type->stride);
    TRACE("\tPacked size %#x.\n", l->type->size_packed);
    TRACE("\tBasetype: %s.\n", debug_d3d10_shader_variable_type(l->type->basetype));
    TRACE("\tTypeclass: %s.\n", debug_d3d10_shader_variable_class(l->type->type_class));

    return S_OK;
}

static int d3d10_effect_type_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct d3d10_effect_type *t = WINE_RB_ENTRY_VALUE(entry, const struct d3d10_effect_type, entry);
    const DWORD *id = key;

    return *id - t->id;
}

static void d3d10_effect_type_member_destroy(struct d3d10_effect_type_member *typem)
{
    TRACE("effect type member %p.\n", typem);

    /* Do not release typem->type, it will be covered by d3d10_effect_type_destroy(). */
    HeapFree(GetProcessHeap(), 0, typem->semantic);
    HeapFree(GetProcessHeap(), 0, typem->name);
}

static void d3d10_effect_type_destroy(struct wine_rb_entry *entry, void *context)
{
    struct d3d10_effect_type *t = WINE_RB_ENTRY_VALUE(entry, struct d3d10_effect_type, entry);

    TRACE("effect type %p.\n", t);

    if (t->elementtype)
    {
        HeapFree(GetProcessHeap(), 0, t->elementtype->name);
        HeapFree(GetProcessHeap(), 0, t->elementtype);
    }

    if (t->members)
    {
        unsigned int i;

        for (i = 0; i < t->member_count; ++i)
        {
            d3d10_effect_type_member_destroy(&t->members[i]);
        }
        HeapFree(GetProcessHeap(), 0, t->members);
    }

    HeapFree(GetProcessHeap(), 0, t->name);
    HeapFree(GetProcessHeap(), 0, t);
}

static const struct wine_rb_functions d3d10_effect_type_rb_functions =
{
    d3d10_rb_alloc,
    d3d10_rb_realloc,
    d3d10_rb_free,
    d3d10_effect_type_compare,
};

static HRESULT parse_fx10_body(struct d3d10_effect *e, const char *data, DWORD data_size)
{
    const char *ptr = data + e->index_offset;
    unsigned int i;
    HRESULT hr;

    if (wine_rb_init(&e->types, &d3d10_effect_type_rb_functions) == -1)
    {
        ERR("Failed to initialize type rbtree.\n");
        return E_FAIL;
    }

    e->local_buffers = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, e->local_buffer_count * sizeof(*e->local_buffers));
    if (!e->local_buffers)
    {
        ERR("Failed to allocate local buffer memory.\n");
        return E_OUTOFMEMORY;
    }

    e->local_variables = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, e->local_variable_count * sizeof(*e->local_variables));
    if (!e->local_variables)
    {
        ERR("Failed to allocate local variable memory.\n");
        return E_OUTOFMEMORY;
    }

    e->anonymous_shaders = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, e->anonymous_shader_count * sizeof(*e->anonymous_shaders));
    if (!e->anonymous_shaders)
    {
        ERR("Failed to allocate anonymous shaders memory\n");
        return E_OUTOFMEMORY;
    }

    e->used_shaders = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, e->used_shader_count * sizeof(*e->used_shaders));
    if (!e->used_shaders)
    {
        ERR("Failed to allocate used shaders memory\n");
        return E_OUTOFMEMORY;
    }

    e->techniques = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, e->technique_count * sizeof(*e->techniques));
    if (!e->techniques)
    {
        ERR("Failed to allocate techniques memory\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < e->local_buffer_count; ++i)
    {
        struct d3d10_effect_variable *l = &e->local_buffers[i];
        l->ID3D10EffectVariable_iface.lpVtbl = (const ID3D10EffectVariableVtbl *)&d3d10_effect_constant_buffer_vtbl;
        l->effect = e;
        l->buffer = &null_local_buffer;

        hr = parse_fx10_local_buffer(l, &ptr, data);
        if (FAILED(hr)) return hr;
    }

    for (i = 0; i < e->local_variable_count; ++i)
    {
        struct d3d10_effect_variable *v = &e->local_variables[i];

        v->effect = e;
        v->ID3D10EffectVariable_iface.lpVtbl = &d3d10_effect_variable_vtbl;
        v->buffer = &null_local_buffer;

        hr = parse_fx10_local_variable(v, &ptr, data);
        if (FAILED(hr)) return hr;
    }

    for (i = 0; i < e->technique_count; ++i)
    {
        struct d3d10_effect_technique *t = &e->techniques[i];

        t->ID3D10EffectTechnique_iface.lpVtbl = &d3d10_effect_technique_vtbl;
        t->effect = e;

        hr = parse_fx10_technique(t, &ptr, data);
        if (FAILED(hr)) return hr;
    }

    return S_OK;
}

static HRESULT parse_fx10(struct d3d10_effect *e, const char *data, DWORD data_size)
{
    const char *ptr = data;
    DWORD unknown;

    /* Compiled target version (e.g. fx_4_0=0xfeff1001, fx_4_1=0xfeff1011). */
    read_dword(&ptr, &e->version);
    TRACE("Target: %#x\n", e->version);

    read_dword(&ptr, &e->local_buffer_count);
    TRACE("Local buffer count: %u.\n", e->local_buffer_count);

    read_dword(&ptr, &e->variable_count);
    TRACE("Variable count: %u\n", e->variable_count);

    read_dword(&ptr, &e->local_variable_count);
    TRACE("Object count: %u\n", e->local_variable_count);

    read_dword(&ptr, &e->sharedbuffers_count);
    TRACE("Sharedbuffers count: %u\n", e->sharedbuffers_count);

    /* Number of variables in shared buffers? */
    read_dword(&ptr, &unknown);
    FIXME("Unknown 0: %u\n", unknown);

    read_dword(&ptr, &e->sharedobjects_count);
    TRACE("Sharedobjects count: %u\n", e->sharedobjects_count);

    read_dword(&ptr, &e->technique_count);
    TRACE("Technique count: %u\n", e->technique_count);

    read_dword(&ptr, &e->index_offset);
    TRACE("Index offset: %#x\n", e->index_offset);

    read_dword(&ptr, &unknown);
    FIXME("Unknown 1: %u\n", unknown);

    read_dword(&ptr, &e->texture_count);
    TRACE("Texture count: %u\n", e->texture_count);

    read_dword(&ptr, &e->dephstencilstate_count);
    TRACE("Depthstencilstate count: %u\n", e->dephstencilstate_count);

    read_dword(&ptr, &e->blendstate_count);
    TRACE("Blendstate count: %u\n", e->blendstate_count);

    read_dword(&ptr, &e->rasterizerstate_count);
    TRACE("Rasterizerstate count: %u\n", e->rasterizerstate_count);

    read_dword(&ptr, &e->samplerstate_count);
    TRACE("Samplerstate count: %u\n", e->samplerstate_count);

    read_dword(&ptr, &e->rendertargetview_count);
    TRACE("Rendertargetview count: %u\n", e->rendertargetview_count);

    read_dword(&ptr, &e->depthstencilview_count);
    TRACE("Depthstencilview count: %u\n", e->depthstencilview_count);

    read_dword(&ptr, &e->used_shader_count);
    TRACE("Used shader count: %u\n", e->used_shader_count);

    read_dword(&ptr, &e->anonymous_shader_count);
    TRACE("Anonymous shader count: %u\n", e->anonymous_shader_count);

    return parse_fx10_body(e, ptr, data_size - (ptr - data));
}

static HRESULT fx10_chunk_handler(const char *data, DWORD data_size, DWORD tag, void *ctx)
{
    struct d3d10_effect *e = ctx;

    TRACE("tag: %s.\n", debugstr_an((const char *)&tag, 4));

    TRACE("chunk size: %#x\n", data_size);

    switch(tag)
    {
        case TAG_FX10:
            return parse_fx10(e, data, data_size);

        default:
            FIXME("Unhandled chunk %s.\n", debugstr_an((const char *)&tag, 4));
            return S_OK;
    }
}

HRESULT d3d10_effect_parse(struct d3d10_effect *This, const void *data, SIZE_T data_size)
{
    return parse_dxbc(data, data_size, fx10_chunk_handler, This);
}

static HRESULT d3d10_effect_object_apply(struct d3d10_effect_object *o)
{
    ID3D10Device *device = o->pass->technique->effect->device;
    struct d3d10_effect_variable *v = (struct d3d10_effect_variable*) o->data;

    TRACE("effect object %p, type %#x.\n", o, o->type);

    switch(o->type)
    {
        case D3D10_EOT_VERTEXSHADER:
            ID3D10Device_VSSetShader(device, ((struct d3d10_effect_shader_variable *)v->data)->shader.vs);
            return S_OK;

        case D3D10_EOT_PIXELSHADER:
            ID3D10Device_PSSetShader(device, ((struct d3d10_effect_shader_variable *)v->data)->shader.ps);
            return S_OK;

        case D3D10_EOT_GEOMETRYSHADER:
            ID3D10Device_GSSetShader(device, ((struct d3d10_effect_shader_variable *)v->data)->shader.gs);
            return S_OK;

        default:
            FIXME("Unhandled effect object type %#x.\n", o->type);
            return E_FAIL;
    }
}

static void d3d10_effect_variable_destroy(struct d3d10_effect_variable *v)
{
    unsigned int i;

    TRACE("variable %p.\n", v);

    HeapFree(GetProcessHeap(), 0, v->name);
    HeapFree(GetProcessHeap(), 0, v->semantic);
    if (v->annotations)
    {
        for (i = 0; i < v->annotation_count; ++i)
        {
            d3d10_effect_variable_destroy(&v->annotations[i]);
        }
        HeapFree(GetProcessHeap(), 0, v->annotations);
    }

    if (v->members)
    {
        for (i = 0; i < v->type->member_count; ++i)
        {
            d3d10_effect_variable_destroy(&v->members[i]);
        }
        HeapFree(GetProcessHeap(), 0, v->members);
    }

    if (v->elements)
    {
        for (i = 0; i < v->type->element_count; ++i)
        {
            d3d10_effect_variable_destroy(&v->elements[i]);
        }
        HeapFree(GetProcessHeap(), 0, v->elements);
    }

    if (v->data)
    {
        switch(v->type->basetype)
        {
            case D3D10_SVT_VERTEXSHADER:
            case D3D10_SVT_PIXELSHADER:
            case D3D10_SVT_GEOMETRYSHADER:
                shader_free_signature(&((struct d3d10_effect_shader_variable *)v->data)->input_signature);
                shader_free_signature(&((struct d3d10_effect_shader_variable *)v->data)->output_signature);
                break;

            default:
                break;
        }
        HeapFree(GetProcessHeap(), 0, v->data);
    }
}

static void d3d10_effect_pass_destroy(struct d3d10_effect_pass *p)
{
    unsigned int i;

    TRACE("pass %p\n", p);

    HeapFree(GetProcessHeap(), 0, p->name);
    HeapFree(GetProcessHeap(), 0, p->objects);

    if (p->annotations)
    {
        for (i = 0; i < p->annotation_count; ++i)
        {
            d3d10_effect_variable_destroy(&p->annotations[i]);
        }
        HeapFree(GetProcessHeap(), 0, p->annotations);
    }
}

static void d3d10_effect_technique_destroy(struct d3d10_effect_technique *t)
{
    unsigned int i;

    TRACE("technique %p\n", t);

    HeapFree(GetProcessHeap(), 0, t->name);
    if (t->passes)
    {
        for (i = 0; i < t->pass_count; ++i)
        {
            d3d10_effect_pass_destroy(&t->passes[i]);
        }
        HeapFree(GetProcessHeap(), 0, t->passes);
    }

    if (t->annotations)
    {
        for (i = 0; i < t->annotation_count; ++i)
        {
            d3d10_effect_variable_destroy(&t->annotations[i]);
        }
        HeapFree(GetProcessHeap(), 0, t->annotations);
    }
}

static void d3d10_effect_local_buffer_destroy(struct d3d10_effect_variable *l)
{
    unsigned int i;

    TRACE("local buffer %p.\n", l);

    HeapFree(GetProcessHeap(), 0, l->name);
    if (l->members)
    {
        for (i = 0; i < l->type->member_count; ++i)
        {
            d3d10_effect_variable_destroy(&l->members[i]);
        }
        HeapFree(GetProcessHeap(), 0, l->members);
    }

    if (l->type->members)
    {
        for (i = 0; i < l->type->member_count; ++i)
        {
            /* Do not release l->type->members[i].type, it will be covered by d3d10_effect_type_destroy(). */
            HeapFree(GetProcessHeap(), 0, l->type->members[i].semantic);
            HeapFree(GetProcessHeap(), 0, l->type->members[i].name);
        }
        HeapFree(GetProcessHeap(), 0, l->type->members);
    }
    HeapFree(GetProcessHeap(), 0, l->type->name);
    HeapFree(GetProcessHeap(), 0, l->type);

    if (l->annotations)
    {
        for (i = 0; i < l->annotation_count; ++i)
        {
            d3d10_effect_variable_destroy(&l->annotations[i]);
        }
        HeapFree(GetProcessHeap(), 0, l->annotations);
    }
}

/* IUnknown methods */

static inline struct d3d10_effect *impl_from_ID3D10Effect(ID3D10Effect *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_effect, ID3D10Effect_iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_QueryInterface(ID3D10Effect *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D10Effect)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d10_effect_AddRef(ID3D10Effect *iface)
{
    struct d3d10_effect *This = impl_from_ID3D10Effect(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d10_effect_Release(ID3D10Effect *iface)
{
    struct d3d10_effect *This = impl_from_ID3D10Effect(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        unsigned int i;

        if (This->techniques)
        {
            for (i = 0; i < This->technique_count; ++i)
            {
                d3d10_effect_technique_destroy(&This->techniques[i]);
            }
            HeapFree(GetProcessHeap(), 0, This->techniques);
        }

        if (This->local_variables)
        {
            for (i = 0; i < This->local_variable_count; ++i)
            {
                d3d10_effect_variable_destroy(&This->local_variables[i]);
            }
            HeapFree(GetProcessHeap(), 0, This->local_variables);
        }

        if (This->local_buffers)
        {
            for (i = 0; i < This->local_buffer_count; ++i)
            {
                d3d10_effect_local_buffer_destroy(&This->local_buffers[i]);
            }
            HeapFree(GetProcessHeap(), 0, This->local_buffers);
        }

        if (This->anonymous_shaders)
        {
            for (i = 0; i < This->anonymous_shader_count; ++i)
            {
                d3d10_effect_variable_destroy(&This->anonymous_shaders[i].shader);
                HeapFree(GetProcessHeap(), 0, This->anonymous_shaders[i].type.name);
            }
            HeapFree(GetProcessHeap(), 0, This->anonymous_shaders);
        }

        HeapFree(GetProcessHeap(), 0, This->used_shaders);

        wine_rb_destroy(&This->types, d3d10_effect_type_destroy, NULL);

        ID3D10Device_Release(This->device);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* ID3D10Effect methods */

static BOOL STDMETHODCALLTYPE d3d10_effect_IsValid(ID3D10Effect *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static BOOL STDMETHODCALLTYPE d3d10_effect_IsPool(ID3D10Effect *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_GetDevice(ID3D10Effect *iface, ID3D10Device **device)
{
    struct d3d10_effect *This = impl_from_ID3D10Effect(iface);

    TRACE("iface %p, device %p\n", iface, device);

    ID3D10Device_AddRef(This->device);
    *device = This->device;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_GetDesc(ID3D10Effect *iface, D3D10_EFFECT_DESC *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);

    return E_NOTIMPL;
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_GetConstantBufferByIndex(ID3D10Effect *iface,
        UINT index)
{
    struct d3d10_effect *This = impl_from_ID3D10Effect(iface);
    struct d3d10_effect_variable *l;

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->local_buffer_count)
    {
        WARN("Invalid index specified\n");
        return (ID3D10EffectConstantBuffer *)&null_local_buffer;
    }

    l = &This->local_buffers[index];

    TRACE("Returning buffer %p, %s.\n", l, debugstr_a(l->name));

    return (ID3D10EffectConstantBuffer *)l;
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_GetConstantBufferByName(ID3D10Effect *iface,
        LPCSTR name)
{
    struct d3d10_effect *This = impl_from_ID3D10Effect(iface);
    unsigned int i;

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    for (i = 0; i < This->local_buffer_count; ++i)
    {
        struct d3d10_effect_variable *l = &This->local_buffers[i];

        if (!strcmp(l->name, name))
        {
            TRACE("Returning buffer %p.\n", l);
            return (ID3D10EffectConstantBuffer *)l;
        }
    }

    WARN("Invalid name specified\n");

    return (ID3D10EffectConstantBuffer *)&null_local_buffer;
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_GetVariableByIndex(ID3D10Effect *iface, UINT index)
{
    struct d3d10_effect *This = impl_from_ID3D10Effect(iface);
    unsigned int i;

    TRACE("iface %p, index %u\n", iface, index);

    for (i = 0; i < This->local_buffer_count; ++i)
    {
        struct d3d10_effect_variable *l = &This->local_buffers[i];

        if (index < l->type->member_count)
        {
            struct d3d10_effect_variable *v = &l->members[index];

            TRACE("Returning variable %p.\n", v);
            return &v->ID3D10EffectVariable_iface;
        }
        index -= l->type->member_count;
    }

    if (index < This->local_variable_count)
    {
        struct d3d10_effect_variable *v = &This->local_variables[index];

        TRACE("Returning variable %p.\n", v);
        return &v->ID3D10EffectVariable_iface;
    }

    WARN("Invalid index specified\n");

    return &null_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_GetVariableByName(ID3D10Effect *iface, LPCSTR name)
{
    struct d3d10_effect *This = impl_from_ID3D10Effect(iface);
    unsigned int i;

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("Invalid name specified\n");
        return &null_variable.ID3D10EffectVariable_iface;
    }

    for (i = 0; i < This->local_buffer_count; ++i)
    {
        struct d3d10_effect_variable *l = &This->local_buffers[i];
        unsigned int j;

        for (j = 0; j < l->type->member_count; ++j)
        {
            struct d3d10_effect_variable *v = &l->members[j];

            if (!strcmp(v->name, name))
            {
                TRACE("Returning variable %p.\n", v);
                return &v->ID3D10EffectVariable_iface;
            }
        }
    }

    for (i = 0; i < This->local_variable_count; ++i)
    {
        struct d3d10_effect_variable *v = &This->local_variables[i];

        if (!strcmp(v->name, name))
        {
            TRACE("Returning variable %p.\n", v);
            return &v->ID3D10EffectVariable_iface;
        }
    }

    WARN("Invalid name specified\n");

    return &null_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_GetVariableBySemantic(ID3D10Effect *iface,
        LPCSTR semantic)
{
    struct d3d10_effect *This = impl_from_ID3D10Effect(iface);
    unsigned int i;

    TRACE("iface %p, semantic %s\n", iface, debugstr_a(semantic));

    if (!semantic)
    {
        WARN("Invalid semantic specified\n");
        return &null_variable.ID3D10EffectVariable_iface;
    }

    for (i = 0; i < This->local_buffer_count; ++i)
    {
        struct d3d10_effect_variable *l = &This->local_buffers[i];
        unsigned int j;

        for (j = 0; j < l->type->member_count; ++j)
        {
            struct d3d10_effect_variable *v = &l->members[j];

            if (!strcmp(v->semantic, semantic))
            {
                TRACE("Returning variable %p.\n", v);
                return &v->ID3D10EffectVariable_iface;
            }
        }
    }

    for (i = 0; i < This->local_variable_count; ++i)
    {
        struct d3d10_effect_variable *v = &This->local_variables[i];

        if (!strcmp(v->semantic, semantic))
        {
            TRACE("Returning variable %p.\n", v);
            return &v->ID3D10EffectVariable_iface;
        }
    }

    WARN("Invalid semantic specified\n");

    return &null_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectTechnique * STDMETHODCALLTYPE d3d10_effect_GetTechniqueByIndex(ID3D10Effect *iface,
        UINT index)
{
    struct d3d10_effect *This = impl_from_ID3D10Effect(iface);
    struct d3d10_effect_technique *t;

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->technique_count)
    {
        WARN("Invalid index specified\n");
        return &null_technique.ID3D10EffectTechnique_iface;
    }

    t = &This->techniques[index];

    TRACE("Returning technique %p, %s.\n", t, debugstr_a(t->name));

    return &t->ID3D10EffectTechnique_iface;
}

static struct ID3D10EffectTechnique * STDMETHODCALLTYPE d3d10_effect_GetTechniqueByName(ID3D10Effect *iface,
        LPCSTR name)
{
    struct d3d10_effect *This = impl_from_ID3D10Effect(iface);
    unsigned int i;

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("Invalid name specified\n");
        return &null_technique.ID3D10EffectTechnique_iface;
    }

    for (i = 0; i < This->technique_count; ++i)
    {
        struct d3d10_effect_technique *t = &This->techniques[i];
        if (!strcmp(t->name, name))
        {
            TRACE("Returning technique %p\n", t);
            return &t->ID3D10EffectTechnique_iface;
        }
    }

    WARN("Invalid name specified\n");

    return &null_technique.ID3D10EffectTechnique_iface;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_Optimize(ID3D10Effect *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static BOOL STDMETHODCALLTYPE d3d10_effect_IsOptimized(ID3D10Effect *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

const struct ID3D10EffectVtbl d3d10_effect_vtbl =
{
    /* IUnknown methods */
    d3d10_effect_QueryInterface,
    d3d10_effect_AddRef,
    d3d10_effect_Release,
    /* ID3D10Effect methods */
    d3d10_effect_IsValid,
    d3d10_effect_IsPool,
    d3d10_effect_GetDevice,
    d3d10_effect_GetDesc,
    d3d10_effect_GetConstantBufferByIndex,
    d3d10_effect_GetConstantBufferByName,
    d3d10_effect_GetVariableByIndex,
    d3d10_effect_GetVariableByName,
    d3d10_effect_GetVariableBySemantic,
    d3d10_effect_GetTechniqueByIndex,
    d3d10_effect_GetTechniqueByName,
    d3d10_effect_Optimize,
    d3d10_effect_IsOptimized,
};

/* ID3D10EffectTechnique methods */

static inline struct d3d10_effect_technique *impl_from_ID3D10EffectTechnique(ID3D10EffectTechnique *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_effect_technique, ID3D10EffectTechnique_iface);
}

static BOOL STDMETHODCALLTYPE d3d10_effect_technique_IsValid(ID3D10EffectTechnique *iface)
{
    struct d3d10_effect_technique *This = impl_from_ID3D10EffectTechnique(iface);

    TRACE("iface %p\n", iface);

    return This != &null_technique;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_technique_GetDesc(ID3D10EffectTechnique *iface,
        D3D10_TECHNIQUE_DESC *desc)
{
    struct d3d10_effect_technique *This = impl_from_ID3D10EffectTechnique(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    if(This == &null_technique)
    {
        WARN("Null technique specified\n");
        return E_FAIL;
    }

    if(!desc)
    {
        WARN("Invalid argument specified\n");
        return E_INVALIDARG;
    }

    desc->Name = This->name;
    desc->Passes = This->pass_count;
    desc->Annotations = This->annotation_count;

    return S_OK;
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_technique_GetAnnotationByIndex(
        ID3D10EffectTechnique *iface, UINT index)
{
    struct d3d10_effect_technique *This = impl_from_ID3D10EffectTechnique(iface);
    struct d3d10_effect_variable *a;

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->annotation_count)
    {
        WARN("Invalid index specified\n");
        return &null_variable.ID3D10EffectVariable_iface;
    }

    a = &This->annotations[index];

    TRACE("Returning annotation %p, %s\n", a, debugstr_a(a->name));

    return &a->ID3D10EffectVariable_iface;
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_technique_GetAnnotationByName(
        ID3D10EffectTechnique *iface, LPCSTR name)
{
    struct d3d10_effect_technique *This = impl_from_ID3D10EffectTechnique(iface);
    unsigned int i;

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    for (i = 0; i < This->annotation_count; ++i)
    {
        struct d3d10_effect_variable *a = &This->annotations[i];
        if (!strcmp(a->name, name))
        {
            TRACE("Returning annotation %p\n", a);
            return &a->ID3D10EffectVariable_iface;
        }
    }

    WARN("Invalid name specified\n");

    return &null_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectPass * STDMETHODCALLTYPE d3d10_effect_technique_GetPassByIndex(ID3D10EffectTechnique *iface,
        UINT index)
{
    struct d3d10_effect_technique *This = impl_from_ID3D10EffectTechnique(iface);
    struct d3d10_effect_pass *p;

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->pass_count)
    {
        WARN("Invalid index specified\n");
        return &null_pass.ID3D10EffectPass_iface;
    }

    p = &This->passes[index];

    TRACE("Returning pass %p, %s.\n", p, debugstr_a(p->name));

    return &p->ID3D10EffectPass_iface;
}

static struct ID3D10EffectPass * STDMETHODCALLTYPE d3d10_effect_technique_GetPassByName(ID3D10EffectTechnique *iface,
        LPCSTR name)
{
    struct d3d10_effect_technique *This = impl_from_ID3D10EffectTechnique(iface);
    unsigned int i;

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    /* Do not check for name==NULL, W7/DX10 crashes in that case. */

    for (i = 0; i < This->pass_count; ++i)
    {
        struct d3d10_effect_pass *p = &This->passes[i];
        if (!strcmp(p->name, name))
        {
            TRACE("Returning pass %p\n", p);
            return &p->ID3D10EffectPass_iface;
        }
    }

    WARN("Invalid name specified\n");

    return &null_pass.ID3D10EffectPass_iface;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_technique_ComputeStateBlockMask(ID3D10EffectTechnique *iface,
        D3D10_STATE_BLOCK_MASK *mask)
{
    FIXME("iface %p,mask %p stub!\n", iface, mask);

    return E_NOTIMPL;
}

static const struct ID3D10EffectTechniqueVtbl d3d10_effect_technique_vtbl =
{
    /* ID3D10EffectTechnique methods */
    d3d10_effect_technique_IsValid,
    d3d10_effect_technique_GetDesc,
    d3d10_effect_technique_GetAnnotationByIndex,
    d3d10_effect_technique_GetAnnotationByName,
    d3d10_effect_technique_GetPassByIndex,
    d3d10_effect_technique_GetPassByName,
    d3d10_effect_technique_ComputeStateBlockMask,
};

/* ID3D10EffectPass methods */

static inline struct d3d10_effect_pass *impl_from_ID3D10EffectPass(ID3D10EffectPass *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_effect_pass, ID3D10EffectPass_iface);
}

static BOOL STDMETHODCALLTYPE d3d10_effect_pass_IsValid(ID3D10EffectPass *iface)
{
    struct d3d10_effect_pass *This = impl_from_ID3D10EffectPass(iface);

    TRACE("iface %p\n", iface);

    return This != &null_pass;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_pass_GetDesc(ID3D10EffectPass *iface,
        D3D10_PASS_DESC *desc)
{
    struct d3d10_effect_pass *This = impl_from_ID3D10EffectPass(iface);
    unsigned int i;

    FIXME("iface %p, desc %p partial stub!\n", iface, desc);

    if(This == &null_pass)
    {
        WARN("Null pass specified\n");
        return E_FAIL;
    }

    if(!desc)
    {
        WARN("Invalid argument specified\n");
        return E_INVALIDARG;
    }

    memset(desc, 0, sizeof(*desc));
    desc->Name = This->name;
    for (i = 0; i < This->object_count; ++i)
    {
        struct d3d10_effect_object *o = &This->objects[i];
        if (o->type == D3D10_EOT_VERTEXSHADER)
        {
            struct d3d10_effect_variable *v = o->data;
            struct d3d10_effect_shader_variable *s = v->data;
            desc->pIAInputSignature = (BYTE *)s->input_signature.signature;
            desc->IAInputSignatureSize = s->input_signature.signature_size;
            break;
        }
    }

    desc->StencilRef = This->stencil_ref;
    desc->SampleMask = This->sample_mask;
    memcpy(desc->BlendFactor, This->blend_factor, 4 * sizeof(float));

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_pass_GetVertexShaderDesc(ID3D10EffectPass *iface,
        D3D10_PASS_SHADER_DESC *desc)
{
    struct d3d10_effect_pass *This = impl_from_ID3D10EffectPass(iface);
    unsigned int i;

    TRACE("iface %p, desc %p\n", iface, desc);

    if (This == &null_pass)
    {
        WARN("Null pass specified\n");
        return E_FAIL;
    }

    if (!desc)
    {
        WARN("Invalid argument specified\n");
        return E_INVALIDARG;
    }

    for (i = 0; i < This->object_count; ++i)
    {
        struct d3d10_effect_object *o = &This->objects[i];

        if (o->type == D3D10_EOT_VERTEXSHADER)
        {
            desc->pShaderVariable = o->data;
            desc->ShaderIndex = o->index;
            return S_OK;
        }
    }

    TRACE("Returning null_shader_variable\n");
    desc->pShaderVariable = (ID3D10EffectShaderVariable *)&null_shader_variable;
    desc->ShaderIndex = 0;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_pass_GetGeometryShaderDesc(ID3D10EffectPass *iface,
        D3D10_PASS_SHADER_DESC *desc)
{
    struct d3d10_effect_pass *This = impl_from_ID3D10EffectPass(iface);
    unsigned int i;

    TRACE("iface %p, desc %p\n", iface, desc);

    if (This == &null_pass)
    {
        WARN("Null pass specified\n");
        return E_FAIL;
    }

    if (!desc)
    {
        WARN("Invalid argument specified\n");
        return E_INVALIDARG;
    }

    for (i = 0; i < This->object_count; ++i)
    {
        struct d3d10_effect_object *o = &This->objects[i];

        if (o->type == D3D10_EOT_GEOMETRYSHADER)
        {
            desc->pShaderVariable = o->data;
            desc->ShaderIndex = o->index;
            return S_OK;
        }
    }

    TRACE("Returning null_shader_variable\n");
    desc->pShaderVariable = (ID3D10EffectShaderVariable *)&null_shader_variable;
    desc->ShaderIndex = 0;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_pass_GetPixelShaderDesc(ID3D10EffectPass *iface,
        D3D10_PASS_SHADER_DESC *desc)
{
    struct d3d10_effect_pass *This = impl_from_ID3D10EffectPass(iface);
    unsigned int i;

    TRACE("iface %p, desc %p\n", iface, desc);

    if (This == &null_pass)
    {
        WARN("Null pass specified\n");
        return E_FAIL;
    }

    if (!desc)
    {
        WARN("Invalid argument specified\n");
        return E_INVALIDARG;
    }

    for (i = 0; i < This->object_count; ++i)
    {
        struct d3d10_effect_object *o = &This->objects[i];

        if (o->type == D3D10_EOT_PIXELSHADER)
        {
            desc->pShaderVariable = o->data;
            desc->ShaderIndex = o->index;
            return S_OK;
        }
    }

    TRACE("Returning null_shader_variable\n");
    desc->pShaderVariable = (ID3D10EffectShaderVariable *)&null_shader_variable;
    desc->ShaderIndex = 0;

    return S_OK;
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_pass_GetAnnotationByIndex(ID3D10EffectPass *iface,
        UINT index)
{
    struct d3d10_effect_pass *This = impl_from_ID3D10EffectPass(iface);
    struct d3d10_effect_variable *a;

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->annotation_count)
    {
        WARN("Invalid index specified\n");
        return &null_variable.ID3D10EffectVariable_iface;
    }

    a = &This->annotations[index];

    TRACE("Returning annotation %p, %s\n", a, debugstr_a(a->name));

    return &a->ID3D10EffectVariable_iface;
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_pass_GetAnnotationByName(ID3D10EffectPass *iface,
        LPCSTR name)
{
    struct d3d10_effect_pass *This = impl_from_ID3D10EffectPass(iface);
    unsigned int i;

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    for (i = 0; i < This->annotation_count; ++i)
    {
        struct d3d10_effect_variable *a = &This->annotations[i];
        if (!strcmp(a->name, name))
        {
            TRACE("Returning annotation %p\n", a);
            return &a->ID3D10EffectVariable_iface;
        }
    }

    WARN("Invalid name specified\n");

    return &null_variable.ID3D10EffectVariable_iface;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_pass_Apply(ID3D10EffectPass *iface, UINT flags)
{
    struct d3d10_effect_pass *This = impl_from_ID3D10EffectPass(iface);
    HRESULT hr = S_OK;
    unsigned int i;

    TRACE("iface %p, flags %#x\n", iface, flags);

    if (flags) FIXME("Ignoring flags (%#x)\n", flags);

    for (i = 0; i < This->object_count; ++i)
    {
        hr = d3d10_effect_object_apply(&This->objects[i]);
        if (FAILED(hr)) break;
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_pass_ComputeStateBlockMask(ID3D10EffectPass *iface,
        D3D10_STATE_BLOCK_MASK *mask)
{
    FIXME("iface %p, mask %p stub!\n", iface, mask);

    return E_NOTIMPL;
}

static const struct ID3D10EffectPassVtbl d3d10_effect_pass_vtbl =
{
    /* ID3D10EffectPass methods */
    d3d10_effect_pass_IsValid,
    d3d10_effect_pass_GetDesc,
    d3d10_effect_pass_GetVertexShaderDesc,
    d3d10_effect_pass_GetGeometryShaderDesc,
    d3d10_effect_pass_GetPixelShaderDesc,
    d3d10_effect_pass_GetAnnotationByIndex,
    d3d10_effect_pass_GetAnnotationByName,
    d3d10_effect_pass_Apply,
    d3d10_effect_pass_ComputeStateBlockMask,
};

/* ID3D10EffectVariable methods */

static BOOL STDMETHODCALLTYPE d3d10_effect_variable_IsValid(ID3D10EffectVariable *iface)
{
    TRACE("iface %p\n", iface);

    return impl_from_ID3D10EffectVariable(iface) != &null_variable;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_variable_GetType(ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    return &This->type->ID3D10EffectType_iface;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_variable_GetDesc(ID3D10EffectVariable *iface,
        D3D10_EFFECT_VARIABLE_DESC *desc)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    if (!iface->lpVtbl->IsValid(iface))
    {
        WARN("Null variable specified\n");
        return E_FAIL;
    }

    if (!desc)
    {
        WARN("Invalid argument specified\n");
        return E_INVALIDARG;
    }

    /* FIXME: This isn't correct. Anonymous shaders let desc->ExplicitBindPoint untouched, but normal shaders set it! */
    memset(desc, 0, sizeof(*desc));
    desc->Name = This->name;
    desc->Semantic = This->semantic;
    desc->Flags = This->flag;
    desc->Annotations = This->annotation_count;
    desc->BufferOffset = This->buffer_offset;

    if (This->flag & D3D10_EFFECT_VARIABLE_EXPLICIT_BIND_POINT)
    {
        desc->ExplicitBindPoint = This->buffer_offset;
    }

    return S_OK;
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_variable_GetAnnotationByIndex(
        ID3D10EffectVariable *iface, UINT index)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);
    struct d3d10_effect_variable *a;

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->annotation_count)
    {
        WARN("Invalid index specified\n");
        return &null_variable.ID3D10EffectVariable_iface;
    }

    a = &This->annotations[index];

    TRACE("Returning annotation %p, %s\n", a, debugstr_a(a->name));

    return &a->ID3D10EffectVariable_iface;
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_variable_GetAnnotationByName(
        ID3D10EffectVariable *iface, LPCSTR name)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);
    unsigned int i;

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    for (i = 0; i < This->annotation_count; ++i)
    {
        struct d3d10_effect_variable *a = &This->annotations[i];
        if (!strcmp(a->name, name))
        {
            TRACE("Returning annotation %p\n", a);
            return &a->ID3D10EffectVariable_iface;
        }
    }

    WARN("Invalid name specified\n");

    return &null_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_variable_GetMemberByIndex(
        ID3D10EffectVariable *iface, UINT index)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);
    struct d3d10_effect_variable *m;

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->type->member_count)
    {
        WARN("Invalid index specified\n");
        return &null_variable.ID3D10EffectVariable_iface;
    }

    m = &This->members[index];

    TRACE("Returning member %p, %s\n", m, debugstr_a(m->name));

    return &m->ID3D10EffectVariable_iface;
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_variable_GetMemberByName(
        ID3D10EffectVariable *iface, LPCSTR name)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);
    unsigned int i;

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("Invalid name specified\n");
        return &null_variable.ID3D10EffectVariable_iface;
    }

    for (i = 0; i < This->type->member_count; ++i)
    {
        struct d3d10_effect_variable *m = &This->members[i];

        if (m->name)
        {
            if (!strcmp(m->name, name))
            {
                TRACE("Returning member %p\n", m);
                return &m->ID3D10EffectVariable_iface;
            }
        }
    }

    WARN("Invalid name specified\n");

    return &null_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_variable_GetMemberBySemantic(
        ID3D10EffectVariable *iface, LPCSTR semantic)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);
    unsigned int i;

    TRACE("iface %p, semantic %s.\n", iface, debugstr_a(semantic));

    if (!semantic)
    {
        WARN("Invalid semantic specified\n");
        return &null_variable.ID3D10EffectVariable_iface;
    }

    for (i = 0; i < This->type->member_count; ++i)
    {
        struct d3d10_effect_variable *m = &This->members[i];

        if (m->semantic)
        {
            if (!strcmp(m->semantic, semantic))
            {
                TRACE("Returning member %p\n", m);
                return &m->ID3D10EffectVariable_iface;
            }
        }
    }

    WARN("Invalid semantic specified\n");

    return &null_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_variable_GetElement(
        ID3D10EffectVariable *iface, UINT index)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);
    struct d3d10_effect_variable *v;

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->type->element_count)
    {
        WARN("Invalid index specified\n");
        return &null_variable.ID3D10EffectVariable_iface;
    }

    v = &This->elements[index];

    TRACE("Returning element %p, %s\n", v, debugstr_a(v->name));

    return &v->ID3D10EffectVariable_iface;
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_variable_GetParentConstantBuffer(
        ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    return (ID3D10EffectConstantBuffer *)This->buffer;
}

static struct ID3D10EffectScalarVariable * STDMETHODCALLTYPE d3d10_effect_variable_AsScalar(
        ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    if (This->ID3D10EffectVariable_iface.lpVtbl == (const ID3D10EffectVariableVtbl *)&d3d10_effect_scalar_variable_vtbl)
        return (ID3D10EffectScalarVariable *)&This->ID3D10EffectVariable_iface;

    return (ID3D10EffectScalarVariable *)&null_scalar_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectVectorVariable * STDMETHODCALLTYPE d3d10_effect_variable_AsVector(
        ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    if (This->ID3D10EffectVariable_iface.lpVtbl == (const ID3D10EffectVariableVtbl *)&d3d10_effect_vector_variable_vtbl)
        return (ID3D10EffectVectorVariable *)&This->ID3D10EffectVariable_iface;

    return (ID3D10EffectVectorVariable *)&null_vector_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectMatrixVariable * STDMETHODCALLTYPE d3d10_effect_variable_AsMatrix(
        ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    if (This->ID3D10EffectVariable_iface.lpVtbl == (const ID3D10EffectVariableVtbl *)&d3d10_effect_matrix_variable_vtbl)
        return (ID3D10EffectMatrixVariable *)&This->ID3D10EffectVariable_iface;

    return (ID3D10EffectMatrixVariable *)&null_matrix_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectStringVariable * STDMETHODCALLTYPE d3d10_effect_variable_AsString(
        ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    if (This->ID3D10EffectVariable_iface.lpVtbl == (const ID3D10EffectVariableVtbl *)&d3d10_effect_string_variable_vtbl)
        return (ID3D10EffectStringVariable *)&This->ID3D10EffectVariable_iface;

    return (ID3D10EffectStringVariable *)&null_string_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectShaderResourceVariable * STDMETHODCALLTYPE d3d10_effect_variable_AsShaderResource(
        ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    if (This->ID3D10EffectVariable_iface.lpVtbl == (const ID3D10EffectVariableVtbl *)&d3d10_effect_shader_resource_variable_vtbl)
        return (ID3D10EffectShaderResourceVariable *)&This->ID3D10EffectVariable_iface;

    return (ID3D10EffectShaderResourceVariable *)&null_shader_resource_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectRenderTargetViewVariable * STDMETHODCALLTYPE d3d10_effect_variable_AsRenderTargetView(
        ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    if (This->ID3D10EffectVariable_iface.lpVtbl == (const ID3D10EffectVariableVtbl *)&d3d10_effect_render_target_view_variable_vtbl)
        return (ID3D10EffectRenderTargetViewVariable *)&This->ID3D10EffectVariable_iface;

    return (ID3D10EffectRenderTargetViewVariable *)&null_render_target_view_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectDepthStencilViewVariable * STDMETHODCALLTYPE d3d10_effect_variable_AsDepthStencilView(
        ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    if (This->ID3D10EffectVariable_iface.lpVtbl == (const ID3D10EffectVariableVtbl *)&d3d10_effect_depth_stencil_view_variable_vtbl)
        return (ID3D10EffectDepthStencilViewVariable *)&This->ID3D10EffectVariable_iface;

    return (ID3D10EffectDepthStencilViewVariable *)&null_depth_stencil_view_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_variable_AsConstantBuffer(
        ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    if (This->ID3D10EffectVariable_iface.lpVtbl == (const ID3D10EffectVariableVtbl *)&d3d10_effect_constant_buffer_vtbl)
        return (ID3D10EffectConstantBuffer *)&This->ID3D10EffectVariable_iface;

    return (ID3D10EffectConstantBuffer *)&null_local_buffer.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectShaderVariable * STDMETHODCALLTYPE d3d10_effect_variable_AsShader(
        ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    if (This->ID3D10EffectVariable_iface.lpVtbl == (const ID3D10EffectVariableVtbl *)&d3d10_effect_shader_variable_vtbl)
        return (ID3D10EffectShaderVariable *)&This->ID3D10EffectVariable_iface;

    return (ID3D10EffectShaderVariable *)&null_shader_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectBlendVariable * STDMETHODCALLTYPE d3d10_effect_variable_AsBlend(ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    if (This->ID3D10EffectVariable_iface.lpVtbl == (const ID3D10EffectVariableVtbl *)&d3d10_effect_blend_variable_vtbl)
        return (ID3D10EffectBlendVariable *)&This->ID3D10EffectVariable_iface;

    return (ID3D10EffectBlendVariable *)&null_blend_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectDepthStencilVariable * STDMETHODCALLTYPE d3d10_effect_variable_AsDepthStencil(
        ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    if (This->ID3D10EffectVariable_iface.lpVtbl == (const ID3D10EffectVariableVtbl *)&d3d10_effect_depth_stencil_variable_vtbl)
        return (ID3D10EffectDepthStencilVariable *)&This->ID3D10EffectVariable_iface;

    return (ID3D10EffectDepthStencilVariable *)&null_depth_stencil_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectRasterizerVariable * STDMETHODCALLTYPE d3d10_effect_variable_AsRasterizer(
        ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    if (This->ID3D10EffectVariable_iface.lpVtbl == (const ID3D10EffectVariableVtbl *)&d3d10_effect_rasterizer_variable_vtbl)
        return (ID3D10EffectRasterizerVariable *)&This->ID3D10EffectVariable_iface;

    return (ID3D10EffectRasterizerVariable *)&null_rasterizer_variable.ID3D10EffectVariable_iface;
}

static struct ID3D10EffectSamplerVariable * STDMETHODCALLTYPE d3d10_effect_variable_AsSampler(
        ID3D10EffectVariable *iface)
{
    struct d3d10_effect_variable *This = impl_from_ID3D10EffectVariable(iface);

    TRACE("iface %p\n", iface);

    if (This->ID3D10EffectVariable_iface.lpVtbl == (const ID3D10EffectVariableVtbl *)&d3d10_effect_sampler_variable_vtbl)
        return (ID3D10EffectSamplerVariable *)&This->ID3D10EffectVariable_iface;

    return (ID3D10EffectSamplerVariable *)&null_sampler_variable.ID3D10EffectVariable_iface;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_variable_SetRawValue(ID3D10EffectVariable *iface,
        void *data, UINT offset, UINT count)
{
    FIXME("iface %p, data %p, offset %u, count %u stub!\n", iface, data, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_variable_GetRawValue(ID3D10EffectVariable *iface,
        void *data, UINT offset, UINT count)
{
    FIXME("iface %p, data %p, offset %u, count %u stub!\n", iface, data, offset, count);

    return E_NOTIMPL;
}

static const struct ID3D10EffectVariableVtbl d3d10_effect_variable_vtbl =
{
    /* ID3D10EffectVariable methods */
    d3d10_effect_variable_IsValid,
    d3d10_effect_variable_GetType,
    d3d10_effect_variable_GetDesc,
    d3d10_effect_variable_GetAnnotationByIndex,
    d3d10_effect_variable_GetAnnotationByName,
    d3d10_effect_variable_GetMemberByIndex,
    d3d10_effect_variable_GetMemberByName,
    d3d10_effect_variable_GetMemberBySemantic,
    d3d10_effect_variable_GetElement,
    d3d10_effect_variable_GetParentConstantBuffer,
    d3d10_effect_variable_AsScalar,
    d3d10_effect_variable_AsVector,
    d3d10_effect_variable_AsMatrix,
    d3d10_effect_variable_AsString,
    d3d10_effect_variable_AsShaderResource,
    d3d10_effect_variable_AsRenderTargetView,
    d3d10_effect_variable_AsDepthStencilView,
    d3d10_effect_variable_AsConstantBuffer,
    d3d10_effect_variable_AsShader,
    d3d10_effect_variable_AsBlend,
    d3d10_effect_variable_AsDepthStencil,
    d3d10_effect_variable_AsRasterizer,
    d3d10_effect_variable_AsSampler,
    d3d10_effect_variable_SetRawValue,
    d3d10_effect_variable_GetRawValue,
};

/* ID3D10EffectVariable methods */
static BOOL STDMETHODCALLTYPE d3d10_effect_constant_buffer_IsValid(ID3D10EffectConstantBuffer *iface)
{
    TRACE("iface %p\n", iface);

    return (struct d3d10_effect_variable *)iface != &null_local_buffer;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_constant_buffer_GetType(ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_GetType((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_constant_buffer_GetDesc(ID3D10EffectConstantBuffer *iface,
        D3D10_EFFECT_VARIABLE_DESC *desc)
{
    return d3d10_effect_variable_GetDesc((ID3D10EffectVariable *)iface, desc);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_GetAnnotationByIndex(
        ID3D10EffectConstantBuffer *iface, UINT index)
{
    return d3d10_effect_variable_GetAnnotationByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_GetAnnotationByName(
        ID3D10EffectConstantBuffer *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetAnnotationByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_GetMemberByIndex(
        ID3D10EffectConstantBuffer *iface, UINT index)
{
    return d3d10_effect_variable_GetMemberByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_GetMemberByName(
        ID3D10EffectConstantBuffer *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetMemberByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_GetMemberBySemantic(
        ID3D10EffectConstantBuffer *iface, LPCSTR semantic)
{
    return d3d10_effect_variable_GetMemberBySemantic((ID3D10EffectVariable *)iface, semantic);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_GetElement(
        ID3D10EffectConstantBuffer *iface, UINT index)
{
    return d3d10_effect_variable_GetElement((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_constant_buffer_GetParentConstantBuffer(
        ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_GetParentConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectScalarVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_AsScalar(
        ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_AsScalar((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectVectorVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_AsVector(
        ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_AsVector((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectMatrixVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_AsMatrix(
        ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_AsMatrix((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectStringVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_AsString(
        ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_AsString((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderResourceVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_AsShaderResource(
        ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_AsShaderResource((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRenderTargetViewVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_AsRenderTargetView(
        ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_AsRenderTargetView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilViewVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_AsDepthStencilView(
        ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_AsDepthStencilView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_constant_buffer_AsConstantBuffer(
        ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_AsConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_AsShader(
        ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_AsShader((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectBlendVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_AsBlend(ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_AsBlend((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_AsDepthStencil(
        ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_AsDepthStencil((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRasterizerVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_AsRasterizer(
        ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_AsRasterizer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectSamplerVariable * STDMETHODCALLTYPE d3d10_effect_constant_buffer_AsSampler(
        ID3D10EffectConstantBuffer *iface)
{
    return d3d10_effect_variable_AsSampler((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_constant_buffer_SetRawValue(ID3D10EffectConstantBuffer *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_SetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_constant_buffer_GetRawValue(ID3D10EffectConstantBuffer *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_GetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

/* ID3D10EffectConstantBuffer methods */
static HRESULT STDMETHODCALLTYPE d3d10_effect_constant_buffer_SetConstantBuffer(ID3D10EffectConstantBuffer *iface,
        ID3D10Buffer *buffer)
{
    FIXME("iface %p, buffer %p stub!\n", iface, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_constant_buffer_GetConstantBuffer(ID3D10EffectConstantBuffer *iface,
        ID3D10Buffer **buffer)
{
    FIXME("iface %p, buffer %p stub!\n", iface, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_constant_buffer_SetTextureBuffer(ID3D10EffectConstantBuffer *iface,
        ID3D10ShaderResourceView *view)
{
    FIXME("iface %p, view %p stub!\n", iface, view);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_constant_buffer_GetTextureBuffer(ID3D10EffectConstantBuffer *iface,
        ID3D10ShaderResourceView **view)
{
    FIXME("iface %p, view %p stub!\n", iface, view);

    return E_NOTIMPL;
}

static const struct ID3D10EffectConstantBufferVtbl d3d10_effect_constant_buffer_vtbl =
{
    /* ID3D10EffectVariable methods */
    d3d10_effect_constant_buffer_IsValid,
    d3d10_effect_constant_buffer_GetType,
    d3d10_effect_constant_buffer_GetDesc,
    d3d10_effect_constant_buffer_GetAnnotationByIndex,
    d3d10_effect_constant_buffer_GetAnnotationByName,
    d3d10_effect_constant_buffer_GetMemberByIndex,
    d3d10_effect_constant_buffer_GetMemberByName,
    d3d10_effect_constant_buffer_GetMemberBySemantic,
    d3d10_effect_constant_buffer_GetElement,
    d3d10_effect_constant_buffer_GetParentConstantBuffer,
    d3d10_effect_constant_buffer_AsScalar,
    d3d10_effect_constant_buffer_AsVector,
    d3d10_effect_constant_buffer_AsMatrix,
    d3d10_effect_constant_buffer_AsString,
    d3d10_effect_constant_buffer_AsShaderResource,
    d3d10_effect_constant_buffer_AsRenderTargetView,
    d3d10_effect_constant_buffer_AsDepthStencilView,
    d3d10_effect_constant_buffer_AsConstantBuffer,
    d3d10_effect_constant_buffer_AsShader,
    d3d10_effect_constant_buffer_AsBlend,
    d3d10_effect_constant_buffer_AsDepthStencil,
    d3d10_effect_constant_buffer_AsRasterizer,
    d3d10_effect_constant_buffer_AsSampler,
    d3d10_effect_constant_buffer_SetRawValue,
    d3d10_effect_constant_buffer_GetRawValue,
    /* ID3D10EffectConstantBuffer methods */
    d3d10_effect_constant_buffer_SetConstantBuffer,
    d3d10_effect_constant_buffer_GetConstantBuffer,
    d3d10_effect_constant_buffer_SetTextureBuffer,
    d3d10_effect_constant_buffer_GetTextureBuffer,
};

/* ID3D10EffectVariable methods */

static BOOL STDMETHODCALLTYPE d3d10_effect_scalar_variable_IsValid(ID3D10EffectScalarVariable *iface)
{
    TRACE("iface %p\n", iface);

    return (struct d3d10_effect_variable *)iface != &null_scalar_variable;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetType(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_GetType((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetDesc(ID3D10EffectScalarVariable *iface,
        D3D10_EFFECT_VARIABLE_DESC *desc)
{
    return d3d10_effect_variable_GetDesc((ID3D10EffectVariable *)iface, desc);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetAnnotationByIndex(
        ID3D10EffectScalarVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetAnnotationByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetAnnotationByName(
        ID3D10EffectScalarVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetAnnotationByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetMemberByIndex(
        ID3D10EffectScalarVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetMemberByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetMemberByName(
        ID3D10EffectScalarVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetMemberByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetMemberBySemantic(
        ID3D10EffectScalarVariable *iface, LPCSTR semantic)
{
    return d3d10_effect_variable_GetMemberBySemantic((ID3D10EffectVariable *)iface, semantic);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetElement(
        ID3D10EffectScalarVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetElement((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetParentConstantBuffer(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_GetParentConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectScalarVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_AsScalar(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_AsScalar((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectVectorVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_AsVector(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_AsVector((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectMatrixVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_AsMatrix(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_AsMatrix((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectStringVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_AsString(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_AsString((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderResourceVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_AsShaderResource(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_AsShaderResource((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRenderTargetViewVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_AsRenderTargetView(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_AsRenderTargetView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilViewVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_AsDepthStencilView(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencilView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_scalar_variable_AsConstantBuffer(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_AsConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_AsShader(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_AsShader((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectBlendVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_AsBlend(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_AsBlend((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_AsDepthStencil(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencil((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRasterizerVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_AsRasterizer(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_AsRasterizer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectSamplerVariable * STDMETHODCALLTYPE d3d10_effect_scalar_variable_AsSampler(
        ID3D10EffectScalarVariable *iface)
{
    return d3d10_effect_variable_AsSampler((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_SetRawValue(ID3D10EffectScalarVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_SetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetRawValue(ID3D10EffectScalarVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_GetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

/* ID3D10EffectScalarVariable methods */

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_SetFloat(ID3D10EffectScalarVariable *iface,
        float value)
{
    FIXME("iface %p, value %.8e stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetFloat(ID3D10EffectScalarVariable *iface,
        float *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_SetFloatArray(ID3D10EffectScalarVariable *iface,
        float *values, UINT offset, UINT count)
{
    FIXME("iface %p, values %p, offset %u, count %u stub!\n", iface, values, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetFloatArray(ID3D10EffectScalarVariable *iface,
        float *values, UINT offset, UINT count)
{
    FIXME("iface %p, values %p, offset %u, count %u stub!\n", iface, values, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_SetInt(ID3D10EffectScalarVariable *iface,
        int value)
{
    FIXME("iface %p, value %d stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetInt(ID3D10EffectScalarVariable *iface,
        int *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_SetIntArray(ID3D10EffectScalarVariable *iface,
        int *values, UINT offset, UINT count)
{
    FIXME("iface %p, values %p, offset %u, count %u stub!\n", iface, values, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetIntArray(ID3D10EffectScalarVariable *iface,
        int *values, UINT offset, UINT count)
{
    FIXME("iface %p, values %p, offset %u, count %u stub!\n", iface, values, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_SetBool(ID3D10EffectScalarVariable *iface,
        BOOL value)
{
    FIXME("iface %p, value %d stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetBool(ID3D10EffectScalarVariable *iface,
        BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_SetBoolArray(ID3D10EffectScalarVariable *iface,
        BOOL *values, UINT offset, UINT count)
{
    FIXME("iface %p, values %p, offset %u, count %u stub!\n", iface, values, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_scalar_variable_GetBoolArray(ID3D10EffectScalarVariable *iface,
        BOOL *values, UINT offset, UINT count)
{
    FIXME("iface %p, values %p, offset %u, count %u stub!\n", iface, values, offset, count);

    return E_NOTIMPL;
}

static const struct ID3D10EffectScalarVariableVtbl d3d10_effect_scalar_variable_vtbl =
{
    /* ID3D10EffectVariable methods */
    d3d10_effect_scalar_variable_IsValid,
    d3d10_effect_scalar_variable_GetType,
    d3d10_effect_scalar_variable_GetDesc,
    d3d10_effect_scalar_variable_GetAnnotationByIndex,
    d3d10_effect_scalar_variable_GetAnnotationByName,
    d3d10_effect_scalar_variable_GetMemberByIndex,
    d3d10_effect_scalar_variable_GetMemberByName,
    d3d10_effect_scalar_variable_GetMemberBySemantic,
    d3d10_effect_scalar_variable_GetElement,
    d3d10_effect_scalar_variable_GetParentConstantBuffer,
    d3d10_effect_scalar_variable_AsScalar,
    d3d10_effect_scalar_variable_AsVector,
    d3d10_effect_scalar_variable_AsMatrix,
    d3d10_effect_scalar_variable_AsString,
    d3d10_effect_scalar_variable_AsShaderResource,
    d3d10_effect_scalar_variable_AsRenderTargetView,
    d3d10_effect_scalar_variable_AsDepthStencilView,
    d3d10_effect_scalar_variable_AsConstantBuffer,
    d3d10_effect_scalar_variable_AsShader,
    d3d10_effect_scalar_variable_AsBlend,
    d3d10_effect_scalar_variable_AsDepthStencil,
    d3d10_effect_scalar_variable_AsRasterizer,
    d3d10_effect_scalar_variable_AsSampler,
    d3d10_effect_scalar_variable_SetRawValue,
    d3d10_effect_scalar_variable_GetRawValue,
    /* ID3D10EffectScalarVariable methods */
    d3d10_effect_scalar_variable_SetFloat,
    d3d10_effect_scalar_variable_GetFloat,
    d3d10_effect_scalar_variable_SetFloatArray,
    d3d10_effect_scalar_variable_GetFloatArray,
    d3d10_effect_scalar_variable_SetInt,
    d3d10_effect_scalar_variable_GetInt,
    d3d10_effect_scalar_variable_SetIntArray,
    d3d10_effect_scalar_variable_GetIntArray,
    d3d10_effect_scalar_variable_SetBool,
    d3d10_effect_scalar_variable_GetBool,
    d3d10_effect_scalar_variable_SetBoolArray,
    d3d10_effect_scalar_variable_GetBoolArray,
};

/* ID3D10EffectVariable methods */

static BOOL STDMETHODCALLTYPE d3d10_effect_vector_variable_IsValid(ID3D10EffectVectorVariable *iface)
{
    TRACE("iface %p\n", iface);

    return (struct d3d10_effect_variable *)iface != &null_vector_variable;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_vector_variable_GetType(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_GetType((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_GetDesc(ID3D10EffectVectorVariable *iface,
        D3D10_EFFECT_VARIABLE_DESC *desc)
{
    return d3d10_effect_variable_GetDesc((ID3D10EffectVariable *)iface, desc);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_GetAnnotationByIndex(
        ID3D10EffectVectorVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetAnnotationByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_GetAnnotationByName(
        ID3D10EffectVectorVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetAnnotationByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_GetMemberByIndex(
        ID3D10EffectVectorVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetMemberByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_GetMemberByName(
        ID3D10EffectVectorVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetMemberByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_GetMemberBySemantic(
        ID3D10EffectVectorVariable *iface, LPCSTR semantic)
{
    return d3d10_effect_variable_GetMemberBySemantic((ID3D10EffectVariable *)iface, semantic);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_GetElement(
        ID3D10EffectVectorVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetElement((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_vector_variable_GetParentConstantBuffer(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_GetParentConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectScalarVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_AsScalar(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_AsScalar((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectVectorVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_AsVector(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_AsVector((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectMatrixVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_AsMatrix(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_AsMatrix((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectStringVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_AsString(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_AsString((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderResourceVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_AsShaderResource(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_AsShaderResource((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRenderTargetViewVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_AsRenderTargetView(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_AsRenderTargetView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilViewVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_AsDepthStencilView(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencilView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_vector_variable_AsConstantBuffer(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_AsConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_AsShader(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_AsShader((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectBlendVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_AsBlend(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_AsBlend((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_AsDepthStencil(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencil((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRasterizerVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_AsRasterizer(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_AsRasterizer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectSamplerVariable * STDMETHODCALLTYPE d3d10_effect_vector_variable_AsSampler(
        ID3D10EffectVectorVariable *iface)
{
    return d3d10_effect_variable_AsSampler((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_SetRawValue(ID3D10EffectVectorVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_SetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_GetRawValue(ID3D10EffectVectorVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_GetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

/* ID3D10EffectVectorVariable methods */

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_SetBoolVector(ID3D10EffectVectorVariable *iface,
        BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_SetIntVector(ID3D10EffectVectorVariable *iface,
        int *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_SetFloatVector(ID3D10EffectVectorVariable *iface,
        float *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_GetBoolVector(ID3D10EffectVectorVariable *iface,
        BOOL *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_GetIntVector(ID3D10EffectVectorVariable *iface,
        int *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_GetFloatVector(ID3D10EffectVectorVariable *iface,
        float *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_SetBoolVectorArray(ID3D10EffectVectorVariable *iface,
        BOOL *values, UINT offset, UINT count)
{
    FIXME("iface %p, values %p, offset %u, count %u stub!\n", iface, values, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_SetIntVectorArray(ID3D10EffectVectorVariable *iface,
        int *values, UINT offset, UINT count)
{
    FIXME("iface %p, values %p, offset %u, count %u stub!\n", iface, values, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_SetFloatVectorArray(ID3D10EffectVectorVariable *iface,
        float *values, UINT offset, UINT count)
{
    FIXME("iface %p, values %p, offset %u, count %u stub!\n", iface, values, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_GetBoolVectorArray(ID3D10EffectVectorVariable *iface,
        BOOL *values, UINT offset, UINT count)
{
    FIXME("iface %p, values %p, offset %u, count %u stub!\n", iface, values, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_GetIntVectorArray(ID3D10EffectVectorVariable *iface,
        int *values, UINT offset, UINT count)
{
    FIXME("iface %p, values %p, offset %u, count %u stub!\n", iface, values, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_vector_variable_GetFloatVectorArray(ID3D10EffectVectorVariable *iface,
        float *values, UINT offset, UINT count)
{
    FIXME("iface %p, values %p, offset %u, count %u stub!\n", iface, values, offset, count);

    return E_NOTIMPL;
}

static const struct ID3D10EffectVectorVariableVtbl d3d10_effect_vector_variable_vtbl =
{
    /* ID3D10EffectVariable methods */
    d3d10_effect_vector_variable_IsValid,
    d3d10_effect_vector_variable_GetType,
    d3d10_effect_vector_variable_GetDesc,
    d3d10_effect_vector_variable_GetAnnotationByIndex,
    d3d10_effect_vector_variable_GetAnnotationByName,
    d3d10_effect_vector_variable_GetMemberByIndex,
    d3d10_effect_vector_variable_GetMemberByName,
    d3d10_effect_vector_variable_GetMemberBySemantic,
    d3d10_effect_vector_variable_GetElement,
    d3d10_effect_vector_variable_GetParentConstantBuffer,
    d3d10_effect_vector_variable_AsScalar,
    d3d10_effect_vector_variable_AsVector,
    d3d10_effect_vector_variable_AsMatrix,
    d3d10_effect_vector_variable_AsString,
    d3d10_effect_vector_variable_AsShaderResource,
    d3d10_effect_vector_variable_AsRenderTargetView,
    d3d10_effect_vector_variable_AsDepthStencilView,
    d3d10_effect_vector_variable_AsConstantBuffer,
    d3d10_effect_vector_variable_AsShader,
    d3d10_effect_vector_variable_AsBlend,
    d3d10_effect_vector_variable_AsDepthStencil,
    d3d10_effect_vector_variable_AsRasterizer,
    d3d10_effect_vector_variable_AsSampler,
    d3d10_effect_vector_variable_SetRawValue,
    d3d10_effect_vector_variable_GetRawValue,
    /* ID3D10EffectVectorVariable methods */
    d3d10_effect_vector_variable_SetBoolVector,
    d3d10_effect_vector_variable_SetIntVector,
    d3d10_effect_vector_variable_SetFloatVector,
    d3d10_effect_vector_variable_GetBoolVector,
    d3d10_effect_vector_variable_GetIntVector,
    d3d10_effect_vector_variable_GetFloatVector,
    d3d10_effect_vector_variable_SetBoolVectorArray,
    d3d10_effect_vector_variable_SetIntVectorArray,
    d3d10_effect_vector_variable_SetFloatVectorArray,
    d3d10_effect_vector_variable_GetBoolVectorArray,
    d3d10_effect_vector_variable_GetIntVectorArray,
    d3d10_effect_vector_variable_GetFloatVectorArray,
};

/* ID3D10EffectVariable methods */

static BOOL STDMETHODCALLTYPE d3d10_effect_matrix_variable_IsValid(ID3D10EffectMatrixVariable *iface)
{
    TRACE("iface %p\n", iface);

    return (struct d3d10_effect_variable *)iface != &null_matrix_variable;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_matrix_variable_GetType(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_GetType((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_matrix_variable_GetDesc(ID3D10EffectMatrixVariable *iface,
        D3D10_EFFECT_VARIABLE_DESC *desc)
{
    return d3d10_effect_variable_GetDesc((ID3D10EffectVariable *)iface, desc);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_GetAnnotationByIndex(
        ID3D10EffectMatrixVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetAnnotationByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_GetAnnotationByName(
        ID3D10EffectMatrixVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetAnnotationByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_GetMemberByIndex(
        ID3D10EffectMatrixVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetMemberByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_GetMemberByName(
        ID3D10EffectMatrixVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetMemberByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_GetMemberBySemantic(
        ID3D10EffectMatrixVariable *iface, LPCSTR semantic)
{
    return d3d10_effect_variable_GetMemberBySemantic((ID3D10EffectVariable *)iface, semantic);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_GetElement(
        ID3D10EffectMatrixVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetElement((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_matrix_variable_GetParentConstantBuffer(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_GetParentConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectScalarVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_AsScalar(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_AsScalar((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectVectorVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_AsVector(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_AsVector((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectMatrixVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_AsMatrix(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_AsMatrix((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectStringVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_AsString(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_AsString((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderResourceVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_AsShaderResource(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_AsShaderResource((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRenderTargetViewVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_AsRenderTargetView(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_AsRenderTargetView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilViewVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_AsDepthStencilView(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencilView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_matrix_variable_AsConstantBuffer(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_AsConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_AsShader(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_AsShader((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectBlendVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_AsBlend(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_AsBlend((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_AsDepthStencil(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencil((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRasterizerVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_AsRasterizer(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_AsRasterizer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectSamplerVariable * STDMETHODCALLTYPE d3d10_effect_matrix_variable_AsSampler(
        ID3D10EffectMatrixVariable *iface)
{
    return d3d10_effect_variable_AsSampler((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_matrix_variable_SetRawValue(ID3D10EffectMatrixVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_SetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_matrix_variable_GetRawValue(ID3D10EffectMatrixVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_GetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

/* ID3D10EffectMatrixVariable methods */

static HRESULT STDMETHODCALLTYPE d3d10_effect_matrix_variable_SetMatrix(ID3D10EffectMatrixVariable *iface,
        float *data)
{
    FIXME("iface %p, data %p stub!\n", iface, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_matrix_variable_GetMatrix(ID3D10EffectMatrixVariable *iface,
        float *data)
{
    FIXME("iface %p, data %p stub!\n", iface, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_matrix_variable_SetMatrixArray(ID3D10EffectMatrixVariable *iface,
        float *data, UINT offset, UINT count)
{
    FIXME("iface %p, data %p, offset %u, count %u stub!\n", iface, data, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_matrix_variable_GetMatrixArray(ID3D10EffectMatrixVariable *iface,
        float *data, UINT offset, UINT count)
{
    FIXME("iface %p, data %p, offset %u, count %u stub!\n", iface, data, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_matrix_variable_SetMatrixTranspose(ID3D10EffectMatrixVariable *iface,
        float *data)
{
    FIXME("iface %p, data %p stub!\n", iface, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_matrix_variable_GetMatrixTranspose(ID3D10EffectMatrixVariable *iface,
        float *data)
{
    FIXME("iface %p, data %p stub!\n", iface, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_matrix_variable_SetMatrixTransposeArray(ID3D10EffectMatrixVariable *iface,
        float *data, UINT offset, UINT count)
{
    FIXME("iface %p, data %p, offset %u, count %u stub!\n", iface, data, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_matrix_variable_GetMatrixTransposeArray(ID3D10EffectMatrixVariable *iface,
        float *data, UINT offset, UINT count)
{
    FIXME("iface %p, data %p, offset %u, count %u stub!\n", iface, data, offset, count);

    return E_NOTIMPL;
}


static const struct ID3D10EffectMatrixVariableVtbl d3d10_effect_matrix_variable_vtbl =
{
    /* ID3D10EffectVariable methods */
    d3d10_effect_matrix_variable_IsValid,
    d3d10_effect_matrix_variable_GetType,
    d3d10_effect_matrix_variable_GetDesc,
    d3d10_effect_matrix_variable_GetAnnotationByIndex,
    d3d10_effect_matrix_variable_GetAnnotationByName,
    d3d10_effect_matrix_variable_GetMemberByIndex,
    d3d10_effect_matrix_variable_GetMemberByName,
    d3d10_effect_matrix_variable_GetMemberBySemantic,
    d3d10_effect_matrix_variable_GetElement,
    d3d10_effect_matrix_variable_GetParentConstantBuffer,
    d3d10_effect_matrix_variable_AsScalar,
    d3d10_effect_matrix_variable_AsVector,
    d3d10_effect_matrix_variable_AsMatrix,
    d3d10_effect_matrix_variable_AsString,
    d3d10_effect_matrix_variable_AsShaderResource,
    d3d10_effect_matrix_variable_AsRenderTargetView,
    d3d10_effect_matrix_variable_AsDepthStencilView,
    d3d10_effect_matrix_variable_AsConstantBuffer,
    d3d10_effect_matrix_variable_AsShader,
    d3d10_effect_matrix_variable_AsBlend,
    d3d10_effect_matrix_variable_AsDepthStencil,
    d3d10_effect_matrix_variable_AsRasterizer,
    d3d10_effect_matrix_variable_AsSampler,
    d3d10_effect_matrix_variable_SetRawValue,
    d3d10_effect_matrix_variable_GetRawValue,
    /* ID3D10EffectMatrixVariable methods */
    d3d10_effect_matrix_variable_SetMatrix,
    d3d10_effect_matrix_variable_GetMatrix,
    d3d10_effect_matrix_variable_SetMatrixArray,
    d3d10_effect_matrix_variable_GetMatrixArray,
    d3d10_effect_matrix_variable_SetMatrixTranspose,
    d3d10_effect_matrix_variable_GetMatrixTranspose,
    d3d10_effect_matrix_variable_SetMatrixTransposeArray,
    d3d10_effect_matrix_variable_GetMatrixTransposeArray,
};

/* ID3D10EffectVariable methods */

static BOOL STDMETHODCALLTYPE d3d10_effect_string_variable_IsValid(ID3D10EffectStringVariable *iface)
{
    TRACE("iface %p\n", iface);

    return (struct d3d10_effect_variable *)iface != &null_string_variable;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_string_variable_GetType(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_GetType((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_string_variable_GetDesc(ID3D10EffectStringVariable *iface,
        D3D10_EFFECT_VARIABLE_DESC *desc)
{
    return d3d10_effect_variable_GetDesc((ID3D10EffectVariable *)iface, desc);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_GetAnnotationByIndex(
        ID3D10EffectStringVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetAnnotationByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_GetAnnotationByName(
        ID3D10EffectStringVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetAnnotationByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_GetMemberByIndex(
        ID3D10EffectStringVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetMemberByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_GetMemberByName(
        ID3D10EffectStringVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetMemberByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_GetMemberBySemantic(
        ID3D10EffectStringVariable *iface, LPCSTR semantic)
{
    return d3d10_effect_variable_GetMemberBySemantic((ID3D10EffectVariable *)iface, semantic);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_GetElement(
        ID3D10EffectStringVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetElement((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_string_variable_GetParentConstantBuffer(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_GetParentConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectScalarVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_AsScalar(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_AsScalar((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectVectorVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_AsVector(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_AsVector((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectMatrixVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_AsMatrix(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_AsMatrix((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectStringVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_AsString(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_AsString((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderResourceVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_AsShaderResource(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_AsShaderResource((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRenderTargetViewVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_AsRenderTargetView(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_AsRenderTargetView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilViewVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_AsDepthStencilView(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencilView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_string_variable_AsConstantBuffer(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_AsConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_AsShader(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_AsShader((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectBlendVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_AsBlend(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_AsBlend((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_AsDepthStencil(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencil((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRasterizerVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_AsRasterizer(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_AsRasterizer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectSamplerVariable * STDMETHODCALLTYPE d3d10_effect_string_variable_AsSampler(
        ID3D10EffectStringVariable *iface)
{
    return d3d10_effect_variable_AsSampler((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_string_variable_SetRawValue(ID3D10EffectStringVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_SetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_string_variable_GetRawValue(ID3D10EffectStringVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_GetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

/* ID3D10EffectStringVariable methods */

static HRESULT STDMETHODCALLTYPE d3d10_effect_string_variable_GetString(ID3D10EffectStringVariable *iface,
        LPCSTR *str)
{
    FIXME("iface %p, str %p stub!\n", iface, str);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_string_variable_GetStringArray(ID3D10EffectStringVariable *iface,
        LPCSTR *strs, UINT offset, UINT count)
{
    FIXME("iface %p, strs %p, offset %u, count %u stub!\n", iface, strs, offset, count);

    return E_NOTIMPL;
}


static const struct ID3D10EffectStringVariableVtbl d3d10_effect_string_variable_vtbl =
{
    /* ID3D10EffectVariable methods */
    d3d10_effect_string_variable_IsValid,
    d3d10_effect_string_variable_GetType,
    d3d10_effect_string_variable_GetDesc,
    d3d10_effect_string_variable_GetAnnotationByIndex,
    d3d10_effect_string_variable_GetAnnotationByName,
    d3d10_effect_string_variable_GetMemberByIndex,
    d3d10_effect_string_variable_GetMemberByName,
    d3d10_effect_string_variable_GetMemberBySemantic,
    d3d10_effect_string_variable_GetElement,
    d3d10_effect_string_variable_GetParentConstantBuffer,
    d3d10_effect_string_variable_AsScalar,
    d3d10_effect_string_variable_AsVector,
    d3d10_effect_string_variable_AsMatrix,
    d3d10_effect_string_variable_AsString,
    d3d10_effect_string_variable_AsShaderResource,
    d3d10_effect_string_variable_AsRenderTargetView,
    d3d10_effect_string_variable_AsDepthStencilView,
    d3d10_effect_string_variable_AsConstantBuffer,
    d3d10_effect_string_variable_AsShader,
    d3d10_effect_string_variable_AsBlend,
    d3d10_effect_string_variable_AsDepthStencil,
    d3d10_effect_string_variable_AsRasterizer,
    d3d10_effect_string_variable_AsSampler,
    d3d10_effect_string_variable_SetRawValue,
    d3d10_effect_string_variable_GetRawValue,
    /* ID3D10EffectStringVariable methods */
    d3d10_effect_string_variable_GetString,
    d3d10_effect_string_variable_GetStringArray,
};

/* ID3D10EffectVariable methods */

static BOOL STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_IsValid(ID3D10EffectShaderResourceVariable *iface)
{
    TRACE("iface %p\n", iface);

    return (struct d3d10_effect_variable *)iface != &null_shader_resource_variable;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_GetType(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_GetType((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_GetDesc(
        ID3D10EffectShaderResourceVariable *iface, D3D10_EFFECT_VARIABLE_DESC *desc)
{
    return d3d10_effect_variable_GetDesc((ID3D10EffectVariable *)iface, desc);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_GetAnnotationByIndex(
        ID3D10EffectShaderResourceVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetAnnotationByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_GetAnnotationByName(
        ID3D10EffectShaderResourceVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetAnnotationByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_GetMemberByIndex(
        ID3D10EffectShaderResourceVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetMemberByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_GetMemberByName(
        ID3D10EffectShaderResourceVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetMemberByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_GetMemberBySemantic(
        ID3D10EffectShaderResourceVariable *iface, LPCSTR semantic)
{
    return d3d10_effect_variable_GetMemberBySemantic((ID3D10EffectVariable *)iface, semantic);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_GetElement(
        ID3D10EffectShaderResourceVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetElement((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_GetParentConstantBuffer(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_GetParentConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectScalarVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_AsScalar(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_AsScalar((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectVectorVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_AsVector(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_AsVector((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectMatrixVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_AsMatrix(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_AsMatrix((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectStringVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_AsString(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_AsString((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderResourceVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_AsShaderResource(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_AsShaderResource((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRenderTargetViewVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_AsRenderTargetView(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_AsRenderTargetView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilViewVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_AsDepthStencilView(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencilView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_AsConstantBuffer(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_AsConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_AsShader(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_AsShader((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectBlendVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_AsBlend(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_AsBlend((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_AsDepthStencil(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencil((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRasterizerVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_AsRasterizer(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_AsRasterizer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectSamplerVariable * STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_AsSampler(
        ID3D10EffectShaderResourceVariable *iface)
{
    return d3d10_effect_variable_AsSampler((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_SetRawValue(
        ID3D10EffectShaderResourceVariable *iface, void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_SetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_GetRawValue(
        ID3D10EffectShaderResourceVariable *iface, void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_GetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

/* ID3D10EffectShaderResourceVariable methods */

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_SetResource(
        ID3D10EffectShaderResourceVariable *iface, ID3D10ShaderResourceView *resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_GetResource(
        ID3D10EffectShaderResourceVariable *iface, ID3D10ShaderResourceView **resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_SetResourceArray(
        ID3D10EffectShaderResourceVariable *iface, ID3D10ShaderResourceView **resources, UINT offset, UINT count)
{
    FIXME("iface %p, resources %p, offset %u, count %u stub!\n", iface, resources, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_resource_variable_GetResourceArray(
        ID3D10EffectShaderResourceVariable *iface, ID3D10ShaderResourceView **resources, UINT offset, UINT count)
{
    FIXME("iface %p, resources %p, offset %u, count %u stub!\n", iface, resources, offset, count);

    return E_NOTIMPL;
}


static const struct ID3D10EffectShaderResourceVariableVtbl d3d10_effect_shader_resource_variable_vtbl =
{
    /* ID3D10EffectVariable methods */
    d3d10_effect_shader_resource_variable_IsValid,
    d3d10_effect_shader_resource_variable_GetType,
    d3d10_effect_shader_resource_variable_GetDesc,
    d3d10_effect_shader_resource_variable_GetAnnotationByIndex,
    d3d10_effect_shader_resource_variable_GetAnnotationByName,
    d3d10_effect_shader_resource_variable_GetMemberByIndex,
    d3d10_effect_shader_resource_variable_GetMemberByName,
    d3d10_effect_shader_resource_variable_GetMemberBySemantic,
    d3d10_effect_shader_resource_variable_GetElement,
    d3d10_effect_shader_resource_variable_GetParentConstantBuffer,
    d3d10_effect_shader_resource_variable_AsScalar,
    d3d10_effect_shader_resource_variable_AsVector,
    d3d10_effect_shader_resource_variable_AsMatrix,
    d3d10_effect_shader_resource_variable_AsString,
    d3d10_effect_shader_resource_variable_AsShaderResource,
    d3d10_effect_shader_resource_variable_AsRenderTargetView,
    d3d10_effect_shader_resource_variable_AsDepthStencilView,
    d3d10_effect_shader_resource_variable_AsConstantBuffer,
    d3d10_effect_shader_resource_variable_AsShader,
    d3d10_effect_shader_resource_variable_AsBlend,
    d3d10_effect_shader_resource_variable_AsDepthStencil,
    d3d10_effect_shader_resource_variable_AsRasterizer,
    d3d10_effect_shader_resource_variable_AsSampler,
    d3d10_effect_shader_resource_variable_SetRawValue,
    d3d10_effect_shader_resource_variable_GetRawValue,
    /* ID3D10EffectShaderResourceVariable methods */
    d3d10_effect_shader_resource_variable_SetResource,
    d3d10_effect_shader_resource_variable_GetResource,
    d3d10_effect_shader_resource_variable_SetResourceArray,
    d3d10_effect_shader_resource_variable_GetResourceArray,
};

/* ID3D10EffectVariable methods */

static BOOL STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_IsValid(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    TRACE("iface %p\n", iface);

    return (struct d3d10_effect_variable *)iface != &null_render_target_view_variable;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_GetType(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_GetType((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_GetDesc(
        ID3D10EffectRenderTargetViewVariable *iface, D3D10_EFFECT_VARIABLE_DESC *desc)
{
    return d3d10_effect_variable_GetDesc((ID3D10EffectVariable *)iface, desc);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_GetAnnotationByIndex(
        ID3D10EffectRenderTargetViewVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetAnnotationByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_GetAnnotationByName(
        ID3D10EffectRenderTargetViewVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetAnnotationByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_GetMemberByIndex(
        ID3D10EffectRenderTargetViewVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetMemberByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_GetMemberByName(
        ID3D10EffectRenderTargetViewVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetMemberByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_GetMemberBySemantic(
        ID3D10EffectRenderTargetViewVariable *iface, LPCSTR semantic)
{
    return d3d10_effect_variable_GetMemberBySemantic((ID3D10EffectVariable *)iface, semantic);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_GetElement(
        ID3D10EffectRenderTargetViewVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetElement((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_GetParentConstantBuffer(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_GetParentConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectScalarVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_AsScalar(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_AsScalar((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectVectorVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_AsVector(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_AsVector((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectMatrixVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_AsMatrix(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_AsMatrix((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectStringVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_AsString(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_AsString((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderResourceVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_AsShaderResource(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_AsShaderResource((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRenderTargetViewVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_AsRenderTargetView(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_AsRenderTargetView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilViewVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_AsDepthStencilView(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencilView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_AsConstantBuffer(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_AsConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_AsShader(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_AsShader((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectBlendVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_AsBlend(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_AsBlend((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_AsDepthStencil(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencil((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRasterizerVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_AsRasterizer(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_AsRasterizer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectSamplerVariable * STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_AsSampler(
        ID3D10EffectRenderTargetViewVariable *iface)
{
    return d3d10_effect_variable_AsSampler((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_SetRawValue(
        ID3D10EffectRenderTargetViewVariable *iface, void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_SetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_GetRawValue(
        ID3D10EffectRenderTargetViewVariable *iface, void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_GetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

/* ID3D10EffectRenderTargetViewVariable methods */

static HRESULT STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_SetRenderTarget(
        ID3D10EffectRenderTargetViewVariable *iface, ID3D10RenderTargetView *view)
{
    FIXME("iface %p, view %p stub!\n", iface, view);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_GetRenderTarget(
        ID3D10EffectRenderTargetViewVariable *iface, ID3D10RenderTargetView **view)
{
    FIXME("iface %p, view %p stub!\n", iface, view);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_SetRenderTargetArray(
        ID3D10EffectRenderTargetViewVariable *iface, ID3D10RenderTargetView **views, UINT offset, UINT count)
{
    FIXME("iface %p, views %p, offset %u, count %u stub!\n", iface, views, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_render_target_view_variable_GetRenderTargetArray(
        ID3D10EffectRenderTargetViewVariable *iface, ID3D10RenderTargetView **views, UINT offset, UINT count)
{
    FIXME("iface %p, views %p, offset %u, count %u stub!\n", iface, views, offset, count);

    return E_NOTIMPL;
}


static const struct ID3D10EffectRenderTargetViewVariableVtbl d3d10_effect_render_target_view_variable_vtbl =
{
    /* ID3D10EffectVariable methods */
    d3d10_effect_render_target_view_variable_IsValid,
    d3d10_effect_render_target_view_variable_GetType,
    d3d10_effect_render_target_view_variable_GetDesc,
    d3d10_effect_render_target_view_variable_GetAnnotationByIndex,
    d3d10_effect_render_target_view_variable_GetAnnotationByName,
    d3d10_effect_render_target_view_variable_GetMemberByIndex,
    d3d10_effect_render_target_view_variable_GetMemberByName,
    d3d10_effect_render_target_view_variable_GetMemberBySemantic,
    d3d10_effect_render_target_view_variable_GetElement,
    d3d10_effect_render_target_view_variable_GetParentConstantBuffer,
    d3d10_effect_render_target_view_variable_AsScalar,
    d3d10_effect_render_target_view_variable_AsVector,
    d3d10_effect_render_target_view_variable_AsMatrix,
    d3d10_effect_render_target_view_variable_AsString,
    d3d10_effect_render_target_view_variable_AsShaderResource,
    d3d10_effect_render_target_view_variable_AsRenderTargetView,
    d3d10_effect_render_target_view_variable_AsDepthStencilView,
    d3d10_effect_render_target_view_variable_AsConstantBuffer,
    d3d10_effect_render_target_view_variable_AsShader,
    d3d10_effect_render_target_view_variable_AsBlend,
    d3d10_effect_render_target_view_variable_AsDepthStencil,
    d3d10_effect_render_target_view_variable_AsRasterizer,
    d3d10_effect_render_target_view_variable_AsSampler,
    d3d10_effect_render_target_view_variable_SetRawValue,
    d3d10_effect_render_target_view_variable_GetRawValue,
    /* ID3D10EffectRenderTargetViewVariable methods */
    d3d10_effect_render_target_view_variable_SetRenderTarget,
    d3d10_effect_render_target_view_variable_GetRenderTarget,
    d3d10_effect_render_target_view_variable_SetRenderTargetArray,
    d3d10_effect_render_target_view_variable_GetRenderTargetArray,
};

/* ID3D10EffectVariable methods */

static BOOL STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_IsValid(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    TRACE("iface %p\n", iface);

    return (struct d3d10_effect_variable *)iface != &null_depth_stencil_view_variable;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_GetType(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_GetType((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_GetDesc(
        ID3D10EffectDepthStencilViewVariable *iface, D3D10_EFFECT_VARIABLE_DESC *desc)
{
    return d3d10_effect_variable_GetDesc((ID3D10EffectVariable *)iface, desc);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_GetAnnotationByIndex(
        ID3D10EffectDepthStencilViewVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetAnnotationByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_GetAnnotationByName(
        ID3D10EffectDepthStencilViewVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetAnnotationByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_GetMemberByIndex(
        ID3D10EffectDepthStencilViewVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetMemberByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_GetMemberByName(
        ID3D10EffectDepthStencilViewVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetMemberByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_GetMemberBySemantic(
        ID3D10EffectDepthStencilViewVariable *iface, LPCSTR semantic)
{
    return d3d10_effect_variable_GetMemberBySemantic((ID3D10EffectVariable *)iface, semantic);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_GetElement(
        ID3D10EffectDepthStencilViewVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetElement((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_GetParentConstantBuffer(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_GetParentConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectScalarVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_AsScalar(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_AsScalar((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectVectorVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_AsVector(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_AsVector((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectMatrixVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_AsMatrix(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_AsMatrix((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectStringVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_AsString(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_AsString((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderResourceVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_AsShaderResource(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_AsShaderResource((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRenderTargetViewVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_AsRenderTargetView(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_AsRenderTargetView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilViewVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_AsDepthStencilView(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencilView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_AsConstantBuffer(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_AsConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_AsShader(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_AsShader((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectBlendVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_AsBlend(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_AsBlend((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_AsDepthStencil(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencil((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRasterizerVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_AsRasterizer(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_AsRasterizer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectSamplerVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_AsSampler(
        ID3D10EffectDepthStencilViewVariable *iface)
{
    return d3d10_effect_variable_AsSampler((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_SetRawValue(
        ID3D10EffectDepthStencilViewVariable *iface, void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_SetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_GetRawValue(
        ID3D10EffectDepthStencilViewVariable *iface, void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_GetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

/* ID3D10EffectDepthStencilViewVariable methods */

static HRESULT STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_SetDepthStencil(
        ID3D10EffectDepthStencilViewVariable *iface, ID3D10DepthStencilView *view)
{
    FIXME("iface %p, view %p stub!\n", iface, view);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_GetDepthStencil(
        ID3D10EffectDepthStencilViewVariable *iface, ID3D10DepthStencilView **view)
{
    FIXME("iface %p, view %p stub!\n", iface, view);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_SetDepthStencilArray(
        ID3D10EffectDepthStencilViewVariable *iface, ID3D10DepthStencilView **views, UINT offset, UINT count)
{
    FIXME("iface %p, views %p, offset %u, count %u stub!\n", iface, views, offset, count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_depth_stencil_view_variable_GetDepthStencilArray(
        ID3D10EffectDepthStencilViewVariable *iface, ID3D10DepthStencilView **views, UINT offset, UINT count)
{
    FIXME("iface %p, views %p, offset %u, count %u stub!\n", iface, views, offset, count);

    return E_NOTIMPL;
}


static const struct ID3D10EffectDepthStencilViewVariableVtbl d3d10_effect_depth_stencil_view_variable_vtbl =
{
    /* ID3D10EffectVariable methods */
    d3d10_effect_depth_stencil_view_variable_IsValid,
    d3d10_effect_depth_stencil_view_variable_GetType,
    d3d10_effect_depth_stencil_view_variable_GetDesc,
    d3d10_effect_depth_stencil_view_variable_GetAnnotationByIndex,
    d3d10_effect_depth_stencil_view_variable_GetAnnotationByName,
    d3d10_effect_depth_stencil_view_variable_GetMemberByIndex,
    d3d10_effect_depth_stencil_view_variable_GetMemberByName,
    d3d10_effect_depth_stencil_view_variable_GetMemberBySemantic,
    d3d10_effect_depth_stencil_view_variable_GetElement,
    d3d10_effect_depth_stencil_view_variable_GetParentConstantBuffer,
    d3d10_effect_depth_stencil_view_variable_AsScalar,
    d3d10_effect_depth_stencil_view_variable_AsVector,
    d3d10_effect_depth_stencil_view_variable_AsMatrix,
    d3d10_effect_depth_stencil_view_variable_AsString,
    d3d10_effect_depth_stencil_view_variable_AsShaderResource,
    d3d10_effect_depth_stencil_view_variable_AsRenderTargetView,
    d3d10_effect_depth_stencil_view_variable_AsDepthStencilView,
    d3d10_effect_depth_stencil_view_variable_AsConstantBuffer,
    d3d10_effect_depth_stencil_view_variable_AsShader,
    d3d10_effect_depth_stencil_view_variable_AsBlend,
    d3d10_effect_depth_stencil_view_variable_AsDepthStencil,
    d3d10_effect_depth_stencil_view_variable_AsRasterizer,
    d3d10_effect_depth_stencil_view_variable_AsSampler,
    d3d10_effect_depth_stencil_view_variable_SetRawValue,
    d3d10_effect_depth_stencil_view_variable_GetRawValue,
    /* ID3D10EffectDepthStencilViewVariable methods */
    d3d10_effect_depth_stencil_view_variable_SetDepthStencil,
    d3d10_effect_depth_stencil_view_variable_GetDepthStencil,
    d3d10_effect_depth_stencil_view_variable_SetDepthStencilArray,
    d3d10_effect_depth_stencil_view_variable_GetDepthStencilArray,
};

/* ID3D10EffectVariable methods */

static BOOL STDMETHODCALLTYPE d3d10_effect_shader_variable_IsValid(ID3D10EffectShaderVariable *iface)
{
    TRACE("iface %p\n", iface);

    return (struct d3d10_effect_variable *)iface != &null_shader_variable;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_shader_variable_GetType(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_GetType((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_variable_GetDesc(ID3D10EffectShaderVariable *iface,
        D3D10_EFFECT_VARIABLE_DESC *desc)
{
    return d3d10_effect_variable_GetDesc((ID3D10EffectVariable *)iface, desc);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_GetAnnotationByIndex(
        ID3D10EffectShaderVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetAnnotationByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_GetAnnotationByName(
        ID3D10EffectShaderVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetAnnotationByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_GetMemberByIndex(
        ID3D10EffectShaderVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetMemberByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_GetMemberByName(
        ID3D10EffectShaderVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetMemberByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_GetMemberBySemantic(
        ID3D10EffectShaderVariable *iface, LPCSTR semantic)
{
    return d3d10_effect_variable_GetMemberBySemantic((ID3D10EffectVariable *)iface, semantic);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_GetElement(
        ID3D10EffectShaderVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetElement((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_shader_variable_GetParentConstantBuffer(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_GetParentConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectScalarVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_AsScalar(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_AsScalar((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectVectorVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_AsVector(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_AsVector((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectMatrixVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_AsMatrix(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_AsMatrix((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectStringVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_AsString(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_AsString((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderResourceVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_AsShaderResource(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_AsShaderResource((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRenderTargetViewVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_AsRenderTargetView(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_AsRenderTargetView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilViewVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_AsDepthStencilView(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencilView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_shader_variable_AsConstantBuffer(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_AsConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_AsShader(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_AsShader((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectBlendVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_AsBlend(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_AsBlend((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_AsDepthStencil(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencil((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRasterizerVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_AsRasterizer(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_AsRasterizer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectSamplerVariable * STDMETHODCALLTYPE d3d10_effect_shader_variable_AsSampler(
        ID3D10EffectShaderVariable *iface)
{
    return d3d10_effect_variable_AsSampler((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_variable_SetRawValue(
        ID3D10EffectShaderVariable *iface, void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_SetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_variable_GetRawValue(
        ID3D10EffectShaderVariable *iface, void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_GetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

/* ID3D10EffectShaderVariable methods */

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_variable_GetShaderDesc(
        ID3D10EffectShaderVariable *iface, UINT index, D3D10_EFFECT_SHADER_DESC *desc)
{
    FIXME("iface %p, index %u, desc %p stub!\n", iface, index, desc);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_variable_GetVertexShader(
        ID3D10EffectShaderVariable *iface, UINT index, ID3D10VertexShader **shader)
{
    FIXME("iface %p, index %u, shader %p stub!\n", iface, index, shader);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_variable_GetGeometryShader(
        ID3D10EffectShaderVariable *iface, UINT index, ID3D10GeometryShader **shader)
{
    FIXME("iface %p, index %u, shader %p stub!\n", iface, index, shader);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_variable_GetPixelShader(
        ID3D10EffectShaderVariable *iface, UINT index, ID3D10PixelShader **shader)
{
    FIXME("iface %p, index %u, shader %p stub!\n", iface, index, shader);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_variable_GetInputSignatureElementDesc(
        ID3D10EffectShaderVariable *iface, UINT shader_index, UINT element_index,
        D3D10_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3d10_effect_variable *This = (struct d3d10_effect_variable *)iface;
    struct d3d10_effect_shader_variable *s;
    D3D10_SIGNATURE_PARAMETER_DESC *d;

    TRACE("iface %p, shader_index %u, element_index %u, desc %p\n",
            iface, shader_index, element_index, desc);

    if (!iface->lpVtbl->IsValid(iface))
    {
        WARN("Null variable specified\n");
        return E_FAIL;
    }

    /* Check shader_index, this crashes on W7/DX10 */
    if (shader_index >= This->effect->used_shader_count)
    {
        WARN("This should crash on W7/DX10!\n");
        return E_FAIL;
    }

    s = This->effect->used_shaders[shader_index]->data;
    if (!s->input_signature.signature)
    {
        WARN("No shader signature\n");
        return D3DERR_INVALIDCALL;
    }

    /* Check desc for NULL, this crashes on W7/DX10 */
    if (!desc)
    {
        WARN("This should crash on W7/DX10!\n");
        return E_FAIL;
    }

    if (element_index >= s->input_signature.element_count)
    {
        WARN("Invalid element index specified\n");
        return E_INVALIDARG;
    }

    d = &s->input_signature.elements[element_index];
    desc->SemanticName = d->SemanticName;
    desc->SemanticIndex  =  d->SemanticIndex;
    desc->SystemValueType =  d->SystemValueType;
    desc->ComponentType =  d->ComponentType;
    desc->Register =  d->Register;
    desc->ReadWriteMask  =  d->ReadWriteMask;
    desc->Mask =  d->Mask;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_shader_variable_GetOutputSignatureElementDesc(
        ID3D10EffectShaderVariable *iface, UINT shader_index, UINT element_index,
        D3D10_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3d10_effect_variable *This = (struct d3d10_effect_variable *)iface;
    struct d3d10_effect_shader_variable *s;
    D3D10_SIGNATURE_PARAMETER_DESC *d;

    TRACE("iface %p, shader_index %u, element_index %u, desc %p\n",
            iface, shader_index, element_index, desc);

    if (!iface->lpVtbl->IsValid(iface))
    {
        WARN("Null variable specified\n");
        return E_FAIL;
    }

    /* Check shader_index, this crashes on W7/DX10 */
    if (shader_index >= This->effect->used_shader_count)
    {
        WARN("This should crash on W7/DX10!\n");
        return E_FAIL;
    }

    s = This->effect->used_shaders[shader_index]->data;
    if (!s->output_signature.signature)
    {
        WARN("No shader signature\n");
        return D3DERR_INVALIDCALL;
    }

    /* Check desc for NULL, this crashes on W7/DX10 */
    if (!desc)
    {
        WARN("This should crash on W7/DX10!\n");
        return E_FAIL;
    }

    if (element_index >= s->output_signature.element_count)
    {
        WARN("Invalid element index specified\n");
        return E_INVALIDARG;
    }

    d = &s->output_signature.elements[element_index];
    desc->SemanticName = d->SemanticName;
    desc->SemanticIndex  =  d->SemanticIndex;
    desc->SystemValueType =  d->SystemValueType;
    desc->ComponentType =  d->ComponentType;
    desc->Register =  d->Register;
    desc->ReadWriteMask  =  d->ReadWriteMask;
    desc->Mask =  d->Mask;

    return S_OK;
}


static const struct ID3D10EffectShaderVariableVtbl d3d10_effect_shader_variable_vtbl =
{
    /* ID3D10EffectVariable methods */
    d3d10_effect_shader_variable_IsValid,
    d3d10_effect_shader_variable_GetType,
    d3d10_effect_shader_variable_GetDesc,
    d3d10_effect_shader_variable_GetAnnotationByIndex,
    d3d10_effect_shader_variable_GetAnnotationByName,
    d3d10_effect_shader_variable_GetMemberByIndex,
    d3d10_effect_shader_variable_GetMemberByName,
    d3d10_effect_shader_variable_GetMemberBySemantic,
    d3d10_effect_shader_variable_GetElement,
    d3d10_effect_shader_variable_GetParentConstantBuffer,
    d3d10_effect_shader_variable_AsScalar,
    d3d10_effect_shader_variable_AsVector,
    d3d10_effect_shader_variable_AsMatrix,
    d3d10_effect_shader_variable_AsString,
    d3d10_effect_shader_variable_AsShaderResource,
    d3d10_effect_shader_variable_AsRenderTargetView,
    d3d10_effect_shader_variable_AsDepthStencilView,
    d3d10_effect_shader_variable_AsConstantBuffer,
    d3d10_effect_shader_variable_AsShader,
    d3d10_effect_shader_variable_AsBlend,
    d3d10_effect_shader_variable_AsDepthStencil,
    d3d10_effect_shader_variable_AsRasterizer,
    d3d10_effect_shader_variable_AsSampler,
    d3d10_effect_shader_variable_SetRawValue,
    d3d10_effect_shader_variable_GetRawValue,
    /* ID3D10EffectShaderVariable methods */
    d3d10_effect_shader_variable_GetShaderDesc,
    d3d10_effect_shader_variable_GetVertexShader,
    d3d10_effect_shader_variable_GetGeometryShader,
    d3d10_effect_shader_variable_GetPixelShader,
    d3d10_effect_shader_variable_GetInputSignatureElementDesc,
    d3d10_effect_shader_variable_GetOutputSignatureElementDesc,
};

/* ID3D10EffectVariable methods */

static BOOL STDMETHODCALLTYPE d3d10_effect_blend_variable_IsValid(ID3D10EffectBlendVariable *iface)
{
    TRACE("iface %p\n", iface);

    return (struct d3d10_effect_variable *)iface != &null_blend_variable;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_blend_variable_GetType(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_GetType((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_blend_variable_GetDesc(ID3D10EffectBlendVariable *iface,
        D3D10_EFFECT_VARIABLE_DESC *desc)
{
    return d3d10_effect_variable_GetDesc((ID3D10EffectVariable *)iface, desc);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_GetAnnotationByIndex(
        ID3D10EffectBlendVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetAnnotationByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_GetAnnotationByName(
        ID3D10EffectBlendVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetAnnotationByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_GetMemberByIndex(
        ID3D10EffectBlendVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetMemberByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_GetMemberByName(
        ID3D10EffectBlendVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetMemberByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_GetMemberBySemantic(
        ID3D10EffectBlendVariable *iface, LPCSTR semantic)
{
    return d3d10_effect_variable_GetMemberBySemantic((ID3D10EffectVariable *)iface, semantic);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_GetElement(
        ID3D10EffectBlendVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetElement((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_blend_variable_GetParentConstantBuffer(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_GetParentConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectScalarVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_AsScalar(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_AsScalar((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectVectorVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_AsVector(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_AsVector((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectMatrixVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_AsMatrix(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_AsMatrix((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectStringVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_AsString(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_AsString((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderResourceVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_AsShaderResource(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_AsShaderResource((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRenderTargetViewVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_AsRenderTargetView(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_AsRenderTargetView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilViewVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_AsDepthStencilView(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencilView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_blend_variable_AsConstantBuffer(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_AsConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_AsShader(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_AsShader((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectBlendVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_AsBlend(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_AsBlend((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_AsDepthStencil(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencil((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRasterizerVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_AsRasterizer(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_AsRasterizer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectSamplerVariable * STDMETHODCALLTYPE d3d10_effect_blend_variable_AsSampler(
        ID3D10EffectBlendVariable *iface)
{
    return d3d10_effect_variable_AsSampler((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_blend_variable_SetRawValue(ID3D10EffectBlendVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_SetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_blend_variable_GetRawValue(ID3D10EffectBlendVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_GetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

/* ID3D10EffectBlendVariable methods */

static HRESULT STDMETHODCALLTYPE d3d10_effect_blend_variable_GetBlendState(ID3D10EffectBlendVariable *iface,
        UINT index, ID3D10BlendState **blend_state)
{
    FIXME("iface %p, index %u, blend_state %p stub!\n", iface, index, blend_state);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_blend_variable_GetBackingStore(ID3D10EffectBlendVariable *iface,
        UINT index, D3D10_BLEND_DESC *desc)
{
    struct d3d10_effect_variable *v = impl_from_ID3D10EffectVariable((ID3D10EffectVariable *)iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    if (index >= max(v->type->element_count, 1))
    {
        WARN("Invalid index %u.\n", index);
        return E_FAIL;
    }

    *desc = ((D3D10_BLEND_DESC *)v->data)[index];

    return S_OK;
}


static const struct ID3D10EffectBlendVariableVtbl d3d10_effect_blend_variable_vtbl =
{
    /* ID3D10EffectVariable methods */
    d3d10_effect_blend_variable_IsValid,
    d3d10_effect_blend_variable_GetType,
    d3d10_effect_blend_variable_GetDesc,
    d3d10_effect_blend_variable_GetAnnotationByIndex,
    d3d10_effect_blend_variable_GetAnnotationByName,
    d3d10_effect_blend_variable_GetMemberByIndex,
    d3d10_effect_blend_variable_GetMemberByName,
    d3d10_effect_blend_variable_GetMemberBySemantic,
    d3d10_effect_blend_variable_GetElement,
    d3d10_effect_blend_variable_GetParentConstantBuffer,
    d3d10_effect_blend_variable_AsScalar,
    d3d10_effect_blend_variable_AsVector,
    d3d10_effect_blend_variable_AsMatrix,
    d3d10_effect_blend_variable_AsString,
    d3d10_effect_blend_variable_AsShaderResource,
    d3d10_effect_blend_variable_AsRenderTargetView,
    d3d10_effect_blend_variable_AsDepthStencilView,
    d3d10_effect_blend_variable_AsConstantBuffer,
    d3d10_effect_blend_variable_AsShader,
    d3d10_effect_blend_variable_AsBlend,
    d3d10_effect_blend_variable_AsDepthStencil,
    d3d10_effect_blend_variable_AsRasterizer,
    d3d10_effect_blend_variable_AsSampler,
    d3d10_effect_blend_variable_SetRawValue,
    d3d10_effect_blend_variable_GetRawValue,
    /* ID3D10EffectBlendVariable methods */
    d3d10_effect_blend_variable_GetBlendState,
    d3d10_effect_blend_variable_GetBackingStore,
};

/* ID3D10EffectVariable methods */

static BOOL STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_IsValid(ID3D10EffectDepthStencilVariable *iface)
{
    TRACE("iface %p\n", iface);

    return (struct d3d10_effect_variable *)iface != &null_depth_stencil_variable;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_GetType(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_GetType((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_GetDesc(ID3D10EffectDepthStencilVariable *iface,
        D3D10_EFFECT_VARIABLE_DESC *desc)
{
    return d3d10_effect_variable_GetDesc((ID3D10EffectVariable *)iface, desc);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_GetAnnotationByIndex(
        ID3D10EffectDepthStencilVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetAnnotationByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_GetAnnotationByName(
        ID3D10EffectDepthStencilVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetAnnotationByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_GetMemberByIndex(
        ID3D10EffectDepthStencilVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetMemberByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_GetMemberByName(
        ID3D10EffectDepthStencilVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetMemberByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_GetMemberBySemantic(
        ID3D10EffectDepthStencilVariable *iface, LPCSTR semantic)
{
    return d3d10_effect_variable_GetMemberBySemantic((ID3D10EffectVariable *)iface, semantic);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_GetElement(
        ID3D10EffectDepthStencilVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetElement((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_GetParentConstantBuffer(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_GetParentConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectScalarVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_AsScalar(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_AsScalar((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectVectorVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_AsVector(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_AsVector((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectMatrixVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_AsMatrix(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_AsMatrix((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectStringVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_AsString(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_AsString((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderResourceVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_AsShaderResource(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_AsShaderResource((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRenderTargetViewVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_AsRenderTargetView(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_AsRenderTargetView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilViewVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_AsDepthStencilView(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencilView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_AsConstantBuffer(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_AsConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_AsShader(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_AsShader((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectBlendVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_AsBlend(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_AsBlend((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_AsDepthStencil(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencil((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRasterizerVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_AsRasterizer(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_AsRasterizer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectSamplerVariable * STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_AsSampler(
        ID3D10EffectDepthStencilVariable *iface)
{
    return d3d10_effect_variable_AsSampler((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_SetRawValue(ID3D10EffectDepthStencilVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_SetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_GetRawValue(ID3D10EffectDepthStencilVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_GetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

/* ID3D10EffectDepthStencilVariable methods */

static HRESULT STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_GetDepthStencilState(ID3D10EffectDepthStencilVariable *iface,
        UINT index, ID3D10DepthStencilState **depth_stencil_state)
{
    FIXME("iface %p, index %u, depth_stencil_state %p stub!\n", iface, index, depth_stencil_state);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_depth_stencil_variable_GetBackingStore(ID3D10EffectDepthStencilVariable *iface,
        UINT index, D3D10_DEPTH_STENCIL_DESC *desc)
{
    struct d3d10_effect_variable *v = impl_from_ID3D10EffectVariable((ID3D10EffectVariable *)iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    if (index >= max(v->type->element_count, 1))
    {
        WARN("Invalid index %u.\n", index);
        return E_FAIL;
    }

    *desc = ((D3D10_DEPTH_STENCIL_DESC *)v->data)[index];

    return S_OK;
}


static const struct ID3D10EffectDepthStencilVariableVtbl d3d10_effect_depth_stencil_variable_vtbl =
{
    /* ID3D10EffectVariable methods */
    d3d10_effect_depth_stencil_variable_IsValid,
    d3d10_effect_depth_stencil_variable_GetType,
    d3d10_effect_depth_stencil_variable_GetDesc,
    d3d10_effect_depth_stencil_variable_GetAnnotationByIndex,
    d3d10_effect_depth_stencil_variable_GetAnnotationByName,
    d3d10_effect_depth_stencil_variable_GetMemberByIndex,
    d3d10_effect_depth_stencil_variable_GetMemberByName,
    d3d10_effect_depth_stencil_variable_GetMemberBySemantic,
    d3d10_effect_depth_stencil_variable_GetElement,
    d3d10_effect_depth_stencil_variable_GetParentConstantBuffer,
    d3d10_effect_depth_stencil_variable_AsScalar,
    d3d10_effect_depth_stencil_variable_AsVector,
    d3d10_effect_depth_stencil_variable_AsMatrix,
    d3d10_effect_depth_stencil_variable_AsString,
    d3d10_effect_depth_stencil_variable_AsShaderResource,
    d3d10_effect_depth_stencil_variable_AsRenderTargetView,
    d3d10_effect_depth_stencil_variable_AsDepthStencilView,
    d3d10_effect_depth_stencil_variable_AsConstantBuffer,
    d3d10_effect_depth_stencil_variable_AsShader,
    d3d10_effect_depth_stencil_variable_AsBlend,
    d3d10_effect_depth_stencil_variable_AsDepthStencil,
    d3d10_effect_depth_stencil_variable_AsRasterizer,
    d3d10_effect_depth_stencil_variable_AsSampler,
    d3d10_effect_depth_stencil_variable_SetRawValue,
    d3d10_effect_depth_stencil_variable_GetRawValue,
    /* ID3D10EffectDepthStencilVariable methods */
    d3d10_effect_depth_stencil_variable_GetDepthStencilState,
    d3d10_effect_depth_stencil_variable_GetBackingStore,
};

/* ID3D10EffectVariable methods */

static BOOL STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_IsValid(ID3D10EffectRasterizerVariable *iface)
{
    TRACE("iface %p\n", iface);

    return (struct d3d10_effect_variable *)iface != &null_rasterizer_variable;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_GetType(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_GetType((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_GetDesc(ID3D10EffectRasterizerVariable *iface,
        D3D10_EFFECT_VARIABLE_DESC *desc)
{
    return d3d10_effect_variable_GetDesc((ID3D10EffectVariable *)iface, desc);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_GetAnnotationByIndex(
        ID3D10EffectRasterizerVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetAnnotationByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_GetAnnotationByName(
        ID3D10EffectRasterizerVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetAnnotationByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_GetMemberByIndex(
        ID3D10EffectRasterizerVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetMemberByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_GetMemberByName(
        ID3D10EffectRasterizerVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetMemberByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_GetMemberBySemantic(
        ID3D10EffectRasterizerVariable *iface, LPCSTR semantic)
{
    return d3d10_effect_variable_GetMemberBySemantic((ID3D10EffectVariable *)iface, semantic);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_GetElement(
        ID3D10EffectRasterizerVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetElement((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_GetParentConstantBuffer(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_GetParentConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectScalarVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_AsScalar(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_AsScalar((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectVectorVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_AsVector(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_AsVector((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectMatrixVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_AsMatrix(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_AsMatrix((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectStringVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_AsString(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_AsString((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderResourceVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_AsShaderResource(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_AsShaderResource((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRenderTargetViewVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_AsRenderTargetView(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_AsRenderTargetView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilViewVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_AsDepthStencilView(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencilView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_AsConstantBuffer(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_AsConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_AsShader(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_AsShader((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectBlendVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_AsBlend(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_AsBlend((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_AsDepthStencil(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencil((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRasterizerVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_AsRasterizer(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_AsRasterizer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectSamplerVariable * STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_AsSampler(
        ID3D10EffectRasterizerVariable *iface)
{
    return d3d10_effect_variable_AsSampler((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_SetRawValue(ID3D10EffectRasterizerVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_SetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_GetRawValue(ID3D10EffectRasterizerVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_GetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

/* ID3D10EffectRasterizerVariable methods */

static HRESULT STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_GetRasterizerState(ID3D10EffectRasterizerVariable *iface,
        UINT index, ID3D10RasterizerState **rasterizer_state)
{
    FIXME("iface %p, index %u, rasterizer_state %p stub!\n", iface, index, rasterizer_state);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_rasterizer_variable_GetBackingStore(ID3D10EffectRasterizerVariable *iface,
        UINT index, D3D10_RASTERIZER_DESC *desc)
{
    struct d3d10_effect_variable *v = impl_from_ID3D10EffectVariable((ID3D10EffectVariable *)iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    if (index >= max(v->type->element_count, 1))
    {
        WARN("Invalid index %u.\n", index);
        return E_FAIL;
    }

    *desc = ((D3D10_RASTERIZER_DESC *)v->data)[index];

    return S_OK;
}


static const struct ID3D10EffectRasterizerVariableVtbl d3d10_effect_rasterizer_variable_vtbl =
{
    /* ID3D10EffectVariable methods */
    d3d10_effect_rasterizer_variable_IsValid,
    d3d10_effect_rasterizer_variable_GetType,
    d3d10_effect_rasterizer_variable_GetDesc,
    d3d10_effect_rasterizer_variable_GetAnnotationByIndex,
    d3d10_effect_rasterizer_variable_GetAnnotationByName,
    d3d10_effect_rasterizer_variable_GetMemberByIndex,
    d3d10_effect_rasterizer_variable_GetMemberByName,
    d3d10_effect_rasterizer_variable_GetMemberBySemantic,
    d3d10_effect_rasterizer_variable_GetElement,
    d3d10_effect_rasterizer_variable_GetParentConstantBuffer,
    d3d10_effect_rasterizer_variable_AsScalar,
    d3d10_effect_rasterizer_variable_AsVector,
    d3d10_effect_rasterizer_variable_AsMatrix,
    d3d10_effect_rasterizer_variable_AsString,
    d3d10_effect_rasterizer_variable_AsShaderResource,
    d3d10_effect_rasterizer_variable_AsRenderTargetView,
    d3d10_effect_rasterizer_variable_AsDepthStencilView,
    d3d10_effect_rasterizer_variable_AsConstantBuffer,
    d3d10_effect_rasterizer_variable_AsShader,
    d3d10_effect_rasterizer_variable_AsBlend,
    d3d10_effect_rasterizer_variable_AsDepthStencil,
    d3d10_effect_rasterizer_variable_AsRasterizer,
    d3d10_effect_rasterizer_variable_AsSampler,
    d3d10_effect_rasterizer_variable_SetRawValue,
    d3d10_effect_rasterizer_variable_GetRawValue,
    /* ID3D10EffectRasterizerVariable methods */
    d3d10_effect_rasterizer_variable_GetRasterizerState,
    d3d10_effect_rasterizer_variable_GetBackingStore,
};

/* ID3D10EffectVariable methods */

static BOOL STDMETHODCALLTYPE d3d10_effect_sampler_variable_IsValid(ID3D10EffectSamplerVariable *iface)
{
    TRACE("iface %p\n", iface);

    return (struct d3d10_effect_variable *)iface != &null_sampler_variable;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_sampler_variable_GetType(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_GetType((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_sampler_variable_GetDesc(ID3D10EffectSamplerVariable *iface,
        D3D10_EFFECT_VARIABLE_DESC *desc)
{
    return d3d10_effect_variable_GetDesc((ID3D10EffectVariable *)iface, desc);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_GetAnnotationByIndex(
        ID3D10EffectSamplerVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetAnnotationByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_GetAnnotationByName(
        ID3D10EffectSamplerVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetAnnotationByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_GetMemberByIndex(
        ID3D10EffectSamplerVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetMemberByIndex((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_GetMemberByName(
        ID3D10EffectSamplerVariable *iface, LPCSTR name)
{
    return d3d10_effect_variable_GetMemberByName((ID3D10EffectVariable *)iface, name);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_GetMemberBySemantic(
        ID3D10EffectSamplerVariable *iface, LPCSTR semantic)
{
    return d3d10_effect_variable_GetMemberBySemantic((ID3D10EffectVariable *)iface, semantic);
}

static struct ID3D10EffectVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_GetElement(
        ID3D10EffectSamplerVariable *iface, UINT index)
{
    return d3d10_effect_variable_GetElement((ID3D10EffectVariable *)iface, index);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_sampler_variable_GetParentConstantBuffer(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_GetParentConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectScalarVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_AsScalar(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_AsScalar((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectVectorVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_AsVector(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_AsVector((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectMatrixVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_AsMatrix(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_AsMatrix((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectStringVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_AsString(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_AsString((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderResourceVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_AsShaderResource(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_AsShaderResource((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRenderTargetViewVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_AsRenderTargetView(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_AsRenderTargetView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilViewVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_AsDepthStencilView(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencilView((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectConstantBuffer * STDMETHODCALLTYPE d3d10_effect_sampler_variable_AsConstantBuffer(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_AsConstantBuffer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectShaderVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_AsShader(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_AsShader((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectBlendVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_AsBlend(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_AsBlend((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectDepthStencilVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_AsDepthStencil(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_AsDepthStencil((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectRasterizerVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_AsRasterizer(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_AsRasterizer((ID3D10EffectVariable *)iface);
}

static struct ID3D10EffectSamplerVariable * STDMETHODCALLTYPE d3d10_effect_sampler_variable_AsSampler(
        ID3D10EffectSamplerVariable *iface)
{
    return d3d10_effect_variable_AsSampler((ID3D10EffectVariable *)iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_sampler_variable_SetRawValue(ID3D10EffectSamplerVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_SetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_sampler_variable_GetRawValue(ID3D10EffectSamplerVariable *iface,
        void *data, UINT offset, UINT count)
{
    return d3d10_effect_variable_GetRawValue((ID3D10EffectVariable *)iface, data, offset, count);
}

/* ID3D10EffectSamplerVariable methods */

static HRESULT STDMETHODCALLTYPE d3d10_effect_sampler_variable_GetSampler(ID3D10EffectSamplerVariable *iface,
        UINT index, ID3D10SamplerState **sampler)
{
    FIXME("iface %p, index %u, sampler %p stub!\n", iface, index, sampler);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_sampler_variable_GetBackingStore(ID3D10EffectSamplerVariable *iface,
        UINT index, D3D10_SAMPLER_DESC *desc)
{
    struct d3d10_effect_variable *v = impl_from_ID3D10EffectVariable((ID3D10EffectVariable *)iface);

    TRACE("iface %p, index %u, desc %p.\n", iface, index, desc);

    if (index >= max(v->type->element_count, 1))
    {
        WARN("Invalid index %u.\n", index);
        return E_FAIL;
    }

    *desc = ((D3D10_SAMPLER_DESC *)v->data)[index];

    return S_OK;
}


static const struct ID3D10EffectSamplerVariableVtbl d3d10_effect_sampler_variable_vtbl =
{
    /* ID3D10EffectVariable methods */
    d3d10_effect_sampler_variable_IsValid,
    d3d10_effect_sampler_variable_GetType,
    d3d10_effect_sampler_variable_GetDesc,
    d3d10_effect_sampler_variable_GetAnnotationByIndex,
    d3d10_effect_sampler_variable_GetAnnotationByName,
    d3d10_effect_sampler_variable_GetMemberByIndex,
    d3d10_effect_sampler_variable_GetMemberByName,
    d3d10_effect_sampler_variable_GetMemberBySemantic,
    d3d10_effect_sampler_variable_GetElement,
    d3d10_effect_sampler_variable_GetParentConstantBuffer,
    d3d10_effect_sampler_variable_AsScalar,
    d3d10_effect_sampler_variable_AsVector,
    d3d10_effect_sampler_variable_AsMatrix,
    d3d10_effect_sampler_variable_AsString,
    d3d10_effect_sampler_variable_AsShaderResource,
    d3d10_effect_sampler_variable_AsRenderTargetView,
    d3d10_effect_sampler_variable_AsDepthStencilView,
    d3d10_effect_sampler_variable_AsConstantBuffer,
    d3d10_effect_sampler_variable_AsShader,
    d3d10_effect_sampler_variable_AsBlend,
    d3d10_effect_sampler_variable_AsDepthStencil,
    d3d10_effect_sampler_variable_AsRasterizer,
    d3d10_effect_sampler_variable_AsSampler,
    d3d10_effect_sampler_variable_SetRawValue,
    d3d10_effect_sampler_variable_GetRawValue,
    /* ID3D10EffectSamplerVariable methods */
    d3d10_effect_sampler_variable_GetSampler,
    d3d10_effect_sampler_variable_GetBackingStore,
};

/* ID3D10EffectType methods */

static inline struct d3d10_effect_type *impl_from_ID3D10EffectType(ID3D10EffectType *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_effect_type, ID3D10EffectType_iface);
}

static BOOL STDMETHODCALLTYPE d3d10_effect_type_IsValid(ID3D10EffectType *iface)
{
    struct d3d10_effect_type *This = impl_from_ID3D10EffectType(iface);

    TRACE("iface %p\n", iface);

    return This != &null_type;
}

static HRESULT STDMETHODCALLTYPE d3d10_effect_type_GetDesc(ID3D10EffectType *iface, D3D10_EFFECT_TYPE_DESC *desc)
{
    struct d3d10_effect_type *This = impl_from_ID3D10EffectType(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    if (This == &null_type)
    {
        WARN("Null type specified\n");
        return E_FAIL;
    }

    if (!desc)
    {
        WARN("Invalid argument specified\n");
        return E_INVALIDARG;
    }

    desc->TypeName = This->name;
    desc->Class = This->type_class;
    desc->Type = This->basetype;
    desc->Elements = This->element_count;
    desc->Members = This->member_count;
    desc->Rows = This->row_count;
    desc->Columns = This->column_count;
    desc->PackedSize = This->size_packed;
    desc->UnpackedSize = This->size_unpacked;
    desc->Stride = This->stride;

    return S_OK;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_type_GetMemberTypeByIndex(ID3D10EffectType *iface,
        UINT index)
{
    struct d3d10_effect_type *This = impl_from_ID3D10EffectType(iface);
    struct d3d10_effect_type *t;

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->member_count)
    {
        WARN("Invalid index specified\n");
        return &null_type.ID3D10EffectType_iface;
    }

    t = (&This->members[index])->type;

    TRACE("Returning member %p, %s\n", t, debugstr_a(t->name));

    return &t->ID3D10EffectType_iface;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_type_GetMemberTypeByName(ID3D10EffectType *iface,
        LPCSTR name)
{
    struct d3d10_effect_type *This = impl_from_ID3D10EffectType(iface);
    unsigned int i;

    TRACE("iface %p, name %s\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("Invalid name specified\n");
        return &null_type.ID3D10EffectType_iface;
    }

    for (i = 0; i < This->member_count; ++i)
    {
        struct d3d10_effect_type_member *typem = &This->members[i];

        if (typem->name)
        {
            if (!strcmp(typem->name, name))
            {
                TRACE("Returning type %p.\n", typem->type);
                return &typem->type->ID3D10EffectType_iface;
            }
        }
    }

    WARN("Invalid name specified\n");

    return &null_type.ID3D10EffectType_iface;
}

static struct ID3D10EffectType * STDMETHODCALLTYPE d3d10_effect_type_GetMemberTypeBySemantic(ID3D10EffectType *iface,
        LPCSTR semantic)
{
    struct d3d10_effect_type *This = impl_from_ID3D10EffectType(iface);
    unsigned int i;

    TRACE("iface %p, semantic %s\n", iface, debugstr_a(semantic));

    if (!semantic)
    {
        WARN("Invalid semantic specified\n");
        return &null_type.ID3D10EffectType_iface;
    }

    for (i = 0; i < This->member_count; ++i)
    {
        struct d3d10_effect_type_member *typem = &This->members[i];

        if (typem->semantic)
        {
            if (!strcmp(typem->semantic, semantic))
            {
                TRACE("Returning type %p.\n", typem->type);
                return &typem->type->ID3D10EffectType_iface;
            }
        }
    }

    WARN("Invalid semantic specified\n");

    return &null_type.ID3D10EffectType_iface;
}

static LPCSTR STDMETHODCALLTYPE d3d10_effect_type_GetMemberName(ID3D10EffectType *iface, UINT index)
{
    struct d3d10_effect_type *This = impl_from_ID3D10EffectType(iface);
    struct d3d10_effect_type_member *typem;

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->member_count)
    {
        WARN("Invalid index specified\n");
        return NULL;
    }

    typem = &This->members[index];

    TRACE("Returning name %s\n", debugstr_a(typem->name));

    return typem->name;
}

static LPCSTR STDMETHODCALLTYPE d3d10_effect_type_GetMemberSemantic(ID3D10EffectType *iface, UINT index)
{
    struct d3d10_effect_type *This = impl_from_ID3D10EffectType(iface);
    struct d3d10_effect_type_member *typem;

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->member_count)
    {
        WARN("Invalid index specified\n");
        return NULL;
    }

    typem = &This->members[index];

    TRACE("Returning semantic %s\n", debugstr_a(typem->semantic));

    return typem->semantic;
}

static const struct ID3D10EffectTypeVtbl d3d10_effect_type_vtbl =
{
    /* ID3D10EffectType */
    d3d10_effect_type_IsValid,
    d3d10_effect_type_GetDesc,
    d3d10_effect_type_GetMemberTypeByIndex,
    d3d10_effect_type_GetMemberTypeByName,
    d3d10_effect_type_GetMemberTypeBySemantic,
    d3d10_effect_type_GetMemberName,
    d3d10_effect_type_GetMemberSemantic,
};
