/*
 * Tool tips class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_TOOLTIPS_H
#define __WINE_TOOLTIPS_H


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
    BOOL32     bActive;
    UINT32     uNumTools;
    COLORREF   clrBk;
    COLORREF   clrText;
    HFONT32    hFont;
    INT32      iMaxTipWidth;
    INT32      iCurrentTool;

    TTTOOL_INFO *tools;
} TOOLTIPS_INFO;


extern void TOOLTIPS_Register (void);

#endif  /* __WINE_TOOLTIPS_H */
