/*
 *	ole2.h - Declarations for OLE2
 */

#ifndef __WINE_OLE2_H
#define __WINE_OLE2_H

#include "windef.h"
#include "winerror.h"
#include "oleidl.h"
#include "oleauto.h"

#define OLEIVERB_PRIMARY            (0L)
#define OLEIVERB_SHOW               (-1L)
#define OLEIVERB_OPEN               (-2L)
#define OLEIVERB_HIDE               (-3L)
#define OLEIVERB_UIACTIVATE         (-4L)
#define OLEIVERB_INPLACEACTIVATE    (-5L)
#define OLEIVERB_DISCARDUNDOSTATE   (-6L)
#define OLEIVERB_PROPERTIES         (-7L)

/*
 * API declarations
 */
HRESULT     WINAPI RegisterDragDrop16(HWND16,LPDROPTARGET);
HRESULT     WINAPI RegisterDragDrop(HWND,LPDROPTARGET);
HRESULT     WINAPI RevokeDragDrop16(HWND16);
HRESULT     WINAPI RevokeDragDrop(HWND);
HRESULT     WINAPI DoDragDrop16(LPDATAOBJECT,LPDROPSOURCE,DWORD,DWORD*);
HRESULT     WINAPI DoDragDrop(LPDATAOBJECT,LPDROPSOURCE,DWORD,DWORD*);
HRESULT  WINAPI OleLoadFromStream(IStream *pStm,REFIID iidInterface,void** ppvObj);
HRESULT  WINAPI OleSaveToStream(IPersistStream *pPStm,IStream *pStm);
HOLEMENU WINAPI OleCreateMenuDescriptor(HMENU hmenuCombined,LPOLEMENUGROUPWIDTHS lpMenuWidths);
HRESULT   WINAPI OleDestroyMenuDescriptor(HOLEMENU hmenuDescriptor);
HRESULT  WINAPI OleSetMenuDescriptor(HOLEMENU hmenuDescriptor,HWND hwndFrame,HWND hwndActiveObject,LPOLEINPLACEFRAME lpFrame,LPOLEINPLACEACTIVEOBJECT lpActiveObject);

HRESULT WINAPI ReadClassStg(IStorage *pstg,CLSID *pclsid);
HRESULT WINAPI WriteClassStm(IStream *pStm,REFCLSID rclsid);
HRESULT WINAPI ReadClassStm(IStream *pStm,REFCLSID pclsid);

#endif  /* __WINE_OLE2_H */

