/*
 *        Menus functions
 */
static char RCSId[] = "$Id$";
static char Copyright[] = "Copyright  Martin Ayotte, 1993";

/*
#define DEBUG_MENU
*/

#include <stdlib.h>
#include <stdio.h>
#include "windows.h"
#include "sysmetrics.h"
#include "prototypes.h"
#include "menu.h"
#include "user.h"
#include "heap.h"
#include "win.h"

#define SC_ABOUTWINE    	SC_SCREENSAVE+1
#define SC_SYSMENU			SC_SCREENSAVE+2
#define SC_ABOUTWINEDLG		SC_SCREENSAVE+3

extern HINSTANCE hSysRes;
HMENU	hSysMenu = 0;
HBITMAP hStdCheck = 0;
HBITMAP hStdMnArrow = 0;
static BOOL	MenuHasFocus = FALSE;

LPPOPUPMENU PopupMenuGetStorageHeader(HWND hwnd);
LPPOPUPMENU PopupMenuGetWindowAndStorage(HWND hwnd, WND **wndPtr);
void StdDrawMenuBar(HDC hDC, LPRECT lprect, LPPOPUPMENU lppop, 
		    BOOL suppress_draw);
BOOL MenuButtonDown(HWND hWnd, LPPOPUPMENU lppop, int x, int y);
void MenuButtonUp(HWND hWnd, LPPOPUPMENU lppop, int x, int y);
void MenuMouseMove(HWND hWnd, LPPOPUPMENU lppop, WORD wParam, int x, int y);
void StdDrawPopupMenu(HWND hwnd);
void ResetHiliteFlags(LPPOPUPMENU lppop);
BOOL ExecFocusedMenuItem(HWND hWnd, LPPOPUPMENU lppop);
void MenuItemSelect(HWND hWnd, LPPOPUPMENU lppop, WORD wIndex);
LPMENUITEM MenuFindItem(LPPOPUPMENU lppop, int x, int y, WORD *lpRet);
LPMENUITEM MenuFindItemBySelKey(LPPOPUPMENU lppop, WORD key, WORD *lpRet);
BOOL ActivateMenuBarFocus(HWND hWnd);
BOOL MenuFocusLoop(HWND hWnd, LPPOPUPMENU lpmenu);
void PopupMenuCalcSize(HWND hwnd);
void MenuBarCalcSize(HDC hDC, LPRECT lprect, LPPOPUPMENU lppop);
LPMENUITEM FindMenuItem(HMENU hMenu, WORD nPos, WORD wFlags);
LPMENUITEM GetMenuItemPtr(LPPOPUPMENU menu, WORD nPos);
WORD GetSelectionKey(LPSTR str);
LPSTR GetShortCutString(LPSTR str);
WORD GetShortCutPos(LPSTR str);
BOOL HideAllSubPopupMenu(LPPOPUPMENU menu);
void InitStdBitmaps();
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
	PAINTSTRUCT ps;
	switch(message) {
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
		InitStdBitmaps();
#ifdef DEBUG_MENU
		printf("PopupMenu End of WM_CREATE !\n");
#endif
		return 0;
	case WM_DESTROY:
		lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
#ifdef DEBUG_MENU
		printf("PopupMenu WM_DESTROY %lX !\n", lppop);
#endif
		return 0;
	case WM_COMMAND:
#ifdef DEBUG_MENU
		printf("PopupMenuWndProc // WM_COMMAND received !\n");
#endif
		lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
		if (lppop == NULL) break;
/*		if (!lppop->BarFlag)  ShowWindow(hwnd, SW_HIDE); */
		if (lppop->SysFlag) {
			MenuHasFocus = FALSE;
			if (wParam == SC_ABOUTWINE) {
				printf("SysMenu // Show 'About Wine ...' !\n");
/*				DialogBox(hSysRes, MAKEINTRESOURCE(SC_ABOUTWINEDLG), */
				DialogBox(hSysRes, MAKEINTRESOURCE(2), 
					GetParent(hwnd), (FARPROC)AboutWine_Proc);
				}
			else
#ifdef DEBUG_MENU
				printf("PopupMenuWndProc // push to Owner WM_SYSCOMMAND !\n");
#endif
				PostMessage(lppop->ownerWnd, WM_SYSCOMMAND, wParam, lParam);
/*				PostMessage(lppop->hWndParent, WM_SYSCOMMAND, wParam, lParam); */
			break;
			}
#ifdef DEBUG_MENU
		printf("PopupMenuWndProc // push to Owner WM_COMMAND !\n");
#endif
		MenuHasFocus = FALSE;
		PostMessage(lppop->hWndParent, WM_COMMAND, wParam, lParam);
/*		PostMessage(lppop->ownerWnd, WM_COMMAND, wParam, lParam); */
		break;
	case WM_SHOWWINDOW:
#ifdef DEBUG_MENU
		printf("PopupMenuWndProc // WM_SHOWWINDOW received !\n");
#endif
		lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
		if (lppop == NULL) break;
		if (wParam == 0 && lParam == 0L) {
			HideAllSubPopupMenu(lppop);
#ifdef DEBUG_MENU
			printf("PopupMenuWndProc hWnd=%04X WM_SHOWWINDOW -> HIDE!\n", hwnd);
#endif
			if (lppop->SysFlag) MenuHasFocus = FALSE;
			SetFocus(lppop->hWndPrev);
			if (GetCapture() != 0) ReleaseCapture(); 
			break;
			}
		lppop->FocusedItem = (WORD)-1;
		if (!lppop->BarFlag) {
			PopupMenuCalcSize(hwnd);
			ResetHiliteFlags(lppop);
#ifdef DEBUG_MENU
			printf("PopupMenuWndProc hWnd=%04X WM_SHOWWINDOW Width=%d Height=%d !\n", 
									hwnd, lppop->Width, lppop->Height);
#endif
			SetWindowPos(hwnd, 0, 0, 0, lppop->Width + 2, lppop->Height, 
											SWP_NOZORDER | SWP_NOMOVE);
#ifdef DEBUG_MENU
			printf("PopupMenuWndProc // End of WM_SHOWWINDOW !\n");
#endif
			}
    	break;
	case WM_LBUTTONDOWN:
		lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
		if (lppop == NULL) break;
		SetCapture(hwnd); 
		MenuButtonDown(hwnd, lppop, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONUP:
		lppop = PopupMenuGetStorageHeader(hwnd);
		if (lppop == NULL) break;
		ReleaseCapture(); 
		MenuButtonUp(hwnd, lppop, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_MOUSEMOVE:
		lppop = PopupMenuGetStorageHeader(hwnd);
		if (lppop == NULL) break;
		MenuMouseMove(hwnd, lppop, wParam, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_KEYUP:
#ifdef DEBUG_MENU
		printf("PopupMenuWndProc hWnd=%04X WM_KEYUP w=%04X l=%08X !\n", 
												hwnd, wParam, lParam);
#endif
		break;
	case WM_KEYDOWN:
#ifdef DEBUG_MENU
		printf("PopupMenuWndProc hWnd=%04X WM_KEYDOWN w=%04X l=%08X !\n", 
												hwnd, wParam, lParam);
#endif
		if (lParam < 0L) break;
		lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
		if (lppop == NULL) break;
		switch(wParam) {
			case VK_HOME:
				if (lppop->FocusedItem == 0) break;
				MenuItemSelect(hwnd, lppop, 0);
				break;
			case VK_UP:
				if (lppop->BarFlag) break;
				if (lppop->FocusedItem < 1) break;
				MenuItemSelect(hwnd, lppop, lppop->FocusedItem - 1);
				break;
			case VK_DOWN:
				if (lppop->BarFlag) goto ProceedSPACE;
				if (lppop->FocusedItem == (WORD)-1) {
					MenuItemSelect(hwnd, lppop, lppop->FocusedItem + 1);
					break;
					}
				if (lppop->FocusedItem >= lppop->nItems - 1) break;
				MenuItemSelect(hwnd, lppop, lppop->FocusedItem + 1);
				break;
			case VK_LEFT:
				if (lppop->SysFlag != 0) {
					ShowWindow(hwnd, SW_HIDE);
					hwnd = lppop->hWndParent;
					lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
					printf("VK_LEFT // try to put focus on MenuBar %08X !\n", lppop);
					if (lppop == NULL) break;
					MenuItemSelect(hwnd, lppop, lppop->nItems - 1);
				    break;
					}
				if (lppop->BarFlag) {
					if (lppop->FocusedItem < 1) {
						MenuItemSelect(hwnd, lppop, -1);
						NC_TrackSysMenu(hwnd);
						break;
						}
					if (HideAllSubPopupMenu(lppop)) {
						MenuItemSelect(hwnd, lppop, lppop->FocusedItem - 1);
						goto ProceedSPACE;
						}
					}
			    if (lppop->hWndParent != 0) {
					PostMessage(lppop->hWndParent, WM_KEYDOWN, wParam, lParam);
				    break;
				    }
				MenuItemSelect(hwnd, lppop, lppop->FocusedItem - 1);
				break;
			case VK_RIGHT:
				if (lppop->SysFlag != 0) {
					ShowWindow(hwnd, SW_HIDE);
					hwnd = lppop->hWndParent;
					lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
					printf("VK_RIGHT // try to put focus on MenuBar %08X !\n", lppop);
					if (lppop == NULL) break;
					MenuItemSelect(hwnd, lppop, 0);
				    break;
					}
				if (lppop->BarFlag) {
					if (lppop->FocusedItem >= lppop->nItems - 1) {
						MenuItemSelect(hwnd, lppop, -1);
						NC_TrackSysMenu(hwnd);
						break;
						}
					if (HideAllSubPopupMenu(lppop)) {
						MenuItemSelect(hwnd, lppop, lppop->FocusedItem + 1);
						goto ProceedSPACE;
						}
					}
			    if (lppop->hWndParent != 0) {
					PostMessage(lppop->hWndParent, WM_KEYDOWN, wParam, lParam);
				    break;
				    }
				MenuItemSelect(hwnd, lppop, lppop->FocusedItem + 1);
				break;
			default:
				break;
			}
			break;
	case WM_CHAR:
#ifdef DEBUG_MENU
		printf("PopupMenuWndProc hWnd=%04X WM_CHAR wParam=%04X !\n", hwnd, wParam);
#endif
		if (lParam < 0L) break;
		hwnd = GetFocus();
		lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
		if (lppop == NULL) break;
		switch(wParam) {
			case VK_RETURN:
			case VK_SPACE:
ProceedSPACE:	lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
				ExecFocusedMenuItem(hwnd, lppop);
				break;
			case VK_ESCAPE:
				if (lppop->BarFlag) {
#ifdef DEBUG_MENU
					printf("VK_ESCAPE // Unselect all MenuBar's Items !\n");
#endif
					if (lppop->FocusedItem != (WORD)-1)
						MenuItemSelect(hwnd, lppop, -1);
					}
				if (lppop->SysFlag) {
#ifdef DEBUG_MENU
					printf("VK_ESCAPE // SysMenu !\n");
#endif
					ShowWindow(hwnd, SW_HIDE);
					break;
					}
				if (lppop->hWndParent != 0) {
#ifdef DEBUG_MENU
					printf("VK_ESCAPE // Hide only SubPopup !\n");
#endif
					lppop2 = PopupMenuGetWindowAndStorage(lppop->hWndParent, &wndPtr);
					if (lppop2 == NULL) break;
					HideAllSubPopupMenu(lppop2);
					break;
					}
				else {
#ifdef DEBUG_MENU
					printf("VK_ESCAPE // Hide Root Popup !\n");
#endif
					ShowWindow(hwnd, SW_HIDE);
					MenuHasFocus = FALSE;
					}
				break;
			default:
				if (wParam >= 'a' && wParam <= 'z') wParam -= 'a' - 'A';
				lpitem = MenuFindItemBySelKey(lppop, wParam, &wRet);
				if (lpitem != NULL) {
				    printf("ShortKey Found  wRet=%d !\n", wRet);
					MenuItemSelect(hwnd, lppop, wRet);
					lppop->FocusedItem = wRet;
					goto ProceedSPACE;
					}
			    printf("ShortKey Not Found wParam=%04X wRet=%d lpitem=%08X !\n",
					wParam, wRet, lpitem);
				if (lppop->hWndParent != (HWND)NULL)
					SendMessage(lppop->hWndParent, WM_MENUCHAR, wParam, 
						MAKELONG(0, 0));
				else 
					SendMessage(lppop->ownerWnd, WM_MENUCHAR, wParam, 
						MAKELONG(0, 0));
				break;
			}
		break;
	case WM_PAINT:
#ifdef DEBUG_MENU
		printf("PopupMenuWndProc // WM_PAINT received !\n");
#endif
		lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
		if (lppop == NULL) break;
		if (!lppop->BarFlag) {
			PopupMenuCalcSize(hwnd);
			StdDrawPopupMenu(hwnd);
			}
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
    }
return 0;
}


BOOL ExecFocusedMenuItem(HWND hWnd, LPPOPUPMENU lppop)
{
	short	x, y;
	LPPOPUPMENU lppop2;
	LPMENUITEM	lpitem;
	HMENU	hSubMenu;
	RECT	rect;
	lpitem = GetMenuItemPtr(lppop, lppop->FocusedItem);
	if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) {
		hSubMenu = (HMENU)lpitem->item_id;
		lppop2 = (LPPOPUPMENU) GlobalLock(hSubMenu);
		if (lppop2 == NULL) return FALSE;
		lppop2->hWndParent = hWnd;
		GetClientRect(hWnd, &rect);
		if (lppop->BarFlag) {
			GetWindowRect(hWnd, &rect);
			y = rect.top + lppop->rect.bottom;
			TrackPopupMenu(hSubMenu, TPM_LEFTBUTTON, 
				rect.left + lpitem->rect.left, 
				y, 0, lppop->ownerWnd, (LPRECT)NULL);
			}
		else {
			x = lppop->rect.right;
			GetWindowRect(hWnd, &rect);
			x += rect.left;
			TrackPopupMenu(hSubMenu, TPM_LEFTBUTTON, 
				x, rect.top + lpitem->rect.top,
				0, lppop->ownerWnd, (LPRECT)NULL);
			}
		GlobalUnlock(hSubMenu);
		return TRUE;
		}
	if (((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
		((lpitem->item_flags & MF_POPUP) != MF_POPUP)) {
		MenuHasFocus = FALSE;
		if (lppop->BarFlag) {
			PostMessage(lppop->ownerWnd, WM_COMMAND, lpitem->item_id, 0L);
			}
		else {
			ShowWindow(lppop->hWnd, SW_HIDE);
			SendMessage(lppop->hWnd, WM_COMMAND, lpitem->item_id, 0L);
			}
		}
	return TRUE;
}



BOOL MenuButtonDown(HWND hWnd, LPPOPUPMENU lppop, int x, int y)
{
	HDC		hDC;
	LPMENUITEM	lpitem, lpitem2;
	RECT	rect;
	HMENU	hSubMenu;
	WORD	wRet;
	LPPOPUPMENU lppop2;
	if (lppop == NULL) return;
	lpitem = MenuFindItem(lppop, x, y, &wRet);
#ifdef DEBUG_MENU
	printf("MenuButtonDown hWnd=%04X x=%d y=%d // wRet=%d lpitem=%08X !\n", 
											hWnd, x, y, wRet, lpitem);
#endif
	if (lpitem != NULL) {
		MenuItemSelect(hWnd, lppop, wRet);
		if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) {
			hSubMenu = (HMENU)lpitem->item_id;
			lppop2 = (LPPOPUPMENU) GlobalLock(hSubMenu);
			if (lppop2 == NULL) return;
			lppop2->hWndParent = hWnd;
			if (lppop->BarFlag) {
				GetWindowRect(hWnd, &rect);
				y = rect.top + lppop->rect.bottom;
				ReleaseCapture(); 
				if (MenuHasFocus) {
					TrackPopupMenu(hSubMenu, TPM_LEFTBUTTON, 
						rect.left + lpitem->rect.left, 
						y, 0, lppop->ownerWnd, (LPRECT)NULL);
					}
				else {
					MenuHasFocus = TRUE;
					TrackPopupMenu(hSubMenu, TPM_LEFTBUTTON, 
						rect.left + lpitem->rect.left, 
						y, 0, lppop->ownerWnd, (LPRECT)NULL);
					MenuHasFocus = FALSE;
					MenuFocusLoop(hWnd, lppop);
					return TRUE;
					}
				}
			else {
				x = lppop->rect.right;
				GetWindowRect(hWnd, &rect);
				x += rect.left;
				TrackPopupMenu(hSubMenu, TPM_LEFTBUTTON,
					x, rect.top + lpitem->rect.top,
					0, lppop->ownerWnd, (LPRECT)NULL);
				}
			GlobalUnlock(hSubMenu);
			return TRUE;
			}
		if (lppop->BarFlag && !MenuHasFocus) {
			MenuFocusLoop(hWnd, lppop);
			}
		return TRUE;
		}
	printf("MenuButtonDown // x=%d y=%d // Not Found !\n", x, y);
	MenuHasFocus = FALSE;
	ShowWindow(lppop->hWnd, SW_HIDE);
	return FALSE;
}



void MenuButtonUp(HWND hWnd, LPPOPUPMENU lppop, int x, int y)
{
	HDC		hDC;
	LPMENUITEM	lpitem, lpitem2;
	RECT	rect;
	HMENU	hSubMenu;
	WORD	wRet;
	LPPOPUPMENU lppop2;
	if (lppop == NULL) return;
	lpitem = MenuFindItem(lppop, x, y, &wRet);
#ifdef DEBUG_MENU
	printf("MenuButtonUp // x=%d y=%d // wRet=%d lpitem=%08X !\n", 
											x, y, wRet, lpitem);
#endif
	if (lpitem != NULL) {
		if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) {
			return;
			}
		if (((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
			((lpitem->item_flags & MF_POPUP) != MF_POPUP)) {
			MenuHasFocus = FALSE;
			if (lppop->BarFlag) {
				PostMessage(lppop->ownerWnd, WM_COMMAND, lpitem->item_id, 0L);
				}
			else {
				ShowWindow(lppop->hWnd, SW_HIDE);
				SendMessage(lppop->hWnd, WM_COMMAND, lpitem->item_id, 0L);
				}
			return;
			}
		}
	if (lppop->FocusedItem != (WORD)-1) {
		MenuItemSelect(hWnd, lppop, lppop->FocusedItem);
		}
}



void MenuMouseMove(HWND hWnd, LPPOPUPMENU lppop, WORD wParam, int x, int y)
{
    HDC		hDC;
    RECT	rect;
    HMENU	hSubMenu;
    LPMENUITEM	lpitem, lpitem2;
    LPPOPUPMENU lppop2;
    WORD	wRet;

    if (GetKeyState(VK_LBUTTON)	!= 0) 
    {
	lpitem = MenuFindItem(lppop, x, y, &wRet);
#ifdef DEBUG_MENU
	printf("MenuMouseMove // x=%d y=%d // wRet=%d lpitem=%08X !\n", 
	       x, y, wRet, lpitem);
#endif
	if ((lpitem != NULL) && (lppop->FocusedItem != wRet)) 
	{
	    lpitem2 = GetMenuItemPtr(lppop, lppop->FocusedItem);
	    hDC = GetWindowDC(hWnd);
	    if ((lpitem2 != NULL ) &&
		(lpitem2->item_flags & MF_POPUP) == MF_POPUP) 
	    {
		HideAllSubPopupMenu(lppop);
	    }
	    MenuItemSelect(hWnd, lppop, wRet);
	    if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) 
	    {
		hSubMenu = (HMENU)lpitem->item_id;
		lppop2 = (LPPOPUPMENU) GlobalLock(hSubMenu);
		if (lppop2 == NULL) 
		{
		    ReleaseDC(hWnd, hDC);
		    return;
		}
		if (lppop->BarFlag) 
		{
		    lppop2->hWndParent = hWnd;
		    GetWindowRect(hWnd, &rect);
		    rect.top += lppop->rect.bottom;
		    TrackPopupMenu(hSubMenu, TPM_LEFTBUTTON, 
				   rect.left + lpitem->rect.left, rect.top, 
				   0, lppop->ownerWnd, (LPRECT)NULL);
		}
		GlobalUnlock(hSubMenu);
	    }
	    ReleaseDC(hWnd, hDC);
	}
    }
}


void ResetHiliteFlags(LPPOPUPMENU lppop)
{
	LPMENUITEM lpitem;
	int		i;
	if (lppop == NULL) return;
	lpitem = lppop->firstItem;
	for(i = 0; i < lppop->nItems; i++) {
		if (lpitem == NULL) return;
		lpitem->item_flags &= MF_HILITE ^ 0xFFFF;
		lpitem = (LPMENUITEM)lpitem->next;
		}
}


void MenuItemSelect0(HWND hWnd, LPPOPUPMENU lppop, 
				LPMENUITEM lpitem, WORD wIndex)
{
    LPMENUITEM lpprev;
    
    if (lppop == NULL) 
	return;
    if (lppop->FocusedItem != (WORD)-1) 
    {
	lpprev = GetMenuItemPtr(lppop, lppop->FocusedItem);
	if (lpprev != NULL) 
	{
	    lpprev->item_flags &= MF_HILITE ^ 0xFFFF;
	    if ((lpprev->item_flags & MF_POPUP) == MF_POPUP)
		HideAllSubPopupMenu(lppop);
	    if (lppop->BarFlag)
		DrawMenuBar(hWnd);
	    else 
	    {
		InvalidateRect(hWnd, &lpprev->rect, TRUE);
		UpdateWindow(hWnd);
	    }
	}
    }
    lppop->FocusedItem = wIndex;
    if (lpitem == NULL || wIndex == (WORD)-1) 
    {
	ResetHiliteFlags(lppop);
	if (lppop->BarFlag) DrawMenuBar(hWnd);
    }
    else 
    {
	lpitem->item_flags |= MF_HILITE;
	if (lppop->BarFlag)
	    DrawMenuBar(hWnd);
	else 
	{
	    InvalidateRect(hWnd, &lpitem->rect, TRUE);
	    UpdateWindow(hWnd);
	}
	SendMessage(hWnd, WM_MENUSELECT, lpitem->item_id, 
		    MAKELONG(0, lpitem->item_flags));
    }
}


void MenuItemSelect(HWND hWnd, LPPOPUPMENU lppop, WORD wIndex)
{
	LPMENUITEM lpitem;
    lpitem = GetMenuItemPtr(lppop, wIndex);
	MenuItemSelect0(hWnd, lppop, lpitem, wIndex);
}


LPPOPUPMENU PopupMenuGetWindowAndStorage(HWND hwnd, WND **wndPtr)
{
    WND  *Ptr;
    LPPOPUPMENU lppop;
    *(wndPtr) = Ptr = WIN_FindWndPtr(hwnd);
    if (Ptr == 0) {
    	printf("PopupMenuGetWindowAndStorage // Bad Window handle !\n");
    	return NULL;
    	}
    lppop = *((LPPOPUPMENU *)&Ptr->wExtra[1]);
	if (lppop == NULL) {
		lppop = (LPPOPUPMENU) GlobalLock(Ptr->wIDmenu);
		if (lppop == NULL) {
			printf("PopupMenuGetWindowAndStorage // Bad Menu Handle !\n");
	    	return NULL;
			}
		}
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
    GlobalUnlock(hMenu);
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
	int		OldBkMode;
	HFONT	hOldFont;
	HBITMAP	hBitMap;
	BITMAP	bm;
	UINT  	i, x;
	hDC = BeginPaint(hwnd, &ps);
	if (!IsWindowVisible(hwnd)) {
		EndPaint(hwnd, &ps);
		return;
		}
	InitStdBitmaps();
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
	CopyRect(&rect2, &lpitem->rect);
	if ((lpitem->item_flags & MF_SEPARATOR) == MF_SEPARATOR) {
		hOldPen = SelectObject(hDC, GetStockObject(BLACK_PEN));
		MoveTo(hDC, rect2.left, rect2.top + 1);
		LineTo(hDC, rect2.right, rect2.top + 1);
		SelectObject(hDC, hOldPen);
		}
	if ((lpitem->item_flags & MF_CHECKED) == MF_CHECKED) {
		hMemDC = CreateCompatibleDC(hDC);
		if (lpitem->hCheckBit == 0) {
			SelectObject(hMemDC, hStdCheck);
			GetObject(hStdCheck, sizeof(BITMAP), (LPSTR)&bm);
			}
		else {
			SelectObject(hMemDC, lpitem->hCheckBit);
			GetObject(lpitem->hCheckBit, sizeof(BITMAP), (LPSTR)&bm);
			}
		BitBlt(hDC, rect2.left, rect2.top + 1,
		bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
		DeleteDC(hMemDC);
		}
	else {
		if (lpitem->hUnCheckBit != 0) {
			hMemDC = CreateCompatibleDC(hDC);
			SelectObject(hMemDC, lpitem->hUnCheckBit);
			GetObject(lpitem->hUnCheckBit, sizeof(BITMAP), (LPSTR)&bm);
			BitBlt(hDC, rect2.left, rect2.top + 1,
			bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
			DeleteDC(hMemDC);
			}
		}
	if ((lpitem->item_flags & MF_BITMAP) == MF_BITMAP) {
		hBitMap = (HBITMAP)LOWORD((LONG)lpitem->item_text);
		rect2.left += lppop->CheckWidth;
		hMemDC = CreateCompatibleDC(hDC);
		SelectObject(hMemDC, hBitMap);
		GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&bm);
#ifdef DEBUG_MENU
		printf("StdDrawPopupMenu // MF_BITMAP hBit=%04X w=%d h=%d\n",
				hBitMap, bm.bmWidth, bm.bmHeight);
#endif
		BitBlt(hDC, rect2.left, rect2.top,
				bm.bmWidth, bm.bmHeight, 
				hMemDC, 0, 0, SRCCOPY);
		DeleteDC(hMemDC);
		if ((lpitem->item_flags & MF_HILITE) == MF_HILITE)
			InvertRect(hDC, &lpitem->rect);
		}
	if (((lpitem->item_flags & MF_BITMAP) != MF_BITMAP) &&
		((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
		((lpitem->item_flags & MF_MENUBREAK) != MF_MENUBREAK)) {
		hOldFont = SelectObject(hDC, GetStockObject(SYSTEM_FONT));
		OldBkMode = SetBkMode(hDC, TRANSPARENT);
		if ((lpitem->item_flags & MF_DISABLED) == MF_DISABLED)
			OldTextColor = SetTextColor(hDC, 0x00C0C0C0L);
		else {
			if ((lpitem->item_flags & MF_HILITE) == MF_HILITE)
				OldTextColor = SetTextColor(hDC, 0x00FFFFFFL);
			else
				OldTextColor = SetTextColor(hDC, 0x00000000L);
			}
		CopyRect(&rect3, &lpitem->rect);
		if ((lpitem->item_flags & MF_HILITE) == MF_HILITE)
			FillRect(hDC, &rect3, GetStockObject(BLACK_BRUSH));
		InflateRect(&rect3, 0, -2);
		rect3.left += lppop->CheckWidth;
		hOldPen = SelectObject(hDC, GetStockObject(BLACK_PEN));
		if ((x = GetShortCutPos(lpitem->item_text)) != (WORD)-1) {
			DrawText(hDC, lpitem->item_text, x, &rect3, 
				DT_LEFT | DT_VCENTER | DT_SINGLELINE);
			DrawText(hDC, &lpitem->item_text[x], -1, &rect3, 
				DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
			} 
	    else
			DrawText(hDC, lpitem->item_text, -1, &rect3, 
				DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		SelectObject(hDC, hOldPen);
		SetTextColor(hDC, OldTextColor);
		SetBkMode(hDC, OldBkMode);
		SelectObject(hDC, hOldFont);
		CopyRect(&rect2, &lpitem->rect);
		}
	if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) {
		CopyRect(&rect3, &lpitem->rect);
		rect3.left = rect3.right - lppop->PopWidth;
		hMemDC = CreateCompatibleDC(hDC);
		SelectObject(hMemDC, hStdMnArrow);
		GetObject(hStdMnArrow, sizeof(BITMAP), (LPSTR)&bm);
		BitBlt(hDC, rect3.left, rect3.top + 1,
		bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
		DeleteDC(hMemDC);
		}
	if (lpitem->next == NULL) goto EndOfPaint;
	lpitem = (LPMENUITEM)lpitem->next;
	}
EndOfPaint:
	EndPaint( hwnd, &ps );
}



void StdDrawMenuBar(HDC hDC, LPRECT lprect, LPPOPUPMENU lppop, 
		    BOOL suppress_draw)
{
	LPMENUITEM 	lpitem;
	HBRUSH 	hBrush;
	HPEN	hOldPen;
	HDC 	hMemDC;
	RECT 	rect, rect2, rect3;
	HFONT	hOldFont;
	DWORD	OldTextColor;
	int		OldBkMode;
	HBITMAP	hBitMap;
	BITMAP	bm;
	UINT  	i, textwidth;
	if (lppop == NULL || lprect == NULL) return;
#ifdef DEBUG_MENU
	printf("StdDrawMenuBar(%04X, %08X, %08X); !\n", hDC, lprect, lppop);
#endif
	MenuBarCalcSize(hDC, lprect, lppop);
	if (suppress_draw) return;
	hOldFont = SelectObject(hDC, GetStockObject(SYSTEM_FONT));
	hOldPen = SelectObject(hDC, GetStockObject(BLACK_PEN));
	hBrush = GetStockObject(WHITE_BRUSH);
	CopyRect(&rect, lprect);
	FillRect(hDC, &rect, hBrush);
	FrameRect(hDC, &rect, GetStockObject(BLACK_BRUSH));
	if (lppop->nItems == 0) goto EndOfPaint;
	lpitem = lppop->firstItem;
	if (lpitem == NULL) goto EndOfPaint;
	for(i = 0; i < lppop->nItems; i++) {
		CopyRect(&rect2, &lpitem->rect);
		if ((lpitem->item_flags & MF_CHECKED) == MF_CHECKED) {
			hMemDC = CreateCompatibleDC(hDC);
			if (lpitem->hCheckBit == 0) {
				SelectObject(hMemDC, hStdCheck);
				GetObject(hStdCheck, sizeof(BITMAP), (LPSTR)&bm);
				}
			else {
				SelectObject(hMemDC, lpitem->hCheckBit);
				GetObject(lpitem->hCheckBit, sizeof(BITMAP), (LPSTR)&bm);
				}
			BitBlt(hDC, rect2.left, rect2.top + 1,
			bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
			DeleteDC(hMemDC);
			}
		else {
			if (lpitem->hUnCheckBit != 0) {
				hMemDC = CreateCompatibleDC(hDC);
				SelectObject(hMemDC, lpitem->hUnCheckBit);
				GetObject(lpitem->hUnCheckBit, sizeof(BITMAP), (LPSTR)&bm);
				BitBlt(hDC, rect2.left, rect2.top + 1,
					bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
				DeleteDC(hMemDC);
				}
			}
		if ((lpitem->item_flags & MF_BITMAP) == MF_BITMAP) {
			hBitMap = (HBITMAP)LOWORD((LONG)lpitem->item_text);
			hMemDC = CreateCompatibleDC(hDC);
			SelectObject(hMemDC, hBitMap);
			GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&bm);
#ifdef DEBUG_MENU
			printf("StdDrawMenuBar // MF_BITMAP hBit=%04X w=%d h=%d\n",
					hBitMap, bm.bmWidth, bm.bmHeight);
#endif
			BitBlt(hDC, rect2.left, rect2.top,
						bm.bmWidth, bm.bmHeight, 
						hMemDC, 0, 0, SRCCOPY);
			DeleteDC(hMemDC);
			}
		if (((lpitem->item_flags & MF_BITMAP) != MF_BITMAP) &&
			((lpitem->item_flags & MF_SEPARATOR) != MF_SEPARATOR) &&
			((lpitem->item_flags & MF_MENUBREAK) != MF_MENUBREAK)) {
			hOldFont = SelectObject(hDC, GetStockObject(SYSTEM_FONT));
			OldBkMode = SetBkMode(hDC, TRANSPARENT);
			if ((lpitem->item_flags & MF_DISABLED) == MF_DISABLED)
				OldTextColor = SetTextColor(hDC, 0x00C0C0C0L);
			else {
				if ((lpitem->item_flags & MF_HILITE) == MF_HILITE)
					OldTextColor = SetTextColor(hDC, 0x00FFFFFFL);
				else
					OldTextColor = SetTextColor(hDC, 0x00000000L);
				}
			if ((lpitem->item_flags & MF_HILITE) == MF_HILITE)
				FillRect(hDC, &rect2, GetStockObject(BLACK_BRUSH));
			DrawText(hDC, lpitem->item_text, -1, &rect2, 
				DT_LEFT | DT_VCENTER | DT_SINGLELINE);
			SetTextColor(hDC, OldTextColor);
			SetBkMode(hDC, OldBkMode);
			SelectObject(hDC, hOldFont);
			}
		if (lpitem->next == NULL) goto EndOfPaint;
		lpitem = (LPMENUITEM)lpitem->next;
		}
EndOfPaint:
	SelectObject(hDC, hOldPen);
	SelectObject(hDC, hOldFont);
} 



LPMENUITEM MenuFindItem(LPPOPUPMENU lppop, int x, int y, WORD *lpRet)
{
	LPMENUITEM 	lpitem;
	UINT  	i;
	if (lpRet != NULL) *lpRet = 0;
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


LPMENUITEM MenuFindItemBySelKey(LPPOPUPMENU lppop, WORD key, WORD *lpRet)
{
	LPMENUITEM 	lpitem;
	UINT  	i;
	if (lppop == NULL) return NULL;

	if (lppop->nItems == 0) return NULL;
	lpitem = lppop->firstItem;
	for(i = 0; i < lppop->nItems; i++) {
		if (lpitem == NULL) return NULL;
#ifdef DEBUG_MENUFINDITEM
		printf("FindItemBySelKey // key=%04X lpitem->sel_key=%04X\n",
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
	InitStdBitmaps();
	lppop = PopupMenuGetWindowAndStorage(hwnd, &wndPtr);
	if (lppop == NULL) return;
	if (lppop->nItems == 0) return;
	hDC = GetDC(hwnd);
	lppop->Width = 20;
	lppop->CheckWidth = lppop->PopWidth = 0;
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
		if ((lpitem->item_flags & MF_CHECKED) == MF_CHECKED) {
			if (lpitem->hCheckBit != 0)
				GetObject(lpitem->hCheckBit, sizeof(BITMAP), (LPSTR)&bm);
			else
				GetObject(hStdCheck, sizeof(BITMAP), (LPSTR)&bm);
			lppop->CheckWidth = max(lppop->CheckWidth, bm.bmWidth);
			}
		else {
			if (lpitem->hUnCheckBit != 0) {
				GetObject(lpitem->hUnCheckBit, sizeof(BITMAP), (LPSTR)&bm);
				lppop->CheckWidth = max(lppop->CheckWidth, bm.bmWidth);
				}
			}
		if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) {
			GetObject(hStdMnArrow, sizeof(BITMAP), (LPSTR)&bm);
			lppop->PopWidth = max(lppop->PopWidth, bm.bmWidth);
			}
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
			TempWidth += lppop->CheckWidth;
			TempWidth += lppop->PopWidth;
			lppop->Width = max(lppop->Width, TempWidth);
			}
		CopyRect(&lpitem->rect, &rect);
		rect.top = rect.bottom;
		lpitem = (LPMENUITEM)lpitem->next;
		}
	if (OldWidth < lppop->Width) goto CalcAGAIN;
	lppop->Height = rect.bottom;
	SetRect(&lppop->rect, 1, 1, lppop->Width, lppop->Height);
#ifdef DEBUG_MENUCALC
	printf("PopupMenuCalcSize w=%d h=%d !\n", lppop->Width, lppop->Height);
#endif
	SelectObject(hDC, hOldFont);
	ReleaseDC(hwnd, hDC);
}



void MenuBarCalcSize(HDC hDC, LPRECT lprect, LPPOPUPMENU lppop)
{
	LPMENUITEM 	lpitem;
	RECT 	rect;
	HBITMAP	hBitMap;
	BITMAP	bm;
	HFONT	hOldFont;
	UINT  	i, OldHeight;
	DWORD	dwRet;
	if (lppop == NULL) return;
	if (lppop->nItems == 0) return;
	InitStdBitmaps();
#ifdef DEBUG_MENUCALC
	printf("MenuBarCalcSize left=%d top=%d right=%d bottom=%d !\n", 
		lprect->left, lprect->top, lprect->right, lprect->bottom);
#endif
	hOldFont = SelectObject(hDC, GetStockObject(SYSTEM_FONT));
	lppop->Height = lprect->bottom - lprect->top;
CalcAGAIN:
	OldHeight = lppop->Height;
	SetRect(&rect, lprect->left, lprect->top, 0, lprect->top + OldHeight);
	lpitem = lppop->firstItem;
	for(i = 0; i < lppop->nItems; i++) {
		if (lpitem == NULL) break;
		rect.bottom = lprect->top + lppop->Height;
		if (rect.right > lprect->right) 
			SetRect(&rect, lprect->left, rect.bottom, 
				0, rect.bottom + SYSMETRICS_CYMENU);
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
			dwRet = max(SYSMETRICS_CYMENU, (HIWORD(dwRet) + 6));
			lppop->Height = max(lppop->Height, (WORD)dwRet);
			}
		CopyRect(&lpitem->rect, &rect);
		rect.left = rect.right;
		lpitem = (LPMENUITEM)lpitem->next;
		}
	if (OldHeight < lppop->Height) goto CalcAGAIN;
	lppop->Width = rect.right;
	lprect->bottom =  lprect->top + lppop->Height;
	CopyRect(&lppop->rect, lprect);
#ifdef DEBUG_MENUCALC
	printf("MenuBarCalcSize w=%d h=%d !\n", lppop->Width, lppop->Height);
#endif
	SelectObject(hDC, hOldFont);
}



/***********************************************************************
 *           FindMenuItem
 */
LPMENUITEM FindMenuItem(HMENU hMenu, WORD nPos, WORD wFlags)
{
	LPPOPUPMENU	menu;
	LPMENUITEM 	lpitem;
	int		i;
	menu = (LPPOPUPMENU) GlobalLock(hMenu);
	if (menu == NULL) {
		GlobalUnlock(hMenu);
		return FALSE;
		}
	lpitem = menu->firstItem;
	if (wFlags & MF_BYPOSITION) {
		for (i = 0; i < nPos; i++, lpitem = lpitem->next) 
			if (lpitem == NULL)	return NULL;
		}
	else {
		for (i = 0; i < menu->nItems && lpitem != NULL; i++) {
			if (lpitem->item_id == nPos) return lpitem;
			lpitem = lpitem->next;
			}
		return NULL;
		}
    return lpitem;
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
		if (str[i] == '&' && str[i + 1] != '&') {
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
		if (str[i] == '\t' && str[i + 1] != '\t') {
			str2 = &str[i + 1];
#ifdef DEBUG_MENUSHORTCUT
			printf("GetShortCutString // '%s' \n", str2);
#endif
			return str2;
			}
		}
#ifdef DEBUG_MENUSHORTCUT
	printf("GetShortCutString NULL \n");
#endif
	return NULL;
}



WORD GetShortCutPos(LPSTR str)
{
	int		i;
	for (i = 0; i < strlen(str); i++) {
		if (str[i] == '\t' && str[i + 1] != '\t') {
#ifdef DEBUG_MENUSHORTCUT
			printf("GetShortCutPos = %d \n", i);
#endif
			return i;
			}
		}
#ifdef DEBUG_MENUSHORTCUT
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
				GlobalUnlock((HMENU)lpitem->item_id);
		    	}
	   	    }
	   	lpitem = (LPMENUITEM)lpitem->next;
	   	}
	return someClosed;
}




/**********************************************************************
 *			ChangeMenu		[USER.153]
 */
BOOL ChangeMenu(HMENU hMenu, WORD nPos, LPSTR lpNewItem, 
			WORD wItemID, WORD wFlags)
{
	if (wFlags & MF_APPEND)
		return AppendMenu(hMenu, wFlags, wItemID, lpNewItem);
	if (wFlags & MF_DELETE)
		return DeleteMenu(hMenu, wItemID, wFlags);
	if (wFlags & MF_INSERT) 
		return InsertMenu(hMenu, nPos, wFlags, wItemID, lpNewItem);
	if (wFlags & MF_CHANGE) 
		return ModifyMenu(hMenu, nPos, wFlags, wItemID, lpNewItem);
	if (wFlags & MF_REMOVE) 
		return RemoveMenu(hMenu, wItemID, wFlags);
	return FALSE;
}


/**********************************************************************
 *			CheckMenuItem		[USER.154]
 */
BOOL CheckMenuItem(HMENU hMenu, WORD wItemID, WORD wFlags)
{
	LPMENUITEM 	lpitem;
#ifdef DEBUG_MENU
	printf("CheckMenuItem (%04X, %04X, %04X) !\n", hMenu, wItemID, wFlags);
#endif
	lpitem = FindMenuItem(hMenu, wItemID, wFlags);
	if (lpitem != NULL) {
		if ((wFlags & MF_CHECKED) == MF_CHECKED)
			lpitem->item_flags |= MF_CHECKED;
		else
			lpitem->item_flags &= ((WORD)-1 ^ MF_CHECKED);
#ifdef DEBUG_MENU
		printf("CheckMenuItem // Found !\n");
#endif
		return(TRUE);
		}
	return FALSE;
}


/**********************************************************************
 *			EnableMenuItem		[USER.155]
 */
BOOL EnableMenuItem(HMENU hMenu, WORD wItemID, WORD wFlags)
{
	LPMENUITEM 	lpitem;
#ifdef DEBUG_MENU
	printf("EnableMenuItem (%04X, %04X, %04X) !\n", hMenu, wItemID, wFlags);
#endif
	lpitem = FindMenuItem(hMenu, wItemID, wFlags);
	if (lpitem != NULL) {
		if (wFlags && MF_DISABLED)
			lpitem->item_flags |= MF_DISABLED;
		else
			lpitem->item_flags &= ((WORD)-1 ^ MF_DISABLED);
#ifdef DEBUG_MENU
		printf("EnableMenuItem // Found !\n");
#endif
		return(TRUE);
		}
	return FALSE;
}


/**********************************************************************
 *			GetMenuString		[USER.161]
 */
int GetMenuString(HMENU hMenu, WORD wItemID, 
	LPSTR str, short nMaxSiz, WORD wFlags)
{
	LPMENUITEM 	lpitem;
	int		maxsiz;
#ifdef DEBUG_MENU
	printf("GetMenuString(%04X, %04X, %08X, %d, %04X);\n",
					hMenu, wItemID, str, nMaxSiz, wFlags);
#endif
	if (str == NULL) return FALSE;
	lpitem = FindMenuItem(hMenu, wItemID, wFlags);
	if (lpitem != NULL) {
		if (lpitem->item_text != NULL) {
			maxsiz = min(nMaxSiz - 1, strlen(lpitem->item_text));
			strncpy(str, lpitem->item_text, maxsiz + 1);
			}
		else
			maxsiz = 0;
#ifdef DEBUG_MENU
		printf("GetMenuString // Found !\n");
#endif
		return maxsiz;
		}
	return 0;
}


/**********************************************************************
 *			HiliteMenuItem		[USER.162]
 */
BOOL HiliteMenuItem(HWND hWnd, HMENU hMenu, WORD wItemID, WORD wHilite)
{
	LPPOPUPMENU	menu;
	LPMENUITEM 	lpitem;
	printf("HiliteMenuItem(%04X, %04X, %04X, %04X);\n", 
						hWnd, hMenu, wItemID, wHilite);
	menu = (LPPOPUPMENU) GlobalLock(hMenu);
	if (menu == NULL) return FALSE;
	lpitem = FindMenuItem(hMenu, wItemID, wHilite);
	if (lpitem == NULL) return FALSE;
	return FALSE;
}


/**********************************************************************
 *			GetMenuItemCount		[USER.263]
 */
WORD GetMenuItemCount(HMENU hMenu)
{
	LPPOPUPMENU	menu;
#ifdef DEBUG_MENU
	printf("GetMenuItemCount(%04X);\n", hMenu);
#endif
	menu = (LPPOPUPMENU) GlobalLock(hMenu);
	if (menu == NULL) return (WORD)-1;
#ifdef DEBUG_MENU
	printf("GetMenuItemCount(%04X) return %d \n", hMenu, menu->nItems);
#endif
	return menu->nItems;
}


/**********************************************************************
 *			GetMenuItemID			[USER.264]
 */
WORD GetMenuItemID(HMENU hMenu, int nPos)
{
	WORD		i;
	LPPOPUPMENU	menu;
	LPMENUITEM 	lpitem;
	printf("GetMenuItemID(%04X, %d);\n", hMenu, nPos);
	menu = (LPPOPUPMENU) GlobalLock(hMenu);
	if (menu == NULL) return -1;
	lpitem = menu->firstItem;
	for (i = 0; i < menu->nItems; i++) {
		if (lpitem == NULL) break;
		if (i == nPos) {
#ifdef DEBUG_MENU
			printf("GetMenuItemID // Found !\n");
#endif
			return lpitem->item_id;
			}
		lpitem = (LPMENUITEM)lpitem->next;
		}
	return -1;
}


/**********************************************************************
 *			InsertMenu		[USER.410]
 */
BOOL InsertMenu(HMENU hMenu, WORD nPos, WORD wFlags, WORD wItemID, LPSTR lpNewItem)
{
	LPPOPUPMENU	menu;
	HANDLE	hNewItem;
	LPMENUITEM 	lpitem, lpitem2;
#ifdef DEBUG_MENU
	if ((wFlags & (MF_BITMAP | MF_SEPARATOR | MF_MENUBREAK | MF_OWNERDRAW)) == 0)
		printf("InsertMenu (%04X, %04X, %04X, '%s') !\n",
					hMenu, wFlags, wItemID, lpNewItem);
	else
		printf("InsertMenu (%04X, %04X, %04X, %04X, %08X) !\n",
					hMenu, nPos, wFlags, wItemID, lpNewItem);
#endif
	menu = (LPPOPUPMENU) GlobalLock(hMenu);
	if (menu == NULL) return FALSE;
	lpitem = FindMenuItem(hMenu, nPos, wFlags);
	if (lpitem == NULL) lpitem = menu->firstItem;
	hNewItem = GlobalAlloc(GMEM_MOVEABLE, sizeof(MENUITEM));
	if (hNewItem == 0) {
		GlobalUnlock(hMenu);
		return FALSE;
		}
	lpitem2 = (LPMENUITEM)GlobalLock(hNewItem);
	if (lpitem2 == NULL) {
		GlobalFree(hNewItem);
		GlobalUnlock(hMenu);
		return FALSE;
		}
	lpitem2->hItem = hNewItem;
	lpitem2->item_flags = wFlags;
	lpitem2->item_id = wItemID;
	if (!(wFlags & (MF_BITMAP | MF_OWNERDRAW | MF_MENUBARBREAK | 
		MF_MENUBREAK | MF_SEPARATOR))) {
		lpitem2->hText = GlobalAlloc(GMEM_MOVEABLE, strlen(lpNewItem) + 1);
		lpitem2->item_text = GlobalLock(lpitem2->hText);
		if (lpitem2->item_text != NULL)
			strcpy(lpitem2->item_text, lpNewItem);
		else {
			printf("InsertMenu // Bad Alloc !\n");
			return FALSE;
			}
		lpitem2->sel_key = GetSelectionKey(lpitem2->item_text);
		}
	else {
		lpitem2->item_text = lpNewItem;
		}
	if (lpitem == NULL) {
		menu->firstItem = lpitem2;
		lpitem2->prev = NULL;
		lpitem2->next = NULL;
		}
	else {
		lpitem2->prev = lpitem;
		lpitem2->next = lpitem->next;
		if (lpitem2->next != NULL) lpitem2->next->prev = lpitem2;
		lpitem->next = lpitem2;
		}
	menu->nItems++;
	GlobalUnlock(hMenu);
	return TRUE;
}


/**********************************************************************
 *			AppendMenu		[USER.411]
 */
BOOL AppendMenu(HMENU hMenu, WORD wFlags, WORD wItemID, LPSTR lpNewItem)
{
	LPPOPUPMENU	menu;
	HANDLE	hNewItem;
	LPMENUITEM 	lpitem, lpitem2;
#ifdef DEBUG_MENU
	if ((wFlags & (MF_BITMAP | MF_SEPARATOR | MF_MENUBREAK | MF_OWNERDRAW)) == 0)
		printf("AppendMenu (%04X, %04X, %04X, '%s') !\n",
					hMenu, wFlags, wItemID, lpNewItem);
	else
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
	if (hNewItem == 0) {
		GlobalUnlock(hMenu);
		return FALSE;
		}
	lpitem2 = (LPMENUITEM)GlobalLock(hNewItem);
	lpitem2->hItem = hNewItem;
	if (lpitem2 == NULL) {
		GlobalFree(hNewItem);
		GlobalUnlock(hMenu);
		return FALSE;
		}
	lpitem2->item_flags = wFlags;
	lpitem2->item_id = wItemID;
	if (!(wFlags & (MF_BITMAP | MF_OWNERDRAW | MF_MENUBARBREAK | 
		MF_MENUBREAK | MF_SEPARATOR))) {
		lpitem2->hText = GlobalAlloc(GMEM_MOVEABLE, strlen(lpNewItem) + 1);
		lpitem2->item_text = GlobalLock(lpitem2->hText);
		if (lpitem2->item_text != NULL)
			strcpy(lpitem2->item_text, lpNewItem);
		else {
			printf("AppendMenu // Bad Alloc !\n");
			return FALSE;
			}
		lpitem2->sel_key = GetSelectionKey(lpitem2->item_text);
		}
	else {
		lpitem2->item_text = lpNewItem;
		}
	if (lpitem == NULL)
		menu->firstItem = lpitem2;
	else
		lpitem->next = lpitem2;
	lpitem2->prev = lpitem;
	lpitem2->next = NULL;
	lpitem2->hCheckBit = (HBITMAP)NULL;
	lpitem2->hUnCheckBit = (HBITMAP)NULL;
	menu->nItems++;
	GlobalUnlock(hMenu);
	return TRUE;
}


/**********************************************************************
 *			RemoveMenu		[USER.412]
 */
BOOL RemoveMenu(HMENU hMenu, WORD nPos, WORD wFlags)
{
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
			if (!(lpitem->item_flags & 
				(MF_BITMAP | MF_OWNERDRAW | MF_MENUBARBREAK | 
				MF_MENUBREAK | MF_SEPARATOR))) {
				GlobalUnlock(lpitem->hText);
				GlobalFree(lpitem->hText);
				}
			GlobalFree(lpitem->hItem);
			GlobalUnlock(hMenu);
			return TRUE;
			}
		lpitem = (LPMENUITEM)lpitem->next;
		printf("RemoveMenu // during loop items !\n");
		}
	printf("RemoveMenu // after loop items !\n");
	GlobalUnlock(hMenu);
	return FALSE;
}


/**********************************************************************
 *			DeleteMenu		[USER.413]
 */
BOOL DeleteMenu(HMENU hMenu, WORD nPos, WORD wFlags)
{
	LPPOPUPMENU	menu;
	LPMENUITEM 	lpitem;
	int		i;
#ifdef DEBUG_MENU
	printf("DeleteMenu (%04X, %04X, %04X) !\n", hMenu, nPos, wFlags);
#endif
	menu = (LPPOPUPMENU) GlobalLock(hMenu);
	if (menu == NULL) {
		GlobalUnlock(hMenu);
		return FALSE;
		}
	lpitem = FindMenuItem(hMenu, nPos, wFlags);
	if (lpitem != NULL) {
		if ((lpitem->item_flags & MF_POPUP) == MF_POPUP) 
			DestroyMenu((HMENU)lpitem->item_id);
		if (!(lpitem->item_flags & 
			(MF_BITMAP | MF_OWNERDRAW | MF_MENUBARBREAK | 
			MF_MENUBREAK | MF_SEPARATOR))) {
			GlobalUnlock(lpitem->hText);
			GlobalFree(lpitem->hText);
			}
		if (lpitem->prev) lpitem->prev->next = lpitem->next;
		if (lpitem->next) lpitem->next->prev = lpitem->prev;
		GlobalFree(lpitem->hItem);
		GlobalUnlock(hMenu);
		return TRUE;
		}
	GlobalUnlock(hMenu);
	return FALSE;
}


/**********************************************************************
 *			ModifyMenu		[USER.414]
 */
BOOL ModifyMenu(HMENU hMenu, WORD nPos, WORD wFlags, WORD wItemID, LPSTR lpNewItem)
{
	LPPOPUPMENU	menu;
	LPMENUITEM 	lpitem;
	int		i;
#ifdef DEBUG_MENU
	if ((wFlags & (MF_BITMAP | MF_SEPARATOR | MF_MENUBREAK | MF_OWNERDRAW)) == 0)
		printf("ModifyMenu (%04X, %04X, %04X, %04X, '%s') !\n",
					hMenu, nPos, wFlags, wItemID, lpNewItem);
	else
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
			if (!(lpitem->item_flags & 
				(MF_BITMAP | MF_OWNERDRAW | MF_MENUBARBREAK | 
				MF_MENUBREAK | MF_SEPARATOR))) {
				GlobalUnlock(lpitem->hText);
				GlobalFree(lpitem->hText);
				lpitem->hText = GlobalAlloc(GMEM_MOVEABLE, strlen(lpNewItem) + 1);
				lpitem->item_text = GlobalLock(lpitem->hText);
				printf("ModifyMenu %08X %08X '%s') !\n",
					lpitem->item_text, lpNewItem, lpNewItem);
				if (lpitem->item_text != NULL)
					strcpy(lpitem->item_text, lpNewItem);
				else
					printf("ModifyMenu // Bad Alloc !\n");
				}
			else
				lpitem->item_text = lpNewItem;
				GlobalUnlock(hMenu);
				return(TRUE);
				}
		lpitem = (LPMENUITEM)lpitem->next;
		}
    GlobalUnlock(hMenu);
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
	menu->nItems 		= 0;
	menu->firstItem 	= NULL;
	menu->ownerWnd		= 0;
	menu->hWndPrev		= 0;
	menu->hWnd			= 0;
	menu->hWndParent	= 0;
	menu->MouseFlags	= 0;
	menu->BarFlag		= FALSE;
	menu->SysFlag		= FALSE;
	menu->ChildFlag		= TRUE;
	menu->Width 		= 100;
	menu->Height 		= 0;
	GlobalUnlock(hMenu);
