/*
 * Combo controls
 * 
 * Copyright 1993 Martin Ayotte
 * Copyright 1995 Bernd Schmidt
 * Copyright 1996 Albrecht Kleine  [some fixes]
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "windows.h"
#include "sysmetrics.h"
#include "win.h"
#include "combo.h"
#include "user.h"
#include "graphics.h"
#include "heap.h"
#include "listbox.h"
#include "dos_fs.h"
#include "drive.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

 /*
  * Note: Combos are probably implemented in a different way by Windows.
  * Using a message spy for Windows, you can see some undocumented
  * messages being passed between ComboBox and ComboLBox.
  * I hope no programs rely on the implementation of combos.
  */

#define ID_EDIT  1
#define ID_CLB   2
#define CBLMM_EDGE   4    /* distance inside box which is same as moving mouse
			     outside box, to trigger scrolling of CBL */

static BOOL CBCheckSize(HWND hwnd);
static BOOL CBLCheckSize(HWND hwnd);

static HBITMAP16 hComboBit = 0;
static WORD CBitHeight, CBitWidth;

static int COMBO_Init()
{
  BITMAP16 bm;
  
  dprintf_combo(stddeb, "COMBO_Init\n");
  hComboBit = LoadBitmap16(0, MAKEINTRESOURCE(OBM_COMBO));
  GetObject16( hComboBit, sizeof(bm), &bm );
  CBitHeight = bm.bmHeight;
  CBitWidth = bm.bmWidth;
  return 0;
}

LPHEADCOMBO ComboGetStorageHeader(HWND hwnd)
{
  return (LPHEADCOMBO)GetWindowLong32A(hwnd,4);
}

LPHEADLIST ComboGetListHeader(HWND hwnd)
{
  return (LPHEADLIST)GetWindowLong32A(hwnd,0);
}

int CreateComboStruct(HWND hwnd, LONG style)
{
  LPHEADCOMBO lphc;

  lphc = (LPHEADCOMBO)xmalloc(sizeof(HEADCOMBO));
  SetWindowLong32A(hwnd,4,(LONG)lphc);
  lphc->hWndEdit = 0;
  lphc->hWndLBox = 0;
  lphc->dwState = 0;
  lphc->LastSel = -1;
  lphc->dwStyle = style; 
  lphc->DropDownVisible = FALSE;
  return TRUE;
}

void ComboUpdateWindow(HWND hwnd, LPHEADLIST lphl, LPHEADCOMBO lphc, BOOL repaint)
{
  WND *wndPtr = WIN_FindWndPtr(hwnd);

  if (wndPtr->dwStyle & WS_VSCROLL) 
    SetScrollRange32(lphc->hWndLBox,SB_VERT,0,ListMaxFirstVisible(lphl),TRUE);
  if (repaint && lphl->bRedrawFlag) InvalidateRect32( hwnd, NULL, TRUE );
}

/***********************************************************************
 *           CBNCCreate
 */
static LRESULT CBNCCreate(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  CREATESTRUCT16 *createStruct;

  if (!hComboBit) COMBO_Init();

  createStruct = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam);
  createStruct->style |= WS_BORDER;
  SetWindowLong32A(hwnd, GWL_STYLE, createStruct->style);

  dprintf_combo(stddeb,"ComboBox WM_NCCREATE!\n");
  return DefWindowProc16(hwnd, WM_NCCREATE, wParam, lParam);

}

/***********************************************************************
 *           CBCreate
 */
static LRESULT CBCreate(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST   lphl;
  LPHEADCOMBO  lphc;
  LONG         style = 0;
  LONG         cstyle = GetWindowLong32A(hwnd,GWL_STYLE);
  RECT16       rect,lboxrect;
  WND*         wndPtr = WIN_FindWndPtr(hwnd);
  char className[] = "COMBOLBOX";  /* Hack so that class names are > 0x10000 */
  char editName[] = "EDIT";
  HWND hwndp=0;

  /* translate combo into listbox styles */
  cstyle |= WS_BORDER;
  if (cstyle & CBS_OWNERDRAWFIXED) style |= LBS_OWNERDRAWFIXED;
  if (cstyle & CBS_OWNERDRAWVARIABLE) style |= LBS_OWNERDRAWVARIABLE;
  if (cstyle & CBS_SORT) style |= LBS_SORT;
  if (cstyle & CBS_HASSTRINGS) style |= LBS_HASSTRINGS;
  style |= LBS_NOTIFY;
  CreateListBoxStruct(hwnd, ODT_COMBOBOX, style, GetParent16(hwnd));
  CreateComboStruct(hwnd,cstyle);

  lphl = ComboGetListHeader(hwnd);
  lphc = ComboGetStorageHeader(hwnd);

  GetClientRect16(hwnd,&rect);
  lphc->LBoxTop = lphl->StdItemHeight;

  switch(cstyle & 3) 
  {
   case CBS_SIMPLE:            /* edit control, list always visible  */
     lboxrect=rect;    
     dprintf_combo(stddeb,"CBS_SIMPLE\n");
     style= WS_BORDER |  WS_CHILD | WS_VISIBLE | WS_VSCROLL;
     SetRectEmpty16(&lphc->RectButton);
     hwndp=hwnd;
     break;

   case CBS_DROPDOWNLIST:      /* static control, dropdown listbox   */
   case CBS_DROPDOWN:          /* edit control, dropdown listbox     */
     GetWindowRect16(hwnd,&lboxrect);
     style = WS_POPUP | WS_BORDER | WS_VSCROLL;
     /* FIXME: WinSight says these should be CHILD windows with the TOPMOST flag
      * set. Wine doesn't support TOPMOST, and simply setting the WS_CHILD
      * flag doesn't work. */
     lphc->RectButton = rect;
     lphc->RectButton.left = lphc->RectButton.right - 6 - CBitWidth;
     lphc->RectButton.bottom = lphc->RectButton.top + lphl->StdItemHeight;
     SetWindowPos(hwnd, 0, 0, 0, rect.right -rect.left + 2*SYSMETRICS_CXBORDER,
		 lphl->StdItemHeight + 2*SYSMETRICS_CYBORDER,
		 SWP_NOMOVE | SWP_NOZORDER | SWP_NOSENDCHANGING | SWP_NOACTIVATE);
     dprintf_combo(stddeb,(cstyle & 3)==CBS_DROPDOWN ? "CBS_DROPDOWN\n": "CBS_DROPDOWNLIST\n");
     break;
     
   default: fprintf(stderr,"COMBOBOX error: bad class style!\n");
     return 0;
  }

  if ((cstyle & 3) != CBS_DROPDOWNLIST)
      lphc->hWndEdit = CreateWindow16( editName, NULL,
				  WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | ES_LEFT,
				  0, 0, rect.right-6-CBitWidth,
				  lphl->StdItemHeight+2*SYSMETRICS_CYBORDER,
				  hwnd, (HMENU16)ID_EDIT, WIN_GetWindowInstance(hwnd), NULL );
				  
  lboxrect.top+=lphc->LBoxTop;
  lphc->hWndLBox = CreateWindow16( className, NULL, style |
 				((cstyle & WS_HSCROLL)? WS_HSCROLL : 0) |
 				((cstyle & WS_VSCROLL)? WS_VSCROLL : 0),
				lboxrect.left, lboxrect.top,
				lboxrect.right - lboxrect.left, 
				lboxrect.bottom - lboxrect.top,
				hwndp,(HMENU16)ID_CLB, WIN_GetWindowInstance(hwnd),
				(LPVOID)(HWND32)hwnd );

   wndPtr->dwStyle &= ~(WS_VSCROLL | WS_HSCROLL);

   dprintf_combo( stddeb, "Combo Creation hwnd=%04x  LBox=%04x  Edit=%04x\n",
                  hwnd, lphc->hWndLBox, lphc->hWndEdit);
   dprintf_combo( stddeb, "  lbox %d,%d-%d,%d     button %d,%d-%d,%d\n",
                  lboxrect.left, lboxrect.top, lboxrect.right, lboxrect.bottom,
                  lphc->RectButton.left, lphc->RectButton.top,
                  lphc->RectButton.right, lphc->RectButton.bottom );
   dprintf_combo( stddeb, "   client %d,%d-%d,%d     window %d,%d-%d,%d\n",
                  wndPtr->rectClient.left, wndPtr->rectClient.top,
                  wndPtr->rectClient.right, wndPtr->rectClient.bottom,
                  wndPtr->rectWindow.left, wndPtr->rectWindow.top,
                  wndPtr->rectWindow.right, wndPtr->rectWindow.bottom );
   return 0;
}

