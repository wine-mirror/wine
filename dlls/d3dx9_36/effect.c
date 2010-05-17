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

#include "config.h"
#include "wine/port.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "wingdi.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

static const struct ID3DXEffectVtbl ID3DXEffect_Vtbl;

typedef struct ID3DXEffectImpl {
    const ID3DXEffectVtbl *lpVtbl;
    LONG ref;
} ID3DXEffectImpl;

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXEffectImpl_QueryInterface(ID3DXEffect* iface, REFIID riid, void** object)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXBaseEffect) ||
        IsEqualGUID(riid, &IID_ID3DXEffect))
    {
        This->lpVtbl->AddRef(iface);
        *object = This;
        return S_OK;
    }

    ERR("Interface %s not found\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXEffectImpl_AddRef(ID3DXEffect* iface)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    TRACE("(%p)->(): AddRef from %u\n", This, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXEffectImpl_Release(ID3DXEffect* iface)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Release from %u\n", This, ref + 1);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** ID3DXBaseEffect methods ***/
static HRESULT WINAPI ID3DXEffectImpl_GetDesc(ID3DXEffect* iface, D3DXEFFECT_DESC* desc)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, desc);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetParameterDesc(ID3DXEffect* iface, D3DXHANDLE parameter, D3DXPARAMETER_DESC* desc)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, desc);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetTechniqueDesc(ID3DXEffect* iface, D3DXHANDLE technique, D3DXTECHNIQUE_DESC* desc)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, technique, desc);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetPassDesc(ID3DXEffect* iface, D3DXHANDLE pass, D3DXPASS_DESC* desc)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, pass, desc);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetFunctionDesc(ID3DXEffect* iface, D3DXHANDLE shader, D3DXFUNCTION_DESC* desc)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, shader, desc);

    return E_NOTIMPL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetParameter(ID3DXEffect* iface, D3DXHANDLE parameter, UINT index)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %u): stub\n", This, parameter, index);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetParameterByName(ID3DXEffect* iface, D3DXHANDLE parameter, LPCSTR name)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %s): stub\n", This, parameter, name);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetParameterBySemantic(ID3DXEffect* iface, D3DXHANDLE parameter, LPCSTR semantic)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %s): stub\n", This, parameter, semantic);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetParameterElement(ID3DXEffect* iface, D3DXHANDLE parameter, UINT index)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %u): stub\n", This, parameter, index);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetTechnique(ID3DXEffect* iface, UINT index)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%u): stub\n", This, index);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetTechniqueByName(ID3DXEffect* iface, LPCSTR name)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%s): stub\n", This, name);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetPass(ID3DXEffect* iface, D3DXHANDLE technique, UINT index)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %u): stub\n", This, technique, index);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetPassByName(ID3DXEffect* iface, D3DXHANDLE technique, LPCSTR name)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %s): stub\n", This, technique, name);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetFunction(ID3DXEffect* iface, UINT index)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%u): stub\n", This, index);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetFunctionByName(ID3DXEffect* iface, LPCSTR name)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%s): stub\n", This, name);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetAnnotation(ID3DXEffect* iface, D3DXHANDLE object, UINT index)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %u): stub\n", This, object, index);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetAnnotationByName(ID3DXEffect* iface, D3DXHANDLE object, LPCSTR name)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %s): stub\n", This, object, name);

    return NULL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetValue(ID3DXEffect* iface, D3DXHANDLE parameter, LPCVOID data, UINT bytes)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, data, bytes);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetValue(ID3DXEffect* iface, D3DXHANDLE parameter, LPVOID data, UINT bytes)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, data, bytes);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetBool(ID3DXEffect* iface, D3DXHANDLE parameter, BOOL b)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %u): stub\n", This, parameter, b);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetBool(ID3DXEffect* iface, D3DXHANDLE parameter, BOOL* b)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, b);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetBoolArray(ID3DXEffect* iface, D3DXHANDLE parameter, CONST BOOL* b, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, b, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetBoolArray(ID3DXEffect* iface, D3DXHANDLE parameter, BOOL* b, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, b, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetInt(ID3DXEffect* iface, D3DXHANDLE parameter, INT n)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %d): stub\n", This, parameter, n);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetInt(ID3DXEffect* iface, D3DXHANDLE parameter, INT* n)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, n);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetIntArray(ID3DXEffect* iface, D3DXHANDLE parameter, CONST INT* n, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, n, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetIntArray(ID3DXEffect* iface, D3DXHANDLE parameter, INT* n, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, n, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetFloat(ID3DXEffect* iface, D3DXHANDLE parameter, FLOAT f)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %f): stub\n", This, parameter, f);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetFloat(ID3DXEffect* iface, D3DXHANDLE parameter, FLOAT* f)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, f);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetFloatArray(ID3DXEffect* iface, D3DXHANDLE parameter, CONST FLOAT* f, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, f, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetFloatArray(ID3DXEffect* iface, D3DXHANDLE parameter, FLOAT* f, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, f, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetVector(ID3DXEffect* iface, D3DXHANDLE parameter, CONST D3DXVECTOR4* vector)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetVector(ID3DXEffect* iface, D3DXHANDLE parameter, D3DXVECTOR4* vector)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetVectorArray(ID3DXEffect* iface, D3DXHANDLE parameter, CONST D3DXVECTOR4* vector, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, vector, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetVectorArray(ID3DXEffect* iface, D3DXHANDLE parameter, D3DXVECTOR4* vector, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, vector, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrix(ID3DXEffect* iface, D3DXHANDLE parameter, CONST D3DXMATRIX* matrix)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrix(ID3DXEffect* iface, D3DXHANDLE parameter, D3DXMATRIX* matrix)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixArray(ID3DXEffect* iface, D3DXHANDLE parameter, CONST D3DXMATRIX* matrix, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixArray(ID3DXEffect* iface, D3DXHANDLE parameter, D3DXMATRIX* matrix, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixPointerArray(ID3DXEffect* iface, D3DXHANDLE parameter, CONST D3DXMATRIX** matrix, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixPointerArray(ID3DXEffect* iface, D3DXHANDLE parameter, D3DXMATRIX** matrix, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixTranspose(ID3DXEffect* iface, D3DXHANDLE parameter, CONST D3DXMATRIX* matrix)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixTranspose(ID3DXEffect* iface, D3DXHANDLE parameter, D3DXMATRIX* matrix)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixTransposeArray(ID3DXEffect* iface, D3DXHANDLE parameter, CONST D3DXMATRIX* matrix, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixTransposeArray(ID3DXEffect* iface, D3DXHANDLE parameter, D3DXMATRIX* matrix, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixTransposePointerArray(ID3DXEffect* iface, D3DXHANDLE parameter, CONST D3DXMATRIX** matrix, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixTransposePointerArray(ID3DXEffect* iface, D3DXHANDLE parameter, D3DXMATRIX** matrix, UINT count)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u): stub\n", This, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetString(ID3DXEffect* iface, D3DXHANDLE parameter, LPCSTR string)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, string);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetString(ID3DXEffect* iface, D3DXHANDLE parameter, LPCSTR* string)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, string);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetTexture(ID3DXEffect* iface, D3DXHANDLE parameter, LPDIRECT3DBASETEXTURE9 texture)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetTexture(ID3DXEffect* iface, D3DXHANDLE parameter, LPDIRECT3DBASETEXTURE9* texture)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetPixelShader(ID3DXEffect* iface, D3DXHANDLE parameter, LPDIRECT3DPIXELSHADER9* pshader)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, pshader);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetVertexShader(ID3DXEffect* iface, D3DXHANDLE parameter, LPDIRECT3DVERTEXSHADER9* vshader)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, vshader);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetArrayRange(ID3DXEffect* iface, D3DXHANDLE parameter, UINT start, UINT end)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %u, %u): stub\n", This, parameter, start, end);

    return E_NOTIMPL;
}

