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
 *   LISTVIEW_GetMaxItemWidth : large icon view
 *   LISTVIEW_Notify : notification from children 
 *   LISTVIEW_KeyDown : key press messages
 *   LISTVIEW_HScroll : small icon and icon
 *   LISTVIEW_VScroll : small icon and icon
 *   LISTVIEW_SortItems : empty stub 
 *   LISTVIEW_SetItemPosition : small icon and icon
 *   LISTVIEW_SetItemCount : empty stub 
 *   LISTVIEW_SetItem32W : no unicode yet!
 *   LISTVIEW_SetColumn32A : DOUBLE CHECK 
 *   LISTVIEW_Scroll : scrolling in pixels 
 *   LISTVIEW_RedrawItems : define bounding rect 
 *   LISTVIEW_InsertItem32W : no unicode yet!
 *   LISTVIEW_InsertColumn32W : no unicode yet!
 *   LISTVIEW_GetViewRect : small icon and icon 
 *   LISTVIEW_GetOrigin : small icon and icon
 *   LISTVIEW_GetNumberOfWorkAreas : small icon and icon
 *   LISTVIEW_GetNextItem : all
 *   LISTVIEW_SetScroll : scrolling in pixels 
 *   LISTVIEW_GetItemRect : all 
 *   LISTVIEW_GetHotCursor : all 
 *   LISTVIEW_GetHotItem : all 
 *   LISTVIEW_GetHoverTime : all
 *   LISTVIEW_GetISearchString : all 
 *   LISTVIEW_GetBkImage : all
 *   LISTVIEW_FindItem : all 
 *   LISTVIEW_EnsureVisible : some 
 *   LISTVIEW_EditLabel : REPORT (need to implement a timer)
 *   LISTVIEW_GetItemPosition : small icon and icon
 *   LISTVIEW_GetItemRect : some
 *   LISTVIEW_Arrange : small icon and icon
 *   LISTVIEW_ApproximateViewRect : report, small icon and icon
 *   LISTVIEW_RefreshIcon : large icon draw function
 */

#include "windows.h"
#include "commctrl.h"
#include "listview.h"
#include "win.h"
#include "debug.h"
#include "winuser.h"
#include "string.h"

/* constants */
#define DISP_TEXT_SIZE 128
#define WIDTH_PADDING 12
#define HEIGHT_PADDING 2
#define MIN_COLUMN_WIDTH 96

/* macro section */
#define GETITEMCOUNT(infoPtr) ((infoPtr)->hdpaItems->nItemCount)
#define ListView_LVNotify(hwnd,lCtrlId,plvnm) \
    (BOOL)SendMessageA((hwnd),WM_NOTIFY,(WPARAM)(INT)lCtrlId,(LPARAM)(LPNMLISTVIEW)(plvnm))
#define ListView_Notify(hwnd,lCtrlId,pnmh) \
    (BOOL)SendMessageA((hwnd),WM_NOTIFY,(WPARAM)(INT)lCtrlId,(LPARAM)(LPNMHDR)(pnmh))

/* forward declarations */
static VOID LISTVIEW_SetSize(HWND hwnd, LONG lStyle, LONG lWidth, 
                             LONG lHeight);
static VOID LISTVIEW_SetViewInfo(HWND hwnd);
static VOID LISTVIEW_SetScroll(HWND hwnd);
static VOID LISTVIEW_AddGroupSelection(HWND hwnd, INT nItem);
static VOID LISTVIEW_AddSelection(HWND hwnd, INT nItem);
static BOOL LISTVIEW_ToggleSelection(HWND hwnd, INT nItem);
static VOID LISTVIEW_SetGroupSelection(HWND hwnd, INT nItem);
static VOID LISTVIEW_SetSelection(HWND hwnd, INT nItem);
static VOID LISTVIEW_RemoveSelections(HWND hwnd, INT nFirst, INT nLast);
static VOID LISTVIEW_SetItemFocus(HWND hwnd, INT nItem);
static BOOL LISTVIEW_RemoveSubItem(HDPA hdpaSubItems, INT nSubItem);
static BOOL LISTVIEW_RemoveColumn(HDPA hdpaItems, INT nColumn);
static BOOL LISTVIEW_InitItem(HWND hwnd, LISTVIEW_ITEM *lpItem, 
                              LPLVITEMA lpLVItem);
static BOOL LISTVIEW_InitSubItem(HWND hwnd, LISTVIEW_SUBITEM *lpSubItem,
                                 LPLVITEMA lpLVItem);
static BOOL LISTVIEW_AddSubItem(HWND hwnd, LPLVITEMA lpLVItem);
static BOOL LISTVIEW_SetItem(HWND hwnd, LPLVITEMA lpLVItem);
static BOOL LISTVIEW_SetSubItem(HWND hwnd, LPLVITEMA lpLVItem);
static INT LISTVIEW_FindInsertPosition(HDPA hdpaSubItems, INT nSubItem);
static LISTVIEW_SUBITEM* LISTVIEW_GetSubItem(HDPA hdpaSubItems, INT nSubItem);


/***
 * DESCRIPTION:
 * Prints a message for unsupported window styles.
 * 
 * PARAMETER(S):
 * [I] LONG : window style
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_UnsupportedStyles(LONG lStyle)
{
  if ((LVS_TYPEMASK & lStyle) == LVS_ALIGNLEFT)
  {
    FIXME(listview, "  LVS_ALIGNLEFT\n");
  } 
     
  if ((LVS_TYPEMASK & lStyle) ==  LVS_ALIGNTOP)
  {
    FIXME(listview, "  LVS_ALIGNTOP\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_AUTOARRANGE)
  {
    FIXME(listview, "  LVS_AUTOARRANGE\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_EDITLABELS)
  {
    FIXME(listview, "  LVS_EDITLABELS\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_ICON)
  {
    FIXME(listview, "  LVS_ICON\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_NOCOLUMNHEADER)
  {
    FIXME(listview, "  LVS_SORTDESCENDING\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_NOLABELWRAP)
  {
    FIXME(listview, "  LVS_NOLABELWRAP\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_NOSCROLL)
  {
    FIXME(listview, "  LVS_NOSCROLL\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_NOSORTHEADER)
  {
    FIXME(listview, "  LVS_NOSORTHEADER\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_OWNERDRAWFIXED)
  {
    FIXME(listview, "  LVS_OWNERDRAWFIXED\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_SHAREIMAGELISTS)
  {
    FIXME(listview, "  LVS_SHAREIMAGELISTS\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_SHOWSELALWAYS)
  {
    FIXME(listview, "  LVS_SHOWSELALWAYS\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_SINGLESEL)
  {
    FIXME(listview, "  LVS_SINGLESEL\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_SMALLICON)
  {
    FIXME(listview, "  LVS_SMALLICON\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_SORTDESCENDING)
  {
    FIXME(listview, "  LVS_SORTDESCENDING\n");
  }

  if ((LVS_TYPEMASK & lStyle) == LVS_SORTDESCENDING)
  {
    FIXME(listview, "  LVS_SORTDESCENDING\n");
  }
}

/***
 * DESCRIPTION:
 * Retrieves display information.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
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
 * Retrieves display information.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] LISTVIEW_SUBITEM* : listview control subitem
 * [O] INT : image index
 * [O] UINT : state value
 * [O] CHAR** : string
 * [I] INT : size of string
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_GetSubItemDispInfo(HWND hwnd, INT nItem, LPARAM lParam,
                                        LISTVIEW_SUBITEM *lpSubItem, 
                                        INT nColumn, INT *pnDispImage, 
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
    dispInfo.item.iSubItem = nColumn;
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
 * Calculates a new column width.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 * Returns item width.
 */