/***********************************************************************
 *           CBDestroy
 */
static LRESULT CBDestroy(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADCOMBO lphc = ComboGetStorageHeader(hwnd);

  if (lphc->hWndEdit) DestroyWindow( lphc->hWndEdit );
  if (lphc->hWndLBox) DestroyWindow( lphc->hWndLBox );
  return 0;
}

/***********************************************************************
 *           CBPaint
 */
static LRESULT CBPaint(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  LPHEADCOMBO lphc = ComboGetStorageHeader(hwnd);
  LPLISTSTRUCT lpls;
  PAINTSTRUCT16  ps;
  HBRUSH32 hBrush;
  HFONT32 hOldFont;
  HDC16 hdc;
  RECT16 rect;
  
  hdc = BeginPaint16(hwnd, &ps);

  GetClientRect16(hwnd, &rect);
  CBCheckSize(hwnd);
  /* 1 for button border */
  rect.right = lphc->RectButton.left - 1;

  if (hComboBit != 0 && !IsRectEmpty16(&lphc->RectButton))
  {
    Rectangle32(hdc,lphc->RectButton.left-1,lphc->RectButton.top-1,
                lphc->RectButton.right+1,lphc->RectButton.bottom+1);
    {
        RECT32 r;
        CONV_RECT16TO32( &lphc->RectButton, &r );
        GRAPH_DrawReliefRect(hdc, &r, 2, 2, FALSE);
    }
    GRAPH_DrawBitmap(hdc, hComboBit,
		     lphc->RectButton.left + 2,lphc->RectButton.top + 2,
		     0, 0, CBitWidth, CBitHeight );
  }
  if (!IsWindowVisible(hwnd) || !lphl->bRedrawFlag 
      || (lphc->dwStyle & 3) != CBS_DROPDOWNLIST) 
  {
    /* we don't want to draw an entry when there is an edit control */
    EndPaint16(hwnd, &ps);
    return 0;
  }

  hOldFont = SelectObject32(hdc, lphl->hFont);

  hBrush = SendMessage32A( lphl->hParent, WM_CTLCOLORLISTBOX, hdc, hwnd );
  if (hBrush == 0) hBrush = GetStockObject32(WHITE_BRUSH);

  lpls = ListBoxGetItem(lphl,lphl->ItemFocused);
  if (lpls != NULL) {  
    FillRect16(hdc, &rect, hBrush);
    ListBoxDrawItem (hwnd, lphl, hdc, lpls, &rect, ODA_DRAWENTIRE, 0);
    if (GetFocus32() == hwnd)
    ListBoxDrawItem (hwnd,lphl, hdc, lpls, &rect, ODA_FOCUS, ODS_FOCUS);
  }
  else FillRect16(hdc, &rect, hBrush);
  SelectObject32(hdc,hOldFont);
  EndPaint16(hwnd, &ps);
  return 0;
}

/***********************************************************************
 *           CBGetDlgCode
 */
static LRESULT CBGetDlgCode(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  return DLGC_WANTARROWS | DLGC_WANTCHARS;
}

/***********************************************************************
 *           CBLButtonDown
 */
static LRESULT CBLButtonDown(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADCOMBO lphc = ComboGetStorageHeader(hwnd);
  SendMessage16(hwnd,CB_SHOWDROPDOWN,!lphc->DropDownVisible,0);
  return 0;
}

/***********************************************************************
 *           CBKeyDown
 */
