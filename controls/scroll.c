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
#include "dirent.h"
#include <sys/stat.h>

LPHEADSCROLL ScrollBarGetWindowAndStorage(HWND hwnd, WND **wndPtr);
LPHEADSCROLL ScrollBarGetStorageHeader(HWND hwnd);
void StdDrawScrollBar(HWND hwnd);
int CreateScrollBarStruct(HWND hwnd);


void SCROLLBAR_CreateScrollBar(LPSTR className, LPSTR scrollLabel, HWND hwnd)
{
    WND *wndPtr    = WIN_FindWndPtr(hwnd);
    WND *parentPtr = WIN_FindWndPtr(wndPtr->hwndParent);
    DWORD style;
    char widgetName[15];

#ifdef DEBUG_SCROLLBAR
    printf("scroll: label = %s, x = %d, y = %d\n", scrollLabel,
	   wndPtr->rectClient.left, wndPtr->rectClient.top);
    printf("        width = %d, height = %d\n",
	   wndPtr->rectClient.right - wndPtr->rectClient.left,
	   wndPtr->rectClient.bottom - wndPtr->rectClient.top);
#endif

    if (!wndPtr)
	return;

    style = wndPtr->dwStyle & 0x0000FFFF;
/*
    if ((style & SBS_NOTIFY) == SBS_NOTIFY)
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
 *           WIDGETS_ScrollBarWndProc
 */
LONG SCROLLBAR_ScrollBarWndProc( HWND hwnd, WORD message,
			   WORD wParam, LONG lParam )
{    
    WORD	wRet;
    short	x, y;
    WND  	*wndPtr;
    LPHEADSCROLL lphs;
    RECT rect;
    static RECT rectsel;
    switch(message)
    {
    case WM_CREATE:
	CreateScrollBarStruct(hwnd);
#ifdef DEBUG_SCROLL
        printf("ScrollBar Creation up=%X down=%X!\n", lphs->hWndUp, lphs->hWndDown);
#endif
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

    case WM_KEYDOWN:
        printf("ScrollBar WM_KEYDOWN wParam %X !\n", wParam);
	break;
    case WM_PAINT:
	StdDrawScrollBar(hwnd);
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
	    printf("WM_MOUSEMOVE val=%d pix=%d\n", x, y);
#endif
            SendMessage(wndPtr->hwndParent, lphs->Direction, 
            		SB_THUMBTRACK, MAKELONG(x, hwnd));
	    }
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
        	WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        	0, 0, width, width, hwnd, 1, wndPtr->hInstance, 0L);
	lphs->hWndDown = CreateWindow("BUTTON", "", 
        	WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        	0, height - width, width, width, hwnd, 2,
        	wndPtr->hInstance, 0L);
	}
    else
	{
	lphs->MaxPix = width - 3 * height;
	lphs->Direction = WM_HSCROLL;
	lphs->hWndUp = CreateWindow("BUTTON", "", 
        	WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        	0, 0, height, height, hwnd, 0, wndPtr->hInstance, 0L);
	lphs->hWndDown = CreateWindow("BUTTON", "", 
        	WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        	width - height, 0, height, height, hwnd, 0,
        	wndPtr->hInstance, 0L);
	}
    if (lphs->MaxPix < 1)  lphs->MaxPix = 1;
    return TRUE;
}



int GetScrollPos(HWND hwnd, int nBar)
{
    LPHEADSCROLL lphs;
    lphs = ScrollBarGetStorageHeader(hwnd);
    if (lphs == NULL) return 0;
    return lphs->CurVal;
}



void GetScrollRange(HWND hwnd, int nBar, LPINT lpMin, LPINT lpMax)
{
    LPHEADSCROLL lphs;
    lphs = ScrollBarGetStorageHeader(hwnd);
    if (lphs == NULL) return;
    *lpMin = lphs->MinVal;
    *lpMax = lphs->MaxVal;
}



int SetScrollPos(HWND hwnd, int nBar, int nPos, BOOL bRedraw)
{
    int nRet;
    LPHEADSCROLL lphs;
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



void SetScrollRange(HWND hwnd, int nBar, int MinPos, int MaxPos, BOOL bRedraw)
{
    LPHEADSCROLL lphs;
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





