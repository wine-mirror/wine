/*
 *	ole2.h - Declarations for OLE2
 */

#ifndef __WINE_OLE2_H
#define __WINE_OLE2_H

#include "oleidl.h"
#include "wintypes.h"

/* OLE version */
#define rmm             23
#define rup            639

/* FIXME: should be in oleidl.h */
typedef struct  tagOleMenuGroupWidths
{ LONG width[ 6 ];
} OLEMENUGROUPWIDTHS32, OLEMENUGROUPWIDTHS;



typedef struct tagOleMenuGroupWidths *LPOLEMENUGROUPWIDTHS32;

typedef HGLOBAL32 HOLEMENU32;

/*
 * API declarations
 */
HRESULT     WINAPI RegisterDragDrop16(HWND16,LPDROPTARGET);
HRESULT     WINAPI RegisterDragDrop32(HWND32,LPDROPTARGET);
#define     RegisterDragDrop WINELIB_NAME(RegisterDragDrop)
HRESULT     WINAPI RevokeDragDrop16(HWND16);
HRESULT     WINAPI RevokeDragDrop32(HWND32);
#define     RevokeDragDrop WINELIB_NAME(RevokeDragDrop)
HRESULT     WINAPI DoDragDrop16(LPDATAOBJECT, 
                                LPDROPSOURCE,
				DWORD,
				DWORD*);
HRESULT     WINAPI DoDragDrop32(LPDATAOBJECT,
				LPDROPSOURCE,
				DWORD,
				DWORD*);
#define     DoDragDrop WINELIB_NAME(DoDragDrop)

#endif  /* __WINE_OLE2_H */
