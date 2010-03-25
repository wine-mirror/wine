/*
 * Copyright 2010 Christian Costa
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

#include <d3dx9.h>

#ifndef __D3DX9EFFECT_H__
#define __D3DX9EFFECT_H__

typedef struct _D3DXEFFECT_DESC {
    LPCSTR Creator;
    UINT Parameters;
    UINT Techniques;
    UINT Functions;
} D3DXEFFECT_DESC;

typedef struct _D3DXPARAMETER_DESC {
    LPCSTR Name;
    LPCSTR Semantic;
    D3DXPARAMETER_CLASS Class;
    D3DXPARAMETER_TYPE Type;
    UINT Rows;
    UINT Columns;
    UINT Elements;
    UINT Annotations;
    UINT StructMembers;
    DWORD Flags;
    UINT Bytes;
} D3DXPARAMETER_DESC;

typedef struct _D3DXTECHNIQUE_DESC {
    LPCSTR Name;
    UINT Passes;
    UINT Annotations;
} D3DXTECHNIQUE_DESC;

typedef struct _D3DXPASS_DESC {
    LPCSTR Name;
    UINT Annotations;
    CONST DWORD *pVertexShaderFunction;
    CONST DWORD *pPixelShaderFunction;
} D3DXPASS_DESC;

typedef struct _D3DXFUNCTION_DESC {
    LPCSTR Name;
    UINT Annotations;
} D3DXFUNCTION_DESC;

typedef struct ID3DXEffectPool *LPD3DXEFFECTPOOL;

DEFINE_GUID(IID_ID3DXEffectPool, 0x9537ab04, 0x3250, 0x412e, 0x82, 0x13, 0xfc, 0xd2, 0xf8, 0x67, 0x79, 0x33);

#undef INTERFACE
#define INTERFACE ID3DXEffectPool

DECLARE_INTERFACE_(ID3DXEffectPool, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID* object) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
};

typedef struct ID3DXBaseEffect *LPD3DXBASEEFFECT;

DEFINE_GUID(IID_ID3DXBaseEffect, 0x17c18ac, 0x103f, 0x4417, 0x8c, 0x51, 0x6b, 0xf6, 0xef, 0x1e, 0x56, 0xbe);

#undef INTERFACE
#define INTERFACE ID3DXBaseEffect

DECLARE_INTERFACE_(ID3DXBaseEffect, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID* object) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /*** ID3DXBaseEffect methods ***/
    STDMETHOD(GetDesc)(THIS_ D3DXEFFECT_DESC* desc) PURE;
    STDMETHOD(GetParameterDesc)(THIS_ D3DXHANDLE parameter, D3DXPARAMETER_DESC* desc) PURE;
    STDMETHOD(GetTechniqueDesc)(THIS_ D3DXHANDLE technique, D3DXTECHNIQUE_DESC* desc) PURE;
    STDMETHOD(GetPassDesc)(THIS_ D3DXHANDLE pass, D3DXPASS_DESC* desc) PURE;
    STDMETHOD(GetFunctionDesc)(THIS_ D3DXHANDLE shader, D3DXFUNCTION_DESC* desc) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameter)(THIS_ D3DXHANDLE parameter, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterByName)(THIS_ D3DXHANDLE parameter, LPCSTR name) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterBySemantic)(THIS_ D3DXHANDLE parameter, LPCSTR semantic) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterElement)(THIS_ D3DXHANDLE parameter, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetTechnique)(THIS_ UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetTechniqueByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD_(D3DXHANDLE, GetPass)(THIS_ D3DXHANDLE technique, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetPassByName)(THIS_ D3DXHANDLE technique, LPCSTR name) PURE;
    STDMETHOD_(D3DXHANDLE, GetFunction)(THIS_ UINT index);
    STDMETHOD_(D3DXHANDLE, GetFunctionByName)(THIS_ LPCSTR name);
    STDMETHOD_(D3DXHANDLE, GetAnnotation)(THIS_ D3DXHANDLE object, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetAnnotationByName)(THIS_ D3DXHANDLE object, LPCSTR name) PURE;
    STDMETHOD(SetValue)(THIS_ D3DXHANDLE parameter, LPCVOID data, UINT bytes) PURE;
    STDMETHOD(GetValue)(THIS_ D3DXHANDLE parameter, LPVOID data, UINT bytes) PURE;
    STDMETHOD(SetBool)(THIS_ D3DXHANDLE parameter, BOOL b) PURE;
    STDMETHOD(GetBool)(THIS_ D3DXHANDLE parameter, BOOL* b) PURE;
    STDMETHOD(SetBoolArray)(THIS_ D3DXHANDLE parameter, CONST BOOL* b, UINT count) PURE;
    STDMETHOD(GetBoolArray)(THIS_ D3DXHANDLE parameter, BOOL* b, UINT count) PURE;
    STDMETHOD(SetInt)(THIS_ D3DXHANDLE parameter, INT n) PURE;
    STDMETHOD(GetInt)(THIS_ D3DXHANDLE parameter, INT* n) PURE;
    STDMETHOD(SetIntArray)(THIS_ D3DXHANDLE parameter, CONST INT* n, UINT count) PURE;
    STDMETHOD(GetIntArray)(THIS_ D3DXHANDLE parameter, INT* n, UINT count) PURE;
    STDMETHOD(SetFloat)(THIS_ D3DXHANDLE parameter, FLOAT f) PURE;
    STDMETHOD(GetFloat)(THIS_ D3DXHANDLE parameter, FLOAT* f) PURE;
    STDMETHOD(SetFloatArray)(THIS_ D3DXHANDLE parameter, CONST FLOAT* f, UINT count) PURE;
    STDMETHOD(GetFloatArray)(THIS_ D3DXHANDLE parameter, FLOAT* f, UINT count) PURE;
    STDMETHOD(SetVector)(THIS_ D3DXHANDLE parameter, CONST D3DXVECTOR4* vector) PURE;
    STDMETHOD(GetVector)(THIS_ D3DXHANDLE parameter, D3DXVECTOR4* vector) PURE;
    STDMETHOD(SetVectorArray)(THIS_ D3DXHANDLE parameter, CONST D3DXVECTOR4* vector, UINT count) PURE;
    STDMETHOD(GetVectorArray)(THIS_ D3DXHANDLE parameter, D3DXVECTOR4* vector, UINT count) PURE;
    STDMETHOD(SetMatrix)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX* matrix) PURE;
    STDMETHOD(GetMatrix)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix) PURE;
    STDMETHOD(SetMatrixArray)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX* matrix, UINT count) PURE;
    STDMETHOD(GetMatrixArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix, UINT count) PURE;
    STDMETHOD(SetMatrixPointerArray)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX** matrix, UINT count) PURE;
    STDMETHOD(GetMatrixPointerArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX** matrix, UINT count) PURE;
    STDMETHOD(SetMatrixTranspose)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX* matrix) PURE;
    STDMETHOD(GetMatrixTranspose)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix) PURE;
    STDMETHOD(SetMatrixTransposeArray)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX* matrix, UINT count) PURE;
    STDMETHOD(GetMatrixTransposeArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix, UINT count) PURE;
    STDMETHOD(SetMatrixTransposePointerArray)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX** matrix, UINT count) PURE;
    STDMETHOD(GetMatrixTransposePointerArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX** matrix, UINT count) PURE;
    STDMETHOD(SetString)(THIS_ D3DXHANDLE parameter, LPCSTR string) PURE;
    STDMETHOD(GetString)(THIS_ D3DXHANDLE parameter, LPCSTR* string) PURE;
    STDMETHOD(SetTexture)(THIS_ D3DXHANDLE parameter, LPDIRECT3DBASETEXTURE9 texture) PURE;
    STDMETHOD(GetTexture)(THIS_ D3DXHANDLE parameter, LPDIRECT3DBASETEXTURE9* texture) PURE;
    STDMETHOD(SetPixelShader)(THIS_ D3DXHANDLE parameter, LPDIRECT3DPIXELSHADER9 pshader) PURE;
    STDMETHOD(GetPixelShader)(THIS_ D3DXHANDLE parameter, LPDIRECT3DPIXELSHADER9* pshader) PURE;
    STDMETHOD(SetVertexShader)(THIS_ D3DXHANDLE parameter, LPDIRECT3DVERTEXSHADER9 vshader) PURE;
    STDMETHOD(GetVertexShader)(THIS_ D3DXHANDLE parameter, LPDIRECT3DVERTEXSHADER9* vshader) PURE;
    STDMETHOD(SetArrayRange)(THIS_ D3DXHANDLE parameter, UINT start, UINT end) PURE;
};

