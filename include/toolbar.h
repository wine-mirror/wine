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
    DWORD      dwStructSize;   /* Size of TBBUTTON-Struct */
    INT32      nHeight;        /* Height of the Toolbar */
    INT32      nWidth;         /* Width of the Toolbar */
    INT32      nButtonTop;     /* top of the button rectangle */
    INT32      nButtonHeight;
    INT32      nButtonWidth;
    INT32      nBitmapHeight;
    INT32      nBitmapWidth;
    INT32      nIndent;

    INT32      nNumButtons;     /* Number of buttons */
    INT32      nNumBitmaps;
    INT32      nNumStrings;

    BOOL32     bCaptured;
    INT32      nButtonDown;
    INT32      nOldHit;

    HIMAGELIST himlDef;         /* default image list */
    HIMAGELIST himlHot;         /* hot image list */
    HIMAGELIST himlDis;         /* disabled image list */

    TBUTTON_INFO *buttons;
} TOOLBAR_INFO;


extern void TOOLBAR_Register (void);

#endif  /* __WINE_TOOLBAR_H */