static INT LISTVIEW_GetMaxItemWidth(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  LISTVIEW_ITEM *lpItem;
  HDPA hdpaSubItems;
  INT nStringWidth;
  INT nMaxItemWidth = 0;
  INT i;
  
  if ((LVS_TYPEMASK & lStyle) == LVS_ICON)
  {
    /* TO DO */
  }
  else
  {
    for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
    { 
      hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, i);
      if (hdpaSubItems != NULL)
      {
        lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
        if (lpItem != NULL)
        {
          CHAR szDispText[DISP_TEXT_SIZE];
          LPSTR pszDispText;
          pszDispText = szDispText;
          LISTVIEW_GetItemDispInfo(hwnd, i, lpItem, NULL, NULL, &pszDispText, 
                                   DISP_TEXT_SIZE);
          nStringWidth = ListView_GetStringWidthA(hwnd, pszDispText);
          nMaxItemWidth = max(nMaxItemWidth, nStringWidth);
        }
      }
    }
    
    /* add arbitrary padding for separating columns */
    nMaxItemWidth += WIDTH_PADDING;

    if (infoPtr->himlSmall != NULL)
    {
      nMaxItemWidth += infoPtr->iconSize.cx;
    }

    if (infoPtr->himlState != NULL)
    {
      nMaxItemWidth += infoPtr->iconSize.cx;
    }
  }

  nMaxItemWidth = max(MIN_COLUMN_WIDTH, nMaxItemWidth);

  return nMaxItemWidth;
}

/***
 * DESCRIPTION:
 * Sets diplay information (needed for drawing and calculations).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * 
 * RETURN:
 * None
 */
static VOID LISTVIEW_SetViewInfo(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nHeight;
  INT nWidth;
  HDC hdc;
  HFONT hOldFont;
  TEXTMETRICA tm; 

  /* get text height */
  hdc = GetDC(hwnd);
  hOldFont = SelectObject(hdc, infoPtr->hFont);
  GetTextMetricsA(hdc, &tm);

  nHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
  nWidth = infoPtr->rcList.right - infoPtr->rcList.left;

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_LIST:
    infoPtr->nColumnWidth = LISTVIEW_GetMaxItemWidth(hwnd);
    infoPtr->nItemHeight = (max(tm.tmHeight, infoPtr->iconSize.cy) + 
                            HEIGHT_PADDING);
    
    infoPtr->nCountPerColumn = nHeight / infoPtr->nItemHeight;
    if (infoPtr->nCountPerColumn == 0)
    {
      infoPtr->nCountPerColumn = 1;
    }
    
    infoPtr->nCountPerRow = nWidth / infoPtr->nColumnWidth;
    if (infoPtr->nCountPerRow == 0)
    {
      infoPtr->nCountPerRow = 1;
    }
    break;

  case LVS_REPORT:
    infoPtr->nItemHeight = (max(tm.tmHeight, infoPtr->iconSize.cy) + 
                            HEIGHT_PADDING);
    infoPtr->nCountPerRow = 1;
    infoPtr->nCountPerColumn = nHeight / infoPtr->nItemHeight;
    if (infoPtr->nCountPerColumn == 0)
    {
      infoPtr->nCountPerColumn = 1;
    }
    break;

  }

  SelectObject(hdc, hOldFont);
  ReleaseDC(hwnd, hdc);
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
 * Reinitilizes the listview items.
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
 * Reinitilizes the listview items.
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
 * Reinitilizes the listview items.
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
  BOOL bResult = FALSE;
  HDPA hdpaSubItems;
  LISTVIEW_SUBITEM *lpSubItem = NULL;
  INT nPosition, nItem;

  if (lpLVItem != NULL)
  {
    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
    if (hdpaSubItems != NULL)
    {
      lpSubItem = (LISTVIEW_SUBITEM *)COMCTL32_Alloc(sizeof(LISTVIEW_SUBITEM));
      if (lpSubItem != NULL)
      {
        if (LISTVIEW_InitSubItem(hwnd, lpSubItem, lpLVItem) == TRUE)
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
 * Sets scrollbar(s). 
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 *
 * RETURN:
 *   TRUE if scrollbars were added, modified or removed.
 */
static VOID LISTVIEW_SetVScroll(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nScrollPos;
  INT nMaxRange;

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_REPORT:
    nMaxRange = GETITEMCOUNT(infoPtr) - infoPtr->nCountPerColumn;
    SetScrollRange(hwnd, SB_VERT, 0, nMaxRange, FALSE);
    nScrollPos = ListView_GetTopIndex(hwnd);
    SetScrollPos(hwnd, SB_VERT, nScrollPos, TRUE);
    break;
    
  default:
    /* TO DO */
  }
}

static VOID LISTVIEW_SetHScroll(HWND hwnd)
{
/*   LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0); */
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
/*   INT nScrollPos; */
/*   INT nMaxRange; */

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_REPORT:
    /* TO DO */
    break;

  default:
    /* TO DO */
  }
}

static VOID LISTVIEW_SetScroll(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nHiddenItemCount;
  INT nScrollPos;
  INT nMaxRange;
  INT nCountPerPage;
  RECT rc;
  BOOL bHScroll = FALSE;
  BOOL bVScroll = FALSE;
  INT nHeaderWidth = 0;
  INT nItemCount;
  INT i;

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_LIST:
    nCountPerPage = infoPtr->nCountPerRow * infoPtr->nCountPerColumn;
    if (nCountPerPage < GETITEMCOUNT(infoPtr))
    {
      /* add scrollbar if not already present */
      if (!(lStyle & WS_HSCROLL))
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
      /* remove scrollbar if present */
      if (lStyle & WS_HSCROLL)
      {
        /* hide scrollbar */
        ShowScrollBar(hwnd, SB_HORZ, FALSE);
      }
    }
    break;
    
  case LVS_REPORT:
    nItemCount = Header_GetItemCount(infoPtr->hwndHeader);
    for (i = 0; i < nItemCount; i++)
    {
      if (Header_GetItemRect(infoPtr->hwndHeader, i, &rc) != 0)
      {
        nHeaderWidth += (rc.right - rc.left);
      }
    }

    if (nHeaderWidth > (infoPtr->rcList.right - infoPtr->rcList.left))
    {
      bHScroll = TRUE;
      
      /* add horizontal scrollbar if not present */
      if (!(lStyle & WS_HSCROLL))
      {
        /* display scrollbar */
        ShowScrollBar(hwnd, SB_HORZ, TRUE);
      }
    }
    else
    {
      /* remove scrollbar if present */
      if (lStyle & WS_HSCROLL)
      {
        /* hide scrollbar */
        ShowScrollBar(hwnd, SB_HORZ, FALSE);
      }
    }
  
    if (infoPtr->nCountPerColumn < GETITEMCOUNT(infoPtr))
    {
      bVScroll = TRUE;

      /* add scrollbar if not already present */
      if (!(lStyle & WS_VSCROLL))
      {
        /* display scrollbar */
        ShowScrollBar(hwnd, SB_VERT, TRUE);
      }
    }
    else
    {
      /* remove scrollbar if present */
      if (lStyle & WS_VSCROLL)
      {
        /* hide scrollbar */
        ShowScrollBar(hwnd, SB_VERT, FALSE);
      }
    }
    break;

  default:
    /* TO DO */
  }

  /* set range and position */
  GetClientRect(hwnd, &rc);
  if ((bHScroll == TRUE) || (bVScroll == TRUE))
  {
    LISTVIEW_SetSize(hwnd, lStyle, rc.right, rc.bottom);
    LISTVIEW_SetViewInfo(hwnd);
    if (bHScroll == TRUE)
    {
      LISTVIEW_SetHScroll(hwnd);
    }

    if (bVScroll == TRUE)
    {
      LISTVIEW_SetVScroll(hwnd);
    }
  }
}

