/*
 * Listview class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_LISTVIEW_H
#define __WINE_LISTVIEW_H


typedef struct tagLISTVIEW_ITEM
{
    UINT32 state;
    LPSTR  pszText;
    INT32  iImage;
    LPARAM lParam;
    INT32  iIndent;

} LISTVIEW_ITEM;


typedef struct tagLISTVIEW_INFO
{
    COLORREF   clrBk;
    COLORREF   clrText;
    COLORREF   clrTextBk;
    HIMAGELIST himlNormal;
    HIMAGELIST himlSmall;
    HIMAGELIST himlState;
  BOOL32 bLButtonDown;
  BOOL32 bRButtonDown;
    INT32      nColumnCount;
  INT32 nFocusedItem;
  INT32 nItemCount;
  INT32 nItemHeight;
  INT32 nColumnWidth;
  INT32 nSelectionMark;
    HWND32     hwndHeader;
    HFONT32    hDefaultFont;
    HFONT32    hFont;
  INT32 nWidth;
  INT32 nHeight;
    BOOL32     bFocus;
    DWORD      dwExStyle;    /* extended listview style */
    HDPA       hdpaItems;

} LISTVIEW_INFO;


extern VOID LISTVIEW_Register (VOID);
extern VOID LISTVIEW_Unregister (VOID);

#endif  /* __WINE_LISTVIEW_H */
