/*
 * Listview control
 *
 * Copyright 1998 Eric Kohl
 * Copyright 1999 Luc Tourangeau
 *
 * NOTES
 * Listview control implementation. The behavior conforms to version 4.70 
 * and earlier of the listview contol.
 *
 * TODO:
 *   Most messages and notifications
 *   Report, small icon and icon display modes 
 */

#include <string.h>
#include "commctrl.h"
#include "listview.h"
#include "win.h"
#include "debug.h"
#include "winuser.h"

/* prototypes */
static INT LISTVIEW_GetItemCountPerColumn(HWND hwnd);
static INT LISTVIEW_GetItemCountPerRow(HWND hwnd);
static INT LISTVIEW_GetFirstVisibleItem(HWND hwnd);
static VOID LISTVIEW_SetVisible(HWND hwnd, INT nItem);


/***
 * DESCRIPTION:
 * Scrolls the specified item into the visible area.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : item index
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_SetVisible(HWND hwnd, INT nItem)
{
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nItemCountPerRow;
  INT nItemCountPerColumn;
  INT nLastItem;
  INT nFirstItem;
  INT nHScrollPos;
  INT nVScrollPos;

  /* retrieve the index of the first visible fully item */
  nFirstItem = LISTVIEW_GetFirstVisibleItem(hwnd);

  /* nunber of fully visible items per row */
  nItemCountPerRow = LISTVIEW_GetItemCountPerRow(hwnd);

  /* nunber of fully visible items per column */
  nItemCountPerColumn = LISTVIEW_GetItemCountPerColumn(hwnd);

  if ((nItemCountPerRow > 0) && (nItemCountPerColumn > 0))
  {
    if (lStyle & LVS_LIST)
    {
      if (lStyle & WS_HSCROLL)
      {
        /* last visible item index */
        nLastItem = nItemCountPerColumn * nItemCountPerRow + nFirstItem - 1;
        if (nItem > nLastItem)
        {
          /* calculate new scroll position based on item index */
          if (((nItem - nLastItem) % nItemCountPerColumn) == 0)
            nHScrollPos = (nItem - nLastItem) / nItemCountPerColumn;
          else
            nHScrollPos = (nItem - nLastItem) / nItemCountPerColumn + 1;
          SendMessageA(hwnd, LVM_SCROLL, (WPARAM)nHScrollPos, (LPARAM)0);
        }
        else if (nItem < nFirstItem)
        {
          /* calculate new scroll position based on item index */
          if (((nItem - nFirstItem) % nItemCountPerColumn) == 0)
            nHScrollPos = (nItem - nFirstItem) / nItemCountPerColumn;
          else
            nHScrollPos = (nItem - nFirstItem) / nItemCountPerColumn - 1;
          SendMessageA(hwnd, LVM_SCROLL, (WPARAM)nHScrollPos, (LPARAM)0);
        }
      }
    }
    else if (lStyle & LVS_REPORT)
    {
      if (lStyle & WS_VSCROLL)
      {
        if (nFirstItem > 0)
        {
          nLastItem = nItemCountPerColumn + nFirstItem;
        }
        else
{
          nLastItem = nItemCountPerColumn + nFirstItem - 1;
        }

        if (nItem > nLastItem)
        {
          /* calculate new scroll position based on item index */
          nVScrollPos = nItem - nLastItem;
          SendMessageA(hwnd, LVM_SCROLL, (WPARAM)0, (LPARAM)nVScrollPos);
        }
        else if (nItem < nFirstItem)
        {
          /* calculate new scroll position based on item index */
          nVScrollPos = nItem - nFirstItem;
          SendMessageA(hwnd, LVM_SCROLL, (WPARAM)0, (LPARAM)nVScrollPos);
        }
      }
    }
    else if ((lStyle & LVS_SMALLICON) || (lStyle & LVS_ICON))
    {
      if (lStyle & WS_VSCROLL)
      {
        /* last visible item index */
        nLastItem = nItemCountPerColumn * nItemCountPerRow + nFirstItem - 1;
        if (nItem > nLastItem)
        {
          /* calculate new scroll position based on item index */
          nVScrollPos = (nItem - nLastItem) / nItemCountPerRow + 1;
          SendMessageA(hwnd, LVM_SCROLL, (WPARAM)0, (LPARAM)nVScrollPos);
        }
        else if (nItem < nFirstItem)
        {
          /* calculate new scroll position based on item index */
          nHScrollPos = (nItem - nFirstItem) / nItemCountPerRow - 1;
          SendMessageA(hwnd, LVM_SCROLL, (WPARAM)0, (LPARAM)nHScrollPos);
        }
      }
    }

    /* refresh display */
    InvalidateRect(hwnd, NULL, FALSE);
    UpdateWindow(hwnd);

}
}

/***
 * DESCRIPTION:
 * Retrieves the index of the item at coordinate (0, 0) of the client area.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 *
 * RETURN:
 * item index
 */
static INT LISTVIEW_GetFirstVisibleItem(HWND hwnd)
{
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nItemCountPerRow;
  INT nItemCountPerColumn;
  INT nMinRange;
  INT nMaxRange;
  INT nHScrollPos;
/*  INT32 nVScrollPos; */
  INT nItem = 0;

  /* get number of items per column */
  nItemCountPerColumn = LISTVIEW_GetItemCountPerColumn(hwnd);

  /* get number of fully visble items per row */ 
  nItemCountPerRow = LISTVIEW_GetItemCountPerRow(hwnd);

  if ((nItemCountPerRow > 0) && (nItemCountPerColumn > 0))
  {
    if (lStyle & LVS_LIST)
    {
      if (lStyle & WS_HSCROLL)
      {
        GetScrollRange(hwnd, SB_HORZ, &nMinRange, &nMaxRange);
        nHScrollPos = GetScrollPos(hwnd, SB_HORZ);
        if (nMinRange < nHScrollPos)
        {
          nItem = ((nHScrollPos - nMinRange) * nItemCountPerColumn * 
                   nItemCountPerRow);
        }
      }
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
  }

  return nItem;
}

/***
 * DESCRIPTION:
 * Retrieves the number of items per row. In other words, the number 
 * visible display columns.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 *
 * RETURN:
 * Number of items per row.
 */
static INT LISTVIEW_GetItemCountPerRow(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nItemCountPerRow = 0;

  if (infoPtr->nWidth > 0)
  {
    if (lStyle & LVS_LIST)
    {
      if (infoPtr->nColumnWidth > 0)
      {
        nItemCountPerRow = infoPtr->nWidth / infoPtr->nColumnWidth;
        if (nItemCountPerRow == 0)
          nItemCountPerRow = 1;
      }
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
  }

  return nItemCountPerRow; 
}

/***
 * DESCRIPTION:
 * Retrieves the number of items that can be displayed vertically in   
 * the listview.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 *
 * RETURN:
 * Number of items per column.
 */
static INT LISTVIEW_GetItemCountPerColumn(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nItemCountPerColumn = 0;
  
  if (infoPtr->nHeight > 0)
  {
    if (lStyle & LVS_LIST)
    {
      if (infoPtr->nItemHeight > 0)
        nItemCountPerColumn = infoPtr->nHeight / infoPtr->nItemHeight;
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
  }

  return nItemCountPerColumn;
}

/***
 * DESCRIPTION:
 * Retrieves the number of columns needed to display  
 * all the items in listview.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 *
 * RETURN:
 * Number of columns.
 */
static INT LISTVIEW_GetColumnCount(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle  = GetWindowLongA(hwnd, GWL_STYLE);
  INT nColumnCount = 0;
  INT nItemCountPerColumn;

  nItemCountPerColumn = LISTVIEW_GetItemCountPerColumn(hwnd);

 if ((infoPtr->nItemCount > 0) && (nItemCountPerColumn > 0))
 {
   if (lStyle & LVS_LIST)
   {
     if ((infoPtr->nItemCount % nItemCountPerColumn) == 0)
     {
       nColumnCount = infoPtr->nItemCount / nItemCountPerColumn ;
     }
     else
     {
       nColumnCount = infoPtr->nItemCount / nItemCountPerColumn + 1;
     }
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
 }

  return nColumnCount;
}

/***
 * DESCRIPTION:
 * Sets the scrolling parameters.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_SetScroll(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nColumnCount;
  INT nItemCountPerRow;
  INT nItemCountPerColumn;
  INT nMinRange, nMaxRange;
  INT nHScrollPos;
/*   INT32 nVScrollPos; */

  nItemCountPerColumn = LISTVIEW_GetItemCountPerColumn(hwnd);
  nItemCountPerRow = LISTVIEW_GetItemCountPerRow(hwnd);

  if ((nItemCountPerColumn > 0) && (nItemCountPerRow > 0))
  {
    if (lStyle & LVS_LIST)
    {
      /* get number of columns needed to display all the listview items */
      nColumnCount = LISTVIEW_GetColumnCount(hwnd);      
      if (nColumnCount > 0) 
      {
        if (nColumnCount > nItemCountPerRow)
        {
          /* add scrollbar if not already present */
          if (!(lStyle & WS_HSCROLL))
          {
            /* display scrollbar */
            ShowScrollBar(hwnd, SB_HORZ, TRUE);
        
            /* set scrollbar range and position */
            nMaxRange = nColumnCount - nItemCountPerRow;
            SetScrollRange(hwnd, SB_HORZ, 0, nMaxRange, FALSE);
            SetScrollPos(hwnd, SB_HORZ, 0, TRUE);
          }
          else
          {
            nHScrollPos = GetScrollPos(hwnd, SB_HORZ);
            GetScrollRange(hwnd, SB_HORZ, &nMinRange, &nMaxRange);
            if (nMaxRange != nColumnCount - nItemCountPerRow)
            {
              nMaxRange = nColumnCount - nItemCountPerRow;
              SetScrollRange(hwnd, SB_HORZ, nMinRange, nMaxRange, FALSE);

              if (nHScrollPos > nMaxRange)
                nHScrollPos = nMaxRange;
        
              SetScrollPos(hwnd, SB_HORZ, nHScrollPos, TRUE);
            }
          }
          
          /* refresh the client area */
          InvalidateRect(hwnd,NULL, TRUE);
          UpdateWindow(hwnd);
        }
        else
        {
          /* remove scrollbar if present */
          if (lStyle & WS_HSCROLL)
          {
            /* hide scrollbar */
            ShowScrollBar(hwnd, SB_HORZ, FALSE);

            /* refresh the client area */
            InvalidateRect(hwnd,NULL, TRUE);
            UpdateWindow(hwnd);
          }
        }
      }
    }
    else if (lStyle & LVS_REPORT) 
    {
      HDLAYOUT hl;
      WINDOWPOS wp;
      RECT rc;
        
      rc.top = 0;
      rc.left = 0;
      rc.right = infoPtr->nWidth;
      rc.bottom = infoPtr->nHeight;

      hl.prc = &rc;
      hl.pwpos = &wp;
      SendMessageA(infoPtr->hwndHeader, HDM_LAYOUT, 0, (LPARAM)&hl);
      
      SetWindowPos(infoPtr->hwndHeader, hwnd,
                     wp.x, wp.y, wp.cx, wp.cy, wp.flags);
      
/*       infoPtr->rcList.top += wp.cy; */
    }
    else if (lStyle & LVS_SMALLICON) 
    {
      /* TO DO */
    }
    else if (lStyle & LVS_ICON) 
    {
      /* TO DO */
    }
  }
}

/***
 * DESCRIPTION:
 * Modifies the state (selected and focused) of the listview items.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : focused item index
 * [I] BOOL32 : flag for determining weither the control keys are used or not
 * [I] BOOL32 : flag for determining the input type (mouse or keyboard)
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_SetItemStates(HWND hwnd, INT nFocusedItem, 
                                   BOOL bVirtualKeys, BOOL bMouseInput)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  WORD wShift = HIWORD(GetKeyState(VK_SHIFT));
  WORD wCtrl = HIWORD(GetKeyState(VK_CONTROL));
  INT i;
  LVITEMA lvItem;

  /* initialize memory */
  ZeroMemory(&lvItem, sizeof(LVITEMA));
    
  if (wShift && (bVirtualKeys == TRUE))
  {
    /* reset the selected and focused states of all the items */
    lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    lvItem.state = 0;
    SendMessageA(hwnd, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&lvItem);

    if (infoPtr->nSelectionMark > nFocusedItem)
    {
      for (i = infoPtr->nSelectionMark; i > nFocusedItem; i--)
      {
        /* select items */
        lvItem.stateMask = LVIS_SELECTED;
        lvItem.state = LVIS_SELECTED;
        SendMessageA(hwnd, LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)&lvItem);
      }
    }
    else
    {
      for (i = infoPtr->nSelectionMark; i < nFocusedItem; i++)
      {
        /* select items */
        lvItem.stateMask = LVIS_SELECTED;
        lvItem.state = LVIS_SELECTED;
        SendMessageA(hwnd, LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)&lvItem);
      }
    }

    /* select anf focus item */
    lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    lvItem.state = LVIS_SELECTED | LVIS_FOCUSED;
    SendMessageA(hwnd, LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)&lvItem);
  }
  else
  {
    if (wCtrl && (bVirtualKeys == TRUE))
    {
      /* make sure the focus is lost */
      lvItem.stateMask = LVIS_FOCUSED;
      lvItem.state = 0;
      SendMessageA(hwnd, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&lvItem);

      if (bMouseInput == TRUE)
      {
        if (SendMessageA(hwnd, LVM_GETITEMSTATE, (WPARAM)nFocusedItem, 
                           (LPARAM)LVIS_SELECTED) & LVIS_SELECTED)
        {
          /* focus and unselect (toggle selection) */
          lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
          lvItem.state = LVIS_FOCUSED;
          SendMessageA(hwnd, LVM_SETITEMSTATE, (WPARAM)nFocusedItem, 
                         (LPARAM)&lvItem);
        }
        else
        {
          /* select and focus */
          lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
          lvItem.state = LVIS_SELECTED | LVIS_FOCUSED;
          SendMessageA(hwnd, LVM_SETITEMSTATE, (WPARAM)nFocusedItem, 
                         (LPARAM)&lvItem);
        }
        
        /* set the group selection start position */
        infoPtr->nSelectionMark = nFocusedItem;
      }
      else
      {
        /* focus */
        lvItem.stateMask = LVIS_FOCUSED;
        lvItem.state = LVIS_FOCUSED;
        SendMessageA(hwnd, LVM_SETITEMSTATE, (WPARAM)nFocusedItem, 
                       (LPARAM)&lvItem);
      }
    }
    else
    {
      /* clear selection and focus for all the items */
      lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
      lvItem.state = 0;
      SendMessageA(hwnd, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&lvItem);

      /* set select and focus for this particular item */
      lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
      lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
      SendMessageA(hwnd, LVM_SETITEMSTATE, (WPARAM)nFocusedItem, 
                     (LPARAM)&lvItem);

      /* set the new group selection start position */
      infoPtr->nSelectionMark = nFocusedItem;
    }
  }
}

