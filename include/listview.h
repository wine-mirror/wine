/*
 * Listview class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_LISTVIEW_H
#define __WINE_LISTVIEW_H

#include "commctrl.h"
#include "windef.h"
#include "wingdi.h"

/* Some definitions for inline edit control */    
typedef BOOL (*EditlblCallback)(HWND, LPSTR, DWORD);

typedef struct tagEDITLABEL_ITEM
{
    WNDPROC EditWndProc;
    DWORD param;
    EditlblCallback EditLblCb;
} EDITLABEL_ITEM;

typedef struct tagLISTVIEW_SUBITEM
{
    LPSTR pszText;
    INT iImage;
    INT iSubItem;

} LISTVIEW_SUBITEM;

typedef struct tagLISTVIEW_ITEM
{
  UINT state;
  LPSTR pszText;
  INT iImage;
  LPARAM lParam;
  INT iIndent;
  POINT ptPosition;

} LISTVIEW_ITEM;


typedef struct tagLISTVIEW_INFO
{
    COLORREF clrBk;
    COLORREF clrText;
    COLORREF clrTextBk;
    HIMAGELIST himlNormal;
    HIMAGELIST himlSmall;
    HIMAGELIST himlState;
    BOOL bLButtonDown;
    BOOL bRButtonDown;
    INT nFocusedItem;
    INT nItemHeight;
    INT nItemWidth;
    INT nSelectionMark;
    INT nHotItem;
    SHORT notifyFormat;
    RECT rcList;
    RECT rcView;
    SIZE iconSize;
    SIZE iconSpacing;
    UINT uCallbackMask;
    HWND hwndHeader;
    HFONT hDefaultFont;
    HFONT hFont;
    BOOL bFocus;
    DWORD dwExStyle;    /* extended listview style */
    HDPA hdpaItems;
    PFNLVCOMPARE pfnCompare;
    LPARAM lParamSort;
    HWND hwndEdit;
    INT nEditLabelItem;
    EDITLABEL_ITEM *pedititem;
    DWORD dwHoverTime;

    WPARAM charCode; /* Added */
    CHAR szSearchParam[ MAX_PATH ]; /* Added */
    DWORD timeSinceLastKeyPress; /* Added */
    INT nSearchParamLength; /* Added */


} LISTVIEW_INFO;


extern VOID LISTVIEW_Register (VOID);
extern VOID LISTVIEW_Unregister (VOID);

#endif  /* __WINE_LISTVIEW_H */