/*** ID3DXEffect methods ***/
static HRESULT WINAPI ID3DXEffectImpl_GetPool(ID3DXEffect* iface, LPD3DXEFFECTPOOL* pool)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, pool);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetTechnique(ID3DXEffect* iface, D3DXHANDLE technique)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, technique);

    return E_NOTIMPL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetCurrentTechnique(ID3DXEffect* iface)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return NULL;
}

static HRESULT WINAPI ID3DXEffectImpl_ValidateTechnique(ID3DXEffect* iface, D3DXHANDLE technique)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, technique);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXEffectImpl_FindNextValidTechnique(ID3DXEffect* iface, D3DXHANDLE technique, D3DXHANDLE* next_technique)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, technique, next_technique);

    return E_NOTIMPL;
}

static BOOL WINAPI ID3DXEffectImpl_IsParameterUsed(ID3DXEffect* iface, D3DXHANDLE parameter, D3DXHANDLE technique)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, technique);

    return FALSE;
}

static HRESULT WINAPI ID3DXEffectImpl_Begin(ID3DXEffect* iface, UINT *passes, DWORD flags)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %#x): stub\n", This, passes, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_BeginPass(ID3DXEffect* iface, UINT pass)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%u): stub\n", This, pass);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_CommitChanges(ID3DXEffect* iface)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_EndPass(ID3DXEffect* iface)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_End(ID3DXEffect* iface)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetDevice(ID3DXEffect* iface, LPDIRECT3DDEVICE9* device)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, device);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_OnLostDevice(ID3DXEffect* iface)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_OnResetDevice(ID3DXEffect* iface)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetStateManager(ID3DXEffect* iface, LPD3DXEFFECTSTATEMANAGER manager)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, manager);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_GetStateManager(ID3DXEffect* iface, LPD3DXEFFECTSTATEMANAGER* manager)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, manager);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_BeginParameterBlock(ID3DXEffect* iface)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_EndParameterBlock(ID3DXEffect* iface)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return NULL;
}