/***
 * DESCRIPTION:
 * Draws listview items when in list display maode.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] HDC32 : device context handle 
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_RefreshList(HWND hwnd, HDC hdc)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
    LISTVIEW_ITEM *lpItem;
  HFONT hOldFont;
  HPEN hPen, hOldPen;
  INT nItemCountPerColumn;
  INT nItemCountPerRow;
  INT nSmallIconWidth;
  SIZE labelSize;
  INT nDrawPosX = 0;
  INT nDrawPosY = 0;
  BOOL bSelected;
  RECT rcBoundingBox;
  COLORREF clrHighlight;
  COLORREF clrHighlightText;
  INT i;
  INT nColumn = 0;
  INT nRow;
  INT j;

  /* get number of items per column */
  nItemCountPerColumn = LISTVIEW_GetItemCountPerColumn(hwnd);

  /* get number of items per row */
  nItemCountPerRow = LISTVIEW_GetItemCountPerColumn(hwnd);
  
  if ((nItemCountPerColumn > 0) && (nItemCountPerRow > 0))
  {
     /* select font */
    hOldFont = SelectObject(hdc, infoPtr->hFont);

    /* inititialize system dependent information */
    clrHighlight = GetSysColor(COLOR_HIGHLIGHT);
    clrHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
    nSmallIconWidth = GetSystemMetrics(SM_CXSMICON);  

    /* select transparent brush (for drawing the focus box) */
    SelectObject(hdc, GetStockObject(NULL_BRUSH)); 

    /* select the doted pen (for drawing the focus box) */
    hPen = CreatePen(PS_DOT, 1, 0);
    hOldPen = SelectObject(hdc, hPen);

    /* get starting index */
    i = LISTVIEW_GetFirstVisibleItem(hwnd);
    
    /* DRAW ITEMS */
    for (j = 0; i < infoPtr->nItemCount; i++, j++)
    {
      /* set draw position for current item */
      nRow = j % nItemCountPerColumn;
      nColumn = j / nItemCountPerColumn;
      if (nRow == 0)
        nDrawPosY = 0;
      else
        nDrawPosY += infoPtr->nItemHeight;

      nDrawPosX = nColumn * infoPtr->nColumnWidth;
    
      /* get item */
      lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(infoPtr->hdpaItems, i);
      if (lpItem != NULL)
      {
        if (lpItem->state & LVIS_SELECTED)
        {
          bSelected = TRUE;
        
          /* set item colors */ 
          SetBkColor(hdc, clrHighlight);
          SetTextColor(hdc, clrHighlightText);

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

        /* state images */
        if (infoPtr->himlState != NULL)
        {
          /* right shift 12 bits to obtain index in image list */
          if (bSelected == TRUE)
            ImageList_Draw(infoPtr->himlState, lpItem->state >> 12, hdc, 
                           nDrawPosX, nDrawPosY, ILD_SELECTED);
          else
            ImageList_Draw(infoPtr->himlState, lpItem->state >> 12, hdc, 
                           nDrawPosX, nDrawPosY, ILD_NORMAL);
        
          nDrawPosX += nSmallIconWidth; 
        }

        /* small icons */
        if (infoPtr->himlSmall != NULL)
        {
          if (bSelected == TRUE)
            ImageList_Draw(infoPtr->himlSmall, lpItem->iImage, hdc, nDrawPosX, 
                           nDrawPosY, ILD_SELECTED);
          else
            ImageList_Draw(infoPtr->himlSmall, lpItem->iImage, hdc, nDrawPosX, 
                           nDrawPosY, ILD_NORMAL);
        
          nDrawPosX += nSmallIconWidth; 
        }

        /* get string size (in pixels) */
        GetTextExtentPoint32A(hdc, lpItem->pszText, 
                              lstrlenA(lpItem->pszText), &labelSize);

        /* define a bounding box */
        rcBoundingBox.left = nDrawPosX;
        rcBoundingBox.top = nDrawPosY;

        /* 2 pixels for padding purposes */
        rcBoundingBox.right = nDrawPosX + labelSize.cx + 2;

        /* padding already included in infoPtr->nItemHeight */
        rcBoundingBox.bottom = nDrawPosY + infoPtr->nItemHeight;
      
        /* draw text */  
        ExtTextOutA(hdc, nDrawPosX + 1, nDrawPosY+ 1, 
                      ETO_OPAQUE|ETO_CLIPPED, &rcBoundingBox, 
                      lpItem->pszText, lstrlenA(lpItem->pszText), NULL);
      
        if (lpItem->state & LVIS_FOCUSED)
          Rectangle(hdc, rcBoundingBox.left, rcBoundingBox.top, 
                      rcBoundingBox.right, rcBoundingBox.bottom); 
      }
    }

    /* unselect objects */
    SelectObject(hdc, hOldFont);
    SelectObject(hdc, hOldPen);

    /* delete pen */
    DeleteObject(hPen);
  }
}

/***
 * DESCRIPTION:
 * Draws listview items when in report display mode.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] HDC32 : device context handle 
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_RefreshReport(HWND hwnd, HDC hdc)
{
  /* TO DO */
}

/***
 * DESCRIPTION:
 * Draws listview items when in small icon display mode.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] HDC32 : device context handle 
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_RefreshSmallIcon(HWND hwnd, HDC hdc)
{
  /* TO DO */
}

/***
 * DESCRIPTION:
 * Draws listview items when in icon display mode.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] HDC32 : device context handle 
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
 * [I] HWND32 : window handle
 * [I] HDC32 : device context handle 
 *
 * RETURN:
 * None
 */
static VOID LISTVIEW_Refresh(HWND hwnd, HDC hdc)
{
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  
  if (lStyle & LVS_LIST)
  {
    LISTVIEW_RefreshList(hwnd, hdc);
  }
  else if (lStyle & LVS_REPORT)
  {
    LISTVIEW_RefreshReport(hwnd, hdc);
  }
  else if (lStyle & LVS_SMALLICON)
  {
    LISTVIEW_RefreshSmallIcon(hwnd, hdc);
  }
  else if (lStyle & LVS_ICON)
  {
    LISTVIEW_RefreshIcon(hwnd, hdc);
  }
}


