/*
 * Copyright (C) 2008 Vijay Kiran Kamuju
 * Copyright (C) 2010 Christian Costa
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

#ifndef __D3DRMOBJ_H__
#define __D3DRMOBJ_H__

#include <objbase.h>
#define VIRTUAL
#include <d3drmdef.h>
#include <d3d.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Predeclare the interfaces
 */

DEFINE_GUID(IID_IDirect3DRMObject,          0xeb16cb00, 0xd271, 0x11ce, 0xac, 0x48, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMVisual,          0xeb16cb04, 0xd271, 0x11ce, 0xac, 0x48, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1);

typedef struct IDirect3DRMObject          *LPDIRECT3DRMOBJECT, **LPLPDIRECT3DRMOBJECT;
typedef struct IDirect3DRMObject2         *LPDIRECT3DRMOBJECT2, **LPLPDIRECT3DRMOBJECT2;
typedef struct IDirect3DRMDevice          *LPDIRECT3DRMDEVICE, **LPLPDIRECT3DRMDEVICE;
typedef struct IDirect3DRMDevice2         *LPDIRECT3DRMDEVICE2, **LPLPDIRECT3DRMDEVICE2;
typedef struct IDirect3DRMDevice3         *LPDIRECT3DRMDEVICE3, **LPLPDIRECT3DRMDEVICE3;
typedef struct IDirect3DRMViewport        *LPDIRECT3DRMVIEWPORT, **LPLPDIRECT3DRMVIEWPORT;
typedef struct IDirect3DRMViewport2       *LPDIRECT3DRMVIEWPORT2, **LPLPDIRECT3DRMVIEWPORT2;
typedef struct IDirect3DRMFrame           *LPDIRECT3DRMFRAME, **LPLPDIRECT3DRMFRAME;
typedef struct IDirect3DRMFrame2          *LPDIRECT3DRMFRAME2, **LPLPDIRECT3DRMFRAME2;
typedef struct IDirect3DRMFrame3          *LPDIRECT3DRMFRAME3, **LPLPDIRECT3DRMFRAME3;
typedef struct IDirect3DRMVisual          *LPDIRECT3DRMVISUAL, **LPLPDIRECT3DRMVISUAL;
typedef struct IDirect3DRMMesh            *LPDIRECT3DRMMESH, **LPLPDIRECT3DRMMESH;
typedef struct IDirect3DRMMeshBuilder     *LPDIRECT3DRMMESHBUILDER, **LPLPDIRECT3DRMMESHBUILDER;
typedef struct IDirect3DRMMeshBuilder2    *LPDIRECT3DRMMESHBUILDER2, **LPLPDIRECT3DRMMESHBUILDER2;
typedef struct IDirect3DRMMeshBuilder3    *LPDIRECT3DRMMESHBUILDER3, **LPLPDIRECT3DRMMESHBUILDER3;
typedef struct IDirect3DRMFace            *LPDIRECT3DRMFACE, **LPLPDIRECT3DRMFACE;
typedef struct IDirect3DRMFace2           *LPDIRECT3DRMFACE2, **LPLPDIRECT3DRMFACE2;
typedef struct IDirect3DRMLight           *LPDIRECT3DRMLIGHT, **LPLPDIRECT3DRMLIGHT;
typedef struct IDirect3DRMTexture         *LPDIRECT3DRMTEXTURE, **LPLPDIRECT3DRMTEXTURE;
typedef struct IDirect3DRMTexture2        *LPDIRECT3DRMTEXTURE2, **LPLPDIRECT3DRMTEXTURE2;
typedef struct IDirect3DRMTexture3        *LPDIRECT3DRMTEXTURE3, **LPLPDIRECT3DRMTEXTURE3;
typedef struct IDirect3DRMWrap            *LPDIRECT3DRMWRAP, **LPLPDIRECT3DRMWRAP;
typedef struct IDirect3DRMMaterial        *LPDIRECT3DRMMATERIAL, **LPLPDIRECT3DRMMATERIAL;
typedef struct IDirect3DRMMaterial2       *LPDIRECT3DRMMATERIAL2, **LPLPDIRECT3DRMMATERIAL2;
typedef struct IDirect3DRMAnimation       *LPDIRECT3DRMANIMATION, **LPLPDIRECT3DRMANIMATION;
typedef struct IDirect3DRMAnimation2      *LPDIRECT3DRMANIMATION2, **LPLPDIRECT3DRMANIMATION2;
typedef struct IDirect3DRMAnimationSet    *LPDIRECT3DRMANIMATIONSET, **LPLPDIRECT3DRMANIMATIONSET;
typedef struct IDirect3DRMAnimationSet2   *LPDIRECT3DRMANIMATIONSET2, **LPLPDIRECT3DRMANIMATIONSET2;
typedef struct IDirect3DRMUserVisual      *LPDIRECT3DRMUSERVISUAL, **LPLPDIRECT3DRMUSERVISUAL;
typedef struct IDirect3DRMShadow          *LPDIRECT3DRMSHADOW, **LPLPDIRECT3DRMSHADOW;
typedef struct IDirect3DRMShadow2         *LPDIRECT3DRMSHADOW2, **LPLPDIRECT3DRMSHADOW2;
typedef struct IDirect3DRMArray           *LPDIRECT3DRMARRAY, **LPLPDIRECT3DRMARRAY;
typedef struct IDirect3DRMObjectArray     *LPDIRECT3DRMOBJECTARRAY, **LPLPDIRECT3DRMOBJECTARRAY;
typedef struct IDirect3DRMDeviceArray     *LPDIRECT3DRMDEVICEARRAY, **LPLPDIRECT3DRMDEVICEARRAY;
typedef struct IDirect3DRMFaceArray       *LPDIRECT3DRMFACEARRAY, **LPLPDIRECT3DRMFACEARRAY;
typedef struct IDirect3DRMViewportArray   *LPDIRECT3DRMVIEWPORTARRAY, **LPLPDIRECT3DRMVIEWPORTARRAY;
typedef struct IDirect3DRMFrameArray      *LPDIRECT3DRMFRAMEARRAY, **LPLPDIRECT3DRMFRAMEARRAY;
typedef struct IDirect3DRMAnimationArray  *LPDIRECT3DRMANIMATIONARRAY, **LPLPDIRECT3DRMANIMATIONARRAY;
typedef struct IDirect3DRMVisualArray     *LPDIRECT3DRMVISUALARRAY, **LPLPDIRECT3DRMVISUALARRAY;
typedef struct IDirect3DRMPickedArray     *LPDIRECT3DRMPICKEDARRAY, **LPLPDIRECT3DRMPICKEDARRAY;
typedef struct IDirect3DRMPicked2Array    *LPDIRECT3DRMPICKED2ARRAY, **LPLPDIRECT3DRMPICKED2ARRAY;
typedef struct IDirect3DRMLightArray      *LPDIRECT3DRMLIGHTARRAY, **LPLPDIRECT3DRMLIGHTARRAY;
typedef struct IDirect3DRMProgressiveMesh *LPDIRECT3DRMPROGRESSIVEMESH, **LPLPDIRECT3DRMPROGRESSIVEMESH;
typedef struct IDirect3DRMClippedVisual   *LPDIRECT3DRMCLIPPEDVISUAL, **LPLPDIRECT3DRMCLIPPEDVISUAL;

/* ********************************************************************
   Types and structures
   ******************************************************************** */

typedef void (__cdecl *D3DRMOBJECTCALLBACK)(LPDIRECT3DRMOBJECT obj, LPVOID arg);
typedef void (__cdecl *D3DRMFRAMEMOVECALLBACK)(LPDIRECT3DRMFRAME obj, LPVOID arg, D3DVALUE delta);
typedef void (__cdecl *D3DRMFRAME3MOVECALLBACK)(LPDIRECT3DRMFRAME3 obj, LPVOID arg, D3DVALUE delta);
typedef void (__cdecl *D3DRMUPDATECALLBACK)(LPDIRECT3DRMDEVICE obj, LPVOID arg, int, LPD3DRECT);
typedef int (__cdecl *D3DRMUSERVISUALCALLBACK)(LPDIRECT3DRMUSERVISUAL obj, LPVOID arg,
    D3DRMUSERVISUALREASON reason, LPDIRECT3DRMDEVICE dev, LPDIRECT3DRMVIEWPORT view);
typedef HRESULT (__cdecl *D3DRMLOADTEXTURECALLBACK)(char *tex_name, void *arg, LPDIRECT3DRMTEXTURE *);
typedef HRESULT (__cdecl *D3DRMLOADTEXTURE3CALLBACK)(char *tex_name, void *arg, LPDIRECT3DRMTEXTURE3 *);
typedef void (__cdecl *D3DRMLOADCALLBACK)(LPDIRECT3DRMOBJECT object, REFIID objectguid, LPVOID arg);

typedef struct _D3DRMPICKDESC
{
    ULONG     ulFaceIdx;
    LONG      lGroupIdx;
    D3DVECTOR vPosition;
} D3DRMPICKDESC, *LPD3DRMPICKDESC;

typedef struct _D3DRMPICKDESC2
{
    ULONG     ulFaceIdx;
    LONG      lGroupIdx;
    D3DVECTOR vPosition;
    D3DVALUE  tu;
    D3DVALUE  tv;
    D3DVECTOR dvNormal;
    D3DCOLOR  dcColor;
} D3DRMPICKDESC2, *LPD3DRMPICKDESC2;

/*****************************************************************************
 * IDirect3DRMObject interface
 */
