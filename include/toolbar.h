/*
 * Toolbar class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_TOOLBAR_H
#define __WINE_TOOLBAR_H


typedef struct tagTBUTTON_INFO
{
    INT32 iBitmap;
    INT32 idCommand;
    BYTE  fsState;
    BYTE  fsStyle;
    DWORD dwData;
    INT32 iString;

    RECT32 rect;

} TBUTTON_INFO; 


typedef struct tagTOOLBAR_INFO
{
    DWORD      dwStructSize;   /* size of TBBUTTON struct */
    INT32      nHeight;        /* height of the toolbar */
    INT32      nWidth;         /* width of the toolbar */
    INT32      nButtonTop;     /* top of the button rectangle */
    INT32      nButtonHeight;
    INT32      nButtonWidth;
    INT32      nBitmapHeight;
    INT32      nBitmapWidth;
    INT32      nIndent;
    INT32      nMaxRows;        /* maximum number of rows */

    INT32      nNumButtons;     /* number of buttons */
    INT32      nNumBitmaps;     /* number of bitmaps */
    INT32      nNumStrings;     /* number of strings */

    BOOL32     bCaptured;
    INT32      nButtonDown;
    INT32      nOldHit;

    HIMAGELIST himlDef;         /* default image list */
    HIMAGELIST himlHot;         /* hot image list */
    HIMAGELIST himlDis;         /* disabled image list */
    HWND32     hwndToolTip;     /* handle to tool tip control */
    HWND32     hwndNotify;      /* handle to the window that gets notifications */

    TBUTTON_INFO *buttons;
    CHAR         **strings;
} TOOLBAR_INFO;


extern void TOOLBAR_Register (void);

#endif  /* __WINE_TOOLBAR_H */