/***
 * DESCRIPTION:
 * Draws a subitem.
 * 
 * PARAMETER(S):
 * [I] HDC : device context handle
 * [I] LISTVIEW_INFO * : listview information
 * [I] LISTVIEW_SUBITEM * : subitem
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
  ExtTextOutA(hdc, lprc->left + 10, lprc->top, ETO_OPAQUE|ETO_CLIPPED, 
              lprc, pszDispText, lstrlenA(pszDispText), NULL);
}

/***
 * DESCRIPTION:
 * Draws an item.
 * 
 * PARAMETER(S):
 * [I] HDC : device context handle
 * [I] LISTVIEW_INFO * : listview information
 * [I] LISTVIEW_ITEM * : item
 * [I] RECT * : clipping rectangle
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_DrawItem(HWND hwnd, HDC hdc, LISTVIEW_ITEM *lpItem, 
                              INT nItem, RECT *lprc)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0); 
  BOOL bSelected;
  INT nLabelWidth;
  INT nImage;
  CHAR szDispText[DISP_TEXT_SIZE];
  LPSTR pszDispText = NULL;
  UINT uState;

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
    if (bSelected == TRUE)
    {
      ImageList_Draw(infoPtr->himlState, uState >> 12, hdc, lprc->left, 
                     lprc->top, ILD_SELECTED);
    }
    else
    {
      ImageList_Draw(infoPtr->himlState, uState >> 12, hdc, lprc->left, 
                     lprc->top, ILD_NORMAL);
    }
    
    lprc->left += infoPtr->iconSize.cx; 
  }
  
  /* small icons */
  if (infoPtr->himlSmall != NULL)
  {
    if (bSelected == TRUE)
    {
      ImageList_Draw(infoPtr->himlSmall, nImage, hdc, lprc->left, 
                     lprc->top, ILD_SELECTED);
    }
    else
    {
      ImageList_Draw(infoPtr->himlSmall, nImage, hdc, lprc->left, 
                     lprc->top, ILD_NORMAL);
    }
    
    lprc->left += infoPtr->iconSize.cx; 
  }

  nLabelWidth = ListView_GetStringWidthA(hwnd, pszDispText);
  if (lprc->left + nLabelWidth < lprc->right)
  {
    lprc->right = lprc->left + nLabelWidth;
  }

  /* draw text */  
  ExtTextOutA(hdc, lprc->left + 1, lprc->top + 1, ETO_OPAQUE|ETO_CLIPPED, 
              lprc, pszDispText, lstrlenA(pszDispText), NULL);
        
  if (lpItem->state & LVIS_FOCUSED)
  {
    Rectangle(hdc, lprc->left, lprc->top, lprc->right, lprc->bottom); 
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
  INT nColumnCount;
  HDPA hdpaSubItems;
  RECT rc;
  INT  j, k;
  INT nItem;
  INT nLast;
  BOOL bNeedSubItem = TRUE;

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
        Header_GetItemRect(infoPtr->hwndHeader, 0, &rc);
        rc.top = nDrawPosY;
        rc.bottom = rc.top + infoPtr->nItemHeight;
        
        /* draw state icon + icon + text */
        LISTVIEW_DrawItem(hwnd, hdc, lpItem, nItem, &rc);
      }

      nColumnCount = Header_GetItemCount(infoPtr->hwndHeader);
      for (k = 1, j = 1; j < nColumnCount; j++)
      {
        Header_GetItemRect(infoPtr->hwndHeader, j, &rc);
        rc.top = nDrawPosY;
        rc.bottom = rc.top + infoPtr->nItemHeight;

        if (k < hdpaSubItems->nItemCount)
        {
          if (bNeedSubItem == TRUE)
          {
            lpSubItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, k);
            k++;
          }

          if (lpSubItem != NULL)
          {
            if (lpSubItem->iSubItem == j)
            {
              LISTVIEW_DrawSubItem(hwnd, hdc, nItem, lpItem->lParam, lpSubItem,
                                   j, &rc);
              bNeedSubItem = TRUE;
            }
            else
            {
              LISTVIEW_DrawSubItem(hwnd, hdc, nItem, lpItem->lParam, NULL, j, 
                                   &rc);
              bNeedSubItem = FALSE;
            }
          }
          else
          {
            LISTVIEW_DrawSubItem(hwnd, hdc, nItem, lpItem->lParam, NULL, j, 
                                 &rc);
            bNeedSubItem = TRUE;
          }
        }
        else
        {
          LISTVIEW_DrawSubItem(hwnd, hdc, nItem, lpItem->lParam, NULL, j, &rc);
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
    if (infoPtr->rcList.right % infoPtr->nColumnWidth == 0)
    {
      nColumnCount = infoPtr->rcList.right / infoPtr->nColumnWidth;
    }
    else
    {
      nColumnCount = infoPtr->rcList.right / infoPtr->nColumnWidth + 1;
    }

    for (i = 0; i < nColumnCount; i++)
    {
      j = 0;
      while ((nItem < GETITEMCOUNT(infoPtr)) && (j < infoPtr->nCountPerColumn))
      {
        hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem);
        if (hdpaSubItems != NULL)
        {
          lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
          if (lpItem != NULL)
          {
            rc.top = j * infoPtr->nItemHeight;
            rc.left = i * infoPtr->nColumnWidth;
            rc.bottom = rc.top + infoPtr->nItemHeight;
            rc.right = rc.left + infoPtr->nColumnWidth;

            /* draw state icon + icon + text */
            LISTVIEW_DrawItem(hwnd, hdc,  lpItem, nItem, &rc);
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
 * Draws listview items when in small icon display mode.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] HDC : device context handle 
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_RefreshSmallIcon(HWND hwnd, HDC hdc)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_ITEM *lpItem;
  HDPA hdpaSubItems;
  RECT rc;
  INT i, j;
  INT nItem = ListView_GetTopIndex(hwnd);

  for (i = 0; i < infoPtr->nCountPerColumn; i++)
  {
    j = 0;
    while ((nItem < GETITEMCOUNT(infoPtr)) && (j < infoPtr->nCountPerRow))
    {
      hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem);
      if (hdpaSubItems != NULL)
      {
        lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
        if (lpItem != NULL)
        {
          rc.top = i * infoPtr->nItemHeight;
          rc.left = j * infoPtr->nColumnWidth;
          rc.bottom = rc.top + infoPtr->nItemHeight;
          rc.right = rc.left + infoPtr->nColumnWidth;

          /* draw state icon + icon + text */
          LISTVIEW_DrawItem(hwnd, hdc,  lpItem, nItem, &rc);
        }
      }
 
      nItem++;
      j++;
    }
  }
}

/***
 * DESCRIPTION:
 * Draws listview items when in icon display mode.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] HDC : device context handle 
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_RefreshIcon(HWND hwnd, HDC hdc)
{
  /* TO DO */
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
    LISTVIEW_RefreshSmallIcon(hwnd, hdc);
    break;
  case LVS_ICON:
    LISTVIEW_RefreshIcon(hwnd, hdc);
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
    wWidth = nColumnCount * infoPtr->nColumnWidth + 2;

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
  LISTVIEW_ITEM *lpItem;
  LISTVIEW_SUBITEM *lpSubItem;
  NMLISTVIEW nmlv;
  BOOL bSuppress;
  BOOL bResult = FALSE;
  INT i;
  INT j;
  HDPA hdpaSubItems;

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
    
    /* reset scroll parameters */
    LISTVIEW_SetScroll(hwnd);

    /* invalidate client area (optimization needed) */
    InvalidateRect(hwnd, NULL, FALSE);
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
  BOOL bResult = FALSE;
  
  if (Header_DeleteItem(infoPtr->hwndHeader, nColumn) == TRUE)
  {
    bResult = LISTVIEW_RemoveColumn(infoPtr->hdpaItems, nColumn);
  }

  /* reset scroll parameters */
  LISTVIEW_SetScroll(hwnd);

  /* refresh client area */
  InvalidateRect(hwnd, NULL, FALSE);

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
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMLISTVIEW nmlv;
  BOOL bResult = FALSE;
  HDPA hdpaSubItems;
  LISTVIEW_ITEM *lpItem;
  LISTVIEW_SUBITEM *lpSubItem;
  INT i;

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

    /* refresh client area */
    InvalidateRect(hwnd, NULL, FALSE);
  }
  
  return bResult;
}