#ifdef WINE_NO_UNICODE_MACROS
#undef GetClassName
#endif
#define INTERFACE IDirect3DRMObject
DECLARE_INTERFACE_(IDirect3DRMObject,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DRMObject methods ***/
    STDMETHOD(Clone)(THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD(AddDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) PURE;
    STDMETHOD(DeleteDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) PURE;
    STDMETHOD(SetAppData)(THIS_ DWORD data) PURE;
    STDMETHOD_(DWORD, GetAppData)(THIS) PURE;
    STDMETHOD(SetName)(THIS_ LPCSTR) PURE;
    STDMETHOD(GetName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    STDMETHOD(GetClassName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirect3DRMObject_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DRMObject_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirect3DRMObject_Release(p)                   (p)->lpVtbl->Release(p)
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMObject_Clone(p,a,b,c)               (p)->lpVtbl->Clone(p,a,b,c)
#define IDirect3DRMObject_AddDestroyCallback(p,a,b)    (p)->lpVtbl->AddDestroyCallback(p,a,b)
#define IDirect3DRMObject_DeleteDestroyCallback(p,a,b) (p)->lpVtbl->DeleteDestroyCallback(p,a,b)
#define IDirect3DRMObject_SetAppData(p,a)              (p)->lpVtbl->SetAppData(p,a)
#define IDirect3DRMObject_GetAppData(p)                (p)->lpVtbl->GetAppData(p)
#define IDirect3DRMObject_SetName(p,a)                 (p)->lpVtbl->SetName(p,a)
#define IDirect3DRMObject_GetName(p,a,b)               (p)->lpVtbl->GetName(p,a,b)
#define IDirect3DRMObject_GetClassName(p,a,b)          (p)->lpVtbl->GetClassName(p,a,b)
#else
/*** IUnknown methods ***/
#define IDirect3DRMObject_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
#define IDirect3DRMObject_AddRef(p)                    (p)->AddRef()
#define IDirect3DRMObject_Release(p)                   (p)->Release()
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMObject_Clone(p,a,b,c)               (p)->Clone(a,b,c)
#define IDirect3DRMObject_AddDestroyCallback(p,a,b)    (p)->AddDestroyCallback(a,b)
#define IDirect3DRMObject_DeleteDestroyCallback(p,a,b) (p)->DeleteDestroyCallback(a,b)
#define IDirect3DRMObject_SetAppData(p,a)              (p)->SetAppData(a)
#define IDirect3DRMObject_GetAppData(p)                (p)->GetAppData()
#define IDirect3DRMObject_SetName(p,a)                 (p)->SetName(a)
#define IDirect3DRMObject_GetName(p,a,b)               (p)->GetName(a,b)
#define IDirect3DRMObject_GetClassName(p,a,b)          (p)->GetClassName(a,b)
#endif

/*****************************************************************************
 * IDirect3DRMObject2 interface
 */
#ifdef WINE_NO_UNICODE_MACROS
#undef GetClassName
#endif
#define INTERFACE IDirect3DRMObject2
DECLARE_INTERFACE_(IDirect3DRMObject2,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DRMObject2 methods ***/
    STDMETHOD(AddDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK pFunc, LPVOID pArg) PURE;
    STDMETHOD(Clone)(THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD(DeleteDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK pFunc, LPVOID pArg) PURE;
    STDMETHOD(GetClientData)(THIS_ DWORD id, LPVOID* ppData) PURE;
    STDMETHOD(GetDirect3DRM)(THIS_ LPDIRECT3DRM* ppDirect3DRM) PURE;
    STDMETHOD(GetName)(THIS_ LPDWORD pSize, LPSTR pName) PURE;
    STDMETHOD(SetClientData)(THIS_ DWORD id, LPVOID pData, DWORD flags) PURE;
    STDMETHOD(SetName)(THIS_ LPCSTR pName) PURE;
    STDMETHOD(GetAge)(THIS_ DWORD flags, LPDWORD pAge) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirect3DRMObject2_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DRMObject2_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirect3DRMObject2_Release(p)                   (p)->lpVtbl->Release(p)
/*** IDirect3DRMObject2 methods ***/
#define IDirect3DRMObject2_AddDestroyCallback(p,a,b)    (p)->lpVtbl->AddDestroyCallback(p,a,b)
#define IDirect3DRMObject2_Clone(p,a,b,c)               (p)->lpVtbl->Clone(p,a,b,c)
#define IDirect3DRMObject2_DeleteDestroyCallback(p,a,b) (p)->lpVtbl->DeleteDestroyCallback(p,a,b)
#define IDirect3DRMObject2_GetClientData(p,a,b)         (p)->lpVtbl->SetClientData(p,a,b)
#define IDirect3DRMObject2_GetDirect3DRM(p,a)           (p)->lpVtbl->GetDirect3DRM(p,a)
#define IDirect3DRMObject2_GetName(p,a,b)               (p)->lpVtbl->GetName(p,a,b)
#define IDirect3DRMObject2_SetClientData(p,a,b,c)       (p)->lpVtbl->SetClientData(p,a,b,c)
#define IDirect3DRMObject2_SetName(p,a)                 (p)->lpVtbl->SetName(p,a)
#define IDirect3DRMObject2_GetAge(p,a,b)                (p)->lpVtbl->GetAge(p,a,b)
#else
/*** IUnknown methods ***/
#define IDirect3DRMObject2_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
#define IDirect3DRMObject2_AddRef(p)                    (p)->AddRef()
#define IDirect3DRMObject2_Release(p)                   (p)->Release()
/*** IDirect3DRMObject2 methods ***/
#define IDirect3DRMObject2_AddDestroyCallback(p,a,b)    (p)->AddDestroyCallback(a,b)
#define IDirect3DRMObject2_Clone(p,a,b,c)               (p)->Clone(a,b,c)
#define IDirect3DRMObject2_DeleteDestroyCallback(p,a,b) (p)->DeleteDestroyCallback(a,b)
#define IDirect3DRMObject2_GetClientData(p,a,b)         (p)->SetClientData(a,b)
#define IDirect3DRMObject2_GetDirect3DRM(p,a)           (p)->GetDirect3DRM(a)
#define IDirect3DRMObject2_GetName(p,a,b)               (p)->GetName(a,b)
#define IDirect3DRMObject2_SetClientData(p,a,b,c)       (p)->SetClientData(a,b,c)
#define IDirect3DRMObject2_SetName(p,a)                 (p)->SetName(a)
#define IDirect3DRMObject2_GetAge(p,a,b)                (p)->GetAge(a,b)
#endif

/*****************************************************************************
 * IDirect3DRMVisual interface
 */
#define INTERFACE IDirect3DRMVisual
DECLARE_INTERFACE_(IDirect3DRMVisual,IDirect3DRMObject)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DRMObject methods ***/
    STDMETHOD(Clone)(THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD(AddDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) PURE;
    STDMETHOD(DeleteDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) PURE;
    STDMETHOD(SetAppData)(THIS_ DWORD data) PURE;
    STDMETHOD_(DWORD, GetAppData)(THIS) PURE;
    STDMETHOD(SetName)(THIS_ LPCSTR) PURE;
    STDMETHOD(GetName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    STDMETHOD(GetClassName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirect3DRMVisual_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DRMVisual_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirect3DRMVisual_Release(p)                   (p)->lpVtbl->Release(p)
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMVisual_Clone(p,a,b,c)               (p)->lpVtbl->Clone(p,a,b,c)
#define IDirect3DRMVisual_AddDestroyCallback(p,a,b)    (p)->lpVtbl->AddDestroyCallback(p,a,b)
#define IDirect3DRMVisual_DeleteDestroyCallback(p,a,b) (p)->lpVtbl->DeleteDestroyCallback(p,a,b)
#define IDirect3DRMVisual_SetAppData(p,a)              (p)->lpVtbl->SetAppData(p,a)
#define IDirect3DRMVisual_GetAppData(p)                (p)->lpVtbl->GetAppData(p)
#define IDirect3DRMVisual_SetName(p,a)                 (p)->lpVtbl->SetName(p,a)
#define IDirect3DRMVisual_GetName(p,a,b)               (p)->lpVtbl->GetName(p,a,b)
#define IDirect3DRMVisual_GetClassName(p,a,b)          (p)->lpVtbl->GetClassName(p,a,b)
#else
/*** IUnknown methods ***/
#define IDirect3DRMVisual_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
#define IDirect3DRMVisual_AddRef(p)                    (p)->AddRef()
#define IDirect3DRMVisual_Release(p)                   (p)->Release()
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMVisual_Clone(p,a,b,c)               (p)->Clone(a,b,c)
#define IDirect3DRMVisual_AddDestroyCallback(p,a,b)    (p)->AddDestroyCallback(a,b)
#define IDirect3DRMVisual_DeleteDestroyCallback(p,a,b) (p)->DeleteDestroyCallback(a,b)
#define IDirect3DRMVisual_SetAppData(p,a)              (p)->SetAppData(a)
#define IDirect3DRMVisual_GetAppData(p)                (p)->GetAppData()
#define IDirect3DRMVisual_SetName(p,a)                 (p)->SetName(a)
#define IDirect3DRMVisual_GetName(p,a,b)               (p)->GetName(a,b)
#define IDirect3DRMVisual_GetClassName(p,a,b)          (p)->GetClassName(a,b)
#endif

/*****************************************************************************
 * IDirect3DRMDevice interface
 */
#ifdef WINE_NO_UNICODE_MACROS
#undef GetClassName
#endif
#define INTERFACE IDirect3DRMDevice
DECLARE_INTERFACE_(IDirect3DRMDevice,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DRMObject methods ***/
    STDMETHOD(Clone)(THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD(AddDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK pFunc, LPVOID pArg) PURE;
    STDMETHOD(DeleteDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK pFunc, LPVOID pArg) PURE;
    STDMETHOD(SetAppData)(THIS_ DWORD data) PURE;
    STDMETHOD_(DWORD, GetAppData)(THIS) PURE;
    STDMETHOD(SetName)(THIS_ LPCSTR) PURE;
    STDMETHOD(GetName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    STDMETHOD(GetClassName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    /*** IDirect3DRMDevice methods ***/
    STDMETHOD(Init)(THIS_ ULONG width, ULONG height) PURE;
    STDMETHOD(InitFromD3D)(THIS_ LPDIRECT3D pD3D, LPDIRECT3DDEVICE pD3DDev) PURE;
    STDMETHOD(InitFromClipper)(THIS_ LPDIRECTDRAWCLIPPER pDDClipper, LPGUID pGUID, int width, int height) PURE;
    STDMETHOD(Update)(THIS) PURE;
    STDMETHOD(AddUpdateCallback)(THIS_ D3DRMUPDATECALLBACK, LPVOID arg) PURE;
    STDMETHOD(DeleteUpdateCallback)(THIS_ D3DRMUPDATECALLBACK, LPVOID arg) PURE;
    STDMETHOD(SetBufferCount)(THIS_ DWORD) PURE;
    STDMETHOD_(DWORD, GetBufferCount)(THIS) PURE;
    STDMETHOD(SetDither)(THIS_ BOOL) PURE;
    STDMETHOD(SetShades)(THIS_ DWORD) PURE;
    STDMETHOD(SetQuality)(THIS_ D3DRMRENDERQUALITY) PURE;
    STDMETHOD(SetTextureQuality)(THIS_ D3DRMTEXTUREQUALITY) PURE;
    STDMETHOD(GetViewports)(THIS_ LPDIRECT3DRMVIEWPORTARRAY *return_views) PURE;
    STDMETHOD_(BOOL, GetDither)(THIS) PURE;
    STDMETHOD_(DWORD, GetShades)(THIS) PURE;
    STDMETHOD_(DWORD, GetHeight)(THIS) PURE;
    STDMETHOD_(DWORD, GetWidth)(THIS) PURE;
    STDMETHOD_(DWORD, GetTrianglesDrawn)(THIS) PURE;
    STDMETHOD_(DWORD, GetWireframeOptions)(THIS) PURE;
    STDMETHOD_(D3DRMRENDERQUALITY, GetQuality)(THIS) PURE;
    STDMETHOD_(D3DCOLORMODEL, GetColorModel)(THIS) PURE;
    STDMETHOD_(D3DRMTEXTUREQUALITY, GetTextureQuality)(THIS) PURE;
    STDMETHOD(GetDirect3DDevice)(THIS_ LPDIRECT3DDEVICE *) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirect3DRMDevice_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DRMDevice_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirect3DRMDevice_Release(p)                   (p)->lpVtbl->Release(p)
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMDevice_Clone(p,a,b,c)               (p)->lpVtbl->Clone(p,a,b,c)
#define IDirect3DRMDevice_AddDestroyCallback(p,a,b)    (p)->lpVtbl->AddDestroyCallback(p,a,b)
#define IDirect3DRMDevice_DeleteDestroyCallback(p,a,b) (p)->lpVtbl->DeleteDestroyCallback(p,a,b)
#define IDirect3DRMDevice_SetAppData(p,a)              (p)->lpVtbl->SetAppData(p,a)
#define IDirect3DRMDevice_GetAppData(p)                (p)->lpVtbl->GetAppData(p)
#define IDirect3DRMDevice_SetName(p,a)                 (p)->lpVtbl->SetName(p,a)
#define IDirect3DRMDevice_GetName(p,a,b)               (p)->lpVtbl->GetName(p,a,b)
#define IDirect3DRMDevice_GetClassName(p,a,b)          (p)->lpVtbl->GetClassName(p,a,b)
/*** IDirect3DRMDevice methods ***/
#define IDirect3DRMDevice_Init(p,a,b)                  (p)->lpVtbl->Init(p,a,b)
#define IDirect3DRMDevice_InitFromD3D(p,a,b)           (p)->lpVtbl->InitFromD3D(p,a,b)
#define IDirect3DRMDevice_InitFromClipper(p,a,b,c,d)   (p)->lpVtbl->InitFromClipper(p,a,b,c,d)
#define IDirect3DRMDevice_Update(p)                    (p)->lpVtbl->Update(p)
#define IDirect3DRMDevice_AddUpdateCallback(p,a,b)     (p)->lpVtbl->AddUpdateCallback(p,a,b)
#define IDirect3DRMDevice_DeleteUpdateCallback(p,a,b)  (p)->lpVtbl->DeleteUpdateCallback(p,a,b,c)
#define IDirect3DRMDevice_SetBufferCount(p,a)          (p)->lpVtbl->SetBufferCount(p,a)
#define IDirect3DRMDevice_GetBufferCount(p)            (p)->lpVtbl->GetBufferCount(p)
#define IDirect3DRMDevice_SetDither(p,a)               (p)->lpVtbl->SetDither(p,a)
#define IDirect3DRMDevice_SetShades(p,a)               (p)->lpVtbl->SetShades(p,a)
#define IDirect3DRMDevice_SetQuality(p,a)              (p)->lpVtbl->SetQuality(p,a)
#define IDirect3DRMDevice_SetTextureQuality(p,a)       (p)->lpVtbl->SetTextureQuality(p,a)
#define IDirect3DRMDevice_GetViewports(p,a)            (p)->lpVtbl->GetViewports(p,a)
#define IDirect3DRMDevice_GetDither(p)                 (p)->lpVtbl->GetDither(p)
#define IDirect3DRMDevice_GetShades(p)                 (p)->lpVtbl->GetShades(p)
#define IDirect3DRMDevice_GetHeight(p)                 (p)->lpVtbl->GetHeight(p)
#define IDirect3DRMDevice_GetWidth(p)                  (p)->lpVtbl->GetWidth(p)
#define IDirect3DRMDevice_GetTrianglesDrawn(p)         (p)->lpVtbl->GetTrianglesDrawn(p)
#define IDirect3DRMDevice_GetWireframeOptions(p)       (p)->lpVtbl->GetWireframeOptions(p)
#define IDirect3DRMDevice_GetQuality(p)                (p)->lpVtbl->GetQuality(p)
#define IDirect3DRMDevice_GetColorModel(p)             (p)->lpVtbl->GetColorModel(p)
#define IDirect3DRMDevice_GetTextureQuality(p)         (p)->lpVtbl->GetTextureQuality(p)
#define IDirect3DRMDevice_GetDirect3DDevice(p,a)       (p)->lpVtbl->GetDirect3DDevice(p,a)
#else
/*** IUnknown methods ***/
#define IDirect3DRMDevice_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
#define IDirect3DRMDevice_AddRef(p)                    (p)->AddRef()
#define IDirect3DRMDevice_Release(p)                   (p)->Release()
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMDevice_Clone(p,a,b,c)               (p)->Clone(a,b,c)
#define IDirect3DRMDevice_AddDestroyCallback(p,a,b)    (p)->AddDestroyCallback(a,b)
#define IDirect3DRMDevice_DeleteDestroyCallback(p,a,b) (p)->DeleteDestroyCallback(a,b)
#define IDirect3DRMDevice_SetAppData(p,a)              (p)->SetAppData(a)
#define IDirect3DRMDevice_GetAppData(p)                (p)->GetAppData()
#define IDirect3DRMDevice_SetName(p,a)                 (p)->SetName(a)
#define IDirect3DRMDevice_GetName(p,a,b)               (p)->GetName(a,b)
#define IDirect3DRMDevice_GetClassName(p,a,b)          (p)->GetClassName(a,b)
/*** IDirect3DRMDevice methods ***/
#define IDirect3DRMDevice_Init(p,a,b)                  (p)->Init(p,a,b)
#define IDirect3DRMDevice_InitFromD3D(p,a,b)           (p)->InitFromD3D(p,a,b)
#define IDirect3DRMDevice_InitFromClipper(p,a,b,c,d)   (p)->InitFromClipper(p,a,b,c,d)
#define IDirect3DRMDevice_Update(p)                    (p)->Update(p)
#define IDirect3DRMDevice_AddUpdateCallback(p,a,b)     (p)->AddUpdateCallback(p,a,b)
#define IDirect3DRMDevice_DeleteUpdateCallback(p,a,b)  (p)->DeleteUpdateCallback(p,a,b,c)
#define IDirect3DRMDevice_SetBufferCount(p,a)          (p)->SetBufferCount(p,a)
#define IDirect3DRMDevice_GetBufferCount(p)            (p)->GetBufferCount(p)
#define IDirect3DRMDevice_SetDither(p,a)               (p)->SetDither(p,a)
#define IDirect3DRMDevice_SetShades(p,a)               (p)->SetShades(p,a)
#define IDirect3DRMDevice_SetQuality(p,a)              (p)->SetQuality(p,a)
#define IDirect3DRMDevice_SetTextureQuality(p,a)       (p)->SetTextureQuality(p,a)
#define IDirect3DRMDevice_GetViewports(p,a)            (p)->GetViewports(p,a)
#define IDirect3DRMDevice_GetDither(p)                 (p)->GetDither(p)
#define IDirect3DRMDevice_GetShades(p)                 (p)->GetShades(p)
#define IDirect3DRMDevice_GetHeight(p)                 (p)->GetHeight(p)
#define IDirect3DRMDevice_GetWidth(p)                  (p)->GetWidth(p)
#define IDirect3DRMDevice_GetTrianglesDrawn(p)         (p)->GetTrianglesDrawn(p)
#define IDirect3DRMDevice_GetWireframeOptions(p)       (p)->GetWireframeOptions(p)
#define IDirect3DRMDevice_GetQuality(p)                (p)->GetQuality(p)
#define IDirect3DRMDevice_GetColorModel(p)             (p)->GetColorModel(p)
#define IDirect3DRMDevice_GetTextureQuality(p)         (p)->GetTextureQuality(p)
#define IDirect3DRMDevice_GetDirect3DDevice(p,a)       (p)->GetDirect3DDevice(p,a)
#endif

/*****************************************************************************
 * IDirect3DRMDevice2 interface
 */
#ifdef WINE_NO_UNICODE_MACROS
#undef GetClassName
#endif
#define INTERFACE IDirect3DRMDevice2
DECLARE_INTERFACE_(IDirect3DRMDevice2,IDirect3DRMDevice)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DRMObject methods ***/
    STDMETHOD(Clone)(THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD(AddDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK pFunc, LPVOID pArg) PURE;
    STDMETHOD(DeleteDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK pFunc, LPVOID pArg) PURE;
    STDMETHOD(SetAppData)(THIS_ DWORD data) PURE;
    STDMETHOD_(DWORD, GetAppData)(THIS) PURE;
    STDMETHOD(SetName)(THIS_ LPCSTR) PURE;
    STDMETHOD(GetName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    STDMETHOD(GetClassName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    /*** IDirect3DRMDevice methods ***/
    STDMETHOD(Init)(THIS_ ULONG width, ULONG height) PURE;
    STDMETHOD(InitFromD3D)(THIS_ LPDIRECT3D pD3D, LPDIRECT3DDEVICE pD3DDev) PURE;
    STDMETHOD(InitFromClipper)(THIS_ LPDIRECTDRAWCLIPPER pDDClipper, LPGUID pGUID, int width, int height) PURE;
    STDMETHOD(Update)(THIS) PURE;
    STDMETHOD(AddUpdateCallback)(THIS_ D3DRMUPDATECALLBACK, LPVOID arg) PURE;
    STDMETHOD(DeleteUpdateCallback)(THIS_ D3DRMUPDATECALLBACK, LPVOID arg) PURE;
    STDMETHOD(SetBufferCount)(THIS_ DWORD) PURE;
    STDMETHOD_(DWORD, GetBufferCount)(THIS) PURE;
    STDMETHOD(SetDither)(THIS_ BOOL) PURE;
    STDMETHOD(SetShades)(THIS_ DWORD) PURE;
    STDMETHOD(SetQuality)(THIS_ D3DRMRENDERQUALITY) PURE;
    STDMETHOD(SetTextureQuality)(THIS_ D3DRMTEXTUREQUALITY) PURE;
    STDMETHOD(GetViewports)(THIS_ LPDIRECT3DRMVIEWPORTARRAY *return_views) PURE;
    STDMETHOD_(BOOL, GetDither)(THIS) PURE;
    STDMETHOD_(DWORD, GetShades)(THIS) PURE;
    STDMETHOD_(DWORD, GetHeight)(THIS) PURE;
    STDMETHOD_(DWORD, GetWidth)(THIS) PURE;
    STDMETHOD_(DWORD, GetTrianglesDrawn)(THIS) PURE;
    STDMETHOD_(DWORD, GetWireframeOptions)(THIS) PURE;
    STDMETHOD_(D3DRMRENDERQUALITY, GetQuality)(THIS) PURE;
    STDMETHOD_(D3DCOLORMODEL, GetColorModel)(THIS) PURE;
    STDMETHOD_(D3DRMTEXTUREQUALITY, GetTextureQuality)(THIS) PURE;
    STDMETHOD(GetDirect3DDevice)(THIS_ LPDIRECT3DDEVICE *) PURE;
    /*** IDirect3DRMDevice2 methods ***/
    STDMETHOD(InitFromD3D2)(THIS_ LPDIRECT3D2 pD3D, LPDIRECT3DDEVICE2 pD3DDev) PURE;
    STDMETHOD(InitFromSurface)(THIS_ LPGUID pGUID, LPDIRECTDRAW pDD, LPDIRECTDRAWSURFACE pDDSBack) PURE;
    STDMETHOD(SetRenderMode)(THIS_ DWORD flags) PURE;
    STDMETHOD_(DWORD, GetRenderMode)(THIS) PURE;
    STDMETHOD(GetDirect3DDevice2)(THIS_ LPDIRECT3DDEVICE2 *) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirect3DRMDevice2_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DRMDevice2_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirect3DRMDevice2_Release(p)                   (p)->lpVtbl->Release(p)
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMDevice2_Clone(p,a,b,c)               (p)->lpVtbl->Clone(p,a,b,c)
#define IDirect3DRMDevice2_AddDestroyCallback(p,a,b)    (p)->lpVtbl->AddDestroyCallback(p,a,b)
#define IDirect3DRMDevice2_DeleteDestroyCallback(p,a,b) (p)->lpVtbl->DeleteDestroyCallback(p,a,b)
#define IDirect3DRMDevice2_SetAppData(p,a)              (p)->lpVtbl->SetAppData(p,a)
#define IDirect3DRMDevice2_GetAppData(p)                (p)->lpVtbl->GetAppData(p)
#define IDirect3DRMDevice2_SetName(p,a)                 (p)->lpVtbl->SetName(p,a)
#define IDirect3DRMDevice2_GetName(p,a,b)               (p)->lpVtbl->GetName(p,a,b)
#define IDirect3DRMDevice2_GetClassName(p,a,b)          (p)->lpVtbl->GetClassName(p,a,b)
/*** IDirect3DRMDevice methods ***/
#define IDirect3DRMDevice2_Init(p,a,b)                  (p)->lpVtbl->Init(p,a,b)
#define IDirect3DRMDevice2_InitFromD3D(p,a,b)           (p)->lpVtbl->InitFromD3D(p,a,b)
#define IDirect3DRMDevice2_InitFromClipper(p,a,b,c,d)   (p)->lpVtbl->InitFromClipper(p,a,b,c,d)
#define IDirect3DRMDevice2_Update(p)                    (p)->lpVtbl->Update(p)
#define IDirect3DRMDevice2_AddUpdateCallback(p,a,b)     (p)->lpVtbl->AddUpdateCallback(p,a,b)
#define IDirect3DRMDevice2_DeleteUpdateCallback(p,a,b)  (p)->lpVtbl->DeleteUpdateCallback(p,a,b,c)
#define IDirect3DRMDevice2_SetBufferCount(p,a)          (p)->lpVtbl->SetBufferCount(p,a)
#define IDirect3DRMDevice2_GetBufferCount(p)            (p)->lpVtbl->GetBufferCount(p)
#define IDirect3DRMDevice2_SetDither(p,a)               (p)->lpVtbl->SetDither(p,a)
#define IDirect3DRMDevice2_SetShades(p,a)               (p)->lpVtbl->SetShades(p,a)
#define IDirect3DRMDevice2_SetQuality(p,a)              (p)->lpVtbl->SetQuality(p,a)
#define IDirect3DRMDevice2_SetTextureQuality(p,a)       (p)->lpVtbl->SetTextureQuality(p,a)
#define IDirect3DRMDevice2_GetViewports(p,a)            (p)->lpVtbl->GetViewports(p,a)
#define IDirect3DRMDevice2_GetDither(p)                 (p)->lpVtbl->GetDither(p)
#define IDirect3DRMDevice2_GetShades(p)                 (p)->lpVtbl->GetShades(p)
#define IDirect3DRMDevice2_GetHeight(p)                 (p)->lpVtbl->GetHeight(p)
#define IDirect3DRMDevice2_GetWidth(p)                  (p)->lpVtbl->GetWidth(p)
#define IDirect3DRMDevice2_GetTrianglesDrawn(p)         (p)->lpVtbl->GetTrianglesDrawn(p)
#define IDirect3DRMDevice2_GetWireframeOptions(p)       (p)->lpVtbl->GetWireframeOptions(p)
#define IDirect3DRMDevice2_GetQuality(p)                (p)->lpVtbl->GetQuality(p)
#define IDirect3DRMDevice2_GetColorModel(p)             (p)->lpVtbl->GetColorModel(p)
#define IDirect3DRMDevice2_GetTextureQuality(p)         (p)->lpVtbl->GetTextureQuality(p)
#define IDirect3DRMDevice2_GetDirect3DDevice(p,a)       (p)->lpVtbl->GetDirect3DDevice(p,a)
/*** IDirect3DRMDevice2 methods ***/
#define IDirect3DRMDevice2_InitFromD3D2(p,a,b)          (p)->lpVtbl->InitFromD3D2(p,a,b)
#define IDirect3DRMDevice2_InitFromSurface(p,a,b,c)     (p)->lpVtbl->InitFromSurface(p,a,b,c)
#define IDirect3DRMDevice2_SetRenderMode(p,a)           (p)->lpVtbl->SetRenderMode(p,a)
#define IDirect3DRMDevice2_GetRenderMode(p)             (p)->lpVtbl->GetRenderMode(p)
#define IDirect3DRMDevice2_GetDirect3DDevice2(p,a)      (p)->lpVtbl->GetDirect3DDevice2(p,a)
#else
/*** IUnknown methods ***/
#define IDirect3DRMDevice2_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
#define IDirect3DRMDevice2_AddRef(p)                    (p)->AddRef()
#define IDirect3DRMDevice2_Release(p)                   (p)->Release()
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMDevice2_Clone(p,a,b,c)               (p)->Clone(a,b,c)
#define IDirect3DRMDevice2_AddDestroyCallback(p,a,b)    (p)->AddDestroyCallback(a,b)
#define IDirect3DRMDevice2_DeleteDestroyCallback(p,a,b) (p)->DeleteDestroyCallback(a,b)
#define IDirect3DRMDevice2_SetAppData(p,a)              (p)->SetAppData(a)
#define IDirect3DRMDevice2_GetAppData(p)                (p)->GetAppData()
#define IDirect3DRMDevice2_SetName(p,a)                 (p)->SetName(a)
#define IDirect3DRMDevice2_GetName(p,a,b)               (p)->GetName(a,b)
#define IDirect3DRMDevice2_GetClassName(p,a,b)          (p)->GetClassName(a,b)
/*** IDirect3DRMDevice methods ***/
#define IDirect3DRMDevice2_Init(p,a,b)                  (p)->Init(p,a,b)
#define IDirect3DRMDevice2_InitFromD3D(p,a,b)           (p)->InitFromD3D(p,a,b)
#define IDirect3DRMDevice2_InitFromClipper(p,a,b,c,d)   (p)->InitFromClipper(p,a,b,c,d)
#define IDirect3DRMDevice2_Update(p)                    (p)->Update(p)
#define IDirect3DRMDevice2_AddUpdateCallback(p,a,b)     (p)->AddUpdateCallback(p,a,b)
#define IDirect3DRMDevice2_DeleteUpdateCallback(p,a,b)  (p)->DeleteUpdateCallback(p,a,b,c)
#define IDirect3DRMDevice2_SetBufferCount(p,a)          (p)->SetBufferCount(p,a)
#define IDirect3DRMDevice2_GetBufferCount(p)            (p)->GetBufferCount(p)
#define IDirect3DRMDevice2_SetDither(p,a)               (p)->SetDither(p,a)
#define IDirect3DRMDevice2_SetShades(p,a)               (p)->SetShades(p,a)
#define IDirect3DRMDevice2_SetQuality(p,a)              (p)->SetQuality(p,a)
#define IDirect3DRMDevice2_SetTextureQuality(p,a)       (p)->SetTextureQuality(p,a)
#define IDirect3DRMDevice2_GetViewports(p,a)            (p)->GetViewports(p,a)
#define IDirect3DRMDevice2_GetDither(p)                 (p)->GetDither(p)
#define IDirect3DRMDevice2_GetShades(p)                 (p)->GetShades(p)
#define IDirect3DRMDevice2_GetHeight(p)                 (p)->GetHeight(p)
#define IDirect3DRMDevice2_GetWidth(p)                  (p)->GetWidth(p)
#define IDirect3DRMDevice2_GetTrianglesDrawn(p)         (p)->GetTrianglesDrawn(p)
#define IDirect3DRMDevice2_GetWireframeOptions(p)       (p)->GetWireframeOptions(p)
#define IDirect3DRMDevice2_GetQuality(p)                (p)->GetQuality(p)
#define IDirect3DRMDevice2_GetColorModel(p)             (p)->GetColorModel(p)
#define IDirect3DRMDevice2_GetTextureQuality(p)         (p)->GetTextureQuality(p)
#define IDirect3DRMDevice2_GetDirect3DDevice(p,a)       (p)->GetDirect3DDevice(p,a)
/*** IDirect3DRMDevice2 methods ***/
#define IDirect3DRMDevice2_InitFromD3D2(p,a,b)          (p)->InitFromD3D2(a,b)
#define IDirect3DRMDevice2_InitFromSurface(p,a,b,c)     (p)->InitFromSurface(a,b,c)
#define IDirect3DRMDevice2_SetRenderMode(p,a)           (p)->SetRenderMode(a)
#define IDirect3DRMDevice2_GetRenderMode(p)             (p)->GetRenderMode()
#define IDirect3DRMDevice2_GetDirect3DDevice2(p,a)      (p)->GetDirect3DDevice2(a)
#endif

/*****************************************************************************
 * IDirect3DRMDevice3 interface
 */
#ifdef WINE_NO_UNICODE_MACROS
#undef GetClassName
#endif
#define INTERFACE IDirect3DRMDevice3
DECLARE_INTERFACE_(IDirect3DRMDevice3,IDirect3DRMObject)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DRMObject methods ***/
    STDMETHOD(Clone)(THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD(AddDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK pFunc, LPVOID pArg) PURE;
    STDMETHOD(DeleteDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK pFunc, LPVOID pArg) PURE;
    STDMETHOD(SetAppData)(THIS_ DWORD data) PURE;
    STDMETHOD_(DWORD, GetAppData)(THIS) PURE;
    STDMETHOD(SetName)(THIS_ LPCSTR) PURE;
    STDMETHOD(GetName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    STDMETHOD(GetClassName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    /*** IDirect3DRMDevice methods ***/
    STDMETHOD(Init)(THIS_ ULONG width, ULONG height) PURE;
    STDMETHOD(InitFromD3D)(THIS_ LPDIRECT3D pD3D, LPDIRECT3DDEVICE pD3DDev) PURE;
    STDMETHOD(InitFromClipper)(THIS_ LPDIRECTDRAWCLIPPER pDDClipper, LPGUID pGUID, int width, int height) PURE;
    STDMETHOD(Update)(THIS) PURE;
    STDMETHOD(AddUpdateCallback)(THIS_ D3DRMUPDATECALLBACK, LPVOID arg) PURE;
    STDMETHOD(DeleteUpdateCallback)(THIS_ D3DRMUPDATECALLBACK, LPVOID arg) PURE;
    STDMETHOD(SetBufferCount)(THIS_ DWORD) PURE;
    STDMETHOD_(DWORD, GetBufferCount)(THIS) PURE;
    STDMETHOD(SetDither)(THIS_ BOOL) PURE;
    STDMETHOD(SetShades)(THIS_ DWORD) PURE;
    STDMETHOD(SetQuality)(THIS_ D3DRMRENDERQUALITY) PURE;
    STDMETHOD(SetTextureQuality)(THIS_ D3DRMTEXTUREQUALITY) PURE;
    STDMETHOD(GetViewports)(THIS_ LPDIRECT3DRMVIEWPORTARRAY *return_views) PURE;
    STDMETHOD_(BOOL, GetDither)(THIS) PURE;
    STDMETHOD_(DWORD, GetShades)(THIS) PURE;
    STDMETHOD_(DWORD, GetHeight)(THIS) PURE;
    STDMETHOD_(DWORD, GetWidth)(THIS) PURE;
    STDMETHOD_(DWORD, GetTrianglesDrawn)(THIS) PURE;
    STDMETHOD_(DWORD, GetWireframeOptions)(THIS) PURE;
    STDMETHOD_(D3DRMRENDERQUALITY, GetQuality)(THIS) PURE;
    STDMETHOD_(D3DCOLORMODEL, GetColorModel)(THIS) PURE;
    STDMETHOD_(D3DRMTEXTUREQUALITY, GetTextureQuality)(THIS) PURE;
    STDMETHOD(GetDirect3DDevice)(THIS_ LPDIRECT3DDEVICE *) PURE;
    /*** IDirect3DRMDevice2 methods ***/
    STDMETHOD(InitFromD3D2)(THIS_ LPDIRECT3D2 pD3D, LPDIRECT3DDEVICE2 pD3DDev) PURE;
    STDMETHOD(InitFromSurface)(THIS_ LPGUID pGUID, LPDIRECTDRAW pDD, LPDIRECTDRAWSURFACE pDDSBack) PURE;
    STDMETHOD(SetRenderMode)(THIS_ DWORD flags) PURE;
    STDMETHOD_(DWORD, GetRenderMode)(THIS) PURE;
    STDMETHOD(GetDirect3DDevice2)(THIS_ LPDIRECT3DDEVICE2 *) PURE;
    /*** IDirect3DRMDevice3 methods ***/
    STDMETHOD(FindPreferredTextureFormat)(THIS_ DWORD BitDepths, DWORD flags, LPDDPIXELFORMAT pDDPF) PURE;
    STDMETHOD(RenderStateChange)(THIS_ D3DRENDERSTATETYPE drsType, DWORD val, DWORD flags) PURE;
    STDMETHOD(LightStateChange)(THIS_ D3DLIGHTSTATETYPE drsType, DWORD val, DWORD flags) PURE;
    STDMETHOD(GetStateChangeOptions)(THIS_ DWORD StateClass, DWORD StateNum, LPDWORD pFlags) PURE;
    STDMETHOD(SetStateChangeOptions)(THIS_ DWORD StateClass, DWORD StateNum, DWORD flags) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirect3DRMDevice3_QueryInterface(p,a,b)               (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DRMDevice3_AddRef(p)                           (p)->lpVtbl->AddRef(p)
#define IDirect3DRMDevice3_Release(p)                          (p)->lpVtbl->Release(p)
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMDevice3_Clone(p,a,b,c)                      (p)->lpVtbl->Clone(p,a,b,c)
#define IDirect3DRMDevice3_AddDestroyCallback(p,a,b)           (p)->lpVtbl->AddDestroyCallback(p,a,b)
#define IDirect3DRMDevice3_DeleteDestroyCallback(p,a,b)        (p)->lpVtbl->DeleteDestroyCallback(p,a,b)
#define IDirect3DRMDevice3_SetAppData(p,a)                     (p)->lpVtbl->SetAppData(p,a)
#define IDirect3DRMDevice3_GetAppData(p)                       (p)->lpVtbl->GetAppData(p)
#define IDirect3DRMDevice3_SetName(p,a)                        (p)->lpVtbl->SetName(p,a)
#define IDirect3DRMDevice3_GetName(p,a,b)                      (p)->lpVtbl->GetName(p,a,b)
#define IDirect3DRMDevice3_GetClassName(p,a,b)                 (p)->lpVtbl->GetClassName(p,a,b)
/*** IDirect3DRMDevice methods ***/
#define IDirect3DRMDevice3_Init(p,a,b)                         (p)->lpVtbl->Init(p,a,b)
#define IDirect3DRMDevice3_InitFromD3D(p,a,b)                  (p)->lpVtbl->InitFromD3D(p,a,b)
#define IDirect3DRMDevice3_InitFromClipper(p,a,b,c,d)          (p)->lpVtbl->InitFromClipper(p,a,b,c,d)
#define IDirect3DRMDevice3_Update(p)                           (p)->lpVtbl->Update(p)
#define IDirect3DRMDevice3_AddUpdateCallback(p,a,b)            (p)->lpVtbl->AddUpdateCallback(p,a,b)
#define IDirect3DRMDevice3_DeleteUpdateCallback(p,a,b)         (p)->lpVtbl->DeleteUpdateCallback(p,a,b,c)
#define IDirect3DRMDevice3_SetBufferCount(p,a)                 (p)->lpVtbl->SetBufferCount(p,a)
#define IDirect3DRMDevice3_GetBufferCount(p)                   (p)->lpVtbl->GetBufferCount(p)
#define IDirect3DRMDevice3_SetDither(p,a)                      (p)->lpVtbl->SetDither(p,a)
#define IDirect3DRMDevice3_SetShades(p,a)                      (p)->lpVtbl->SetShades(p,a)
#define IDirect3DRMDevice3_SetQuality(p,a)                     (p)->lpVtbl->SetQuality(p,a)
#define IDirect3DRMDevice3_SetTextureQuality(p,a)              (p)->lpVtbl->SetTextureQuality(p,a)
#define IDirect3DRMDevice3_GetViewports(p,a)                   (p)->lpVtbl->GetViewports(p,a)
#define IDirect3DRMDevice3_GetDither(p)                        (p)->lpVtbl->GetDither(p)
#define IDirect3DRMDevice3_GetShades(p)                        (p)->lpVtbl->GetShades(p)
#define IDirect3DRMDevice3_GetHeight(p)                        (p)->lpVtbl->GetHeight(p)
#define IDirect3DRMDevice3_GetWidth(p)                         (p)->lpVtbl->GetWidth(p)
#define IDirect3DRMDevice3_GetTrianglesDrawn(p)                (p)->lpVtbl->GetTrianglesDrawn(p)
#define IDirect3DRMDevice3_GetWireframeOptions(p)              (p)->lpVtbl->GetWireframeOptions(p)
#define IDirect3DRMDevice3_GetQuality(p)                       (p)->lpVtbl->GetQuality(p)
#define IDirect3DRMDevice3_GetColorModel(p)                    (p)->lpVtbl->GetColorModel(p)
#define IDirect3DRMDevice3_GetTextureQuality(p)                (p)->lpVtbl->GetTextureQuality(p)
#define IDirect3DRMDevice3_GetDirect3DDevice(p,a)              (p)->lpVtbl->GetDirect3DDevice(p,a)
/*** IDirect3DRMDevice2 methods ***/
#define IDirect3DRMDevice3_InitFromD3D2(p,a,b)                 (p)->lpVtbl->InitFromD3D2(p,a,b)
#define IDirect3DRMDevice3_InitFromSurface(p,a,b,c)            (p)->lpVtbl->InitFromSurface(p,a,b,c)
#define IDirect3DRMDevice3_SetRenderMode(p,a)                  (p)->lpVtbl->SetRenderMode(p,a)
#define IDirect3DRMDevice3_GetRenderMode(p)                    (p)->lpVtbl->GetRenderMode(p)
#define IDirect3DRMDevice3_GetDirect3DDevice2(p,a)             (p)->lpVtbl->GetDirect3DDevice2(p,a)
/*** IDirect3DRMDevice3 methods ***/
#define IDirect3DRMDevice3_FindPreferredTextureFormat(p,a,b,c) (p)->lpVtbl->FindPreferredTextureFormat(p,a,b)
#define IDirect3DRMDevice3_RenderStateChange(p,a,b,c)          (p)->lpVtbl->RenderStateChange(p,a,b)
#define IDirect3DRMDevice3_LightStateChange(p,a,b,c)           (p)->lpVtbl->LightStateChange(p,a,b)
#define IDirect3DRMDevice3_GetStateChangeOptions(p,a,b,c)      (p)->lpVtbl->GetStateChangeOptions(p,a,b)
#define IDirect3DRMDevice3_SetStateChangeOptions(p,a,b,c)      (p)->lpVtbl->SetStateChangeOptions(p,a,b)
#else
/*** IUnknown methods ***/
#define IDirect3DRMDevice3_QueryInterface(p,a,b)               (p)->QueryInterface(a,b)
#define IDirect3DRMDevice3_AddRef(p)                           (p)->AddRef()
#define IDirect3DRMDevice3_Release(p)                          (p)->Release()
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMDevice3_Clone(p,a,b,c)                      (p)->Clone(a,b,c)
#define IDirect3DRMDevice3_AddDestroyCallback(p,a,b)           (p)->AddDestroyCallback(a,b)
#define IDirect3DRMDevice3_DeleteDestroyCallback(p,a,b)        (p)->DeleteDestroyCallback(a,b)
#define IDirect3DRMDevice3_SetAppData(p,a)                     (p)->SetAppData(a)
#define IDirect3DRMDevice3_GetAppData(p)                       (p)->GetAppData()
#define IDirect3DRMDevice3_SetName(p,a)                        (p)->SetName(a)
#define IDirect3DRMDevice3_GetName(p,a,b)                      (p)->GetName(a,b)
#define IDirect3DRMDevice3_GetClassName(p,a,b)                 (p)->GetClassName(a,b)
/*** IDirect3DRMDevice methods ***/
#define IDirect3DRMDevice3_Init(p,a,b)                         (p)->Init(p,a,b)
#define IDirect3DRMDevice3_InitFromD3D(p,a,b)                  (p)->InitFromD3D(p,a,b)
#define IDirect3DRMDevice3_InitFromClipper(p,a,b,c,d)          (p)->InitFromClipper(p,a,b,c,d)
#define IDirect3DRMDevice3_Update(p)                           (p)->Update(p)
#define IDirect3DRMDevice3_AddUpdateCallback(p,a,b)            (p)->AddUpdateCallback(p,a,b)
#define IDirect3DRMDevice3_DeleteUpdateCallback(p,a,b)         (p)->DeleteUpdateCallback(p,a,b,c)
#define IDirect3DRMDevice3_SetBufferCount(p,a)                 (p)->SetBufferCount(p,a)
#define IDirect3DRMDevice3_GetBufferCount(p)                   (p)->GetBufferCount(p)
#define IDirect3DRMDevice3_SetDither(p,a)                      (p)->SetDither(p,a)
#define IDirect3DRMDevice3_SetShades(p,a)                      (p)->SetShades(p,a)
#define IDirect3DRMDevice3_SetQuality(p,a)                     (p)->SetQuality(p,a)
#define IDirect3DRMDevice3_SetTextureQuality(p,a)              (p)->SetTextureQuality(p,a)
#define IDirect3DRMDevice3_GetViewports(p,a)                   (p)->GetViewports(p,a)
#define IDirect3DRMDevice3_GetDither(p)                        (p)->GetDither(p)
#define IDirect3DRMDevice3_GetShades(p)                        (p)->GetShades(p)
#define IDirect3DRMDevice3_GetHeight(p)                        (p)->GetHeight(p)
#define IDirect3DRMDevice3_GetWidth(p)                         (p)->GetWidth(p)
#define IDirect3DRMDevice3_GetTrianglesDrawn(p)                (p)->GetTrianglesDrawn(p)
#define IDirect3DRMDevice3_GetWireframeOptions(p)              (p)->GetWireframeOptions(p)
#define IDirect3DRMDevice3_GetQuality(p)                       (p)->GetQuality(p)
#define IDirect3DRMDevice3_GetColorModel(p)                    (p)->GetColorModel(p)
#define IDirect3DRMDevice3_GetTextureQuality(p)                (p)->GetTextureQuality(p)
#define IDirect3DRMDevice3_GetDirect3DDevice(p,a)              (p)->GetDirect3DDevice(p,a)
/*** IDirect3DRMDevice2 methods ***/
#define IDirect3DRMDevice3_InitFromD3D2(p,a,b)                 (p)->InitFromD3D2(a,b)
#define IDirect3DRMDevice3_InitFromSurface(p,a,b,c)            (p)->InitFromSurface(a,b,c)
#define IDirect3DRMDevice3_SetRenderMode(p,a)                  (p)->SetRenderMode(a)
#define IDirect3DRMDevice3_GetRenderMode(p)                    (p)->GetRenderMode()
#define IDirect3DRMDevice3_GetDirect3DDevice2(p,a)             (p)->GetDirect3DDevice2(a)
/*** IDirect3DRMDevice3 methods ***/
#define IDirect3DRMDevice3_FindPreferredTextureFormat(p,a,b,c) (p)->FindPreferredTextureFormat(a,b,c)
#define IDirect3DRMDevice3_RenderStateChange(p,a,b,c)          (p)->RenderStateChange(a,b,c)
#define IDirect3DRMDevice3_LightStateChange(p,a,b,c)           (p)->LightStateChange(a,b,c)
#define IDirect3DRMDevice3_GetStateChangeOptions(p,a,b,c)      (p)->GetStateChangeOptions(a,b,c)
#define IDirect3DRMDevice3_SetStateChangeOptions(p,a,b,c)      (p)->SetStateChangeOptions(a,b,c)
#endif

/*****************************************************************************
 * IDirect3DRMViewport interface
 */
#define INTERFACE IDirect3DRMViewport
DECLARE_INTERFACE_(IDirect3DRMViewport,IDirect3DRMObject)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DRMObject methods ***/
    STDMETHOD(Clone)(THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD(AddDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) PURE;
    STDMETHOD(DeleteDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) PURE;
    STDMETHOD(SetAppData)(THIS_ DWORD data) PURE;
    STDMETHOD_(DWORD, GetAppData)(THIS) PURE;
    STDMETHOD(SetName)(THIS_ LPCSTR) PURE;
    STDMETHOD(GetName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    STDMETHOD(GetClassName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    /*** IDirect3DRMViewport methods ***/
    STDMETHOD(Init) (THIS_ LPDIRECT3DRMDEVICE dev, LPDIRECT3DRMFRAME camera, DWORD xpos, DWORD ypos,
        DWORD width, DWORD height) PURE;
    STDMETHOD(Clear)(THIS) PURE;
    STDMETHOD(Render)(THIS_ LPDIRECT3DRMFRAME) PURE;
    STDMETHOD(SetFront)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetBack)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetField)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetUniformScaling)(THIS_ BOOL) PURE;
    STDMETHOD(SetCamera)(THIS_ LPDIRECT3DRMFRAME) PURE;
    STDMETHOD(SetProjection)(THIS_ D3DRMPROJECTIONTYPE) PURE;
    STDMETHOD(Transform)(THIS_ D3DRMVECTOR4D *d, D3DVECTOR *s) PURE;
    STDMETHOD(InverseTransform)(THIS_ D3DVECTOR *d, D3DRMVECTOR4D *s) PURE;
    STDMETHOD(Configure)(THIS_ LONG x, LONG y, DWORD width, DWORD height) PURE;
    STDMETHOD(ForceUpdate)(THIS_ DWORD x1, DWORD y1, DWORD x2, DWORD y2) PURE;
    STDMETHOD(SetPlane)(THIS_ D3DVALUE left, D3DVALUE right, D3DVALUE bottom, D3DVALUE top) PURE;
    STDMETHOD(GetCamera)(THIS_ LPDIRECT3DRMFRAME *) PURE;
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DRMDEVICE *) PURE;
    STDMETHOD(GetPlane)(THIS_ D3DVALUE *left, D3DVALUE *right, D3DVALUE *bottom, D3DVALUE *top) PURE;
    STDMETHOD(Pick)(THIS_ LONG x, LONG y, LPDIRECT3DRMPICKEDARRAY *return_visuals) PURE;
    STDMETHOD_(BOOL, GetUniformScaling)(THIS) PURE;
    STDMETHOD_(LONG, GetX)(THIS) PURE;
    STDMETHOD_(LONG, GetY)(THIS) PURE;
    STDMETHOD_(DWORD, GetWidth)(THIS) PURE;
    STDMETHOD_(DWORD, GetHeight)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetField)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetBack)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetFront)(THIS) PURE;
    STDMETHOD_(D3DRMPROJECTIONTYPE, GetProjection)(THIS) PURE;
    STDMETHOD(GetDirect3DViewport)(THIS_ LPDIRECT3DVIEWPORT *) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirect3DRMViewport_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DRMViewport_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirect3DRMViewport_Release(p)                   (p)->lpVtbl->Release(p)
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMViewport_Clone(p,a,b,c)               (p)->lpVtbl->Clone(p,a,b,c)
#define IDirect3DRMViewport_AddDestroyCallback(p,a,b)    (p)->lpVtbl->AddDestroyCallback(p,a,b)
#define IDirect3DRMViewport_DeleteDestroyCallback(p,a,b) (p)->lpVtbl->DeleteDestroyCallback(p,a,b)
#define IDirect3DRMViewport_SetAppData(p,a)              (p)->lpVtbl->SetAppData(p,a)
#define IDirect3DRMViewport_GetAppData(p)                (p)->lpVtbl->GetAppData(p)
#define IDirect3DRMViewport_SetName(p,a)                 (p)->lpVtbl->SetName(p,a)
#define IDirect3DRMViewport_GetName(p,a,b)               (p)->lpVtbl->GetName(p,a,b)
#define IDirect3DRMViewport_GetClassName(p,a,b)          (p)->lpVtbl->GetClassName(p,a,b)
/*** IDirect3DRMViewport methods ***/
#define IDirect3DRMViewport_Init(p,a,b,c,d)              (p)->lpVtbl->Init(p,a,b,c,d)
#define IDirect3DRMViewport_Clear(p)                     (p)->lpVtbl->Clear(p)
#define IDirect3DRMViewport_Render(p,a)                  (p)->lpVtbl->Render(p,a)
#define IDirect3DRMViewport_SetFront(p,a)                (p)->lpVtbl->SetFront(p,a)
#define IDirect3DRMViewport_SetBack(p,a)                 (p)->lpVtbl->(p,a)
#define IDirect3DRMViewport_SetField(p,a)                (p)->lpVtbl->(p,a)
#define IDirect3DRMViewport_SetUniformScaling(p,a)       (p)->lpVtbl->SetUniformScaling(p,a)
#define IDirect3DRMViewport_SetCamera(p,a)               (p)->lpVtbl->SetCamera(p,a)
#define IDirect3DRMViewport_SetProjection(p,a)           (p)->lpVtbl->SetProjection(p,a)
#define IDirect3DRMViewport_Transform(p,a,b)             (p)->lpVtbl->Transform(p,a,b)
#define IDirect3DRMViewport_InverseTransform(p,a,b)      (p)->lpVtbl->(p,a,b)
#define IDirect3DRMViewport_Configure(p,a,b,c,d)         (p)->lpVtbl->Configure(p,a,b,c,d)
#define IDirect3DRMViewport_ForceUpdate(p,a,b,c,d)       (p)->lpVtbl->ForceUpdate(p,a,b,c,d)
#define IDirect3DRMViewport_SetPlane(p,a,b,c,d)          (p)->lpVtbl->SetPlane(p,a,b,c,d)
#define IDirect3DRMViewport_GetCamera(p,a)               (p)->lpVtbl->(p,a)
#define IDirect3DRMViewport_GetDevice(p,a)               (p)->lpVtbl->GetDevice(p,a)
#define IDirect3DRMViewport_GetPlane(p,a,b,c,d)          (p)->lpVtbl->GetPlane(p,a,b,c,d)
#define IDirect3DRMViewport_Pick(p,a,b,c)                (p)->lpVtbl->Pick(p,a,b,c)
#define IDirect3DRMViewport_GetUniformScaling(p)         (p)->lpVtbl->GetUniformScaling(p)
#define IDirect3DRMViewport_GetX(p)                      (p)->lpVtbl->GetX(p)
#define IDirect3DRMViewport_GetY(p)                      (p)->lpVtbl->GetY(p)
#define IDirect3DRMViewport_GetWidth(p)                  (p)->lpVtbl->GetWidth(p)
#define IDirect3DRMViewport_GetHeight(p)                 (p)->lpVtbl->GetHeight(p)
#define IDirect3DRMViewport_GetField(p)                  (p)->lpVtbl->GetField(p)
#define IDirect3DRMViewport_GetBack(p)                   (p)->lpVtbl->GetBack(p)
#define IDirect3DRMViewport_GetFront(p)                  (p)->lpVtbl->GetFront(p)
#define IDirect3DRMViewport_GetProjection(p)             (p)->lpVtbl->GetProjection(p)
#define IDirect3DRMViewport_GetDirect3DViewport(p,a)     (p)->lpVtbl->GetDirect3DViewport(p,a)
#else
/*** IUnknown methods ***/
#define IDirect3DRMViewport_QueryInterface(p,a,b)        (p)->QueryInterface(a,b)
#define IDirect3DRMViewport_AddRef(p)                    (p)->AddRef()
#define IDirect3DRMViewport_Release(p)                   (p)->Release()
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMViewport_Clone(p,a,b,c)               (p)->Clone(a,b,c)
#define IDirect3DRMViewport_AddDestroyCallback(p,a,b)    (p)->AddDestroyCallback(a,b)
#define IDirect3DRMViewport_DeleteDestroyCallback(p,a,b) (p)->DeleteDestroyCallback(a,b)
#define IDirect3DRMViewport_SetAppData(p,a)              (p)->SetAppData(a)
#define IDirect3DRMViewport_GetAppData(p)                (p)->GetAppData()
#define IDirect3DRMViewport_SetName(p,a)                 (p)->SetName(a)
#define IDirect3DRMViewport_GetName(p,a,b)               (p)->GetName(a,b)
#define IDirect3DRMViewport_GetClassName(p,a,b)          (p)->GetClassName(a,b)
/*** IDirect3DRMViewport methods ***/
#define IDirect3DRMViewport_Init(p,a,b,c,d)              (p)->Init(p,a,b,c,d)
#define IDirect3DRMViewport_Clear(p)                     (p)->Clear(p)
#define IDirect3DRMViewport_Render(p,a)                  (p)->Render(p,a)
#define IDirect3DRMViewport_SetFront(p,a)                (p)->SetFront(p,a)
#define IDirect3DRMViewport_SetBack(p,a)                 (p)->(p,a)
#define IDirect3DRMViewport_SetField(p,a)                (p)->(p,a)
#define IDirect3DRMViewport_SetUniformScaling(p,a)       (p)->SetUniformScaling(p,a)
#define IDirect3DRMViewport_SetCamera(p,a)               (p)->SetCamera(p,a)
#define IDirect3DRMViewport_SetProjection(p,a)           (p)->SetProjection(p,a)
#define IDirect3DRMViewport_Transform(p,a,b)             (p)->Transform(p,a,b)
#define IDirect3DRMViewport_InverseTransform(p,a,b)      (p)->(p,a,b)
#define IDirect3DRMViewport_Configure(p,a,b,c,d)         (p)->Configure(p,a,b,c,d)
#define IDirect3DRMViewport_ForceUpdate(p,a,b,c,d)       (p)->ForceUpdate(p,a,b,c,d)
#define IDirect3DRMViewport_SetPlane(p,a,b,c,d)          (p)->SetPlane(p,a,b,c,d)
#define IDirect3DRMViewport_GetCamera(p,a)               (p)->GetCamera(p,a)
#define IDirect3DRMViewport_GetDevice(p,a)               (p)->GetDevice(p,a)
#define IDirect3DRMViewport_GetPlane(p,a,b,c,d)          (p)->GetPlane(p,a,b,c,d)
#define IDirect3DRMViewport_Pick(p,a,b,c)                (p)->Pick(p,a,b,c)
#define IDirect3DRMViewport_GetUniformScaling(p)         (p)->GetUniformScaling(p)
#define IDirect3DRMViewport_GetX(p)                      (p)->GetX(p)
#define IDirect3DRMViewport_GetY(p)                      (p)->GetY(p)
#define IDirect3DRMViewport_GetWidth(p)                  (p)->GetWidth(p)
#define IDirect3DRMViewport_GetHeight(p)                 (p)->GetHeight(p)
#define IDirect3DRMViewport_GetField(p)                  (p)->GetField(p)
#define IDirect3DRMViewport_GetBack(p)                   (p)->GetBack(p)
#define IDirect3DRMViewport_GetFront(p)                  (p)->GetFront(p)
#define IDirect3DRMViewport_GetProjection(p)             (p)->GetProjection(p)
#define IDirect3DRMViewport_GetDirect3DViewport(p,a)     (p)->GetDirect3DViewport(p,a)
#endif

/*****************************************************************************
 * IDirect3DRMViewport2 interface
 */
#define INTERFACE IDirect3DRMViewport2
DECLARE_INTERFACE_(IDirect3DRMViewport2,IDirect3DRMObject)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DRMObject methods ***/
    STDMETHOD(Clone)(THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD(AddDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) PURE;
    STDMETHOD(DeleteDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) PURE;
    STDMETHOD(SetAppData)(THIS_ DWORD data) PURE;
    STDMETHOD_(DWORD, GetAppData)(THIS) PURE;
    STDMETHOD(SetName)(THIS_ LPCSTR) PURE;
    STDMETHOD(GetName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    STDMETHOD(GetClassName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    /*** IDirect3DRMViewport2 methods ***/
    STDMETHOD(Init) (THIS_ LPDIRECT3DRMDEVICE3 dev, LPDIRECT3DRMFRAME3 camera, DWORD xpos, DWORD ypos,
        DWORD width, DWORD height) PURE;
    STDMETHOD(Clear)(THIS_ DWORD flags) PURE;
    STDMETHOD(Render)(THIS_ LPDIRECT3DRMFRAME3) PURE;
    STDMETHOD(SetFront)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetBack)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetField)(THIS_ D3DVALUE) PURE;
    STDMETHOD(SetUniformScaling)(THIS_ BOOL) PURE;
    STDMETHOD(SetCamera)(THIS_ LPDIRECT3DRMFRAME3) PURE;
    STDMETHOD(SetProjection)(THIS_ D3DRMPROJECTIONTYPE) PURE;
    STDMETHOD(Transform)(THIS_ D3DRMVECTOR4D *d, D3DVECTOR *s) PURE;
    STDMETHOD(InverseTransform)(THIS_ D3DVECTOR *d, D3DRMVECTOR4D *s) PURE;
    STDMETHOD(Configure)(THIS_ LONG x, LONG y, DWORD width, DWORD height) PURE;
    STDMETHOD(ForceUpdate)(THIS_ DWORD x1, DWORD y1, DWORD x2, DWORD y2) PURE;
    STDMETHOD(SetPlane)(THIS_ D3DVALUE left, D3DVALUE right, D3DVALUE bottom, D3DVALUE top) PURE;
    STDMETHOD(GetCamera)(THIS_ LPDIRECT3DRMFRAME3 *) PURE;
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DRMDEVICE3 *) PURE;
    STDMETHOD(GetPlane)(THIS_ D3DVALUE *left, D3DVALUE *right, D3DVALUE *bottom, D3DVALUE *top) PURE;
    STDMETHOD(Pick)(THIS_ LONG x, LONG y, LPDIRECT3DRMPICKEDARRAY *return_visuals) PURE;
    STDMETHOD_(BOOL, GetUniformScaling)(THIS) PURE;
    STDMETHOD_(LONG, GetX)(THIS) PURE;
    STDMETHOD_(LONG, GetY)(THIS) PURE;
    STDMETHOD_(DWORD, GetWidth)(THIS) PURE;
    STDMETHOD_(DWORD, GetHeight)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetField)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetBack)(THIS) PURE;
    STDMETHOD_(D3DVALUE, GetFront)(THIS) PURE;
    STDMETHOD_(D3DRMPROJECTIONTYPE, GetProjection)(THIS) PURE;
    STDMETHOD(GetDirect3DViewport)(THIS_ LPDIRECT3DVIEWPORT *) PURE;
    STDMETHOD(TransformVectors)(THIS_ DWORD NumVectors, LPD3DRMVECTOR4D pDstVectors,
        LPD3DVECTOR pSrcVectors) PURE;
    STDMETHOD(InverseTransformVectors)(THIS_ DWORD NumVectors, LPD3DVECTOR pDstVectors,
        LPD3DRMVECTOR4D pSrcVectors) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirect3DRMViewport2_QueryInterface(p,a,b)            (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DRMViewport2_AddRef(p)                        (p)->lpVtbl->AddRef(p)
#define IDirect3DRMViewport2_Release(p)                       (p)->lpVtbl->Release(p)
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMViewport_2Clone(p,a,b,c)                   (p)->lpVtbl->Clone(p,a,b,c)
#define IDirect3DRMViewport2_AddDestroyCallback(p,a,b)        (p)->lpVtbl->AddDestroyCallback(p,a,b)
#define IDirect3DRMViewport2_DeleteDestroyCallback(p,a,b)     (p)->lpVtbl->DeleteDestroyCallback(p,a,b)
#define IDirect3DRMViewport2_SetAppData(p,a)                  (p)->lpVtbl->SetAppData(p,a)
#define IDirect3DRMViewport2_GetAppData(p)                    (p)->lpVtbl->GetAppData(p)
#define IDirect3DRMViewport2_SetName(p,a)                     (p)->lpVtbl->SetName(p,a)
#define IDirect3DRMViewport2_GetName(p,a,b)                   (p)->lpVtbl->GetName(p,a,b)
#define IDirect3DRMViewport2_GetClassName(p,a,b)              (p)->lpVtbl->GetClassName(p,a,b)
/*** IDirect3DRMViewport2 methods ***/
#define IDirect3DRMViewport2_Init(p,a,b,c,d,e,f)              (p)->lpVtbl->Init(p,a,b,c,d,e,f)
#define IDirect3DRMViewport2_Clear(p,a)                       (p)->lpVtbl->Clear(p,a)
#define IDirect3DRMViewport2_Render(p,a)                      (p)->lpVtbl->Render(p,a)
#define IDirect3DRMViewport2_SetFront(p,a)                    (p)->lpVtbl->SetFront(p,a)
#define IDirect3DRMViewport2_SetBack(p,a)                     (p)->lpVtbl->(p,a)
#define IDirect3DRMViewport2_SetField(p,a)                    (p)->lpVtbl->(p,a)
#define IDirect3DRMViewport2_SetUniformScaling(p,a)           (p)->lpVtbl->SetUniformScaling(p,a)
#define IDirect3DRMViewport2_SetCamera(p,a)                   (p)->lpVtbl->SetCamera(p,a)
#define IDirect3DRMViewport2_SetProjection(p,a)               (p)->lpVtbl->SetProjection(p,a)
#define IDirect3DRMViewport2_Transform(p,a,b)                 (p)->lpVtbl->Transform(p,a,b)
#define IDirect3DRMViewport2_InverseTransform(p,a,b)          (p)->lpVtbl->(p,a,b)
#define IDirect3DRMViewport2_Configure(p,a,b,c,d)             (p)->lpVtbl->Configure(p,a,b,c,d)
#define IDirect3DRMViewport2_ForceUpdate(p,a,b,c,d)           (p)->lpVtbl->ForceUpdate(p,a,b,c,d)
#define IDirect3DRMViewport2_SetPlane(p,a,b,c,d)              (p)->lpVtbl->SetPlane(p,a,b,c,d)
#define IDirect3DRMViewport2_GetCamera(p,a)                   (p)->lpVtbl->(p,a)
#define IDirect3DRMViewport2_GetDevice(p,a)                   (p)->lpVtbl->GetDevice(p,a)
#define IDirect3DRMViewport2_GetPlane(p,a,b,c,d)              (p)->lpVtbl->GetPlane(p,a,b,c,d)
#define IDirect3DRMViewport2_Pick(p,a,b,c)                    (p)->lpVtbl->Pick(p,a,b,c)
#define IDirect3DRMViewport2_GetUniformScaling(p)             (p)->lpVtbl->GetUniformScaling(p)
#define IDirect3DRMViewport2_GetX(p)                          (p)->lpVtbl->GetX(p)
#define IDirect3DRMViewport2_GetY(p)                          (p)->lpVtbl->GetY(p)
#define IDirect3DRMViewport2_GetWidth(p)                      (p)->lpVtbl->GetWidth(p)
#define IDirect3DRMViewport2_GetHeight(p)                     (p)->lpVtbl->GetHeight(p)
#define IDirect3DRMViewport2_GetField(p)                      (p)->lpVtbl->GetField(p)
#define IDirect3DRMViewport2_GetBack(p)                       (p)->lpVtbl->GetBack(p)
#define IDirect3DRMViewport2_GetFront(p)                      (p)->lpVtbl->GetFront(p)
#define IDirect3DRMViewport2_GetProjection(p)                 (p)->lpVtbl->GetProjection(p)
#define IDirect3DRMViewport2_GetDirect3DViewport(p,a)         (p)->lpVtbl->GetDirect3DViewport(p,a)
#define IDirect3DRMViewport2_TransformVectors(p,a,b,c)        (p)->lpVtbl->TransformVectors(p,a,b,c)
#define IDirect3DRMViewport2_InverseTransformVectors(p,a,b,c) (p)->lpVtbl->InverseTransformVectors(p,ab,c)
#else
/*** IUnknown methods ***/
#define IDirect3DRMViewport2_QueryInterface(p,a,b)            (p)->QueryInterface(a,b)
#define IDirect3DRMViewport2_AddRef(p)                        (p)->AddRef()
#define IDirect3DRMViewport2_Release(p)                       (p)->Release()
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMViewport2_Clone(p,a,b,c)                   (p)->Clone(a,b,c)
#define IDirect3DRMViewport2_AddDestroyCallback(p,a,b)        (p)->AddDestroyCallback(a,b)
#define IDirect3DRMViewport2_DeleteDestroyCallback(p,a,b)     (p)->DeleteDestroyCallback(a,b)
#define IDirect3DRMViewport2_SetAppData(p,a)                  (p)->SetAppData(a)
#define IDirect3DRMViewport2_GetAppData(p)                    (p)->GetAppData()
#define IDirect3DRMViewport2_SetName(p,a)                     (p)->SetName(a)
#define IDirect3DRMViewport2_GetName(p,a,b)                   (p)->GetName(a,b)
#define IDirect3DRMViewport2_GetClassName(p,a,b)              (p)->GetClassName(a,b)
/*** IDirect3DRMViewport2 methods ***/
#define IDirect3DRMViewport2_Init(p,a,b,c,d)                  (p)->Init(p,a,b,c,d)
#define IDirect3DRMViewport2_Clear(p)                         (p)->Clear(p)
#define IDirect3DRMViewport2_Render(p,a)                      (p)->Render(p,a)
#define IDirect3DRMViewport2_SetFront(p,a)                    (p)->SetFront(p,a)
#define IDirect3DRMViewport2_SetBack(p,a)                     (p)->(p,a)
#define IDirect3DRMViewport2_SetField(p,a)                    (p)->(p,a)
#define IDirect3DRMViewport2_SetUniformScaling(p,a)           (p)->SetUniformScaling(p,a)
#define IDirect3DRMViewport2_SetCamera(p,a)                   (p)->SetCamera(p,a)
#define IDirect3DRMViewport2_SetProjection(p,a)               (p)->SetProjection(p,a)
#define IDirect3DRMViewport2_Transform(p,a,b)                 (p)->Transform(p,a,b)
#define IDirect3DRMViewport2_InverseTransform(p,a,b)          (p)->(p,a,b)
#define IDirect3DRMViewport2_Configure(p,a,b,c,d)             (p)->Configure(p,a,b,c,d)
#define IDirect3DRMViewport2_ForceUpdate(p,a,b,c,d)           (p)->ForceUpdate(p,a,b,c,d)
#define IDirect3DRMViewport2_SetPlane(p,a,b,c,d)              (p)->SetPlane(p,a,b,c,d)
#define IDirect3DRMViewport2_GetCamera(p,a)                   (p)->GetCamera(p,a)
#define IDirect3DRMViewport2_GetDevice(p,a)                   (p)->GetDevice(p,a)
#define IDirect3DRMViewport2_GetPlane(p,a,b,c,d)              (p)->GetPlane(p,a,b,c,d)
#define IDirect3DRMViewport2_Pick(p,a,b,c)                    (p)->Pick(p,a,b,c)
#define IDirect3DRMViewport2_GetUniformScaling(p)             (p)->GetUniformScaling(p)
#define IDirect3DRMViewport2_GetX(p)                          (p)->GetX(p)
#define IDirect3DRMViewport2_GetY(p)                          (p)->GetY(p)
#define IDirect3DRMViewport2_GetWidth(p)                      (p)->GetWidth(p)
#define IDirect3DRMViewport2_GetHeight(p)                     (p)->GetHeight(p)
#define IDirect3DRMViewport2_GetField(p)                      (p)->GetField(p)
#define IDirect3DRMViewport2_GetBack(p)                       (p)->GetBack(p)
#define IDirect3DRMViewport2_GetFront(p)                      (p)->GetFront(p)
#define IDirect3DRMViewport2_GetProjection(p)                 (p)->GetProjection(p)
#define IDirect3DRMViewport2_GetDirect3DViewport(p,a)         (p)->GetDirect3DViewport(p,a)
#define IDirect3DRMViewport2_TransformVectors(p,a,b,c)        (p)->TransformVectors(a,b,c)
#define IDirect3DRMViewport2_InverseTransformVectors(p,a,b,c) (p)->InverseTransformVectors(ab,c)
#endif

/*****************************************************************************
 * IDirect3DRMFrame interface
 */
#define INTERFACE IDirect3DRMFrame
DECLARE_INTERFACE_(IDirect3DRMFrame,IDirect3DRMVisual)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DRMObject methods ***/
    STDMETHOD(Clone)(THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD(AddDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) PURE;
    STDMETHOD(DeleteDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) PURE;
    STDMETHOD(SetAppData)(THIS_ DWORD data) PURE;
    STDMETHOD_(DWORD, GetAppData)(THIS) PURE;
    STDMETHOD(SetName)(THIS_ LPCSTR) PURE;
    STDMETHOD(GetName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    STDMETHOD(GetClassName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    /*** IDirect3DRMFrame methods ***/
    STDMETHOD(AddChild)(THIS_ LPDIRECT3DRMFRAME child) PURE;
    STDMETHOD(AddLight)(THIS_ LPDIRECT3DRMLIGHT) PURE;
    STDMETHOD(AddMoveCallback)(THIS_ D3DRMFRAMEMOVECALLBACK, VOID *arg) PURE;
    STDMETHOD(AddTransform)(THIS_ D3DRMCOMBINETYPE, D3DRMMATRIX4D) PURE;
    STDMETHOD(AddTranslation)(THIS_ D3DRMCOMBINETYPE, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(AddScale)(THIS_ D3DRMCOMBINETYPE, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz) PURE;
    STDMETHOD(AddRotation)(THIS_ D3DRMCOMBINETYPE, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta) PURE;
    STDMETHOD(AddVisual)(THIS_ LPDIRECT3DRMVISUAL) PURE;
    STDMETHOD(GetChildren)(THIS_ LPDIRECT3DRMFRAMEARRAY *children) PURE;
    STDMETHOD_(D3DCOLOR, GetColor)(THIS) PURE;
    STDMETHOD(GetLights)(THIS_ LPDIRECT3DRMLIGHTARRAY *lights) PURE;
    STDMETHOD_(D3DRMMATERIALMODE, GetMaterialMode)(THIS) PURE;
    STDMETHOD(GetParent)(THIS_ LPDIRECT3DRMFRAME *) PURE;
    STDMETHOD(GetPosition)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR return_position) PURE;
    STDMETHOD(GetRotation)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR axis, LPD3DVALUE return_theta) PURE;
    STDMETHOD(GetScene)(THIS_ LPDIRECT3DRMFRAME *) PURE;
    STDMETHOD_(D3DRMSORTMODE, GetSortMode)(THIS) PURE;
    STDMETHOD(GetTexture)(THIS_ LPDIRECT3DRMTEXTURE *) PURE;
    STDMETHOD(GetTransform)(THIS_ D3DRMMATRIX4D return_matrix) PURE;
    STDMETHOD(GetVelocity)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR return_velocity, BOOL with_rotation) PURE;
    STDMETHOD(GetOrientation)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR dir, LPD3DVECTOR up) PURE;
    STDMETHOD(GetVisuals)(THIS_ LPDIRECT3DRMVISUALARRAY *visuals) PURE;
    STDMETHOD(GetTextureTopology)(THIS_ BOOL *wrap_u, BOOL *wrap_v) PURE;
    STDMETHOD(InverseTransform)(THIS_ D3DVECTOR *d, D3DVECTOR *s) PURE;
    STDMETHOD(Load)(THIS_ LPVOID filename, LPVOID name, D3DRMLOADOPTIONS loadflags, D3DRMLOADTEXTURECALLBACK,
        LPVOID pArg)PURE;
    STDMETHOD(LookAt)(THIS_ LPDIRECT3DRMFRAME target, LPDIRECT3DRMFRAME reference, D3DRMFRAMECONSTRAINT) PURE;
    STDMETHOD(Move)(THIS_ D3DVALUE delta) PURE;
    STDMETHOD(DeleteChild)(THIS_ LPDIRECT3DRMFRAME) PURE;
    STDMETHOD(DeleteLight)(THIS_ LPDIRECT3DRMLIGHT) PURE;
    STDMETHOD(DeleteMoveCallback)(THIS_ D3DRMFRAMEMOVECALLBACK, VOID *arg) PURE;
    STDMETHOD(DeleteVisual)(THIS_ LPDIRECT3DRMVISUAL) PURE;
    STDMETHOD_(D3DCOLOR, GetSceneBackground)(THIS) PURE;
    STDMETHOD(GetSceneBackgroundDepth)(THIS_ LPDIRECTDRAWSURFACE *) PURE;
    STDMETHOD_(D3DCOLOR, GetSceneFogColor)(THIS) PURE;
    STDMETHOD_(BOOL, GetSceneFogEnable)(THIS) PURE;
    STDMETHOD_(D3DRMFOGMODE, GetSceneFogMode)(THIS) PURE;
    STDMETHOD(GetSceneFogParams)(THIS_ D3DVALUE *return_start, D3DVALUE *return_end, D3DVALUE *return_density) PURE;
    STDMETHOD(SetSceneBackground)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetSceneBackgroundRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD(SetSceneBackgroundDepth)(THIS_ LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(SetSceneBackgroundImage)(THIS_ LPDIRECT3DRMTEXTURE) PURE;
    STDMETHOD(SetSceneFogEnable)(THIS_ BOOL) PURE;
    STDMETHOD(SetSceneFogColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetSceneFogMode)(THIS_ D3DRMFOGMODE) PURE;
    STDMETHOD(SetSceneFogParams)(THIS_ D3DVALUE start, D3DVALUE end, D3DVALUE density) PURE;
    STDMETHOD(SetColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetColorRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD_(D3DRMZBUFFERMODE, GetZbufferMode)(THIS) PURE;
    STDMETHOD(SetMaterialMode)(THIS_ D3DRMMATERIALMODE) PURE;
    STDMETHOD(SetOrientation)(THIS_ LPDIRECT3DRMFRAME reference, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz) PURE;
    STDMETHOD(SetPosition)(THIS_ LPDIRECT3DRMFRAME reference, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(SetRotation)(THIS_ LPDIRECT3DRMFRAME reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta) PURE;
    STDMETHOD(SetSortMode)(THIS_ D3DRMSORTMODE) PURE;
    STDMETHOD(SetTexture)(THIS_ LPDIRECT3DRMTEXTURE) PURE;
    STDMETHOD(SetTextureTopology)(THIS_ BOOL wrap_u, BOOL wrap_v) PURE;
    STDMETHOD(SetVelocity)(THIS_ LPDIRECT3DRMFRAME reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, BOOL with_rotation) PURE;
    STDMETHOD(SetZbufferMode)(THIS_ D3DRMZBUFFERMODE) PURE;
    STDMETHOD(Transform)(THIS_ D3DVECTOR *d, D3DVECTOR *s) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirect3DRMFrame_QueryInterface(p,a,b)            (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DRMFrame_AddRef(p)                        (p)->lpVtbl->AddRef(p)
#define IDirect3DRMFrame_Release(p)                       (p)->lpVtbl->Release(p)
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMFrame_Clone(p,a,b,c)                   (p)->lpVtbl->Clone(p,a,b,c)
#define IDirect3DRMFrame_AddDestroyCallback(p,a,b)        (p)->lpVtbl->AddDestroyCallback(p,a,b)
#define IDirect3DRMFrame_DeleteDestroyCallback(p,a,b)     (p)->lpVtbl->DeleteDestroyCallback(p,a,b)
#define IDirect3DRMFrame_SetAppData(p,a)                  (p)->lpVtbl->SetAppData(p,a)
#define IDirect3DRMFrame_GetAppData(p)                    (p)->lpVtbl->GetAppData(p)
#define IDirect3DRMFrame_SetName(p,a)                     (p)->lpVtbl->SetName(p,a)
#define IDirect3DRMFrame_GetName(p,a,b)                   (p)->lpVtbl->GetName(p,a,b)
#define IDirect3DRMFrame_GetClassName(p,a,b)              (p)->lpVtbl->GetClassName(p,a,b)
/*** IDirect3DRMFrame methods ***/
#define IDirect3DRMFrame_AddChild(p,a)                    (p)->lpVtbl->AddChild(p,a)
#define IDirect3DRMFrame_AddLight(p,a)                    (p)->lpVtbl->AddLight(p,a)
#define IDirect3DRMFrame_AddMoveCallback(p,a,b)           (p)->lpVtbl->AddMoveCallback(p,a,b)
#define IDirect3DRMFrame_AddTransform(p,a,b)              (p)->lpVtbl->AddTransform(p,a,b)
#define IDirect3DRMFrame_AddTranslation(p,a,b,c,d)        (p)->lpVtbl->AddTranslation(p,a,b,c,d)
#define IDirect3DRMFrame_AddScale(p,a,b,c,d)              (p)->lpVtbl->AddScale(p,a,b,c,d)
#define IDirect3DRMFrame_AddRotation(p,a,b,c,d,e)         (p)->lpVtbl->AddRotation(p,a,b,c,d,e)
#define IDirect3DRMFrame_AddVisual(p,a)                   (p)->lpVtbl->AddVisual(p,a)
#define IDirect3DRMFrame_GetChildren(p,a)                 (p)->lpVtbl->GetChildren(p,a)
#define IDirect3DRMFrame_GetColor(p)                      (p)->lpVtbl->GetColor(p)
#define IDirect3DRMFrame_GetLights(p,a)                   (p)->lpVtbl->GetLights(p,a)
#define IDirect3DRMFrame_GetMaterialMode(p)               (p)->lpVtbl->GetMaterialMode(p)
#define IDirect3DRMFrame_GetParent(p,a)                   (p)->lpVtbl->GetParent(p,a)
#define IDirect3DRMFrame_GetPosition(p,a,b)               (p)->lpVtbl->GetPosition(p,a,b)
#define IDirect3DRMFrame_GetRotation(p,a,b,c)             (p)->lpVtbl->GetRotation(p,a,b,c)
#define IDirect3DRMFrame_GetScene(p,a)                    (p)->lpVtbl->GetScene(p,a)
#define IDirect3DRMFrame_GetSortMode(p)                   (p)->lpVtbl->GetSortMode(p)
#define IDirect3DRMFrame_GetTexture(p,a)                  (p)->lpVtbl->GetTexture(p,a)
#define IDirect3DRMFrame_GetTransform(p,a)                (p)->lpVtbl->GetTransform(p,a)
#define IDirect3DRMFrame_GetVelocity(p,a,b,c)             (p)->lpVtbl->GetVelocity(p,a,b,c)
#define IDirect3DRMFrame_GetOrientation(p,a,b,c)          (p)->lpVtbl->GetOrientation(p,a,b,c)
#define IDirect3DRMFrame_GetVisuals(p,a)                  (p)->lpVtbl->GetVisuals(p,a)
#define IDirect3DRMFrame_GetTextureTopology(p,a,b)        (p)->lpVtbl->GetTextureTopology(p,a,b)
#define IDirect3DRMFrame_InverseTransform(p,a,b)          (p)->lpVtbl->InverseTransform(p,a,b)
#define IDirect3DRMFrame_Load(p,a,b,c,d,e)                (p)->lpVtbl->Load(p,a,b,c,d,e)
#define IDirect3DRMFrame_LookAt(p,a,b,c)                  (p)->lpVtbl->LookAt(p,a,b,c)
#define IDirect3DRMFrame_Move(p,a)                        (p)->lpVtbl->Move(p,a)
#define IDirect3DRMFrame_DeleteChild(p,a)                 (p)->lpVtbl->DeleteChild(p,a)
#define IDirect3DRMFrame_DeleteLight(p,a)                 (p)->lpVtbl->DeleteLight(p,a)
#define IDirect3DRMFrame_DeleteMoveCallback(p,a,b)        (p)->lpVtbl->DeleteMoveCallback(p,a,b)
#define IDirect3DRMFrame_DeleteVisual(p,a)                (p)->lpVtbl->DeleteVisual(p,a)
#define IDirect3DRMFrame_GetSceneBackground(p)            (p)->lpVtbl->GetSceneBackground(p)
#define IDirect3DRMFrame_GetSceneBackgroundDepth(p,a)     (p)->lpVtbl->GetSceneBackgroundDepth(p,a)
#define IDirect3DRMFrame_GetSceneFogColor(p)              (p)->lpVtbl->GetSceneFogColor(p)
#define IDirect3DRMFrame_GetSceneFogEnable(p)             (p)->lpVtbl->GetSceneFogEnable(p)
#define IDirect3DRMFrame_GetSceneFogMode(p)               (p)->lpVtbl->GetSceneFogMode(p)
#define IDirect3DRMFrame_GetSceneFogParams(p,a,b,c)       (p)->lpVtbl->GetSceneFogParams(p,a,b,c)
#define IDirect3DRMFrame_SetSceneBackground(p,a)          (p)->lpVtbl->SetSceneBackground(p,a)
#define IDirect3DRMFrame_SetSceneBackgroundRGB(p,a,b,c)   (p)->lpVtbl->SetSceneBackgroundRGB(p,a,b,c)
#define IDirect3DRMFrame_SetSceneBackgroundDepth(p,a)     (p)->lpVtbl->SetSceneBackgroundDepth(p,a)
#define IDirect3DRMFrame_SetSceneBackgroundImage(p,a)     (p)->lpVtbl->SetSceneBackgroundImage(p,a)
#define IDirect3DRMFrame_SetSceneFogEnable(p,a)           (p)->lpVtbl->SetSceneFogEnable(p,a)
#define IDirect3DRMFrame_SetSceneFogColor(p,a)            (p)->lpVtbl->SetSceneFogColor(p,a)
#define IDirect3DRMFrame_SetSceneFogMode(p,a)             (p)->lpVtbl->SetSceneFogMode(p,a)
#define IDirect3DRMFrame_SetSceneFogParams(p,a,b,c)       (p)->lpVtbl->SetSceneFogParams(p,a,b,c)
#define IDirect3DRMFrame_SetColor(p,a)                    (p)->lpVtbl->SetColor(p,a)
#define IDirect3DRMFrame_SetColorRGB(p,a,b,c)             (p)->lpVtbl->SetColorRGB(p,a,b,c)
#define IDirect3DRMFrame_GetZbufferMode(p)                (p)->lpVtbl->GetZbufferMode(p)
#define IDirect3DRMFrame_SetMaterialMode(p,a)             (p)->lpVtbl->SetMaterialMode(p,a)
#define IDirect3DRMFrame_SetOrientation(p,a,b,c,d,e,f,g)  (p)->lpVtbl->SetOrientation(p,a,b,c,d,e,f,g)
#define IDirect3DRMFrame_SetPosition(p,a,b,c,d)           (p)->lpVtbl->SetPosition(p,a,b,c,d)
#define IDirect3DRMFrame_SetRotation(p,a,b,c,d,e)         (p)->lpVtbl->SetRotation(p,a,b,c,d,e)
#define IDirect3DRMFrame_SetSortMode(p,a)                 (p)->lpVtbl->SetSortMode(p,a)
#define IDirect3DRMFrame_SetTexture(p,a)                  (p)->lpVtbl->SetTexture(p,a)
#define IDirect3DRMFrame_SetTextureTopology(p,a,b)        (p)->lpVtbl->SetTextureTopology(p,a,b)
#define IDirect3DRMFrame_SetVelocity(p,a,b,c,d,e)         (p)->lpVtbl->SetVelocity(p,a,b,c,d,e)
#define IDirect3DRMFrame_SetZbufferMode(p,a)              (p)->lpVtbl->SetZbufferMode(p,a)
#define IDirect3DRMFrame_Transform(p,a,b)                 (p)->lpVtbl->Transform(p,a,b)
#else
/*** IUnknown methods ***/
#define IDirect3DRMFrame_QueryInterface(p,a,b)            (p)->QueryInterface(a,b)
#define IDirect3DRMFrame_AddRef(p)                        (p)->AddRef()
#define IDirect3DRMFrame_Release(p)                       (p)->Release()
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMFrame_Clone(p,a,b,c)                   (p)->Clone(a,b,c)
#define IDirect3DRMFrame_AddDestroyCallback(p,a,b)        (p)->AddDestroyCallback(a,b)
#define IDirect3DRMFrame_DeleteDestroyCallback(p,a,b)     (p)->DeleteDestroyCallback(a,b)
#define IDirect3DRMFrame_SetAppData(p,a)                  (p)->SetAppData(a)
#define IDirect3DRMFrame_GetAppData(p)                    (p)->GetAppData()
#define IDirect3DRMFrame_SetName(p,a)                     (p)->SetName(a)
#define IDirect3DRMFrame_GetName(p,a,b)                   (p)->GetName(a,b)
#define IDirect3DRMFrame_GetClassName(p,a,b)              (p)->GetClassName(a,b)
/*** IDirect3DRMFrame methods ***/
#define IDirect3DRMFrame_AddChild(p,a)                    (p)->AddChild(a)
#define IDirect3DRMFrame_AddLight(p,a)                    (p)->AddLight(a)
#define IDirect3DRMFrame_AddMoveCallback(p,a,b)           (p)->AddMoveCallback(a,b)
#define IDirect3DRMFrame_AddTransform(p,a,b)              (p)->AddTransform(a,b)
#define IDirect3DRMFrame_AddTranslation(p,a,b,c,d)        (p)->AddTranslation(a,b,c,d)
#define IDirect3DRMFrame_AddScale(p,a,b,c,d)              (p)->AddScale(a,b,c,d)
#define IDirect3DRMFrame_AddRotation(p,a,b,c,d,e)         (p)->AddRotation(a,b,c,d,e)
#define IDirect3DRMFrame_AddVisual(p,a)                   (p)->AddVisual(a)
#define IDirect3DRMFrame_GetChildren(p,a)                 (p)->GetChildren(a)
#define IDirect3DRMFrame_GetColor(p)                      (p)->GetColor()
#define IDirect3DRMFrame_GetLights(p,a)                   (p)->GetLights(a)
#define IDirect3DRMFrame_GetMaterialMode(p)               (p)->GetMaterialMode()
#define IDirect3DRMFrame_GetParent(p,a)                   (p)->GetParent(a)
#define IDirect3DRMFrame_GetPosition(p,a,b)               (p)->GetPosition(a,b)
#define IDirect3DRMFrame_GetRotation(p,a,b,c)             (p)->GetRotation(a,b,c)
#define IDirect3DRMFrame_GetScene(p,a)                    (p)->GetScene(a)
#define IDirect3DRMFrame_GetSortMode(p)                   (p)->GetSortMode()
#define IDirect3DRMFrame_GetTexture(p,a)                  (p)->GetTexture(a)
#define IDirect3DRMFrame_GetTransform(p,a)                (p)->GetTransform(a)
#define IDirect3DRMFrame_GetVelocity(p,a,b,c)             (p)->GetVelocity(a,b,c)
#define IDirect3DRMFrame_GetOrientation(p,a,b,c)          (p)->GetOrientation(a,b,c)
#define IDirect3DRMFrame_GetVisuals(p,a)                  (p)->GetVisuals(a)
#define IDirect3DRMFrame_GetTextureTopology(p,a,b)        (p)->GetTextureTopology(a,b)
#define IDirect3DRMFrame_InverseTransform(p,a,b)          (p)->InverseTransform(a,b)
#define IDirect3DRMFrame_Load(p,a,b,c,d,e)                (p)->Load(a,b,c,d,e)
#define IDirect3DRMFrame_LookAt(p,a,b,c)                  (p)->LookAt(a,b,c)
#define IDirect3DRMFrame_Move(p,a)                        (p)->Move(a)
#define IDirect3DRMFrame_DeleteChild(p,a)                 (p)->DeleteChild(a)
#define IDirect3DRMFrame_DeleteLight(p,a)                 (p)->DeleteLight(a)
#define IDirect3DRMFrame_DeleteMoveCallback(p,a,b)        (p)->DeleteMoveCallback(a,b)
#define IDirect3DRMFrame_DeleteVisual(p,a)                (p)->DeleteVisual(a)
#define IDirect3DRMFrame_GetSceneBackground(p)            (p)->GetSceneBackground()
#define IDirect3DRMFrame_GetSceneBackgroundDepth(p,a)     (p)->GetSceneBackgroundDepth(a)
#define IDirect3DRMFrame_GetSceneFogColor(p)              (p)->GetSceneFogColor()
#define IDirect3DRMFrame_GetSceneFogEnable(p)             (p)->GetSceneFogEnable()
#define IDirect3DRMFrame_GetSceneFogMode(p)               (p)->GetSceneFogMode()
#define IDirect3DRMFrame_GetSceneFogParams(p,a,b,c)       (p)->GetSceneFogParams(a,b,c)
#define IDirect3DRMFrame_SetSceneBackground(p,a)          (p)->SetSceneBackground(a)
#define IDirect3DRMFrame_SetSceneBackgroundRGB(p,a,b,c)   (p)->SetSceneBackgroundRGB(a,b,c)
#define IDirect3DRMFrame_SetSceneBackgroundDepth(p,a)     (p)->SetSceneBackgroundDepth(a)
#define IDirect3DRMFrame_SetSceneBackgroundImage(p,a)     (p)->SetSceneBackgroundImage(a)
#define IDirect3DRMFrame_SetSceneFogEnable(p,a)           (p)->SetSceneFogEnable(a)
#define IDirect3DRMFrame_SetSceneFogColor(p,a)            (p)->SetSceneFogColor(a)
#define IDirect3DRMFrame_SetSceneFogMode(p,a)             (p)->SetSceneFogMode(a)
#define IDirect3DRMFrame_SetSceneFogParams(p,a,b,c)       (p)->SetSceneFogParams(a,b,c)
#define IDirect3DRMFrame_SetColor(p,a)                    (p)->SetColor(a)
#define IDirect3DRMFrame_SetColorRGB(p,a,b,c)             (p)->SetColorRGB(a,b,c)
#define IDirect3DRMFrame_GetZbufferMode(p)                (p)->GetZbufferMode()
#define IDirect3DRMFrame_SetMaterialMode(p,a)             (p)->SetMaterialMode(a)
#define IDirect3DRMFrame_SetOrientation(p,a,b,c,d,e,f,g)  (p)->SetOrientation(a,b,c,d,e,f,g)
#define IDirect3DRMFrame_SetPosition(p,a,b,c,d)           (p)->SetPosition(a,b,c,d)
#define IDirect3DRMFrame_SetRotation(p,a,b,c,d,e)         (p)->SetRotation(a,b,c,d,e)
#define IDirect3DRMFrame_SetSortMode(p,a)                 (p)->SetSortMode(a)
#define IDirect3DRMFrame_SetTexture(p,a)                  (p)->SetTexture(a)
#define IDirect3DRMFrame_SetTextureTopology(p,a,b)        (p)->SetTextureTopology(a,b)
#define IDirect3DRMFrame_SetVelocity(p,a,b,c,d,e)         (p)->SetVelocity(a,b,c,d,e)
#define IDirect3DRMFrame_SetZbufferMode(p,a)              (p)->SetZbufferMode(a)
#define IDirect3DRMFrame_Transform(p,a,b)                 (p)->Transform(a,b)
#endif

/*****************************************************************************
 * IDirect3DRMFrame2 interface
 */
#define INTERFACE IDirect3DRMFrame2
DECLARE_INTERFACE_(IDirect3DRMFrame2,IDirect3DRMFrame)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DRMObject methods ***/
    STDMETHOD(Clone)(THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD(AddDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) PURE;
    STDMETHOD(DeleteDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) PURE;
    STDMETHOD(SetAppData)(THIS_ DWORD data) PURE;
    STDMETHOD_(DWORD, GetAppData)(THIS) PURE;
    STDMETHOD(SetName)(THIS_ LPCSTR) PURE;
    STDMETHOD(GetName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    STDMETHOD(GetClassName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    /*** IDirect3DRMFrame methods ***/
    STDMETHOD(AddChild)(THIS_ LPDIRECT3DRMFRAME child) PURE;
    STDMETHOD(AddLight)(THIS_ LPDIRECT3DRMLIGHT) PURE;
    STDMETHOD(AddMoveCallback)(THIS_ D3DRMFRAMEMOVECALLBACK, VOID *arg) PURE;
    STDMETHOD(AddTransform)(THIS_ D3DRMCOMBINETYPE, D3DRMMATRIX4D) PURE;
    STDMETHOD(AddTranslation)(THIS_ D3DRMCOMBINETYPE, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(AddScale)(THIS_ D3DRMCOMBINETYPE, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz) PURE;
    STDMETHOD(AddRotation)(THIS_ D3DRMCOMBINETYPE, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta) PURE;
    STDMETHOD(AddVisual)(THIS_ LPDIRECT3DRMVISUAL) PURE;
    STDMETHOD(GetChildren)(THIS_ LPDIRECT3DRMFRAMEARRAY *children) PURE;
    STDMETHOD_(D3DCOLOR, GetColor)(THIS) PURE;
    STDMETHOD(GetLights)(THIS_ LPDIRECT3DRMLIGHTARRAY *lights) PURE;
    STDMETHOD_(D3DRMMATERIALMODE, GetMaterialMode)(THIS) PURE;
    STDMETHOD(GetParent)(THIS_ LPDIRECT3DRMFRAME *) PURE;
    STDMETHOD(GetPosition)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR return_position) PURE;
    STDMETHOD(GetRotation)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR axis, LPD3DVALUE return_theta) PURE;
    STDMETHOD(GetScene)(THIS_ LPDIRECT3DRMFRAME *) PURE;
    STDMETHOD_(D3DRMSORTMODE, GetSortMode)(THIS) PURE;
    STDMETHOD(GetTexture)(THIS_ LPDIRECT3DRMTEXTURE *) PURE;
    STDMETHOD(GetTransform)(THIS_ D3DRMMATRIX4D return_matrix) PURE;
    STDMETHOD(GetVelocity)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR return_velocity, BOOL with_rotation) PURE;
    STDMETHOD(GetOrientation)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DVECTOR dir, LPD3DVECTOR up) PURE;
    STDMETHOD(GetVisuals)(THIS_ LPDIRECT3DRMVISUALARRAY *visuals) PURE;
    STDMETHOD(GetTextureTopology)(THIS_ BOOL *wrap_u, BOOL *wrap_v) PURE;
    STDMETHOD(InverseTransform)(THIS_ D3DVECTOR *d, D3DVECTOR *s) PURE;
    STDMETHOD(Load)(THIS_ LPVOID filename, LPVOID name, D3DRMLOADOPTIONS loadflags, D3DRMLOADTEXTURECALLBACK,
        LPVOID pArg)PURE;
    STDMETHOD(LookAt)(THIS_ LPDIRECT3DRMFRAME target, LPDIRECT3DRMFRAME reference, D3DRMFRAMECONSTRAINT) PURE;
    STDMETHOD(Move)(THIS_ D3DVALUE delta) PURE;
    STDMETHOD(DeleteChild)(THIS_ LPDIRECT3DRMFRAME) PURE;
    STDMETHOD(DeleteLight)(THIS_ LPDIRECT3DRMLIGHT) PURE;
    STDMETHOD(DeleteMoveCallback)(THIS_ D3DRMFRAMEMOVECALLBACK, VOID *arg) PURE;
    STDMETHOD(DeleteVisual)(THIS_ LPDIRECT3DRMVISUAL) PURE;
    STDMETHOD_(D3DCOLOR, GetSceneBackground)(THIS) PURE;
    STDMETHOD(GetSceneBackgroundDepth)(THIS_ LPDIRECTDRAWSURFACE *) PURE;
    STDMETHOD_(D3DCOLOR, GetSceneFogColor)(THIS) PURE;
    STDMETHOD_(BOOL, GetSceneFogEnable)(THIS) PURE;
    STDMETHOD_(D3DRMFOGMODE, GetSceneFogMode)(THIS) PURE;
    STDMETHOD(GetSceneFogParams)(THIS_ D3DVALUE *return_start, D3DVALUE *return_end, D3DVALUE *return_density) PURE;
    STDMETHOD(SetSceneBackground)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetSceneBackgroundRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD(SetSceneBackgroundDepth)(THIS_ LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(SetSceneBackgroundImage)(THIS_ LPDIRECT3DRMTEXTURE) PURE;
    STDMETHOD(SetSceneFogEnable)(THIS_ BOOL) PURE;
    STDMETHOD(SetSceneFogColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetSceneFogMode)(THIS_ D3DRMFOGMODE) PURE;
    STDMETHOD(SetSceneFogParams)(THIS_ D3DVALUE start, D3DVALUE end, D3DVALUE density) PURE;
    STDMETHOD(SetColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetColorRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD_(D3DRMZBUFFERMODE, GetZbufferMode)(THIS) PURE;
    STDMETHOD(SetMaterialMode)(THIS_ D3DRMMATERIALMODE) PURE;
    STDMETHOD(SetOrientation)(THIS_ LPDIRECT3DRMFRAME reference, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz) PURE;
    STDMETHOD(SetPosition)(THIS_ LPDIRECT3DRMFRAME reference, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(SetRotation)(THIS_ LPDIRECT3DRMFRAME reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta) PURE;
    STDMETHOD(SetSortMode)(THIS_ D3DRMSORTMODE) PURE;
    STDMETHOD(SetTexture)(THIS_ LPDIRECT3DRMTEXTURE) PURE;
    STDMETHOD(SetTextureTopology)(THIS_ BOOL wrap_u, BOOL wrap_v) PURE;
    STDMETHOD(SetVelocity)(THIS_ LPDIRECT3DRMFRAME reference, D3DVALUE x, D3DVALUE y, D3DVALUE z, BOOL with_rotation) PURE;
    STDMETHOD(SetZbufferMode)(THIS_ D3DRMZBUFFERMODE) PURE;
    STDMETHOD(Transform)(THIS_ D3DVECTOR *d, D3DVECTOR *s) PURE;
    /*** IDirect3DRMFrame2 methods ***/
    STDMETHOD(AddMoveCallback2)(THIS_ D3DRMFRAMEMOVECALLBACK, VOID *arg, DWORD flags) PURE;
    STDMETHOD(GetBox)(THIS_ LPD3DRMBOX) PURE;
    STDMETHOD_(BOOL, GetBoxEnable)(THIS) PURE;
    STDMETHOD(GetAxes)(THIS_ LPD3DVECTOR dir, LPD3DVECTOR up);
    STDMETHOD(GetMaterial)(THIS_ LPDIRECT3DRMMATERIAL *) PURE;
    STDMETHOD_(BOOL, GetInheritAxes)(THIS);
    STDMETHOD(GetHierarchyBox)(THIS_ LPD3DRMBOX) PURE;
    STDMETHOD(SetBox)(THIS_ LPD3DRMBOX) PURE;
    STDMETHOD(SetBoxEnable)(THIS_ BOOL) PURE;
    STDMETHOD(SetAxes)(THIS_ D3DVALUE dx, D3DVALUE dy, D3DVALUE dz, D3DVALUE ux, D3DVALUE uy, D3DVALUE uz);
    STDMETHOD(SetInheritAxes)(THIS_ BOOL inherit_from_parent);
    STDMETHOD(SetMaterial)(THIS_ LPDIRECT3DRMMATERIAL) PURE;
    STDMETHOD(SetQuaternion)(THIS_ LPDIRECT3DRMFRAME reference, D3DRMQUATERNION *q) PURE;
    STDMETHOD(RayPick)(THIS_ LPDIRECT3DRMFRAME reference, LPD3DRMRAY ray, DWORD flags,
        LPDIRECT3DRMPICKED2ARRAY *return_visuals) PURE;
    STDMETHOD(Save)(THIS_ LPCSTR filename, D3DRMXOFFORMAT d3dFormat, D3DRMSAVEOPTIONS d3dSaveFlags);
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirect3DRMFrame2_QueryInterface(p,a,b)            (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DRMFrame2_AddRef(p)                        (p)->lpVtbl->AddRef(p)
#define IDirect3DRMFrame2_Release(p)                       (p)->lpVtbl->Release(p)
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMFrame2_Clone(p,a,b,c)                   (p)->lpVtbl->Clone(p,a,b,c)
#define IDirect3DRMFrame2_AddDestroyCallback(p,a,b)        (p)->lpVtbl->AddDestroyCallback(p,a,b)
#define IDirect3DRMFrame2_DeleteDestroyCallback(p,a,b)     (p)->lpVtbl->DeleteDestroyCallback(p,a,b)
#define IDirect3DRMFrame2_SetAppData(p,a)                  (p)->lpVtbl->SetAppData(p,a)
#define IDirect3DRMFrame2_GetAppData(p)                    (p)->lpVtbl->GetAppData(p)
#define IDirect3DRMFrame2_SetName(p,a)                     (p)->lpVtbl->SetName(p,a)
#define IDirect3DRMFrame2_GetName(p,a,b)                   (p)->lpVtbl->GetName(p,a,b)
#define IDirect3DRMFrame2_GetClassName(p,a,b)              (p)->lpVtbl->GetClassName(p,a,b)
/*** IDirect3DRMFrame methods ***/
#define IDirect3DRMFrame2_AddChild(p,a)                    (p)->lpVtbl->AddChild(p,a)
#define IDirect3DRMFrame2_AddLight(p,a)                    (p)->lpVtbl->AddLight(p,a)
#define IDirect3DRMFrame2_AddMoveCallback(p,a,b)           (p)->lpVtbl->AddMoveCallback(p,a,b)
#define IDirect3DRMFrame2_AddTransform(p,a,b)              (p)->lpVtbl->AddTransform(p,a,b)
#define IDirect3DRMFrame2_AddTranslation(p,a,b,c,d)        (p)->lpVtbl->AddTranslation(p,a,b,c,d)
#define IDirect3DRMFrame2_AddScale(p,a,b,c,d)              (p)->lpVtbl->AddScale(p,a,b,c,d)
#define IDirect3DRMFrame2_AddRotation(p,a,b,c,d,e)         (p)->lpVtbl->AddRotation(p,a,b,c,d,e)
#define IDirect3DRMFrame2_AddVisual(p,a)                   (p)->lpVtbl->AddVisual(p,a)
#define IDirect3DRMFrame2_GetChildren(p,a)                 (p)->lpVtbl->GetChildren(p,a)
#define IDirect3DRMFrame2_GetColor(p)                      (p)->lpVtbl->GetColor(p)
#define IDirect3DRMFrame2_GetLights(p,a)                   (p)->lpVtbl->GetLights(p,a)
#define IDirect3DRMFrame2_GetMaterialMode(p)               (p)->lpVtbl->GetMaterialMode(p)
#define IDirect3DRMFrame2_GetParent(p,a)                   (p)->lpVtbl->GetParent(p,a)
#define IDirect3DRMFrame2_GetPosition(p,a,b)               (p)->lpVtbl->GetPosition(p,a,b)
#define IDirect3DRMFrame2_GetRotation(p,a,b,c)             (p)->lpVtbl->GetRotation(p,a,b,c)
#define IDirect3DRMFrame2_GetScene(p,a)                    (p)->lpVtbl->GetScene(p,a)
#define IDirect3DRMFrame2_GetSortMode(p)                   (p)->lpVtbl->GetSortMode(p)
#define IDirect3DRMFrame2_GetTexture(p,a)                  (p)->lpVtbl->GetTexture(p,a)
#define IDirect3DRMFrame2_GetTransform(p,a)                (p)->lpVtbl->GetTransform(p,a)
#define IDirect3DRMFrame2_GetVelocity(p,a,b,c)             (p)->lpVtbl->GetVelocity(p,a,b,c)
#define IDirect3DRMFrame2_GetOrientation(p,a,b,c)          (p)->lpVtbl->GetOrientation(p,a,b,c)
#define IDirect3DRMFrame2_GetVisuals(p,a)                  (p)->lpVtbl->GetVisuals(p,a)
#define IDirect3DRMFrame2_GetTextureTopology(p,a,b)        (p)->lpVtbl->GetTextureTopology(p,a,b)
#define IDirect3DRMFrame2_InverseTransform(p,a,b)          (p)->lpVtbl->InverseTransform(p,a,b)
#define IDirect3DRMFrame2_Load(p,a,b,c,d,e)                (p)->lpVtbl->Load(p,a,b,c,d,e)
#define IDirect3DRMFrame2_LookAt(p,a,b,c)                  (p)->lpVtbl->LookAt(p,a,b,c)
#define IDirect3DRMFrame2_Move(p,a)                        (p)->lpVtbl->Move(p,a)
#define IDirect3DRMFrame2_DeleteChild(p,a)                 (p)->lpVtbl->DeleteChild(p,a)
#define IDirect3DRMFrame2_DeleteLight(p,a)                 (p)->lpVtbl->DeleteLight(p,a)
#define IDirect3DRMFrame2_DeleteMoveCallback(p,a,b)        (p)->lpVtbl->DeleteMoveCallback(p,a,b)
#define IDirect3DRMFrame2_DeleteVisual(p,a)                (p)->lpVtbl->DeleteVisual(p,a)
#define IDirect3DRMFrame2_GetSceneBackground(p)            (p)->lpVtbl->GetSceneBackground(p)
#define IDirect3DRMFrame2_GetSceneBackgroundDepth(p,a)     (p)->lpVtbl->GetSceneBackgroundDepth(p,a)
#define IDirect3DRMFrame2_GetSceneFogColor(p)              (p)->lpVtbl->GetSceneFogColor(p)
#define IDirect3DRMFrame2_GetSceneFogEnable(p)             (p)->lpVtbl->GetSceneFogEnable(p)
#define IDirect3DRMFrame2_GetSceneFogMode(p)               (p)->lpVtbl->GetSceneFogMode(p)
#define IDirect3DRMFrame2_GetSceneFogParams(p,a,b,c)       (p)->lpVtbl->GetSceneFogParams(p,a,b,c)
#define IDirect3DRMFrame2_SetSceneBackground(p,a)          (p)->lpVtbl->SetSceneBackground(p,a)
#define IDirect3DRMFrame2_SetSceneBackgroundRGB(p,a,b,c)   (p)->lpVtbl->SetSceneBackgroundRGB(p,a,b,c)
#define IDirect3DRMFrame2_SetSceneBackgroundDepth(p,a)     (p)->lpVtbl->SetSceneBackgroundDepth(p,a)
#define IDirect3DRMFrame2_SetSceneBackgroundImage(p,a)     (p)->lpVtbl->SetSceneBackgroundImage(p,a)
#define IDirect3DRMFrame2_SetSceneFogEnable(p,a)           (p)->lpVtbl->SetSceneFogEnable(p,a)
#define IDirect3DRMFrame2_SetSceneFogColor(p,a)            (p)->lpVtbl->SetSceneFogColor(p,a)
#define IDirect3DRMFrame2_SetSceneFogMode(p,a)             (p)->lpVtbl->SetSceneFogMode(p,a)
#define IDirect3DRMFrame2_SetSceneFogParams(p,a,b,c)       (p)->lpVtbl->SetSceneFogParams(p,a,b,c)
#define IDirect3DRMFrame2_SetColor(p,a)                    (p)->lpVtbl->SetColor(p,a)
#define IDirect3DRMFrame2_SetColorRGB(p,a,b,c)             (p)->lpVtbl->SetColorRGB(p,a,b,c)
#define IDirect3DRMFrame2_GetZbufferMode(p)                (p)->lpVtbl->GetZbufferMode(p)
#define IDirect3DRMFrame2_SetMaterialMode(p,a)             (p)->lpVtbl->SetMaterialMode(p,a)
#define IDirect3DRMFrame2_SetOrientation(p,a,b,c,d,e,f,g)  (p)->lpVtbl->SetOrientation(p,a,b,c,d,e,f,g)
#define IDirect3DRMFrame2_SetPosition(p,a,b,c,d)           (p)->lpVtbl->SetPosition(p,a,b,c,d)
#define IDirect3DRMFrame2_SetRotation(p,a,b,c,d,e)         (p)->lpVtbl->SetRotation(p,a,b,c,d,e)
#define IDirect3DRMFrame2_SetSortMode(p,a)                 (p)->lpVtbl->SetSortMode(p,a)
#define IDirect3DRMFrame2_SetTexture(p,a)                  (p)->lpVtbl->SetTexture(p,a)
#define IDirect3DRMFrame2_SetTextureTopology(p,a,b)        (p)->lpVtbl->SetTextureTopology(p,a,b)
#define IDirect3DRMFrame2_SetVelocity(p,a,b,c,d,e)         (p)->lpVtbl->SetVelocity(p,a,b,c,d,e)
#define IDirect3DRMFrame2_SetZbufferMode(p,a)              (p)->lpVtbl->SetZbufferMode(p,a)
#define IDirect3DRMFrame2_Transform(p,a,b)                 (p)->lpVtbl->Transform(p,a,b)
/*** IDirect3DRMFrame2 methods ***/
#define IDirect3DRMFrame2_AddMoveCallback2(p,a,b,c)        (p)->lpVtbl->AddMoveCallback2(p,a,b,c)
#define IDirect3DRMFrame2_GetBox(p,a)                      (p)->lpVtbl->GetBox(p,a)
#define IDirect3DRMFrame2_GetBoxEnable(p)                  (p)->lpVtbl->GetBoxEnable(p)
#define IDirect3DRMFrame2_GetAxes(p,a,b)                   (p)->lpVtbl->GetAxes(p,a,b)
#define IDirect3DRMFrame2_GetMaterial(p,a)                 (p)->lpVtbl->GetMaterial(p,a)
#define IDirect3DRMFrame2_GetInheritAxes(p,a,b)            (p)->lpVtbl->GetInheritAxes(p,a,b)
#define IDirect3DRMFrame2_GetHierarchyBox(p,a)             (p)->lpVtbl->GetHierarchyBox(p,a)
#define IDirect3DRMFrame2_SetBox(p,a)                      (p)->lpVtbl->SetBox(p,a)
#define IDirect3DRMFrame2_SetBoxEnable(p,a)                (p)->lpVtbl->SetBoxEnable(p,a)
#define IDirect3DRMFrame2_SetAxes(p,a,b,c,d,e,f)           (p)->lpVtbl->SetAxes(p,a,b,c,d,e,f)
#define IDirect3DRMFrame2_SetInheritAxes(p,a)              (p)->lpVtbl->SetInheritAxes(p,a)
#define IDirect3DRMFrame2_SetMaterial(p,a)                 (p)->lpVtbl->SetMaterial(p,a)
#define IDirect3DRMFrame2_SetQuaternion(p,a,b)             (p)->lpVtbl->SetQuaternion(p,a,b)
#define IDirect3DRMFrame2_RayPick(p,a,b,c,d)               (p)->lpVtbl->RayPick(p,a,b,c,d)
#define IDirect3DRMFrame2_Save(p,a,b,c)                    (p)->lpVtbl->Save(p,a,b,c)
#else
/*** IUnknown methods ***/
#define IDirect3DRMFrame2_QueryInterface(p,a,b)            (p)->QueryInterface(a,b)
#define IDirect3DRMFrame2_AddRef(p)                        (p)->AddRef()
#define IDirect3DRMFrame2_Release(p)                       (p)->Release()
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMFrame2_Clone(p,a,b,c)                   (p)->Clone(a,b,c)
#define IDirect3DRMFrame2_AddDestroyCallback(p,a,b)        (p)->AddDestroyCallback(a,b)
#define IDirect3DRMFrame2_DeleteDestroyCallback(p,a,b)     (p)->DeleteDestroyCallback(a,b)
#define IDirect3DRMFrame2_SetAppData(p,a)                  (p)->SetAppData(a)
#define IDirect3DRMFrame2_GetAppData(p)                    (p)->GetAppData()
#define IDirect3DRMFrame2_SetName(p,a)                     (p)->SetName(a)
#define IDirect3DRMFrame2_GetName(p,a,b)                   (p)->GetName(a,b)
#define IDirect3DRMFrame2_GetClassName(p,a,b)              (p)->GetClassName(a,b)
/*** IDirect3DRMFrame methods ***/
#define IDirect3DRMFrame2_AddChild(p,a)                    (p)->AddChild(a)
#define IDirect3DRMFrame2_AddLight(p,a)                    (p)->AddLight(a)
#define IDirect3DRMFrame2_AddMoveCallback(p,a,b)           (p)->AddMoveCallback(a,b)
#define IDirect3DRMFrame2_AddTransform(p,a,b)              (p)->AddTransform(a,b)
#define IDirect3DRMFrame2_AddTranslation(p,a,b,c,d)        (p)->AddTranslation(a,b,c,d)
#define IDirect3DRMFrame2_AddScale(p,a,b,c,d)              (p)->AddScale(a,b,c,d)
#define IDirect3DRMFrame2_AddRotation(p,a,b,c,d,e)         (p)->AddRotation(a,b,c,d,e)
#define IDirect3DRMFrame2_AddVisual(p,a)                   (p)->AddVisual(a)
#define IDirect3DRMFrame2_GetChildren(p,a)                 (p)->GetChildren(a)
#define IDirect3DRMFrame2_GetColor(p)                      (p)->GetColor()
#define IDirect3DRMFrame2_GetLights(p,a)                   (p)->GetLights(a)
#define IDirect3DRMFrame2_GetMaterialMode(p)               (p)->GetMaterialMode()
#define IDirect3DRMFrame2_GetParent(p,a)                   (p)->GetParent(a)
#define IDirect3DRMFrame2_GetPosition(p,a,b)               (p)->GetPosition(a,b)
#define IDirect3DRMFrame2_GetRotation(p,a,b,c)             (p)->GetRotation(a,b,c)
#define IDirect3DRMFrame2_GetScene(p,a)                    (p)->GetScene(a)
#define IDirect3DRMFrame2_GetSortMode(p)                   (p)->GetSortMode()
#define IDirect3DRMFrame2_GetTexture(p,a)                  (p)->GetTexture(a)
#define IDirect3DRMFrame2_GetTransform(p,a)                (p)->GetTransform(a)
#define IDirect3DRMFrame2_GetVelocity(p,a,b,c)             (p)->GetVelocity(a,b,c)
#define IDirect3DRMFrame2_GetOrientation(p,a,b,c)          (p)->GetOrientation(a,b,c)
#define IDirect3DRMFrame2_GetVisuals(p,a)                  (p)->GetVisuals(a)
#define IDirect3DRMFrame2_GetTextureTopology(p,a,b)        (p)->GetTextureTopology(a,b)
#define IDirect3DRMFrame2_InverseTransform(p,a,b)          (p)->InverseTransform(a,b)
#define IDirect3DRMFrame2_Load(p,a,b,c,d,e)                (p)->Load(a,b,c,d,e)
#define IDirect3DRMFrame2_LookAt(p,a,b,c)                  (p)->LookAt(a,b,c)
#define IDirect3DRMFrame2_Move(p,a)                        (p)->Move(a)
#define IDirect3DRMFrame2_DeleteChild(p,a)                 (p)->DeleteChild(a)
#define IDirect3DRMFrame2_DeleteLight(p,a)                 (p)->DeleteLight(a)
#define IDirect3DRMFrame2_DeleteMoveCallback(p,a,b)        (p)->DeleteMoveCallback(a,b)
#define IDirect3DRMFrame2_DeleteVisual(p,a)                (p)->DeleteVisual(a)
#define IDirect3DRMFrame2_GetSceneBackground(p)            (p)->GetSceneBackground()
#define IDirect3DRMFrame2_GetSceneBackgroundDepth(p,a)     (p)->GetSceneBackgroundDepth(a)
#define IDirect3DRMFrame2_GetSceneFogColor(p)              (p)->GetSceneFogColor()
#define IDirect3DRMFrame2_GetSceneFogEnable(p)             (p)->GetSceneFogEnable()
#define IDirect3DRMFrame2_GetSceneFogMode(p)               (p)->GetSceneFogMode()
#define IDirect3DRMFrame2_GetSceneFogParams(p,a,b,c)       (p)->GetSceneFogParams(a,b,c)
#define IDirect3DRMFrame2_SetSceneBackground(p,a)          (p)->SetSceneBackground(a)
#define IDirect3DRMFrame2_SetSceneBackgroundRGB(p,a,b,c)   (p)->SetSceneBackgroundRGB(a,b,c)
#define IDirect3DRMFrame2_SetSceneBackgroundDepth(p,a)     (p)->SetSceneBackgroundDepth(a)
#define IDirect3DRMFrame2_SetSceneBackgroundImage(p,a)     (p)->SetSceneBackgroundImage(a)
#define IDirect3DRMFrame2_SetSceneFogEnable(p,a)           (p)->SetSceneFogEnable(a)
#define IDirect3DRMFrame2_SetSceneFogColor(p,a)            (p)->SetSceneFogColor(a)
#define IDirect3DRMFrame2_SetSceneFogMode(p,a)             (p)->SetSceneFogMode(a)
#define IDirect3DRMFrame2_SetSceneFogParams(p,a,b,c)       (p)->SetSceneFogParams(a,b,c)
#define IDirect3DRMFrame2_SetColor(p,a)                    (p)->SetColor(a)
#define IDirect3DRMFrame2_SetColorRGB(p,a,b,c)             (p)->SetColorRGB(a,b,c)
#define IDirect3DRMFrame2_GetZbufferMode(p)                (p)->GetZbufferMode()
#define IDirect3DRMFrame2_SetMaterialMode(p,a)             (p)->SetMaterialMode(a)
#define IDirect3DRMFrame2_SetOrientation(p,a,b,c,d,e,f,g)  (p)->SetOrientation(a,b,c,d,e,f,g)
#define IDirect3DRMFrame2_SetPosition(p,a,b,c,d)           (p)->SetPosition(a,b,c,d)
#define IDirect3DRMFrame2_SetRotation(p,a,b,c,d,e)         (p)->SetRotation(a,b,c,d,e)
#define IDirect3DRMFrame2_SetSortMode(p,a)                 (p)->SetSortMode(a)
#define IDirect3DRMFrame2_SetTexture(p,a)                  (p)->SetTexture(a)
#define IDirect3DRMFrame2_SetTextureTopology(p,a,b)        (p)->SetTextureTopology(a,b)
#define IDirect3DRMFrame2_SetVelocity(p,a,b,c,d,e)         (p)->SetVelocity(a,b,c,d,e)
#define IDirect3DRMFrame2_SetZbufferMode(p,a)              (p)->SetZbufferMode(a)
#define IDirect3DRMFrame2_Transform(p,a,b)                 (p)->Transform(a,b)
/*** IDirect3DRMFrame2 methods ***/
#define IDirect3DRMFrame2_AddMoveCallback2(p,a,b,c)        (p)->AddMoveCallback2(a,b,c)
#define IDirect3DRMFrame2_GetBox(p,a)                      (p)->GetBox(a)
#define IDirect3DRMFrame2_GetBoxEnable(p)                  (p)->GetBoxEnable()
#define IDirect3DRMFrame2_GetAxes(p,a,b)                   (p)->GetAxes(a,b)
#define IDirect3DRMFrame2_GetMaterial(p,a)                 (p)->GetMaterial(a)
#define IDirect3DRMFrame2_GetInheritAxes(p,a,b)            (p)->GetInheritAxes(a,b)
#define IDirect3DRMFrame2_GetHierarchyBox(p,a)             (p)->GetHierarchyBox(a)
#define IDirect3DRMFrame2_SetBox(p,a)                      (p)->SetBox(a)
#define IDirect3DRMFrame2_SetBoxEnable(p,a)                (p)->SetBoxEnable(a)
#define IDirect3DRMFrame2_SetAxes(p,a,b,c,d,e,f)           (p)->SetAxes(a,b,c,d,e,f)
#define IDirect3DRMFrame2_SetInheritAxes(p,a)              (p)->SetInheritAxes(a)
#define IDirect3DRMFrame2_SetMaterial(p,a)                 (p)->SetMaterial(a)
#define IDirect3DRMFrame2_SetQuaternion(p,a,b)             (p)->SetQuaternion(a,b)
#define IDirect3DRMFrame2_RayPick(p,a,b,c,d)               (p)->RayPick(a,b,c,d)
#define IDirect3DRMFrame2_Save(p,a,b,c)                    (p)->Save(a,b,c)
#endif

/*****************************************************************************
 * IDirect3DRMFrame3 interface
 */
#define INTERFACE IDirect3DRMFrame3
DECLARE_INTERFACE_(IDirect3DRMFrame3,IDirect3DRMVisual)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DRMObject methods ***/
    STDMETHOD(Clone)(THIS_ LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD(AddDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) PURE;
    STDMETHOD(DeleteDestroyCallback)(THIS_ D3DRMOBJECTCALLBACK, LPVOID argument) PURE;
    STDMETHOD(SetAppData)(THIS_ DWORD data) PURE;
    STDMETHOD_(DWORD, GetAppData)(THIS) PURE;
    STDMETHOD(SetName)(THIS_ LPCSTR) PURE;
    STDMETHOD(GetName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    STDMETHOD(GetClassName)(THIS_ LPDWORD lpdwSize, LPSTR lpName) PURE;
    /*** IDirect3DRMFrame3 methods ***/
    STDMETHOD(AddChild)(THIS_ LPDIRECT3DRMFRAME3 child) PURE;
    STDMETHOD(AddLight)(THIS_ LPDIRECT3DRMLIGHT) PURE;
    STDMETHOD(AddMoveCallback)(THIS_ D3DRMFRAME3MOVECALLBACK, VOID *arg, DWORD flags) PURE;
    STDMETHOD(AddTranslation)(THIS_ D3DRMCOMBINETYPE, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(AddScale)(THIS_ D3DRMCOMBINETYPE, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz) PURE;
    STDMETHOD(AddRotation)(THIS_ D3DRMCOMBINETYPE, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta) PURE;
    STDMETHOD(AddVisual)(THIS_ LPUNKNOWN) PURE;
    STDMETHOD(GetChildren)(THIS_ LPDIRECT3DRMFRAMEARRAY *children) PURE;
    STDMETHOD_(D3DCOLOR, GetColor)(THIS) PURE;
    STDMETHOD(GetLights)(THIS_ LPDIRECT3DRMLIGHTARRAY *lights) PURE;
    STDMETHOD_(D3DRMMATERIALMODE, GetMaterialMode)(THIS) PURE;
    STDMETHOD(GetParent)(THIS_ LPDIRECT3DRMFRAME3 *) PURE;
    STDMETHOD(GetPosition)(THIS_ LPDIRECT3DRMFRAME3 reference, LPD3DVECTOR return_position) PURE;
    STDMETHOD(GetRotation)(THIS_ LPDIRECT3DRMFRAME3 reference, LPD3DVECTOR axis, LPD3DVALUE return_theta) PURE;
    STDMETHOD(GetScene)(THIS_ LPDIRECT3DRMFRAME3 *) PURE;
    STDMETHOD_(D3DRMSORTMODE, GetSortMode)(THIS) PURE;
    STDMETHOD(GetTexture)(THIS_ LPDIRECT3DRMTEXTURE3 *) PURE;
    STDMETHOD(GetTransform)(THIS_ LPDIRECT3DRMFRAME3 reference, D3DRMMATRIX4D rmMatrix) PURE;
    STDMETHOD(GetVelocity)(THIS_ LPDIRECT3DRMFRAME3 reference, LPD3DVECTOR return_velocity,
        BOOL with_rotation) PURE;
    STDMETHOD(GetOrientation)(THIS_ LPDIRECT3DRMFRAME3 reference, LPD3DVECTOR dir, LPD3DVECTOR up) PURE;
    STDMETHOD(GetVisuals)(THIS_ LPDWORD pCount, LPUNKNOWN *) PURE;
    STDMETHOD(InverseTransform)(THIS_ D3DVECTOR *d, D3DVECTOR *s) PURE;
    STDMETHOD(Load)(THIS_ LPVOID filename, LPVOID name, D3DRMLOADOPTIONS loadflags,
        D3DRMLOADTEXTURE3CALLBACK, LPVOID pArg) PURE;
    STDMETHOD(LookAt)(THIS_ LPDIRECT3DRMFRAME3 target, LPDIRECT3DRMFRAME3 reference, D3DRMFRAMECONSTRAINT) PURE;
    STDMETHOD(Move)(THIS_ D3DVALUE delta) PURE;
    STDMETHOD(DeleteChild)(THIS_ LPDIRECT3DRMFRAME3) PURE;
    STDMETHOD(DeleteLight)(THIS_ LPDIRECT3DRMLIGHT) PURE;
    STDMETHOD(DeleteMoveCallback)(THIS_ D3DRMFRAME3MOVECALLBACK, VOID *arg) PURE;
    STDMETHOD(DeleteVisual)(THIS_ LPUNKNOWN) PURE;
    STDMETHOD_(D3DCOLOR, GetSceneBackground)(THIS) PURE;
    STDMETHOD(GetSceneBackgroundDepth)(THIS_ LPDIRECTDRAWSURFACE *) PURE;
    STDMETHOD_(D3DCOLOR, GetSceneFogColor)(THIS) PURE;
    STDMETHOD_(BOOL, GetSceneFogEnable)(THIS) PURE;
    STDMETHOD_(D3DRMFOGMODE, GetSceneFogMode)(THIS) PURE;
    STDMETHOD(GetSceneFogParams)(THIS_ D3DVALUE *return_start, D3DVALUE *return_end,
        D3DVALUE *return_density) PURE;
    STDMETHOD(SetSceneBackground)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetSceneBackgroundRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD(SetSceneBackgroundDepth)(THIS_ LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(SetSceneBackgroundImage)(THIS_ LPDIRECT3DRMTEXTURE3) PURE;
    STDMETHOD(SetSceneFogEnable)(THIS_ BOOL) PURE;
    STDMETHOD(SetSceneFogColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetSceneFogMode)(THIS_ D3DRMFOGMODE) PURE;
    STDMETHOD(SetSceneFogParams)(THIS_ D3DVALUE start, D3DVALUE end, D3DVALUE density) PURE;
    STDMETHOD(SetColor)(THIS_ D3DCOLOR) PURE;
    STDMETHOD(SetColorRGB)(THIS_ D3DVALUE red, D3DVALUE green, D3DVALUE blue) PURE;
    STDMETHOD_(D3DRMZBUFFERMODE, GetZbufferMode)(THIS) PURE;
    STDMETHOD(SetMaterialMode)(THIS_ D3DRMMATERIALMODE) PURE;
    STDMETHOD(SetOrientation)(THIS_ LPDIRECT3DRMFRAME3 reference, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz,
        D3DVALUE ux, D3DVALUE uy, D3DVALUE uz) PURE;
    STDMETHOD(SetPosition)(THIS_ LPDIRECT3DRMFRAME3 reference, D3DVALUE x, D3DVALUE y, D3DVALUE z) PURE;
    STDMETHOD(SetRotation)(THIS_ LPDIRECT3DRMFRAME3 reference, D3DVALUE x, D3DVALUE y, D3DVALUE z,
        D3DVALUE theta) PURE;
    STDMETHOD(SetSortMode)(THIS_ D3DRMSORTMODE) PURE;
    STDMETHOD(SetTexture)(THIS_ LPDIRECT3DRMTEXTURE3) PURE;
    STDMETHOD(SetVelocity)(THIS_ LPDIRECT3DRMFRAME3 reference, D3DVALUE x, D3DVALUE y, D3DVALUE z,
        BOOL with_rotation) PURE;
    STDMETHOD(SetZbufferMode)(THIS_ D3DRMZBUFFERMODE) PURE;
    STDMETHOD(Transform)(THIS_ D3DVECTOR *d, D3DVECTOR *s) PURE;
    STDMETHOD(GetBox)(THIS_ LPD3DRMBOX) PURE;
    STDMETHOD_(BOOL, GetBoxEnable)(THIS) PURE;
    STDMETHOD(GetAxes)(THIS_ LPD3DVECTOR dir, LPD3DVECTOR up);
    STDMETHOD(GetMaterial)(THIS_ LPDIRECT3DRMMATERIAL2 *) PURE;
    STDMETHOD_(BOOL, GetInheritAxes)(THIS);
    STDMETHOD(GetHierarchyBox)(THIS_ LPD3DRMBOX) PURE;
    STDMETHOD(SetBox)(THIS_ LPD3DRMBOX) PURE;
    STDMETHOD(SetBoxEnable)(THIS_ BOOL) PURE;
    STDMETHOD(SetAxes)(THIS_ D3DVALUE dx, D3DVALUE dy, D3DVALUE dz, D3DVALUE ux, D3DVALUE uy, D3DVALUE uz);
    STDMETHOD(SetInheritAxes)(THIS_ BOOL inherit_from_parent);
    STDMETHOD(SetMaterial)(THIS_ LPDIRECT3DRMMATERIAL2) PURE;
    STDMETHOD(SetQuaternion)(THIS_ LPDIRECT3DRMFRAME3 reference, D3DRMQUATERNION *q) PURE;
    STDMETHOD(RayPick)(THIS_ LPDIRECT3DRMFRAME3 reference, LPD3DRMRAY ray, DWORD flags,
        LPDIRECT3DRMPICKED2ARRAY *return_visuals) PURE;
    STDMETHOD(Save)(THIS_ LPCSTR filename, D3DRMXOFFORMAT d3dFormat, D3DRMSAVEOPTIONS d3dSaveFlags);
    STDMETHOD(TransformVectors)(THIS_ LPDIRECT3DRMFRAME3 reference, DWORD NumVectors,
        LPD3DVECTOR pDstVectors, LPD3DVECTOR pSrcVectors) PURE;
    STDMETHOD(InverseTransformVectors)(THIS_ LPDIRECT3DRMFRAME3 reference, DWORD NumVectors,
        LPD3DVECTOR pDstVectors, LPD3DVECTOR pSrcVectors) PURE;
    STDMETHOD(SetTraversalOptions)(THIS_ DWORD flags) PURE;
    STDMETHOD(GetTraversalOptions)(THIS_ LPDWORD pFlags) PURE;
    STDMETHOD(SetSceneFogMethod)(THIS_ DWORD flags) PURE;
    STDMETHOD(GetSceneFogMethod)(THIS_ LPDWORD pFlags) PURE;
    STDMETHOD(SetMaterialOverride)(THIS_ LPD3DRMMATERIALOVERRIDE) PURE;
    STDMETHOD(GetMaterialOverride)(THIS_ LPD3DRMMATERIALOVERRIDE) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDirect3DRMFrame3_QueryInterface(p,a,b)              (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DRMFrame3_AddRef(p)                          (p)->lpVtbl->AddRef(p)
#define IDirect3DRMFrame3_Release(p)                         (p)->lpVtbl->Release(p)
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMFrame3_Clone(p,a,b,c)                     (p)->lpVtbl->Clone(p,a,b,c)
#define IDirect3DRMFrame3_AddDestroyCallback(p,a,b)          (p)->lpVtbl->AddDestroyCallback(p,a,b)
#define IDirect3DRMFrame3_DeleteDestroyCallback(p,a,b)       (p)->lpVtbl->DeleteDestroyCallback(p,a,b)
#define IDirect3DRMFrame3_SetAppData(p,a)                    (p)->lpVtbl->SetAppData(p,a)
#define IDirect3DRMFrame3_GetAppData(p)                      (p)->lpVtbl->GetAppData(p)
#define IDirect3DRMFrame3_SetName(p,a)                       (p)->lpVtbl->SetName(p,a)
#define IDirect3DRMFrame3_GetName(p,a,b)                     (p)->lpVtbl->GetName(p,a,b)
#define IDirect3DRMFrame3_GetClassName(p,a,b)                (p)->lpVtbl->GetClassName(p,a,b)
/*** IDirect3DRMFrame3 methods ***/
#define IDirect3DRMFrame3_AddChild(p,a)                      (p)->lpVtbl->AddChild(p,a)
#define IDirect3DRMFrame3_AddLight(p,a)                      (p)->lpVtbl->AddLight(p,a)
#define IDirect3DRMFrame3_AddMoveCallback(p,a,b,c)           (p)->lpVtbl->AddMoveCallback(p,a,b,c)
#define IDirect3DRMFrame3_AddTransform(p,a,b)                (p)->lpVtbl->AddTransform(p,a,b)
#define IDirect3DRMFrame3_AddTranslation(p,a,b,c,d)          (p)->lpVtbl->AddTranslation(p,a,b,c,d)
#define IDirect3DRMFrame3_AddScale(p,a,b,c,d)                (p)->lpVtbl->AddScale(p,a,b,c,d)
#define IDirect3DRMFrame3_AddRotation(p,a,b,c,d,e)           (p)->lpVtbl->AddRotation(p,a,b,c,d,e)
#define IDirect3DRMFrame3_AddVisual(p,a)                     (p)->lpVtbl->AddVisual(p,a)
#define IDirect3DRMFrame3_GetChildren(p,a)                   (p)->lpVtbl->GetChildren(p,a)
#define IDirect3DRMFrame3_GetColor(p)                        (p)->lpVtbl->GetColor(p)
#define IDirect3DRMFrame3_GetLights(p,a)                     (p)->lpVtbl->GetLights(p,a)
#define IDirect3DRMFrame3_GetMaterialMode(p)                 (p)->lpVtbl->GetMaterialMode(p)
#define IDirect3DRMFrame3_GetParent(p,a)                     (p)->lpVtbl->GetParent(p,a)
#define IDirect3DRMFrame3_GetPosition(p,a,b)                 (p)->lpVtbl->GetPosition(p,a,b)
#define IDirect3DRMFrame3_GetRotation(p,a,b,c)               (p)->lpVtbl->GetRotation(p,a,b,c)
#define IDirect3DRMFrame3_GetScene(p,a)                      (p)->lpVtbl->GetScene(p,a)
#define IDirect3DRMFrame3_GetSortMode(p)                     (p)->lpVtbl->GetSortMode(p)
#define IDirect3DRMFrame3_GetTexture(p,a)                    (p)->lpVtbl->GetTexture(p,a)
#define IDirect3DRMFrame3_GetTransform(p,a,b)                (p)->lpVtbl->GetTransform(p,a,b)
#define IDirect3DRMFrame3_GetVelocity(p,a,b,c)               (p)->lpVtbl->GetVelocity(p,a,b,c)
#define IDirect3DRMFrame3_GetOrientation(p,a,b,c)            (p)->lpVtbl->GetOrientation(p,a,b,c)
#define IDirect3DRMFrame3_GetVisuals(p,a,b)                  (p)->lpVtbl->GetVisuals(p,a,b)
#define IDirect3DRMFrame3_InverseTransform(p,a,b)            (p)->lpVtbl->InverseTransform(p,a,b)
#define IDirect3DRMFrame3_Load(p,a,b,c,d,e)                  (p)->lpVtbl->Load(p,a,b,c,d,e)
#define IDirect3DRMFrame3_LookAt(p,a,b,c)                    (p)->lpVtbl->LookAt(p,a,b,c)
#define IDirect3DRMFrame3_Move(p,a)                          (p)->lpVtbl->Move(p,a)
#define IDirect3DRMFrame3_DeleteChild(p,a)                   (p)->lpVtbl->DeleteChild(p,a)
#define IDirect3DRMFrame3_DeleteLight(p,a)                   (p)->lpVtbl->DeleteLight(p,a)
#define IDirect3DRMFrame3_DeleteMoveCallback(p,a,b)          (p)->lpVtbl->DeleteMoveCallback(p,a,b)
#define IDirect3DRMFrame3_DeleteVisual(p,a)                  (p)->lpVtbl->DeleteVisual(p,a)
#define IDirect3DRMFrame3_GetSceneBackground(p)              (p)->lpVtbl->GetSceneBackground(p)
#define IDirect3DRMFrame3_GetSceneBackgroundDepth(p,a)       (p)->lpVtbl->GetSceneBackgroundDepth(p,a)
#define IDirect3DRMFrame3_GetSceneFogColor(p)                (p)->lpVtbl->GetSceneFogColor(p)
#define IDirect3DRMFrame3_GetSceneFogEnable(p)               (p)->lpVtbl->GetSceneFogEnable(p)
#define IDirect3DRMFrame3_GetSceneFogMode(p)                 (p)->lpVtbl->GetSceneFogMode(p)
#define IDirect3DRMFrame3_GetSceneFogParams(p,a,b,c)         (p)->lpVtbl->GetSceneFogParams(p,a,b,c)
#define IDirect3DRMFrame3_SetSceneBackground(p,a)            (p)->lpVtbl->SetSceneBackground(p,a)
#define IDirect3DRMFrame3_SetSceneBackgroundRGB(p,a,b,c)     (p)->lpVtbl->SetSceneBackgroundRGB(p,a,b,c)
#define IDirect3DRMFrame3_SetSceneBackgroundDepth(p,a)       (p)->lpVtbl->SetSceneBackgroundDepth(p,a)
#define IDirect3DRMFrame3_SetSceneBackgroundImage(p,a)       (p)->lpVtbl->SetSceneBackgroundImage(p,a)
#define IDirect3DRMFrame3_SetSceneFogEnable(p,a)             (p)->lpVtbl->SetSceneFogEnable(p,a)
#define IDirect3DRMFrame3_SetSceneFogColor(p,a)              (p)->lpVtbl->SetSceneFogColor(p,a)
#define IDirect3DRMFrame3_SetSceneFogMode(p,a)               (p)->lpVtbl->SetSceneFogMode(p,a)
#define IDirect3DRMFrame3_SetSceneFogParams(p,a,b,c)         (p)->lpVtbl->SetSceneFogParams(p,a,b,c)
#define IDirect3DRMFrame3_SetColor(p,a)                      (p)->lpVtbl->SetColor(p,a)
#define IDirect3DRMFrame3_SetColorRGB(p,a,b,c)               (p)->lpVtbl->SetColorRGB(p,a,b,c)
#define IDirect3DRMFrame3_GetZbufferMode(p)                  (p)->lpVtbl->GetZbufferMode(p)
#define IDirect3DRMFrame3_SetMaterialMode(p,a)               (p)->lpVtbl->SetMaterialMode(p,a)
#define IDirect3DRMFrame3_SetOrientation(p,a,b,c,d,e,f,g)    (p)->lpVtbl->SetOrientation(p,a,b,c,d,e,f,g)
#define IDirect3DRMFrame3_SetPosition(p,a,b,c,d)             (p)->lpVtbl->SetPosition(p,a,b,c,d)
#define IDirect3DRMFrame3_SetRotation(p,a,b,c,d,e)           (p)->lpVtbl->SetRotation(p,a,b,c,d,e)
#define IDirect3DRMFrame3_SetSortMode(p,a)                   (p)->lpVtbl->SetSortMode(p,a)
#define IDirect3DRMFrame3_SetTexture(p,a)                    (p)->lpVtbl->SetTexture(p,a)
#define IDirect3DRMFrame3_SetVelocity(p,a,b,c,d,e)           (p)->lpVtbl->SetVelocity(p,a,b,c,d,e)
#define IDirect3DRMFrame3_SetZbufferMode(p,a)                (p)->lpVtbl->SetZbufferMode(p,a)
#define IDirect3DRMFrame3_Transform(p,a,b)                   (p)->lpVtbl->Transform(p,a,b)
#define IDirect3DRMFrame3_GetBox(p,a)                        (p)->lpVtbl->GetBox(p,a)
#define IDirect3DRMFrame3_GetBoxEnable(p)                    (p)->lpVtbl->GetBoxEnable(p)
#define IDirect3DRMFrame3_GetAxes(p,a,b)                     (p)->lpVtbl->GetAxes(p,a,b)
#define IDirect3DRMFrame3_GetMaterial(p,a)                   (p)->lpVtbl->GetMaterial(p,a)
#define IDirect3DRMFrame3_GetInheritAxes(p)                  (p)->lpVtbl->GetInheritAxes(p)
#define IDirect3DRMFrame3_GetHierarchyBox(p,a)               (p)->lpVtbl->GetHierarchyBox(p,a)
#define IDirect3DRMFrame3_SetBox(p,a)                        (p)->lpVtbl->SetBox(p,a)
#define IDirect3DRMFrame3_SetBoxEnable(p,a)                  (p)->lpVtbl->SetBoxEnable(p,a)
#define IDirect3DRMFrame3_SetAxes(p,a,b,c,d,e,f)             (p)->lpVtbl->SetAxes(p,a,b,c,d,e,f)
#define IDirect3DRMFrame3_SetInheritAxes(p,a)                (p)->lpVtbl->SetInheritAxes(p,a)
#define IDirect3DRMFrame3_SetMaterial(p,a)                   (p)->lpVtbl->SetMaterial(p,a)
#define IDirect3DRMFrame3_SetQuaternion(p,a,b)               (p)->lpVtbl->SetQuaternion(p,a,b)
#define IDirect3DRMFrame3_RayPick(p,a,b,c,d)                 (p)->lpVtbl->RayPick(p,a,b,c,d)
#define IDirect3DRMFrame3_Save(p,a,b,c)                      (p)->lpVtbl->Save(p,a,b,c)
#define IDirect3DRMFrame3_TransformVectors(p,a,b,c,d)        (p)->lpVtbl->TransformVectors(p,a,b,c,d)
#define IDirect3DRMFrame3_InverseTransformVectors(p,a,b,c,d) (p)->lpVtbl->InverseTransformVectors(p,a,b,c,d)
#define IDirect3DRMFrame3_SetTraversalOptions(p,a)           (p)->lpVtbl->SetTraversalOptions(p,a)
#define IDirect3DRMFrame3_GetTraversalOptions(p,a)           (p)->lpVtbl->GetTraversalOptions(p,a)
#define IDirect3DRMFrame3_SetSceneFogMethod(p,a)             (p)->lpVtbl->SetSceneFogMethod(p,a)
#define IDirect3DRMFrame3_GetSceneFogMethod(p,a)             (p)->lpVtbl->GetSceneFogMethod(p,a)
#define IDirect3DRMFrame3_SetMaterialOverride(p,a)           (p)->lpVtbl->SetMaterialOverride(p,a)
#define IDirect3DRMFrame3_GetMaterialOverride(p,a)           (p)->lpVtbl->GetMaterialOverride(p,a)
#else
/*** IUnknown methods ***/
#define IDirect3DRMFrame3_QueryInterface(p,a,b)              (p)->QueryInterface(a,b)
#define IDirect3DRMFrame3_AddRef(p)                          (p)->AddRef()
#define IDirect3DRMFrame3_Release(p)                         (p)->Release()
/*** IDirect3DRMObject methods ***/
#define IDirect3DRMFrame3_Clone(p,a,b,c)                     (p)->Clone(a,b,c)
#define IDirect3DRMFrame3_AddDestroyCallback(p,a,b)          (p)->AddDestroyCallback(a,b)
#define IDirect3DRMFrame3_DeleteDestroyCallback(p,a,b)       (p)->DeleteDestroyCallback(a,b)
#define IDirect3DRMFrame3_SetAppData(p,a)                    (p)->SetAppData(a)
#define IDirect3DRMFrame3_GetAppData(p)                      (p)->GetAppData()
#define IDirect3DRMFrame3_SetName(p,a)                       (p)->SetName(a)
#define IDirect3DRMFrame3_GetName(p,a,b)                     (p)->GetName(a,b)
#define IDirect3DRMFrame3_GetClassName(p,a,b)                (p)->GetClassName(a,b)
/*** IDirect3DRMFrame3 methods ***/
#define IDirect3DRMFrame3_AddChild(p,a)                      (p)->AddChild(a)
#define IDirect3DRMFrame3_AddLight(p,a)                      (p)->AddLight(a)
#define IDirect3DRMFrame3_AddMoveCallback(p,a,b,c)           (p)->AddMoveCallback(a,b,c)
#define IDirect3DRMFrame3_AddTransform(p,a,b)                (p)->AddTransform(a,b)
#define IDirect3DRMFrame3_AddTranslation(p,a,b,c,d)          (p)->AddTranslation(a,b,c,d)
#define IDirect3DRMFrame3_AddScale(p,a,b,c,d)                (p)->AddScale(a,b,c,d)
#define IDirect3DRMFrame3_AddRotation(p,a,b,c,d,e)           (p)->AddRotation(a,b,c,d,e)
#define IDirect3DRMFrame3_AddVisual(p,a)                     (p)->AddVisual(a)
#define IDirect3DRMFrame3_GetChildren(p,a)                   (p)->GetChildren(a)
#define IDirect3DRMFrame3_GetColor(p)                        (p)->GetColor()
#define IDirect3DRMFrame3_GetLights(p,a)                     (p)->GetLights(a)
#define IDirect3DRMFrame3_GetMaterialMode(p)                 (p)->GetMaterialMode()
#define IDirect3DRMFrame3_GetParent(p,a)                     (p)->GetParent(a)
#define IDirect3DRMFrame3_GetPosition(p,a,b)                 (p)->GetPosition(a,b)
#define IDirect3DRMFrame3_GetRotation(p,a,b,c)               (p)->GetRotation(a,b,c)
#define IDirect3DRMFrame3_GetScene(p,a)                      (p)->GetScene(a)
#define IDirect3DRMFrame3_GetSortMode(p)                     (p)->GetSortMode()
#define IDirect3DRMFrame3_GetTexture(p,a)                    (p)->GetTexture(a)
#define IDirect3DRMFrame3_GetTransform(p,a,b)                (p)->GetTransform(a,b)
#define IDirect3DRMFrame3_GetVelocity(p,a,b,c)               (p)->GetVelocity(a,b,c)
#define IDirect3DRMFrame3_GetOrientation(p,a,b,c)            (p)->GetOrientation(a,b,c)
#define IDirect3DRMFrame3_GetVisuals(p,a,b)                  (p)->GetVisuals(a,b)
#define IDirect3DRMFrame3_InverseTransform(p,a,b)            (p)->InverseTransform(a,b)
#define IDirect3DRMFrame3_Load(p,a,b,c,d,e)                  (p)->Load(a,b,c,d,e)
#define IDirect3DRMFrame3_LookAt(p,a,b,c)                    (p)->LookAt(a,b,c)
#define IDirect3DRMFrame3_Move(p,a)                          (p)->Move(a)
#define IDirect3DRMFrame3_DeleteChild(p,a)                   (p)->DeleteChild(a)
#define IDirect3DRMFrame3_DeleteLight(p,a)                   (p)->DeleteLight(a)
#define IDirect3DRMFrame3_DeleteMoveCallback(p,a,b)          (p)->DeleteMoveCallback(a,b)
#define IDirect3DRMFrame3_DeleteVisual(p,a)                  (p)->DeleteVisual(a)
#define IDirect3DRMFrame3_GetSceneBackground(p)              (p)->GetSceneBackground()
#define IDirect3DRMFrame3_GetSceneBackgroundDepth(p,a)       (p)->GetSceneBackgroundDepth(a)
#define IDirect3DRMFrame3_GetSceneFogColor(p)                (p)->GetSceneFogColor()
#define IDirect3DRMFrame3_GetSceneFogEnable(p)               (p)->GetSceneFogEnable()
#define IDirect3DRMFrame3_GetSceneFogMode(p)                 (p)->GetSceneFogMode()
#define IDirect3DRMFrame3_GetSceneFogParams(p,a,b,c)         (p)->GetSceneFogParams(a,b,c)
#define IDirect3DRMFrame3_SetSceneBackground(p,a)            (p)->SetSceneBackground(a)
#define IDirect3DRMFrame3_SetSceneBackgroundRGB(p,a,b,c)     (p)->SetSceneBackgroundRGB(a,b,c)
#define IDirect3DRMFrame3_SetSceneBackgroundDepth(p,a)       (p)->SetSceneBackgroundDepth(a)
#define IDirect3DRMFrame3_SetSceneBackgroundImage(p,a)       (p)->SetSceneBackgroundImage(a)
#define IDirect3DRMFrame3_SetSceneFogEnable(p,a)             (p)->SetSceneFogEnable(a)
#define IDirect3DRMFrame3_SetSceneFogColor(p,a)              (p)->SetSceneFogColor(a)
#define IDirect3DRMFrame3_SetSceneFogMode(p,a)               (p)->SetSceneFogMode(a)
#define IDirect3DRMFrame3_SetSceneFogParams(p,a,b,c)         (p)->SetSceneFogParams(a,b,c)
#define IDirect3DRMFrame3_SetColor(p,a)                      (p)->SetColor(a)
#define IDirect3DRMFrame3_SetColorRGB(p,a,b,c)               (p)->SetColorRGB(a,b,c)
#define IDirect3DRMFrame3_GetZbufferMode(p)                  (p)->GetZbufferMode()
#define IDirect3DRMFrame3_SetMaterialMode(p,a)               (p)->SetMaterialMode(a)
#define IDirect3DRMFrame3_SetOrientation(p,a,b,c,d,e,f,g)    (p)->SetOrientation(a,b,c,d,e,f,g)
#define IDirect3DRMFrame3_SetPosition(p,a,b,c,d)             (p)->SetPosition(a,b,c,d)
#define IDirect3DRMFrame3_SetRotation(p,a,b,c,d,e)           (p)->SetRotation(a,b,c,d,e)
#define IDirect3DRMFrame3_SetSortMode(p,a)                   (p)->SetSortMode(a)
#define IDirect3DRMFrame3_SetTexture(p,a)                    (p)->SetTexture(a)
#define IDirect3DRMFrame3_SetVelocity(p,a,b,c,d,e)           (p)->SetVelocity(a,b,c,d,e)
#define IDirect3DRMFrame3_SetZbufferMode(p,a)                (p)->SetZbufferMode(a)
#define IDirect3DRMFrame3_Transform(p,a,b)                   (p)->Transform(a,b)
#define IDirect3DRMFrame3_GetBox(p,a)                        (p)->GetBox(a)
#define IDirect3DRMFrame3_GetBoxEnable(p)                    (p)->GetBoxEnable()
#define IDirect3DRMFrame3_GetAxes(p,a,b)                     (p)->GetAxes(a,b)
#define IDirect3DRMFrame3_GetMaterial(p,a)                   (p)->GetMaterial(a)
#define IDirect3DRMFrame3_GetInheritAxes(p)                  (p)->GetInheritAxes()
#define IDirect3DRMFrame3_GetHierarchyBox(p,a)               (p)->GetHierarchyBox(a)
#define IDirect3DRMFrame3_SetBox(p,a)                        (p)->SetBox(a)
#define IDirect3DRMFrame3_SetBoxEnable(p,a)                  (p)->SetBoxEnable(a)
#define IDirect3DRMFrame3_SetAxes(p,a,b,c,d,e,f)             (p)->SetAxes(a,b,c,d,e,f)
#define IDirect3DRMFrame3_SetInheritAxes(p,a)                (p)->SetInheritAxes(a)
#define IDirect3DRMFrame3_SetMaterial(p,a)                   (p)->SetMaterial(a)
#define IDirect3DRMFrame3_SetQuaternion(p,a,b)               (p)->SetQuaternion(a,b)
#define IDirect3DRMFrame3_RayPick(p,a,b,c,d)                 (p)->RayPick(a,b,c,d)
#define IDirect3DRMFrame3_Save(p,a,b,c)                      (p)->Save(a,b,c)
#define IDirect3DRMFrame3_TransformVectors(p,a,b,c,d)        (p)->TransformVectors(a,b,c,d)
#define IDirect3DRMFrame3_InverseTransformVectors(p,a,b,c,d) (p)->InverseTransformVectors(a,b,c,d)
#define IDirect3DRMFrame3_SetTraversalOptions(p,a)           (p)->SetTraversalOptions(a)
#define IDirect3DRMFrame3_GetTraversalOptions(p,a)           (p)->GetTraversalOptions(a)
#define IDirect3DRMFrame3_SetSceneFogMethod(p,a)             (p)->SetSceneFogMethod(a)
#define IDirect3DRMFrame3_GetSceneFogMethod(p,a)             (p)->GetSceneFogMethod(a)
#define IDirect3DRMFrame3_SetMaterialOverride(p,a)           (p)->SetMaterialOverride(a)
#define IDirect3DRMFrame3_GetMaterialOverride(p,a)           (p)->GetMaterialOverride(a)
#endif

#ifdef __cplusplus
};
#endif

#endif /* __D3DRMOBJ_H__ */
