/*
 * Interface code to listbox widgets
 *
 * Copyright  Martin Ayotte, 1993
 * Copyright  Constantine Sapuntzakis, 1995
 *
static char Copyright[] = "Copyright Martin Ayotte, 1993";
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "windows.h"
#include "user.h"
#include "win.h"
#include "msdos.h"
#include "listbox.h"
#include "dos_fs.h"
#include "stddebug.h"
#include "debug.h"

#define GMEM_ZEROINIT 0x0040

LPLISTSTRUCT ListBoxGetItem (HWND hwnd, UINT uIndex);
int ListBoxScrolltoFocus(HWND hwnd);
LPHEADLIST ListBoxGetWindowAndStorage(HWND hwnd, WND **wndPtr);
LPHEADLIST ListBoxGetStorageHeader(HWND hwnd);
void RepaintListBox(HWND hwnd);
int ListBoxFindMouse(HWND hwnd, int X, int Y);
int CreateListBoxStruct(HWND hwnd);
void ListBoxAskMeasure(WND *wndPtr, LPHEADLIST lphl, LPLISTSTRUCT lpls);
int ListBoxAddString(HWND hwnd, LPSTR newstr);
int ListBoxInsertString(HWND hwnd, UINT uIndex, LPSTR newstr);
int ListBoxGetText(HWND hwnd, UINT uIndex, LPSTR OutStr, BOOL bItemData);
int ListBoxSetItemData(HWND hwnd, UINT uIndex, DWORD ItemData);
int ListBoxDeleteString(HWND hwnd, UINT uIndex);
int ListBoxFindString(HWND hwnd, UINT nFirst, LPSTR MatchStr);
int ListBoxResetContent(HWND hwnd);
int ListBoxSetCurSel(HWND hwnd, WORD wIndex);
int ListBoxSetSel(HWND hwnd, WORD wIndex, WORD state);
int ListBoxGetSel(HWND hwnd, WORD wIndex);
int ListBoxDirectory(HWND hwnd, UINT attrib, LPSTR filespec);
int ListBoxGetItemRect(HWND hwnd, WORD wIndex, LPRECT rect);
int ListBoxSetItemHeight(HWND hwnd, WORD wIndex, long height);
int ListBoxDefaultItem(HWND hwnd, WND *wndPtr, 
	LPHEADLIST lphl, LPLISTSTRUCT lpls);
int ListBoxFindNextMatch(HWND hwnd, WORD wChar);
int ListMaxFirstVisible(LPHEADLIST lphl);
void ListBoxSendNotification(HWND hwnd, WORD code);

#define OWNER_DRAWN(wndPtr) \
  ((wndPtr->dwStyle & LBS_OWNERDRAWFIXED) ||  \
   (wndPtr->dwStyle & LBS_OWNERDRAWVARIABLE))

#define HasStrings(wndPtr) (  \
  (! OWNER_DRAWN (wndPtr)) || \
  (wndPtr->dwStyle & LBS_HASSTRINGS))

#if 0
#define LIST_HEAP_ALLOC(lphl,f,size) ((int)HEAP_Alloc(&lphl->Heap,f,size) & 0xffff)
#define LIST_HEAP_FREE(lphl,handle) (HEAP_Free(&lphl->Heap,LIST_HEAP_ADDR(lphl,handle)))
#define LIST_HEAP_ADDR(lphl,handle) \
    ((void *)((handle) ? ((handle) | ((int)lphl->Heap & 0xffff0000)) : 0))
#else
/* FIXME: shouldn't each listbox have its own heap? */
#define LIST_HEAP_ALLOC(lphl,f,size) USER_HEAP_ALLOC(size)
#define LIST_HEAP_FREE(lphl,handle)  USER_HEAP_FREE(handle)
#define LIST_HEAP_ADDR(lphl,handle)  USER_HEAP_LIN_ADDR(handle)
#define LIST_HEAP_SEG_ADDR(lphl,handle) USER_HEAP_SEG_ADDR(handle)
#endif

#define LIST_HEAP_SIZE 0x10000

/* Design notes go here */

