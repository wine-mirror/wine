/*
 * Interface code to listbox widgets
 *
 * Copyright  Martin Ayotte, 1993
 *
 */

/*
#define DEBUG_LISTBOX
*/

static char Copyright[] = "Copyright Martin Ayotte, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "windows.h"
#include "user.h"
#include "heap.h"
#include "win.h"
#include "msdos.h"
#include "wine.h"
#include "listbox.h"
#include "scroll.h"
#include "prototypes.h"

#define GMEM_ZEROINIT 0x0040


LPHEADLIST ListBoxGetWindowAndStorage(HWND hwnd, WND **wndPtr);
LPHEADLIST ListBoxGetStorageHeader(HWND hwnd);
void StdDrawListBox(HWND hwnd);
void OwnerDrawListBox(HWND hwnd);
int ListBoxFindMouse(HWND hwnd, int X, int Y);
int CreateListBoxStruct(HWND hwnd);
void ListBoxAskMeasure(WND *wndPtr, LPHEADLIST lphl, LPLISTSTRUCT lpls);
int ListBoxAddString(HWND hwnd, LPSTR newstr);
int ListBoxInsertString(HWND hwnd, UINT uIndex, LPSTR newstr);
int ListBoxGetText(HWND hwnd, UINT uIndex, LPSTR OutStr);
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


/***********************************************************************
 *           ListBoxWndProc 
 */
