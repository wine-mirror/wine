/*
 * Interface code to listbox widgets
 *
 * Copyright  Martin Ayotte, 1993
 *
static char Copyright[] = "Copyright Martin Ayotte, 1993";
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "windows.h"
#include "user.h"
#include "heap.h"
#include "win.h"
#include "msdos.h"
#include "listbox.h"
#include "dos_fs.h"
#include "stddebug.h"
#include "debug.h"

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

#define HasStrings(wndPtr) ( \
  ( ((wndPtr->dwStyle & LBS_OWNERDRAWFIXED) != LBS_OWNERDRAWFIXED) && \
    ((wndPtr->dwStyle & LBS_OWNERDRAWVARIABLE) != LBS_OWNERDRAWVARIABLE) ) || \
  ((wndPtr->dwStyle & LBS_HASSTRINGS) == LBS_HASSTRINGS) )

#define LIST_HEAP_ALLOC(lphl,f,size) ((int)HEAP_Alloc(&lphl->Heap,f,size) & 0xffff)
#define LIST_HEAP_FREE(lphl,handle) (HEAP_Free(&lphl->Heap,LIST_HEAP_ADDR(lphl,handle)))
#define LIST_HEAP_ADDR(lphl,handle) \
    ((void *)((handle) ? ((handle) | ((int)lphl->Heap & 0xffff0000)) : 0))
#define LIST_HEAP_SIZE 0x10000

/***********************************************************************
 *           ListBoxWndProc 
 */