/* << LISTVIEW_EditLabel >> */

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
  INT nLast;
  INT nFirst;
  INT nScrollPos;
  BOOL bResult = TRUE;

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_LIST:
    if (lStyle & WS_HSCROLL)
    {
      nFirst = ListView_GetTopIndex(hwnd);

      /* calculate last fully visible item index */
      nLast = infoPtr->nCountPerColumn * infoPtr->nCountPerRow + nFirst - 1;

      if (nItem > nLast)
      {
        /* calculate new scroll position based on item index */
        if (((nItem - nLast) % infoPtr->nCountPerColumn) == 0)
        {
          nScrollPos = (nItem - nLast) / infoPtr->nCountPerColumn;
        }
        else
        {
          nScrollPos = (nItem - nLast) / infoPtr->nCountPerColumn + 1;
        }

        bResult = ListView_Scroll(hwnd, nScrollPos, 0);
      }
      else if (nItem < nFirst)
      {
        /* calculate new scroll position based on item index */
        if (((nItem - nFirst) % infoPtr->nCountPerColumn) == 0)
        {
          nScrollPos = (nItem - nFirst) / infoPtr->nCountPerColumn;
        }
        else
        {
          nScrollPos = (nItem - nFirst) / infoPtr->nCountPerColumn -1;
        }

        bResult = ListView_Scroll(hwnd, nScrollPos, 0);
      }
    }
    break;

  case LVS_REPORT:
    if (lStyle & WS_VSCROLL)
    {
      nFirst = ListView_GetTopIndex(hwnd);

      /* calculate last fully visible item index */
      nLast = infoPtr->nCountPerColumn + nFirst - 1;
      if (nItem > nLast)
      {
        nScrollPos = nItem - nLast;
        bResult = ListView_Scroll(hwnd, 0, nScrollPos);
      }
      else if (nItem < nFirst)
      {
        nScrollPos = nItem - nFirst;
        bResult = ListView_Scroll(hwnd, 0, nScrollPos);
      }
    }
    break;

  case LVS_SMALLICON:
    /* TO DO */
    break;

  case LVS_ICON:
    /* TO DO */
    break;
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
/*   LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0); */
/*   LISTVIEW_ITEM *lpItem; */
/*   INT nItem; */
/*   INT nEnd = GETITEMCOUNT(infoPtr); */
/*   BOOL bWrap = FALSE; */
/*   if (nStart == -1) */
/*     nStart = 0; */
/*   else */
/*     nStart++ ; */
/*   if (lpFindInfo->flags & LVFI_PARAM) */
/*   { */
/*     for (nItem = nStart; nItem < nEnd; nItem++) */
/*     { */
       /* get item pointer */ 
/*       lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(infoPtr->hdpaItems, nItem); */
/*       if (lpItem != NULL)  */
/*       { */
/*         if (lpFindInfo->lParam == lpItem->lParam) */
/*           return nItem; */
/*       } */
/*     } */
/*   } */
/*   else */
/*   { */
/*     if (lpFindInfo->flags & LVFI_PARTIAL) */
/*     { */
/*       for (nItem = nStart; nItem < nEnd; nItem++) */
/*       { */
         /* get item pointer */
/*         lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(infoPtr->hdpaItems, nItem); */
/*         if (lpItem)  */
/*         { */
/*           if (strncmp(lpItem->pszText, lpFindInfo->psz, strlen(lpFindInfo->psz))  */
/*               == 0) */
/*             return nItem; */
/*         } */
/*       } */
/*     } */

/*     if (lpFindInfo->flags & LVFI_STRING) */
/*     { */
/*       for (nItem = nStart; nItem < nEnd; nItem++) */
/*       { */
         /* get item pointer */
/*         lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(infoPtr->hdpaItems, nItem); */
/*         if (lpItem != NULL)  */
/*         { */
/*           if (strcmp(lpItem->pszText, lpFindInfo->psz) == 0) */
/*             return nItem; */
/*         } */
/*       } */
/*     } */
    
/*     if ((lpFindInfo->flags & LVFI_WRAP) && nStart)  */
/*     { */
/*       nEnd = nStart; */
/*       nStart = 0; */
/*       bWrap = TRUE; */
/*     } */
/*     else */
/*       bWrap = FALSE; */

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
/*  static LRESULT LISTVIEW_GetBkImage(HWND hwnd, LPLVBKIMAGE lpBkImage)  */
/*  {  */
/*    return FALSE;  */
/*  }  */

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
    if (bResult == TRUE)
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

/* << LISTVIEW_GetColumnW >> */
/* << LISTVIEW_GetColumnOrderArray >> */

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

  if ((LVS_TYPEMASK & lStyle) ==  LVS_LIST)
  {
    nColumnWidth = infoPtr->nColumnWidth;
  }
  else if ((LVS_TYPEMASK & lStyle) ==  LVS_REPORT)
  {
    /* get column width from header */
    ZeroMemory(&hdi, sizeof(HDITEMA));
    hdi.mask = HDI_WIDTH;
    if (Header_GetItemA(infoPtr->hwndHeader, nColumn, &hdi) == TRUE)
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
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nItemCount = 0;

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_LIST:
    if (infoPtr->rcList.right / infoPtr->nColumnWidth)
    {
      nItemCount = infoPtr->nCountPerRow * infoPtr->nCountPerColumn;
    }
    break;
  
  case LVS_REPORT:
    nItemCount = infoPtr->nCountPerColumn;
    break;

  default:
    nItemCount = GETITEMCOUNT(infoPtr);
  }

  return nItemCount;
}

/* << LISTVIEW_GetEditControl >> */
/* << LISTVIEW_GetExtendedListViewStyle >> */

/***
 * DESCRIPTION:
 * Retrieves a header handle.
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

/* << LISTVIEW_GetHotCursor >> */
/* << LISTVIEW_GetHotItem >> */
/* << LISTVIEW_GetHoverTime >> */

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

/* << LISTVIEW_GetISearchString >> */

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