static LRESULT CBKeyDown(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  WORD       newFocused = lphl->ItemFocused;

  switch(wParam) {
  case VK_HOME:
    newFocused = 0;
    break;
  case VK_END:
    newFocused = lphl->ItemsCount - 1;
    break;
  case VK_UP:
    if (newFocused > 0) newFocused--;
    break;
  case VK_DOWN:
    newFocused++;
    break;
  default:
    return 0;
  }

  if (newFocused >= lphl->ItemsCount)
    newFocused = lphl->ItemsCount - 1;
  
  ListBoxSetCurSel(lphl, newFocused);
  SendMessage16(hwnd, WM_COMMAND,ID_CLB,MAKELONG(0,CBN_SELCHANGE));
  ListBoxSendNotification(lphl, CBN_SELCHANGE);

  lphl->ItemFocused = newFocused;
  ListBoxScrollToFocus(lphl);
/*  SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);*/
  InvalidateRect32( hwnd, NULL, TRUE );

  return 0;
}

/***********************************************************************
 *           CBChar
 */
static LRESULT CBChar(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  WORD       newFocused;

  newFocused = ListBoxFindNextMatch(lphl, wParam);
  if (newFocused == (WORD)LB_ERR) return 0;

  if (newFocused >= lphl->ItemsCount)
    newFocused = lphl->ItemsCount - 1;
  
  ListBoxSetCurSel(lphl, newFocused);
  SendMessage16(hwnd, WM_COMMAND,ID_CLB,MAKELONG(0,CBN_SELCHANGE));
  ListBoxSendNotification(lphl, CBN_SELCHANGE);
  lphl->ItemFocused = newFocused;
  ListBoxScrollToFocus(lphl);
  
/*  SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);*/
  InvalidateRect32( hwnd, NULL, TRUE );

  return 0;
}

/***********************************************************************
 *           CBKillFocus
 */
static LRESULT CBKillFocus(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  return 0;
}

/***********************************************************************
 *           CBSetFocus
 */
static LRESULT CBSetFocus(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  return 0;
}

/***********************************************************************
 *           CBResetContent
 */
static LRESULT CBResetContent(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  LPHEADCOMBO lphc = ComboGetStorageHeader(hwnd);

  ListBoxResetContent(lphl);
  ComboUpdateWindow(hwnd, lphl, lphc, TRUE);
  return 0;
}

/***********************************************************************
 *           CBDir
 */
static LRESULT CBDir(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  WORD wRet;
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  LPHEADCOMBO lphc = ComboGetStorageHeader(hwnd);

  wRet = ListBoxDirectory(lphl, wParam, (LPSTR)PTR_SEG_TO_LIN(lParam));
  ComboUpdateWindow(hwnd, lphl, lphc, TRUE);
  return wRet;
}

/***********************************************************************
 *           CBInsertString
 */
static LRESULT CBInsertString(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  WORD  wRet;
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  LPHEADCOMBO lphc = ComboGetStorageHeader(hwnd);

  if (lphl->HasStrings)
    wRet = ListBoxInsertString(lphl, wParam, (LPSTR)PTR_SEG_TO_LIN(lParam));
  else
    wRet = ListBoxInsertString(lphl, wParam, (LPSTR)lParam);
  ComboUpdateWindow(hwnd, lphl, lphc, TRUE);
  return wRet;
}

/***********************************************************************
 *           CBAddString
 */
static LRESULT CBAddString(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  WORD  wRet;
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  LPHEADCOMBO lphc = ComboGetStorageHeader(hwnd);

  wRet = ListBoxAddString(lphl, (SEGPTR)lParam);

  ComboUpdateWindow(hwnd, lphl, lphc, TRUE);
  return wRet;
}

/***********************************************************************
 *           CBDeleteString
 */
static LRESULT CBDeleteString(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  LPHEADCOMBO lphc = ComboGetStorageHeader(hwnd);
  LONG lRet = ListBoxDeleteString(lphl,wParam);
  
  ComboUpdateWindow(hwnd, lphl, lphc, TRUE);
  return lRet;
}

/***********************************************************************
 *           CBSelectString
 */
static LRESULT CBSelectString(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  WORD  wRet;

  wRet = ListBoxFindString(lphl, wParam, (SEGPTR)lParam);

  /* XXX add functionality here */

  return 0;
}

/***********************************************************************
 *           CBFindString
 */
static LRESULT CBFindString(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  return ListBoxFindString(lphl, wParam, (SEGPTR)lParam);
}

/***********************************************************************
 *           CBFindStringExact
 */
static LRESULT CBFindStringExact(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  return ListBoxFindStringExact(lphl, wParam, (SEGPTR)lParam);
}

/***********************************************************************
 *           CBGetCount
 */
static LRESULT CBGetCount(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  return lphl->ItemsCount;
}

/***********************************************************************
 *           CBSetCurSel
 */
static LRESULT CBSetCurSel(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  WORD  wRet;

  wRet = ListBoxSetCurSel(lphl, wParam);

  dprintf_combo(stddeb,"CBSetCurSel: hwnd %04x wp %x lp %lx wRet %d\n",
		hwnd,wParam,lParam,wRet);
/*  SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);*/
  InvalidateRect32( hwnd, NULL, TRUE );

  return wRet;
}

/***********************************************************************
 *           CBGetCurSel
 */
static LRESULT CBGetCurSel(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  return lphl->ItemFocused;
}

/***********************************************************************
 *           CBGetItemHeight
 */
static LRESULT CBGetItemHeight(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  LPLISTSTRUCT lpls = ListBoxGetItem (lphl, wParam);

  if (lpls == NULL) return LB_ERR;
  return lpls->mis.itemHeight;
}

/***********************************************************************
 *           CBSetItemHeight
 */
static LRESULT CBSetItemHeight(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  return ListBoxSetItemHeight(lphl, wParam, lParam);
}

/***********************************************************************
 *           CBSetRedraw
 */
static LRESULT CBSetRedraw(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  lphl->bRedrawFlag = wParam;
  return 0;
}

/***********************************************************************
 *           CBSetFont
 */
