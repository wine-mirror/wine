/*
 * Tab control
 *
 * Copyright 1998 Anders Carlsson
 * Copyright 1999 Alex Priem <alexp@sci.kun.nl>
 * Copyright 1999 Francis Beaudet
 *
 * TODO:
 *  Image list support
 *  Multiline support
 *  Unicode support
 */

#include <string.h>

#include "winbase.h"
#include "commctrl.h"
#include "tab.h"
#include "debugtools.h"
#include "cache.h"
#include "win.h"

DEFAULT_DEBUG_CHANNEL(tab)

/******************************************************************************
 * Positioning constants
 */
#define SELECTED_TAB_OFFSET     2
#define HORIZONTAL_ITEM_PADDING 5
#define VERTICAL_ITEM_PADDING   3
#define ROUND_CORNER_SIZE       2
#define FOCUS_RECT_HOFFSET      2
#define FOCUS_RECT_VOFFSET      1
#define DISPLAY_AREA_PADDINGX   2
#define DISPLAY_AREA_PADDINGY   2
#define CONTROL_BORDER_SIZEX    2
#define CONTROL_BORDER_SIZEY    2
#define BUTTON_SPACINGX         10
#define DEFAULT_TAB_WIDTH       96

#define TAB_GetInfoPtr(hwnd) ((TAB_INFO *)GetWindowLongA(hwnd,0))

/******************************************************************************
 * Prototypes
 */
static void TAB_Refresh (HWND hwnd, HDC hdc);
static void TAB_InvalidateTabArea(HWND      hwnd, TAB_INFO* infoPtr);
static void TAB_EnsureSelectionVisible(HWND hwnd, TAB_INFO* infoPtr);

static BOOL
TAB_SendSimpleNotify (HWND hwnd, UINT code)
{
    NMHDR nmhdr;

    nmhdr.hwndFrom = hwnd;
    nmhdr.idFrom = GetWindowLongA(hwnd, GWL_ID);
    nmhdr.code = code;

    return (BOOL) SendMessageA (GetParent (hwnd), WM_NOTIFY,
            (WPARAM) nmhdr.idFrom, (LPARAM) &nmhdr);
}


static VOID
TAB_RelayEvent (HWND hwndTip, HWND hwndMsg, UINT uMsg,
            WPARAM wParam, LPARAM lParam)
{
    MSG msg;

    msg.hwnd = hwndMsg;
    msg.message = uMsg;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.time = GetMessageTime ();
    msg.pt.x = LOWORD(GetMessagePos ());
    msg.pt.y = HIWORD(GetMessagePos ());

    SendMessageA (hwndTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
}



static LRESULT
TAB_GetCurSel (HWND hwnd)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
 
    return infoPtr->iSelected;
}

static LRESULT
TAB_GetCurFocus (HWND hwnd)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
 
    return infoPtr->uFocus;
}

static LRESULT
TAB_GetToolTips (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

    if (infoPtr == NULL) return 0;
    return infoPtr->hwndToolTip;
}


static LRESULT
TAB_SetCurSel (HWND hwnd,WPARAM wParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  INT iItem=(INT) wParam;
  INT prevItem;
 
  prevItem=-1;
  if ((iItem >= 0) && (iItem < infoPtr->uNumItem)) {
    prevItem=infoPtr->iSelected;
      infoPtr->iSelected=iItem;
      TAB_EnsureSelectionVisible(hwnd, infoPtr);
      TAB_InvalidateTabArea(hwnd, infoPtr);
  }
  return prevItem;
}

static LRESULT
TAB_SetCurFocus (HWND hwnd,WPARAM wParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  INT iItem=(INT) wParam;
 
  if ((iItem < 0) || (iItem >= infoPtr->uNumItem)) return 0;

  if (GetWindowLongA(hwnd, GWL_STYLE) & TCS_BUTTONS) {
    FIXME("Should set input focus\n");
  } else { 
    if (infoPtr->iSelected != iItem || infoPtr->uFocus == -1 ) {
      infoPtr->uFocus=iItem;
      if (TAB_SendSimpleNotify(hwnd, TCN_SELCHANGING)!=TRUE)  {
        infoPtr->iSelected = iItem;
        TAB_SendSimpleNotify(hwnd, TCN_SELCHANGE);

	TAB_EnsureSelectionVisible(hwnd, infoPtr);
	TAB_InvalidateTabArea(hwnd, infoPtr);
      }
    }
  }
  return 0;
}

static LRESULT
TAB_SetToolTips (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

    if (infoPtr == NULL) return 0;
    infoPtr->hwndToolTip = (HWND)wParam;
    return 0;
}

/******************************************************************************
 * TAB_InternalGetItemRect
 *
 * This method will calculate the rectangle representing a given tab item in
 * client coordinates. This method takes scrolling into account.
 *
 * This method returns TRUE if the item is visible in the window and FALSE
 * if it is completely outside the client area.
 */
static BOOL TAB_InternalGetItemRect(
  HWND        hwnd,
  TAB_INFO*   infoPtr,
  INT         itemIndex,
  RECT*       itemRect,
  RECT*       selectedRect)
{
  RECT tmpItemRect;

  /*
   * Perform a sanity check and a trivial visibility check.
   */
  if ( (infoPtr->uNumItem <=0) || 
       (itemIndex >= infoPtr->uNumItem) ||
       (itemIndex < infoPtr->leftmostVisible) )
    return FALSE;

  /*
   * Avoid special cases in this procedure by assigning the "out"
   * parameters if the caller didn't supply them
   */
  if (itemRect==NULL)
    itemRect = &tmpItemRect;
  
  /*
   * Retrieve the unmodified item rect.
   */
  *itemRect = infoPtr->items[itemIndex].rect;

  /*
   * "scroll" it to make sure the item at the very left of the 
   * tab control is the leftmost visible tab.
   */
  OffsetRect(itemRect,
	     -infoPtr->items[infoPtr->leftmostVisible].rect.left, 
	     0);

  /*
   * Move the rectangle so the first item is slightly offset from 
   * the left of the tab control.
   */
  OffsetRect(itemRect,
	     SELECTED_TAB_OFFSET,
	     0);


  /*
   * Now, calculate the position of the item as if it were selected.
   */
  if (selectedRect!=NULL)
  {
    CopyRect(selectedRect, itemRect);

    /*
     * The rectangle of a selected item is a bit wider.
     */
    InflateRect(selectedRect, SELECTED_TAB_OFFSET, 0);

    /*
     * If it also a bit higher.
     */
    if (GetWindowLongA(hwnd, GWL_STYLE) & TCS_BOTTOM) 
    {      
      selectedRect->top    -=2; /* the border is thicker on the bottom */
      selectedRect->bottom +=SELECTED_TAB_OFFSET;
    }
    else
    {
      selectedRect->top   -=SELECTED_TAB_OFFSET;
      selectedRect->bottom+=1;
    }
  }

  return TRUE;
}

static BOOL TAB_GetItemRect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  return TAB_InternalGetItemRect(hwnd, TAB_GetInfoPtr(hwnd), (INT)wParam, 
                                 (LPRECT)lParam, (LPRECT)NULL);
}

/******************************************************************************
 * TAB_KeyUp
 *
 * This method is called to handle keyboard input
 */