LONG ListBoxWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{    
	WND  *wndPtr;
	LPHEADLIST  lphl;
	HWND	hWndCtl;
	WORD	wRet;
	LONG	lRet;
	RECT	rect;
	int		y;
	CREATESTRUCT *createStruct;
	static RECT rectsel;
    switch(message) {
	case WM_CREATE:
		CreateListBoxStruct(hwnd);
		lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
		dprintf_listbox(stddeb,"ListBox WM_CREATE %p !\n", lphl);
		if (lphl == NULL) return 0;
		createStruct = (CREATESTRUCT *)lParam;
		if (HIWORD(createStruct->lpCreateParams) != 0)
			lphl->hWndLogicParent = (HWND)HIWORD(createStruct->lpCreateParams);
		else
			lphl->hWndLogicParent = GetParent(hwnd);
		lphl->hFont = GetStockObject(SYSTEM_FONT);
		lphl->ColumnsWidth = wndPtr->rectClient.right - wndPtr->rectClient.left;
                SetScrollRange(hwnd, SB_VERT, 1, ListMaxFirstVisible(lphl), TRUE);
                SetScrollRange(hwnd, SB_HORZ, 1, 1, TRUE);
		if ((wndPtr->dwStyle & LBS_OWNERDRAWFIXED) == LBS_OWNERDRAWFIXED) {
			}
		return 0;
	case WM_DESTROY:
		lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
		if (lphl == NULL) return 0;
		ListBoxResetContent(hwnd);
		/* XXX need to free lphl->Heap */
		free(lphl);
		*((LPHEADLIST *)&wndPtr->wExtra[1]) = 0;
		dprintf_listbox(stddeb,"ListBox WM_DESTROY %p !\n", lphl);
		return 0;

	case WM_VSCROLL:
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
	
	case WM_HSCROLL:
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
		if (lphl->FirstVisible < 1)    lphl->FirstVisible = 1;
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
	
	case WM_LBUTTONDOWN:
		SetFocus(hwnd);
		SetCapture(hwnd);
		lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
		if (lphl == NULL) return 0;
		lphl->PrevFocused = lphl->ItemFocused;
	        y = ListBoxFindMouse(hwnd, LOWORD(lParam), HIWORD(lParam));
		if (y==-1)
		    return 0;
		if ((wndPtr->dwStyle & LBS_MULTIPLESEL) == LBS_MULTIPLESEL) {
		    lphl->ItemFocused = y;
		    wRet = ListBoxGetSel(hwnd, y);
		    ListBoxSetSel(hwnd, y, !wRet);
		    }
		else {
		    ListBoxSetCurSel(hwnd, y);
		    if ((wndPtr->dwStyle && LBS_NOTIFY) != 0)
				SendMessage(lphl->hWndLogicParent, WM_COMMAND, 
	    	    wndPtr->wIDmenu, MAKELONG(hwnd, LBN_SELCHANGE));
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
		return 0;
	case WM_MOUSEMOVE:
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
					break;
					}
				}
			GetClientRect(hwnd, &rect);
			if (y > (rect.bottom - 4)) {
				if (lphl->FirstVisible < ListMaxFirstVisible(lphl)) {
					lphl->FirstVisible++;
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
					    if ((wndPtr->dwStyle && LBS_NOTIFY) != 0)
							SendMessage(lphl->hWndLogicParent, WM_COMMAND, 
				    	    wndPtr->wIDmenu, MAKELONG(hwnd, LBN_SELCHANGE));
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
		lphl->FirstVisible = lphl->ItemFocused / lphl->ItemsVisible * 
											lphl->ItemsVisible + 1;
		if (lphl->FirstVisible < 1) lphl->FirstVisible = 1;
		if ((wndPtr->dwStyle & LBS_MULTIPLESEL) != LBS_MULTIPLESEL) {
			ListBoxSetCurSel(hwnd, lphl->ItemFocused);
		    if ((wndPtr->dwStyle && LBS_NOTIFY) != 0)
				SendMessage(lphl->hWndLogicParent, WM_COMMAND, 
	    	    wndPtr->wIDmenu, MAKELONG(hwnd, LBN_SELCHANGE));
			}
                SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		break;
	case WM_SETFONT:
		lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
		if (lphl == NULL) return 0;
		if (wParam == 0)
			lphl->hFont = GetStockObject(SYSTEM_FONT);
		else
			lphl->hFont = wParam;
		if (wParam == 0) break;
		break;
	case WM_SETREDRAW:
		dprintf_listbox(stddeb,"ListBox WM_SETREDRAW hWnd=%04X w=%04X !\n", hwnd, wParam);
		lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
		if (lphl == NULL) return 0;
		lphl->bRedrawFlag = wParam;
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
		dprintf_listbox(stddeb,"ListBox WM_SETFOCUS !\n");
		lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
		break;
	case WM_KILLFOCUS:
		dprintf_listbox(stddeb,"ListBox WM_KILLFOCUS !\n");
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		break;

    case LB_RESETCONTENT:
		dprintf_listbox(stddeb,"ListBox LB_RESETCONTENT !\n");
		ListBoxResetContent(hwnd);
		return 0;
    case LB_DIR:
		dprintf_listbox(stddeb,"ListBox LB_DIR !\n");
		wRet = ListBoxDirectory(hwnd, wParam, (LPSTR)lParam);
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		return wRet;
	case LB_ADDSTRING:
		wRet = ListBoxAddString(hwnd, (LPSTR)lParam);
		return wRet;
	case LB_GETTEXT:
		dprintf_listbox(stddeb, "LB_GETTEXT  wParam=%d\n",wParam);
		wRet = ListBoxGetText(hwnd, wParam, (LPSTR)lParam, FALSE);
                return wRet;
	case LB_INSERTSTRING:
		wRet = ListBoxInsertString(hwnd, wParam, (LPSTR)lParam);
		return wRet;
	case LB_DELETESTRING:
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
		dprintf_listbox(stddeb,"ListBox LB_GETCURSEL %u !\n", 
				lphl->ItemFocused);
		return lphl->ItemFocused;
	case LB_GETHORIZONTALEXTENT:
		return wRet;
	case LB_GETITEMDATA:
		dprintf_listbox(stddeb, "LB_GETITEMDATA wParam=%x\n", wParam);
		lRet = ListBoxGetText(hwnd, wParam, (LPSTR)lParam, TRUE);
		return lRet;
	case LB_GETITEMHEIGHT:
                ListBoxGetItemRect(hwnd, wParam, &rect);
                return (rect.bottom - rect.top);
	case LB_GETITEMRECT:
                ListBoxGetItemRect(hwnd, wParam, (LPRECT)lParam);
                return 0;
	case LB_GETSEL:
		wRet = ListBoxGetSel(hwnd, wParam);
		return wRet;
	case LB_GETSELCOUNT:
		lphl = ListBoxGetStorageHeader(hwnd);
		if (lphl == NULL) return LB_ERR;
		return lphl->SelCount;
	case LB_GETSELITEMS:
		return wRet;
	case LB_GETTEXTLEN:
		return wRet;
	case LB_GETTOPINDEX:
		lphl = ListBoxGetStorageHeader(hwnd);
		if (lphl == NULL) return LB_ERR;
		return lphl->FirstVisible;
	case LB_SELECTSTRING:
		return wRet;
	case LB_SELITEMRANGE:
		return wRet;
	case LB_SETCARETINDEX:
		return wRet;
	case LB_SETCOLUMNWIDTH:
		lphl = ListBoxGetStorageHeader(hwnd);
		if (lphl == NULL) return LB_ERR;
		lphl->ColumnsWidth = wParam;
		break;
	case LB_SETHORIZONTALEXTENT:
		return wRet;
	case LB_SETITEMDATA:
		dprintf_listbox(stddeb, "LB_SETITEMDATA  wParam=%x  lParam=%lx\n", wParam, lParam);
		wRet = ListBoxSetItemData(hwnd, wParam, lParam);
		return wRet;
	case LB_SETTABSTOPS:
		lphl = ListBoxGetStorageHeader(hwnd);
		if (lphl == NULL) return LB_ERR;
		lphl->FirstVisible = wParam;
		return 0;
	case LB_SETCURSEL:
		dprintf_listbox(stddeb,"ListBox LB_SETCURSEL wParam=%x !\n", 
				wParam);
		wRet = ListBoxSetCurSel(hwnd, wParam);
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		return wRet;
	case LB_SETSEL:
		dprintf_listbox(stddeb,"ListBox LB_SETSEL wParam=%x lParam=%lX !\n", wParam, lParam);
		wRet = ListBoxSetSel(hwnd, LOWORD(lParam), wParam);
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		return wRet;
	case LB_SETTOPINDEX:
		dprintf_listbox(stddeb,"ListBox LB_SETTOPINDEX wParam=%x !\n",
				wParam);
		lphl = ListBoxGetStorageHeader(hwnd);
		lphl->FirstVisible = wParam;
                SetScrollPos(hwnd, SB_VERT, lphl->FirstVisible, TRUE);
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		break;
	case LB_SETITEMHEIGHT:
		dprintf_listbox(stddeb,"ListBox LB_SETITEMHEIGHT wParam=%x lParam=%lX !\n", wParam, lParam);
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
	HDC 	hdc;
	RECT 	rect;
	int     i, h, h2, maxwidth, ipc;
	h = 0;
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
		TextOut(hdc, rect.left + 5, h + 2, (char *)lpls->itemText, 
			strlen((char *)lpls->itemText));
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
}



void OwnerDrawListBox(HWND hwnd)
{
	WND 	*wndPtr,*ParentWndPtr;
	LPHEADLIST  lphl;
	LPLISTSTRUCT lpls;
	PAINTSTRUCT ps;
	HBRUSH 	hBrush;
	DWORD   itemData;
	HDC 	hdc;
	RECT 	rect;
	int     i, h, h2, maxwidth;
	h = 0;
	hdc = BeginPaint(hwnd, &ps);
	if (!IsWindowVisible(hwnd)) {
		EndPaint( hwnd, &ps );
		return;
		}
	lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
	if (lphl == NULL) goto EndOfPaint;
	if (!lphl->bRedrawFlag) goto EndOfPaint;
	hBrush = SendMessage(lphl->hWndLogicParent, WM_CTLCOLOR, (WORD)hdc,
		    MAKELONG(hwnd, CTLCOLOR_LISTBOX));
	if (hBrush == (HBRUSH)NULL)  hBrush = GetStockObject(WHITE_BRUSH);
	GetClientRect(hwnd, &rect);
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
		lpls->dis.hwndItem = hwnd;
		lpls->dis.CtlType = ODT_LISTBOX;
		lpls->dis.itemID = i - 1;
		if ((!lpls->dis.CtlID) && (lphl->hWndLogicParent)) {
			ParentWndPtr = WIN_FindWndPtr(lphl->hWndLogicParent);
			lpls->dis.CtlID = ParentWndPtr->wIDmenu;
			}
		h2 = lpls->dis.rcItem.bottom - lpls->dis.rcItem.top;
		lpls->dis.rcItem.top = h;
		lpls->dis.rcItem.bottom = h + h2;
		lpls->dis.rcItem.left = rect.left;
		lpls->dis.rcItem.right = rect.right;
		lpls->dis.itemAction = ODA_DRAWENTIRE;
/*		if (lpls->dis.itemState != 0) {
		    lpls->dis.itemAction |= ODA_SELECT;
		    }
		if (lphl->ItemFocused == i - 1) {
		    lpls->dis.itemAction |= ODA_FOCUS;
		    }*/
		dprintf_listbox(stddeb,"LBOX WM_DRAWITEM #%d left=%d top=%d right=%d bottom=%d !\n", 
			i-1, lpls->dis.rcItem.left, lpls->dis.rcItem.top, 
			lpls->dis.rcItem.right, lpls->dis.rcItem.bottom);
		dprintf_listbox(stddeb,"LBOX WM_DRAWITEM &dis=%lX CtlID=%u !\n", 
			(LONG)&lpls->dis, lpls->dis.CtlID);
		dprintf_listbox(stddeb,"LBOX WM_DRAWITEM %08lX!\n",lpls->dis.itemData);
		if (HasStrings(wndPtr))
		  dprintf_listbox(stddeb,"  '%s'\n",lpls->itemText);
		if (HasStrings(wndPtr)) {
			itemData = lpls->dis.itemData;
			lpls->dis.itemData = (DWORD)lpls->itemText;
			}
		SendMessage(lphl->hWndLogicParent, WM_DRAWITEM, 
							i-1, (LPARAM)&lpls->dis);
		if (HasStrings(wndPtr))
			lpls->dis.itemData = itemData;

/*		if (lpls->dis.itemState != 0) {
  		    InvertRect(hdc, &lpls->dis.rcItem);  
		    }  */
		h += h2;
		lphl->ItemsVisible++;
		/* if (h > rect.bottom) goto EndOfPaint;*/
		}
	    if (lpls->lpNext == NULL) goto EndOfPaint;
	    lpls = (LPLISTSTRUCT)lpls->lpNext;
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



int CreateListBoxStruct(HWND hwnd)
{
	WND  *wndPtr;
	LPHEADLIST lphl;
	int HeapHandle;
	void *HeapBase;
	wndPtr = WIN_FindWndPtr(hwnd);
	lphl = (LPHEADLIST)malloc(sizeof(HEADLIST));
	lphl->lpFirst = NULL;
	*((LPHEADLIST *)&wndPtr->wExtra[1]) = lphl;     /* HEAD of List */
	lphl->ItemsCount = 0;
	lphl->ItemsVisible = 0;
	lphl->FirstVisible = 1;
	lphl->ColumnsVisible = 1;
	lphl->ItemsPerColumn = 0;
	lphl->StdItemHeight = 15;
	lphl->ItemFocused = 0;
	lphl->PrevFocused = 0;
	lphl->SelCount = 0;
	lphl->DrawCtlType = ODT_LISTBOX;
	lphl->bRedrawFlag = TRUE;
	HeapHandle = GlobalAlloc(GMEM_FIXED, LIST_HEAP_SIZE);
	HeapBase = GlobalLock(HeapHandle);
	HEAP_Init(&lphl->Heap, HeapBase, LIST_HEAP_SIZE);
	return TRUE;
}


void ListBoxAskMeasure(WND *wndPtr, LPHEADLIST lphl, LPLISTSTRUCT lpls)  
{
	MEASUREITEMSTRUCT 	*lpmeasure;
	HANDLE hTemp = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(MEASUREITEMSTRUCT));
	lpmeasure = (MEASUREITEMSTRUCT *) USER_HEAP_ADDR(hTemp);
	if (lpmeasure == NULL) {
		fprintf(stderr,"ListBoxAskMeasure() // Bad allocation of Measure struct !\n");
		return;
		}
	lpmeasure->CtlType = ODT_LISTBOX;
	lpmeasure->CtlID = wndPtr->wIDmenu;
	lpmeasure->itemID = lpls->dis.itemID;
	lpmeasure->itemWidth = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
	lpmeasure->itemHeight = 0;
	if (HasStrings(wndPtr))
		lpmeasure->itemData = (DWORD)lpls->itemText;
	else
		lpmeasure->itemData = lpls->dis.itemData;
	SendMessage(lphl->hWndLogicParent, WM_MEASUREITEM, 0, (DWORD)lpmeasure);
	lpls->dis.rcItem.right = lpls->dis.rcItem.left + lpmeasure->itemWidth;
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
    WND  	*wndPtr;
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
		}
	else {
		lplsnew->itemText = NULL;
		lplsnew->dis.itemData = (DWORD)newstr;
		}
	lplsnew->dis.itemID = lphl->ItemsCount;
	lplsnew->hData = hStr;
	if (((wndPtr->dwStyle & LBS_OWNERDRAWVARIABLE) == LBS_OWNERDRAWVARIABLE) ||
		((wndPtr->dwStyle & LBS_OWNERDRAWFIXED) == LBS_OWNERDRAWFIXED))
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
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls;
    UINT	Count;
    if ((!OutStr)&&(!bItemData))
	fprintf(stderr, "ListBoxGetText // OutStr==NULL\n"); 
    if (!bItemData)   	*OutStr=0;
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
    if (bItemData)
	return lpls->dis.itemData;
    if (!(HasStrings(wndPtr)) )
      {
	*((long *)OutStr) = lpls->dis.itemData;
	return 4;
      }
	
    strcpy(OutStr, lpls->itemText);
    return strlen(OutStr);
}