LONG ListBoxWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{    
	WND  *wndPtr;
	LPHEADLIST  lphl;
	HWND	hWndCtl;
	WORD	wRet;
	RECT	rect;
	int		y;
	CREATESTRUCT *createStruct;
	static RECT rectsel;
    switch(message) {
	case WM_CREATE:
		CreateListBoxStruct(hwnd);
		lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
#ifdef DEBUG_LISTBOX
		printf("ListBox WM_CREATE %lX !\n", lphl);
#endif
		createStruct = (CREATESTRUCT *)lParam;
		if (HIWORD(createStruct->lpCreateParams) != 0)
			lphl->hWndLogicParent = (HWND)HIWORD(createStruct->lpCreateParams);
		else
			lphl->hWndLogicParent = GetParent(hwnd);
		lphl->ColumnsWidth = wndPtr->rectClient.right - wndPtr->rectClient.left;
		if (wndPtr->dwStyle & WS_VSCROLL) {
			SetScrollRange(hwnd, SB_VERT, 1, lphl->ItemsCount, TRUE);
			ShowScrollBar(hwnd, SB_VERT, FALSE);
			}
		if (wndPtr->dwStyle & WS_HSCROLL) {
			SetScrollRange(hwnd, SB_HORZ, 1, 1, TRUE);
			ShowScrollBar(hwnd, SB_HORZ, FALSE);
			}
		if ((wndPtr->dwStyle & LBS_OWNERDRAWFIXED) == LBS_OWNERDRAWFIXED) {
			}
		return 0;
	case WM_DESTROY:
		lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
		if (lphl == 0) return 0;
		ListBoxResetContent(hwnd);
		free(lphl);
		*((LPHEADLIST *)&wndPtr->wExtra[1]) = 0;
#ifdef DEBUG_LISTBOX
		printf("ListBox WM_DESTROY %lX !\n", lphl);
#endif
		return 0;

	case WM_VSCROLL:
#ifdef DEBUG_LISTBOX
		printf("ListBox WM_VSCROLL w=%04X l=%08X !\n", wParam, lParam);
#endif
		lphl = ListBoxGetStorageHeader(hwnd);
		if (lphl == NULL) return 0;
		y = lphl->FirstVisible;
		switch(wParam) {
			case SB_LINEUP:
				if (lphl->FirstVisible > 1)	
					lphl->FirstVisible--;
				break;
			case SB_LINEDOWN:
				if (lphl->FirstVisible < lphl->ItemsCount)
					lphl->FirstVisible++;
				break;
			case SB_PAGEUP:
				if (lphl->FirstVisible > 1)  
					lphl->FirstVisible -= lphl->ItemsVisible;
				break;
			case SB_PAGEDOWN:
				if (lphl->FirstVisible < lphl->ItemsCount)  
					lphl->FirstVisible += lphl->ItemsVisible;
				break;
			case SB_THUMBTRACK:
				lphl->FirstVisible = LOWORD(lParam);
				break;
			}
		if (lphl->FirstVisible < 1)    lphl->FirstVisible = 1;
		if (lphl->FirstVisible > lphl->ItemsCount)
			lphl->FirstVisible = lphl->ItemsCount;
		if (y != lphl->FirstVisible) {
			SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
			InvalidateRect(hwnd, NULL, TRUE);
			UpdateWindow(hwnd);
			}
		return 0;
	
	case WM_HSCROLL:
#ifdef DEBUG_LISTBOX
		printf("ListBox WM_HSCROLL w=%04X l=%08X !\n", wParam, lParam);
#endif
		lphl = ListBoxGetStorageHeader(hwnd);
		if (lphl == NULL) return 0;
		y = lphl->FirstVisible;
		switch(wParam) {
			case SB_LINEUP:
				if (lphl->FirstVisible > 1)
					lphl->FirstVisible -= lphl->ItemsPerColumn;
				break;
			case SB_LINEDOWN:
				if (lphl->FirstVisible < lphl->ItemsCount)
					lphl->FirstVisible += lphl->ItemsPerColumn;
				break;
			case SB_PAGEUP:
				if (lphl->FirstVisible > 1 && lphl->ItemsPerColumn != 0)  
					lphl->FirstVisible -= lphl->ItemsVisible /
					lphl->ItemsPerColumn * lphl->ItemsPerColumn;
				break;
			case SB_PAGEDOWN:
				if (lphl->FirstVisible < lphl->ItemsCount &&
							lphl->ItemsPerColumn != 0)  
					lphl->FirstVisible += lphl->ItemsVisible /
					lphl->ItemsPerColumn * lphl->ItemsPerColumn;
				break;
			case SB_THUMBTRACK:
				lphl->FirstVisible = lphl->ItemsPerColumn * 
								(LOWORD(lParam) - 1) + 1;
				break;
			}
		if (lphl->FirstVisible < 1)    lphl->FirstVisible = 1;
		if (lphl->FirstVisible > lphl->ItemsCount)
			lphl->FirstVisible = lphl->ItemsCount;
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
	
	case WM_LBUTTONDOWN:
		SetFocus(hwnd);
		SetCapture(hwnd);
		lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
	        if (lphl == NULL) return 0;
		lphl->PrevFocused = lphl->ItemFocused;
	        y = ListBoxFindMouse(hwnd, LOWORD(lParam), HIWORD(lParam));
		if ((wndPtr->dwStyle & LBS_MULTIPLESEL) == LBS_MULTIPLESEL) {
		    lphl->ItemFocused = y;
		    wRet = ListBoxGetSel(hwnd, y);
		    ListBoxSetSel(hwnd, y, !wRet);
		    }
		else {
		    ListBoxSetCurSel(hwnd, y);
		    }
		ListBoxGetItemRect(hwnd, y, &rectsel);
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		return 0;
	case WM_LBUTTONUP:
		ReleaseCapture();
		lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
		if (lphl == NULL) return 0;
		if (lphl->PrevFocused != lphl->ItemFocused)
			SendMessage(lphl->hWndLogicParent, WM_COMMAND, wndPtr->wIDmenu,
											MAKELONG(hwnd, LBN_SELCHANGE));
		return 0;
	case WM_RBUTTONUP:
	case WM_LBUTTONDBLCLK:
		lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
		if (lphl == NULL) return 0;
		SendMessage(lphl->hWndLogicParent, WM_COMMAND, wndPtr->wIDmenu,
										MAKELONG(hwnd, LBN_DBLCLK));
		printf("ListBox Send LBN_DBLCLK !\n");
		return 0;
	case WM_MOUSEMOVE:
		if ((wParam & MK_LBUTTON) != 0) {
			y = HIWORD(lParam);
			if (y < 4) {
				lphl = ListBoxGetStorageHeader(hwnd);
				if (lphl->FirstVisible > 1) {
					lphl->FirstVisible--;
					if (wndPtr->dwStyle & WS_VSCROLL)
					SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
					InvalidateRect(hwnd, NULL, TRUE);
					UpdateWindow(hwnd);
					break;
					}
				}
			GetClientRect(hwnd, &rect);
			if (y > (rect.bottom - 4)) {
				lphl = ListBoxGetStorageHeader(hwnd);
				if (lphl->FirstVisible < lphl->ItemsCount) {
					lphl->FirstVisible++;
					if (wndPtr->dwStyle & WS_VSCROLL)
					SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
					InvalidateRect(hwnd, NULL, TRUE);
					UpdateWindow(hwnd);
					break;
					}
				}
			if ((y > 0) && (y < (rect.bottom - 4))) {
				if ((y < rectsel.top) || (y > rectsel.bottom)) {
					wRet = ListBoxFindMouse(hwnd, LOWORD(lParam), HIWORD(lParam));
					if ((wndPtr->dwStyle & LBS_MULTIPLESEL) == LBS_MULTIPLESEL) {
						lphl->ItemFocused = wRet;
						}
					else {
						ListBoxSetCurSel(hwnd, wRet);
						}
					ListBoxGetItemRect(hwnd, wRet, &rectsel);
					InvalidateRect(hwnd, NULL, TRUE);
					UpdateWindow(hwnd);
					}
				}
			}
		break;
	case WM_KEYDOWN:
		lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
		if (lphl == NULL) return 0;
		switch(wParam) {
			case VK_TAB:
				hWndCtl = GetNextDlgTabItem(lphl->hWndLogicParent,
					hwnd, !(GetKeyState(VK_SHIFT) < 0));
				SetFocus(hWndCtl);
				if ((GetKeyState(VK_SHIFT) < 0))
					printf("ListBox PreviousDlgTabItem %04X !\n", hWndCtl);
				else
					printf("ListBox NextDlgTabItem %04X !\n", hWndCtl);
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
		lphl->FirstVisible = lphl->ItemFocused / lphl->ItemsVisible * 
											lphl->ItemsVisible + 1;
		if (lphl->FirstVisible < 1) lphl->FirstVisible = 1;
		if ((wndPtr->dwStyle & LBS_MULTIPLESEL) != LBS_MULTIPLESEL) {
			ListBoxSetCurSel(hwnd, lphl->ItemFocused);
			}
		if (wndPtr->dwStyle & WS_VSCROLL)
			SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		break;
	case WM_PAINT:
		wndPtr = WIN_FindWndPtr(hwnd);
		if ((wndPtr->dwStyle & LBS_OWNERDRAWFIXED) == LBS_OWNERDRAWFIXED) {
			OwnerDrawListBox(hwnd);
			break;
			}
		if ((wndPtr->dwStyle & LBS_OWNERDRAWVARIABLE) == LBS_OWNERDRAWVARIABLE) {
			OwnerDrawListBox(hwnd);
			break;
			}
		StdDrawListBox(hwnd);
		break;
	case WM_SETFOCUS:
#ifdef DEBUG_LISTBOX
		printf("ListBox WM_SETFOCUS !\n");
#endif
		lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
		break;
	case WM_KILLFOCUS:
#ifdef DEBUG_LISTBOX
		printf("ListBox WM_KILLFOCUS !\n");
#endif
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		break;

    case LB_RESETCONTENT:
#ifdef DEBUG_LISTBOX
		printf("ListBox LB_RESETCONTENT !\n");
#endif
		ListBoxResetContent(hwnd);
		return 0;
    case LB_DIR:
#ifdef DEBUG_LISTBOX
		printf("ListBox LB_DIR !\n");
#endif
		wRet = ListBoxDirectory(hwnd, wParam, (LPSTR)lParam);
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		return wRet;
	case LB_ADDSTRING:
		wRet = ListBoxAddString(hwnd, (LPSTR)lParam);
		return wRet;
	case LB_GETTEXT:
		wRet = ListBoxGetText(hwnd, wParam, (LPSTR)lParam);
#ifdef DEBUG_LISTBOX
		printf("ListBox LB_GETTEXT #%u '%s' !\n", wParam, (LPSTR)lParam);
#endif
		return wRet;
	case LB_INSERTSTRING:
		wRet = ListBoxInsertString(hwnd, wParam, (LPSTR)lParam);
		return wRet;
	case LB_DELETESTRING:
		printf("ListBox LB_DELETESTRING #%u !\n", wParam);
		wRet = ListBoxDeleteString(hwnd, wParam);
		return wRet;
	case LB_FINDSTRING:
		wRet = ListBoxFindString(hwnd, wParam, (LPSTR)lParam);
		return wRet;
	case LB_GETCARETINDEX:
		return wRet;
	case LB_GETCOUNT:
		lphl = ListBoxGetStorageHeader(hwnd);
		return lphl->ItemsCount;
	case LB_GETCURSEL:
		lphl = ListBoxGetStorageHeader(hwnd);
#ifdef DEBUG_LISTBOX
		printf("ListBox LB_GETCURSEL %u !\n", lphl->ItemFocused);
#endif
		return lphl->ItemFocused;
	case LB_GETHORIZONTALEXTENT:
		return wRet;
	case LB_GETITEMDATA:
		return wRet;
	case LB_GETITEMHEIGHT:
		return wRet;
	case LB_GETITEMRECT:
		return wRet;
	case LB_GETSEL:
		wRet = ListBoxGetSel(hwnd, wParam);
		return wRet;
	case LB_GETSELCOUNT:
		return wRet;
	case LB_GETSELITEMS:
		return wRet;
	case LB_GETTEXTLEN:
		return wRet;
	case LB_GETTOPINDEX:
		return wRet;
	case LB_SELECTSTRING:
		return wRet;
	case LB_SELITEMRANGE:
		return wRet;
	case LB_SETCARETINDEX:
		return wRet;
	case LB_SETCOLUMNWIDTH:
		lphl = ListBoxGetStorageHeader(hwnd);
		lphl->ColumnsWidth = wParam;
		break;
	case LB_SETHORIZONTALEXTENT:
		return wRet;
	case LB_SETITEMDATA:
		return wRet;
	case LB_SETTABSTOPS:
		return wRet;
	case LB_SETCURSEL:
#ifdef DEBUG_LISTBOX
		printf("ListBox LB_SETCURSEL wParam=%x !\n", wParam);
#endif
		wRet = ListBoxSetCurSel(hwnd, wParam);
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		return wRet;
	case LB_SETSEL:
		printf("ListBox LB_SETSEL wParam=%x lParam=%lX !\n", wParam, lParam);
		wRet = ListBoxSetSel(hwnd, LOWORD(lParam), wParam);
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		return wRet;
	case LB_SETTOPINDEX:
		printf("ListBox LB_SETTOPINDEX wParam=%x !\n", wParam);
		lphl = ListBoxGetStorageHeader(hwnd);
		lphl->FirstVisible = wParam;
		if (wndPtr->dwStyle & WS_VSCROLL)
		SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		break;
	case LB_SETITEMHEIGHT:
#ifdef DEBUG_LISTBOX
		printf("ListBox LB_SETITEMHEIGHT wParam=%x lParam=%lX !\n", wParam, lParam);
#endif
		wRet = ListBoxSetItemHeight(hwnd, wParam, lParam);
		return wRet;

	default:
		return DefWindowProc( hwnd, message, wParam, lParam );
    }
return 0;
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


void StdDrawListBox(HWND hwnd)
{
	WND 	*wndPtr;
	LPHEADLIST  lphl;
	LPLISTSTRUCT lpls;
	PAINTSTRUCT ps;
	HBRUSH 	hBrush;
	int 	OldBkMode;
	DWORD 	dwOldTextColor;
	HWND	hWndParent;
	HDC 	hdc;
	RECT 	rect;
	UINT	i, h, h2, maxwidth, ipc;
	char	C[128];
	h = 0;
	hdc = BeginPaint( hwnd, &ps );
    	if (!IsWindowVisible(hwnd)) {
	    EndPaint( hwnd, &ps );
	    return;
	    }
	lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
	if (lphl == NULL) goto EndOfPaint;
	hBrush = SendMessage(lphl->hWndLogicParent, WM_CTLCOLOR, (WORD)hdc,
		    MAKELONG(hwnd, CTLCOLOR_LISTBOX));
	if (hBrush == (HBRUSH)NULL)  hBrush = GetStockObject(WHITE_BRUSH);
	GetClientRect(hwnd, &rect);
/*
	if (wndPtr->dwStyle & WS_VSCROLL) rect.right -= 16;
	if (wndPtr->dwStyle & WS_HSCROLL) rect.bottom -= 16;
*/
	FillRect(hdc, &rect, hBrush);
	maxwidth = rect.right;
	rect.right = lphl->ColumnsWidth;
	if (lphl->ItemsCount == 0) goto EndOfPaint;
	lpls = lphl->lpFirst;
	if (lpls == NULL) goto EndOfPaint;
	lphl->ItemsVisible = 0;
	lphl->ItemsPerColumn = ipc = 0;
	for(i = 1; i <= lphl->ItemsCount; i++) {
	    if (i >= lphl->FirstVisible) {
	        if (lpls == NULL) break;
		if ((h + lpls->dis.rcItem.bottom - lpls->dis.rcItem.top) > rect.bottom) {
		    if ((wndPtr->dwStyle & LBS_MULTICOLUMN) == LBS_MULTICOLUMN) {
			lphl->ItemsPerColumn = max(lphl->ItemsPerColumn, ipc);
			ipc = 0;
			h = 0;
			rect.left += lphl->ColumnsWidth;
			rect.right += lphl->ColumnsWidth;
			if (rect.left > maxwidth) break;
			}
		    else 
			break;
		    }
		h2 = lpls->dis.rcItem.bottom - lpls->dis.rcItem.top;
		lpls->dis.rcItem.top = h;
		lpls->dis.rcItem.bottom = h + h2;
		lpls->dis.rcItem.left = rect.left;
		lpls->dis.rcItem.right = rect.right;
		OldBkMode = SetBkMode(hdc, TRANSPARENT);
		if (lpls->dis.itemState != 0) {
			dwOldTextColor = SetTextColor(hdc, 0x00FFFFFFL);
		    FillRect(hdc, &lpls->dis.rcItem, GetStockObject(BLACK_BRUSH));
		    }
		TextOut(hdc, rect.left + 5, h + 2, (char *)lpls->dis.itemData, 
			strlen((char *)lpls->dis.itemData));
		if (lpls->dis.itemState != 0) {
			SetTextColor(hdc, dwOldTextColor);
		    }
		SetBkMode(hdc, OldBkMode);
		if ((lphl->ItemFocused == i - 1) && GetFocus() == hwnd) {
		    DrawFocusRect(hdc, &lpls->dis.rcItem);
		    }
		h += h2;
		lphl->ItemsVisible++;
		ipc++;
		}
	    if (lpls->lpNext == NULL) goto EndOfPaint;
	    lpls = (LPLISTSTRUCT)lpls->lpNext;
	}
EndOfPaint:
    EndPaint( hwnd, &ps );
    if ((lphl->ItemsCount > lphl->ItemsVisible) &
	(wndPtr->dwStyle & WS_VSCROLL)) {
/*
        InvalidateRect(wndPtr->hWndVScroll, NULL, TRUE);
        UpdateWindow(wndPtr->hWndVScroll);
*/
 	}
}



void OwnerDrawListBox(HWND hwnd)
{
	WND 	*wndPtr;
	LPHEADLIST  lphl;
	LPLISTSTRUCT lpls;
	PAINTSTRUCT ps;
	HBRUSH 	hBrush;
	HWND	hWndParent;
	HDC hdc;
	RECT rect;
	UINT  i, h, h2, maxwidth;
	char	C[128];
	h = 0;
	hdc = BeginPaint(hwnd, &ps);
    	if (!IsWindowVisible(hwnd)) {
	    EndPaint( hwnd, &ps );
	    return;
	    }
	lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
	if (lphl == NULL) goto EndOfPaint;
	hBrush = SendMessage(lphl->hWndLogicParent, WM_CTLCOLOR, (WORD)hdc,
		    MAKELONG(hwnd, CTLCOLOR_LISTBOX));
	if (hBrush == (HBRUSH)NULL)  hBrush = GetStockObject(WHITE_BRUSH);
	GetClientRect(hwnd, &rect);
	if (wndPtr->dwStyle & WS_VSCROLL) rect.right -= 16;
	if (wndPtr->dwStyle & WS_HSCROLL) rect.bottom -= 16;
	FillRect(hdc, &rect, hBrush);
	maxwidth = rect.right;
	rect.right = lphl->ColumnsWidth;
	if (lphl->ItemsCount == 0) goto EndOfPaint;
	lpls = lphl->lpFirst;
	if (lpls == NULL) goto EndOfPaint;
	lphl->ItemsVisible = 0;
	for (i = 1; i <= lphl->ItemsCount; i++) {
	    if (i >= lphl->FirstVisible) {
		lpls->dis.hDC = hdc;
		lpls->dis.itemID = i - 1;
		h2 = lpls->dis.rcItem.bottom - lpls->dis.rcItem.top;
		lpls->dis.rcItem.top = h;
		lpls->dis.rcItem.bottom = h + h2;
		lpls->dis.rcItem.left = rect.left;
		lpls->dis.rcItem.right = rect.right;
		lpls->dis.itemAction = ODA_DRAWENTIRE;
		if (lpls->dis.itemState != 0) {
		    lpls->dis.itemAction |= ODA_SELECT;
		    }
		if (lphl->ItemFocused == i - 1) {
		    lpls->dis.itemAction |= ODA_FOCUS;
		    }
#ifdef DEBUT_LISTBOX
		printf("LBOX WM_DRAWITEM #%d left=%d top=%d right=%d bottom=%d !\n", 
			i, lpls->dis.rcItem.left, lpls->dis.rcItem.top, 
			lpls->dis.rcItem.right, lpls->dis.rcItem.bottom);
		printf("LBOX WM_DRAWITEM Parent=%X &dis=%lX CtlID=%u !\n", 
			hWndParent, (LONG)&lpls->dis, lpls->dis.CtlID);
		printf("LBOX WM_DRAWITEM '%s' !\n", lpls->dis.itemData);
#endif
		SendMessage(lphl->hWndLogicParent, WM_DRAWITEM, i, (LPARAM)&lpls->dis);
		if (lpls->dis.itemState != 0) {
		    InvertRect(hdc, &lpls->dis.rcItem);
		    }
		h += h2;
		lphl->ItemsVisible++;
		if (h > rect.bottom) goto EndOfPaint;
		}
	    if (lpls->lpNext == NULL) goto EndOfPaint;
	    lpls = (LPLISTSTRUCT)lpls->lpNext;
	}
EndOfPaint:
    EndPaint( hwnd, &ps );
    if ((lphl->ItemsCount > lphl->ItemsVisible) &
	(wndPtr->dwStyle & WS_VSCROLL)) {
/*
        InvalidateRect(wndPtr->hWndVScroll, NULL, TRUE);
        UpdateWindow(wndPtr->hWndVScroll);
*/
        }
}



int ListBoxFindMouse(HWND hwnd, int X, int Y)
{
    WND 		*wndPtr;
    LPHEADLIST 		lphl;
    LPLISTSTRUCT	lpls;
    RECT 		rect;
    UINT  		i, h, h2, w, w2;
    char		C[128];
    lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
    if (lphl == NULL) return LB_ERR;
    if (lphl->ItemsCount == 0) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    GetClientRect(hwnd, &rect);
    if (wndPtr->dwStyle & WS_VSCROLL) rect.right -= 16;
    if (wndPtr->dwStyle & WS_HSCROLL) rect.bottom -= 16;
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



int CreateListBoxStruct(HWND hwnd)
{
    WND  *wndPtr;
    LPHEADLIST lphl;
    wndPtr = WIN_FindWndPtr(hwnd);
    lphl = (LPHEADLIST)malloc(sizeof(HEADLIST));
    lphl->lpFirst = NULL;
    *((LPHEADLIST *)&wndPtr->wExtra[1]) = lphl; 	/* HEAD of List */
    lphl->ItemsCount = 0;
    lphl->ItemsVisible = 0;
    lphl->FirstVisible = 1;
    lphl->StdItemHeight = 15;
    lphl->DrawCtlType = ODT_LISTBOX;
    return TRUE;
}


void ListBoxAskMeasure(WND *wndPtr, LPHEADLIST lphl, LPLISTSTRUCT lpls)  
{
	MEASUREITEMSTRUCT *measure;
	HANDLE hTemp = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(MEASUREITEMSTRUCT));
	measure = (MEASUREITEMSTRUCT *) USER_HEAP_ADDR(hTemp);
	if (measure == NULL) return;
	measure->CtlType = ODT_LISTBOX;
	measure->CtlID = wndPtr->wIDmenu;
	measure->itemID = lpls->dis.itemID;
	measure->itemWidth = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
	measure->itemHeight = 0;
	measure->itemData = lpls->dis.itemData;
	SendMessage(lphl->hWndLogicParent, WM_MEASUREITEM, 0, (DWORD)measure);
	lpls->dis.rcItem.right = lpls->dis.rcItem.left + measure->itemWidth;
	lpls->dis.rcItem.bottom = lpls->dis.rcItem.top + measure->itemHeight;
	USER_HEAP_FREE(hTemp);			
}


int ListBoxAddString(HWND hwnd, LPSTR newstr)
{
    WND  	*wndPtr;
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls, lplsnew;
    HANDLE 	hTemp;
    LPSTR 	str;
    lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
    if (lphl == NULL) return LB_ERR;
    hTemp = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(LISTSTRUCT));
    lplsnew = (LPLISTSTRUCT) USER_HEAP_ADDR(hTemp);
    lpls = lphl->lpFirst;
    if (lpls != NULL) {
	while(lpls->lpNext != NULL) {
	    lpls = (LPLISTSTRUCT)lpls->lpNext;
	    }
	lpls->lpNext = lplsnew;
 	}
    else
	lphl->lpFirst = lplsnew;
    lphl->ItemsCount++;
#ifdef DEBUG_LISTBOX
    printf("Items Count = %u\n", lphl->ItemsCount);
#endif
    hTemp = 0;
    if ((wndPtr->dwStyle & LBS_HASSTRINGS) != LBS_HASSTRINGS) {
	if (((wndPtr->dwStyle & LBS_OWNERDRAWFIXED) != LBS_OWNERDRAWFIXED) &&
	    ((wndPtr->dwStyle & LBS_OWNERDRAWVARIABLE) != LBS_OWNERDRAWVARIABLE)) {
	    hTemp = USER_HEAP_ALLOC(GMEM_MOVEABLE, strlen(newstr) + 1);
	    str = (LPSTR)USER_HEAP_ADDR(hTemp);
	    if (str == NULL) return LB_ERRSPACE;
	    strcpy(str, newstr);
	    newstr = str;
#ifdef DEBUG_LISTBOX
	    printf("ListBoxAddString// after strcpy '%s'\n", str);
#endif
	    }
	}
    ListBoxDefaultItem(hwnd, wndPtr, lphl, lplsnew);
    lplsnew->hMem = hTemp;
    lplsnew->lpNext = NULL;
    lplsnew->dis.itemID = lphl->ItemsCount;
    lplsnew->dis.itemData = (DWORD)newstr;
    lplsnew->hData = hTemp;
	if ((wndPtr->dwStyle & LBS_OWNERDRAWVARIABLE) == LBS_OWNERDRAWVARIABLE) 
		ListBoxAskMeasure(wndPtr, lphl, lplsnew);
    if (wndPtr->dwStyle & WS_VSCROLL)
	SetScrollRange(hwnd, SB_VERT, 1, lphl->ItemsCount, 
	    (lphl->FirstVisible != 1));
    if ((wndPtr->dwStyle & WS_HSCROLL) && lphl->ItemsPerColumn != 0)
	SetScrollRange(hwnd, SB_HORZ, 1, lphl->ItemsVisible / 
	    lphl->ItemsPerColumn + 1, (lphl->FirstVisible != 1));
    if (lphl->FirstVisible >= (lphl->ItemsCount - lphl->ItemsVisible)) {
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        }
    if ((lphl->ItemsCount - lphl->FirstVisible) == lphl->ItemsVisible) {
	if (wndPtr->dwStyle & WS_VSCROLL)
	    ShowScrollBar(hwnd, SB_VERT, TRUE);
	if (wndPtr->dwStyle & WS_HSCROLL)
	    ShowScrollBar(hwnd, SB_HORZ, TRUE);
	}
    return lphl->ItemsCount;
}


int ListBoxInsertString(HWND hwnd, UINT uIndex, LPSTR newstr)
{
	WND  	*wndPtr;
	LPHEADLIST 	lphl;
	LPLISTSTRUCT lpls, lplsnew;
	HANDLE	hTemp;
	LPSTR	str;
	UINT	Count;
	lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
	if (lphl == NULL) return LB_ERR;
	if (uIndex >= lphl->ItemsCount) return LB_ERR;
	lpls = lphl->lpFirst;
	if (lpls == NULL) return LB_ERR;
	if (uIndex > lphl->ItemsCount) return LB_ERR;
	for(Count = 1; Count < uIndex; Count++) {
		if (lpls->lpNext == NULL) return LB_ERR;
		lpls = (LPLISTSTRUCT)lpls->lpNext;
		}
	hTemp = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(LISTSTRUCT));
	lplsnew = (LPLISTSTRUCT) USER_HEAP_ADDR(hTemp);
	ListBoxDefaultItem(hwnd, wndPtr, lphl, lplsnew);
	lplsnew->hMem = hTemp;
	lpls->lpNext = lplsnew;
	lphl->ItemsCount++;
	hTemp = 0;
	if ((wndPtr->dwStyle & LBS_HASSTRINGS) != LBS_HASSTRINGS) {
		if (((wndPtr->dwStyle & LBS_OWNERDRAWFIXED) != LBS_OWNERDRAWFIXED) &&
			((wndPtr->dwStyle & LBS_OWNERDRAWVARIABLE) != LBS_OWNERDRAWVARIABLE)) {
			hTemp = USER_HEAP_ALLOC(GMEM_MOVEABLE, strlen(newstr) + 1);
			str = (LPSTR)USER_HEAP_ADDR(hTemp);
			if (str == NULL) return LB_ERRSPACE;
			strcpy(str, newstr);
			newstr = str;
			}
		}
	lplsnew->lpNext = NULL;
	lplsnew->dis.itemID = lphl->ItemsCount;
	lplsnew->dis.itemData = (DWORD)newstr;
	lplsnew->hData = hTemp;
	if ((wndPtr->dwStyle & LBS_OWNERDRAWVARIABLE) == LBS_OWNERDRAWVARIABLE) 
		ListBoxAskMeasure(wndPtr, lphl, lplsnew);
	if (wndPtr->dwStyle & WS_VSCROLL)
		SetScrollRange(hwnd, SB_VERT, 1, lphl->ItemsCount, 
						    (lphl->FirstVisible != 1));
	if ((wndPtr->dwStyle & WS_HSCROLL) && lphl->ItemsPerColumn != 0)
		SetScrollRange(hwnd, SB_HORZ, 1, lphl->ItemsVisible / 
			lphl->ItemsPerColumn + 1, (lphl->FirstVisible != 1));
	if (((lphl->ItemsCount - lphl->FirstVisible) == lphl->ItemsVisible) && 
		(lphl->ItemsVisible != 0)) {
		if (wndPtr->dwStyle & WS_VSCROLL) ShowScrollBar(hwnd, SB_VERT, TRUE);
		if (wndPtr->dwStyle & WS_HSCROLL) ShowScrollBar(hwnd, SB_HORZ, TRUE);
		}
	if ((lphl->FirstVisible <= uIndex) &&
		((lphl->FirstVisible + lphl->ItemsVisible) >= uIndex)) {
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		}
	return lphl->ItemsCount;
}