/* << LISTVIEW_GetItemW >> */
/* << LISTVIEW_GetHotCursor >> */
/* << LISTVIEW_GetHotItem >> */
/* << LISTVIEW_GetHoverTime >> */

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
static LRESULT LISTVIEW_GetItemPosition(HWND hwnd, INT nItem, 
                                        LPPOINT lppt)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nColumn;
  INT nRow;
  BOOL bResult = FALSE;
  INT nFirst;
  INT nLast;

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)) && (lppt != NULL))
  {
    switch (LVS_TYPEMASK & lStyle)
    {
    case LVS_LIST:
      nFirst = ListView_GetTopIndex(hwnd);
      nLast = infoPtr->nCountPerColumn * infoPtr->nCountPerRow + nFirst - 1;

      if ((nItem >= nFirst) || (nItem <= nLast))
      {
        nItem -= nFirst; 
       
        /* get column */
        nColumn = nItem / infoPtr->nCountPerColumn;

        /* get row */
        nRow = nItem % infoPtr->nCountPerColumn;
          
        /* X coordinate of the column */
        lppt->x = nColumn * infoPtr->nColumnWidth + infoPtr->rcList.left;

        /* Y coordinate of the item */
        lppt->y = nRow * infoPtr->nItemHeight + infoPtr->rcList.top;
          
        bResult = TRUE;
      }
      break;

    case LVS_REPORT:
      nFirst = ListView_GetTopIndex(hwnd);
      nLast = infoPtr->nCountPerColumn * infoPtr->nCountPerRow + nFirst - 1;

      /* get column */
      nColumn = nItem / infoPtr->nCountPerColumn;
      
      /* get row */
      nRow = nItem % infoPtr->nCountPerColumn;

      if ((nItem >= nFirst) || (nItem <= nLast))
      {
        nItem -= nFirst; 
   
        /* get column */
        nColumn = nItem / infoPtr->nCountPerColumn;

        /* get row */
        nRow = nItem % infoPtr->nCountPerColumn;

        /* X coordinate of the column */
        lppt->x = infoPtr->rcList.left;

        /* Y coordinate of the item */
        lppt->y = nRow * infoPtr->nItemHeight + infoPtr->rcList.top;
          
        bResult = TRUE;
      }        

    default:
      /* TO DO */
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
  LISTVIEW_ITEM *lpItem;
  HDPA hdpaSubItems;
  INT nLabelWidth = 0;
  INT nStateWidth = 0;
  INT nIconWidth = 0;
  BOOL bResult = FALSE;
  CHAR szDispText[DISP_TEXT_SIZE];
  LPSTR pszDispText;
  POINT pt;

  /*init pointer */
  pszDispText = szDispText;

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)) && (lprc != NULL))
  {
    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem);
    if (hdpaSubItems != NULL)
    {
      lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
      if (lpItem != NULL)
      {
        if ((LVS_TYPEMASK & lStyle) == LVS_ICON)
        {

        }
        else
        {
          if (ListView_GetItemPosition(hwnd, nItem, &pt) == TRUE)
          {
            /* get width of label in pixels */
            LISTVIEW_GetItemDispInfo(hwnd, nItem, lpItem, NULL, NULL, 
                                     &pszDispText, DISP_TEXT_SIZE);
            nLabelWidth = ListView_GetStringWidthA(hwnd, pszDispText); 
          }

          if (infoPtr->himlState != NULL)
          {
            nStateWidth = infoPtr->iconSize.cx;
          }

          if (infoPtr->himlSmall == NULL)
          {
            nIconWidth = infoPtr->iconSize.cx;
          }

          switch(lprc->left)  
          {  
          case LVIR_BOUNDS:  
            lprc->left = pt.x;
            lprc->top = pt.y;
            lprc->right = lprc->left + nStateWidth + nIconWidth + nLabelWidth;
            lprc->bottom = lprc->top + infoPtr->nItemHeight;
            bResult = TRUE;
            break;
          case LVIR_ICON:
            lprc->left = pt.x + nStateWidth;
            lprc->top = pt.y;
            lprc->right = lprc->left + nIconWidth;
            lprc->bottom = lprc->top + infoPtr->nItemHeight;
            bResult = TRUE;
            break;
          case LVIR_LABEL: 
            lprc->left = pt.x + nIconWidth + nStateWidth;
            lprc->top = pt.y;
            lprc->right = lprc->left + nLabelWidth;
            lprc->bottom = infoPtr->nItemHeight;
            bResult = TRUE;
            break;
          case LVIR_SELECTBOUNDS:
            lprc->left = pt.x + nStateWidth;
            lprc->top = pt.y;
            lprc->right = lprc->left + nIconWidth + nLabelWidth;
            lprc->bottom = lprc->top + infoPtr->nItemHeight;
            bResult = TRUE;
          }
        }
      }
    }
  }
        
  return bResult;
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

  if (bSmall == TRUE)
  {
    lResult = MAKELONG(infoPtr->largeIconSpacing.cx, 
                       infoPtr->largeIconSpacing.cy);
  }
  else
  {
    lResult = MAKELONG(infoPtr->smallIconSpacing.cx, 
                       infoPtr->smallIconSpacing.cy);
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
    if (ListView_GetItemA(hwnd, &lvItem) == TRUE)
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
static LRESULT LISTVIEW_GetItemTextA(HWND hwnd, INT nItem, 
                                       LPLVITEMA lpLVItem)
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
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nResult = -1;

  if (nItem == -1)
  {
    /* start at begin */
    nItem = 0;
  }
  else
  {
    /* exclude specified item */
    nItem++;
  }

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
  {
    /* TO DO */
  }  
  
  return nResult;
}

/* << LISTVIEW_GetNumberOfWorkAreas >> */

