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
 *
 */

#ifndef __WINE_D3D10SHADER_H
#define __WINE_D3D10SHADER_H

#include "d3d10.h"

#define D3D10_SHADER_DEBUG                          0x0001
#define D3D10_SHADER_SKIP_VALIDATION                0x0002
#define D3D10_SHADER_SKIP_OPTIMIZATION              0x0004
#define D3D10_SHADER_PACK_MATRIX_ROW_MAJOR          0x0008
#define D3D10_SHADER_PACK_MATRIX_COLUMN_MAJOR       0x0010
#define D3D10_SHADER_PARTIAL_PRECISION              0x0020
#define D3D10_SHADER_FORCE_VS_SOFTWARE_NO_OPT       0x0040
#define D3D10_SHADER_FORCE_PS_SOFTWARE_NO_OPT       0x0080
#define D3D10_SHADER_NO_PRESHADER                   0x0100
#define D3D10_SHADER_AVOID_FLOW_CONTROL             0x0200
#define D3D10_SHADER_PREFER_FLOW_CONTROL            0x0400
#define D3D10_SHADER_ENABLE_STRICTNESS              0x0800
#define D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY 0x1000
#define D3D10_SHADER_IEEE_STRICTNESS                0x2000
#define D3D10_SHADER_WARNINGS_ARE_ERRORS           0x40000

#define D3D10_SHADER_OPTIMIZATION_LEVEL0            0x4000
#define D3D10_SHADER_OPTIMIZATION_LEVEL1            0x0000
#define D3D10_SHADER_OPTIMIZATION_LEVEL2            0xC000
#define D3D10_SHADER_OPTIMIZATION_LEVEL3            0x8000

typedef enum _D3D10_SHADER_VARIABLE_CLASS
{
    D3D10_SVC_SCALAR,
    D3D10_SVC_VECTOR,
    D3D10_SVC_MATRIX_ROWS,
    D3D10_SVC_MATRIX_COLUMNS,
    D3D10_SVC_OBJECT,
    D3D10_SVC_STRUCT,
    D3D10_SVC_FORCE_DWORD = 0x7fffffff
} D3D10_SHADER_VARIABLE_CLASS, *LPD3D10_SHADER_VARIABLE_CLASS;

typedef enum _D3D10_SHADER_VARIABLE_TYPE
{
    D3D10_SVT_VOID = 0,
    D3D10_SVT_BOOL = 1,
    D3D10_SVT_INT = 2,
    D3D10_SVT_FLOAT = 3,
    D3D10_SVT_STRING = 4,
    D3D10_SVT_TEXTURE = 5,
    D3D10_SVT_TEXTURE1D = 6,
    D3D10_SVT_TEXTURE2D = 7,
    D3D10_SVT_TEXTURE3D = 8,
    D3D10_SVT_TEXTURECUBE = 9,
    D3D10_SVT_SAMPLER = 10,
    D3D10_SVT_PIXELSHADER = 15,
    D3D10_SVT_VERTEXSHADER = 16,
    D3D10_SVT_UINT = 19,
    D3D10_SVT_UINT8 = 20,
    D3D10_SVT_GEOMETRYSHADER = 21,
    D3D10_SVT_RASTERIZER = 22,
    D3D10_SVT_DEPTHSTENCIL = 23,
    D3D10_SVT_BLEND = 24,
    D3D10_SVT_BUFFER = 25,
    D3D10_SVT_CBUFFER = 26,
    D3D10_SVT_TBUFFER = 27,
    D3D10_SVT_TEXTURE1DARRAY = 28,
    D3D10_SVT_TEXTURE2DARRAY = 29,
    D3D10_SVT_RENDERTARGETVIEW = 30,
    D3D10_SVT_DEPTHSTENCILVIEW = 31,
    D3D10_SVT_TEXTURE2DMS = 32,
    D3D10_SVT_TEXTURE2DMSARRAY = 33,
    D3D10_SVT_TEXTURECUBEARRAY = 34,
    D3D10_SVT_FORCE_DWORD = 0x7fffffff
} D3D10_SHADER_VARIABLE_TYPE, *LPD3D10_SHADER_VARIABLE_TYPE;

typedef enum _D3D10_SHADER_INPUT_TYPE
{
    D3D10_SIT_CBUFFER = 0,
    D3D10_SIT_TBUFFER = 1,
    D3D10_SIT_TEXTURE = 2,
    D3D10_SIT_SAMPLER = 3
} D3D10_SHADER_INPUT_TYPE, *LPD3D10_SHADER_INPUT_TYPE;

