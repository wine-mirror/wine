/*
 * Listview control
 *
 * Copyright 1998 Eric Kohl
 * Copyright 1999 Luc Tourangeau
 *
 * NOTES
 * Listview control implementation. 
 *
 * TODO:
 *
 *   1. Multiple selections in icon or small icon display modes DO NOT
 *      behave like the Microsoft control.
 *   2. No horizontal scrolling when header is larger than the client area.
 *   3. Implement LVM_FINDITEM for key selections.
 *   4. Drawing optimizations.
 *
 * Notifications:
 *   LISTVIEW_Notify : most notifications from children (editbox and header)
 *
 * Data structure:
 *   LISTVIEW_SortItems : empty stub 
 *   LISTVIEW_SetItemCount : empty stub 
 * 
 * Unicode:
 *   LISTVIEW_SetItem32W : no unicode support
 *   LISTVIEW_InsertItem32W : no unicode support
 *   LISTVIEW_InsertColumn32W : no unicode support
 *   LISTVIEW_GetColumnW : no unicode support
 *
 * Advanced functionality:
 *   LISTVIEW_GetNumberOfWorkAreas : not implemented
 *   LISTVIEW_GetNextItem : empty stub
 *   LISTVIEW_GetHotCursor : not implemented
 *   LISTVIEW_GetHotItem : not implemented
 *   LISTVIEW_GetHoverTime : not implemented
 *   LISTVIEW_GetISearchString : not implemented 
 *   LISTVIEW_GetBkImage : not implemented
 *   LISTVIEW_EditLabel : REPORT (need to implement a timer)
 *   LISTVIEW_GetColumnOrderArray : not implemented
 *   LISTVIEW_Arrange : empty stub
 *   LISTVIEW_FindItem : empty stub 
 *   LISTVIEW_ApproximateViewRect : incomplete
 *   LISTVIEW_Scroll : not implemented 
 *   LISTVIEW_KeyDown : page up and page down  + redo small icon and icon 
 *   LISTVIEW_RedrawItems : empty stub
 *   LISTVIEW_Update : not completed
 */

#include <string.h>
#include "winbase.h"
#include "commctrl.h"
#include "listview.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(listview)

/*
 * constants 
 */

/* maximum size of a label */
#define DISP_TEXT_SIZE 128

/* padding for items in list and small icon display modes */
#define WIDTH_PADDING 12

/* padding for items in list, report and small icon display modes */
#define HEIGHT_PADDING 1

/* offset of items in report display mode */
#define REPORT_MARGINX 2

/* padding for icon in large icon display mode */
#define ICON_TOP_PADDING 2
#define ICON_BOTTOM_PADDING 2

/* padding for label in large icon display mode */
#define LABEL_VERT_OFFSET 2

/* default label width for items in list and small icon display modes */
#define DEFAULT_LABEL_WIDTH 40

/* default column width for items in list display mode */
#define DEFAULT_COLUMN_WIDTH 96 

/* 
 * macros
 */

/* retrieve the number of items in the listview */
#define GETITEMCOUNT(infoPtr) ((infoPtr)->hdpaItems->nItemCount)
#define ListView_LVNotify(hwnd,lCtrlId,plvnm) \
    (BOOL)SendMessageA((hwnd),WM_NOTIFY,(WPARAM)(INT)lCtrlId,(LPARAM)(LPNMLISTVIEW)(plvnm))
#define ListView_Notify(hwnd,lCtrlId,pnmh) \
    (BOOL)SendMessageA((hwnd),WM_NOTIFY,(WPARAM)(INT)lCtrlId,(LPARAM)(LPNMHDR)(pnmh))

/* 
 * forward declarations 
 */

static VOID LISTVIEW_AlignLeft(HWND);
static VOID LISTVIEW_AlignTop(HWND);
static VOID LISTVIEW_AddGroupSelection(HWND, INT);
static VOID LISTVIEW_AddSelection(HWND, INT);
static BOOL LISTVIEW_AddSubItem(HWND, LPLVITEMA);
static INT LISTVIEW_FindInsertPosition(HDPA, INT);
static VOID LISTVIEW_GetItemDispInfo(HWND, INT, LISTVIEW_ITEM *lpItem, INT *, 
                                     UINT *, CHAR **, INT);
static INT LISTVIEW_GetItemHeight(HWND, LONG);
static BOOL LISTVIEW_GetItemPosition(HWND, INT, LPPOINT);
static LRESULT LISTVIEW_GetItemRect(HWND, INT, LPRECT);
static INT LISTVIEW_GetItemWidth(HWND, LONG);
static INT LISTVIEW_GetLabelWidth(HWND, INT);
static LRESULT LISTVIEW_GetOrigin(HWND, LPPOINT);
static LISTVIEW_SUBITEM* LISTVIEW_GetSubItem(HDPA, INT);
static VOID LISTVIEW_GetSubItemDispInfo(HWND hwnd, INT, LPARAM,
                                        LISTVIEW_SUBITEM *, INT, INT *, 
                                        CHAR **, INT);
static LRESULT LISTVIEW_GetViewRect(HWND, LPRECT);
static BOOL LISTVIEW_InitItem(HWND, LISTVIEW_ITEM *, LPLVITEMA);
static BOOL LISTVIEW_InitSubItem(HWND, LISTVIEW_SUBITEM *, LPLVITEMA);
static LRESULT LISTVIEW_MouseSelection(HWND, INT, INT);
static BOOL LISTVIEW_RemoveColumn(HDPA, INT);
static VOID LISTVIEW_RemoveSelections(HWND, INT, INT);
static BOOL LISTVIEW_RemoveSubItem(HDPA, INT);
static BOOL LISTVIEW_ScrollView(HWND, INT, INT);
static VOID LISTVIEW_SetGroupSelection(HWND, INT);
static BOOL LISTVIEW_SetItem(HWND, LPLVITEMA);
static VOID LISTVIEW_SetItemFocus(HWND, INT);
static BOOL LISTVIEW_SetItemPosition(HWND, INT, INT, INT);
static VOID LISTVIEW_SetScroll(HWND, LONG);
static VOID LISTVIEW_SetSelection(HWND, INT);
static VOID LISTVIEW_SetSize(HWND, LONG, LONG, LONG);
static BOOL LISTVIEW_SetSubItem(HWND, LPLVITEMA);
static VOID LISTVIEW_SetViewInfo(HWND, LONG);
static LRESULT LISTVIEW_SetViewRect(HWND, LPRECT);
static BOOL LISTVIEW_ToggleSelection(HWND, INT);
static VOID LISTVIEW_UnsupportedStyles(LONG lStyle);

/***
 * DESCRIPTION:
 * Scrolls the content of the listview.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : number of horizontal scroll positions 
 *           (relative to the current scroll postioon)
 * [I] INT : number of vertical scroll positions
 *           (relative to the current scroll postioon)
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_ScrollView(HWND hwnd, INT nHScroll, INT nVScroll)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0); 
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
  INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
  INT nHScrollPos = 0;
  INT nVScrollPos = 0;
  INT nHScrollInc = 0;
  INT nVScrollInc = 0;
  BOOL bResult = FALSE;

  if (((lStyle & WS_HSCROLL) != 0) && (nHScroll != 0))
  {
    switch (LVS_TYPEMASK & lStyle)
    {
    case LVS_LIST:
      nHScrollInc = nHScroll * infoPtr->nItemWidth;
      break;

    case LVS_REPORT:
      /* TO DO : not implemented at this point. I experiences some 
         problems when performing child window scrolling. */
      break;

    case LVS_SMALLICON:
    case LVS_ICON:
      nHScrollInc = nHScroll * max(nListWidth, nListWidth / 10);
      break;
    }
    
    nHScrollPos = GetScrollPos(hwnd, SB_HORZ) + nHScroll;
  }

  if (((lStyle & WS_VSCROLL) != 0) & (nVScroll != 0))
  {
    switch (LVS_TYPEMASK & lStyle)
    {
    case LVS_REPORT:
      nVScrollInc = nVScroll * infoPtr->nItemHeight;
      nVScrollPos = GetScrollPos(hwnd, SB_VERT) + nVScroll;
      break;
      
    case LVS_SMALLICON:
    case LVS_ICON:
      nVScrollInc = nVScroll * max(nListHeight, nListHeight / 10);
      nVScrollPos = GetScrollPos(hwnd, SB_VERT) + nVScroll;
      break;
    }
  }

  /* perform scroll operation & set new scroll position */
  if ((nHScrollInc != 0) || (nVScrollInc != 0))
  {
    RECT rc;
    HDC hdc = GetDC(hwnd);
    ScrollDC(hdc, -nHScrollInc, -nVScrollInc, &infoPtr->rcList, NULL, 
             (HRGN) NULL, &rc);
    InvalidateRect(hwnd, &rc, TRUE);
    SetScrollPos(hwnd, SB_HORZ, nHScrollPos, TRUE);
    SetScrollPos(hwnd, SB_VERT, nVScrollPos, TRUE);
    ReleaseDC(hwnd, hdc);
    bResult = TRUE;
  }

  return bResult; 
}

/***
 * DESCRIPTION:
 * Prints a message for unsupported window styles.
 * A kind of TODO list for window styles.
 * 
 * PARAMETER(S):
 * [I] LONG : window style
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_UnsupportedStyles(LONG lStyle)
{
  if ((LVS_TYPEMASK & lStyle) == LVS_EDITLABELS)
  {
    FIXME( listview, "  LVS_EDITLABELS\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_NOCOLUMNHEADER)
  {
    FIXME( listview, "  LVS_SORTDESCENDING\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_NOLABELWRAP)
  {
    FIXME( listview, "  LVS_NOLABELWRAP\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_NOSCROLL)
  {
    FIXME( listview, "  LVS_NOSCROLL\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_NOSORTHEADER)
  {
    FIXME( listview, "  LVS_NOSORTHEADER\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_OWNERDRAWFIXED)
  {
    FIXME( listview, "  LVS_OWNERDRAWFIXED\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_SHAREIMAGELISTS)
  {
    FIXME( listview, "  LVS_SHAREIMAGELISTS\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_SHOWSELALWAYS)
  {
    FIXME( listview, "  LVS_SHOWSELALWAYS\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_SINGLESEL)
  {
    FIXME( listview, "  LVS_SINGLESEL\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_SORTASCENDING)
  {
    FIXME( listview, "  LVS_SORTASCENDING\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_SORTDESCENDING)
  {
    FIXME( listview, "  LVS_SORTDESCENDING\n");
  }
}

/***
 * DESCRIPTION:
 * Aligns the items with the top edge of the window.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_AlignTop(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
  POINT ptItem;
  RECT rcView;
  INT i;
  
  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_SMALLICON:
  case LVS_ICON:
    ZeroMemory(&ptItem, sizeof(POINT));
    ZeroMemory(&rcView, sizeof(RECT));
    if (nListWidth > infoPtr->nItemWidth)
    {
      for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
      {
        if (ptItem.x + infoPtr->nItemWidth > nListWidth)
        {
          ptItem.x = 0;
          ptItem.y += infoPtr->nItemHeight;
        }
        
        LISTVIEW_SetItemPosition(hwnd, i, ptItem.x, ptItem.y);
        ptItem.x += infoPtr->nItemWidth;
        rcView.right = max(rcView.right, ptItem.x);
      }
    }
    else
    {
      for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
      {
        LISTVIEW_SetItemPosition(hwnd, i, ptItem.x, ptItem.y);
        ptItem.x += infoPtr->nItemWidth;
      }
      rcView.right = ptItem.x;
      rcView.bottom = infoPtr->nItemHeight;
    }

    rcView.bottom = ptItem.y + infoPtr->nItemHeight;
    LISTVIEW_SetViewRect(hwnd, &rcView);
  }
}

/***
 * DESCRIPTION:
 * Aligns the items with the left edge of the window.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_AlignLeft(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
  POINT ptItem;
  RECT rcView;
  INT i;
  
  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_SMALLICON:
  case LVS_ICON:
    ZeroMemory(&ptItem, sizeof(POINT));
    ZeroMemory(&rcView, sizeof(RECT));
    if (nListHeight > infoPtr->nItemHeight)
    {
      for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
      {
        if (ptItem.y + infoPtr->nItemHeight > nListHeight)
        {
          ptItem.y = 0;
          ptItem.x += infoPtr->nItemWidth;
        }

        LISTVIEW_SetItemPosition(hwnd, i, ptItem.x, ptItem.y);
        ptItem.y += infoPtr->nItemHeight;
        rcView.bottom = max(rcView.bottom, ptItem.y);
      }
    }
    else
    {
      for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
      {
        LISTVIEW_SetItemPosition(hwnd, i, ptItem.x, ptItem.y);
        ptItem.y += infoPtr->nItemHeight;
      }
      rcView.bottom = ptItem.y;
      rcView.right = infoPtr->nItemWidth;
    }

    rcView.right = ptItem.x + infoPtr->nItemWidth;
    LISTVIEW_SetViewRect(hwnd, &rcView);
    break;
 }
}

/***
 * DESCRIPTION:
 * Retrieves display information.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index
 * [I] LISTVIEW_ITEM* : listview control item
 * [O] INT : image index
 * [O] UINT : state value
 * [O] CHAR** : string
 * [I] INT : size of string
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_GetItemDispInfo(HWND hwnd, INT nItem, 
                                     LISTVIEW_ITEM *lpItem, INT *pnDispImage, 
                                     UINT *puState, CHAR **ppszDispText, 
                                     INT nDispTextSize)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMLVDISPINFOA dispInfo;
  ZeroMemory(&dispInfo, sizeof(NMLVDISPINFOA));
 
  if ((pnDispImage != NULL) && (lpItem->iImage == I_IMAGECALLBACK))
  {
    dispInfo.item.mask |= LVIF_IMAGE;
  }

  if ((ppszDispText != NULL) && (lpItem->pszText == LPSTR_TEXTCALLBACKA))
  {
    ZeroMemory(*ppszDispText, sizeof(CHAR)*nDispTextSize);
    dispInfo.item.mask |= LVIF_TEXT;
    dispInfo.item.pszText = *ppszDispText;
    dispInfo.item.cchTextMax = nDispTextSize;
  }

  if ((puState != NULL) && (infoPtr->uCallbackMask != 0))
  {
    dispInfo.item.mask |= LVIF_STATE;
    dispInfo.item.stateMask = infoPtr->uCallbackMask; 
  }

  if (dispInfo.item.mask != 0)
  {
    dispInfo.hdr.hwndFrom = hwnd;
    dispInfo.hdr.idFrom = lCtrlId;
    dispInfo.hdr.code = LVN_GETDISPINFOA;
    dispInfo.item.iItem = nItem;
    dispInfo.item.iSubItem = 0;
    dispInfo.item.lParam = lpItem->lParam;
    ListView_Notify(GetParent(hwnd), lCtrlId, &dispInfo);
  }

  if (pnDispImage != NULL)
  {
    if (dispInfo.item.mask & LVIF_IMAGE)
    {
      *pnDispImage = dispInfo.item.iImage;
    }
    else
    {
      *pnDispImage = lpItem->iImage;
    }
  }

  if (ppszDispText != NULL)
  {
    if (dispInfo.item.mask & LVIF_TEXT)
    {
      if (dispInfo.item.mask & LVIF_DI_SETITEM)
      {
        Str_SetPtrA(&lpItem->pszText, dispInfo.item.pszText);
      }
      *ppszDispText = dispInfo.item.pszText;
    }
    else
    {
      *ppszDispText = lpItem->pszText;
    }
  }

  if (puState != NULL)
  {
    if (dispInfo.item.mask & LVIF_STATE)
    {
      *puState = lpItem->state;
      *puState &= ~dispInfo.item.stateMask;
      *puState |= (dispInfo.item.state & dispInfo.item.stateMask);
    }
    else
    {
      *puState = lpItem->state;
    }
  }
}

/***
 * DESCRIPTION:
 * Retrieves subitem display information.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index
 * [I] LONG : LPARAM of item
 * [I] LISTVIEW_SUBITEM* : listview control subitem
 * [I] INT : subitem position/order 
 * [O] INT : image index
 * [O] UINT : state value
 * [O] CHAR** : display string
 * [I] INT : size of string
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_GetSubItemDispInfo(HWND hwnd, INT nItem, LPARAM lParam,
                                        LISTVIEW_SUBITEM *lpSubItem, 
                                        INT nSubItemPos, INT *pnDispImage, 
                                        CHAR **ppszDispText, INT nDispTextSize)
{
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMLVDISPINFOA dispInfo;
  ZeroMemory(&dispInfo, sizeof(NMLVDISPINFOA));

  if (lpSubItem == NULL)
  {
    ZeroMemory(*ppszDispText, sizeof(CHAR)*nDispTextSize);
    dispInfo.item.mask |= LVIF_TEXT;
    dispInfo.item.pszText = *ppszDispText;
    dispInfo.item.cchTextMax = nDispTextSize;
    dispInfo.hdr.hwndFrom = hwnd;
    dispInfo.hdr.idFrom = lCtrlId;
    dispInfo.hdr.code = LVN_GETDISPINFOA;
    dispInfo.item.iItem = nItem;
    dispInfo.item.iSubItem = nSubItemPos;
    dispInfo.item.lParam = lParam;
    ListView_Notify(GetParent(hwnd), lCtrlId, &dispInfo);
    if (dispInfo.item.mask & LVIF_DI_SETITEM)
    {
      Str_SetPtrA(&lpSubItem->pszText, dispInfo.item.pszText);
    }
    *ppszDispText = dispInfo.item.pszText;
  }
  else
  {
    if ((pnDispImage != NULL) && (lpSubItem->iImage == I_IMAGECALLBACK))
    {
      dispInfo.item.mask |= LVIF_IMAGE;
    }

    if ((ppszDispText != NULL) && (lpSubItem->pszText == LPSTR_TEXTCALLBACKA))
    {
      ZeroMemory(*ppszDispText, sizeof(CHAR)*nDispTextSize);
      dispInfo.item.mask |= LVIF_TEXT;
      dispInfo.item.pszText = *ppszDispText;
      dispInfo.item.cchTextMax = nDispTextSize;
    }

    if (dispInfo.item.mask != 0)
    {
      dispInfo.hdr.hwndFrom = hwnd;
      dispInfo.hdr.idFrom = lCtrlId;
      dispInfo.hdr.code = LVN_GETDISPINFOA;
      dispInfo.item.iItem = nItem;
      dispInfo.item.iSubItem = lpSubItem->iSubItem;
      dispInfo.item.lParam = lParam;
      ListView_Notify(GetParent(hwnd), lCtrlId, &dispInfo);
    }

    if (pnDispImage != NULL)
    {
      if (dispInfo.item.mask & LVIF_IMAGE)
      {
        *pnDispImage = dispInfo.item.iImage;
      }
      else
      {
        *pnDispImage = lpSubItem->iImage;
      }
    }

    if (ppszDispText != NULL)
    {
      if (dispInfo.item.mask & LVIF_TEXT)
      {
        if (dispInfo.item.mask & LVIF_DI_SETITEM)
        {
          Str_SetPtrA(&lpSubItem->pszText, dispInfo.item.pszText);
        }
        *ppszDispText = dispInfo.item.pszText;
      }
      else
      {
        *ppszDispText = lpSubItem->pszText;
      }
    }
  }
}

/***
 * DESCRIPTION:
 * Calculates the width of an item.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] LONG : window style
 *
 * RETURN:
 * Returns item width.
 */
static INT LISTVIEW_GetItemWidth(HWND hwnd, LONG lStyle)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nHeaderItemCount;
  RECT rcHeaderItem;
  INT nItemWidth = 0;
  INT nLabelWidth;
  INT i;

  TRACE(listview, "(hwnd=%x,lStyle=%lx)\n", hwnd, lStyle);

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_ICON:
    nItemWidth = infoPtr->iconSpacing.cx;
    break;

  case LVS_REPORT:
    /* calculate width of header */
    nHeaderItemCount = Header_GetItemCount(infoPtr->hwndHeader);
    for (i = 0; i < nHeaderItemCount; i++)
    {
      if (Header_GetItemRect(infoPtr->hwndHeader, i, &rcHeaderItem) != 0)
      {
        nItemWidth += (rcHeaderItem.right - rcHeaderItem.left);
      }
    }
    break;

  case LVS_SMALLICON:
  case LVS_LIST:
    for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
    { 
      nLabelWidth = LISTVIEW_GetLabelWidth(hwnd, i);
      nItemWidth = max(nItemWidth, nLabelWidth);
    }

    /* default label size */
    if (GETITEMCOUNT(infoPtr) == 0)
    {
      nItemWidth = DEFAULT_COLUMN_WIDTH;
    }
    else
    {
      if (nItemWidth == 0)
      {
        nItemWidth = DEFAULT_LABEL_WIDTH;
      }
      else
      {
        /* add padding */
        nItemWidth += WIDTH_PADDING;
      
        if (infoPtr->himlSmall != NULL)
        {
          nItemWidth += infoPtr->iconSize.cx;
        }

        if (infoPtr->himlState != NULL)
        {
          nItemWidth += infoPtr->iconSize.cx;
        }
      }
    }
    break;

  }

  return nItemWidth;
}

/***
 * DESCRIPTION:
 * Calculates the height of an item.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] LONG : window style
 *
 * RETURN:
 * Returns item width.
 */
static INT LISTVIEW_GetItemHeight(HWND hwnd, LONG lStyle)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nItemHeight = 0;
  TEXTMETRICA tm; 
  HFONT hOldFont;
  HDC hdc;

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_ICON:
    nItemHeight = infoPtr->iconSpacing.cy;
    break;

  case LVS_SMALLICON:
  case LVS_REPORT:
  case LVS_LIST:
    hdc = GetDC(hwnd);
    hOldFont = SelectObject(hdc, infoPtr->hFont);
    GetTextMetricsA(hdc, &tm);
    nItemHeight = max(tm.tmHeight, infoPtr->iconSize.cy) + HEIGHT_PADDING;
    SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);
    break;
  }

  return nItemHeight;
}

