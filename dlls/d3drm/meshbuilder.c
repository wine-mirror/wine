/*
 * Implementation of IDirect3DRMMeshBuilderX and IDirect3DRMMesh interfaces
 *
 * Copyright 2010, 2012 Christian Costa
 * Copyright 2011 André Hentschel
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

#define NONAMELESSUNION

#define COBJMACROS

#include "wine/debug.h"

#include "winbase.h"
#include "wingdi.h"
#include "dxfile.h"
#include "rmxfguid.h"

#include "d3drm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3drm);

typedef struct {
    unsigned nb_vertices;
    D3DRMVERTEX* vertices;
    unsigned nb_faces;
    unsigned vertex_per_face;
    DWORD face_data_size;
    unsigned* face_data;
    D3DCOLOR color;
    IDirect3DRMMaterial2* material;
    IDirect3DRMTexture3* texture;
} mesh_group;

typedef struct {
    IDirect3DRMMesh IDirect3DRMMesh_iface;
    LONG ref;
    DWORD groups_capacity;
    DWORD nb_groups;
    mesh_group* groups;
} IDirect3DRMMeshImpl;

typedef struct {
    D3DVALUE u;
    D3DVALUE v;
} Coords2d;

typedef struct {
    IDirect3DRMMeshBuilder2 IDirect3DRMMeshBuilder2_iface;
    IDirect3DRMMeshBuilder3 IDirect3DRMMeshBuilder3_iface;
    LONG ref;
    char* name;
    DWORD nb_vertices;
    D3DVECTOR* pVertices;
    DWORD nb_normals;
    D3DVECTOR* pNormals;
    DWORD nb_faces;
    DWORD face_data_size;
    LPVOID pFaceData;
    DWORD nb_coords2d;
    Coords2d* pCoords2d;
    D3DCOLOR color;
    IDirect3DRMMaterial2* material;
    IDirect3DRMTexture3* texture;
} IDirect3DRMMeshBuilderImpl;

char templates[] = {
"xof 0302txt 0064"
"template Header"
"{"
"<3D82AB43-62DA-11CF-AB39-0020AF71E433>"
"WORD major;"
"WORD minor;"
"DWORD flags;"
"}"
"template Vector"
"{"
"<3D82AB5E-62DA-11CF-AB39-0020AF71E433>"
"FLOAT x;"
"FLOAT y;"
"FLOAT z;"
"}"
"template Coords2d"
"{"
"<F6F23F44-7686-11CF-8F52-0040333594A3>"
"FLOAT u;"
"FLOAT v;"
"}"
"template Matrix4x4"
"{"
"<F6F23F45-7686-11CF-8F52-0040333594A3>"
"array FLOAT matrix[16];"
"}"
"template ColorRGBA"
"{"
"<35FF44E0-6C7C-11CF-8F52-0040333594A3>"
"FLOAT red;"
"FLOAT green;"
"FLOAT blue;"
"FLOAT alpha;"
"}"
"template ColorRGB"
"{"
"<D3E16E81-7835-11CF-8F52-0040333594A3>"
"FLOAT red;"
"FLOAT green;"
"FLOAT blue;"
"}"
"template IndexedColor"
"{"
"<1630B820-7842-11CF-8F52-0040333594A3>"
"DWORD index;"
"ColorRGBA indexColor;"
"}"
"template Boolean"
"{"
"<537DA6A0-CA37-11D0-941C-0080C80CFA7B>"
"DWORD truefalse;"
"}"
"template Boolean2d"
"{"
"<4885AE63-78E8-11CF-8F52-0040333594A3>"
"Boolean u;"
"Boolean v;"
"}"
"template MaterialWrap"
"{"
"<4885AE60-78E8-11CF-8F52-0040333594A3>"
"Boolean u;"
"Boolean v;"
"}"
"template TextureFilename"
"{"
"<A42790E1-7810-11CF-8F52-0040333594A3>"
"STRING filename;"
"}"
"template Material"
"{"
"<3D82AB4D-62DA-11CF-AB39-0020AF71E433>"
"ColorRGBA faceColor;"
"FLOAT power;"
"ColorRGB specularColor;"
"ColorRGB emissiveColor;"
"[...]"
"}"
"template MeshFace"
"{"
"<3D82AB5F-62DA-11CF-AB39-0020AF71E433>"
"DWORD nFaceVertexIndices;"
"array DWORD faceVertexIndices[nFaceVertexIndices];"
"}"
"template MeshFaceWraps"
"{"
"<ED1EC5C0-C0A8-11D0-941C-0080C80CFA7B>"
"DWORD nFaceWrapValues;"
"array Boolean2d faceWrapValues[nFaceWrapValues];"
"}"
"template MeshTextureCoords"
"{"
"<F6F23F40-7686-11CF-8F52-0040333594A3>"
"DWORD nTextureCoords;"
"array Coords2d textureCoords[nTextureCoords];"
"}"
"template MeshMaterialList"
"{"
"<F6F23F42-7686-11CF-8F52-0040333594A3>"
"DWORD nMaterials;"
"DWORD nFaceIndexes;"
"array DWORD faceIndexes[nFaceIndexes];"
"[Material]"
"}"
"template MeshNormals"
"{"
"<F6F23F43-7686-11CF-8F52-0040333594A3>"
"DWORD nNormals;"
"array Vector normals[nNormals];"
"DWORD nFaceNormals;"
"array MeshFace faceNormals[nFaceNormals];"
"}"
"template MeshVertexColors"
"{"
"<1630B821-7842-11CF-8F52-0040333594A3>"
"DWORD nVertexColors;"
"array IndexedColor vertexColors[nVertexColors];"
"}"
"template Mesh"
"{"
"<3D82AB44-62DA-11CF-AB39-0020AF71E433>"
"DWORD nVertices;"
"array Vector vertices[nVertices];"
"DWORD nFaces;"
"array MeshFace faces[nFaces];"
"[...]"
"}"
"template FrameTransformMatrix"
"{"
"<F6F23F41-7686-11CF-8F52-0040333594A3>"
"Matrix4x4 frameMatrix;"
"}"
"template Frame"
"{"
"<3D82AB46-62DA-11CF-AB39-0020AF71E433>"
"[...]"
"}"
"template FloatKeys"
"{"
"<10DD46A9-775B-11CF-8F52-0040333594A3>"
"DWORD nValues;"
"array FLOAT values[nValues];"
"}"
"template TimedFloatKeys"
"{"
"<F406B180-7B3B-11CF-8F52-0040333594A3>"
"DWORD time;"
"FloatKeys tfkeys;"
"}"
"template AnimationKey"
"{"
"<10DD46A8-775B-11CF-8F52-0040333594A3>"
"DWORD keyType;"
"DWORD nKeys;"
"array TimedFloatKeys keys[nKeys];"
"}"
"template AnimationOptions"
"{"
"<E2BF56C0-840F-11CF-8F52-0040333594A3>"
"DWORD openclosed;"
"DWORD positionquality;"
"}"
"template Animation"
"{"
"<3D82AB4F-62DA-11CF-AB39-0020AF71E433>"
"[...]"
"}"
"template AnimationSet"
"{"
"<3D82AB50-62DA-11CF-AB39-0020AF71E433>"
"[Animation]"
"}"
"template InlineData"
"{"
"<3A23EEA0-94B1-11D0-AB39-0020AF71E433>"
"[BINARY]"
"}"
"template Url"
"{"
"<3A23EEA1-94B1-11D0-AB39-0020AF71E433>"
"DWORD nUrls;"
"array STRING urls[nUrls];"
"}"
"template ProgressiveMesh"
"{"
"<8A63C360-997D-11D0-941C-0080C80CFA7B>"
"[Url,InlineData]"
"}"
"template Guid"
"{"
"<A42790E0-7810-11CF-8F52-0040333594A3>"
"DWORD data1;"
"WORD data2;"
"WORD data3;"
"array UCHAR data4[8];"
"}"
"template StringProperty"
"{"
"<7F0F21E0-BFE1-11D1-82C0-00A0C9697271>"
"STRING key;"
"STRING value;"
"}"
"template PropertyBag"
"{"
"<7F0F21E1-BFE1-11D1-82C0-00A0C9697271>"
"[StringProperty]"
"}"
"template ExternalVisual"
"{"
"<98116AA0-BDBA-11D1-82C0-00A0C9697271>"
"Guid guidExternalVisual;"
"[...]"
"}"
"template RightHanded"
"{"
"<7F5D5EA0-D53A-11D1-82C0-00A0C9697271>"
"DWORD bRightHanded;"
"}"
};

static inline IDirect3DRMMeshImpl *impl_from_IDirect3DRMMesh(IDirect3DRMMesh *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMMeshImpl, IDirect3DRMMesh_iface);
}

static inline IDirect3DRMMeshBuilderImpl *impl_from_IDirect3DRMMeshBuilder2(IDirect3DRMMeshBuilder2 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMMeshBuilderImpl, IDirect3DRMMeshBuilder2_iface);
}

static inline IDirect3DRMMeshBuilderImpl *impl_from_IDirect3DRMMeshBuilder3(IDirect3DRMMeshBuilder3 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMMeshBuilderImpl, IDirect3DRMMeshBuilder3_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_QueryInterface(IDirect3DRMMeshBuilder2* iface,
                                                                 REFIID riid, void** ppvObject)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IDirect3DRMMeshBuilder) ||
       IsEqualGUID(riid, &IID_IDirect3DRMMeshBuilder2))
    {
        *ppvObject = &This->IDirect3DRMMeshBuilder2_iface;
    }
    else if(IsEqualGUID(riid, &IID_IDirect3DRMMeshBuilder3))
    {
        *ppvObject = &This->IDirect3DRMMeshBuilder3_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDirect3DRMMeshBuilder_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI IDirect3DRMMeshBuilder2Impl_AddRef(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirect3DRMMeshBuilder2Impl_Release(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    if (!ref)
    {
        if (This->material)
            IDirect3DRMMaterial2_Release(This->material);
        if (This->texture)
            IDirect3DRMTexture3_Release(This->texture);
        HeapFree(GetProcessHeap(), 0, This->name);
        HeapFree(GetProcessHeap(), 0, This->pVertices);
        HeapFree(GetProcessHeap(), 0, This->pNormals);
        HeapFree(GetProcessHeap(), 0, This->pFaceData);
        HeapFree(GetProcessHeap(), 0, This->pCoords2d);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_Clone(IDirect3DRMMeshBuilder2* iface,
                                                        LPUNKNOWN pUnkOuter, REFIID riid,
                                                        LPVOID *ppvObj)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p,%s,%p): stub\n", This, pUnkOuter, debugstr_guid(riid), ppvObj);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_AddDestroyCallback(IDirect3DRMMeshBuilder2* iface,
                                                                     D3DRMOBJECTCALLBACK cb,
                                                                     LPVOID argument)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_DeleteDestroyCallback(IDirect3DRMMeshBuilder2* iface,
                                                                        D3DRMOBJECTCALLBACK cb,
                                                                        LPVOID argument)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetAppData(IDirect3DRMMeshBuilder2* iface,
                                                             DWORD data)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%u): stub\n", This, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMMeshBuilder2Impl_GetAppData(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(): stub\n", This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetName(IDirect3DRMMeshBuilder2* iface,
                                                          LPCSTR name)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_a(name));

    return IDirect3DRMMeshBuilder3_SetName(&This->IDirect3DRMMeshBuilder3_iface, name);
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetName(IDirect3DRMMeshBuilder2* iface,
                                                          LPDWORD size, LPSTR name)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%p,%p)\n", This, size, name);

    return IDirect3DRMMeshBuilder3_GetName(&This->IDirect3DRMMeshBuilder3_iface, size, name);
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetClassName(IDirect3DRMMeshBuilder2* iface,
                                                               LPDWORD size, LPSTR name)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%p, %p)\n", This, size, name);

    return IDirect3DRMMeshBuilder3_GetClassName(&This->IDirect3DRMMeshBuilder3_iface, size, name);
}

/*** IDirect3DRMMeshBuilder2 methods ***/
static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_Load(IDirect3DRMMeshBuilder2* iface,
                                                       LPVOID filename, LPVOID name,
                                                       D3DRMLOADOPTIONS loadflags,
                                                       D3DRMLOADTEXTURECALLBACK cb, LPVOID arg)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%p,%p,%x,%p,%p)\n", This, filename, name, loadflags, cb, arg);

    if (cb)
        FIXME("Texture callback is not yet supported\n");

    return IDirect3DRMMeshBuilder3_Load(&This->IDirect3DRMMeshBuilder3_iface, filename, name,
                                        loadflags, NULL, arg);
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_Save(IDirect3DRMMeshBuilder2* iface,
                                                       const char *filename, D3DRMXOFFORMAT format,
                                                       D3DRMSAVEOPTIONS save)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%s,%d,%d): stub\n", This, filename, format, save);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_Scale(IDirect3DRMMeshBuilder2* iface,
                                                        D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%f,%f,%f)\n", This, sx, sy, sz);

    return IDirect3DRMMeshBuilder3_Scale(&This->IDirect3DRMMeshBuilder3_iface, sx, sy, sz);
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_Translate(IDirect3DRMMeshBuilder2* iface,
                                                            D3DVALUE tx, D3DVALUE ty, D3DVALUE tz)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, tx, ty, tz);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetColorSource(IDirect3DRMMeshBuilder2* iface,
                                                                 D3DRMCOLORSOURCE color)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%x): stub\n", This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetBox(IDirect3DRMMeshBuilder2* iface,
                                                         D3DRMBOX *pBox)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pBox);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GenerateNormals(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static D3DRMCOLORSOURCE WINAPI IDirect3DRMMeshBuilder2Impl_GetColorSource(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_AddMesh(IDirect3DRMMeshBuilder2* iface,
                                                          LPDIRECT3DRMMESH pMesh)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pMesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_AddMeshBuilder(IDirect3DRMMeshBuilder2* iface,
                                                                 LPDIRECT3DRMMESHBUILDER pMeshBuilder)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pMeshBuilder);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_AddFrame(IDirect3DRMMeshBuilder2* iface,
                                                           LPDIRECT3DRMFRAME pFrame)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pFrame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_AddFace(IDirect3DRMMeshBuilder2* iface,
                                                          LPDIRECT3DRMFACE pFace)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pFace);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_AddFaces(IDirect3DRMMeshBuilder2* iface,
                                                           DWORD vcount, D3DVECTOR *vertices,
                                                           DWORD ncount, D3DVECTOR *normals,
                                                           DWORD *data,
                                                           LPDIRECT3DRMFACEARRAY* pFaceArray)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d,%p,%d,%p,%p,%p): stub\n", This, vcount, vertices, ncount, normals, data, pFaceArray);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_ReserveSpace(IDirect3DRMMeshBuilder2* iface,
                                                               DWORD vertex_Count,
                                                               DWORD normal_count,
                                                               DWORD face_count)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d,%d,%d): stub\n", This, vertex_Count, normal_count, face_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetColorRGB(IDirect3DRMMeshBuilder2* iface,
                                                              D3DVALUE red, D3DVALUE green,
                                                              D3DVALUE blue)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%f,%f,%f)\n", This, red, green, blue);

    return IDirect3DRMMeshBuilder3_SetColorRGB(&This->IDirect3DRMMeshBuilder3_iface, red, green, blue);
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetColor(IDirect3DRMMeshBuilder2* iface,
                                                           D3DCOLOR color)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%x)\n", This, color);

    return IDirect3DRMMeshBuilder3_SetColor(&This->IDirect3DRMMeshBuilder3_iface, color);
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetTexture(IDirect3DRMMeshBuilder2* iface,
                                                             LPDIRECT3DRMTEXTURE texture)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);
    LPDIRECT3DRMTEXTURE3 texture3 = NULL;
    HRESULT hr = D3DRM_OK;

    if (texture)
        hr = IDirect3DRMTexture_QueryInterface(texture, &IID_IDirect3DRMTexture3, (LPVOID*)&texture3);
    if (SUCCEEDED(hr))
        hr = IDirect3DRMMeshBuilder3_SetTexture(&This->IDirect3DRMMeshBuilder3_iface, texture3);
    if (texture3)
        IDirect3DRMTexture3_Release(texture3);

    return hr;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetMaterial(IDirect3DRMMeshBuilder2* iface,
                                                              LPDIRECT3DRMMATERIAL material)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%p)\n", This, material);

    return IDirect3DRMMeshBuilder3_SetMaterial(&This->IDirect3DRMMeshBuilder3_iface, (LPDIRECT3DRMMATERIAL2)material);
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetTextureTopology(IDirect3DRMMeshBuilder2* iface,
                                                                     BOOL wrap_u, BOOL wrap_v)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d,%d): stub\n", This, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetQuality(IDirect3DRMMeshBuilder2* iface,
                                                             D3DRMRENDERQUALITY quality)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d): stub\n", This, quality);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetPerspective(IDirect3DRMMeshBuilder2* iface,
                                                                 BOOL enable)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d): stub\n", This, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetVertex(IDirect3DRMMeshBuilder2* iface,
                                                            DWORD index,
                                                            D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetNormal(IDirect3DRMMeshBuilder2* iface,
                                                            DWORD index,
                                                            D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetTextureCoordinates(IDirect3DRMMeshBuilder2* iface,
                                                                        DWORD index,
                                                                        D3DVALUE u, D3DVALUE v)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%u,%f,%f)\n", This, index, u, v);

    return IDirect3DRMMeshBuilder3_SetTextureCoordinates(&This->IDirect3DRMMeshBuilder3_iface,
                                                         index, u, v);
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetVertexColor(IDirect3DRMMeshBuilder2* iface,
                                                                 DWORD index, D3DCOLOR color)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d,%x): stub\n", This, index, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetVertexColorRGB(IDirect3DRMMeshBuilder2* iface,
                                                                    DWORD index, D3DVALUE red,
                                                                    D3DVALUE green, D3DVALUE blue)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d,%f,%f,%f): stub\n", This, index, red, green, blue);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetFaces(IDirect3DRMMeshBuilder2* iface,
                                                           LPDIRECT3DRMFACEARRAY* pFaceArray)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pFaceArray);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetVertices(IDirect3DRMMeshBuilder2* iface,
                                                              DWORD *vcount, D3DVECTOR *vertices,
                                                              DWORD *ncount, D3DVECTOR *normals,
                                                              DWORD *face_data_size,
                                                              DWORD *face_data)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%p,%p,%p,%p,%p,%p)\n", This, vcount, vertices, ncount, normals, face_data_size, face_data);

    if (vcount)
        *vcount = This->nb_vertices;
    if (vertices && This->nb_vertices)
        memcpy(vertices, This->pVertices, This->nb_vertices * sizeof(D3DVECTOR));
    if (ncount)
        *ncount = This->nb_normals;
    if (normals && This->nb_normals)
        memcpy(normals, This->pNormals, This->nb_normals * sizeof(D3DVECTOR));
    if (face_data_size)
        *face_data_size = This->face_data_size;
    if (face_data && This->face_data_size)
        memcpy(face_data, This->pFaceData, This->face_data_size * sizeof(DWORD));

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetTextureCoordinates(IDirect3DRMMeshBuilder2* iface,
                                                                        DWORD index,
                                                                        D3DVALUE *u, D3DVALUE *v)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%u,%p,%p)\n", This, index, u, v);

    return IDirect3DRMMeshBuilder3_GetTextureCoordinates(&This->IDirect3DRMMeshBuilder3_iface,
                                                         index, u, v);
}

