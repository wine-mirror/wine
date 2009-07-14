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

#ifndef __WINE_D3D10EFFECT_H
#define __WINE_D3D10EFFECT_H

#include "d3d10.h"

#define D3D10_EFFECT_VARIABLE_POOLED                0x1
#define D3D10_EFFECT_VARIABLE_ANNOTATION            0x2
#define D3D10_EFFECT_VARIABLE_EXPLICIT_BIND_POINT   0x4

#ifndef D3D10_BYTES_FROM_BITS
#define D3D10_BYTES_FROM_BITS(x) (((x) + 7) >> 3)
#endif

typedef struct _D3D10_EFFECT_TYPE_DESC
{
    LPCSTR TypeName;
    D3D10_SHADER_VARIABLE_CLASS Class;
    D3D10_SHADER_VARIABLE_TYPE Type;
    UINT Elements;
    UINT Members;
    UINT Rows;
    UINT Columns;
    UINT PackedSize;
    UINT UnpackedSize;
    UINT Stride;
} D3D10_EFFECT_TYPE_DESC;

typedef struct _D3D10_EFFECT_VARIABLE_DESC
{
    LPCSTR Name;
    LPCSTR Semantic;
    UINT Flags;
    UINT Annotations;
    UINT BufferOffset;
    UINT ExplicitBindPoint;
} D3D10_EFFECT_VARIABLE_DESC;

typedef struct _D3D10_TECHNIQUE_DESC
{
    LPCSTR Name;
    UINT Passes;
    UINT Annotations;
} D3D10_TECHNIQUE_DESC;

typedef struct _D3D10_STATE_BLOCK_MASK
{
    BYTE VS;
    BYTE VSSamplers[D3D10_BYTES_FROM_BITS(D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT)];
    BYTE VSShaderResources[D3D10_BYTES_FROM_BITS(D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)];
    BYTE VSConstantBuffers[D3D10_BYTES_FROM_BITS(D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)];
    BYTE GS;
    BYTE GSSamplers[D3D10_BYTES_FROM_BITS(D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT)];
    BYTE GSShaderResources[D3D10_BYTES_FROM_BITS(D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)];
    BYTE GSConstantBuffers[D3D10_BYTES_FROM_BITS(D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)];
    BYTE PS;
    BYTE PSSamplers[D3D10_BYTES_FROM_BITS(D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT)];
    BYTE PSShaderResources[D3D10_BYTES_FROM_BITS(D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)];
    BYTE PSConstantBuffers[D3D10_BYTES_FROM_BITS(D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)];
    BYTE IAVertexBuffers[D3D10_BYTES_FROM_BITS(D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT)];
    BYTE IAIndexBuffer;
    BYTE IAInputLayout;
    BYTE IAPrimitiveTopology;
    BYTE OMRenderTargets;
    BYTE OMDepthStencilState;
    BYTE OMBlendState;
    BYTE RSViewports;
    BYTE RSScissorRects;
    BYTE RSRasterizerState;
    BYTE SOBuffers;
    BYTE Predication;
} D3D10_STATE_BLOCK_MASK;

typedef struct _D3D10_EFFECT_DESC
{
    BOOL IsChildEffect;
    UINT ConstantBuffers;
    UINT SharedConstantBuffers;
    UINT GlobalVariables;
    UINT SharedGlobalVariables;
    UINT Techniques;
} D3D10_EFFECT_DESC;

typedef struct _D3D10_PASS_DESC
{
    LPCSTR Name;
    UINT Annotations;
    BYTE *pIAInputSignature;
    SIZE_T IAInputSignatureSize;
    UINT StencilRef;
    UINT SampleMask;
    FLOAT BlendFactor[4];
} D3D10_PASS_DESC;

typedef struct _D3D10_PASS_SHADER_DESC
{
    struct ID3D10EffectShaderVariable *pShaderVariable;
    UINT ShaderIndex;
} D3D10_PASS_SHADER_DESC;