static LRESULT TAB_KeyUp(
  HWND   hwnd, 
  WPARAM keyCode)
{
  TAB_INFO* infoPtr = TAB_GetInfoPtr(hwnd);
  int       newItem = -1;

  switch (keyCode)
  {
    case VK_LEFT:
      newItem = infoPtr->uFocus-1;
      break;
    case VK_RIGHT:
      newItem = infoPtr->uFocus+1;
      break;
  }
  
  /*
   * If we changed to a valid item, change the selection
   */
  if ( (newItem >= 0) &&
       (newItem < infoPtr->uNumItem) &&
       (infoPtr->uFocus != newItem) )
  {
    if (!TAB_SendSimpleNotify(hwnd, TCN_SELCHANGING))
    {
      infoPtr->iSelected = newItem;
      infoPtr->uFocus    = newItem;
      TAB_SendSimpleNotify(hwnd, TCN_SELCHANGE);

      TAB_EnsureSelectionVisible(hwnd, infoPtr);
      TAB_InvalidateTabArea(hwnd, infoPtr);
    }
  }

  return 0;
}

/******************************************************************************
 * TAB_FocusChanging
 *
 * This method is called whenever the focus goes in or out of this control
 * it is used to update the visual state of the control.
 */
static LRESULT TAB_FocusChanging(
  HWND   hwnd, 
  UINT   uMsg, 
  WPARAM wParam, 
  LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  RECT      selectedRect;
  BOOL      isVisible;

  /*
   * Get the rectangle for the item.
   */
  isVisible = TAB_InternalGetItemRect(hwnd,
				      infoPtr,
				      infoPtr->uFocus,
				      NULL,
				      &selectedRect);
  
  /*
   * If the rectangle is not completely invisible, invalidate that
   * portion of the window.
   */
  if (isVisible)
  {
    InvalidateRect(hwnd, &selectedRect, TRUE);
  }

  /*
   * Don't otherwise disturb normal behavior.
   */
  return DefWindowProcA (hwnd, uMsg, wParam, lParam);
}

static HWND TAB_InternalHitTest (
  HWND      hwnd,
  TAB_INFO* infoPtr, 
  POINT     pt, 
  UINT*     flags)

{
  RECT rect;
  int iCount; 
  
  for (iCount = 0; iCount < infoPtr->uNumItem; iCount++) 
  {
    TAB_InternalGetItemRect(hwnd,
			    infoPtr, 
			    iCount,
			    &rect,
			    NULL);

    if (PtInRect (&rect, pt))
    {
      *flags = TCHT_ONITEM;
      return iCount;
    }
  }

  *flags=TCHT_NOWHERE;
  return -1;
}

static LRESULT
TAB_HitTest (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  LPTCHITTESTINFO lptest=(LPTCHITTESTINFO) lParam;
  
  return TAB_InternalHitTest (hwnd, infoPtr,lptest->pt,&lptest->flags);
}


static LRESULT
TAB_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  POINT pt;
  INT newItem,dummy;

  if (infoPtr->hwndToolTip)
    TAB_RelayEvent (infoPtr->hwndToolTip, hwnd,
		    WM_LBUTTONDOWN, wParam, lParam);

  if (GetWindowLongA(hwnd, GWL_STYLE) & TCS_FOCUSONBUTTONDOWN ) {
    SetFocus (hwnd);
  }

  if (infoPtr->hwndToolTip)
    TAB_RelayEvent (infoPtr->hwndToolTip, hwnd,
		    WM_LBUTTONDOWN, wParam, lParam);
  
  pt.x = (INT)LOWORD(lParam);
  pt.y = (INT)HIWORD(lParam);
  
  newItem=TAB_InternalHitTest (hwnd, infoPtr,pt,&dummy);
  
  TRACE("On Tab, item %d\n", newItem);
    
  if ( (newItem!=-1) &&
       (infoPtr->iSelected != newItem) )
  {
    if (TAB_SendSimpleNotify(hwnd, TCN_SELCHANGING)!=TRUE)
    {
      infoPtr->iSelected = newItem;
      infoPtr->uFocus    = newItem;
      TAB_SendSimpleNotify(hwnd, TCN_SELCHANGE);

      TAB_EnsureSelectionVisible(hwnd, infoPtr);

      TAB_InvalidateTabArea(hwnd, infoPtr);
    }
  }
  return 0;
}

static LRESULT
TAB_LButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_SendSimpleNotify(hwnd, NM_CLICK);

  return 0;
}

static LRESULT
TAB_RButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_SendSimpleNotify(hwnd, NM_RCLICK);
  return 0;
}

static LRESULT
TAB_MouseMove (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

  if (infoPtr->hwndToolTip)
    TAB_RelayEvent (infoPtr->hwndToolTip, hwnd,
		    WM_LBUTTONDOWN, wParam, lParam);
  return 0;
}

/******************************************************************************
 * TAB_AdjustRect
 *
 * Calculates the tab control's display area given the windows rectangle or
 * the window rectangle given the requested display rectangle.
 */
static LRESULT TAB_AdjustRect(
  HWND   hwnd, 
  WPARAM fLarger, 
  LPRECT prc)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

  if (fLarger) 
  {
    /*
     * Go from display rectangle
     */

    /*
     * Add the height of the tabs.
     */
    if (GetWindowLongA(hwnd, GWL_STYLE) & TCS_BOTTOM) 
      prc->bottom += infoPtr->tabHeight;
    else
      prc->top -= infoPtr->tabHeight;

    /*
     * Inflate the rectangle for the padding
     */
    InflateRect(prc, DISPLAY_AREA_PADDINGX, DISPLAY_AREA_PADDINGY);

    /*
     * Inflate for the border
     */
    InflateRect(prc, CONTROL_BORDER_SIZEX, CONTROL_BORDER_SIZEX);
  }
  else 
  {
    /*
     * Go from window rectangle.
     */
  
    /*
     * Deflate the rectangle for the border
     */
    InflateRect(prc, -CONTROL_BORDER_SIZEX, -CONTROL_BORDER_SIZEX);

    /*
     * Deflate the rectangle for the padding
     */
    InflateRect(prc, -DISPLAY_AREA_PADDINGX, -DISPLAY_AREA_PADDINGY);

    /*
     * Remove the height of the tabs.
     */
    if (GetWindowLongA(hwnd, GWL_STYLE) & TCS_BOTTOM) 
      prc->bottom -= infoPtr->tabHeight;
    else
      prc->top += infoPtr->tabHeight;

  }
  
  return 0;
}

/******************************************************************************
 * TAB_OnHScroll
 *
 * This method will handle the notification from the scroll control and
 * perform the scrolling operation on the tab control.
 */
static LRESULT TAB_OnHScroll(
  HWND    hwnd, 
  int     nScrollCode,
  int     nPos,
  HWND    hwndScroll)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

  if(nScrollCode == SB_THUMBPOSITION && nPos != infoPtr->leftmostVisible)
  {
     if(nPos < infoPtr->leftmostVisible)
        infoPtr->leftmostVisible--;
     else
        infoPtr->leftmostVisible++;

     TAB_InvalidateTabArea(hwnd, infoPtr);
     SendMessageA(infoPtr->hwndUpDown, UDM_SETPOS, 0,
                   MAKELONG(infoPtr->leftmostVisible, 0));
   }

   return 0;
}

/******************************************************************************
 * TAB_SetupScroling
 *
 * This method will check the current scrolling state and make sure the 
 * scrolling control is displayed (or not).
 */
