/*
 * Interface code to SCROLLBAR widget
 *
 * Copyright  Martin Ayotte, 1993
 *
 */

/*
#define DEBUG_SCROLL
*/
static char Copyright[] = "Copyright Martin Ayotte, 1993";

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "windows.h"
#include "sysmetrics.h"
#include "scroll.h"
#include "heap.h"
#include "win.h"
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

HBITMAP hUpArrow = 0;
HBITMAP hDnArrow = 0;
HBITMAP hLfArrow = 0;
HBITMAP hRgArrow = 0;
HBITMAP hUpArrowD = 0;
HBITMAP hDnArrowD = 0;
HBITMAP hLfArrowD = 0;
HBITMAP hRgArrowD = 0;

LPHEADSCROLL ScrollBarGetWindowAndStorage(HWND hWnd, WND **wndPtr);
LPHEADSCROLL ScrollBarGetStorageHeader(HWND hWnd);
LPHEADSCROLL GetScrollObjectHandle(HWND hWnd, int nBar);
void ScrollBarButtonDown(HWND hWnd, int nBar, int x, int y);
void ScrollBarButtonUp(HWND hWnd, int nBar, int x, int y);
void ScrollBarMouseMove(HWND hWnd, int nBar, WORD wParam, int x, int y);
void StdDrawScrollBar(HWND hWnd, HDC hDC, int nBar, LPRECT lprect, LPHEADSCROLL lphs);
int CreateScrollBarStruct(HWND hWnd);
void NC_CreateScrollBars(HWND hWnd);
LPHEADSCROLL AllocScrollBar(DWORD dwStyle, int width, int height);


/***********************************************************************
 *           WIDGETS_ScrollBarWndProc
 */
LONG ScrollBarWndProc( HWND hWnd, WORD message, WORD wParam, LONG lParam )
{    
    WORD	wRet;
    short	x, y;
    short	width, height;
    WND  	*wndPtr;
    LPHEADSCROLL lphs;
    PAINTSTRUCT ps;
    HDC		hDC;
    BITMAP	bm;
    RECT 	rect, rect2;
    static RECT rectsel;
    switch(message)
    {
    case WM_CREATE:
	CreateScrollBarStruct(hWnd);
#ifdef DEBUG_SCROLL
        printf("ScrollBar Creation !\n");
#endif
	if (hUpArrow == (HBITMAP)NULL) 
	    hUpArrow = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_UPARROWI));
	if (hDnArrow == (HBITMAP)NULL) 
	    hDnArrow = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_DNARROWI));
	if (hLfArrow == (HBITMAP)NULL) 
	    hLfArrow = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_LFARROWI));
	if (hRgArrow == (HBITMAP)NULL) 
	    hRgArrow = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_RGARROWI));
	if (hUpArrowD == (HBITMAP)NULL) 
	    hUpArrowD = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_UPARROWD));
	if (hDnArrowD == (HBITMAP)NULL) 
	    hDnArrowD = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_DNARROWD));
	if (hLfArrowD == (HBITMAP)NULL) 
	    hLfArrowD = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_LFARROWD));
	if (hRgArrowD == (HBITMAP)NULL) 
	    hRgArrowD = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_RGARROWD));
	return 0;
    case WM_DESTROY:
	lphs = ScrollBarGetWindowAndStorage(hWnd, &wndPtr);
	if (lphs == 0) return 0;
#ifdef DEBUG_SCROLL
        printf("ScrollBar WM_DESTROY %lX !\n", lphs);
#endif
	free(lphs);
	*((LPHEADSCROLL *)&wndPtr->wExtra[1]) = 0;
	return 0;
	
    case WM_LBUTTONDOWN:
	SetCapture(hWnd);
	ScrollBarButtonDown(hWnd, SB_CTL, LOWORD(lParam), HIWORD(lParam));
	break;
    case WM_LBUTTONUP:
	ReleaseCapture();
	ScrollBarButtonUp(hWnd, SB_CTL, LOWORD(lParam), HIWORD(lParam));
	break;

    case WM_MOUSEMOVE:
	ScrollBarMouseMove(hWnd, SB_CTL, wParam, LOWORD(lParam), HIWORD(lParam));
	break;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
	lphs = ScrollBarGetWindowAndStorage(hWnd, &wndPtr);
	return(SendMessage(wndPtr->hwndParent, message, wParam, lParam));

    case WM_TIMER:
#ifdef DEBUG_SCROLL
        printf("ScrollBar WM_TIMER wParam=%X lParam=%lX !\n", wParam, lParam);
#endif
	lphs = ScrollBarGetWindowAndStorage(hWnd, &wndPtr);
	KillTimer(hWnd, wParam);
	switch(lphs->ButtonDown) {
	    case 0:
		lphs->TimerPending = FALSE;
		return 0;
	    case 1:
	    case 3:
		SendMessage(wndPtr->hwndParent, lphs->Direction, 
			SB_LINEUP, MAKELONG(0, hWnd));
		break;
	    case 2:
	    case 4:
		SendMessage(wndPtr->hwndParent, lphs->Direction, 
			SB_LINEDOWN, MAKELONG(0, hWnd));
		break;
	    case 5:
		SendMessage(wndPtr->hwndParent, lphs->Direction, 
			SB_PAGEUP, MAKELONG(0, hWnd));
		break;
	    case 6:
		SendMessage(wndPtr->hwndParent, lphs->Direction, 
			SB_PAGEDOWN, MAKELONG(0, hWnd));
		break;
	    }
	SetTimer(hWnd, 1, 100, NULL);
	return 0;

    case WM_PAINT:
	hDC = BeginPaint(hWnd, &ps);
	lphs = ScrollBarGetStorageHeader(hWnd);
	if (lphs != NULL) {
	    GetClientRect(hWnd, &rect);
	    StdDrawScrollBar(hWnd, hDC, SB_CTL, &rect, lphs);
	    }
	EndPaint(hWnd, &ps);
	break;
    default:
	return DefWindowProc( hWnd, message, wParam, lParam );
    }
return(0);
}