typedef enum D3D10_CBUFFER_TYPE
{
    D3D10_CT_CBUFFER = 0,
    D3D10_CT_TBUFFER = 1
} D3D10_CBUFFER_TYPE, *LPD3D10_CBUFFER_TYPE;

typedef enum D3D10_NAME
{
    D3D10_NAME_UNDEFINED = 0,
    D3D10_NAME_POSITION = 1,
    D3D10_NAME_CLIP_DISTANCE = 2,
    D3D10_NAME_CULL_DISTANCE = 3,
    D3D10_NAME_RENDER_TARGET_ARRAY_INDEX = 4,
    D3D10_NAME_VIEWPORT_ARRAY_INDEX = 5,
    D3D10_NAME_VERTEX_ID = 6,
    D3D10_NAME_PRIMITIVE_ID = 7,
    D3D10_NAME_INSTANCE_ID = 8,
    D3D10_NAME_IS_FRONT_FACE = 9,
    D3D10_NAME_SAMPLE_INDEX = 10,
    D3D10_NAME_TARGET = 64,
    D3D10_NAME_DEPTH = 65,
} D3D10_NAME;

typedef enum D3D10_REGISTER_COMPONENT_TYPE
{
    D3D10_REGISTER_COMPONENT_UNKNOWN = 0,
    D3D10_REGISTER_COMPONENT_UINT32 = 1,
    D3D10_REGISTER_COMPONENT_SINT32 = 2,
    D3D10_REGISTER_COMPONENT_FLOAT32 = 3,
} D3D10_REGISTER_COMPONENT_TYPE;

typedef struct _D3D10_SHADER_MACRO
{
    LPCSTR Name;
    LPCSTR Definition;
} D3D10_SHADER_MACRO, *LPD3D10_SHADER_MACRO;

typedef enum D3D10_RESOURCE_RETURN_TYPE
{
    D3D10_RETURN_TYPE_UNORM = 1,
    D3D10_RETURN_TYPE_SNORM = 2,
    D3D10_RETURN_TYPE_SINT = 3,
    D3D10_RETURN_TYPE_UINT = 4,
    D3D10_RETURN_TYPE_FLOAT = 5,
    D3D10_RETURN_TYPE_MIXED = 6
} D3D10_RESOURCE_RETURN_TYPE;

typedef struct _D3D10_SHADER_INPUT_BIND_DESC
{
    LPCSTR Name;
    D3D10_SHADER_INPUT_TYPE Type;
    UINT BindPoint;
    UINT BindCount;
    UINT uFlags;
    D3D10_RESOURCE_RETURN_TYPE ReturnType;
    D3D10_SRV_DIMENSION Dimension;
    UINT NumSamples;
} D3D10_SHADER_INPUT_BIND_DESC;

typedef struct _D3D10_SIGNATURE_PARAMETER_DESC
{
    LPCSTR SemanticName;
    UINT SemanticIndex;
    UINT Register;
    D3D10_NAME SystemValueType;
    D3D10_REGISTER_COMPONENT_TYPE ComponentType;
    BYTE Mask;
    BYTE ReadWriteMask;
} D3D10_SIGNATURE_PARAMETER_DESC;

typedef struct _D3D10_SHADER_DESC
{
    UINT Version;
    LPCSTR Creator;
    UINT Flags;
    UINT ConstantBuffers;
    UINT BoundResources;
    UINT InputParameters;
    UINT OutputParameters;
    UINT InstructionCount;
    UINT TempRegisterCount;
    UINT TempArrayCount;
    UINT DefCount;
    UINT DclCount;
    UINT TextureNormalInstructions;
    UINT TextureLoadInstructions;
    UINT TextureCompInstructions;
    UINT TextureBiasInstructions;
    UINT TextureGradientInstructions;
    UINT FloatInstructionCount;
    UINT IntInstructionCount;
    UINT UintInstructionCount;
    UINT StaticFlowControlCount;
    UINT DynamicFlowControlCount;
    UINT MacroInstructionCount;
    UINT ArrayInstructionCount;
    UINT CutInstructionCount;
    UINT EmitInstructionCount;
    D3D10_PRIMITIVE_TOPOLOGY GSOutputTopology;
    UINT GSMaxOutputVertexCount;
} D3D10_SHADER_DESC;

typedef struct _D3D10_SHADER_BUFFER_DESC
{
    LPCSTR Name;
    D3D10_CBUFFER_TYPE Type;
    UINT Variables;
    UINT Size;
    UINT uFlags;
} D3D10_SHADER_BUFFER_DESC;