#ifdef DEBUG_MENU
	printf("CreatePopupMenu // return %04X\n", hMenu);
#endif
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
	RECT	rect;
#ifdef DEBUG_MENU
	printf("TrackPopupMenu (%04X, %04X, %d, %d, %04X, %04X, %08X) !\n",
		hMenu, wFlags, x, y, nReserved, hWnd, lpRect);
#endif
	lppop = (LPPOPUPMENU) GlobalLock(hMenu);
	if (lppop == NULL) {
		printf("TrackPopupMenu // Bad menu handle %04X !\n", hMenu);
		return FALSE;
		}
	wndPtr = WIN_FindWndPtr(hWnd);
	if (wndPtr == NULL) {
		printf("TrackPopupMenu // Bad window handle %04X !\n", hWnd);
		return FALSE;
		}
	lppop->ownerWnd = hWnd;
	lppop->hWndPrev = GetFocus();
	if (lppop->hWnd == (HWND)NULL) {
		lppop->hWnd = CreateWindow("POPUPMENU", "", WS_POPUP | WS_VISIBLE,
			x, y, lppop->Width, lppop->Height, (HWND)NULL, 0, 
			wndPtr->hInstance, (LPSTR)lppop);
		if (lppop->hWnd == 0) {
			printf("TrackPopupMenu // Can't create PopupMenu window !\n");
			return FALSE;
			}
		}
	else {
		ShowWindow(lppop->hWnd, SW_SHOW);
		}
	if (!lppop->BarFlag) {
		PopupMenuCalcSize(lppop->hWnd);
#ifdef DEBUG_MENU
		printf("TrackPopupMenu // x=%d y=%d Width=%d Height=%d\n", 
			x, y, lppop->Width, lppop->Height); 
#endif
		SetWindowPos(lppop->hWnd, 0, x, y, lppop->Width + 2, lppop->Height, 
			SWP_NOZORDER);
		}
	SetFocus(lppop->hWnd);
	if (!MenuHasFocus) {
#ifdef DEBUG_MENU
		printf("TrackPopupMenu // before MenuFocusLoop !\n");
#endif
		MenuFocusLoop(hWnd, NULL);
#ifdef DEBUG_MENU
		printf("TrackPopupMenu // after MenuFocusLoop !\n");
#endif
		}
	GlobalUnlock(hMenu);
	return TRUE;
}


