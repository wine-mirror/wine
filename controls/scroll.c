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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "windows.h"
#include "sysmetrics.h"
#include "scroll.h"
#include "heap.h"
#include "win.h"
#include "prototypes.h"

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
LPHEADSCROLL GetScrollObjectStruct(HWND hWnd, int nBar);
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
	lphs = GetScrollObjectStruct(hWnd, nBar);
	if (nBar == SB_CTL) {
		hWndParent = GetParent(hWnd);
		dwOwner = MAKELONG(0, lphs->hWndOwner);
#ifdef DEBUG_SCROLL
		printf("ScrollBarButtonDown SB_CTL // x=%d y=%d\n", x, y);
#endif
		}
	else {
		hWndParent = hWnd;
		dwOwner = 0L; 
#ifdef DEBUG_SCROLL
		printf("ScrollBarButtonDown SB_?SCROLL // x=%d y=%d\n", x, y);
#endif
		}
/*
	SetFocus(lphs->hWndOwner);
*/
	CopyRect(&rect, &lphs->rect);
#ifdef DEBUG_SCROLL
	printf("ScrollDown / x=%d y=%d left=%d top=%d right=%d bottom=%d \n",
					x, y, rect.left, rect.top, rect.right, rect.bottom);
#endif
	if (lphs->Direction == WM_VSCROLL) {
		width = rect.right - rect.left;
		if (y < (lphs->CurPix + width)) {
			if (y < width) {
				lphs->ButtonDown = 1;
				CopyRect(&rect2, &rect);
				rect2.bottom = rect2.top + width;
				InvalidateRect(lphs->hWndOwner, &rect2, TRUE);
#ifdef DEBUG_SCROLL
				printf("ScrollBarButtonDown send SB_LINEUP\n");
#endif
				SendMessage(hWndParent, lphs->Direction, 
									SB_LINEUP, dwOwner);
				}
			else {
				lphs->ButtonDown = 5;
#ifdef DEBUG_SCROLL
				printf("ScrollBarButtonDown send SB_PAGEUP\n");
#endif
				SendMessage(hWndParent, lphs->Direction, 
									SB_PAGEUP, dwOwner);
				}
			}
		if (y > (lphs->CurPix + (width << 1))) {
			if (y > (rect.bottom - rect.top - width)) {
				lphs->ButtonDown = 2;
				CopyRect(&rect2, &rect);
				rect2.top = rect2.bottom - width;
				InvalidateRect(lphs->hWndOwner, &rect2, TRUE);
#ifdef DEBUG_SCROLL
				printf("ScrollBarButtonDown send SB_LINEDOWN\n");
#endif
				SendMessage(hWndParent, lphs->Direction, 
								SB_LINEDOWN, dwOwner);
				}
			else {
				lphs->ButtonDown = 6;
#ifdef DEBUG_SCROLL
				printf("ScrollBarButtonDown send SB_PAGEDOWN\n");
#endif
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
		height = rect.bottom - rect.top;
		if (x < (lphs->CurPix + height)) {
			if (x < height) {
				lphs->ButtonDown = 3;
				CopyRect(&rect2, &rect);
				rect2.right = rect2.left + height;
				InvalidateRect(lphs->hWndOwner, &rect2, TRUE);
#ifdef DEBUG_SCROLL
				printf("ScrollBarButtonDown send SB_LINEUP\n");
#endif
				SendMessage(hWndParent, lphs->Direction, 
									SB_LINEUP, dwOwner);
				}
			else {
				lphs->ButtonDown = 5;
#ifdef DEBUG_SCROLL
				printf("ScrollBarButtonDown send SB_PAGEUP\n");
#endif
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
#ifdef DEBUG_SCROLL
				printf("ScrollBarButtonDown send SB_LINEDOWN\n");
#endif
				SendMessage(hWndParent, lphs->Direction, 
								SB_LINEDOWN, dwOwner);
				}
			else {
				lphs->ButtonDown = 6;
#ifdef DEBUG_SCROLL
				printf("ScrollBarButtonDown send SB_PAGEDOWN\n");
#endif
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
	HDC		hDC;
#ifdef DEBUG_SCROLL
	printf("ScrollBarButtonUp // x=%d y=%d\n", x, y); 
#endif
	lphs = GetScrollObjectStruct(hWnd, nBar);
	lphs->ThumbActive = FALSE;
	if (lphs->ButtonDown != 0) {
		lphs->ButtonDown = 0;
		GetClientRect(lphs->hWndOwner, &rect);
		if (nBar == SB_CTL) {
			InvalidateRect(lphs->hWndOwner, &rect, TRUE);
			UpdateWindow(lphs->hWndOwner);
			}
		else {
			hDC = GetWindowDC(lphs->hWndOwner);
			StdDrawScrollBar(lphs->hWndOwner, hDC, nBar, &lphs->rect, lphs);
			ReleaseDC(lphs->hWndOwner, hDC);
			}
		}
}


void ScrollBarMouseMove(HWND hWnd, int nBar, WORD wParam, int x, int y)
{
	LPHEADSCROLL lphs;
	HWND	hWndParent;
	HWND	hWndOwner;
	LONG	dwOwner;
	if ((wParam & MK_LBUTTON) == 0) return;
	lphs = GetScrollObjectStruct(hWnd, nBar);
	if (lphs->ThumbActive == 0) return;
	if (nBar == SB_CTL) {
		hWndParent = GetParent(hWnd);
		hWndOwner = lphs->hWndOwner;
#ifdef DEBUG_SCROLL
		printf("ScrollBarButtonMove SB_CTL // x=%d y=%d\n", x, y);
#endif
		}
	else {
		hWndParent = hWnd;
		hWndOwner = 0;
#ifdef DEBUG_SCROLL
		printf("ScrollBarButtonMove SB_?SCROLL // x=%d y=%d\n", x, y);
#endif
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
    if (lphs->Direction == WM_VSCROLL)
	lphs->MaxPix = h - 3 * w;
    else
	lphs->MaxPix = w - 3 * h;
    if (lphs->MaxVal != lphs->MinVal)
	lphs->CurPix = lphs->MaxPix * (abs((short)lphs->CurVal) - abs(lphs->MinVal)) / 
    		(abs(lphs->MaxVal) - abs(lphs->MinVal));
    if (lphs->CurPix > lphs->MaxPix)  lphs->CurPix = lphs->MaxPix;
    hMemDC = CreateCompatibleDC(hDC);
    if (lphs->Direction == WM_VSCROLL) {
	GetObject(hUpArrow, sizeof(BITMAP), (LPSTR)&bm);
	if (lphs->ButtonDown == 1)
	    SelectObject(hMemDC, hUpArrowD);
	else
	    SelectObject(hMemDC, hUpArrow);
	StretchBlt(hDC, rect.left, rect.top, w, w, hMemDC, 
			0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
	GetObject(hDnArrow, sizeof(BITMAP), (LPSTR)&bm);
	if (lphs->ButtonDown == 2)
	    SelectObject(hMemDC, hDnArrowD);
	else
	    SelectObject(hMemDC, hDnArrow);
	StretchBlt(hDC, rect.left, rect.bottom - w, w, w, hMemDC, 
			0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
	rect.top += w;
	rect.bottom -= w;
	}
    else {
	GetObject(hLfArrow, sizeof(BITMAP), (LPSTR)&bm);
	if (lphs->ButtonDown == 3)
	    SelectObject(hMemDC, hLfArrowD);
	else
	    SelectObject(hMemDC, hLfArrow);
	StretchBlt(hDC, rect.left, rect.top, h, h, hMemDC, 
			0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
	GetObject(hRgArrow, sizeof(BITMAP), (LPSTR)&bm);
	if (lphs->ButtonDown == 4)
	    SelectObject(hMemDC, hRgArrowD);
	else
	    SelectObject(hMemDC, hRgArrow);
	StretchBlt(hDC, rect.right - h, rect.top, h, h, hMemDC, 
			0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
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
    width = wndPtr->rectClient.right - wndPtr->rectClient.left;
    height = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
    if (width <= height)
	lphs = AllocScrollBar(WS_VSCROLL, width, height);
    else
	lphs = AllocScrollBar(WS_HSCROLL, width, height);
#ifdef DEBUG_SCROLL
        printf("CreateScrollBarStruct %lX !\n", lphs);
#endif
    *((LPHEADSCROLL *)&wndPtr->wExtra[1]) = lphs;
    lphs->hWndOwner = hWnd;
    CopyRect(&lphs->rect, &wndPtr->rectClient);
    return TRUE;
}



LPHEADSCROLL AllocScrollBar(DWORD dwStyle, int width, int height)
{
    LPHEADSCROLL lphs;
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
 *			GetScrollObjectStruct [internal]
 */
LPHEADSCROLL GetScrollObjectStruct(HWND hWnd, int nBar)
{
    WND *wndPtr;
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
	LPHEADSCROLL lphs;
	HDC		hDC;
	int 	nRet;
	lphs = GetScrollObjectStruct(hWnd, nBar);
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
		if (nBar == SB_CTL) {
	        InvalidateRect(lphs->hWndOwner, &lphs->rect, TRUE);
	        UpdateWindow(lphs->hWndOwner);
			}
		else {
			if (lphs->rect.right != 0 && lphs->rect.bottom != 0) {
				hDC = GetWindowDC(lphs->hWndOwner);
				StdDrawScrollBar(lphs->hWndOwner, hDC, nBar, &lphs->rect, lphs);
				ReleaseDC(lphs->hWndOwner, hDC);
				}
			}
        }
    return nRet;
}



/*************************************************************************
 *			GetScrollPos [USER.63]
 */
int GetScrollPos(HWND hWnd, int nBar)
{
    LPHEADSCROLL lphs;
    lphs = GetScrollObjectStruct(hWnd, nBar);
    if (lphs == NULL) return 0;
    return lphs->CurVal;
}



/*************************************************************************
 *			SetScrollRange [USER.64]
 */
void SetScrollRange(HWND hWnd, int nBar, int MinPos, int MaxPos, BOOL bRedraw)
{
    LPHEADSCROLL lphs;
	HDC		hDC;
    lphs = GetScrollObjectStruct(hWnd, nBar);
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
		if (nBar == SB_CTL) {
	        InvalidateRect(lphs->hWndOwner, &lphs->rect, TRUE);
	        UpdateWindow(lphs->hWndOwner);
			}
		else {
			if (lphs->rect.right != 0 && lphs->rect.bottom != 0) {
				hDC = GetWindowDC(lphs->hWndOwner);
				StdDrawScrollBar(lphs->hWndOwner, hDC, nBar, &lphs->rect, lphs);
				ReleaseDC(lphs->hWndOwner, hDC);
				}
			}
        }
}



/*************************************************************************
 *			GetScrollRange [USER.65]
 */
void GetScrollRange(HWND hWnd, int nBar, LPINT lpMin, LPINT lpMax)
{
    LPHEADSCROLL lphs;
    lphs = GetScrollObjectStruct(hWnd, nBar);
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
/*
    if ((wBar == SB_VERT) || (wBar == SB_BOTH)) {
    	if (bFlag)
	    wndPtr->dwStyle |= WS_VSCROLL;
	else
	    wndPtr->dwStyle &= 0xFFFFFFFFL ^ WS_VSCROLL;
	}
    if ((wBar == SB_HORZ) || (wBar == SB_BOTH)) {
    	if (bFlag)
	    wndPtr->dwStyle |= WS_HSCROLL;
	else
	    wndPtr->dwStyle &= 0xFFFFFFFFL ^ WS_HSCROLL;
	}
*/
}