DEFINE_GUID(IID_ID3D10EffectType, 0x4e9e1ddc, 0xcd9d, 0x4772, 0xa8, 0x37, 0x00, 0x18, 0x0b, 0x9b, 0x88, 0xfd);

#define INTERFACE ID3D10EffectType
DECLARE_INTERFACE(ID3D10EffectType)
{
    STDMETHOD_(BOOL, IsValid)(THIS) PURE;
    STDMETHOD(GetDesc)(THIS_ D3D10_EFFECT_TYPE_DESC *desc) PURE;
    STDMETHOD_(struct ID3D10EffectType *, GetMemberTypeByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectType *, GetMemberTypeByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD_(struct ID3D10EffectType *, GetMemberTypeBySemantic)(THIS_ LPCSTR semantic) PURE;
    STDMETHOD_(LPCSTR, GetMemberName)(THIS_ UINT index) PURE;
    STDMETHOD_(LPCSTR, GetMemberSemantic)(THIS_ UINT index) PURE;
};
#undef INTERFACE

DEFINE_GUID(IID_ID3D10EffectVariable, 0xae897105, 0x00e6, 0x45bf, 0xbb, 0x8e, 0x28, 0x1d, 0xd6, 0xdb, 0x8e, 0x1b);

#define INTERFACE ID3D10EffectVariable
DECLARE_INTERFACE(ID3D10EffectVariable)
{
    STDMETHOD_(BOOL, IsValid)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectType *, GetType)(THIS) PURE;
    STDMETHOD(GetDesc)(THIS_ D3D10_EFFECT_VARIABLE_DESC *desc) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetAnnotationByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetAnnotationByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetMemberByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetMemberByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetMemberBySemantic)(THIS_ LPCSTR semantic) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetElement)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectConstantBuffer *, GetParentConstantBuffer)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectScalarVariable *, AsScalar)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectVectorVariable *, AsVector)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectMatrixVariable *, AsMatrix)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectStringVariable *, AsString)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectShaderResourceVariable *, AsShaderResource)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectRenderTargetViewVariable *, AsRenderTargetView)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectDepthStencilViewVariable *, AsDepthStencilView)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectConstantBuffer *, AsConstantBuffer)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectShaderVariable *, AsShader)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectBlendVariable *, AsBlend)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectDepthStencilVariable *, AsDepthStencil)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectRasterizerVariable *, AsRasterizer)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectSamplerVariable *, AsSampler)(THIS) PURE;
    STDMETHOD(SetRawValue)(THIS_ void *data, UINT offset, UINT count) PURE;
    STDMETHOD(GetRawValue)(THIS_ void *data, UINT offset, UINT count) PURE;
};
#undef INTERFACE

DEFINE_GUID(IID_ID3D10EffectConstantBuffer, 0x56648f4d, 0xcc8b, 0x4444, 0xa5, 0xad, 0xb5, 0xa3, 0xd7, 0x6e, 0x91, 0xb3);