/***
 * DESCRIPTION:
 * Calculates the approximate width and height of a given number of items.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : number of items
 * [I] INT32 : width
 * [I] INT32 : height
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
    nItemCount = infoPtr->nItemCount;

  if (lStyle & LVS_LIST)
  {
    if (wHeight == 0xFFFF)
    {
      /* use current height */
      wHeight = infoPtr->nHeight;
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
 * [I] HWND32 : window handle
 * [I] INT32 : alignment code
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_Arrange(HWND hwnd, INT nAlignCode)
{
  /* LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLong32A(hwnd, 0); */
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  BOOL bResult = FALSE;

  if (lStyle & LVS_ICON)
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
 * Removes all listview items.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
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
  NMLISTVIEW nmlv;
  BOOL bNotify;
  INT i;
  BOOL bResult = TRUE;

  if (infoPtr->nItemCount > 0)
  {
    /* send LVN_DELETEALLITEMS notification */
    ZeroMemory (&nmlv, sizeof (NMLISTVIEW));
    nmlv.hdr.hwndFrom = hwnd;
    nmlv.hdr.idFrom = lCtrlId;
    nmlv.hdr.code     = LVN_DELETEALLITEMS;
    nmlv.iItem        = -1;
    bNotify = !(BOOL)SendMessageA(GetParent(hwnd), WM_NOTIFY, 
                                      (WPARAM)lCtrlId, (LPARAM)&nmlv);

    nmlv.hdr.code     = LVN_DELETEITEM;

    for (i = 0; i < infoPtr->nItemCount; i++) 
    {
      if (bNotify == TRUE)
      {
        /* send LVN_DELETEITEM notification */
        nmlv.iItem = i;
        SendMessageA(GetParent(hwnd), WM_NOTIFY, (WPARAM)lCtrlId, 
                       (LPARAM)&nmlv);
	}

      /* get item */
      lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(infoPtr->hdpaItems, i);
      if (lpItem != NULL) 
      {
        /* free item string */
        if ((lpItem->pszText != NULL) && 
            (lpItem->pszText != LPSTR_TEXTCALLBACKA))
		COMCTL32_Free (lpItem->pszText);

        /* free item */
	    COMCTL32_Free (lpItem);
	}
    }

    /* ???? needs follow up */ 
    DPA_DeleteAllPtrs (infoPtr->hdpaItems);
 
    /* reset item counter */
    infoPtr->nItemCount = 0;

    /* reset scrollbar */
    LISTVIEW_SetScroll(hwnd);
}

  return bResult;
}

/***
 * DESCRIPTION:
 * Removes a column from the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : column index
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_DeleteColumn(HWND hwnd, INT nColumn)
{
/*   LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLong32A(hwnd, 0); */

    /* FIXME ??? */
/* if (infoPtr->nItemCount > 0) */
/* return FALSE; */
/* if (!SendMessage32A(infoPtr->hwndHeader, HDM_DELETEITEM, wParam, 0)) */
/* return FALSE; */
/* infoPtr->nColumnCount--; */
/* FIXME (listview, "semi stub!\n"); */

    return TRUE;
}

/***
 * DESCRIPTION:
 * Removes an item from the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : item index  
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_DeleteItem(HWND hwnd, INT nItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
    LISTVIEW_ITEM *lpItem;
    NMLISTVIEW nmlv;
  BOOL bResult = FALSE;

  if ((nItem >= 0) && (nItem < infoPtr->nItemCount))
  {
    /* send LVN_DELETEITEM notification */
    ZeroMemory (&nmlv, sizeof (NMLISTVIEW));
    nmlv.hdr.hwndFrom = hwnd;
    nmlv.hdr.idFrom = lCtrlId;
    nmlv.hdr.code     = LVN_DELETEITEM;
    nmlv.iItem        = nItem;
    SendMessageA(GetParent(hwnd), WM_NOTIFY, (WPARAM)lCtrlId, 
                   (LPARAM)&nmlv);

    /* remove item */
    lpItem = (LISTVIEW_ITEM*)DPA_DeletePtr (infoPtr->hdpaItems, nItem);
    if (lpItem != NULL)
    {
      /* free item string */
      if ((lpItem->pszText != NULL) && 
          (lpItem->pszText != LPSTR_TEXTCALLBACKA))
	COMCTL32_Free (lpItem->pszText);

      /* free item */
    COMCTL32_Free (lpItem);
    }

    /* decrement item counter */
    infoPtr->nItemCount--;

    /* reset some of the draw data */
    LISTVIEW_SetScroll(hwnd);
   
    bResult = TRUE;
}

  return bResult;
}

/* << LISTVIEW_EditLabel >> */
/* << LISTVIEW_EnsureVisible >> */

/***
 * DESCRIPTION:
 * Searches for an item with specific characteristics.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : base item index
 * [I] LPLVFINDINFO : item information to look for
 * 
 * RETURN:
 *   SUCCESS : index of item
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_FindItem(HWND hwnd, INT nStart, 
                                 LPLVFINDINFO lpFindInfo)
{
/*   LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLong32A(hwnd, 0); */
/*   LISTVIEW_ITEM *lpItem; */
/*   INT32 nItem; */
/*   INT32 nEnd = infoPtr->nItemCount; */
/*   BOOL32 bWrap = FALSE; */
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
 * [I] HWND32 : window handle
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
 * [I] HWND32 : window handle
 * [O] LPLVMKBIMAGE : background image information
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
/* static LRESULT LISTVIEW_GetBkImage(HWND32 hwnd, LPLVBKIMAGE lpBkImage) */
/* { */
/*   return FALSE; */
/* } */

/* << LISTVIEW_GetCallbackMask >> */

/***
 * DESCRIPTION:
 * Retrieves column attributes.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 :  column index
 * [IO] LPLVCOLUMN32A : column information
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_GetColumnA(HWND hwnd, INT nIndex, 
                                     LPLVCOLUMNA lpColumn)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
    HDITEMA hdi;

  if (lpColumn == NULL)
	return FALSE;

    ZeroMemory (&hdi, sizeof(HDITEMA));

  if (lpColumn->mask & LVCF_FMT)
	hdi.mask |= HDI_FORMAT;

  if (lpColumn->mask & LVCF_WIDTH)
        hdi.mask |= HDI_WIDTH;

  if (lpColumn->mask & LVCF_TEXT)
        hdi.mask |= (HDI_TEXT | HDI_FORMAT);

  if (lpColumn->mask & LVCF_IMAGE)
        hdi.mask |= HDI_IMAGE;

  if (lpColumn->mask & LVCF_ORDER)
        hdi.mask |= HDI_ORDER;

    if (!SendMessageA (infoPtr->hwndHeader, HDM_GETITEMA,
                       (WPARAM)nIndex, (LPARAM)&hdi))
	return FALSE;

  if (lpColumn->mask & LVCF_FMT) 
  {
    lpColumn->fmt = 0;

	if (hdi.fmt & HDF_LEFT)
      lpColumn->fmt |= LVCFMT_LEFT;
	else if (hdi.fmt & HDF_RIGHT)
      lpColumn->fmt |= LVCFMT_RIGHT;
	else if (hdi.fmt & HDF_CENTER)
      lpColumn->fmt |= LVCFMT_CENTER;

	if (hdi.fmt & HDF_IMAGE)
      lpColumn->fmt |= LVCFMT_COL_HAS_IMAGES;
    }

  if (lpColumn->mask & LVCF_WIDTH)
    lpColumn->cx = hdi.cxy;

  if ((lpColumn->mask & LVCF_TEXT) && (lpColumn->pszText) && (hdi.pszText))
    lstrcpynA (lpColumn->pszText, hdi.pszText, lpColumn->cchTextMax);

  if (lpColumn->mask & LVCF_IMAGE)
    lpColumn->iImage = hdi.iImage;

  if (lpColumn->mask & LVCF_ORDER)
    lpColumn->iOrder = hdi.iOrder;

    return TRUE;
}

/* << LISTVIEW_GetColumn32W >> */
/* << LISTVIEW_GetColumnOrderArray >> */

/***
 * DESCRIPTION:
 * Retrieves the column width when list or report display mode.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] int32 : column index
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

  if ((lStyle & LVS_LIST) || (lStyle & LVS_SMALLICON) || (lStyle & LVS_ICON))
  {
    nColumnWidth = infoPtr->nColumnWidth;
  }
  else if (lStyle & LVS_REPORT)
  {
    /* verify validity of index */
    if ((nColumn >= 0) && (nColumn < infoPtr->nColumnCount))
    {
      /* get column width from header */
    hdi.mask = HDI_WIDTH;
    if (SendMessageA (infoPtr->hwndHeader, HDM_GETITEMA,
                         (WPARAM)nColumn, (LPARAM)&hdi))
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
 * [I] HWND32 : window handle
 *
 * RETURN:
 * Number of fully visible items.
 */
static LRESULT LISTVIEW_GetCountPerPage(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nItemCount = 0;
  INT nItemCountPerRow = LISTVIEW_GetItemCountPerRow(hwnd);
  INT nItemCountPerColumn = LISTVIEW_GetItemCountPerColumn(hwnd);

  if (lStyle & LVS_LIST)
  {
    if ((nItemCountPerRow > 0) && (nItemCountPerColumn > 0))
    {
      nItemCount = nItemCountPerRow * nItemCountPerColumn;
    }
  }
  else if (lStyle & LVS_REPORT)
  {
    nItemCount = nItemCountPerColumn;
  }
  else if ((lStyle & LVS_SMALLICON) || (lStyle & LVS_ICON))
  {
    nItemCount = infoPtr->nItemCount;
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
 * [I] HWND32 : window handle
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
 * [I] HWND32 : window handle
 * [I] INT32 : image list identifier 
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
	case LVSIL_SMALL:
    himl = infoPtr->himlSmall;
	case LVSIL_STATE:
    himl = infoPtr->himlState;
    }

  return (LRESULT)himl;
}


/* << LISTVIEW_GetISearchString >> */

/***
 * DESCRIPTION:
 * Retrieves item attributes.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [IO] LPLVITEM32A : item info
 * 
 * RETURN:
 *   SUCCESS : TRUE 
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_GetItemA(HWND hwnd, LPLVITEMA lpItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_ITEM *lpListItem;
  BOOL bResult = FALSE;

  if (lpItem != NULL)
  {
    if ((lpItem->iItem >= 0) && (lpItem->iItem < infoPtr->nItemCount))
    {
      /* get item */
      lpListItem = DPA_GetPtr(infoPtr->hdpaItems, lpItem->iItem);
      if (lpListItem != NULL)
      {
        /* not tested for subitem > 0 */
        lpListItem += lpItem->iSubItem;
        if (lpListItem != NULL) 
        {
          /* retrieve valid data */
    if (lpItem->mask & LVIF_STATE)
            lpItem->state = lpListItem->state & lpItem->stateMask;

          if (lpItem->mask & LVIF_TEXT) 
          {
            if (lpListItem->pszText == LPSTR_TEXTCALLBACKA)
	    lpItem->pszText = LPSTR_TEXTCALLBACKA;
	else
              Str_GetPtrA(lpListItem->pszText, lpItem->pszText, 
			   lpItem->cchTextMax);
    }

    if (lpItem->mask & LVIF_IMAGE)
            lpItem->iImage = lpListItem->iImage;

    if (lpItem->mask & LVIF_PARAM)
            lpItem->lParam = lpListItem->lParam;

    if (lpItem->mask & LVIF_INDENT)
            lpItem->iIndent = lpListItem->iIndent;

          bResult = TRUE;
}
      }
    }
  }

  return bResult;
}

/* << LISTVIEW_GetItem32W >> */
/* << LISTVIEW_GetHotCursor >> */
/* << LISTVIEW_GetHotItem >> */
/* << LISTVIEW_GetHoverTime >> */

/***
 * DESCRIPTION:
 * Retrieves the number of items in the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * 
 * RETURN:
 * Number of items.
 */
static LRESULT LISTVIEW_GetItemCount(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLongA(hwnd, 0);

    return infoPtr->nItemCount;
}