static void TAB_SetupScrolling(
  HWND        hwnd,
  TAB_INFO*   infoPtr,
  const RECT* clientRect)
{
  INT maxRange = 0;
  if (infoPtr->needsScrolling)
  {
    RECT controlPos;
    INT vsize, tabwidth;

    /*
     * Calculate the position of the scroll control.
     */
    controlPos.right = clientRect->right;
    controlPos.left  = controlPos.right - 2*GetSystemMetrics(SM_CXHSCROLL);

    if (GetWindowLongA(hwnd, GWL_STYLE) & TCS_BOTTOM) 
    {
      controlPos.top    = clientRect->bottom - infoPtr->tabHeight;
      controlPos.bottom = controlPos.top + GetSystemMetrics(SM_CYHSCROLL);
    }
    else
    {
      controlPos.bottom = clientRect->top + infoPtr->tabHeight;
      controlPos.top    = controlPos.bottom - GetSystemMetrics(SM_CYHSCROLL);
    }

    /*
     * If we don't have a scroll control yet, we want to create one.
     * If we have one, we want to make sure it's positioned right.
     */
    if (infoPtr->hwndUpDown==0)
    {
      /*
       * I use a scrollbar since it seems to be more stable than the Updown
       * control.
       */
      infoPtr->hwndUpDown = CreateWindowA("msctls_updown32",
					  "",
					  WS_VISIBLE | WS_CHILD | UDS_HORZ,
					  controlPos.left, controlPos.top,
					  controlPos.right - controlPos.left,
					  controlPos.bottom - controlPos.top,
					  hwnd,
					  (HMENU)NULL, 
					  (HINSTANCE)NULL, 
					  NULL);	
    }
    else
    {
      SetWindowPos(infoPtr->hwndUpDown, 
		   (HWND)NULL,
		   controlPos.left, controlPos.top,
		   controlPos.right - controlPos.left,
		   controlPos.bottom - controlPos.top,
		   SWP_SHOWWINDOW | SWP_NOZORDER);		   
    }

    /* Now calculate upper limit of the updown control range.
     * We do this by calculating how many tabs will be offscreen when the
     * last tab is visible.
     */
    if(infoPtr->uNumItem)
    {
       vsize = clientRect->right - (controlPos.right - controlPos.left + 1);
       maxRange = infoPtr->uNumItem;
       tabwidth = infoPtr->items[maxRange-1].rect.right;

       for(; maxRange > 0; maxRange--)
       {
          if(tabwidth - infoPtr->items[maxRange - 1].rect.left > vsize)
             break;
       }

       if(maxRange == infoPtr->uNumItem)
          maxRange--;
    }
  }
  else
  {
    /*
     * If we once had a scroll control... hide it.
     */
    if (infoPtr->hwndUpDown!=0)
    {
      ShowWindow(infoPtr->hwndUpDown, SW_HIDE);
    }
  }
  if (infoPtr->hwndUpDown)
     SendMessageA(infoPtr->hwndUpDown, UDM_SETRANGE32, 0, maxRange);
}

/******************************************************************************
 * TAB_SetItemBounds
 *
 * This method will calculate the position rectangles of all the items in the
 * control. The rectangle calculated starts at 0 for the first item in the
 * list and ignores scrolling and selection.
 * It also uses the current font to determine the height of the tab row and
 * it checks if all the tabs fit in the client area of the window. If they
 * dont, a scrolling control is added.
 */
static void TAB_SetItemBounds (HWND hwnd)
{
  TAB_INFO*   infoPtr = TAB_GetInfoPtr(hwnd);
  LONG        lStyle  = GetWindowLongA(hwnd, GWL_STYLE);
  TEXTMETRICA fontMetrics;
  INT         curItem;
  INT         curItemLeftPos;
  HFONT       hFont, hOldFont;
  HDC         hdc;
  RECT        clientRect;
  SIZE        size;

  /*
   * We need to get text information so we need a DC and we need to select
   * a font.
   */
  hdc = GetDC(hwnd); 
    
  hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);
  hOldFont = SelectObject (hdc, hFont);

  /*
   * We will base the rectangle calculations on the client rectangle
   * of the control.
   */
  GetClientRect(hwnd, &clientRect);
  
  /*
   * The leftmost item will be "0" aligned
   */
  curItemLeftPos = 0;

  if ( !(lStyle & TCS_FIXEDWIDTH) && !((lStyle & TCS_OWNERDRAWFIXED) && infoPtr->fSizeSet) )
  {
    int item_height;
    int icon_height = 0;

    /*
     * Use the current font to determine the height of a tab.
     */
    GetTextMetricsA(hdc, &fontMetrics);

    /*
     * Get the icon height
     */
    if (infoPtr->himl)
      ImageList_GetIconSize(infoPtr->himl, 0, &icon_height);

    /*
     * Take the highest between font or icon
     */
    if (fontMetrics.tmHeight > icon_height)
      item_height = fontMetrics.tmHeight;
    else
      item_height = icon_height;

    /*
     * Make sure there is enough space for the letters + icon + growing the 
     * selected item + extra space for the selected item.   
     */
    infoPtr->tabHeight = item_height + 2*VERTICAL_ITEM_PADDING +  
      SELECTED_TAB_OFFSET;
  }

  for (curItem = 0; curItem < infoPtr->uNumItem; curItem++)
  {
    /*
     * Calculate the vertical position of the tab
     */
    if (lStyle & TCS_BOTTOM) 
    {
      infoPtr->items[curItem].rect.bottom = clientRect.bottom - 
                                            SELECTED_TAB_OFFSET;
      infoPtr->items[curItem].rect.top = clientRect.bottom - 
                                         infoPtr->tabHeight;
    }
    else 
    {
      infoPtr->items[curItem].rect.top = clientRect.top + 
                                         SELECTED_TAB_OFFSET;
      infoPtr->items[curItem].rect.bottom = clientRect.top + 
                                            infoPtr->tabHeight;
    }

    /*
     * Set the leftmost position of the tab.
     */
    infoPtr->items[curItem].rect.left = curItemLeftPos;

    if ( (lStyle & TCS_FIXEDWIDTH) || ((lStyle & TCS_OWNERDRAWFIXED) && infoPtr->fSizeSet))
    {
      infoPtr->items[curItem].rect.right = infoPtr->items[curItem].rect.left +
                                           infoPtr->tabWidth +
                                           2*HORIZONTAL_ITEM_PADDING;
    }
    else
    {
      int icon_width  = 0;
      int num = 2;

      /*
       * Calculate how wide the tab is depending on the text it contains
       */
      GetTextExtentPoint32A(hdc, infoPtr->items[curItem].pszText, 
                            lstrlenA(infoPtr->items[curItem].pszText), &size);

      /*
       * Add the icon width
       */
      if (infoPtr->himl)
      {
        ImageList_GetIconSize(infoPtr->himl, &icon_width, 0);
        num++;
      }

      infoPtr->items[curItem].rect.right = infoPtr->items[curItem].rect.left +
                                           size.cx + icon_width + 
                                           num*HORIZONTAL_ITEM_PADDING;
    }

    TRACE("TextSize: %i\n ", size.cx);
    TRACE("Rect: T %i, L %i, B %i, R %i\n", 
	  infoPtr->items[curItem].rect.top,
	  infoPtr->items[curItem].rect.left,
	  infoPtr->items[curItem].rect.bottom,
	  infoPtr->items[curItem].rect.right);  

    /*
     * The leftmost position of the next item is the rightmost position
     * of this one.
     */
    if (lStyle & TCS_BUTTONS)
      curItemLeftPos = infoPtr->items[curItem].rect.right + BUTTON_SPACINGX;
    else
      curItemLeftPos = infoPtr->items[curItem].rect.right;
  }

  /*
   * Check if we need a scrolling control.
   */
  infoPtr->needsScrolling = (curItemLeftPos + (2*SELECTED_TAB_OFFSET) > 
                             clientRect.right);

  /* Don't need scrolling, then update infoPtr->leftmostVisible */
  if(!infoPtr->needsScrolling)
    infoPtr->leftmostVisible = 0; 

  TAB_SetupScrolling(hwnd, infoPtr, &clientRect);      
  
  /*
   * Cleanup
   */
  SelectObject (hdc, hOldFont);
  ReleaseDC (hwnd, hdc);
}

