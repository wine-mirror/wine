/*
 * Defines the COM interfaces and APIs related to the moniker functionality.
 *
 * This file depends on 'obj_storage.h' and 'obj_base.h'.
 */


#ifndef __WINE_WINE_OBJ_MONIKER_H
#define __WINE_WINE_OBJ_MONIKER_H


/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IBindCtx,0xe,0,0);
typedef struct IBindCtx IBindCtx,*LPBINDCTX;
typedef LPBINDCTX LPBC;

DEFINE_OLEGUID(IID_IClassActivator,	0x00000140L, 0, 0);
typedef struct IClassActivator IClassActivator,*LPCLASSACTIVATOR;

DEFINE_OLEGUID(IID_IEnumMoniker,	0x00000102L, 0, 0);
typedef struct IEnumMoniker IEnumMoniker,*LPENUMMONIKER;

DEFINE_OLEGUID(IID_IMoniker,		0x0000000fL, 0, 0);
typedef struct IMoniker IMoniker,*LPMONIKER;

DEFINE_GUID   (IID_IROTData,		0xf29f6bc0L, 0x5021, 0x11ce, 0xaa, 0x15, 0x00, 0x00, 0x69, 0x01, 0x29, 0x3f);
typedef struct IROTData IROTData,*LPROTDATA;

DEFINE_OLEGUID(IID_IRunnableObject,	0x00000126L, 0, 0);
typedef struct IRunnableObject IRunnableObject,*LPRUNNABLEOBJECT;

DEFINE_OLEGUID(IID_IRunningObjectTable,	0x00000010L, 0, 0);
typedef struct IRunningObjectTable IRunningObjectTable,*LPRUNNINGOBJECTTABLE;


/*****************************************************************************
 * IBindCtx interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IClassActivator interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IEnumMoniker interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IMoniker interface
 */
/* FIXME: not implemented */

HRESULT WINAPI CreateFileMoniker16(LPCOLESTR16 lpszPathName,LPMONIKER* ppmk);
HRESULT WINAPI CreateFileMoniker32(LPCOLESTR32 lpszPathName,LPMONIKER* ppmk);
#define CreateFileMoniker WINELIB_NAME(CreateFileMoniker)

HRESULT WINAPI CreateItemMoniker32(LPCOLESTR32 lpszDelim,LPCOLESTR32 lpszItem,LPMONIKER* ppmk);
#define CreateItemMoniker WINELIB_NAME(CreateItemMoniker)


/*****************************************************************************
 * IROTData interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IRunnableObject interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IRunningObjectTable interface
 */
/* FIXME: not implemented */


#endif /* __WINE_WINE_OBJ_MONIKER_H */