static HRESULT WINAPI ID3DXEffectImpl_ApplyParameterBlock(ID3DXEffect* iface, D3DXHANDLE parameter_block)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, parameter_block);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_DeleteParameterBlock(ID3DXEffect* iface, D3DXHANDLE parameter_block)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, parameter_block);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_CloneEffect(ID3DXEffect* iface, LPDIRECT3DDEVICE9 device, LPD3DXEFFECT* effect)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p): stub\n", This, device, effect);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetRawValue(ID3DXEffect* iface, D3DXHANDLE parameter, LPCVOID data, UINT byte_offset, UINT bytes)
{
    ID3DXEffectImpl *This = (ID3DXEffectImpl *)iface;

    FIXME("(%p)->(%p, %p, %u, %u): stub\n", This, parameter, data, byte_offset, bytes);

    return E_NOTIMPL;
}

static const struct ID3DXEffectVtbl ID3DXEffect_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXEffectImpl_QueryInterface,
    ID3DXEffectImpl_AddRef,
    ID3DXEffectImpl_Release,
    /*** ID3DXBaseEffect methods ***/
    ID3DXEffectImpl_GetDesc,
    ID3DXEffectImpl_GetParameterDesc,
    ID3DXEffectImpl_GetTechniqueDesc,
    ID3DXEffectImpl_GetPassDesc,
    ID3DXEffectImpl_GetFunctionDesc,
    ID3DXEffectImpl_GetParameter,
    ID3DXEffectImpl_GetParameterByName,
    ID3DXEffectImpl_GetParameterBySemantic,
    ID3DXEffectImpl_GetParameterElement,
    ID3DXEffectImpl_GetTechnique,
    ID3DXEffectImpl_GetTechniqueByName,
    ID3DXEffectImpl_GetPass,
    ID3DXEffectImpl_GetPassByName,
    ID3DXEffectImpl_GetFunction,
    ID3DXEffectImpl_GetFunctionByName,
    ID3DXEffectImpl_GetAnnotation,
    ID3DXEffectImpl_GetAnnotationByName,
    ID3DXEffectImpl_SetValue,
    ID3DXEffectImpl_GetValue,
    ID3DXEffectImpl_SetBool,
    ID3DXEffectImpl_GetBool,
    ID3DXEffectImpl_SetBoolArray,
    ID3DXEffectImpl_GetBoolArray,
    ID3DXEffectImpl_SetInt,
    ID3DXEffectImpl_GetInt,
    ID3DXEffectImpl_SetIntArray,
    ID3DXEffectImpl_GetIntArray,
    ID3DXEffectImpl_SetFloat,
    ID3DXEffectImpl_GetFloat,
    ID3DXEffectImpl_SetFloatArray,
    ID3DXEffectImpl_GetFloatArray,
    ID3DXEffectImpl_SetVector,
    ID3DXEffectImpl_GetVector,
    ID3DXEffectImpl_SetVectorArray,
    ID3DXEffectImpl_GetVectorArray,
    ID3DXEffectImpl_SetMatrix,
    ID3DXEffectImpl_GetMatrix,
    ID3DXEffectImpl_SetMatrixArray,
    ID3DXEffectImpl_GetMatrixArray,
    ID3DXEffectImpl_SetMatrixPointerArray,
    ID3DXEffectImpl_GetMatrixPointerArray,
    ID3DXEffectImpl_SetMatrixTranspose,
    ID3DXEffectImpl_GetMatrixTranspose,
    ID3DXEffectImpl_SetMatrixTransposeArray,
    ID3DXEffectImpl_GetMatrixTransposeArray,
    ID3DXEffectImpl_SetMatrixTransposePointerArray,
    ID3DXEffectImpl_GetMatrixTransposePointerArray,
    ID3DXEffectImpl_SetString,
    ID3DXEffectImpl_GetString,
    ID3DXEffectImpl_SetTexture,
    ID3DXEffectImpl_GetTexture,
    ID3DXEffectImpl_GetPixelShader,
    ID3DXEffectImpl_GetVertexShader,
    ID3DXEffectImpl_SetArrayRange,
    /*** ID3DXEffect methods ***/
    ID3DXEffectImpl_GetPool,
    ID3DXEffectImpl_SetTechnique,
    ID3DXEffectImpl_GetCurrentTechnique,
    ID3DXEffectImpl_ValidateTechnique,
    ID3DXEffectImpl_FindNextValidTechnique,
    ID3DXEffectImpl_IsParameterUsed,
    ID3DXEffectImpl_Begin,
    ID3DXEffectImpl_BeginPass,
    ID3DXEffectImpl_CommitChanges,
    ID3DXEffectImpl_EndPass,
    ID3DXEffectImpl_End,
    ID3DXEffectImpl_GetDevice,
    ID3DXEffectImpl_OnLostDevice,
    ID3DXEffectImpl_OnResetDevice,
    ID3DXEffectImpl_SetStateManager,
    ID3DXEffectImpl_GetStateManager,
    ID3DXEffectImpl_BeginParameterBlock,
    ID3DXEffectImpl_EndParameterBlock,
    ID3DXEffectImpl_ApplyParameterBlock,
    ID3DXEffectImpl_DeleteParameterBlock,
    ID3DXEffectImpl_CloneEffect,
    ID3DXEffectImpl_SetRawValue
};