/***
 * DESCRIPTION:
 * Sets diplay information (needed for drawing operations).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * 
 * RETURN:
 * None
 */
static VOID LISTVIEW_SetViewInfo(HWND hwnd, LONG lStyle)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
  INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_REPORT:
    /* get number of fully visible items per column */
    infoPtr->nCountPerColumn = max(1, nListHeight / infoPtr->nItemHeight);
    break;

  case LVS_LIST:
    /* get number of fully visible items per column */
    infoPtr->nCountPerColumn = max(1, nListHeight / infoPtr->nItemHeight);
    
    /* get number of fully visible items per row */
    infoPtr->nCountPerRow = max(1, nListWidth / infoPtr->nItemWidth);
    break;
  }
}

/***
 * DESCRIPTION:
 * Adds a block of selections.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index 
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_AddGroupSelection(HWND hwnd, INT nItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nFirst = min(infoPtr->nSelectionMark, nItem);
  INT nLast = max(infoPtr->nSelectionMark, nItem);
  LVITEMA lvItem;
  INT i;

  lvItem.state = LVIS_SELECTED;
  lvItem.stateMask= LVIS_SELECTED;
  
  for (i = nFirst; i <= nLast; i++)
  {
    ListView_SetItemState(hwnd, i, &lvItem);
  }
  
  LISTVIEW_SetItemFocus(hwnd, nItem);
  infoPtr->nSelectionMark = nItem;
}

/***
 * DESCRIPTION:
 * Adds a single selection.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index 
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_AddSelection(HWND hwnd, INT nItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LVITEMA lvItem;

  lvItem.state = LVIS_SELECTED;
  lvItem.stateMask= LVIS_SELECTED;

  ListView_SetItemState(hwnd, nItem, &lvItem);

  LISTVIEW_SetItemFocus(hwnd, nItem);
  infoPtr->nSelectionMark = nItem;
}

/***
 * DESCRIPTION:
 * Selects or unselects an item.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index 
 *
 * RETURN:
 *   SELECT: TRUE 
 *   UNSELECT : FALSE
 */
static BOOL LISTVIEW_ToggleSelection(HWND hwnd, INT nItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  BOOL bResult;
  LVITEMA lvItem;

  lvItem.stateMask= LVIS_SELECTED;

  if (ListView_GetItemState(hwnd, nItem, LVIS_SELECTED) & LVIS_SELECTED)
  {
    lvItem.state = 0;
    ListView_SetItemState(hwnd, nItem, &lvItem);
    bResult = FALSE;
  }
  else
  {
    lvItem.state = LVIS_SELECTED;
    ListView_SetItemState(hwnd, nItem, &lvItem);
    bResult = TRUE;
  }

  LISTVIEW_SetItemFocus(hwnd, nItem);
  infoPtr->nSelectionMark = nItem;

  return bResult;
}

/***
 * DESCRIPTION:
 * Sets a single group selection.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index 
 *
 * RETURN:
 * None 
 */
static VOID LISTVIEW_SetGroupSelection(HWND hwnd, INT nItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nFirst = min(infoPtr->nSelectionMark, nItem);
  INT nLast = max(infoPtr->nSelectionMark, nItem);
  LVITEMA lvItem;
  INT i;

  if (nFirst > 0)
  {
    LISTVIEW_RemoveSelections(hwnd, 0, nFirst - 1);
  }

  if (nLast < GETITEMCOUNT(infoPtr))
  {
    LISTVIEW_RemoveSelections(hwnd, nLast + 1, GETITEMCOUNT(infoPtr));
  }

  lvItem.state = LVIS_SELECTED;
  lvItem.stateMask = LVIS_SELECTED;

  for (i = nFirst; i <= nLast; i++)
  {
    ListView_SetItemState(hwnd, i, &lvItem);
  }

  LISTVIEW_SetItemFocus(hwnd, nItem);
}

/***
 * DESCRIPTION:
 * Manages the item focus.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index 
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_SetItemFocus(HWND hwnd, INT nItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LVITEMA lvItem;

  lvItem.state = 0;
  lvItem.stateMask = LVIS_FOCUSED;
  ListView_SetItemState(hwnd, infoPtr->nFocusedItem, &lvItem); 
  
  lvItem.state =  LVIS_FOCUSED;
  lvItem.stateMask = LVIS_FOCUSED;
  ListView_SetItemState(hwnd, nItem, &lvItem);

  infoPtr->nFocusedItem = nItem;

  /* if multiple selection is allowed */
  ListView_EnsureVisible(hwnd, nItem, FALSE);
}

/***
 * DESCRIPTION:
 * Sets a single selection.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index 
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_SetSelection(HWND hwnd, INT nItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LVITEMA lvItem;

  if (nItem > 0)
  {
    LISTVIEW_RemoveSelections(hwnd, 0, nItem - 1);
  }

  if (nItem < GETITEMCOUNT(infoPtr))
  {
    LISTVIEW_RemoveSelections(hwnd, nItem + 1, GETITEMCOUNT(infoPtr));
  }

  lvItem.state = 0;
  lvItem.stateMask = LVIS_FOCUSED;
  ListView_SetItemState(hwnd, infoPtr->nFocusedItem, &lvItem); 
  
  lvItem.state =  LVIS_SELECTED | LVIS_FOCUSED;
  lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
  ListView_SetItemState(hwnd, nItem, &lvItem);

  infoPtr->nFocusedItem = nItem;
  infoPtr->nSelectionMark = nItem;
}

/***
 * DESCRIPTION:
 * Set selection(s) with keyboard.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index 
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_KeySelection(HWND hwnd, INT nItem)
{
  WORD wShift = HIWORD(GetKeyState(VK_SHIFT));
  WORD wCtrl = HIWORD(GetKeyState(VK_CONTROL));

  if (wShift)
  {
    LISTVIEW_SetGroupSelection(hwnd, nItem); 
  }
  else if (wCtrl)
  {
    LISTVIEW_SetItemFocus(hwnd, nItem);
  }
  else
  {
    LISTVIEW_SetSelection(hwnd, nItem); 

    /* if multiple selection is allowed */
    ListView_EnsureVisible(hwnd, nItem, FALSE);
  }
}

/***
 * DESCRIPTION:
 * Determines the selected item.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : x ccordinate
 * [I] INT : y coordinate
 *
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_MouseSelection(HWND hwnd, INT nPosX, INT nPosY)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  RECT rcItem;
  INT i;
  
  for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
  {
    rcItem.left = LVIR_SELECTBOUNDS;
    if (LISTVIEW_GetItemRect(hwnd, i, &rcItem) == TRUE)
    {
      if ((rcItem.left <= nPosX) && (nPosX <= rcItem.right) && 
          (rcItem.top <= nPosY) && (nPosY <= rcItem.bottom))
      {
        return i;
      }
    }
  }

  return -1;
}

/***
 * DESCRIPTION:
 * Removes all selection states.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index 
 *
 * RETURN:
 *   SUCCCESS : TRUE
 *   FAILURE : FALSE
 */
static VOID LISTVIEW_RemoveSelections(HWND hwnd, INT nFirst, INT nLast)
{
  LVITEMA lvItem;
  INT i;

  lvItem.state = 0;
  lvItem.stateMask = LVIS_SELECTED;

  for (i = nFirst; i <= nLast; i++)
  {
    ListView_SetItemState(hwnd, i, &lvItem);
  }
}

/***
 * DESCRIPTION:
 * Removes a column.
 * 
 * PARAMETER(S):
 * [IO] HDPA : dynamic pointer array handle
 * [I] INT : column index (subitem index)
 *
 * RETURN:
 *   SUCCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_RemoveColumn(HDPA hdpaItems, INT nSubItem)
{
  BOOL bResult = TRUE;
  HDPA hdpaSubItems;
  INT i;

  for (i = 0; i < hdpaItems->nItemCount; i++)
  {
    hdpaSubItems = (HDPA)DPA_GetPtr(hdpaItems, i);
    if (hdpaSubItems != NULL)
    {
      if (LISTVIEW_RemoveSubItem(hdpaSubItems, nSubItem) == FALSE)
      {
        bResult = FALSE;
      }
    }
  }
    
  return bResult;
}

/***
 * DESCRIPTION:
 * Removes a subitem at a given position.
 * 
 * PARAMETER(S):
 * [IO] HDPA : dynamic pointer array handle
 * [I] INT : subitem index
 *
 * RETURN:
 *   SUCCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_RemoveSubItem(HDPA hdpaSubItems, INT nSubItem)
{
  LISTVIEW_SUBITEM *lpSubItem;
  INT i;

  for (i = 1; i < hdpaSubItems->nItemCount; i++)
  {
    lpSubItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, i);
    if (lpSubItem != NULL)
    {
      if (lpSubItem->iSubItem == nSubItem)
      {
        /* free string */
        if ((lpSubItem->pszText != NULL) && 
            (lpSubItem->pszText != LPSTR_TEXTCALLBACKA))
        {
          COMCTL32_Free(lpSubItem->pszText);
        }
      
        /* free item */
        COMCTL32_Free(lpSubItem);

        /* free dpa memory */
        if (DPA_DeletePtr(hdpaSubItems, i) == NULL)
        {
          return FALSE;
        }
      }
      else if (lpSubItem->iSubItem > nSubItem)
      {
        return TRUE;
      }
    }
  }    
  
  return TRUE;
}

/***
 * DESCRIPTION:
 * Compares the item information.
 * 
 * PARAMETER(S):
 * [I] LISTVIEW_ITEM *: destination item 
 * [I] LPLVITEM : source item 
 *
 * RETURN:
 *   SUCCCESS : TRUE (EQUAL)
 *   FAILURE : FALSE (NOT EQUAL)
 */
static UINT LISTVIEW_GetItemChanges(LISTVIEW_ITEM *lpItem, LPLVITEMA lpLVItem)
{
  UINT uChanged = 0;

  if ((lpItem != NULL) && (lpLVItem != NULL))
  {
    if (lpLVItem->mask & LVIF_STATE)
    {
      if ((lpItem->state & lpLVItem->stateMask) != 
          (lpLVItem->state & lpLVItem->stateMask))
      {
        uChanged |= LVIF_STATE; 
      }
    }
    
    if (lpLVItem->mask & LVIF_IMAGE)
    {
      if (lpItem->iImage != lpLVItem->iImage)
      {
        uChanged |= LVIF_IMAGE; 
      }
    }
  
    if (lpLVItem->mask & LVIF_PARAM)
    {
      if (lpItem->lParam != lpLVItem->lParam)
      {
        uChanged |= LVIF_PARAM; 
      }
    }
    
    if (lpLVItem->mask & LVIF_INDENT)
    {
      if (lpItem->iIndent != lpLVItem->iIndent)
      {
        uChanged |= LVIF_INDENT; 
      }
    }

    if (lpLVItem->mask & LVIF_TEXT) 
    {
      if (lpLVItem->pszText == LPSTR_TEXTCALLBACKA) 
      {
        if (lpItem->pszText != LPSTR_TEXTCALLBACKA)
        {
          uChanged |= LVIF_TEXT; 
        }
      }
      else
      {
        if (lpItem->pszText == LPSTR_TEXTCALLBACKA)
        {
          uChanged |= LVIF_TEXT; 
        }
      }
    }
  }

  return uChanged;
}

/***
 * DESCRIPTION:
 * Initializes item attributes.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [O] LISTVIEW_ITEM *: destination item 
 * [I] LPLVITEM : source item 
 *
 * RETURN:
 *   SUCCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_InitItem(HWND hwnd, LISTVIEW_ITEM *lpItem, 
                              LPLVITEMA lpLVItem)
{
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  BOOL bResult = FALSE;

  if ((lpItem != NULL) && (lpLVItem != NULL))
  {
    bResult = TRUE;
    
    if (lpLVItem->mask & LVIF_STATE)
    {
      lpItem->state &= ~lpLVItem->stateMask;
      lpItem->state |= (lpLVItem->state & lpLVItem->stateMask);
    }
    
    if (lpLVItem->mask & LVIF_IMAGE)
    {
      lpItem->iImage = lpLVItem->iImage;
    }
  
    if (lpLVItem->mask & LVIF_PARAM)
    {
      lpItem->lParam = lpLVItem->lParam;
    }
    
    if (lpLVItem->mask & LVIF_INDENT)
    {
      lpItem->iIndent = lpLVItem->iIndent;
    }

    if (lpLVItem->mask & LVIF_TEXT) 
    {
      if (lpLVItem->pszText == LPSTR_TEXTCALLBACKA) 
      {
        if ((lStyle & LVS_SORTASCENDING) || (lStyle & LVS_SORTDESCENDING))
        {
          return FALSE;
        }

        if ((lpItem->pszText != NULL) && 
            (lpItem->pszText != LPSTR_TEXTCALLBACKA))
        {
          COMCTL32_Free(lpItem->pszText);
        }
    
        lpItem->pszText = LPSTR_TEXTCALLBACKA;
      }
      else 
      {
        if (lpItem->pszText == LPSTR_TEXTCALLBACKA)
        {
          lpItem->pszText = NULL;
        }
        
        bResult = Str_SetPtrA(&lpItem->pszText, lpLVItem->pszText);
      }
    }
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Initializes subitem attributes.
 *
 * NOTE: the documentation specifies that the operation fails if the user
 * tries to set the indent of a subitem.
 *
 * PARAMETER(S):
 * [I] HWND : window handle
 * [O] LISTVIEW_SUBITEM *: destination subitem
 * [I] LPLVITEM : source subitem
 *
 * RETURN:
 *   SUCCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_InitSubItem(HWND hwnd, LISTVIEW_SUBITEM *lpSubItem, 
                                   LPLVITEMA lpLVItem)
{
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  BOOL bResult = FALSE;
  
  if ((lpSubItem != NULL) && (lpLVItem != NULL))
  {
    if (!(lpLVItem->mask & LVIF_INDENT))
    {
      bResult = TRUE;
      ZeroMemory(lpSubItem, sizeof(LISTVIEW_SUBITEM));

      lpSubItem->iSubItem = lpLVItem->iSubItem;

      if (lpLVItem->mask & LVIF_IMAGE)
      {
        lpSubItem->iImage = lpLVItem->iImage;
      }
      
      if (lpLVItem->mask & LVIF_TEXT) 
      {
        if (lpLVItem->pszText == LPSTR_TEXTCALLBACKA) 
        {
          if ((lStyle & LVS_SORTASCENDING) || (lStyle & LVS_SORTDESCENDING))
          {
            return FALSE;
          } 

          if ((lpSubItem->pszText != NULL) && 
              (lpSubItem->pszText != LPSTR_TEXTCALLBACKA))
          {
            COMCTL32_Free(lpSubItem->pszText);
          }
    
          lpSubItem->pszText = LPSTR_TEXTCALLBACKA;
        }
        else 
        {
          if (lpSubItem->pszText == LPSTR_TEXTCALLBACKA)
          {
            lpSubItem->pszText = NULL;
          }
        
          bResult = Str_SetPtrA(&lpSubItem->pszText, lpLVItem->pszText);
        }
      }
    }
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Adds a subitem at a given position (column index).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] LPLVITEM : new subitem atttributes 
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_AddSubItem(HWND hwnd, LPLVITEMA lpLVItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_SUBITEM *lpSubItem = NULL;
  BOOL bResult = FALSE;
  HDPA hdpaSubItems;
  INT nPosition, nItem;

  if (lpLVItem != NULL)
  {
    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
    if (hdpaSubItems != NULL)
    {
      lpSubItem = (LISTVIEW_SUBITEM *)COMCTL32_Alloc(sizeof(LISTVIEW_SUBITEM));
      if (lpSubItem != NULL)
      {
        if (LISTVIEW_InitSubItem(hwnd, lpSubItem, lpLVItem) != FALSE)
        {
          nPosition = LISTVIEW_FindInsertPosition(hdpaSubItems, 
                                                  lpSubItem->iSubItem);
          nItem = DPA_InsertPtr(hdpaSubItems, nPosition, lpSubItem);
          if (nItem != -1)
          {
            bResult = TRUE;
          }            
        }
      }
    }
  }
  
  /* cleanup if unsuccessful */   
  if ((bResult == FALSE) && (lpSubItem != NULL))
  {
    COMCTL32_Free(lpSubItem);
  }
  
  return bResult;
}

/***
 * DESCRIPTION:
 * Finds the dpa insert position (array index).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : subitem index
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static INT LISTVIEW_FindInsertPosition(HDPA hdpaSubItems, INT nSubItem)
{
  LISTVIEW_SUBITEM *lpSubItem;
  INT i;

  for (i = 1; i < hdpaSubItems->nItemCount; i++)
  {
    lpSubItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, i);
    if (lpSubItem != NULL)
    {
      if (lpSubItem->iSubItem > nSubItem)
      {
        return i;
      }
    } 
  }

  return hdpaSubItems->nItemCount;
}

/***
 * DESCRIPTION:
 * Retrieves a listview subitem at a given position (column index).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : subitem index
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LISTVIEW_SUBITEM* LISTVIEW_GetSubItem(HDPA hdpaSubItems, INT nSubItem)
{
  LISTVIEW_SUBITEM *lpSubItem;
  INT i;

  for (i = 1; i < hdpaSubItems->nItemCount; i++)
  {
    lpSubItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, i);
    if (lpSubItem != NULL)
    {
      if (lpSubItem->iSubItem == nSubItem)
      {
        return lpSubItem;
      }
      else if (lpSubItem->iSubItem > nSubItem)
      {
        return NULL;
      }  
    }
  }
  
  return NULL;
}

/***
 * DESCRIPTION:
 * Sets item attributes.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] LPLVITEM : new item atttributes 
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItem(HWND hwnd, LPLVITEMA lpLVItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  BOOL bResult = FALSE;
  HDPA hdpaSubItems;
  LISTVIEW_ITEM *lpItem;
  NMLISTVIEW nmlv;
  UINT uChanged;
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);

  if (lpLVItem != NULL)
  {
    if (lpLVItem->iSubItem == 0)
    {
      hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
      if (hdpaSubItems != NULL)
      {
        lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, lpLVItem->iSubItem);
        if (lpItem != NULL)
        {
          ZeroMemory(&nmlv, sizeof(NMLISTVIEW));
          nmlv.hdr.hwndFrom = hwnd;
          nmlv.hdr.idFrom = lCtrlId;
          nmlv.hdr.code = LVN_ITEMCHANGING;
          nmlv.lParam = lpItem->lParam;
          uChanged = LISTVIEW_GetItemChanges(lpItem, lpLVItem);
          if (uChanged != 0)
          {
            if (uChanged & LVIF_STATE)
            {
              nmlv.uNewState = lpLVItem->state & lpLVItem->stateMask;
              nmlv.uOldState = lpItem->state & lpLVItem->stateMask;
            }

            nmlv.uChanged = uChanged;
            nmlv.iItem = lpLVItem->iItem;
            nmlv.lParam = lpItem->lParam;
            /* send LVN_ITEMCHANGING notification */
            ListView_LVNotify(GetParent(hwnd), lCtrlId, &nmlv);

            /* copy information */
            bResult = LISTVIEW_InitItem(hwnd, lpItem, lpLVItem);

            /* send LVN_ITEMCHANGED notification */
            nmlv.hdr.code = LVN_ITEMCHANGED;
            ListView_LVNotify(GetParent(hwnd), lCtrlId, &nmlv);
          }
          else
          {
            bResult = TRUE;
          }

          InvalidateRect(hwnd, NULL, FALSE);
        }
      }
    }
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Sets subitem attributes.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] LPLVITEM : new subitem atttributes 
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetSubItem(HWND hwnd, LPLVITEMA lpLVItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  BOOL bResult = FALSE;
  HDPA hdpaSubItems;
  LISTVIEW_SUBITEM *lpSubItem;

  if (lpLVItem != NULL)
  {
    if (lpLVItem->iSubItem > 0)
    {
      hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
      if (hdpaSubItems != NULL)
      {
        /* set subitem only if column is present */
        if (Header_GetItemCount(infoPtr->hwndHeader) > lpLVItem->iSubItem)
        {
          lpSubItem = LISTVIEW_GetSubItem(hdpaSubItems, lpLVItem->iSubItem);
          if (lpSubItem != NULL)
          {
            bResult = LISTVIEW_InitSubItem(hwnd, lpSubItem, lpLVItem);
          }
          else
          {
            bResult = LISTVIEW_AddSubItem(hwnd, lpLVItem);
          }
          
          InvalidateRect(hwnd, NULL, FALSE);
        } 
      }
    }
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Retrieves the index of the item at coordinate (0, 0) of the client area.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * item index
 */
static INT LISTVIEW_GetTopIndex(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *) GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nItem = 0;

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_LIST:
    if (lStyle & WS_HSCROLL)
    {
      nItem = GetScrollPos(hwnd, SB_HORZ) * infoPtr->nCountPerColumn;
    }
    break;

  case LVS_REPORT:
    if (lStyle & WS_VSCROLL)
    {
      nItem = GetScrollPos(hwnd, SB_VERT);
    }
    break;
  }

  return nItem;
}