static int WINAPI IDirect3DRMMeshBuilder2Impl_AddVertex(IDirect3DRMMeshBuilder2* iface,
                                                        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return 0;
}

static int WINAPI IDirect3DRMMeshBuilder2Impl_AddNormal(IDirect3DRMMeshBuilder2* iface,
                                                        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_CreateFace(IDirect3DRMMeshBuilder2* iface,
                                                             LPDIRECT3DRMFACE* ppFace)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, ppFace);

    return E_NOTIMPL;
}

static D3DRMRENDERQUALITY WINAPI IDirect3DRMMeshBuilder2Impl_GetQuality(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(): stub\n", This);

    return 0;
}

static BOOL WINAPI IDirect3DRMMeshBuilder2Impl_GetPerspective(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(): stub\n", This);

    return FALSE;
}

static int WINAPI IDirect3DRMMeshBuilder2Impl_GetFaceCount(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->()\n", This);

    return This->nb_faces;
}

static int WINAPI IDirect3DRMMeshBuilder2Impl_GetVertexCount(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->()\n", This);

    return This->nb_vertices;
}

static D3DCOLOR WINAPI IDirect3DRMMeshBuilder2Impl_GetVertexColor(IDirect3DRMMeshBuilder2* iface,
                                                                  DWORD index)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d): stub\n", This, index);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_CreateMesh(IDirect3DRMMeshBuilder2* iface,
                                                             LPDIRECT3DRMMESH* mesh)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%p)\n", This, mesh);

    return IDirect3DRMMeshBuilder3_CreateMesh(&This->IDirect3DRMMeshBuilder3_iface, mesh);
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GenerateNormals2(IDirect3DRMMeshBuilder2* iface,
                                                                   D3DVALUE crease, DWORD dwFlags)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%u): stub\n", This, crease, dwFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetFace(IDirect3DRMMeshBuilder2* iface,
                                                          DWORD index, LPDIRECT3DRMFACE* ppFace)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%u,%p): stub\n", This, index, ppFace);

    return E_NOTIMPL;
}