LONG LBCreate( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBGetDlgCode( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBDestroy( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBVScroll( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBHScroll( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBLButtonDown( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBLButtonUp( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBRButtonUp( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBMouseMove( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBKeyDown( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBSetFont( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBSetRedraw( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBPaint( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBSetFocus( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBKillFocus( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBResetContent( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBDir( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBAddString( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBGetText( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBInsertString( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBDeleteString( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBFindString( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBGetCaretIndex( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBGetCount( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBGetCurSel( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBGetHorizontalExtent(HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBGetItemData( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBGetItemHeight( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBGetItemRect( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBGetSel( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBGetSelCount( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBGetSelItems( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBGetTextLen( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBGetTopIndex( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBSelectString( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBSelItemRange( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBSetCaretIndex( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBSetColumnWidth( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBSetHorizontalExtent(HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBSetItemData( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBSetTabStops( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBSetCurSel( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBSetSel( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBSetTopIndex( HWND hwnd, WORD message, WORD wParam, LONG lParam );
LONG LBSetItemHeight( HWND hwnd, WORD message, WORD wParam, LONG lParam );


typedef struct {
  WORD   message;
  LONG  (*handler)(HWND, WORD, WPARAM, LPARAM);
} msg_tbl;

static msg_tbl methods[] = {
  {WM_CREATE, LBCreate},
  {WM_DESTROY, LBDestroy},
  {WM_GETDLGCODE, LBGetDlgCode},
  {WM_VSCROLL, LBVScroll},
  {WM_HSCROLL, LBHScroll},
  {WM_LBUTTONDOWN, LBLButtonDown},
  {WM_LBUTTONUP, LBLButtonUp},
  {WM_RBUTTONUP, LBRButtonUp},
  {WM_LBUTTONDBLCLK, LBRButtonUp},
  {WM_MOUSEMOVE, LBMouseMove},
  {WM_KEYDOWN, LBKeyDown},
  {WM_SETFONT, LBSetFont},
  {WM_SETREDRAW, LBSetRedraw},
  {WM_PAINT, LBPaint},
  {WM_SETFOCUS, LBSetFocus},
  {WM_KILLFOCUS, LBKillFocus},
  {LB_RESETCONTENT, LBResetContent},
  {LB_DIR, LBDir},
  {LB_ADDSTRING, LBAddString},
  {LB_INSERTSTRING, LBInsertString},
  {LB_DELETESTRING, LBDeleteString},
  {LB_FINDSTRING, LBFindString},
  {LB_GETCARETINDEX, LBGetCaretIndex},
  {LB_GETCOUNT, LBGetCount},
  {LB_GETCURSEL, LBGetCurSel},
  {LB_GETHORIZONTALEXTENT, LBGetHorizontalExtent},
  {LB_GETITEMDATA, LBGetItemData},
  {LB_GETITEMHEIGHT, LBGetItemHeight},
  {LB_GETITEMRECT, LBGetItemRect},
  {LB_GETSEL, LBGetSel},
  {LB_GETSELCOUNT, LBGetSelCount},
  {LB_GETSELITEMS, LBGetSelItems},
  {LB_GETTEXT, LBGetText},
  {LB_GETTEXTLEN, LBGetTextLen},
  {LB_GETTOPINDEX, LBGetTopIndex},
  {LB_SELECTSTRING, LBSelectString},
  {LB_SELITEMRANGE, LBSelItemRange},
  {LB_SETCARETINDEX, LBSetCaretIndex},
  {LB_SETCOLUMNWIDTH, LBSetColumnWidth},
  {LB_SETHORIZONTALEXTENT, LBSetHorizontalExtent},
  {LB_SETITEMDATA, LBSetItemData},
  {LB_SETTABSTOPS, LBSetTabStops},
  {LB_SETCURSEL, LBSetCurSel},
  {LB_SETSEL, LBSetSel},
  {LB_SETTOPINDEX, LBSetTopIndex},
  {LB_SETITEMHEIGHT, LBSetItemHeight}
};

/***********************************************************************
 *           LBCreate
 */
LONG LBCreate( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST  lphl;
  CREATESTRUCT *createStruct;
  WND          *wndPtr;

  CreateListBoxStruct(hwnd);
  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);

  dprintf_listbox(stddeb,"ListBox WM_CREATE %p !\n", lphl);

  if (lphl == NULL) return 0;

  createStruct = (CREATESTRUCT *)PTR_SEG_TO_LIN(lParam);

  if (HIWORD(createStruct->lpCreateParams) != 0)
    lphl->hWndLogicParent = (HWND)HIWORD(createStruct->lpCreateParams);
  else
    lphl->hWndLogicParent = GetParent(hwnd);

  lphl->hFont = GetStockObject(SYSTEM_FONT);
  lphl->ColumnsWidth = wndPtr->rectClient.right - wndPtr->rectClient.left;

  SetScrollRange(hwnd, SB_VERT, 1, ListMaxFirstVisible(lphl), TRUE);
  SetScrollRange(hwnd, SB_HORZ, 1, 1, TRUE);

  return 0;
}

int CreateListBoxStruct(HWND hwnd)

{
  WND  *wndPtr;
  LPHEADLIST lphl;

  wndPtr = WIN_FindWndPtr(hwnd);

  lphl = (LPHEADLIST)malloc(sizeof(HEADLIST));
  *((LPHEADLIST *)&wndPtr->wExtra[1]) = lphl;

  lphl->lpFirst        = NULL;
  lphl->ItemsCount     = 0;
  lphl->ItemsVisible   = 0;
  lphl->FirstVisible   = 1;
  lphl->ColumnsVisible = 1;
  lphl->ItemsPerColumn = 0;
  lphl->StdItemHeight  = 15;
  lphl->ItemFocused    = -1;
  lphl->PrevFocused    = -1;
  lphl->DrawCtlType    = ODT_LISTBOX;
  lphl->bRedrawFlag    = TRUE;
  lphl->iNumStops      = 0;
  lphl->TabStops       = NULL;

  if (OWNER_DRAWN(wndPtr)) 
    lphl->hDrawItemStruct = USER_HEAP_ALLOC(sizeof(DRAWITEMSTRUCT));
  else
    lphl->hDrawItemStruct = 0;

#if 0
  HeapHandle = GlobalAlloc(GMEM_FIXED, LIST_HEAP_SIZE);
  HeapBase = GlobalLock(HeapHandle);
  HEAP_Init(&lphl->Heap, HeapBase, LIST_HEAP_SIZE);
#endif
  return TRUE;
}


/***********************************************************************
 *           LBDestroy
 */
LONG LBDestroy( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST lphl;
  WND        *wndPtr;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);

  if (lphl == NULL) return 0;
  ListBoxResetContent(hwnd);

  if (lphl->hDrawItemStruct)
    USER_HEAP_FREE(lphl->hDrawItemStruct);

  /* XXX need to free lphl->Heap */
  free(lphl);
  *((LPHEADLIST *)&wndPtr->wExtra[1]) = 0;
  dprintf_listbox(stddeb,"ListBox WM_DESTROY %p !\n", lphl);
  return 0;
}

/* get the maximum value of lphl->FirstVisible */
int ListMaxFirstVisible(LPHEADLIST lphl)
{
    int m = lphl->ItemsCount-lphl->ItemsVisible+1;
    return (m < 1) ? 1 : m;
}


/***********************************************************************
 *           LBVScroll
 */
LONG LBVScroll( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST lphl;
  int  y;

  dprintf_listbox(stddeb,"ListBox WM_VSCROLL w=%04X l=%08lX !\n",
		  wParam, lParam);
  lphl = ListBoxGetStorageHeader(hwnd);
  if (lphl == NULL) return 0;
  y = lphl->FirstVisible;

  switch(wParam) {
  case SB_LINEUP:
    if (lphl->FirstVisible > 1)	
      lphl->FirstVisible--;
    break;

  case SB_LINEDOWN:
    if (lphl->FirstVisible < ListMaxFirstVisible(lphl))
      lphl->FirstVisible++;
    break;

  case SB_PAGEUP:
    if (lphl->FirstVisible > 1)  
      lphl->FirstVisible -= lphl->ItemsVisible;
    break;

  case SB_PAGEDOWN:
    if (lphl->FirstVisible < ListMaxFirstVisible(lphl))  
      lphl->FirstVisible += lphl->ItemsVisible;
    break;

  case SB_THUMBTRACK:
    lphl->FirstVisible = LOWORD(lParam);
    break;
  }

  if (lphl->FirstVisible < 1)    lphl->FirstVisible = 1;
  if (lphl->FirstVisible > ListMaxFirstVisible(lphl))
    lphl->FirstVisible = ListMaxFirstVisible(lphl);

  if (y != lphl->FirstVisible) {
    SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
  }
  return 0;
}

/***********************************************************************
 *           LBHScroll
 */
LONG LBHScroll( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST lphl;
  int        y;

  dprintf_listbox(stddeb,"ListBox WM_HSCROLL w=%04X l=%08lX !\n",
		  wParam, lParam);
  lphl = ListBoxGetStorageHeader(hwnd);
  if (lphl == NULL) return 0;
  y = lphl->FirstVisible;
  switch(wParam) {
  case SB_LINEUP:
    if (lphl->FirstVisible > 1)
      lphl->FirstVisible -= lphl->ItemsPerColumn;
    break;
  case SB_LINEDOWN:
    if (lphl->FirstVisible < ListMaxFirstVisible(lphl))
      lphl->FirstVisible += lphl->ItemsPerColumn;
    break;
  case SB_PAGEUP:
    if (lphl->FirstVisible > 1 && lphl->ItemsPerColumn != 0)  
      lphl->FirstVisible -= lphl->ItemsVisible /
	lphl->ItemsPerColumn * lphl->ItemsPerColumn;
    break;
  case SB_PAGEDOWN:
    if (lphl->FirstVisible < ListMaxFirstVisible(lphl) &&
	lphl->ItemsPerColumn != 0)  
      lphl->FirstVisible += lphl->ItemsVisible /
	lphl->ItemsPerColumn * lphl->ItemsPerColumn;
    break;
  case SB_THUMBTRACK:
    lphl->FirstVisible = lphl->ItemsPerColumn * 
      (LOWORD(lParam) - 1) + 1;
    break;
  } 
  if (lphl->FirstVisible < 1) lphl->FirstVisible = 1;
  if (lphl->FirstVisible > ListMaxFirstVisible(lphl))
    lphl->FirstVisible = ListMaxFirstVisible(lphl);

  if (lphl->ItemsPerColumn != 0) {
    lphl->FirstVisible = lphl->FirstVisible /
      lphl->ItemsPerColumn * lphl->ItemsPerColumn + 1;
    if (y != lphl->FirstVisible) {
      SetScrollPos(hwnd, SB_HORZ, lphl->FirstVisible / 
		   lphl->ItemsPerColumn + 1, TRUE);
      InvalidateRect(hwnd, NULL, TRUE);
      UpdateWindow(hwnd);
    }
  }
  return 0;
}

/***********************************************************************
 *           LBLButtonDown
 */
LONG LBLButtonDown( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST lphl;
  WND        *wndPtr;
  WORD       wRet;
  int        y;
  RECT       rectsel;

  SetFocus(hwnd);
  SetCapture(hwnd);

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return 0;

  lphl->PrevFocused = lphl->ItemFocused;

  y = ListBoxFindMouse(hwnd, LOWORD(lParam), HIWORD(lParam));
  if (y==-1)
    return 0;

  if (wndPtr->dwStyle & LBS_MULTIPLESEL) {
    lphl->ItemFocused = y;
    wRet = ListBoxGetSel(hwnd, y);
    ListBoxSetSel(hwnd, y, !wRet);
  }
  else
    ListBoxSetCurSel(hwnd, y);

  if (wndPtr->dwStyle & LBS_MULTIPLESEL)
    ListBoxSendNotification( hwnd, LBN_SELCHANGE );

  ListBoxGetItemRect(hwnd, y, &rectsel);

  InvalidateRect(hwnd, NULL, TRUE);
  UpdateWindow(hwnd);

  return 0;
}

/***********************************************************************
 *           LBLButtonUp
 */
LONG LBLButtonUp( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST lphl;
  WND        *wndPtr;

  if (GetCapture() == hwnd) ReleaseCapture();

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return 0;
  if (lphl->PrevFocused != lphl->ItemFocused)
    ListBoxSendNotification( hwnd, LBN_SELCHANGE );

  return 0;
}

/***********************************************************************
 *           LBRButtonUp
 */
LONG LBRButtonUp( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST lphl;
  WND        *wndPtr;
  
  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return 0;
  SendMessage(lphl->hWndLogicParent, WM_COMMAND, wndPtr->wIDmenu,
		MAKELONG(hwnd, LBN_DBLCLK));

  return 0;
}

/***********************************************************************
 *           LBMouseMove
 */
LONG LBMouseMove( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST  lphl;
  WND        *wndPtr;
  int  y;
  WORD        wRet;
  RECT        rect, rectsel;   /* XXX Broken */

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return 0;
  if ((wParam & MK_LBUTTON) != 0) {
    y = HIWORD(lParam);
    if (y < 4) {
      if (lphl->FirstVisible > 1) {
	lphl->FirstVisible--;
	SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
	return 0;
      }
    }
    GetClientRect(hwnd, &rect);
    if (y > (rect.bottom - 4)) {
      if (lphl->FirstVisible < ListMaxFirstVisible(lphl)) {
	lphl->FirstVisible++;
	SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
	return 0;
      }
    }
    if ((y > 0) && (y < (rect.bottom - 4))) {
      if ((y < rectsel.top) || (y > rectsel.bottom)) {
	wRet = ListBoxFindMouse(hwnd, LOWORD(lParam), HIWORD(lParam));
	if ((wndPtr->dwStyle & LBS_MULTIPLESEL) == LBS_MULTIPLESEL) {
	  lphl->ItemFocused = wRet;
	  ListBoxSendNotification(hwnd, LBN_SELCHANGE);
	}
	else
	  ListBoxSetCurSel(hwnd, wRet);
	ListBoxGetItemRect(hwnd, wRet, &rectsel);
	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
      }
    }
  }

  return 0;
}

/***********************************************************************
 *           LBKeyDown
 */
LONG LBKeyDown( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST  lphl;
  WND        *wndPtr;
  HWND        hWndCtl;
  WORD        wRet;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return 0;
  switch(wParam) {
  case VK_TAB:
    hWndCtl = GetNextDlgTabItem(lphl->hWndLogicParent,
				hwnd, !(GetKeyState(VK_SHIFT) < 0));
    SetFocus(hWndCtl);
    if(debugging_listbox){
      if ((GetKeyState(VK_SHIFT) < 0))
	dprintf_listbox(stddeb,"ListBox PreviousDlgTabItem %04X !\n", hWndCtl);
      else
	dprintf_listbox(stddeb,"ListBox NextDlgTabItem %04X !\n", hWndCtl);
    }
    break;
  case VK_HOME:
    lphl->ItemFocused = 0;
    break;
  case VK_END:
    lphl->ItemFocused = lphl->ItemsCount - 1;
    break;
  case VK_LEFT:
    if ((wndPtr->dwStyle & LBS_MULTICOLUMN) == LBS_MULTICOLUMN) {
      lphl->ItemFocused -= lphl->ItemsPerColumn;
    }
    break;
  case VK_UP:
    lphl->ItemFocused--;
    break;
  case VK_RIGHT:
    if ((wndPtr->dwStyle & LBS_MULTICOLUMN) == LBS_MULTICOLUMN) {
      lphl->ItemFocused += lphl->ItemsPerColumn;
    }
    break;
  case VK_DOWN:
    lphl->ItemFocused++;
    break;
  case VK_PRIOR:
    lphl->ItemFocused -= lphl->ItemsVisible;
    break;
  case VK_NEXT:
    lphl->ItemFocused += lphl->ItemsVisible;
    break;
  case VK_SPACE:
    wRet = ListBoxGetSel(hwnd, lphl->ItemFocused);
    ListBoxSetSel(hwnd, lphl->ItemFocused, !wRet);
    break;
  default:
    ListBoxFindNextMatch(hwnd, wParam);
    return 0;
  }

  if (lphl->ItemFocused < 0) lphl->ItemFocused = 0;
  if (lphl->ItemFocused >= lphl->ItemsCount)
    lphl->ItemFocused = lphl->ItemsCount - 1;

  if (lphl->ItemsVisible != 0)
    lphl->FirstVisible = lphl->ItemFocused / lphl->ItemsVisible * 
      lphl->ItemsVisible + 1;

  if (lphl->FirstVisible < 1) lphl->FirstVisible = 1;
  if (lphl->FirstVisible > ListMaxFirstVisible(lphl))
    lphl->FirstVisible = ListMaxFirstVisible(lphl);

  if ((wndPtr->dwStyle & LBS_MULTIPLESEL) != LBS_MULTIPLESEL) {
    ListBoxSetCurSel(hwnd, lphl->ItemFocused);
    ListBoxSendNotification(hwnd, LBN_SELCHANGE);
  }

  SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
  InvalidateRect(hwnd, NULL, TRUE);
  UpdateWindow(hwnd);

  return 0;
}

/***********************************************************************
 *           LBSetRedraw
 */
LONG LBSetRedraw( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST  lphl;
  WND        *wndPtr;

  dprintf_listbox(stddeb,"ListBox WM_SETREDRAW hWnd=%04X w=%04X !\n", hwnd, wParam);
  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return 0;
  lphl->bRedrawFlag = wParam;

  return 0;
}

/***********************************************************************
 *           LBSetFont
 */

LONG LBSetFont( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST  lphl;
  WND        *wndPtr;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return 0;

  if (wParam == 0)
    lphl->hFont = GetStockObject(SYSTEM_FONT);
  else
    lphl->hFont = wParam;

  return 0;
}

/***********************************************************************
 *           LBPaint
 */
LONG LBPaint( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  RepaintListBox(hwnd);
  return 0;
}

/***********************************************************************
 *           LBSetFocus
 */
LONG LBSetFocus( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST lphl;
  WND       *wndPtr;

  dprintf_listbox(stddeb,"ListBox WM_SETFOCUS !\n");
  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);

  return 0;
}

/***********************************************************************
 *           LBKillFocus
 */
LONG LBKillFocus( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  dprintf_listbox(stddeb,"ListBox WM_KILLFOCUS !\n");

  InvalidateRect(hwnd, NULL, TRUE);
  UpdateWindow(hwnd);
  
  return 0;
}

/***********************************************************************
 *           LBResetContent
 */
LONG LBResetContent( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  dprintf_listbox(stddeb,"ListBox LB_RESETCONTENT !\n");
  ListBoxResetContent(hwnd);

  return 0;
}

/***********************************************************************
 *           LBDir
 */
LONG LBDir( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  WORD   wRet;
  dprintf_listbox(stddeb,"ListBox LB_DIR !\n");

  wRet = ListBoxDirectory(hwnd, wParam,
			  (LPSTR)PTR_SEG_TO_LIN(lParam));
  InvalidateRect(hwnd, NULL, TRUE);
  UpdateWindow(hwnd);
  return wRet;
}

/***********************************************************************
 *           LBAddString
 */
LONG LBAddString( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  WORD  wRet;
  WND  *wndPtr;

  wndPtr = WIN_FindWndPtr(hwnd);

  if (HasStrings(wndPtr))
    wRet = ListBoxAddString(hwnd, (LPSTR)PTR_SEG_TO_LIN(lParam));
  else
    wRet = ListBoxAddString(hwnd, (LPSTR)lParam);

  return wRet;
}

/***********************************************************************
 *           LBGetText
 */
LONG LBGetText( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LONG   wRet;

  dprintf_listbox(stddeb, "LB_GETTEXT  wParam=%d\n",wParam);
  wRet = ListBoxGetText(hwnd, wParam,
			(LPSTR)PTR_SEG_TO_LIN(lParam), FALSE);

  return wRet;
}

/***********************************************************************
 *           LBInsertString
 */
LONG LBInsertString( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  WORD  wRet;
  WND  *wndPtr;

  wndPtr = WIN_FindWndPtr(hwnd);

  if (HasStrings(wndPtr))
    wRet = ListBoxInsertString(hwnd, wParam, (LPSTR)PTR_SEG_TO_LIN(lParam));
  else
    wRet = ListBoxInsertString(hwnd, wParam, (LPSTR)lParam);

  return wRet;
}

/***********************************************************************
 *           LBDeleteString
 */    
LONG LBDeleteString( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  return ListBoxDeleteString(hwnd, wParam); 
}

/***********************************************************************
 *           LBFindString
 */
LONG LBFindString( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  return ListBoxFindString(hwnd, wParam,
			   (LPSTR)PTR_SEG_TO_LIN(lParam));
}

/***********************************************************************
 *           LBGetCaretIndex
 */
LONG LBGetCaretIndex( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST  lphl;

  lphl = ListBoxGetStorageHeader(hwnd);
  if (lphl == NULL) return LB_ERR;

  return lphl->ItemFocused;
}

/***********************************************************************
 *           LBGetCount
 */
LONG LBGetCount( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST  lphl;

  lphl = ListBoxGetStorageHeader(hwnd);
  return lphl->ItemsCount;
}

/***********************************************************************
 *           LBGetCurSel
 */
LONG LBGetCurSel( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST  lphl;

  lphl = ListBoxGetStorageHeader(hwnd);
  dprintf_listbox(stddeb,"ListBox LB_GETCURSEL %u !\n", 
		  lphl->ItemFocused);
  return lphl->ItemFocused;
}

/***********************************************************************
 *           LBGetHorizontalExtent
 */
LONG LBGetHorizontalExtent( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{    
  return 0;
}

/***********************************************************************
 *           LBGetItemData
 */
LONG LBGetItemData( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
    dprintf_listbox(stddeb, "LB_GETITEMDATA wParam=%x\n", wParam);
    return ListBoxGetText(hwnd, wParam,
			  (LPSTR)PTR_SEG_TO_LIN(lParam), TRUE);
}

/***********************************************************************
 *           LBGetItemHeight
 */
LONG LBGetItemHeight( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  RECT   rect;

  ListBoxGetItemRect(hwnd, wParam, &rect);
  return (rect.bottom - rect.top);
}

/***********************************************************************
 *           LBGetItemRect
 */
LONG LBGetItemRect( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  return ListBoxGetItemRect (hwnd, wParam, PTR_SEG_TO_LIN(lParam));
}

/***********************************************************************
 *           LBGetSel
 */
LONG LBGetSel( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  return ListBoxGetSel (hwnd, wParam);
}

/***********************************************************************
 *           LBGetSelCount
 */
LONG LBGetSelCount( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST   lphl;
  LPLISTSTRUCT lpls;
  int          cnt = 0;
  WND        *wndPtr;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return LB_ERR;

  if (!(wndPtr->dwStyle & LBS_MULTIPLESEL)) return LB_ERR;

  lpls = lphl->lpFirst;

  while (lpls != NULL) {
    if (lpls->dis.itemState > 0) cnt++;

    lpls = lpls->lpNext;
  }

  return cnt;
}

/***********************************************************************
 *           LBGetSelItems
 */
LONG LBGetSelItems( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST   lphl;
  LPLISTSTRUCT lpls;
  int          cnt, idx;
  WND         *wndPtr;
  int         *lpItems = PTR_SEG_TO_LIN(lParam);

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return LB_ERR;

  if (!(wndPtr->dwStyle & LBS_MULTIPLESEL)) return LB_ERR;

  if (wParam == 0) return 0;

  lpls = lphl->lpFirst;
  cnt = 0; idx = 0;

  while (lpls != NULL) {
    if (lpls->dis.itemState > 0) lpItems[cnt++] = idx;

    if (cnt == wParam) break;
    idx++;
    lpls = lpls->lpNext;
  }

  return cnt;
}

/***********************************************************************
 *           LBGetTextLen
 */
LONG LBGetTextLen( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST   lphl;
  LPLISTSTRUCT lpls;
  WND         *wndPtr;
  int          cnt = 0;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return LB_ERR;

  if (!HasStrings(wndPtr)) return LB_ERR;

  if (wParam >= lphl->ItemsCount) return LB_ERR;
    
  lpls = lphl->lpFirst;

  while (cnt++ < wParam) lpls = lpls->lpNext;
  
  return strlen(lpls->itemText);
}

/***********************************************************************
 *           LBGetDlgCode
 */
LONG LBGetDlgCode( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  return DLGC_WANTALLKEYS;
}

/***********************************************************************
 *           LBGetTopIndex
 */
LONG LBGetTopIndex( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST lphl;

  lphl = ListBoxGetStorageHeader(hwnd);
  if (lphl == NULL) return LB_ERR;

  return (lphl->FirstVisible - 1);
}


/***********************************************************************
 *           LBSelectString
 */
LONG LBSelectString( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  WND  *wndPtr;
  WORD  wRet;

  wndPtr = WIN_FindWndPtr(hwnd);

  wRet = ListBoxFindString(hwnd, wParam,
			   (LPSTR)PTR_SEG_TO_LIN(lParam));

  /* XXX add functionality here */

  return 0;
}

/***********************************************************************
 *           LBSelItemRange
 */
LONG LBSelItemRange( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST   lphl;
  LPLISTSTRUCT lpls;
  WND         *wndPtr;
  WORD         cnt;
  WORD         first = LOWORD(lParam);
  WORD         last = HIWORD(lParam);
  BOOL         select = wParam;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return LB_ERR;

  if (!(wndPtr->dwStyle & LBS_MULTIPLESEL)) return LB_ERR;

  if (first >= lphl->ItemsCount ||
      last >= lphl->ItemsCount) return LB_ERR;

  lpls = lphl->lpFirst;
  cnt = 0;

  while (lpls != NULL) {
    if (cnt++ >= first)
      lpls->dis.itemState = select ? ODS_SELECTED : 0;

    if (cnt > last)
      break;

    lpls = lpls->lpNext;
  }

  return 0;
}

/***********************************************************************
 *           LBSetCaretIndex
 */
LONG LBSetCaretIndex( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST   lphl;
  WND         *wndPtr;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return LB_ERR;

  if (!(wndPtr->dwStyle & LBS_MULTIPLESEL)) return 0;
  if (wParam >= lphl->ItemsCount) return LB_ERR;

  lphl->ItemFocused = wParam;
  ListBoxScrolltoFocus (hwnd);

  SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
  InvalidateRect(hwnd, NULL, TRUE);
  UpdateWindow(hwnd);
  return 0;
}

/***********************************************************************
 *           LBSetColumnWidth
 */
LONG LBSetColumnWidth( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST   lphl;

  lphl = ListBoxGetStorageHeader(hwnd);
  if (lphl == NULL) return LB_ERR;
  lphl->ColumnsWidth = wParam;

  return 0;
}

/***********************************************************************
 *           LBSetHorizontalExtent
 */
LONG LBSetHorizontalExtent( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  return 0;
}

/***********************************************************************
 *           LBSetItemData
 */
LONG LBSetItemData( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  dprintf_listbox(stddeb, "LB_SETITEMDATA  wParam=%x  lParam=%lx\n", wParam, lParam);
  return ListBoxSetItemData(hwnd, wParam, lParam);
}

/***********************************************************************
 *           LBSetTabStops
 */
LONG LBSetTabStops( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST  lphl;

  lphl = ListBoxGetStorageHeader(hwnd);

  if (lphl->TabStops != NULL) {
    lphl->iNumStops = 0;
    free (lphl->TabStops);
  }

  lphl->TabStops = malloc (wParam * sizeof (short));
  if (lphl->TabStops) {
    lphl->iNumStops = wParam;
    memcpy (lphl->TabStops, PTR_SEG_TO_LIN(lParam), wParam * sizeof (short));
    return TRUE;
  }

  return FALSE;
}

/***********************************************************************
 *           LBSetCurSel
 */
LONG LBSetCurSel( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST  lphl;
  WORD  wRet;

  lphl = ListBoxGetStorageHeader(hwnd);
  if (lphl == NULL) return LB_ERR; 

  dprintf_listbox(stddeb,"ListBox LB_SETCURSEL wParam=%x !\n", 
		  wParam);

  wRet = ListBoxSetCurSel(hwnd, wParam);

  SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
  InvalidateRect(hwnd, NULL, TRUE);
  UpdateWindow(hwnd);

  return wRet;
}

/***********************************************************************
 *           LBSetSel
 */
LONG LBSetSel( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  WORD wRet;

  dprintf_listbox(stddeb,"ListBox LB_SETSEL wParam=%x lParam=%lX !\n", wParam, lParam);

  wRet = ListBoxSetSel(hwnd, LOWORD(lParam), wParam);
  InvalidateRect(hwnd, NULL, TRUE);
  UpdateWindow(hwnd);

  return wRet;
}

/***********************************************************************
 *           LBSetTopIndex
 */
LONG LBSetTopIndex( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{
  LPHEADLIST lphl;

  dprintf_listbox(stddeb,"ListBox LB_SETTOPINDEX wParam=%x !\n",
		  wParam);
  lphl = ListBoxGetStorageHeader(hwnd);
  lphl->FirstVisible = wParam;
  SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);

  InvalidateRect(hwnd, NULL, TRUE);
  UpdateWindow(hwnd);

  return 0;
}

/***********************************************************************
 *           LBSetItemHeight
 */
LONG LBSetItemHeight( HWND hwnd, WORD message, WORD wParam, LONG lParam)

{
  WORD  wRet;

  dprintf_listbox(stddeb,"ListBox LB_SETITEMHEIGHT wParam=%x lParam=%lX !\n", wParam, lParam);
  wRet = ListBoxSetItemHeight(hwnd, wParam, lParam);
  return wRet;
}


/***********************************************************************
 *           ListBoxWndProc 
 */

LONG ListBoxWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam )

{ 
  int idx = 0;
  int table_size = sizeof (methods) / sizeof (msg_tbl);

  while (idx < table_size) {
    if (message == methods[idx].message) {
      return (*(methods[idx].handler))(hwnd, message, wParam, lParam);
    }
    idx++;
  }
  return DefWindowProc (hwnd, message, wParam, lParam);
}


LPLISTSTRUCT ListBoxGetItem(HWND hwnd, UINT uIndex)

{
  LPHEADLIST lphl = ListBoxGetStorageHeader(hwnd);
  LPLISTSTRUCT lpls;
  UINT         Count = 0;

  if (uIndex >= lphl->ItemsCount) return NULL;

  lpls = lphl->lpFirst;

  while (Count++ < uIndex) lpls = lpls->lpNext;

  return lpls;
}

 
LPHEADLIST ListBoxGetWindowAndStorage(HWND hwnd, WND **wndPtr)
{
    WND  *Ptr;
    LPHEADLIST lphl;
    *(wndPtr) = Ptr = WIN_FindWndPtr(hwnd);
    lphl = *((LPHEADLIST *)&Ptr->wExtra[1]);
    return lphl;
}


LPHEADLIST ListBoxGetStorageHeader(HWND hwnd)
{
    WND  *wndPtr;
    LPHEADLIST lphl;
    wndPtr = WIN_FindWndPtr(hwnd);
    lphl = *((LPHEADLIST *)&wndPtr->wExtra[1]);
    return lphl;
}


void ListBoxDrawItem (HWND hwnd, HDC hdc, LPLISTSTRUCT lpls,
		      WORD itemAction, WORD itemState)

{
  LPHEADLIST  lphl;
  WND        *wndPtr;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);

  if (OWNER_DRAWN(wndPtr)) {
    DRAWITEMSTRUCT   *dis = USER_HEAP_LIN_ADDR(lphl->hDrawItemStruct);

    memcpy (dis, &lpls->dis, sizeof(DRAWITEMSTRUCT));

    dis->CtlType  = ODT_LISTBOX;
    dis->hDC      = hdc;

    if ((!dis->CtlID) && lphl->hWndLogicParent) {
      WND   *ParentWndPtr;

      ParentWndPtr = WIN_FindWndPtr(lphl->hWndLogicParent);
      dis->CtlID   = ParentWndPtr->wIDmenu;
    }

    if (HasStrings(wndPtr)) dis->itemData = LIST_HEAP_SEG_ADDR(lpls,lpls->hData);
   
    dis->itemAction = itemAction;
    dis->itemState  = itemState;

    SendMessage(lphl->hWndLogicParent, WM_DRAWITEM, 
		0, (LPARAM)USER_HEAP_SEG_ADDR(lphl->hDrawItemStruct));
  } else {

    if (itemAction == ODA_DRAWENTIRE ||
	itemAction == ODA_SELECT) {
      int 	OldBkMode;
      DWORD 	dwOldTextColor;

      OldBkMode = SetBkMode(hdc, TRANSPARENT);

      if (itemState != 0) {
	dwOldTextColor = SetTextColor(hdc, 0x00FFFFFFL);
	FillRect(hdc, &lpls->dis.rcItem, GetStockObject(BLACK_BRUSH));
      }

      if (wndPtr->dwStyle & LBS_USETABSTOPS)
	TabbedTextOut(hdc, lpls->dis.rcItem.left + 5, 
		      lpls->dis.rcItem.top + 2, 
		      (char *)lpls->itemText, 
		      strlen((char *)lpls->itemText), lphl->iNumStops,
		      lphl->TabStops, 0);
      else
	TextOut(hdc, lpls->dis.rcItem.left + 5, lpls->dis.rcItem.top + 2, 
		(char *)lpls->itemText, strlen((char *)lpls->itemText));

      if (itemState != 0) {
	SetTextColor(hdc, dwOldTextColor);
      }
      
      SetBkMode(hdc, OldBkMode);
    } else DrawFocusRect(hdc, &lpls->dis.rcItem);
  }

  return;
}

void RepaintListBox(HWND hwnd)

{
  WND 	*wndPtr;
  LPHEADLIST  lphl;
  LPLISTSTRUCT lpls;
  PAINTSTRUCT ps;
  HBRUSH 	hBrush;

  HDC 	hdc;
  RECT 	rect;
  int   i, top, height, maxwidth, ipc;

  top = 0;

  hdc = BeginPaint( hwnd, &ps );

  if (!IsWindowVisible(hwnd)) {
    EndPaint( hwnd, &ps );
    return;
  }

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) goto EndOfPaint;
  if (!lphl->bRedrawFlag) goto EndOfPaint;

  SelectObject(hdc, lphl->hFont);

  hBrush = SendMessage(lphl->hWndLogicParent, WM_CTLCOLOR, (WORD)hdc,
		       MAKELONG(hwnd, CTLCOLOR_LISTBOX));

  if (hBrush == (HBRUSH)NULL)  hBrush = GetStockObject(WHITE_BRUSH);

  GetClientRect(hwnd, &rect);
  FillRect(hdc, &rect, hBrush);

  maxwidth = rect.right;
  rect.right = lphl->ColumnsWidth;

  if (lphl->ItemsCount == 0) goto EndOfPaint;

  lpls = lphl->lpFirst;

  lphl->ItemsVisible = 0;
  lphl->ItemsPerColumn = ipc = 0;

  for(i = 0; i < lphl->ItemsCount; i++) {
    if (lpls == NULL) goto EndOfPaint;

    if (i >= lphl->FirstVisible - 1) {
      height = lpls->dis.rcItem.bottom - lpls->dis.rcItem.top;

      if (top > rect.bottom) {
	if ((wndPtr->dwStyle & LBS_MULTICOLUMN) == LBS_MULTICOLUMN) {
	  lphl->ItemsPerColumn = max(lphl->ItemsPerColumn, ipc);
	  ipc = 0;
	  top = 0;
	  rect.left += lphl->ColumnsWidth;
	  rect.right += lphl->ColumnsWidth;
	  if (rect.left > maxwidth) break;
	}
	else 
	  break;
      }

      lpls->dis.rcItem.top    = top;
      lpls->dis.rcItem.bottom = top + height;
      lpls->dis.rcItem.left   = rect.left;
      lpls->dis.rcItem.right  = rect.right;

      if (OWNER_DRAWN(wndPtr)) {
	ListBoxDrawItem (hwnd, hdc, lpls, ODA_DRAWENTIRE, 0);
	if (lpls->dis.itemState)
	  ListBoxDrawItem (hwnd, hdc, lpls, ODA_SELECT, ODS_SELECTED);
      }
      else 
	ListBoxDrawItem (hwnd, hdc, lpls, ODA_DRAWENTIRE, lpls->dis.itemState);

      if ((lphl->ItemFocused == i) && GetFocus() == hwnd)
	ListBoxDrawItem (hwnd, hdc, lpls, ODA_FOCUS, ODS_FOCUS);

      top += height;
      lphl->ItemsVisible++;
      ipc++;
    }

    lpls = lpls->lpNext;
  }
 EndOfPaint:
  EndPaint( hwnd, &ps );
}

int ListBoxFindMouse(HWND hwnd, int X, int Y)

{
  WND 		*wndPtr;
  LPHEADLIST 		lphl;
  LPLISTSTRUCT	lpls;
  RECT 		rect;
  int                 i, h, h2, w, w2;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);

  if (lphl == NULL) return LB_ERR;
  if (lphl->ItemsCount == 0) return LB_ERR;

  lpls = lphl->lpFirst;
  if (lpls == NULL) return LB_ERR;
  GetClientRect(hwnd, &rect);
  h = w2 = 0;
  w = lphl->ColumnsWidth;

  for(i = 1; i <= lphl->ItemsCount; i++) {
    if (i >= lphl->FirstVisible) {
      h2 = h;
      h += lpls->dis.rcItem.bottom - lpls->dis.rcItem.top;
      if ((Y > h2) && (Y < h) &&
	  (X > w2) && (X < w)) return(i - 1);
      if (h > rect.bottom) {
	if ((wndPtr->dwStyle & LBS_MULTICOLUMN) != LBS_MULTICOLUMN) return LB_ERR;
	h = 0;
	w2 = w;
	w += lphl->ColumnsWidth;
	if (w2 > rect.right) return LB_ERR;
      }
    }
    if (lpls->lpNext == NULL) return LB_ERR;
    lpls = (LPLISTSTRUCT)lpls->lpNext;
  }
  return(LB_ERR);
}

void ListBoxAskMeasure(WND *wndPtr, LPHEADLIST lphl, LPLISTSTRUCT lpls)  

{
  MEASUREITEMSTRUCT 	*lpmeasure;

  HANDLE hTemp = USER_HEAP_ALLOC( sizeof(MEASUREITEMSTRUCT) );

  lpmeasure = (MEASUREITEMSTRUCT *) USER_HEAP_LIN_ADDR(hTemp);

  if (lpmeasure == NULL) {
    fprintf(stderr,"ListBoxAskMeasure() // Bad allocation of Measure struct !\n");
    return;
  }
 
  lpmeasure->CtlType    = ODT_LISTBOX;
  lpmeasure->CtlID      = wndPtr->wIDmenu;
  lpmeasure->itemID     = lpls->dis.itemID;
  lpmeasure->itemWidth  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
  lpmeasure->itemHeight = 0;

  if (HasStrings(wndPtr))
    lpmeasure->itemData = LIST_HEAP_SEG_ADDR(lpls,lpls->hData);
  else
    lpmeasure->itemData = lpls->dis.itemData;

  SendMessage(lphl->hWndLogicParent, WM_MEASUREITEM,
	      0, USER_HEAP_SEG_ADDR(hTemp));

  if (wndPtr->dwStyle & LBS_OWNERDRAWFIXED) {
    lphl->StdItemHeight = lpmeasure->itemHeight;
  }

  lpls->dis.rcItem.right  = lpls->dis.rcItem.left + lpmeasure->itemWidth;
  lpls->dis.rcItem.bottom = lpls->dis.rcItem.top + lpmeasure->itemHeight;
  USER_HEAP_FREE(hTemp);			
}


int ListBoxAddString(HWND hwnd, LPSTR newstr)
{
    LPHEADLIST	lphl;
    UINT	pos = (UINT) -1;
    WND		*wndPtr;
    
    lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
    if (lphl == NULL) return LB_ERR;

    if (HasStrings(wndPtr) && (wndPtr->dwStyle & LBS_SORT)) {
	LPLISTSTRUCT lpls = lphl->lpFirst;
	for (pos = 0; lpls; lpls = lpls->lpNext, pos++)
	    if (strcmp(lpls->itemText, newstr) >= 0)
		break;
    }
    return ListBoxInsertString(hwnd, pos, newstr);
}

int ListBoxInsertString(HWND hwnd, UINT uIndex, LPSTR newstr)

{
  WND  	       *wndPtr;
  LPHEADLIST 	lphl;
  LPLISTSTRUCT *lppls, lplsnew;
  HANDLE 	hItem;
  HANDLE 	hStr;
  LPSTR	str;
  UINT	Count;
    
  dprintf_listbox(stddeb,"ListBoxInsertString(%04X, %d, %p);\n", 
		  hwnd, uIndex, newstr);
    
  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return LB_ERR;
    
  if (uIndex == (UINT)-1)
    uIndex = lphl->ItemsCount;

  if (uIndex > lphl->ItemsCount) return LB_ERR;

  lppls = (LPLISTSTRUCT *) &lphl->lpFirst;
    
  for(Count = 0; Count < uIndex; Count++) {
    if (*lppls == NULL) return LB_ERR;
    lppls = (LPLISTSTRUCT *) &(*lppls)->lpNext;
  }
    
  hItem = LIST_HEAP_ALLOC(lphl, GMEM_MOVEABLE, sizeof(LISTSTRUCT));
  lplsnew = (LPLISTSTRUCT) LIST_HEAP_ADDR(lphl, hItem);

  if (lplsnew == NULL) {
    printf("ListBoxInsertString() // Bad allocation of new item !\n");
    return LB_ERRSPACE;
  }

  ListBoxDefaultItem(hwnd, wndPtr, lphl, lplsnew);
  lplsnew->hMem = hItem;
  lplsnew->lpNext = *lppls;
  *lppls = lplsnew;
  lphl->ItemsCount++;
  hStr = 0;

  if (HasStrings(wndPtr)) {
    hStr = LIST_HEAP_ALLOC(lphl, GMEM_MOVEABLE, strlen(newstr) + 1);
    str = (LPSTR)LIST_HEAP_ADDR(lphl, hStr);
    if (str == NULL) return LB_ERRSPACE;
    strcpy(str, newstr);
    newstr = str;
    lplsnew->itemText = str;
    dprintf_listbox(stddeb,"ListBoxInsertString // LBS_HASSTRINGS after strcpy '%s'\n", str);
  } else {
    lplsnew->itemText = NULL;
    lplsnew->dis.itemData = (DWORD)newstr;
  }

  lplsnew->dis.itemID = lphl->ItemsCount;
  lplsnew->hData = hStr;
 
  if ((wndPtr->dwStyle & LBS_OWNERDRAWFIXED) && (lphl->ItemsCount == 1)) {
    ListBoxAskMeasure(wndPtr, lphl, lplsnew);
  }

  if (wndPtr->dwStyle & LBS_OWNERDRAWVARIABLE)
    ListBoxAskMeasure(wndPtr, lphl, lplsnew);   

  SetScrollRange(hwnd, SB_VERT, 1, ListMaxFirstVisible(lphl), 
		 (lphl->FirstVisible != 1 && lphl->bRedrawFlag));

  if (lphl->ItemsPerColumn != 0)
    SetScrollRange(hwnd, SB_HORZ, 1, lphl->ItemsVisible / 
		   lphl->ItemsPerColumn + 1,
		   (lphl->FirstVisible != 1 && lphl->bRedrawFlag));

  if ((lphl->FirstVisible <= uIndex) &&
      ((lphl->FirstVisible + lphl->ItemsVisible) >= uIndex)) {
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
  }

  dprintf_listbox(stddeb,"ListBoxInsertString // count=%d\n", lphl->ItemsCount);
  return uIndex;
}


int ListBoxGetText(HWND hwnd, UINT uIndex, LPSTR OutStr, BOOL bItemData)

{
  WND  	*wndPtr;
  LPLISTSTRUCT lpls;

  wndPtr = WIN_FindWndPtr(hwnd);

  if (!OutStr && !bItemData) {
    dprintf_listbox(stddeb, "ListBoxGetText // OutStr==NULL\n");
    return 0;
  }

  if (!bItemData) *OutStr=0;

  if ((lpls = ListBoxGetItem (hwnd, uIndex)) == NULL) 
    return LB_ERR;

  if (bItemData)
    return lpls->dis.itemData;

  if (!HasStrings(wndPtr)) {
    *((long *)OutStr) = lpls->dis.itemData;
    return 4;
  }
	
  strcpy(OutStr, lpls->itemText);
  return strlen(OutStr);
}

int ListBoxSetItemData(HWND hwnd, UINT uIndex, DWORD ItemData)

{
  LPLISTSTRUCT lpls;

  if ((lpls = ListBoxGetItem(hwnd, uIndex)) == NULL)
    return LB_ERR;

  lpls->dis.itemData = ItemData;
  return 1;
}


int ListBoxDeleteString(HWND hwnd, UINT uIndex)

{
  WND  	*wndPtr;
  LPHEADLIST 	lphl;
  LPLISTSTRUCT lpls, lpls2;
  UINT	Count;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return LB_ERR;

  if (uIndex >= lphl->ItemsCount) return LB_ERR;

  lpls = lphl->lpFirst;
  if (lpls == NULL) return LB_ERR;

  if( uIndex == 0 )
    lphl->lpFirst = lpls->lpNext;
  else {
    for(Count = 0; Count < uIndex; Count++) {
      if (lpls->lpNext == NULL) return LB_ERR;

      lpls2 = lpls;
      lpls = (LPLISTSTRUCT)lpls->lpNext;
    }
    lpls2->lpNext = (LPLISTSTRUCT)lpls->lpNext;
  }

  lphl->ItemsCount--;

  if (lpls->hData != 0) LIST_HEAP_FREE(lphl, lpls->hData);
  if (lpls->hMem != 0) LIST_HEAP_FREE(lphl, lpls->hMem);

  SetScrollRange(hwnd, SB_VERT, 1, ListMaxFirstVisible(lphl), TRUE);
  if (lphl->ItemsPerColumn != 0)
    SetScrollRange(hwnd, SB_HORZ, 1, lphl->ItemsVisible / 
		   lphl->ItemsPerColumn + 1, TRUE);

  if ((lphl->FirstVisible <= uIndex) &&
      ((lphl->FirstVisible + lphl->ItemsVisible) >= uIndex)) {
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
  }

  return lphl->ItemsCount;
}


int ListBoxFindString(HWND hwnd, UINT nFirst, LPSTR MatchStr)
{
  WND          *wndPtr;
  LPHEADLIST   lphl;
  LPLISTSTRUCT lpls;
  UINT	       Count;
  UINT         First = nFirst + 1;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);

  if (lphl == NULL) return LB_ERR;

  if (First > lphl->ItemsCount) return LB_ERR;
 
  lpls = ListBoxGetItem(hwnd, First);
  Count = 0;
  while(lpls != NULL) {
    if (HasStrings(wndPtr)) {
      if (strstr(lpls->itemText, MatchStr) == lpls->itemText) return Count;
    } else if (wndPtr->dwStyle & LBS_SORT) {
      /* XXX Do a compare item */
    }
    else
      if (lpls->dis.itemData == (DWORD)MatchStr) return Count;

    lpls = lpls->lpNext;
    Count++;
  }

  /* Start over at top */
  Count = 0;
  lpls = lphl->lpFirst;

  while (Count < First) {
    if (HasStrings(wndPtr)) {
      if (strstr(lpls->itemText, MatchStr) == lpls->itemText) return Count;
    } else if (wndPtr->dwStyle & LBS_SORT) {
      /* XXX Do a compare item */
    }
    else
      if (lpls->dis.itemData == (DWORD)MatchStr) return Count;

    lpls = lpls->lpNext;
    Count++;
  }

  return LB_ERR;
}


int ListBoxResetContent(HWND hwnd)
{
    WND  *wndPtr;
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls;
    UINT	i;

    lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
    if (lphl == NULL) return LB_ERR;

    if (lphl->ItemsCount == 0) return 0;

    lpls = lphl->lpFirst;

    dprintf_listbox(stddeb, "ListBoxResetContent // ItemCount = %d\n",
	lphl->ItemsCount);

    for(i = 0; i < lphl->ItemsCount; i++) {
      LPLISTSTRUCT lpls2;

      if (lpls == NULL) return LB_ERR;

      lpls2 = lpls->lpNext;

      if (i != 0) {
	dprintf_listbox(stddeb,"ResetContent #%u\n", i);
	if (lpls->hData != 0 && lpls->hData != lpls->hMem)
	  LIST_HEAP_FREE(lphl, lpls->hData);

	if (lpls->hMem != 0) LIST_HEAP_FREE(lphl, lpls->hMem);
      }  

      lpls = lpls2;
    }

    lphl->lpFirst      = NULL;
    lphl->FirstVisible = 1;
    lphl->ItemsCount   = 0;
    lphl->ItemFocused  = -1;
    lphl->PrevFocused  = -1;

    SetScrollRange(hwnd, SB_VERT, 1, ListMaxFirstVisible(lphl), TRUE);

    if (lphl->ItemsPerColumn != 0)
	SetScrollRange(hwnd, SB_HORZ, 1, lphl->ItemsVisible / 
	    lphl->ItemsPerColumn + 1, TRUE);

    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

    return TRUE;
}


int ListBoxSetCurSel(HWND hwnd, WORD wIndex)

{
  WND  *wndPtr;
  LPHEADLIST 	lphl;
  LPLISTSTRUCT lpls;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return LB_ERR;

  if (wndPtr->dwStyle & LBS_MULTIPLESEL) return 0;

  if (lphl->ItemFocused != -1) {
    lpls = ListBoxGetItem(hwnd, lphl->ItemFocused);
    if (lpls == 0) return LB_ERR;
    lpls->dis.itemState = 0;
  }

  if (wIndex != (UINT)-1) {
    lphl->ItemFocused = wIndex;
    lpls = ListBoxGetItem(hwnd, wIndex);
    if (lpls == 0) return LB_ERR;
    lpls->dis.itemState = ODS_SELECTED | ODS_FOCUS;

    return 0;
  }

  return LB_ERR;
}

int ListBoxSetSel(HWND hwnd, WORD wIndex, WORD state)

{
  LPHEADLIST 	lphl;
  LPLISTSTRUCT lpls;
  WND         *wndPtr;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return LB_ERR;

  if (!(wndPtr->dwStyle & LBS_MULTIPLESEL)) return 0;

  if (wIndex == (UINT)-1) {
    lpls = lphl->lpFirst;

    while (lpls != NULL) {
      lpls->dis.itemState = state;
      lpls = lpls->lpNext;
    }

    return 0;
  }

  if (wIndex >= lphl->ItemsCount) return LB_ERR;

  lpls = ListBoxGetItem(hwnd, wIndex);
  lpls->dis.itemState = state;

  return 0;
}


int ListBoxGetSel(HWND hwnd, WORD wIndex)
{
  LPLISTSTRUCT lpls;

  if ((lpls = ListBoxGetItem(hwnd, wIndex)) == NULL) return LB_ERR;

  return lpls->dis.itemState;
}


int ListBoxDirectory(HWND hwnd, UINT attrib, LPSTR filespec)
{
  struct dosdirent *dp, *dp_old;
  int	x, wRet = LB_OKAY;
  BOOL  OldFlag;
  char 	temp[256];
  LPHEADLIST 	lphl;
  int   drive;
  LPSTR tstr;

  dprintf_listbox(stddeb,"ListBoxDirectory: %s, %4x\n",filespec,attrib);

  if( strchr( filespec, '\\' ) || strchr( filespec, ':' ) ) {
    drive = DOS_GetDefaultDrive();
    if( filespec[1] == ':' ) {
      drive = toupper(filespec[0]) - 'A';
      filespec += 2;
    }
    strcpy(temp,filespec);
    tstr = strrchr(temp, '\\');
    if( tstr == NULL ) 
      DOS_SetDefaultDrive( drive );
    else {
      *tstr = 0;
      filespec = tstr + 1;
      DOS_ChangeDir( drive, temp );
      if (!DOS_ChangeDir( drive, temp )) return 0;
    }
    dprintf_listbox(stddeb,"Changing directory to %c:%s, filemask is %s\n",
		    drive+'A', temp, filespec );
  }
  lphl = ListBoxGetStorageHeader(hwnd);
  if (lphl == NULL) return LB_ERR;
  if ((dp = (struct dosdirent *)DOS_opendir(filespec)) ==NULL) return 0;
  dp_old = dp;
  OldFlag = lphl->bRedrawFlag;
  lphl->bRedrawFlag = FALSE;
  while ((dp = (struct dosdirent *)DOS_readdir(dp))) {
    if (!dp->inuse) break;
    dprintf_listbox( stddeb, "ListBoxDirectory %p '%s' !\n", dp->filename, 
		    dp->filename);
    if (dp->attribute & FA_DIREC) {
      if (attrib & DDL_DIRECTORY && strcmp(dp->filename, ".") != 0) {
	sprintf(temp, "[%s]", dp->filename);
	if ( (wRet = ListBoxAddString(hwnd, temp)) == LB_ERR) break;
      }
    } 
    else {
      if (attrib & DDL_EXCLUSIVE) {
	if (attrib & (DDL_READWRITE | DDL_READONLY | DDL_HIDDEN |
		      DDL_SYSTEM) )
	  if ( (wRet = ListBoxAddString(hwnd, dp->filename)) 
	      == LB_ERR) break;
      } 
      else {
	if ( (wRet = ListBoxAddString(hwnd, dp->filename)) 
	    == LB_ERR) break;
      }
    }
  }
  DOS_closedir(dp_old);
  
  if (attrib & DDL_DRIVES) {
    for (x=0;x!=MAX_DOS_DRIVES;x++) {
      if (DOS_ValidDrive(x)) {
	sprintf(temp, "[-%c-]", 'a'+x);
	if((wRet = ListBoxInsertString(hwnd, (UINT)-1, temp)) == LB_ERR) break;
      }		
    }
  }
  lphl->bRedrawFlag = OldFlag;
  if (OldFlag) {
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
  }
  dprintf_listbox(stddeb,"End of ListBoxDirectory !\n");
  return 1;  /* FIXME: Should be 0 if "filespec" is invalid */
}


int ListBoxGetItemRect(HWND hwnd, WORD wIndex, LPRECT lprect)

{
  LPLISTSTRUCT lpls;

  if ((lpls = ListBoxGetItem(hwnd, wIndex)) == NULL) return LB_ERR;

  *(lprect) = lpls->dis.rcItem;

  return 0;
}


int ListBoxSetItemHeight(HWND hwnd, WORD wIndex, long height)

{
  LPHEADLIST    lphl;
  WND          *wndPtr;
  LPLISTSTRUCT  lpls;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return LB_ERR;

  if (!(wndPtr->dwStyle & LBS_OWNERDRAWVARIABLE)) {
    lphl->StdItemHeight = (short)height;
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

    return 0;
  }

  if ((lpls = ListBoxGetItem(hwnd, wIndex)) == NULL) return LB_ERR;
  
  lpls->dis.rcItem.bottom = lpls->dis.rcItem.top + (short)height;
  InvalidateRect(hwnd, NULL, TRUE);
  UpdateWindow(hwnd);

  return 0;
}

int ListBoxDefaultItem(HWND hwnd, WND *wndPtr, 
	LPHEADLIST lphl, LPLISTSTRUCT lpls)

{
  RECT	rect;

  if (wndPtr == NULL || lphl == NULL || lpls == NULL) {
    fprintf(stderr,"ListBoxDefaultItem() // Bad Pointers !\n");
    return FALSE;
  }

  GetClientRect(hwnd, &rect);
  SetRect(&lpls->dis.rcItem, 0, 0, rect.right, lphl->StdItemHeight);

  lpls->dis.CtlType    = lphl->DrawCtlType;
  lpls->dis.CtlID      = wndPtr->wIDmenu;
  lpls->dis.itemID     = 0;
  lpls->dis.itemAction = 0;
  lpls->dis.itemState  = 0;
  lpls->dis.hwndItem   = hwnd;
  lpls->dis.hDC        = 0;
  lpls->dis.itemData   = 0;

  return TRUE;
}



int ListBoxFindNextMatch(HWND hwnd, WORD wChar)

{
  WND  	        *wndPtr;
  LPHEADLIST 	lphl;
  LPLISTSTRUCT  lpls;
  UINT	        Count;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
  if (lphl == NULL) return LB_ERR;
  lpls = lphl->lpFirst;
  if (lpls == NULL) return LB_ERR;
  if (wChar < ' ') return LB_ERR;

  if (!HasStrings(wndPtr)) return LB_ERR;

  Count = 0;
  while(lpls != NULL) {
    if (Count > lphl->ItemFocused) {
      if (*(lpls->itemText) == (char)wChar) {
	if ((wndPtr->dwStyle & LBS_MULTIPLESEL) == LBS_MULTIPLESEL) {
	  lphl->ItemFocused = Count;
	  ListBoxScrolltoFocus(hwnd);
	}
	else {
	  ListBoxSetCurSel(hwnd, Count);
	}
	SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
	return Count;
      }
    }
    lpls = (LPLISTSTRUCT)lpls->lpNext;
    Count++;
  }
  Count = 0;
  lpls = lphl->lpFirst;
  while(lpls != NULL) {
    if (*(lpls->itemText) == (char)wChar) {
      if (Count == lphl->ItemFocused)    return LB_ERR;

      if ((wndPtr->dwStyle & LBS_MULTIPLESEL) == LBS_MULTIPLESEL) {
	lphl->ItemFocused = Count;
	ListBoxScrolltoFocus(hwnd);
      }
      else {
	ListBoxSetCurSel(hwnd, Count);
      }
      SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
      InvalidateRect(hwnd, NULL, TRUE);
      UpdateWindow(hwnd);
      return Count;
    }
    lpls = lpls->lpNext;
    Count++;
  }
  return LB_ERR;
}


/************************************************************************
 * 		      	DlgDirSelect			[USER.99]
 */
BOOL DlgDirSelect(HWND hDlg, LPSTR lpStr, int nIDLBox)
{
  HWND hwnd;
  LPHEADLIST lphl;
  char s[130];

  dprintf_listbox( stddeb, "DlgDirSelect(%04X, '%s', %d) \n", hDlg, lpStr, 
		  nIDLBox );

  hwnd = GetDlgItem(hDlg, nIDLBox);
  lphl = ListBoxGetStorageHeader(hwnd);
  if( lphl->ItemFocused == -1 ) {
    dprintf_listbox( stddeb, "Nothing selected!\n" );
    return FALSE;
  }
  ListBoxGetText(hwnd, lphl->ItemFocused, (LPSTR)s, FALSE);
  dprintf_listbox( stddeb, "Selection is %s\n", s );
  if( s[0] == '[' ) {
    if( s[1] == '-' ) {
      strncpy( lpStr, s+2, strlen(s)-4 );    /* device name */
      lpStr[ strlen(s)-4 ] = 0;
      strcat( lpStr, ":" );
    }
    else {
      strncpy( lpStr, s+1, strlen(s)-2 );    /* directory name */
      lpStr[ strlen(s)-2 ] = 0;
      strcat( lpStr, "\\" );
    }
    dprintf_listbox( stddeb, "Returning %s\n", lpStr );
    return TRUE;
  }
  else {
    strcpy( lpStr, s );                     /* file name */
    dprintf_listbox( stddeb, "Returning %s\n", lpStr );
    return FALSE;
  }
}


/************************************************************************
 * 			   DlgDirList				[USER.100]
 */
int DlgDirList(HWND hDlg, LPSTR lpPathSpec, 
	int nIDLBox, int nIDStat, WORD wType)
{
  HWND	hWnd;
  int ret;
  dprintf_listbox(stddeb,"DlgDirList(%04X, '%s', %d, %d, %04X) \n",
		  hDlg, lpPathSpec, nIDLBox, nIDStat, wType);
  if (nIDLBox)
    hWnd = GetDlgItem(hDlg, nIDLBox);
  else
    hWnd = 0;
  if (hWnd)
    ListBoxResetContent(hWnd);
  if (hWnd)
    ret=ListBoxDirectory(hWnd, wType, lpPathSpec);
  else
    ret=0;
  if (nIDStat)
    {
      int drive;
      HANDLE hTemp;
      char *temp;
      drive = DOS_GetDefaultDrive();
      hTemp = USER_HEAP_ALLOC( 256 );
      temp = (char *) USER_HEAP_LIN_ADDR( hTemp );
      strcpy( temp+3, DOS_GetCurrentDir(drive) );
      if( temp[3] == '\\' ) {
	temp[1] = 'A'+drive;
	temp[2] = ':';
	SendDlgItemMessage( hDlg, nIDStat, WM_SETTEXT, 0,
                            USER_HEAP_SEG_ADDR(hTemp) + 1 );
      }
      else {
	temp[0] = 'A'+drive;
	temp[1] = ':';
	temp[2] = '\\';
	SendDlgItemMessage( hDlg, nIDStat, WM_SETTEXT, 0,
                            USER_HEAP_SEG_ADDR(hTemp) );
      }
      USER_HEAP_FREE( hTemp );
    } 
  return ret;
}


/* Returns: 0 if nothing needs to be changed */
/*          1 if FirstVisible changed */

int ListBoxScrolltoFocus(HWND hwnd)

{
  WND  *wndPtr;
  LPHEADLIST  lphl;
  short       end;

  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);

  if (lphl->ItemsCount == 0) return 0;
  if (lphl->ItemFocused == -1) return 0;

  end = lphl->FirstVisible + lphl->ItemsVisible - 2;

  if (lphl->ItemFocused < lphl->FirstVisible - 1) {
    lphl->FirstVisible = lphl->ItemFocused + 1;
  }
  else if (lphl->ItemFocused > end) {
    UINT maxFirstVisible = ListMaxFirstVisible(lphl);

    lphl->FirstVisible = lphl->ItemFocused;

    if (lphl->FirstVisible > maxFirstVisible) {
      lphl->FirstVisible = maxFirstVisible;
    }
  } else return 0;

  return 1;
}

/* Send notification "code" as part of a WM_COMMAND-message if hwnd
   has the LBS_NOTIFY style */
void ListBoxSendNotification(HWND hwnd, WORD code)
{
  WND  *wndPtr;
  LPHEADLIST  lphl;
  lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);

  if (wndPtr && (wndPtr->dwStyle & LBS_NOTIFY))
    SendMessage(lphl->hWndLogicParent, WM_COMMAND,
		wndPtr->wIDmenu, MAKELONG(hwnd, code));
}