int ListBoxSetItemData(HWND hwnd, UINT uIndex, DWORD ItemData)
{
    WND         *wndPtr;
    LPHEADLIST  lphl;
    LPLISTSTRUCT lpls;
    UINT        Count;
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
    if (uIndex > lphl->ItemsCount) return LB_ERR;
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
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls;
    UINT	Count;
    lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
    if (lphl == NULL) return LB_ERR;
    if (nFirst > lphl->ItemsCount) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    Count = 0;
    while(lpls != NULL) {
      if (HasStrings(wndPtr))
	{
	  if (strcmp(lpls->itemText, MatchStr) == 0) return Count;
	}
      else
	{
	  if (lpls->dis.itemData == (DWORD)MatchStr) return Count;
	}
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
    dprintf_listbox(stddeb, "ListBoxResetContent // ItemCount = %d\n",
	lphl->ItemsCount);
    for(i = 0; i <= lphl->ItemsCount; i++) {
	lpls2 = lpls;
	lpls = (LPLISTSTRUCT)lpls->lpNext;
	if (i != 0) {
	    dprintf_listbox(stddeb,"ResetContent #%u\n", i);
	    if (lpls2->hData != 0 && lpls2->hData != lpls2->hMem)
		LIST_HEAP_FREE(lphl, lpls2->hData);
	    if (lpls2->hMem != 0) LIST_HEAP_FREE(lphl, lpls2->hMem);
	    }  
	if (lpls == NULL)  break;
    }
    lphl->lpFirst = NULL;
    lphl->FirstVisible = 1;
    lphl->ItemsCount = 0;
    lphl->ItemFocused = -1;
    lphl->PrevFocused = -1;
    if ((wndPtr->dwStyle && LBS_NOTIFY) != 0)
	SendMessage(lphl->hWndLogicParent, WM_COMMAND, 
    	    wndPtr->wIDmenu, MAKELONG(hwnd, LBN_SELCHANGE));
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
	SendMessage(lphl->hWndLogicParent, WM_COMMAND, 
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
	int	x, wRet = LB_OKAY;
	BOOL    OldFlag;
	char 	temp[256];
    LPHEADLIST 	lphl;
	dprintf_listbox(stddeb,"ListBoxDirectory: %s, %4x\n",filespec,attrib);
    lphl = ListBoxGetStorageHeader(hwnd);
    if (lphl == NULL) return LB_ERR;
	if ((dp = (struct dosdirent *)DOS_opendir(filespec)) ==NULL) return 0;
	OldFlag = lphl->bRedrawFlag;
	lphl->bRedrawFlag = FALSE;
	while ((dp = (struct dosdirent *)DOS_readdir(dp))) {
		if (!dp->inuse) break;
		dprintf_listbox(stddeb,"ListBoxDirectory %p '%s' !\n", dp->filename, dp->filename);
		if (dp->attribute & FA_DIREC) {
			if (attrib & DDL_DIRECTORY &&
					strcmp(dp->filename, ".")) {
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
	DOS_closedir(dp);

	if (attrib & DDL_DRIVES) {
		for (x=0;x!=MAX_DOS_DRIVES;x++) {
			if (DOS_ValidDrive(x)) {
				sprintf(temp, "[-%c-]", 'a'+x);
				if ( (wRet = ListBoxAddString(hwnd, temp)) == LB_ERR) break;
				}		
			}
		}
	lphl->bRedrawFlag = OldFlag;
	if (OldFlag) {
		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
		}
	dprintf_listbox(stddeb,"End of ListBoxDirectory !\n");
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
	if (wndPtr == NULL || lphl == NULL || lpls == NULL) {
		fprintf(stderr,"ListBoxDefaultItem() // Bad Pointers !\n");
		return FALSE;
		}
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
	return TRUE;
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
	    if (*(lpls->itemText) == (char)wChar) {
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
	if (*(lpls->itemText) == (char)wChar) {
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


/************************************************************************
 * 					DlgDirSelect			[USER.99]
 */
BOOL DlgDirSelect(HWND hDlg, LPSTR lpStr, int nIDLBox)
{
	fprintf(stdnimp,"DlgDirSelect(%04X, '%s', %d) \n", hDlg, lpStr, nIDLBox);
	return FALSE;
}


/************************************************************************
 * 					DlgDirList				[USER.100]
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
	    drive = DOS_GetDefaultDrive();
	    SendDlgItemMessage(hDlg, nIDStat, WM_SETTEXT, 0, 
			       (LONG) DOS_GetCurrentDir(drive) );
	  } 
	return ret;
}

/* get the maximum value of lphl->FirstVisible */
int ListMaxFirstVisible(LPHEADLIST lphl)
{
    int m = lphl->ItemsCount-lphl->ItemsVisible+1;
    return (m < 1) ? 1 : m;
}