static const struct IDirect3DRMMeshBuilder2Vtbl Direct3DRMMeshBuilder2_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMMeshBuilder2Impl_QueryInterface,
    IDirect3DRMMeshBuilder2Impl_AddRef,
    IDirect3DRMMeshBuilder2Impl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMMeshBuilder2Impl_Clone,
    IDirect3DRMMeshBuilder2Impl_AddDestroyCallback,
    IDirect3DRMMeshBuilder2Impl_DeleteDestroyCallback,
    IDirect3DRMMeshBuilder2Impl_SetAppData,
    IDirect3DRMMeshBuilder2Impl_GetAppData,
    IDirect3DRMMeshBuilder2Impl_SetName,
    IDirect3DRMMeshBuilder2Impl_GetName,
    IDirect3DRMMeshBuilder2Impl_GetClassName,
    /*** IDirect3DRMMeshBuilder methods ***/
    IDirect3DRMMeshBuilder2Impl_Load,
    IDirect3DRMMeshBuilder2Impl_Save,
    IDirect3DRMMeshBuilder2Impl_Scale,
    IDirect3DRMMeshBuilder2Impl_Translate,
    IDirect3DRMMeshBuilder2Impl_SetColorSource,
    IDirect3DRMMeshBuilder2Impl_GetBox,
    IDirect3DRMMeshBuilder2Impl_GenerateNormals,
    IDirect3DRMMeshBuilder2Impl_GetColorSource,
    IDirect3DRMMeshBuilder2Impl_AddMesh,
    IDirect3DRMMeshBuilder2Impl_AddMeshBuilder,
    IDirect3DRMMeshBuilder2Impl_AddFrame,
    IDirect3DRMMeshBuilder2Impl_AddFace,
    IDirect3DRMMeshBuilder2Impl_AddFaces,
    IDirect3DRMMeshBuilder2Impl_ReserveSpace,
    IDirect3DRMMeshBuilder2Impl_SetColorRGB,
    IDirect3DRMMeshBuilder2Impl_SetColor,
    IDirect3DRMMeshBuilder2Impl_SetTexture,
    IDirect3DRMMeshBuilder2Impl_SetMaterial,
    IDirect3DRMMeshBuilder2Impl_SetTextureTopology,
    IDirect3DRMMeshBuilder2Impl_SetQuality,
    IDirect3DRMMeshBuilder2Impl_SetPerspective,
    IDirect3DRMMeshBuilder2Impl_SetVertex,
    IDirect3DRMMeshBuilder2Impl_SetNormal,
    IDirect3DRMMeshBuilder2Impl_SetTextureCoordinates,
    IDirect3DRMMeshBuilder2Impl_SetVertexColor,
    IDirect3DRMMeshBuilder2Impl_SetVertexColorRGB,
    IDirect3DRMMeshBuilder2Impl_GetFaces,
    IDirect3DRMMeshBuilder2Impl_GetVertices,
    IDirect3DRMMeshBuilder2Impl_GetTextureCoordinates,
    IDirect3DRMMeshBuilder2Impl_AddVertex,
    IDirect3DRMMeshBuilder2Impl_AddNormal,
    IDirect3DRMMeshBuilder2Impl_CreateFace,
    IDirect3DRMMeshBuilder2Impl_GetQuality,
    IDirect3DRMMeshBuilder2Impl_GetPerspective,
    IDirect3DRMMeshBuilder2Impl_GetFaceCount,
    IDirect3DRMMeshBuilder2Impl_GetVertexCount,
    IDirect3DRMMeshBuilder2Impl_GetVertexColor,
    IDirect3DRMMeshBuilder2Impl_CreateMesh,
    /*** IDirect3DRMMeshBuilder2 methods ***/
    IDirect3DRMMeshBuilder2Impl_GenerateNormals2,
    IDirect3DRMMeshBuilder2Impl_GetFace
};


/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_QueryInterface(IDirect3DRMMeshBuilder3* iface,
                                                                 REFIID riid, void** ppvObject)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);
    return IDirect3DRMMeshBuilder2_QueryInterface(&This->IDirect3DRMMeshBuilder2_iface, riid, ppvObject);
}

static ULONG WINAPI IDirect3DRMMeshBuilder3Impl_AddRef(IDirect3DRMMeshBuilder3* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);
    return IDirect3DRMMeshBuilder2_AddRef(&This->IDirect3DRMMeshBuilder2_iface);
}

static ULONG WINAPI IDirect3DRMMeshBuilder3Impl_Release(IDirect3DRMMeshBuilder3* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);
    return IDirect3DRMMeshBuilder2_Release(&This->IDirect3DRMMeshBuilder2_iface);
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_Clone(IDirect3DRMMeshBuilder3* iface,
                                                        LPUNKNOWN pUnkOuter, REFIID riid,
                                                        LPVOID *ppvObj)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p,%s,%p): stub\n", This, pUnkOuter, debugstr_guid(riid), ppvObj);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_AddDestroyCallback(IDirect3DRMMeshBuilder3* iface,
                                                                     D3DRMOBJECTCALLBACK cb,
                                                                     LPVOID argument)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_DeleteDestroyCallback(IDirect3DRMMeshBuilder3* iface,
                                                                        D3DRMOBJECTCALLBACK cb,
                                                                        LPVOID argument)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetAppData(IDirect3DRMMeshBuilder3* iface,
                                                             DWORD data)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u): stub\n", This, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMMeshBuilder3Impl_GetAppData(IDirect3DRMMeshBuilder3* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(): stub\n", This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetName(IDirect3DRMMeshBuilder3* iface,
                                                          LPCSTR name)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);
    char *string = NULL;

    TRACE("(%p)->(%s)\n", This, debugstr_a(name));

    if (name)
    {
        string = HeapAlloc(GetProcessHeap(), 0, strlen(name) + 1);
        if (!string) return E_OUTOFMEMORY;
        strcpy(string, name);
    }
    HeapFree(GetProcessHeap(), 0, This->name);
    This->name = string;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetName(IDirect3DRMMeshBuilder3* iface,
                                                          LPDWORD size, LPSTR name)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("(%p)->(%p,%p)\n", This, size, name);

    if (!size)
        return E_POINTER;

    if (!This->name)
    {
        *size = 0;
        return D3DRM_OK;
    }

    if (*size < (strlen(This->name) + 1))
        return E_INVALIDARG;

    strcpy(name, This->name);
    *size = strlen(This->name) + 1;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetClassName(IDirect3DRMMeshBuilder3* iface,
                                                               LPDWORD size, LPSTR name)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("(%p)->(%p, %p)\n", This, size, name);

    if (!size || *size < strlen("Builder") || !name)
        return E_INVALIDARG;

    strcpy(name, "Builder");
    *size = sizeof("Builder");

    return D3DRM_OK;
}