/***
 * DESCRIPTION:
 * Retrieves the position (upper-left) of the listview control item.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : item index
 * [O] LPPOINT32 : coordinate information
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_GetItemPosition(HWND hwnd, INT nItem, 
                                        LPPOINT lpPt)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nColumn;
  INT nRow;
  BOOL bResult = FALSE;
  INT nItemCountPerColumn;
  INT nItemCountPerRow;
  INT nFirstItem;
  INT nLastItem;

  if ((nItem >= 0) && (nItem < infoPtr->nItemCount) && (lpPt != NULL))
  {
    if (lStyle & LVS_LIST)
{
      nItemCountPerColumn = LISTVIEW_GetItemCountPerColumn(hwnd);
      nItemCountPerRow = LISTVIEW_GetItemCountPerRow(hwnd);

      if ((nItemCountPerColumn > 0) && (nItemCountPerRow > 0))
      {
        nFirstItem = LISTVIEW_GetFirstVisibleItem(hwnd); 
        nLastItem = nFirstItem + nItemCountPerRow * nItemCountPerRow - 1;

        if ((nItem >= nFirstItem) || (nItem <= nLastItem))
        {
          nItem -= nFirstItem; 

          /* get column */
          nColumn = nItem / nItemCountPerColumn;

          /* get row */
          nRow = nItem % nItemCountPerColumn;

          /* X coordinate of the column */
          lpPt->x = nColumn * infoPtr->nColumnWidth;

          /* Y coordinate of the item */
          lpPt->y = nRow * infoPtr->nItemHeight;

          bResult = TRUE;
        }
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
    }
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Retrieves the bounding rectangle for a listview control item.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : item index
 * [IO] LPRECT32 : bounding rectangle coordinates
 * 
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_GetItemRect(HWND hwnd, INT nItem, LPRECT lpRect)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_ITEM *lpItem;
  POINT pt;
  INT nLabelWidth;
  BOOL bResult = FALSE;
  INT nSmallIconWidth;

  nSmallIconWidth = GetSystemMetrics(SM_CXSMICON);

  if ((nItem < 0) || (nItem >= infoPtr->nItemCount) || (lpRect == NULL))
    return FALSE;

  /* get item */
  lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(infoPtr->hdpaItems, nItem);
  if (lpItem == NULL) 
    return FALSE;

  if (!SendMessageA(hwnd, LVM_GETITEMPOSITION, (WPARAM)nItem, (LPARAM)&pt))
    return FALSE;

  nLabelWidth = SendMessageA(hwnd, LVM_GETSTRINGWIDTHA, (WPARAM)0, 
                               (LPARAM)lpItem->pszText);
  switch(lpRect->left) 
  { 
  case LVIR_BOUNDS: 
    lpRect->left = pt.x;
    lpRect->top = pt.y;
    lpRect->right = lpRect->left + nSmallIconWidth + nLabelWidth;
    lpRect->bottom = lpRect->top + infoPtr->nItemHeight;
    bResult = TRUE;
    break;
  case LVIR_ICON:
    lpRect->left = pt.x;
    lpRect->top = pt.y;
    lpRect->right = lpRect->left + nSmallIconWidth;
    lpRect->bottom = lpRect->top + infoPtr->nItemHeight;
    bResult = TRUE;
    break;
  case LVIR_LABEL: 
    lpRect->left = pt.x + nSmallIconWidth;
    lpRect->top = pt.y;
    lpRect->right = lpRect->left + nLabelWidth;
    lpRect->bottom = infoPtr->nItemHeight;
    bResult = TRUE;
    break;
  case LVIR_SELECTBOUNDS:
    /* TO DO */
    break;
  }
        
  return bResult;
}

/***
 * DESCRIPTION:
 * Retrieves the spacing between listview control items.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] BOOL32 : small icon (TRUE) or large icon (FALSE)
 *
 * RETURN:
 * Horizontal + vertical spacing
 */
static LRESULT LISTVIEW_GetItemSpacing(HWND hwnd, BOOL bSmall)
{
  INT nSmallIconHSpacing;
  INT nSmallIconVSpacing;
  INT nIconHSpacing;
  INT nIconVSpacing;
  LONG lResult = 0;

  if (bSmall == TRUE)
  {
    /* ??? */
    nSmallIconHSpacing = 0;
    nSmallIconVSpacing = 0;
    lResult = MAKELONG(nSmallIconHSpacing, nSmallIconVSpacing);
    }
    else
  {
    nIconHSpacing = GetSystemMetrics(SM_CXICONSPACING);
    nIconVSpacing = GetSystemMetrics(SM_CYICONSPACING);
    lResult = MAKELONG(nIconHSpacing, nIconVSpacing);
  }
  
  return lResult;
}

/***
 * DESCRIPTION:
 * Retrieves the state of a listview control item.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : item index
 * [I] UINT32 : state mask
 * 
 * RETURN:
 * State specified by the mask.
 */
static LRESULT LISTVIEW_GetItemState(HWND hwnd, INT nItem, UINT uMask)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_ITEM *lpItem;
  INT nState = 0;

  if ((nItem >= 0) && (nItem < infoPtr->nItemCount))
  {
    lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(infoPtr->hdpaItems, nItem);
    if (lpItem != NULL) 
      nState =   lpItem->state & uMask;
  }

  return nState;
}

/***
 * DESCRIPTION:
 * Retrieves the text of a listview control item or subitem. 
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : item index
 * [IO] LPLVITEM32A : item information
 * 
 * RETURN:
 * None
 */
static VOID LISTVIEW_GetItemTextA(HWND hwnd, INT nItem, 
                                    LPLVITEMA lpItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_ITEM *lpListItem;

  if ((nItem < 0) || (nItem >= infoPtr->nItemCount) || (lpItem == NULL))
    return;

  lpListItem = (LISTVIEW_ITEM *)DPA_GetPtr(infoPtr->hdpaItems, nItem);
  if (lpListItem == NULL)
    return;

  lpListItem += lpItem->iSubItem;
  if (lpListItem == NULL)
    return;

  if (lpListItem->pszText == LPSTR_TEXTCALLBACKA) 
    lpItem->pszText = LPSTR_TEXTCALLBACKA;
    else
    Str_GetPtrA(lpListItem->pszText, lpItem->pszText, lpItem->cchTextMax);
}