/***
 * DESCRIPTION:
 * Retrieves the current origin when in icon or small icon display mode.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [O] LPPOINT : coordinate information
 * 
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_GetOrigin(HWND hwnd, LPPOINT lpOrigin)
{
  LONG lStyle = GetWindowLongA(hwnd, GWL_ID);
  BOOL bResult = FALSE;

  if ((lStyle & LVS_TYPEMASK) == LVS_SMALLICON) 
  {
    /* TO DO */
    lpOrigin->x = 0;
    lpOrigin->y = 0;
  }
  else if ((lStyle & LVS_TYPEMASK) == LVS_ICON) 
  {
    /* TO DO */
    lpOrigin->x = 0;
    lpOrigin->y = 0;
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
static LRESULT LISTVIEW_GetStringWidthA(HWND hwnd, LPCSTR lpsz)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  HFONT hFont, hOldFont;
  HDC hdc;
  SIZE textSize;

  /* initialize */ 
  ZeroMemory(&textSize, sizeof(SIZE));
  if (lpsz != NULL)
  {
    hFont = infoPtr->hFont ? infoPtr->hFont : infoPtr->hDefaultFont;
    hdc = GetDC(hwnd);
    hOldFont = SelectObject(hdc, hFont);
    GetTextExtentPointA(hdc, lpsz, lstrlenA(lpsz), &textSize);
    SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);
  }

  return textSize.cx;
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
static LRESULT LISTVIEW_GetViewRect(HWND hwnd, LPRECT lprc)
{
  /* LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0); */
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  BOOL bResult = FALSE;

  if (lprc != NULL)
  {
    if (((lStyle & LVS_TYPEMASK) == LVS_ICON) || 
        ((lStyle & LVS_TYPEMASK) == LVS_SMALLICON))
    {
      /* TO DO */
    }
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Determines which section of the item was selected (if any).
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : item index
 * [IO] LPLVHITTESTINFO : hit test information
 *
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static INT LISTVIEW_HitTestItem(HWND hwnd, INT nItem, 
                                  LPLVHITTESTINFO lpHitTestInfo)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  HDPA hdpaSubItems;
  LISTVIEW_ITEM *lpItem;
  INT nOffset;
  INT nLabelWidth;
  INT nPosX = 0;
  CHAR szDispText[DISP_TEXT_SIZE];
  LPSTR pszDispText;

  /*init pointer */
  pszDispText = szDispText;

  if ((LVS_TYPEMASK & lStyle) == LVS_LIST)
  {
    /* calculate offset from start of item (in pixels) */
    nOffset = lpHitTestInfo->pt.x % infoPtr->nColumnWidth;
  }
  else
  {
    nOffset = lpHitTestInfo->pt.x;
  }

  /* verify existance of item */
  if (nItem < GETITEMCOUNT(infoPtr))
  {
    /* get item */
    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem);
    if (hdpaSubItems != NULL)
    {
      lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
      if (lpItem != NULL)
      {
        if (infoPtr->himlState != NULL)
        {
          nPosX += infoPtr->iconSize.cx;
          if (nOffset <= nPosX)
          {
            lpHitTestInfo->flags = LVHT_ONITEMSTATEICON | LVHT_ONITEM;
            lpHitTestInfo->iItem = nItem;
            lpHitTestInfo->iSubItem = 0;
            return nItem;
          }
        }

        if (infoPtr->himlSmall != NULL)
        {
          nPosX += infoPtr->iconSize.cx;
          if (nOffset <= nPosX)
          {
            lpHitTestInfo->flags = LVHT_ONITEMICON | LVHT_ONITEM;
            lpHitTestInfo->iItem = nItem;
            lpHitTestInfo->iSubItem = 0;
            return nItem;
          }
        }

        /* get width of label in pixels */
        LISTVIEW_GetItemDispInfo(hwnd, nItem, lpItem, NULL, NULL, 
                                 &pszDispText, DISP_TEXT_SIZE);
        nLabelWidth = ListView_GetStringWidthA(hwnd, pszDispText); 
        nLabelWidth += nPosX;

        if (nOffset <= nLabelWidth)
        {
          lpHitTestInfo->flags = LVHT_ONITEMLABEL | LVHT_ONITEM;
          lpHitTestInfo->iItem = nItem;
          lpHitTestInfo->iSubItem = 0;
          return nItem;
        }
      }
    }
  }

  /* hit is not on item */
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
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nColumn;
  INT nRow;
  INT nTopIndex;
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
    switch (LVS_TYPEMASK & lStyle)
    {
    case LVS_LIST:
      /* get current column */
      nColumn = lpHitTestInfo->pt.x / infoPtr->nColumnWidth;

      /* get current row */ 
      nRow = lpHitTestInfo->pt.y / infoPtr->nItemHeight;
    
      /* get the index of the first visible item */
      nTopIndex = ListView_GetTopIndex(hwnd);

      nItem = nColumn * infoPtr->nCountPerColumn + nTopIndex + nRow;
      nItem = LISTVIEW_HitTestItem(hwnd, nItem, lpHitTestInfo);
      break;

    case LVS_REPORT:
      /* get current row */ 
      nRow = ((lpHitTestInfo->pt.y - infoPtr->rcList.top) / 
              infoPtr->nItemHeight);

      /* get the index of the first visible item */
      nTopIndex = ListView_GetTopIndex(hwnd);
    
      nItem = nTopIndex + nRow;
      nItem = LISTVIEW_HitTestItem(hwnd, nItem, lpHitTestInfo);
      break;

    default:
/*       nItem = LISTVIEW_HitTestItem(hwnd, nItem, lpHitTestInfo); */
    }
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

    LISTVIEW_SetScroll(hwnd);
    InvalidateRect(hwnd, NULL, FALSE);
  }

  return nNewColumn;
}

/* << LISTVIEW_InsertColumnW >> */

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
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMLISTVIEW nmlv;
  INT nItem = -1;
  HDPA hdpaSubItems;
  LISTVIEW_ITEM *lpItem = NULL;

  if (lpLVItem != NULL)
  {
    /* make sure it's not a subitem; cannot insert a subitem */
    if (lpLVItem->iSubItem == 0)
    {
      lpItem = (LISTVIEW_ITEM *)COMCTL32_Alloc(sizeof(LISTVIEW_ITEM));
      if (lpItem != NULL)
      {
        ZeroMemory(lpItem, sizeof(LISTVIEW_ITEM));
        if (LISTVIEW_InitItem(hwnd, lpItem, lpLVItem) == TRUE)
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

                /* set scrolling parameters */
                LISTVIEW_SetScroll(hwnd);
                
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

/* << LISTVIEW_InsertItemW >> */

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

/***
 * DESCRIPTION:
 * Scrolls the content of a listview.
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] INT : amount of horizontal scrolling
 * [I] INT : amount of vertical scrolling
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_Scroll(HWND hwnd, INT nHScroll, INT nVScroll)
{
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nScrollPos = 0;
  BOOL bResult = FALSE;
  INT nMinRange;
  INT nMaxRange;

  if (lStyle & WS_HSCROLL)
  {
    nScrollPos = GetScrollPos(hwnd, SB_HORZ);
    GetScrollRange(hwnd, SB_HORZ, &nMinRange, &nMaxRange);
    nScrollPos += nHScroll;

    if ((LVS_TYPEMASK & lStyle) == LVS_LIST)
    {
      nScrollPos = GetScrollPos(hwnd, SB_HORZ);
      GetScrollRange(hwnd, SB_HORZ, &nMinRange, &nMaxRange);
      nScrollPos += nHScroll;
      if ((nMinRange <= nScrollPos) && (nScrollPos <= nMaxRange))
      {
        bResult = TRUE;
        SetScrollPos(hwnd, SB_HORZ, nScrollPos, TRUE);
      }
    }
    else
    {
      /* TO DO */
    }
  }
  
  if (lStyle & WS_VSCROLL)
  {
    if ((LVS_TYPEMASK & lStyle) == LVS_REPORT)
    {
      nScrollPos = GetScrollPos(hwnd, SB_VERT);
      GetScrollRange(hwnd, SB_VERT, &nMinRange, &nMaxRange);
      nScrollPos += nVScroll;
      if ((nMinRange <= nScrollPos) && (nScrollPos <= nMaxRange))
      {
        bResult = TRUE;
        SetScrollPos(hwnd, SB_VERT, nScrollPos, TRUE);
      }
    }
    else
    {
      /* TO DO */
    }
  }

  if (bResult == TRUE)
  {
    /* refresh client area */
    InvalidateRect(hwnd, NULL, TRUE);
  }

  return bResult; 
}

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
 * window stores the state information (some or all).
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
 * Sets column attributes.
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
  HDITEMA hdi;
  BOOL bResult = FALSE;

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
 * Sets image list.
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
 * Sets item attributes.
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

/* << LISTVIEW_SetItemW >> */

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
  /* TO DO */
}

/***
 * DESCRIPTION:
 * Sets item position.
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
static LRESULT LISTVIEW_SetItemPosition(HWND hwnd, INT nItem, 
                                        INT nPosX, INT nPosY)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  BOOL bResult = FALSE;

  if ((nItem >= 0) || (nItem < GETITEMCOUNT(infoPtr)))
  {
    if (((lStyle & LVS_TYPEMASK) == LVS_ICON) || 
        ((lStyle & LVS_TYPEMASK) == LVS_SMALLICON))
    {
      /* TO DO */
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
static LRESULT LISTVIEW_SetItemState(HWND hwnd, INT nItem, 
                                     LPLVITEMA lpLVItem)
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
static BOOL LISTVIEW_SetItemTextA(HWND hwnd, INT nItem, 
                                      LPLVITEMA lpLVItem)
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

  return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the text background color.
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

  return TRUE;
}

