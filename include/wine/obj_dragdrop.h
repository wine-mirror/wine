/*
 * Defines the COM interfaces and APIs related to OLE Drag and Drop.
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

#ifndef __WINE_WINE_OBJ_DRAGDROP_H
#define __WINE_WINE_OBJ_DRAGDROP_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IDropSource,	0x00000121L, 0, 0);
typedef struct IDropSource IDropSource,*LPDROPSOURCE;

DEFINE_OLEGUID(IID_IDropTarget,	0x00000122L, 0, 0);
typedef struct IDropTarget IDropTarget,*LPDROPTARGET;

/*****************************************************************************
 * DROPEFFECT enumeration
 */
#define MK_ALT (0x20)
#define	DROPEFFECT_NONE		0
#define	DROPEFFECT_COPY		1
#define	DROPEFFECT_MOVE		2
#define	DROPEFFECT_LINK		4
#define	DROPEFFECT_SCROLL	0x80000000
#define DD_DEFSCROLLINSET 11
#define DD_DEFSCROLLDELAY 50
#define DD_DEFSCROLLINTERVAL 50
#define DD_DEFDRAGDELAY   50
#define DD_DEFDRAGMINDIST  2

/*****************************************************************************
 * IDropSource interface
 */
#define INTERFACE IDropSource
#define IDropSource_METHODS \
    STDMETHOD(QueryContinueDrag)(THIS_ BOOL  fEscapePressed, DWORD  grfKeyState) PURE; \
    STDMETHOD(GiveFeedback)(THIS_ DWORD  dwEffect) PURE;
#define IDropSource_IMETHODS \
	IUnknown_IMETHODS \
	IDropSource_METHODS
ICOM_DEFINE(IDropSource,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDropSource_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IDropSource_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IDropSource_Release(p)            (p)->lpVtbl->Release(p)
/*** IDropSource methods ***/
#define IDropSource_QueryContinueDrag(p,a,b) (p)->lpVtbl->QueryContinueDrag(p,a,b)
#define IDropSource_GiveFeedback(p,a)        (p)->lpVtbl->GiveFeedback(p,a)
#endif

/*****************************************************************************
 * IDropTarget interface
 */
#define INTERFACE IDropTarget
#define IDropTarget_METHODS \
    STDMETHOD(DragEnter)(THIS_ IDataObject * pDataObject, DWORD  grfKeyState, POINTL  pt, DWORD * pdwEffect) PURE; \
    STDMETHOD(DragOver)(THIS_ DWORD  grfKeyState, POINTL  pt, DWORD * pdwEffect) PURE; \
    STDMETHOD(DragLeave)(THIS) PURE; \
    STDMETHOD(Drop)(THIS_ IDataObject * pDataObject, DWORD  grfKeyState, POINTL  pt, DWORD * pdwEffect) PURE;
#define IDropTarget_IMETHODS \
	IUnknown_IMETHODS \
	IDropTarget_METHODS
ICOM_DEFINE(IDropTarget,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IDropTarget_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IDropTarget_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IDropTarget_Release(p)            (p)->lpVtbl->Release(p)
/*** IDropTarget methods ***/
#define IDropTarget_DragEnter(p,a,b,c,d)  (p)->lpVtbl->DragEnter(p,a,b,c,d)
#define IDropTarget_DragOver(p,a,b,c)     (p)->lpVtbl->DragOver(p,a,b,c)
#define IDropTarget_DragLeave(p)          (p)->lpVtbl->DragLeave(p)
#define IDropTarget_Drop(p,a,b,c,d)       (p)->lpVtbl->Drop(p,a,b,c,d)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /*  __WINE_WINE_OBJ_DRAGDROP_H */