/******************************************************************************
 * TAB_DrawItem
 *
 * This method is used to draw a single tab into the tab control.
 */         
static void TAB_DrawItem(
  HWND hwnd, 
  HDC  hdc, 
  INT  iItem)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  LONG      lStyle  = GetWindowLongA(hwnd, GWL_STYLE);
  RECT      itemRect;
  RECT      selectedRect;
  BOOL      isVisible;
  RECT      r;

  /*
   * Get the rectangle for the item.
   */
  isVisible = TAB_InternalGetItemRect(hwnd,
				      infoPtr,
				      iItem,
				      &itemRect,
				      &selectedRect);

  if (isVisible)
  {
    HBRUSH hbr       = CreateSolidBrush (GetSysColor(COLOR_BTNFACE));    
    HPEN   hwPen     = GetSysColorPen (COLOR_3DHILIGHT);
    HPEN   hbPen     = GetSysColorPen (COLOR_BTNSHADOW);
    HPEN   hsdPen    = GetSysColorPen (COLOR_BTNTEXT);
    HPEN   hfocusPen = CreatePen(PS_DOT, 1, GetSysColor(COLOR_BTNTEXT));
    HPEN   holdPen;
    INT    oldBkMode;
    INT    cx,cy; 
    BOOL   deleteBrush = TRUE;

    if (lStyle & TCS_BUTTONS)
    {
      /* 
       * Get item rectangle.
       */
      r = itemRect;

      holdPen = SelectObject (hdc, hwPen);

      if (iItem == infoPtr->iSelected)
      {
        /* 
         * Background color. 
         */
        if (!((lStyle & TCS_OWNERDRAWFIXED) && infoPtr->fSizeSet))
	{
              COLORREF bk = GetSysColor(COLOR_3DHILIGHT);
	  DeleteObject(hbr);
              hbr = GetSysColorBrush(COLOR_SCROLLBAR);
              SetTextColor(hdc, GetSysColor(COLOR_3DFACE));
              SetBkColor(hdc, bk);

              /* if COLOR_WINDOW happens to be the same as COLOR_3DHILIGHT
               * we better use 0x55aa bitmap brush to make scrollbar's background
               * look different from the window background.
               */
              if (bk == GetSysColor(COLOR_WINDOW))
                  hbr = CACHE_GetPattern55AABrush();

              deleteBrush = FALSE;
	}

        /*
         * Erase the background.
         */     
        FillRect(hdc, &r, hbr);

        /*
         * Draw the tab now.
         * The rectangles calculated exclude the right and bottom
         * borders of the rectangle. To simply the following code, those
         * borders are shaved-off beforehand.
         */
        r.right--;
        r.bottom--;

        /* highlight */
        MoveToEx (hdc, r.left, r.bottom, NULL);
        LineTo   (hdc, r.right, r.bottom);
        LineTo   (hdc, r.right, r.top);
        
        /* shadow */
        SelectObject(hdc, hbPen);
        LineTo  (hdc, r.left, r.top);
        LineTo  (hdc, r.left, r.bottom);
      }
      else
      {
        /*
         * Erase the background.
         */     
        FillRect(hdc, &r, hbr);

        /* highlight */
        MoveToEx (hdc, r.left, r.bottom, NULL);
        LineTo   (hdc, r.left, r.top);
        LineTo   (hdc, r.right, r.top);
        
        /* shadow */
        SelectObject(hdc, hbPen);
        LineTo  (hdc, r.right, r.bottom);
        LineTo  (hdc, r.left, r.bottom);
      }
    }
    else
    {
      /* 
       * Background color. 
       */
      DeleteObject(hbr);
      hbr = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));    

      /*
       * We draw a rectangle of different sizes depending on the selection
       * state.
       */
      if (iItem == infoPtr->iSelected)
        r = selectedRect;
      else
        r = itemRect;

      /*
       * Erase the background.
       * This is necessary when drawing the selected item since it is larger 
       * than the others, it might overlap with stuff already drawn by the 
       * other tabs
       */     
      FillRect(hdc, &r, hbr);

      /*
       * Draw the tab now.
       * The rectangles calculated exclude the right and bottom
       * borders of the rectangle. To simply the following code, those
       * borders are shaved-off beforehand.
       */
      r.right--;
      r.bottom--;
      
      holdPen = SelectObject (hdc, hwPen);

      if (lStyle & TCS_BOTTOM) 
      {
        /* highlight */
        MoveToEx (hdc, r.left, r.top, NULL);
        LineTo   (hdc, r.left, r.bottom - ROUND_CORNER_SIZE);
        LineTo   (hdc, r.left + ROUND_CORNER_SIZE, r.bottom);
        
        /* shadow */
        SelectObject(hdc, hbPen);
        LineTo  (hdc, r.right - ROUND_CORNER_SIZE, r.bottom);
        LineTo  (hdc, r.right, r.bottom - ROUND_CORNER_SIZE);
        LineTo  (hdc, r.right, r.top);
      }
      else 
      {
        /* highlight */
        MoveToEx (hdc, r.left, r.bottom, NULL);
        LineTo   (hdc, r.left, r.top + ROUND_CORNER_SIZE);
        LineTo   (hdc, r.left + ROUND_CORNER_SIZE, r.top);
        LineTo   (hdc, r.right - ROUND_CORNER_SIZE, r.top);
        
        /* shadow */
        SelectObject(hdc, hbPen);
        LineTo (hdc, r.right,  r.top + ROUND_CORNER_SIZE);
        LineTo (hdc, r.right,  r.bottom);
      }
    }
  
    /*
     * Text pen
     */
    SelectObject(hdc, hsdPen); 

    oldBkMode = SetBkMode(hdc, TRANSPARENT); 
    SetTextColor (hdc, COLOR_BTNTEXT);

    /*
     * Deflate the rectangle to acount for the padding
     */
    InflateRect(&r, -HORIZONTAL_ITEM_PADDING, -VERTICAL_ITEM_PADDING);

    /*
     * if owner draw, tell the owner to draw
     */
    if ( (lStyle & TCS_OWNERDRAWFIXED) && GetParent(hwnd) )
    {
	DRAWITEMSTRUCT dis;
	WND *pwndPtr;
	UINT id;

	/*
	 * get the control id
	 */
	pwndPtr = WIN_FindWndPtr( hwnd );
	id = pwndPtr->wIDmenu;
	WIN_ReleaseWndPtr(pwndPtr);

	/* 
	 * put together the DRAWITEMSTRUCT
	 */
	dis.CtlType    = ODT_TAB;	
	dis.CtlID      = id;		
	dis.itemID     = iItem;		
	dis.itemAction = ODA_DRAWENTIRE;	
	if ( iItem == infoPtr->iSelected )
	    dis.itemState = ODS_SELECTED;	
	else				
	    dis.itemState = 0;		
	dis.hwndItem = hwnd;		/* */
	dis.hDC      = hdc;		
	dis.rcItem   = r;		/* */
	dis.itemData = infoPtr->items[iItem].lParam;

	/*
	 * send the draw message
	 */
	SendMessageA( GetParent(hwnd), WM_DRAWITEM, (WPARAM)id, (LPARAM)&dis );
    }
    else
    {
	/*
	 * If not owner draw, then do the drawing ourselves.
	 *
     * Draw the icon.
     */
    if (infoPtr->himl && (infoPtr->items[iItem].mask & TCIF_IMAGE) )
    {
      ImageList_Draw (infoPtr->himl, infoPtr->items[iItem].iImage, hdc, 
		      r.left, r.top+1, ILD_NORMAL);
      ImageList_GetIconSize (infoPtr->himl, &cx, &cy);
      r.left+=(cx + HORIZONTAL_ITEM_PADDING);
    }

    /*
     * Draw the text;
     */
    DrawTextA(hdc,
	      infoPtr->items[iItem].pszText, 
	      lstrlenA(infoPtr->items[iItem].pszText),
	      &r, 
	      DT_LEFT|DT_SINGLELINE|DT_VCENTER);
    }

    /*
     * Draw the focus rectangle
     */
    if (((lStyle & TCS_FOCUSNEVER) == 0) &&
	 (GetFocus() == hwnd) &&
	 (iItem == infoPtr->uFocus) )
    {
      InflateRect(&r, FOCUS_RECT_HOFFSET, FOCUS_RECT_VOFFSET);

      SelectObject(hdc, hfocusPen);

      MoveToEx (hdc, r.left,    r.top, NULL);
      LineTo   (hdc, r.right-1, r.top); 
      LineTo   (hdc, r.right-1, r.bottom -1);
      LineTo   (hdc, r.left,    r.bottom -1);
      LineTo   (hdc, r.left,    r.top);
    }

    /*
     * Cleanup
     */
    SetBkMode(hdc, oldBkMode);
    SelectObject(hdc, holdPen);
    DeleteObject(hfocusPen);
    if (deleteBrush) DeleteObject(hbr);
  }
}