#define INTERFACE ID3D10EffectConstantBuffer
DECLARE_INTERFACE_(ID3D10EffectConstantBuffer, ID3D10EffectVariable)
{
    /* ID3D10EffectVariable methods */
    STDMETHOD_(BOOL, IsValid)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectType *, GetType)(THIS) PURE;
    STDMETHOD(GetDesc)(THIS_ D3D10_EFFECT_VARIABLE_DESC *desc) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetAnnotationByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetAnnotationByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetMemberByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetMemberByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetMemberBySemantic)(THIS_ LPCSTR semantic) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetElement)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectConstantBuffer *, GetParentConstantBuffer)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectScalarVariable *, AsScalar)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectVectorVariable *, AsVector)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectMatrixVariable *, AsMatrix)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectStringVariable *, AsString)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectShaderResourceVariable *, AsShaderResource)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectRenderTargetViewVariable *, AsRenderTargetView)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectDepthStencilViewVariable *, AsDepthStencilView)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectConstantBuffer *, AsConstantBuffer)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectShaderVariable *, AsShader)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectBlendVariable *, AsBlend)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectDepthStencilVariable *, AsDepthStencil)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectRasterizerVariable *, AsRasterizer)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectSamplerVariable *, AsSampler)(THIS) PURE;
    STDMETHOD(SetRawValue)(THIS_ void *data, UINT offset, UINT count) PURE;
    STDMETHOD(GetRawValue)(THIS_ void *data, UINT offset, UINT count) PURE;
    /* ID3D10EffectConstantBuffer methods */
    STDMETHOD(SetConstantBuffer)(THIS_ ID3D10Buffer *buffer) PURE;
    STDMETHOD(GetConstantBuffer)(THIS_ ID3D10Buffer **buffer) PURE;
    STDMETHOD(SetTextureBuffer)(THIS_ ID3D10ShaderResourceView *view) PURE;
    STDMETHOD(GetTextureBuffer)(THIS_ ID3D10ShaderResourceView **view) PURE;
};
#undef INTERFACE

DEFINE_GUID(IID_ID3D10EffectMatrixVariable, 0x50666c24, 0xb82f, 0x4eed, 0xa1, 0x72, 0x5b, 0x6e, 0x7e, 0x85, 0x22, 0xe0);

#define INTERFACE ID3D10EffectMatrixVariable
DECLARE_INTERFACE_(ID3D10EffectMatrixVariable, ID3D10EffectVariable)
{
    /* ID3D10EffectVariable methods */
    STDMETHOD_(BOOL, IsValid)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectType *, GetType)(THIS) PURE;
    STDMETHOD(GetDesc)(THIS_ D3D10_EFFECT_VARIABLE_DESC *desc) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetAnnotationByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetAnnotationByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetMemberByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetMemberByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetMemberBySemantic)(THIS_ LPCSTR semantic) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetElement)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectConstantBuffer *, GetParentConstantBuffer)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectScalarVariable *, AsScalar)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectVectorVariable *, AsVector)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectMatrixVariable *, AsMatrix)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectStringVariable *, AsString)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectShaderResourceVariable *, AsShaderResource)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectRenderTargetViewVariable *, AsRenderTargetView)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectDepthStencilViewVariable *, AsDepthStencilView)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectConstantBuffer *, AsConstantBuffer)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectShaderVariable *, AsShader)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectBlendVariable *, AsBlend)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectDepthStencilVariable *, AsDepthStencil)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectRasterizerVariable *, AsRasterizer)(THIS) PURE;
    STDMETHOD_(struct ID3D10EffectSamplerVariable *, AsSampler)(THIS) PURE;
    STDMETHOD(SetRawValue)(THIS_ void *data, UINT offset, UINT count) PURE;
    STDMETHOD(GetRawValue)(THIS_ void *data, UINT offset, UINT count) PURE;
    /* ID3D10EffectMatrixVariable methods */
    STDMETHOD(SetMatrix)(THIS_ float *data) PURE;
    STDMETHOD(GetMatrix)(THIS_ float *data) PURE;
    STDMETHOD(SetMatrixArray)(THIS_ float *data, UINT offset, UINT count) PURE;
    STDMETHOD(GetMatrixArray)(THIS_ float *data, UINT offset, UINT count) PURE;
    STDMETHOD(SetMatrixTranspose)(THIS_ float *data) PURE;
    STDMETHOD(GetMatrixTranspose)(THIS_ float *data) PURE;
    STDMETHOD(SetMatrixTransposeArray)(THIS_ float *data, UINT offset, UINT count) PURE;
    STDMETHOD(GetMatrixTransposeArray)(THIS_ float *data, UINT offset, UINT count) PURE;
};
#undef INTERFACE

DEFINE_GUID(IID_ID3D10EffectTechnique, 0xdb122ce8, 0xd1c9, 0x4292, 0xb2, 0x37, 0x24, 0xed, 0x3d, 0xe8, 0xb1, 0x75);