/***
 * DESCRIPTION:
 * Evaluates if scrollbars are needed & sets the scroll range/position.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] LONG : window style
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_SetScroll(HWND hwnd, LONG lStyle)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
  INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
  INT nHScrollHeight = GetSystemMetrics(SM_CYHSCROLL);
  INT nVScrollWidth = GetSystemMetrics(SM_CXVSCROLL);
  INT nHiddenWidth;
  INT nHiddenHeight;
  INT nHiddenItemCount;
  INT nScrollPos;
  INT nMaxRange;
  INT nCountPerPage;
  INT nPixPerScrollPos;
  RECT rcView;

  TRACE(listview, "(hwnd=%x,lStyle=%lx)\n", hwnd, lStyle);

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_LIST:
    nCountPerPage = infoPtr->nCountPerRow * infoPtr->nCountPerColumn;
    if (nCountPerPage < GETITEMCOUNT(infoPtr))
    {
      /* display horizontal scrollbar */
      if ((lStyle &  WS_HSCROLL) == 0)
      {
	ShowScrollBar(hwnd, SB_HORZ, TRUE);
      }

      /* calculate new scrollbar range */
      nHiddenItemCount = GETITEMCOUNT(infoPtr) - nCountPerPage;
      if ((nHiddenItemCount % infoPtr->nCountPerColumn) == 0)
      {
        nMaxRange = nHiddenItemCount / infoPtr->nCountPerColumn;
      }
     else
      {
        nMaxRange = nHiddenItemCount / infoPtr->nCountPerColumn + 1;
      }
        
      SetScrollRange(hwnd, SB_HORZ, 0, nMaxRange, FALSE);
      nScrollPos = ListView_GetTopIndex(hwnd) / infoPtr->nCountPerColumn;
      SetScrollPos(hwnd, SB_HORZ, nScrollPos, TRUE);
    }
    else
    {
      /* hide scrollbar */
      if ((lStyle & WS_HSCROLL) != 0)
      {
	ShowScrollBar(hwnd, SB_HORZ, FALSE);
      }
    }
    break;
    
  case LVS_REPORT:
/*
 * This section was commented out because I experienced some problems
 * with the scrolling of the header control. The idea was to add a 
 * horizontal scrollbar when the width of the client area was smaller
 * than the width of the header control.
 */

/*     if (infoPtr->nItemWidth > nListWidth) */
/*     { */
/*       if ((lStyle &  WS_HSCROLL) == 0) */
/*       { */
/*         ShowScrollBar(hwnd, SB_HORZ, TRUE); */
/*         LISTVIEW_SetSize(hwnd, lStyle, -1, -1); */
/*         LISTVIEW_SetViewInfo(hwnd, lStyle); */
/*       } */
      
/*       nListWidth = infoPtr->rcList.right - infoPtr->rcList.left; */
/*       nHiddenWidth = infoPtr->nItemWidth - nListWidth; */
/*       nPixPerScrollPos = max(1, nListWidth / 10); */
      
/*       if ((nHiddenWidth % nPixPerScrollPos) == 0) */
/*       { */
/*         nMaxRange = nHiddenWidth / nPixPerScrollPos;  */
/*       } */
/*       else */
/*       { */
/*         nMaxRange = nHiddenWidth / nPixPerScrollPos + 1;  */
/*       } */

/*       SetScrollRange(hwnd, SB_HORZ, 0, nMaxRange, FALSE); */
/*       SetScrollPos(hwnd, SB_HORZ, 0, TRUE); */
/*     } */
/*     else */
/*     { */
/*       if ((lStyle &  WS_HSCROLL) != 0) */
/*       { */
/*         ShowScrollBar(hwnd, SB_HORZ, FASLE); */
/*         LISTVIEW_SetSize(hwnd, lStyle, -1, -1);  */
/*         LISTVIEW_SetViewInfo(hwnd, lStyle);  */
/*       } */
/*     } */
  
    if (infoPtr->nCountPerColumn < GETITEMCOUNT(infoPtr))
    {
      if ((lStyle &  WS_VSCROLL) == 0)
      {
        if (nListWidth > nVScrollWidth)
        {
          ShowScrollBar(hwnd, SB_VERT, TRUE);
          nListWidth -= nVScrollWidth;
        }
      }
        
      /* vertical range & position */
      nMaxRange = GETITEMCOUNT(infoPtr) - infoPtr->nCountPerColumn;
      SetScrollRange(hwnd, SB_VERT, 0, nMaxRange, FALSE);
      SetScrollPos(hwnd, SB_VERT, ListView_GetTopIndex(hwnd), TRUE);
    }
    else
    {
      if ((lStyle &  WS_VSCROLL) != 0)
      {
        ShowScrollBar(hwnd, SB_VERT, FALSE);
        nListWidth += nVScrollWidth;
      }
    }
    break;

  case LVS_ICON:
  case LVS_SMALLICON:
    if (LISTVIEW_GetViewRect(hwnd, &rcView) != FALSE)
    {
      if (rcView.right - rcView.left > nListWidth)
      {
        if ((lStyle & WS_HSCROLL) == 0)
        {
          if (nListHeight > nHScrollHeight)
          {
            ShowScrollBar(hwnd, SB_HORZ, TRUE);
            nListHeight -= nHScrollHeight;
          }
        }
 
        /* calculate size of hidden items */
        nHiddenWidth = rcView.right - rcView.left - nListWidth;
        nPixPerScrollPos = max(1, nListWidth / 10);

        /* vertical range & position */
        if ((nHiddenWidth % nPixPerScrollPos) == 0)
        {
          nMaxRange = nHiddenWidth / nPixPerScrollPos; 
        }
        else
        {
          nMaxRange = nHiddenWidth / nPixPerScrollPos + 1; 
        }

        /* set range and position */
        SetScrollRange(hwnd, SB_HORZ, 0, nMaxRange, FALSE);
        SetScrollPos(hwnd, SB_HORZ, 0, TRUE);
      }
      else
      {
        if ((lStyle &  WS_HSCROLL) != 0)
        {
          ShowScrollBar(hwnd, SB_HORZ, FALSE);
          nListHeight += nHScrollHeight;
        }
      }

      if (rcView.bottom - rcView.top > nListHeight)
      {
        if ((lStyle &  WS_VSCROLL) == 0)
        {
          if (nListWidth > nVScrollWidth)
          {
            ShowScrollBar(hwnd, SB_VERT, TRUE);
            nListWidth -= nVScrollWidth;
          }
        }
        
        /* calculate size of hidden items */
        nHiddenHeight = rcView.bottom - rcView.top - nListHeight;
        nPixPerScrollPos = max(1, nListHeight / 10);
        
        /* set vertical range & position */
        if ((nHiddenHeight % nPixPerScrollPos) == 0)
        {
          nMaxRange = nHiddenHeight / nPixPerScrollPos; 
        }
        else
        {
          nMaxRange = nHiddenHeight / nPixPerScrollPos + 1; 
        }
          
        /* set range and position */
        SetScrollRange(hwnd, SB_VERT, 0, nMaxRange, FALSE);
        SetScrollPos(hwnd, SB_VERT, 0, TRUE);
      }
      else
      {
        if ((lStyle &  WS_VSCROLL) != 0)
        {
          ShowScrollBar(hwnd, SB_VERT, FALSE);
          nListWidth += nVScrollWidth;
        }
      }
    }
    break;
  }
}

/***
 * DESCRIPTION:
 * Draws a subitem.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] HDC : device context handle
 * [I] INT : item index
 * [I] LPARAM : item lparam 
 * [I] LISTVIEW_SUBITEM * : item
 * [I] INT : column index (header index)
 * [I] RECT * : clipping rectangle
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_DrawSubItem(HWND hwnd, HDC hdc, INT nItem, LPARAM lParam,
                                 LISTVIEW_SUBITEM *lpSubItem, INT nColumn, 
                                 RECT *lprc)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  CHAR szDispText[DISP_TEXT_SIZE];
  LPSTR pszDispText = NULL;

  /* set item colors */
  SetBkColor(hdc, infoPtr->clrTextBk);
  SetTextColor(hdc, infoPtr->clrText);
  
  pszDispText = szDispText;
  LISTVIEW_GetSubItemDispInfo(hwnd, nItem, lParam, lpSubItem, nColumn, NULL, 
                              &pszDispText, DISP_TEXT_SIZE);

  /* draw text : using arbitrary offset of 10 pixels */  
  
  ExtTextOutA(hdc, lprc->left, lprc->top, ETO_OPAQUE|ETO_CLIPPED, 
              lprc, pszDispText, lstrlenA(pszDispText), NULL);
}

/***
 * DESCRIPTION:
 * Draws an item.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] HDC : device context handle
 * [I] LISTVIEW_ITEM * : item
 * [I] INT : item index
 * [I] RECT * : clipping rectangle
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_DrawItem(HWND hwnd, HDC hdc, LISTVIEW_ITEM *lpItem, 
                              INT nItem, RECT rc)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0); 
  CHAR szDispText[DISP_TEXT_SIZE];
  LPSTR pszDispText = NULL;
  BOOL bSelected;
  INT nLabelWidth;
  INT nImage;
  UINT uState;

  TRACE(listview, "(hwnd=%x,hdc=%x,lpItem=%p,nItem=%d,rc.left=%d,rctop=%d,rc.right=%d,rc.bottom=%d)\n", hwnd, hdc, lpItem, nItem, rc.left, rc.top, rc.right, rc.bottom);

  pszDispText = szDispText;
  LISTVIEW_GetItemDispInfo(hwnd, nItem, lpItem, &nImage, &uState, &pszDispText,
                           DISP_TEXT_SIZE);
 
 if (uState & LVIS_SELECTED)
  {
    bSelected = TRUE;

    /* set item colors */ 
    SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
    SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
    
    /* set raster mode */
    SetROP2(hdc, R2_XORPEN);
  }
  else
  {
    bSelected = FALSE;
    
    /* set item colors */
    SetBkColor(hdc, infoPtr->clrTextBk);
    SetTextColor(hdc, infoPtr->clrText);
    
    /* set raster mode */
    SetROP2(hdc, R2_COPYPEN);
  }

  /* state icons */
  if (infoPtr->himlState != NULL)
  {
    /* right shift 12 bits to obtain index in image list */
    if (bSelected != FALSE)
    {
      ImageList_Draw(infoPtr->himlState, uState >> 12, hdc, rc.left, 
                     rc.top, ILD_SELECTED);
    }
    else
    {
      ImageList_Draw(infoPtr->himlState, uState >> 12, hdc, rc.left, 
                     rc.top, ILD_NORMAL);
    }
    
    rc.left += infoPtr->iconSize.cx; 
  }
  
  /* small icons */
  if (infoPtr->himlSmall != NULL)
  {
    if (bSelected != FALSE)
    {
      ImageList_Draw(infoPtr->himlSmall, nImage, hdc, rc.left, 
                     rc.top, ILD_SELECTED);
    }
    else
    {
      ImageList_Draw(infoPtr->himlSmall, nImage, hdc, rc.left, 
                     rc.top, ILD_NORMAL);
    }
    
    rc.left += infoPtr->iconSize.cx; 
  }
  
  nLabelWidth = ListView_GetStringWidthA(hwnd, pszDispText);
  if (rc.left + nLabelWidth < rc.right)
  {
    rc.right = rc.left + nLabelWidth;
  } 

  /* draw label */  
  ExtTextOutA(hdc, rc.left, rc.top, ETO_OPAQUE|ETO_CLIPPED, 
              &rc, pszDispText, lstrlenA(pszDispText), NULL);
  
  if (lpItem->state & LVIS_FOCUSED)
  {
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom); 
  }
}

/***
 * DESCRIPTION:
 * Draws an item when in large icon display mode.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] HDC : device context handle
 * [I] LISTVIEW_ITEM * : item
 * [I] INT : item index
 * [I] RECT * : clipping rectangle
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_DrawLargeItem(HWND hwnd, HDC hdc, LISTVIEW_ITEM *lpItem, 
                                   INT nItem, RECT rc)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0); 
  CHAR szDispText[DISP_TEXT_SIZE];
  LPSTR pszDispText = NULL;
  BOOL bSelected;
  INT nLabelWidth;
  INT nImage;
  UINT uState;
  INT nDrawPosX = 0;
  TEXTMETRICA tm;

  TRACE(listview, "(hwnd=%x,hdc=%x,lpItem=%p,nItem=%d,rc.left=%d,rctop=%d,rc.right=%d,rc.bottom=%d)\n", hwnd, hdc, lpItem, nItem, rc.left, rc.top, rc.right, rc.bottom);

  pszDispText = szDispText;
  LISTVIEW_GetItemDispInfo(hwnd, nItem, lpItem, &nImage, &uState, &pszDispText,
                           DISP_TEXT_SIZE);
  if (uState & LVIS_SELECTED)
  {
    bSelected = TRUE;

    /* set item colors */ 
    SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
    SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
    
    /* set raster mode */
    SetROP2(hdc, R2_XORPEN);
  }
  else
  {
    bSelected = FALSE;

    /* set item colors */
    SetBkColor(hdc, infoPtr->clrTextBk);
    SetTextColor(hdc, infoPtr->clrText);
    
    /* set raster mode */
    SetROP2(hdc, R2_COPYPEN);
  }

  if (infoPtr->himlNormal != NULL)
  {
    rc.top += ICON_TOP_PADDING;
    nDrawPosX = rc.left + (infoPtr->iconSpacing.cx - infoPtr->iconSize.cx) / 2;
    if (bSelected != FALSE)
    {
      ImageList_Draw(infoPtr->himlNormal, nImage, hdc, nDrawPosX, rc.top, 
                     ILD_SELECTED);
    }
    else
    {
      ImageList_Draw(infoPtr->himlNormal, nImage, hdc, nDrawPosX, rc.top, 
                     ILD_NORMAL);
    }
  }

  rc.top += infoPtr->iconSize.cy + ICON_BOTTOM_PADDING; 
  nLabelWidth = ListView_GetStringWidthA(hwnd, pszDispText);
  nDrawPosX = infoPtr->iconSpacing.cx - nLabelWidth;
  if (nDrawPosX > 1)
  {
    rc.left += nDrawPosX / 2;
    rc.right = rc.left + nLabelWidth;
  }
  else
  {
    rc.left += 1;
    rc.right = rc.left + infoPtr->iconSpacing.cx - 1;
  }

  /* draw label */  
  GetTextMetricsA(hdc, &tm);
  rc.bottom = rc.top + tm.tmHeight + HEIGHT_PADDING; 
  ExtTextOutA(hdc, rc.left, rc.top, ETO_OPAQUE|ETO_CLIPPED, 
              &rc, pszDispText, lstrlenA(pszDispText), NULL);
        
  if (lpItem->state & LVIS_FOCUSED)
  {
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom); 
  }
}

/***
 * DESCRIPTION:
 * Draws listview items when in report display mode.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] HDC : device context handle 
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_RefreshReport(HWND hwnd, HDC hdc)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd,0);
  INT nDrawPosY = infoPtr->rcList.top;
  LISTVIEW_ITEM *lpItem;
  LISTVIEW_SUBITEM *lpSubItem = NULL;
  BOOL bNeedSubItem = TRUE;
  INT nColumnCount;
  HDPA hdpaSubItems;
  RECT rcItem;
  INT  j, k;
  INT nItem;
  INT nLast;

  nItem = ListView_GetTopIndex(hwnd);
  nLast = nItem + infoPtr->nCountPerColumn;
  while (nItem <= nLast)
  {
    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem);
    if (hdpaSubItems != NULL)
    {
      lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
      if (lpItem != NULL)
      {
        /* the width of the header items will determine the size of the 
           listview items */
        Header_GetItemRect(infoPtr->hwndHeader, 0, &rcItem);
        rcItem.left += REPORT_MARGINX;
        rcItem.right = max(rcItem.left, rcItem.right - REPORT_MARGINX);
        rcItem.top = nDrawPosY;
        rcItem.bottom = rcItem.top + infoPtr->nItemHeight;
        LISTVIEW_DrawItem(hwnd, hdc, lpItem, nItem, rcItem);
      }

      nColumnCount = Header_GetItemCount(infoPtr->hwndHeader);
      for (k = 1, j = 1; j < nColumnCount; j++)
      {
        Header_GetItemRect(infoPtr->hwndHeader, j, &rcItem);
        rcItem.left += REPORT_MARGINX;
        rcItem.right = max(rcItem.left, rcItem.right - REPORT_MARGINX);
        rcItem.top = nDrawPosY;
        rcItem.bottom = rcItem.top + infoPtr->nItemHeight;

        if (k < hdpaSubItems->nItemCount)
        {
          if (bNeedSubItem != FALSE)
          {
            lpSubItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, k);
            k++;
          }

          if (lpSubItem != NULL)
          {
            if (lpSubItem->iSubItem == j)
            {
              LISTVIEW_DrawSubItem(hwnd, hdc, nItem, lpItem->lParam, lpSubItem,
                                   j, &rcItem);
              bNeedSubItem = TRUE;
            }
            else
            {
              LISTVIEW_DrawSubItem(hwnd, hdc, nItem, lpItem->lParam, NULL, j, 
                                   &rcItem);
              bNeedSubItem = FALSE;
            }
          }
          else
          {
            LISTVIEW_DrawSubItem(hwnd, hdc, nItem, lpItem->lParam, NULL, j, 
                                 &rcItem);
            bNeedSubItem = TRUE;
          }
        }
        else
        {
          LISTVIEW_DrawSubItem(hwnd, hdc, nItem, lpItem->lParam, NULL, j, 
                               &rcItem);
        }
      }
    }

    nDrawPosY += infoPtr->nItemHeight;
    nItem++;
  }
}

/***
 * DESCRIPTION:
 * Draws listview items when in list display mode.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] HDC : device context handle 
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_RefreshList(HWND hwnd, HDC hdc)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_ITEM *lpItem;
  HDPA hdpaSubItems;
  RECT rc;
  INT i, j;
  INT nColumnCount;
  INT nItem = ListView_GetTopIndex(hwnd);

  if (infoPtr->rcList.right > 0)
  {
    /* get number of display columns */
    if (infoPtr->rcList.right % infoPtr->nItemWidth == 0)
    {
      nColumnCount = infoPtr->rcList.right / infoPtr->nItemWidth;
    }
    else
    {
      nColumnCount = infoPtr->rcList.right / infoPtr->nItemWidth + 1;
    }

    for (i = 0; i < nColumnCount; i++)
    {
      j = 0;
      while ((nItem < GETITEMCOUNT(infoPtr)) && 
             (j<infoPtr->nCountPerColumn))
      {
        hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem);
        if (hdpaSubItems != NULL)
        {
          lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
          if (lpItem != NULL)
          {
            rc.top = j * infoPtr->nItemHeight;
            rc.left = i * infoPtr->nItemWidth;
            rc.bottom = rc.top + infoPtr->nItemHeight;
            rc.right = rc.left + infoPtr->nItemWidth;
            LISTVIEW_DrawItem(hwnd, hdc,  lpItem, nItem, rc);
          }
        }
 
        nItem++;
        j++;
      }
    }
  }
}

/***
 * DESCRIPTION:
 * Draws listview items when in icon or small icon display mode.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] HDC : device context handle 
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_RefreshIcon(HWND hwnd, HDC hdc, BOOL bSmall)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_ITEM *lpItem;
  HDPA hdpaSubItems;
  POINT ptPosition;
  POINT ptOrigin;
  RECT rc;
  INT i;

  LISTVIEW_GetOrigin(hwnd, &ptOrigin);

  for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
  {
    LISTVIEW_GetItemPosition(hwnd, i, &ptPosition);
    ptPosition.x += ptOrigin.x;
    ptPosition.y += ptOrigin.y;

    if (ptPosition.y + infoPtr->nItemHeight > infoPtr->rcList.top)
    {
      if (ptPosition.x + infoPtr->nItemWidth > infoPtr->rcList.left)
      {
        if (ptPosition.y < infoPtr->rcList.bottom)
        {
          if (ptPosition.x < infoPtr->rcList.right)
          {
            hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, i);
            if (hdpaSubItems != NULL)
            {
              lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
              if (lpItem != NULL)
              {
                rc.top = ptPosition.y;
                rc.left = ptPosition.x;
                rc.bottom = rc.top + infoPtr->nItemHeight;
                rc.right = rc.left + infoPtr->nItemWidth;
                if (bSmall == FALSE)
                {
                  LISTVIEW_DrawLargeItem(hwnd, hdc, lpItem, i, rc);
                }
                else
                {
                  LISTVIEW_DrawItem(hwnd, hdc, lpItem, i, rc);
                }
              }
            }
          }
        }
      }
    }
  }
}

/***
 * DESCRIPTION:
 * Draws listview items.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] HDC : device context handle 
 *
 * RETURN:
 * NoneX
 */
static VOID LISTVIEW_Refresh(HWND hwnd, HDC hdc)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  HFONT hOldFont;
  HPEN hPen, hOldPen;

  /* select font */
  hOldFont = SelectObject(hdc, infoPtr->hFont);

  /* select the doted pen (for drawing the focus box) */
  hPen = CreatePen(PS_DOT, 1, 0);
  hOldPen = SelectObject(hdc, hPen);

  /* select transparent brush (for drawing the focus box) */
  SelectObject(hdc, GetStockObject(NULL_BRUSH)); 

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_LIST:
    LISTVIEW_RefreshList(hwnd, hdc); 
    break;
  case LVS_REPORT:
    LISTVIEW_RefreshReport(hwnd, hdc);
    break;
  case LVS_SMALLICON:
    LISTVIEW_RefreshIcon(hwnd, hdc, TRUE);
    break;
  case LVS_ICON:
    LISTVIEW_RefreshIcon(hwnd, hdc, FALSE);
  }

  /* unselect objects */
  SelectObject(hdc, hOldFont);
  SelectObject(hdc, hOldPen);
  
  /* delete pen */
  DeleteObject(hPen);
}


/***
 * DESCRIPTION:
 * Calculates the approximate width and height of a given number of items.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : number of items
 * [I] INT : width
 * [I] INT : height
 *
 * RETURN:
 * Returns a DWORD. The width in the low word and the height in high word.
 */
