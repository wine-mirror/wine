static char RCSId[] = "$Id$";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";
static char Copyright2[] = "Copyright  Martin Ayotte, 1993";

/*
#define DEBUG_MENU
#define DEBUG_SYSMENU
*/
#define USE_POPUPMENU

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "windows.h"
#include "sysmetrics.h"
#include "menu.h"
#include "heap.h"
#include "win.h"
#include "bitmaps/check_bitmap"
#include "bitmaps/nocheck_bitmap"

#define SC_ABOUTWINE     SC_SCREENSAVE+1
#define SC_SYSMENU	 SC_SCREENSAVE+2
#define SC_ABOUTWINEDLG	 SC_SCREENSAVE+3

extern HINSTANCE hSysRes;
HMENU	hSysMenu = 0;
HBITMAP hStdCheck = 0;
HBITMAP hStdMnArrow = 0;

static LPMENUBAR firstMenu = NULL;
static MENUITEM *parentItem;
static MENUITEM *siblingItem;
static int       lastLevel;
static int       menuId = 0;
static Pixmap    checkBitmap = XtUnspecifiedPixmap;
static Pixmap    nocheckBitmap = XtUnspecifiedPixmap;

LPPOPUPMENU PopupMenuGetStorageHeader(HWND hwnd);
LPPOPUPMENU PopupMenuGetWindowAndStorage(HWND hwnd, WND **wndPtr);
void StdDrawMenuBar(HWND hwnd);
void StdDrawPopupMenu(HWND hwnd);
LPMENUITEM MenuFindItem(HWND hwnd, int x, int y, WORD *lpRet);
LPMENUITEM MenuFindItemBySelKey(HWND hwnd, WORD key, WORD *lpRet);
void PopupMenuCalcSize(HWND hwnd);
void MenuBarCalcSize(HWND hwnd);
LPMENUITEM GetMenuItemPtr(LPPOPUPMENU menu, WORD nPos);
WORD GetSelectionKey(LPSTR str);
LPSTR GetShortCutString(LPSTR str);
WORD GetShortCutPos(LPSTR str);
BOOL HideAllSubPopupMenu(LPPOPUPMENU menu);
HMENU CopySysMenu();
WORD * ParseMenuResource(WORD *first_item, int level, HMENU hMenu);
void SetMenuLogicalParent(HMENU hMenu, HWND hWnd);

BOOL FAR PASCAL AboutWine_Proc(HWND hDlg, WORD msg, WORD wParam, LONG lParam);

/***********************************************************************
 *           PopupMenuWndProc
 */
