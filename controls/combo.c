/*
 * Interface code to COMBOBOX widget
 *
 * Copyright  Martin Ayotte, 1993
 *
 */

#define DEBUG_COMBO
/*
#define DEBUG_COMBO
*/

static char Copyright[] = "Copyright Martin Ayotte, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "windows.h"
#include "combo.h"
#include "heap.h"
#include "win.h"
#include "prototypes.h"

HBITMAP hComboBit = 0;

LPHEADCOMBO ComboGetStorageHeader(HWND hwnd);
int CreateComboStruct(HWND hwnd);


/***********************************************************************
 *           ComboWndProc
 */
LONG ComboBoxWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{    
    WORD	wRet;
    RECT	rect;
    int		y, count;
    int		width, height;
    int		AltState;
    WND  	*wndPtr;
    LPHEADCOMBO lphc;
    HDC		hDC, hMemDC;
    BITMAP	bm;
    char	str[128];
    PAINTSTRUCT paintstruct;
    static RECT rectsel;
    switch(message)
    {
    case WM_CREATE:
		wndPtr = WIN_FindWndPtr(hwnd);
		if (wndPtr == NULL) return 0;
#ifdef DEBUG_COMBO
		printf("Combo WM_CREATE %lX !\n", lphc);
#endif
		if (hComboBit == (HBITMAP)NULL) 
		hComboBit = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_COMBO));
		GetObject(hComboBit, sizeof(BITMAP), (LPSTR)&bm);
		wndPtr->dwStyle &= 0xFFFFFFFFL ^ (WS_VSCROLL | WS_HSCROLL);
		GetWindowRect(hwnd, &rect);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
		SetWindowPos(hwnd, 0, 0, 0, width + bm.bmHeight, bm.bmHeight, 
										SWP_NOMOVE | SWP_NOZORDER); 
		CreateComboStruct(hwnd);
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		if (wndPtr->dwStyle & CBS_SIMPLE)
/*			lphc->hWndEdit = CreateWindow("EDIT", "", */
			lphc->hWndEdit = CreateWindow("STATIC", "", 
				WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | SS_LEFT,
				0, 0, width - bm.bmHeight, bm.bmHeight, 
				hwnd, 1, wndPtr->hInstance, 0L);
		else
			lphc->hWndEdit = CreateWindow("STATIC", "", 
				WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | SS_LEFT,
				0, 0, width - bm.bmHeight, bm.bmHeight, 
				hwnd, 1, wndPtr->hInstance, 0L);
		lphc->hWndLBox = CreateWindow("LISTBOX", "", 
			WS_CHILD | WS_CLIPCHILDREN | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
			wndPtr->rectClient.left, wndPtr->rectClient.top + bm.bmHeight, 
			width, height, wndPtr->hwndParent, 1, 
			wndPtr->hInstance, (LPSTR)MAKELONG(0, hwnd));
		ShowWindow(lphc->hWndLBox, SW_HIDE);
#ifdef DEBUG_COMBO
		printf("Combo Creation LBox=%X!\n", lphc->hWndLBox);
#endif
		return 0;
    case WM_DESTROY:
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == 0) return 0;
/*
		DestroyWindow(lphc->hWndEdit);
*/
		DestroyWindow(lphc->hWndLBox);
		free(lphc);
/*
		*((LPHEADCOMBO *)&wndPtr->wExtra[1]) = 0; 
		printf("Combo WM_DESTROY after clearing wExtra !\n");
*/
#ifdef DEBUG_COMBO
		printf("Combo WM_DESTROY %lX !\n", lphc);
#endif
		return DefWindowProc( hwnd, message, wParam, lParam );
	case WM_SHOWWINDOW:
#ifdef DEBUG_COMBO
		printf("ComboBox WM_SHOWWINDOW hWnd=%04X !\n", hwnd);