/***
 * DESCRIPTION:
 * Searches for an item based on properties + relationships.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : item index
 * [I] UINT32 : relationship flag
 * 
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_GetNextItem (HWND hwnd, INT nItem, UINT uFlags)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nResult = -1;

  /* start at begin */
  if (nItem == -1)
    nItem = 0;

  if ((nItem >= 0) && (nItem < infoPtr->nItemCount))
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
 * [I] HWND32 : window handle
 * [O] LPPOINT32 : coordinate information
 * 
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_GetOrigin(HWND hwnd, LPPOINT lpOrigin)
{
  LONG lStyle = GetWindowLongA(hwnd, GWL_ID);
  BOOL bResult = FALSE;

  if (lStyle & LVS_SMALLICON) 
  {
    /* TO DO */
  }
  else if (lStyle & LVS_ICON)
  {
    /* TO DO */
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Retrieves the number of items that are marked as selected.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * 
 * RETURN:
 * Number of items selected.
 */
static LRESULT LISTVIEW_GetSelectedCount(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_ITEM *lpItem;
  INT nSelectedCount = 0;
  INT i;

  for (i = 0; i < infoPtr->nItemCount; i++)
  {
    lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(infoPtr->hdpaItems, i);
    if (lpItem != NULL)
    {
      if (lpItem->state & LVIS_SELECTED)
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
 * [I] HWND32 : window handle
 * 
 * RETURN:
 * Index number or -1 if there is no selection mark.
 */
static LRESULT LISTVIEW_GetSelectionMark(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  INT nResult = -1;

  if (SendMessageA(hwnd, LVM_GETSELECTEDCOUNT, (WPARAM)0, (LPARAM)0) != 0)
    nResult = infoPtr->nSelectionMark;

  return nResult;
}

/***
 * DESCRIPTION:
 * Retrieves the width of a string.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
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
  INT nResult = 0;

  if (lpsz != NULL)
  {
    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);
    hdc = GetDC(hwnd);
    hOldFont = SelectObject (hdc, hFont);
    GetTextExtentPoint32A(hdc, lpsz, lstrlenA(lpsz), &textSize);
    SelectObject (hdc, hOldFont);
    ReleaseDC(hwnd, hdc);
    nResult = textSize.cx;
  }

  return nResult;
}

/***
 * DESCRIPTION:
 * Retrieves the text backgound color.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
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
 * [I] HWND32 : window handle
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
 * [I] HWND32 : window handle
 * [O] LPRECT32 : bounding rectangle
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_GetViewRect(HWND hwnd, LPRECT lpBoundingRect)
{
  /* LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLong32A(hwnd, 0); */
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  BOOL bResult = FALSE;

  if (lpBoundingRect != NULL)
  {
    if ((lStyle & LVS_ICON) || (lStyle & LVS_SMALLICON))
    {
      /* TO DO */
    }
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Determines item index when in small icon view.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [IO] LPLVHITTESTINFO : hit test information
 *
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static LRESULT HitTestSmallIconView(HWND hwnd, LPLVHITTESTINFO lpHitTestInfo)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_ITEM *lpItem;
  INT nColumn;
  INT nRow;
  INT nItem = 0;
  INT nLabelWidth;
  INT nOffset;
  INT nPosX = 0;
  INT nSmallIconWidth;
  INT nItemCountPerRow;

  /* get column */
  nColumn = lpHitTestInfo->pt.x / infoPtr->nColumnWidth;

  /* get row */
  nRow = lpHitTestInfo->pt.y / infoPtr->nItemHeight;
  
  /* calculate offset from start of column (in pixels) */
  nOffset = lpHitTestInfo->pt.x % infoPtr->nColumnWidth;

  /* get recommended width of a small icon */
  nSmallIconWidth = GetSystemMetrics(SM_CXSMICON);

  /* calculate index */
  nItemCountPerRow = LISTVIEW_GetItemCountPerRow(hwnd);
  nItem = nRow * nItemCountPerRow + LISTVIEW_GetFirstVisibleItem(hwnd) + nColumn;

  /* verify existance of item */
  if (nItem < infoPtr->nItemCount)
  {
    /* get item */
    lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(infoPtr->hdpaItems, nItem);
    if (lpItem != NULL)
    {
      if (infoPtr->himlState != NULL)
      {
        nPosX += nSmallIconWidth;
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
        nPosX += nSmallIconWidth;
        if (nOffset <= nPosX)
        {
          lpHitTestInfo->flags = LVHT_ONITEMICON | LVHT_ONITEM;
          lpHitTestInfo->iItem = nItem;
          lpHitTestInfo->iSubItem = 0;
          return nItem;
}
      }

      /* get width of label in pixels */
      nLabelWidth = SendMessageA(hwnd, LVM_GETSTRINGWIDTHA, (WPARAM)0, 
                                   (LPARAM)lpItem->pszText); 
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

  /* hit is not on an item */
  lpHitTestInfo->flags = LVHT_NOWHERE; 
     
  return -1;
}

/***
 * DESCRIPTION:
 * Determines item index when in list display mode.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [IO] LPLVHITTESTINFO : hit test information
 *
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static LRESULT HitTestListView(HWND hwnd, LPLVHITTESTINFO lpHitTestInfo)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_ITEM *lpItem;
  INT nColumn;
  INT nRow;
  INT nItem = 0;
  INT nLabelWidth;
  INT nOffset;
  INT nPosX = 0;
  INT nSmallIconWidth;
  INT nItemCountPerColumn;

  /* get column */
  nColumn = lpHitTestInfo->pt.x / infoPtr->nColumnWidth;

  /* get row */ 
  nRow = lpHitTestInfo->pt.y / infoPtr->nItemHeight;
  
  /* calculate offset from start of column (in pixels) */
  nOffset = lpHitTestInfo->pt.x % infoPtr->nColumnWidth;

  /* get recommended width of a small icon */
  nSmallIconWidth = GetSystemMetrics(SM_CXSMICON);

  /* calculate index */
  nItemCountPerColumn = LISTVIEW_GetItemCountPerColumn(hwnd);
  nItem = (nColumn * nItemCountPerColumn + LISTVIEW_GetFirstVisibleItem(hwnd) 
           + nRow);

  /* verify existance of item */
  if (nItem < infoPtr->nItemCount)
  {
    /* get item */
    lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(infoPtr->hdpaItems, nItem);
    if (lpItem != NULL)
    {
      if (infoPtr->himlState != NULL)
      {
        nPosX += nSmallIconWidth;
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
        nPosX += nSmallIconWidth;
        if (nOffset <= nPosX)
{
          lpHitTestInfo->flags = LVHT_ONITEMICON | LVHT_ONITEM;
          lpHitTestInfo->iItem = nItem;
          lpHitTestInfo->iSubItem = 0;
          return nItem;
        }
      }

      /* get width of label in pixels */
      nLabelWidth = SendMessageA(hwnd, LVM_GETSTRINGWIDTHA, (WPARAM)0, 
                                   (LPARAM)lpItem->pszText); 
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

  /* hit is not on an item */
  lpHitTestInfo->flags = LVHT_NOWHERE; 
     
    return -1;
}

/***
 * DESCRIPTION:
 * Determines wich listview item is located at the specified position.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [IO} LPLVHITTESTINFO : hit test information
 *
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_HitTest(HWND hwnd, LPLVHITTESTINFO lpHitTestInfo)
{
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);

  if (lStyle & LVS_LIST)
    return HitTestListView(hwnd, lpHitTestInfo);
  else if (lStyle & LVS_REPORT)
  {
    /* TO DO */
  }
  else if (lStyle & LVS_SMALLICON)
  {
    return HitTestSmallIconView(hwnd, lpHitTestInfo);
  }
  else if (lStyle & LVS_ICON)
  {
    /* TO DO */
  }

  return -1;
}

/***
 * DESCRIPTION:
 * Inserts a new column.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : column index
 * [I] LPLVCOLUMN32A : column information
 *
 * RETURN:
 *   SUCCESS : new column index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_InsertColumnA(HWND hwnd, INT nIndex, 
                                        LPLVCOLUMNA lpColumn)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLongA(hwnd, 0);
    HDITEMA hdi;
    INT nResult;

  if ((lpColumn == NULL) || (infoPtr->nItemCount > 0))
	return -1;

  FIXME (listview, "(%d %p): semi stub!\n", nIndex, lpColumn);

  /* initialize memory */
    ZeroMemory (&hdi, sizeof(HDITEMA));

  if (lpColumn->mask & LVCF_FMT) 
  {
	if (nIndex == 0)
	    hdi.fmt |= HDF_LEFT;
    else if (lpColumn->fmt & LVCFMT_LEFT)
	    hdi.fmt |= HDF_LEFT;
    else if (lpColumn->fmt & LVCFMT_RIGHT)
	    hdi.fmt |= HDF_RIGHT;
    else if (lpColumn->fmt & LVCFMT_CENTER)
	    hdi.fmt |= HDF_CENTER;
    if (lpColumn->fmt & LVCFMT_COL_HAS_IMAGES)
	    hdi.fmt |= HDF_IMAGE;
	    
	hdi.mask |= HDI_FORMAT;
    }

  if (lpColumn->mask & LVCF_WIDTH) 
  {
        hdi.mask |= HDI_WIDTH;
    hdi.cxy = lpColumn->cx;
    }
    
  if (lpColumn->mask & LVCF_TEXT) 
  {
        hdi.mask |= (HDI_TEXT | HDI_FORMAT);
    hdi.pszText = lpColumn->pszText;
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

    nResult = SendMessageA (infoPtr->hwndHeader, HDM_INSERTITEMA,
                           (WPARAM)nIndex, (LPARAM)&hdi);
    if (nResult == -1)
	return -1;

  /* increment column counter */
    infoPtr->nColumnCount++;

    return nResult;
}

/* << LISTVIEW_InsertColumn32W >> */

/***
 * DESCRIPTION:
 * Inserts a new item in the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] LPLVITEM32A : item information
 *
 * RETURN:
 *   SUCCESS : new item index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_InsertItemA(HWND hwnd, LPLVITEMA lpItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
    LISTVIEW_ITEM *lpListItem;
  INT nItem;
    NMLISTVIEW nmlv;
  LVITEMA lvItem;
  INT nColumnWidth = 0;
  INT nSmallIconWidth;

  if (lpItem == NULL)
	return -1;

  if (lpItem->iSubItem != 0)
	return -1;

  /* allocate memory */
  lpListItem = (LISTVIEW_ITEM *)COMCTL32_Alloc(sizeof(LISTVIEW_ITEM));
  if (lpListItem == NULL)
	return -1;

  nSmallIconWidth = GetSystemMetrics(SM_CXSMICON);

  /* initialize listview control item */
  ZeroMemory(lpListItem, sizeof(LISTVIEW_ITEM));

  /* copy only valid data */
  if (lpItem->mask & LVIF_TEXT) 
  {
	if (lpItem->pszText == LPSTR_TEXTCALLBACKA)
    {
      if ((lStyle & LVS_SORTASCENDING) || (lStyle & LVS_SORTDESCENDING))
        return -1;

      lpListItem->pszText = LPSTR_TEXTCALLBACKA;
    }
	else
    {
      nColumnWidth = SendMessageA(hwnd, LVM_GETSTRINGWIDTHA, 
                                    (WPARAM)0, (LPARAM)lpItem->pszText);

      /* add padding for separating columns */
      nColumnWidth += 10;

      /* calculate column width; make sure it's at least 96 pixels */
      if (nColumnWidth < 96)
        nColumnWidth = 96;
        
      Str_SetPtrA(&lpListItem->pszText, lpItem->pszText);
    }
  }

  if (lpItem->mask & LVIF_STATE)
  {
    if (lpItem->stateMask & LVIS_FOCUSED)
    {
      /* clear LVIS_FOCUSED states */
      ZeroMemory(&lvItem, sizeof(LVITEMA));
      lvItem.stateMask = LVIS_FOCUSED;
      lvItem.state = 0;
      SendMessageA(hwnd, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&lvItem);

      /* set focused item index */
      infoPtr->nFocusedItem = lpItem->iItem;
    }

    lpListItem->state = lpItem->state & lpItem->stateMask;
  }

    if (lpItem->mask & LVIF_IMAGE)
    lpListItem->iImage = lpItem->iImage;

    if (lpItem->mask & LVIF_PARAM)
    lpListItem->lParam = lpItem->lParam;

    if (lpItem->mask & LVIF_INDENT)
    lpListItem->iIndent = lpItem->iIndent;

  /* insert item in listview control data structure */
  nItem = DPA_InsertPtr(infoPtr->hdpaItems, lpItem->iItem, lpListItem);
  if (nItem == -1)
    return -1;

  /* increment item counter */
    infoPtr->nItemCount++;

  /* calculate column width */
  if (infoPtr->himlSmall != NULL)
    nColumnWidth += nSmallIconWidth;

  /* calculate column width */
  if (infoPtr->himlState != NULL)
    nColumnWidth += nSmallIconWidth;

  /* set column width */
   if (nColumnWidth > infoPtr->nColumnWidth)
     infoPtr->nColumnWidth = nColumnWidth;

  /* set drawing data */
  LISTVIEW_SetScroll(hwnd);

  /* send LVN_INSERTITEM notification */
    ZeroMemory (&nmlv, sizeof (NMLISTVIEW));
  nmlv.hdr.hwndFrom = hwnd;
  nmlv.hdr.idFrom = lCtrlId;
    nmlv.hdr.code     = LVN_INSERTITEM;
  nmlv.iItem = nItem;
  SendMessageA(GetParent (hwnd), WM_NOTIFY, (WPARAM)lCtrlId, 
                 (LPARAM)&nmlv);

  InvalidateRect(hwnd, NULL, FALSE);
  UpdateWindow(hwnd);

  return nItem;
}

/* << LISTVIEW_InsertItem32W >> */

/***
 * DESCRIPTION:
 * Redraws a range of items.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : first item
 * [I] INT32 : last item
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_RedrawItems(HWND hwnd, INT nFirst, INT nLast)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0); 

  if (nFirst > nLast)
    return FALSE;

  if ((nFirst < 0) || (nFirst >= infoPtr->nItemCount))
    return FALSE;

  if ((nLast < 0) || (nLast >= infoPtr->nItemCount))
    return FALSE;

  /* TO DO */

  return TRUE;
}

/***
 * DESCRIPTION:
 * Scrolls the content of a listview.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : amount of horizontal scrolling
 * [I] INT32 : amount of vertical scrolling
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_Scroll(HWND hwnd, INT nHScroll, INT nVScroll)
{
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  BOOL bResult = FALSE;
  INT nHScrollPos;
  INT nMinRange;
  INT nMaxRange;

  if (lStyle & LVS_LIST)
  {
    nHScrollPos = GetScrollPos(hwnd, SB_HORZ);
    GetScrollRange(hwnd, SB_HORZ, &nMinRange, &nMaxRange);

    if ((nMinRange <= nHScrollPos + nHScroll) && 
        (nHScrollPos + nHScroll <= nMaxRange))
    {
      bResult = TRUE;
      nHScrollPos += nHScroll;
      SetScrollPos(hwnd, SB_HORZ, nHScrollPos, TRUE);

      /* refresh client area */
      InvalidateRect(hwnd, NULL, TRUE);
      UpdateWindow(hwnd);
    }
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

  return bResult; 
}

/***
 * DESCRIPTION:
 * Sets the background color.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
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
 * Sets column attributes.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : column index
 * [I] LPLVCOLUMN32A : column information
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetColumnA(HWND hwnd, INT nIndex, 
                                     LPLVCOLUMNA lpColumn)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
    HDITEMA hdi;

  if (lpColumn == NULL)
	return -1;

  /* initialize memory */
    ZeroMemory (&hdi, sizeof(HDITEMA));

  if (lpColumn->mask & LVCF_FMT) 
  {
	if (nIndex == 0)
	    hdi.fmt |= HDF_LEFT;
    else if (lpColumn->fmt & LVCFMT_LEFT)
	    hdi.fmt |= HDF_LEFT;
    else if (lpColumn->fmt & LVCFMT_RIGHT)
	    hdi.fmt |= HDF_RIGHT;
    else if (lpColumn->fmt & LVCFMT_CENTER)
	    hdi.fmt |= HDF_CENTER;

    if (lpColumn->fmt & LVCFMT_COL_HAS_IMAGES)
	    hdi.fmt |= HDF_IMAGE;
	    
	hdi.mask |= HDI_FORMAT;
    }

  if (lpColumn->mask & LVCF_WIDTH) 
  {
        hdi.mask |= HDI_WIDTH;
    hdi.cxy = lpColumn->cx;
    }
    
  if (lpColumn->mask & LVCF_TEXT) 
  {
        hdi.mask |= (HDI_TEXT | HDI_FORMAT);
    hdi.pszText = lpColumn->pszText;
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

    return (LRESULT)SendMessageA (infoPtr->hwndHeader, HDM_SETITEMA,
                                  (WPARAM)nIndex, (LPARAM)&hdi);
}

/***
 * DESCRIPTION:
 * Sets image list.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : image list type  
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
 * [I] HWND32 : window handle
 * [I] LPLVITEM32 : item information 
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetItemA(HWND hwnd, LPLVITEMA lpItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  LISTVIEW_ITEM *lpListItem;
    NMLISTVIEW nmlv;

  if (lpItem == NULL)
	return FALSE;

    if ((lpItem->iItem < 0) || (lpItem->iItem >= infoPtr->nItemCount))
	return FALSE;

    /* send LVN_ITEMCHANGING notification */
    ZeroMemory (&nmlv, sizeof (NMLISTVIEW));
  nmlv.hdr.hwndFrom = hwnd;
  nmlv.hdr.idFrom = lCtrlId;
    nmlv.hdr.code     = LVN_ITEMCHANGING;
    nmlv.iItem        = lpItem->iItem;
    nmlv.iSubItem     = lpItem->iSubItem;
    nmlv.uChanged     = lpItem->mask;
  if (SendMessageA(GetParent(hwnd), WM_NOTIFY,  (WPARAM)lCtrlId, 
                     (LPARAM)&nmlv) == FALSE) 
	return FALSE;

  /* get item */
  lpListItem = DPA_GetPtr(infoPtr->hdpaItems, lpItem->iItem);
  if (lpListItem == NULL)
    return FALSE;

  /* copy valid data */
  if (lpItem->iSubItem > 0)
  {
    /* verify existance of sub-item */
    lpListItem += lpItem->iSubItem;
    if (lpListItem == NULL)
	return FALSE;

    /* exit if indent attribute is valid */
    if (lpItem->mask & LVIF_INDENT)
	return FALSE;

    if (lpItem->mask & LVIF_IMAGE)
    {
      /* set sub-item attribute */
    }

	}
  else
  {
    if (lpItem->mask & LVIF_STATE)
    {
      lpListItem->state &= ~lpItem->stateMask;
      lpListItem->state |= (lpItem->state & lpItem->stateMask);
    }

    if (lpItem->mask & LVIF_IMAGE)
      lpListItem->iImage = lpItem->iImage;

    if (lpItem->mask & LVIF_PARAM)
      lpListItem->lParam = lpItem->lParam;

    if (lpItem->mask & LVIF_INDENT)
      lpListItem->iIndent = lpItem->iIndent;
  }
  
  if (lpItem->mask & LVIF_TEXT) 
  {
    if (lpItem->pszText == LPSTR_TEXTCALLBACKA) 
    {
      if ((lpListItem->pszText != NULL) &&
          (lpListItem->pszText != LPSTR_TEXTCALLBACKA))
        COMCTL32_Free (lpListItem->pszText);

      lpListItem->pszText = LPSTR_TEXTCALLBACKA;
    }
    else 
    {
      if (lpListItem->pszText == LPSTR_TEXTCALLBACKA)
        lpListItem->pszText = NULL;
      
      if (Str_SetPtrA(&lpListItem->pszText, lpItem->pszText) == FALSE)
        return FALSE;
    }
  }

    /* send LVN_ITEMCHANGED notification */
    nmlv.hdr.code = LVN_ITEMCHANGED;
  SendMessageA(GetParent(hwnd), WM_NOTIFY, (WPARAM)lCtrlId, 
                 (LPARAM)&nmlv);

    return TRUE;
}

/* << LISTVIEW_SetItem32W >> */

/* LISTVIEW_SetItemCount*/

/***
 * DESCRIPTION:
 * Sets item position.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : item index
 * [I] INT32 : x coordinate
 * [I] INT32 : y coordinate
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

  if ((nItem < 0) || (nItem >= infoPtr->nItemCount))
	return FALSE;

  if ((lStyle & LVS_ICON) && (lStyle & LVS_SMALLICON))
  {
    /* get item */

    /* set position of item */

    /* refresh */
    if (lStyle & LVS_AUTOARRANGE)
    {
      InvalidateRect(hwnd, NULL, FALSE);
      UpdateWindow(hwnd);
    }

    return TRUE;
}

  return FALSE;
}