typedef struct _D3D10_SHADER_VARIABLE_DESC
{
    LPCSTR Name;
    UINT StartOffset;
    UINT Size;
    UINT uFlags;
    LPVOID DefaultValue;
} D3D10_SHADER_VARIABLE_DESC;

typedef struct _D3D10_SHADER_TYPE_DESC
{
    D3D10_SHADER_VARIABLE_CLASS Class;
    D3D10_SHADER_VARIABLE_TYPE Type;
    UINT Rows;
    UINT Columns;
    UINT Elements;
    UINT Members;
    UINT Offset;
} D3D10_SHADER_TYPE_DESC;

DEFINE_GUID(IID_ID3D10ShaderReflectionType, 0xc530ad7d, 0x9b16, 0x4395, 0xa9, 0x79, 0xba, 0x2e, 0xcf, 0xf8, 0x3a, 0xdd);

#define INTERFACE ID3D10ShaderReflectionType
DECLARE_INTERFACE(ID3D10ShaderReflectionType)
{
    STDMETHOD(GetDesc)(THIS_ D3D10_SHADER_TYPE_DESC *desc) PURE;
    STDMETHOD_(struct ID3D10ShaderReflectionType *, GetMemberTypeByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10ShaderReflectionType *, GetMemberTypeByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD_(LPCSTR, GetMemberTypeName)(THIS_ UINT index) PURE;
};
#undef INTERFACE

DEFINE_GUID(IID_ID3D10ShaderReflectionVariable, 0x1bf63c95, 0x2650, 0x405d, 0x99, 0xc1, 0x36, 0x36, 0xbd, 0x1d, 0xa0, 0xa1);

#define INTERFACE ID3D10ShaderReflectionVariable
DECLARE_INTERFACE(ID3D10ShaderReflectionVariable)
{
    STDMETHOD(GetDesc)(THIS_ D3D10_SHADER_VARIABLE_DESC *desc) PURE;
    STDMETHOD_(struct ID3D10ShaderReflectionType *, GetType)(THIS) PURE;
};
#undef INTERFACE

DEFINE_GUID(IID_ID3D10ShaderReflectionConstantBuffer, 0x66c66a94, 0xdddd, 0x4b62, 0xa6, 0x6a, 0xf0, 0xda, 0x33, 0xc2, 0xb4, 0xd0);

#define INTERFACE ID3D10ShaderReflectionConstantBuffer
DECLARE_INTERFACE(ID3D10ShaderReflectionConstantBuffer)
{
    STDMETHOD(GetDesc)(THIS_ D3D10_SHADER_BUFFER_DESC *desc) PURE;
    STDMETHOD_(struct ID3D10ShaderReflectionVariable *, GetVariableByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10ShaderReflectionVariable *, GetVariableByName)(THIS_ LPCSTR name) PURE;
};
#undef INTERFACE

DEFINE_GUID(IID_ID3D10ShaderReflection, 0xd40e20b6, 0xf8f7, 0x42ad, 0xab, 0x20, 0x4b, 0xaf, 0x8f, 0x15, 0xdf, 0xaa);

#define INTERFACE ID3D10ShaderReflection
DECLARE_INTERFACE_(ID3D10ShaderReflection, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *object) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* ID3D10ShaderReflection methods */
    STDMETHOD(GetDesc)(THIS_ D3D10_SHADER_DESC *desc) PURE;
    STDMETHOD_(struct ID3D10ShaderReflectionConstantBuffer *, GetConstantBufferByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10ShaderReflectionConstantBuffer *, GetConstantBufferByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD(GetResourceBindingDesc)(THIS_ UINT index, D3D10_SHADER_INPUT_BIND_DESC *desc) PURE;
    STDMETHOD(GetInputParameterDesc)(THIS_ UINT index, D3D10_SIGNATURE_PARAMETER_DESC *desc) PURE;
    STDMETHOD(GetOutputParameterDesc)(THIS_ UINT index, D3D10_SIGNATURE_PARAMETER_DESC *desc) PURE;
};
#undef INTERFACE

LPCSTR WINAPI D3D10GetVertexShaderProfile(ID3D10Device *device);
LPCSTR WINAPI D3D10GetGeometryShaderProfile(ID3D10Device *device);
LPCSTR WINAPI D3D10GetPixelShaderProfile(ID3D10Device *device);

HRESULT WINAPI D3D10ReflectShader(const void *data, SIZE_T data_size, ID3D10ShaderReflection **reflector);

#endif /* __WINE_D3D10SHADER_H */
