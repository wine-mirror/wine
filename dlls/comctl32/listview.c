/*
 * Listview control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 1999 Luc Tourangeau
 *
 * NOTES
 * Listview control implementation. 
 *
 * TODO:
 *   1. No horizontal scrolling when header is larger than the client area.
 *   2. Drawing optimizations.
 *   3. Hot item handling.
 *
 * Notifications:
 *   LISTVIEW_Notify : most notifications from children (editbox and header)
 *
 * Data structure:
 *   LISTVIEW_SetItemCount : empty stub 
 * 
 * Unicode:
 *   LISTVIEW_SetItemW : no unicode support
 *   LISTVIEW_InsertItemW : no unicode support
 *   LISTVIEW_InsertColumnW : no unicode support
 *   LISTVIEW_GetColumnW : no unicode support
 *   LISTVIEW_SetColumnW : no unicode support
 *
 * Advanced functionality:
 *   LISTVIEW_GetNumberOfWorkAreas : not implemented
 *   LISTVIEW_GetHotCursor : not implemented
 *   LISTVIEW_GetHoverTime : not implemented
 *   LISTVIEW_GetISearchString : not implemented 
 *   LISTVIEW_GetBkImage : not implemented
 *   LISTVIEW_EditLabel : REPORT (need to implement a timer)
 *   LISTVIEW_GetColumnOrderArray : not implemented
 *   LISTVIEW_SetColumnOrderArray : not implemented
 *   LISTVIEW_Arrange : empty stub
 *   LISTVIEW_ApproximateViewRect : incomplete
 *   LISTVIEW_Scroll : not implemented 
 *   LISTVIEW_RedrawItems : empty stub
 *   LISTVIEW_Update : not completed
 */

#include <string.h>
#include "winbase.h"
#include "commctrl.h"
#include "listview.h"
#include "debugtools.h"

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
#define GETITEMCOUNT(infoPtr) ((infoPtr)->hdpaItems->nItemCount)
#define ListView_LVNotify(hwnd,lCtrlId,plvnm) \
    (BOOL)SendMessageA((hwnd),WM_NOTIFY,(WPARAM)(INT)lCtrlId,(LPARAM)(LPNMLISTVIEW)(plvnm))
#define ListView_Notify(hwnd,lCtrlId,pnmh) \
    (BOOL)SendMessageA((hwnd),WM_NOTIFY,(WPARAM)(INT)lCtrlId,(LPARAM)(LPNMHDR)(pnmh))
/* retrieve the number of items in the listview */
#define GETITEMCOUNT(infoPtr) ((infoPtr)->hdpaItems->nItemCount)

/* 
 * forward declarations 
 */
static INT LISTVIEW_HitTestItem(HWND, LPLVHITTESTINFO);
static INT LISTVIEW_GetCountPerRow(HWND);
static INT LISTVIEW_GetCountPerColumn(HWND);
static VOID LISTVIEW_AlignLeft(HWND);
static VOID LISTVIEW_AlignTop(HWND);
static VOID LISTVIEW_AddGroupSelection(HWND, INT);
static VOID LISTVIEW_AddSelection(HWND, INT);
static BOOL LISTVIEW_AddSubItem(HWND, LPLVITEMA);
static INT LISTVIEW_FindInsertPosition(HDPA, INT);
static INT LISTVIEW_GetItemHeight(HWND);
static BOOL LISTVIEW_GetItemPosition(HWND, INT, LPPOINT);
static LRESULT LISTVIEW_GetItemRect(HWND, INT, LPRECT);
static INT LISTVIEW_GetItemWidth(HWND);
static INT LISTVIEW_GetLabelWidth(HWND, INT);
static LRESULT LISTVIEW_GetOrigin(HWND, LPPOINT);
static LISTVIEW_SUBITEM* LISTVIEW_GetSubItem(HDPA, INT);
static LRESULT LISTVIEW_GetViewRect(HWND, LPRECT);
static BOOL LISTVIEW_InitItem(HWND, LISTVIEW_ITEM *, LPLVITEMA);
static BOOL LISTVIEW_InitSubItem(HWND, LISTVIEW_SUBITEM *, LPLVITEMA);
static LRESULT LISTVIEW_MouseSelection(HWND, POINT);
static BOOL LISTVIEW_RemoveColumn(HDPA, INT);
static VOID LISTVIEW_RemoveSelections(HWND, INT, INT);
static BOOL LISTVIEW_RemoveSubItem(HDPA, INT);
static VOID LISTVIEW_SetGroupSelection(HWND, INT);
static BOOL LISTVIEW_SetItem(HWND, LPLVITEMA);
static BOOL LISTVIEW_SetItemFocus(HWND, INT);
static BOOL LISTVIEW_SetItemPosition(HWND, INT, INT, INT);
static VOID LISTVIEW_UpdateScroll(HWND);
static VOID LISTVIEW_SetSelection(HWND, INT);
static VOID LISTVIEW_UpdateSize(HWND);
static BOOL LISTVIEW_SetSubItem(HWND, LPLVITEMA);
static LRESULT LISTVIEW_SetViewRect(HWND, LPRECT);
static BOOL LISTVIEW_ToggleSelection(HWND, INT);
static VOID LISTVIEW_UnsupportedStyles(LONG lStyle);