void ScrollBarButtonDown(HWND hWnd, int nBar, int x, int y)
{
    LPHEADSCROLL lphs;
    HWND	hWndParent;
    RECT	rect, rect2;
    int		width, height;
    LONG	dwOwner;
    lphs = GetScrollObjectHandle(hWnd, nBar);
	    printf("ScrollBarButtonDown // x=%d y=%d\n", x, y);
#ifdef DEBUG_SCROLL
	    printf("ScrollBarButtonDown // x=%d y=%d\n", x, y);
#endif
    if (nBar == SB_CTL) {
	hWndParent = GetParent(hWnd);
	dwOwner = MAKELONG(0, lphs->hWndOwner);
	}
    else {
	hWndParent = hWnd;
	dwOwner = 0L;
	}
/*
    SetFocus(lphs->hWndOwner);
*/
    if (nBar != SB_CTL) {
	GetWindowRect(lphs->hWndOwner, &rect);
	x -= rect.left;
	y -= rect.top;
	}
    CopyRect(&rect, &lphs->rect);
    printf("ScrollDown / x=%d y=%d left=%d top=%d right=%d bottom=%d \n",
    	x, y, rect.left, rect.top, rect.right, rect.bottom);
    if (lphs->Direction == WM_VSCROLL) {
	y -= rect.top;
	width = rect.right - rect.left;
	if (y < (lphs->CurPix + width)) {
	    if (y < width) {
		lphs->ButtonDown = 1;
		CopyRect(&rect2, &rect);
		rect2.bottom = rect2.top + width;
		InvalidateRect(lphs->hWndOwner, &rect2, TRUE);
	    printf("ScrollBarButtonDown // SB_LINEUP \n");
	        SendMessage(hWndParent, lphs->Direction, 
				SB_LINEUP, dwOwner);
		}
	    else {
		lphs->ButtonDown = 5;
	    printf("ScrollBarButtonDown // SB_PAGEUP \n");
		SendMessage(hWndParent, lphs->Direction, 
				SB_PAGEUP, dwOwner);
		}
	    }
	if (y > (lphs->CurPix + (width << 1))) {
	    if (y > (rect.bottom - width)) {
		lphs->ButtonDown = 2;
		CopyRect(&rect2, &rect);
		rect2.top = rect2.bottom - width;
		InvalidateRect(lphs->hWndOwner, &rect2, TRUE);
	    printf("ScrollBarButtonDown // SB_LINEDOWN \n");
	        SendMessage(hWndParent, lphs->Direction, 
				SB_LINEDOWN, dwOwner);
		}
	    else {
		lphs->ButtonDown = 6;
	    printf("ScrollBarButtonDown // SB_PAGEDOWN \n");
		SendMessage(hWndParent, lphs->Direction, 
				SB_PAGEDOWN, dwOwner);
		}
	    }
	if ((y > (lphs->CurPix + width)) &&
	    (y < (lphs->CurPix + (width << 1)))) {
	    lphs->ThumbActive = TRUE;
#ifdef DEBUG_SCROLL
	    printf("THUMB DOWN !\n");
#endif
	    }
	}
    else {
	x -= rect.left;
	height = rect.bottom - rect.top;
	if (x < (lphs->CurPix + height)) {
	    if (x < height) {
		lphs->ButtonDown = 3;
		CopyRect(&rect2, &rect);
		rect2.right = rect2.left + height;
		InvalidateRect(lphs->hWndOwner, &rect2, TRUE);
	        SendMessage(hWndParent, lphs->Direction, 
				SB_LINEUP, dwOwner);
		}
	    else {
		lphs->ButtonDown = 5;
		SendMessage(hWndParent, lphs->Direction, 
				SB_PAGEUP, dwOwner);
		}
	    }
	if (x > (lphs->CurPix + (height << 1))) {
	    if (x > (rect.right - rect.left - height)) {
		lphs->ButtonDown = 4;
		CopyRect(&rect2, &rect);
		rect2.left = rect2.right - height;
		InvalidateRect(lphs->hWndOwner, &rect2, TRUE);
	        SendMessage(hWndParent, lphs->Direction, 
				SB_LINEDOWN, dwOwner);
		}
	    else {
		lphs->ButtonDown = 6;
		SendMessage(hWndParent, lphs->Direction, 
				SB_PAGEDOWN, dwOwner);
		}
	    }
	if ((x > (lphs->CurPix + height)) &&
	    (x < (lphs->CurPix + (height << 1)))) {
	    lphs->ThumbActive = TRUE;
#ifdef DEBUG_SCROLL
	    printf("THUMB DOWN !\n");
#endif
	    }
	}
    if (lphs->ButtonDown != 0) {
	UpdateWindow(lphs->hWndOwner);
	if (!lphs->TimerPending && nBar == SB_CTL) {
	    lphs->TimerPending = TRUE;
	    SetTimer(lphs->hWndOwner, 1, 500, NULL);
	    }
	}
}


void ScrollBarButtonUp(HWND hWnd, int nBar, int x, int y)
{
    LPHEADSCROLL lphs;
    RECT	rect, rect2;
    printf("ScrollBarButtonUp // x=%d y=%d\n", x, y); 
#ifdef DEBUG_SCROLL
    printf("ScrollBarButtonUp // x=%d y=%d\n", x, y); 
#endif
    lphs = GetScrollObjectHandle(hWnd, nBar);
    lphs->ThumbActive = FALSE;
    if (lphs->ButtonDown != 0) {
	lphs->ButtonDown = 0;
	GetClientRect(lphs->hWndOwner, &rect);
	InvalidateRect(lphs->hWndOwner, &rect, TRUE);
	UpdateWindow(lphs->hWndOwner);
	}
}


