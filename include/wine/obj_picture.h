/*
 * Defines the COM interfaces and APIs related to OLE picture support.
 *
 * Depends on 'obj_base.h'.
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

#ifndef __WINE_WINE_OBJ_PICTURE_H
#define __WINE_WINE_OBJ_PICTURE_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the structures
 */
typedef UINT OLE_HANDLE;
typedef LONG OLE_XPOS_HIMETRIC;
typedef LONG OLE_YPOS_HIMETRIC;
typedef LONG OLE_XSIZE_HIMETRIC;
typedef LONG OLE_YSIZE_HIMETRIC;

typedef enum tagPicture { /* bitmasks */
    PICTURE_SCALABLE	= 0x1,
    PICTURE_TRANSPARENT	= 0x2
} PICTUREATTRIBUTES;

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IPicture, 0x7bf80980, 0xbf32, 0x101a, 0x8b, 0xbb, 0x00, 0xAA, 0x00, 0x30, 0x0C, 0xAB);
typedef struct IPicture IPicture, *LPPICTURE;

DEFINE_GUID(IID_IPictureDisp, 0x7bf80981, 0xbf32, 0x101a, 0x8b, 0xbb, 0x00, 0xAA, 0x00, 0x30, 0x0C, 0xAB);
typedef struct IPictureDisp IPictureDisp, *LPPICTUREDISP;

/*****************************************************************************
 * IPicture interface
 */
#define INTERFACE IPicture
#define IPicture_METHODS \
  STDMETHOD(get_Handle)(THIS_ OLE_HANDLE *pHandle) PURE; \
  STDMETHOD(get_hPal)(THIS_ OLE_HANDLE *phPal) PURE; \
  STDMETHOD(get_Type)(THIS_ SHORT *pType) PURE; \
  STDMETHOD(get_Width)(THIS_ OLE_XSIZE_HIMETRIC *pWidth) PURE; \
  STDMETHOD(get_Height)(THIS_ OLE_YSIZE_HIMETRIC *pHeight) PURE; \
  STDMETHOD(Render)(THIS_ HDC hdc, LONG x, LONG y, LONG cx, LONG cy, OLE_XPOS_HIMETRIC xSrc, OLE_YPOS_HIMETRIC ySrc, OLE_XSIZE_HIMETRIC cxSrc, OLE_YSIZE_HIMETRIC cySrc, LPCRECT pRcWBounds) PURE; \
  STDMETHOD(set_hPal)(THIS_ OLE_HANDLE hPal) PURE; \
  STDMETHOD(get_CurDC)(THIS_ HDC *phDC) PURE; \
  STDMETHOD(SelectPicture)(THIS_ HDC hDCIn, HDC *phDCOut, OLE_HANDLE *phBmpOut) PURE; \
  STDMETHOD(get_KeepOriginalFormat)(THIS_ BOOL *pKeep) PURE; \
  STDMETHOD(put_KeepOriginalFormat)(THIS_ BOOL Keep) PURE; \
  STDMETHOD(PictureChanged)(THIS) PURE; \
  STDMETHOD(SaveAsFile)(THIS_ LPSTREAM pStream, BOOL fSaveMemCopy, LONG *pCbSize) PURE; \
  STDMETHOD(get_Attributes)(THIS_ DWORD *pDwAttr) PURE;
#define IPicture_IMETHODS \
	IUnknown_IMETHODS \
	IPicture_METHODS
ICOM_DEFINE(IPicture,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IPicture_QueryInterface(p,a,b)         (p)->lpVtbl->QueryInterface(p,a,b)
#define IPicture_AddRef(p)                     (p)->lpVtbl->AddRef(p)
#define IPicture_Release(p)                    (p)->lpVtbl->Release(p)
/*** IPicture methods ***/
#define IPicture_get_Handle(p,a)               (p)->lpVtbl->get_Handle(p,a)
#define IPicture_get_hPal(p,a)                 (p)->lpVtbl->get_hPal(p,a)
#define IPicture_get_Type(p,a)                 (p)->lpVtbl->get_Type(p,a)
#define IPicture_get_Width(p,a)                (p)->lpVtbl->get_Width(p,a)
#define IPicture_get_Height(p,a)               (p)->lpVtbl->get_Height(p,a)
#define IPicture_Render(p,a,b,c,d,e,f,g,h,i,j) (p)->lpVtbl->Render(p,a,b,c,d,e,f,g,h,i,j)
#define IPicture_set_hPal(p,a)                 (p)->lpVtbl->set_hPal(p,a)
#define IPicture_get_CurDC(p,a)                (p)->lpVtbl->get_CurDC(p,a)
#define IPicture_SelectPicture(p,a,b,c)        (p)->lpVtbl->SelectPicture(p,a,b,c)
#define IPicture_get_KeepOriginalFormat(p,a)   (p)->lpVtbl->get_KeepOriginalFormat(p,a)
#define IPicture_put_KeepOriginalFormat(p,a)   (p)->lpVtbl->put_KeepOriginalFormat(p,a)
#define IPicture_PictureChanged(p)             (p)->lpVtbl->PictureChanged(p)
#define IPicture_SaveAsFile(p,a,b,c)           (p)->lpVtbl->SaveAsFile(p,a,b,c)
#define IPicture_get_Attributes(p,a)           (p)->lpVtbl->get_Attributes(p,a)
#endif


/*****************************************************************************
 * IPictureDisp interface
 */
#define INTERFACE IPictureDisp
#define IPictureDisp_METHODS
#define IPictureDisp_IMETHODS \
				IDispatch_IMETHODS \
				IPictureDisp_METHODS
ICOM_DEFINE(IPictureDisp,IDispatch)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IPictureDisp_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IPictureDisp_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IPictureDisp_Release(p)                 (p)->lpVtbl->Release(p)
/*** IDispatch methods ***/
#define IPictureDisp_GetTypeInfoCount(p,a)      (p)->lpVtbl->GetTypeInfoCount(p,a)
#define IPictureDisp_GetTypeInfo(p,a,b,c)       (p)->lpVtbl->GetTypeInfo(p,b,c)
#define IPictureDisp_GetIDsOfNames(p,a,b,c,d,e) (p)->lpVtbl->GetIDsOfNames(p,a,b,c,d,e)
#define IPictureDisp_Invoke(p,a,b,c,d,e,f,g,h)  (p)->lpVtbl->Invoke(p,a,b,c,d,e,f,g,h)
/*** IPictureDisp methods ***/
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_PICTURE_H */
