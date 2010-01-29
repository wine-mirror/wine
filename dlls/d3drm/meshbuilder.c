/*
 * Implementation of IDirect3DRMMeshBuilder Interface
 *
 * Copyright 2010 Christian Costa
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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

#include "wine/debug.h"

#define COBJMACROS

#include "winbase.h"
#include "wingdi.h"

#include "d3drm_private.h"
#include "d3drm.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3drm);

typedef struct {
    const IDirect3DRMMeshBuilderVtbl *lpVtbl;
    LONG ref;
} IDirect3DRMMeshBuilderImpl;

static const struct IDirect3DRMMeshBuilderVtbl Direct3DRMMeshBuilder_Vtbl;

HRESULT Direct3DRMMeshBuilder_create(LPDIRECT3DRMMESHBUILDER* ppMeshBuilder)
{
    IDirect3DRMMeshBuilderImpl* object;

    TRACE("(%p)\n", ppMeshBuilder);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DRMMeshBuilderImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->lpVtbl = &Direct3DRMMeshBuilder_Vtbl;
    object->ref = 1;

    *ppMeshBuilder = (IDirect3DRMMeshBuilder*)object;

    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_QueryInterface(IDirect3DRMMeshBuilder* iface, REFIID riid, void** ppvObject)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirect3DRMMeshBuilder))
    {
        IClassFactory_AddRef(iface);
        *ppvObject = This;
        return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DRMMeshBuilderImpl_AddRef(IDirect3DRMMeshBuilder* iface)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IDirect3DRMMeshBuilderImpl_Release(IDirect3DRMMeshBuilder* iface)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)\n", This);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_Clone(IDirect3DRMMeshBuilder* iface, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p,%s,%p): stub\n", This, pUnkOuter, debugstr_guid(riid), ppvObj);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_AddDestroyCallback(IDirect3DRMMeshBuilder* iface, D3DRMOBJECTCALLBACK cb, LPVOID argument)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p,%p): stub\n", This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_DeleteDestroyCallback(IDirect3DRMMeshBuilder* iface, D3DRMOBJECTCALLBACK cb, LPVOID argument)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p,%p): stub\n", This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetAppData(IDirect3DRMMeshBuilder* iface, DWORD data)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%u): stub\n", This, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMMeshBuilderImpl_GetAppData(IDirect3DRMMeshBuilder* iface)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetName(IDirect3DRMMeshBuilder* iface, LPCSTR pName)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%s): stub\n", This, pName);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_GetName(IDirect3DRMMeshBuilder* iface, LPDWORD lpdwSize, LPSTR lpName)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p,%p): stub\n", This, lpdwSize, lpName);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_GetClassName(IDirect3DRMMeshBuilder* iface, LPDWORD lpdwSize, LPSTR lpName)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p,%p): stub\n", This, lpdwSize, lpName);

    return E_NOTIMPL;
}

/*** IDirect3DRMMeshBuilder methods ***/
static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_Load(IDirect3DRMMeshBuilder* iface, LPVOID filename, LPVOID name, D3DRMLOADOPTIONS loadflags, D3DRMLOADTEXTURECALLBACK cb, LPVOID pArg)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p,%p,%x,%p,%p): stub\n", This, filename, name, loadflags, cb, pArg);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_Save(IDirect3DRMMeshBuilder* iface, const char *filename, D3DRMXOFFORMAT format, D3DRMSAVEOPTIONS save)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%s,%d,%d): stub\n", This, filename, format, save);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_Scale(IDirect3DRMMeshBuilder* iface, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%f,%f,%f): stub\n", This, sx, sy, sz);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_Translate(IDirect3DRMMeshBuilder* iface, D3DVALUE tx, D3DVALUE ty, D3DVALUE tz)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%f,%f,%f): stub\n", This, tx, ty, tz);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetColorSource(IDirect3DRMMeshBuilder* iface, D3DRMCOLORSOURCE color)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%x): stub\n", This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_GetBox(IDirect3DRMMeshBuilder* iface, D3DRMBOX *pBox)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, pBox);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_GenerateNormals(IDirect3DRMMeshBuilder* iface)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static D3DRMCOLORSOURCE WINAPI IDirect3DRMMeshBuilderImpl_GetColorSource(IDirect3DRMMeshBuilder* iface)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_AddMesh(IDirect3DRMMeshBuilder* iface, LPDIRECT3DRMMESH pMesh)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, pMesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_AddMeshBuilder(IDirect3DRMMeshBuilder* iface, LPDIRECT3DRMMESHBUILDER pMeshBuilder)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, pMeshBuilder);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_AddFrame(IDirect3DRMMeshBuilder* iface, LPDIRECT3DRMFRAME pFrame)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, pFrame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_AddFace(IDirect3DRMMeshBuilder* iface, LPDIRECT3DRMFACE pFace)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, pFace);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_AddFaces(IDirect3DRMMeshBuilder* iface, DWORD vcount, D3DVECTOR *vertices, DWORD ncount, D3DVECTOR *normals, DWORD *data, LPDIRECT3DRMFACEARRAY* pFaceArray)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%d,%p,%d,%p,%p,%p): stub\n", This, vcount, vertices, ncount, normals, data, pFaceArray);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_ReserveSpace(IDirect3DRMMeshBuilder* iface, DWORD vertex_Count, DWORD normal_count, DWORD face_count)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%d,%d,%d): stub\n", This, vertex_Count, normal_count, face_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetColorRGB(IDirect3DRMMeshBuilder* iface, D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%f,%f,%f): stub\n", This, red, green, blue);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetColor(IDirect3DRMMeshBuilder* iface, D3DCOLOR color)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%x): stub\n", This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetTexture(IDirect3DRMMeshBuilder* iface, LPDIRECT3DRMTEXTURE pTexture)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, pTexture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetMaterial(IDirect3DRMMeshBuilder* iface, LPDIRECT3DRMMATERIAL pMaterial)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, pMaterial);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetTextureTopology(IDirect3DRMMeshBuilder* iface, BOOL wrap_u, BOOL wrap_v)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%d,%d): stub\n", This, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetQuality(IDirect3DRMMeshBuilder* iface, D3DRMRENDERQUALITY quality)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%d): stub\n", This, quality);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetPerspective(IDirect3DRMMeshBuilder* iface, BOOL enable)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%d): stub\n", This, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetVertex(IDirect3DRMMeshBuilder* iface, DWORD index, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetNormal(IDirect3DRMMeshBuilder* iface, DWORD index, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetTextureCoordinates(IDirect3DRMMeshBuilder* iface, DWORD index, D3DVALUE u, D3DVALUE v)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%f,%f): stub\n", This, u, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetVertexColor(IDirect3DRMMeshBuilder* iface, DWORD index, D3DCOLOR color)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%d,%x): stub\n", This, index, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_SetVertexColorRGB(IDirect3DRMMeshBuilder* iface, DWORD index, D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%d,%f,%f,%f): stub\n", This, index, red, green, blue);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_GetFaces(IDirect3DRMMeshBuilder* iface, LPDIRECT3DRMFACEARRAY* pFaceArray)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, pFaceArray);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_GetVertices(IDirect3DRMMeshBuilder* iface, DWORD *vcount, D3DVECTOR *vertices, DWORD *ncount, D3DVECTOR *normals, DWORD *face_data_size, DWORD *face_data)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p,%p,%p,%p,%p,%p): stub\n", This, vcount, vertices, ncount, normals, face_data_size, face_data);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_GetTextureCoordinates(IDirect3DRMMeshBuilder* iface, DWORD index, D3DVALUE *u, D3DVALUE *v)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%d,%p,%p): stub\n", This, index, u, v);

    return E_NOTIMPL;
}