/***
 * DESCRIPTION:
 * Sets the state of one or many items.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I]INT32 : item index
 * [I] LPLVITEM32 : item information 
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
*/
static LRESULT LISTVIEW_SetItemState(HWND hwnd, INT nItem, 
                                     LPLVITEMA lpItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LISTVIEW_ITEM *lpListItem;
  INT i;

  if (nItem == -1)
  {
    /* apply to all the items */
    for (i = 0; i< infoPtr->nItemCount; i++)
    {
      lpListItem = (LISTVIEW_ITEM *)DPA_GetPtr(infoPtr->hdpaItems, i);
      if (lpListItem == NULL)
	return FALSE;

      lpListItem->state &= ~lpItem->stateMask;
      lpListItem->state |= (lpItem->state & lpItem->stateMask);
	}
	}
  else if ((nItem >= 0) && (nItem < infoPtr->nItemCount))
  {
    lpListItem = (LISTVIEW_ITEM *)DPA_GetPtr(infoPtr->hdpaItems, nItem);
    if (lpListItem == NULL)
      return FALSE;

    /* set state */
    lpListItem->state &= ~lpItem->stateMask;
    lpListItem->state |= (lpItem->state & lpItem->stateMask);
    }
  else
    return FALSE;

    return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the text background color.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
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
 * [I] HWND32 : window handle
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
 * ??
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
*/
static LRESULT LISTVIEW_SortItems(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLong32A(hwnd, 0); */

    FIXME (listview, "empty stub!\n");

    return TRUE;
}

/***
 * DESCRIPTION:
 * Updates an items.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : item index
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
*/
static LRESULT LISTVIEW_Update(HWND hwnd, INT nItem)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  RECT rcItemRect;
  BOOL bResult = FALSE;

  if ((nItem >= 0) || (nItem < infoPtr->nItemCount))
  {
    /* get item bounding rectangle */
    rcItemRect.left = LVIR_BOUNDS;
    SendMessageA(hwnd, LVM_GETITEMRECT, (WPARAM)nItem, 
                   (LPARAM)&rcItemRect);

    InvalidateRect(hwnd, &rcItemRect, FALSE);

    /* rearrange with default alignment style */
    if ((lStyle & LVS_AUTOARRANGE) && 
        ((lStyle & LVS_ICON) || (lStyle & LVS_SMALLICON)))
      SendMessageA(hwnd, LVM_ARRANGE, (WPARAM)LVA_DEFAULT, (LPARAM)0);
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Creates a listview control.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_Create(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE); 
  LOGFONTA logFont;
  HFONT hOldFont;
  HDC hdc;
  TEXTMETRICA tm;
  INT nSmallIconHeight;

  /* initialize color information  */
  infoPtr->clrBk = GetSysColor(COLOR_WINDOW);
  infoPtr->clrText = GetSysColor(COLOR_WINDOWTEXT);
  infoPtr->clrTextBk = GetSysColor(COLOR_WINDOW); 

    /* get default font (icon title) */
    SystemParametersInfoA (SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
    infoPtr->hDefaultFont = CreateFontIndirectA (&logFont);
    infoPtr->hFont = infoPtr->hDefaultFont;

  /* initialize column width */
  infoPtr->nColumnWidth = 96;

  /* get text height */
  hdc = GetDC(hwnd);
  hOldFont = SelectObject(hdc, infoPtr->hFont);
  GetTextMetricsA(hdc, &tm);

  /* initialize item height with padding */
  if (!(lStyle & LVS_ICON)) 
  {
    nSmallIconHeight = GetSystemMetrics(SM_CYSMICON);
    infoPtr->nItemHeight = max(tm.tmHeight, nSmallIconHeight) + 2;
  }

  SelectObject(hdc, hOldFont);
  ReleaseDC(hwnd, hdc);

 /* create header */
  infoPtr->hwndHeader =	CreateWindowA(WC_HEADERA, "", lStyle, 0, 0, 0, 0, 
                                        hwnd, (HMENU)0, 
                                        GetWindowLongA(hwnd, GWL_HINSTANCE),
                                        NULL);
    /* set header font */
  SendMessageA(infoPtr->hwndHeader, WM_SETFONT, (WPARAM)infoPtr->hFont, 
                 (LPARAM)TRUE);

  /* allocate memory */
    infoPtr->hdpaItems = DPA_Create (10);

    return 0;
}


static LRESULT
LISTVIEW_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLongA(hwnd, 0);

    /* delete all items */
    LISTVIEW_DeleteAllItems(hwnd);

    /* destroy dpa */
    DPA_Destroy (infoPtr->hdpaItems);

    /* destroy header */
    if (infoPtr->hwndHeader)
	DestroyWindow (infoPtr->hwndHeader);

    /* destroy font */
    infoPtr->hFont = (HFONT)0;
    if (infoPtr->hDefaultFont)
	DeleteObject (infoPtr->hDefaultFont);

    infoPtr->nColumnWidth = 96;

    return 0;
}

/***
 * DESCRIPTION:
 * Erases the background of the listview control
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_EraseBackground(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  BOOL bResult;

  if (infoPtr->clrBk == CLR_NONE) 
    bResult = SendMessageA(GetParent(hwnd), WM_ERASEBKGND, wParam, lParam);
  else 
  {
    HBRUSH hBrush = CreateSolidBrush(infoPtr->clrBk);
    RECT   clientRect;

    GetClientRect(hwnd, &clientRect);
    FillRect((HDC)wParam, &clientRect, hBrush);
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
 * [I] HWND32 : window handle
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
 * [I] HWND32 : window handle
 * [I] INT32 : scroll code
 * [I] INT32 : scroll position
 * [I] HWND32 : scrollbar control window handle
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_HScroll(HWND hwnd, INT nScrollCode, 
                                INT nScrollPos, HWND hScrollWnd)
{
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nHScrollPos;
  INT nMinRange;
  INT nMaxRange;

  GetScrollRange(hwnd, SB_HORZ, &nMinRange, &nMaxRange);
  nHScrollPos = GetScrollPos(hwnd, SB_HORZ);

  if (lStyle & LVS_LIST)
  {
    switch (nScrollCode)
    {
    case SB_LINELEFT:
      if (nHScrollPos > nMinRange)
        nHScrollPos--;
      break;
    case SB_LINERIGHT:
      if (nHScrollPos < nMaxRange)
        nHScrollPos++;
      break;
    case SB_PAGELEFT:
      if (nHScrollPos > nMinRange)
        nHScrollPos--;
      break;
    case SB_PAGERIGHT:
      if (nHScrollPos < nMaxRange)
        nHScrollPos++;
      break;
    case SB_THUMBPOSITION:
      nHScrollPos = nScrollPos;
    break;
    }

    SetScrollPos(hwnd, SB_HORZ, nHScrollPos, TRUE);

    /* refresh client area */
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * ?????
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : virtual key 
 * [I] LONG : key data
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_KeyDown(HWND hwnd, INT nVirtualKey, LONG lKeyData)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  HWND hwndParent = GetParent(hwnd);
  INT nItemCountPerColumn;
  INT nItemCountPerRow;
  NMHDR nmh;
  INT nColumn;
/*   NMLVKEYDOWN nmKeyDown; */

  /* send LVN_KEYDOWN notification */
/*   nmh.hwndFrom = hwnd; */
/*   nmh.idFrom = lCtrlId; */
/*   nmh.code = LVN_KEYDOWN; */
/*   nmKeyDown.hdr = nmh; */
/*   nmKeyDown..wVKey = nVirtualKey; */
/*   nmKeyCode.flags = 0; */
/*   SendMessage32A(hwndParent, WM_NOTIFY, (WPARAM32)lCtrlId, (LPARAM)&nmKeyDown); */

  switch (nVirtualKey)
  {
  case VK_RETURN:
    if ((infoPtr->nItemCount > 0) && (infoPtr->nFocusedItem != -1))
    {
      /* send NM_RETURN notification */
      nmh.code = NM_RETURN;
      SendMessageA(hwndParent, WM_NOTIFY, (WPARAM)lCtrlId, (LPARAM)&nmh);

      /* send LVN_ITEMACTIVATE notification */
      nmh.code = LVN_ITEMACTIVATE;
      SendMessageA(hwndParent, WM_NOTIFY, (WPARAM)lCtrlId, (LPARAM)&nmh);
    }
    break;

  case VK_HOME:
    if (infoPtr->nItemCount > 0)
    {
      /* set item state(s) */
      infoPtr->nFocusedItem = 0;
      LISTVIEW_SetItemStates(hwnd, infoPtr->nFocusedItem, TRUE, FALSE);

      /* make item visible */
      LISTVIEW_SetVisible(hwnd, infoPtr->nFocusedItem);
    }
    break;

  case VK_END:
    if (infoPtr->nItemCount > 0)
{
      /* set item state(s) */
      infoPtr->nFocusedItem = infoPtr->nItemCount - 1;
      LISTVIEW_SetItemStates(hwnd, infoPtr->nFocusedItem, TRUE, FALSE);

      /* make item visible */
      LISTVIEW_SetVisible(hwnd, infoPtr->nFocusedItem);
    }

    break;

  case VK_LEFT:
    if (lStyle & LVS_LIST)
    {
      nItemCountPerColumn = LISTVIEW_GetItemCountPerColumn(hwnd);
      if (infoPtr->nFocusedItem >= nItemCountPerColumn) 
      {
         /* set item state(s) */
        infoPtr->nFocusedItem -= nItemCountPerColumn;
        LISTVIEW_SetItemStates(hwnd, infoPtr->nFocusedItem, TRUE, FALSE);

        /* make item visible */
        LISTVIEW_SetVisible(hwnd, infoPtr->nFocusedItem);
      }
    }
    else if (lStyle & LVS_REPORT)
    {
      /* TO DO ; does not affect the focused item, it only scrolls */
    }
    else if ((lStyle & LVS_SMALLICON) || (lStyle & LVS_ICON))
    {
      nItemCountPerRow = LISTVIEW_GetItemCountPerRow(hwnd);
      nColumn = infoPtr->nFocusedItem % nItemCountPerRow;
      if (nColumn != 0)
      {
        infoPtr->nFocusedItem -= 1; 
        LISTVIEW_SetItemStates(hwnd, infoPtr->nFocusedItem, TRUE, FALSE);

        /* refresh display */
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
      }
    }
    break;
    
  case VK_UP:
    if ((lStyle & LVS_LIST) || (lStyle & LVS_REPORT))
    {
      if (infoPtr->nFocusedItem > 0)
      {
        infoPtr->nFocusedItem -= 1;
        LISTVIEW_SetItemStates(hwnd, infoPtr->nFocusedItem, TRUE, FALSE);

        LISTVIEW_SetVisible(hwnd, infoPtr->nFocusedItem);
}
    }
    else if ((lStyle & LVS_SMALLICON) || (lStyle & LVS_ICON))
    {
      nItemCountPerRow = LISTVIEW_GetItemCountPerRow(hwnd);
      if (infoPtr->nFocusedItem >= nItemCountPerRow)
      {
        infoPtr->nFocusedItem -= nItemCountPerRow;
        LISTVIEW_SetItemStates(hwnd, infoPtr->nFocusedItem, TRUE, FALSE);

        LISTVIEW_SetVisible(hwnd, infoPtr->nFocusedItem);
      }
    }
    break;

  case VK_RIGHT:
    if (lStyle & LVS_LIST)
{
      nItemCountPerColumn = LISTVIEW_GetItemCountPerColumn(hwnd);
      if (infoPtr->nFocusedItem < infoPtr->nItemCount - nItemCountPerColumn)
      {
        infoPtr->nFocusedItem += nItemCountPerColumn;
        LISTVIEW_SetItemStates(hwnd, infoPtr->nFocusedItem, TRUE, FALSE);

        LISTVIEW_SetVisible(hwnd, infoPtr->nFocusedItem);
    }
    }
    else if (lStyle & LVS_REPORT)
    {
    }
    else if ((lStyle & LVS_SMALLICON) || (lStyle & LVS_ICON))
    {
      nItemCountPerRow = LISTVIEW_GetItemCountPerRow(hwnd);
      nColumn = infoPtr->nFocusedItem % nItemCountPerRow;
      if (nColumn != nItemCountPerRow - 1)
      {
        infoPtr->nFocusedItem += 1; 
        LISTVIEW_SetItemStates(hwnd, infoPtr->nFocusedItem, TRUE, FALSE);

        /* refresh display */
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
}

    }
    break;

  case VK_DOWN:
    if ((lStyle & LVS_LIST) || (lStyle & LVS_REPORT))
{
      if (infoPtr->nFocusedItem < infoPtr->nItemCount - 1)
      {
        infoPtr->nFocusedItem += 1;
        LISTVIEW_SetItemStates(hwnd, infoPtr->nFocusedItem, TRUE, FALSE);

        LISTVIEW_SetVisible(hwnd, infoPtr->nFocusedItem);
}
    }
    else if ((lStyle & LVS_SMALLICON) || (lStyle & LVS_ICON))
    {
      nItemCountPerRow = LISTVIEW_GetItemCountPerRow(hwnd);
      if (infoPtr->nFocusedItem < infoPtr->nItemCount - nItemCountPerRow)
      {
        infoPtr->nFocusedItem += nItemCountPerRow;
        LISTVIEW_SetItemStates(hwnd, infoPtr->nFocusedItem, TRUE, FALSE);

        LISTVIEW_SetVisible(hwnd, infoPtr->nFocusedItem);
      }
    }
    break;

  case VK_PRIOR:
    break;

  case VK_NEXT:
    break;
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Kills the focus.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_KillFocus(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO*)GetWindowLongA(hwnd, 0);
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
    NMHDR nmh;

  nmh.hwndFrom = hwnd;
  nmh.idFrom = lCtrlId;
    nmh.code = NM_KILLFOCUS;
  SendMessageA(GetParent(hwnd), WM_NOTIFY, (WPARAM)lCtrlId, 
                 (LPARAM)&nmh);

  /* set window focus flag */
    infoPtr->bFocus = FALSE;

    return 0;
}

/***
 * DESCRIPTION:
 * Left mouse button double click.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
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
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMHDR nmh;

  /* send NM_DBLCLK notification */
  nmh.hwndFrom = hwnd;
  nmh.idFrom = lCtrlId;
  nmh.code = NM_DBLCLK;
  SendMessageA(GetParent (hwnd), WM_NOTIFY, (WPARAM)lCtrlId, 
                 (LPARAM)&nmh);

  /* send LVN_ITEMACTIVATE notification */
  nmh.code = LVN_ITEMACTIVATE;
  SendMessageA(GetParent (hwnd), WM_NOTIFY, (WPARAM)lCtrlId,
                 (LPARAM)&nmh);

  return 0;
}