HRESULT load_mesh_data(IDirect3DRMMeshBuilder3* iface, LPDIRECTXFILEDATA pData)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);
    LPDIRECTXFILEOBJECT pObject = NULL;
    LPDIRECTXFILEDATA pData2 = NULL;
    const GUID* pGuid;
    DWORD size;
    LPBYTE ptr;
    HRESULT hr;
    HRESULT ret = D3DRMERR_BADOBJECT;
    DWORD* faces_vertex_idx_data = NULL;
    DWORD* faces_vertex_idx_ptr;
    DWORD faces_vertex_idx_size;
    DWORD* faces_normal_idx_data = NULL;
    DWORD* faces_normal_idx_ptr = NULL;
    DWORD* faces_data_ptr;
    DWORD faces_data_size = 0;
    DWORD i;

    TRACE("(%p)->(%p)\n", This, pData);

    /* Remove previous name */
    HeapFree(GetProcessHeap(), 0, This->name);
    This->name = NULL;
    hr = IDirectXFileData_GetName(pData, NULL, &size);
    if (hr != DXFILE_OK)
        return hr;
    if (size)
    {
        This->name = HeapAlloc(GetProcessHeap(), 0, size);
        if (!This->name)
            return E_OUTOFMEMORY;

        hr = IDirectXFileData_GetName(pData, This->name, &size);
        if (hr != DXFILE_OK)
        {
            HeapFree(GetProcessHeap(), 0, This->name);
            This->name = NULL;
            return hr;
        }
    }

    TRACE("Mesh name is '%s'\n", This->name ? This->name : "");

    This->nb_normals = 0;

    hr = IDirectXFileData_GetData(pData, NULL, &size, (void**)&ptr);
    if (hr != DXFILE_OK)
        goto end;

    This->nb_vertices = *(DWORD*)ptr;
    This->nb_faces = *(DWORD*)(ptr + sizeof(DWORD) + This->nb_vertices * sizeof(D3DVECTOR));
    faces_vertex_idx_size = size - sizeof(DWORD) - This->nb_vertices * sizeof(D3DVECTOR) - sizeof(DWORD);
    faces_vertex_idx_ptr = (DWORD*)(ptr + sizeof(DWORD) + This->nb_vertices * sizeof(D3DVECTOR) + sizeof(DWORD));

    TRACE("Mesh: nb_vertices = %d, nb_faces = %d, faces_vertex_idx_size = %d\n", This->nb_vertices, This->nb_faces, faces_vertex_idx_size);

    This->pVertices = HeapAlloc(GetProcessHeap(), 0, This->nb_vertices * sizeof(D3DVECTOR));
    memcpy(This->pVertices, ptr + sizeof(DWORD), This->nb_vertices * sizeof(D3DVECTOR));

    faces_vertex_idx_ptr = faces_vertex_idx_data = HeapAlloc(GetProcessHeap(), 0, faces_vertex_idx_size);
    memcpy(faces_vertex_idx_data, ptr + sizeof(DWORD) + This->nb_vertices * sizeof(D3DVECTOR) + sizeof(DWORD), faces_vertex_idx_size);

    /* Each vertex index will have its normal index counterpart so just allocate twice the size */
    This->pFaceData = HeapAlloc(GetProcessHeap(), 0, faces_vertex_idx_size*2);
    faces_data_ptr = (DWORD*)This->pFaceData;

    while (1)
    {
        hr =  IDirectXFileData_GetNextObject(pData, &pObject);
        if (hr == DXFILEERR_NOMOREOBJECTS)
        {
            TRACE("No more object\n");
            break;
        }
        if (hr != DXFILE_OK)
           goto end;

        hr = IDirectXFileObject_QueryInterface(pObject, &IID_IDirectXFileData, (void**)&pData2);
        IDirectXFileObject_Release(pObject);
        if (hr != DXFILE_OK)
            goto end;

        hr = IDirectXFileData_GetType(pData2, &pGuid);
        if (hr != DXFILE_OK)
            goto end;

        TRACE("Found object type whose GUID = %s\n", debugstr_guid(pGuid));

        if (IsEqualGUID(pGuid, &TID_D3DRMMeshNormals))
        {
            DWORD nb_faces_normals;
            DWORD faces_normal_idx_size;

            hr = IDirectXFileData_GetData(pData2, NULL, &size, (void**)&ptr);
            if (hr != DXFILE_OK)
                goto end;

            This->nb_normals = *(DWORD*)ptr;
            nb_faces_normals = *(DWORD*)(ptr + sizeof(DWORD) + This->nb_normals * sizeof(D3DVECTOR));

            TRACE("MeshNormals: nb_normals = %d, nb_faces_normals = %d\n", This->nb_normals, nb_faces_normals);
            if (nb_faces_normals != This->nb_faces)
                WARN("nb_face_normals (%d) != nb_faces (%d)\n", nb_faces_normals, This->nb_normals);

            This->pNormals = HeapAlloc(GetProcessHeap(), 0, This->nb_normals * sizeof(D3DVECTOR));
            memcpy(This->pNormals, ptr + sizeof(DWORD), This->nb_normals * sizeof(D3DVECTOR));

            faces_normal_idx_size = size - (2*sizeof(DWORD) + This->nb_normals * sizeof(D3DVECTOR));
            faces_normal_idx_ptr = faces_normal_idx_data = HeapAlloc(GetProcessHeap(), 0, faces_normal_idx_size);
            memcpy(faces_normal_idx_data, ptr + sizeof(DWORD) + This->nb_normals * sizeof(D3DVECTOR) + sizeof(DWORD), faces_normal_idx_size);
        }
        else if (IsEqualGUID(pGuid, &TID_D3DRMMeshTextureCoords))
        {
            hr = IDirectXFileData_GetData(pData2, NULL, &size, (void**)&ptr);
            if (hr != DXFILE_OK)
                goto end;

            This->nb_coords2d = *(DWORD*)ptr;

            TRACE("MeshTextureCoords: nb_coords2d = %d\n", This->nb_coords2d);

            This->pCoords2d = HeapAlloc(GetProcessHeap(), 0, This->nb_coords2d * sizeof(Coords2d));
            memcpy(This->pCoords2d, ptr + sizeof(DWORD), This->nb_coords2d * sizeof(Coords2d));

        }
        else if (IsEqualGUID(pGuid, &TID_D3DRMMeshMaterialList))
        {
            DWORD nb_materials;
            DWORD nb_face_indices;
            DWORD data_size;
            LPDIRECTXFILEOBJECT child;
            DWORD i = 0;
            float* values;

            TRACE("Process MeshMaterialList\n");

            hr = IDirectXFileData_GetData(pData2, NULL, &size, (void**)&ptr);
            if (hr != DXFILE_OK)
                goto end;

            nb_materials = *(DWORD*)ptr;
            nb_face_indices = *(DWORD*)(ptr + sizeof(DWORD));
            data_size = 2 * sizeof(DWORD) + nb_face_indices * sizeof(DWORD);

            TRACE("nMaterials = %u, nFaceIndexes = %u\n", nb_materials, nb_face_indices);

            if (size != data_size)
                WARN("Returned size %u does not match expected one %u\n", size, data_size);

            if (nb_materials > 2)
            {
                FIXME("Only one material per mesh supported, first one applies to all faces\n");
                nb_materials = 1;
            }

            while (SUCCEEDED(hr = IDirectXFileData_GetNextObject(pData2, &child)) && (i < nb_materials))
            {
                LPDIRECTXFILEDATA data;
                LPDIRECTXFILEDATAREFERENCE reference;
                LPDIRECT3DRMMATERIAL2 material;
                LPDIRECTXFILEOBJECT material_child;

                hr = IDirectXFileObject_QueryInterface(child, &IID_IDirectXFileData, (void **)&data);
                if (FAILED(hr))
                {
                    hr = IDirectXFileObject_QueryInterface(child, &IID_IDirectXFileDataReference, (void **)&reference);
                    if (FAILED(hr))
                        goto end;

                    hr = IDirectXFileDataReference_Resolve(reference, &data);
                    IDirectXFileDataReference_Release(reference);
                    if (FAILED(hr))
                        goto end;
                }

                hr = Direct3DRMMaterial_create(&material);
                if (FAILED(hr))
                {
                    IDirectXFileData_Release(data);
                    goto end;
                }

                hr = IDirectXFileData_GetData(data, NULL, &size, (void**)&ptr);
                if (hr != DXFILE_OK)
                {
                    IDirectXFileData_Release(data);
                    goto end;
                }

                if (size != 44)
                    WARN("Material size %u does not match expected one %u\n", size, 44);

                values = (float*)ptr;

                IDirect3DRMMeshBuilder3_SetColorRGB(iface, values[0], values [1], values[2]); /* Alpha ignored */

                IDirect3DRMMaterial2_SetAmbient(material, values[0], values [1], values[2]); /* Alpha ignored */
                IDirect3DRMMaterial2_SetPower(material, values[4]);
                IDirect3DRMMaterial2_SetSpecular(material, values[5], values[6], values[7]);
                IDirect3DRMMaterial2_SetEmissive(material, values[8], values[9], values[10]);

                This->material = material;

                hr = IDirectXFileData_GetNextObject(data, &material_child);
                if (hr == S_OK)
                {
                    LPDIRECTXFILEDATA data;
                    LPDIRECT3DRMTEXTURE3 texture;
                    char** filename;

                    hr = IDirectXFileObject_QueryInterface(material_child, &IID_IDirectXFileData, (void **)&data);
                    if (FAILED(hr))
                    {
                        LPDIRECTXFILEDATAREFERENCE reference;

                        hr = IDirectXFileObject_QueryInterface(material_child, &IID_IDirectXFileDataReference, (void **)&reference);
                        if (FAILED(hr))
                            goto end;

                        hr = IDirectXFileDataReference_Resolve(reference, &data);
                        IDirectXFileDataReference_Release(reference);
                        if (FAILED(hr))
                            goto end;
                    }

                    hr = IDirectXFileData_GetType(data, &pGuid);
                    if (hr != DXFILE_OK)
                        goto end;
                    if (!IsEqualGUID(pGuid, &TID_D3DRMTextureFilename))
                    {
                         WARN("Not a texture filename\n");
                         goto end;
                    }

                    size = 4;
                    hr = IDirectXFileData_GetData(data, NULL, &size, (void**)&filename);
                    if (SUCCEEDED(hr))
                    {
                        HANDLE file;

                        file = CreateFileA(*filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
                        if (file != INVALID_HANDLE_VALUE)
                        {
                            hr = Direct3DRMTexture_create(&IID_IDirect3DRMTexture3, (LPUNKNOWN*)&texture);
                            if (FAILED(hr))
                            {
                                IDirectXFileData_Release(data);
                                goto end;
                            }
                            This->texture = texture;
                        }
                    }
                }
                else if (hr != DXFILEERR_NOMOREOBJECTS)
                {
                    goto end;
                }
                hr = S_OK;

                IDirectXFileData_Release(data);
                i++;
            }
            if (hr == S_OK)
                WARN("Found more sub-objects than expected\n");
            else if (hr != DXFILEERR_NOMOREOBJECTS)
                goto end;
            hr = S_OK;
        }
        else
        {
            FIXME("Unknown GUID %s, ignoring...\n", debugstr_guid(pGuid));
        }

        IDirectXFileData_Release(pData2);
        pData2 = NULL;
    }

    if (!This->nb_normals)
    {
        /* Allocate normals, one per vertex */
        This->pNormals = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->nb_vertices * sizeof(D3DVECTOR));
        if (!This->pNormals)
            goto end;
    }

    for (i = 0; i < This->nb_faces; i++)
    {
        DWORD j;
        DWORD nb_face_indexes;
        D3DVECTOR face_normal;

        if (faces_vertex_idx_size < sizeof(DWORD))
            WARN("Not enough data to read number of indices of face %d\n", i);

        nb_face_indexes  = *(faces_data_ptr + faces_data_size++) = *(faces_vertex_idx_ptr++);
        faces_vertex_idx_size--;
        if (faces_normal_idx_data && (*(faces_normal_idx_ptr++) != nb_face_indexes))
            WARN("Faces indices number mismatch\n");

        if (faces_vertex_idx_size < (nb_face_indexes * sizeof(DWORD)))
            WARN("Not enough data to read all indices of face %d\n", i);

        if (!This->nb_normals)
        {
            /* Compute face normal */
            if (nb_face_indexes > 2)
            {
                D3DVECTOR a, b;

                D3DRMVectorSubtract(&a, &This->pVertices[faces_vertex_idx_ptr[2]], &This->pVertices[faces_vertex_idx_ptr[1]]);
                D3DRMVectorSubtract(&a, &This->pVertices[faces_vertex_idx_ptr[1]], &This->pVertices[faces_vertex_idx_ptr[0]]);
                D3DRMVectorCrossProduct(&face_normal, &a, &b);
                D3DRMVectorNormalize(&face_normal);
            }
            else
            {
                face_normal.u1.x = 0.0f;
                face_normal.u2.y = 0.0f;
                face_normal.u3.z = 0.0f;
            }
        }

        for (j = 0; j < nb_face_indexes; j++)
        {
            /* Copy vertex index */
            *(faces_data_ptr + faces_data_size++) = *faces_vertex_idx_ptr;
            /* Copy normal index */
            if (This->nb_normals)
            {
                /* Read from x file */
                *(faces_data_ptr + faces_data_size++) = *(faces_normal_idx_ptr++);
            }
            else
            {
                DWORD vertex_idx = *faces_vertex_idx_ptr;
                if (vertex_idx >= This->nb_vertices)
                {
                    WARN("Found vertex index %u but only %u vertices available => use index 0\n", vertex_idx, This->nb_vertices);
                    vertex_idx = 0;
                }
                *(faces_data_ptr + faces_data_size++) = vertex_idx;
                /* Add face normal to vertex normal */
                D3DRMVectorAdd(&This->pNormals[vertex_idx], &This->pNormals[vertex_idx], &face_normal);
            }
            faces_vertex_idx_ptr++;
        }
        faces_vertex_idx_size -= nb_face_indexes;
    }

    /* Last DWORD must be 0 */
    *(faces_data_ptr + faces_data_size++) = 0;

    /* Set size (in number of DWORD) of all faces data */
    This->face_data_size = faces_data_size;

    if (!This->nb_normals)
    {
        /* Normalize all normals */
        for (i = 0; i < This->nb_vertices; i++)
        {
            D3DRMVectorNormalize(&This->pNormals[i]);
        }
        This->nb_normals = This->nb_vertices;
    }

    /* If there is no texture coordinates, generate default texture coordinates (0.0f, 0.0f) for each vertex */
    if (!This->pCoords2d)
    {
        This->nb_coords2d = This->nb_vertices;
        This->pCoords2d = HeapAlloc(GetProcessHeap(), 0, This->nb_coords2d * sizeof(Coords2d));
        for (i = 0; i < This->nb_coords2d; i++)
        {
            This->pCoords2d[i].u = 0.0f;
            This->pCoords2d[i].v = 0.0f;
        }
    }

    TRACE("Mesh data loaded successfully\n");

    ret = D3DRM_OK;