static LRESULT LISTVIEW_ApproximateViewRect(HWND hwnd, INT nItemCount, 
                                            WORD wWidth, WORD wHeight)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nItemCountPerColumn = 1;
  INT nColumnCount = 0;
  DWORD dwViewRect = 0;

  if (nItemCount == -1)
    nItemCount = GETITEMCOUNT(infoPtr);

  if (lStyle & LVS_LIST)
  {
    if (wHeight == 0xFFFF)
    {
      /* use current height */
      wHeight = infoPtr->rcList.bottom;
    }

    if (wHeight < infoPtr->nItemHeight)
    {
      wHeight = infoPtr->nItemHeight;
    }

    if (nItemCount > 0)
    {
      if (infoPtr->nItemHeight > 0)
      {
        nItemCountPerColumn = wHeight / infoPtr->nItemHeight;
        if (nItemCountPerColumn == 0)
          nItemCountPerColumn = 1;

        if (nItemCount % nItemCountPerColumn != 0)
          nColumnCount = nItemCount / nItemCountPerColumn;
        else
          nColumnCount = nItemCount / nItemCountPerColumn + 1;
      }
    }

    /* Microsoft padding magic */
    wHeight = nItemCountPerColumn * infoPtr->nItemHeight + 2;
    wWidth = nColumnCount * infoPtr->nItemWidth + 2;

    dwViewRect = MAKELONG(wWidth, wHeight);
  }
  else if (lStyle & LVS_REPORT)
  {
    /* TO DO */
    }
  else if (lStyle & LVS_SMALLICON)
  {
    /* TO DO */
  }
  else if (lStyle & LVS_ICON)
  {
    /* TO DO */
  }
 
  return dwViewRect;
}

/***
 * DESCRIPTION:
 * Arranges listview items in icon display mode.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : alignment code
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_Arrange(HWND hwnd, INT nAlignCode)
{
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  BOOL bResult = FALSE;

  if (((LVS_TYPEMASK & lStyle) == LVS_ICON) || 
      ((LVS_TYPEMASK & lStyle) == LVS_SMALLICON))
  {
    switch (nAlignCode)
    {
    case LVA_ALIGNLEFT:
      /* TO DO */
      break;
    case LVA_ALIGNTOP:
      /* TO DO */
      break;
    case LVA_DEFAULT:
      /* TO DO */
      break;
    case LVA_SNAPTOGRID:
      /* TO DO */
      break;
    }
  }

  return bResult;
}

/* << LISTVIEW_CreateDragImage >> */

/***
 * DESCRIPTION:
 * Removes all listview items and subitems.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_DeleteAllItems(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  LISTVIEW_ITEM *lpItem;
  LISTVIEW_SUBITEM *lpSubItem;
  NMLISTVIEW nmlv;
  BOOL bSuppress;
  BOOL bResult = FALSE;
  INT i;
  INT j;
  HDPA hdpaSubItems;

  TRACE(listview, "(hwnd=%x,)\n", hwnd);

  if (GETITEMCOUNT(infoPtr) > 0)
  {
    /* initialize memory */
    ZeroMemory(&nmlv, sizeof(NMLISTVIEW));
    
    /* send LVN_DELETEALLITEMS notification */
    nmlv.hdr.hwndFrom = hwnd;
    nmlv.hdr.idFrom = lCtrlId;
    nmlv.hdr.code = LVN_DELETEALLITEMS;
    nmlv.iItem = -1;

    /* verify if subsequent LVN_DELETEITEM notifications should be 
       suppressed */
    bSuppress = ListView_LVNotify(GetParent(hwnd), lCtrlId, &nmlv);

    for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
    {
      hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, i);
      if (hdpaSubItems != NULL)
      {
        for (j = 1; j < hdpaSubItems->nItemCount; j++)
        {
          lpSubItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, j);
          if (lpSubItem != NULL)
          {
            /* free subitem string */
            if ((lpSubItem->pszText != NULL) && 
                (lpSubItem->pszText != LPSTR_TEXTCALLBACKA))
            {
              COMCTL32_Free(lpSubItem->pszText);
            }
            
            /* free subitem */
            COMCTL32_Free(lpSubItem);
          }    
        }
    
        lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
        if (lpItem != NULL)
        {
          if (bSuppress == FALSE)
          {
            /* send LVN_DELETEITEM notification */
            nmlv.hdr.code = LVN_DELETEITEM;
            nmlv.iItem = i;
            nmlv.lParam = lpItem->lParam;
            ListView_LVNotify(GetParent(hwnd), lCtrlId, &nmlv);
          }

          /* free item string */
          if ((lpItem->pszText != NULL) && 
              (lpItem->pszText != LPSTR_TEXTCALLBACKA))
          {
            COMCTL32_Free(lpItem->pszText);
          }
          
          /* free item */
          COMCTL32_Free(lpItem);
        }
        
        DPA_Destroy(hdpaSubItems);
      }
    }

    /* reinitialize listview memory */
    bResult = DPA_DeleteAllPtrs(infoPtr->hdpaItems);
    
    /* align items (set position of each item) */
    switch (lStyle & LVS_TYPEMASK)
    {
    case LVS_ICON:
    case LVS_SMALLICON:
      if (lStyle & LVS_ALIGNLEFT)
      {
        LISTVIEW_AlignLeft(hwnd);
      }
      else
      {
        LISTVIEW_AlignTop(hwnd);
      }
      break;
    }
    
    LISTVIEW_SetScroll(hwnd, lStyle);

    /* invalidate client area (optimization needed) */
    InvalidateRect(hwnd, NULL, TRUE);
  }
  
  return bResult;
}

/***
 * DESCRIPTION:
 * Removes a column from the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : column index
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_DeleteColumn(HWND hwnd, INT nColumn)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  BOOL bResult = FALSE;
  
  if (Header_DeleteItem(infoPtr->hwndHeader, nColumn) != FALSE)
  {
    bResult = LISTVIEW_RemoveColumn(infoPtr->hdpaItems, nColumn);

    /* reset scroll parameters */
    if ((lStyle & LVS_TYPEMASK) == LVS_REPORT)
    {
      LISTVIEW_SetViewInfo(hwnd, lStyle);
      LISTVIEW_SetScroll(hwnd, lStyle);
    }

    /* refresh client area */
    InvalidateRect(hwnd, NULL, FALSE);
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Removes an item from the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index  
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_DeleteItem(HWND hwnd, INT nItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMLISTVIEW nmlv;
  BOOL bResult = FALSE;
  HDPA hdpaSubItems;
  LISTVIEW_ITEM *lpItem;
  LISTVIEW_SUBITEM *lpSubItem;
  INT i;

  TRACE(listview, "(hwnd=%x,nItem=%d)\n", hwnd, nItem);

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
  {
    /* initialize memory */
    ZeroMemory(&nmlv, sizeof(NMLISTVIEW));

    hdpaSubItems = (HDPA)DPA_DeletePtr(infoPtr->hdpaItems, nItem);
    if (hdpaSubItems != NULL)
    {
      for (i = 1; i < hdpaSubItems->nItemCount; i++)
      {
        lpSubItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, i);
        if (lpSubItem != NULL)
        {
          /* free item string */
          if ((lpSubItem->pszText != NULL) && 
              (lpSubItem->pszText != LPSTR_TEXTCALLBACKA))
          {
            COMCTL32_Free(lpSubItem->pszText);
          }
          
          /* free item */
          COMCTL32_Free(lpSubItem);
        }    
      }
    
      lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
      if (lpItem != NULL)
      {
        /* send LVN_DELETEITEM notification */
        nmlv.hdr.hwndFrom = hwnd;
        nmlv.hdr.idFrom = lCtrlId;
        nmlv.hdr.code = LVN_DELETEITEM;
        nmlv.iItem = nItem;
        nmlv.lParam = lpItem->lParam;
        SendMessageA(GetParent(hwnd), WM_NOTIFY, (WPARAM)lCtrlId, 
                     (LPARAM)&nmlv);

        /* free item string */
        if ((lpItem->pszText != NULL) && 
            (lpItem->pszText != LPSTR_TEXTCALLBACKA))
        {
          COMCTL32_Free(lpItem->pszText);
        }
        
        /* free item */
        COMCTL32_Free(lpItem);
      }
      
      bResult = DPA_Destroy(hdpaSubItems);
    }

    /* align items (set position of each item) */
    switch(lStyle & LVS_TYPEMASK)
    {
    case LVS_ICON:
    case LVS_SMALLICON:
      if (lStyle & LVS_ALIGNLEFT)
      {
        LISTVIEW_AlignLeft(hwnd);
      }
      else
      {
        LISTVIEW_AlignTop(hwnd);
      }
      break;
    }

    LISTVIEW_SetScroll(hwnd, lStyle);

    /* refresh client area */
    InvalidateRect(hwnd, NULL, TRUE);
  }
  
  return bResult;
}

/* LISTVIEW_EditLabel */

/***
 * DESCRIPTION:
 * Ensures the specified item is visible, scrolling into view if necessary.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index
 * [I] BOOL : partially or entirely visible
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_EnsureVisible(HWND hwnd, INT nItem, BOOL bPartial)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
  INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
  INT nHScrollPos = 0;
  INT nVScrollPos = 0;
  INT nScrollPosHeight = 0;
  INT nScrollPosWidth = 0;
  RECT rcItem;
  BOOL bResult = FALSE;

  /* ALWAYS bPartial == FALSE, FOR NOW! */

  rcItem.left = LVIR_BOUNDS;
  if (LISTVIEW_GetItemRect(hwnd, nItem, &rcItem) != FALSE)
  {
    if (rcItem.left < infoPtr->rcList.left)
    {
      /* scroll left */
      switch (LVS_TYPEMASK & lStyle)
      {
      case LVS_LIST:
        rcItem.left += infoPtr->rcList.left;
        nScrollPosWidth = infoPtr->nItemWidth;
        break;

      case LVS_SMALLICON:
      case LVS_ICON:
        nScrollPosWidth = max(1, nListWidth / 10);
        rcItem.left += infoPtr->rcList.left;
        break;
      }

      if (rcItem.left % nScrollPosWidth == 0)
      {
        nHScrollPos = rcItem.left / nScrollPosWidth;
      }
      else
      {
        nHScrollPos = rcItem.left / nScrollPosWidth - 1;
      }
    }
    else if (rcItem.right > infoPtr->rcList.right)
    {
      /* scroll right */
      switch (LVS_TYPEMASK & lStyle)
      {
      case LVS_LIST:
        rcItem.right -= infoPtr->rcList.right;
        nScrollPosWidth = infoPtr->nItemWidth;
        break;

      case LVS_SMALLICON:
      case LVS_ICON:
        nScrollPosWidth = max(1, nListWidth / 10);
        rcItem.right -= infoPtr->rcList.right;
        break;
      }

      if (rcItem.right % nScrollPosWidth == 0)
      {
        nHScrollPos = rcItem.right / nScrollPosWidth;
      }
      else
      {
        nHScrollPos = rcItem.right / nScrollPosWidth + 1;
      }
    }
    
    if (rcItem.top < infoPtr->rcList.top)
    {
      /* scroll up */
      switch (LVS_TYPEMASK & lStyle)
      {
      case LVS_REPORT:
        rcItem.top -= infoPtr->rcList.top; 
        nScrollPosHeight = infoPtr->nItemHeight;
        break;

      case LVS_SMALLICON:
      case LVS_ICON:
        nScrollPosHeight = max(1, nListHeight / 10);
        rcItem.top += infoPtr->rcList.top;
        break;
      }

      if (rcItem.top % nScrollPosHeight == 0)
      {
        nVScrollPos = rcItem.top / nScrollPosHeight;
      }
      else
      {
        nVScrollPos = rcItem.top / nScrollPosHeight - 1;
      }
    }
    else if (rcItem.bottom > infoPtr->rcList.bottom)
    {
      switch (LVS_TYPEMASK & lStyle)
      {
      case LVS_REPORT:
        rcItem.bottom -= infoPtr->rcList.bottom;
        nScrollPosHeight = infoPtr->nItemHeight;
        break;

      case LVS_SMALLICON:
      case LVS_ICON:
        nScrollPosHeight = max(1, nListHeight / 10);
        rcItem.bottom -= infoPtr->rcList.bottom;
        break;
      }

      if (rcItem.bottom % nScrollPosHeight == 0)
      {
        nVScrollPos = rcItem.bottom / nScrollPosHeight;
      }
      else
      {
        nVScrollPos = rcItem.bottom / nScrollPosHeight + 1;
      }
    }

    bResult = LISTVIEW_ScrollView(hwnd, nHScrollPos, nVScrollPos);
  }
  
  return bResult;
}

/***
 * DESCRIPTION:
 * Searches for an item with specific characteristics.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : base item index
 * [I] LPLVFINDINFO : item information to look for
 * 
 * RETURN:
 *   SUCCESS : index of item
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_FindItem(HWND hwnd, INT nStart, 
                                 LPLVFINDINFO lpFindInfo)
{
  FIXME (listview, "empty stub!\n");

  return -1; 
}

/***
 * DESCRIPTION:
 * Retrieves the background color of the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * COLORREF associated with the background.
 */
static LRESULT LISTVIEW_GetBkColor(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

  return infoPtr->clrBk;
}

/***
 * DESCRIPTION:
 * Retrieves the background image of the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [O] LPLVMKBIMAGE : background image attributes
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE`
 */
/* static LRESULT LISTVIEW_GetBkImage(HWND hwnd, LPLVBKIMAGE lpBkImage)   */
/* {   */
/*   FIXME (listview, "empty stub!\n"); */
/*   return FALSE;   */
/* }   */

/***
 * DESCRIPTION:
 * Retrieves the callback mask.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 *  Value of mask
 */
static UINT LISTVIEW_GetCallbackMask(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

  return infoPtr->uCallbackMask;
}

/***
 * DESCRIPTION:
 * Retrieves column attributes.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT :  column index
 * [IO] LPLVCOLUMNA : column information
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_GetColumnA(HWND hwnd, INT nItem, 
                                     LPLVCOLUMNA lpColumn)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  HDITEMA hdi;
  BOOL bResult = FALSE;
  
  if (lpColumn != NULL)
  {
    /* initialize memory */
    ZeroMemory(&hdi, sizeof(HDITEMA));
    
    if (lpColumn->mask & LVCF_FMT)
    {
      hdi.mask |= HDI_FORMAT;
    }

    if (lpColumn->mask & LVCF_WIDTH)
    {
      hdi.mask |= HDI_WIDTH;
    }

    if (lpColumn->mask & LVCF_TEXT)
    {
      hdi.mask |= (HDI_TEXT | HDI_FORMAT);
    }

    if (lpColumn->mask & LVCF_IMAGE)
    {
      hdi.mask |= HDI_IMAGE;
    }

    if (lpColumn->mask & LVCF_ORDER)
    {
      hdi.mask |= HDI_ORDER;
    }

    bResult = Header_GetItemA(infoPtr->hwndHeader, nItem, &hdi);
    if (bResult != FALSE)
    {
      if (lpColumn->mask & LVCF_FMT) 
      {
        lpColumn->fmt = 0;

        if (hdi.fmt & HDF_LEFT)
        {
          lpColumn->fmt |= LVCFMT_LEFT;
        }
        else if (hdi.fmt & HDF_RIGHT)
        {
          lpColumn->fmt |= LVCFMT_RIGHT;
        }
        else if (hdi.fmt & HDF_CENTER)
        {
          lpColumn->fmt |= LVCFMT_CENTER;
        }

        if (hdi.fmt & HDF_IMAGE)
        {
          lpColumn->fmt |= LVCFMT_COL_HAS_IMAGES;
        }
      }

      if (lpColumn->mask & LVCF_WIDTH)
      {
        lpColumn->cx = hdi.cxy;
      }
      
      if ((lpColumn->mask & LVCF_TEXT) && (lpColumn->pszText) && (hdi.pszText))
      {
        lstrcpynA (lpColumn->pszText, hdi.pszText, lpColumn->cchTextMax);
      }

      if (lpColumn->mask & LVCF_IMAGE)
      {
        lpColumn->iImage = hdi.iImage;
      }

      if (lpColumn->mask & LVCF_ORDER)
      {
        lpColumn->iOrder = hdi.iOrder;
      }
    }
  }

  return bResult;
}

/* LISTVIEW_GetColumnW */
/* LISTVIEW_GetColumnOrderArray */

/***
 * DESCRIPTION:
 * Retrieves the column width.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] int : column index
 *
 * RETURN:
 *   SUCCESS : column width
 *   FAILURE : zero 
 */
static LRESULT LISTVIEW_GetColumnWidth(HWND hwnd, INT nColumn)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  HDITEMA hdi;
  INT nColumnWidth = 0;

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_LIST:
    nColumnWidth = infoPtr->nItemWidth;
    break;

  case LVS_REPORT:
    /* get column width from header */
    ZeroMemory(&hdi, sizeof(HDITEMA));
    hdi.mask = HDI_WIDTH;
    if (Header_GetItemA(infoPtr->hwndHeader, nColumn, &hdi) != FALSE)
    {
      nColumnWidth = hdi.cxy;
    }
    break;
  }

  return nColumnWidth;
}

/***
 * DESCRIPTION:
 * In list or report display mode, retrieves the number of items that can fit 
 * vertically in the visible area. In icon or small icon display mode, 
 * retrieves the total number of visible items.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * Number of fully visible items.
 */
static LRESULT LISTVIEW_GetCountPerPage(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nItemCount = 0;

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_LIST:
    if (infoPtr->rcList.right / infoPtr->nItemWidth)
    {
      nItemCount = infoPtr->nCountPerRow * infoPtr->nCountPerColumn;
    }
    break;
  
  case LVS_REPORT:
    nItemCount = infoPtr->nCountPerColumn;
    break;
    
  case LVS_SMALLICON:
  case LVS_ICON:
    nItemCount = GETITEMCOUNT(infoPtr);
    break;
  }

  return nItemCount;
}

/* LISTVIEW_GetEditControl */
/* LISTVIEW_GetExtendedListViewStyle */

/***
 * DESCRIPTION:
 * Retrieves the handle to the header control.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * Header handle.
 */
static LRESULT LISTVIEW_GetHeader(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

  return infoPtr->hwndHeader;
}

/* LISTVIEW_GetHotCursor */
/* LISTVIEW_GetHotItem */
/* LISTVIEW_GetHoverTime */

/***
 * DESCRIPTION:
 * Retrieves an image list handle.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : image list identifier 
 * 
 * RETURN:
 *   SUCCESS : image list handle
 *   FAILURE : NULL
 */
static LRESULT LISTVIEW_GetImageList(HWND hwnd, INT nImageList)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  HIMAGELIST himl = NULL;

  switch (nImageList) 
  {
  case LVSIL_NORMAL:
    himl = infoPtr->himlNormal;
    break;
  case LVSIL_SMALL:
    himl = infoPtr->himlSmall;
    break;
  case LVSIL_STATE:
    himl = infoPtr->himlState;
    break;
  }

  return (LRESULT)himl;
}

/* LISTVIEW_GetISearchString */

/***
 * DESCRIPTION:
 * Retrieves item attributes.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [IO] LPLVITEMA : item info
 * 
 * RETURN:
 *   SUCCESS : TRUE 
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_GetItemA(HWND hwnd, LPLVITEMA lpLVItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_ITEM *lpItem;
  LISTVIEW_SUBITEM *lpSubItem;
  HDPA hdpaSubItems;
  BOOL bResult = FALSE;

  TRACE(listview, "(hwnd=%x,lpLVItem=%p)\n", hwnd, lpLVItem);

  if (lpLVItem != NULL)
  {
    if ((lpLVItem->iItem >= 0) && (lpLVItem->iItem < GETITEMCOUNT(infoPtr)))
    {
      hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
      if (hdpaSubItems != NULL)
      {
        if (lpLVItem->iSubItem == 0)
        {
          lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
          if (lpItem != NULL)
          {
            bResult = TRUE;

            /* retrieve valid data */
            if (lpLVItem->mask & LVIF_STATE)
            {
              lpLVItem->state = lpItem->state & lpLVItem->stateMask;
            }

            if (lpLVItem->mask & LVIF_TEXT) 
            {
              if (lpItem->pszText == LPSTR_TEXTCALLBACKA)
              {
                lpLVItem->pszText = LPSTR_TEXTCALLBACKA;
              }
              else
              {
                bResult = Str_GetPtrA(lpItem->pszText, lpLVItem->pszText, 
                                        lpLVItem->cchTextMax);
              }
            }

            if (lpLVItem->mask & LVIF_IMAGE)
            {
              lpLVItem->iImage = lpItem->iImage;
            }
          
            if (lpLVItem->mask & LVIF_PARAM)
            {
              lpLVItem->lParam = lpItem->lParam;
            }
      
            if (lpLVItem->mask & LVIF_INDENT)
            {
              lpLVItem->iIndent = lpItem->iIndent;
            }
          }
        }
        else
        {
          lpSubItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, 
                                                     lpLVItem->iSubItem);
          if (lpSubItem != NULL)
          {
            bResult = TRUE;

            if (lpLVItem->mask & LVIF_TEXT) 
            {
              if (lpSubItem->pszText == LPSTR_TEXTCALLBACKA)
              {
                lpLVItem->pszText = LPSTR_TEXTCALLBACKA;
              }
              else
              {
                bResult = Str_GetPtrA(lpSubItem->pszText, lpLVItem->pszText, 
                                        lpLVItem->cchTextMax);
              }
            }

            if (lpLVItem->mask & LVIF_IMAGE)
            {
              lpLVItem->iImage = lpSubItem->iImage;
            }
          }
        }
      }
    }
  }

  return bResult;
}

/* LISTVIEW_GetItemW */
/* LISTVIEW_GetHotCursor */
/* LISTVIEW_GetHotItem */
/* LISTVIEW_GetHoverTime> */

/***
 * DESCRIPTION:
 * Retrieves the number of items in the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * 
 * RETURN:
 * Number of items.
 */
static LRESULT LISTVIEW_GetItemCount(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

  return GETITEMCOUNT(infoPtr);
}

