/*
 *	ole2.h - Declarations for OLE2
 */

#ifndef __WINE_OLE2_H
#define __WINE_OLE2_H

#include "wintypes.h"
#include "winerror.h"
#include "oleidl.h"

/* OLE version */
#define rmm             23
#define rup            639

/*
 * API declarations
 */
HRESULT     WINAPI RegisterDragDrop16(HWND16,LPDROPTARGET);
HRESULT     WINAPI RegisterDragDrop(HWND,LPDROPTARGET);
HRESULT     WINAPI RevokeDragDrop16(HWND16);
HRESULT     WINAPI RevokeDragDrop(HWND);
HRESULT     WINAPI DoDragDrop16(LPDATAOBJECT,LPDROPSOURCE,DWORD,DWORD*);
HRESULT     WINAPI DoDragDrop(LPDATAOBJECT,LPDROPSOURCE,DWORD,DWORD*);

HOLEMENU  WINAPI OleCreateMenuDescriptor(HMENU              hmenuCombined,
					   LPOLEMENUGROUPWIDTHS lpMenuWidths);
void        WINAPI OleDestroyMenuDescriptor(HOLEMENU hmenuDescriptor);
HRESULT     WINAPI OleSetMenuDescriptor(HOLEMENU               hmenuDescriptor,
					HWND                   hwndFrame,
					HWND                   hwndActiveObject,
					LPOLEINPLACEFRAME        lpFrame,
					LPOLEINPLACEACTIVEOBJECT lpActiveObject);


#endif  /* __WINE_OLE2_H */