/******************************************************************************
 * TAB_DrawBorder
 *
 * This method is used to draw the raised border around the tab control
 * "content" area.
 */         
static void TAB_DrawBorder (HWND hwnd, HDC hdc)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  HPEN htmPen;
  HPEN hwPen  = GetSysColorPen (COLOR_3DHILIGHT);
  HPEN hbPen  = GetSysColorPen (COLOR_3DDKSHADOW);
  HPEN hShade = GetSysColorPen (COLOR_BTNSHADOW);
  RECT rect;

  GetClientRect (hwnd, &rect);

  /*
   * Adjust for the style
   */
  if (GetWindowLongA(hwnd, GWL_STYLE) & TCS_BOTTOM) 
  {
    rect.bottom -= infoPtr->tabHeight;
  }
  else
  {
    rect.top += infoPtr->tabHeight;
  }

  /*
   * Shave-off the right and bottom margins (exluded in the
   * rect)
   */
  rect.right--;
  rect.bottom--;

  /* highlight */
  htmPen = SelectObject (hdc, hwPen);
  
  MoveToEx (hdc, rect.left, rect.bottom, NULL);
  LineTo (hdc, rect.left, rect.top); 
  LineTo (hdc, rect.right, rect.top);

  /* Dark Shadow */
  SelectObject (hdc, hbPen);
  LineTo (hdc, rect.right, rect.bottom );
  LineTo (hdc, rect.left, rect.bottom);

  /* shade */
  SelectObject (hdc, hShade );
  MoveToEx (hdc, rect.right-1, rect.top, NULL);
  LineTo   (hdc, rect.right-1, rect.bottom-1);
  LineTo   (hdc, rect.left,    rect.bottom-1);

  SelectObject(hdc, htmPen);
}

/******************************************************************************
 * TAB_Refresh
 *
 * This method repaints the tab control..
 */             
static void TAB_Refresh (HWND hwnd, HDC hdc)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  HFONT hOldFont;
  INT i;

  if (!infoPtr->DoRedraw)
    return;

  hOldFont = SelectObject (hdc, infoPtr->hFont);

  if (GetWindowLongA(hwnd, GWL_STYLE) & TCS_BUTTONS)
  {
    for (i = 0; i < infoPtr->uNumItem; i++) 
    {
	TAB_DrawItem (hwnd, hdc, i);
    }
  }
  else
  {
    /*
     * Draw all the non selected item first.
     */
    for (i = 0; i < infoPtr->uNumItem; i++) 
    {
      if (i != infoPtr->iSelected)
	TAB_DrawItem (hwnd, hdc, i);
    }

    /*
     * Now, draw the border, draw it before the selected item
     * since the selected item overwrites part of the border.
     */
    TAB_DrawBorder (hwnd, hdc);

    /*
     * Then, draw the selected item
     */
    TAB_DrawItem (hwnd, hdc, infoPtr->iSelected);
    
    /*
     * If we haven't set the current focus yet, set it now.
     * Only happens when we first paint the tab controls.
     */
     if (infoPtr->uFocus == -1)
       TAB_SetCurFocus(hwnd, infoPtr->iSelected); 
  }

  SelectObject (hdc, hOldFont);
}

static LRESULT
TAB_SetRedraw (HWND hwnd, WPARAM wParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  
  infoPtr->DoRedraw=(BOOL) wParam;
  return 0;
}

static LRESULT TAB_EraseBackground(
  HWND hwnd, 
  HDC  givenDC)
{
  HDC  hdc;
  RECT clientRect;

  HBRUSH brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

  hdc = givenDC ? givenDC : GetDC(hwnd);

  GetClientRect(hwnd, &clientRect);

  FillRect(hdc, &clientRect, brush);

  if (givenDC==0)
    ReleaseDC(hwnd, hdc);

  DeleteObject(brush);

  return 0;
}

/******************************************************************************
 * TAB_EnsureSelectionVisible
 *
 * This method will make sure that the current selection is completely
 * visible by scrolling until it is.
 */
static void TAB_EnsureSelectionVisible(
  HWND      hwnd,
  TAB_INFO* infoPtr)
{
  INT iSelected = infoPtr->iSelected;

  /*
   * Do the trivial cases first.
   */
  if ( (!infoPtr->needsScrolling) ||
       (infoPtr->hwndUpDown==0) )
    return;

  if (infoPtr->leftmostVisible >= iSelected)
  {
    infoPtr->leftmostVisible = iSelected;
  }
  else
  {
     RECT r;
     INT  width, i;
     /*
      * Calculate the part of the client area that is visible.
      */
     GetClientRect(hwnd, &r);
     width = r.right;

     GetClientRect(infoPtr->hwndUpDown, &r);
     width -= r.right;

     if ((infoPtr->items[iSelected].rect.right -
          infoPtr->items[iSelected].rect.left) >= width )
     {
        /* Special case: width of selected item is greater than visible
         * part of control.
         */
        infoPtr->leftmostVisible = iSelected;
     }
     else
     {
        for (i = infoPtr->leftmostVisible; i < infoPtr->uNumItem; i++)
        {
           if ((infoPtr->items[iSelected].rect.right -
                infoPtr->items[i].rect.left) < width)
              break;
        }
        infoPtr->leftmostVisible = i;
     }
  }

  SendMessageA(infoPtr->hwndUpDown, UDM_SETPOS, 0,
               MAKELONG(infoPtr->leftmostVisible, 0));
}