#endif
		if (!(wParam == 0 && lParam == 0L)) {
			InvalidateRect(hwnd, NULL, TRUE);
			}
	    break;
	
    case WM_COMMAND:
		wndPtr = WIN_FindWndPtr(hwnd);
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		if (LOWORD(lParam) == lphc->hWndLBox) {
            switch(HIWORD(lParam)) {
	        	case LBN_SELCHANGE:
				    lphc->dwState = lphc->dwState & (CB_SHOWDROPDOWN ^ 0xFFFFFFFFL);
				    ShowWindow(lphc->hWndLBox, SW_HIDE);
				    y = SendMessage(lphc->hWndLBox, LB_GETCURSEL, 0, 0L);
				    if (y != LB_ERR) {
				    	SendMessage(lphc->hWndLBox, LB_GETTEXT, (WORD)y, (LPARAM)str);
				    	SendMessage(lphc->hWndEdit, WM_SETTEXT, (WORD)y, (LPARAM)str); 
				    	}
				    SendMessage(GetParent(hwnd), WM_COMMAND, wndPtr->wIDmenu,
					        	MAKELONG(hwnd, CBN_SELCHANGE));
	        	    break;
	        	case LBN_DBLCLK:
				    SendMessage(GetParent(hwnd), WM_COMMAND, wndPtr->wIDmenu,
					        	MAKELONG(hwnd, CBN_DBLCLK));
	        	    break;
	        	}
            }
		break;
    case WM_LBUTTONDOWN:
		printf("Combo WM_LBUTTONDOWN wParam=%x lParam=%lX !\n", wParam, lParam);
		GetClientRect(hwnd, &rect);
		rect.left = rect.right - (rect.bottom - rect.top); 
		hDC = GetDC(hwnd);
		InflateRect(&rect, -1, -1);
		DrawReliefRect(hDC, rect, 1, 1);
		ReleaseDC(hwnd, hDC);
		wndPtr = WIN_FindWndPtr(hwnd);
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		lphc->dwState = lphc->dwState ^ CB_SHOWDROPDOWN;
		if ((lphc->dwState & CB_SHOWDROPDOWN) == CB_SHOWDROPDOWN) {
		    ShowWindow(lphc->hWndLBox, SW_SHOW);
		    SetFocus(lphc->hWndLBox);
			}
		else {
			printf("before Combo Restore Focus !\n");
			SetFocus(lphc->hWndEdit);
			printf("before Combo List Hide !\n");
			ShowWindow(lphc->hWndLBox, SW_HIDE);
			printf("before Combo List GetCurSel !\n");
			y = SendMessage(lphc->hWndLBox, LB_GETCURSEL, 0, 0L);
			if (y != LB_ERR) {
				printf("before Combo List GetText !\n");
				SendMessage(lphc->hWndLBox, LB_GETTEXT, (WORD)y, (LPARAM)str);
				SendMessage(lphc->hWndEdit, WM_SETTEXT, (WORD)y, (LPARAM)str);
				}
			printf("End of Combo List Hide !\n");
			}
		break;
    case WM_LBUTTONUP:
		printf("Combo WM_LBUTTONUP wParam=%x lParam=%lX !\n", wParam, lParam);
		GetClientRect(hwnd, &rect);
		rect.left = rect.right - (rect.bottom - rect.top); 
		hDC = GetDC(hwnd);
		InflateRect(&rect, -1, -1);
		DrawReliefRect(hDC, rect, 1, 0);
		ReleaseDC(hwnd, hDC);
		break;
   case WM_KEYDOWN:
		wndPtr = WIN_FindWndPtr(hwnd);
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		y = SendMessage(lphc->hWndLBox, LB_GETCURSEL, 0, 0L);
		count = SendMessage(lphc->hWndLBox, LB_GETCOUNT, 0, 0L);
		printf("COMBOBOX // GetKeyState(VK_MENU)=%d\n", GetKeyState(VK_MENU));
		if (GetKeyState(VK_MENU) < 0) {
			lphc->dwState = lphc->dwState ^ CB_SHOWDROPDOWN;
			if ((lphc->dwState & CB_SHOWDROPDOWN) == CB_SHOWDROPDOWN) {
				ShowWindow(lphc->hWndLBox, SW_SHOW);
			    SetFocus(lphc->hWndLBox);
				}
			else {
				ShowWindow(lphc->hWndLBox, SW_HIDE);
				y = SendMessage(lphc->hWndLBox, LB_GETCURSEL, 0, 0L);
				if (y != LB_ERR) {
					SendMessage(lphc->hWndLBox, LB_GETTEXT, (WORD)y, (LPARAM)str);
					SendMessage(lphc->hWndEdit, WM_SETTEXT, (WORD)y, (LPARAM)str);
					}
				}
		    }
		else {
		    switch(wParam) {
				case VK_HOME:
				    y = 0;
					break;
				case VK_END:
				    y = count - 1;
				    break;
				case VK_UP:
				    y--;
				    break;
				case VK_DOWN:
				    y++;
				    break;
				}
			if (y < 0) y = 0;
			if (y >= count) y = count - 1;
			SendMessage(lphc->hWndLBox, LB_SETCURSEL, y, 0L);
			SendMessage(lphc->hWndLBox, LB_GETTEXT, (WORD)y, (LPARAM)str);
			SendMessage(lphc->hWndEdit, WM_SETTEXT, (WORD)y, (LPARAM)str);
			SendMessage(GetParent(hwnd), WM_COMMAND, wndPtr->wIDmenu,
			MAKELONG(hwnd, CBN_SELCHANGE));
			}
		break;
    case WM_MEASUREITEM:
		printf("ComboBoxWndProc WM_MEASUREITEM !\n");
    	return(SendMessage(GetParent(hwnd), WM_MEASUREITEM, wParam, lParam));
    case WM_CTLCOLOR:
    	return(SendMessage(GetParent(hwnd), WM_CTLCOLOR, wParam, lParam));
    case WM_PAINT:
		GetClientRect(hwnd, &rect);
		hDC = BeginPaint(hwnd, &paintstruct);
		hMemDC = CreateCompatibleDC(hDC);
		if (hMemDC != 0 && hComboBit != 0) {
			GetObject(hComboBit, sizeof(BITMAP), (LPSTR)&bm);
			SelectObject(hMemDC, hComboBit);
			BitBlt(hDC, rect.right - bm.bmWidth, 0, 
			bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
			}
		DeleteDC(hMemDC);
		EndPaint(hwnd, &paintstruct);
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		InvalidateRect(lphc->hWndEdit, NULL, TRUE);
		UpdateWindow(lphc->hWndEdit);
		if ((lphc->dwState & CB_SHOWDROPDOWN) == CB_SHOWDROPDOWN) {
			InvalidateRect(lphc->hWndLBox, NULL, TRUE);
			UpdateWindow(lphc->hWndLBox);
			}
		break;
	case WM_SETFOCUS:
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		SetFocus(lphc->hWndEdit);
		break;
	case WM_KILLFOCUS:
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		ShowWindow(lphc->hWndLBox, SW_HIDE);
		y = SendMessage(lphc->hWndLBox, LB_GETCURSEL, 0, 0L);
		if (y != LB_ERR) {
			SendMessage(lphc->hWndLBox, LB_GETTEXT, (WORD)y, (LPARAM)str);
			SendMessage(lphc->hWndEdit, WM_SETTEXT, (WORD)y, (LPARAM)str);
			}
		break;
	case CB_ADDSTRING:
#ifdef DEBUG_COMBO
		printf("CB_ADDSTRING '%s' !\n", (LPSTR)lParam);
#endif
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		return(SendMessage(lphc->hWndLBox, LB_ADDSTRING, wParam, lParam));
    case CB_GETLBTEXT:
#ifdef DEBUG_COMBO
		printf("CB_GETLBTEXT #%u !\n", wParam);
#endif
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		return(SendMessage(lphc->hWndLBox, LB_GETTEXT, wParam, lParam));
    case CB_GETLBTEXTLEN:
		printf("CB_GETLBTEXTLEN !\n");
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		return(SendMessage(lphc->hWndLBox, LB_GETTEXTLEN, wParam, lParam));
    case CB_INSERTSTRING:
		printf("CB_INSERTSTRING '%s' !\n", (LPSTR)lParam);
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		return(SendMessage(lphc->hWndLBox, LB_INSERTSTRING, wParam, lParam));
	case CB_DELETESTRING:
		printf("CB_DELETESTRING #%u !\n", wParam);
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		return(SendMessage(lphc->hWndLBox, LB_DELETESTRING, wParam, 0L));
	case CB_RESETCONTENT:
		printf("CB_RESETCONTENT !\n");
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		return(SendMessage(lphc->hWndLBox, LB_RESETCONTENT, 0, 0L));
    case CB_DIR:
		printf("ComboBox CB_DIR !\n");
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		return(SendMessage(lphc->hWndLBox, LB_DIR, wParam, lParam));
	case CB_FINDSTRING:
		lphc = ComboGetStorageHeader(hwnd);
		return(SendMessage(lphc->hWndLBox, LB_FINDSTRING, wParam, lParam));
	case CB_GETCOUNT:
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		return(SendMessage(lphc->hWndLBox, LB_GETCOUNT, 0, 0L));
	case CB_GETCURSEL:
		printf("ComboBox CB_GETCURSEL !\n");
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		return(SendMessage(lphc->hWndLBox, LB_GETCURSEL, 0, 0L));
    case CB_SETCURSEL:
		printf("ComboBox CB_SETCURSEL wParam=%X !\n", wParam);
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		return(SendMessage(lphc->hWndLBox, LB_SETCURSEL, wParam, 0L));
	case CB_GETEDITSEL:
		printf("ComboBox CB_GETEDITSEL !\n");
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
/*        return(SendMessage(lphc->hWndEdit, EM_GETSEL, 0, 0L)); */
		break;
	case CB_SETEDITSEL:
		printf("ComboBox CB_SETEDITSEL lParam=%lX !\n", lParam);
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
/*        return(SendMessage(lphc->hWndEdit, EM_SETSEL, 0, lParam)); */
		break;
	case CB_SELECTSTRING:
		printf("ComboBox CB_SELECTSTRING !\n");
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		break;
	case CB_SHOWDROPDOWN:
		printf("ComboBox CB_SHOWDROPDOWN !\n");
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		lphc->dwState = lphc->dwState | CB_SHOWDROPDOWN;
		if (wParam != 0) {
			ShowWindow(lphc->hWndLBox, SW_SHOW);
			}
		else {
			lphc->dwState = lphc->dwState ^ CB_SHOWDROPDOWN;
			ShowWindow(lphc->hWndLBox, SW_HIDE);
			SendMessage(GetParent(hwnd), WM_COMMAND, wndPtr->wIDmenu,
									MAKELONG(hwnd, CBN_DROPDOWN));
		}
		break;
    case CB_GETITEMDATA:
		printf("ComboBox CB_GETITEMDATA wParam=%X !\n", wParam);
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		return(SendMessage(lphc->hWndLBox, LB_GETITEMDATA, wParam, 0L));
        break;
    case CB_SETITEMDATA:
		printf("ComboBox CB_SETITEMDATA wParam=%X lParam=%lX !\n", wParam, lParam);
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
		return(SendMessage(lphc->hWndLBox, LB_SETITEMDATA, wParam, lParam));
        break;
    case CB_LIMITTEXT:
		printf("ComboBox CB_LIMITTEXT !\n");
		lphc = ComboGetStorageHeader(hwnd);
		if (lphc == NULL) return 0;
/*        return(SendMessage(lphc->hWndEdit, EM_LIMITTEXT, wParam, 0L)); */
        break;

    default:
		return DefWindowProc( hwnd, message, wParam, lParam );
    }