BOOL ActivateMenuBarFocus(HWND hWnd)
{
	WND		*wndPtr;
	LPPOPUPMENU	lpmenu;
	BOOL	bRet;
	MSG		msg;
	if (MenuHasFocus) return FALSE;
	wndPtr = WIN_FindWndPtr(hWnd);
	if (wndPtr == NULL) return FALSE;
#ifdef DEBUG_MENU
	printf("ActivateMenuBarFocus (%04X) !\n", hWnd);
#endif
	while((wndPtr->dwStyle & WS_CHILD) == WS_CHILD) {
		hWnd = GetParent(hWnd);
		printf("ActivateMenuBarFocus // next Parent=%04X !\n", hWnd);
		wndPtr = WIN_FindWndPtr(hWnd);
		if (wndPtr == NULL) return FALSE;
		}
	if ((wndPtr->dwStyle & WS_CHILD) == 0 && wndPtr->wIDmenu != 0) {
		lpmenu = (LPPOPUPMENU) GlobalLock(wndPtr->wIDmenu);
		if (lpmenu == NULL) return FALSE;
		lpmenu->hWndPrev = GetFocus();
		SetFocus(hWnd);
		MenuItemSelect(hWnd, lpmenu, 0);
		bRet = MenuFocusLoop(hWnd, lpmenu);
		GlobalUnlock(wndPtr->wIDmenu);
		return bRet;
		}
	return FALSE;
}


