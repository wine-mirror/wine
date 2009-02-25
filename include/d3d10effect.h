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

#endif /* __WINE_D3D10EFFECT_H */
