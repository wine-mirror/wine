/*
 * Implementation of IDirect3DRMMeshBuilder2 Interface
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
} IDirect3DRMMeshBuilder2Impl;

typedef struct {
    WORD major;
    WORD minor;
    DWORD flags;
} Header;

static const struct IDirect3DRMMeshBuilder2Vtbl Direct3DRMMeshBuilder2_Vtbl;

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

static inline IDirect3DRMMeshBuilder2Impl *impl_from_IDirect3DRMMeshBuilder2(IDirect3DRMMeshBuilder2 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DRMMeshBuilder2Impl, IDirect3DRMMeshBuilder2_iface);
}

HRESULT Direct3DRMMeshBuilder_create(IUnknown** ppObj)
{
    IDirect3DRMMeshBuilder2Impl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DRMMeshBuilder2Impl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->IDirect3DRMMeshBuilder2_iface.lpVtbl = &Direct3DRMMeshBuilder2_Vtbl;
    object->ref = 1;

    *ppObj = (IUnknown*)object;

    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_QueryInterface(IDirect3DRMMeshBuilder2* iface,
                                                                 REFIID riid, void** ppvObject)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirect3DRMMeshBuilder) ||
        IsEqualGUID(riid, &IID_IDirect3DRMMeshBuilder2))
    {
        IClassFactory_AddRef(iface);
        *ppvObject = This;
        return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DRMMeshBuilder2Impl_AddRef(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IDirect3DRMMeshBuilder2Impl_Release(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);
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
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p,%s,%p): stub\n", This, pUnkOuter, debugstr_guid(riid), ppvObj);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_AddDestroyCallback(IDirect3DRMMeshBuilder2* iface,
                                                                     D3DRMOBJECTCALLBACK cb,
                                                                     LPVOID argument)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_DeleteDestroyCallback(IDirect3DRMMeshBuilder2* iface,
                                                                        D3DRMOBJECTCALLBACK cb,
                                                                        LPVOID argument)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, cb, argument);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetAppData(IDirect3DRMMeshBuilder2* iface,
                                                             DWORD data)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%u): stub\n", This, data);

    return E_NOTIMPL;
}

static DWORD WINAPI IDirect3DRMMeshBuilder2Impl_GetAppData(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(): stub\n", This);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetName(IDirect3DRMMeshBuilder2* iface,
                                                          LPCSTR pName)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%s): stub\n", This, pName);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetName(IDirect3DRMMeshBuilder2* iface,
                                                          LPDWORD lpdwSize, LPSTR lpName)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, lpdwSize, lpName);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetClassName(IDirect3DRMMeshBuilder2* iface,
                                                               LPDWORD lpdwSize, LPSTR lpName)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, lpdwSize, lpName);

    return E_NOTIMPL;
}

/*** IDirect3DRMMeshBuilder2 methods ***/
static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_Load(IDirect3DRMMeshBuilder2* iface,
                                                       LPVOID filename, LPVOID name,
                                                       D3DRMLOADOPTIONS loadflags,
                                                       D3DRMLOADTEXTURECALLBACK cb, LPVOID pArg)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);
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

    FIXME("(%p)->(%p,%p,%x,%p,%p): partial stub\n", This, filename, name, loadflags, cb, pArg);

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
	    FIXME("no more object\n");
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

        FIXME("Found object type whose GUID = %s\n", debugstr_guid(pGuid));

        if (!IsEqualGUID(pGuid, &TID_D3DRMMeshNormals))
        {
            DWORD tmp;

            hr = IDirectXFileData_GetData(pData, NULL, &size, (void**)&ptr);
            if (hr != DXFILE_OK)
                goto end;

            This->nb_normals = *(DWORD*)ptr;
            tmp = *(DWORD*)(ptr + sizeof(DWORD) + This->nb_normals * sizeof(D3DVECTOR));

            FIXME("MeshNormals: nb_normals = %d, nb_faces_normals = %d\n", This->nb_normals, tmp);

            This->pNormals = HeapAlloc(GetProcessHeap(), 0, This->nb_normals * sizeof(D3DVECTOR));
            memcpy(This->pNormals, ptr + sizeof(DWORD), This->nb_normals * sizeof(D3DVECTOR));
        }
        else if(!IsEqualGUID(pGuid, &TID_D3DRMMeshTextureCoords))
        {
            hr = IDirectXFileData_GetData(pData, NULL, &size, (void**)&ptr);
            if (hr != DXFILE_OK)
                goto end;

            This->nb_coords2d = *(DWORD*)ptr;

            FIXME("MeshTextureCoords: nb_coords2d = %d\n", This->nb_coords2d);

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

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_Save(IDirect3DRMMeshBuilder2* iface,
                                                       const char *filename, D3DRMXOFFORMAT format,
                                                       D3DRMSAVEOPTIONS save)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%s,%d,%d): stub\n", This, filename, format, save);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_Scale(IDirect3DRMMeshBuilder2* iface,
                                                        D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, sx, sy, sz);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_Translate(IDirect3DRMMeshBuilder2* iface,
                                                            D3DVALUE tx, D3DVALUE ty, D3DVALUE tz)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, tx, ty, tz);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetColorSource(IDirect3DRMMeshBuilder2* iface,
                                                                 D3DRMCOLORSOURCE color)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%x): stub\n", This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetBox(IDirect3DRMMeshBuilder2* iface,
                                                         D3DRMBOX *pBox)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pBox);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GenerateNormals(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static D3DRMCOLORSOURCE WINAPI IDirect3DRMMeshBuilder2Impl_GetColorSource(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_AddMesh(IDirect3DRMMeshBuilder2* iface,
                                                          LPDIRECT3DRMMESH pMesh)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pMesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_AddMeshBuilder(IDirect3DRMMeshBuilder2* iface,
                                                                 LPDIRECT3DRMMESHBUILDER pMeshBuilder)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pMeshBuilder);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_AddFrame(IDirect3DRMMeshBuilder2* iface,
                                                           LPDIRECT3DRMFRAME pFrame)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pFrame);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_AddFace(IDirect3DRMMeshBuilder2* iface,
                                                          LPDIRECT3DRMFACE pFace)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pFace);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_AddFaces(IDirect3DRMMeshBuilder2* iface,
                                                           DWORD vcount, D3DVECTOR *vertices,
                                                           DWORD ncount, D3DVECTOR *normals,
                                                           DWORD *data,
                                                           LPDIRECT3DRMFACEARRAY* pFaceArray)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d,%p,%d,%p,%p,%p): stub\n", This, vcount, vertices, ncount, normals, data, pFaceArray);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_ReserveSpace(IDirect3DRMMeshBuilder2* iface,
                                                               DWORD vertex_Count,
                                                               DWORD normal_count,
                                                               DWORD face_count)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d,%d,%d): stub\n", This, vertex_Count, normal_count, face_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetColorRGB(IDirect3DRMMeshBuilder2* iface,
                                                              D3DVALUE red, D3DVALUE green,
                                                              D3DVALUE blue)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, red, green, blue);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetColor(IDirect3DRMMeshBuilder2* iface,
                                                           D3DCOLOR color)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%x): stub\n", This, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetTexture(IDirect3DRMMeshBuilder2* iface,
                                                             LPDIRECT3DRMTEXTURE pTexture)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pTexture);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetMaterial(IDirect3DRMMeshBuilder2* iface,
                                                              LPDIRECT3DRMMATERIAL pMaterial)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pMaterial);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetTextureTopology(IDirect3DRMMeshBuilder2* iface,
                                                                     BOOL wrap_u, BOOL wrap_v)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d,%d): stub\n", This, wrap_u, wrap_v);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetQuality(IDirect3DRMMeshBuilder2* iface,
                                                             D3DRMRENDERQUALITY quality)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d): stub\n", This, quality);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetPerspective(IDirect3DRMMeshBuilder2* iface,
                                                                 BOOL enable)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d): stub\n", This, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetVertex(IDirect3DRMMeshBuilder2* iface,
                                                            DWORD index,
                                                            D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetNormal(IDirect3DRMMeshBuilder2* iface,
                                                            DWORD index,
                                                            D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetTextureCoordinates(IDirect3DRMMeshBuilder2* iface,
                                                                        DWORD index,
                                                                        D3DVALUE u, D3DVALUE v)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%f): stub\n", This, u, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetVertexColor(IDirect3DRMMeshBuilder2* iface,
                                                                 DWORD index, D3DCOLOR color)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d,%x): stub\n", This, index, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_SetVertexColorRGB(IDirect3DRMMeshBuilder2* iface,
                                                                    DWORD index, D3DVALUE red,
                                                                    D3DVALUE green, D3DVALUE blue)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d,%f,%f,%f): stub\n", This, index, red, green, blue);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetFaces(IDirect3DRMMeshBuilder2* iface,
                                                           LPDIRECT3DRMFACEARRAY* pFaceArray)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, pFaceArray);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetVertices(IDirect3DRMMeshBuilder2* iface,
                                                              DWORD *vcount, D3DVECTOR *vertices,
                                                              DWORD *ncount, D3DVECTOR *normals,
                                                              DWORD *face_data_size,
                                                              DWORD *face_data)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

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
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d,%p,%p): stub\n", This, index, u, v);

    if (index >= This->nb_coords2d)
        return D3DRMERR_NOTFOUND;

    *u = This->pCoords2d[index].u;
    *v = This->pCoords2d[index].v;

    return D3DRM_OK;
}