BOOL MenuFocusLoop(HWND hWnd, LPPOPUPMENU lpmenu)
{
	MSG		msg;
#ifdef DEBUG_MENU
	printf("Enter in Menu Focus Loop !\n");
#endif
	MenuHasFocus = TRUE;
	while(TRUE) {
		if (!MenuHasFocus) break;
		if (!GetMessage(&msg, (HWND)NULL, 0, 0)) break;
		TranslateMessage(&msg);
		if (hWnd == msg.hwnd && lpmenu != NULL) {
			if ((msg.message == WM_SYSKEYDOWN && msg.wParam == VK_MENU) ||
				(msg.message == WM_CHAR && msg.wParam == VK_ESCAPE)) {
				HideAllSubPopupMenu(lpmenu);
				break;
				}
			ScreenToClient(hWnd, &msg.pt);
			msg.pt.y += lpmenu->rect.bottom;
			switch(msg.message) {
				case WM_LBUTTONDOWN:
				case WM_NCLBUTTONDOWN:
				    SetCapture(hWnd);
				    MenuButtonDown(hWnd, lpmenu, msg.pt.x, msg.pt.y);
				    break;
				case WM_LBUTTONUP:
				case WM_NCLBUTTONUP:
				    MenuButtonUp(hWnd, lpmenu, msg.pt.x, msg.pt.y);
				    ReleaseCapture();
				    break;
				case WM_MOUSEMOVE:
				case WM_NCMOUSEMOVE:
				    MenuMouseMove(hWnd, lpmenu, msg.wParam, msg.pt.x, msg.pt.y);
				    break;
				case WM_KEYDOWN:
				case WM_KEYUP:
				case WM_CHAR:
					PopupMenuWndProc(hWnd, msg.message, msg.wParam, msg.lParam);
				default:
					DispatchMessage(&msg);
				}
			}
		else
			DispatchMessage(&msg);
		}
EndOfFocus:
	MenuHasFocus = FALSE;
	if (lpmenu != NULL) MenuItemSelect(hWnd, lpmenu, -1);
#ifdef DEBUG_MENU
	printf("End of Menu Focus Loop !\n");
#endif
	return TRUE;
}


