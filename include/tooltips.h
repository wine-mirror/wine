/*
 * Tool tips class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_TOOLTIPS_H
#define __WINE_TOOLTIPS_H


typedef struct tagTT_SUBCLASS_INFO
{
    WNDPROC32 wpOrigProc;
    HWND32    hwndToolTip;
    UINT32    uRefCount;
} TT_SUBCLASS_INFO, *LPTT_SUBCLASS_INFO;


typedef struct tagTTTOOL_INFO
{
    UINT32 uFlags;
    HWND32 hwnd;
    UINT32 uId;
    RECT32 rect;
    HINSTANCE32 hinst;
    LPSTR  lpszText;
    LPARAM lParam;
} TTTOOL_INFO; 


typedef struct tagTOOLTIPS_INFO
{
    CHAR       szTipText[INFOTIPSIZE];
    BOOL32     bActive;
    BOOL32     bTrackActive;
    UINT32     uNumTools;
    COLORREF   clrBk;
    COLORREF   clrText;
    HFONT32    hFont;
    INT32      xTrackPos;
    INT32      yTrackPos;
    INT32      nMaxTipWidth;
    INT32      nTool;
    INT32      nOldTool;
    INT32      nCurrentTool;
    INT32      nTrackTool;
    INT32      nAutomaticTime;
    INT32      nReshowTime;
    INT32      nAutoPopTime;
    INT32      nInitialTime;
    RECT32     rcMargin;
    TTTOOL_INFO *tools;
} TOOLTIPS_INFO;


extern void TOOLTIPS_Register (void);

#endif  /* __WINE_TOOLTIPS_H */
