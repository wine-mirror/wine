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

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "windows.h"
#include "user.h"
#include "heap.h"
#include "win.h"
#include "listbox.h"
#include "scroll.h"
#include "dirent.h"
#include <sys/stat.h>

#define GMEM_ZEROINIT 0x0040


LPHEADLIST ListBoxGetWindowAndStorage(HWND hwnd, WND **wndPtr);
LPHEADLIST ListBoxGetStorageHeader(HWND hwnd);
void StdDrawListBox(HWND hwnd);
void OwnerDrawListBox(HWND hwnd);
int ListBoxFindMouse(HWND hwnd, int X, int Y);
int CreateListBoxStruct(HWND hwnd);
int ListBoxAddString(HWND hwnd, LPSTR newstr);
int ListBoxInsertString(HWND hwnd, UINT uIndex, LPSTR newstr);
int ListBoxGetText(HWND hwnd, UINT uIndex, LPSTR OutStr);
int ListBoxDeleteString(HWND hwnd, UINT uIndex);
int ListBoxFindString(HWND hwnd, UINT nFirst, LPSTR MatchStr);
int ListBoxResetContent(HWND hwnd);
int ListBoxSetCurSel(HWND hwnd, WORD wIndex);
int ListBoxSetSel(HWND hwnd, WORD wIndex);
int ListBoxDirectory(HWND hwnd, UINT attrib, LPSTR filespec);
int ListBoxGetItemRect(HWND hwnd, WORD wIndex, LPRECT rect);
int ListBoxSetItemHeight(HWND hwnd, WORD wIndex, long height);
int ListBoxDefaultItem(HWND hwnd, WND *wndPtr, 
	LPHEADLIST lphl, LPLISTSTRUCT lpls);
int ListBoxFindNextMatch(HWND hwnd, WORD wChar);



void LISTBOX_CreateListBox(LPSTR className, LPSTR listboxLabel, HWND hwnd)
{
    WND *wndPtr    = WIN_FindWndPtr(hwnd);
    WND *parentPtr = WIN_FindWndPtr(wndPtr->hwndParent);
    DWORD style;
    char widgetName[15];

#ifdef DEBUG_LISTBOX
    printf("listbox: label = %s, x = %d, y = %d\n", listboxLabel,
	   wndPtr->rectClient.left, wndPtr->rectClient.top);
    printf("        width = %d, height = %d\n",
	   wndPtr->rectClient.right - wndPtr->rectClient.left,
	   wndPtr->rectClient.bottom - wndPtr->rectClient.top);
#endif

    if (!wndPtr)
	return;

    style = wndPtr->dwStyle & 0x0000FFFF;
/*
    if ((style & LBS_NOTIFY) == LBS_NOTIFY)
    if ((style & LBS_SORT) == LBS_SORT)
*/    
    sprintf(widgetName, "%s%d", className, wndPtr->wIDmenu);
    wndPtr->winWidget = XtVaCreateManagedWidget(widgetName,
				    compositeWidgetClass,
				    parentPtr->winWidget,
				    XtNx, wndPtr->rectClient.left,
				    XtNy, wndPtr->rectClient.top,
				    XtNwidth, wndPtr->rectClient.right -
				        wndPtr->rectClient.left,
				    XtNheight, wndPtr->rectClient.bottom -
				        wndPtr->rectClient.top,
				    NULL );
    GlobalUnlock(hwnd);
    GlobalUnlock(wndPtr->hwndParent);
}



/***********************************************************************
 *           WIDGETS_ListBoxWndProc
 */