/******************************************************************************
 * TAB_InvalidateTabArea
 *
 * This method will invalidate the portion of the control that contains the
 * tabs. It is called when the state of the control changes and needs
 * to be redisplayed
 */
static void TAB_InvalidateTabArea(
  HWND      hwnd,
  TAB_INFO* infoPtr)
{
  RECT clientRect;

  GetClientRect(hwnd, &clientRect);

  if (GetWindowLongA(hwnd, GWL_STYLE) & TCS_BOTTOM) 
  {
    clientRect.top = clientRect.bottom - (infoPtr->tabHeight + 3);
  }
  else
  {
    clientRect.bottom = clientRect.top + (infoPtr->tabHeight + 1);
  }

  InvalidateRect(hwnd, &clientRect, TRUE);
}

static LRESULT
TAB_Paint (HWND hwnd, WPARAM wParam)
{
  HDC hdc;
  PAINTSTRUCT ps;
    
  hdc = wParam== 0 ? BeginPaint (hwnd, &ps) : (HDC)wParam;
  TAB_Refresh (hwnd, hdc);
    
  if(!wParam)
    EndPaint (hwnd, &ps);

  return 0;
}

static LRESULT
TAB_InsertItem (HWND hwnd, WPARAM wParam, LPARAM lParam)
{    
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  TCITEMA *pti;
  INT iItem, len;
  RECT rect;
  
  GetClientRect (hwnd, &rect);
  TRACE("Rect: %x T %i, L %i, B %i, R %i\n", hwnd,
        rect.top, rect.left, rect.bottom, rect.right);  
  
  pti = (TCITEMA *)lParam;
  iItem = (INT)wParam;
  
  if (iItem < 0) return -1;
  if (iItem > infoPtr->uNumItem)
    iItem = infoPtr->uNumItem;
  
  if (infoPtr->uNumItem == 0) {
    infoPtr->items = COMCTL32_Alloc (sizeof (TAB_ITEM));
    infoPtr->uNumItem++;
    infoPtr->iSelected = 0;
  }
  else {
    TAB_ITEM *oldItems = infoPtr->items;
    
    infoPtr->uNumItem++;
    infoPtr->items = COMCTL32_Alloc (sizeof (TAB_ITEM) * infoPtr->uNumItem);
    
    /* pre insert copy */
    if (iItem > 0) {
      memcpy (&infoPtr->items[0], &oldItems[0],
	      iItem * sizeof(TAB_ITEM));
    }
    
    /* post insert copy */
    if (iItem < infoPtr->uNumItem - 1) {
      memcpy (&infoPtr->items[iItem+1], &oldItems[iItem],
	      (infoPtr->uNumItem - iItem - 1) * sizeof(TAB_ITEM));
      
    }

    if (iItem <= infoPtr->iSelected)
      infoPtr->iSelected++;

    COMCTL32_Free (oldItems);
  }
  
  infoPtr->items[iItem].mask = pti->mask;
  if (pti->mask & TCIF_TEXT) {
    len = lstrlenA (pti->pszText);
    infoPtr->items[iItem].pszText = COMCTL32_Alloc (len+1);
    lstrcpyA (infoPtr->items[iItem].pszText, pti->pszText);
    infoPtr->items[iItem].cchTextMax = pti->cchTextMax;
  }
  
  if (pti->mask & TCIF_IMAGE)
    infoPtr->items[iItem].iImage = pti->iImage;
  
  if (pti->mask & TCIF_PARAM)
    infoPtr->items[iItem].lParam = pti->lParam;
  
  TAB_SetItemBounds(hwnd);
  TAB_InvalidateTabArea(hwnd, infoPtr);
  
  TRACE("[%04x]: added item %d '%s'\n",
	hwnd, iItem, infoPtr->items[iItem].pszText);

  return iItem;
}

static LRESULT 
TAB_SetItemSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  LONG lStyle = GetWindowLongA(hwnd, GWL_STYLE);
  LONG lResult = 0;

  if ((lStyle & TCS_FIXEDWIDTH) || (lStyle & TCS_OWNERDRAWFIXED))
  {
    lResult = MAKELONG(infoPtr->tabWidth, infoPtr->tabHeight);
    infoPtr->tabWidth = (INT)LOWORD(lParam);
    infoPtr->tabHeight = (INT)HIWORD(lParam);
  }
  infoPtr->fSizeSet = TRUE;

  return lResult;
}

static LRESULT 
TAB_SetItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  TCITEMA *tabItem; 
  TAB_ITEM *wineItem; 
  INT    iItem,len;

  iItem=(INT) wParam;
  tabItem=(LPTCITEMA ) lParam;
  TRACE("%d %p\n",iItem, tabItem);
  if ((iItem<0) || (iItem>=infoPtr->uNumItem)) return FALSE;

  wineItem=& infoPtr->items[iItem];

  if (tabItem->mask & TCIF_IMAGE)
    wineItem->iImage=tabItem->iImage;

  if (tabItem->mask & TCIF_PARAM)
    wineItem->lParam=tabItem->lParam;

  if (tabItem->mask & TCIF_RTLREADING) 
    FIXME("TCIF_RTLREADING\n");

  if (tabItem->mask & TCIF_STATE) 
    wineItem->dwState=tabItem->dwState;

  if (tabItem->mask & TCIF_TEXT) {
   len=lstrlenA (tabItem->pszText);
   if (len>wineItem->cchTextMax) 
     wineItem->pszText= COMCTL32_ReAlloc (wineItem->pszText, len+1);
   lstrcpyA (wineItem->pszText, tabItem->pszText);
  }

  /*
   * Update and repaint tabs.
   */
  TAB_SetItemBounds(hwnd);
  TAB_InvalidateTabArea(hwnd,infoPtr);

  return TRUE;
}

static LRESULT 
TAB_GetItemCount (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
   TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

   return infoPtr->uNumItem;
}


static LRESULT 
TAB_GetItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
   TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
   TCITEMA *tabItem;
   TAB_ITEM *wineItem;
   INT    iItem;

  iItem=(INT) wParam;
  tabItem=(LPTCITEMA) lParam;
  TRACE("\n");
  if ((iItem<0) || (iItem>=infoPtr->uNumItem)) return FALSE;

  wineItem=& infoPtr->items[iItem];

  if (tabItem->mask & TCIF_IMAGE) 
    tabItem->iImage=wineItem->iImage;

  if (tabItem->mask & TCIF_PARAM) 
    tabItem->lParam=wineItem->lParam;

  if (tabItem->mask & TCIF_RTLREADING) 
    FIXME("TCIF_RTLREADING\n");

  if (tabItem->mask & TCIF_STATE) 
    tabItem->dwState=wineItem->dwState;

  if (tabItem->mask & TCIF_TEXT) 
   lstrcpynA (tabItem->pszText, wineItem->pszText, tabItem->cchTextMax);

  return TRUE;
}

