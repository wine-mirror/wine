/* MDI.H
 *
 * Copyright 1994, Bob Amstadt
 *           1995  Alex Korobka
 *
 * MDI structure definitions.
 */

#ifndef __WINE_MDI_H
#define __WINE_MDI_H

#include "windows.h"

#define MDI_MAXLISTLENGTH	0x40
#define MDI_MAXTITLELENGTH	0xA1

#define MDI_NOFRAMEREPAINT	0
#define MDI_REPAINTFRAMENOW	1
#define MDI_REPAINTFRAME	2

#define WM_MDICALCCHILDSCROLL   0x10AC /* this is exactly what Windows uses */

extern LRESULT WINAPI MDIClientWndProc( HWND32 hwnd, UINT32 message, 
                                        WPARAM32 wParam, LPARAM lParam );

typedef struct 
{
    UINT32      nActiveChildren;
    HWND32      hwndChildMaximized;
    HWND32      hwndActiveChild;
    HMENU32     hWindowMenu;
    UINT32      idFirstChild;
    LPSTR       frameTitle;
    UINT32      nTotalCreated;
    UINT32      mdiFlags;
    UINT32      sbRecalc;   /* SB_xxx flags for scrollbar fixup */
    HWND32      self;
} MDICLIENTINFO;

#endif /* __WINE_MDI_H */