static int WINAPI IDirect3DRMMeshBuilderImpl_AddVertex(IDirect3DRMMeshBuilder* iface, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return 0;
}

static int WINAPI IDirect3DRMMeshBuilderImpl_AddNormal(IDirect3DRMMeshBuilder* iface, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_CreateFace(IDirect3DRMMeshBuilder* iface, LPDIRECT3DRMFACE* ppFace)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, ppFace);

    return E_NOTIMPL;
}

static D3DRMRENDERQUALITY WINAPI IDirect3DRMMeshBuilderImpl_GetQuality(IDirect3DRMMeshBuilder* iface)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return 0;
}

static BOOL WINAPI IDirect3DRMMeshBuilderImpl_GetPerspective(IDirect3DRMMeshBuilder* iface)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return FALSE;
}

static int WINAPI IDirect3DRMMeshBuilderImpl_GetFaceCount(IDirect3DRMMeshBuilder* iface)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return 0;
}

static int WINAPI IDirect3DRMMeshBuilderImpl_GetVertexCount(IDirect3DRMMeshBuilder* iface)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(): stub\n", This);

    return 0;
}

static D3DCOLOR WINAPI IDirect3DRMMeshBuilderImpl_GetVertexColor(IDirect3DRMMeshBuilder* iface, DWORD index)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%d): stub\n", This, index);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMeshBuilderImpl_CreateMesh(IDirect3DRMMeshBuilder* iface, LPDIRECT3DRMMESH* ppMesh)
{
    IDirect3DRMMeshBuilderImpl *This = (IDirect3DRMMeshBuilderImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, ppMesh);

    return E_NOTIMPL;
}