/**********************************************************************
 *			NC_TrackSysMenu		[Internal]
 */
void NC_TrackSysMenu(HWND hWnd)
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
	lpsys->BarFlag = FALSE;
	lpsys->SysFlag = TRUE;
	lpsys->ChildFlag = FALSE;
	lpsys->hWndParent = hWnd;
	if (!IsWindowVisible(lpsys->hWnd)) {
		GetWindowRect(hWnd, &rect);
#ifdef DEBUG_MENU
		printf("NC_TrackSysMenu lpsys->hWnd=%04X !\n", lpsys->hWnd);
#endif
		if (MenuHasFocus) {
			TrackPopupMenu(wndPtr->hSysMenu, TPM_LEFTBUTTON, 
				rect.left, rect.top + SYSMETRICS_CYSIZE, 
				0, hWnd, (LPRECT)NULL);
			}
		else {
			MenuHasFocus = TRUE;
			TrackPopupMenu(wndPtr->hSysMenu, TPM_LEFTBUTTON, 
				rect.left, rect.top + SYSMETRICS_CYSIZE, 
				0, hWnd, (LPRECT)NULL);
			MenuHasFocus = FALSE;
#ifdef DEBUG_MENU
			printf("NC_TrackSysMenu // before MenuFocusLoop !\n");
#endif
			MenuFocusLoop(hWnd, NULL);
#ifdef DEBUG_MENU
			printf("NC_TrackSysMenu // after MenuFocusLoop !\n");
#endif
			}
		}
	else {
		ShowWindow(lpsys->hWnd, SW_HIDE);
		}
	GlobalUnlock(wndPtr->hSysMenu);
}


