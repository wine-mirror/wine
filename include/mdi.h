/* MDI.H
 *
 * Copyright 1994, Bob Amstadt
 *           1995  Alex Korobka
 *
 * MDI structure definitions.
 */

#ifndef MDI_H
#define MDI_H

#include "windows.h"

#define MDI_MAXLISTLENGTH	0x40
#define MDI_MAXTITLELENGTH	0xA1

#define MDI_NOFRAMEREPAINT	0
#define MDI_REPAINTFRAMENOW	1
#define MDI_REPAINTFRAME	2

#define WM_MDICALCCHILDSCROLL   0x10AC /* this is exactly what Windows uses */

extern LRESULT MDIClientWndProc(HWND hwnd, UINT message, 
				WPARAM wParam, LPARAM lParam); /* mdi.c */

typedef struct tagMDIWCL
{
  HWND		 	 hChild;
  struct tagMDIWCL	*prev;
} MDIWCL;

typedef struct 
{
    WORD   	nActiveChildren;
    HWND   	hwndChildMaximized;
    HWND   	hwndActiveChild;
    HMENU  	hWindowMenu;
    WORD   	idFirstChild;
    WORD	nTotalCreated;
    HANDLE 	hFrameTitle;
    WORD   	sbNeedUpdate;
    WORD   	sbRecalc;
    HBITMAP 	obmClose;
    HBITMAP 	obmRestore;
    HWND   	self;
} MDICLIENTINFO;

#endif /* MDI_H */

