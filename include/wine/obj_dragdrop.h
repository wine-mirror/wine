/*
 * Defines the COM interfaces and APIs related to OLE Drag and Drop.
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_DRAGDROP_H
#define __WINE_WINE_OBJ_DRAGDROP_H

#include "winnt.h"
#include "windef.h"

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
#define	DROPEFFECT_NONE		0
#define	DROPEFFECT_COPY		1
#define	DROPEFFECT_MOVE		2
#define	DROPEFFECT_LINK		4
#define	DROPEFFECT_SCROLL       0x80000000

/*****************************************************************************
 * IDropSource interface
 */
#define ICOM_INTERFACE IDropSource
ICOM_BEGIN(IDropSource,IUnknown)
    ICOM_METHOD2(HRESULT, QueryContinueDrag, BOOL32, fEscapePressed, DWORD, grfKeyState);
    ICOM_METHOD1(HRESULT, GiveFeedback, DWORD, dwEffect);
ICOM_END(IDropSource)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDropSource_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IDropSource_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IDropSource_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** IDropTarget methods ***/
#define IDropSource_QueryContinueDrag(p,a,b) ICOM_CALL2(QueryContinueDrag,p,a,b)
#define IDropSource_GiveFeedback(p,a)        ICOM_CALL1(GiveFeedback,p,a)
#endif

/*****************************************************************************
 * IDropTarget interface
 */
#define ICOM_INTERFACE IDropTarget
ICOM_BEGIN(IDropTarget,IUnknown)
    ICOM_METHOD4(HRESULT, DragEnter, IDataObject*, pDataObjhect, DWORD, grfKeyState, POINTL, pt, DWORD*, pdwEffect);
    ICOM_METHOD3(HRESULT, DragOver, DWORD, grfKeyState, POINTL, pt, DWORD*, pdwEffect);
    ICOM_METHOD(HRESULT, DragLeave);
    ICOM_METHOD4(HRESULT, Drop, IDataObject*, pDataObjhect, DWORD, grfKeyState, POINTL, pt, DWORD*, pdwEffect);
ICOM_END(IDropTarget)
#undef ICOM_INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDropTarget_QueryInterface(p,a,b) ICOM_ICALL2(IUnknown,QueryInterface,p,a,b)
#define IDropTarget_AddRef(p)             ICOM_ICALL (IUnknown,AddRef,p)
#define IDropTarget_Release(p)            ICOM_ICALL (IUnknown,Release,p)
/*** IDropTarget methods ***/
#define IDropTarget_DragEnter(p,a,b,c,d)  ICOM_CALL4(DragEnter,p,a,b,c,d)
#define IDropTarget_DragOver(p,a,b,c)     ICOM_CALL3(DragOver,p,a,b,c)
#define IDropTarget_DragLeave(p)          ICOM_CALL(DragLeave,p)
#define IDropTarget_Drop(p,a,b,c,d)       ICOM_CALL4(Drop,p,a,b,c,d)
#endif

#endif /*  __WINE_WINE_OBJ_DRAGDROP_H */