/***
 * DESCRIPTION:
 * Retrieves the position (upper-left) of the listview control item.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index
 * [O] LPPOINT : coordinate information
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_GetItemPosition(HWND hwnd, INT nItem, 
                                     LPPOINT lpptPosition)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  BOOL bResult = FALSE;
  HDPA hdpaSubItems;
  LISTVIEW_ITEM *lpItem;
  INT nRow;

  TRACE(listview, "(hwnd=%x,nItem=%d,lpptPosition=%p)\n", hwnd, nItem, 
        lpptPosition);

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)) && 
      (lpptPosition != NULL))
  {
    switch (LVS_TYPEMASK & lStyle)
    {
    case LVS_LIST:
      bResult = TRUE;
      nItem = nItem - ListView_GetTopIndex(hwnd);
      if (nItem < 0)
      {
        nRow = nItem % infoPtr->nCountPerColumn;
        if (nRow == 0)
        {
          lpptPosition->x = (nItem / infoPtr->nCountPerColumn * 
                             infoPtr->nItemWidth);
          lpptPosition->y = 0;
        }
        else
        {
          lpptPosition->x = ((nItem / infoPtr->nCountPerColumn - 1) * 
                             infoPtr->nItemWidth);
          lpptPosition->y = ((nRow + infoPtr->nCountPerColumn) * 
                             infoPtr->nItemHeight);  
        }
      }
      else
      {
        lpptPosition->x = (nItem / infoPtr->nCountPerColumn * 
                           infoPtr->nItemWidth);
        lpptPosition->y = (nItem % infoPtr->nCountPerColumn * 
                           infoPtr->nItemHeight);
      }
      break;

    case LVS_REPORT:
      bResult = TRUE;
      lpptPosition->x = REPORT_MARGINX;
      lpptPosition->y = ((nItem - ListView_GetTopIndex(hwnd)) * 
                         infoPtr->nItemHeight) + infoPtr->rcList.top;
      break;

    case LVS_SMALLICON:
    case LVS_ICON:
      hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem);
      if (hdpaSubItems != NULL)
      {
        lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
        if (lpItem != NULL)
        {
          bResult = TRUE;
          lpptPosition->x = lpItem->ptPosition.x;
          lpptPosition->y = lpItem->ptPosition.y;
        }
      }
      break;
    }
  }
    
  return bResult;
}

/***
 * DESCRIPTION:
 * Retrieves the bounding rectangle for a listview control item.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index
 * [IO] LPRECT : bounding rectangle coordinates
 * 
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_GetItemRect(HWND hwnd, INT nItem, LPRECT lprc)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  BOOL bResult = FALSE;
  POINT ptOrigin;
  POINT ptItem;
  HDC hdc;
  HFONT hOldFont;
  INT nMaxWidth;
  INT nLabelWidth;
  TEXTMETRICA tm;

  TRACE(listview, "(hwnd=%x,nItem=%d,lprc=%p)\n", hwnd, nItem, lprc);

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)) && (lprc != NULL))
  {
    if (ListView_GetItemPosition(hwnd, nItem, &ptItem) != FALSE)
    {
      switch(lprc->left)  
      {  
      case LVIR_ICON: 
        switch (LVS_TYPEMASK & lStyle)
        {
        case LVS_ICON:
          if (infoPtr->himlNormal != NULL)
          {
            if (LISTVIEW_GetOrigin(hwnd, &ptOrigin) != FALSE)
            {
              bResult = TRUE;
              lprc->left = ptItem.x + ptOrigin.x;
              lprc->top = ptItem.y + ptOrigin.y;
              lprc->right = lprc->left + infoPtr->iconSize.cx;
              lprc->bottom = (lprc->top + infoPtr->iconSize.cy + 
                              ICON_BOTTOM_PADDING + ICON_TOP_PADDING);
            }
          }
          break;

        case LVS_SMALLICON:
          if (LISTVIEW_GetOrigin(hwnd, &ptOrigin) != FALSE)
          {
            bResult = TRUE;
            lprc->left = ptItem.x + ptOrigin.x;
            lprc->top = ptItem.y + ptOrigin.y;
            lprc->bottom = lprc->top + infoPtr->nItemHeight;

            if (infoPtr->himlState != NULL)
            {
              lprc->left += infoPtr->iconSize.cx;
            }
              
            if (infoPtr->himlSmall != NULL)
            {
              lprc->right = lprc->left + infoPtr->iconSize.cx;
            }
            else
            {
              lprc->right = lprc->left;
            }
          }
          break;

        case LVS_REPORT:
        case LVS_LIST:
          bResult = TRUE;
          lprc->left = ptItem.x;
          lprc->top = ptItem.y;
          lprc->bottom = lprc->top + infoPtr->nItemHeight;

          if (infoPtr->himlState != NULL)
          {
            lprc->left += infoPtr->iconSize.cx;
          }
            
          if (infoPtr->himlSmall != NULL)
          {
            lprc->right = lprc->left + infoPtr->iconSize.cx;
          }
          else
          {
            lprc->right = lprc->left;
          }
          break;
        }
        break;

      case LVIR_LABEL: 
        switch (LVS_TYPEMASK & lStyle)
        {
        case LVS_ICON:
          if (infoPtr->himlNormal != NULL)
          {
            if (LISTVIEW_GetOrigin(hwnd, &ptOrigin) != FALSE)
            {
              bResult = TRUE;
              lprc->left = ptItem.x + ptOrigin.x;
              lprc->top = (ptItem.y + ptOrigin.y + infoPtr->iconSize.cy +
                           ICON_BOTTOM_PADDING + ICON_TOP_PADDING);
              nLabelWidth = LISTVIEW_GetLabelWidth(hwnd, nItem);
              if (infoPtr->iconSpacing.cx - nLabelWidth > 1)
              {
                lprc->left += (infoPtr->iconSpacing.cx - nLabelWidth) / 2;
                lprc->right = lprc->left + nLabelWidth;
              }
              else
              {
                lprc->left += 1;
                lprc->right = lprc->left + infoPtr->iconSpacing.cx - 1;
              }
            
              hdc = GetDC(hwnd);
              hOldFont = SelectObject(hdc, infoPtr->hFont);
              GetTextMetricsA(hdc, &tm);
              lprc->bottom = lprc->top + tm.tmHeight + HEIGHT_PADDING;
              SelectObject(hdc, hOldFont);
              ReleaseDC(hwnd, hdc);
            }              
          }
          break;

        case LVS_SMALLICON:
          if (LISTVIEW_GetOrigin(hwnd, &ptOrigin) != FALSE)
          {
            bResult = TRUE;
            nMaxWidth = lprc->left = ptItem.x + ptOrigin.x; 
            lprc->top = ptItem.y + ptOrigin.y;
            lprc->bottom = lprc->top + infoPtr->nItemHeight;
            
            if (infoPtr->himlState != NULL)
            {
              lprc->left += infoPtr->iconSize.cx;
            }
            
            if (infoPtr->himlSmall != NULL)
            {
              lprc->left += infoPtr->iconSize.cx;
            }
            
            nLabelWidth = LISTVIEW_GetLabelWidth(hwnd, nItem);
            if (lprc->left + nLabelWidth < nMaxWidth + infoPtr->nItemWidth)
            {
              lprc->right = lprc->left + nLabelWidth;
            }
            else
            {
              lprc->right = nMaxWidth + infoPtr->nItemWidth;
            }
          }
          break;

        case LVS_REPORT:
        case LVS_LIST:
          bResult = TRUE;
          lprc->left = ptItem.x; 
          lprc->top = ptItem.y;
          lprc->bottom = lprc->top + infoPtr->nItemHeight;

          if (infoPtr->himlState != NULL)
          {
            lprc->left += infoPtr->iconSize.cx;
          }

          if (infoPtr->himlSmall != NULL)
          {
            lprc->left += infoPtr->iconSize.cx;
          }

          lprc->right = lprc->left + LISTVIEW_GetLabelWidth(hwnd, nItem);
          break;
        }
        break;

      case LVIR_BOUNDS:
        switch (LVS_TYPEMASK & lStyle)
        {
        case LVS_ICON:
          if (infoPtr->himlNormal != NULL)
          {
            if (LISTVIEW_GetOrigin(hwnd, &ptOrigin) != FALSE)
            {
              bResult = TRUE;
              lprc->left = ptItem.x + ptOrigin.x;
              lprc->top = ptItem.y + ptOrigin.y; 
              lprc->right = lprc->left + infoPtr->iconSpacing.cx;
              lprc->bottom = lprc->top + infoPtr->iconSpacing.cy;
            }
          } 
          break;

        case LVS_SMALLICON:
          if (LISTVIEW_GetOrigin(hwnd, &ptOrigin) != FALSE)
          {
            bResult = TRUE;
            lprc->left = ptItem.x +ptOrigin.x; 
            lprc->right = lprc->left;
            lprc->top = ptItem.y + ptOrigin.y;
            lprc->bottom = lprc->top + infoPtr->nItemHeight;
            
            if (infoPtr->himlState != NULL)
            {
              lprc->right += infoPtr->iconSize.cx;
            }
            
            if (infoPtr->himlSmall != NULL)
            {
              lprc->right += infoPtr->iconSize.cx;
            }
            
            lprc->right += LISTVIEW_GetLabelWidth(hwnd, nItem);
          }
          break;

        case LVS_REPORT:
        case LVS_LIST:
          bResult = TRUE;
          lprc->left = ptItem.x; 
          lprc->right = lprc->left;
          lprc->top = ptItem.y;
          lprc->bottom = lprc->top + infoPtr->nItemHeight;

          if (infoPtr->himlState != NULL)
          {
            lprc->right += infoPtr->iconSize.cx;
          }

          if (infoPtr->himlSmall != NULL)
          {
            lprc->right += infoPtr->iconSize.cx;
          }

          lprc->right += LISTVIEW_GetLabelWidth(hwnd, nItem);
          break;
        }
        break;
        
      case LVIR_SELECTBOUNDS:
        switch (LVS_TYPEMASK & lStyle)
        {
        case LVS_ICON:
          if (infoPtr->himlNormal != NULL)
          {
            if (LISTVIEW_GetOrigin(hwnd, &ptOrigin) != FALSE)
            {
              bResult = TRUE;
              lprc->left = ptItem.x + ptOrigin.x;
              lprc->top = ptItem.y + ptOrigin.y;
              lprc->right = lprc->left + infoPtr->iconSpacing.cx;
              lprc->bottom = lprc->top + infoPtr->iconSpacing.cy;
            }
          } 
          break;

        case LVS_SMALLICON:
          if (LISTVIEW_GetOrigin(hwnd, &ptOrigin) != FALSE)
          {
            bResult = TRUE;
            lprc->left = ptItem.x + ptOrigin.x;  
            lprc->top = ptItem.y + ptOrigin.y;
            lprc->bottom = lprc->top + infoPtr->nItemHeight;
            
            if (infoPtr->himlState != NULL)
            {
              lprc->left += infoPtr->iconSize.cx;
            }
            
            lprc->right = lprc->left;
            
            if (infoPtr->himlSmall != NULL)
            {
              lprc->right += infoPtr->iconSize.cx;
            }
            
            lprc->right += LISTVIEW_GetLabelWidth(hwnd, nItem);
          }
          break;

        case LVS_REPORT:
        case LVS_LIST:
          bResult = TRUE;
          lprc->left = ptItem.x; 
          lprc->top = ptItem.y;
          lprc->bottom = lprc->top + infoPtr->nItemHeight;

          if (infoPtr->himlState != NULL)
          {
            lprc->left += infoPtr->iconSize.cx;
          }
          
          lprc->right = lprc->left;
        
          if (infoPtr->himlSmall != NULL)
          {
            lprc->right += infoPtr->iconSize.cx;
          }

          lprc->right += LISTVIEW_GetLabelWidth(hwnd, nItem);
          break;
        }
        break;
      }
    }
  }
        
  return bResult;
}

/***
 * DESCRIPTION:
 * Retrieves the width of a label.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * 
 * RETURN:
 *   SUCCESS : string width (in pixels)
 *   FAILURE : zero
 */

static INT LISTVIEW_GetLabelWidth(HWND hwnd, INT nItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_ITEM *lpItem;
  HDPA hdpaSubItems;
  INT nLabelWidth = 0;

  TRACE(listview, "(hwnd=%x,nItem=%d)\n", hwnd, nItem);

  hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem);
  if (hdpaSubItems != NULL)
  {
    lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
    if (lpItem != NULL)
    {
      CHAR szDispText[DISP_TEXT_SIZE];
      LPSTR pszDispText = NULL;
      pszDispText = szDispText;
      LISTVIEW_GetItemDispInfo(hwnd, nItem, lpItem, NULL, NULL, &pszDispText,
                               DISP_TEXT_SIZE);
      nLabelWidth = ListView_GetStringWidthA(hwnd, pszDispText);
    }
  }

  return nLabelWidth;
}

/***
 * DESCRIPTION:
 * Retrieves the spacing between listview control items.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] BOOL : flag for small or large icon 
 *
 * RETURN:
 * Horizontal + vertical spacing
 */
static LRESULT LISTVIEW_GetItemSpacing(HWND hwnd, BOOL bSmall)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lResult;

  if (bSmall == FALSE)
  {
    lResult = MAKELONG(infoPtr->iconSpacing.cx, infoPtr->iconSpacing.cy);
  }
  else
  {
    /* TODO: need to store width of smallicon item */
    lResult = MAKELONG(0, infoPtr->nItemHeight);
  }  
  
  return lResult;
}

/***
 * DESCRIPTION:
 * Retrieves the state of a listview control item.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index
 * [I] UINT : state mask
 * 
 * RETURN:
 * State specified by the mask.
 */
static LRESULT LISTVIEW_GetItemState(HWND hwnd, INT nItem, UINT uMask)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LVITEMA lvItem;
  UINT uState = 0;

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
  {
    ZeroMemory(&lvItem, sizeof(LVITEMA));
    lvItem.iItem = nItem;
    lvItem.stateMask = uMask;
    lvItem.mask = LVIF_STATE;
    if (ListView_GetItemA(hwnd, &lvItem) != FALSE)
    {
      uState = lvItem.state;
    }
  }

  return uState;
}

/***
 * DESCRIPTION:
 * Retrieves the text of a listview control item or subitem. 
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index
 * [IO] LPLVITEMA : item information
 * 
 * RETURN:
 * None
 */
static LRESULT LISTVIEW_GetItemTextA(HWND hwnd, INT nItem, LPLVITEMA lpLVItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_ITEM *lpItem;
  LISTVIEW_SUBITEM *lpSubItem;
  HDPA hdpaSubItems;
  INT nLength = 0;
  
  if (lpLVItem != NULL)
  {
    if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
    {
      hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
      if (hdpaSubItems != NULL)
      {
        if (lpLVItem->iSubItem == 0)
        {
          lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
          if (lpItem != NULL)
          {
            if (lpLVItem->mask & LVIF_TEXT) 
            {
              if (lpItem->pszText == LPSTR_TEXTCALLBACKA)
              {
                lpLVItem->pszText = LPSTR_TEXTCALLBACKA;
              }
              else
              {
                nLength = Str_GetPtrA(lpItem->pszText, lpLVItem->pszText, 
                                      lpLVItem->cchTextMax);
              }
            }
          }
        }
        else
        {
          lpSubItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, 
                                                     lpLVItem->iSubItem);
          if (lpSubItem != NULL)
          {
            if (lpLVItem->mask & LVIF_TEXT) 
            {
              if (lpSubItem->pszText == LPSTR_TEXTCALLBACKA)
              {
                lpLVItem->pszText = LPSTR_TEXTCALLBACKA;
              }
              else
              {
                nLength = Str_GetPtrA(lpSubItem->pszText, lpLVItem->pszText, 
                                      lpLVItem->cchTextMax);
              }
            }
          }
        }
      }
    }
  }

  return nLength;
}

/***
 * DESCRIPTION:
 * Searches for an item based on properties + relationships.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index
 * [I] UINT : relationship flag
 * 
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_GetNextItem(HWND hwnd, INT nItem, UINT uFlags)
{
  FIXME (listview, "empty stub!\n");

  return -1;
}

/* LISTVIEW_GetNumberOfWorkAreas */

/***
 * DESCRIPTION:
 * Retrieves the origin coordinates when in icon or small icon display mode.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [O] LPPOINT : coordinate information
 * 
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_GetOrigin(HWND hwnd, LPPOINT lpptOrigin)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
  INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
  BOOL bResult = FALSE;

  TRACE(listview, "(hwnd=%x,lpptOrigin=%p)\n", hwnd, lpptOrigin);

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_ICON:
  case LVS_SMALLICON:
    if ((lStyle & WS_HSCROLL) != 0)
    {
      lpptOrigin->x = -GetScrollPos(hwnd, SB_HORZ) * nListWidth / 10;
    }
    else
    {
      lpptOrigin->x = 0;
    }

    if ((lStyle & WS_VSCROLL) != 0)
    {
      lpptOrigin->y = -GetScrollPos(hwnd, SB_VERT) * nListHeight / 10;
    }
    else
    {
      lpptOrigin->y = 0;
    }
  
    bResult = TRUE;
    break;
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Retrieves the number of items that are marked as selected.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * 
 * RETURN:
 * Number of items selected.
 */
static LRESULT LISTVIEW_GetSelectedCount(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nSelectedCount = 0;
  INT i;

  for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
  {
    if (ListView_GetItemState(hwnd, i, LVIS_SELECTED) & LVIS_SELECTED)
    {
      nSelectedCount++;
    }
  }

  return nSelectedCount;
}

/***
 * DESCRIPTION:
 * Retrieves item index that marks the start of a multiple selection.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * 
 * RETURN:
 * Index number or -1 if there is no selection mark.
 */
static LRESULT LISTVIEW_GetSelectionMark(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

  return infoPtr->nSelectionMark;
}

/***
 * DESCRIPTION:
 * Retrieves the width of a string.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * 
 * RETURN:
 *   SUCCESS : string width (in pixels)
 *   FAILURE : zero
 */
static LRESULT LISTVIEW_GetStringWidthA(HWND hwnd, LPCSTR lpszText)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  HFONT hFont, hOldFont;
  SIZE stringSize;
  HDC hdc;

  ZeroMemory(&stringSize, sizeof(SIZE));
  if (lpszText != NULL)
  {
    hFont = infoPtr->hFont ? infoPtr->hFont : infoPtr->hDefaultFont;
    hdc = GetDC(hwnd);
    hOldFont = SelectObject(hdc, hFont);
    GetTextExtentPointA(hdc, lpszText, lstrlenA(lpszText), &stringSize);
    SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);
  }

  return stringSize.cx;
}

/***
 * DESCRIPTION:
 * Retrieves the text backgound color.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * 
 * RETURN:
 * COLORREF associated with the the background.
 */
static LRESULT LISTVIEW_GetTextBkColor(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLongA(hwnd, 0);

  return infoPtr->clrTextBk;
}

/***
 * DESCRIPTION:
 * Retrieves the text color.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * 
 * RETURN:
 * COLORREF associated with the text.
 */
static LRESULT LISTVIEW_GetTextColor(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLongA(hwnd, 0);

  return infoPtr->clrText;
}

