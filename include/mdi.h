/* MDI.H
 *
 * Copyright 1994, Bob Amstadt
 *
 * MDI structure definitions.
 */

#ifndef MDI_H
#define MDI_H

#include "windows.h"

#define MDI_MAXLISTLENGTH	64
extern LRESULT MDIClientWndProc(HWND hwnd, UINT message, 
				WPARAM wParam, LPARAM lParam); /* mdi.c */


typedef struct
{
    HLOCAL next, prev;
    HWND hwnd;
} MDICHILDINFO;

typedef struct 
{
    HMENU  hWindowMenu;
    HLOCAL infoActiveChildren;
    WORD   nActiveChildren;
    WORD   idFirstChild;
    HWND   hwndActiveChild;
    HWND   hwndHitTest;
    BOOL   flagMenuAltered;
    BOOL   flagChildMaximized;
    RECT   rectMaximize;
    RECT   rectRestore;
} MDICLIENTINFO;

#endif /* MDI_H */