/***
 * DESCRIPTION:
 * Sort items.
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
 * Creates a listview control.
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
  HDLAYOUT hl;
  WINDOWPOS wp;

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
  infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
  infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
  ZeroMemory(&infoPtr->rcList, sizeof(RECT));

  /* get default font (icon title) */
  SystemParametersInfoA(SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
  infoPtr->hDefaultFont = CreateFontIndirectA(&logFont);
  infoPtr->hFont = infoPtr->hDefaultFont;
  
  /* create header */
  infoPtr->hwndHeader =	CreateWindowA(WC_HEADERA, (LPCSTR)NULL, 
                                        WS_CHILD | HDS_HORZ | HDS_BUTTONS , 
                                        0, 0, 0, 0, hwnd, (HMENU)0, 
                                        lpcs->hInstance, NULL);

  /* set header font */
  SendMessageA(infoPtr->hwndHeader, WM_SETFONT, (WPARAM)infoPtr->hFont, 
               (LPARAM)TRUE);

  switch (lpcs->style & LVS_TYPEMASK)
  {
  case LVS_REPORT:
    /* reset header */
    hl.prc = &infoPtr->rcList;
    hl.pwpos = &wp;
    Header_Layout(infoPtr->hwndHeader, &hl);
    SetWindowPos(infoPtr->hwndHeader, hwnd, wp.x, wp.y, wp.cx, wp.cy, 
                   wp.flags);

    /* set new top coord */
    infoPtr->rcList.top = wp.cy;

    /* display header */
    ShowWindow(infoPtr->hwndHeader, SW_SHOWNORMAL);
    break;

  case LVS_LIST:
    break;

  default:
    /*  temporary (until there is support) */
    SetWindowLongA(hwnd, GWL_STYLE, 
                     (lpcs->style & ~LVS_TYPEMASK) | LVS_LIST);
  }

  /* TEMPORARY */
  LISTVIEW_UnsupportedStyles(lpcs->style);

  /* allocate memory */
  infoPtr->hdpaItems = DPA_Create(10);

  /* set view dependent information */
  LISTVIEW_SetViewInfo(hwnd);

  return 0;
}