int ListBoxGetText(HWND hwnd, UINT uIndex, LPSTR OutStr)
{
    WND  	*wndPtr;
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls;
    UINT	Count;
    lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
    if (lphl == NULL) return LB_ERR;
    if (uIndex >= lphl->ItemsCount) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    if (uIndex > lphl->ItemsCount) return LB_ERR;
    for(Count = 0; Count < uIndex; Count++) {
	if (lpls->lpNext == NULL) return LB_ERR;
	lpls = (LPLISTSTRUCT)lpls->lpNext;
    }
    if (((wndPtr->dwStyle & LBS_OWNERDRAWFIXED) == LBS_OWNERDRAWFIXED) ||
	    ((wndPtr->dwStyle & LBS_OWNERDRAWVARIABLE) == LBS_OWNERDRAWVARIABLE)) {
	if ((wndPtr->dwStyle & LBS_HASSTRINGS) != LBS_HASSTRINGS) { 
	    *((long *)OutStr) = lpls->dis.itemData;
	    return 4;
	    }
	}
	
    strcpy(OutStr, (char *)lpls->dis.itemData);
    return strlen(OutStr);
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
    if (uIndex > lphl->ItemsCount) return LB_ERR;
    for(Count = 1; Count < uIndex; Count++) {
	if (lpls->lpNext == NULL) return LB_ERR;
	lpls2 = lpls;
	lpls = (LPLISTSTRUCT)lpls->lpNext;
    }
    lpls2->lpNext = (LPLISTSTRUCT)lpls->lpNext;
    lphl->ItemsCount--;
    if (lpls->hData != 0) USER_HEAP_FREE(lpls->hData);
    if (lpls->hMem != 0) USER_HEAP_FREE(lpls->hMem);
    if (wndPtr->dwStyle & WS_VSCROLL)
	SetScrollRange(hwnd, SB_VERT, 1, lphl->ItemsCount, TRUE);
    if ((wndPtr->dwStyle & WS_HSCROLL) && lphl->ItemsPerColumn != 0)
	SetScrollRange(hwnd, SB_HORZ, 1, lphl->ItemsVisible / 
	    lphl->ItemsPerColumn + 1, TRUE);
    if (lphl->ItemsCount < lphl->ItemsVisible) {
	if (wndPtr->dwStyle & WS_VSCROLL)
	    ShowScrollBar(hwnd, SB_VERT, FALSE);
	if (wndPtr->dwStyle & WS_HSCROLL)
	    ShowScrollBar(hwnd, SB_HORZ, FALSE);
	}
    if ((lphl->FirstVisible <= uIndex) &&
        ((lphl->FirstVisible + lphl->ItemsVisible) >= uIndex)) {
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        }
    return lphl->ItemsCount;
}