typedef struct ID3DXEffectStateManager *LPD3DXEFFECTSTATEMANAGER;

DEFINE_GUID(IID_ID3DXEffectStateManager, 0x79aab587, 0x6dbc, 0x4fa7, 0x82, 0xde, 0x37, 0xfa, 0x17, 0x81, 0xc5, 0xce);

#undef INTERFACE
#define INTERFACE ID3DXEffectStateManager

DECLARE_INTERFACE_(ID3DXEffectStateManager, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID* object) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /*** ID3DXEffectStateManager methods ***/
    STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX* matrix) PURE;
    STDMETHOD(SetMaterial)(THIS_ CONST D3DMATERIAL9* material) PURE;
    STDMETHOD(SetLight)(THIS_ DWORD index, CONST D3DLIGHT9* light) PURE;
    STDMETHOD(LightEnable)(THIS_ DWORD index, BOOL enable) PURE;
    STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE state, DWORD value) PURE;
    STDMETHOD(SetTexture)(THIS_ DWORD stage, LPDIRECT3DBASETEXTURE9 texture) PURE;
    STDMETHOD(SetTextureStageState)(THIS_ DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value) PURE;
    STDMETHOD(SetSamplerState)(THIS_ DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value) PURE;
    STDMETHOD(SetNPatchMode)(THIS_ FLOAT num_segments) PURE;
    STDMETHOD(SetFVF)(THIS_ DWORD format) PURE;
    STDMETHOD(SetVertexShader)(THIS_ LPDIRECT3DVERTEXSHADER9 shader) PURE;
    STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT register_index, CONST FLOAT* constant_data, UINT register_count) PURE;
    STDMETHOD(SetVertexShaderConstantI)(THIS_ UINT register_index, CONST INT* constant_data, UINT register_count) PURE;
    STDMETHOD(SetVertexShaderConstantB)(THIS_ UINT register_index, CONST BOOL* constant_data, UINT register_count) PURE;
    STDMETHOD(SetPixelShader)(THIS_ LPDIRECT3DPIXELSHADER9 shader) PURE;
    STDMETHOD(SetPixelShaderConstantF)(THIS_ UINT register_index, CONST FLOAT* constant_data, UINT register_count) PURE;
    STDMETHOD(SetPixelShaderConstantI)(THIS_ UINT register_index, CONST INT * constant_data, UINT register_count) PURE;
    STDMETHOD(SetPixelShaderConstantB)(THIS_ UINT register_index, CONST BOOL* constant_data, UINT register_count) PURE;
};