LONG LISTBOX_ListBoxWndProc( HWND hwnd, WORD message,
			   WORD wParam, LONG lParam )
{    
    WND  *wndPtr;
    LPHEADLIST  lphl;
    WORD	wRet;
    RECT	rect;
    int		y;
    int		width, height;
    CREATESTRUCT *createStruct;
    static RECT rectsel;
    switch(message)
    {
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
	width = wndPtr->rectClient.right - wndPtr->rectClient.left;
	height = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
	lphl->hWndScroll = CreateWindow("SCROLLBAR", "",
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBS_VERT,
		width - 17, 0, 16, height, hwnd, 1, wndPtr->hInstance, NULL);
    	ShowWindow(lphl->hWndScroll, SW_HIDE);
	SetScrollRange(lphl->hWndScroll, WM_VSCROLL, 1, lphl->ItemsCount, TRUE);
	return 0;
    case WM_DESTROY:
        lphl = ListBoxGetStorageHeader(hwnd);
        if (lphl == 0) return 0;
	ListBoxResetContent(hwnd);
	DestroyWindow(lphl->hWndScroll);
	free(lphl);
	*((LPHEADLIST *)&wndPtr->wExtra[1]) = 0;
#ifdef DEBUG_LISTBOX
        printf("ListBox WM_DESTROY %lX !\n", lphl);
#endif
	return 0;

    case WM_VSCROLL:
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
	    SetScrollPos(lphl->hWndScroll, WM_VSCROLL, lphl->FirstVisible, TRUE);
	    InvalidateRect(hwnd, NULL, TRUE);
	    UpdateWindow(hwnd);
            }
	return 0;
	
    case WM_LBUTTONDOWN:
/*
	SetFocus(hwnd);
*/
	SetCapture(hwnd);
        lphl = ListBoxGetStorageHeader(hwnd);
        if (lphl == NULL) return 0;
	lphl->PrevSelected = lphl->ItemSelected;
        wRet = ListBoxFindMouse(hwnd, LOWORD(lParam), HIWORD(lParam));
	ListBoxSetCurSel(hwnd, wRet);
	ListBoxGetItemRect(hwnd, wRet, &rectsel);
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
	return 0;
    case WM_LBUTTONUP:
	ReleaseCapture();
	lphl = ListBoxGetWindowAndStorage(hwnd, &wndPtr);
        if (lphl == NULL) return 0;
	if (lphl->PrevSelected != lphl->ItemSelected)
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
    case WM_KEYDOWN:
        printf("ListBox WM_KEYDOWN wParam %X!\n", wParam);
	ListBoxFindNextMatch(hwnd, wParam);
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
    case WM_MOUSEMOVE:
        if ((wParam & MK_LBUTTON) != 0) {
            y = HIWORD(lParam);
	    if (y < 4) {
	        lphl = ListBoxGetStorageHeader(hwnd);
		if (lphl->FirstVisible > 1) {
		    lphl->FirstVisible--;
		    SetScrollPos(lphl->hWndScroll, WM_VSCROLL, lphl->FirstVisible, TRUE);
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
		    SetScrollPos(lphl->hWndScroll, WM_VSCROLL, lphl->FirstVisible, TRUE);
		    InvalidateRect(hwnd, NULL, TRUE);
		    UpdateWindow(hwnd);
		    break;
		    }
		}
	    if ((y > 0) && (y < (rect.bottom - 4))) {
		if ((y < rectsel.top) || (y > rectsel.bottom)) {
		    wRet = ListBoxFindMouse(hwnd, LOWORD(lParam), HIWORD(lParam));
		    ListBoxSetCurSel(hwnd, wRet);
		    ListBoxGetItemRect(hwnd, wRet, &rectsel);
		    InvalidateRect(hwnd, NULL, TRUE);
		    UpdateWindow(hwnd);
		    }
	        
		}
	    }
	break;

    case LB_RESETCONTENT:
        printf("ListBox LB_RESETCONTENT !\n");
	ListBoxResetContent(hwnd);
	return 0;
    case LB_DIR:
        printf("ListBox LB_DIR !\n");
	wRet = ListBoxDirectory(hwnd, wParam, (LPSTR)lParam);
	if (IsWindowVisible(hwnd)) {
	    InvalidateRect(hwnd, NULL, TRUE);
	    UpdateWindow(hwnd);
	    }
	return wRet;
    case LB_ADDSTRING:
	wRet = ListBoxAddString(hwnd, (LPSTR)lParam);
	return wRet;
    case LB_GETTEXT:
	wRet = ListBoxGetText(hwnd, wParam, (LPSTR)lParam);
        printf("ListBox LB_GETTEXT #%u '%s' !\n", wParam, (LPSTR)lParam);
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
        printf("ListBox LB_GETCURSEL %u !\n", lphl->ItemSelected);
	if (lphl->ItemSelected == 0) return LB_ERR;
	return lphl->ItemSelected;
    case LB_GETHORIZONTALEXTENT:
	return wRet;
    case LB_GETITEMDATA:
	return wRet;
    case LB_GETITEMHEIGHT:
	return wRet;
    case LB_GETITEMRECT:
	return wRet;
    case LB_GETSEL:
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
	return wRet;
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
	if (IsWindowVisible(hwnd)) {
	    InvalidateRect(hwnd, NULL, TRUE);
	    UpdateWindow(hwnd);
	    }
	return wRet;
    case LB_SETSEL:
        printf("ListBox LB_SETSEL wParam=%x lParam=%lX !\n", wParam, lParam);
	wRet = ListBoxSetSel(hwnd, wParam);
	if (IsWindowVisible(hwnd)) {
	    InvalidateRect(hwnd, NULL, TRUE);
	    UpdateWindow(hwnd);
	    }
	return wRet;
    case LB_SETTOPINDEX:
        printf("ListBox LB_SETTOPINDEX wParam=%x !\n", wParam);
        lphl = ListBoxGetStorageHeader(hwnd);
	lphl->FirstVisible = wParam;
	SetScrollPos(lphl->hWndScroll, WM_VSCROLL, lphl->FirstVisible, TRUE);
	if (IsWindowVisible(hwnd)) {
	    InvalidateRect(hwnd, NULL, TRUE);
	    UpdateWindow(hwnd);
	    }
	break;
    case LB_SETITEMHEIGHT:
#ifdef DEBUG_LISTBOX
        printf("ListBox LB_SETITEMHEIGHT wParam=%x lParam=%lX !\n", wParam, lParam);
#endif
	wRet = ListBoxSetItemHeight(hwnd, wParam, lParam);
	if (IsWindowVisible(hwnd)) {
	    InvalidateRect(hwnd, NULL, TRUE);
	    UpdateWindow(hwnd);
	    }
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
	LPHEADLIST  lphl;
	LPLISTSTRUCT lpls;
	PAINTSTRUCT ps;
	HBRUSH hBrush;
	HWND	hWndParent;
	HDC hdc;
	RECT rect;
	UINT  i, h, h2;
	char	C[128];
	h = 0;
	hdc = BeginPaint( hwnd, &ps );
    	if (!IsWindowVisible(hwnd)) {
	    EndPaint( hwnd, &ps );
	    return;
	    }
	GetClientRect(hwnd, &rect);
        lphl = ListBoxGetStorageHeader(hwnd);
	if (lphl == NULL) goto EndOfPaint;
	hBrush = SendMessage(lphl->hWndLogicParent, WM_CTLCOLOR, (WORD)hdc,
		    MAKELONG(hwnd, CTLCOLOR_LISTBOX));
	if (hBrush == (HBRUSH)NULL)  hBrush = GetStockObject(WHITE_BRUSH);
	if (lphl->ItemsCount > lphl->ItemsVisible) rect.right -= 16;
	FillRect(hdc, &rect, hBrush);
	if (lphl->ItemsCount == 0) goto EndOfPaint;
	lpls = lphl->lpFirst;
	if (lpls == NULL) goto EndOfPaint;
	lphl->ItemsVisible = 0;
	for(i = 1; i <= lphl->ItemsCount; i++) {
	    if (i >= lphl->FirstVisible) {
		h2 = lpls->dis.rcItem.bottom - lpls->dis.rcItem.top;
		lpls->dis.rcItem.top = h;
		lpls->dis.rcItem.bottom = h + h2;
		lpls->dis.rcItem.right = rect.right;
		TextOut(hdc, 5, h + 2, (char *)lpls->dis.itemData, 
			strlen((char *)lpls->dis.itemData));
		if (lpls->dis.itemState != 0) {
		    InvertRect(hdc, &lpls->dis.rcItem);
		    }
		h += h2;
		lphl->ItemsVisible++;
		if (h > rect.bottom) break;
		}
	    if (lpls->lpNext == NULL) goto EndOfPaint;
	    lpls = (LPLISTSTRUCT)lpls->lpNext;
	}
EndOfPaint:
    EndPaint( hwnd, &ps );
    if (lphl->ItemsCount > lphl->ItemsVisible) {
        InvalidateRect(lphl->hWndScroll, NULL, TRUE);
        UpdateWindow(lphl->hWndScroll);
 	}
}