int ListBoxFindString(HWND hwnd, UINT nFirst, LPSTR MatchStr)
{
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls;
    UINT	Count;
    lphl = ListBoxGetStorageHeader(hwnd);
    if (lphl == NULL) return LB_ERR;
    if (nFirst > lphl->ItemsCount) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    Count = 0;
    while(lpls != NULL) {
    	if (strcmp((char *)lpls->dis.itemData, MatchStr) == 0) return Count;
	lpls = (LPLISTSTRUCT)lpls->lpNext;
	Count++;
        }
    return LB_ERR;
}


int ListBoxResetContent(HWND hwnd)
{
    WND  *wndPtr;
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls, lpls2;
    UINT	i;
    lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
    if (lphl == NULL) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    for(i = 0; i <= lphl->ItemsCount; i++) {
	lpls2 = lpls;
	lpls = (LPLISTSTRUCT)lpls->lpNext;
	if (i != 0) {
#ifdef DEBUG_LISTBOX
	    printf("ResetContent #%u\n", i);
#endif
	    if (lpls2->hData != 0) USER_HEAP_FREE(lpls->hData);
	    if (lpls2->hMem != 0) USER_HEAP_FREE(lpls->hMem);
	    }  
	if (lpls == NULL)  break;
    }
    lphl->lpFirst = NULL;
    lphl->FirstVisible = 1;
    lphl->ItemsCount = 0;
    lphl->ItemFocused = 0;
    lphl->PrevFocused = 0;
    if ((wndPtr->dwStyle && LBS_NOTIFY) != 0)
	SendMessage(wndPtr->hwndParent, WM_COMMAND, 
    	    wndPtr->wIDmenu, MAKELONG(hwnd, LBN_SELCHANGE));

    if (wndPtr->dwStyle & WS_VSCROLL)
	SetScrollRange(hwnd, SB_VERT, 1, lphl->ItemsCount, TRUE);
    if ((wndPtr->dwStyle & WS_HSCROLL) && lphl->ItemsPerColumn != 0)
	SetScrollRange(hwnd, SB_HORZ, 1, lphl->ItemsVisible / 
	    lphl->ItemsPerColumn + 1, TRUE);
    if (wndPtr->dwStyle & WS_VSCROLL)
	ShowScrollBar(hwnd, SB_VERT, FALSE);
    if (wndPtr->dwStyle & WS_HSCROLL)
	ShowScrollBar(hwnd, SB_HORZ, FALSE);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    return TRUE;
}


