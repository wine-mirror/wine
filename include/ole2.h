/*
 *	ole2.h - Declarations for OLE2
 */

#ifndef __WINE_OLE2_H
#define __WINE_OLE2_H

#include "wintypes.h"

/* to be implemented */
/* FIXME: this should be defined somewhere in oleidl.h instead, should it be repeated here ? */
typedef LPVOID LPDROPTARGET;


/* OLE version */
#define rmm             23
#define rup            639

/* FIXME: should be in oleidl.h */
typedef struct  tagOleMenuGroupWidths
{ LONG width[ 6 ];
} OLEMENUGROUPWIDTHS32, OLEMENUGROUPWIDTHS;



typedef struct tagOleMenuGroupWidths *LPOLEMENUGROUPWIDTHS32;

typedef HGLOBAL32 HOLEMENU32;

HRESULT     WINAPI RegisterDragDrop16(HWND16,LPVOID);
HRESULT     WINAPI RegisterDragDrop32(HWND32,LPVOID);
#define     RegisterDragDrop WINELIB_NAME(RegisterDragDrop)
HRESULT     WINAPI RevokeDragDrop16(HWND16);
HRESULT     WINAPI RevokeDragDrop32(HWND32);
#define     RevokeDragDrop WINELIB_NAME(RevokeDragDrop)

#endif  /* __WINE_OLE2_H */