typedef struct ID3DXEffect *LPD3DXEFFECT;

DEFINE_GUID(IID_ID3DXEffect, 0xf6ceb4b3, 0x4e4c, 0x40dd, 0xb8, 0x83, 0x8d, 0x8d, 0xe5, 0xea, 0xc, 0xd5);

#undef INTERFACE
#define INTERFACE ID3DXEffect

DECLARE_INTERFACE_(ID3DXEffect, ID3DXBaseEffect)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID* object) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /*** ID3DXBaseEffect methods ***/
    STDMETHOD(GetDesc)(THIS_ D3DXEFFECT_DESC* desc) PURE;
    STDMETHOD(GetParameterDesc)(THIS_ D3DXHANDLE parameter, D3DXPARAMETER_DESC* desc) PURE;
    STDMETHOD(GetTechniqueDesc)(THIS_ D3DXHANDLE technique, D3DXTECHNIQUE_DESC* desc) PURE;
    STDMETHOD(GetPassDesc)(THIS_ D3DXHANDLE pass, D3DXPASS_DESC* desc) PURE;
    STDMETHOD(GetFunctionDesc)(THIS_ D3DXHANDLE shader, D3DXFUNCTION_DESC* desc) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameter)(THIS_ D3DXHANDLE parameter, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterByName)(THIS_ D3DXHANDLE parameter, LPCSTR name) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterBySemantic)(THIS_ D3DXHANDLE parameter, LPCSTR semantic) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterElement)(THIS_ D3DXHANDLE parameter, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetTechnique)(THIS_ UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetTechniqueByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD_(D3DXHANDLE, GetPass)(THIS_ D3DXHANDLE technique, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetPassByName)(THIS_ D3DXHANDLE technique, LPCSTR name) PURE;
    STDMETHOD_(D3DXHANDLE, GetFunction)(THIS_ UINT index);
    STDMETHOD_(D3DXHANDLE, GetFunctionByName)(THIS_ LPCSTR name);
    STDMETHOD_(D3DXHANDLE, GetAnnotation)(THIS_ D3DXHANDLE object, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetAnnotationByName)(THIS_ D3DXHANDLE object, LPCSTR name) PURE;
    STDMETHOD(SetValue)(THIS_ D3DXHANDLE parameter, LPCVOID data, UINT bytes) PURE;
    STDMETHOD(GetValue)(THIS_ D3DXHANDLE parameter, LPVOID data, UINT bytes) PURE;
    STDMETHOD(SetBool)(THIS_ D3DXHANDLE parameter, BOOL b) PURE;
    STDMETHOD(GetBool)(THIS_ D3DXHANDLE parameter, BOOL* b) PURE;
    STDMETHOD(SetBoolArray)(THIS_ D3DXHANDLE parameter, CONST BOOL* b, UINT count) PURE;
    STDMETHOD(GetBoolArray)(THIS_ D3DXHANDLE parameter, BOOL* b, UINT count) PURE;
    STDMETHOD(SetInt)(THIS_ D3DXHANDLE parameter, INT n) PURE;
    STDMETHOD(GetInt)(THIS_ D3DXHANDLE parameter, INT* n) PURE;
    STDMETHOD(SetIntArray)(THIS_ D3DXHANDLE parameter, CONST INT* n, UINT count) PURE;
    STDMETHOD(GetIntArray)(THIS_ D3DXHANDLE parameter, INT* n, UINT count) PURE;
    STDMETHOD(SetFloat)(THIS_ D3DXHANDLE parameter, FLOAT f) PURE;
    STDMETHOD(GetFloat)(THIS_ D3DXHANDLE parameter, FLOAT* f) PURE;
    STDMETHOD(SetFloatArray)(THIS_ D3DXHANDLE parameter, CONST FLOAT* f, UINT count) PURE;
    STDMETHOD(GetFloatArray)(THIS_ D3DXHANDLE parameter, FLOAT* f, UINT count) PURE;
    STDMETHOD(SetVector)(THIS_ D3DXHANDLE parameter, CONST D3DXVECTOR4* vector) PURE;
    STDMETHOD(GetVector)(THIS_ D3DXHANDLE parameter, D3DXVECTOR4* vector) PURE;
    STDMETHOD(SetVectorArray)(THIS_ D3DXHANDLE parameter, CONST D3DXVECTOR4* vector, UINT count) PURE;
    STDMETHOD(GetVectorArray)(THIS_ D3DXHANDLE parameter, D3DXVECTOR4* vector, UINT count) PURE;
    STDMETHOD(SetMatrix)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX* matrix) PURE;
    STDMETHOD(GetMatrix)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix) PURE;
    STDMETHOD(SetMatrixArray)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX* matrix, UINT count) PURE;
    STDMETHOD(GetMatrixArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix, UINT count) PURE;
    STDMETHOD(SetMatrixPointerArray)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX** matrix, UINT count) PURE;
    STDMETHOD(GetMatrixPointerArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX** matrix, UINT count) PURE;
    STDMETHOD(SetMatrixTranspose)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX* matrix) PURE;
    STDMETHOD(GetMatrixTranspose)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix) PURE;
    STDMETHOD(SetMatrixTransposeArray)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX* matrix, UINT count) PURE;
    STDMETHOD(GetMatrixTransposeArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix, UINT count) PURE;
    STDMETHOD(SetMatrixTransposePointerArray)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX** matrix, UINT count) PURE;
    STDMETHOD(GetMatrixTransposePointerArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX** matrix, UINT count) PURE;
    STDMETHOD(SetString)(THIS_ D3DXHANDLE parameter, LPCSTR string) PURE;
    STDMETHOD(GetString)(THIS_ D3DXHANDLE parameter, LPCSTR* string) PURE;
    STDMETHOD(SetTexture)(THIS_ D3DXHANDLE parameter, LPDIRECT3DBASETEXTURE9 texture) PURE;
    STDMETHOD(GetTexture)(THIS_ D3DXHANDLE parameter, LPDIRECT3DBASETEXTURE9* texture) PURE;
    STDMETHOD(GetPixelShader)(THIS_ D3DXHANDLE parameter, LPDIRECT3DPIXELSHADER9* pshader) PURE;
    STDMETHOD(GetVertexShader)(THIS_ D3DXHANDLE parameter, LPDIRECT3DVERTEXSHADER9* vshader) PURE;
    STDMETHOD(SetArrayRange)(THIS_ D3DXHANDLE parameter, UINT start, UINT end) PURE;
    /*** ID3DXEffect methods ***/
    STDMETHOD(GetPool)(THIS_ LPD3DXEFFECTPOOL* pool) PURE;
    STDMETHOD(SetTechnique)(THIS_ D3DXHANDLE technique) PURE;
    STDMETHOD_(D3DXHANDLE, GetCurrentTechnique)(THIS) PURE;
    STDMETHOD(ValidateTechnique)(THIS_ D3DXHANDLE technique) PURE;
    STDMETHOD(FindNextValidTechnique)(THIS_ D3DXHANDLE technique, D3DXHANDLE* next_technique) PURE;
    STDMETHOD_(BOOL, IsParameterUsed)(THIS_ D3DXHANDLE parameter, D3DXHANDLE technique) PURE;
    STDMETHOD(Begin)(THIS_ UINT *passes, DWORD flags) PURE;
    STDMETHOD(BeginPass)(THIS_ UINT pass) PURE;
    STDMETHOD(CommitChanges)(THIS) PURE;
    STDMETHOD(EndPass)(THIS) PURE;
    STDMETHOD(End)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* device) PURE;
    STDMETHOD(OnLostDevice)(THIS) PURE;
    STDMETHOD(OnResetDevice)(THIS) PURE;
    STDMETHOD(SetStateManager)(THIS_ LPD3DXEFFECTSTATEMANAGER manager) PURE;
    STDMETHOD(GetStateManager)(THIS_ LPD3DXEFFECTSTATEMANAGER* manager) PURE;
    STDMETHOD(BeginParameterBlock)(THIS) PURE;
    STDMETHOD_(D3DXHANDLE, EndParameterBlock)(THIS) PURE;
    STDMETHOD(ApplyParameterBlock)(THIS_ D3DXHANDLE parameter_block) PURE;
    STDMETHOD(DeleteParameterBlock)(THIS_ D3DXHANDLE parameter_block) PURE;
    STDMETHOD(CloneEffect)(THIS_ LPDIRECT3DDEVICE9 device, LPD3DXEFFECT* effect) PURE;
    STDMETHOD(SetRawValue)(THIS_ D3DXHANDLE parameter, LPCVOID data, UINT byte_offset, UINT bytes) PURE;
};