/**********************************************************************
 *			GetMenuCheckMarkDimensions	[USER.417]
 */
DWORD GetMenuCheckMarkDimensions()
{
    BITMAP	bm;
	InitStdBitmaps();
    GetObject(hStdCheck, sizeof(BITMAP), (LPSTR)&bm);
    return MAKELONG(bm.bmWidth, bm.bmHeight);
}


/**********************************************************************
 *			SetMenuItemBitmaps	[USER.418]
 */
BOOL SetMenuItemBitmaps(HMENU hMenu, WORD nPos, WORD wFlags,
		HBITMAP hNewCheck, HBITMAP hNewUnCheck)
{
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
			GlobalUnlock(hMenu);
			return TRUE;
			}
		lpitem = (LPMENUITEM)lpitem->next;
		}
	GlobalUnlock(hMenu);
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
	printf("CreateMenu !\n");
#endif
	hMenu = GlobalAlloc(GMEM_MOVEABLE, sizeof(POPUPMENU));
	menu = (LPPOPUPMENU) GlobalLock(hMenu);
	if (menu == NULL) {
		GlobalFree(hMenu);
		return 0;
		}
	menu->nItems 		= 0;
	menu->firstItem 	= NULL;
	menu->hWndPrev		= 0;
	menu->ownerWnd		= 0;
	menu->hWnd			= 0;
	menu->hWndParent	= 0;
	menu->MouseFlags	= 0;
	menu->BarFlag		= TRUE;
	menu->SysFlag		= FALSE;
	menu->ChildFlag 	= TRUE;
	menu->Width 		= 100;
	menu->Height 		= 0;
	GlobalUnlock(hMenu);