return 0;
}



LPHEADCOMBO ComboGetStorageHeader(HWND hwnd)
{
    WND  *wndPtr;
    LPHEADCOMBO lphc;
    wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr == 0) {
    	printf("Bad Window handle on ComboBox !\n");
    	return 0;
    	}
    lphc = *((LPHEADCOMBO *)&wndPtr->wExtra[1]);
    return lphc;
}



int CreateComboStruct(HWND hwnd)
{
    WND  *wndPtr;
    LPHEADCOMBO lphc;
    wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr == 0) {
    	printf("Bad Window handle on ComboBox !\n");
    	return 0;
    	}
    lphc = (LPHEADCOMBO)malloc(sizeof(HEADCOMBO));
    *((LPHEADCOMBO *)&wndPtr->wExtra[1]) = lphc;
    lphc->dwState = 0;
    return TRUE;
}



/************************************************************************
 * 					DlgDirSelectComboBox	[USER.194]
 */
BOOL DlgDirSelectComboBox(HWND hDlg, LPSTR lpStr, int nIDLBox)
{
	printf("DlgDirSelectComboBox(%04X, '%s', %d) \n",	hDlg, lpStr, nIDLBox);
}


/************************************************************************
 * 					DlgDirListComboBox		[USER.195]
 */
int DlgDirListComboBox(HWND hDlg, LPSTR lpPathSpec, 
	int nIDLBox, int nIDStat, WORD wType)
{
	HWND		hWnd;
    LPHEADCOMBO lphc;
	printf("DlgDirListComboBox(%04X, '%s', %d, %d, %04X) \n",
			hDlg, lpPathSpec, nIDLBox, nIDStat, wType);
	hWnd = GetDlgItem(hDlg, nIDLBox);
	lphc = ComboGetStorageHeader(hWnd);
	if (lphc == NULL) return 0;
	SendMessage(lphc->hWndLBox, LB_RESETCONTENT, 0, 0L);
	return SendMessage(lphc->hWndLBox, LB_DIR, wType, (DWORD)lpPathSpec);
}



