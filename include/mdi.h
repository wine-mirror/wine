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

extern LRESULT MDIClientWndProc(HWND16 hwnd, UINT16 message, 
				WPARAM16 wParam, LPARAM lParam); /* mdi.c */

typedef struct 
{
    UINT16   	nActiveChildren;
    HWND16   	hwndChildMaximized;
    HWND16   	hwndActiveChild;
    HMENU16  	hWindowMenu;
    UINT32   	idFirstChild;
    LPSTR 	frameTitle;
    UINT16	nTotalCreated;
    UINT16   	mdiFlags;
    UINT16   	sbRecalc;		/* SB_xxx flags for scrollbar fixup */
    HWND16   	self;
} MDICLIENTINFO;

#endif /* __WINE_MDI_H */