#ifdef DEBUG_MENU
	printf("CreateMenu // return %04X\n", hMenu);
#endif
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
	GlobalUnlock(hMenu);
	GlobalFree(hMenu);
#ifdef DEBUG_MENU
	printf("DestroyMenu (%04X) // End !\n", hMenu);
#endif
	return TRUE;
}


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
		(menu_desc = (MENU_HEADER *) GlobalLock(hMenu_desc)) == NULL) {
		return 0;
		}
	hMenu = LoadMenuIndirect((LPSTR)menu_desc);
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
 *			SetSystemMenu		[USER.280]
 */
BOOL SetSystemMenu(HWND hWnd, HMENU newHmenu)
{
	WND	*wndPtr;

	if ((wndPtr = WIN_FindWndPtr(hWnd)) != NULL)
		wndPtr->hSysMenu = newHmenu;
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
	RECT	rect;
	LPPOPUPMENU lpmenu;
	NCCALCSIZE_PARAMS *params;
	HANDLE hparams;
	WND * wndPtr = WIN_FindWndPtr(hWnd);
	if (wndPtr == NULL) {
		printf("SetMenu(%04X, %04X) // Bad window handle !\n", hWnd, hMenu);
		return FALSE;
		}
#ifdef DEBUG_MENU
	printf("SetMenu(%04X, %04X);\n", hWnd, hMenu);
#endif
	if (GetCapture() == hWnd) ReleaseCapture();
	wndPtr->wIDmenu = hMenu;
	if (hMenu == 0) {
		printf("SetMenu(%04X, %04X) // Menu removed !\n", hWnd, hMenu);
		hparams = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(NCCALCSIZE_PARAMS));
		if (hparams) {
			params = (NCCALCSIZE_PARAMS *) USER_HEAP_ADDR(hparams);
			params->rgrc[0] = wndPtr->rectWindow;
			params->lppos = NULL;
			SendMessage(hWnd, WM_NCCALCSIZE, FALSE, (LONG)params);
			wndPtr->rectClient = params->rgrc[0];
			USER_HEAP_FREE(hparams);
			}	
		SendMessage(hWnd, WM_NCPAINT, 0, 0L);
		return TRUE;
		}
	lpmenu = (LPPOPUPMENU) GlobalLock(hMenu);
	if (lpmenu == NULL) {
		printf("SetMenu(%04X, %04X) // Bad menu handle !\n", hWnd, hMenu);
		return FALSE;
		}
	lpmenu->ownerWnd = hWnd;
	ResetHiliteFlags(lpmenu);
	GlobalUnlock(hMenu);
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
#ifdef DEBUG_MENU
			printf("    found %04x\n", lpitem->item_id);