end:

    HeapFree(GetProcessHeap(), 0, faces_normal_idx_data);
    HeapFree(GetProcessHeap(), 0, faces_vertex_idx_data);

    return ret;
}

/*** IDirect3DRMMeshBuilder3 methods ***/
static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_Load(IDirect3DRMMeshBuilder3* iface,
                                                       LPVOID filename, LPVOID name,
                                                       D3DRMLOADOPTIONS loadflags,
                                                       D3DRMLOADTEXTURE3CALLBACK cb, LPVOID arg)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);
    DXFILELOADOPTIONS load_options;
    LPDIRECTXFILE pDXFile = NULL;
    LPDIRECTXFILEENUMOBJECT pEnumObject = NULL;
    LPDIRECTXFILEDATA pData = NULL;
    LPDIRECTXFILEDATA pData2 = NULL;
    const GUID* pGuid;
    DWORD size;
    Header* pHeader;
    HRESULT hr;
    HRESULT ret = D3DRMERR_BADOBJECT;

    TRACE("(%p)->(%p,%p,%x,%p,%p)\n", This, filename, name, loadflags, cb, arg);

    /* First free allocated buffers of previous mesh data */
    HeapFree(GetProcessHeap(), 0, This->pVertices);
    This->pVertices = NULL;
    HeapFree(GetProcessHeap(), 0, This->pNormals);
    This->pNormals = NULL;
    HeapFree(GetProcessHeap(), 0, This->pFaceData);
    This->pFaceData = NULL;
    HeapFree(GetProcessHeap(), 0, This->pCoords2d);
    This->pCoords2d = NULL;

    if (loadflags == D3DRMLOAD_FROMMEMORY)
    {
        load_options = DXFILELOAD_FROMMEMORY;
    }
    else if (loadflags == D3DRMLOAD_FROMFILE)
    {
        load_options = DXFILELOAD_FROMFILE;
        TRACE("Loading from file %s\n", debugstr_a(filename));
    }
    else
    {
        FIXME("Load options %d not supported yet\n", loadflags);
        return E_NOTIMPL;
    }

    hr = DirectXFileCreate(&pDXFile);
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFile_RegisterTemplates(pDXFile, templates, strlen(templates));
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFile_CreateEnumObject(pDXFile, filename, load_options, &pEnumObject);
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFileEnumObject_GetNextDataObject(pEnumObject, &pData);
    if (hr != DXFILE_OK)
        goto end;

    hr = IDirectXFileData_GetType(pData, &pGuid);
    if (hr != DXFILE_OK)
        goto end;

    TRACE("Found object type whose GUID = %s\n", debugstr_guid(pGuid));

    if (!IsEqualGUID(pGuid, &TID_DXFILEHeader))
    {
        ret = D3DRMERR_BADFILE;
        goto end;
    }

    hr = IDirectXFileData_GetData(pData, NULL, &size, (void**)&pHeader);
    if ((hr != DXFILE_OK) || (size != sizeof(Header)))
        goto end;

    TRACE("Version is %d %d %d\n", pHeader->major, pHeader->minor, pHeader->flags);

    /* Version must be 1.0.x */
    if ((pHeader->major != 1) || (pHeader->minor != 0))
    {
        ret = D3DRMERR_BADFILE;
        goto end;
    }

    IDirectXFileData_Release(pData);
    pData = NULL;

    hr = IDirectXFileEnumObject_GetNextDataObject(pEnumObject, &pData);
    if (hr != DXFILE_OK)
    {
        ret = D3DRMERR_NOTFOUND;
        goto end;
    }

    hr = IDirectXFileData_GetType(pData, &pGuid);
    if (hr != DXFILE_OK)
        goto end;

    TRACE("Found object type whose GUID = %s\n", debugstr_guid(pGuid));

    if (!IsEqualGUID(pGuid, &TID_D3DRMMesh))
    {
        ret = D3DRMERR_NOTFOUND;
        goto end;
    }

    hr = load_mesh_data(iface, pData);
    if (hr == S_OK)
        ret = D3DRM_OK;

end:

    if (pData2)
        IDirectXFileData_Release(pData2);
    if (pData)
        IDirectXFileData_Release(pData);
    if (pEnumObject)
        IDirectXFileEnumObject_Release(pEnumObject);
    if (pDXFile)
        IDirectXFile_Release(pDXFile);

    if (ret != D3DRM_OK)
    {
        /* Clean mesh data */
        This->nb_vertices = 0;
        This->nb_normals = 0;
        This->nb_faces = 0;
        This->face_data_size = 0;
        This->nb_coords2d = 0;
        HeapFree(GetProcessHeap(), 0, This->pVertices);
        This->pVertices = NULL;
        HeapFree(GetProcessHeap(), 0, This->pNormals);
        This->pNormals = NULL;
        HeapFree(GetProcessHeap(), 0, This->pFaceData);
        This->pFaceData = NULL;
        HeapFree(GetProcessHeap(), 0, This->pCoords2d);
        This->pCoords2d = NULL;
    }

    return ret;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_Save(IDirect3DRMMeshBuilder3* iface,
                                                       const char* filename, D3DRMXOFFORMAT format,
                                                       D3DRMSAVEOPTIONS save)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%s,%d,%d): stub\n", This, filename, format, save);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_Scale(IDirect3DRMMeshBuilder3* iface,
                                                        D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);
    int i;

    TRACE("(%p)->(%f,%f,%f)\n", This, sx, sy, sz);

    for (i = 0; i < This->nb_vertices; i++)
    {
        This->pVertices[i].u1.x *= sx;
        This->pVertices[i].u2.y *= sy;
        This->pVertices[i].u3.z *= sz;
    }

    /* Normals are not affected by Scale */

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_Translate(IDirect3DRMMeshBuilder3* iface,
                                                            D3DVALUE tx, D3DVALUE ty, D3DVALUE tz)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, tx, ty, tz);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetColorSource(IDirect3DRMMeshBuilder3* iface,
                                                                 D3DRMCOLORSOURCE color)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%x): stub\n", This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetBox(IDirect3DRMMeshBuilder3* iface,
                                                         D3DRMBOX* box)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p): stub\n", This, box);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GenerateNormals(IDirect3DRMMeshBuilder3* iface,
                                                                  D3DVALUE crease, DWORD flags)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%f,%u): stub\n", This, crease, flags);

    return E_NOTIMPL;
}

static D3DRMCOLORSOURCE WINAPI IDirect3DRMMeshBuilder3Impl_GetColorSource(IDirect3DRMMeshBuilder3* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_AddMesh(IDirect3DRMMeshBuilder3* iface,
                                                          LPDIRECT3DRMMESH Mesh)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p): stub\n", This, Mesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_AddMeshBuilder(IDirect3DRMMeshBuilder3* iface,
                                                                 LPDIRECT3DRMMESHBUILDER3 MeshBuilder,
                                                                 DWORD flags)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p,%u): stub\n", This, MeshBuilder, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_AddFrame(IDirect3DRMMeshBuilder3* iface,
                                                           LPDIRECT3DRMFRAME3 Frame)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p): stub\n", This, Frame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_AddFace(IDirect3DRMMeshBuilder3* iface,
                                                          LPDIRECT3DRMFACE2 Face)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p): stub\n", This, Face);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_AddFaces(IDirect3DRMMeshBuilder3* iface,
                                                           DWORD vcount, D3DVECTOR* vertices,
                                                           DWORD ncount, D3DVECTOR* normals,
                                                           DWORD* data,
                                                           LPDIRECT3DRMFACEARRAY* FaceArray)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%d,%p,%d,%p,%p,%p): stub\n", This, vcount, vertices, ncount,
          normals, data, FaceArray);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_ReserveSpace(IDirect3DRMMeshBuilder3* iface,
                                                               DWORD vertex_Count,
                                                               DWORD normal_count,
                                                               DWORD face_count)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%d,%d,%d): stub\n", This, vertex_Count, normal_count, face_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetColorRGB(IDirect3DRMMeshBuilder3* iface,
                                                              D3DVALUE red, D3DVALUE green,
                                                              D3DVALUE blue)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("(%p)->(%f,%f,%f)\n", This, red, green, blue);

    This->color = D3DCOLOR_ARGB(0xff, (BYTE)(red   * 255.0f),
                                      (BYTE)(green * 255.0f),
                                      (BYTE)(blue  * 255.0f));

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetColor(IDirect3DRMMeshBuilder3* iface,
                                                           D3DCOLOR color)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("(%p)->(%x)\n", This, color);

    This->color = color;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetTexture(IDirect3DRMMeshBuilder3* iface,
                                                             LPDIRECT3DRMTEXTURE3 texture)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("(%p)->(%p)\n", This, texture);

    if (texture)
        IDirect3DRMTexture3_AddRef(texture);
    if (This->texture)
        IDirect3DRMTexture3_Release(This->texture);
    This->texture = texture;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetMaterial(IDirect3DRMMeshBuilder3* iface,
                                                              LPDIRECT3DRMMATERIAL2 material)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("(%p)->(%p)\n", This, material);

    if (material)
        IDirect3DRMTexture2_AddRef(material);
    if (This->material)
        IDirect3DRMTexture2_Release(This->material);
    This->material = material;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetTextureTopology(IDirect3DRMMeshBuilder3* iface,
                                                                     BOOL wrap_u, BOOL wrap_v)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%d,%d): stub\n", This, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetQuality(IDirect3DRMMeshBuilder3* iface,
                                                             D3DRMRENDERQUALITY quality)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%d): stub\n", This, quality);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetPerspective(IDirect3DRMMeshBuilder3* iface,
                                                                 BOOL enable)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%d): stub\n", This, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetVertex(IDirect3DRMMeshBuilder3* iface,
                                                            DWORD index,
                                                            D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetNormal(IDirect3DRMMeshBuilder3* iface,
                                                            DWORD index,
                                                            D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetTextureCoordinates(IDirect3DRMMeshBuilder3* iface,
                                                                        DWORD index, D3DVALUE u,
                                                                        D3DVALUE v)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("(%p)->(%u,%f,%f)\n", This, index, u, v);

    if (index >= This->nb_coords2d)
        return D3DRMERR_BADVALUE;

    This->pCoords2d[index].u = u;
    This->pCoords2d[index].v = v;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetVertexColor(IDirect3DRMMeshBuilder3* iface,
                                                                 DWORD index, D3DCOLOR color)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%d,%x): stub\n", This, index, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetVertexColorRGB(IDirect3DRMMeshBuilder3* iface,
                                                                    DWORD index,
                                                                    D3DVALUE red, D3DVALUE green,
                                                                    D3DVALUE blue)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%d,%f,%f,%f): stub\n", This, index, red, green, blue);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetFaces(IDirect3DRMMeshBuilder3* iface,
                                                           LPDIRECT3DRMFACEARRAY* FaceArray)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p): stub\n", This, FaceArray);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetGeometry(IDirect3DRMMeshBuilder3* iface,
                                                              DWORD* vcount, D3DVECTOR* vertices,
                                                              DWORD* ncount, D3DVECTOR* normals,
                                                              DWORD* face_data_size,
                                                              DWORD* face_data)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p,%p,%p,%p,%p,%p): stub\n", This, vcount, vertices, ncount, normals,
          face_data_size, face_data);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetTextureCoordinates(IDirect3DRMMeshBuilder3* iface,
                                                                        DWORD index, D3DVALUE* u,
                                                                        D3DVALUE* v)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("(%p)->(%d,%p,%p)\n", This, index, u, v);

    if (index >= This->nb_coords2d)
        return D3DRMERR_BADVALUE;

    *u = This->pCoords2d[index].u;
    *v = This->pCoords2d[index].v;

    return D3DRM_OK;
}


