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
    INT32      nItemCount;
    INT32      nColumnCount;
    HWND32     hwndHeader;
    HFONT32    hDefaultFont;
    HFONT32    hFont;
    RECT32     rcList;       /* "client" area of the list (without header) */
    BOOL32     bFocus;

    HDPA       hdpaItems;

} LISTVIEW_INFO;


extern VOID LISTVIEW_Register (VOID);
extern VOID LISTVIEW_Unregister (VOID);

#endif  /* __WINE_LISTVIEW_H */