int ListBoxSetCurSel(HWND hwnd, WORD wIndex)
{
    WND  *wndPtr;
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls, lpls2;
    UINT	i;
    lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
    if (lphl == NULL) return LB_ERR;
    lphl->ItemFocused = LB_ERR;
    if (wIndex >= lphl->ItemsCount) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    for(i = 0; i < lphl->ItemsCount; i++) {
	lpls2 = lpls;
	lpls = (LPLISTSTRUCT)lpls->lpNext;
	if (i == wIndex)
	    lpls2->dis.itemState = 1;
	else 
	    if (lpls2->dis.itemState != 0)
	        lpls2->dis.itemState = 0;
	if (lpls == NULL)  break;
    }
    lphl->ItemFocused = wIndex;
    if ((wndPtr->dwStyle && LBS_NOTIFY) != 0)
	SendMessage(wndPtr->hwndParent, WM_COMMAND, 
    	    wndPtr->wIDmenu, MAKELONG(hwnd, LBN_SELCHANGE));
    return LB_ERR;
}



int ListBoxSetSel(HWND hwnd, WORD wIndex, WORD state)
{
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls, lpls2;
    UINT	i;
    lphl = ListBoxGetStorageHeader(hwnd);
    if (lphl == NULL) return LB_ERR;
    if (wIndex >= lphl->ItemsCount) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    for(i = 0; i < lphl->ItemsCount; i++) {
	lpls2 = lpls;
	lpls = (LPLISTSTRUCT)lpls->lpNext;
	if ((i == wIndex) || (wIndex == (WORD)-1)) {
	    lpls2->dis.itemState = state;
	    break;
	    }  
	if (lpls == NULL)  break;
    }
    return LB_ERR;
}


