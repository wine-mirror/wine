/*
 * Implementation of IDirect3DRMMeshBuilder2 Interface
 *
 * Copyright 2010 Christian Costa
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

#include "wine/debug.h"

#define COBJMACROS

#include "winbase.h"
#include "wingdi.h"
#include "dxfile.h"
#include "rmxfguid.h"

#include "d3drm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3drm);

typedef struct {
    D3DVALUE u;
    D3DVALUE v;
} Coords2d;

typedef struct {
    IDirect3DRMMeshBuilder2 IDirect3DRMMeshBuilder2_iface;
    IDirect3DRMMeshBuilder3 IDirect3DRMMeshBuilder3_iface;
    LONG ref;
    DWORD nb_vertices;
    D3DVECTOR* pVertices;
    DWORD nb_normals;
    D3DVECTOR* pNormals;
    DWORD nb_faces;
    DWORD face_data_size;
    LPVOID pFaceData;
    DWORD nb_coords2d;
    Coords2d* pCoords2d;
} IDirect3DRMMeshBuilderImpl;

typedef struct {
    WORD major;
    WORD minor;
    DWORD flags;
} Header;

static char templates[] = {
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

    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IDirect3DRMMeshBuilder2Impl_Release(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)\n", This);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This->pVertices);
        HeapFree(GetProcessHeap(), 0, This->pNormals);
        HeapFree(GetProcessHeap(), 0, This->pFaceData);
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
                                                          LPCSTR pName)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%s): stub\n", This, pName);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetName(IDirect3DRMMeshBuilder2* iface,
                                                          LPDWORD lpdwSize, LPSTR lpName)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, lpdwSize, lpName);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetClassName(IDirect3DRMMeshBuilder2* iface,
                                                               LPDWORD lpdwSize, LPSTR lpName)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, lpdwSize, lpName);

    return E_NOTIMPL;
}

/*** IDirect3DRMMeshBuilder2 methods ***/
static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_Load(IDirect3DRMMeshBuilder2* iface,
                                                       LPVOID filename, LPVOID name,
                                                       D3DRMLOADOPTIONS loadflags,
                                                       D3DRMLOADTEXTURECALLBACK cb, LPVOID arg)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%p,%p,%x,%p,%p): partial stub\n", This, filename, name, loadflags, cb, arg);

    return IDirect3DRMMeshBuilder3_Load(&This->IDirect3DRMMeshBuilder3_iface, filename, name,
                                        loadflags, (D3DRMLOADTEXTURE3CALLBACK)cb, arg);
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

    FIXME("(%p)->(%f,%f,%f): stub\n", This, sx, sy, sz);

    return E_NOTIMPL;
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

    FIXME("(%p)->(%f,%f,%f): stub\n", This, red, green, blue);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetColor(IDirect3DRMMeshBuilder2* iface,
                                                           D3DCOLOR color)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%x): stub\n", This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetTexture(IDirect3DRMMeshBuilder2* iface,
                                                             LPDIRECT3DRMTEXTURE pTexture)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pTexture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetMaterial(IDirect3DRMMeshBuilder2* iface,
                                                              LPDIRECT3DRMMATERIAL pMaterial)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pMaterial);

    return E_NOTIMPL;
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

    FIXME("(%p)->(%f,%f): stub\n", This, u, v);

    return E_NOTIMPL;
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
        memcpy(face_data, This->pFaceData, This->face_data_size);

    return D3DRM_OK;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetTextureCoordinates(IDirect3DRMMeshBuilder2* iface,
                                                                        DWORD index,
                                                                        D3DVALUE *u, D3DVALUE *v)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%d,%p,%p)\n", This, index, u, v);

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
                                                             LPDIRECT3DRMMESH* ppMesh)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, ppMesh);

    return E_NOTIMPL;
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
                                                          LPCSTR pName)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%s): stub\n", This, pName);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetName(IDirect3DRMMeshBuilder3* iface,
                                                          LPDWORD lpdwSize, LPSTR lpName)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, lpdwSize, lpName);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_GetClassName(IDirect3DRMMeshBuilder3* iface,
                                                               LPDWORD lpdwSize, LPSTR lpName)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, lpdwSize, lpName);

    return E_NOTIMPL;
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
    LPDIRECTXFILEOBJECT pObject = NULL;
    LPDIRECTXFILEDATA pData2 = NULL;
    const GUID* pGuid;
    DWORD size;
    Header* pHeader;
    LPBYTE ptr;
    HRESULT hr;
    HRESULT ret = D3DRMERR_BADOBJECT;

    FIXME("(%p)->(%p,%p,%x,%p,%p): partial stub\n", This, filename, name, loadflags, cb, arg);

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

    hr = IDirectXFileData_GetData(pData, NULL, &size, (void**)&ptr);
    if (hr != DXFILE_OK)
        goto end;

    This->nb_vertices = *(DWORD*)ptr;
    This->nb_faces = *(DWORD*)(ptr + sizeof(DWORD) + This->nb_vertices * sizeof(D3DVECTOR));
    This->face_data_size = size - sizeof(DWORD) - This->nb_vertices * sizeof(D3DVECTOR) - sizeof(DWORD);

    TRACE("Mesh: nb_vertices = %d, nb_faces = %d, face_data_size = %d\n", This->nb_vertices, This->nb_faces, This->face_data_size);

    This->pVertices = HeapAlloc(GetProcessHeap(), 0, This->nb_vertices * sizeof(D3DVECTOR));
    memcpy(This->pVertices, ptr + sizeof(DWORD), This->nb_vertices * sizeof(D3DVECTOR));

    This->pFaceData = HeapAlloc(GetProcessHeap(), 0, This->face_data_size);
    memcpy(This->pFaceData, ptr + sizeof(DWORD) + This->nb_vertices * sizeof(D3DVECTOR) + sizeof(DWORD), This->face_data_size);

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
        {
            IDirectXFileData_Release(pData2);
            goto end;
        }

        TRACE("Found object type whose GUID = %s\n", debugstr_guid(pGuid));

        if (!IsEqualGUID(pGuid, &TID_D3DRMMeshNormals))
        {
            DWORD tmp;

            hr = IDirectXFileData_GetData(pData, NULL, &size, (void**)&ptr);
            if (hr != DXFILE_OK)
                goto end;

            This->nb_normals = *(DWORD*)ptr;
            tmp = *(DWORD*)(ptr + sizeof(DWORD) + This->nb_normals * sizeof(D3DVECTOR));

            TRACE("MeshNormals: nb_normals = %d, nb_faces_normals = %d\n", This->nb_normals, tmp);

            This->pNormals = HeapAlloc(GetProcessHeap(), 0, This->nb_normals * sizeof(D3DVECTOR));
            memcpy(This->pNormals, ptr + sizeof(DWORD), This->nb_normals * sizeof(D3DVECTOR));
        }
        else if(!IsEqualGUID(pGuid, &TID_D3DRMMeshTextureCoords))
        {
            hr = IDirectXFileData_GetData(pData, NULL, &size, (void**)&ptr);
            if (hr != DXFILE_OK)
                goto end;

            This->nb_coords2d = *(DWORD*)ptr;

            TRACE("MeshTextureCoords: nb_coords2d = %d\n", This->nb_coords2d);

            This->pCoords2d = HeapAlloc(GetProcessHeap(), 0, This->nb_coords2d * sizeof(Coords2d));
            memcpy(This->pCoords2d, ptr + sizeof(DWORD), This->nb_coords2d * sizeof(Coords2d));

        }
        else if(!IsEqualGUID(pGuid, &TID_D3DRMMeshMaterialList))
        {
            FIXME("MeshMaterialList not supported yet, ignoring...\n");
        }
        else
        {
            FIXME("Unknown GUID %s, ignoring...\n", debugstr_guid(pGuid));
        }

        IDirectXFileData_Release(pData2);
    }

    ret = D3DRM_OK;

end:
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

    FIXME("(%p)->(%f,%f,%f): stub\n", This, sx, sy, sz);

    return E_NOTIMPL;
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

    FIXME("(%p)->(%f,%f,%f): stub\n", This, red, green, blue);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetColor(IDirect3DRMMeshBuilder3* iface,
                                                           D3DCOLOR color)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%x): stub\n", This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetTexture(IDirect3DRMMeshBuilder3* iface,
                                                             LPDIRECT3DRMTEXTURE3 texture)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p): stub\n", This, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder3Impl_SetMaterial(IDirect3DRMMeshBuilder3* iface,
                                                              LPDIRECT3DRMMATERIAL2 material)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p): stub\n", This, material);

    return E_NOTIMPL;
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

    FIXME("(%p)->(%f,%f): stub\n", This, u, v);

    return E_NOTIMPL;
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
        return D3DRMERR_NOTFOUND;

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
                                                             LPDIRECT3DRMMESH* Mesh)
{
    IDirect3DRMMeshBuilderImpl *This = impl_from_IDirect3DRMMeshBuilder3(iface);

    FIXME("(%p)->(%p): stub\n", This, Mesh);

    return E_NOTIMPL;
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
