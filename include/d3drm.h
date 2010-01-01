/*
 * Copyright (C) 2005 Peter Berg Larsen
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

#ifndef __D3DRM_H__
#define __D3DRM_H__

#include <ddraw.h>
#include <d3drmobj.h>


/* Direct3DRM Object CLSID */
DEFINE_GUID(CLSID_CDirect3DRM,              0x4516ec41, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);

/* Direct3DRM Interface GUIDs */
DEFINE_GUID(IID_IDirect3DRM,                0x2bc49361, 0x8327, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRM2,               0x4516ecc8, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRM3,               0x4516ec83, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);

typedef void *LPDIRECT3DRM;

HRESULT WINAPI Direct3DRMCreate(LPDIRECT3DRM* ppDirect3DRM);

/*****************************************************************************
 * IDirect3DRMObject interface
 */
#ifdef WINE_NO_UNICODE_MACROS
#undef GetClassName
#endif
#define INTERFACE IDirect3DRM
DECLARE_INTERFACE_(IDirect3DRM,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DRM methods ***/
    STDMETHOD(CreateObject)(THIS_ REFCLSID rclsid, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD(CreateFrame)(THIS_ LPDIRECT3DRMFRAME, LPDIRECT3DRMFRAME *) PURE;
    STDMETHOD(CreateMesh)(THIS_ LPDIRECT3DRMMESH *) PURE;
    STDMETHOD(CreateMeshBuilder)(THIS_ LPDIRECT3DRMMESHBUILDER *) PURE;
    STDMETHOD(CreateFace)(THIS_ LPDIRECT3DRMFACE *) PURE;
    STDMETHOD(CreateAnimation)(THIS_ LPDIRECT3DRMANIMATION *) PURE;
    STDMETHOD(CreateAnimationSet)(THIS_ LPDIRECT3DRMANIMATIONSET *) PURE;
    STDMETHOD(CreateTexture)(THIS_ LPD3DRMIMAGE, LPDIRECT3DRMTEXTURE *) PURE;
    STDMETHOD(CreateLight)(THIS_ D3DRMLIGHTTYPE, D3DCOLOR, LPDIRECT3DRMLIGHT *) PURE;
    STDMETHOD(CreateLightRGB)(THIS_ D3DRMLIGHTTYPE, D3DVALUE, D3DVALUE, D3DVALUE, LPDIRECT3DRMLIGHT *) PURE;
    STDMETHOD(CreateMaterial)(THIS_ D3DVALUE, LPDIRECT3DRMMATERIAL *) PURE;
    STDMETHOD(CreateDevice)(THIS_ DWORD, DWORD, LPDIRECT3DRMDEVICE *) PURE;
    STDMETHOD(CreateDeviceFromSurface)(THIS_ LPGUID pGUID, LPDIRECTDRAW pDD, LPDIRECTDRAWSURFACE pDDSBack,
        LPDIRECT3DRMDEVICE *) PURE;
    STDMETHOD(CreateDeviceFromD3D)(THIS_ LPDIRECT3D pD3D, LPDIRECT3DDEVICE pD3DDev, LPDIRECT3DRMDEVICE *) PURE;
    STDMETHOD(CreateDeviceFromClipper)(THIS_ LPDIRECTDRAWCLIPPER pDDClipper, LPGUID pGUID, int width, int height,
        LPDIRECT3DRMDEVICE *) PURE;
    STDMETHOD(CreateShadow)(THIS_ LPDIRECT3DRMVISUAL, LPDIRECT3DRMLIGHT, D3DVALUE px, D3DVALUE py, D3DVALUE pz,
        D3DVALUE nx, D3DVALUE ny, D3DVALUE nz, LPDIRECT3DRMVISUAL *) PURE;
    STDMETHOD(CreateTextureFromSurface)(THIS_ LPDIRECTDRAWSURFACE pDDS, LPDIRECT3DRMTEXTURE *) PURE;
    STDMETHOD(CreateViewport)(THIS_ LPDIRECT3DRMDEVICE, LPDIRECT3DRMFRAME, DWORD, DWORD, DWORD, DWORD,
        LPDIRECT3DRMVIEWPORT *) PURE;
    STDMETHOD(CreateWrap)(THIS_ D3DRMWRAPTYPE, LPDIRECT3DRMFRAME, D3DVALUE ox, D3DVALUE oy, D3DVALUE oz,
        D3DVALUE dx, D3DVALUE dy, D3DVALUE dz, D3DVALUE ux, D3DVALUE uy, D3DVALUE uz, D3DVALUE ou, D3DVALUE ov,
        D3DVALUE su, D3DVALUE sv, LPDIRECT3DRMWRAP *) PURE;
    STDMETHOD(CreateUserVisual)(THIS_ D3DRMUSERVISUALCALLBACK, LPVOID pArg, LPDIRECT3DRMUSERVISUAL *) PURE;
    STDMETHOD(LoadTexture)(THIS_ const char *, LPDIRECT3DRMTEXTURE *) PURE;
    STDMETHOD(LoadTextureFromResource)(THIS_ HRSRC rs, LPDIRECT3DRMTEXTURE *) PURE;
    STDMETHOD(SetSearchPath)(THIS_ LPCSTR) PURE;
    STDMETHOD(AddSearchPath)(THIS_ LPCSTR) PURE;
    STDMETHOD(GetSearchPath)(THIS_ DWORD *size_return, LPSTR path_return) PURE;
    STDMETHOD(SetDefaultTextureColors)(THIS_ DWORD) PURE;
    STDMETHOD(SetDefaultTextureShades)(THIS_ DWORD) PURE;
    STDMETHOD(GetDevices)(THIS_ LPDIRECT3DRMDEVICEARRAY *) PURE;
    STDMETHOD(GetNamedObject)(THIS_ const char *, LPDIRECT3DRMOBJECT *) PURE;
    STDMETHOD(EnumerateObjects)(THIS_ D3DRMOBJECTCALLBACK, LPVOID) PURE;
    STDMETHOD(Load)(THIS_ LPVOID, LPVOID, LPIID *, DWORD, D3DRMLOADOPTIONS, D3DRMLOADCALLBACK, LPVOID,
        D3DRMLOADTEXTURECALLBACK, LPVOID, LPDIRECT3DRMFRAME) PURE;
    STDMETHOD(Tick)(THIS_ D3DVALUE) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirect3DRM_QueryInterface(p,a,b)                         (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DRM_AddRef(p)                                     (p)->lpVtbl->AddRef(p)
#define IDirect3DRM_Release(p)                                    (p)->lpVtbl->Release(p)
/*** IDirect3DRM methods ***/
#define IDirect3DRM_CreateObject(p,a,b,c,d)                       (p)->lpVtbl->CreateObject(p,a,b,d)
#define IDirect3DRM_CreateFrame(p,a,b)                            (p)->lpVtbl->CreateFrame(p,a,b)
#define IDirect3DRM_CreateMesh(p,a)                               (p)->lpVtbl->CreateMesh(p,a)
#define IDirect3DRM_CreateMeshBuilder(p,a)                        (p)->lpVtbl->CreateMeshBuilder(p,a)
#define IDirect3DRM_CreateFace(p,a)                               (p)->lpVtbl->CreateFace(p,a)
#define IDirect3DRM_CreateAnimation(p,a)                          (p)->lpVtbl->CreateAnimation(p,a)
#define IDirect3DRM_CreateAnimationSet(p,a)                       (p)->lpVtbl->CreateAnimationSet(p,a)
#define IDirect3DRM_CreateTexture(p,a,b)                          (p)->lpVtbl->CreateTexture(p,a,b)
#define IDirect3DRM_CreateLight(p,a,b,c)                          (p)->lpVtbl->CreateLight(p,a,b,c)
#define IDirect3DRM_CreateLightRGB(p,a,b,c,d,e)                   (p)->lpVtbl->CreateLightRGB(p,a,b,c,d,e)
#define IDirect3DRM_CreateMaterial(p,a,b)                         (p)->lpVtbl->CreateMaterial(p,a,b)
#define IDirect3DRM_CreateDevice(p,a,b,c)                         (p)->lpVtbl->CreateDevice(p,a,b,c)
#define IDirect3DRM_CreateDeviceFromSurface(p,a,b,c,d)            (p)->lpVtbl->CreateDeviceFromSurface(p,a,b,c,d)
#define IDirect3DRM_CreateDeviceFromD3D(p,a,b,c)                  (p)->lpVtbl->CreateDeviceFromD3D(p,a,b,c)
#define IDirect3DRM_CreateDeviceFromClipper(p,a,b,c,d,e)          (p)->lpVtbl->CreateDeviceFromClipper(p,a,b,c,d,e)
#define IDirect3DRM_CreateTextureFromSurface(p,a,b)               (p)->lpVtbl->CreateTextureFromSurface(p,a,b)
#define IDirect3DRM_CreateShadow(p,a,b,c,d,e,f,g,h,i)             (p)->lpVtbl->CreateShadow(p,a,b,c,d,e,f,g,h,i)
#define IDirect3DRM_CreateViewport(p,a,b,c,d,e,f,g)               (p)->lpVtbl->CreateViewport(p,a,b,c,d,e,f,g)
#define IDirect3DRM_CreateWrap(p,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,q) (p)->lpVtbl->CreateWrap(p,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,q)
#define IDirect3DRM_CreateUserVisual(p,a,b,c)                     (p)->lpVtbl->CreateUserVisual(p,a,b,c)
#define IDirect3DRM_LoadTexture(p,a,b)                            (p)->lpVtbl->LoadTexture(p,a,b)
#define IDirect3DRM_LoadTextureFromResource(p,a,b)                (p)->lpVtbl->LoadTextureFromResource(p,a,b)
#define IDirect3DRM_SetSearchPath(p,a)                            (p)->lpVtbl->SetSearchPath(p,a)
#define IDirect3DRM_AddSearchPath(p,a)                            (p)->lpVtbl->AddSearchPath(p,a)
#define IDirect3DRM_GetSearchPath(p,a,b)                          (p)->lpVtbl->GetSearchPath(p,a,b)
#define IDirect3DRM_SetDefaultTextureColors(p,a)                  (p)->lpVtbl->SetDefaultTextureColors(p,a)
#define IDirect3DRM_SetDefaultTextureShades(p,a)                  (p)->lpVtbl->SetDefaultTextureShades(p,a)
#define IDirect3DRM_GetDevices(p,a)                               (p)->lpVtbl->GetDevices(p,a)
#define IDirect3DRM_GetNamedObject(p,a,b)                         (p)->lpVtbl->GetNamedObject(p,a,b)
#define IDirect3DRM_EnumerateObjects(p,a,b)                       (p)->lpVtbl->EnumerateObjects(p,a,b)
#define IDirect3DRM_Load(p,a,b,c,d,e,f,g,h,i,j)                   (p)->lpVtbl->Load(p,a,b,c,d,e,f,g,h,i,j)
#define IDirect3DRM_Tick(p,a)                                     (p)->lpVtbl->Tick(p,a)
#else
/*** IUnknown methods ***/
#define IDirect3DRM_QueryInterface(p,a,b)                         (p)->QueryInterface(a,b)
#define IDirect3DRM_AddRef(p)                                     (p)->AddRef()
#define IDirect3DRM_Release(p)                                    (p)->Release()
/*** IDirect3DRM methods ***/
#define IDirect3DRM_CreateObject(p,a,b,c,d)                       (p)->CreateObject(a,b,d)
#define IDirect3DRM_CreateFrame(p,a,b)                            (p)->CreateFrame(a,b)
#define IDirect3DRM_CreateMesh(p,a)                               (p)->CreateMesh(a)
#define IDirect3DRM_CreateMeshBuilder(p,a)                        (p)->CreateMeshBuilder(a)
#define IDirect3DRM_CreateFace(p,a)                               (p)->CreateFace(a)
#define IDirect3DRM_CreateAnimation(p,a)                          (p)->CreateAnimation(a)
#define IDirect3DRM_CreateAnimationSet(p,a)                       (p)->CreateAnimationSet(a)
#define IDirect3DRM_CreateTexture(p,a,b)                          (p)->CreateTexture(a,b)
#define IDirect3DRM_CreateLight(p,a,b,c)                          (p)->CreateLight(a,b,c)
#define IDirect3DRM_CreateLightRGB(p,a,b,c,d,e)                   (p)->CreateLightRGB(a,b,c,d,e)
#define IDirect3DRM_CreateMaterial(p,a,b)                         (p)->CreateMaterial(a,b)
#define IDirect3DRM_CreateDevice(p,a,b,c)                         (p)->CreateDevice(a,b,c)
#define IDirect3DRM_CreateDeviceFromSurface(p,a,b,c,d)            (p)->CreateDeviceFromSurface(a,b,c,d)
#define IDirect3DRM_CreateDeviceFromD3D(p,a,b,c)                  (p)->CreateDeviceFromD3D(a,b,c)
#define IDirect3DRM_CreateDeviceFromClipper(p,a,b,c,d,e)          (p)->CreateDeviceFromClipper(a,b,c,d,e)
#define IDirect3DRM_CreateTextureFromSurface(p,a,b)               (p)->CreateTextureFromSurface(a,b)
#define IDirect3DRM_CreateShadow(p,a,b,c,d,e,f,g,h,i)             (p)->CreateShadow(a,b,c,d,e,f,g,h,i)
#define IDirect3DRM_CreateViewport(p,a,b,c,d,e,f,g)               (p)->CreateViewport(a,b,c,d,e,f,g)
#define IDirect3DRM_CreateWrap(p,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) (p)->CreateWrap(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)
#define IDirect3DRM_CreateUserVisual(p,a,b,c)                     (p)->CreateUserVisual(a,b,c)
#define IDirect3DRM_LoadTexture(p,a,b)                            (p)->LoadTexture(a,b)
#define IDirect3DRM_LoadTextureFromResource(p,a,b)                (p)->LoadTextureFromResource(a,b)
#define IDirect3DRM_SetSearchPath(p,a)                            (p)->SetSearchPath(a)
#define IDirect3DRM_AddSearchPath(p,a)                            (p)->AddSearchPath(a)
#define IDirect3DRM_GetSearchPath(p,a,b)                          (p)->GetSearchPath(a,b)
#define IDirect3DRM_SetDefaultTextureColors(p,a)                  (p)->SetDefaultTextureColors(a)
#define IDirect3DRM_SetDefaultTextureShades(p,a)                  (p)->SetDefaultTextureShades(a)
#define IDirect3DRM_GetDevices(p,a)                               (p)->GetDevices(a)
#define IDirect3DRM_GetNamedObject(p,a,b)                         (p)->GetNamedObject(a,b)
#define IDirect3DRM_EnumerateObjects(p,a,b)                       (p)->EnumerateObjects(a,b)
#define IDirect3DRM_Load(p,a,b,c,d,e,f,g,h,i,j)                   (p)->Load(a,b,c,d,e,f,g,h,i,j)
#define IDirect3DRM_Tick(p,a)                                     (p)->Tick(a)
#endif

#define D3DRM_OK                        DD_OK
#define D3DRMERR_BADOBJECT              MAKE_DDHRESULT(781)
#define D3DRMERR_BADTYPE                MAKE_DDHRESULT(782)
#define D3DRMERR_BADALLOC               MAKE_DDHRESULT(783)
#define D3DRMERR_FACEUSED               MAKE_DDHRESULT(784)
#define D3DRMERR_NOTFOUND               MAKE_DDHRESULT(785)
#define D3DRMERR_NOTDONEYET             MAKE_DDHRESULT(786)
#define D3DRMERR_FILENOTFOUND           MAKE_DDHRESULT(787)
#define D3DRMERR_BADFILE                MAKE_DDHRESULT(788)
#define D3DRMERR_BADDEVICE              MAKE_DDHRESULT(789)
#define D3DRMERR_BADVALUE               MAKE_DDHRESULT(790)
#define D3DRMERR_BADMAJORVERSION        MAKE_DDHRESULT(791)
#define D3DRMERR_BADMINORVERSION        MAKE_DDHRESULT(792)
#define D3DRMERR_UNABLETOEXECUTE        MAKE_DDHRESULT(793)
#define D3DRMERR_LIBRARYNOTFOUND        MAKE_DDHRESULT(794)
#define D3DRMERR_INVALIDLIBRARY         MAKE_DDHRESULT(795)
#define D3DRMERR_PENDING                MAKE_DDHRESULT(796)
#define D3DRMERR_NOTENOUGHDATA          MAKE_DDHRESULT(797)
#define D3DRMERR_REQUESTTOOLARGE        MAKE_DDHRESULT(798)
#define D3DRMERR_REQUESTTOOSMALL        MAKE_DDHRESULT(799)
#define D3DRMERR_CONNECTIONLOST         MAKE_DDHRESULT(800)
#define D3DRMERR_LOADABORTED            MAKE_DDHRESULT(801)
#define D3DRMERR_NOINTERNET             MAKE_DDHRESULT(802)
#define D3DRMERR_BADCACHEFILE           MAKE_DDHRESULT(803)
#define D3DRMERR_BOXNOTSET              MAKE_DDHRESULT(804)
#define D3DRMERR_BADPMDATA              MAKE_DDHRESULT(805)
#define D3DRMERR_CLIENTNOTREGISTERED    MAKE_DDHRESULT(806)
#define D3DRMERR_NOTCREATEDFROMDDS      MAKE_DDHRESULT(807)
#define D3DRMERR_NOSUCHKEY              MAKE_DDHRESULT(808)
#define D3DRMERR_INCOMPATABLEKEY        MAKE_DDHRESULT(809)
#define D3DRMERR_ELEMENTINUSE           MAKE_DDHRESULT(810)
#define D3DRMERR_TEXTUREFORMATNOTFOUND  MAKE_DDHRESULT(811)
#define D3DRMERR_NOTAGGREGATED          MAKE_DDHRESULT(812)

#endif /* __D3DRM_H__ */