/***
 * DESCRIPTION:
 * Set the bounding rectangle of all the items.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] LPRECT : bounding rectangle
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetViewRect(HWND hwnd, LPRECT lprcView)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  BOOL bResult = FALSE;

  TRACE(listview, "(hwnd=%x,lprcView->left=%d,lprcView->top=%d,lprcView->right=%d,lprcView->bottom=%d)\n", hwnd, lprcView->left, lprcView->top, lprcView->right, lprcView->bottom);

  if (lprcView != NULL)
  {
    switch (lStyle & LVS_TYPEMASK)
    {
    case LVS_ICON:
    case LVS_SMALLICON:
      bResult = TRUE;
      infoPtr->rcView.left = lprcView->left;
      infoPtr->rcView.top = lprcView->top;
      infoPtr->rcView.right = lprcView->right;
      infoPtr->rcView.bottom = lprcView->bottom;
      break;
    }
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Retrieves the bounding rectangle of all the items.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [O] LPRECT : bounding rectangle
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_GetViewRect(HWND hwnd, LPRECT lprcView)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLongA(hwnd, 0);
  BOOL bResult = FALSE;
  POINT ptOrigin;

  TRACE(listview, "(hwnd=%x,lprcView=%p)\n", hwnd, lprcView);

  if (lprcView != NULL)
  {
    bResult = LISTVIEW_GetOrigin(hwnd, &ptOrigin);
    if (bResult != FALSE)
    {
      lprcView->left = infoPtr->rcView.left + ptOrigin.x;
      lprcView->top = infoPtr->rcView.top + ptOrigin.y;
      lprcView->right = infoPtr->rcView.right + ptOrigin.x;
      lprcView->bottom = infoPtr->rcView.bottom + ptOrigin.y;
    }

    TRACE(listview, "(lprcView->left=%d,lprcView->top=%d,lprcView->right=%d,lprcView->bottom=%d)\n", lprcView->left, lprcView->top, lprcView->right, lprcView->bottom);

  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Determines which section of the item was selected (if any).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [IO] LPLVHITTESTINFO : hit test information
 *
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static INT LISTVIEW_HitTestItem(HWND hwnd, LPLVHITTESTINFO lpHitTestInfo)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  RECT rcItem;
  INT i;

  for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
  {
    rcItem.left = LVIR_BOUNDS;
    if (LISTVIEW_GetItemRect(hwnd, i, &rcItem) != FALSE)
    {
      rcItem.left = LVIR_ICON;
      if (LISTVIEW_GetItemRect(hwnd, i, &rcItem) != FALSE)
      {
        if ((lpHitTestInfo->pt.x >= rcItem.left) && 
            (lpHitTestInfo->pt.x <= rcItem.right) && 
            (lpHitTestInfo->pt.y >= rcItem.top) &&
            (lpHitTestInfo->pt.y <= rcItem.bottom))
        {
          lpHitTestInfo->flags = LVHT_ONITEMICON | LVHT_ONITEM;
          lpHitTestInfo->iItem = i;
          lpHitTestInfo->iSubItem = 0;
          return i;
        }
      }
      
      rcItem.left = LVIR_LABEL;
      if (LISTVIEW_GetItemRect(hwnd, i, &rcItem) != FALSE)
      {
        if ((lpHitTestInfo->pt.x >= rcItem.left) && 
            (lpHitTestInfo->pt.x <= rcItem.right) &&
            (lpHitTestInfo->pt.y >= rcItem.top) &&
            (lpHitTestInfo->pt.y <= rcItem.bottom))
        {
          lpHitTestInfo->flags = LVHT_ONITEMLABEL | LVHT_ONITEM;
          lpHitTestInfo->iItem = i;
          lpHitTestInfo->iSubItem = 0;
          return i;
        }
      }

      lpHitTestInfo->flags = LVHT_ONITEMSTATEICON | LVHT_ONITEM;
      lpHitTestInfo->iItem = i;
      lpHitTestInfo->iSubItem = 0;
      return i;
    }
  }
     
  lpHitTestInfo->flags = LVHT_NOWHERE;

  return -1;
}

/***
 * DESCRIPTION:
 * Determines wich listview item is located at the specified position.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [IO} LPLVHITTESTINFO : hit test information
 *
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_HitTest(HWND hwnd, LPLVHITTESTINFO lpHitTestInfo)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nItem = -1;

  lpHitTestInfo->flags = 0;
  if (infoPtr->rcList.left > lpHitTestInfo->pt.x)
  {
    lpHitTestInfo->flags = LVHT_TOLEFT;
  }
  else if (infoPtr->rcList.right < lpHitTestInfo->pt.x) 
  {
    lpHitTestInfo->flags = LVHT_TORIGHT;
  }
  
  if (infoPtr->rcList.top > lpHitTestInfo->pt.y)
  {
    lpHitTestInfo->flags |= LVHT_ABOVE;
  } 
  else if (infoPtr->rcList.bottom < lpHitTestInfo->pt.y) 
  {
    lpHitTestInfo->flags |= LVHT_BELOW;
  }

  if (lpHitTestInfo->flags == 0)
  {
    nItem = LISTVIEW_HitTestItem(hwnd, lpHitTestInfo);
  }
  
  return nItem;
}

/***
 * DESCRIPTION:
 * Inserts a new column.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : column index
 * [I] LPLVCOLUMNA : column information
 *
 * RETURN:
 *   SUCCESS : new column index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_InsertColumnA(HWND hwnd, INT nColumn, 
                                      LPLVCOLUMNA lpColumn)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  HDITEMA hdi;
  INT nNewColumn = -1;

  TRACE(listview,"(hwnd=%x,nColumn=%d,lpColumn=%p)\n",hwnd, nColumn, lpColumn);

  if (lpColumn != NULL) 
  {
    /* initialize memory */
    ZeroMemory(&hdi, sizeof(HDITEMA));

    if (lpColumn->mask & LVCF_FMT) 
    {
      /* format member is valid */
      hdi.mask |= HDI_FORMAT;

      /* set text alignment (leftmost column must be left-aligned) */
      if (nColumn == 0)
      {
        hdi.fmt |= HDF_LEFT;
      }
      else
      {
        if (lpColumn->fmt & LVCFMT_LEFT)
        {
          hdi.fmt |= HDF_LEFT;
        }
        else if (lpColumn->fmt & LVCFMT_RIGHT)
        {
          hdi.fmt |= HDF_RIGHT;
        }
        else if (lpColumn->fmt & LVCFMT_CENTER)
        {
          hdi.fmt |= HDF_CENTER;
        }
      }
 
      if (lpColumn->fmt & LVCFMT_BITMAP_ON_RIGHT)
      {
        hdi.fmt |= HDF_BITMAP_ON_RIGHT;
        /* ??? */
      }

      if (lpColumn->fmt & LVCFMT_COL_HAS_IMAGES)
      {
        /* ??? */
      }
      
      if (lpColumn->fmt & LVCFMT_IMAGE)
      {
        hdi.fmt |= HDF_IMAGE;
        hdi.iImage = I_IMAGECALLBACK;
      }
    }

    if (lpColumn->mask & LVCF_WIDTH) 
    {
      hdi.mask |= HDI_WIDTH;
      hdi.cxy = lpColumn->cx;
    }
  
    if (lpColumn->mask & LVCF_TEXT) 
    {
      hdi.mask |= HDI_TEXT | HDI_FORMAT;
      hdi.pszText = lpColumn->pszText;
      hdi.cchTextMax = lstrlenA(lpColumn->pszText);
      hdi.fmt |= HDF_STRING;
    }
  
    if (lpColumn->mask & LVCF_IMAGE) 
    {
      hdi.mask |= HDI_IMAGE;
      hdi.iImage = lpColumn->iImage;
    }

    if (lpColumn->mask & LVCF_ORDER) 
    {
      hdi.mask |= HDI_ORDER;
      hdi.iOrder = lpColumn->iOrder;
    }

    /* insert item in header control */
    nNewColumn = SendMessageA(infoPtr->hwndHeader, HDM_INSERTITEMA,
                             (WPARAM)nColumn, (LPARAM)&hdi);

    LISTVIEW_SetScroll(hwnd, lStyle);
    InvalidateRect(hwnd, NULL, FALSE);
  }

  return nNewColumn;
}

/* LISTVIEW_InsertColumnW  */

/***
 * DESCRIPTION:
 * Inserts a new item in the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] LPLVITEMA : item information
 *
 * RETURN:
 *   SUCCESS : new item index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_InsertItemA(HWND hwnd, LPLVITEMA lpLVItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMLISTVIEW nmlv;
  INT nItem = -1;
  HDPA hdpaSubItems;
  LISTVIEW_ITEM *lpItem = NULL;

  TRACE(listview, "(hwnd=%x,lpLVItem=%p)\n", hwnd, lpLVItem);

  if (lpLVItem != NULL)
  {
    /* make sure it's not a subitem; cannot insert a subitem */
    if (lpLVItem->iSubItem == 0)
    {
      lpItem = (LISTVIEW_ITEM *)COMCTL32_Alloc(sizeof(LISTVIEW_ITEM));
      if (lpItem != NULL)
      {
        ZeroMemory(lpItem, sizeof(LISTVIEW_ITEM));
        if (LISTVIEW_InitItem(hwnd, lpItem, lpLVItem) != FALSE)
        {
          /* insert item in listview control data structure */
          hdpaSubItems = DPA_Create(8);
          if (hdpaSubItems != NULL)
          {
            nItem = DPA_InsertPtr(hdpaSubItems, 0, lpItem);
            if (nItem != -1)
            {
              nItem = DPA_InsertPtr(infoPtr->hdpaItems, lpLVItem->iItem, 
                                    hdpaSubItems);
              if (nItem != -1)
              {
                /* manage item focus */
                if (lpLVItem->mask & LVIF_STATE)
                {
                  if (lpLVItem->stateMask & LVIS_FOCUSED)
                  {
                    LISTVIEW_SetItemFocus(hwnd, nItem);
                  }           
                }
                
                /* send LVN_INSERTITEM notification */
                ZeroMemory(&nmlv, sizeof(NMLISTVIEW));
                nmlv.hdr.hwndFrom = hwnd;
                nmlv.hdr.idFrom = lCtrlId;
                nmlv.hdr.code = LVN_INSERTITEM;
                nmlv.iItem = nItem;
                nmlv.lParam = lpItem->lParam;;
                ListView_LVNotify(GetParent(hwnd), lCtrlId, &nmlv);
                
                /* align items (set position of each item) */
                switch (lStyle & LVS_TYPEMASK)
                {
                case LVS_ICON:
                case LVS_SMALLICON:
                  if (lStyle & LVS_ALIGNLEFT)
                  {
                    LISTVIEW_AlignLeft(hwnd);
                  }
                  else
                  {
                    LISTVIEW_AlignTop(hwnd);
                  }
                  break;
                }

                LISTVIEW_SetScroll(hwnd, lStyle);
                /* refresh client area */
                InvalidateRect(hwnd, NULL, FALSE);
              }
            }
          }
        }
      }
    }
  }

  /* free memory if unsuccessful */
  if ((nItem == -1) && (lpItem != NULL))
  {
    COMCTL32_Free(lpItem);
  }
  
  return nItem;
}

/* LISTVIEW_InsertItemW */

/***
 * DESCRIPTION:
 * Redraws a range of items.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : first item
 * [I] INT : last item
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_RedrawItems(HWND hwnd, INT nFirst, INT nLast)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0); 
  BOOL bResult = FALSE;
  RECT rc;

  if (nFirst <= nLast)
  {
    if ((nFirst >= 0) && (nFirst < GETITEMCOUNT(infoPtr)))
    {
      if ((nLast >= 0) && (nLast < GETITEMCOUNT(infoPtr)))
      {
        /* bResult = */
        InvalidateRect(hwnd, &rc, FALSE);
      }
    }
  }

  return bResult;
}

/* LISTVIEW_Scroll */

/***
 * DESCRIPTION:
 * Sets the background color.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] COLORREF : background color
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetBkColor(HWND hwnd, COLORREF clrBk)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

  infoPtr->clrBk = clrBk;
  InvalidateRect(hwnd, NULL, TRUE);
  
  return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the callback mask. This mask will be used when the parent
 * window stores state information (some or all).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] UINT : state mask
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetCallbackMask(HWND hwnd, UINT uMask)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

  infoPtr->uCallbackMask = uMask;

  return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the attributes of a header item.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : column index
 * [I] LPLVCOLUMNA : column attributes
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetColumnA(HWND hwnd, INT nColumn, 
                                   LPLVCOLUMNA lpColumn)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  BOOL bResult = FALSE;
  HDITEMA hdi;

  if ((lpColumn != NULL) && (nColumn >= 0) && 
      (nColumn < Header_GetItemCount(infoPtr->hwndHeader)))
  {
    /* initialize memory */
    ZeroMemory(&hdi, sizeof(HDITEMA));

    if (lpColumn->mask & LVCF_FMT) 
    {
      /* format member is valid */
      hdi.mask |= HDI_FORMAT;

      /* set text alignment (leftmost column must be left-aligned) */
      if (nColumn == 0)
      {
        hdi.fmt |= HDF_LEFT;
      }
      else
      {
        if (lpColumn->fmt & LVCFMT_LEFT)
        {
          hdi.fmt |= HDF_LEFT;
        }
        else if (lpColumn->fmt & LVCFMT_RIGHT)
        {
          hdi.fmt |= HDF_RIGHT;
        }
        else if (lpColumn->fmt & LVCFMT_CENTER)
        {
          hdi.fmt |= HDF_CENTER;
        }
      }
      
      if (lpColumn->fmt & LVCFMT_BITMAP_ON_RIGHT)
      {
        hdi.fmt |= HDF_BITMAP_ON_RIGHT;
      }

      if (lpColumn->fmt & LVCFMT_COL_HAS_IMAGES)
      {
        hdi.fmt |= HDF_IMAGE;
      }
      
      if (lpColumn->fmt & LVCFMT_IMAGE)
      {
        hdi.fmt |= HDF_IMAGE;
        hdi.iImage = I_IMAGECALLBACK;
      }
    }

    if (lpColumn->mask & LVCF_WIDTH) 
    {
      hdi.mask |= HDI_WIDTH;
      hdi.cxy = lpColumn->cx;
    }
    
    if (lpColumn->mask & LVCF_TEXT) 
    {
      hdi.mask |= HDI_TEXT | HDI_FORMAT;
      hdi.pszText = lpColumn->pszText;
      hdi.cchTextMax = lstrlenA(lpColumn->pszText);
      hdi.fmt |= HDF_STRING;
    }
  
    if (lpColumn->mask & LVCF_IMAGE) 
    {
      hdi.mask |= HDI_IMAGE;
      hdi.iImage = lpColumn->iImage;
    }

    if (lpColumn->mask & LVCF_ORDER) 
    {
      hdi.mask |= HDI_ORDER;
      hdi.iOrder = lpColumn->iOrder;
    }

    /* set header item attributes */
    bResult = Header_SetItemA(infoPtr->hwndHeader, nColumn, &hdi);
  }
  
  return bResult;
}

/***
 * DESCRIPTION:
 * Sets image lists.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : image list type  
 * [I] HIMAGELIST : image list handle
 *
 * RETURN:
 *   SUCCESS : old image list
 *   FAILURE : NULL
 */
static LRESULT LISTVIEW_SetImageList(HWND hwnd, INT nType, HIMAGELIST himl)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  HIMAGELIST himlTemp = 0;

  switch (nType) 
  {
  case LVSIL_NORMAL:
    himlTemp = infoPtr->himlNormal;
    infoPtr->himlNormal = himl;
    return (LRESULT)himlTemp;

  case LVSIL_SMALL:
    himlTemp = infoPtr->himlSmall;
    infoPtr->himlSmall = himl;
    return (LRESULT)himlTemp;

  case LVSIL_STATE:
    himlTemp = infoPtr->himlState;
    infoPtr->himlState = himl;
    return (LRESULT)himlTemp;
  }

  return (LRESULT)NULL;
}


/***
 * DESCRIPTION:
 * Sets the attributes of an item.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] LPLVITEM : item information 
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetItemA(HWND hwnd, LPLVITEMA lpLVItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  BOOL bResult = FALSE;

  if (lpLVItem != NULL)
  {
    if ((lpLVItem->iItem >= 0) && (lpLVItem->iItem < GETITEMCOUNT(infoPtr)))
    {
      if (lpLVItem->iSubItem == 0)
      {
        bResult = LISTVIEW_SetItem(hwnd, lpLVItem);
      }
      else
      {
        bResult = LISTVIEW_SetSubItem(hwnd, lpLVItem);
      }
    }
  }


  return bResult;
}

/* LISTVIEW_SetItemW  */