HRESULT WINAPI D3DXCreateEffectEx(LPDIRECT3DDEVICE9 device,
                                  LPCVOID srcdata,
                                  UINT srcdatalen,
                                  CONST D3DXMACRO* defines,
                                  LPD3DXINCLUDE include,
                                  LPCSTR skip_constants,
                                  DWORD flags,
                                  LPD3DXEFFECTPOOL pool,
                                  LPD3DXEFFECT* effect,
                                  LPD3DXBUFFER* compilation_errors)
{
    ID3DXEffectImpl* object;

    FIXME("(%p, %p, %u, %p, %p, %p, %#x, %p, %p, %p): semi-stub\n", device, srcdata, srcdatalen, defines, include,
        skip_constants, flags, pool, effect, compilation_errors);

    if (!device || !srcdata)
        return D3DERR_INVALIDCALL;

    if (!srcdatalen)
        return E_FAIL;

    /* Native dll allows effect to be null so just return D3D_OK after doing basic checks */
    if (!effect)
        return D3D_OK;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ID3DXEffectImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->lpVtbl = &ID3DXEffect_Vtbl;
    object->ref = 1;

    *effect = (LPD3DXEFFECT)object;

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateEffect(LPDIRECT3DDEVICE9 device,
                                LPCVOID srcdata,
                                UINT srcdatalen,
                                CONST D3DXMACRO* defines,
                                LPD3DXINCLUDE include,
                                DWORD flags,
                                LPD3DXEFFECTPOOL pool,
                                LPD3DXEFFECT* effect,
                                LPD3DXBUFFER* compilation_errors)
{
    TRACE("(%p, %p, %u, %p, %p, %#x, %p, %p, %p): Forwarded to D3DXCreateEffectEx\n", device, srcdata, srcdatalen, defines,
        include, flags, pool, effect, compilation_errors);

    return D3DXCreateEffectEx(device, srcdata, srcdatalen, defines, include, NULL, flags, pool, effect, compilation_errors);
}

HRESULT WINAPI D3DXCreateEffectCompiler(LPCSTR srcdata,
                                        UINT srcdatalen,
                                        CONST D3DXMACRO* defines,
                                        LPD3DXINCLUDE include,
                                        DWORD flags,
                                        LPD3DXEFFECTCOMPILER* compiler,
                                        LPD3DXBUFFER* parse_errors)
{
    FIXME("(%p, %u, %p, %p, %#x, %p, %p): stub\n", srcdata, srcdatalen, defines, include, flags, compiler, parse_errors);

    return E_NOTIMPL;
}

static const struct ID3DXEffectPoolVtbl ID3DXEffectPool_Vtbl;

typedef struct ID3DXEffectPoolImpl {
    const ID3DXEffectPoolVtbl *lpVtbl;
    LONG ref;
} ID3DXEffectPoolImpl;

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXEffectPoolImpl_QueryInterface(ID3DXEffectPool* iface, REFIID riid, void** object)
{
    ID3DXEffectPoolImpl *This = (ID3DXEffectPoolImpl *)iface;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXEffectPool))
    {
        This->lpVtbl->AddRef(iface);
        *object = This;
        return S_OK;
    }

    WARN("Interface %s not found\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXEffectPoolImpl_AddRef(ID3DXEffectPool* iface)
{
    ID3DXEffectPoolImpl *This = (ID3DXEffectPoolImpl *)iface;

    TRACE("(%p)->(): AddRef from %u\n", This, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXEffectPoolImpl_Release(ID3DXEffectPool* iface)
{
    ID3DXEffectPoolImpl *This = (ID3DXEffectPoolImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Release from %u\n", This, ref + 1);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static const struct ID3DXEffectPoolVtbl ID3DXEffectPool_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXEffectPoolImpl_QueryInterface,
    ID3DXEffectPoolImpl_AddRef,
    ID3DXEffectPoolImpl_Release
};

HRESULT WINAPI D3DXCreateEffectPool(LPD3DXEFFECTPOOL* pool)
{
    ID3DXEffectPoolImpl* object;

    TRACE("(%p)\n", pool);

    if (!pool)
        return D3DERR_INVALIDCALL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ID3DXEffectImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->lpVtbl = &ID3DXEffectPool_Vtbl;
    object->ref = 1;

    *pool = (LPD3DXEFFECTPOOL)object;

    return S_OK;
}

HRESULT WINAPI D3DXCreateEffectFromFileExW(LPDIRECT3DDEVICE9 device, LPCWSTR srcfile,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, LPCSTR skipconstants, DWORD flags,
    LPD3DXEFFECTPOOL pool, LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    LPVOID buffer;
    HRESULT ret;
    DWORD size;

    TRACE("(%s): relay\n", debugstr_w(srcfile));

    if (!device || !srcfile || !defines)
        return D3DERR_INVALIDCALL;

    ret = map_view_of_file(srcfile, &buffer, &size);

    if (FAILED(ret))
        return D3DXERR_INVALIDDATA;

    ret = D3DXCreateEffectEx(device, buffer, size, defines, include, skipconstants, flags, pool, effect, compilationerrors);
    UnmapViewOfFile(buffer);

    return ret;
}

HRESULT WINAPI D3DXCreateEffectFromFileExA(LPDIRECT3DDEVICE9 device, LPCSTR srcfile,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, LPCSTR skipconstants, DWORD flags,
    LPD3DXEFFECTPOOL pool, LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    LPWSTR srcfileW;
    HRESULT ret;
    DWORD len;

    TRACE("(void): relay\n");

    if (!srcfile || !defines)
        return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, srcfile, -1, NULL, 0);
    srcfileW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, srcfile, -1, srcfileW, len);

    ret = D3DXCreateEffectFromFileExW(device, srcfileW, defines, include, skipconstants, flags, pool, effect, compilationerrors);
    HeapFree(GetProcessHeap(), 0, srcfileW);

    return ret;
}

HRESULT WINAPI D3DXCreateEffectFromFileW(LPDIRECT3DDEVICE9 device, LPCWSTR srcfile,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, DWORD flags, LPD3DXEFFECTPOOL pool,
    LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    TRACE("(void): relay\n");
    return D3DXCreateEffectFromFileExW(device, srcfile, defines, include, NULL, flags, pool, effect, compilationerrors);
}

HRESULT WINAPI D3DXCreateEffectFromFileA(LPDIRECT3DDEVICE9 device, LPCSTR srcfile,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, DWORD flags, LPD3DXEFFECTPOOL pool,
    LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    TRACE("(void): relay\n");
    return D3DXCreateEffectFromFileExA(device, srcfile, defines, include, NULL, flags, pool, effect, compilationerrors);
}

HRESULT WINAPI D3DXCreateEffectFromResourceExW(LPDIRECT3DDEVICE9 device, HMODULE srcmodule, LPCWSTR srcresource,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, LPCSTR skipconstants, DWORD flags,
    LPD3DXEFFECTPOOL pool, LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    HRSRC resinfo;

    TRACE("(%p, %s): relay\n", srcmodule, debugstr_w(srcresource));

    if (!device || !defines)
        return D3DERR_INVALIDCALL;

    resinfo = FindResourceW(srcmodule, srcresource, (LPCWSTR) RT_RCDATA);

    if (resinfo)
    {
        LPVOID buffer;
        HRESULT ret;
        DWORD size;

        ret = load_resource_into_memory(srcmodule, resinfo, &buffer, &size);

        if (FAILED(ret))
            return D3DXERR_INVALIDDATA;

        return D3DXCreateEffectEx(device, buffer, size, defines, include, skipconstants, flags, pool, effect, compilationerrors);
    }

    return D3DXERR_INVALIDDATA;
}

HRESULT WINAPI D3DXCreateEffectFromResourceExA(LPDIRECT3DDEVICE9 device, HMODULE srcmodule, LPCSTR srcresource,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, LPCSTR skipconstants, DWORD flags,
    LPD3DXEFFECTPOOL pool, LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    HRSRC resinfo;

    TRACE("(%p, %s): relay\n", srcmodule, debugstr_a(srcresource));

    if (!device || !defines)
        return D3DERR_INVALIDCALL;

    resinfo = FindResourceA(srcmodule, srcresource, (LPCSTR) RT_RCDATA);

    if (resinfo)
    {
        LPVOID buffer;
        HRESULT ret;
        DWORD size;

        ret = load_resource_into_memory(srcmodule, resinfo, &buffer, &size);

        if (FAILED(ret))
            return D3DXERR_INVALIDDATA;

        return D3DXCreateEffectEx(device, buffer, size, defines, include, skipconstants, flags, pool, effect, compilationerrors);
    }

    return D3DXERR_INVALIDDATA;
}

HRESULT WINAPI D3DXCreateEffectFromResourceW(LPDIRECT3DDEVICE9 device, HMODULE srcmodule, LPCWSTR srcresource,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, DWORD flags, LPD3DXEFFECTPOOL pool,
    LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    TRACE("(void): relay\n");
    return D3DXCreateEffectFromResourceExW(device, srcmodule, srcresource, defines, include, NULL, flags, pool, effect, compilationerrors);
}

HRESULT WINAPI D3DXCreateEffectFromResourceA(LPDIRECT3DDEVICE9 device, HMODULE srcmodule, LPCSTR srcresource,
    const D3DXMACRO *defines, LPD3DXINCLUDE include, DWORD flags, LPD3DXEFFECTPOOL pool,
    LPD3DXEFFECT *effect, LPD3DXBUFFER *compilationerrors)
{
    TRACE("(void): relay\n");
    return D3DXCreateEffectFromResourceExA(device, srcmodule, srcresource, defines, include, NULL, flags, pool, effect, compilationerrors);
}

HRESULT WINAPI D3DXCreateEffectCompilerFromFileW(LPCWSTR srcfile, const D3DXMACRO *defines, LPD3DXINCLUDE include,
    DWORD flags, LPD3DXEFFECTCOMPILER *effectcompiler, LPD3DXBUFFER *parseerrors)
{
    LPVOID buffer;
    HRESULT ret;
    DWORD size;

    TRACE("(%s): relay\n", debugstr_w(srcfile));

    if (!srcfile || !defines)
        return D3DERR_INVALIDCALL;

    ret = map_view_of_file(srcfile, &buffer, &size);

    if (FAILED(ret))
        return D3DXERR_INVALIDDATA;

    ret = D3DXCreateEffectCompiler(buffer, size, defines, include, flags, effectcompiler, parseerrors);
    UnmapViewOfFile(buffer);

    return ret;
}

HRESULT WINAPI D3DXCreateEffectCompilerFromFileA(LPCSTR srcfile, const D3DXMACRO *defines, LPD3DXINCLUDE include,
    DWORD flags, LPD3DXEFFECTCOMPILER *effectcompiler, LPD3DXBUFFER *parseerrors)
{
    LPWSTR srcfileW;
    HRESULT ret;
    DWORD len;

    TRACE("(void): relay\n");

    if (!srcfile || !defines)
        return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, srcfile, -1, NULL, 0);
    srcfileW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, srcfile, -1, srcfileW, len);

    ret = D3DXCreateEffectCompilerFromFileW(srcfileW, defines, include, flags, effectcompiler, parseerrors);
    HeapFree(GetProcessHeap(), 0, srcfileW);

    return ret;
}

