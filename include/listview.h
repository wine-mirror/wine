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

    HDSA   hdsaItem;
} LISTVIEW_ITEM;


typedef struct tagLISTVIEW_INFO
{
    COLORREF   clrBk;
    HIMAGELIST himlNormal;
    HIMAGELIST himlSmall;
    HIMAGELIST himlState;
    INT32      nItemCount;
    HWND32     hwndHeader;
    HFONT32    hDefaultFont;
    HFONT32    hFont;
    RECT32     rcList;       /* "client" area of the list (without header) */

    HDSA       hdsaItems;

} LISTVIEW_INFO;


extern void LISTVIEW_Register (void);

#endif  /* __WINE_LISTVIEW_H */
