/*
 * Defines the COM interfaces and APIs related to ViewObject
 *
 * Copyright (C) 1999 Paul Quinn
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINE_OBJ_OLEVIEW_H
#define __WINE_WINE_OBJ_OLEVIEW_H

struct tagLOGPALETTE;

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Declare the structures
 */


/*****************************************************************************
 * Predeclare the interfaces
 */

DEFINE_OLEGUID(IID_IViewObject,  0x0000010dL, 0, 0);
typedef struct IViewObject IViewObject, *LPVIEWOBJECT;

DEFINE_OLEGUID(IID_IViewObject2,  0x00000127L, 0, 0);
typedef struct IViewObject2 IViewObject2, *LPVIEWOBJECT2;

/*****************************************************************************
 * IViewObject interface
 */
typedef BOOL    (CALLBACK *IVO_ContCallback)(DWORD);

#define INTERFACE IViewObject
#define IViewObject_METHODS \
	STDMETHOD(Draw)(THIS_ DWORD dwDrawAspect, LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw, LPCRECTL lprcBounds, LPCRECTL lprcWBounds, IVO_ContCallback  pfnContinue, DWORD dwContinue) PURE; \
	STDMETHOD(GetColorSet)(THIS_ DWORD dwDrawAspect, LONG lindex, void *pvAspect, DVTARGETDEVICE *ptd, HDC hicTargetDevice, struct tagLOGPALETTE **ppColorSet) PURE; \
	STDMETHOD(Freeze)(THIS_ DWORD dwDrawAspect, LONG lindex, void *pvAspect, DWORD *pdwFreeze) PURE; \
	STDMETHOD(Unfreeze)(THIS_ DWORD dwFreeze) PURE; \
	STDMETHOD(SetAdvise)(THIS_ DWORD aspects, DWORD advf, IAdviseSink *pAdvSink) PURE; \
	STDMETHOD(GetAdvise)(THIS_ DWORD *pAspects, DWORD *pAdvf, IAdviseSink **ppAdvSink) PURE;
#define IViewObject_IMETHODS \
	IUnknown_IMETHODS \
	IViewObject_METHODS
ICOM_DEFINE(IViewObject,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IViewObject_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IViewObject_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IViewObject_Release(p)                   (p)->lpVtbl->Release(p)
/*** IViewObject methods ***/
#define IViewObject_Draw(p,a,b,c,d,e,f,g,h,i,j)  (p)->lpVtbl->Draw(p,a,b,c,d,e,f,g,h,i,j)
#define IViewObject_GetColorSet(p,a,b,c,d,e,f)   (p)->lpVtbl->GetColorSet(p,a,b,c,d,e,f)
#define IViewObject_Freeze(p,a,b,c,d)            (p)->lpVtbl->Freeze(p,a,b,c,d)
#define IViewObject_Unfreeze(p,a)                (p)->lpVtbl->Unfreeze(p,a)
#define IViewObject_SetAdvise(p,a,b,c)           (p)->lpVtbl->SetAdvise(p,a,b,c)
#define IViewObject_GetAdvise(p,a,b,c)           (p)->lpVtbl->GetAdvise(p,a,b,c)
#endif



/*****************************************************************************
 * IViewObject2 interface
 */
#define INTERFACE IViewObject2
#define IViewObject2_METHODS \
	STDMETHOD(GetExtent)(THIS_ DWORD dwDrawAspect, LONG lindex, DVTARGETDEVICE *ptd, LPSIZEL lpsizel) PURE;
#define IViewObject2_IMETHODS \
	IViewObject_IMETHODS \
	IViewObject2_METHODS
ICOM_DEFINE(IViewObject2,IViewObject)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IViewObject2_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IViewObject2_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IViewObject2_Release(p)                   (p)->lpVtbl->Release(p)
/*** IViewObject methods ***/
#define IViewObject2_Draw(p,a,b,c,d,e,f,g,h,i,j)  (p)->lpVtbl->Draw(p,a,b,c,d,e,f,g,h,i,j)
#define IViewObject2_GetColorSet(p,a,b,c,d,e,f)   (p)->lpVtbl->GetColorSet(p,a,b,c,d,e,f)
#define IViewObject2_Freeze(p,a,b,c,d)            (p)->lpVtbl->Freeze(p,a,b,c,d)
#define IViewObject2_Unfreeze(p,a)                (p)->lpVtbl->Unfreeze(p,a)
#define IViewObject2_SetAdvise(p,a,b,c)           (p)->lpVtbl->SetAdvise(p,a,b,c)
#define IViewObject2_GetAdvise(p,a,b,c)           (p)->lpVtbl->GetAdvise(p,a,b,c)
/*** IViewObject2 methods ***/
#define IViewObject2_GetExtent(p,a,b,c,d)         (p)->lpVtbl->GetExtent(p,a,b,c,d)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_OLEVIEW_H */
