/*
 * Status window definitions
 *
 * Copyright 1996 Bruce Milner
 */

#ifndef __WINE_STATUS_H
#define __WINE_STATUS_H

typedef struct
{
    INT32	x;
    INT32	style;
    RECT32	bound;
    LPWSTR	text;
    HICON32     hIcon;
} STATUSWINDOWPART;

typedef struct
{
    UINT16              numParts;
    UINT16              textHeight;
    UINT32              height;
    BOOL32              simple;
    HWND32              hwndToolTip;
    HFONT32             hFont;
    HFONT32             hDefaultFont;
    COLORREF            clrBk;     /* background color */
    BOOL32              bUnicode;  /* unicode flag */
    STATUSWINDOWPART	part0;	   /* simple window */
    STATUSWINDOWPART   *parts;
} STATUSWINDOWINFO;


extern VOID STATUS_Register (VOID);
extern VOID STATUS_Unregister (VOID);

#endif  /* __WINE_STATUS_H */
