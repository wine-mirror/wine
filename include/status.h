/*
 * Status window definitions
 *
 * Copyright 1996 Bruce Milner
 */

#ifndef __WINE_STATUS_H
#define __WINE_STATUS_H

#include "windef.h"
#include "wingdi.h"

typedef struct
{
    INT	x;
    INT	style;
    RECT	bound;
    LPWSTR	text;
    HICON     hIcon;
} STATUSWINDOWPART;

typedef struct
{
    UINT16              numParts;
    UINT16              textHeight;
    UINT              height;
    BOOL              simple;
    HWND              hwndToolTip;
    HFONT             hFont;
    HFONT             hDefaultFont;
    COLORREF            clrBk;     /* background color */
    BOOL              bUnicode;  /* unicode flag */
    STATUSWINDOWPART	part0;	   /* simple window */
    STATUSWINDOWPART   *parts;
} STATUSWINDOWINFO;


extern VOID STATUS_Register (VOID);
extern VOID STATUS_Unregister (VOID);

#endif  /* __WINE_STATUS_H */