void ScrollBarMouseMove(HWND hWnd, int nBar, WORD wParam, int x, int y)
{
    LPHEADSCROLL lphs;
    HWND	hWndParent;
    HWND	hWndOwner;
    LONG	dwOwner;
    if ((wParam & MK_LBUTTON) == 0) return;
#ifdef DEBUG_SCROLL
    printf("ScrollBarButtonMove // w=%04X x=%d y=%d \n", wParam, x, y);
#endif
    lphs = GetScrollObjectHandle(hWnd, nBar);
    if (lphs->ThumbActive == 0) return;
    if (nBar == SB_CTL) {
	hWndParent = GetParent(hWnd);
	hWndOwner = lphs->hWndOwner;
	}
    else {
	hWndParent = hWnd;
	hWndOwner = 0;
	}
    if (lphs->Direction == WM_VSCROLL) {
	int butsiz = lphs->rect.right - lphs->rect.left;
	y = y - butsiz - (butsiz >> 1);
	}
    else {
	int butsiz = lphs->rect.bottom - lphs->rect.top;
	y = x - butsiz - (butsiz >> 1);
	}
    x = (y * (lphs->MaxVal - lphs->MinVal) / 
		lphs->MaxPix) + lphs->MinVal;
#ifdef DEBUG_SCROLL
    printf("Scroll WM_MOUSEMOVE val=%d pix=%d\n", x, y);
#endif
    SendMessage(hWndParent, lphs->Direction, 
	SB_THUMBTRACK, MAKELONG(x, hWndOwner));
}


LPHEADSCROLL ScrollBarGetWindowAndStorage(HWND hWnd, WND **wndPtr)
{
    WND  *Ptr;
    LPHEADSCROLL lphs;
    *(wndPtr) = Ptr = WIN_FindWndPtr(hWnd);
    if (Ptr == 0) {
    	printf("Bad Window handle on ScrollBar !\n");
    	return 0;
    	}
    lphs = *((LPHEADSCROLL *)&Ptr->wExtra[1]);
    return lphs;
}


LPHEADSCROLL ScrollBarGetStorageHeader(HWND hWnd)
{
    WND  *wndPtr;
    LPHEADSCROLL lphs;
    wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == 0) {
    	printf("Bad Window handle on ScrollBar !\n");
    	return 0;
    	}
    lphs = *((LPHEADSCROLL *)&wndPtr->wExtra[1]);
    return lphs;
}


