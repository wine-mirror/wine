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

LPHEADSCROLL ScrollBarGetWindowAndStorage(HWND hwnd, WND **wndPtr);
LPHEADSCROLL ScrollBarGetStorageHeader(HWND hwnd);
void StdDrawScrollBar(HWND hwnd);
int CreateScrollBarStruct(HWND hwnd);


/***********************************************************************
 *           WIDGETS_ScrollBarWndProc
 */
LONG ScrollBarWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{    
    WORD	wRet;
    short	x, y;
    short	width, height;
    WND  	*wndPtr;
    LPHEADSCROLL lphs;
    LPDRAWITEMSTRUCT lpdis;
    HDC		hMemDC;
    BITMAP	bm;
    RECT 	rect;
    static RECT rectsel;
    switch(message)
    {
    case WM_CREATE:
	CreateScrollBarStruct(hwnd);
#ifdef DEBUG_SCROLL
        printf("ScrollBar Creation up=%X down=%X!\n", lphs->hWndUp, lphs->hWndDown);
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
	lphs = ScrollBarGetWindowAndStorage(hwnd, &wndPtr);
	if (lphs == 0) return 0;
#ifdef DEBUG_SCROLL
        printf("ScrollBar WM_DESTROY %lX !\n", lphs);
#endif
	DestroyWindow(lphs->hWndUp);
	DestroyWindow(lphs->hWndDown);
	free(lphs);
	*((LPHEADSCROLL *)&wndPtr->wExtra[1]) = 0;
	return 0;
	
    case WM_COMMAND:
#ifdef DEBUG_SCROLL
        printf("ScrollBar WM_COMMAND wParam=%X lParam=%lX !\n", wParam, lParam);
#endif
	lphs = ScrollBarGetWindowAndStorage(hwnd, &wndPtr);
	if (HIWORD(lParam) != BN_CLICKED) return 0;
        if (LOWORD(lParam) == lphs->hWndUp)
            SendMessage(wndPtr->hwndParent, lphs->Direction, 
            	SB_LINEUP, MAKELONG(0, hwnd));
        if (LOWORD(lParam) == lphs->hWndDown)
            SendMessage(wndPtr->hwndParent, lphs->Direction, 
            	SB_LINEDOWN, MAKELONG(0, hwnd));
/*
	SetFocus(hwnd);
*/
	return 0;

    case WM_LBUTTONDOWN:
	lphs = ScrollBarGetWindowAndStorage(hwnd, &wndPtr);
/*
	SetFocus(hwnd);
*/
	SetCapture(hwnd);
	GetClientRect(hwnd, &rect);
	if (lphs->Direction == WM_VSCROLL) {
	    y = HIWORD(lParam);
#ifdef DEBUG_SCROLL
	    printf("WM_LBUTTONDOWN y=%d cur+right=%d %d\n", 
	    	y, lphs->CurPix + rect.right, lphs->CurPix + (rect.right << 1));
#endif
	    if (y < (lphs->CurPix + rect.right)) 
	        SendMessage(wndPtr->hwndParent, lphs->Direction, 
	        	SB_PAGEUP, MAKELONG(0, hwnd));
	    if (y > (lphs->CurPix + (rect.right << 1))) 
	        SendMessage(wndPtr->hwndParent, lphs->Direction, 
	        	SB_PAGEDOWN, MAKELONG(0, hwnd));
	    if ((y > (lphs->CurPix + rect.right)) &&
	        (y < (lphs->CurPix + (rect.right << 1)))) {
	        lphs->ThumbActive = TRUE;
#ifdef DEBUG_SCROLL
	        printf("THUMB DOWN !\n");
#endif
	        }
	    }
	else {
	    x = LOWORD(lParam);
#ifdef DEBUG_SCROLL
	    printf("WM_LBUTTONDOWN x=%d Cur+bottom=%d %d\n",
	    	 x, lphs->CurPix + rect.bottom, lphs->CurPix + (rect.bottom << 1));
#endif
	    if (x < (lphs->CurPix + rect.bottom))
	        SendMessage(wndPtr->hwndParent, lphs->Direction, 
	        	SB_PAGEUP, MAKELONG(0, hwnd));
	    if (x > (lphs->CurPix + (rect.bottom << 1)))
	        SendMessage(wndPtr->hwndParent, lphs->Direction, 
	        	SB_PAGEDOWN, MAKELONG(0, hwnd));
	    if ((x > (lphs->CurPix + rect.bottom)) &&
		(x < (lphs->CurPix + (rect.bottom << 1)))) {
	        lphs->ThumbActive = TRUE;
#ifdef DEBUG_SCROLL
	        printf("THUMB DOWN !\n");
#endif
		}
	    }
	break;
    case WM_LBUTTONUP:
	lphs = ScrollBarGetStorageHeader(hwnd);
        lphs->ThumbActive = FALSE;
	ReleaseCapture();
	break;

    case WM_MOUSEMOVE:
        if ((wParam & MK_LBUTTON) != 0) {
	    lphs = ScrollBarGetWindowAndStorage(hwnd, &wndPtr);
	    if (lphs->ThumbActive == 0) break;
	    GetClientRect(hwnd, &rect);
	    if (lphs->Direction == WM_VSCROLL)
		y = HIWORD(lParam) - rect.right - (rect.right >> 1);
	    else
		y = LOWORD(lParam) - rect.bottom - (rect.bottom >> 1);
	    x = (y * (lphs->MaxVal - lphs->MinVal) / 
	    		lphs->MaxPix) + lphs->MinVal;
#ifdef DEBUG_SCROLL
	    printf("Scroll WM_MOUSEMOVE val=%d pix=%d\n", x, y);
#endif
            SendMessage(wndPtr->hwndParent, lphs->Direction, 
            		SB_THUMBTRACK, MAKELONG(x, hwnd));
	    }
	break;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
	lphs = ScrollBarGetWindowAndStorage(hwnd, &wndPtr);
	return(SendMessage(wndPtr->hwndParent, message, wParam, lParam));

    case WM_SIZE:
	lphs = ScrollBarGetWindowAndStorage(hwnd, &wndPtr);
	width  = LOWORD(lParam);
	height = HIWORD(lParam);
	if (lphs->Direction == WM_VSCROLL) {
	    MoveWindow(lphs->hWndUp, 0, 0, width, width, TRUE);
	    MoveWindow(lphs->hWndDown, 0, height - width, width, width, TRUE);
	    }
	else {
	    MoveWindow(lphs->hWndUp, 0, 0, height, height, TRUE);
	    MoveWindow(lphs->hWndDown, width - height, 0, height, height, TRUE);
	    }
	break;
    case WM_DRAWITEM:
#ifdef DEBUG_SCROLL
	    printf("Scroll WM_DRAWITEM w=%04X l=%08X\n", wParam, lParam);
#endif
        lpdis = (LPDRAWITEMSTRUCT)lParam;
	if (lpdis->CtlType == ODT_BUTTON && lpdis->itemAction == ODA_DRAWENTIRE) {
	    hMemDC = CreateCompatibleDC(lpdis->hDC);
	    if (lpdis->CtlID == 1) {
		GetObject(hUpArrow, sizeof(BITMAP), (LPSTR)&bm);
		SelectObject(hMemDC, hUpArrow);
		BitBlt(lpdis->hDC, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
		}
	    if (lpdis->CtlID == 2) {
		GetObject(hDnArrow, sizeof(BITMAP), (LPSTR)&bm);
		SelectObject(hMemDC, hDnArrow);
		BitBlt(lpdis->hDC, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
		}
	    if (lpdis->CtlID == 3) {
		GetObject(hLfArrow, sizeof(BITMAP), (LPSTR)&bm);
		SelectObject(hMemDC, hLfArrow);
		BitBlt(lpdis->hDC, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
		}
	    if (lpdis->CtlID == 4) {
		GetObject(hRgArrow, sizeof(BITMAP), (LPSTR)&bm);
		SelectObject(hMemDC, hRgArrow);
		BitBlt(lpdis->hDC, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
		}
	    DeleteDC(hMemDC);
	    }
	if (lpdis->CtlType == ODT_BUTTON && lpdis->itemAction == ODA_SELECT) {
	    hMemDC = CreateCompatibleDC(lpdis->hDC);
	    if (lpdis->CtlID == 1) {
		GetObject(hUpArrowD, sizeof(BITMAP), (LPSTR)&bm);
		SelectObject(hMemDC, hUpArrowD);
		BitBlt(lpdis->hDC, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
		}
	    if (lpdis->CtlID == 2) {
		GetObject(hDnArrowD, sizeof(BITMAP), (LPSTR)&bm);
		SelectObject(hMemDC, hDnArrowD);
		BitBlt(lpdis->hDC, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
		}
	    if (lpdis->CtlID == 3) {
		GetObject(hLfArrowD, sizeof(BITMAP), (LPSTR)&bm);
		SelectObject(hMemDC, hLfArrowD);
		BitBlt(lpdis->hDC, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
		}
	    if (lpdis->CtlID == 4) {
		GetObject(hRgArrowD, sizeof(BITMAP), (LPSTR)&bm);
		SelectObject(hMemDC, hRgArrowD);
		BitBlt(lpdis->hDC, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
		}
	    DeleteDC(hMemDC);
	    }
	break;
    case WM_PAINT:
	StdDrawScrollBar(hwnd);
	break;
    default:
	return DefWindowProc( hwnd, message, wParam, lParam );
    }
return(0);
}



LPHEADSCROLL ScrollBarGetWindowAndStorage(HWND hwnd, WND **wndPtr)
{
    WND  *Ptr;
    LPHEADSCROLL lphs;
    *(wndPtr) = Ptr = WIN_FindWndPtr(hwnd);
    if (Ptr == 0) {
    	printf("Bad Window handle on ScrollBar !\n");
    	return 0;
    	}
    lphs = *((LPHEADSCROLL *)&Ptr->wExtra[1]);
    return lphs;
}


LPHEADSCROLL ScrollBarGetStorageHeader(HWND hwnd)
{
    WND  *wndPtr;
    LPHEADSCROLL lphs;
    wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr == 0) {
    	printf("Bad Window handle on ScrollBar !\n");
    	return 0;
    	}
    lphs = *((LPHEADSCROLL *)&wndPtr->wExtra[1]);
    return lphs;
}


void StdDrawScrollBar(HWND hwnd)
{
    LPHEADSCROLL lphs;
    PAINTSTRUCT ps;
    HBRUSH hBrush;
    HDC hdc;
    RECT rect;
    UINT  i, w, h, siz;
    char	C[128];
    hdc = BeginPaint( hwnd, &ps );
    if (!IsWindowVisible(hwnd)) {
	EndPaint( hwnd, &ps );
	return;
	}
    hBrush = SendMessage(GetParent(hwnd), WM_CTLCOLOR, (WORD)hdc,
			MAKELONG(hwnd, CTLCOLOR_SCROLLBAR));
    if (hBrush == (HBRUSH)NULL)  hBrush = GetStockObject(LTGRAY_BRUSH);
    lphs = ScrollBarGetStorageHeader(hwnd);
    if (lphs == NULL) goto EndOfPaint;
    GetClientRect(hwnd, &rect);
    w = rect.right - rect.left;
    h = rect.bottom - rect.top;
    if (lphs->Direction == WM_VSCROLL) {
	rect.top += w;
	rect.bottom -= w;
	}
    else {
	rect.left += h;
	rect.right -= h;
	}
    FillRect(hdc, &rect, hBrush);
    if (lphs->Direction == WM_VSCROLL)
	SetRect(&rect, 0, lphs->CurPix + w, w, lphs->CurPix + (w << 1));
    else
	SetRect(&rect, lphs->CurPix + h, 0, lphs->CurPix + (h << 1), h);
    FrameRect(hdc, &rect, GetStockObject(BLACK_BRUSH));
    InflateRect(&rect, -1, -1);
    FillRect(hdc, &rect, GetStockObject(LTGRAY_BRUSH));
    DrawReliefRect(hdc, rect, 2, 0);
    InflateRect(&rect, -3, -3);
    DrawReliefRect(hdc, rect, 1, 1);
    if (!lphs->ThumbActive) {
	InvalidateRect(lphs->hWndUp, NULL, TRUE);
	UpdateWindow(lphs->hWndUp);
	InvalidateRect(lphs->hWndDown, NULL, TRUE);
	UpdateWindow(lphs->hWndDown);
	}
EndOfPaint:
    EndPaint( hwnd, &ps );
}



int CreateScrollBarStruct(HWND hwnd)
{
    RECT	rect;
    int		width, height;
    WND  *wndPtr;
    LPHEADSCROLL lphs;
    wndPtr = WIN_FindWndPtr(hwnd);
    lphs = (LPHEADSCROLL)malloc(sizeof(HEADSCROLL));
    if (lphs == 0) {
    	printf("Bad Memory Alloc on ScrollBar !\n");
    	return 0;
    	}

#ifdef DEBUG_SCROLL
        printf("CreateScrollBarStruct %lX !\n", lphs);
#endif
    *((LPHEADSCROLL *)&wndPtr->wExtra[1]) = lphs;
    lphs->ThumbActive = FALSE;
    lphs->MinVal = 0;
    lphs->MaxVal = 100;
    lphs->CurVal = 0;
    lphs->CurPix = 0;
    width = wndPtr->rectClient.right - wndPtr->rectClient.left;
    height = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
    if (width <= height)
	{
	lphs->MaxPix = height - 3 * width;
	lphs->Direction = WM_VSCROLL;
	lphs->hWndUp = CreateWindow("BUTTON", "", 
        	WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        	0, 0, width, width, hwnd, 1, wndPtr->hInstance, 0L);
	lphs->hWndDown = CreateWindow("BUTTON", "", 
        	WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        	0, height - width, width, width, hwnd, 2,
        	wndPtr->hInstance, 0L);
	}
    else
	{
	lphs->MaxPix = width - 3 * height;
	lphs->Direction = WM_HSCROLL;
	lphs->hWndUp = CreateWindow("BUTTON", "", 
        	WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        	0, 0, height, height, hwnd, 3, wndPtr->hInstance, 0L);
	lphs->hWndDown = CreateWindow("BUTTON", "", 
        	WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        	width - height, 0, height, height, hwnd, 4,
        	wndPtr->hInstance, 0L);
	}
    if (lphs->MaxPix < 1)  lphs->MaxPix = 1;
    if (wndPtr->hCursor == (HCURSOR)NULL)
	wndPtr->hCursor = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
    return TRUE;
}


/*************************************************************************
 *			GetScrollWindowHandle
 */
HWND GetScrollWindowHandle(HWND hWnd, int nBar)
{
    WND *wndPtr;
    if (nBar != SB_CTL) {
    wndPtr = WIN_FindWndPtr(hWnd);
    	if (nBar == SB_VERT) return wndPtr->hWndVScroll;
    	if (nBar == SB_HORZ) return wndPtr->hWndHScroll;
    	return (HWND)NULL;
	}
    return hWnd;
}


/*************************************************************************
 *			SetScrollPos [USER.62]
 */
int SetScrollPos(HWND hwnd, int nBar, int nPos, BOOL bRedraw)
{
    int nRet;
    LPHEADSCROLL lphs;
    hwnd = GetScrollWindowHandle(hwnd, nBar);
    lphs = ScrollBarGetStorageHeader(hwnd);
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
    if ((bRedraw) && (IsWindowVisible(hwnd))) {
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        }
    return nRet;
}



/*************************************************************************
 *			GetScrollPos [USER.63]
 */
int GetScrollPos(HWND hwnd, int nBar)
{
    LPHEADSCROLL lphs;
    hwnd = GetScrollWindowHandle(hwnd, nBar);
    lphs = ScrollBarGetStorageHeader(hwnd);
    if (lphs == NULL) return 0;
    return lphs->CurVal;
}



/*************************************************************************
 *			SetScrollRange [USER.64]
 */
void SetScrollRange(HWND hwnd, int nBar, int MinPos, int MaxPos, BOOL bRedraw)
{
    LPHEADSCROLL lphs;
    hwnd = GetScrollWindowHandle(hwnd, nBar);
    lphs = ScrollBarGetStorageHeader(hwnd);
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
    if ((bRedraw) && (IsWindowVisible(hwnd))) {
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        }
}



/*************************************************************************
 *			GetScrollRange [USER.65]
 */
void GetScrollRange(HWND hwnd, int nBar, LPINT lpMin, LPINT lpMax)
{
    LPHEADSCROLL lphs;
    hwnd = GetScrollWindowHandle(hwnd, nBar);
    lphs = ScrollBarGetStorageHeader(hwnd);
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
    	if (bFlag)
	    ShowWindow(wndPtr->hWndVScroll, SW_SHOW);
	else
	    ShowWindow(wndPtr->hWndVScroll, SW_HIDE);
	}
    if ((wBar == SB_HORZ) || (wBar == SB_BOTH)) {
    	if (bFlag)
	    ShowWindow(wndPtr->hWndHScroll, SW_SHOW);
	else
	    ShowWindow(wndPtr->hWndHScroll, SW_HIDE);
	}
}