/***
 * DESCRIPTION:
 * Preallocates memory.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item count (prjected number of items)
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_SetItemCount(HWND hwnd, INT nItemCount)
{
  FIXME (listview, "empty stub!\n");
}

/***
 * DESCRIPTION:
 * Sets the position of an item.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index
 * [I] INT : x coordinate
 * [I] INT : y coordinate
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItemPosition(HWND hwnd, INT nItem, 
                                     INT nPosX, INT nPosY)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  LISTVIEW_ITEM *lpItem;
  HDPA hdpaSubItems;
  BOOL bResult = FALSE;

  TRACE(listview, "(hwnd=%x,nItem=%d,X=%d,Y=%d)\n", hwnd, nItem, nPosX, nPosY);

  if ((nItem >= 0) || (nItem < GETITEMCOUNT(infoPtr)))
  {
    switch (lStyle & LVS_TYPEMASK)
    {
    case LVS_ICON:
    case LVS_SMALLICON:
      hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem);
      if (hdpaSubItems != NULL)
      {
        lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
        if (lpItem != NULL)
        {
          bResult = TRUE;
          lpItem->ptPosition.x = nPosX;
          lpItem->ptPosition.y = nPosY;
        }
      }
      break;
    }
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Sets the state of one or many items.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I]INT : item index
 * [I] LPLVITEM : item or subitem info
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetItemState(HWND hwnd, INT nItem, LPLVITEMA lpLVItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  BOOL bResult = FALSE;
  LVITEMA lvItem;
  INT i;

  if (nItem == -1)
  {
    bResult = TRUE;
    ZeroMemory(&lvItem, sizeof(LVITEMA));
    lvItem.mask = LVIF_STATE;
    lvItem.state = lpLVItem->state;
    lvItem.stateMask = lpLVItem->stateMask;
    
    /* apply to all items */
    for (i = 0; i< GETITEMCOUNT(infoPtr); i++)
    {
      lvItem.iItem = i;
      if (ListView_SetItemA(hwnd, &lvItem) == FALSE)
      {
        bResult = FALSE;
      }
    }
  }
  else
  {
    ZeroMemory(&lvItem, sizeof(LVITEMA));
    lvItem.mask = LVIF_STATE;
    lvItem.state = lpLVItem->state;
    lvItem.stateMask = lpLVItem->stateMask;
    lvItem.iItem = nItem;
    bResult = ListView_SetItemA(hwnd, &lvItem);
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Sets the text of an item or subitem.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index
 * [I] LPLVITEMA : item or subitem info
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItemTextA(HWND hwnd, INT nItem, LPLVITEMA lpLVItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  BOOL bResult = FALSE;
  LVITEMA lvItem;

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
  {
    ZeroMemory(&lvItem, sizeof(LVITEMA));
    lvItem.mask = LVIF_TEXT;
    lvItem.pszText = lpLVItem->pszText;
    lvItem.iItem = nItem;
    lvItem.iSubItem = lpLVItem->iSubItem;
    bResult = ListView_SetItemA(hwnd, &lvItem);
  }
  
  return bResult;
}

/***
 * DESCRIPTION:
 * Sets the text background color.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] COLORREF : text background color
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetTextBkColor(HWND hwnd, COLORREF clrTextBk)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

  infoPtr->clrTextBk = clrTextBk;
  InvalidateRect(hwnd, NULL, TRUE);

  return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the text foreground color.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] COLORREF : text color 
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetTextColor (HWND hwnd, COLORREF clrText)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

  infoPtr->clrText = clrText;
  InvalidateRect(hwnd, NULL, TRUE);

  return TRUE;
}

/***
 * DESCRIPTION:
 * Sorts the listview items.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SortItems(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  FIXME (listview, "empty stub!\n");

  return TRUE;
}

/***
 * DESCRIPTION:
 * Updates an items or rearranges the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_Update(HWND hwnd, INT nItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  BOOL bResult = FALSE;
  RECT rc;

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
  {
    bResult = TRUE;

    /* rearrange with default alignment style */
    if ((lStyle & LVS_AUTOARRANGE) && (((lStyle & LVS_TYPEMASK) == LVS_ICON) ||
        ((lStyle & LVS_TYPEMASK)  == LVS_SMALLICON)))
    {
      ListView_Arrange(hwnd, 0);
    }
    else
    {
      /* get item bounding rectangle */
      rc.left = LVIR_BOUNDS;
      ListView_GetItemRect(hwnd, nItem, &rc);
      InvalidateRect(hwnd, &rc, FALSE);
    }
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Creates the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_Create(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LPCREATESTRUCTA lpcs = (LPCREATESTRUCTA)lParam;
  LOGFONTA logFont;

  /* initialize info pointer */
  ZeroMemory(infoPtr, sizeof(LISTVIEW_INFO));

  /* determine the type of structures to use */
  infoPtr->notifyFormat = SendMessageA(GetParent(hwnd), WM_NOTIFYFORMAT, 
                                       (WPARAM)hwnd, (LPARAM)NF_QUERY);
  if (infoPtr->notifyFormat != NFR_ANSI)
  {
    FIXME (listview, "ANSI notify format is NOT used\n");
  }
  
  /* initialize color information  */
  infoPtr->clrBk = GetSysColor(COLOR_WINDOW);
  infoPtr->clrText = GetSysColor(COLOR_WINDOWTEXT);
  infoPtr->clrTextBk = GetSysColor(COLOR_WINDOW); 

  /* set default values */
  infoPtr->uCallbackMask = 0;
  infoPtr->nFocusedItem = -1;
  infoPtr->nSelectionMark = -1;
  infoPtr->iconSpacing.cx = GetSystemMetrics(SM_CXICONSPACING);
  infoPtr->iconSpacing.cy = GetSystemMetrics(SM_CYICONSPACING);
  ZeroMemory(&infoPtr->rcList, sizeof(RECT));

  /* get default font (icon title) */
  SystemParametersInfoA(SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
  infoPtr->hDefaultFont = CreateFontIndirectA(&logFont);
  infoPtr->hFont = infoPtr->hDefaultFont;
  
  /* create header */
  infoPtr->hwndHeader =	CreateWindowA(WC_HEADERA, (LPCSTR)NULL, 
                                      WS_CHILD | HDS_HORZ | HDS_BUTTONS, 
                                      0, 0, 0, 0, hwnd, (HMENU)0, 
                                      lpcs->hInstance, NULL);

  /* set header font */
  SendMessageA(infoPtr->hwndHeader, WM_SETFONT, (WPARAM)infoPtr->hFont, 
               (LPARAM)TRUE);

  
  switch (lpcs->style & LVS_TYPEMASK)
  {
  case LVS_ICON:
    infoPtr->iconSize.cx = GetSystemMetrics(SM_CXICON);
    infoPtr->iconSize.cy = GetSystemMetrics(SM_CYICON);
    break;

  case LVS_REPORT:
    ShowWindow(infoPtr->hwndHeader, SW_SHOWNORMAL);
  case LVS_SMALLICON:
  case LVS_LIST:
    infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
    infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
    break;
  }

  /* display unsupported listview window styles */
  LISTVIEW_UnsupportedStyles(lpcs->style);

  /* allocate memory for the data structure */
  infoPtr->hdpaItems = DPA_Create(10);

  /* initialize size of items */
  infoPtr->nItemWidth = LISTVIEW_GetItemWidth(hwnd, lpcs->style);
  infoPtr->nItemHeight = LISTVIEW_GetItemHeight(hwnd, lpcs->style);

  return 0;
}

/***
 * DESCRIPTION:
 * Erases the background of the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] WPARAM : device context handle
 * [I] LPARAM : not used
 * 
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_EraseBackground(HWND hwnd, WPARAM wParam, 
                                        LPARAM lParam)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  BOOL bResult;

  if (infoPtr->clrBk == CLR_NONE) 
  {
    bResult = SendMessageA(GetParent(hwnd), WM_ERASEBKGND, wParam, lParam);
  }
  else 
  {
    RECT rc;
    HBRUSH hBrush = CreateSolidBrush(infoPtr->clrBk);
    GetClientRect(hwnd, &rc);
    FillRect((HDC)wParam, &rc, hBrush);
    DeleteObject(hBrush);
    bResult = TRUE;
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Retrieves the listview control font.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * Font handle.
 */
static LRESULT LISTVIEW_GetFont(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

  return infoPtr->hFont;
}

/***
 * DESCRIPTION:
 * Performs vertical scrolling.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : scroll code
 * [I] INT : scroll position
 * [I] HWND : scrollbar control window handle
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_VScroll(HWND hwnd, INT nScrollCode, INT nScroll, 
                                HWND hScrollWnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nScrollPosInc = 0;
  INT nScrollPos;
  INT nMinRange;
  INT nMaxRange;

  GetScrollRange(hwnd, SB_VERT, &nMinRange, &nMaxRange);
  nScrollPos = GetScrollPos(hwnd, SB_VERT);

  switch (nScrollCode)
  {
  case SB_LINEUP:
    if (nScrollPos > nMinRange)
    {
      nScrollPosInc = -1;
    }
    break;
    
  case SB_LINEDOWN:
    if (nScrollPos < nMaxRange)
    {
      nScrollPosInc = 1;
    }
    break;

  case SB_PAGEUP:
    switch (LVS_TYPEMASK & lStyle)
    {
    case LVS_REPORT:
      if (nScrollPos > nMinRange + infoPtr->nCountPerColumn)
      {
        nScrollPosInc = -infoPtr->nCountPerColumn;
      } 
      else
      {
        nScrollPosInc = nMinRange - nScrollPos;
      }
      break;

    case LVS_SMALLICON:
    case LVS_ICON:
      if (nScrollPos > nMinRange + 10)
      {
        nScrollPosInc = -10;
      }
      else
      {
        nScrollPosInc = nMinRange - nScrollPos;
      }
      break;
    }
    break;

   case SB_PAGEDOWN:
     switch (LVS_TYPEMASK & lStyle)
     {
     case LVS_REPORT:
       if (nScrollPos < nMaxRange - infoPtr->nCountPerColumn)
       {
         nScrollPosInc = infoPtr->nCountPerColumn;
       }
       else
       {
         nScrollPosInc = nMaxRange - nScrollPos;
       }
       break;

     case LVS_SMALLICON:
     case LVS_ICON:
       if (nScrollPos < nMaxRange - 10)
       {
         nScrollPosInc = 10;
       }
       else
       {
         nScrollPosInc = nMaxRange - nScrollPos;
       }
       break;
     }
     break;

   case SB_THUMBPOSITION:
     nScrollPosInc = nScroll - nScrollPos; 
    break;
  }

  if (nScrollPosInc != 0)
  {
    LISTVIEW_ScrollView(hwnd, 0, nScrollPosInc);
  }

  return 0;
}


/***
 * DESCRIPTION:
 * Performs horizontal scrolling.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : scroll code
 * [I] INT : scroll position
 * [I] HWND : scrollbar control window handle
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_HScroll(HWND hwnd, INT nScrollCode, 
                                INT nScroll, HWND hScrollWnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nScrollPosInc = 0;
  INT nScrollPos;
  INT nMinRange;
  INT nMaxRange;

  GetScrollRange(hwnd, SB_HORZ, &nMinRange, &nMaxRange);
  nScrollPos = GetScrollPos(hwnd, SB_HORZ);

  switch (nScrollCode)
  {
  case SB_LINELEFT:
    if (nScrollPos > nMinRange)
    {
      nScrollPosInc = -1;
    }
    break;

  case SB_LINERIGHT:
    if (nScrollPos < nMaxRange)
    {
      nScrollPosInc = 1;
    }
    break;

  case SB_PAGELEFT:
    switch (LVS_TYPEMASK & lStyle)
    {
    case LVS_LIST:
      if (nScrollPos > nMinRange + infoPtr->nCountPerRow)
      {
        nScrollPosInc = -infoPtr->nCountPerRow;
      }
      else
      {
        nScrollPosInc = nMinRange - nScrollPos;
      }
      break;

    case LVS_REPORT:
    case LVS_SMALLICON:
    case LVS_ICON:
      if (nScrollPos > nMinRange + 10)
      {
        nScrollPosInc = -10;
      }
      else
      {
        nScrollPosInc = nMinRange - nScrollPos;
      }
      break;
    }
    break;

  case SB_PAGERIGHT:
    switch (LVS_TYPEMASK & lStyle)
    {
    case LVS_LIST:
      if (nScrollPos < nMaxRange - infoPtr->nCountPerRow)
      {
        nScrollPosInc = infoPtr->nCountPerRow;
      }
      else
      {
        nScrollPosInc = nMaxRange - nScrollPos;
      }
      break;
      
    case LVS_REPORT:
    case LVS_SMALLICON:
    case LVS_ICON:
      if (nScrollPos < nMaxRange - 10)
      {
        nScrollPosInc = 10;
      }
      else
      {
        nScrollPosInc = nMaxRange - nScrollPos;
      }
      break;
    }
    break;

  case SB_THUMBPOSITION:
    nScrollPosInc = nScroll - nScrollPos;
    break;
  }

  if (nScrollPosInc != 0)
  {
    LISTVIEW_ScrollView(hwnd, nScrollPosInc, 0);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * ??? 
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : virtual key 
 * [I] LONG : key data
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_KeyDown(HWND hwnd, INT nVirtualKey, LONG lKeyData)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nCtrlId = GetWindowLongA(hwnd, GWL_ID);
  INT nCountPerColumn;
  INT nCountPerRow;
  HWND hwndParent = GetParent(hwnd);
  NMLVKEYDOWN nmKeyDown; 
  NMHDR nmh;

  /* send LVN_KEYDOWN notification */
  ZeroMemory(&nmKeyDown, sizeof(NMLVKEYDOWN));
  nmKeyDown.hdr.hwndFrom = hwnd;  
  nmKeyDown.hdr.idFrom = nCtrlId;  
  nmKeyDown.hdr.code = LVN_KEYDOWN;  
  nmKeyDown.wVKey = nVirtualKey; 
  nmKeyDown.flags = 0; 
  SendMessageA(hwndParent, WM_NOTIFY, (WPARAM)nCtrlId, (LPARAM)&nmKeyDown); 
  
  /* initialize */
  nmh.hwndFrom = hwnd;
  nmh.idFrom = nCtrlId;

  switch (nVirtualKey)
  {
  case VK_RETURN:
    if ((GETITEMCOUNT(infoPtr) > 0) && (infoPtr->nFocusedItem != -1))
    {
      /* send NM_RETURN notification */
      nmh.code = NM_RETURN;
      ListView_Notify(hwndParent, nCtrlId, &nmh);
      
      /* send LVN_ITEMACTIVATE notification */
      nmh.code = LVN_ITEMACTIVATE;
      ListView_Notify(hwndParent, nCtrlId, &nmh);
    }
    break;

  case VK_HOME:
    if (GETITEMCOUNT(infoPtr) > 0)
    {
      LISTVIEW_KeySelection(hwnd, 0); 
    }
    break;

  case VK_END:
    if (GETITEMCOUNT(infoPtr) > 0)
    {
      LISTVIEW_KeySelection(hwnd, GETITEMCOUNT(infoPtr) - 1);
    }
    break;

  case VK_LEFT:
    switch (LVS_TYPEMASK & lStyle)
    {
    case LVS_LIST:
      if (infoPtr->nFocusedItem >= infoPtr->nCountPerColumn) 
      {
        LISTVIEW_KeySelection(hwnd, infoPtr->nFocusedItem - 
                              infoPtr->nCountPerColumn);
      }
      break;

    case LVS_SMALLICON:
    case LVS_ICON:
      if (lStyle & LVS_ALIGNLEFT)
      {
        nCountPerColumn = max((infoPtr->rcList.bottom - infoPtr->rcList.top) /
                              infoPtr->nItemHeight, 1);
        if (infoPtr->nFocusedItem >= nCountPerColumn) 
        {
          LISTVIEW_KeySelection(hwnd, infoPtr->nFocusedItem - nCountPerColumn);
        }
      }
      else
      {
        nCountPerRow = max((infoPtr->rcList.right - infoPtr->rcList.left) /
                           infoPtr->nItemWidth, 1);
        if (infoPtr->nFocusedItem % nCountPerRow != 0)
        {
          LISTVIEW_SetSelection(hwnd, infoPtr->nFocusedItem - 1); 
        }
      }
      break;
    }
    break;
  
  case VK_UP:
    switch (LVS_TYPEMASK & lStyle)
    {
    case LVS_LIST:
    case LVS_REPORT:
      if (infoPtr->nFocusedItem > 0)
      {
        LISTVIEW_KeySelection(hwnd, infoPtr->nFocusedItem - 1);
      }
      break;

    default:
      if (lStyle & LVS_ALIGNLEFT)
      {
        nCountPerColumn = max((infoPtr->rcList.bottom - infoPtr->rcList.top) /
                              infoPtr->nItemHeight, 1);
        if (infoPtr->nFocusedItem % nCountPerColumn != 0)
        {
          LISTVIEW_KeySelection(hwnd, infoPtr->nFocusedItem - 1);
        }
      }
      else
      {
        nCountPerRow = max((infoPtr->rcList.right - infoPtr->rcList.left) /
                           infoPtr->nItemWidth, 1);
        if (infoPtr->nFocusedItem >= nCountPerRow)
        {
          LISTVIEW_KeySelection(hwnd, infoPtr->nFocusedItem - nCountPerRow);
        }
      }
    }
    break;
    
  case VK_RIGHT:
    switch (LVS_TYPEMASK & lStyle)
    {
    case LVS_LIST:
      if (infoPtr->nFocusedItem < GETITEMCOUNT(infoPtr) - 
          infoPtr->nCountPerColumn)
      {
        LISTVIEW_KeySelection(hwnd, infoPtr->nFocusedItem +
                              infoPtr->nCountPerColumn);
      }
      break;

    case LVS_ICON:
    case LVS_SMALLICON:
      if (lStyle & LVS_ALIGNLEFT)
      {
        nCountPerColumn = max((infoPtr->rcList.bottom - infoPtr->rcList.top) /
                              infoPtr->nItemHeight, 1);
        if (infoPtr->nFocusedItem < GETITEMCOUNT(infoPtr) - nCountPerColumn)
        {
          LISTVIEW_KeySelection(hwnd, infoPtr->nFocusedItem + nCountPerColumn);
        }
      }
      else
      {
        nCountPerRow = max((infoPtr->rcList.right - infoPtr->rcList.left) /
                           infoPtr->nItemWidth, 1);
        if ((infoPtr->nFocusedItem % nCountPerRow != nCountPerRow - 1) && 
            (infoPtr->nFocusedItem < GETITEMCOUNT(infoPtr) - 1))
        {
          LISTVIEW_KeySelection(hwnd, infoPtr->nFocusedItem + 1);
        }
      }
    }
    break;

  case VK_DOWN:
    switch (LVS_TYPEMASK & lStyle)
    {
    case LVS_LIST:
    case LVS_REPORT:
      if (infoPtr->nFocusedItem < GETITEMCOUNT(infoPtr) - 1)
      {
        LISTVIEW_KeySelection(hwnd, infoPtr->nFocusedItem + 1);
      }
      break;

    case LVS_SMALLICON:
    case LVS_ICON:
      if (lStyle & LVS_ALIGNLEFT)
      {
        nCountPerColumn = max((infoPtr->rcList.bottom - infoPtr->rcList.top) /
                              infoPtr->nItemHeight, 1);
        if (infoPtr->nFocusedItem % nCountPerColumn != nCountPerColumn - 1)
        {
          LISTVIEW_KeySelection(hwnd, infoPtr->nFocusedItem + 1);
        }
      }
      else
      {
        nCountPerRow = max((infoPtr->rcList.right - infoPtr->rcList.left) /
                           infoPtr->nItemWidth, 1);
        if (infoPtr->nFocusedItem < GETITEMCOUNT(infoPtr) - nCountPerRow)
        {
          LISTVIEW_KeySelection(hwnd, infoPtr->nFocusedItem + nCountPerRow);
        }
      }
    }
    break;

  case VK_PRIOR:
    break;

  case VK_NEXT:
    break;
  }

  /* refresh client area */
  InvalidateRect(hwnd, NULL, TRUE);

  return 0;
}

/***
 * DESCRIPTION:
 * Kills the focus.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_KillFocus(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLongA(hwnd, 0);
  INT nCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMHDR nmh;

  /* send NM_KILLFOCUS notification */
  nmh.hwndFrom = hwnd;
  nmh.idFrom = nCtrlId;
  nmh.code = NM_KILLFOCUS;
  ListView_Notify(GetParent(hwnd), nCtrlId, &nmh);

  /* set window focus flag */
  infoPtr->bFocus = FALSE;

  return 0;
}

/***
 * DESCRIPTION:
 * Processes double click messages (left mouse button).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] WORD : key flag
 * [I] WORD : x coordinate
 * [I] WORD : y coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_LButtonDblClk(HWND hwnd, WORD wKey, WORD wPosX, 
                                      WORD wPosY)
{
  LONG nCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMHDR nmh;

  TRACE(listview, "(hwnd=%x,key=%hu,X=%hu,Y=%hu)\n", hwnd, wKey, wPosX, wPosY);

  /* send NM_DBLCLK notification */
  nmh.hwndFrom = hwnd;
  nmh.idFrom = nCtrlId;
  nmh.code = NM_DBLCLK;
  ListView_Notify(GetParent(hwnd), nCtrlId, &nmh);

  /* send LVN_ITEMACTIVATE notification */
  nmh.code = LVN_ITEMACTIVATE;
  ListView_Notify(GetParent(hwnd), nCtrlId, &nmh);

  return 0;
}

/***
 * DESCRIPTION:
 * Processes mouse down messages (left mouse button).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] WORD : key flag
 * [I] WORD : x coordinate
 * [I] WORD : y coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_LButtonDown(HWND hwnd, WORD wKey, WORD wPosX, 
                                    WORD wPosY)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nCtrlId = GetWindowLongA(hwnd, GWL_ID);
  static BOOL bGroupSelect = TRUE;
  NMHDR nmh;
  INT nItem;

  TRACE(listview, "(hwnd=%x,key=%hu,X=%hu,Y=%hu)\n", hwnd, wKey, wPosX, wPosY);

  /* send NM_RELEASEDCAPTURE notification */ 
  nmh.hwndFrom = hwnd;
  nmh.idFrom = nCtrlId;
  nmh.code = NM_RELEASEDCAPTURE;
  ListView_Notify(GetParent(hwnd), nCtrlId, &nmh);
 
  if (infoPtr->bFocus == FALSE)
  {
    SetFocus(hwnd);
  }

  /* set left button down flag */
  infoPtr->bLButtonDown = TRUE;

  nItem = LISTVIEW_MouseSelection(hwnd, wPosX, wPosY);
  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
  {
    if ((wKey & MK_CONTROL) && (wKey & MK_SHIFT))
    {
      if (bGroupSelect != FALSE)
      {
        LISTVIEW_AddGroupSelection(hwnd, nItem);
      }
      else
      {
        LISTVIEW_AddSelection(hwnd, nItem);
      }
    }
    else if (wKey & MK_CONTROL)
    {
      bGroupSelect = LISTVIEW_ToggleSelection(hwnd, nItem);
    }
    else  if (wKey & MK_SHIFT)
    {
      LISTVIEW_SetGroupSelection(hwnd, nItem);
    }
    else
    {
      LISTVIEW_SetSelection(hwnd, nItem);
    }
  }
  else
  {
    /* remove all selections */
    LISTVIEW_RemoveSelections(hwnd, 0, GETITEMCOUNT(infoPtr));
  }

  InvalidateRect(hwnd, NULL, TRUE);

  return 0;
}

/***
 * DESCRIPTION:
 * Processes mouse up messages (left mouse button).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] WORD : key flag
 * [I] WORD : x coordinate
 * [I] WORD : y coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_LButtonUp(HWND hwnd, WORD wKey, WORD wPosX, 
                                  WORD wPosY)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

  TRACE(listview, "(hwnd=%x,key=%hu,X=%hu,Y=%hu)\n", hwnd, wKey, wPosX, wPosY);

  if (infoPtr->bLButtonDown != FALSE) 
  {
    INT nCtrlId = GetWindowLongA(hwnd, GWL_ID);
    NMHDR nmh;

    /* send NM_CLICK notification */
    nmh.hwndFrom = hwnd;
    nmh.idFrom = nCtrlId;
    nmh.code = NM_CLICK;
    ListView_Notify(GetParent(hwnd), nCtrlId, &nmh);

    /* set left button flag */
    infoPtr->bLButtonDown = FALSE;
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Creates the listview control (called before WM_CREATE).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] WPARAM : unhandled 
 * [I] LPARAM : widow creation info
 * 
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_NCCreate(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  LISTVIEW_INFO *infoPtr;

  TRACE(listview, "(hwnd=%x,wParam=%x,lParam=%lx)\n", hwnd, wParam, lParam);

  /* allocate memory for info structure */
  infoPtr = (LISTVIEW_INFO *)COMCTL32_Alloc(sizeof(LISTVIEW_INFO));
  SetWindowLongA(hwnd, 0, (LONG)infoPtr);
  if (infoPtr == NULL) 
  {
    ERR(listview, "could not allocate info memory!\n");
    return 0;
  }

  if ((LISTVIEW_INFO *)GetWindowLongA(hwnd, 0) != infoPtr) 
  {
    ERR(listview, "pointer assignment error!\n");
    return 0;
  }

  return DefWindowProcA(hwnd, WM_NCCREATE, wParam, lParam);
}

/***
 * DESCRIPTION:
 * Destroys the listview control (called after WM_DESTROY).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * 
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_NCDestroy(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

  TRACE(listview, "(hwnd=%x)\n", hwnd);

  /* delete all items */
  LISTVIEW_DeleteAllItems(hwnd);

  /* destroy data structure */
  DPA_Destroy(infoPtr->hdpaItems);

  /* destroy font */
  infoPtr->hFont = (HFONT)0;
  if (infoPtr->hDefaultFont)
  {
    DeleteObject(infoPtr->hDefaultFont);
  }

  /* free listview info pointer*/
  COMCTL32_Free(infoPtr);

  return 0;
}

/***
 * DESCRIPTION:
 * Handles notifications from children.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : control identifier
 * [I] LPNMHDR : notification information
 * 
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_Notify(HWND hwnd, INT nCtrlId, LPNMHDR lpnmh)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  
  if (lpnmh->hwndFrom == infoPtr->hwndHeader) 
  {
    /* handle notification from header control */
    if (lpnmh->code == HDN_ENDTRACKA)
    {
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(hwnd, LVS_REPORT);
      InvalidateRect(hwnd, NULL, TRUE);
    }
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Determines the type of structure to use.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle of the sender
 * [I] HWND : listview window handle 
 * [I] INT : command specifying the nature of the WM_NOTIFYFORMAT
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_NotifyFormat(HWND hwndFrom, HWND hwnd, INT nCommand)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

  if (nCommand == NF_REQUERY)
  {
    /* determine the type of structure to use */
    infoPtr->notifyFormat = SendMessageA(hwndFrom, WM_NOTIFYFORMAT, 
                                         (WPARAM)hwnd, (LPARAM)NF_QUERY);
    if (infoPtr->notifyFormat == NFR_UNICODE)
    {
      FIXME (listview, "NO support for unicode structures");
    }
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Paints/Repaints the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] HDC : device context handle
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_Paint(HWND hwnd, HDC hdc)
{
  PAINTSTRUCT ps;

   TRACE(listview, "(hwnd=%x,hdc=%x)\n", hwnd, hdc);

  if (hdc == 0)
  {
    hdc = BeginPaint(hwnd, &ps);
    LISTVIEW_Refresh(hwnd, hdc);
    EndPaint(hwnd, &ps);
  }
  else
  {
    LISTVIEW_Refresh(hwnd, hdc);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Processes double click messages (right mouse button).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] WORD : key flag
 * [I] WORD : x coordinate
 * [I] WORD : y coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_RButtonDblClk(HWND hwnd, WORD wKey, WORD wPosX, 
                                      WORD wPosY)
{
  INT nCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMHDR nmh;

  TRACE(listview, "(hwnd=%x,key=%hu,X=%hu,Y=%hu)\n", hwnd, wKey, wPosX, wPosY);

  /* send NM_RELEASEDCAPTURE notification */ 
  nmh.hwndFrom = hwnd;
  nmh.idFrom = nCtrlId;
  nmh.code = NM_RELEASEDCAPTURE;
  ListView_Notify(GetParent(hwnd), nCtrlId, &nmh);

  /* send NM_RDBLCLK notification */
  nmh.code = NM_RDBLCLK;
  ListView_Notify(GetParent(hwnd), nCtrlId, &nmh);

  return 0;
}

/***
 * DESCRIPTION:
 * Processes mouse down messages (right mouse button).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] WORD : key flag
 * [I] WORD : x coordinate
 * [I] WORD : y coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_RButtonDown(HWND hwnd, WORD wKey, WORD wPosX, 
                                    WORD wPosY)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMHDR nmh;
  INT nItem;

  TRACE(listview, "(hwnd=%x,key=%hu,X=%hu,Y=%hu)\n", hwnd, wKey, wPosX, wPosY);

  /* send NM_RELEASEDCAPTURE notification */
  nmh.hwndFrom = hwnd;
  nmh.idFrom = nCtrlId;
  nmh.code = NM_RELEASEDCAPTURE;
  ListView_Notify(GetParent(hwnd), nCtrlId, &nmh);
 
  /* make sure the listview control window has the focus */
  if (infoPtr->bFocus == FALSE)
  {
    SetFocus(hwnd);
  }

  /* set right button down flag */
  infoPtr->bRButtonDown = TRUE;

  /* determine the index of the selected item */
  nItem = LISTVIEW_MouseSelection(hwnd, wPosX, wPosY);
  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
  {
    if (!((wKey & MK_SHIFT) || (wKey & MK_CONTROL)))
    {
      LISTVIEW_SetSelection(hwnd, nItem);
    }
  }
  else
  {
    LISTVIEW_RemoveSelections(hwnd, 0, GETITEMCOUNT(infoPtr));
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Processes mouse up messages (right mouse button).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] WORD : key flag
 * [I] WORD : x coordinate
 * [I] WORD : y coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_RButtonUp(HWND hwnd, WORD wKey, WORD wPosX, 
                                  WORD wPosY)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMHDR nmh;

  TRACE(listview, "(hwnd=%x,key=%hu,X=%hu,Y=%hu)\n", hwnd, wKey, wPosX, wPosY);

  if (infoPtr->bRButtonDown != FALSE) 
  {
    /* send NM_RClICK notification */
    ZeroMemory(&nmh, sizeof(NMHDR));
    nmh.hwndFrom = hwnd;
    nmh.idFrom = nCtrlId;
    nmh.code = NM_RCLICK;
    ListView_Notify(GetParent(hwnd), nCtrlId, &nmh);

    /* set button flag */
    infoPtr->bRButtonDown = FALSE;
  }
  
  return 0;
}

