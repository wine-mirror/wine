/*
 * Toolbar class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_TOOLBAR_H
#define __WINE_TOOLBAR_H

#include "commctrl.h"
#include "windef.h"

typedef struct tagTBUTTON_INFO
{
    INT iBitmap;
    INT idCommand;
    BYTE  fsState;
    BYTE  fsStyle;
    DWORD dwData;
    INT iString;

    BOOL bHot;
    INT nRow;
    RECT rect;
} TBUTTON_INFO; 


typedef struct tagTOOLBAR_INFO
{
    DWORD      dwStructSize;   /* size of TBBUTTON struct */
    INT      nHeight;        /* height of the toolbar */
    INT      nWidth;         /* width of the toolbar */
    INT      nButtonHeight;
    INT      nButtonWidth;
    INT      nBitmapHeight;
    INT      nBitmapWidth;
    INT      nIndent;
    INT      nRows;           /* number of button rows */
    INT      nMaxTextRows;    /* maximum number of text rows */
    INT      cxMin;           /* minimum button width */
    INT      cxMax;           /* maximum button width */
    INT      nNumButtons;     /* number of buttons */
    INT      nNumBitmaps;     /* number of bitmaps */
    INT      nNumStrings;     /* number of strings */
    BOOL     bUnicode;        /* ASCII (FALSE) or Unicode (TRUE)? */
    BOOL     bCaptured;       /* mouse captured? */
    INT      nButtonDown;
    INT      nOldHit;
    INT      nHotItem;        /* index of the "hot" item */
    HFONT    hFont;           /* text font */
    HIMAGELIST himlInt;         /* image list created internally */
    HIMAGELIST himlDef;         /* default image list */
    HIMAGELIST himlHot;         /* hot image list */
    HIMAGELIST himlDis;         /* disabled image list */
    HWND     hwndToolTip;     /* handle to tool tip control */
    HWND     hwndNotify;      /* handle to the window that gets notifications */
    BOOL     bTransparent;    /* background transparency flag */
    BOOL     bAutoSize;       /* auto size deadlock indicator */
    DWORD      dwExStyle;       /* extended toolbar style */
    DWORD      dwDTFlags;       /* DrawText flags */

    COLORREF   clrInsertMark;   /* insert mark color */
    RECT     rcBound;         /* bounding rectangle */

    TBUTTON_INFO *buttons;      /* pointer to button array */
    LPWSTR       *strings;      /* pointer to string array */
} TOOLBAR_INFO;


extern BOOL WINAPI TBARCUST_DialogProc (HWND, UINT, WPARAM, LPARAM);

extern VOID TOOLBAR_Register (VOID);
extern VOID TOOLBAR_Unregister (VOID);

#endif  /* __WINE_TOOLBAR_H */