/***
 * DESCRIPTION:
 * Left mouse button down.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
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
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  LVHITTESTINFO hitTestInfo;
  LVITEMA lvItem;
  NMHDR nmh;
  INT nItem;

  /* send NM_RELEASEDCAPTURE notification */ 
  nmh.hwndFrom = hwnd;
  nmh.idFrom = lCtrlId;
  nmh.code = NM_RELEASEDCAPTURE;
  SendMessageA(GetParent(hwnd), WM_NOTIFY, (WPARAM)lCtrlId, 
                 (LPARAM)&nmh);

  if (infoPtr->bFocus == FALSE)
    SetFocus(hwnd);

  /* set left button down flag */
  infoPtr->bLButtonDown = TRUE;

  /* set left button hit coordinates */
  hitTestInfo.pt.x = wPosX;
  hitTestInfo.pt.y = wPosY;
  
  /* perform hit test */
  nItem = SendMessageA(hwnd, LVM_HITTEST, (WPARAM)0, (LPARAM)&hitTestInfo);
    if ((nItem >= 0) && (nItem < infoPtr->nItemCount))
  {
    /* perform state changes (selection and focus) */
    infoPtr->nFocusedItem = nItem;
    LISTVIEW_SetItemStates(hwnd, infoPtr->nFocusedItem, TRUE, TRUE);

    /* scroll intem into view if doing a multiple selection */

    if ((wKey & MK_CONTROL) || (wKey & MK_SHIFT))
{
      LISTVIEW_SetVisible(hwnd, infoPtr->nFocusedItem);
    }
    else
    {
      /* refresh display */
      InvalidateRect(hwnd, NULL, FALSE);
      UpdateWindow(hwnd);
    }
  }
  else
  {
    /* clear selection(s) */
    ZeroMemory(&lvItem, sizeof(LVITEMA));
    lvItem.stateMask = LVIS_SELECTED;
    lvItem.state = 0;
    SendMessageA(hwnd, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&lvItem);

    /* repaint everything */
    InvalidateRect(hwnd, NULL, FALSE);
    UpdateWindow(hwnd);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Left mouse button up.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
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
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMHDR nmh;

  if (infoPtr->bLButtonDown == TRUE) 
  {
    /* send NM_CLICK notification */
    nmh.hwndFrom = hwnd;
    nmh.idFrom = lCtrlId;
    nmh.code = NM_CLICK;
    SendMessageA(GetParent (hwnd), WM_NOTIFY, (WPARAM)lCtrlId, 
                     (LPARAM)&nmh);
  }

  /* set left button flag */
  infoPtr->bLButtonDown = FALSE;

    return 0;
}

