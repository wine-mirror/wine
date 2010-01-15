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
typedef void (__cdecl *D3DRMUPDATECALLBACK)(LPDIRECT3DRMDEVICE obj, LPVOID arg, int, LPD3DRECT);
typedef int (__cdecl *D3DRMUSERVISUALCALLBACK)(LPDIRECT3DRMUSERVISUAL obj, LPVOID arg,
    D3DRMUSERVISUALREASON reason, LPDIRECT3DRMDEVICE dev, LPDIRECT3DRMVIEWPORT view);
typedef HRESULT (__cdecl *D3DRMLOADTEXTURECALLBACK)(char *tex_name, void *arg, LPDIRECT3DRMTEXTURE *);
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

#ifdef __cplusplus
};
#endif

#endif /* __D3DRMOBJ_H__ */