void OwnerDrawListBox(HWND hwnd)
{
	LPHEADLIST  lphl;
	LPLISTSTRUCT lpls;
	PAINTSTRUCT ps;
	HBRUSH 	hBrush;
	HWND	hWndParent;
	HDC hdc;
	RECT rect;
	UINT  i, h, h2;
	char	C[128];
	h = 0;
	hdc = BeginPaint( hwnd, &ps );
    	if (!IsWindowVisible(hwnd)) {
	    EndPaint( hwnd, &ps );
	    return;
	    }
	GetClientRect(hwnd, &rect);
        lphl = ListBoxGetStorageHeader(hwnd);
	if (lphl == NULL) goto EndOfPaint;
	hBrush = SendMessage(lphl->hWndLogicParent, WM_CTLCOLOR, (WORD)hdc,
		    MAKELONG(hwnd, CTLCOLOR_LISTBOX));
	if (hBrush == (HBRUSH)NULL)  hBrush = GetStockObject(WHITE_BRUSH);
	if (lphl->ItemsCount > lphl->ItemsVisible) rect.right -= 16;
	FillRect(hdc, &rect, hBrush);
	if (lphl->ItemsCount == 0) goto EndOfPaint;
	lpls = lphl->lpFirst;
	if (lpls == NULL) goto EndOfPaint;
	lphl->ItemsVisible = 0;
	for(i = 1; i <= lphl->ItemsCount; i++) {
	    if (i >= lphl->FirstVisible) {
		lpls->dis.hDC = hdc;
		lpls->dis.itemID = i;
		h2 = lpls->dis.rcItem.bottom - lpls->dis.rcItem.top;
		lpls->dis.rcItem.top = h;
		lpls->dis.rcItem.bottom = h + h2;
		lpls->dis.rcItem.right = rect.right;
		lpls->dis.itemAction = ODA_DRAWENTIRE;
		if (lpls->dis.itemState != 0) {
		    lpls->dis.itemAction |= ODA_SELECT;
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
		if (h > rect.bottom) break;
		}
	    if (lpls->lpNext == NULL) goto EndOfPaint;
	    lpls = (LPLISTSTRUCT)lpls->lpNext;
	}
EndOfPaint:
    EndPaint( hwnd, &ps );
    if (lphl->ItemsCount > lphl->ItemsVisible) {
        InvalidateRect(lphl->hWndScroll, NULL, TRUE);
        UpdateWindow(lphl->hWndScroll);
        }
}



int ListBoxFindMouse(HWND hwnd, int X, int Y)
{
LPHEADLIST 	lphl;
LPLISTSTRUCT	lpls;
RECT rect;
UINT  i, h, h2;
char	C[128];
h = 0;
lphl = ListBoxGetStorageHeader(hwnd);
if (lphl == NULL) return LB_ERR;
if (lphl->ItemsCount == 0) return LB_ERR;
lpls = lphl->lpFirst;
if (lpls == NULL) return LB_ERR;
for(i = 1; i <= lphl->ItemsCount; i++) {
    if (i >= lphl->FirstVisible) {
	h2 = h;
	h += lpls->dis.rcItem.bottom - lpls->dis.rcItem.top;
	if ((Y > h2) && (Y < h)) return(i);
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
	    }
	}
    ListBoxDefaultItem(hwnd, wndPtr, lphl, lplsnew);
    lplsnew->hMem = hTemp;
    lplsnew->lpNext = NULL;
    lplsnew->dis.itemID = lphl->ItemsCount;
    lplsnew->dis.itemData = (DWORD)newstr;
    lplsnew->hData = hTemp;
    SetScrollRange(lphl->hWndScroll, WM_VSCROLL, 1, lphl->ItemsCount, 
    	(lphl->FirstVisible != 1));
    if (lphl->FirstVisible >= (lphl->ItemsCount - lphl->ItemsVisible)) {
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        }
    if ((lphl->ItemsCount - lphl->FirstVisible) == lphl->ItemsVisible) 
    	ShowWindow(lphl->hWndScroll, SW_NORMAL);
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
    if (uIndex < 1 || uIndex > lphl->ItemsCount) return LB_ERR;
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
    SetScrollRange(lphl->hWndScroll, WM_VSCROLL, 1, lphl->ItemsCount, 
    	(lphl->FirstVisible != 1));
    if (((lphl->ItemsCount - lphl->FirstVisible) == lphl->ItemsVisible) && 
        (lphl->ItemsVisible != 0)) 
    	ShowWindow(lphl->hWndScroll, SW_NORMAL);
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
    if (uIndex < 1 || uIndex > lphl->ItemsCount) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    if (uIndex > lphl->ItemsCount) return LB_ERR;
    for(Count = 1; Count < uIndex; Count++) {
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
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls, lpls2;
    UINT	Count;
    lphl = ListBoxGetStorageHeader(hwnd);
    if (lphl == NULL) return LB_ERR;
    if (uIndex < 1 || uIndex > lphl->ItemsCount) return LB_ERR;
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
    SetScrollRange(lphl->hWndScroll, WM_VSCROLL, 1, lphl->ItemsCount, TRUE);
    if (lphl->ItemsCount < lphl->ItemsVisible) 
    	ShowWindow(lphl->hWndScroll, SW_HIDE);
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
    if (nFirst < 1 || nFirst > lphl->ItemsCount) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    Count = 1;
    while(lpls != NULL) {
    	if (strcmp((char *)lpls->dis.itemData, MatchStr) == 0)
	    return Count;
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
    lphl->ItemSelected = 0;
    lphl->PrevSelected = 0;
    if ((wndPtr->dwStyle && LBS_NOTIFY) != 0)
	SendMessage(wndPtr->hwndParent, WM_COMMAND, 
    	    wndPtr->wIDmenu, MAKELONG(hwnd, LBN_SELCHANGE));

    SetScrollRange(lphl->hWndScroll, WM_VSCROLL, 1, lphl->ItemsCount, TRUE);
    ShowWindow(lphl->hWndScroll, SW_HIDE);
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
    lphl->ItemSelected = LB_ERR;
    if (wIndex < 0 || wIndex >= lphl->ItemsCount) return LB_ERR;
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
    lphl->ItemSelected = wIndex;
    if ((wndPtr->dwStyle && LBS_NOTIFY) != 0)
	SendMessage(wndPtr->hwndParent, WM_COMMAND, 
    	    wndPtr->wIDmenu, MAKELONG(hwnd, LBN_SELCHANGE));
    return LB_ERR;
}



int ListBoxSetSel(HWND hwnd, WORD wIndex)
{
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls, lpls2;
    UINT	i;
    lphl = ListBoxGetStorageHeader(hwnd);
    if (lphl == NULL) return LB_ERR;
    if (wIndex < 0 || wIndex >= lphl->ItemsCount) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    for(i = 0; i < lphl->ItemsCount; i++) {
	lpls2 = lpls;
	lpls = (LPLISTSTRUCT)lpls->lpNext;
	if (i == wIndex) {
	    lpls2->dis.itemState = 1;
	    break;
	    }  
	if (lpls == NULL)  break;
    }
    return LB_ERR;
}


int ListBoxDirectory(HWND hwnd, UINT attrib, LPSTR filespec)
{
DIR           	*dirp;
struct dirent 	*dp;
struct stat	st;
char		str[128];
int		wRet;
dirp = opendir(".");
while ( (dp = readdir(dirp)) != NULL) 
    {
    stat(dp->d_name, &st);
#ifdef DEBUG_LBDIR
    printf("LB_DIR : st_mode=%lX / d_name='%s'\n", st.st_mode, dp->d_name);
#endif
    if S_ISDIR(st.st_mode) {
	sprintf(str, "[%s]", dp->d_name);
	}
    else
	strcpy(str, dp->d_name);
    wRet = ListBoxAddString(hwnd, str);
    if (wRet == LB_ERR) break;
    }
closedir(dirp);	
return wRet;
}


int ListBoxGetItemRect(HWND hwnd, WORD wIndex, LPRECT lprect)
{
    LPHEADLIST 	lphl;
    LPLISTSTRUCT lpls, lpls2;
    UINT	i;
    lphl = ListBoxGetStorageHeader(hwnd);
    if (lphl == NULL) return LB_ERR;
    if (wIndex < 1 || wIndex > lphl->ItemsCount) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    for(i = 0; i <= lphl->ItemsCount; i++) {
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
    if (wIndex < 1 || wIndex > lphl->ItemsCount) return LB_ERR;
    lpls = lphl->lpFirst;
    if (lpls == NULL) return LB_ERR;
    for(i = 0; i <= lphl->ItemsCount; i++) {
	lpls2 = lpls;
	lpls = (LPLISTSTRUCT)lpls->lpNext;
	if (i == wIndex) {
	    lpls2->dis.rcItem.bottom = lpls2->dis.rcItem.top + (short)height;
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
    Count = 1;
    while(lpls != NULL) {
        if (Count > lphl->ItemSelected) {
	    if (*((char *)lpls->dis.itemData) == (char)wChar) {
		lphl->FirstVisible = Count - lphl->ItemsVisible / 2;
		if (lphl->FirstVisible < 1) lphl->FirstVisible = 1;
		ListBoxSetCurSel(hwnd, Count);
		SetScrollPos(lphl->hWndScroll, WM_VSCROLL, lphl->FirstVisible, TRUE);
	        InvalidateRect(hwnd, NULL, TRUE);
	        UpdateWindow(hwnd);
	        return Count;
	        }
	    }
	lpls = (LPLISTSTRUCT)lpls->lpNext;
	Count++;
	}
    Count = 1;
    lpls = lphl->lpFirst;
    while(lpls != NULL) {
	if (*((char *)lpls->dis.itemData) == (char)wChar) {
	    if (Count == lphl->ItemSelected)    return LB_ERR;
	    lphl->FirstVisible = Count - lphl->ItemsVisible / 2;
	    if (lphl->FirstVisible < 1) lphl->FirstVisible = 1;
	    ListBoxSetCurSel(hwnd, Count);
	    SetScrollPos(lphl->hWndScroll, WM_VSCROLL, lphl->FirstVisible, TRUE);
	    InvalidateRect(hwnd, NULL, TRUE);
	    UpdateWindow(hwnd);
	    return Count;
	    }
	lpls = (LPLISTSTRUCT)lpls->lpNext;
	Count++;
        }
    return LB_ERR;
}