static int WINAPI IDirect3DRMMeshBuilder3Impl_AddVertex(IDirect3DRMMeshBuilder3* iface,
                                                        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return 0;
}

static int WINAPI IDirect3DRMMeshBuilder3Impl_AddNormal(IDirect3DRMMeshBuilder3* iface,
                                                        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_CreateFace(IDirect3DRMMeshBuilder3* iface,
                                                             LPDIRECT3DRMFACE2* Face)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p): stub\n", This, Face);

    return E_NOTIMPL;
}

static D3DRMRENDERQUALITY WINAPI IDirect3DRMMeshBuilder3Impl_GetQuality(IDirect3DRMMeshBuilder3* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(): stub\n", This);

    return 0;
}

static BOOL WINAPI IDirect3DRMMeshBuilder3Impl_GetPerspective(IDirect3DRMMeshBuilder3* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(): stub\n", This);

    return FALSE;
}

static int WINAPI IDirect3DRMMeshBuilder3Impl_GetFaceCount(IDirect3DRMMeshBuilder3* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("(%p)->()\n", This);

    return This->nb_faces;
}

static int WINAPI IDirect3DRMMeshBuilder3Impl_GetVertexCount(IDirect3DRMMeshBuilder3* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    TRACE("(%p)->()\n", This);

    return This->nb_vertices;
}

static D3DCOLOR WINAPI IDirect3DRMMeshBuilder3Impl_GetVertexColor(IDirect3DRMMeshBuilder3* iface,
                                                                  DWORD index)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%d): stub\n", This, index);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_CreateMesh(IDirect3DRMMeshBuilder3* iface,
                                                             LPDIRECT3DRMMESH* mesh)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);
    HRESULT hr;
    D3DRMGROUPINDEX group;

    TRACE("(%p)->(%p)\n", This, mesh);

    if (!mesh)
        return E_POINTER;

    hr = Direct3DRMMesh_create(mesh);
    if (FAILED(hr))
        return hr;

    /* If there is mesh data, create a group and put data inside */
    if (This->nb_vertices)
    {
        unsigned* face_data;
        unsigned* out_ptr;
        DWORD* in_ptr = This->pFaceData;
        int i, j;

        face_data = HeapAlloc(GetProcessHeap(), 0, This->face_data_size * sizeof(DWORD));
        if (!face_data)
        {
            IDirect3DRMMesh_Release(*mesh);
            return E_OUTOFMEMORY;
        }
        out_ptr = face_data;

        /* Put only vertex indices */
        for (i = 0; i < This->nb_faces; i++)
        {
            DWORD nb_indices = *out_ptr++ = *in_ptr++;
            for (j = 0; j < nb_indices; j++)
            {
                *out_ptr++ = *in_ptr++;
                /* Skip normal index */
                in_ptr++;
            }
        }

        hr = IDirect3DRMMesh_AddGroup(*mesh, This->nb_vertices, This->nb_faces, 0, face_data, &group);
        HeapFree(GetProcessHeap(), 0, face_data);
        if (SUCCEEDED(hr))
        {
            D3DRMVERTEX* vertices;

            vertices = HeapAlloc(GetProcessHeap(), 0, This->nb_vertices * sizeof(D3DRMVERTEX));
            if (vertices)
            {
                for (i = 0; i < This->nb_vertices; i++)
                {
                    vertices[i].position = This->pVertices[i];
                }
                hr = IDirect3DRMMesh_SetVertices(*mesh, 0, 0, This->nb_vertices, vertices);
                HeapFree(GetProcessHeap(), 0, vertices);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        if (SUCCEEDED(hr))
            hr = IDirect3DRMMesh_SetGroupColor(*mesh, 0, This->color);
        if (SUCCEEDED(hr))
            hr = IDirect3DRMMesh_SetGroupMaterial(*mesh, 0, (LPDIRECT3DRMMATERIAL)This->material);
        if (SUCCEEDED(hr) && This->texture)
        {
             LPDIRECT3DRMTEXTURE texture;

             IDirect3DRMTexture3_QueryInterface(This->texture, &IID_IDirect3DRMTexture, (LPVOID*)&texture);
             hr = IDirect3DRMMesh_SetGroupTexture(*mesh, 0, texture);
             IDirect3DRMTexture_Release(texture);
        }
        if (FAILED(hr))
            IDirect3DRMMesh_Release(*mesh);
    }

    return hr;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetFace(IDirect3DRMMeshBuilder3* iface,
                                                          DWORD index, LPDIRECT3DRMFACE2* Face)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u,%p): stub\n", This, index, Face);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetVertex(IDirect3DRMMeshBuilder3* iface,
                                                            DWORD index, LPD3DVECTOR vector)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u,%p): stub\n", This, index, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetNormal(IDirect3DRMMeshBuilder3* iface,
                                                            DWORD index, LPD3DVECTOR vector)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u,%p): stub\n", This, index, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_DeleteVertices(IDirect3DRMMeshBuilder3* iface,
                                                                 DWORD IndexFirst, DWORD count)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u,%u): stub\n", This, IndexFirst, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_DeleteNormals(IDirect3DRMMeshBuilder3* iface,
                                                                DWORD IndexFirst, DWORD count)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u,%u): stub\n", This, IndexFirst, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_DeleteFace(IDirect3DRMMeshBuilder3* iface,
                                                             LPDIRECT3DRMFACE2 Face)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p): stub\n", This, Face);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_Empty(IDirect3DRMMeshBuilder3* iface, DWORD flags)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u): stub\n", This, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_Optimize(IDirect3DRMMeshBuilder3* iface,
                                                           DWORD flags)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u): stub\n", This, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_AddFacesIndexed(IDirect3DRMMeshBuilder3* iface,
                                                                  DWORD flags, DWORD* indices,
                                                                  DWORD* IndexFirst, DWORD* count)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u,%p,%p,%p): stub\n", This, flags, indices, IndexFirst, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_CreateSubMesh(IDirect3DRMMeshBuilder3* iface,
                                                                LPUNKNOWN *unkwn)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p): stub\n", This, unkwn);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetParentMesh(IDirect3DRMMeshBuilder3* iface,
                                                                DWORD flags, LPUNKNOWN *unkwn)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u,%p): stub\n", This, flags, unkwn);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetSubMeshes(IDirect3DRMMeshBuilder3* iface,
                                                               LPDWORD count, LPUNKNOWN *unkwn)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, count, unkwn);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_DeleteSubMesh(IDirect3DRMMeshBuilder3* iface,
                                                                LPUNKNOWN unkwn)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p): stub\n", This, unkwn);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_Enable(IDirect3DRMMeshBuilder3* iface,
                                                         DWORD index)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u): stub\n", This, index);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetEnable(IDirect3DRMMeshBuilder3* iface,
                                                            DWORD* indices)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p): stub\n", This, indices);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_AddTriangles(IDirect3DRMMeshBuilder3* iface,
                                                               DWORD flags, DWORD format,
                                                               DWORD VertexCount, LPVOID data)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u,%u,%u,%p): stub\n", This, flags, format, VertexCount, data);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetVertices(IDirect3DRMMeshBuilder3* iface,
                                                              DWORD IndexFirst, DWORD count,
                                                              LPD3DVECTOR vector)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u,%u,%p): stub\n", This, IndexFirst, count, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetVertices(IDirect3DRMMeshBuilder3* iface,
                                                              DWORD IndexFirst, LPDWORD vcount,
                                                              LPD3DVECTOR vertices)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);
    DWORD count = This->nb_vertices - IndexFirst;

    TRACE("(%p)->(%u,%p,%p)\n", This, IndexFirst, vcount, vertices);

    if (vcount)
        *vcount = count;
    if (vertices && This->nb_vertices)
        memcpy(vertices, This->pVertices + IndexFirst, count * sizeof(D3DVECTOR));

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetNormals(IDirect3DRMMeshBuilder3* iface,
                                                             DWORD IndexFirst, DWORD count,
                                                             LPD3DVECTOR vector)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u,%u,%p): stub\n", This, IndexFirst, count, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetNormals(IDirect3DRMMeshBuilder3* iface,
                                                             DWORD IndexFirst, LPDWORD count,
                                                             LPD3DVECTOR vector)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%u,%p,%p): stub\n", This, IndexFirst, count, vector);

    return E_NOTIMPL;
}

