/*
 * Copyright (C) 2008 Vijay Kiran Kamuju
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

#ifdef __cplusplus
};
#endif

#endif /* __D3DRMOBJ_H__ */