void StdDrawScrollBar(HWND hWnd, HDC hDC, int nBar, LPRECT lprect, LPHEADSCROLL lphs)
{
    HWND	hWndParent;
    HBRUSH 	hBrush;
    HDC 	hMemDC;
    BITMAP	bm;
    RECT 	rect;
    UINT  	i, w, h, siz;
    char	C[128];
    if (lphs == NULL) return;
#ifdef DEBUG_SCROLL
    if (lphs->Direction == WM_VSCROLL)
        printf("StdDrawScrollBar Vertical left=%d top=%d right=%d bottom=%d !\n", 
        	lprect->left, lprect->top, lprect->right, lprect->bottom);
    else
        printf("StdDrawScrollBar Horizontal left=%d top=%d right=%d bottom=%d !\n", 
        	lprect->left, lprect->top, lprect->right, lprect->bottom);
#endif
    if (nBar == SB_CTL)
	hWndParent = GetParent(hWnd);
    else
	hWndParent = lphs->hWndOwner;
    hBrush = SendMessage(hWndParent, WM_CTLCOLOR, (WORD)hDC,
			MAKELONG(hWnd, CTLCOLOR_SCROLLBAR));
    if (hBrush == (HBRUSH)NULL)  hBrush = GetStockObject(LTGRAY_BRUSH);
    CopyRect(&lphs->rect, lprect);
    CopyRect(&rect, lprect);
    w = rect.right - rect.left;
    h = rect.bottom - rect.top;
    hMemDC = CreateCompatibleDC(hDC);
    if (lphs->Direction == WM_VSCROLL) {
	GetObject(hUpArrow, sizeof(BITMAP), (LPSTR)&bm);
	if (lphs->ButtonDown == 1)
	    SelectObject(hMemDC, hUpArrowD);
	else
	    SelectObject(hMemDC, hUpArrow);
	BitBlt(hDC, rect.left, rect.top, 
		bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
/*
	StretchBlt(hDC, 0, 0, lpdis->rcItem.right, lpdis->rcItem.right,
			hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
*/
	GetObject(hDnArrow, sizeof(BITMAP), (LPSTR)&bm);
	if (lphs->ButtonDown == 2)
	    SelectObject(hMemDC, hDnArrowD);
	else
	    SelectObject(hMemDC, hDnArrow);
	BitBlt(hDC, rect.left, rect.bottom - bm.bmHeight, 
		bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	rect.top += w;
	rect.bottom -= w;
	}
    else {
	GetObject(hLfArrow, sizeof(BITMAP), (LPSTR)&bm);
	if (lphs->ButtonDown == 3)
	    SelectObject(hMemDC, hLfArrowD);
	else
	    SelectObject(hMemDC, hLfArrow);
	BitBlt(hDC, rect.left, rect.top, 
		bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	GetObject(hRgArrow, sizeof(BITMAP), (LPSTR)&bm);
	if (lphs->ButtonDown == 4)
	    SelectObject(hMemDC, hRgArrowD);
	else
	    SelectObject(hMemDC, hRgArrow);
	BitBlt(hDC, rect.right - bm.bmWidth, rect.top, 
		bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	rect.left += h;
	rect.right -= h;
	}
    DeleteDC(hMemDC);
    FillRect(hDC, &rect, hBrush);
    if (lphs->Direction == WM_VSCROLL)
	SetRect(&rect, rect.left, rect.top + lphs->CurPix, 
		rect.left + w, rect.top + lphs->CurPix + w);
    else
	SetRect(&rect, rect.left + lphs->CurPix, rect.top, 
		rect.left + lphs->CurPix + h, rect.top + h);
/*
    if (lphs->Direction == WM_VSCROLL)
	SetRect(&rect, rect.left, rect.top + lphs->CurPix + w, 
		rect.left + w, rect.top + lphs->CurPix + (w << 1));
    else
	SetRect(&rect, rect.left + lphs->CurPix + h, rect.top, 
		rect.left + lphs->CurPix + (h << 1), rect.top + h);
*/
    FrameRect(hDC, &rect, GetStockObject(BLACK_BRUSH));
    InflateRect(&rect, -1, -1);
    FillRect(hDC, &rect, GetStockObject(LTGRAY_BRUSH));
    DrawReliefRect(hDC, rect, 2, 0);
    InflateRect(&rect, -3, -3);
    DrawReliefRect(hDC, rect, 1, 1);
}



int CreateScrollBarStruct(HWND hWnd)
{
    RECT	rect;
    int		width, height;
    WND  *wndPtr;
    LPHEADSCROLL lphs;
    wndPtr = WIN_FindWndPtr(hWnd);
    lphs = (LPHEADSCROLL)malloc(sizeof(HEADSCROLL));
    if (lphs == 0) {
    	printf("Bad Memory Alloc on ScrollBar !\n");
    	return 0;
    	}
#ifdef DEBUG_SCROLL
        printf("CreateScrollBarStruct %lX !\n", lphs);
#endif
    *((LPHEADSCROLL *)&wndPtr->wExtra[1]) = lphs;
    lphs->hWndOwner = hWnd;
    lphs->ThumbActive = FALSE;
    lphs->TimerPending = FALSE;
    lphs->ButtonDown = 0;
    lphs->MinVal = 0;
    lphs->MaxVal = 100;
    lphs->CurVal = 0;
    lphs->CurPix = 0;
    width = wndPtr->rectClient.right - wndPtr->rectClient.left;
    height = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
    CopyRect(&lphs->rect, &wndPtr->rectClient);
    if (width <= height)
	{
	lphs->MaxPix = height - 3 * width;
	lphs->Direction = WM_VSCROLL;
	}
    else
	{
	lphs->MaxPix = width - 3 * height;
	lphs->Direction = WM_HSCROLL;
	}
    if (lphs->MaxPix < 1)  lphs->MaxPix = 1;
    if (wndPtr->hCursor == (HCURSOR)NULL)
	wndPtr->hCursor = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
    return TRUE;
}



LPHEADSCROLL AllocScrollBar(DWORD dwStyle, int width, int height)
{
    LPHEADSCROLL lphs;
    lphs = (LPHEADSCROLL)malloc(sizeof(HEADSCROLL));
    if (lphs == 0) {
    	printf("Bad Memory Alloc on ScrollBar !\n");
    	return NULL;
    	}
    lphs->ThumbActive = FALSE;
    lphs->TimerPending = FALSE;
    lphs->ButtonDown = 0;
    lphs->MinVal = 0;
    lphs->MaxVal = 100;
    lphs->CurVal = 0;
    lphs->CurPix = 0;
    if (dwStyle & WS_VSCROLL) {
	lphs->MaxPix = height - 3 * width;
	lphs->Direction = WM_VSCROLL;
	}
    else {
	lphs->MaxPix = width - 3 * height;
	lphs->Direction = WM_HSCROLL;
	}
    if (lphs->MaxPix < 1)  lphs->MaxPix = 1;
    return lphs;
}


void NC_CreateScrollBars(HWND hWnd)
{
    RECT	rect;
    int		width, height;
    WND  	*wndPtr;
    LPHEADSCROLL lphs;
    wndPtr = WIN_FindWndPtr(hWnd);
    width = wndPtr->rectClient.right - wndPtr->rectClient.left;
    height = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
    if (wndPtr->dwStyle & WS_VSCROLL) {
	if (wndPtr->dwStyle & WS_HSCROLL) height -= SYSMETRICS_CYHSCROLL;
	lphs = AllocScrollBar(WS_VSCROLL, SYSMETRICS_CXVSCROLL, height);
#ifdef DEBUG_SCROLL
        printf("NC_CreateScrollBars Vertical %lX !\n", lphs);
#endif
	lphs->hWndOwner = hWnd;
	wndPtr->VScroll = lphs;
	}
    if (wndPtr->dwStyle & WS_HSCROLL) {
	if (wndPtr->dwStyle & WS_VSCROLL) width -= SYSMETRICS_CYVSCROLL;
	lphs = AllocScrollBar(WS_HSCROLL, width, SYSMETRICS_CYHSCROLL);
#ifdef DEBUG_SCROLL
        printf("NC_CreateScrollBars Horizontal %lX !\n", lphs);
#endif
	lphs->hWndOwner = hWnd;
	wndPtr->HScroll = lphs;
	}
}


/*************************************************************************
 *			GetScrollObjectHandle
 */
LPHEADSCROLL GetScrollObjectHandle(HWND hWnd, int nBar)
{
    WND *wndPtr;
    LPHEADSCROLL lphs;
    if (nBar != SB_CTL) {
	wndPtr = WIN_FindWndPtr(hWnd);
    	if (nBar == SB_VERT) return (LPHEADSCROLL)wndPtr->VScroll;
    	if (nBar == SB_HORZ) return (LPHEADSCROLL)wndPtr->HScroll;
    	return NULL;
	}
    return ScrollBarGetStorageHeader(hWnd);
}


/*************************************************************************
 *			SetScrollPos [USER.62]
 */
int SetScrollPos(HWND hWnd, int nBar, int nPos, BOOL bRedraw)
{
    int nRet;
    LPHEADSCROLL lphs;
    lphs = GetScrollObjectHandle(hWnd, nBar);
    if (lphs == NULL) return 0;
    nRet = lphs->CurVal;
    lphs->CurVal = (short)nPos;
    if (lphs->MaxVal != lphs->MinVal)
	lphs->CurPix = lphs->MaxPix * (abs((short)nPos) - abs(lphs->MinVal)) / 
    		(abs(lphs->MaxVal) - abs(lphs->MinVal));
    if (lphs->CurPix > lphs->MaxPix)  lphs->CurPix = lphs->MaxPix;
#ifdef DEBUG_SCROLL
    printf("SetScrollPos val=%d pixval=%d pixmax%d\n",
	    (short)nPos, lphs->CurPix, lphs->MaxPix);
    printf("SetScrollPos min=%d max=%d\n", 
	    lphs->MinVal, lphs->MaxVal);
#endif
    if ((bRedraw) && (IsWindowVisible(lphs->hWndOwner))) {
        InvalidateRect(lphs->hWndOwner, &lphs->rect, TRUE);
        UpdateWindow(lphs->hWndOwner);
        }
    return nRet;
}



/*************************************************************************
 *			GetScrollPos [USER.63]
 */
int GetScrollPos(HWND hWnd, int nBar)
{
    LPHEADSCROLL lphs;
    lphs = GetScrollObjectHandle(hWnd, nBar);
    if (lphs == NULL) return 0;
    return lphs->CurVal;
}



/*************************************************************************
 *			SetScrollRange [USER.64]
 */
void SetScrollRange(HWND hWnd, int nBar, int MinPos, int MaxPos, BOOL bRedraw)
{
    LPHEADSCROLL lphs;
    lphs = GetScrollObjectHandle(hWnd, nBar);
    if (lphs == NULL) return;
    lphs->MinVal = (short)MinPos;
    lphs->MaxVal = (short)MaxPos;
    if (lphs->MaxVal != lphs->MinVal)
	lphs->CurPix = abs(lphs->MaxVal) * 
		(abs(lphs->CurVal) - abs(lphs->MinVal)) / 
    		(abs(lphs->MaxVal) - abs(lphs->MinVal));
    if (lphs->CurPix > lphs->MaxPix)  lphs->CurPix = lphs->MaxPix;
#ifdef DEBUG_SCROLL
    printf("SetScrollRange min=%d max=%d\n", lphs->MinVal, lphs->MaxVal);
#endif
    if ((bRedraw) && (IsWindowVisible(lphs->hWndOwner))) {
        InvalidateRect(lphs->hWndOwner, &lphs->rect, TRUE);
        UpdateWindow(lphs->hWndOwner);
        }
}



/*************************************************************************
 *			GetScrollRange [USER.65]
 */
void GetScrollRange(HWND hWnd, int nBar, LPINT lpMin, LPINT lpMax)
{
    LPHEADSCROLL lphs;
    lphs = GetScrollObjectHandle(hWnd, nBar);
    if (lphs == NULL) return;
    *lpMin = lphs->MinVal;
    *lpMax = lphs->MaxVal;
}



/*************************************************************************
 *			ShowScrollBar [USER.267]
 */
void ShowScrollBar(HWND hWnd, WORD wBar, BOOL bFlag)
{
    WND  *wndPtr;
#ifdef DEBUG_SCROLL
    printf("ShowScrollBar hWnd=%04X wBar=%d bFlag=%d\n", hWnd, wBar, bFlag);
#endif
    if (wBar == SB_CTL) {
    	if (bFlag)
	    ShowWindow(hWnd, SW_SHOW);
	else
	    ShowWindow(hWnd, SW_HIDE);
	return;
	}
    wndPtr = WIN_FindWndPtr(hWnd);
    if ((wBar == SB_VERT) || (wBar == SB_BOTH)) {
/*
    	if (bFlag)
	    ShowWindow(wndPtr->hWndVScroll, SW_SHOW);
	else
	    ShowWindow(wndPtr->hWndVScroll, SW_HIDE);
*/
	}
    if ((wBar == SB_HORZ) || (wBar == SB_BOTH)) {
/*
    	if (bFlag)
	    ShowWindow(wndPtr->hWndHScroll, SW_SHOW);
	else
	    ShowWindow(wndPtr->hWndHScroll, SW_HIDE);
*/
	}
}


