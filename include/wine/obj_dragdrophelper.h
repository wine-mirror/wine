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


#define ICOM_INTERFACE IDragSourceHelper
#define IDragSourceHelper_METHODS \
    ICOM_METHOD2(HRESULT, InitializeFromBitmap, LPSHDRAGIMAGE, pshdi, IDataObject*, pDataObject) \
    ICOM_METHOD3(HRESULT, InitializeFromWindow, HWND, hwnd, POINT*, ppt, IDataObject*, pDataObject)
#define IDragSourceHelper_IMETHODS \
	IUnknown_IMETHODS \
	IDragSourceHelper_METHODS
ICOM_DEFINE(IDragSourceHelper,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IDragSourceHelper_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDragSourceHelper_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDragSourceHelper_Release(p)            ICOM_CALL (Release,p)
/*** IDropSource methods ***/
#define IDragSourceHelper_InitializeFromBitmap(p,a,b)   ICOM_CALL2(InitializeFromBitmap,p,a,b)
#define IDragSourceHelper_InitializeFromWindow(p,a,b,c) ICOM_CALL1(InitializeFromWindow,p,a,b,c)


/*****************************************************************************
 * IDropTargetHelper interface
 */
#define ICOM_INTERFACE IDropTargetHelper
#define IDropTargetHelper_METHODS \
    ICOM_METHOD4(HRESULT,DragEnter, HWND, hwndTarget, IDataObject*, pDataObject, POINT*, ppt, DWORD, dwEffect) \
    ICOM_METHOD (HRESULT,DragLeave) \
    ICOM_METHOD2(HRESULT,DragOver, POINT*, ppt, DWORD, dwEffect) \
    ICOM_METHOD3(HRESULT,Drop, IDataObject*, pDataObject, POINT*, ppt,DWORD, dwEffect) \
    ICOM_METHOD1(HRESULT,Show, BOOL, fShow)
#define IDropTargetHelper_IMETHODS \
	IUnknown_IMETHODS \
	IDropTargetHelper_METHODS
ICOM_DEFINE(IDropTargetHelper,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IDropTargetHelper_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDropTargetHelper_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDropTargetHelper_Release(p)            ICOM_CALL (Release,p)
/*** IDropTargetHelper methods ***/
#define IDropTargetHelper_DragEnter(p,a,b,c,d)  ICOM_CALL4(DragEnter,p,a,b,c,d)
#define IDropTargetHelper_DragLeave(p)          ICOM_CALL (DragLeave,p)
#define IDropTargetHelper_DragOver(p,a,b)       ICOM_CALL2(DragOver,p,a,b)
#define IDropTargetHelper_Drop(p,a,b,c)         ICOM_CALL3(Drop,p,a,b,c)
#define IDropTargetHelper_Show(p,a)             ICOM_CALL4(Show,p,a,b,c,d)


#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /*  __WINE_WINE_OBJ_DRAGDROPHELPER_H */