#endif	    
			if (lpitem->item_flags & MF_POPUP)
				return lpitem->item_id;
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
	LPPOPUPMENU lppop;
	HDC		hDC;
#ifdef DEBUG_MENU
	printf("DrawMenuBar (%04X)\n", hWnd);
#endif
	wndPtr = WIN_FindWndPtr(hWnd);
	if (wndPtr != NULL && (wndPtr->dwStyle & WS_CHILD) == 0 && 
		wndPtr->wIDmenu != 0) {
#ifdef DEBUG_MENU
		printf("DrawMenuBar wIDmenu=%04X \n", wndPtr->wIDmenu);
#endif
		lppop = (LPPOPUPMENU) GlobalLock(wndPtr->wIDmenu);
		if (lppop == NULL) return;
		if ((lppop->rect.bottom - lppop->rect.top) != 0) {
			hDC = GetWindowDC(hWnd);
			StdDrawMenuBar(hDC, &lppop->rect, lppop, FALSE);
			ReleaseDC(hWnd, hDC);
			}
		else
			SendMessage(hWnd, WM_NCPAINT, 1, 0L);
		GlobalUnlock(wndPtr->wIDmenu);
		}
}


/**********************************************************************
 *			LoadMenuIndirect	[USER.220]
 */
HMENU LoadMenuIndirect(LPSTR menu_template)
{
	HMENU     		hMenu;
	MENU_HEADER 	*menu_desc;
	LPPOPUPMENU lppop;
#ifdef DEBUG_MENU
	printf("LoadMenuIndirect: menu_template '%08X'\n", menu_template);
#endif
	hMenu = CreateMenu();
	menu_desc = (MENU_HEADER *)menu_template;
	ParseMenuResource((WORD *)(menu_desc + 1), 0, hMenu); 
	lppop = (LPPOPUPMENU) GlobalLock(hMenu);
	ResetHiliteFlags(lppop);
	GlobalUnlock(hMenu);
	return hMenu;
}


/**********************************************************************
 *			InitStdBitmaps (Internal)
 */
void InitStdBitmaps()
{
	if (hStdCheck == (HBITMAP)NULL) 
		hStdCheck = LoadBitmap((HANDLE)NULL, (LPSTR)OBM_CHECK);
	if (hStdMnArrow == (HBITMAP)NULL) 
		hStdMnArrow = LoadBitmap((HANDLE)NULL, (LPSTR)OBM_MNARROW);
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
/*		hSysMenu = LoadMenu((HINSTANCE)NULL, MAKEINTRESOURCE(SC_SYSMENU));*/
/*		hSysMenu = LoadMenu((HINSTANCE)NULL, "SYSMENU"); */
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
		sysmenu->BarFlag = FALSE;
		sysmenu->SysFlag = TRUE;
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
#ifdef DEBUG_MENU
	printf("CopySysMenu hMenu=%04X !\n", hMenu);
#endif
	return hMenu;
}


/**********************************************************************
 *			ParseMenuResource (from Resource or Template)
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
		MENUITEMTEMPLATE *normal_item = (MENUITEMTEMPLATE *) item;
		next_item = (WORD *) (normal_item->item_text + 
		strlen(normal_item->item_text) + 1);
		if (strlen(normal_item->item_text) == 0 && normal_item->item_id == 0) 
			normal_item->item_flags |= MF_SEPARATOR;
		AppendMenu(hMenu, normal_item->item_flags, 
			normal_item->item_id, normal_item->item_text);
	    }
	}
    while (!(*item & MF_END));
    return next_item;
}