static LRESULT 
TAB_DeleteItem (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  INT iItem = (INT) wParam;
  BOOL bResult = FALSE;

  if ((iItem >= 0) && (iItem < infoPtr->uNumItem))
  {
    TAB_ITEM *oldItems = infoPtr->items;
    
    infoPtr->uNumItem--;
    infoPtr->items = COMCTL32_Alloc(sizeof (TAB_ITEM) * infoPtr->uNumItem);
    
    if (iItem > 0) 
      memcpy(&infoPtr->items[0], &oldItems[0], iItem * sizeof(TAB_ITEM));
    
    if (iItem < infoPtr->uNumItem) 
      memcpy(&infoPtr->items[iItem], &oldItems[iItem + 1],
              (infoPtr->uNumItem - iItem) * sizeof(TAB_ITEM));
    
    COMCTL32_Free (oldItems);

    /*
     * Readjust the selected index.
     */
    if ((iItem == infoPtr->iSelected) && (iItem > 0))
      infoPtr->iSelected--;
      
    if (iItem < infoPtr->iSelected)
      infoPtr->iSelected--;

    if (infoPtr->uNumItem == 0)
      infoPtr->iSelected = -1;

    /*
     * Reposition and repaint tabs.
     */
    TAB_SetItemBounds(hwnd);
    TAB_InvalidateTabArea(hwnd,infoPtr);

    bResult = TRUE;
  }

  return bResult;
}

static LRESULT 
TAB_DeleteAllItems (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
   TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

  COMCTL32_Free (infoPtr->items);
  infoPtr->uNumItem = 0;
  infoPtr->iSelected = -1;
 
  TAB_SetItemBounds(hwnd);
  TAB_InvalidateTabArea(hwnd,infoPtr);
  return TRUE;
}


static LRESULT
TAB_GetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

  TRACE("\n");
  return (LRESULT)infoPtr->hFont;
}

static LRESULT
TAB_SetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  
  TRACE("%x %lx\n",wParam, lParam);
  
  infoPtr->hFont = (HFONT)wParam;
  
  TAB_SetItemBounds(hwnd);

  TAB_InvalidateTabArea(hwnd, infoPtr);

  return 0;
}


static LRESULT
TAB_GetImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

  TRACE("\n");
  return (LRESULT)infoPtr->himl;
}

static LRESULT
TAB_SetImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
    HIMAGELIST himlPrev;

    TRACE("\n");
    himlPrev = infoPtr->himl;
    infoPtr->himl= (HIMAGELIST)lParam;
    return (LRESULT)himlPrev;
}


static LRESULT
TAB_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)

{
/* I'm not really sure what the following code was meant to do.
   This is what it is doing:
   When WM_SIZE is sent with SIZE_RESTORED, the control
   gets positioned in the top left corner.

  RECT parent_rect;
  HWND parent;
  UINT uPosFlags,cx,cy;

  uPosFlags=0;
  if (!wParam) {
    parent = GetParent (hwnd);
    GetClientRect(parent, &parent_rect);
    cx=LOWORD (lParam);
    cy=HIWORD (lParam);
    if (GetWindowLongA(hwnd, GWL_STYLE) & CCS_NORESIZE) 
        uPosFlags |= (SWP_NOSIZE | SWP_NOMOVE);

    SetWindowPos (hwnd, 0, parent_rect.left, parent_rect.top,
            cx, cy, uPosFlags | SWP_NOZORDER);
  } else {
    FIXME (tab,"WM_SIZE flag %x %lx not handled\n", wParam, lParam);
  } */

  /*
   * Recompute the size/position of the tabs.
   */
  TAB_SetItemBounds (hwnd);

  /*
   * Force a repaint of the control.
   */
  InvalidateRect(hwnd, NULL, TRUE);

  return 0;
}


static LRESULT 
TAB_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr;
  TEXTMETRICA fontMetrics;
  HDC hdc;
  HFONT hOldFont;
  DWORD dwStyle;

  infoPtr = (TAB_INFO *)COMCTL32_Alloc (sizeof(TAB_INFO));

  SetWindowLongA(hwnd, 0, (DWORD)infoPtr);
   
  infoPtr->uNumItem        = 0;
  infoPtr->hFont           = 0;
  infoPtr->items           = 0;
  infoPtr->hcurArrow       = LoadCursorA (0, IDC_ARROWA);
  infoPtr->iSelected       = -1;
  infoPtr->uFocus          = -1;  
  infoPtr->hwndToolTip     = 0;
  infoPtr->DoRedraw        = TRUE;
  infoPtr->needsScrolling  = FALSE;
  infoPtr->hwndUpDown      = 0;
  infoPtr->leftmostVisible = 0;
  infoPtr->fSizeSet	   = FALSE;
  
  TRACE("Created tab control, hwnd [%04x]\n", hwnd); 

  /* The tab control always has the WS_CLIPSIBLINGS style. Even 
     if you don't specify in CreateWindow. This is necesary in 
     order for paint to work correctly. This follows windows behaviour. */
  dwStyle = GetWindowLongA(hwnd, GWL_STYLE);
  SetWindowLongA(hwnd, GWL_STYLE, dwStyle|WS_CLIPSIBLINGS);

  if (dwStyle & TCS_TOOLTIPS) {
    /* Create tooltip control */
    infoPtr->hwndToolTip =
      CreateWindowExA (0, TOOLTIPS_CLASSA, NULL, 0,
		       CW_USEDEFAULT, CW_USEDEFAULT,
		       CW_USEDEFAULT, CW_USEDEFAULT,
		       hwnd, 0, 0, 0);
    
    /* Send NM_TOOLTIPSCREATED notification */
    if (infoPtr->hwndToolTip) {
      NMTOOLTIPSCREATED nmttc;
      
      nmttc.hdr.hwndFrom = hwnd;
      nmttc.hdr.idFrom = GetWindowLongA(hwnd, GWL_ID);
      nmttc.hdr.code = NM_TOOLTIPSCREATED;
      nmttc.hwndToolTips = infoPtr->hwndToolTip;
      
      SendMessageA (GetParent (hwnd), WM_NOTIFY,
		    (WPARAM)GetWindowLongA(hwnd, GWL_ID), (LPARAM)&nmttc);
    }
  }  
    
  /*
   * We need to get text information so we need a DC and we need to select
   * a font.
   */
  hdc = GetDC(hwnd); 
  hOldFont = SelectObject (hdc, GetStockObject (SYSTEM_FONT));

  /*
   * Use the system font to determine the initial height of a tab.
   */
  GetTextMetricsA(hdc, &fontMetrics);

  /*
   * Make sure there is enough space for the letters + growing the 
   * selected item + extra space for the selected item.   
   */
  infoPtr->tabHeight = fontMetrics.tmHeight + 2*VERTICAL_ITEM_PADDING +  
                       SELECTED_TAB_OFFSET;

  /*
   * Initialize the width of a tab.
   */
  infoPtr->tabWidth = DEFAULT_TAB_WIDTH;

  SelectObject (hdc, hOldFont);
  ReleaseDC(hwnd, hdc);

  return 0;
}