static int WINAPI IDirect3DRMMeshBuilder3Impl_GetNormalCount(IDirect3DRMMeshBuilder3* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static const struct IDirect3DRMMeshBuilder3Vtbl Direct3DRMMeshBuilder3_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMMeshBuilder3Impl_QueryInterface,
    IDirect3DRMMeshBuilder3Impl_AddRef,
    IDirect3DRMMeshBuilder3Impl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMMeshBuilder3Impl_Clone,
    IDirect3DRMMeshBuilder3Impl_AddDestroyCallback,
    IDirect3DRMMeshBuilder3Impl_DeleteDestroyCallback,
    IDirect3DRMMeshBuilder3Impl_SetAppData,
    IDirect3DRMMeshBuilder3Impl_GetAppData,
    IDirect3DRMMeshBuilder3Impl_SetName,
    IDirect3DRMMeshBuilder3Impl_GetName,
    IDirect3DRMMeshBuilder3Impl_GetClassName,
    /*** IDirect3DRMMeshBuilder3 methods ***/
    IDirect3DRMMeshBuilder3Impl_Load,
    IDirect3DRMMeshBuilder3Impl_Save,
    IDirect3DRMMeshBuilder3Impl_Scale,
    IDirect3DRMMeshBuilder3Impl_Translate,
    IDirect3DRMMeshBuilder3Impl_SetColorSource,
    IDirect3DRMMeshBuilder3Impl_GetBox,
    IDirect3DRMMeshBuilder3Impl_GenerateNormals,
    IDirect3DRMMeshBuilder3Impl_GetColorSource,
    IDirect3DRMMeshBuilder3Impl_AddMesh,
    IDirect3DRMMeshBuilder3Impl_AddMeshBuilder,
    IDirect3DRMMeshBuilder3Impl_AddFrame,
    IDirect3DRMMeshBuilder3Impl_AddFace,
    IDirect3DRMMeshBuilder3Impl_AddFaces,
    IDirect3DRMMeshBuilder3Impl_ReserveSpace,
    IDirect3DRMMeshBuilder3Impl_SetColorRGB,
    IDirect3DRMMeshBuilder3Impl_SetColor,
    IDirect3DRMMeshBuilder3Impl_SetTexture,
    IDirect3DRMMeshBuilder3Impl_SetMaterial,
    IDirect3DRMMeshBuilder3Impl_SetTextureTopology,
    IDirect3DRMMeshBuilder3Impl_SetQuality,
    IDirect3DRMMeshBuilder3Impl_SetPerspective,
    IDirect3DRMMeshBuilder3Impl_SetVertex,
    IDirect3DRMMeshBuilder3Impl_SetNormal,
    IDirect3DRMMeshBuilder3Impl_SetTextureCoordinates,
    IDirect3DRMMeshBuilder3Impl_SetVertexColor,
    IDirect3DRMMeshBuilder3Impl_SetVertexColorRGB,
    IDirect3DRMMeshBuilder3Impl_GetFaces,
    IDirect3DRMMeshBuilder3Impl_GetGeometry,
    IDirect3DRMMeshBuilder3Impl_GetTextureCoordinates,
    IDirect3DRMMeshBuilder3Impl_AddVertex,
    IDirect3DRMMeshBuilder3Impl_AddNormal,
    IDirect3DRMMeshBuilder3Impl_CreateFace,
    IDirect3DRMMeshBuilder3Impl_GetQuality,
    IDirect3DRMMeshBuilder3Impl_GetPerspective,
    IDirect3DRMMeshBuilder3Impl_GetFaceCount,
    IDirect3DRMMeshBuilder3Impl_GetVertexCount,
    IDirect3DRMMeshBuilder3Impl_GetVertexColor,
    IDirect3DRMMeshBuilder3Impl_CreateMesh,
    IDirect3DRMMeshBuilder3Impl_GetFace,
    IDirect3DRMMeshBuilder3Impl_GetVertex,
    IDirect3DRMMeshBuilder3Impl_GetNormal,
    IDirect3DRMMeshBuilder3Impl_DeleteVertices,
    IDirect3DRMMeshBuilder3Impl_DeleteNormals,
    IDirect3DRMMeshBuilder3Impl_DeleteFace,
    IDirect3DRMMeshBuilder3Impl_Empty,
    IDirect3DRMMeshBuilder3Impl_Optimize,
    IDirect3DRMMeshBuilder3Impl_AddFacesIndexed,
    IDirect3DRMMeshBuilder3Impl_CreateSubMesh,
    IDirect3DRMMeshBuilder3Impl_GetParentMesh,
    IDirect3DRMMeshBuilder3Impl_GetSubMeshes,
    IDirect3DRMMeshBuilder3Impl_DeleteSubMesh,
    IDirect3DRMMeshBuilder3Impl_Enable,
    IDirect3DRMMeshBuilder3Impl_GetEnable,
    IDirect3DRMMeshBuilder3Impl_AddTriangles,
    IDirect3DRMMeshBuilder3Impl_SetVertices,
    IDirect3DRMMeshBuilder3Impl_GetVertices,
    IDirect3DRMMeshBuilder3Impl_SetNormals,
    IDirect3DRMMeshBuilder3Impl_GetNormals,
    IDirect3DRMMeshBuilder3Impl_GetNormalCount
};