LONG PopupMenuWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{    
    CREATESTRUCT *createStruct;
    WORD	wRet;
    short	x, y;
    WND  	*wndPtr;
    LPPOPUPMENU lppop, lppop2;
    LPMENUITEM	lpitem, lpitem2;
    HMENU	hSubMenu;
    RECT	rect;
    HDC		hDC;
    switch(message) 
    {
    case WM_CREATE:
#ifdef DEBUG_MENU
        printf("PopupMenu WM_CREATE lParam=%08X !\n", lParam);
#endif
	createStruct = (CREATESTRUCT *)lParam;
     	lppop = (LPPOPUPMENU)createStruct->lpCreateParams;
     	if (lppop == NULL) break;
	wndPtr = WIN_FindWndPtr(hwnd);
	*((LPPOPUPMENU *)&wndPtr->wExtra[1]) = lppop;
#ifdef DEBUG_MENU
        printf("PopupMenu WM_CREATE lppop=%08X !\n", lppop);
#endif
/*
	if (lppop->BarFlags == 0) {
	    PopupMenuCalcSize(hwnd);
	    printf("PopupMenu WM_CREATE Width=%d Height=%d !\n", 
			lppop->Width, lppop->Height);
	    SetWindowPos(hwnd, 0, 0, 0, lppop->Width + 2, lppop->Height, 
		SWP_NOZORDER | SWP_NOMOVE);
	    }
	else {
	    MenuBarCalcSize(hwnd);
	    SetWindowPos(hwnd, 0, 0, -16, lppop->Width, lppop->Height, 
		SWP_NOZORDER);
	    }
*/
	if (hStdCheck == (HBITMAP)NULL) 
	    hStdCheck = LoadBitmap((HANDLE)NULL, (LPSTR)OBM_CHECK);
	if (hStdMnArrow == (HBITMAP)NULL) 
	    hStdMnArrow = LoadBitmap((HANDLE)NULL, (LPSTR)OBM_MNARROW);
	return 0;
    case WM_DESTROY:
	lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
#ifdef DEBUG_MENU
        printf("PopupMenu WM_DESTROY %lX !\n", lppop);
#endif
	return 0;
    case WM_COMMAND:
	lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
	if (lppop->hWndParent != (HWND)NULL) {
	    SendMessage(lppop->hWndParent, WM_COMMAND, wParam, lParam);
#ifdef DEBUG_MENU
	    printf("PopupMenu // push to lower parent WM_COMMAND !\n");
#endif
	    }
	else {
	    if (lppop->SysFlag == 0) {
		SendMessage(lppop->ownerWnd, WM_COMMAND, wParam, lParam);
#ifdef DEBUG_MENU
		printf("PopupMenu // push to Owner WM_COMMAND !\n");
#endif
		}
	    else {
#ifdef DEBUG_SYSMENU
		printf("PopupMenu // push to Owner WM_SYSCOMMAND !\n");
#endif
		SendMessage(lppop->ownerWnd, WM_SYSCOMMAND, wParam, lParam);
		}
	    }
	if (lppop->BarFlags == 0) ShowWindow(hwnd, SW_HIDE);
    	break;
    case WM_SHOWWINDOW:
	lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
    	if (wParam == 0) {
	    HideAllSubPopupMenu(lppop);
#ifdef DEBUG_MENU
    	printf("PopupMenu WM_SHOWWINDOW -> HIDE!\n");
#endif
	    break;
	    }
	lppop->FocusedItem = (WORD)-1;
	if (lppop->BarFlags == 0) {
	    PopupMenuCalcSize(hwnd);
#ifdef DEBUG_MENU
	    printf("PopupMenu WM_SHOWWINDOW Width=%d Height=%d !\n", 
			lppop->Width, lppop->Height);
#endif
	    SetWindowPos(hwnd, 0, 0, 0, lppop->Width + 2, lppop->Height, 
		SWP_NOZORDER | SWP_NOMOVE);
	    }
	else {
	    MenuBarCalcSize(hwnd);
#ifdef DEBUG_MENU
	    printf("MenuBarMenu WM_SHOWWINDOW Width=%d Height=%d !\n", 
			lppop->Width, lppop->Height);
#endif
	    SetWindowPos(hwnd, 0, 0, -16, lppop->Width, lppop->Height, 
		SWP_NOZORDER);
	    }
    	break;
    case WM_LBUTTONDOWN:
	lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
	SetCapture(hwnd);
	lpitem = MenuFindItem(hwnd, LOWORD(lParam), HIWORD(lParam), &wRet);
#ifdef DEBUG_MENU
	printf("PopupMenu WM_LBUTTONDOWN wRet=%d lpitem=%08X !\n", wRet, lpitem);
#endif
	if (lpitem != NULL) {
	    if (lppop->FocusedItem != (WORD)-1) {
		HideAllSubPopupMenu(lppop);
		lpitem2 = GetMenuItemPtr(lppop, lppop->FocusedItem);
		if (((lpitem2->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
		    ((lpitem2->item_flags & MF_MENUBREAK) != MF_MENUBREAK)) {
	            hDC = GetDC(hwnd);
	            InvertRect(hDC, &lpitem2->rect);
		    ReleaseDC(hwnd, hDC);
		    }
		}
	    lppop->FocusedItem = wRet;
	    if (((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
		((lpitem->item_flags & MF_MENUBREAK) != MF_MENUBREAK)) {
	        hDC = GetDC(hwnd);
	        InvertRect(hDC, &lpitem->rect);
		ReleaseDC(hwnd, hDC);
		}
	    if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) {
		hSubMenu = (HMENU)lpitem->item_id;
		lppop2 = (LPPOPUPMENU) GlobalLock(hSubMenu);
		if (lppop2 == NULL) break;
		lppop2->hWndParent = hwnd;
		GetClientRect(hwnd, &rect);
		if (lppop->BarFlags != 0) {
		    y = rect.bottom - rect.top;
		    GetWindowRect(hwnd, &rect);
		    y += rect.top;
		    TrackPopupMenu(hSubMenu, TPM_LEFTBUTTON, 
			rect.left + lpitem->rect.left, 
			y, 0, lppop->ownerWnd, (LPRECT)NULL);
		    }
		else {
		    x = rect.right;
		    GetWindowRect(hwnd, &rect);
		    x += rect.left;
		    TrackPopupMenu(hSubMenu, TPM_LEFTBUTTON, 
			x, rect.top + lpitem->rect.top,
			0, lppop->ownerWnd, (LPRECT)NULL);
		    }
		break;
		}
	    }
	break;
    case WM_LBUTTONUP:
	lppop = PopupMenuGetStorageHeader(hwnd);
	ReleaseCapture();
	lpitem = MenuFindItem(hwnd, LOWORD(lParam), HIWORD(lParam), &wRet);
#ifdef DEBUG_MENU
	printf("PopupMenu WM_LBUTTONUP wRet=%d lpitem=%08X !\n", wRet, lpitem);
#endif
	if (lpitem != NULL) {
	    if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) {
		break;
		}
	    if (((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
		((lpitem->item_flags & MF_POPUP) != MF_POPUP)) {
	    	ShowWindow(lppop->hWnd, SW_HIDE);
		if (lppop->hWndParent != (HWND)NULL) {
		    SendMessage(lppop->hWndParent, WM_COMMAND, 
					lpitem->item_id, 0L);
#ifdef DEBUG_MENU
		    printf("PopupMenu // WM_COMMAND to ParentMenu wParam=%d !\n", 
			lpitem->item_id);
#endif
		    }
		else {
		    if (lppop->SysFlag == 0) {
#ifdef DEBUG_MENU
			printf("PopupMenu // WM_COMMAND wParam=%d !\n", 
				lpitem->item_id);
#endif
			SendMessage(lppop->ownerWnd, WM_COMMAND, 
					lpitem->item_id, 0L);
			}
		    else {
			if (lpitem->item_id == SC_ABOUTWINE) {
			    printf("SysMenu // Show 'About Wine ...' !\n");
/*			    DialogBox(hSysRes, MAKEINTRESOURCE(SC_ABOUTWINEDLG), */
			    DialogBox(hSysRes, MAKEINTRESOURCE(2), 
				GetParent(hwnd), (FARPROC)AboutWine_Proc);
			    }
			else {
			    SendMessage(lppop->ownerWnd, WM_SYSCOMMAND,
						 lpitem->item_id, 0L);
#ifdef DEBUG_SYSMENU
			    printf("PopupMenu // WM_SYSCOMMAND wParam=%04X !\n", 
					lpitem->item_id);
#endif
			    }
			}
		    }
#ifdef DEBUG_MENU
		printf("PopupMenu // SendMessage WM_COMMAND wParam=%d !\n", 
			lpitem->item_id);
#endif
	    	break;
		}
	    }
	if (lppop->FocusedItem != (WORD)-1) {
	    HideAllSubPopupMenu(lppop);
	    lpitem2 = GetMenuItemPtr(lppop, lppop->FocusedItem);
	    if (((lpitem2->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
		((lpitem2->item_flags & MF_MENUBREAK) != MF_MENUBREAK)) {
	        hDC = GetDC(hwnd);
	        InvertRect(hDC, &lpitem2->rect);
		ReleaseDC(hwnd, hDC);
		}
	    }
	break;
    case WM_MOUSEMOVE:
        if ((wParam & MK_LBUTTON) != 0) {
	    lppop = PopupMenuGetStorageHeader(hwnd);
	    lpitem = MenuFindItem(hwnd, LOWORD(lParam), HIWORD(lParam), &wRet);
	    if ((lpitem != NULL) && (lppop->FocusedItem != wRet)) {
		lpitem2 = GetMenuItemPtr(lppop, lppop->FocusedItem);
		hDC = GetDC(hwnd);
		if (((lpitem2->item_flags & MF_POPUP) == MF_POPUP) ||
		    ((lpitem2->item_flags & MF_STRING) == MF_STRING)) {
		    InvertRect(hDC, &lpitem2->rect);
		    }
		if ((lpitem2->item_flags & MF_POPUP) == MF_POPUP) {
		    HideAllSubPopupMenu(lppop);
		    }
		lppop->FocusedItem = wRet;
		if (((lpitem->item_flags & MF_POPUP) == MF_POPUP) ||
		    ((lpitem->item_flags & MF_STRING) == MF_STRING)) {
		    InvertRect(hDC, &lpitem->rect);
		    }
		if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) {
		    hSubMenu = (HMENU)lpitem->item_id;
		    lppop2 = (LPPOPUPMENU) GlobalLock(hSubMenu);
		    if (lppop2 == NULL) break;
		    if (lppop->BarFlags != 0) {
			lppop2->hWndParent = hwnd;
			GetWindowRect(hwnd, &rect);
			TrackPopupMenu(hSubMenu, TPM_LEFTBUTTON, 
				lpitem->rect.left, rect.top, 
				0, lppop->ownerWnd, (LPRECT)NULL);
			}
		    }
		ReleaseDC(hwnd, hDC);
		}
            }
	break;

    case WM_KEYDOWN:
    case WM_KEYUP:
	if (lParam < 0L) break;
	lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
	if (lppop->FocusedItem == (WORD)-1) {
	    if (wParam == VK_UP || wParam == VK_DOWN || 
		wParam == VK_LEFT || wParam == VK_RIGHT) {
		hDC = GetDC(hwnd);
		lppop->FocusedItem = 0;
		lpitem = GetMenuItemPtr(lppop, lppop->FocusedItem);
		if (((lpitem->item_flags & MF_POPUP) == MF_POPUP) ||
		    ((lpitem->item_flags & MF_STRING) == MF_STRING)) {
		    InvertRect(hDC, &lpitem->rect);
		    }
		ReleaseDC(hwnd, hDC);
		}
	    break;
	    }
	switch(wParam) {
	    case VK_UP:
		if (lppop->BarFlags != 0) break;
		if (lppop->FocusedItem < 1) break;
		lpitem = GetMenuItemPtr(lppop, lppop->FocusedItem);
		if ((lpitem->item_flags & MF_POPUP) == MF_POPUP)
		    HideAllSubPopupMenu(lppop);
		hDC = GetDC(hwnd);
		if (((lpitem->item_flags & MF_POPUP) == MF_POPUP) ||
		    ((lpitem->item_flags & MF_STRING) == MF_STRING)) {
		    InvertRect(hDC, &lpitem->rect);
		    }
		lppop->FocusedItem--;
		lpitem = GetMenuItemPtr(lppop, lppop->FocusedItem);
		if (((lpitem->item_flags & MF_POPUP) == MF_POPUP) ||
		    ((lpitem->item_flags & MF_STRING) == MF_STRING)) {
		    InvertRect(hDC, &lpitem->rect);
		    }
		ReleaseDC(hwnd, hDC);
		break;
	    case VK_DOWN:
		if (lppop->BarFlags != 0) goto ProceedSPACE;
		if (lppop->FocusedItem >= lppop->nItems - 1) break;
		lpitem = GetMenuItemPtr(lppop, lppop->FocusedItem);
		if ((lpitem->item_flags & MF_POPUP) == MF_POPUP)
		    HideAllSubPopupMenu(lppop);
		hDC = GetDC(hwnd);
		if (((lpitem->item_flags & MF_POPUP) == MF_POPUP) ||
		    ((lpitem->item_flags & MF_STRING) == MF_STRING)) {
		    InvertRect(hDC, &lpitem->rect);
		    }
		lppop->FocusedItem++;
		lpitem = GetMenuItemPtr(lppop, lppop->FocusedItem);
		if (((lpitem->item_flags & MF_POPUP) == MF_POPUP) ||
		    ((lpitem->item_flags & MF_STRING) == MF_STRING)) {
		    InvertRect(hDC, &lpitem->rect);
		    }
		ReleaseDC(hwnd, hDC);
		break;
	    case VK_LEFT:
		if (lppop->BarFlags == 0) {
		    if (lppop->hWndParent != 0) 
			SendMessage(lppop->hWndParent, WM_KEYDOWN, wParam, lParam);
		    break;
		    }
		if (lppop->FocusedItem < 1) break;
		lpitem = GetMenuItemPtr(lppop, lppop->FocusedItem);
		if ((lpitem->item_flags & MF_POPUP) == MF_POPUP)
		    HideAllSubPopupMenu(lppop);
		hDC = GetDC(hwnd);
		if (((lpitem->item_flags & MF_POPUP) == MF_POPUP) ||
		    ((lpitem->item_flags & MF_STRING) == MF_STRING)) {
		    InvertRect(hDC, &lpitem->rect);
		    }
		lppop->FocusedItem--;
		lpitem = GetMenuItemPtr(lppop, lppop->FocusedItem);
		if (((lpitem->item_flags & MF_POPUP) == MF_POPUP) ||
		    ((lpitem->item_flags & MF_STRING) == MF_STRING)) {
		    InvertRect(hDC, &lpitem->rect);
		    }
		ReleaseDC(hwnd, hDC);
		break;
	    case VK_RIGHT:
		if (lppop->BarFlags == 0) {
		    if (lppop->hWndParent != 0) 
			SendMessage(lppop->hWndParent, WM_KEYDOWN, wParam, lParam);
		    break;
		    }
		if (lppop->FocusedItem >= lppop->nItems - 1) break;
		lpitem = GetMenuItemPtr(lppop, lppop->FocusedItem);
		if ((lpitem->item_flags & MF_POPUP) == MF_POPUP)
		    HideAllSubPopupMenu(lppop);
		hDC = GetDC(hwnd);
		if (((lpitem->item_flags & MF_POPUP) == MF_POPUP) ||
		    ((lpitem->item_flags & MF_STRING) == MF_STRING)) {
		    InvertRect(hDC, &lpitem->rect);
		    }
		lppop->FocusedItem++;
		lpitem = GetMenuItemPtr(lppop, lppop->FocusedItem);
		if (((lpitem->item_flags & MF_POPUP) == MF_POPUP) ||
		    ((lpitem->item_flags & MF_STRING) == MF_STRING)) {
		    InvertRect(hDC, &lpitem->rect);
		    }
		ReleaseDC(hwnd, hDC);
		break;
	    case VK_RETURN:
	    case VK_SPACE:
ProceedSPACE:
		printf("PopupMenu VK_SPACE !\n");
		lpitem = GetMenuItemPtr(lppop, lppop->FocusedItem);
		if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) {
		    hSubMenu = (HMENU)lpitem->item_id;
		    lppop2 = (LPPOPUPMENU) GlobalLock(hSubMenu);
		    if (lppop2 == NULL) break;
		    lppop2->hWndParent = hwnd;
		    GetClientRect(hwnd, &rect);
		    if (lppop->BarFlags != 0) {
			y = rect.bottom - rect.top;
			TrackPopupMenu(hSubMenu, TPM_LEFTBUTTON, 
				lpitem->rect.left, 0, 
				0, lppop->ownerWnd, (LPRECT)NULL);
			}
		    else {
			x = rect.right;
			GetWindowRect(hwnd, &rect);
			x += rect.left;
			TrackPopupMenu(hSubMenu, TPM_LEFTBUTTON, 
				x, lpitem->rect.top,
				0, lppop->ownerWnd, (LPRECT)NULL);
			}
		    break;
		    }
		if (((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
		    ((lpitem->item_flags & MF_POPUP) != MF_POPUP)) {
		    ShowWindow(lppop->hWnd, SW_HIDE);
		    if (lppop->hWndParent != (HWND)NULL)
			SendMessage(lppop->hWndParent, WM_COMMAND, 
					lpitem->item_id, 0L);
		    else 
			SendMessage(lppop->ownerWnd, WM_COMMAND, 
					lpitem->item_id, 0L);
#ifdef DEBUG_MENU
		    printf("PopupMenu // SendMessage WM_COMMAND wParam=%d !\n", 
				lpitem->item_id);
#endif
		   }
		break;
	    }
	break;
    case WM_CHAR:
	if (lParam < 0L) break;
	lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
	if (wParam == VK_ESCAPE) {
	    if (lppop->hWndParent != 0) {
		lppop2 = PopupMenuGetWindowAndStorage(
			lppop->hWndParent, &wndPtr);
		HideAllSubPopupMenu(lppop2);
		break;
		}
	    if (lppop->FocusedItem != (WORD)-1) {
		lpitem2 = GetMenuItemPtr(lppop, lppop->FocusedItem);
		hDC = GetDC(hwnd);
		if (((lpitem2->item_flags & MF_POPUP) == MF_POPUP) ||
		    ((lpitem2->item_flags & MF_STRING) == MF_STRING)) {
		    InvertRect(hDC, &lpitem2->rect);
		    }
		ReleaseDC(hwnd, hDC);
		lppop->FocusedItem = (WORD)-1;
		}
	    }
	if (wParam >= 'a' && wParam <= 'z') wParam -= 'a' - 'A';
	lpitem = MenuFindItemBySelKey(hwnd, wParam, &wRet);
	if (lpitem != NULL) {
	    printf("Found  wRet=%d !\n", wRet);
	    if (lppop->FocusedItem != (WORD)-1) {
		lpitem2 = GetMenuItemPtr(lppop, lppop->FocusedItem);
		if ((lpitem2->item_flags & MF_POPUP) == MF_POPUP)
		    HideAllSubPopupMenu(lppop);
		hDC = GetDC(hwnd);
		if (((lpitem2->item_flags & MF_POPUP) == MF_POPUP) ||
		    ((lpitem2->item_flags & MF_STRING) == MF_STRING)) {
		    InvertRect(hDC, &lpitem2->rect);
		    }
		ReleaseDC(hwnd, hDC);
		}
	    lppop->FocusedItem = wRet;
	    goto ProceedSPACE;
	    }
	break;
    case WM_PAINT:
	lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
	if (lppop->BarFlags != 0) {
	    MenuBarCalcSize(hwnd);
	    printf("PopupMenu WM_PAINT Width=%d Height=%d !\n", 
			lppop->Width, lppop->Height);
	    StdDrawMenuBar(hwnd);
	    }
	else{
	    PopupMenuCalcSize(hwnd);
	    StdDrawPopupMenu(hwnd);
	    }
	break;
    default:
	return DefWindowProc( hwnd, message, wParam, lParam );
    }
return(0);
}



LPPOPUPMENU PopupMenuGetWindowAndStorage(HWND hwnd, WND **wndPtr)
{
    WND  *Ptr;
    LPPOPUPMENU lppop;
    *(wndPtr) = Ptr = WIN_FindWndPtr(hwnd);
    if (Ptr == 0) {
    	printf("Bad Window handle on PopupMenu !\n");
    	return 0;
    	}
    lppop = *((LPPOPUPMENU *)&Ptr->wExtra[1]);
    return lppop;
}


LPPOPUPMENU PopupMenuGetStorageHeader(HWND hwnd)
{
    WND  *Ptr;
    LPPOPUPMENU lppop;
    Ptr = WIN_FindWndPtr(hwnd);
    if (Ptr == 0) {
    	printf("Bad Window handle on PopupMenu !\n");
    	return 0;
    	}
    lppop = *((LPPOPUPMENU *)&Ptr->wExtra[1]);
    return lppop;
}


void SetMenuLogicalParent(HMENU hMenu, HWND hWnd)
{
    LPPOPUPMENU lppop;
    lppop = (LPPOPUPMENU)GlobalLock(hMenu);
    lppop->hWndParent = hWnd;
}


void StdDrawPopupMenu(HWND hwnd)
{
    WND 	*wndPtr;
    LPPOPUPMENU lppop;
    LPMENUITEM 	lpitem;
    PAINTSTRUCT ps;
    HBRUSH 	hBrush;
    HPEN	hOldPen;
    HWND	hWndParent;
    HDC 	hDC, hMemDC;
    RECT 	rect, rect2, rect3;
    DWORD	OldTextColor;
    HFONT	hOldFont;
    HBITMAP	hBitMap;
    BITMAP	bm;
    UINT  	i, x;
    hDC = BeginPaint( hwnd, &ps );
    if (!IsWindowVisible(hwnd)) {
	EndPaint( hwnd, &ps );
	return;
	}
    lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
    if (lppop == NULL) goto EndOfPaint;
    hBrush = GetStockObject(WHITE_BRUSH);
    GetClientRect(hwnd, &rect);
    GetClientRect(hwnd, &rect2);
    FillRect(hDC, &rect, hBrush);
    FrameRect(hDC, &rect, GetStockObject(BLACK_BRUSH));
    if (lppop->nItems == 0) goto EndOfPaint;
    lpitem = lppop->firstItem;
    if (lpitem == NULL) goto EndOfPaint;
    for(i = 0; i < lppop->nItems; i++) {
	if ((lpitem->item_flags & MF_SEPARATOR) == MF_SEPARATOR) {
	    CopyRect(&rect2, &lpitem->rect);
    	    hOldPen = SelectObject(hDC, GetStockObject(BLACK_PEN));
	    MoveTo(hDC, rect2.left, rect2.top + 1);
	    LineTo(hDC, rect2.right, rect2.top + 1);
	    SelectObject(hDC, hOldPen);
	    }
	if ((lpitem->item_flags & MF_BITMAP) == MF_BITMAP) {
	    hBitMap = (HBITMAP)LOWORD((LONG)lpitem->item_text);
	    CopyRect(&rect2, &lpitem->rect);
	    hMemDC = CreateCompatibleDC(hDC);
	    SelectObject(hMemDC, hBitMap);
	    GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&bm);
	    BitBlt(hDC, rect2.left, rect2.top,
		bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	    DeleteDC(hMemDC);
	    }
	if (((lpitem->item_flags & MF_BITMAP) != MF_BITMAP) &&
	    ((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
	    ((lpitem->item_flags & MF_MENUBREAK) != MF_MENUBREAK)) {
	    hOldFont = SelectObject(hDC, GetStockObject(SYSTEM_FONT));
	    if ((lpitem->item_flags & MF_DISABLED) == MF_DISABLED)
		OldTextColor = SetTextColor(hDC, 0x00C0C0C0L);
	    else
		OldTextColor = SetTextColor(hDC, 0x00000000L);
	    CopyRect(&rect3, &lpitem->rect);
	    InflateRect(&rect3, 0, -2);
	    if ((x = GetShortCutPos(lpitem->item_text)) != (WORD)-1) {
		DrawText(hDC, lpitem->item_text, x, &rect3, 
		    DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		DrawText(hDC, &lpitem->item_text[x], -1, &rect3, 
		    DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
		} 
	    else
		DrawText(hDC, lpitem->item_text, -1, &rect3, 
		    DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	    SetTextColor(hDC, OldTextColor);
	    SelectObject(hDC, hOldFont);
	    CopyRect(&rect2, &lpitem->rect);
	    }
	if ((lpitem->item_flags & MF_CHECKED) == MF_CHECKED) {
	    CopyRect(&rect3, &rect2);
	    rect3.left = rect3.right - rect3.bottom + rect3.top;
	    hMemDC = CreateCompatibleDC(hDC);
	    if (lpitem->hCheckBit == 0)
		SelectObject(hMemDC, hStdCheck);
	    else
		SelectObject(hMemDC, lpitem->hCheckBit);
	    GetObject(hStdCheck, sizeof(BITMAP), (LPSTR)&bm);
	    BitBlt(hDC, rect3.left, rect3.top,
		bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	    DeleteDC(hMemDC);
	    printf("StdDrawPopupMenu // MF_CHECKED hStdCheck=%04X !\n", hStdCheck);
	    }
	else {
	    if (lpitem->hUnCheckBit != 0)
		SelectObject(hMemDC, lpitem->hUnCheckBit);
	    }
	if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) {
	    CopyRect(&rect3, &rect2);
	    rect3.left = rect3.right - rect3.bottom + rect3.top;
	    hMemDC = CreateCompatibleDC(hDC);
	    SelectObject(hMemDC, hStdMnArrow);
	    GetObject(hStdMnArrow, sizeof(BITMAP), (LPSTR)&bm);
	    BitBlt(hDC, rect3.left, rect3.top,
		bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	    DeleteDC(hMemDC);
	    }
	if (lpitem->next == NULL) goto EndOfPaint;
	lpitem = (LPMENUITEM)lpitem->next;
    }
EndOfPaint:
    EndPaint( hwnd, &ps );
}



void StdDrawMenuBar(HWND hwnd)
{
    WND 	*wndPtr;
    LPPOPUPMENU lppop;
    LPMENUITEM 	lpitem;
    PAINTSTRUCT ps;
    HBRUSH 	hBrush;
    HPEN	hOldPen;
    HWND	hWndParent;
    HDC 	hDC, hMemDC;
    RECT 	rect, rect2, rect3;
    HFONT	hOldFont;
    HBITMAP	hBitMap;
    BITMAP	bm;
    UINT  	i, textwidth;
    hDC = BeginPaint( hwnd, &ps );
    if (!IsWindowVisible(hwnd)) {
	EndPaint( hwnd, &ps );
	return;
	}
    hOldFont = SelectObject(hDC, GetStockObject(SYSTEM_FONT));
    lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
    if (lppop == NULL) goto EndOfPaint;
    hBrush = GetStockObject(WHITE_BRUSH);
    GetClientRect(hwnd, &rect);
    FillRect(hDC, &rect, hBrush);
    FrameRect(hDC, &rect, GetStockObject(BLACK_BRUSH));
    if (lppop->nItems == 0) goto EndOfPaint;
    lpitem = lppop->firstItem;
    if (lpitem == NULL) goto EndOfPaint;
    for(i = 0; i < lppop->nItems; i++) {
	if ((lpitem->item_flags & MF_BITMAP) == MF_BITMAP) {
	    hBitMap = (HBITMAP)LOWORD((LONG)lpitem->item_text);
	    CopyRect(&rect2, &lpitem->rect);
	    hMemDC = CreateCompatibleDC(hDC);
	    SelectObject(hMemDC, hBitMap);
	    GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&bm);
	    BitBlt(hDC, rect2.left, rect2.top,
		bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	    DeleteDC(hMemDC);
	    }
	if (((lpitem->item_flags & MF_BITMAP) != MF_BITMAP) &&
	    ((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
	    ((lpitem->item_flags & MF_MENUBREAK) != MF_MENUBREAK)) {
	    CopyRect(&rect2, &lpitem->rect);
	    DrawText(hDC, lpitem->item_text, -1, &rect2, 
	    	DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	    }
	if ((lpitem->item_flags & MF_CHECKED) == MF_CHECKED) {
	    CopyRect(&rect3, &rect2);
	    rect3.left = rect3.right - rect3.bottom + rect3.top;
	    hMemDC = CreateCompatibleDC(hDC);
	    SelectObject(hMemDC, hStdCheck);
	    GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&bm);
	    BitBlt(hDC, rect3.left, rect3.top,
		bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	    DeleteDC(hMemDC);
	    }
	if (lpitem->next == NULL) goto EndOfPaint;
	lpitem = (LPMENUITEM)lpitem->next;
    }
EndOfPaint:
    SelectObject(hDC, hOldFont);
    EndPaint( hwnd, &ps );
}



LPMENUITEM MenuFindItem(HWND hwnd, int x, int y, WORD *lpRet)
{
    WND 	*wndPtr;
    LPPOPUPMENU lppop;
    LPMENUITEM 	lpitem;
    UINT  	i;
    lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
    if (lppop == NULL) return NULL;
    if (lppop->nItems == 0) return NULL;
    lpitem = lppop->firstItem;
    for(i = 0; i < lppop->nItems; i++) {
	if (lpitem == NULL) return NULL;
#ifdef DEBUG_MENUFINDITEM
	printf("FindItem // left=%d top=%d right=%d bottom=%d\n",
		lpitem->rect.left, lpitem->rect.top, 
		lpitem->rect.right, lpitem->rect.bottom);
#endif
	if (x > lpitem->rect.left && x < lpitem->rect.right && 
	    y > lpitem->rect.top && y < lpitem->rect.bottom) {
	    if (lpRet != NULL) *lpRet = i;
	    return lpitem;
	    }
	lpitem = (LPMENUITEM)lpitem->next;
    }
    return NULL;
}


LPMENUITEM MenuFindItemBySelKey(HWND hwnd, WORD key, WORD *lpRet)
{
    WND 	*wndPtr;
    LPPOPUPMENU lppop;
    LPMENUITEM 	lpitem;
    UINT  	i;
    lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
    if (lppop == NULL) return NULL;
    if (lppop->nItems == 0) return NULL;
    lpitem = lppop->firstItem;
    for(i = 0; i < lppop->nItems; i++) {
	if (lpitem == NULL) return NULL;
#ifdef DEBUG_MENUFINDITEM
	printf("FindItemBySelKey // key=%d lpitem->sel_key=%d\n",
		key, lpitem->sel_key);
#endif
	if (key == lpitem->sel_key) {
	    if (lpRet != NULL) *lpRet = i;
	    return lpitem;
	    }
	lpitem = (LPMENUITEM)lpitem->next;
    }
    return NULL;
}


void PopupMenuCalcSize(HWND hwnd)
{
    WND 	*wndPtr;
    LPPOPUPMENU lppop;
    LPMENUITEM 	lpitem;
    HDC		hDC;
    RECT 	rect;
    HBITMAP	hBitMap;
    BITMAP	bm;
    HFONT	hOldFont;
    UINT  	i, OldWidth, TempWidth;
    DWORD	dwRet;
#ifdef DEBUG_MENUCALC
	printf("PopupMenuCalcSize hWnd=%04X !\n", hWnd);
#endif
    lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
    if (lppop == NULL) return;
    if (lppop->nItems == 0) return;
    hDC = GetDC(hwnd);
    lppop->Width = 20;
    hOldFont = SelectObject(hDC, GetStockObject(SYSTEM_FONT));
CalcAGAIN:
    OldWidth = lppop->Width;
    SetRect(&rect, 1, 1, OldWidth, 0);
    lpitem = lppop->firstItem;
    for(i = 0; i < lppop->nItems; i++) {
	if (lpitem == NULL) break;
#ifdef DEBUG_MENUCALC
	printf("PopupMenuCalcSize item #%d !\n", i);
#endif
	rect.right = rect.left + lppop->Width;
	if ((lpitem->item_flags & MF_SEPARATOR) == MF_SEPARATOR) {
	    rect.bottom = rect.top + 3;
	    }
	if ((lpitem->item_flags & MF_BITMAP) == MF_BITMAP) {
	    hBitMap = (HBITMAP)LOWORD((LONG)lpitem->item_text);
	    GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&bm);
	    rect.bottom = rect.top + bm.bmHeight;
	    lppop->Width = max(lppop->Width, bm.bmWidth);
	    }
	if (((lpitem->item_flags & MF_BITMAP) != MF_BITMAP) &&
	    ((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
	    ((lpitem->item_flags & MF_MENUBREAK) != MF_MENUBREAK)) {
	    dwRet = GetTextExtent(hDC, (char *)lpitem->item_text, 
		strlen((char *)lpitem->item_text));
	    rect.bottom = rect.top + HIWORD(dwRet);
	    InflateRect(&rect, 0, 2);
	    TempWidth = LOWORD(dwRet);
	    if (GetShortCutPos(lpitem->item_text) != (WORD)-1)
	        TempWidth += 15;
	    lppop->Width = max(lppop->Width, TempWidth);
	    }
	CopyRect(&lpitem->rect, &rect);
	rect.top = rect.bottom;
	lpitem = (LPMENUITEM)lpitem->next;
	}
    if (OldWidth < lppop->Width) goto CalcAGAIN;
    lppop->Height = rect.bottom;
#ifdef DEBUG_MENUCALC
    printf("PopupMenuCalcSize w=%d h=%d !\n", lppop->Width, lppop->Height);
#endif
    SelectObject(hDC, hOldFont);
    ReleaseDC(hwnd, hDC);
}



void MenuBarCalcSize(HWND hwnd)
{
    WND 	*wndPtr;
    LPPOPUPMENU lppop;
    LPMENUITEM 	lpitem;
    HDC		hDC;
    RECT 	rect;
    HBITMAP	hBitMap;
    BITMAP	bm;
    HFONT	hOldFont;
    UINT  	i, OldHeight;
    DWORD	dwRet;
    lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
    if (lppop == NULL) return;
    if (lppop->nItems == 0) return;
    hDC = GetDC(hwnd);
    hOldFont = SelectObject(hDC, GetStockObject(SYSTEM_FONT));
    lppop->Height = 10;
CalcAGAIN:
    OldHeight = lppop->Height;
    SetRect(&rect, 1, 1, 0, OldHeight);
    lpitem = lppop->firstItem;
    for(i = 0; i < lppop->nItems; i++) {
	if (lpitem == NULL) break;
	rect.bottom = rect.top + lppop->Height;
	if ((lpitem->item_flags & MF_BITMAP) == MF_BITMAP) {
	    hBitMap = (HBITMAP)LOWORD((LONG)lpitem->item_text);
	    GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&bm);
	    rect.right = rect.left + bm.bmWidth;
	    lppop->Height = max(lppop->Height, bm.bmHeight);
	    }
	if (((lpitem->item_flags & MF_BITMAP) != MF_BITMAP) &&
	    ((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
	    ((lpitem->item_flags & MF_MENUBREAK) != MF_MENUBREAK)) {
	    dwRet = GetTextExtent(hDC, (char *)lpitem->item_text, 
		strlen((char *)lpitem->item_text));
	    rect.right = rect.left + LOWORD(dwRet) + 10;
	    lppop->Height = max(lppop->Height, HIWORD(dwRet) + 10);
	    }
	CopyRect(&lpitem->rect, &rect);
	rect.left = rect.right;
	lpitem = (LPMENUITEM)lpitem->next;
	}
    if (OldHeight < lppop->Height) goto CalcAGAIN;
    lppop->Width = rect.right;
#ifdef DEBUG_MENUCALC
    printf("MenuBarCalcSize w=%d h=%d !\n", 
    	lppop->Width, lppop->Height);
#endif
    SelectObject(hDC, hOldFont);
    ReleaseDC(hwnd, hDC);
}



LPMENUITEM GetMenuItemPtr(LPPOPUPMENU menu, WORD nPos)
{
    LPMENUITEM 	lpitem;
    int		i;
    if (menu == NULL) return NULL;
    lpitem = menu->firstItem;
    for (i = 0; i < menu->nItems; i++) {
    	if (lpitem == NULL) return NULL;
    	if (i == nPos) return(lpitem);
    	lpitem = (LPMENUITEM)lpitem->next;
    	}
    return NULL;
}


WORD GetSelectionKey(LPSTR str)
{
    int		i;
    WORD	sel_key;
    for (i = 0; i < strlen(str); i++) {
	if (str[i] == '&' && str[i + 1] != '&') 
	    {
	    sel_key = str[i + 1];
	    if (sel_key >= 'a' && sel_key <= 'z') sel_key -= 'a' - 'A';
#ifdef DEBUG_MENU
	    printf("GetSelectionKey // %04X\n", sel_key);
#endif
	    return sel_key;
	    }
	}
#ifdef DEBUG_MENU
    printf("GetSelectionKey NULL \n");
#endif
    return 0;
}



LPSTR GetShortCutString(LPSTR str)
{
    int		i;
    LPSTR	str2;
    for (i = 0; i < strlen(str); i++) {
	if (str[i] == '\t' && str[i + 1] != '\t') 
	    {
	    str2 = &str[i + 1];
#ifdef DEBUG_MENU
	    printf("GetShortCutString // '%s' \n", str2);
#endif
	    return str2;
	    }
	}
#ifdef DEBUG_MENU
    printf("GetShortCutString NULL \n");
#endif
    return NULL;
}



WORD GetShortCutPos(LPSTR str)
{
    int		i;
    for (i = 0; i < strlen(str); i++) {
	if (str[i] == '\t' && str[i + 1] != '\t') 
	    {
#ifdef DEBUG_MENU
	    printf("GetShortCutPos = %d \n", i);
#endif
	    return i;
	    }
	}
#ifdef DEBUG_MENU
    printf("GetShortCutString NULL \n");
#endif
    return -1;
}



BOOL HideAllSubPopupMenu(LPPOPUPMENU menu)
{
    LPPOPUPMENU submenu;
    LPMENUITEM 	lpitem;
    BOOL	someClosed = FALSE;
    int		i;
    if (menu == NULL) return;
    lpitem = menu->firstItem;
    for (i = 0; i < menu->nItems; i++) {
    	if (lpitem == NULL) return;
    	if (lpitem->item_flags & MF_POPUP) {
	    submenu = (LPPOPUPMENU) GlobalLock((HMENU)lpitem->item_id);
	    if (submenu != NULL) {
		if (IsWindowVisible(submenu->hWnd)) {
		    ShowWindow(submenu->hWnd, SW_HIDE);
		    someClosed = TRUE;
		    }
	    	}
    	    }
    	lpitem = (LPMENUITEM)lpitem->next;
    	}
    return someClosed;
}


#ifdef USE_XTMENU

/**********************************************************************
 *					MENU_CheckWidget
 */
void
MENU_CheckWidget(Widget w, Boolean check)
{
    if (checkBitmap == XtUnspecifiedPixmap)
    {
	Display *display = XtDisplayOfObject(w);
	    
	checkBitmap = XCreateBitmapFromData(display,
					    DefaultRootWindow(display),
					    check_bitmap_bits,
					    check_bitmap_width,
					    check_bitmap_height);
	nocheckBitmap = XCreateBitmapFromData(display,
					    DefaultRootWindow(display),
					    nocheck_bitmap_bits,
					    nocheck_bitmap_width,
					    nocheck_bitmap_height);
    }
	    
    if (check)
	XtVaSetValues(w, XtNleftBitmap, checkBitmap, NULL);
    else
	XtVaSetValues(w, XtNleftBitmap, nocheckBitmap, NULL);
}

/**********************************************************************
 *					MENU_ParseMenu
 */
WORD *
MENU_ParseMenu(WORD *first_item, 
	       int level,
	       int limit,
	       int (*action)(WORD *item, int level, void *app_data),
	       void *app_data)
{
    WORD *item;
    WORD *next_item;
    int   i;

    level++;
    next_item = first_item;
    i = 0;
    do
    {
	i++;
	item = next_item;
	(*action)(item, level, app_data);
	if (*item & MF_POPUP)
	{
	    MENU_POPUPITEM *popup_item = (MENU_POPUPITEM *) item;

	    next_item = (WORD *) (popup_item->item_text + 
				  strlen(popup_item->item_text) + 1);
	    next_item = MENU_ParseMenu(next_item, level, 0, action, app_data);
	}
	else
	{
	    MENU_NORMALITEM *normal_item = (MENU_NORMALITEM *) item;

	    next_item = (WORD *) (normal_item->item_text + 
				  strlen(normal_item->item_text) + 1);
	}
    }
    while (!(*item & MF_END) && i != limit);

    return next_item;
}

/**********************************************************************
 *					MENU_FindMenuBar
 */
LPMENUBAR
MENU_FindMenuBar(MENUITEM *this_item)
{
    MENUITEM *root;
    LPMENUBAR menu;

    /*
     * Find root item on menu bar.
     */
    for (root = this_item; root->parent != NULL; root = root->parent)
	;
    for ( ; root->prev != NULL; root = root->prev)
	;

    /*
     * Find menu bar for the root item.
     */
    for (menu = firstMenu; 
	 menu != NULL && menu->firstItem != root; 
	 menu = menu->next)
	;
    
    return menu;
}

/**********************************************************************
 *					MENU_SelectionCallback
 */
static void
MENU_SelectionCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
    MENUITEM *this_item = (MENUITEM *) client_data;
    LPMENUBAR menu;
    WND	     *wndPtr;

    if (this_item->menu_w != NULL || (this_item->item_flags & MF_DISABLED))
	return;

    /*
     * Find menu bar for the root item.
     */
    menu = MENU_FindMenuBar(this_item);
    if (menu != NULL)
    {
	wndPtr = WIN_FindWndPtr(menu->ownerWnd);
	if (wndPtr == NULL)
	    return;

#ifdef DEBUG_MENU
	printf("Selected '%s' (%d).\n", 
	       this_item->item_text, this_item->item_id);
#endif
	
	CallWindowProc(wndPtr->lpfnWndProc, menu->ownerWnd, WM_COMMAND,
		       this_item->item_id, 0);
    }
}

/**********************************************************************
 *					MENU_CreateItems
 */
int
MENU_CreateItems(WORD *item, int level, void *app_data)
{
    MENU_POPUPITEM     *popup_item;
    MENU_NORMALITEM    *normal_item;
    MENUITEM	       *this_item;
    Arg			this_args[10];
    int			n_args = 0;
    LPMENUBAR           menu = (LPMENUBAR) app_data;

    if (menu->nItems == 0)
	this_item = menu->firstItem;
    else
	this_item = (MENUITEM *) GlobalQuickAlloc(sizeof(MENUITEM));

    if (this_item == NULL)
	return 0;

    if (level > lastLevel)
    {
	parentItem  = siblingItem;
	siblingItem = NULL;
    }

    while (level < lastLevel)
    {
	siblingItem = parentItem;
	if (siblingItem != NULL)
	    parentItem = siblingItem->parent;
	else
	    parentItem = NULL;

	lastLevel--;
    }
    lastLevel = level;
    
    this_item->next = NULL;
    this_item->prev = siblingItem;
    this_item->child = NULL;
    this_item->parent = parentItem;
    
    if (siblingItem !=  NULL)
	siblingItem->next = this_item;
    if (parentItem != NULL && parentItem->child == NULL)
	parentItem->child = this_item;
    
    siblingItem = this_item;
    
    if (*item & MF_POPUP)
    {
	popup_item = (MENU_POPUPITEM *) item;
	this_item->item_flags = popup_item->item_flags;
	this_item->item_id    = -1;
	this_item->item_text  = popup_item->item_text;

#ifdef DEBUG_MENU
	printf("%d: popup %s\n", level, this_item->item_text);
#endif
    }
    else
    {
	normal_item = (MENU_NORMALITEM *) item;
	this_item->item_flags = normal_item->item_flags;
	this_item->item_id    = normal_item->item_id;
	this_item->item_text  = normal_item->item_text;

#ifdef DEBUG_MENU
	printf("%d: normal %s (%04x)\n", level, this_item->item_text,
	       this_item->item_flags);
#endif
    }

    if (level == 1)
    {
	menu->nItems++;

	if (this_item->prev != NULL)
	{
	    XtSetArg(this_args[n_args], XtNhorizDistance, 10); 
	    n_args++;
	    XtSetArg(this_args[n_args], XtNfromHoriz, this_item->prev->w); 
	    n_args++;
	}

	if (this_item->item_flags & MF_POPUP)
	{
	    sprintf(this_item->menu_name, "Menu%d", menuId++);
	    XtSetArg(this_args[n_args], XtNmenuName, this_item->menu_name);
	    n_args++;
	    
	    this_item->w = XtCreateManagedWidget(this_item->item_text,
						 winMenuButtonWidgetClass, 
						 menu->parentWidget,
						 this_args, n_args);
	    this_item->menu_w = XtCreatePopupShell(this_item->menu_name,
						 simpleMenuWidgetClass, 
						 this_item->w,
						 NULL, 0);
	}
	else
	{
	    this_item->w = XtCreateManagedWidget(this_item->item_text,
						 winCommandWidgetClass, 
						 menu->parentWidget,
						 this_args, n_args);
	    this_item->menu_w = NULL;
	    XtAddCallback(this_item->w, XtNcallback, MENU_SelectionCallback,
			  (XtPointer) this_item);
	}

	if (menu->firstItem == NULL)
	    menu->firstItem = this_item;
    }
    else
    {
	if ((this_item->item_flags & MF_MENUBREAK) ||
	    (strlen(this_item->item_text) == 0))
	{
	    XtSetArg(this_args[n_args], XtNheight, 10);
	    n_args++;
	    this_item->w = XtCreateManagedWidget("separator",
						 smeLineObjectClass,
						 this_item->parent->menu_w,
						 this_args, n_args);
	}
	else
	{
	    XtSetArg(this_args[n_args], XtNmenuName, this_item->menu_name);
	    n_args++;
	    this_item->w = XtCreateManagedWidget(this_item->item_text,
						 smeMenuButtonObjectClass,
						 this_item->parent->menu_w,
						 this_args, n_args);

	    if (this_item->item_flags & MF_POPUP)
	    {
		sprintf(this_item->menu_name, "Menu%d", menuId++);
		this_item->menu_w = XtCreatePopupShell(this_item->menu_name,
						     simpleMenuWidgetClass, 
						     this_item->parent->menu_w,
						     NULL, 0);
	    }
	    else
	    {
		this_item->menu_w = NULL;
		XtAddCallback(this_item->w, XtNcallback, 
			      MENU_SelectionCallback, (XtPointer) this_item);
	    }
	}
    }

    if (this_item->w != NULL)
    {
	if (this_item->item_flags & MF_GRAYED)
	    XtSetSensitive(this_item->w, False);
	if (this_item->item_flags & MF_DISABLED)
	    XtVaSetValues(this_item->w, XtNinactive, True, NULL);
	if (this_item->item_flags & MF_CHECKED)
	    MENU_CheckWidget(this_item->w, True);
    }

    return 1;
}

/**********************************************************************
 *					MENU_UseMenu
 */
LPMENUBAR
MENU_UseMenu(Widget parent, HANDLE instance, HWND wnd, HMENU hmenu, int width)
{
    LPMENUBAR 		menubar;
    MENUITEM           *menu;
    MENU_HEADER        *menu_desc;

    menu  = (MENUITEM *) GlobalLock(hmenu);
    if (hmenu == 0 || menu == NULL)
    {
	return NULL;
    }

    menubar = MENU_FindMenuBar(menu);
    if (menubar == NULL)
    {
	GlobalUnlock(hmenu);
	return NULL;
    }

    menubar->nItems 	     = 0;
    menubar->parentWidget    = parent;
    menubar->ownerWnd	     = wnd;

    menu_desc = (MENU_HEADER *) GlobalLock(menubar->menuDescription);
    
    parentItem  = NULL;
    siblingItem = NULL;
    lastLevel   = 0;
    MENU_ParseMenu((WORD *) (menu_desc + 1), 0, 0, MENU_CreateItems, menubar);

    menubar->menuBarWidget = menubar->firstItem->w;

    menubar->next = firstMenu;
    firstMenu = menubar;
    
    return menubar;
}

/**********************************************************************
 *					MENU_CreateMenuBar
 */
LPMENUBAR
MENU_CreateMenuBar(Widget parent, HANDLE instance, HWND wnd,
		   char *menu_name, int width)
{
    LPMENUBAR 		menubar;
    HMENU     		hmenu;
    MENUITEM           *menu;
    MENU_HEADER        *menu_desc;

#ifdef DEBUG_MENU
    printf("CreateMenuBar: instance %02x, menu '%s', width %d\n",
	   instance, menu_name, width);
#endif

    hmenu = LoadMenu(instance, menu_name);
    return MENU_UseMenu(parent, instance, wnd, hmenu, width);
}

/**********************************************************************
 *					MENU_FindItem
 */
MENUITEM *
MENU_FindItem(MENUITEM *menu, WORD item_id, WORD flags)
{
    MENUITEM *item;
    WORD position;
    
    if (flags & MF_BYPOSITION)
    {
	item = menu;
	for (position = 0; item != NULL && position != item_id; position++)
	    item = item->next;
	
	if (position == item_id)
	    return item;
    }
    else
    {
	for ( ; menu != NULL; menu = menu->next)
	{
	    if (menu->item_id == item_id && menu->child == NULL)
		return menu;
	    if (menu->child != NULL)
	    {
		item = MENU_FindItem(menu->child, item_id, flags);
		if (item != NULL)
		    return item;
	    }
	}
    }

    return NULL;
}

/**********************************************************************
 *					MENU_CollapseMenu
 */
static void
MENU_CollapseBranch(MENUITEM *item, Boolean first_flag)
{
    MENUITEM *next_item;
    
    for ( ; item != NULL; item = next_item)
    {
	next_item = item->next;
	
	if (item->child != NULL)
	    MENU_CollapseBranch(item->child, False);
	    
	if (item->w != NULL)
	    XtDestroyWidget(item->w);
	if (item->menu_w != NULL)
	    XtDestroyWidget(item->menu_w);
	
	if (first_flag)
	{
	    item->prev 	     = NULL;
	    item->child      = NULL;
	    item->next 	     = NULL;
	    item->parent     = NULL;
	    item->item_flags = 0;
	    item->item_id    = 0;
	    item->item_text  = NULL;
	    item->w 	     = NULL;
	    item->menu_w     = NULL;

	    first_flag = False;
	}
	else
	{
	    GlobalFree((unsigned int) item);
	}
    }
}

void
MENU_CollapseMenu(LPMENUBAR menubar)
{
    MENU_CollapseBranch(menubar->firstItem, True);
    
    menubar->nItems 	     = 0;
    menubar->parentWidget    = NULL;
    menubar->ownerWnd	     = 0;
    menubar->menuBarWidget   = NULL;
}


/**********************************************************************
 *					CheckMenu
 */
BOOL
CheckMenu(HMENU hmenu, WORD item_id, WORD check_flags)
{
    MENUITEM *item;
    Pixmap left_bitmap;

    if ((item = (MENUITEM *) GlobalLock(hmenu)) == NULL)
	return -1;
    
    item = MENU_FindItem(item, item_id, check_flags);
    if (item == NULL)
    {
	GlobalUnlock(hmenu);
	return -1;
    }

    XtVaGetValues(item->w, XtNleftBitmap, &left_bitmap, NULL);
    MENU_CheckWidget(item->w, (check_flags & MF_CHECKED));

    if (left_bitmap == XtUnspecifiedPixmap)
	return MF_UNCHECKED;
    else
	return MF_CHECKED;
}


/**********************************************************************
 *					LoadMenu
 */
HMENU
LoadMenu(HINSTANCE instance, char *menu_name)
{
    HANDLE		hmenubar;
    LPMENUBAR 		menu;
    HANDLE		hmenu_desc;
    HMENU     		hmenu;
    MENU_HEADER        *menu_desc;

#ifdef DEBUG_MENU
    printf("LoadMenu: instance %02x, menu '%s'\n",
	   instance, menu_name);
#endif

    if (menu_name == NULL || 
	(hmenu_desc = RSC_LoadMenu(instance, menu_name)) == 0 ||
	(menu_desc = (MENU_HEADER *) GlobalLock(hmenu_desc)) == NULL)
    {
	return 0;
    }

    hmenubar = GlobalAlloc(GMEM_MOVEABLE, sizeof(MENUBAR));
    menu = (LPMENUBAR) GlobalLock(hmenubar);
    if (menu == NULL)
    {
	GlobalFree(hmenu_desc);
	GlobalFree(hmenubar);
	return 0;
    }

    hmenu = GlobalAlloc(GMEM_MOVEABLE, sizeof(MENUITEM));
    if (hmenu == 0)
    {
	GlobalFree(hmenu_desc);
	GlobalFree(hmenubar);
	return 0;
    }

    menu->menuDescription = hmenu_desc;
    menu->nItems 	  = 0;
    menu->parentWidget    = NULL;
    menu->firstItem       = (MENUITEM *) GlobalLock(hmenu);
    menu->ownerWnd	  = 0;
    menu->menuBarWidget   = NULL;

    menu->firstItem->next 	= NULL;
    menu->firstItem->prev 	= NULL;
    menu->firstItem->child 	= NULL;
    menu->firstItem->parent 	= NULL;
    menu->firstItem->item_flags	= 0;
    menu->firstItem->item_id 	= 0;
    menu->firstItem->item_text 	= NULL;
    menu->firstItem->w 		= NULL;
    menu->firstItem->menu_w 	= NULL;

    menu->next = firstMenu;
    firstMenu  = menu;
    
    return GlobalHandleFromPointer(menu->firstItem);
}

#endif


/**********************************************************************
 *			CheckMenuItem		[USER.154]
 */
BOOL CheckMenuItem(HMENU hMenu, WORD wItemID, WORD wFlags)
{
    WND		*wndPtr;
    LPPOPUPMENU	menu;
    LPMENUITEM 	lpitem;
    int		i;
#ifdef DEBUG_MENU
    printf("CheckMenuItem (%04X, %04X, %04X) !\n", hMenu, wItemID, wFlags);
#endif
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    if (menu == NULL) return FALSE;
    lpitem = menu->firstItem;
    for (i = 0; i < menu->nItems; i++) {
    	if (lpitem == NULL) break;
    	if (i == wItemID) {
	    if (wFlags && MF_CHECKED)
		lpitem->item_flags |= MF_CHECKED;
	    else
		lpitem->item_flags &= ((WORD)-1 ^ MF_CHECKED);
	    return(TRUE);
	    }
    	lpitem = (LPMENUITEM)lpitem->next;
    	}
    return FALSE;
}


/**********************************************************************
 *			EnableMenuItem		[USER.155]
 */
BOOL EnableMenuItem(HMENU hMenu, WORD wItemID, WORD wFlags)
{
    WND		*wndPtr;
    LPPOPUPMENU	menu;
    LPMENUITEM 	lpitem;
    int		i;
#ifdef DEBUG_MENU
    printf("EnableMenuItem (%04X, %04X, %04X) !\n", hMenu, wItemID, wFlags);
#endif
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    if (menu == NULL) return FALSE;
    lpitem = menu->firstItem;
    for (i = 0; i < menu->nItems; i++) {
    	if (lpitem == NULL) break;
    	if (i == wItemID) {
	    if (wFlags && MF_DISABLED)
		lpitem->item_flags |= MF_DISABLED;
	    else
		lpitem->item_flags &= ((WORD)-1 ^ MF_DISABLED);
	    return(TRUE);
	    }
    	lpitem = (LPMENUITEM)lpitem->next;
    	}
    return FALSE;
}


/**********************************************************************
 *			InsertMenu		[USER.410]
 */
BOOL InsertMenu(HMENU hMenu, WORD nPos, WORD wFlags, WORD wItemID, LPSTR lpNewItem)
{
    WND		*wndPtr;
    LPPOPUPMENU	menu;
    HANDLE	hNewItem;
    LPMENUITEM 	lpitem, lpitem2;
    int		i;
#ifdef DEBUG_MENU
    printf("InsertMenu (%04X, %04X, %04X, %04X, %08X) !\n",
	hMenu, nPos, wFlags, wItemID, lpNewItem);
#endif
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    if (menu == NULL) return FALSE;
    lpitem = menu->firstItem;
    for (i = 0; i < menu->nItems; i++) {
    	if (lpitem == NULL) break;
    	if (i == nPos) break;
    	lpitem = (LPMENUITEM)lpitem->next;
	printf("InsertMenu // during loop items !\n");
    	}
    printf("InsertMenu // after loop items !\n");
    hNewItem = GlobalAlloc(GMEM_MOVEABLE, sizeof(MENUITEM));
    if (hNewItem == 0) return FALSE;
    lpitem2 = (LPMENUITEM)GlobalLock(hNewItem);
    if (lpitem2 == NULL) {
	GlobalFree(hNewItem);
	return FALSE;
	}
    lpitem2->item_flags = wFlags;
    lpitem2->item_id = wItemID;
    if (((wFlags & MF_BITMAP) != MF_BITMAP) &&
	((wFlags & MF_MENUBREAK) != MF_MENUBREAK) &&
	((wFlags & MF_STRING) != MF_SEPARATOR)) {
	lpitem2->item_text = lpNewItem;
	lpitem2->sel_key = GetSelectionKey(lpitem2->item_text);
	}
    else
	lpitem2->item_text = lpNewItem;
    lpitem2->prev = lpitem;
    if (lpitem->next != NULL)
	lpitem2->next = lpitem->next;
    else
	lpitem2->next = NULL;
    lpitem->next = lpitem2;
    lpitem2->child = NULL;
    lpitem2->parent = NULL;
    lpitem2->w = NULL;
    lpitem2->menu_w = NULL;
    menu->nItems++;
    return TRUE;
}


/**********************************************************************
 *			AppendMenu		[USER.411]
 */
BOOL AppendMenu(HMENU hMenu, WORD wFlags, WORD wItemID, LPSTR lpNewItem)
{
    WND		*wndPtr;
    LPPOPUPMENU	menu;
    HANDLE	hNewItem;
    LPMENUITEM 	lpitem, lpitem2;
#ifdef DEBUG_MENU
    printf("AppendMenu (%04X, %04X, %04X, %08X) !\n",
	hMenu, wFlags, wItemID, lpNewItem);
#endif
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    if (menu == NULL) return FALSE;
    lpitem = menu->firstItem;
    if (lpitem != NULL) {
	while (lpitem->next != NULL) {
	    lpitem = (LPMENUITEM)lpitem->next;
	    }
	}
    hNewItem = GlobalAlloc(GMEM_MOVEABLE, sizeof(MENUITEM));
    if (hNewItem == 0) return FALSE;
    lpitem2 = (LPMENUITEM)GlobalLock(hNewItem);
    if (lpitem2 == NULL) {
	GlobalFree(hNewItem);
	return FALSE;
	}
    lpitem2->item_flags = wFlags;
    lpitem2->item_id = wItemID;
    if (((wFlags & MF_BITMAP) != MF_BITMAP) &&
	((wFlags & MF_MENUBREAK) != MF_MENUBREAK) &&
	((wFlags & MF_STRING) != MF_SEPARATOR)) {
	lpitem2->item_text = lpNewItem;
	lpitem2->sel_key = GetSelectionKey(lpitem2->item_text);
	lpitem2->shortcut = GetShortCutString(lpitem2->item_text);
	}
    else
	lpitem2->item_text = lpNewItem;
    if (lpitem == NULL)
    	menu->firstItem = lpitem2;
    else
    	lpitem->next = lpitem2;
    lpitem2->prev = lpitem;
    lpitem2->next = NULL;
    lpitem2->child = NULL;
    lpitem2->parent = NULL;
    lpitem2->w = NULL;
    lpitem2->menu_w = NULL;
    lpitem2->hCheckBit = (HBITMAP)NULL;
    lpitem2->hUnCheckBit = (HBITMAP)NULL;
    menu->nItems++;
    return TRUE;
}


/**********************************************************************
 *			RemoveMenu		[USER.412]
 */
BOOL RemoveMenu(HMENU hMenu, WORD nPos, WORD wFlags)
{
    WND		*wndPtr;
    LPPOPUPMENU	menu;
    LPMENUITEM 	lpitem;
    int		i;
#ifdef DEBUG_MENU
    printf("RemoveMenu (%04X, %04X, %04X) !\n", hMenu, nPos, wFlags);
#endif
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    if (menu == NULL) return FALSE;
    lpitem = menu->firstItem;
    for (i = 0; i < menu->nItems; i++) {
    	if (lpitem == NULL) break;
    	if (i == nPos) {
    	    lpitem->prev->next = lpitem->next;
    	    lpitem->next->prev = lpitem->prev;
    	    GlobalFree(HIWORD(lpitem));
    	    return(TRUE);
	    }
    	lpitem = (LPMENUITEM)lpitem->next;
	printf("RemoveMenu // during loop items !\n");
    	}
    printf("RemoveMenu // after loop items !\n");
    return FALSE;
}


/**********************************************************************
 *			DeleteMenu		[USER.413]
 */
BOOL DeleteMenu(HMENU hMenu, WORD nPos, WORD wFlags)
{
#ifdef DEBUG_MENU
    printf("DeleteMenu (%04X, %04X, %04X) !\n", hMenu, nPos, wFlags);
#endif
    return TRUE;
}


/**********************************************************************
 *			ModifyMenu		[USER.414]
 */
BOOL ModifyMenu(HMENU hMenu, WORD nPos, WORD wFlags, WORD wItemID, LPSTR lpNewItem)
{
    WND		*wndPtr;
    LPPOPUPMENU	menu;
    LPMENUITEM 	lpitem;
    int		i;
#ifdef DEBUG_MENU
    printf("ModifyMenu (%04X, %04X, %04X, %04X, %08X) !\n",
	hMenu, nPos, wFlags, wItemID, lpNewItem);
#endif
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    if (menu == NULL) return FALSE;
    lpitem = menu->firstItem;
    for (i = 0; i < menu->nItems; i++) {
    	if (lpitem == NULL) break;
    	if (i == nPos) {
    	    lpitem->item_flags = wFlags;
    	    lpitem->item_id    = wItemID;
    	    lpitem->item_text  = lpNewItem;
    	    return(TRUE);
	    }
    	lpitem = (LPMENUITEM)lpitem->next;
    	}
    return FALSE;
}


/**********************************************************************
 *			CreatePopupMenu		[USER.415]
 */
HMENU CreatePopupMenu()
{
    HANDLE	hItem;
    HMENU	hMenu;
    LPPOPUPMENU menu;
#ifdef DEBUG_MENU
    printf("CreatePopupMenu !\n");
#endif
    hMenu = GlobalAlloc(GMEM_MOVEABLE, sizeof(POPUPMENU));
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    if (menu == NULL) {
	GlobalFree(hMenu);
	return 0;
 	}
    menu->nItems 	  = 0;
    menu->firstItem       = NULL;
    menu->ownerWnd	  = 0;
    menu->hWnd		  = 0;
    menu->hWndParent	  = 0;
    menu->MouseFlags	  = 0;
    menu->BarFlags	  = 0;
    menu->SysFlag	  = FALSE;
    menu->Width = 100;
    menu->Height = 0;
    return hMenu;
}


/**********************************************************************
 *			TrackPopupMenu		[USER.416]
 */
BOOL TrackPopupMenu(HMENU hMenu, WORD wFlags, short x, short y,
	short nReserved, HWND hWnd, LPRECT lpRect)
{
    WND		*wndPtr;
    LPPOPUPMENU	lppop;
#ifdef DEBUG_MENU
    printf("TrackPopupMenu (%04X, %04X, %d, %d, %04X, %04X, %08X) !\n",
	hMenu, wFlags, x, y, nReserved, hWnd, lpRect);
#endif
    lppop = (LPPOPUPMENU) GlobalLock(hMenu);
    if (lppop == NULL) return FALSE;
    wndPtr = WIN_FindWndPtr(hWnd);
    lppop->ownerWnd = hWnd;
    if (lppop->hWnd == (HWND)NULL) {
        lppop->hWnd = CreateWindow("POPUPMENU", "", WS_POPUP | WS_VISIBLE,
        	x, y, lppop->Width, lppop->Height, (HWND)NULL, 0, 
        	wndPtr->hInstance, (LPSTR)lppop);
        }
    else {
    	ShowWindow(lppop->hWnd, SW_SHOW);
    	}
    if (lppop->BarFlags == 0) {
	PopupMenuCalcSize(lppop->hWnd);
#ifdef DEBUG_MENU
	printf("TrackPopupMenu // x=%d y=%d Width=%d Height=%d\n", 
		x, y, lppop->Width, lppop->Height); 
#endif
	SetWindowPos(lppop->hWnd, 0, x, y, lppop->Width + 2, lppop->Height, 
		SWP_NOZORDER);
	}
    else {
	MenuBarCalcSize(lppop->hWnd);
#ifdef DEBUG_MENU
	printf("TrackMenuBar // x=%d y=%d Width=%d Height=%d\n", 
		x, y, lppop->Width, lppop->Height); 
#endif
	SetWindowPos(lppop->hWnd, 0, 0, -16, lppop->Width, lppop->Height, 
		SWP_NOZORDER);
	}
    return TRUE;
}


/**********************************************************************
 *			NC_TrackSysMenu		[Internal]
 */
void NC_TrackSysMenu(hWnd)
{
    RECT	rect;
    LPPOPUPMENU	lpsys;
    WND *wndPtr = WIN_FindWndPtr(hWnd);    
#ifdef DEBUG_MENU
    printf("NC_TrackSysMenu hWnd=%04X !\n", hWnd);
#endif
    if (!wndPtr) return;
    lpsys = (LPPOPUPMENU)GlobalLock(wndPtr->hSysMenu);
#ifdef DEBUG_MENU
    printf("NC_TrackSysMenu wndPtr->hSysMenu=%04X !\n", wndPtr->hSysMenu);
#endif
    if (lpsys == NULL) return;
#ifdef DEBUG_MENU
    printf("NC_TrackSysMenu wndPtr->hSysMenu=%04X !\n", wndPtr->hSysMenu);
#endif
    lpsys->BarFlags = FALSE;
    lpsys->SysFlag = TRUE;
    if (!IsWindowVisible(lpsys->hWnd)) {
	GetWindowRect(hWnd, &rect);
#ifdef DEBUG_MENU
	printf("NC_TrackSysMenu lpsys->hWnd=%04X !\n", lpsys->hWnd);
#endif
	TrackPopupMenu(wndPtr->hSysMenu, TPM_LEFTBUTTON, 
		rect.left, rect.top + SYSMETRICS_CYSIZE, 
		0, hWnd, (LPRECT)NULL);
	}
    else {
	ShowWindow(lpsys->hWnd, SW_HIDE);
	}
}


/**********************************************************************
 *			SetMenuItemBitmaps	[USER.418]
 */
BOOL SetMenuItemBitmaps(HMENU hMenu, WORD nPos, WORD wFlags,
		HBITMAP hNewCheck, HBITMAP hNewUnCheck)
{
    WND		*wndPtr;
    LPPOPUPMENU	menu;
    LPMENUITEM 	lpitem;
    int		i;
#ifdef DEBUG_MENU
    printf("SetMenuItemBitmaps (%04X, %04X, %04X, %04X, %08X) !\n",
	hMenu, nPos, wFlags, hNewCheck, hNewUnCheck);
#endif
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    if (menu == NULL) return FALSE;
    lpitem = menu->firstItem;
    for (i = 0; i < menu->nItems; i++) {
    	if (lpitem == NULL) break;
    	if (i == nPos) {
    	    lpitem->hCheckBit   = hNewCheck;
     	    lpitem->hUnCheckBit = hNewUnCheck;
    	    return(TRUE);
	    }
    	lpitem = (LPMENUITEM)lpitem->next;
    	}
    return FALSE;
}


/**********************************************************************
 *			CreateMenu		[USER.151]
 */
HMENU CreateMenu()
{
    HANDLE	hItem;
    HMENU	hMenu;
    LPPOPUPMENU menu;
#ifdef DEBUG_MENU
    printf("CreatePopupMenu !\n");
#endif
    hMenu = GlobalAlloc(GMEM_MOVEABLE, sizeof(POPUPMENU));
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    if (menu == NULL) {
	GlobalFree(hMenu);
	return 0;
 	}
    menu->nItems 	  = 0;
    menu->firstItem       = NULL;
    menu->ownerWnd	  = 0;
    menu->hWnd		  = 0;
    menu->hWndParent	  = 0;
    menu->MouseFlags	  = 0;
    menu->BarFlags	  = TRUE;
    menu->SysFlag	  = FALSE;
    menu->Width = 100;
    menu->Height = 0;
    return hMenu;
}


/**********************************************************************
 *			DestroyMenu		[USER.152]
 */
BOOL DestroyMenu(HMENU hMenu)
{
    LPPOPUPMENU lppop;
    LPMENUITEM 	lpitem, lpitem2;
#ifdef DEBUG_MENU
    printf("DestroyMenu (%04X) !\n", hMenu);
#endif
    if (hMenu == 0) return FALSE;
    lppop = (LPPOPUPMENU) GlobalLock(hMenu);
    if (lppop == NULL) return FALSE;
    if (lppop->hWnd) DestroyWindow (lppop->hWnd);
    lpitem = lppop->firstItem;
    while (lpitem != NULL) {
#ifdef DEBUG_MENU
	printf("DestroyMenu (%04X) // during loop items !\n", hMenu);
#endif
	if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) {
	    DestroyMenu((HMENU)lpitem->item_id);
	    }
    	lpitem = (LPMENUITEM)lpitem->next;
    	}
    GlobalFree(hMenu);
#ifdef DEBUG_MENU
    printf("DestroyMenu (%04X) // End !\n", hMenu);
#endif
    return TRUE;
}


#ifdef USE_POPUPMENU


/**********************************************************************
 *			LoadMenu		[USER.150]
 */
HMENU LoadMenu(HINSTANCE instance, char *menu_name)
{
    HMENU     		hMenu;
    HANDLE		hMenu_desc;
    MENU_HEADER 	*menu_desc;

#ifdef DEBUG_MENU
    if ((LONG)menu_name & 0xFFFF0000L)
	printf("LoadMenu: instance %02x, menu '%s'\n", instance, menu_name);
    else
	printf("LoadMenu: instance %02x, menu '%04X'\n", instance, menu_name);
#endif
    if (instance == (HANDLE)NULL)  instance = hSysRes;
    if (menu_name == NULL || 
	(hMenu_desc = RSC_LoadMenu(instance, menu_name)) == 0 ||
	(menu_desc = (MENU_HEADER *) GlobalLock(hMenu_desc)) == NULL)
    {
	return 0;
    }
    hMenu = CreateMenu();
    ParseMenuResource((WORD *) (menu_desc + 1), 0, hMenu);
    return hMenu;
}


/**********************************************************************
 *			GetSystemMenu		[USER.156]
 */
HMENU GetSystemMenu(HWND hWnd, BOOL bRevert)
{
    WND		*wndPtr;
    wndPtr = WIN_FindWndPtr(hWnd);
    if (!bRevert) {
	return wndPtr->hSysMenu;
	}
    else {
	DestroyMenu(wndPtr->hSysMenu);
	wndPtr->hSysMenu = CopySysMenu();
	}
    return wndPtr->hSysMenu;
}


/**********************************************************************
 *			GetMenu		[USER.157]
 */
HMENU GetMenu(HWND hWnd) 
{ 
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == NULL) return 0;
    return wndPtr->wIDmenu;
}

/**********************************************************************
 * 			SetMenu 	[USER.158]
 */
BOOL SetMenu(HWND hWnd, HMENU hMenu)
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == NULL) return FALSE;
    wndPtr->wIDmenu = hMenu;
    return TRUE;
}


/**********************************************************************
 *			GetSubMenu		[USER.159]
 */
HMENU GetSubMenu(HMENU hMenu, short nPos)
{
    HMENU	hSubMenu;
    LPPOPUPMENU lppop;
    LPMENUITEM 	lpitem;
    int		i;
#ifdef DEBUG_MENU
    printf("GetSubMenu (%04X, %04X) !\n", hMenu, nPos);
#endif
    if (hMenu == 0) return 0;
    lppop = (LPPOPUPMENU) GlobalLock(hMenu);
    if (lppop == NULL) return 0;
    lpitem = lppop->firstItem;
    for (i = 0; i < lppop->nItems; i++) {
    	if (lpitem == NULL) break;
    	if (i == nPos) {
	    if (lpitem->item_flags & MF_POPUP)
		return hSubMenu;
	    else
		return 0;
	    }
    	lpitem = (LPMENUITEM)lpitem->next;
    	}
    return 0;
}


/**********************************************************************
 *			DrawMenuBar		[USER.160]
 */
void DrawMenuBar(HWND hWnd)
{
    WND		*wndPtr;
#ifdef DEBUG_MENU
    printf("DrawMenuBar (%04X)\n", hWnd);
#endif
    wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr != NULL && wndPtr->hWndMenuBar != 0) {
	InvalidateRect(wndPtr->hWndMenuBar, NULL, TRUE);
	UpdateWindow(wndPtr->hWndMenuBar);
	}
}


/**********************************************************************
 *			CopySysMenu (Internal)
 */
HMENU CopySysMenu()
{
    HMENU     		hMenu;
    LPPOPUPMENU 	menu;
    LPPOPUPMENU 	sysmenu;
#ifdef DEBUG_MENU
    printf("CopySysMenu entry !\n");
#endif
    if (hSysMenu == 0) {
	hSysMenu = LoadMenu((HINSTANCE)NULL, MAKEINTRESOURCE(1));
/*	hSysMenu = LoadMenu((HINSTANCE)NULL, MAKEINTRESOURCE(SC_SYSMENU));*/
/*	hSysMenu = LoadMenu((HINSTANCE)NULL, "SYSMENU"); */
	if (hSysMenu == 0) {
	    printf("SysMenu not found in system resources !\n");
	    return (HMENU)NULL;
	    }
#ifdef DEBUG_MENU
	else
	    printf("SysMenu loaded from system resources %04X !\n", hSysMenu);
#endif
	}
    hMenu = GlobalAlloc(GMEM_MOVEABLE, sizeof(POPUPMENU));
    menu = (LPPOPUPMENU) GlobalLock(hMenu);
    sysmenu = (LPPOPUPMENU) GlobalLock(hSysMenu);
    if (menu != NULL && sysmenu != NULL) {
	sysmenu->BarFlags = FALSE;
	memcpy(menu, sysmenu, sizeof(POPUPMENU));
	}
    else {
	printf("CopySysMenu // Bad SysMenu pointers !\n");
	if (menu != NULL) {
	    GlobalUnlock(hMenu);
	    GlobalFree(hMenu);
	    }
	return (HMENU)NULL;
	}
    GlobalUnlock(hMenu);
    GlobalUnlock(hSysMenu);
    return hMenu;
}


/**********************************************************************
 *			ParseMenuResource (for Xlib version)
 */
WORD * ParseMenuResource(WORD *first_item, int level, HMENU hMenu)
{
    WORD 	*item;
    WORD 	*next_item;
    HMENU	hSubMenu;
    int   	i;

    level++;
    next_item = first_item;
    i = 0;
    do {
	i++;
	item = next_item;
	if (*item & MF_POPUP) {
	    MENU_POPUPITEM *popup_item = (MENU_POPUPITEM *) item;
	    next_item = (WORD *) (popup_item->item_text + 
				  strlen(popup_item->item_text) + 1);
	    hSubMenu = CreatePopupMenu();
	    next_item = ParseMenuResource(next_item, level, hSubMenu);
	    AppendMenu(hMenu, popup_item->item_flags, 
	    	hSubMenu, popup_item->item_text);
	    }
	else {
	    MENU_NORMALITEM *normal_item = (MENU_NORMALITEM *) item;
	    next_item = (WORD *) (normal_item->item_text + 
				  strlen(normal_item->item_text) + 1);
	    AppendMenu(hMenu, normal_item->item_flags, 
	    	normal_item->item_id, normal_item->item_text);
	    }
	}
    while (!(*item & MF_END));
    return next_item;
}


#endif