typedef struct ID3DXEffectCompiler *LPD3DXEFFECTCOMPILER;

DEFINE_GUID(IID_ID3DXEffectCompiler, 0x51b8a949, 0x1a31, 0x47e6, 0xbe, 0xa0, 0x4b, 0x30, 0xdb, 0x53, 0xf1, 0xe0);

#undef INTERFACE
#define INTERFACE ID3DXEffectCompiler

DECLARE_INTERFACE_(ID3DXEffectCompiler, ID3DXBaseEffect)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID* object) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /*** ID3DXBaseEffect methods ***/
    STDMETHOD(GetDesc)(THIS_ D3DXEFFECT_DESC* desc) PURE;
    STDMETHOD(GetParameterDesc)(THIS_ D3DXHANDLE parameter, D3DXPARAMETER_DESC* desc) PURE;
    STDMETHOD(GetTechniqueDesc)(THIS_ D3DXHANDLE technique, D3DXTECHNIQUE_DESC* desc) PURE;
    STDMETHOD(GetPassDesc)(THIS_ D3DXHANDLE pass, D3DXPASS_DESC* desc) PURE;
    STDMETHOD(GetFunctionDesc)(THIS_ D3DXHANDLE shader, D3DXFUNCTION_DESC* desc) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameter)(THIS_ D3DXHANDLE parameter, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterByName)(THIS_ D3DXHANDLE parameter, LPCSTR name) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterBySemantic)(THIS_ D3DXHANDLE parameter, LPCSTR semantic) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterElement)(THIS_ D3DXHANDLE parameter, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetTechnique)(THIS_ UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetTechniqueByName)(THIS_ LPCSTR name) PURE;
    STDMETHOD_(D3DXHANDLE, GetPass)(THIS_ D3DXHANDLE technique, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetPassByName)(THIS_ D3DXHANDLE technique, LPCSTR name) PURE;
    STDMETHOD_(D3DXHANDLE, GetFunction)(THIS_ UINT index);
    STDMETHOD_(D3DXHANDLE, GetFunctionByName)(THIS_ LPCSTR name);
    STDMETHOD_(D3DXHANDLE, GetAnnotation)(THIS_ D3DXHANDLE object, UINT index) PURE;
    STDMETHOD_(D3DXHANDLE, GetAnnotationByName)(THIS_ D3DXHANDLE object, LPCSTR name) PURE;
    STDMETHOD(SetValue)(THIS_ D3DXHANDLE parameter, LPCVOID data, UINT bytes) PURE;
    STDMETHOD(GetValue)(THIS_ D3DXHANDLE parameter, LPVOID data, UINT bytes) PURE;
    STDMETHOD(SetBool)(THIS_ D3DXHANDLE parameter, BOOL b) PURE;
    STDMETHOD(GetBool)(THIS_ D3DXHANDLE parameter, BOOL* b) PURE;
    STDMETHOD(SetBoolArray)(THIS_ D3DXHANDLE parameter, CONST BOOL* b, UINT count) PURE;
    STDMETHOD(GetBoolArray)(THIS_ D3DXHANDLE parameter, BOOL* b, UINT count) PURE;
    STDMETHOD(SetInt)(THIS_ D3DXHANDLE parameter, INT n) PURE;
    STDMETHOD(GetInt)(THIS_ D3DXHANDLE parameter, INT* n) PURE;
    STDMETHOD(SetIntArray)(THIS_ D3DXHANDLE parameter, CONST INT* n, UINT count) PURE;
    STDMETHOD(GetIntArray)(THIS_ D3DXHANDLE parameter, INT* n, UINT count) PURE;
    STDMETHOD(SetFloat)(THIS_ D3DXHANDLE parameter, FLOAT f) PURE;
    STDMETHOD(GetFloat)(THIS_ D3DXHANDLE parameter, FLOAT* f) PURE;
    STDMETHOD(SetFloatArray)(THIS_ D3DXHANDLE parameter, CONST FLOAT* f, UINT count) PURE;
    STDMETHOD(GetFloatArray)(THIS_ D3DXHANDLE parameter, FLOAT* f, UINT count) PURE;
    STDMETHOD(SetVector)(THIS_ D3DXHANDLE parameter, CONST D3DXVECTOR4* vector) PURE;
    STDMETHOD(GetVector)(THIS_ D3DXHANDLE parameter, D3DXVECTOR4* vector) PURE;
    STDMETHOD(SetVectorArray)(THIS_ D3DXHANDLE parameter, CONST D3DXVECTOR4* vector, UINT count) PURE;
    STDMETHOD(GetVectorArray)(THIS_ D3DXHANDLE parameter, D3DXVECTOR4* vector, UINT count) PURE;
    STDMETHOD(SetMatrix)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX* matrix) PURE;
    STDMETHOD(GetMatrix)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix) PURE;
    STDMETHOD(SetMatrixArray)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX* matrix, UINT count) PURE;
    STDMETHOD(GetMatrixArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix, UINT count) PURE;
    STDMETHOD(SetMatrixPointerArray)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX** matrix, UINT count) PURE;
    STDMETHOD(GetMatrixPointerArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX** matrix, UINT count) PURE;
    STDMETHOD(SetMatrixTranspose)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX* matrix) PURE;
    STDMETHOD(GetMatrixTranspose)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix) PURE;
    STDMETHOD(SetMatrixTransposeArray)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX* matrix, UINT count) PURE;
    STDMETHOD(GetMatrixTransposeArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX* matrix, UINT count) PURE;
    STDMETHOD(SetMatrixTransposePointerArray)(THIS_ D3DXHANDLE parameter, CONST D3DXMATRIX** matrix, UINT count) PURE;
    STDMETHOD(GetMatrixTransposePointerArray)(THIS_ D3DXHANDLE parameter, D3DXMATRIX** matrix, UINT count) PURE;
    STDMETHOD(SetString)(THIS_ D3DXHANDLE parameter, LPCSTR string) PURE;
    STDMETHOD(GetString)(THIS_ D3DXHANDLE parameter, LPCSTR* string) PURE;
    STDMETHOD(SetTexture)(THIS_ D3DXHANDLE parameter, LPDIRECT3DBASETEXTURE9 texture) PURE;
    STDMETHOD(GetTexture)(THIS_ D3DXHANDLE parameter, LPDIRECT3DBASETEXTURE9* texture) PURE;
    STDMETHOD(SetPixelShader)(THIS_ D3DXHANDLE parameter, LPDIRECT3DPIXELSHADER9 pshader) PURE;
    STDMETHOD(GetPixelShader)(THIS_ D3DXHANDLE parameter, LPDIRECT3DPIXELSHADER9* pshader) PURE;
    STDMETHOD(SetVertexShader)(THIS_ D3DXHANDLE parameter, LPDIRECT3DVERTEXSHADER9 vshader) PURE;
    STDMETHOD(GetVertexShader)(THIS_ D3DXHANDLE parameter, LPDIRECT3DVERTEXSHADER9* vshader) PURE;
    STDMETHOD(SetArrayRange)(THIS_ D3DXHANDLE parameter, UINT start, UINT end) PURE;
    /*** ID3DXEffectCompiler methods ***/
    STDMETHOD(SetLiteral)(THIS_ D3DXHANDLE parameter, BOOL literal) PURE;
    STDMETHOD(GetLiteral)(THIS_ D3DXHANDLE parameter, BOOL* literal) PURE;
    STDMETHOD(CompileEffect)(THIS_ DWORD flags, LPD3DXBUFFER* effect, LPD3DXBUFFER* error_msgs) PURE;
    STDMETHOD(CompileShader)(THIS_ D3DXHANDLE function, LPCSTR target, DWORD flags, LPD3DXBUFFER* shader,
        LPD3DXBUFFER* error_msgs, LPD3DXCONSTANTTABLE* constant_table) PURE;
};

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI D3DXCreateEffectPool(LPD3DXEFFECTPOOL* pool);

