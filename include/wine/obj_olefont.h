/*
 * Defines the COM interfaces and APIs related to OLE font support.
 *
 * Depends on 'obj_base.h'.
 *
 * Copyright (C) 1999 Francis Beaudet
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

#ifndef __WINE_WINE_OBJ_OLEFONT_H
#define __WINE_WINE_OBJ_OLEFONT_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IFont, 0xBEF6E002, 0xA874, 0x101A, 0x8B, 0xBA, 0x00, 0xAA, 0x00, 0x30, 0x0C, 0xAB);
typedef struct IFont IFont,*LPFONT;

DEFINE_GUID(IID_IFontDisp, 0xBEF6E003, 0xA874, 0x101A, 0x8B, 0xBA, 0x00, 0xAA, 0x00, 0x30, 0x0C, 0xAB);
typedef struct IFontDisp IFontDisp,*LPFONTDISP;

typedef TEXTMETRICW TEXTMETRICOLE;

/*****************************************************************************
 * IFont interface
 */
#define INTERFACE IFont
#define IFont_METHODS \
  IUnknown_METHODS \
  STDMETHOD(get_Name)(THIS_ BSTR * pname) PURE; \
  STDMETHOD(put_Name)(THIS_ BSTR  name) PURE; \
  STDMETHOD(get_Size)(THIS_ CY * psize) PURE; \
  STDMETHOD(put_Size)(THIS_ CY  size) PURE; \
  STDMETHOD(get_Bold)(THIS_ BOOL * pbold) PURE; \
  STDMETHOD(put_Bold)(THIS_ BOOL  bold) PURE; \
  STDMETHOD(get_Italic)(THIS_ BOOL * pitalic) PURE; \
  STDMETHOD(put_Italic)(THIS_ BOOL  italic) PURE; \
  STDMETHOD(get_Underline)(THIS_ BOOL * punderline) PURE; \
  STDMETHOD(put_Underline)(THIS_ BOOL  underline) PURE; \
  STDMETHOD(get_Strikethrough)(THIS_ BOOL * pstrikethrough) PURE; \
  STDMETHOD(put_Strikethrough)(THIS_ BOOL  strikethrough) PURE; \
  STDMETHOD(get_Weight)(THIS_ short * pweight) PURE; \
  STDMETHOD(put_Weight)(THIS_ short  weight) PURE; \
  STDMETHOD(get_Charset)(THIS_ short * pcharset) PURE; \
  STDMETHOD(put_Charset)(THIS_ short  charset) PURE; \
  STDMETHOD(get_hFont)(THIS_ HFONT * phfont) PURE; \
  STDMETHOD(Clone)(THIS_ IFont ** ppfont) PURE; \
  STDMETHOD(IsEqual)(THIS_ IFont * pFontOther) PURE; \
  STDMETHOD(SetRatio)(THIS_ long  cyLogical, long  cyHimetric) PURE; \
  STDMETHOD(QueryTextMetrics)(THIS_ TEXTMETRICOLE * ptm) PURE; \
  STDMETHOD(AddRefHfont)(THIS_ HFONT  hfont) PURE; \
  STDMETHOD(ReleaseHfont)(THIS_ HFONT  hfont) PURE; \
  STDMETHOD(SetHdc)(THIS_ HDC  hdc) PURE;
ICOM_DEFINE(IFont,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IFont_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IFont_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IFont_Release(p)             (p)->lpVtbl->Release(p)
/*** IFont methods ***/
#define IFont_getName(p,a)           (p)->lpVtbl->get_Name(p,a)
#define IFont_putName(p,a)           (p)->lpVtbl->put_Name(p,a)
#define IFont_get_Size(p,a)          (p)->lpVtbl->get_Size(p,a)
#define IFont_put_Size(p,a)          (p)->lpVtbl->put_Size(p,a)
#define IFont_get_Bold(p,a)          (p)->lpVtbl->get_Bold(p,a)
#define IFont_put_Bold(p,a)          (p)->lpVtbl->put_Bold(p,a)
#define IFont_get_Italic(p,a)        (p)->lpVtbl->get_Italic(p,a)
#define IFont_put_Italic(p,a)        (p)->lpVtbl->put_Italic(p,a)
#define IFont_get_Underline(p,a)     (p)->lpVtbl->get_Underline(p,a)
#define IFont_put_Underline(p,a)     (p)->lpVtbl->put_Underline(p,a)
#define IFont_get_Strikethrough(p,a) (p)->lpVtbl->get_Strikethrough(p,a)
#define IFont_put_Strikethrough(p,a) (p)->lpVtbl->put_Strikethrough(p,a)
#define IFont_get_Weight(p,a)        (p)->lpVtbl->get_Weight(p,a)
#define IFont_put_Weight(p,a)        (p)->lpVtbl->put_Weight(p,a)
#define IFont_get_Charset(p,a)       (p)->lpVtbl->get_Charset(p,a)
#define IFont_put_Charset(p,a)       (p)->lpVtbl->put_Charset(p,a)
#define IFont_get_hFont(p,a)         (p)->lpVtbl->get_hFont(p,a)
#define IFont_put_hFont(p,a)         (p)->lpVtbl->put_hFont(p,a)
#define IFont_Clone(p,a)             (p)->lpVtbl->Clone(p,a)
#define IFont_IsEqual(p,a)           (p)->lpVtbl->IsEqual(p,a)
#define IFont_SetRatio(p,a,b)        (p)->lpVtbl->SetRatio(p,a,b)
#define IFont_QueryTextMetrics(p,a)  (p)->lpVtbl->QueryTextMetrics(p,a)
#define IFont_AddRefHfont(p,a)       (p)->lpVtbl->AddRefHfont(p,a)
#define IFont_ReleaseHfont(p,a)      (p)->lpVtbl->ReleaseHfont(p,a)
#define IFont_SetHdc(p,a)            (p)->lpVtbl->SetHdc(p,a)
#endif

/*****************************************************************************
 * IFont interface
 */
#define INTERFACE IFontDisp
#define IFontDisp_METHODS \
  IDispatch_METHODS
ICOM_DEFINE(IFontDisp,IDispatch)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IFontDisp_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IFontDisp_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IFontDisp_Release(p)             (p)->lpVtbl->Release(p)
/*** IDispatch methods ***/
#define IFontDisp_GetTypeInfoCount(p,a)      (p)->lpVtbl->GetTypeInfoCount(p,a)
#define IFontDisp_GetTypeInfo(p,a,b,c)       (p)->lpVtbl->GetTypeInfo(p,a,b,c)
#define IFontDisp_GetIDsOfNames(p,a,b,c,d,e) (p)->lpVtbl->GetIDsOfNames(p,a,b,c,d,e)
#define IFontDisp_Invoke(p,a,b,c,d,e,f,g,h)  (p)->lpVtbl->Invoke(p,a,b,c,d,e,f,g,h)
/*** IFontDisp methods ***/
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_OLEFONT_H */