/***
 * DESCRIPTION:
 * Creates the listview control (called before WM_CREATE).
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] WPARAM32 : unhandled 
 * [I] LPARAM : widow creation info
 * 
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_NCCreate(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (LISTVIEW_INFO *)COMCTL32_Alloc (sizeof(LISTVIEW_INFO));
  SetWindowLongA(hwnd, 0, (LONG)infoPtr);
  if (infoPtr == NULL) 
  {
	ERR (listview, "could not allocate info memory!\n");
	return 0;
    }

  if ((LISTVIEW_INFO *)GetWindowLongA(hwnd, 0) != infoPtr) 
  {
	ERR (listview, "pointer assignment error!\n");
	return 0;
    }

  return DefWindowProcA(hwnd, WM_NCCREATE, wParam, lParam);
}

/***
 * DESCRIPTION:
 * Destroys the listview control (called after WM_DESTROY).
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * 
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_NCDestroy(HWND hwnd)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);

    /* free list view info data */
    COMCTL32_Free (infoPtr);

    return 0;
}

/***
 * DESCRIPTION:
 * Handles notification from children.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] INT32 : control identifier
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
	FIXME (listview, "WM_NOTIFY from header!\n");
    }
  else 
  {
    /* handle notification from unknown source */
	FIXME (listview, "WM_NOTIFY from unknown source!\n");
    }

    return 0;
}

/***
 * DESCRIPTION:
 * Draws the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] HDC32 : device context handle
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
    LISTVIEW_Refresh(hwnd, hdc);

  return 0;
}

/***
 * DESCRIPTION:
 * Right mouse button double clisk.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
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
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMHDR nmh;

  /* send NM_RELEASEDCAPTURE notification */ 
  nmh.hwndFrom = hwnd;
  nmh.idFrom = lCtrlId;
  nmh.code = NM_RELEASEDCAPTURE;
  SendMessageA(GetParent(hwnd), WM_NOTIFY, (WPARAM)lCtrlId, 
                 (LPARAM)&nmh);

  /* send NM_RDBLCLK notification */
  nmh.code = NM_RDBLCLK;
  SendMessageA(GetParent(hwnd), WM_NOTIFY, (WPARAM)lCtrlId, 
                 (LPARAM)&nmh);

    return 0;
}

/***
 * DESCRIPTION:
 * Right mouse button input.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
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
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  LVHITTESTINFO hitTestInfo;
  NMHDR nmh;
  LVITEMA lvItem;
  INT nItem;

  /* The syslistview32 control sends a NM_RELEASEDCAPTURE notification.
     I do not know why, but I decided to send it as well for compatibility 
     purposes. */
  ZeroMemory(&nmh, sizeof(NMHDR));
  nmh.hwndFrom = hwnd;
  nmh.idFrom = lCtrlId;
  nmh.code = NM_RELEASEDCAPTURE;
  SendMessageA(GetParent(hwnd), WM_NOTIFY, (WPARAM)lCtrlId, 
                 (LPARAM)&nmh);
 
  /* make sure listview control has focus */
  if (infoPtr->bFocus == FALSE)
    SetFocus(hwnd);

  /* set left button down flag */
  infoPtr->bRButtonDown = TRUE;

  /* set hit coordinates */
  hitTestInfo.pt.x = wPosX;
  hitTestInfo.pt.y = wPosY;

  /* perform hit test */
  nItem = SendMessageA(hwnd, LVM_HITTEST, (WPARAM)0, (LPARAM)&hitTestInfo);
  if ((nItem >= 0) && (nItem < infoPtr->nItemCount))
  {
    /* perform state changes (selection and focus) */
    infoPtr->nFocusedItem = nItem;
    LISTVIEW_SetItemStates(hwnd, infoPtr->nFocusedItem, FALSE, TRUE);
  }
  else
  {
    /* clear selection(s) */
    ZeroMemory(&lvItem, sizeof(LVITEMA));
    lvItem.stateMask = LVIS_SELECTED;
    lvItem.state = 0;
    SendMessageA(hwnd, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&lvItem);
  }

  /* repaint everything */
  InvalidateRect(hwnd, NULL, FALSE);
  UpdateWindow(hwnd);

    return 0;
}

/***
 * DESCRIPTION:
 * Right mouse button up.
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
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
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
    NMHDR nmh;

  if (infoPtr->bRButtonDown == TRUE) 
  {
    /* initialize notification information */
    ZeroMemory(&nmh, sizeof(NMHDR));
    nmh.hwndFrom = hwnd;
    nmh.idFrom = lCtrlId;
    nmh.code = NM_RCLICK;
    SendMessageA(GetParent (hwnd), WM_NOTIFY, (WPARAM)lCtrlId, 
                   (LPARAM)&nmh);
  }

  /* set button flag */
  infoPtr->bRButtonDown = FALSE;

    return 0;
}

/***
 * DESCRIPTION:
 * Sets the focus.  
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] HWND32 : window handle of previously focused window
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_SetFocus(HWND hwnd, HWND hwndLoseFocus)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lCtrlId = GetWindowLongA(hwnd, GWL_ID);
  NMHDR nmh;
    
  /* send NM_SETFOCUS notification */
  nmh.hwndFrom = hwnd;
  nmh.idFrom = lCtrlId;
  nmh.code = NM_SETFOCUS;
  SendMessageA(GetParent(hwnd), WM_NOTIFY, (WPARAM)lCtrlId, 
                 (LPARAM)&nmh);

  /* set window focus flag */
  infoPtr->bFocus = TRUE;

  return 0;
}

/***
 * DESCRIPTION:
 * Sets the font.  
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] HFONT32 : font handle
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

  if (lStyle & LVS_REPORT)
  {
    /* set font of header */
    SendMessageA(infoPtr->hwndHeader, WM_SETFONT, (WPARAM)hFont, 
                   MAKELPARAM(fRedraw, 0));
}

  /* invalidate listview control client area */
  InvalidateRect(hwnd, NULL, FALSE);

  if (fRedraw == TRUE)
    UpdateWindow(hwnd);

  return 0;
}

/***
 * DESCRIPTION:
 * Resizes the listview control.  
 * 
 * PARAMETER(S):
 * [I] HWND32 : window handle
 * [I] WORD : new width
 * [I] WORD : new height
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_Size(HWND hwnd, WORD wWidth, WORD wHeight)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongA(hwnd, 0);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  INT nHScrollHeight;

  if (lStyle & LVS_LIST)
  {
    if (!(lStyle & WS_HSCROLL))
    {
      nHScrollHeight = GetSystemMetrics(SM_CYHSCROLL);
      if (wHeight > nHScrollHeight)
        wHeight -= nHScrollHeight;
    }

    infoPtr->nHeight = (INT)wHeight;
    infoPtr->nWidth = (INT)wWidth;
  }

  /* set scrollbar(s) if needed */
  LISTVIEW_SetScroll(hwnd);

  /* refresh client area */
  InvalidateRect(hwnd, NULL, TRUE);
  UpdateWindow(hwnd);

    return 0;
}

/***
 * DESCRIPTION:
 * Window procedure of the listview control.
 * 
 * PARAMETER(S):
 * [I] HWND32 :
 * [I] UINT32 :
 * [I] WPARAM32 :
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

/*	case LVM_CREATEDRAGIMAGE: */

	case LVM_DELETEALLITEMS:
    return LISTVIEW_DeleteAllItems(hwnd);

	case LVM_DELETECOLUMN:
    return LISTVIEW_DeleteColumn(hwnd, (INT)wParam);

	case LVM_DELETEITEM:
    return LISTVIEW_DeleteItem(hwnd, (INT)wParam);

/*	case LVM_EDITLABEL: */
/*	case LVM_ENSUREVISIBLE: */

  case LVM_FINDITEMA:
    return LISTVIEW_FindItem(hwnd, (INT)wParam, (LPLVFINDINFO)lParam);

	case LVM_GETBKCOLOR:
    return LISTVIEW_GetBkColor(hwnd);

/*	case LVM_GETBKIMAGE: */
/*	case LVM_GETCALLBACKMASK: */

	case LVM_GETCOLUMNA:
    return LISTVIEW_GetColumnA(hwnd, (INT)wParam, (LPLVCOLUMNA)lParam);

/*	case LVM_GETCOLUMN32W: */
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

/*	case LVM_GETITEM32W: */

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

/*	case LVM_GETITEMTEXT32W: */

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

/*	case LVM_GETSTRINGWIDTH32W: */
/*	case LVM_GETSUBITEMRECT: */

	case LVM_GETTEXTBKCOLOR:
    return LISTVIEW_GetTextBkColor(hwnd);

	case LVM_GETTEXTCOLOR:
    return LISTVIEW_GetTextColor(hwnd);

/*	case LVM_GETTOOLTIPS: */
/*	case LVM_GETTOPINDEX: */
/*	case LVM_GETUNICODEFORMAT: */

  case LVM_GETVIEWRECT:
    return LISTVIEW_GetViewRect(hwnd, (LPRECT)lParam);

/*	case LVM_GETWORKAREAS: */

	case LVM_HITTEST:
    return LISTVIEW_HitTest(hwnd, (LPLVHITTESTINFO)lParam);

	case LVM_INSERTCOLUMNA:
    return LISTVIEW_InsertColumnA(hwnd, (INT)wParam, 
                                    (LPLVCOLUMNA)lParam);

/*	case LVM_INSERTCOLUMN32W: */

	case LVM_INSERTITEMA:
    return LISTVIEW_InsertItemA(hwnd, (LPLVITEMA)lParam);

/*	case LVM_INSERTITEM32W: */

	case LVM_REDRAWITEMS:
    return LISTVIEW_RedrawItems(hwnd, (INT)wParam, (INT)lParam);

  case LVM_SCROLL: 
    return LISTVIEW_Scroll(hwnd, (INT)wParam, (INT)lParam);

	case LVM_SETBKCOLOR:
    return LISTVIEW_SetBkColor(hwnd, (COLORREF)lParam);

/*	case LVM_SETBKIMAGE: */
/*	case LVM_SETCALLBACKMASK: */

	case LVM_SETCOLUMNA:
    return LISTVIEW_SetColumnA(hwnd, (INT)wParam, (LPLVCOLUMNA)lParam);

/*	case LVM_SETCOLUMN32W: */
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

/*	case LVM_SETITEM32W: */
/*	case LVM_SETITEMCOUNT: */

	case LVM_SETITEMPOSITION16:
    return LISTVIEW_SetItemPosition(hwnd, (INT)wParam, 
                                    (INT)LOWORD(lParam), 
                                    (INT) HIWORD(lParam));

/*	case LVM_SETITEMPOSITION32: */

  case LVM_SETITEMSTATE: 
    return LISTVIEW_SetItemState(hwnd, (INT)wParam, (LPLVITEMA)lParam);

/*	case LVM_SETITEMTEXT: */
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
    return LISTVIEW_Destroy(hwnd, wParam, lParam);

	case WM_ERASEBKGND:
    return LISTVIEW_EraseBackground (hwnd, wParam, lParam);

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

/*	case WM_TIMER: */
/*	case WM_VSCROLL: */
/*	case WM_WINDOWPOSCHANGED: */
/*	case WM_WININICHANGE: */

	default:
	    if (uMsg >= WM_USER)
		ERR (listview, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);

    /* call default window procedure */
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
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
    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC)LISTVIEW_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(LISTVIEW_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
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
    if (GlobalFindAtomA (WC_LISTVIEWA))
	UnregisterClassA (WC_LISTVIEWA, (HINSTANCE)NULL);
}