#define INTERFACE ID3D10EffectTechnique
DECLARE_INTERFACE(ID3D10EffectTechnique)
{
    STDMETHOD_(BOOL, IsValid)(THIS) PURE;
    STDMETHOD(GetDesc)(THIS_ D3D10_TECHNIQUE_DESC *desc) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetAnnotationByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetAnnotationByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD_(struct ID3D10EffectPass *, GetPassByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectPass *, GetPassByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD(ComputeStateBlockMask)(THIS_ D3D10_STATE_BLOCK_MASK *mask) PURE;
};
#undef INTERFACE

DEFINE_GUID(IID_ID3D10Effect, 0x51b0ca8b, 0xec0b, 0x4519, 0x87, 0x0d, 0x8e, 0xe1, 0xcb, 0x50, 0x17, 0xc7);

#define INTERFACE ID3D10Effect
DECLARE_INTERFACE_(ID3D10Effect, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *object) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* ID3D10Effect methods */
    STDMETHOD_(BOOL, IsValid)(THIS) PURE;
    STDMETHOD_(BOOL, IsPool)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ ID3D10Device **device) PURE;
    STDMETHOD(GetDesc)(THIS_ D3D10_EFFECT_DESC *desc) PURE;
    STDMETHOD_(struct ID3D10EffectConstantBuffer *, GetConstantBufferByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectConstantBuffer *, GetConstantBufferByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetVariableByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetVariableByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetVariableBySemantic)(THIS_ LPCSTR semantic) PURE;
    STDMETHOD_(struct ID3D10EffectTechnique *, GetTechniqueByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectTechnique *, GetTechniqueByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD(Optimize)(THIS) PURE;
    STDMETHOD_(BOOL, IsOptimized)(THIS) PURE;
};
#undef INTERFACE

DEFINE_GUID(IID_ID3D10EffectPool, 0x9537ab04, 0x3250, 0x412e, 0x82, 0x13, 0xfc, 0xd2, 0xf8, 0x67, 0x79, 0x33);

#define INTERFACE ID3D10EffectPool
DECLARE_INTERFACE_(ID3D10EffectPool, IUnknown)
{
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *object) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* ID3D10EffectPool methods */
    STDMETHOD_(struct ID3D10Effect *, AsEffect)(THIS) PURE;
};
#undef INTERFACE

DEFINE_GUID(IID_ID3D10EffectPass, 0x5cfbeb89, 0x1a06, 0x46e0, 0xb2, 0x82, 0xe3, 0xf9, 0xbf, 0xa3, 0x6a, 0x54);

#define INTERFACE ID3D10EffectPass
DECLARE_INTERFACE(ID3D10EffectPass)
{
    STDMETHOD_(BOOL, IsValid)(THIS) PURE;
    STDMETHOD(GetDesc)(THIS_ D3D10_PASS_DESC *desc) PURE;
    STDMETHOD(GetVertexShaderDesc)(THIS_ D3D10_PASS_SHADER_DESC *desc) PURE;
    STDMETHOD(GetGeometryShaderDesc)(THIS_ D3D10_PASS_SHADER_DESC *desc) PURE;
    STDMETHOD(GetPixelShaderDesc)(THIS_ D3D10_PASS_SHADER_DESC *desc) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetAnnotationByIndex)(THIS_ UINT index) PURE;
    STDMETHOD_(struct ID3D10EffectVariable *, GetAnnotationByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD(Apply)(THIS_ UINT flags) PURE;
    STDMETHOD(ComputeStateBlockMask)(THIS_ D3D10_STATE_BLOCK_MASK *mask) PURE;
};
#undef INTERFACE

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI D3D10CreateEffectFromMemory(void *data, SIZE_T data_size, UINT flags,
        ID3D10Device *device, ID3D10EffectPool *effect_pool, ID3D10Effect **effect);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_D3D10EFFECT_H */