/***
 * DESCRIPTION:
 * Destroys the window
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * 
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_Destroy(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLongA(hwnd, 0);

  /* delete all items */
  LISTVIEW_DeleteAllItems(hwnd);

  /* destroy dpa */
  DPA_Destroy(infoPtr->hdpaItems);

  /* destroy header */
  if (infoPtr->hwndHeader)
  {
    DestroyWindow(infoPtr->hwndHeader);
  }

  /* destroy font */
  infoPtr->hFont = (HFONT)0;
  if (infoPtr->hDefaultFont)
  {
    DeleteObject(infoPtr->hDefaultFont);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Erases the background of the listview control
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
    HBRUSH hBrush = CreateSolidBrush(infoPtr->clrBk);
    FillRect((HDC)wParam, &infoPtr->rcList, hBrush);
    DeleteObject(hBrush);
    bResult = TRUE;
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Gets the listview control font.
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
static LRESULT LISTVIEW_VScroll(HWND hwnd, INT nScrollCode, 
                                INT nScroll, HWND hScrollWnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nScrollPos, nOldScrollPos;
  INT nMinRange;
  INT nMaxRange;

  GetScrollRange(hwnd, SB_VERT, &nMinRange, &nMaxRange);
  nOldScrollPos = nScrollPos = GetScrollPos(hwnd, SB_VERT);

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_REPORT:
    switch (nScrollCode)
    {
    case SB_LINEUP:
      if (nScrollPos > nMinRange)
      {
        nScrollPos--;
      }
      break;
    case SB_LINEDOWN:
      if (nScrollPos < nMaxRange)
      {
        nScrollPos++;
      }
      break;
    case SB_PAGEUP:
      if (nScrollPos > nMinRange + infoPtr->nCountPerColumn)
      {
        nScrollPos -= infoPtr->nCountPerColumn;
      }
      else
      {
        nScrollPos = nMinRange;
      }
      break;
    case SB_PAGEDOWN:
      if (nScrollPos < nMaxRange - infoPtr->nCountPerColumn)
      {
        nScrollPos += infoPtr->nCountPerColumn;
      }
      else
      {
        nScrollPos = nMaxRange;
      }
      break;
    case SB_THUMBPOSITION:
      nScrollPos = nScroll;
      break;
    }
    break;

    default:
      /* TO DO */
  }

  /* set new scroll position */
  if (nScrollPos != nOldScrollPos)
  {
    SetScrollPos(hwnd, SB_VERT, nScrollPos, TRUE);
    
    /* refresh client area */
    InvalidateRect(hwnd, NULL, TRUE);
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
  INT nScrollPos, nOldScrollPos;
  INT nMinRange;
  INT nMaxRange;

  GetScrollRange(hwnd, SB_HORZ, &nMinRange, &nMaxRange);
  nOldScrollPos = nScrollPos = GetScrollPos(hwnd, SB_HORZ);

  switch (LVS_TYPEMASK & lStyle)
  {
  case LVS_LIST:

    /* list display mode horizontal scrolling */
    switch (nScrollCode)
    {
    case SB_LINELEFT:
      if (nScrollPos > nMinRange)
      {
        nScrollPos--;
      }
      break;
    case SB_LINERIGHT:
      if (nScrollPos < nMaxRange)
      {
        nScrollPos++;
      }
      break;
    case SB_PAGELEFT:
      if (nScrollPos > nMinRange)
      {
        nScrollPos = max(nMinRange, nScrollPos - infoPtr->nCountPerRow);
      }
      else
      {
        nScrollPos = nMinRange;
      }
      break;
    case SB_PAGERIGHT:
      if (nScrollPos < nMaxRange - 1)
      {
        nScrollPos = min(nMaxRange, nScrollPos + infoPtr->nCountPerRow);
      }
      else
      {
        nScrollPos = nMaxRange;
      }
      break;
    case SB_THUMBPOSITION:
      nScrollPos = nScroll;
      break;
    }
    break;

  case LVS_REPORT:

    /* report/details display mode horizontal scrolling */
    switch (nScrollCode)
    {
    case SB_LINELEFT:
      if (nScrollPos > nMinRange)
      {
        nScrollPos--;
      }
      break;
    case SB_LINERIGHT:
      if (nScrollPos < nMaxRange)
      {
        nScrollPos++;
      }
      break;
    case SB_PAGELEFT:
      if (nScrollPos > nMinRange)
      {
        nScrollPos = max(nMinRange, nScrollPos - infoPtr->rcList.right);
      }
      else
      {
        nScrollPos = nMinRange;
      }
      break;
    case SB_PAGERIGHT:
      if (nScrollPos < nMaxRange)
      {
        nScrollPos = min(nMaxRange, nScrollPos + infoPtr->rcList.right);
      }
      else
      {
        nScrollPos = nMaxRange;
      }
      break;
    case SB_THUMBPOSITION:
      nScrollPos = nScroll;
      break;
    }
    break;

    default:
      /* TO DO */
  }

  /* set new scroll position */
  if (nScrollPos != nOldScrollPos)
  {
    SetScrollPos(hwnd, SB_HORZ, nScrollPos, TRUE);

    /* refresh client area */
    InvalidateRect(hwnd, NULL, TRUE);
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
        LISTVIEW_KeySelection(hwnd, infoPtr->nFocusedItem - infoPtr->nCountPerColumn);
      }
      break;
    case LVS_REPORT:
      break;

    default:
      if (infoPtr->nFocusedItem % infoPtr->nCountPerRow != 0)
      {
        LISTVIEW_SetSelection(hwnd, infoPtr->nFocusedItem - 1); 
      }
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
      if (infoPtr->nFocusedItem >= infoPtr->nCountPerRow)
      {
        LISTVIEW_SetSelection(hwnd, infoPtr->nFocusedItem - 
                              infoPtr->nCountPerRow);
      }
    }
    break;
    
  case VK_RIGHT:
    switch (LVS_TYPEMASK & lStyle)
    {
    case LVS_LIST:
      if (infoPtr->nFocusedItem < 
          (GETITEMCOUNT(infoPtr) - infoPtr->nCountPerColumn))
      {
        LISTVIEW_KeySelection(hwnd, infoPtr->nFocusedItem + 
                              infoPtr->nCountPerColumn);
      }
    case LVS_REPORT:
      break;

    default:
      if (infoPtr->nCountPerRow > 0)
      {
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
    default:
      if (infoPtr->nCountPerRow > 0)
      {
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
 * Left mouse button double click.
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
 * Left mouse button down.
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
  LVHITTESTINFO hitTestInfo;
  NMHDR nmh;
  INT nItem;
  static BOOL bGroupSelect = TRUE;

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

  /* set left button hit coordinates */
  hitTestInfo.pt.x = wPosX;
  hitTestInfo.pt.y = wPosY;
  
  /* perform hit test */
  nItem = ListView_HitTest(hwnd, &hitTestInfo);
  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
  {
    if ((wKey & MK_CONTROL) && (wKey & MK_SHIFT))
    {
      if (bGroupSelect == TRUE)
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
 * Left mouse button up.
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
  INT nCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMHDR nmh;

  if (infoPtr->bLButtonDown == TRUE) 
  {
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

  /* free listview info */
  COMCTL32_Free(infoPtr);

  return 0;
}

/***
 * DESCRIPTION:
 * Handles notification from children.
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
      InvalidateRect(hwnd, NULL, TRUE);
    }
  }

  return 0;
}

static LRESULT LISTVIEW_NotifyFormat(HWND hwndFrom, HWND hwnd, INT nCommand)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  
  if (nCommand == NF_REQUERY)
  {
    /* determine the type of structures to use */
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
 * Draws the listview control.
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
 * Right mouse button double click.
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
 * Right mouse button input.
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
  LVHITTESTINFO hitTestInfo;
  NMHDR nmh;
  INT nItem;

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

  /* set hit coordinates */
  hitTestInfo.pt.x = wPosX;
  hitTestInfo.pt.y = wPosY;

  /* perform hit test */
  nItem = ListView_HitTest(hwnd, &hitTestInfo);
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
 * Right mouse button up.
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

  if (infoPtr->bRButtonDown == TRUE) 
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
  InvalidateRect(hwnd, NULL, FALSE);
  
  if (fRedraw == TRUE)
  {
    UpdateWindow(hwnd);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Resizes the listview control.  
 * 
 * PARAMETER(S):
 * [I] HWND : window handle
 * [I] WORD : new width
 * [I] WORD : new height
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_Size(HWND hwnd, WORD wWidth, WORD wHeight)
{
  LISTVIEW_SetSize(hwnd, GetWindowLongA(hwnd, GWL_STYLE), wWidth, wHeight);
  LISTVIEW_SetViewInfo(hwnd);
  LISTVIEW_SetScroll(hwnd);

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
  INT lTop = 0;
  RECT rc;

  switch (lStyle & LVS_TYPEMASK)
  {
  case LVS_LIST:
    if ((lStyle & LVS_TYPEMASK) == LVS_LIST)
    {
      if (!(lStyle & WS_HSCROLL))
      {
        INT nHScrollHeight;
      
        nHScrollHeight = GetSystemMetrics(SM_CYHSCROLL);
        if (lHeight > nHScrollHeight)
        { 
          lHeight -= nHScrollHeight;
        }
      }
    }
    break;

  case LVS_REPORT:
    rc.left = 0;
    rc.top = 0;
    rc.right = lWidth;
    rc.bottom = lHeight;
    hl.prc = &rc;
    hl.pwpos = &wp;
    Header_Layout(infoPtr->hwndHeader, &hl);
    lTop = wp.cy;
    break;
  }
    
  infoPtr->rcList.top = lTop;
  infoPtr->rcList.left = 0;
  infoPtr->rcList.bottom = lHeight;
  infoPtr->rcList.right = lWidth;
}

/***
 * DESCRIPTION:
 * Notification sent when the window style is modified.
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
  HDLAYOUT hl;
  WINDOWPOS wp;
  HDC hdc;
  RECT rc;
  HBRUSH hBrush;

  if (wStyleType == GWL_STYLE)
  {
    /* remove vertical scrollbar */
    if (lpss->styleOld & WS_VSCROLL)
    {
      ShowScrollBar(hwnd, SB_VERT, FALSE);
    }

    /* remove horizontal scrollbar */
    if (lpss->styleOld & WS_HSCROLL)
    {
      ShowScrollBar(hwnd, SB_HORZ, FALSE);
    }

    if ((LVS_TYPEMASK & lpss->styleOld) == LVS_REPORT)
    {
      /* hide header */
      ShowWindow(infoPtr->hwndHeader, SW_HIDE);
    }

    /* erase everything */
    GetClientRect(hwnd, &rc);
    hdc = GetDC(hwnd);
    hBrush = CreateSolidBrush(infoPtr->clrBk);
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);
    ReleaseDC(hwnd, hdc);

    if ((lpss->styleNew & LVS_TYPEMASK) == LVS_REPORT)
    {
      hl.prc = &rc;
      hl.pwpos = &wp;
      Header_Layout(infoPtr->hwndHeader, &hl);
      SetWindowPos(infoPtr->hwndHeader, hwnd, wp.x, wp.y, wp.cx, wp.cy, 
                   wp.flags);
      ShowWindow(infoPtr->hwndHeader, SW_SHOWNORMAL);
    }
    else if (((lpss->styleNew & LVS_TYPEMASK) == LVS_ICON) ||
             ((lpss->styleNew & LVS_TYPEMASK) == LVS_SMALLICON))
    {
      /* for NOW (it's only temporary) */
      SetWindowLongA(hwnd, GWL_STYLE, (lpss->styleNew&~LVS_TYPEMASK)|LVS_LIST);
    }

    /* TEMPORARY */
    LISTVIEW_UnsupportedStyles(lpss->styleNew);

    /* set new size */
    LISTVIEW_SetSize(hwnd, lpss->styleNew, rc.right, rc.bottom);

    /* recalculate attributes */
    LISTVIEW_SetViewInfo(hwnd);

    /* set scrollbars, if needed */
    LISTVIEW_SetScroll(hwnd);
    
    /* invalidate + erase background */
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

  case LVM_SCROLL: 
    return LISTVIEW_Scroll(hwnd, (INT)wParam, (INT)lParam);

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
    
  case WM_DESTROY:
    return LISTVIEW_Destroy(hwnd);
    
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
    return LISTVIEW_Size(hwnd, LOWORD(lParam), HIWORD(lParam));

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
      ERR(listview, "unknown msg %04x wp=%08x lp=%08lx\n",
          uMsg, wParam, lParam);
    }

    /* call default window procedure */
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
  }

  return 0;
}

/***
 * DESCRIPTION:`
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
    RegisterClassA (&wndClass);
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
    UnregisterClassA(WC_LISTVIEWA, (HINSTANCE)NULL);
}

