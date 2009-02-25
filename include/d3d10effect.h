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

#endif /* __WINE_D3D10EFFECT_H */