static LRESULT CBSetFont(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST  lphl = ComboGetListHeader(hwnd);
  LPHEADCOMBO lphc = ComboGetStorageHeader(hwnd);
  
  if (wParam == 0)
    lphl->hFont = GetStockObject32(SYSTEM_FONT);
  else
    lphl->hFont = (HFONT16)wParam;
  if (lphc->hWndEdit)
     SendMessage16(lphc->hWndEdit,WM_SETFONT,lphl->hFont,0); 
  return 0;
}

/***********************************************************************
 *           CBGetLBTextLen
 */
static LRESULT CBGetLBTextLen(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST   lphl = ComboGetListHeader(hwnd);
  LPLISTSTRUCT lpls = ListBoxGetItem(lphl,wParam);

  if (lpls == NULL || !lphl->HasStrings) return LB_ERR;
  return strlen(lpls->itemText);
}

/***********************************************************************
 *           CBGetLBText
 */
static LRESULT CBGetLBText(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  return ListBoxGetText(lphl, wParam, (LPSTR)PTR_SEG_TO_LIN(lParam));
}

/***********************************************************************
 *           CBGetItemData
 */
static LRESULT CBGetItemData(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  return ListBoxGetItemData(lphl, wParam);
}

/***********************************************************************
 *           CBSetItemData
 */
static LRESULT CBSetItemData(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  return ListBoxSetItemData(lphl, wParam, lParam);
}

/***********************************************************************
 *           CBShowDropDown
 */