static int WINAPI IDirect3DRMMeshBuilder2Impl_AddVertex(IDirect3DRMMeshBuilder2* iface,
                                                        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return 0;
}

static int WINAPI IDirect3DRMMeshBuilder2Impl_AddNormal(IDirect3DRMMeshBuilder2* iface,
                                                        D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%f,%f): stub\n", This, x, y, z);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_CreateFace(IDirect3DRMMeshBuilder2* iface,
                                                             LPDIRECT3DRMFACE* ppFace)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, ppFace);

    return E_NOTIMPL;
}

static D3DRMRENDERQUALITY WINAPI IDirect3DRMMeshBuilder2Impl_GetQuality(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(): stub\n", This);

    return 0;
}

static BOOL WINAPI IDirect3DRMMeshBuilder2Impl_GetPerspective(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(): stub\n", This);

    return FALSE;
}

static int WINAPI IDirect3DRMMeshBuilder2Impl_GetFaceCount(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->()\n", This);

    return This->nb_faces;
}

static int WINAPI IDirect3DRMMeshBuilder2Impl_GetVertexCount(IDirect3DRMMeshBuilder2* iface)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    TRACE("(%p)->()\n", This);

    return This->nb_vertices;
}

static D3DCOLOR WINAPI IDirect3DRMMeshBuilder2Impl_GetVertexColor(IDirect3DRMMeshBuilder2* iface,
                                                                  DWORD index)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%d): stub\n", This, index);

    return 0;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_CreateMesh(IDirect3DRMMeshBuilder2* iface,
                                                             LPDIRECT3DRMMESH* ppMesh)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%p): stub\n", This, ppMesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GenerateNormals2(IDirect3DRMMeshBuilder2* iface,
                                                                   D3DVALUE crease, DWORD dwFlags)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

    FIXME("(%p)->(%f,%u): stub\n", This, crease, dwFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirect3DRMMeshBuilder2Impl_GetFace(IDirect3DRMMeshBuilder2* iface,
                                                          DWORD index, LPDIRECT3DRMFACE* ppFace)
{
    IDirect3DRMMeshBuilder2Impl *This = impl_from_IDirect3DRMMeshBuilder2(iface);

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