/***
 * DESCRIPTION:
 * Update the scrollbars. This functions should be called whenever 
 * the content, size or view changes.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_UpdateScroll(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0); 
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  UINT uView =  lStyle & LVS_TYPEMASK;
  INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
  INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
  SCROLLINFO scrollInfo;

  ZeroMemory(&scrollInfo, sizeof(SCROLLINFO));
  scrollInfo.cbSize = sizeof(SCROLLINFO);

  if (uView == LVS_LIST)
  {
    INT nCountPerColumn = LISTVIEW_GetCountPerColumn(hwnd);
    INT nCountPerPage = LISTVIEW_GetCountPerRow(hwnd) * nCountPerColumn;
    if (nCountPerPage < GETITEMCOUNT(infoPtr))
    {
      /* calculate new scrollbar range */
      INT nHiddenItemCount = GETITEMCOUNT(infoPtr) - nCountPerPage;
      if (nHiddenItemCount % nCountPerColumn == 0)
      {
        scrollInfo.nMax = nHiddenItemCount / nCountPerColumn;
      }
      else
      {
        scrollInfo.nMax = nHiddenItemCount / nCountPerColumn + 1;
      }
      
      scrollInfo.nPos = ListView_GetTopIndex(hwnd) / nCountPerColumn;
      /*scrollInfo.nPage = LISTVIEW_GetCountPerRow(hwnd);*/
      scrollInfo.fMask = SIF_RANGE | SIF_POS /*| SIF_PAGE*/;
      SetScrollInfo(hwnd, SB_HORZ, &scrollInfo, TRUE);
    }
    else
    {
      /* hide scrollbar */
      if (lStyle & WS_HSCROLL)
      {
	ShowScrollBar(hwnd, SB_HORZ, FALSE);
      }
    }
  }
  else if (uView == LVS_REPORT)
  {
    /* update vertical scrollbar */
    scrollInfo.nMin = 0;
    scrollInfo.nMax = GETITEMCOUNT(infoPtr) - LISTVIEW_GetCountPerColumn(hwnd);
    scrollInfo.nPos = ListView_GetTopIndex(hwnd);
    /*scrollInfo.nPage = nCountPerColumn;*/
    scrollInfo.fMask = SIF_RANGE | SIF_POS /*| SIF_PAGE*/;
    SetScrollInfo(hwnd, SB_VERT, &scrollInfo, TRUE);

    /* update horizontal scrollbar */
/*     if (infoPtr->nItemWidth > nListWidth) */
/*     { */
/*       scrollInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE; */
/*       SetScrollInfo(hwnd, SB_HORZ, &scrollInfo, TRUE); */
/*     } */

    /* horizontal scrolling has not been implemented yet! I experienced some 
       problems when performing child window scrolling. */
  }
  else
  {
    RECT rcView;

    if (LISTVIEW_GetViewRect(hwnd, &rcView) != FALSE)
    {
      INT nViewWidth = rcView.right - rcView.left;
      INT nViewHeight = rcView.bottom - rcView.top;

      if (nViewWidth > nListWidth)
      {
        INT nHiddenWidth;
        INT nScrollPosWidth = nListWidth / 10;

        if (nScrollPosWidth == 0)
        {
          nScrollPosWidth = 1;
          nHiddenWidth = nViewWidth - nListWidth;
        }
        else
        {
          nHiddenWidth = nViewWidth - nScrollPosWidth * 10;
        }

        scrollInfo.fMask = SIF_POS;
        if (GetScrollInfo(hwnd, SB_HORZ, &scrollInfo) == FALSE)
        {
          scrollInfo.nPos = 0;
        }
  
        if (nHiddenWidth % nScrollPosWidth == 0)
        {
          scrollInfo.nMax = nHiddenWidth / nScrollPosWidth; 
        }
        else
        {
          scrollInfo.nMax = nHiddenWidth / nScrollPosWidth + 1; 
        }
          
        scrollInfo.nMin = 0;
        /*scrollInfo.nPage = 10;*/
        scrollInfo.fMask = SIF_RANGE | SIF_POS /*| SIF_PAGE*/;
        SetScrollInfo(hwnd, SB_HORZ, &scrollInfo, TRUE);
      }
      else
      {
        if (lStyle & WS_HSCROLL)
        {
          ShowScrollBar(hwnd, SB_HORZ, FALSE);
        }
      }

      if (nViewHeight > nListHeight)
      {
        INT nHiddenHeight;
        INT nScrollPosHeight = nListHeight / 10;

        if (nScrollPosHeight == 0)
        {
          nScrollPosHeight = 1;
          nHiddenHeight = nViewHeight - nListHeight;
        }
        else
        {
          nHiddenHeight = nViewHeight - nScrollPosHeight * 10;
        }

        scrollInfo.fMask = SIF_POS;
        if (GetScrollInfo(hwnd, SB_VERT, &scrollInfo) == FALSE)
        {
          scrollInfo.nPos = 0;
        }

        if (nHiddenHeight % nScrollPosHeight == 0)
        {
          scrollInfo.nMax = nHiddenHeight / nScrollPosHeight; 
        }
        else
        {
          scrollInfo.nMax = nHiddenHeight / nScrollPosHeight + 1; 
        }

        scrollInfo.nMin = 0;
        /*scrollInfo.nPage = 10;*/
        scrollInfo.fMask = SIF_RANGE | SIF_POS /*| SIF_PAGE*/;
        SetScrollInfo(hwnd, SB_VERT, &scrollInfo, TRUE);
      }
      else
      {
        if (lStyle & WS_VSCROLL)
        {
          ShowScrollBar(hwnd, SB_VERT, FALSE);
        }
      }
    }
  }
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
    FIXME("  LVS_EDITLABELS\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_NOLABELWRAP)
  {
    FIXME("  LVS_NOLABELWRAP\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_NOSCROLL)
  {
    FIXME("  LVS_NOSCROLL\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_NOSORTHEADER)
  {
    FIXME("  LVS_NOSORTHEADER\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_OWNERDRAWFIXED)
  {
    FIXME("  LVS_OWNERDRAWFIXED\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_SHAREIMAGELISTS)
  {
    FIXME("  LVS_SHAREIMAGELISTS\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_SORTASCENDING)
  {
    FIXME("  LVS_SORTASCENDING\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_SORTDESCENDING)
  {
    FIXME("  LVS_SORTDESCENDING\n");
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
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
  POINT ptItem;
  RECT rcView;
  INT i;
  
  if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
  {
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
        
        ListView_SetItemPosition(hwnd, i, ptItem.x, ptItem.y);
        ptItem.x += infoPtr->nItemWidth;
        rcView.right = max(rcView.right, ptItem.x);
      }

      rcView.bottom = ptItem.y + infoPtr->nItemHeight;
    }
    else
    {
      for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
      {
        ListView_SetItemPosition(hwnd, i, ptItem.x, ptItem.y);
        ptItem.y += infoPtr->nItemHeight;
      }

      rcView.right = infoPtr->nItemWidth;
      rcView.bottom = ptItem.y;
    }

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
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
  POINT ptItem;
  RECT rcView;
  INT i;
  
  if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
  {
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

        ListView_SetItemPosition(hwnd, i, ptItem.x, ptItem.y);
        ptItem.y += infoPtr->nItemHeight;
        rcView.bottom = max(rcView.bottom, ptItem.y);
      }

      rcView.right = ptItem.x + infoPtr->nItemWidth;
    }
    else
    {
      for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
      {
        ListView_SetItemPosition(hwnd, i, ptItem.x, ptItem.y);
        ptItem.x += infoPtr->nItemWidth;
      }

      rcView.bottom = infoPtr->nItemHeight;
      rcView.right = ptItem.x;
    }

    LISTVIEW_SetViewRect(hwnd, &rcView);
  }
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
  BOOL bResult = FALSE;

  TRACE("(hwnd=%x, left=%d, top=%d, right=%d, bottom=%d)\n", hwnd, 
        lprcView->left, lprcView->top, lprcView->right, lprcView->bottom);
  
  if (lprcView != NULL)
  {
    bResult = TRUE;
    infoPtr->rcView.left = lprcView->left;
    infoPtr->rcView.top = lprcView->top;
    infoPtr->rcView.right = lprcView->right;
    infoPtr->rcView.bottom = lprcView->bottom;
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

  TRACE("(hwnd=%x, lprcView=%p)\n", hwnd, lprcView);

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

    TRACE("(left=%d, top=%d, right=%d, bottom=%d)\n", 
          lprcView->left, lprcView->top, lprcView->right, lprcView->bottom);
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Retrieves the subitem pointer associated with the subitem index.
 * 
 * PARAMETER(S):
 * [I] HDPA : DPA handle for a specific item
 * [I] INT : index of subitem
 *
 * RETURN:
 *   SUCCESS : subitem pointer
 *   FAILURE : NULL
 */
static LISTVIEW_SUBITEM* LISTVIEW_GetSubItemPtr(HDPA hdpaSubItems, 
                                                INT nSubItem)
{
  LISTVIEW_SUBITEM *lpSubItem;
  INT i;

  for (i = 1; i < hdpaSubItems->nItemCount; i++)
  {
    lpSubItem = (LISTVIEW_SUBITEM *) DPA_GetPtr(hdpaSubItems, i);
    if (lpSubItem != NULL)
    {
      if (lpSubItem->iSubItem == nSubItem)
      {
        return lpSubItem;
      }
    }
  }

  return NULL;
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
static INT LISTVIEW_GetItemWidth(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  INT nHeaderItemCount;
  RECT rcHeaderItem;
  INT nItemWidth = 0;
  INT nLabelWidth;
  INT i;

  TRACE("(hwnd=%x)\n", hwnd);

  if (uView == LVS_ICON)
  {
    nItemWidth = infoPtr->iconSpacing.cx;
  }
  else if (uView == LVS_REPORT)
  {
    /* calculate width of header */
    nHeaderItemCount = Header_GetItemCount(infoPtr->hwndHeader);
    for (i = 0; i < nHeaderItemCount; i++)
    {
      if (Header_GetItemRect(infoPtr->hwndHeader, i, &rcHeaderItem) != 0)
      {
        nItemWidth += (rcHeaderItem.right - rcHeaderItem.left);
      }
    }
  }
  else
  {
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
 * Returns item height.
 */
static INT LISTVIEW_GetItemHeight(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  INT nItemHeight = 0;

  if (uView == LVS_ICON)
  {
    nItemHeight = infoPtr->iconSpacing.cy;
  }
  else 
  {
    TEXTMETRICA tm; 
    HDC hdc = GetDC(hwnd);
    HFONT hOldFont = SelectObject(hdc, infoPtr->hFont);
    GetTextMetricsA(hdc, &tm);
    nItemHeight = max(tm.tmHeight, infoPtr->iconSize.cy) + HEIGHT_PADDING;
    SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);
  }

  return nItemHeight;
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
 * Selects items based on view coorddiantes. 
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] RECT : selection rectangle  
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_SetSelectionRect(HWND hwnd, RECT rcSelRect)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LVITEMA lvItem;
  POINT ptItem;
  INT i;

  lvItem.stateMask = LVIS_SELECTED;

  for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
  {
    LISTVIEW_GetItemPosition(hwnd, i, &ptItem);
    if (PtInRect(&rcSelRect, ptItem) != FALSE)
    {
      lvItem.state = LVIS_SELECTED;
    }
    else
    {
      lvItem.state = 0;
    }

    ListView_SetItemState(hwnd, i, &lvItem);
  }
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
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  LVITEMA lvItem;

  if ((uView == LVS_LIST) || (uView == LVS_REPORT))
  {
    INT i;
    INT nFirst = min(infoPtr->nSelectionMark, nItem);
    INT nLast = max(infoPtr->nSelectionMark, nItem);
    lvItem.stateMask = LVIS_SELECTED;
   
    for (i = 0; i <= GETITEMCOUNT(infoPtr); i++)
    {
      if ((i < nFirst) || (i > nLast))
      {
        lvItem.state = 0;
      }
      else
      {
        lvItem.state = LVIS_SELECTED;
      }

      ListView_SetItemState(hwnd, i, &lvItem);
    }
  }
  else
  {
    POINT ptItem;
    POINT ptSelMark;
    RECT rcSel;
    LISTVIEW_GetItemPosition(hwnd, nItem, &ptItem);
    LISTVIEW_GetItemPosition(hwnd, infoPtr->nSelectionMark, &ptSelMark);
    rcSel.left = min(ptSelMark.x, ptItem.x);
    rcSel.top = min(ptSelMark.y, ptItem.y);
    rcSel.right = max(ptSelMark.x, ptItem.x) + infoPtr->nItemWidth;
    rcSel.bottom = max(ptSelMark.y, ptItem.y) + infoPtr->nItemHeight;
    LISTVIEW_SetSelectionRect(hwnd, rcSel);
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
 *   TRUE : focused item changed
 *   FALSE : focused item has NOT changed
 */
static BOOL LISTVIEW_SetItemFocus(HWND hwnd, INT nItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  BOOL bResult = FALSE;
  LVITEMA lvItem;

  if (infoPtr->nFocusedItem != nItem)
  {
    bResult = TRUE;
    ZeroMemory(&lvItem, sizeof(LVITEMA));
    lvItem.stateMask = LVIS_FOCUSED;
    ListView_SetItemState(hwnd, infoPtr->nFocusedItem, &lvItem); 

    lvItem.state =  LVIS_FOCUSED;
    lvItem.stateMask = LVIS_FOCUSED;
    ListView_SetItemState(hwnd, nItem, &lvItem);

    infoPtr->nFocusedItem = nItem;
    ListView_EnsureVisible(hwnd, nItem, FALSE);
  }
  
  return bResult; 
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

  ZeroMemory(&lvItem, sizeof(LVITEMA));
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
 *   SUCCESS : TRUE (needs to be repainted)
 *   FAILURE : FALSE (nothing has changed)
 */
static BOOL LISTVIEW_KeySelection(HWND hwnd, INT nItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  WORD wShift = HIWORD(GetKeyState(VK_SHIFT));
  WORD wCtrl = HIWORD(GetKeyState(VK_CONTROL));
  BOOL bResult = FALSE;

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
  {
    if (lStyle & LVS_SINGLESEL)
    {
      bResult = TRUE;
      LISTVIEW_SetSelection(hwnd, nItem); 
      ListView_EnsureVisible(hwnd, nItem, FALSE);
    }
    else
    {
      if (wShift)
      {
        bResult = TRUE;
        LISTVIEW_SetGroupSelection(hwnd, nItem); 
      }
      else if (wCtrl)
      {
        bResult = LISTVIEW_SetItemFocus(hwnd, nItem);
      }
      else
      {
        bResult = TRUE;
        LISTVIEW_SetSelection(hwnd, nItem); 
        ListView_EnsureVisible(hwnd, nItem, FALSE);
      }
    }
  }

  return bResult;
}
  
/***
 * DESCRIPTION:
 * Selects an item based on coordinates.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] POINT : mouse click ccordinates
 *
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_MouseSelection(HWND hwnd, POINT pt)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  RECT rcItem;
  INT i;
  
  for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
  {
    rcItem.left = LVIR_SELECTBOUNDS;
    if (LISTVIEW_GetItemRect(hwnd, i, &rcItem) == TRUE)
    {
      if (PtInRect(&rcItem, pt) != FALSE)
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
	else
	{
	  if (lpLVItem->pszText)
	  {
	    if (lpItem->pszText)
	    {
	      if (strcmp(lpLVItem->pszText, lpItem->pszText) != 0)
	      {
		uChanged |= LVIF_TEXT;
	      }
	    }
	    else
	    {
	      uChanged |= LVIF_TEXT;
	    }
	  }
	  else
	  {
	    if (lpItem->pszText)
	    {
	      uChanged |= LVIF_TEXT;
	    }
	  }
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
 * NOTE: The documentation specifies that the operation fails if the user
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
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  UINT uView = lStyle & LVS_TYPEMASK;
  INT nItem = 0;
  SCROLLINFO scrollInfo;

  ZeroMemory(&scrollInfo, sizeof(SCROLLINFO));
  scrollInfo.cbSize = sizeof(SCROLLINFO);
  scrollInfo.fMask = SIF_POS;
  
  if (uView == LVS_LIST)
  {
    if (lStyle & WS_HSCROLL)
    {
      if (GetScrollInfo(hwnd, SB_HORZ, &scrollInfo) != FALSE)
      {
        nItem = scrollInfo.nPos * LISTVIEW_GetCountPerColumn(hwnd);
      }
    }
  }
  else if (uView == LVS_REPORT)
  {
    if (lStyle & WS_VSCROLL)
    {
      if (GetScrollInfo(hwnd, SB_VERT, &scrollInfo) != FALSE)
      {
        nItem = scrollInfo.nPos;
      }
    }
  }
  
  return nItem;
}

/***
 * DESCRIPTION:
 * Draws a subitem.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] HDC : device context handle
 * [I] INT : item index
 * [I] INT : subitem index
 * [I] RECT * : clipping rectangle
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_DrawSubItem(HWND hwnd, HDC hdc, INT nItem, INT nSubItem, 
                                 RECT rcItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0); 
  CHAR szDispText[DISP_TEXT_SIZE];
  LVITEMA lvItem;

  TRACE("(hwnd=%x, hdc=%x, nItem=%d, nSubItem=%d\n", hwnd, hdc, 
        nItem, nSubItem);

  /* get information needed for drawing the item */
  ZeroMemory(&lvItem, sizeof(LVITEMA));
  lvItem.mask = LVIF_TEXT;
  lvItem.iItem = nItem;
  lvItem.iSubItem = nSubItem;
  lvItem.cchTextMax = DISP_TEXT_SIZE;
  lvItem.pszText = szDispText;
  ListView_GetItemA(hwnd, &lvItem);

  /* set item colors */
  SetBkColor(hdc, infoPtr->clrTextBk);
  SetTextColor(hdc, infoPtr->clrText);
  
  ExtTextOutA(hdc, rcItem.left, rcItem.top, ETO_OPAQUE | ETO_CLIPPED, 
              &rcItem, lvItem.pszText, lstrlenA(lvItem.pszText), NULL);
}


/***
 * DESCRIPTION:
 * Draws an item.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] HDC : device context handle
 * [I] INT : item index
 * [I] RECT * : clipping rectangle
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_DrawItem(HWND hwnd, HDC hdc, INT nItem, RECT rcItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0); 
  CHAR szDispText[DISP_TEXT_SIZE];
  INT nLabelWidth;
  LVITEMA lvItem;
  INT nMixMode;
  DWORD dwBkColor;
  DWORD dwTextColor;

  TRACE("(hwnd=%x, hdc=%x, nItem=%d\n", hwnd, hdc, nItem);

  /* get information needed for drawing the item */
  ZeroMemory(&lvItem, sizeof(LVITEMA));
  lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_INDENT;
  lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED | LVIS_STATEIMAGEMASK;
  lvItem.iItem = nItem;
  lvItem.iSubItem = 0;
  lvItem.cchTextMax = DISP_TEXT_SIZE;
  lvItem.pszText = szDispText;
  ListView_GetItemA(hwnd, &lvItem);

  /* state icons */
  if (infoPtr->himlState != NULL)
  {
     UINT uStateImage = (lvItem.state & LVIS_STATEIMAGEMASK) >> 12; 
     if (uStateImage != 0)
     {
       ImageList_Draw(infoPtr->himlState, uStateImage - 1, hdc, rcItem.left, 
                      rcItem.top, ILD_NORMAL);
     }
 
     rcItem.left += infoPtr->iconSize.cx; 
  }
  
  /* small icons */
  if (infoPtr->himlSmall != NULL)
  {
    if (lvItem.state & LVIS_SELECTED)
    {
      ImageList_SetBkColor(infoPtr->himlSmall, CLR_NONE);
      ImageList_Draw(infoPtr->himlSmall, lvItem.iImage, hdc, rcItem.left, 
                     rcItem.top, ILD_SELECTED);
    }
    else
    {
      ImageList_SetBkColor(infoPtr->himlSmall, CLR_NONE);
      ImageList_Draw(infoPtr->himlSmall, lvItem.iImage, hdc, rcItem.left, 
                     rcItem.top, ILD_NORMAL);
    }
    
    rcItem.left += infoPtr->iconSize.cx; 
  }

  if ((lvItem.state & LVIS_SELECTED) && (infoPtr->bFocus != FALSE))
  {
    /* set item colors */ 
    dwBkColor = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
    dwTextColor = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
    /* set raster mode */
    nMixMode = SetROP2(hdc, R2_XORPEN);
  }
  else if ((GetWindowLongA(hwnd, GWL_STYLE) & LVS_SHOWSELALWAYS) && 
           (lvItem.state & LVIS_SELECTED) && (infoPtr->bFocus == FALSE))
  {
    dwBkColor = SetBkColor(hdc, GetSysColor(COLOR_3DFACE));
    dwTextColor = SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
    /* set raster mode */
    nMixMode = SetROP2(hdc, R2_COPYPEN);
  }
  else
  {
    /* set item colors */
    dwBkColor = SetBkColor(hdc, infoPtr->clrTextBk);
    dwTextColor = SetTextColor(hdc, infoPtr->clrText);
    /* set raster mode */
    nMixMode = SetROP2(hdc, R2_COPYPEN);
  }
  
  nLabelWidth = ListView_GetStringWidthA(hwnd, lvItem.pszText);
  if (rcItem.left + nLabelWidth < rcItem.right)
  {
    rcItem.right = rcItem.left + nLabelWidth;
  }
  
  /* draw label */  
  ExtTextOutA(hdc, rcItem.left, rcItem.top, ETO_OPAQUE | ETO_CLIPPED, 
              &rcItem, lvItem.pszText, lstrlenA(lvItem.pszText), NULL);
  
  if ((lvItem.state & LVIS_FOCUSED) && (infoPtr->bFocus == TRUE))
  {
    Rectangle(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom); 
  }

  if (nMixMode != 0)
  {
    SetROP2(hdc, R2_COPYPEN);
    SetBkColor(hdc, infoPtr->clrTextBk);
    SetTextColor(hdc, infoPtr->clrText);
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
static VOID LISTVIEW_DrawLargeItem(HWND hwnd, HDC hdc, INT nItem, RECT rcItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0); 
  CHAR szDispText[DISP_TEXT_SIZE];
  INT nDrawPosX = rcItem.left;
  INT nLabelWidth;
  TEXTMETRICA tm;
  LVITEMA lvItem;

  TRACE("(hwnd=%x, hdc=%x, nItem=%d, left=%d, top=%d, right=%d, \
bottom=%d)\n", hwnd, hdc, nItem, rcItem.left, rcItem.top, rcItem.right, 
        rcItem.bottom);

  /* get information needed for drawing the item */
  ZeroMemory(&lvItem, sizeof(LVITEMA));
  lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
  lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
  lvItem.iItem = nItem;
  lvItem.iSubItem = 0;
  lvItem.cchTextMax = DISP_TEXT_SIZE;
  lvItem.pszText = szDispText;
  ListView_GetItemA(hwnd, &lvItem);

  if (lvItem.state & LVIS_SELECTED)
  {
    /* set item colors */ 
    SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
    SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
    /* set raster mode */
    SetROP2(hdc, R2_XORPEN);
  }
  else
  {
    /* set item colors */
    SetBkColor(hdc, infoPtr->clrTextBk);
    SetTextColor(hdc, infoPtr->clrText);
    /* set raster mode */
    SetROP2(hdc, R2_COPYPEN);
  }

  if (infoPtr->himlNormal != NULL)
  {
    rcItem.top += ICON_TOP_PADDING;
    nDrawPosX += (infoPtr->iconSpacing.cx - infoPtr->iconSize.cx) / 2;
    if (lvItem.state & LVIS_SELECTED)
    {
      ImageList_Draw(infoPtr->himlNormal, lvItem.iImage, hdc, nDrawPosX, 
                     rcItem.top, ILD_SELECTED);
    }
    else
    {
      ImageList_Draw(infoPtr->himlNormal, lvItem.iImage, hdc, nDrawPosX, 
                     rcItem.top, ILD_NORMAL);
    }
  }

  rcItem.top += infoPtr->iconSize.cy + ICON_BOTTOM_PADDING; 
  nLabelWidth = ListView_GetStringWidthA(hwnd, lvItem.pszText);
  nDrawPosX = infoPtr->iconSpacing.cx - nLabelWidth;
  if (nDrawPosX > 1)
  {
    rcItem.left += nDrawPosX / 2;
    rcItem.right = rcItem.left + nLabelWidth;
  }
  else
  {
    rcItem.left += 1;
    rcItem.right = rcItem.left + infoPtr->iconSpacing.cx - 1;
  }

  /* draw label */  
  GetTextMetricsA(hdc, &tm);
  rcItem.bottom = rcItem.top + tm.tmHeight + HEIGHT_PADDING; 
  ExtTextOutA(hdc, rcItem.left, rcItem.top, ETO_OPAQUE | ETO_CLIPPED, 
              &rcItem, lvItem.pszText, lstrlenA(lvItem.pszText), NULL);
        
  if (lvItem.state & LVIS_FOCUSED)
  {
    Rectangle(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom); 
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
  INT nColumnCount;
  RECT rcItem;
  INT  j;
  INT nItem;
  INT nLast;

  nItem = ListView_GetTopIndex(hwnd);

  /* add 1 for displaying a partial item at the bottom */
  nLast = nItem + LISTVIEW_GetCountPerColumn(hwnd) + 1;
  nLast = min(nLast, GETITEMCOUNT(infoPtr));
  for (; nItem < nLast; nItem++)
  {
    nColumnCount = Header_GetItemCount(infoPtr->hwndHeader);
    for (j = 0; j < nColumnCount; j++)
    {
      Header_GetItemRect(infoPtr->hwndHeader, j, &rcItem);
      rcItem.left += REPORT_MARGINX;
      rcItem.right = max(rcItem.left, rcItem.right - REPORT_MARGINX);
      rcItem.top = nDrawPosY;
      rcItem.bottom = rcItem.top + infoPtr->nItemHeight;
      if (j == 0)
      {
        LISTVIEW_DrawItem(hwnd, hdc, nItem, rcItem); 
      }
      else 
      {
        LISTVIEW_DrawSubItem(hwnd, hdc, nItem, j, rcItem);
      }
    }
    
    nDrawPosY += infoPtr->nItemHeight;
  }
}

/***
 * DESCRIPTION:
 * Retrieves the number of items that can fit vertically in the client area.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * Number of items per row.
 */
static INT LISTVIEW_GetCountPerRow(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd,0);
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
  INT nCountPerRow = 1;

  if (nListWidth > 0)
  {
    if (uView == LVS_REPORT)
    {
      nCountPerRow = 1;
    }
    else
    {
      nCountPerRow =  nListWidth / infoPtr->nItemWidth;
      if (nCountPerRow == 0)
      {
        nCountPerRow = 1;
      }
    }
  }

  return nCountPerRow;
}

/***
 * DESCRIPTION:
 * Retrieves the number of items that can fit horizontally in the client 
 * area.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * Number of items per column.
 */
static INT LISTVIEW_GetCountPerColumn(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd,0);
  INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
  INT nCountPerColumn = 1;

  if (nListHeight > 0)
  {
    nCountPerColumn =  nListHeight / infoPtr->nItemHeight;
    if (nCountPerColumn == 0)
    {
      nCountPerColumn = 1;
    }
  }

  return nCountPerColumn;
}

/***
 * DESCRIPTION:
 * Retrieves the number of columns needed to display all the items when in 
 * list display mode.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * Number of columns.
 */
static INT LISTVIEW_GetColumnCount(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nColumnCount = 0;

  if ((lStyle & LVS_TYPEMASK) == LVS_LIST)
  {
    if (infoPtr->rcList.right % infoPtr->nItemWidth == 0)
    {
      nColumnCount = infoPtr->rcList.right / infoPtr->nItemWidth;
    }
    else
    {
      nColumnCount = infoPtr->rcList.right / infoPtr->nItemWidth + 1;
    }
  }

  return nColumnCount;
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
  RECT rcItem;
  INT i, j;
  INT nItem;
  INT nColumnCount;
  INT nCountPerColumn;
  
  /* get number of fully visible columns */
  nColumnCount = LISTVIEW_GetColumnCount(hwnd);
  nCountPerColumn = LISTVIEW_GetCountPerColumn(hwnd);
  nItem = ListView_GetTopIndex(hwnd);

  for (i = 0; i < nColumnCount; i++)
  {
    for (j = 0; j < nCountPerColumn; j++, nItem++)
    {
      if (nItem >= GETITEMCOUNT(infoPtr))
        return;

      rcItem.top = j * infoPtr->nItemHeight;
      rcItem.left = i * infoPtr->nItemWidth;
      rcItem.bottom = rcItem.top + infoPtr->nItemHeight;
      rcItem.right = rcItem.left + infoPtr->nItemWidth;
      LISTVIEW_DrawItem(hwnd, hdc, nItem, rcItem);
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
  POINT ptPosition;
  POINT ptOrigin;
  RECT rcItem;
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
            rcItem.top = ptPosition.y;
            rcItem.left = ptPosition.x;
            rcItem.bottom = rcItem.top + infoPtr->nItemHeight;
            rcItem.right = rcItem.left + infoPtr->nItemWidth;
            if (bSmall == FALSE)
            {
              LISTVIEW_DrawLargeItem(hwnd, hdc, i, rcItem);
            }
            else
            {
              LISTVIEW_DrawItem(hwnd, hdc, i, rcItem);
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
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  HFONT hOldFont;
  HPEN hPen, hOldPen;

  /* select font */
  hOldFont = SelectObject(hdc, infoPtr->hFont);

  /* select the doted pen (for drawing the focus box) */
  hPen = CreatePen(PS_DOT, 1, 0);
  hOldPen = SelectObject(hdc, hPen);

  /* select transparent brush (for drawing the focus box) */
  SelectObject(hdc, GetStockObject(NULL_BRUSH)); 

  if (uView == LVS_LIST)
  {
    LISTVIEW_RefreshList(hwnd, hdc); 
  }
  else if (uView == LVS_REPORT)
  {
    LISTVIEW_RefreshReport(hwnd, hdc);
  }
  else if (uView == LVS_SMALLICON)
  {
    LISTVIEW_RefreshIcon(hwnd, hdc, TRUE);
  }
  else if (uView == LVS_ICON)
  {
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
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  INT nItemCountPerColumn = 1;
  INT nColumnCount = 0;
  DWORD dwViewRect = 0;

  if (nItemCount == -1)
  {
    nItemCount = GETITEMCOUNT(infoPtr);
  }

  if (uView == LVS_LIST)
  {
    if (wHeight == 0xFFFF)
    {
      /* use current height */
      wHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
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
        {
          nItemCountPerColumn = 1;
        }
        
        if (nItemCount % nItemCountPerColumn != 0)
        {
          nColumnCount = nItemCount / nItemCountPerColumn;
        }
        else
        {
          nColumnCount = nItemCount / nItemCountPerColumn + 1;
        }
      }
    }

    /* Microsoft padding magic */
    wHeight = nItemCountPerColumn * infoPtr->nItemHeight + 2;
    wWidth = nColumnCount * infoPtr->nItemWidth + 2;

    dwViewRect = MAKELONG(wWidth, wHeight);
  }
  else if (uView == LVS_REPORT)
  {
    /* TO DO */
  }
  else if (uView == LVS_SMALLICON)
  {
    /* TO DO */
  }
  else if (uView == LVS_ICON)
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
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK; 
  BOOL bResult = FALSE;

  if ((uView == LVS_ICON) || (uView == LVS_SMALLICON))
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
  UINT uView = lStyle & LVS_TYPEMASK;
  LISTVIEW_ITEM *lpItem;
  LISTVIEW_SUBITEM *lpSubItem;
  NMLISTVIEW nmlv;
  BOOL bSuppress;
  BOOL bResult = FALSE;
  INT i;
  INT j;
  HDPA hdpaSubItems;

  TRACE("(hwnd=%x,)\n", hwnd);

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
    if ((uView == LVS_ICON) || (uView == LVS_SMALLICON))
    {
      if (lStyle & LVS_ALIGNLEFT)
      {
        LISTVIEW_AlignLeft(hwnd);
      }
      else
      {
        LISTVIEW_AlignTop(hwnd);
      }
    }
    
    LISTVIEW_UpdateScroll(hwnd);

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
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  BOOL bResult = FALSE;
  
  if (Header_DeleteItem(infoPtr->hwndHeader, nColumn) != FALSE)
  {
    bResult = LISTVIEW_RemoveColumn(infoPtr->hdpaItems, nColumn);

    /* reset scroll parameters */
    if (uView == LVS_REPORT)
    {
      /* update scrollbar(s) */
      LISTVIEW_UpdateScroll(hwnd);

      /* refresh client area */
      InvalidateRect(hwnd, NULL, FALSE);
    }
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
  UINT uView = lStyle & LVS_TYPEMASK;
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMLISTVIEW nmlv;
  BOOL bResult = FALSE;
  HDPA hdpaSubItems;
  LISTVIEW_ITEM *lpItem;
  LISTVIEW_SUBITEM *lpSubItem;
  INT i;

  TRACE("(hwnd=%x,nItem=%d)\n", hwnd, nItem);

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
    if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
    {
      if (lStyle & LVS_ALIGNLEFT)
      {
        LISTVIEW_AlignLeft(hwnd);
      }
      else
      {
        LISTVIEW_AlignTop(hwnd);
      }
    }

    LISTVIEW_UpdateScroll(hwnd);

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
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
  INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
  INT nScrollPosHeight = 0;
  INT nScrollPosWidth = 0;
  SCROLLINFO scrollInfo;
  RECT rcItem;

  ZeroMemory(&scrollInfo, sizeof(SCROLLINFO));
  scrollInfo.cbSize = sizeof(SCROLLINFO);
  scrollInfo.fMask = SIF_POS;

  /* ALWAYS bPartial == FALSE, FOR NOW! */

  rcItem.left = LVIR_BOUNDS;
  if (LISTVIEW_GetItemRect(hwnd, nItem, &rcItem) != FALSE)
  {
    if (rcItem.left < infoPtr->rcList.left)
    {
      if (GetScrollInfo(hwnd, SB_HORZ, &scrollInfo) != FALSE)
      {
        /* scroll left */
        if (uView == LVS_LIST)
        {
          nScrollPosWidth = infoPtr->nItemWidth;
          rcItem.left += infoPtr->rcList.left;
        }
        else if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
        {
          nScrollPosWidth = max(1, nListWidth / 10);
          rcItem.left += infoPtr->rcList.left;
        }
        
        if (rcItem.left % nScrollPosWidth == 0)
        {
          scrollInfo.nPos += rcItem.left / nScrollPosWidth;
        }
        else
        {
          scrollInfo.nPos += rcItem.left / nScrollPosWidth - 1;
        }
        
        SetScrollInfo(hwnd, SB_HORZ, &scrollInfo, TRUE);
      }
    }
    else if (rcItem.right > infoPtr->rcList.right)
    {
      if (GetScrollInfo(hwnd, SB_HORZ, &scrollInfo) != FALSE)
      {
        /* scroll right */
        if (uView == LVS_LIST)
        {
          rcItem.right -= infoPtr->rcList.right;
          nScrollPosWidth = infoPtr->nItemWidth;
        }
        else if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
        {
          rcItem.right -= infoPtr->rcList.right;
          nScrollPosWidth = max(1, nListWidth / 10);
        }

        if (rcItem.right % nScrollPosWidth == 0)
        {
          scrollInfo.nPos += rcItem.right / nScrollPosWidth;
        }
        else
        {
          scrollInfo.nPos += rcItem.right / nScrollPosWidth + 1;
        }

        SetScrollInfo(hwnd, SB_HORZ, &scrollInfo, TRUE);
      }
    }
    
    if (rcItem.top < infoPtr->rcList.top)
    {
      /* scroll up */
      if (GetScrollInfo(hwnd, SB_VERT, &scrollInfo) != FALSE)
      {
        if (uView == LVS_REPORT)
        {
          rcItem.top -= infoPtr->rcList.top; 
          nScrollPosHeight = infoPtr->nItemHeight;
        }
        else if ((uView == LVS_ICON) || (uView == LVS_SMALLICON))
        {
          nScrollPosHeight = max(1, nListHeight / 10);
          rcItem.top += infoPtr->rcList.top;
        }

        if (rcItem.top % nScrollPosHeight == 0)
        {
          scrollInfo.nPos += rcItem.top / nScrollPosHeight;
        }
        else
        {
          scrollInfo.nPos += rcItem.top / nScrollPosHeight - 1;
        }
        
        SetScrollInfo(hwnd, SB_VERT, &scrollInfo, TRUE);
      }
    }
    else if (rcItem.bottom > infoPtr->rcList.bottom)
    {
      /* scroll down */
      if (GetScrollInfo(hwnd, SB_VERT, &scrollInfo) != FALSE)
      {
        if (uView == LVS_REPORT)
        {
          rcItem.bottom -= infoPtr->rcList.bottom;
          nScrollPosHeight = infoPtr->nItemHeight;
        }
        else if ((uView == LVS_ICON) || (uView == LVS_SMALLICON))
        {
          nScrollPosHeight = max(1, nListHeight / 10);
          rcItem.bottom -= infoPtr->rcList.bottom;
        }

        if (rcItem.bottom % nScrollPosHeight == 0)
        {
          scrollInfo.nPos += rcItem.bottom / nScrollPosHeight;
        }
        else
        {
          scrollInfo.nPos += rcItem.bottom / nScrollPosHeight + 1;
        }
        
        SetScrollInfo(hwnd, SB_VERT, &scrollInfo, TRUE);
      }
    }
  }
  
  return TRUE;
}

/***
 * DESCRIPTION:
 * Retrieves the nearest item, given a position and a direction.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] POINT : start position
 * [I] UINT : direction
 *
 * RETURN:
 * Item index if successdful, -1 otherwise.
 */
static INT LISTVIEW_GetNearestItem(HWND hwnd, POINT pt, UINT vkDirection)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LVHITTESTINFO lvHitTestInfo;
  INT nItem = -1;
  RECT rcView;

  if (LISTVIEW_GetViewRect(hwnd, &rcView) != FALSE)
  {
    ZeroMemory(&lvHitTestInfo, sizeof(LVHITTESTINFO));
    LISTVIEW_GetOrigin(hwnd, &lvHitTestInfo.pt);
    lvHitTestInfo.pt.x += pt.x;
    lvHitTestInfo.pt.y += pt.y;
    
    do
    { 
      if (vkDirection == VK_DOWN)
      {
        lvHitTestInfo.pt.y += infoPtr->nItemHeight;
      }      
      else if (vkDirection == VK_UP)
      {
        lvHitTestInfo.pt.y -= infoPtr->nItemHeight;
      }
      else if (vkDirection == VK_LEFT)
      {
        lvHitTestInfo.pt.x -= infoPtr->nItemWidth;
      }
      else if (vkDirection == VK_RIGHT)
      {
        lvHitTestInfo.pt.x += infoPtr->nItemWidth;
      }

      if (PtInRect(&rcView, lvHitTestInfo.pt) == FALSE)
      {
        return -1;
      }
      else
      {
        nItem = LISTVIEW_HitTestItem(hwnd, &lvHitTestInfo);
      }

    }
    while (nItem == -1);
  }

  return nItem;
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
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  POINT ptItem;
  CHAR szDispText[DISP_TEXT_SIZE];
  LVITEMA lvItem;
  BOOL bWrap = FALSE;
  INT nItem = nStart;
  INT nLast = GETITEMCOUNT(infoPtr);

  if ((nItem >= -1) && (lpFindInfo != NULL))
  {
    ZeroMemory(&lvItem, sizeof(LVITEMA));

    if (lpFindInfo->flags & LVFI_PARAM)
    {
      lvItem.mask |= LVIF_PARAM;
    }
    
    if (lpFindInfo->flags & LVFI_STRING)
    {
      lvItem.mask |= LVIF_TEXT;
      lvItem.pszText = szDispText;
      lvItem.cchTextMax = DISP_TEXT_SIZE;
    }

    if (lpFindInfo->flags & LVFI_PARTIAL)
    {
      lvItem.mask |= LVIF_TEXT;
      lvItem.pszText = szDispText;
      lvItem.cchTextMax = DISP_TEXT_SIZE;
    }

    if (lpFindInfo->flags & LVFI_WRAP)
    {
      bWrap = TRUE;
    }

    if (lpFindInfo->flags & LVFI_NEARESTXY)
    {
      ptItem.x = lpFindInfo->pt.x;
      ptItem.y = lpFindInfo->pt.y;
    }

    while (1)
    {
      while (nItem < nLast)
      {
        if (lpFindInfo->flags & LVFI_NEARESTXY)
        {
          nItem = LISTVIEW_GetNearestItem(hwnd, ptItem, 
                                          lpFindInfo->vkDirection);
          if (nItem != -1)
          {
            /* get position of the new item index */
            if (ListView_GetItemPosition(hwnd, nItem, &ptItem) == FALSE)
              return -1;
          }
          else
            return -1;
        }
        else
        {
          nItem++;
        }
        
        lvItem.iItem = nItem;
        lvItem.iSubItem = 0;
        if (ListView_GetItemA(hwnd, &lvItem) != FALSE)
        {
          if (lvItem.mask & LVIF_TEXT)
          {
            if (lpFindInfo->flags & LVFI_PARTIAL)
            {
              if (strstr(lvItem.pszText, lpFindInfo->psz) == NULL)
                continue;
            }
            else
            {
              if (strcmp(lvItem.pszText, lpFindInfo->psz) != 0)
                continue;
            }
          }
          
          if (lvItem.mask & LVIF_PARAM)
          {
            if (lpFindInfo->lParam != lvItem.lParam)
              continue;
          }
          
          return nItem;
        }
      }

      if (bWrap != FALSE)
      {
        nItem = -1;
        nLast = nStart + 1;
        bWrap = FALSE;
      }
      else
      {
        return -1;
      }       
    }
  }

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
static LRESULT LISTVIEW_GetColumnA(HWND hwnd, INT nItem, LPLVCOLUMNA lpColumn)
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

	if (hdi.fmt & HDF_BITMAP_ON_RIGHT)
	{
	  lpColumn->fmt |= LVCFMT_BITMAP_ON_RIGHT;
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
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  INT nColumnWidth = 0;
  HDITEMA hdi;

  if (uView == LVS_LIST)
  {
    nColumnWidth = infoPtr->nItemWidth;
  }
  else if (uView == LVS_REPORT)
  {
    /* get column width from header */
    ZeroMemory(&hdi, sizeof(HDITEMA));
    hdi.mask = HDI_WIDTH;
    if (Header_GetItemA(infoPtr->hwndHeader, nColumn, &hdi) != FALSE)
    {
      nColumnWidth = hdi.cxy;
    }
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
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  INT nItemCount = 0;

  if (uView == LVS_LIST)
  {
    if (infoPtr->rcList.right > infoPtr->nItemWidth)
    {
      nItemCount = LISTVIEW_GetCountPerRow(hwnd) * 
                   LISTVIEW_GetCountPerColumn(hwnd);
    }
  }
  else if (uView == LVS_REPORT)
  {
    nItemCount = LISTVIEW_GetCountPerColumn(hwnd);
  }
  else
  {
    nItemCount = GETITEMCOUNT(infoPtr);
  }

  return nItemCount;
}

/* LISTVIEW_GetEditControl */

/***
 * DESCRIPTION:
 * Retrieves the extended listview style.
 *
 * PARAMETERS:
 * [I] HWND  : window handle
 *
 * RETURN:
 *   SUCCESS : previous style
 *   FAILURE : 0
 */
static LRESULT LISTVIEW_GetExtendedListViewStyle(HWND hwnd)
{
    LISTVIEW_INFO *infoPtr;

    /* make sure we can get the listview info */
    if (!(infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0)))
	return (0);

    return (infoPtr->dwExStyle);
}

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
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  BOOL bResult = FALSE;
  NMLVDISPINFOA dispInfo;
  LISTVIEW_SUBITEM *lpSubItem;
  LISTVIEW_ITEM *lpItem;
  HDPA hdpaSubItems;

  TRACE("(hwnd=%x, lpLVItem=%p)\n", hwnd, lpLVItem);

  if (lpLVItem != NULL)
  {
    if ((lpLVItem->iItem >= 0) && (lpLVItem->iItem < GETITEMCOUNT(infoPtr)))
    {
      ZeroMemory(&dispInfo, sizeof(NMLVDISPINFOA));
      hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
      if (hdpaSubItems != NULL)
      {
        lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
        if (lpItem != NULL)
        {
          bResult = TRUE;
          if (lpLVItem->iSubItem == 0)
          {
            if ((lpItem->iImage == I_IMAGECALLBACK) &&
                (lpLVItem->mask & LVIF_IMAGE))
            {
              dispInfo.item.mask |= LVIF_IMAGE;
            }
            
            if ((lpItem->pszText == LPSTR_TEXTCALLBACKA) && 
                (lpLVItem->mask & LVIF_TEXT))
            {
              dispInfo.item.mask |= LVIF_TEXT;
              ZeroMemory(lpLVItem->pszText, sizeof(CHAR)*lpLVItem->cchTextMax);
              dispInfo.item.pszText = lpLVItem->pszText;
              dispInfo.item.cchTextMax = lpLVItem->cchTextMax;
            }
            
            if ((infoPtr->uCallbackMask != 0) && (lpLVItem->mask & LVIF_STATE))
            {        
              dispInfo.item.mask |= LVIF_STATE;
              dispInfo.item.stateMask = infoPtr->uCallbackMask; 
            }
            
            if (dispInfo.item.mask != 0)
            {
              dispInfo.hdr.hwndFrom = hwnd;
              dispInfo.hdr.idFrom = lCtrlId;
              dispInfo.hdr.code = LVN_GETDISPINFOA;
              dispInfo.item.iItem = lpLVItem->iItem;
              dispInfo.item.iSubItem = 0;
              dispInfo.item.lParam = lpItem->lParam;
              ListView_Notify(GetParent(hwnd), lCtrlId, &dispInfo);
            }
            
            if (dispInfo.item.mask & LVIF_IMAGE)
            {
              lpLVItem->iImage = dispInfo.item.iImage;
            }
            else if (lpLVItem->mask & LVIF_IMAGE)
            {
              lpLVItem->iImage = lpItem->iImage;
            }
            
            if (dispInfo.item.mask & LVIF_TEXT)
            {
              if (dispInfo.item.mask & LVIF_DI_SETITEM)
              {
                Str_SetPtrA(&lpItem->pszText, dispInfo.item.pszText);
              }
              lpLVItem->pszText = dispInfo.item.pszText;
            }
            else if (lpLVItem->mask & LVIF_TEXT)
            {
              lpLVItem->pszText = lpItem->pszText;
            }
            
            if (dispInfo.item.mask & LVIF_STATE)
            {
              lpLVItem->state = lpItem->state;
              lpLVItem->state &= ~dispInfo.item.stateMask;
              lpLVItem->state |= (dispInfo.item.state & 
                                  dispInfo.item.stateMask);
            }
            else if (lpLVItem->mask & LVIF_STATE)
            {
              lpLVItem->state = lpItem->state & lpLVItem->stateMask;
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
          else
          {
            lpSubItem = LISTVIEW_GetSubItemPtr(hdpaSubItems, 
                                               lpLVItem->iSubItem);
            if (lpSubItem != NULL)
            {
              if ((lpSubItem->iImage == I_IMAGECALLBACK) &&
                  (lpLVItem->mask & LVIF_IMAGE))
              {
                dispInfo.item.mask |= LVIF_IMAGE;
              }
              
              if ((lpSubItem->pszText == LPSTR_TEXTCALLBACKA) && 
                  (lpLVItem->mask & LVIF_TEXT))
              {
                dispInfo.item.mask |= LVIF_TEXT;
                ZeroMemory(lpLVItem->pszText, 
                           sizeof(CHAR)*lpLVItem->cchTextMax);
                dispInfo.item.pszText = lpLVItem->pszText;
                dispInfo.item.cchTextMax = lpLVItem->cchTextMax;
              }
            }
            else
            {
              if (lpLVItem->mask & LVIF_IMAGE)
              {
                dispInfo.item.mask |= LVIF_IMAGE;
              }
          
              if (lpLVItem->mask & LVIF_TEXT)
              {
                dispInfo.item.mask |= LVIF_TEXT;
                ZeroMemory(lpLVItem->pszText, 
                           sizeof(CHAR)*lpLVItem->cchTextMax);
                dispInfo.item.pszText = lpLVItem->pszText;
                dispInfo.item.cchTextMax = lpLVItem->cchTextMax;
              }
            }

            if (dispInfo.item.mask != 0)
            {
              dispInfo.hdr.hwndFrom = hwnd;
              dispInfo.hdr.idFrom = lCtrlId;
              dispInfo.hdr.code = LVN_GETDISPINFOA;
              dispInfo.item.iItem = lpLVItem->iItem;
              dispInfo.item.iSubItem = lpLVItem->iSubItem;
              dispInfo.item.lParam = lpItem->lParam;
              ListView_Notify(GetParent(hwnd), lCtrlId, &dispInfo);
            }

            if (dispInfo.item.mask & LVIF_IMAGE)
            {
              lpLVItem->iImage = dispInfo.item.iImage;
            }
            else if (lpLVItem->mask & LVIF_IMAGE)
            {
              lpLVItem->iImage = lpItem->iImage;
            }

            if (dispInfo.item.mask & LVIF_TEXT)
            {
              if (dispInfo.item.mask & LVIF_DI_SETITEM)
              {
                if (lpSubItem)
                  Str_SetPtrA(&lpSubItem->pszText, dispInfo.item.pszText);
              }
              lpLVItem->pszText = dispInfo.item.pszText;
            }
            else if (lpLVItem->mask & LVIF_TEXT)
            {
              lpLVItem->pszText = lpSubItem->pszText;
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

/***
 * DESCRIPTION:
 * Retrieves the index of the hot item.
 *
 * PARAMETERS:
 * [I] HWND  : window handle
 *
 * RETURN:
 *   SUCCESS : hot item index
 *   FAILURE : -1 (no hot item)
 */
static LRESULT LISTVIEW_GetHotItem(HWND hwnd)
{
    LISTVIEW_INFO *infoPtr;

    /* make sure we can get the listview info */
    if (!(infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0)))
	return (-1);

    return (infoPtr->nHotItem);
}

/* LISTVIEW_GetHoverTime */

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
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  BOOL bResult = FALSE;
  HDPA hdpaSubItems;
  LISTVIEW_ITEM *lpItem;
  INT nCountPerColumn;
  INT nRow;

  TRACE("(hwnd=%x,nItem=%d,lpptPosition=%p)\n", hwnd, nItem, 
        lpptPosition);

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)) && 
      (lpptPosition != NULL))
  {
    if (uView == LVS_LIST)
    {
      bResult = TRUE;
      nItem = nItem - ListView_GetTopIndex(hwnd);
      nCountPerColumn = LISTVIEW_GetCountPerColumn(hwnd);
      if (nItem < 0)
      {
        nRow = nItem % nCountPerColumn;
        if (nRow == 0)
        {
          lpptPosition->x = nItem / nCountPerColumn * infoPtr->nItemWidth;
          lpptPosition->y = 0;
        }
        else
        {
          lpptPosition->x = (nItem / nCountPerColumn -1) * infoPtr->nItemWidth;
          lpptPosition->y = (nRow + nCountPerColumn) * infoPtr->nItemHeight;  
        }
      }
      else
      {
        lpptPosition->x = nItem / nCountPerColumn * infoPtr->nItemWidth;
        lpptPosition->y = nItem % nCountPerColumn * infoPtr->nItemHeight;
      }
    }
    else if (uView == LVS_REPORT)
    {
      bResult = TRUE;
      lpptPosition->x = REPORT_MARGINX;
      lpptPosition->y = ((nItem - ListView_GetTopIndex(hwnd)) * 
                         infoPtr->nItemHeight) + infoPtr->rcList.top;
    }
    else
    {
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
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  BOOL bResult = FALSE;
  POINT ptOrigin;
  POINT ptItem;
  HDC hdc;
  HFONT hOldFont;
  INT nMaxWidth;
  INT nLabelWidth;
  TEXTMETRICA tm;

  TRACE("(hwnd=%x, nItem=%d, lprc=%p)\n", hwnd, nItem, lprc);
  
  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)) && (lprc != NULL))
  {
    if (ListView_GetItemPosition(hwnd, nItem, &ptItem) != FALSE)
    {
      switch(lprc->left)  
      {  
      case LVIR_ICON: 
        if (uView == LVS_ICON)
        {
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
        }
        else if (uView == LVS_SMALLICON)
        {
          if (LISTVIEW_GetOrigin(hwnd, &ptOrigin) != FALSE)
          {
            bResult = TRUE;
            lprc->left = ptItem.x + ptOrigin.x;
            lprc->top = ptItem.y + ptOrigin.y;
            lprc->bottom = lprc->top + infoPtr->nItemHeight;

            if (infoPtr->himlState != NULL)
              lprc->left += infoPtr->iconSize.cx;
              
            if (infoPtr->himlSmall != NULL)
              lprc->right = lprc->left + infoPtr->iconSize.cx;
            else
              lprc->right = lprc->left;
          }
        }
        else 
        {
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
        }
        break;

      case LVIR_LABEL: 
        if (uView == LVS_ICON)
        {
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
        }
        else if (uView == LVS_SMALLICON)
        {
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
        }
        else
        {
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
        }
        break;

      case LVIR_BOUNDS:
        if (uView == LVS_ICON)
        {
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
        }
        else if (uView == LVS_SMALLICON)
        {
          if (LISTVIEW_GetOrigin(hwnd, &ptOrigin) != FALSE)
          {
            bResult = TRUE;
            lprc->left = ptItem.x + ptOrigin.x; 
            lprc->right = lprc->left;
            lprc->top = ptItem.y + ptOrigin.y;
            lprc->bottom = lprc->top + infoPtr->nItemHeight;
            if (infoPtr->himlState != NULL)
              lprc->right += infoPtr->iconSize.cx;
            if (infoPtr->himlSmall != NULL)
              lprc->right += infoPtr->iconSize.cx;
            lprc->right += LISTVIEW_GetLabelWidth(hwnd, nItem);
          }
        }
        else
        {
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
        }
        break;
        
      case LVIR_SELECTBOUNDS:
        if (uView == LVS_ICON)
        {
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
        }
        else if (uView == LVS_SMALLICON)
        {
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
        }
        else
        {
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
  CHAR szDispText[DISP_TEXT_SIZE];
  INT nLabelWidth = 0;
  LVITEMA lvItem;

  TRACE("(hwnd=%x, nItem=%d)\n", hwnd, nItem);

  ZeroMemory(&lvItem, sizeof(LVITEMA));
  lvItem.mask = LVIF_TEXT;
  lvItem.iItem = nItem;
  lvItem.cchTextMax = DISP_TEXT_SIZE;
  lvItem.pszText = szDispText;
  if (ListView_GetItemA(hwnd, &lvItem) != FALSE)
  {
    nLabelWidth = ListView_GetStringWidthA(hwnd, lvItem.pszText); 
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
 *   SUCCESS : string length
 *   FAILURE : 0
 */
static LRESULT LISTVIEW_GetItemTextA(HWND hwnd, INT nItem, LPLVITEMA lpLVItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nLength = 0;
  
  if (lpLVItem != NULL)
  {
    if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
    {
      lpLVItem->mask = LVIF_TEXT;
      lpLVItem->iItem = nItem;
      if (ListView_GetItemA(hwnd, lpLVItem) != FALSE)
      {
        nLength = lstrlenA(lpLVItem->pszText);
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
 * [I] INT : relationship flag
 * 
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_GetNextItem(HWND hwnd, INT nItem, UINT uFlags)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  UINT uMask = 0;
  LVFINDINFO lvFindInfo;
  INT nCountPerColumn;
  INT i;
  
  if ((nItem >= -1) && (nItem < GETITEMCOUNT(infoPtr)))
  { 
    ZeroMemory(&lvFindInfo, sizeof(LVFINDINFO));

    if (uFlags & LVNI_CUT)
      uMask |= LVIS_CUT;
    
    if (uFlags & LVNI_DROPHILITED)
      uMask |= LVIS_DROPHILITED;
          
    if (uFlags & LVNI_FOCUSED)
      uMask |= LVIS_FOCUSED;

    if (uFlags & LVNI_SELECTED)
      uMask |= LVIS_SELECTED;

    if (uFlags & LVNI_ABOVE)
    {
      if ((uView == LVS_LIST) || (uView == LVS_REPORT))
      {
        while (nItem >= 0)
        {
          nItem--;
          if ((ListView_GetItemState(hwnd, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
      else
      {
        lvFindInfo.flags = LVFI_NEARESTXY;
        lvFindInfo.vkDirection = VK_UP;
        ListView_GetItemPosition(hwnd, nItem, &lvFindInfo.pt);
        while ((nItem = ListView_FindItem(hwnd, nItem, &lvFindInfo)) != -1)
        {
          if ((ListView_GetItemState(hwnd, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else if (uFlags & LVNI_BELOW)
    {
      if ((uView == LVS_LIST) || (uView == LVS_REPORT))
      {
        while (nItem < GETITEMCOUNT(infoPtr))
        {
          nItem++;
          if ((ListView_GetItemState(hwnd, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
      else
      {
        lvFindInfo.flags = LVFI_NEARESTXY;
        lvFindInfo.vkDirection = VK_DOWN;
        ListView_GetItemPosition(hwnd, nItem, &lvFindInfo.pt);
        while ((nItem = ListView_FindItem(hwnd, nItem, &lvFindInfo)) != -1)
        {
          if ((ListView_GetItemState(hwnd, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else if (uFlags & LVNI_TOLEFT)
    {
      if (uView == LVS_LIST)
      {
        nCountPerColumn = LISTVIEW_GetCountPerColumn(hwnd);
        while (nItem - nCountPerColumn >= 0)
        {
          nItem -= nCountPerColumn;
          if ((ListView_GetItemState(hwnd, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
      else if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
      {
        lvFindInfo.flags = LVFI_NEARESTXY;
        lvFindInfo.vkDirection = VK_LEFT;
        ListView_GetItemPosition(hwnd, nItem, &lvFindInfo.pt);
        while ((nItem = ListView_FindItem(hwnd, nItem, &lvFindInfo)) != -1)
        {
          if ((ListView_GetItemState(hwnd, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else if (uFlags & LVNI_TORIGHT)
    {
      if (uView == LVS_LIST)
      {
        nCountPerColumn = LISTVIEW_GetCountPerColumn(hwnd);
        while (nItem + nCountPerColumn < GETITEMCOUNT(infoPtr))
        {
          nItem += nCountPerColumn;
          if ((ListView_GetItemState(hwnd, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
      else if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
      {
        lvFindInfo.flags = LVFI_NEARESTXY;
        lvFindInfo.vkDirection = VK_RIGHT;
        ListView_GetItemPosition(hwnd, nItem, &lvFindInfo.pt);
        while ((nItem = ListView_FindItem(hwnd, nItem, &lvFindInfo)) != -1)
        {
          if ((ListView_GetItemState(hwnd, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else
    {
      nItem++;

      /* search by index */
      for (i = nItem; i < GETITEMCOUNT(infoPtr); i++)
      {
        if ((ListView_GetItemState(hwnd, i, uMask) & uMask) == uMask)
          return i;
      }
    }
  }

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
  UINT uView = lStyle & LVS_TYPEMASK;
  INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
  INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
  BOOL bResult = FALSE;
  
  TRACE("(hwnd=%x, lpptOrigin=%p)\n", hwnd, lpptOrigin);

  if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
  {
    SCROLLINFO scrollInfo;
    ZeroMemory(lpptOrigin, sizeof(POINT));
    ZeroMemory(&scrollInfo, sizeof(SCROLLINFO));
    scrollInfo.cbSize = sizeof(SCROLLINFO);

    if (lStyle & WS_HSCROLL)
    {
      scrollInfo.fMask = SIF_POS;
      if (GetScrollInfo(hwnd, SB_HORZ, &scrollInfo) != FALSE) 
      {
        lpptOrigin->x = -scrollInfo.nPos * max(nListWidth / 10, 1);
      }
    }

    if (lStyle & WS_VSCROLL)
    {
      scrollInfo.fMask = SIF_POS;
      if (GetScrollInfo(hwnd, SB_VERT, &scrollInfo) != FALSE)
      {
        lpptOrigin->y = -scrollInfo.nPos * max(nListHeight / 10, 1);
      }
    }
      
    bResult = TRUE;
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
  
  TRACE("(hwnd=%x, x=%ld, y=%ld)\n", hwnd, lpHitTestInfo->pt.x,
        lpHitTestInfo->pt.y);

  for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
  {
    rcItem.left = LVIR_BOUNDS;
    if (LISTVIEW_GetItemRect(hwnd, i, &rcItem) != FALSE)
    {
      if (PtInRect(&rcItem, lpHitTestInfo->pt) != FALSE)
      {
        rcItem.left = LVIR_ICON;
        if (LISTVIEW_GetItemRect(hwnd, i, &rcItem) != FALSE)
        {
          if (PtInRect(&rcItem, lpHitTestInfo->pt) != FALSE)
          {
            lpHitTestInfo->flags = LVHT_ONITEMICON;
            lpHitTestInfo->iItem = i;
            lpHitTestInfo->iSubItem = 0;
            return i;
          }
        }
      
        rcItem.left = LVIR_LABEL;
        if (LISTVIEW_GetItemRect(hwnd, i, &rcItem) != FALSE)
        {
          if (PtInRect(&rcItem, lpHitTestInfo->pt) != FALSE)
          {
            lpHitTestInfo->flags = LVHT_ONITEMLABEL;
            lpHitTestInfo->iItem = i;
            lpHitTestInfo->iSubItem = 0;
            return i;
          }
        }
        
        lpHitTestInfo->flags = LVHT_ONITEMSTATEICON;
        lpHitTestInfo->iItem = i;
        lpHitTestInfo->iSubItem = 0;
        return i;
      }
    }
  }
     
  lpHitTestInfo->flags = LVHT_NOWHERE;

  return -1;
}

/***
 * DESCRIPTION:
 * Determines which listview item is located at the specified position.
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
  HDITEMA hdi;
  INT nNewColumn = -1;

  TRACE("(hwnd=%x, nColumn=%d, lpColumn=%p)\n",hwnd, nColumn, 
        lpColumn);

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

    LISTVIEW_UpdateScroll(hwnd);
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
  UINT uView = lStyle & LVS_TYPEMASK;
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMLISTVIEW nmlv;
  INT nItem = -1;
  HDPA hdpaSubItems;
  LISTVIEW_ITEM *lpItem = NULL;

  TRACE("(hwnd=%x,lpLVItem=%p)\n", hwnd, lpLVItem);

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
                if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
                {
                  if (lStyle & LVS_ALIGNLEFT)
                  {
                    LISTVIEW_AlignLeft(hwnd);
                  }
                  else
                  {
                    LISTVIEW_AlignTop(hwnd);
                  }
                }
                
                LISTVIEW_UpdateScroll(hwnd);
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

/* LISTVIEW_SetBkImage */

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
  HDITEMA hdi, hdiget;

  if ((lpColumn != NULL) && (nColumn >= 0) && 
      (nColumn < Header_GetItemCount(infoPtr->hwndHeader)))
  {
    /* initialize memory */
    ZeroMemory(&hdi, sizeof(HDITEMA));

    if (lpColumn->mask & LVCF_FMT) 
    {
      /* format member is valid */
      hdi.mask |= HDI_FORMAT;

      /* get current format first */
      hdiget.mask = HDI_FORMAT;
      if (Header_GetItemA(infoPtr->hwndHeader, nColumn, &hdiget))
	      /* preserve HDF_STRING if present */
	      hdi.fmt = hdiget.fmt & HDF_STRING;

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

/* LISTVIEW_SetColumnW */
/* LISTVIEW_SetColumnOrderArray */

/***
 * DESCRIPTION:
 * Sets the width of a column
 *
 * PARAMETERS:
 * [I] HWND : window handle
 * [I] INT : column index
 * [I] INT : column width
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetColumnWidth(HWND hwnd, INT iCol, INT cx)
{
    LISTVIEW_INFO *infoPtr;
    HDITEMA hdi;
    LRESULT lret;
    LONG lStyle;

    /* set column width only if in report mode */
    lStyle = GetWindowLongA(hwnd, GWL_STYLE);
    if ((lStyle & LVS_TYPEMASK) != LVS_REPORT)
	return (FALSE);

    /* make sure we can get the listview info */
    if (!(infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0)))
	return (FALSE);  
    if (!infoPtr->hwndHeader) /* make sure we have a header */
	return (FALSE);	
	
    /* FIXME: currently ignoring LVSCW_AUTOSIZE (-1) and
	  * LVSCV_AUTOSIZE_USEHEADER (-2)
	  */
    if (cx < 0) 
	return (FALSE);
	
    hdi.mask = HDI_WIDTH;
    hdi.cxy = cx;

    /* call header to update the column change */
    lret = Header_SetItemA(infoPtr->hwndHeader, (WPARAM)iCol, (LPARAM)&hdi);

    infoPtr->nItemWidth = LISTVIEW_GetItemWidth(hwnd);

    InvalidateRect(hwnd, NULL, TRUE); /* force redraw of the listview */

    return lret;
}

/***
 * DESCRIPTION:
 * Sets the extended listview style.
 *
 * PARAMETERS:
 * [I] HWND  : window handle
 * [I] DWORD : mask
 * [I] DWORD : style
 *
 * RETURN:
 *   SUCCESS : previous style
 *   FAILURE : 0
 */
static LRESULT LISTVIEW_SetExtendedListViewStyle(HWND hwnd, DWORD dwMask, DWORD dwStyle)
{
    LISTVIEW_INFO *infoPtr;
    DWORD dwOldStyle;

    /* make sure we can get the listview info */
    if (!(infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0)))
	return (0);

    /* store previous style */
    dwOldStyle = infoPtr->dwExStyle;

    /* set new style */
    infoPtr->dwExStyle = (dwOldStyle & ~dwMask) | (dwStyle & dwMask);

    return (dwOldStyle);
}

/* LISTVIEW_SetHotCursor */

/***
 * DESCRIPTION:
 * Sets the hot item index.
 *
 * PARAMETERS:
 * [I] HWND  : window handle
 * [I] INT   : index
 *
 * RETURN:
 *   SUCCESS : previous hot item index
 *   FAILURE : -1 (no hot item)
 */
static LRESULT LISTVIEW_SetHotItem(HWND hwnd, INT iIndex)
{
    LISTVIEW_INFO *infoPtr;
    INT iOldIndex;

    /* make sure we can get the listview info */
    if (!(infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0)))
	return (-1);

    /* store previous index */
    iOldIndex = infoPtr->nHotItem;

    /* set new style */
    infoPtr->nHotItem = iIndex;

    return (iOldIndex);
}

/* LISTVIEW_SetIconSpacing */

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
    ImageList_SetBkColor(infoPtr->himlState, CLR_NONE);
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
 * [I] DWORD : update flags
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItemCount(HWND hwnd, INT nItems, DWORD dwFlags)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLongA(hwnd, 0);

  FIXME("(%d %08lx)empty stub!\n", nItems, dwFlags);

  if (nItems == 0)
    return LISTVIEW_DeleteAllItems (hwnd);

  if (nItems > GETITEMCOUNT(infoPtr))
{
    /* append items */
    FIXME("append items\n");

  }
  else if (nItems < GETITEMCOUNT(infoPtr))
  {
    /* remove items */
    FIXME("remove items\n");

  }

  return TRUE;
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
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  LISTVIEW_ITEM *lpItem;
  HDPA hdpaSubItems;
  BOOL bResult = FALSE;

  TRACE("(hwnd=%x,nItem=%d,X=%d,Y=%d)\n", hwnd, nItem, nPosX, nPosY);

  if ((nItem >= 0) || (nItem < GETITEMCOUNT(infoPtr)))
  {
    if ((uView == LVS_ICON) || (uView == LVS_SMALLICON))
    {
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
    }
  }

  return bResult;
}

/* LISTVIEW_SetItemPosition32 */

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
    lvItem.stateMask = lpLVItem->stateMask ;
    
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

/* LISTVIEW_SetItemTextW */

/***
 * DESCRIPTION:
 * Set item index that marks the start of a multiple selection.
 *
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT  : index
 *
 * RETURN:
 * Index number or -1 if there is no selection mark.
 */
static LRESULT LISTVIEW_SetSelectionMark(HWND hwnd, INT nIndex)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nOldIndex = infoPtr->nSelectionMark;

  infoPtr->nSelectionMark = nIndex;

  return nOldIndex;
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

/* LISTVIEW_SetToolTips */
/* LISTVIEW_SetUnicodeFormat */
/* LISTVIEW_SetWorkAreas */

/***
 * DESCRIPTION:
 * Callback internally used by LISTVIEW_SortItems()
 * 
 * PARAMETER(S):
 * [I] LPVOID : first LISTVIEW_ITEM to compare
 * [I] LPVOID : second LISTVIEW_ITEM to compare
 * [I] LPARAM : HWND of control
 *
 * RETURN:
 *   if first comes before second : negative
 *   if first comes after second : positive
 *   if first and second are equivalent : zero
 */
static INT WINAPI LISTVIEW_CallBackCompare( 
  LPVOID first, 
  LPVOID second, 
  LPARAM lParam)
{
  /* Forward the call to the client defined callback */
  HWND hwnd = (HWND)lParam;
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

  return (infoPtr->pfnCompare)(
    ((LISTVIEW_ITEM *)first)->lParam,
    ((LISTVIEW_ITEM *)second)->lParam,
    infoPtr->lParamSort);
}

/***
 * DESCRIPTION:
 * Sorts the listview items.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] WPARAM : application-defined value
 * [I] LPARAM : pointer to comparision callback
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SortItems(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
    HDPA hdpaSubItems;
    LISTVIEW_ITEM *lpItem;
    int nCount, i;
    HDPA sortList;
    
    if (!infoPtr || !infoPtr->hdpaItems)
	return FALSE;
    
    nCount = GETITEMCOUNT(infoPtr);
    /* if there are 0 or 1 items, there is no need to sort */
    if (nCount > 1)
    {
	sortList = DPA_Create(nCount);

	infoPtr->pfnCompare = (PFNLVCOMPARE)lParam;
	infoPtr->lParamSort = (LPARAM)wParam;
	
	/* append pointers one by one to sortList */
	for (i = 0; i < nCount; i++)
	{
	    if ((hdpaSubItems = (HDPA) DPA_GetPtr(infoPtr->hdpaItems, i)))
		if ((lpItem = (LISTVIEW_ITEM *) DPA_GetPtr(hdpaSubItems, 0)))
		    DPA_InsertPtr(sortList, nCount + 1, lpItem);
	}

	/* sort the sortList */
	DPA_Sort(sortList, LISTVIEW_CallBackCompare, hwnd);

	/* copy the pointers back */
	for (i = 0; i < nCount; i++)
	{
	    if ((hdpaSubItems = (HDPA) DPA_GetPtr(infoPtr->hdpaItems, i)) &&
		(lpItem = (LISTVIEW_ITEM *) DPA_GetPtr(sortList, i)))
		DPA_SetPtr(hdpaSubItems, 0, lpItem);
	}

	DPA_Destroy(sortList);
    }

    return TRUE;
}

/* LISTVIEW_SubItemHitTest */

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
      InvalidateRect(hwnd, &rc, TRUE);
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
  UINT uView = lpcs->style & LVS_TYPEMASK;
  LOGFONTA logFont;

  /* initialize info pointer */
  ZeroMemory(infoPtr, sizeof(LISTVIEW_INFO));

  /* determine the type of structures to use */
  infoPtr->notifyFormat = SendMessageA(GetParent(hwnd), WM_NOTIFYFORMAT, 
                                       (WPARAM)hwnd, (LPARAM)NF_QUERY);
  if (infoPtr->notifyFormat != NFR_ANSI)
  {
    FIXME("ANSI notify format is NOT used\n");
  }
  
  /* initialize color information  */
  infoPtr->clrBk = GetSysColor(COLOR_WINDOW);
  infoPtr->clrText = GetSysColor(COLOR_WINDOWTEXT);
  infoPtr->clrTextBk = GetSysColor(COLOR_WINDOW); 

  /* set default values */
  infoPtr->uCallbackMask = 0;
  infoPtr->nFocusedItem = -1;
  infoPtr->nSelectionMark = -1;
  infoPtr->nHotItem = -1;
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
  
  if (uView == LVS_ICON)
  {
    infoPtr->iconSize.cx = GetSystemMetrics(SM_CXICON);
    infoPtr->iconSize.cy = GetSystemMetrics(SM_CYICON);
  }
  else if (uView == LVS_REPORT)
  {
    if (!(LVS_NOCOLUMNHEADER & lpcs->style))
    {
      ShowWindow(infoPtr->hwndHeader, SW_SHOWNORMAL);
    }

    infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
    infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
  }
  else
  {
    infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
    infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
  }

  /* display unsupported listview window styles */
  LISTVIEW_UnsupportedStyles(lpcs->style);

  /* allocate memory for the data structure */
  infoPtr->hdpaItems = DPA_Create(10);

  /* initialize size of items */
  infoPtr->nItemWidth = LISTVIEW_GetItemWidth(hwnd);
  infoPtr->nItemHeight = LISTVIEW_GetItemHeight(hwnd);

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
 * [I] SHORT : current scroll position if scroll code is SB_THIMBPOSITION 
 *             or SB_THUMBTRACK.
 * [I] HWND : scrollbar control window handle
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_VScroll(HWND hwnd, INT nScrollCode, SHORT nCurrentPos,
                                HWND hScrollWnd)
{
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  SCROLLINFO scrollInfo;

  ZeroMemory(&scrollInfo, sizeof(SCROLLINFO));
  scrollInfo.cbSize = sizeof(SCROLLINFO);
  scrollInfo.fMask = /*SIF_PAGE |*/ SIF_POS | SIF_RANGE;
 
  if (GetScrollInfo(hwnd, SB_VERT, &scrollInfo) != FALSE)
  {
    INT nOldScrollPos = scrollInfo.nPos;
    switch (nScrollCode)
    {
    case SB_LINEUP:
      if (scrollInfo.nPos > scrollInfo.nMin)
      {
        scrollInfo.nPos--;
      }
    break;
    
    case SB_LINEDOWN:
      if (scrollInfo.nPos < scrollInfo.nMax)
      {
        scrollInfo.nPos++;
      }
      break;
      
    case SB_PAGEUP:
      if (scrollInfo.nPos > scrollInfo.nMin)
      {
        INT nPage = 0;

        if (uView == LVS_REPORT)
        {
          nPage = LISTVIEW_GetCountPerColumn(hwnd);
        }
        else if ((uView == LVS_ICON) || (uView == LVS_SMALLICON))
        {
          nPage = 10;
        }

        if (scrollInfo.nPos >= nPage)
        {
          scrollInfo.nPos -= nPage;
        }
        else
        {
          scrollInfo.nPos = scrollInfo.nMin;
        }
      }
      break;
      
    case SB_PAGEDOWN:
      if (scrollInfo.nPos < scrollInfo.nMax)
      {
        INT nPage = 0;

        if (uView == LVS_REPORT)
        {
          nPage = LISTVIEW_GetCountPerColumn(hwnd);
        }
        else if ((uView == LVS_ICON) || (uView == LVS_SMALLICON))
        {
          nPage = 10;
        }
        
        if (scrollInfo.nPos <= scrollInfo.nMax - nPage)
        {
          scrollInfo.nPos += nPage;
        }
        else
        {
          scrollInfo.nPos = scrollInfo.nMax;
        }
      }
      break;

    case SB_THUMBPOSITION:
      break;
    }

    if (nOldScrollPos != scrollInfo.nPos)
    {
      scrollInfo.fMask = SIF_POS;
      SetScrollInfo(hwnd, SB_VERT, &scrollInfo, TRUE);
      InvalidateRect(hwnd, NULL, TRUE);
    }
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
 * [I] SHORT : current scroll position if scroll code is SB_THIMBPOSITION 
 *             or SB_THUMBTRACK.
 * [I] HWND : scrollbar control window handle
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_HScroll(HWND hwnd, INT nScrollCode, SHORT nCurrentPos,
                                HWND hScrollWnd)
{
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;
  SCROLLINFO scrollInfo;

  ZeroMemory(&scrollInfo, sizeof(SCROLLINFO));
  scrollInfo.cbSize = sizeof(SCROLLINFO);
  scrollInfo.fMask = /*SIF_PAGE |*/ SIF_POS | SIF_RANGE;
 
  if (GetScrollInfo(hwnd, SB_HORZ, &scrollInfo) != FALSE)
  {
    INT nOldScrollPos = scrollInfo.nPos;

    switch (nScrollCode)
    {
    case SB_LINELEFT:
      if (scrollInfo.nPos > scrollInfo.nMin)
      {
        scrollInfo.nPos--;
      }
      break;
    
    case SB_LINERIGHT:
      if (scrollInfo.nPos < scrollInfo.nMax)
      {
        scrollInfo.nPos++;
      }
      break;
      
    case SB_PAGELEFT:
      if (scrollInfo.nPos > scrollInfo.nMin)
      {
        INT nPage = 0;

        if (uView == LVS_LIST)
        {
          nPage = LISTVIEW_GetCountPerRow(hwnd);
        }
        else if ((uView == LVS_ICON) || (uView == LVS_SMALLICON))
        {
          nPage = 10;
        }
        
        if (scrollInfo.nPos >= nPage)
        {
          scrollInfo.nPos -= nPage;
        }
        else
        {
          scrollInfo.nPos = scrollInfo.nMin;
        }
      }
      break;
      
    case SB_PAGERIGHT:
      if (scrollInfo.nPos < scrollInfo.nMax)
      {
        INT nPage = 0;

        if (uView == LVS_LIST)
        {
          nPage = LISTVIEW_GetCountPerRow(hwnd);
        }
        else if ((uView == LVS_ICON) || (uView == LVS_SMALLICON))
        {
          nPage = 10;
        }
        
        if (scrollInfo.nPos <= scrollInfo.nMax - nPage)
        {
          scrollInfo.nPos += nPage;
        }
        else
        {
          scrollInfo.nPos = scrollInfo.nMax;
        }
      }
      break;

    case SB_THUMBPOSITION:
      break;
    }

    if (nOldScrollPos != scrollInfo.nPos)
    {
      scrollInfo.fMask = SIF_POS;
      SetScrollInfo(hwnd, SB_HORZ, &scrollInfo, TRUE);
      InvalidateRect(hwnd, NULL, TRUE);
    }
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
  INT nCtrlId = GetWindowLongA(hwnd, GWL_ID);
  HWND hwndParent = GetParent(hwnd);
  NMLVKEYDOWN nmKeyDown; 
  NMHDR nmh;
  INT nItem = -1;
  BOOL bRedraw = FALSE;

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
      nItem = 0;
    }
    break;

  case VK_END:
    if (GETITEMCOUNT(infoPtr) > 0)
    {
      nItem = GETITEMCOUNT(infoPtr) - 1;
    }
    break;

  case VK_LEFT:
    nItem = ListView_GetNextItem(hwnd, infoPtr->nFocusedItem, LVNI_TOLEFT);
    break;

  case VK_UP:
    nItem = ListView_GetNextItem(hwnd, infoPtr->nFocusedItem, LVNI_ABOVE);
    break;
    
  case VK_RIGHT:
    nItem = ListView_GetNextItem(hwnd, infoPtr->nFocusedItem, LVNI_TORIGHT);
    break;

  case VK_DOWN:
    nItem = ListView_GetNextItem(hwnd, infoPtr->nFocusedItem, LVNI_BELOW);
    break;

  case VK_PRIOR:
    /* TO DO */
    break;

  case VK_NEXT:
    /* TO DO */
    break;
  }

  if ((nItem != -1) && (nItem != infoPtr->nFocusedItem))
  {
    bRedraw = LISTVIEW_KeySelection(hwnd, nItem);
    if (bRedraw != FALSE)
    {
      /* refresh client area */
      InvalidateRect(hwnd, NULL, TRUE);
      UpdateWindow(hwnd);
    }
  }

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

  /* NEED drawing optimization ; redraw the selected items */
  InvalidateRect(hwnd, NULL, FALSE);

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

  TRACE("(hwnd=%x,key=%hu,X=%hu,Y=%hu)\n", hwnd, wKey, wPosX, wPosY);

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
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nCtrlId = GetWindowLongA(hwnd, GWL_ID);
  static BOOL bGroupSelect = TRUE;
  POINT ptPosition;
  NMHDR nmh;
  INT nItem;

  TRACE("(hwnd=%x, key=%hu, X=%hu, Y=%hu)\n", hwnd, wKey, wPosX, 
        wPosY);

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
  
  ptPosition.x = wPosX;
  ptPosition.y = wPosY;
  nItem = LISTVIEW_MouseSelection(hwnd, ptPosition);
  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
  {
    if (lStyle & LVS_SINGLESEL)
    {
      LISTVIEW_SetSelection(hwnd, nItem);
    }
    else
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

  TRACE("(hwnd=%x,key=%hu,X=%hu,Y=%hu)\n", hwnd, wKey, wPosX, wPosY);

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

  TRACE("(hwnd=%x,wParam=%x,lParam=%lx)\n", hwnd, wParam, lParam);

  /* allocate memory for info structure */
  infoPtr = (LISTVIEW_INFO *)COMCTL32_Alloc(sizeof(LISTVIEW_INFO));
  SetWindowLongA(hwnd, 0, (LONG)infoPtr);
  if (infoPtr == NULL) 
  {
    ERR("could not allocate info memory!\n");
    return 0;
  }

  if ((LISTVIEW_INFO *)GetWindowLongA(hwnd, 0) != infoPtr) 
  {
    ERR("pointer assignment error!\n");
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

  TRACE("(hwnd=%x)\n", hwnd);

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
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(hwnd);
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
      FIXME("NO support for unicode structures");
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

   TRACE("(hwnd=%x,hdc=%x)\n", hwnd, hdc);

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

  TRACE("(hwnd=%x,key=%hu,X=%hu,Y=%hu)\n", hwnd, wKey, wPosX, wPosY);

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
  POINT ptPosition;
  NMHDR nmh;
  INT nItem;

  TRACE("(hwnd=%x,key=%hu,X=%hu,Y=%hu)\n", hwnd, wKey, wPosX, wPosY);

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
  ptPosition.x = wPosX;
  ptPosition.y = wPosY;
  nItem = LISTVIEW_MouseSelection(hwnd, ptPosition);
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

  TRACE("(hwnd=%x,key=%hu,X=%hu,Y=%hu)\n", hwnd, wKey, wPosX, wPosY);

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
  UINT uView = GetWindowLongA(hwnd, GWL_STYLE) & LVS_TYPEMASK;

  TRACE("(hwnd=%x,hfont=%x,redraw=%hu)\n", hwnd, hFont, fRedraw);

  if (hFont == 0)
  {
    infoPtr->hFont = infoPtr->hDefaultFont;
  }
  else
  {
    infoPtr->hFont = hFont;
  }

  if (uView == LVS_REPORT)
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
  UINT uView = lStyle & LVS_TYPEMASK;

  TRACE("(hwnd=%x, width=%d, height=%d)\n",hwnd, Width, Height);

  LISTVIEW_UpdateSize(hwnd);

  if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
  {
    if (lStyle & LVS_ALIGNLEFT)
    {
      LISTVIEW_AlignLeft(hwnd);
    }
    else
    {
      LISTVIEW_AlignTop(hwnd);
    }
  }

  LISTVIEW_UpdateScroll(hwnd);
  
  /* invalidate client area + erase background */
  InvalidateRect(hwnd, NULL, TRUE);

  return 0;
}

/***
 * DESCRIPTION:
 * Sets the size information.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * Zero
 */
static VOID LISTVIEW_UpdateSize(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  UINT uView = lStyle & LVS_TYPEMASK;
  RECT rcList;
  
  GetClientRect(hwnd, &rcList);
  infoPtr->rcList.left = 0;
  infoPtr->rcList.right = max(rcList.right - rcList.left, 1);
  infoPtr->rcList.top = 0;
  infoPtr->rcList.bottom = max(rcList.bottom - rcList.top, 1);
     
  if (uView == LVS_LIST)
  {
    if ((lStyle & WS_HSCROLL) == 0)
    {
      INT nHScrollHeight = GetSystemMetrics(SM_CYHSCROLL);
      if (infoPtr->rcList.bottom > nHScrollHeight)
      { 
        infoPtr->rcList.bottom -= nHScrollHeight;
      }
    }
  }
  else if (uView == LVS_REPORT)
  {
    HDLAYOUT hl;
    WINDOWPOS wp;

    hl.prc = &rcList;
    hl.pwpos = &wp;
    Header_Layout(infoPtr->hwndHeader, &hl);
    if (!(LVS_NOCOLUMNHEADER & lStyle))
    {
      infoPtr->rcList.top = max(wp.cy, 0);
    }
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
  UINT uNewView = lpss->styleNew & LVS_TYPEMASK;
  UINT uOldView = lpss->styleOld & LVS_TYPEMASK;
  RECT rcList = infoPtr->rcList;

  TRACE("(hwnd=%x, styletype=%x, stylestruct=%p)\n", 
        hwnd, wStyleType, lpss);

  if (wStyleType == GWL_STYLE)
  {
    if (uOldView == LVS_REPORT)
    {
      ShowWindow(infoPtr->hwndHeader, SW_HIDE);
    }
 
    if ((lpss->styleOld & WS_HSCROLL) != 0)
    {
       ShowScrollBar(hwnd, SB_HORZ, FALSE);
    }
 
    if ((lpss->styleOld & WS_VSCROLL) != 0)
    {
       ShowScrollBar(hwnd, SB_VERT, FALSE);
    }
 
    if (uNewView == LVS_ICON)
    {
      infoPtr->iconSize.cx = GetSystemMetrics(SM_CXICON);
      infoPtr->iconSize.cy = GetSystemMetrics(SM_CYICON);
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(hwnd);
      infoPtr->nItemHeight = LISTVIEW_GetItemHeight(hwnd);
      if (lpss->styleNew & LVS_ALIGNLEFT)
      {
        LISTVIEW_AlignLeft(hwnd);
      }
      else
      {
        LISTVIEW_AlignTop(hwnd);
      }
    }
    else if (uNewView == LVS_REPORT)
    {
      HDLAYOUT hl;
      WINDOWPOS wp;

      hl.prc = &rcList;
      hl.pwpos = &wp;
      Header_Layout(infoPtr->hwndHeader, &hl);
      SetWindowPos(infoPtr->hwndHeader, hwnd, wp.x, wp.y, wp.cx, wp.cy, 
                   wp.flags);
      if (!(LVS_NOCOLUMNHEADER & lpss->styleNew))
      {
        ShowWindow(infoPtr->hwndHeader, SW_SHOWNORMAL);
      }
      infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
      infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(hwnd);
      infoPtr->nItemHeight = LISTVIEW_GetItemHeight(hwnd);
    }
    else if (uNewView == LVS_LIST)
    {
      infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
      infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(hwnd);
      infoPtr->nItemHeight = LISTVIEW_GetItemHeight(hwnd);
    }
    else
    {
      infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
      infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(hwnd);
      infoPtr->nItemHeight = LISTVIEW_GetItemHeight(hwnd);
      if (lpss->styleNew & LVS_ALIGNLEFT)
      {
        LISTVIEW_AlignLeft(hwnd);
      }
      else
      {
        LISTVIEW_AlignTop(hwnd);
      }
    }

    /* update the size of the client area */
    LISTVIEW_UpdateSize(hwnd);

    /* add scrollbars if needed */
    LISTVIEW_UpdateScroll(hwnd);
    
    /* invalidate client area + erase background */
    InvalidateRect(hwnd, NULL, TRUE);

    /* print the list of unsupported window styles */
    LISTVIEW_UnsupportedStyles(lpss->styleNew);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Window procedure of the listview control.
 *
 */
static LRESULT WINAPI LISTVIEW_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
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

  case LVM_GETCOLUMNORDERARRAY:
    FIXME("Unimplemented msg LVM_GETCOLUMNORDERARRAY\n");
    return 0;

  case LVM_GETCOLUMNWIDTH:
    return LISTVIEW_GetColumnWidth(hwnd, (INT)wParam);

  case LVM_GETCOUNTPERPAGE:
    return LISTVIEW_GetCountPerPage(hwnd);

/*	case LVM_GETEDITCONTROL: */

  case LVM_GETEXTENDEDLISTVIEWSTYLE:
    return LISTVIEW_GetExtendedListViewStyle(hwnd);

  case LVM_GETHEADER:
    return LISTVIEW_GetHeader(hwnd);

/*	case LVM_GETHOTCURSOR: */

  case LVM_GETHOTITEM:
    return LISTVIEW_GetHotItem(hwnd);

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

  case LVM_SETCOLUMNW:
    FIXME("Unimplemented msg LVM_SETCOLUMNW\n");
    return 0;

  case LVM_SETCOLUMNORDERARRAY:
    FIXME("Unimplemented msg LVM_SETCOLUMNORDERARRAY\n");
    return 0;

  case LVM_SETCOLUMNWIDTH:
    return LISTVIEW_SetColumnWidth(hwnd, (INT)wParam, (INT)lParam);

  case LVM_SETEXTENDEDLISTVIEWSTYLE:
    return LISTVIEW_SetExtendedListViewStyle(hwnd, (DWORD)wParam, (DWORD)lParam);

/*	case LVM_SETHOTCURSOR: */

  case LVM_SETHOTITEM:
    return LISTVIEW_SetHotItem(hwnd, (INT)wParam);

/*	case LVM_SETHOVERTIME: */
/*	case LVM_SETICONSPACING: */
	
  case LVM_SETIMAGELIST:
    return LISTVIEW_SetImageList(hwnd, (INT)wParam, (HIMAGELIST)lParam);

  case LVM_SETITEMA:
    return LISTVIEW_SetItemA(hwnd, (LPLVITEMA)lParam);

/*	case LVM_SETITEMW: */

  case LVM_SETITEMCOUNT: 
    return LISTVIEW_SetItemCount(hwnd, (INT)wParam, (DWORD)lParam);
    
  case LVM_SETITEMPOSITION:
    return LISTVIEW_SetItemPosition(hwnd, (INT)wParam, (INT)LOWORD(lParam),
                                    (INT)HIWORD(lParam));

/*	case LVM_SETITEMPOSITION32: */

  case LVM_SETITEMSTATE: 
    return LISTVIEW_SetItemState(hwnd, (INT)wParam, (LPLVITEMA)lParam);

  case LVM_SETITEMTEXTA:
    return LISTVIEW_SetItemTextA(hwnd, (INT)wParam, (LPLVITEMA)lParam);

/*	case LVM_SETITEMTEXTW: */

  case LVM_SETSELECTIONMARK:
    return LISTVIEW_SetSelectionMark(hwnd, (INT)lParam);

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
      ERR("unknown msg %04x wp=%08x lp=%08lx\n", uMsg, wParam, 
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
VOID LISTVIEW_Register(void)
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
VOID LISTVIEW_Unregister(void)
{
  if (GlobalFindAtomA(WC_LISTVIEWA))
  {
    UnregisterClassA(WC_LISTVIEWA, (HINSTANCE)NULL);
  }
}