static LRESULT CBShowDropDown(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADCOMBO lphc = ComboGetStorageHeader(hwnd);
  RECT32 rect;
  
  if ((lphc->dwStyle & 3) == CBS_SIMPLE) return LB_ERR;
  
  wParam = !!wParam;
  if (wParam != lphc->DropDownVisible) {
    lphc->DropDownVisible = wParam;
    GetWindowRect32(hwnd,&rect);
    SetWindowPos(lphc->hWndLBox, 0, rect.left, rect.top+lphc->LBoxTop, 0, 0,
		 SWP_NOSIZE | SWP_NOACTIVATE |
                 (wParam ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
    if (!wParam) SetFocus32(hwnd);
  }
  return 0;
}


/***********************************************************************
 *             CBCheckSize
 */
static BOOL CBCheckSize(HWND hwnd)
{
  LPHEADCOMBO  lphc = ComboGetStorageHeader(hwnd);
  LPHEADLIST   lphl = ComboGetListHeader(hwnd);
  LONG         cstyle = GetWindowLong32A(hwnd,GWL_STYLE);
  RECT16       cRect, wRect;

  if (lphc->hWndLBox == 0) return FALSE;

  GetClientRect16(hwnd,&cRect);
  GetWindowRect16(hwnd,&wRect);

  dprintf_combo(stddeb,
	 "CBCheckSize: hwnd %04x Rect %d,%d-%d,%d  wRect %d,%d-%d,%d\n", 
		hwnd,cRect.left,cRect.top,cRect.right,cRect.bottom,
		wRect.left,wRect.top,wRect.right,wRect.bottom);
  if ((cstyle & 3) == CBS_SIMPLE) return TRUE;

  if ((cRect.bottom - cRect.top) >
      (lphl->StdItemHeight + 2*SYSMETRICS_CYBORDER)) {
    SetWindowPos(hwnd, 0, 0, 0, 
		 cRect.right-cRect.left,
		 lphl->StdItemHeight+2*SYSMETRICS_CYBORDER, 
		 SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE );
    GetClientRect16(hwnd,&cRect);
    GetWindowRect16(hwnd,&wRect);

    lphc->RectButton.right = cRect.right;
    lphc->RectButton.left = cRect.right - 2*SYSMETRICS_CXBORDER - 4 
                            - CBitWidth;
    lphc->RectButton.top = cRect.top;
    lphc->RectButton.bottom = cRect.bottom;
  }

  if (cRect.right < lphc->RectButton.left) {
    /* if the button is outside the window move it in */
    if ((wRect.right - wRect.left - 2*SYSMETRICS_CXBORDER) == (cRect.right - cRect.left)) {
      lphc->RectButton.right = cRect.right;
      lphc->RectButton.left = cRect.right - 2*SYSMETRICS_CXBORDER - 4 
	                       - CBitWidth;
      lphc->RectButton.top = cRect.top;
      lphc->RectButton.bottom = cRect.bottom;
    }
    /* otherwise we need to make the client include the button */
    else
      SetWindowPos(hwnd, 0, 0, 0, lphc->RectButton.right,
		   lphl->StdItemHeight+2*SYSMETRICS_CYBORDER,
		   SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE);

      if ((lphc->dwStyle & 3) != CBS_DROPDOWNLIST)
        SetWindowPos(lphc->hWndEdit, 0, 0, 0, lphc->RectButton.left,
		     lphl->StdItemHeight,
		     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
  }

  CBLCheckSize(hwnd);
  return TRUE;
}

/***********************************************************************
 *           CBCommand        
 */
static LRESULT CBCommand(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
  LPHEADCOMBO lphc = ComboGetStorageHeader(hwnd);
  LPHEADLIST lphl = ComboGetListHeader(hwnd);
  char       buffer[256];
  WORD       newFocused;
  WORD       id;
  if (lphc->hWndEdit)  /* interdependence only used for CBS_SIMPLE and CBS_DROPDOWN styles */
  {
   switch (wParam)
   {
    case ID_CLB:                                        /* update EDIT window */
                if (HIWORD(lParam)==CBN_SELCHANGE)
                 if (lphl->HasStrings)
                 { 
                  ListBoxGetText(lphl,lphl->ItemFocused, buffer);
                  dprintf_combo(stddeb,"CBCommand: update Edit: %s\n",buffer);
                  SetWindowText32A( lphc->hWndEdit, buffer );
                 }
                break;
    case ID_EDIT:                                      /* update LISTBOX window */
                 id=GetWindowWord(hwnd,GWW_ID);
                 switch (HIWORD(lParam))
                 {
                  case EN_UPDATE:GetWindowText32A(lphc->hWndEdit,buffer,255);
                                 if (*buffer)
                                 {
                                  char *str = SEGPTR_STRDUP(buffer);
                                  newFocused=ListBoxFindString(lphl, -1, SEGPTR_GET(str));
                                  SEGPTR_FREE(str);
                                  dprintf_combo(stddeb,"CBCommand: new selection #%d is= %s\n",
                                                newFocused,buffer);
                                  if (newFocused != (WORD)LB_ERR)
                                  {                             /* if found something */
                                   ListBoxSetCurSel(lphl, newFocused);
                                   ListBoxSendNotification(lphl, CBN_SELCHANGE);
                                   InvalidateRect32(hwnd, NULL, TRUE); 
                                  }
                                 }
                                 SendMessage16(GetParent16(hwnd),WM_COMMAND,id,
                                         MAKELONG(hwnd, CBN_EDITUPDATE));
                                 break;
                  case EN_CHANGE:SendMessage16(GetParent16(hwnd),WM_COMMAND,id,
                                         MAKELONG(hwnd, CBN_EDITCHANGE));
                                 break;
                  case EN_ERRSPACE:SendMessage16(GetParent16(hwnd),WM_COMMAND,id,
                                         MAKELONG(hwnd, CBN_ERRSPACE));
                                 break;
                }
                break;        
   }
  } 
  return 0;
}


/***********************************************************************
 *           CBGetEditSel
 * Look out! Under Win32, the parameter packing is very different.
 */
static LRESULT CBGetEditSel(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
    LPHEADCOMBO lphc = ComboGetStorageHeader(hwnd);

    if ((lphc->dwStyle & 3) == CBS_DROPDOWNLIST)
      return CB_ERR;	/* err, documented for CBSetEditSel */
    return SendMessage16(lphc->hWndEdit, EM_GETSEL, 0, 0);
}


/***********************************************************************
 *           CBSetEditSel
 * Look out! Under Win32, the parameter packing is very different.
 */
static LRESULT CBSetEditSel(HWND hwnd, WPARAM16 wParam, LPARAM lParam)
{
    LPHEADCOMBO lphc = ComboGetStorageHeader(hwnd);

    if ((lphc->dwStyle & 3) == CBS_DROPDOWNLIST)
      return CB_ERR;
    return SendMessage16(lphc->hWndEdit, EM_SETSEL, 0, lParam);
}


/***********************************************************************
 *           ComboWndProc
 */
LRESULT ComboBoxWndProc(HWND hwnd, UINT message, WPARAM16 wParam, LPARAM lParam)
{
    switch(message) {	
     case WM_NCCREATE: return CBNCCreate(hwnd, wParam, lParam);
     case WM_CREATE: return CBCreate(hwnd, wParam, lParam);
     case WM_DESTROY: return CBDestroy(hwnd, wParam, lParam);
     case WM_GETDLGCODE: return CBGetDlgCode(hwnd, wParam, lParam);
     case WM_KEYDOWN: return CBKeyDown(hwnd, wParam, lParam);
     case WM_CHAR: return CBChar(hwnd, wParam, lParam);
     case WM_SETFONT: return CBSetFont(hwnd, wParam, lParam);
     case WM_SETREDRAW: return CBSetRedraw(hwnd, wParam, lParam);
     case WM_PAINT: return CBPaint(hwnd, wParam, lParam);
     case WM_LBUTTONDOWN: return CBLButtonDown(hwnd, wParam, lParam);
     case WM_SETFOCUS: return CBSetFocus(hwnd, wParam, lParam);
     case WM_KILLFOCUS: return CBKillFocus(hwnd, wParam, lParam);
     case WM_SIZE: return CBCheckSize(hwnd);
     case WM_COMMAND: return CBCommand(hwnd, wParam, lParam);
     case CB_RESETCONTENT: return CBResetContent(hwnd, wParam, lParam);
     case CB_DIR: return CBDir(hwnd, wParam, lParam);
     case CB_ADDSTRING: return CBAddString(hwnd, wParam, lParam);
     case CB_INSERTSTRING: return CBInsertString(hwnd, wParam, lParam);
     case CB_DELETESTRING: return CBDeleteString(hwnd, wParam, lParam);
     case CB_FINDSTRING: return CBFindString(hwnd, wParam, lParam);
     case CB_GETCOUNT: return CBGetCount(hwnd, wParam, lParam);
     case CB_GETCURSEL: return CBGetCurSel(hwnd, wParam, lParam);
     case CB_GETITEMDATA: return CBGetItemData(hwnd, wParam, lParam);
     case CB_GETITEMHEIGHT: return CBGetItemHeight(hwnd, wParam, lParam);
     case CB_GETLBTEXT: return CBGetLBText(hwnd, wParam, lParam);
     case CB_GETLBTEXTLEN: return CBGetLBTextLen(hwnd, wParam, lParam);
     case CB_SELECTSTRING: return CBSelectString(hwnd, wParam, lParam);
     case CB_SETITEMDATA: return CBSetItemData(hwnd, wParam, lParam);
     case CB_SETCURSEL: return CBSetCurSel(hwnd, wParam, lParam);
     case CB_SETITEMHEIGHT: return CBSetItemHeight(hwnd, wParam, lParam);
     case CB_SHOWDROPDOWN: return CBShowDropDown(hwnd, wParam, lParam);
     case CB_GETEDITSEL: return CBGetEditSel(hwnd, wParam, lParam);
     case CB_SETEDITSEL: return CBSetEditSel(hwnd, wParam, lParam);
     case CB_FINDSTRINGEXACT: return CBFindStringExact(hwnd, wParam, lParam);
    }
    return DefWindowProc16(hwnd, message, wParam, lParam);
}

/*--------------------------------------------------------------------*/
/* ComboLBox code starts here */

HWND CLBoxGetCombo(HWND hwnd)
{
  return (HWND)GetWindowLong32A(hwnd,0);
}

LPHEADLIST CLBoxGetListHeader(HWND hwnd)
{
  return ComboGetListHeader(CLBoxGetCombo(hwnd));
}

/***********************************************************************
 *           CBLCreate
 */
static LRESULT CBLCreate( HWND hwnd, WPARAM16 wParam, LPARAM lParam )
{
  CREATESTRUCT16 *createStruct = (CREATESTRUCT16 *)PTR_SEG_TO_LIN(lParam);
  SetWindowLong32A(hwnd,0,(LONG)createStruct->lpCreateParams);
  return 0;
}

/***********************************************************************
 *           CBLGetDlgCode
 */
static LRESULT CBLGetDlgCode( HWND hwnd, WPARAM16 wParam, LPARAM lParam )
{
  return DLGC_WANTARROWS | DLGC_WANTCHARS;
}

/***********************************************************************
 *           CBLKeyDown
 */
static LRESULT CBLKeyDown( HWND hwnd, WPARAM16 wParam, LPARAM lParam ) 
{
  LPHEADLIST lphl = CLBoxGetListHeader(hwnd);
  WORD newFocused = lphl->ItemFocused;

  switch(wParam) {
  case VK_HOME:
    newFocused = 0;
    break;
  case VK_END:
    newFocused = lphl->ItemsCount - 1;
    break;
  case VK_UP:
    if (newFocused > 0) newFocused--;
    break;
  case VK_DOWN:
    newFocused++;
    break;
  case VK_PRIOR:
    if (newFocused > lphl->ItemsVisible) {
      newFocused -= lphl->ItemsVisible;
    } else {
      newFocused = 0;
    }
    break;
  case VK_NEXT:
    newFocused += lphl->ItemsVisible;
    break;
  default:
    return 0;
  }

  if (newFocused >= lphl->ItemsCount)
    newFocused = lphl->ItemsCount - 1;
  
  ListBoxSetCurSel(lphl, newFocused);
  ListBoxSendNotification(lphl, CBN_SELCHANGE);
  SendMessage16(GetParent16(hwnd), WM_COMMAND,ID_CLB,MAKELONG(0,CBN_SELCHANGE));
  lphl->ItemFocused = newFocused;
  ListBoxScrollToFocus(lphl);
  SetScrollPos32(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
  InvalidateRect32( hwnd, NULL, TRUE );
  return 0;
}

/***********************************************************************
 *           CBLChar
 */
static LRESULT CBLChar( HWND hwnd, WPARAM16 wParam, LPARAM lParam )
{
  return 0;
}

/***********************************************************************
 *           CBLPaint
 */
static LRESULT CBLPaint( HWND hwnd, WPARAM16 wParam, LPARAM lParam )
{
  LPHEADLIST   lphl = CLBoxGetListHeader(hwnd);
  LPLISTSTRUCT lpls;
  PAINTSTRUCT16  ps;
  HBRUSH32 hBrush;
  HFONT32 hOldFont;
  WND * wndPtr = WIN_FindWndPtr(hwnd);
  HWND  combohwnd = CLBoxGetCombo(hwnd);
  HDC16 hdc;
  RECT16 rect;
  int   i, top, height;

  top = 0;
  hdc = BeginPaint16( hwnd, &ps );

  if (!IsWindowVisible(hwnd) || !lphl->bRedrawFlag) {
    EndPaint16(hwnd, &ps);
    return 0;
  }

  hOldFont = SelectObject32(hdc, lphl->hFont);
  /* listboxes should be white */
  hBrush = GetStockObject32(WHITE_BRUSH);

  GetClientRect16(hwnd, &rect);
  FillRect16(hdc, &rect, hBrush);
  CBLCheckSize(hwnd);

  lpls = lphl->lpFirst;

  lphl->ItemsVisible = 0;
  for(i = 0; i < lphl->ItemsCount; i++) {
    if (lpls == NULL) break;

    if (i >= lphl->FirstVisible) {
      height = lpls->mis.itemHeight;
      /* must have enough room to draw entire item */
      if (top > (rect.bottom-height+1)) break;

      lpls->itemRect.top    = top;
      lpls->itemRect.bottom = top + height;
      lpls->itemRect.left   = rect.left;
      lpls->itemRect.right  = rect.right;

      dprintf_listbox(stddeb,"drawing item: %d %d %d %d %d\n",
                      rect.left,top,rect.right,top+height,lpls->itemState);
      if (lphl->OwnerDrawn) {
	ListBoxDrawItem (combohwnd, lphl, hdc, lpls, &lpls->itemRect, ODA_DRAWENTIRE, 0);
	if (lpls->itemState)
	  ListBoxDrawItem (combohwnd, lphl, hdc, lpls, &lpls->itemRect, ODA_SELECT, ODS_SELECTED);
      } else {
	ListBoxDrawItem (combohwnd, lphl, hdc, lpls, &lpls->itemRect, ODA_DRAWENTIRE, 
			 lpls->itemState);
      }
      if ((lphl->ItemFocused == i) && GetFocus32() == hwnd)
	ListBoxDrawItem (combohwnd, lphl, hdc, lpls, &lpls->itemRect, ODA_FOCUS, ODS_FOCUS);

      top += height;
      lphl->ItemsVisible++;
    }

    lpls = lpls->lpNext;
  }

  if (wndPtr->dwStyle & WS_VSCROLL) 
      SetScrollRange32(hwnd, SB_VERT, 0, ListMaxFirstVisible(lphl), TRUE);

  SelectObject32(hdc,hOldFont);
  EndPaint16( hwnd, &ps );
  return 0;

}

/***********************************************************************
 *           CBLKillFocus
 */
static LRESULT CBLKillFocus( HWND hwnd, WPARAM16 wParam, LPARAM lParam )
{
/*  SendMessage16(CLBoxGetCombo(hwnd),CB_SHOWDROPDOWN,0,0);*/
  return 0;
}

/***********************************************************************
 *           CBLActivate
 */
static LRESULT CBLActivate( HWND hwnd, WPARAM16 wParam, LPARAM lParam )
{
  if (wParam == WA_INACTIVE)
    SendMessage16(CLBoxGetCombo(hwnd),CB_SHOWDROPDOWN,0,0);
  return 0;
}

/***********************************************************************
 *           CBLLButtonDown
 */
static LRESULT CBLLButtonDown( HWND hwnd, WPARAM16 wParam, LPARAM lParam )
{
  LPHEADLIST lphl = CLBoxGetListHeader(hwnd);
  int        y;
  RECT16     rectsel;

/*  SetFocus32(hwnd); */
  SetCapture32(hwnd);

  lphl->PrevFocused = lphl->ItemFocused;

  y = ListBoxFindMouse(lphl, LOWORD(lParam), HIWORD(lParam));
  if (y == -1)
    return 0;

  ListBoxSetCurSel(lphl, y);
  ListBoxGetItemRect(lphl, y, &rectsel);

  InvalidateRect32( hwnd, NULL, TRUE );
  return 0;
}

/***********************************************************************
 *           CBLLButtonUp
 */
static LRESULT CBLLButtonUp( HWND hwnd, WPARAM16 wParam, LPARAM lParam )
{
  LPHEADLIST lphl = CLBoxGetListHeader(hwnd);

  if (GetCapture32() == hwnd) ReleaseCapture();

  if(!lphl)
     {
      fprintf(stdnimp,"CBLLButtonUp: CLBoxGetListHeader returned NULL!\n");
     }
  else if (lphl->PrevFocused != lphl->ItemFocused) 
          {
      		SendMessage16(CLBoxGetCombo(hwnd),CB_SETCURSEL,lphl->ItemFocused,0);
      		SendMessage16(GetParent16(hwnd), WM_COMMAND,ID_CLB,MAKELONG(0,CBN_SELCHANGE));
      		ListBoxSendNotification(lphl, CBN_SELCHANGE);
     	  }

  SendMessage16(CLBoxGetCombo(hwnd),CB_SHOWDROPDOWN,0,0);

  return 0;
}

/***********************************************************************
 *           CBLMouseMove
 */
static LRESULT CBLMouseMove( HWND hwnd, WPARAM16 wParam, LPARAM lParam )
{
  LPHEADLIST lphl = CLBoxGetListHeader(hwnd);
  short y;
  WORD wRet;
  RECT16 rect, rectsel;

  y = SHIWORD(lParam);
  wRet = ListBoxFindMouse(lphl, LOWORD(lParam), HIWORD(lParam));
  ListBoxGetItemRect(lphl, wRet, &rectsel);
  GetClientRect16(hwnd, &rect);

  dprintf_combo(stddeb,"CBLMouseMove: hwnd %04x wp %x lp %lx  y %d  if %d wret %d %d,%d-%d,%d\n",
hwnd,wParam,lParam,y,lphl->ItemFocused,wRet,rectsel.left,rectsel.top,rectsel.right,rectsel.bottom);
  
  if ((wParam & MK_LBUTTON) != 0) {
    if (y < CBLMM_EDGE) {
      if (lphl->FirstVisible > 0) {
	lphl->FirstVisible--;
	SetScrollPos32(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
	ListBoxSetCurSel(lphl, wRet);
	InvalidateRect32( hwnd, NULL, TRUE );
	return 0;
      }
    }
    else if (y >= (rect.bottom-CBLMM_EDGE)) {
      if (lphl->FirstVisible < ListMaxFirstVisible(lphl)) {
	lphl->FirstVisible++;
	SetScrollPos32(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
	ListBoxSetCurSel(lphl, wRet);
	InvalidateRect32( hwnd, NULL, TRUE );
	return 0;
      }
    }
    else {
      if ((short) wRet == lphl->ItemFocused) return 0;
      ListBoxSetCurSel(lphl, wRet);
      InvalidateRect32( hwnd, NULL, TRUE );
    }
  }

  return 0;
}

/***********************************************************************
 *           CBLVScroll
 */
static LRESULT CBLVScroll( HWND hwnd, WPARAM16 wParam, LPARAM lParam )
{
  LPHEADLIST lphl = CLBoxGetListHeader(hwnd);
  int  y;

  y = lphl->FirstVisible;

  switch(wParam) {
  case SB_LINEUP:
    if (lphl->FirstVisible > 0)
      lphl->FirstVisible--;
    break;

  case SB_LINEDOWN:
    lphl->FirstVisible++;
    break;

  case SB_PAGEUP:
    if (lphl->FirstVisible > lphl->ItemsVisible) {
      lphl->FirstVisible -= lphl->ItemsVisible;
    } else {
      lphl->FirstVisible = 0;
    }
    break;

  case SB_PAGEDOWN:
    lphl->FirstVisible += lphl->ItemsVisible;
    break;

  case SB_THUMBTRACK:
    lphl->FirstVisible = LOWORD(lParam);
    break;
  }

  if (lphl->FirstVisible > ListMaxFirstVisible(lphl))
    lphl->FirstVisible = ListMaxFirstVisible(lphl);

  if (y != lphl->FirstVisible) {
    SetScrollPos32(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
    InvalidateRect32( hwnd, NULL, TRUE );
  }

  return 0;
}


/***********************************************************************
 *             CBLCheckSize
 */
static BOOL CBLCheckSize(HWND hwnd)
{
  LPHEADCOMBO  lphc = ComboGetStorageHeader(hwnd);
  LPHEADLIST   lphl = ComboGetListHeader(hwnd);
  LPLISTSTRUCT lpls;
  HWND         hWndLBox;
  RECT16 cRect,wRect,lRect,lwRect;
  int totheight,dw;
  char className[80];

  GetClassName32A(hwnd,className,80);
  fflush(stddeb);
  if (strncmp(className,"COMBOBOX",8)) return FALSE;
  if ((hWndLBox = lphc->hWndLBox) == 0) return FALSE;
  dprintf_combo(stddeb,"CBLCheckSize headers hw %04x  lb %04x  name %s\n",
		hwnd,hWndLBox,className);

  GetClientRect16(hwnd,&cRect);
  GetWindowRect16(hwnd,&wRect);
  GetClientRect16(hWndLBox,&lRect);
  GetWindowRect16(hWndLBox,&lwRect);

  dprintf_combo(stddeb,"CBLCheckSize: init cRect %d,%d-%d,%d  wRect %d,%d-%d,%d\n",
		cRect.left,cRect.top,cRect.right,cRect.bottom,
		wRect.left,wRect.top,wRect.right,wRect.bottom);
  dprintf_combo(stddeb,"                   lRect %d,%d-%d,%d  lwRect %d,%d-%d,%d\n",
	      lRect.left,lRect.top,lRect.right,lRect.bottom,
	      lwRect.left,lwRect.top,lwRect.right,lwRect.bottom);
  fflush(stddeb);
  
  totheight = 0;
  for (lpls=lphl->lpFirst; lpls != NULL; lpls=lpls->lpNext)
    totheight += lpls->mis.itemHeight;

  dw = cRect.right-cRect.left+2*SYSMETRICS_CXBORDER+SYSMETRICS_CXVSCROLL;
  dw -= lwRect.right-lwRect.left;
  dw -= SYSMETRICS_CXVSCROLL;

  /* TODO: This isn't really what windows does */
  if ((lRect.bottom-lRect.top < 3*lphl->StdItemHeight) || dw) {
    dprintf_combo(stddeb,"    Changing; totHeight %d  StdItemHght %d  dw %d\n",
		  totheight,lphl->StdItemHeight,dw);
    SetWindowPos(hWndLBox, 0, lRect.left, lRect.top, 
		 lwRect.right-lwRect.left+dw, totheight+2*SYSMETRICS_CYBORDER, 
		 SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE );
  }
  return TRUE;
}


/***********************************************************************
 *           ComboLBoxWndProc
 */
LRESULT ComboLBoxWndProc(HWND hwnd, UINT message, WPARAM16 wParam, LPARAM lParam)
{
    switch(message) {	
     case WM_CREATE: return CBLCreate(hwnd, wParam, lParam);
     case WM_GETDLGCODE: return CBLGetDlgCode(hwnd, wParam, lParam);
     case WM_KEYDOWN: return CBLKeyDown(hwnd, wParam, lParam);
     case WM_CHAR: return CBLChar(hwnd, wParam, lParam);
     case WM_PAINT: return CBLPaint(hwnd, wParam, lParam);
     case WM_KILLFOCUS: return CBLKillFocus(hwnd, wParam, lParam);
     case WM_ACTIVATE: return CBLActivate(hwnd, wParam, lParam);
     case WM_LBUTTONDOWN: return CBLLButtonDown(hwnd, wParam, lParam);
     case WM_LBUTTONUP: return CBLLButtonUp(hwnd, wParam, lParam);
     case WM_MOUSEMOVE: return CBLMouseMove(hwnd, wParam, lParam);
     case WM_VSCROLL: return CBLVScroll(hwnd, wParam, lParam);
     case WM_SIZE: return CBLCheckSize(hwnd);
     case WM_MOUSEACTIVATE:  /* We don't want to be activated */
	return MA_NOACTIVATE;
    }
    return DefWindowProc16(hwnd, message, wParam, lParam);
}

/************************************************************************
 * 			       	DlgDirSelectComboBox	[USER.194]
 */
BOOL DlgDirSelectComboBox(HWND hDlg, LPSTR lpStr, INT nIDLBox)
{
	fprintf(stdnimp,"DlgDirSelectComboBox(%04x, '%s', %d) \n",
				hDlg, lpStr, nIDLBox);
	return TRUE;
}

static INT32 COMBO_DlgDirList( HWND32 hDlg, LPARAM path, INT32 idCBox,
                               INT32 idStatic, UINT32 wType, BOOL32 unicode )
{
    LRESULT res = 0;

    if (idCBox)
    {
        SendDlgItemMessage32A( hDlg, idCBox, CB_RESETCONTENT, 0, 0 );
        if (unicode)
            res = SendDlgItemMessage32W( hDlg, idCBox, CB_DIR, wType, path );
        else
            res = SendDlgItemMessage32A( hDlg, idCBox, CB_DIR, wType, path );
    }
    if (idStatic)
    {
        char temp[512] = "A:\\";
        int drive = DRIVE_GetCurrentDrive();
        temp[0] += drive;
        lstrcpyn32A( temp + 3, DRIVE_GetDosCwd(drive), sizeof(temp)-3 );
        AnsiLower( temp );
        SetDlgItemText32A( hDlg, idStatic, temp );
    } 
    return (res >= 0);
}


/***********************************************************************
 *           DlgDirListComboBox16   (USER.195)
 */
INT16 DlgDirListComboBox16( HWND16 hDlg, SEGPTR path, INT16 idCBox,
                            INT16 idStatic, UINT16 wType )
{
    dprintf_combo( stddeb,"DlgDirListComboBox16(%04x,%08x,%d,%d,%04x)\n",
                   hDlg, (UINT32)path, idCBox, idStatic, wType );
    return COMBO_DlgDirList( hDlg, (LPARAM)path, idCBox,
                             idStatic, wType, FALSE );
}


/***********************************************************************
 *           DlgDirListComboBox32A   (USER32.143)
 */
INT32 DlgDirListComboBox32A( HWND32 hDlg, LPCSTR path, INT32 idCBox,
                             INT32 idStatic, UINT32 wType )
{
    dprintf_combo( stddeb,"DlgDirListComboBox32A(%08x,'%s',%d,%d,%08X)\n",
                   hDlg, path, idCBox, idStatic, wType );
    return COMBO_DlgDirList( hDlg, (LPARAM)path, idCBox,
                             idStatic, wType, FALSE );
}


/***********************************************************************
 *           DlgDirListComboBox32W   (USER32.144)
 */
INT32 DlgDirListComboBox32W( HWND32 hDlg, LPCWSTR path, INT32 idCBox,
                             INT32 idStatic, UINT32 wType )
{
    dprintf_combo( stddeb,"DlgDirListComboBox32W(%08x,%p,%d,%d,%08X)\n",
                   hDlg, path, idCBox, idStatic, wType );
    return COMBO_DlgDirList( hDlg, (LPARAM)path, idCBox,
                             idStatic, wType, TRUE );
}