static const struct IDirect3DRMMeshBuilderVtbl Direct3DRMMeshBuilder_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMMeshBuilderImpl_QueryInterface,
    IDirect3DRMMeshBuilderImpl_AddRef,
    IDirect3DRMMeshBuilderImpl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMMeshBuilderImpl_Clone,
    IDirect3DRMMeshBuilderImpl_AddDestroyCallback,
    IDirect3DRMMeshBuilderImpl_DeleteDestroyCallback,
    IDirect3DRMMeshBuilderImpl_SetAppData,
    IDirect3DRMMeshBuilderImpl_GetAppData,
    IDirect3DRMMeshBuilderImpl_SetName,
    IDirect3DRMMeshBuilderImpl_GetName,
    IDirect3DRMMeshBuilderImpl_GetClassName,
    /*** IDirect3DRMMeshBuilder methods ***/
    IDirect3DRMMeshBuilderImpl_Load,
    IDirect3DRMMeshBuilderImpl_Save,
    IDirect3DRMMeshBuilderImpl_Scale,
    IDirect3DRMMeshBuilderImpl_Translate,
    IDirect3DRMMeshBuilderImpl_SetColorSource,
    IDirect3DRMMeshBuilderImpl_GetBox,
    IDirect3DRMMeshBuilderImpl_GenerateNormals,
    IDirect3DRMMeshBuilderImpl_GetColorSource,
    IDirect3DRMMeshBuilderImpl_AddMesh,
    IDirect3DRMMeshBuilderImpl_AddMeshBuilder,
    IDirect3DRMMeshBuilderImpl_AddFrame,
    IDirect3DRMMeshBuilderImpl_AddFace,
    IDirect3DRMMeshBuilderImpl_AddFaces,
    IDirect3DRMMeshBuilderImpl_ReserveSpace,
    IDirect3DRMMeshBuilderImpl_SetColorRGB,
    IDirect3DRMMeshBuilderImpl_SetColor,
    IDirect3DRMMeshBuilderImpl_SetTexture,
    IDirect3DRMMeshBuilderImpl_SetMaterial,
    IDirect3DRMMeshBuilderImpl_SetTextureTopology,
    IDirect3DRMMeshBuilderImpl_SetQuality,
    IDirect3DRMMeshBuilderImpl_SetPerspective,
    IDirect3DRMMeshBuilderImpl_SetVertex,
    IDirect3DRMMeshBuilderImpl_SetNormal,
    IDirect3DRMMeshBuilderImpl_SetTextureCoordinates,
    IDirect3DRMMeshBuilderImpl_SetVertexColor,
    IDirect3DRMMeshBuilderImpl_SetVertexColorRGB,
    IDirect3DRMMeshBuilderImpl_GetFaces,
    IDirect3DRMMeshBuilderImpl_GetVertices,
    IDirect3DRMMeshBuilderImpl_GetTextureCoordinates,
    IDirect3DRMMeshBuilderImpl_AddVertex,
    IDirect3DRMMeshBuilderImpl_AddNormal,
    IDirect3DRMMeshBuilderImpl_CreateFace,
    IDirect3DRMMeshBuilderImpl_GetQuality,
    IDirect3DRMMeshBuilderImpl_GetPerspective,
    IDirect3DRMMeshBuilderImpl_GetFaceCount,
    IDirect3DRMMeshBuilderImpl_GetVertexCount,
    IDirect3DRMMeshBuilderImpl_GetVertexColor,
    IDirect3DRMMeshBuilderImpl_CreateMesh
};