/***
 * DESCRIPTION:
 * Sets the focus.  
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] HWND : window handle of previously focused window
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_SetFocus(HWND hwnd, HWND hwndLoseFocus)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMHDR nmh;

  /* send NM_SETFOCUS notification */
  nmh.hwndFrom = hwnd;
  nmh.idFrom = nCtrlId;
  nmh.code = NM_SETFOCUS;
  ListView_Notify(GetParent(hwnd), nCtrlId, &nmh);

  /* set window focus flag */
  infoPtr->bFocus = TRUE;

  return 0;
}

/***
 * DESCRIPTION:
 * Sets the font.  
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] HFONT : font handle
 * [I] WORD : redraw flag
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_SetFont(HWND hwnd, HFONT hFont, WORD fRedraw)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);

  TRACE(listview, "(hwnd=%x,hfont=%x,redraw=%hu)\n", hwnd, hFont, fRedraw);

  if (hFont == 0)
  {
    infoPtr->hFont = infoPtr->hDefaultFont;
  }
  else
  {
    infoPtr->hFont = hFont;
  }

  if ((LVS_TYPEMASK & lStyle ) == LVS_REPORT)
  {
    /* set header font */
    SendMessageA(infoPtr->hwndHeader, WM_SETFONT, (WPARAM)hFont, 
                   MAKELPARAM(fRedraw, 0));
  }

  /* invalidate listview control client area */
  InvalidateRect(hwnd, NULL, TRUE);
  
  if (fRedraw != FALSE)
  {
    UpdateWindow(hwnd);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Resizes the listview control. This function processes WM_SIZE
 * messages.  At this time, the width and height are not used.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] WORD : new width
 * [I] WORD : new height
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_Size(HWND hwnd, int Width, int Height)
{
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);

  TRACE(listview, "(hwnd=%x,width=%d,height=%d)\n",hwnd, Width, Height);

  LISTVIEW_SetSize(hwnd, lStyle, -1, -1);
  switch (lStyle & LVS_TYPEMASK)
  {
  case LVS_LIST:
  case LVS_REPORT:
    LISTVIEW_SetViewInfo(hwnd, lStyle);
    break;
    
  case LVS_ICON:
  case LVS_SMALLICON:
    if (lStyle & LVS_ALIGNLEFT)
    {
      LISTVIEW_AlignLeft(hwnd);
    }
    else
    {
      LISTVIEW_AlignTop(hwnd);
    }
    break;
  }

  LISTVIEW_SetScroll(hwnd, lStyle);

  /* invalidate + erase background */
  InvalidateRect(hwnd, NULL, TRUE);

  return 0;
}

/***
 * DESCRIPTION:
 * Sets the size information for a given style. 
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] LONG : window style
 * [I] WORD : new width 
 * [I] WORD : new height
 *
 * RETURN:
 * Zero
 */
static VOID LISTVIEW_SetSize(HWND hwnd, LONG lStyle, LONG lWidth, LONG lHeight)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  HDLAYOUT hl;
  WINDOWPOS wp;
  RECT rcList;
  
  GetClientRect(hwnd, &rcList);
  if (lWidth == -1)
  {
    infoPtr->rcList.left = max(rcList.left, 0);
    infoPtr->rcList.right = max(rcList.right, 0);
  }
  else
  {
    infoPtr->rcList.left = max(rcList.left, 0);
    infoPtr->rcList.right = infoPtr->rcList.left + max(lWidth, 0);
  }

  if (lHeight == -1)
  {
    infoPtr->rcList.top = max(rcList.top, 0);
    infoPtr->rcList.bottom = max(rcList.bottom, 0);
  }
  else
  {
    infoPtr->rcList.top = max(rcList.top, 0);
    infoPtr->rcList.bottom = infoPtr->rcList.top + max(lHeight, 0);
  }
    
  switch (lStyle & LVS_TYPEMASK)
  {
  case LVS_LIST:
    if ((lStyle & WS_HSCROLL) == 0)
    {
      INT nHScrollHeight;
      nHScrollHeight = GetSystemMetrics(SM_CYHSCROLL);
      if (infoPtr->rcList.bottom > nHScrollHeight)
      { 
        infoPtr->rcList.bottom -= nHScrollHeight;
      }
    }
    break;

  case LVS_REPORT:
    hl.prc = &rcList;
    hl.pwpos = &wp;
    Header_Layout(infoPtr->hwndHeader, &hl);
    infoPtr->rcList.top = max(wp.cy, 0);
    break;
  }
}

/***
 * DESCRIPTION:
 * Processes WM_STYLECHANGED messages. 
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] WPARAM : window style type (normal or extended)
 * [I] LPSTYLESTRUCT : window style information
 *
 * RETURN:
 * Zero
 */
static INT LISTVIEW_StyleChanged(HWND hwnd, WPARAM wStyleType, 
                                 LPSTYLESTRUCT lpss)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  RECT rcList = infoPtr->rcList;
  HDLAYOUT hl;
  WINDOWPOS wp;

  TRACE(listview, "(hwnd=%x,styletype=%x,stylestruct=%p)\n", 
        hwnd, wStyleType, lpss);

  if (wStyleType == GWL_STYLE)
  {
    if ((lpss->styleOld & WS_HSCROLL) != 0)
    {
      ShowScrollBar(hwnd, SB_HORZ, FALSE);
    }

    if ((lpss->styleOld & WS_VSCROLL) != 0)
    {
      ShowScrollBar(hwnd, SB_VERT, FALSE);
    }

    if ((LVS_TYPEMASK & lpss->styleOld) == LVS_REPORT)
    {
      /* remove header */
      ShowWindow(infoPtr->hwndHeader, SW_HIDE);
    }

    switch (lpss->styleNew & LVS_TYPEMASK)
    {
    case LVS_ICON:
      infoPtr->iconSize.cx = GetSystemMetrics(SM_CXICON);
      infoPtr->iconSize.cy = GetSystemMetrics(SM_CYICON);
      LISTVIEW_SetSize(hwnd, lpss->styleNew, -1, -1);
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(hwnd, lpss->styleNew);
      infoPtr->nItemHeight = LISTVIEW_GetItemHeight(hwnd, lpss->styleNew);
      if (lpss->styleNew & LVS_ALIGNLEFT)
      {
        LISTVIEW_AlignLeft(hwnd);
      }
      else
      {
        LISTVIEW_AlignTop(hwnd);
      }
      break;

    case LVS_REPORT:
      hl.prc = &rcList;
      hl.pwpos = &wp;
      Header_Layout(infoPtr->hwndHeader, &hl);
      SetWindowPos(infoPtr->hwndHeader, hwnd, wp.x, wp.y, wp.cx, 
                   wp.cy, wp.flags);
      ShowWindow(infoPtr->hwndHeader, SW_SHOWNORMAL);
      infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
      infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(hwnd, lpss->styleNew);
      infoPtr->nItemHeight = LISTVIEW_GetItemHeight(hwnd, lpss->styleNew);
      LISTVIEW_SetSize(hwnd, lpss->styleNew, -1, -1);
      LISTVIEW_SetViewInfo(hwnd, lpss->styleNew);
      break;

    case LVS_LIST:
      infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
      infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(hwnd, lpss->styleNew);
      infoPtr->nItemHeight = LISTVIEW_GetItemHeight(hwnd, lpss->styleNew);
      LISTVIEW_SetSize(hwnd, lpss->styleNew, -1, -1);
      LISTVIEW_SetViewInfo(hwnd, lpss->styleNew);
      break;

    case LVS_SMALLICON:
      infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
      infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
      LISTVIEW_SetSize(hwnd, lpss->styleNew, -1, -1);
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(hwnd, lpss->styleNew);
      infoPtr->nItemHeight = LISTVIEW_GetItemHeight(hwnd, lpss->styleNew);
      if (lpss->styleNew & LVS_ALIGNLEFT)
      {
        LISTVIEW_AlignLeft(hwnd);
      }
      else
      {
        LISTVIEW_AlignTop(hwnd);
      }
      break;
    }

    LISTVIEW_SetScroll(hwnd, lpss->styleNew);
    
    /* print unsupported styles */
    LISTVIEW_UnsupportedStyles(lpss->styleNew);

    /* invalidate client area */
    InvalidateRect(hwnd, NULL, TRUE);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Window procedure of the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND :
 * [I] UINT :
 * [I] WPARAM :
 * [I] LPARAM : 
 *
 * RETURN:
 * 
 */
LRESULT WINAPI LISTVIEW_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam)
{
  switch (uMsg)
  {
  case LVM_APPROXIMATEVIEWRECT: 
    return LISTVIEW_ApproximateViewRect(hwnd, (INT)wParam, 
                                        LOWORD(lParam), HIWORD(lParam));
  case LVM_ARRANGE: 
    return LISTVIEW_Arrange(hwnd, (INT)wParam);

/* case LVM_CREATEDRAGIMAGE: */

  case LVM_DELETEALLITEMS:
    return LISTVIEW_DeleteAllItems(hwnd);

  case LVM_DELETECOLUMN:
    return LISTVIEW_DeleteColumn(hwnd, (INT)wParam);

  case LVM_DELETEITEM:
    return LISTVIEW_DeleteItem(hwnd, (INT)wParam);

/*	case LVM_EDITLABEL: */

  case LVM_ENSUREVISIBLE:
    return LISTVIEW_EnsureVisible(hwnd, (INT)wParam, (BOOL)lParam);

  case LVM_FINDITEMA:
    return LISTVIEW_FindItem(hwnd, (INT)wParam, (LPLVFINDINFO)lParam);

  case LVM_GETBKCOLOR:
    return LISTVIEW_GetBkColor(hwnd);

/*	case LVM_GETBKIMAGE: */

  case LVM_GETCALLBACKMASK:
    return LISTVIEW_GetCallbackMask(hwnd);

  case LVM_GETCOLUMNA:
    return LISTVIEW_GetColumnA(hwnd, (INT)wParam, (LPLVCOLUMNA)lParam);

/*	case LVM_GETCOLUMNW: */
/*	case LVM_GETCOLUMNORDERARRAY: */

  case LVM_GETCOLUMNWIDTH:
    return LISTVIEW_GetColumnWidth(hwnd, (INT)wParam);

  case LVM_GETCOUNTPERPAGE:
    return LISTVIEW_GetCountPerPage(hwnd);

/*	case LVM_GETEDITCONTROL: */
/*	case LVM_GETEXTENDEDLISTVIEWSTYLE: */

  case LVM_GETHEADER:
    return LISTVIEW_GetHeader(hwnd);

/*	case LVM_GETHOTCURSOR: */
/*	case LVM_GETHOTITEM: */
/*	case LVM_GETHOVERTIME: */

  case LVM_GETIMAGELIST:
    return LISTVIEW_GetImageList(hwnd, (INT)wParam);

/*	case LVM_GETISEARCHSTRING: */

  case LVM_GETITEMA:
    return LISTVIEW_GetItemA(hwnd, (LPLVITEMA)lParam);

/*	case LVM_GETITEMW: */

  case LVM_GETITEMCOUNT:
    return LISTVIEW_GetItemCount(hwnd);

  case LVM_GETITEMPOSITION:
    return LISTVIEW_GetItemPosition(hwnd, (INT)wParam, (LPPOINT)lParam);

  case LVM_GETITEMRECT: 
    return LISTVIEW_GetItemRect(hwnd, (INT)wParam, (LPRECT)lParam);

  case LVM_GETITEMSPACING: 
    return LISTVIEW_GetItemSpacing(hwnd, (BOOL)wParam);

  case LVM_GETITEMSTATE: 
    return LISTVIEW_GetItemState(hwnd, (INT)wParam, (UINT)lParam);
    
  case LVM_GETITEMTEXTA:
    LISTVIEW_GetItemTextA(hwnd, (INT)wParam, (LPLVITEMA)lParam);
    break;

/*	case LVM_GETITEMTEXTW: */

  case LVM_GETNEXTITEM:
    return LISTVIEW_GetNextItem(hwnd, (INT)wParam, LOWORD(lParam));

/*	case LVM_GETNUMBEROFWORKAREAS: */

  case LVM_GETORIGIN:
    return LISTVIEW_GetOrigin(hwnd, (LPPOINT)lParam);

  case LVM_GETSELECTEDCOUNT:
    return LISTVIEW_GetSelectedCount(hwnd);

  case LVM_GETSELECTIONMARK: 
    return LISTVIEW_GetSelectionMark(hwnd);

  case LVM_GETSTRINGWIDTHA:
    return LISTVIEW_GetStringWidthA (hwnd, (LPCSTR)lParam);

/*	case LVM_GETSTRINGWIDTHW: */
/*	case LVM_GETSUBITEMRECT: */

  case LVM_GETTEXTBKCOLOR:
    return LISTVIEW_GetTextBkColor(hwnd);

  case LVM_GETTEXTCOLOR:
    return LISTVIEW_GetTextColor(hwnd);

/*	case LVM_GETTOOLTIPS: */

  case LVM_GETTOPINDEX:
    return LISTVIEW_GetTopIndex(hwnd);

/*	case LVM_GETUNICODEFORMAT: */

  case LVM_GETVIEWRECT:
    return LISTVIEW_GetViewRect(hwnd, (LPRECT)lParam);

/*	case LVM_GETWORKAREAS: */

  case LVM_HITTEST:
    return LISTVIEW_HitTest(hwnd, (LPLVHITTESTINFO)lParam);

  case LVM_INSERTCOLUMNA:
    return LISTVIEW_InsertColumnA(hwnd, (INT)wParam, 
                                    (LPLVCOLUMNA)lParam);

/*	case LVM_INSERTCOLUMNW: */

  case LVM_INSERTITEMA:
    return LISTVIEW_InsertItemA(hwnd, (LPLVITEMA)lParam);

/*	case LVM_INSERTITEMW: */

  case LVM_REDRAWITEMS:
    return LISTVIEW_RedrawItems(hwnd, (INT)wParam, (INT)lParam);

/*   case LVM_SCROLL:  */
/*     return LISTVIEW_Scroll(hwnd, (INT)wParam, (INT)lParam); */

  case LVM_SETBKCOLOR:
    return LISTVIEW_SetBkColor(hwnd, (COLORREF)lParam);

/*	case LVM_SETBKIMAGE: */

  case LVM_SETCALLBACKMASK:
    return LISTVIEW_SetCallbackMask(hwnd, (UINT)wParam);

  case LVM_SETCOLUMNA:
    return LISTVIEW_SetColumnA(hwnd, (INT)wParam, (LPLVCOLUMNA)lParam);

/*	case LVM_SETCOLUMNW: */
/*	case LVM_SETCOLUMNORDERARRAY: */
/*	case LVM_SETCOLUMNWIDTH: */
/*	case LVM_SETEXTENDEDLISTVIEWSTYLE: */
/*	case LVM_SETHOTCURSOR: */
/*	case LVM_SETHOTITEM: */
/*	case LVM_SETHOVERTIME: */
/*	case LVM_SETICONSPACING: */
	
  case LVM_SETIMAGELIST:
    return LISTVIEW_SetImageList(hwnd, (INT)wParam, (HIMAGELIST)lParam);

  case LVM_SETITEMA:
    return LISTVIEW_SetItemA(hwnd, (LPLVITEMA)lParam);

/*	case LVM_SETITEMW: */

  case LVM_SETITEMCOUNT: 
    LISTVIEW_SetItemCount(hwnd, (INT)wParam);
    break;
    
  case LVM_SETITEMPOSITION:
    return LISTVIEW_SetItemPosition(hwnd, (INT)wParam, (INT)LOWORD(lParam),
                                    (INT)HIWORD(lParam));

/*	case LVM_SETITEMPOSITION: */

  case LVM_SETITEMSTATE: 
    return LISTVIEW_SetItemState(hwnd, (INT)wParam, (LPLVITEMA)lParam);

  case LVM_SETITEMTEXTA:
    return LISTVIEW_SetItemTextA(hwnd, (INT)wParam, (LPLVITEMA)lParam);

/*	case LVM_SETSELECTIONMARK: */

  case LVM_SETTEXTBKCOLOR:
    return LISTVIEW_SetTextBkColor(hwnd, (COLORREF)lParam);

  case LVM_SETTEXTCOLOR:
    return LISTVIEW_SetTextColor(hwnd, (COLORREF)lParam);

/*	case LVM_SETTOOLTIPS: */
/*	case LVM_SETUNICODEFORMAT: */
/*	case LVM_SETWORKAREAS: */

  case LVM_SORTITEMS:
    return LISTVIEW_SortItems(hwnd, wParam, lParam);

/*	case LVM_SUBITEMHITTEST: */

  case LVM_UPDATE: 
    return LISTVIEW_Update(hwnd, (INT)wParam);

/*	case WM_CHAR: */
/*	case WM_COMMAND: */

  case WM_CREATE:
    return LISTVIEW_Create(hwnd, wParam, lParam);
    
  case WM_ERASEBKGND:
    return LISTVIEW_EraseBackground(hwnd, wParam, lParam);

  case WM_GETDLGCODE:
    return DLGC_WANTTAB | DLGC_WANTARROWS;

  case WM_GETFONT:
    return LISTVIEW_GetFont(hwnd);

  case WM_HSCROLL:
    return LISTVIEW_HScroll(hwnd, (INT)LOWORD(wParam), 
                            (INT)HIWORD(wParam), (HWND)lParam);

  case WM_KEYDOWN:
    return LISTVIEW_KeyDown(hwnd, (INT)wParam, (LONG)lParam);

  case WM_KILLFOCUS:
    return LISTVIEW_KillFocus(hwnd);

  case WM_LBUTTONDBLCLK:
    return LISTVIEW_LButtonDblClk(hwnd, (WORD)wParam, LOWORD(lParam), 
                                HIWORD(lParam));
    
  case WM_LBUTTONDOWN:
    return LISTVIEW_LButtonDown(hwnd, (WORD)wParam, LOWORD(lParam), 
                                HIWORD(lParam));
  case WM_LBUTTONUP:
    return LISTVIEW_LButtonUp(hwnd, (WORD)wParam, LOWORD(lParam), 
                              HIWORD(lParam));
    
/*	case WM_MOUSEMOVE: */
/*	    return LISTVIEW_MouseMove (hwnd, wParam, lParam); */

  case WM_NCCREATE:
    return LISTVIEW_NCCreate(hwnd, wParam, lParam);

  case WM_NCDESTROY:
    return LISTVIEW_NCDestroy(hwnd);

  case WM_NOTIFY:
    return LISTVIEW_Notify(hwnd, (INT)wParam, (LPNMHDR)lParam);

  case WM_NOTIFYFORMAT:
    return LISTVIEW_NotifyFormat(hwnd, (HWND)wParam, (INT)lParam);

  case WM_PAINT: 
    return LISTVIEW_Paint(hwnd, (HDC)wParam); 

  case WM_RBUTTONDBLCLK:
    return LISTVIEW_RButtonDblClk(hwnd, (WORD)wParam, LOWORD(lParam), 
                                  HIWORD(lParam));

  case WM_RBUTTONDOWN:
    return LISTVIEW_RButtonDown(hwnd, (WORD)wParam, LOWORD(lParam), 
                                HIWORD(lParam));

  case WM_RBUTTONUP:
    return LISTVIEW_RButtonUp(hwnd, (WORD)wParam, LOWORD(lParam), 
                              HIWORD(lParam));

  case WM_SETFOCUS:
    return LISTVIEW_SetFocus(hwnd, (HWND)wParam);

  case WM_SETFONT:
    return LISTVIEW_SetFont(hwnd, (HFONT)wParam, (WORD)lParam);

/*	case WM_SETREDRAW: */

  case WM_SIZE:
    return LISTVIEW_Size(hwnd, (int)SLOWORD(lParam), (int)SHIWORD(lParam));

  case WM_STYLECHANGED:
    return LISTVIEW_StyleChanged(hwnd, wParam, (LPSTYLESTRUCT)lParam);

/*	case WM_TIMER: */

  case WM_VSCROLL:
    return LISTVIEW_VScroll(hwnd, (INT)LOWORD(wParam), 
                            (INT)HIWORD(wParam), (HWND)lParam);

/*	case WM_WINDOWPOSCHANGED: */
/*	case WM_WININICHANGE: */

  default:
    if (uMsg >= WM_USER)
    {
      ERR(listview, "unknown msg %04x wp=%08x lp=%08lx\n", uMsg, wParam, 
          lParam);
    }

    /* call default window procedure */
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Registers the window class.
 * 
 * PARAMETER(S):
 * None
 *
 * RETURN:
 * None
 */
VOID LISTVIEW_Register(VOID)
{
  WNDCLASSA wndClass;

  if (!GlobalFindAtomA(WC_LISTVIEWA)) 
  {
    ZeroMemory(&wndClass, sizeof(WNDCLASSA));
    wndClass.style = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc = (WNDPROC)LISTVIEW_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = sizeof(LISTVIEW_INFO *);
    wndClass.hCursor = LoadCursorA(0, IDC_ARROWA);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = WC_LISTVIEWA;
    RegisterClassA(&wndClass);
  }
}

/***
 * DESCRIPTION:
 * Unregisters the window class.
 * 
 * PARAMETER(S):
 * None
 *
 * RETURN:
 * None
 */
VOID LISTVIEW_Unregister(VOID)
{
  if (GlobalFindAtomA(WC_LISTVIEWA))
  {
    UnregisterClassA(WC_LISTVIEWA, (HINSTANCE)NULL);
  }
}