int ListBoxGetSel(HWND hwnd, WORD wIndex)
{
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls, lpls2;
    UINT	i;
    lphl = ListBoxGetStorageHeader(hwnd);
    if (lphl == NULL) return LB_ERR;
    if (wIndex >= lphl->ItemsCount) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    for(i = 0; i < lphl->ItemsCount; i++) {
	lpls2 = lpls;
	lpls = (LPLISTSTRUCT)lpls->lpNext;
	if (i == wIndex) {
	    return lpls2->dis.itemState;
	    }  
	if (lpls == NULL)  break;
    }
    return LB_ERR;
}


int ListBoxDirectory(HWND hwnd, UINT attrib, LPSTR filespec)
{
	struct dosdirent *dp;
	struct stat	st;
	int	x, wRet;
	char 	temp[256];
#ifdef DEBUG_LISTBOX
	fprintf(stderr,"ListBoxDirectory: %s, %4x\n",filespec,attrib);
#endif
	if ((dp = (struct dosdirent *)DOS_opendir(filespec)) ==NULL) return 0;
	while (1) {
		dp = (struct dosdirent *)DOS_readdir(dp);
		if (!dp->inuse)	break;
#ifdef DEBUG_LISTBOX
		printf("ListBoxDirectory %08X '%s' !\n", dp->filename, dp->filename);
#endif
		if (dp->attribute & FA_DIREC) {
			if (attrib & DDL_DIRECTORY) {
				sprintf(temp, "[%s]", dp->filename);
				if ( (wRet = ListBoxAddString(hwnd, temp)) == LB_ERR) break;
			}
		} else 
		if (attrib & DDL_EXCLUSIVE) {
	    		if (attrib & (DDL_READWRITE | DDL_READONLY | DDL_HIDDEN | DDL_SYSTEM) )
		    		if ( (wRet = ListBoxAddString(hwnd, dp->filename)) == LB_ERR)
		    			break;
		} else {
			if ( (wRet = ListBoxAddString(hwnd, dp->filename)) == LB_ERR)
	    			break;
		}
	}
	DOS_closedir(dp);
	
	if (attrib & DDL_DRIVES)
	{
		for (x=0;x!=MAX_DOS_DRIVES;x++)
		{
			if (DOS_ValidDrive(x))
			{
				sprintf(temp, "[-%c-]", 'A'+x);
				if ( (wRet = ListBoxAddString(hwnd, temp)) == LB_ERR)
	    				break;
			}		
		}
	}
#ifdef DEBUG_LISTBOX
	printf("End of ListBoxDirectory !\n");
#endif
	return wRet;
}