HRESULT WINAPI D3DXCreateEffectCompilerFromResourceA(HMODULE srcmodule, LPCSTR srcresource, const D3DXMACRO *defines,
    LPD3DXINCLUDE include, DWORD flags, LPD3DXEFFECTCOMPILER *effectcompiler, LPD3DXBUFFER *parseerrors)
{
    HRSRC resinfo;

    TRACE("(%p, %s): relay\n", srcmodule, debugstr_a(srcresource));

    resinfo = FindResourceA(srcmodule, srcresource, (LPCSTR) RT_RCDATA);

    if (resinfo)
    {
        LPVOID buffer;
        HRESULT ret;
        DWORD size;

        ret = load_resource_into_memory(srcmodule, resinfo, &buffer, &size);

        if (FAILED(ret))
            return D3DXERR_INVALIDDATA;

        return D3DXCreateEffectCompiler(buffer, size, defines, include, flags, effectcompiler, parseerrors);
    }

    return D3DXERR_INVALIDDATA;
}

HRESULT WINAPI D3DXCreateEffectCompilerFromResourceW(HMODULE srcmodule, LPCWSTR srcresource, const D3DXMACRO *defines,
    LPD3DXINCLUDE include, DWORD flags, LPD3DXEFFECTCOMPILER *effectcompiler, LPD3DXBUFFER *parseerrors)
{
    HRSRC resinfo;

    TRACE("(%p, %s): relay\n", srcmodule, debugstr_w(srcresource));

    resinfo = FindResourceW(srcmodule, srcresource, (LPCWSTR) RT_RCDATA);

    if (resinfo)
    {
        LPVOID buffer;
        HRESULT ret;
        DWORD size;

        ret = load_resource_into_memory(srcmodule, resinfo, &buffer, &size);

        if (FAILED(ret))
            return D3DXERR_INVALIDDATA;

        return D3DXCreateEffectCompiler(buffer, size, defines, include, flags, effectcompiler, parseerrors);
    }

    return D3DXERR_INVALIDDATA;
}