static LRESULT
TAB_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
  TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);
  INT iItem;

  if (!infoPtr)
      return 0;

  if (infoPtr->items) {
    for (iItem = 0; iItem < infoPtr->uNumItem; iItem++) {
      if (infoPtr->items[iItem].pszText)
	COMCTL32_Free (infoPtr->items[iItem].pszText);
    }
    COMCTL32_Free (infoPtr->items);
  }
  
  if (infoPtr->hwndToolTip) 
    DestroyWindow (infoPtr->hwndToolTip);
 
  if (infoPtr->hwndUpDown)
    DestroyWindow(infoPtr->hwndUpDown);

  COMCTL32_Free (infoPtr);
  return 0;
}

static LRESULT WINAPI
TAB_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case TCM_GETIMAGELIST:
      return TAB_GetImageList (hwnd, wParam, lParam);
      
    case TCM_SETIMAGELIST:
      return TAB_SetImageList (hwnd, wParam, lParam);
      
    case TCM_GETITEMCOUNT:
      return TAB_GetItemCount (hwnd, wParam, lParam);
      
    case TCM_GETITEMA:
      return TAB_GetItemA (hwnd, wParam, lParam);
      
    case TCM_GETITEMW:
      FIXME("Unimplemented msg TCM_GETITEMW\n");
      return 0;
      
    case TCM_SETITEMA:
      return TAB_SetItemA (hwnd, wParam, lParam);
      
    case TCM_SETITEMW:
      FIXME("Unimplemented msg TCM_SETITEMW\n");
      return 0;
      
    case TCM_DELETEITEM:
      return TAB_DeleteItem (hwnd, wParam, lParam);
      
    case TCM_DELETEALLITEMS:
     return TAB_DeleteAllItems (hwnd, wParam, lParam);
     
    case TCM_GETITEMRECT:
     return TAB_GetItemRect (hwnd, wParam, lParam);
      
    case TCM_GETCURSEL:
      return TAB_GetCurSel (hwnd);
      
    case TCM_HITTEST:
      return TAB_HitTest (hwnd, wParam, lParam);
      
    case TCM_SETCURSEL:
      return TAB_SetCurSel (hwnd, wParam);
      
    case TCM_INSERTITEMA:
      return TAB_InsertItem (hwnd, wParam, lParam);
      
    case TCM_INSERTITEMW:
      FIXME("Unimplemented msg TCM_INSERTITEMW\n");
      return 0;
      
    case TCM_SETITEMEXTRA:
      FIXME("Unimplemented msg TCM_SETITEMEXTRA\n");
      return 0;
      
    case TCM_ADJUSTRECT:
      return TAB_AdjustRect (hwnd, (BOOL)wParam, (LPRECT)lParam);
      
    case TCM_SETITEMSIZE:
      return TAB_SetItemSize (hwnd, wParam, lParam);
      
    case TCM_REMOVEIMAGE:
      FIXME("Unimplemented msg TCM_REMOVEIMAGE\n");
      return 0;
      
    case TCM_SETPADDING:
      FIXME("Unimplemented msg TCM_SETPADDING\n");
      return 0;
      
    case TCM_GETROWCOUNT:
      FIXME("Unimplemented msg TCM_GETROWCOUNT\n");
      return 0;

    case TCM_GETUNICODEFORMAT:
      FIXME("Unimplemented msg TCM_GETUNICODEFORMAT\n");
      return 0;

    case TCM_SETUNICODEFORMAT:
      FIXME("Unimplemented msg TCM_SETUNICODEFORMAT\n");
      return 0;

    case TCM_HIGHLIGHTITEM:
      FIXME("Unimplemented msg TCM_HIGHLIGHTITEM\n");
      return 0;
      
    case TCM_GETTOOLTIPS:
      return TAB_GetToolTips (hwnd, wParam, lParam);
      
    case TCM_SETTOOLTIPS:
      return TAB_SetToolTips (hwnd, wParam, lParam);
      
    case TCM_GETCURFOCUS:
      return TAB_GetCurFocus (hwnd);
      
    case TCM_SETCURFOCUS:
      return TAB_SetCurFocus (hwnd, wParam);
      
    case TCM_SETMINTABWIDTH:
      FIXME("Unimplemented msg TCM_SETMINTABWIDTH\n");
      return 0;
      
    case TCM_DESELECTALL:
      FIXME("Unimplemented msg TCM_DESELECTALL\n");
      return 0;
      
    case TCM_GETEXTENDEDSTYLE:
      FIXME("Unimplemented msg TCM_GETEXTENDEDSTYLE\n");
      return 0;

    case TCM_SETEXTENDEDSTYLE:
      FIXME("Unimplemented msg TCM_SETEXTENDEDSTYLE\n");
      return 0;

    case WM_GETFONT:
      return TAB_GetFont (hwnd, wParam, lParam);
      
    case WM_SETFONT:
      return TAB_SetFont (hwnd, wParam, lParam);
      
    case WM_CREATE:
      return TAB_Create (hwnd, wParam, lParam);
      
    case WM_NCDESTROY:
      return TAB_Destroy (hwnd, wParam, lParam);
      
    case WM_GETDLGCODE:
      return DLGC_WANTARROWS | DLGC_WANTCHARS;
      
    case WM_LBUTTONDOWN:
      return TAB_LButtonDown (hwnd, wParam, lParam);
      
    case WM_LBUTTONUP:
      return TAB_LButtonUp (hwnd, wParam, lParam);
      
    case WM_RBUTTONDOWN:
      return TAB_RButtonDown (hwnd, wParam, lParam);
      
    case WM_MOUSEMOVE:
      return TAB_MouseMove (hwnd, wParam, lParam);
      
    case WM_ERASEBKGND:
      return TAB_EraseBackground (hwnd, (HDC)wParam);

    case WM_PAINT:
      return TAB_Paint (hwnd, wParam);

    case WM_SIZE:
      return TAB_Size (hwnd, wParam, lParam);
      
    case WM_SETREDRAW:
      return TAB_SetRedraw (hwnd, wParam);

    case WM_HSCROLL:
      return TAB_OnHScroll(hwnd, (int)LOWORD(wParam), (int)HIWORD(wParam), (HWND)lParam);

    case WM_STYLECHANGED:
      TAB_SetItemBounds (hwnd);
      InvalidateRect(hwnd, NULL, TRUE);
      return 0;
      
    case WM_KILLFOCUS:
    case WM_SETFOCUS:
      return TAB_FocusChanging(hwnd, uMsg, wParam, lParam);

    case WM_KEYUP:
      return TAB_KeyUp(hwnd, wParam);

    default:
      if (uMsg >= WM_USER)
	WARN("unknown msg %04x wp=%08x lp=%08lx\n",
	     uMsg, wParam, lParam);
      return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }

    return 0;
}


VOID
TAB_Register (void)
{
  WNDCLASSA wndClass;

  ZeroMemory (&wndClass, sizeof(WNDCLASSA));
  wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc   = (WNDPROC)TAB_WindowProc;
  wndClass.cbClsExtra    = 0;
  wndClass.cbWndExtra    = sizeof(TAB_INFO *);
  wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
  wndClass.hbrBackground = (HBRUSH)NULL;
  wndClass.lpszClassName = WC_TABCONTROLA;
  
  RegisterClassA (&wndClass);
}


VOID
TAB_Unregister (void)
{
    UnregisterClassA (WC_TABCONTROLA, (HINSTANCE)NULL);
}