HRESULT WINAPI D3DXCreateEffect(LPDIRECT3DDEVICE9 device,
                                LPCVOID srcdata,
                                UINT srcdatalen,
                                CONST D3DXMACRO* defines,
                                LPD3DXINCLUDE include,
                                DWORD flags,
                                LPD3DXEFFECTPOOL pool,
                                LPD3DXEFFECT* effect,
                                LPD3DXBUFFER* compilation_errors);

HRESULT WINAPI D3DXCreateEffectEx(LPDIRECT3DDEVICE9 device,
                                  LPCVOID srcdata,
                                  UINT srcdatalen,
                                  CONST D3DXMACRO* defines,
                                  LPD3DXINCLUDE include,
                                  LPCSTR skip_constants,
                                  DWORD flags,
                                  LPD3DXEFFECTPOOL pool,
                                  LPD3DXEFFECT* effect,
                                  LPD3DXBUFFER* compilation_errors);

HRESULT WINAPI D3DXCreateEffectCompiler(LPCSTR srcdata,
                                        UINT srcdatalen,
                                        CONST D3DXMACRO* defines,
                                        LPD3DXINCLUDE include,
                                        DWORD flags,
                                        LPD3DXEFFECTCOMPILER* compiler,
                                        LPD3DXBUFFER* parse_errors);

#ifdef __cplusplus
}
#endif

#endif /* __D3DX9EFFECT_H__ */
