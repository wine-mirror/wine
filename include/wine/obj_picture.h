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

/*** IUnknown methods ***/
#define IPicture_QueryInterface(p,a,b)         ICOM_CALL2(QueryInterface,p,a,b)
#define IPicture_AddRef(p)                     ICOM_CALL (AddRef,p)
#define IPicture_Release(p)                    ICOM_CALL (Release,p)
/*** IPicture methods ***/
#define IPicture_get_Handle(p,a)               ICOM_CALL1(get_Handle,p,a)
#define IPicture_get_hPal(p,a)                 ICOM_CALL1(get_hPal,p,a)
#define IPicture_get_Type(p,a)                 ICOM_CALL1(get_Type,p,a)
#define IPicture_get_Width(p,a)                ICOM_CALL1(get_Width,p,a)
#define IPicture_get_Height(p,a)               ICOM_CALL1(get_Height,p,a)
#define IPicture_Render(p,a,b,c,d,e,f,g,h,i,j) ICOM_CALL10(Render,p,a,b,c,d,e,f,g,h,i,j)
#define IPicture_set_hPal(p,a)                 ICOM_CALL1(set_hPal,p,a)
#define IPicture_get_CurDC(p,a)                ICOM_CALL1(get_CurDC,p,a)
#define IPicture_SelectPicture(p,a,b,c)        ICOM_CALL3(SelectPicture,p,a,b,c)
#define IPicture_get_KeepOriginalFormat(p,a)   ICOM_CALL1(get_KeepOriginalFormat,p,a)
#define IPicture_put_KeepOriginalFormat(p,a)   ICOM_CALL1(put_KeepOriginalFormat,p,a)
#define IPicture_PictureChanged(p)             ICOM_CALL (PictureChanged,p)
#define IPicture_SaveAsFile(p,a,b,c)           ICOM_CALL3(SaveAsFile,p,a,b,c)
#define IPicture_get_Attributes(p,a)           ICOM_CALL1(get_Attributes,p,a)


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

/*** IUnknown methods ***/
#define IPictureDisp_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IPictureDisp_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IPictureDisp_Release(p)                 ICOM_CALL (Release,p)
/*** IDispatch methods ***/
#define IPictureDisp_GetTypeInfoCount(p,a)      ICOM_CALL1 (GetTypeInfoCount,p,a)
#define IPictureDisp_GetTypeInfo(p,a,b,c)       ICOM_CALL3 (GetTypeInfo,p,b,c)
#define IPictureDisp_GetIDsOfNames(p,a,b,c,d,e) ICOM_CALL5 (GetIDsOfNames,p,a,b,c,d,e)
#define IPictureDisp_Invoke(p,a,b,c,d,e,f,g,h)  ICOM_CALL8 (Invoke,p,a,b,c,d,e,f,g,h)
/*** IPictureDisp methods ***/

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_PICTURE_H */
