/*
 * Defines the COM interfaces related to SHELL DragDropHelper
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

#ifndef __WINE_WINE_OBJ_DRAGDROPHELPER_H
#define __WINE_WINE_OBJ_DRAGDROPHELPER_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(CLSID_DragDropHelper, 0x4657278a, 0x411b, 0x11d2, 0x83, 0x9a, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0xd0);

DEFINE_GUID(IID_IDropTargetHelper, 0x4657278b, 0x411b, 0x11d2, 0x83, 0x9a, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0xd0);
typedef struct IDropTargetHelper IDropTargetHelper,*LPDROPTARGETHELPER;

DEFINE_GUID(IID_IDragSourceHelper,  0xde5bf786, 0x477a, 0x11d2, 0x83, 0x9d, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0xd0);
typedef struct IDragSourceHelper IDragSourceHelper,*LPDRAGSOURCEHELPER;

/*****************************************************************************
 * IDragSourceHelper interface
 */

typedef struct {
    SIZE        sizeDragImage;
    POINT       ptOffset;
    HBITMAP     hbmpDragImage;
    COLORREF    crColorKey;
} SHDRAGIMAGE, *LPSHDRAGIMAGE;


#define INTERFACE IDragSourceHelper
#define IDragSourceHelper_METHODS \
    STDMETHOD(InitializeFromBitmap)(THIS_ LPSHDRAGIMAGE  pshdi, IDataObject * pDataObject) PURE; \
    STDMETHOD(InitializeFromWindow)(THIS_ HWND  hwnd, POINT * ppt, IDataObject * pDataObject) PURE;
#define IDragSourceHelper_IMETHODS \
	IUnknown_IMETHODS \
	IDragSourceHelper_METHODS
ICOM_DEFINE(IDragSourceHelper,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDragSourceHelper_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IDragSourceHelper_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IDragSourceHelper_Release(p)            (p)->lpVtbl->Release(p)
/*** IDropSource methods ***/
#define IDragSourceHelper_InitializeFromBitmap(p,a,b)   (p)->lpVtbl->InitializeFromBitmap(p,a,b)
#define IDragSourceHelper_InitializeFromWindow(p,a,b,c) (p)->lpVtbl->InitializeFromWindow(p,a,b,c)
#endif


/*****************************************************************************
 * IDropTargetHelper interface
 */
#define INTERFACE IDropTargetHelper
#define IDropTargetHelper_METHODS \
    STDMETHOD(DragEnter)(THIS_ HWND  hwndTarget, IDataObject * pDataObject, POINT * ppt, DWORD  dwEffect) PURE; \
    STDMETHOD(DragLeave)(THIS) PURE; \
    STDMETHOD(DragOver)(THIS_ POINT * ppt, DWORD  dwEffect) PURE; \
    STDMETHOD(Drop)(THIS_ IDataObject * pDataObject, POINT * ppt,DWORD  dwEffect) PURE; \
    STDMETHOD(Show)(THIS_ BOOL  fShow) PURE;
#define IDropTargetHelper_IMETHODS \
	IUnknown_IMETHODS \
	IDropTargetHelper_METHODS
ICOM_DEFINE(IDropTargetHelper,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDropTargetHelper_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IDropTargetHelper_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IDropTargetHelper_Release(p)            (p)->lpVtbl->Release(p)
/*** IDropTargetHelper methods ***/
#define IDropTargetHelper_DragEnter(p,a,b,c,d)  (p)->lpVtbl->DragEnter(p,a,b,c,d)
#define IDropTargetHelper_DragLeave(p)          (p)->lpVtbl->DragLeave(p)
#define IDropTargetHelper_DragOver(p,a,b)       (p)->lpVtbl->DragOver(p,a,b)
#define IDropTargetHelper_Drop(p,a,b,c)         (p)->lpVtbl->Drop(p,a,b,c)
#define IDropTargetHelper_Show(p,a)             (p)->lpVtbl->Show(p,a,b,c,d)
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /*  __WINE_WINE_OBJ_DRAGDROPHELPER_H */