int ListBoxGetItemRect(HWND hwnd, WORD wIndex, LPRECT lprect)
{
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls, lpls2;
    UINT	i;
    lphl = ListBoxGetStorageHeader(hwnd);
    if (lphl == NULL) return LB_ERR;
    if (wIndex > lphl->ItemsCount) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    for(i = 0; i < lphl->ItemsCount; i++) {
	lpls2 = lpls;
	lpls = (LPLISTSTRUCT)lpls->lpNext;
	if (i == wIndex) {
	    *(lprect) = lpls2->dis.rcItem;
	    break;
	    }  
	if (lpls == NULL)  break;
    }
    return LB_ERR;
}



int ListBoxSetItemHeight(HWND hwnd, WORD wIndex, long height)
{
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls, lpls2;
    UINT	i;
    lphl = ListBoxGetStorageHeader(hwnd);
    if (lphl == NULL) return LB_ERR;
    if (wIndex > lphl->ItemsCount) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    for(i = 0; i < lphl->ItemsCount; i++) {
	lpls2 = lpls;
	lpls = (LPLISTSTRUCT)lpls->lpNext;
	if (i == wIndex) {
	    lpls2->dis.rcItem.bottom = lpls2->dis.rcItem.top + (short)height;
	    InvalidateRect(hwnd, NULL, TRUE);
	    UpdateWindow(hwnd);
	    break;
	    }  
	if (lpls == NULL)  break;
    }
    return LB_ERR;
}