HRESULT Direct3DRMMeshBuilder_create(REFIID riid, IUnknown** ppObj)
{
    IDirect3DRMMeshBuilderImpl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DRMMeshBuilderImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->IDirect3DRMMeshBuilder2_iface.lpVtbl = &Direct3DRMMeshBuilder2_Vtbl;
    object->IDirect3DRMMeshBuilder3_iface.lpVtbl = &Direct3DRMMeshBuilder3_Vtbl;
    object->ref = 1;

    if (IsEqualGUID(riid, &IID_IDirect3DRMMeshBuilder3))
        *ppObj = (IUnknown*)&object->IDirect3DRMMeshBuilder3_iface;
    else
        *ppObj = (IUnknown*)&object->IDirect3DRMMeshBuilder2_iface;

    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMMeshImpl_QueryInterface(IDirect3DRMMesh* iface,
                                                         REFIID riid, void** ppvObject)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirect3DRMMesh))
    {
        *ppvObject = &This->IDirect3DRMMesh_iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDirect3DRMMesh_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI IDirect3DRMMeshImpl_AddRef(IDirect3DRMMesh* iface)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirect3DRMMeshImpl_Release(IDirect3DRMMesh* iface)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    if (!ref)
    {
        int i;

        for (i = 0; i < This->nb_groups; i++)
        {
            HeapFree(GetProcessHeap(), 0, This->groups[i].vertices);
            HeapFree(GetProcessHeap(), 0, This->groups[i].face_data);
            if (This->groups[i].material)
                IDirect3DRMMaterial2_Release(This->groups[i].material);
            if (This->groups[i].texture)
                IDirect3DRMTexture3_Release(This->groups[i].texture);
        }
        HeapFree(GetProcessHeap(), 0, This->groups);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** IDirect3DRMObject methods ***/
static HRESULT WINAPI IDirect3DRMMeshImpl_Clone(IDirect3DRMMesh* iface,
                                                LPUNKNOWN pUnkOuter, REFIID riid,
                                                LPVOID *ppvObj)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    FIXME("(%p)->(%p,%s,%p): stub\n", This, pUnkOuter, debugstr_guid(riid), ppvObj);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_AddDestroyCallback(IDirect3DRMMesh* iface,
                                                             D3DRMOBJECTCALLBACK cb,
                                                             LPVOID argument)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_DeleteDestroyCallback(IDirect3DRMMesh* iface,
                                                                 D3DRMOBJECTCALLBACK cb,
                                                                 LPVOID argument)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_SetAppData(IDirect3DRMMesh* iface,
                                                     DWORD data)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    FIXME("(%p)->(%u): stub\n", This, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMMeshImpl_GetAppData(IDirect3DRMMesh* iface)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    FIXME("(%p)->(): stub\n", This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_SetName(IDirect3DRMMesh* iface,
                                                  LPCSTR pName)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    FIXME("(%p)->(%s): stub\n", This, pName);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_GetName(IDirect3DRMMesh* iface,
                                                  LPDWORD lpdwSize, LPSTR lpName)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, lpdwSize, lpName);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_GetClassName(IDirect3DRMMesh* iface,
                                                       LPDWORD size, LPSTR name)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    TRACE("(%p)->(%p, %p)\n", This, size, name);

    if (!size || *size < strlen("Mesh") || !name)
        return E_INVALIDARG;

    strcpy(name, "Mesh");
    *size = sizeof("Mesh");

    return D3DRM_OK;
}

/*** IDirect3DRMMesh methods ***/
static HRESULT WINAPI IDirect3DRMMeshImpl_Scale(IDirect3DRMMesh* iface,
                                                D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, sx, sy,sz);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_Translate(IDirect3DRMMesh* iface,
                                                    D3DVALUE tx, D3DVALUE ty, D3DVALUE tz)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, tx, ty,tz);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_GetBox(IDirect3DRMMesh* iface,
                                                 D3DRMBOX * box)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    FIXME("(%p)->(%p): stub\n", This, box);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_AddGroup(IDirect3DRMMesh* iface,
                                                   unsigned vertex_count, unsigned face_count, unsigned vertex_per_face,
                                                   unsigned *face_data, D3DRMGROUPINDEX *return_id)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);
    mesh_group* group;

    TRACE("(%p)->(%u,%u,%u,%p,%p)\n", This, vertex_count, face_count, vertex_per_face, face_data, return_id);

    if (!face_data || !return_id)
        return E_POINTER;

    if ((This->nb_groups + 1) > This->groups_capacity)
    {
        ULONG new_capacity;
        mesh_group* groups;

        if (!This->groups_capacity)
        {
            new_capacity = 16;
            groups = HeapAlloc(GetProcessHeap(), 0, new_capacity * sizeof(mesh_group));
        }
        else
        {
            new_capacity = This->groups_capacity * 2;
            groups = HeapReAlloc(GetProcessHeap(), 0, This->groups, new_capacity * sizeof(mesh_group));
        }

        if (!groups)
            return E_OUTOFMEMORY;

        This->groups_capacity = new_capacity;
        This->groups = groups;
    }

    group = This->groups + This->nb_groups;

    group->vertices = HeapAlloc(GetProcessHeap(), 0, vertex_count * sizeof(D3DRMVERTEX));
    if (!group->vertices)
        return E_OUTOFMEMORY;
    group->nb_vertices = vertex_count;
    group->nb_faces = face_count;
    group->vertex_per_face = vertex_per_face;

    if (vertex_per_face)
    {
        group->face_data_size = face_count * vertex_per_face;
    }
    else
    {
        int i;
        unsigned nb_indices;
        unsigned* face_data_ptr = face_data;
        group->face_data_size = 0;

        for (i = 0; i < face_count; i++)
        {
            nb_indices = *face_data_ptr;
            group->face_data_size += nb_indices + 1;
            face_data_ptr += nb_indices;
        }
    }

    group->face_data = HeapAlloc(GetProcessHeap(), 0, group->face_data_size * sizeof(unsigned));
    if (!group->face_data)
    {
        HeapFree(GetProcessHeap(), 0 , group->vertices);
        return E_OUTOFMEMORY;
    }

    memcpy(group->face_data, face_data, group->face_data_size * sizeof(unsigned));

    group->material = NULL;
    group->texture = NULL;

    *return_id = This->nb_groups++;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_SetVertices(IDirect3DRMMesh* iface,
                                                      D3DRMGROUPINDEX id, unsigned index, unsigned count,
                                                      D3DRMVERTEX *values)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    TRACE("(%p)->(%u,%u,%u,%p)\n", This, id, index, count, values);

    if (id >= This->nb_groups)
        return D3DRMERR_BADVALUE;

    if ((index + count - 1) >= This->groups[id].nb_vertices)
        return D3DRMERR_BADVALUE;

    if (!values)
        return E_POINTER;

    memcpy(This->groups[id].vertices + index, values, count * sizeof(D3DRMVERTEX));

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_SetGroupColor(IDirect3DRMMesh* iface,
                                                        D3DRMGROUPINDEX id, D3DCOLOR color)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    TRACE("(%p)->(%u,%x)\n", This, id, color);

    if (id >= This->nb_groups)
        return D3DRMERR_BADVALUE;

    This->groups[id].color = color;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_SetGroupColorRGB(IDirect3DRMMesh* iface,
                                                           D3DRMGROUPINDEX id, D3DVALUE red, D3DVALUE green, D3DVALUE blue)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    TRACE("(%p)->(%u,%f,%f,%f)\n", This, id, red, green, blue);

    if (id >= This->nb_groups)
        return D3DRMERR_BADVALUE;

    This->groups[id].color = D3DCOLOR_ARGB(0xff, (BYTE)(red   * 255.0f),
                                                 (BYTE)(green * 255.0f),
                                                 (BYTE)(blue  * 255.0f));

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_SetGroupMapping(IDirect3DRMMesh* iface,
                                                          D3DRMGROUPINDEX id, D3DRMMAPPING value)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    FIXME("(%p)->(%u,%u): stub\n", This, id, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_SetGroupQuality(IDirect3DRMMesh* iface,
                                                          D3DRMGROUPINDEX id, D3DRMRENDERQUALITY value)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    FIXME("(%p)->(%u,%u): stub\n", This, id, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_SetGroupMaterial(IDirect3DRMMesh* iface,
                                                           D3DRMGROUPINDEX id, LPDIRECT3DRMMATERIAL material)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    TRACE("(%p)->(%u,%p)\n", This, id, material);

    if (id >= This->nb_groups)
        return D3DRMERR_BADVALUE;

    if (This->groups[id].material)
        IDirect3DRMMaterial2_Release(This->groups[id].material);

    This->groups[id].material = (LPDIRECT3DRMMATERIAL2)material;

    if (material)
        IDirect3DRMMaterial2_AddRef(This->groups[id].material);

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_SetGroupTexture(IDirect3DRMMesh* iface,
                                                          D3DRMGROUPINDEX id, LPDIRECT3DRMTEXTURE texture)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    TRACE("(%p)->(%u,%p)\n", This, id, texture);

    if (id >= This->nb_groups)
        return D3DRMERR_BADVALUE;

    if (This->groups[id].texture)
        IDirect3DRMTexture3_Release(This->groups[id].texture);

    if (!texture)
    {
        This->groups[id].texture = NULL;
        return D3DRM_OK;
    }

    return IDirect3DRMTexture3_QueryInterface(texture, &IID_IDirect3DRMTexture, (LPVOID*)&This->groups[id].texture);
}

static DWORD WINAPI IDirect3DRMMeshImpl_GetGroupCount(IDirect3DRMMesh* iface)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    TRACE("(%p)->()\n", This);

    return This->nb_groups;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_GetGroup(IDirect3DRMMesh* iface,
                                                   D3DRMGROUPINDEX id, unsigned *vertex_count, unsigned *face_count, unsigned *vertex_per_face,
                                                   DWORD *face_data_size, unsigned *face_data)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    TRACE("(%p)->(%u,%p,%p,%p,%p,%p)\n", This, id, vertex_count, face_count, vertex_per_face, face_data_size, face_data);

    if (id >= This->nb_groups)
        return D3DRMERR_BADVALUE;

    if (vertex_count)
        *vertex_count = This->groups[id].nb_vertices;
    if (face_count)
        *face_count = This->groups[id].nb_faces;
    if (vertex_per_face)
        *vertex_per_face = This->groups[id].vertex_per_face;
    if (face_data_size)
        *face_data_size = This->groups[id].face_data_size;
    if (face_data)
        memcpy(face_data, This->groups[id].face_data, This->groups[id].face_data_size);

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_GetVertices(IDirect3DRMMesh* iface,
                                                      D3DRMGROUPINDEX id, DWORD index, DWORD count, D3DRMVERTEX *return_ptr)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    TRACE("(%p)->(%u,%u,%u,%p)\n", This, id, index, count, return_ptr);

    if (id >= This->nb_groups)
        return D3DRMERR_BADVALUE;

    if ((index + count - 1) >= This->groups[id].nb_vertices)
        return D3DRMERR_BADVALUE;

    if (!return_ptr)
        return E_POINTER;

    memcpy(return_ptr, This->groups[id].vertices + index, count * sizeof(D3DRMVERTEX));

    return D3DRM_OK;
}

static D3DCOLOR WINAPI IDirect3DRMMeshImpl_GetGroupColor(IDirect3DRMMesh* iface, D3DRMGROUPINDEX id)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    TRACE("(%p)->(%u)\n", This, id);

    return This->groups[id].color;
}

static D3DRMMAPPING WINAPI IDirect3DRMMeshImpl_GetGroupMapping(IDirect3DRMMesh* iface, D3DRMGROUPINDEX id)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    FIXME("(%p)->(%u): stub\n", This, id);

    return 0;
}
static D3DRMRENDERQUALITY WINAPI IDirect3DRMMeshImpl_GetGroupQuality(IDirect3DRMMesh* iface, D3DRMGROUPINDEX id)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    FIXME("(%p)->(%u): stub\n", This, id);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_GetGroupMaterial(IDirect3DRMMesh* iface,
                                                           D3DRMGROUPINDEX id, LPDIRECT3DRMMATERIAL *material)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    TRACE("(%p)->(%u,%p)\n", This, id, material);

    if (id >= This->nb_groups)
        return D3DRMERR_BADVALUE;

    if (!material)
        return E_POINTER;

    if (This->groups[id].material)
        IDirect3DRMTexture_QueryInterface(This->groups[id].material, &IID_IDirect3DRMMaterial, (void**)material);
    else
        *material = NULL;

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshImpl_GetGroupTexture(IDirect3DRMMesh* iface,
                                                          D3DRMGROUPINDEX id, LPDIRECT3DRMTEXTURE *texture)
{
    IDirect3DRMMeshImpl *This = impl_from_IDirect3DRMMesh(iface);

    TRACE("(%p)->(%u,%p)\n", This, id, texture);

    if (id >= This->nb_groups)
        return D3DRMERR_BADVALUE;

    if (!texture)
        return E_POINTER;

    if (This->groups[id].texture)
        IDirect3DRMTexture_QueryInterface(This->groups[id].texture, &IID_IDirect3DRMTexture, (void**)texture);
    else
        *texture = NULL;

    return D3DRM_OK;
}

static const struct IDirect3DRMMeshVtbl Direct3DRMMesh_Vtbl =
{
    /*** IUnknown methods ***/
    IDirect3DRMMeshImpl_QueryInterface,
    IDirect3DRMMeshImpl_AddRef,
    IDirect3DRMMeshImpl_Release,
    /*** IDirect3DRMObject methods ***/
    IDirect3DRMMeshImpl_Clone,
    IDirect3DRMMeshImpl_AddDestroyCallback,
    IDirect3DRMMeshImpl_DeleteDestroyCallback,
    IDirect3DRMMeshImpl_SetAppData,
    IDirect3DRMMeshImpl_GetAppData,
    IDirect3DRMMeshImpl_SetName,
    IDirect3DRMMeshImpl_GetName,
    IDirect3DRMMeshImpl_GetClassName,
    /*** IDirect3DRMMesh methods ***/
    IDirect3DRMMeshImpl_Scale,
    IDirect3DRMMeshImpl_Translate,
    IDirect3DRMMeshImpl_GetBox,
    IDirect3DRMMeshImpl_AddGroup,
    IDirect3DRMMeshImpl_SetVertices,
    IDirect3DRMMeshImpl_SetGroupColor,
    IDirect3DRMMeshImpl_SetGroupColorRGB,
    IDirect3DRMMeshImpl_SetGroupMapping,
    IDirect3DRMMeshImpl_SetGroupQuality,
    IDirect3DRMMeshImpl_SetGroupMaterial,
    IDirect3DRMMeshImpl_SetGroupTexture,
    IDirect3DRMMeshImpl_GetGroupCount,
    IDirect3DRMMeshImpl_GetGroup,
    IDirect3DRMMeshImpl_GetVertices,
    IDirect3DRMMeshImpl_GetGroupColor,
    IDirect3DRMMeshImpl_GetGroupMapping,
    IDirect3DRMMeshImpl_GetGroupQuality,
    IDirect3DRMMeshImpl_GetGroupMaterial,
    IDirect3DRMMeshImpl_GetGroupTexture
};

HRESULT Direct3DRMMesh_create(IDirect3DRMMesh** obj)
{
    IDirect3DRMMeshImpl* object;

    TRACE("(%p)\n", obj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DRMMeshImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->IDirect3DRMMesh_iface.lpVtbl = &Direct3DRMMesh_Vtbl;
    object->ref = 1;

    *obj = &object->IDirect3DRMMesh_iface;

    return S_OK;
}