int ListBoxDefaultItem(HWND hwnd, WND *wndPtr, 
	LPHEADLIST lphl, LPLISTSTRUCT lpls)
{
    RECT	rect;
    GetClientRect(hwnd, &rect);
    SetRect(&lpls->dis.rcItem, 0, 0, rect.right, lphl->StdItemHeight);
    lpls->dis.CtlType = lphl->DrawCtlType;
    lpls->dis.CtlID = wndPtr->wIDmenu;
    lpls->dis.itemID = 0;
    lpls->dis.itemAction = 0;
    lpls->dis.itemState = 0;
    lpls->dis.hwndItem = hwnd;
    lpls->dis.hDC = 0;
    lpls->dis.itemData = 0;
}



int ListBoxFindNextMatch(HWND hwnd, WORD wChar)
{
    WND  	*wndPtr;
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls;
    UINT	Count;
    lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
    if (lphl == NULL) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    if (wChar < ' ') return LB_ERR;
    if (((wndPtr->dwStyle & LBS_OWNERDRAWFIXED) == LBS_OWNERDRAWFIXED) ||
	    ((wndPtr->dwStyle & LBS_OWNERDRAWVARIABLE) == LBS_OWNERDRAWVARIABLE)) {
	if ((wndPtr->dwStyle & LBS_HASSTRINGS) != LBS_HASSTRINGS) { 
	    return LB_ERR;
	    }
	}
    Count = 0;
    while(lpls != NULL) {
        if (Count > lphl->ItemFocused) {
	    if (*((char *)lpls->dis.itemData) == (char)wChar) {
		lphl->FirstVisible = Count - lphl->ItemsVisible / 2;
		if (lphl->FirstVisible < 1) lphl->FirstVisible = 1;
		if ((wndPtr->dwStyle & LBS_MULTIPLESEL) == LBS_MULTIPLESEL) {
		    lphl->ItemFocused = Count;
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
	if (*((char *)lpls->dis.itemData) == (char)wChar) {
	    if (Count == lphl->ItemFocused)    return LB_ERR;
	    lphl->FirstVisible = Count - lphl->ItemsVisible / 2;
	    if (lphl->FirstVisible < 1) lphl->FirstVisible = 1;
	    if ((wndPtr->dwStyle & LBS_MULTIPLESEL) == LBS_MULTIPLESEL) {
		lphl->ItemFocused = Count;
		}
	    else {
		ListBoxSetCurSel(hwnd, Count);
		}
	    SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
	    InvalidateRect(hwnd, NULL, TRUE);
	    UpdateWindow(hwnd);
	    return Count;
	    }
	lpls = (LPLISTSTRUCT)lpls->lpNext;
	Count++;
        }
    return LB_ERR;
}


