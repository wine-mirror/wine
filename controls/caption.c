/*
 * Interface code to CAPTION widget
 *
 * Copyright  Martin Ayotte, 1994
 *
 */

/*
#define DEBUG_CAPTION
*/

static char Copyright[] = "Copyright Martin Ayotte, 1994";

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "windows.h"
#include "caption.h"
#include "heap.h"
#include "win.h"
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

HBITMAP hStdClose = (HBITMAP)NULL;
HBITMAP hStdCloseD = (HBITMAP)NULL;
HBITMAP hStdMinim = (HBITMAP)NULL;
HBITMAP hStdMinimD = (HBITMAP)NULL;
HBITMAP hStdMaxim = (HBITMAP)NULL;
HBITMAP hStdMaximD = (HBITMAP)NULL;
HMENU hStdSysMenu = (HMENU)NULL;

LPHEADCAPTION CaptionBarGetWindowAndStorage(HWND hWnd, WND **wndPtr);
LPHEADCAPTION CaptionBarGetStorageHeader(HWND hWnd);
void SetMenuLogicalParent(HMENU hMenu, HWND hWnd);


/***********************************************************************
 *           CaptionBarWndProc
 */
LONG CaptionBarWndProc( HWND hWnd, WORD message, WORD wParam, LONG lParam )
{    
    WORD	wRet;
    short	x, y;
    short	width, height;
    WND  	*wndPtr;
    LPHEADCAPTION lphs;
    PAINTSTRUCT	ps;
    HDC		hDC;
    HDC		hMemDC;
    BITMAP	bm;
    RECT 	rect;
    char	str[128];
    switch(message)
    {
    case WM_CREATE:
	wndPtr = WIN_FindWndPtr(hWnd);
	lphs = (LPHEADCAPTION)malloc(sizeof(HEADCAPTION));
	if (lphs == 0) {
	    printf("Bad Memory Alloc on CAPTIONBAR !\n");
	    return 0;
	    }
	memset(lphs, 0, sizeof(HEADCAPTION));
#ifdef DEBUG_CAPTION
 	printf("CreateCaptionBarStruct %lX !\n", lphs);
#endif
	*((LPHEADCAPTION *)&wndPtr->wExtra[1]) = lphs;
	if (hStdClose == (HBITMAP)NULL) 
	    hStdClose = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_CLOSE));
	lphs->hClose = hStdClose;
	if (hStdMinim == (HBITMAP)NULL) 
	    hStdMinim = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_REDUCE));
	lphs->hMinim = hStdMinim;
	if (hStdMaxim == (HBITMAP)NULL) 
	    hStdMaxim = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_RESTORE));
	lphs->hMaxim = hStdMaxim;
	if (hStdCloseD == (HBITMAP)NULL) 
	    hStdCloseD = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_CLOSE));
	if (hStdMinimD == (HBITMAP)NULL) 
	    hStdMinimD = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_REDUCED));
	if (hStdMaximD == (HBITMAP)NULL) 
	    hStdMaximD = LoadBitmap((HINSTANCE)NULL, MAKEINTRESOURCE(OBM_RESTORED));
	if (hStdSysMenu == (HBITMAP)NULL) 
	    hStdSysMenu = LoadMenu((HINSTANCE)NULL, "SYSMENU");
	lphs->hSysMenu = hStdSysMenu;
 	printf("CaptionBar SYSMENU %04X !\n", lphs->hSysMenu);
	if (lphs->hSysMenu == 0) lphs->hSysMenu = CreatePopupMenu();
	AppendMenu(lphs->hSysMenu, MF_STRING, 9999, "About &Wine ...");
	GetClientRect(hWnd, &rect);
	CopyRect(&lphs->rectClose, &rect);
	CopyRect(&lphs->rectMaxim, &rect);
	lphs->rectClose.right = lphs->rectClose.left +
		 lphs->rectClose.bottom + lphs->rectClose.top;
	lphs->rectMaxim.left = lphs->rectMaxim.right -
		 lphs->rectMaxim.bottom + lphs->rectMaxim.top;
	CopyRect(&lphs->rectMinim, &lphs->rectMaxim);
	if (lphs->hMaxim != 0) {
	    lphs->rectMinim.left = lphs->rectMaxim.bottom + lphs->rectMaxim.top;
	    lphs->rectMinim.right = lphs->rectMaxim.bottom + lphs->rectMaxim.top;
	    }
	if (lphs->hClose == 0) lphs->rectClose.right = lphs->rectClose.left;
	printf("CAPTION Close.right=%d Maxim.left=%d Minim.left=%d !\n",
	    lphs->rectClose.right, lphs->rectMaxim.left, lphs->rectMinim.left);
	return 0;
    case WM_DESTROY:
	lphs = CaptionBarGetWindowAndStorage(hWnd, &wndPtr);
	if (lphs == 0) return 0;
#ifdef DEBUG_CAPTION
        printf("CaptionBar WM_DESTROY %lX !\n", lphs);
#endif
	DestroyMenu(lphs->hSysMenu);
	free(lphs);
	*((LPHEADCAPTION *)&wndPtr->wExtra[1]) = 0;
	return 0;
    case WM_COMMAND:
#ifdef DEBUG_CAPTION
    	printf("CaptionBar WM_COMMAND %04X %08X !\n", wParam, lParam);
#endif
	lphs = CaptionBarGetWindowAndStorage(hWnd, &wndPtr);
    	if (wParam == 9999) {
	    printf("CaptionBar Show 'About Wine ...' !\n");
	    }
	SendMessage(wndPtr->hwndParent, message, wParam, lParam);
	break;
    case WM_SIZE:
	lphs = CaptionBarGetWindowAndStorage(hWnd, &wndPtr);
	width  = LOWORD(lParam);
	height = HIWORD(lParam);
	if (lphs->hClose != 0)
	    SetRect(&lphs->rectClose, 0, 0, height, height);
	if (lphs->hMinim != 0) {
	    if (lphs->hMaxim != 0)
		SetRect(&lphs->rectMinim, width - 2 * height, 0, 
				width - height, height);
	    else
		SetRect(&lphs->rectMinim, width - height, 0, width, height);
	    }
	if (lphs->hMaxim != 0)
	    SetRect(&lphs->rectMaxim, width - height, 0, width, height);
	break;
    case WM_LBUTTONDOWN:
	lphs = CaptionBarGetWindowAndStorage(hWnd, &wndPtr);
	SetCapture(hWnd);
	x = LOWORD(lParam);
	y = HIWORD(lParam);
	hDC = GetDC(hWnd);
	if (x > lphs->rectClose.left && x < lphs->rectClose.right) {
	    lphs->hClose = hStdCloseD;
	    InvalidateRect(hWnd, &lphs->rectClose, TRUE);
	    UpdateWindow(hWnd);
	    }
	if (x > lphs->rectMinim.left && x < lphs->rectMinim.right) {
	    lphs->hMinim = hStdMinimD;
	    InvalidateRect(hWnd, &lphs->rectMinim, TRUE);
	    UpdateWindow(hWnd);
	    }
	if (x > lphs->rectMaxim.left && x < lphs->rectMaxim.right &&
	    lphs->hMaxim != 0) {
	    lphs->hMaxim = hStdMaximD;
	    InvalidateRect(hWnd, &lphs->rectMaxim, TRUE);
	    UpdateWindow(hWnd);
	    }
	ReleaseDC(hWnd, hDC);
	break;
    case WM_LBUTTONUP:
	lphs = CaptionBarGetWindowAndStorage(hWnd, &wndPtr);
	ReleaseCapture();
#ifdef DEBUG_CAPTION
        printf("CaptionBar WM_LBUTTONUP %lX !\n", lParam);
#endif
	x = LOWORD(lParam);
	y = HIWORD(lParam);
	if (x > lphs->rectClose.left && x < lphs->rectClose.right) {
	    lphs->hClose = hStdClose;
	    InvalidateRect(hWnd, &lphs->rectClose, TRUE);
	    UpdateWindow(hWnd);
	    TrackPopupMenu(lphs->hSysMenu, TPM_LEFTBUTTON, 0, -20, 
		0, wndPtr->hwndParent, (LPRECT)NULL);
	    SetMenuLogicalParent(lphs->hSysMenu, hWnd);
	    printf("CAPTION Pop the SYSMENU !\n");
	    break;
	    }
	if (x > lphs->rectMinim.left && x < lphs->rectMinim.right) {
	    SendMessage(wndPtr->hwndParent, WM_SYSCOMMAND, SC_MINIMIZE, 0L);
	    lphs->hMinim = hStdMinim;
	    InvalidateRect(hWnd, &lphs->rectMinim, TRUE);
	    UpdateWindow(hWnd);
	    printf("CAPTION Minimize Window !\n");
	    break;
	    }
	if (x > lphs->rectMaxim.left && x < lphs->rectMaxim.right) {
	    lphs->hMaxim = hStdMaxim;
	    InvalidateRect(hWnd, &lphs->rectMaxim, TRUE);
	    UpdateWindow(hWnd);
	    SendMessage(wndPtr->hwndParent, WM_SYSCOMMAND, SC_MAXIMIZE, 0L);
	    printf("CAPTION Maximize Window !\n");
	    break;
	    }
	break;

    case WM_LBUTTONDBLCLK:
	lphs = CaptionBarGetWindowAndStorage(hWnd, &wndPtr);
#ifdef DEBUG_CAPTION
        printf("CaptionBar WM_LBUTTONDBLCLK %lX !\n", lParam);
#endif
	x = LOWORD(lParam);
	y = HIWORD(lParam);
	if (x > lphs->rectClose.left && x < lphs->rectClose.right) {
	    SendMessage(wndPtr->hwndParent, WM_SYSCOMMAND, SC_CLOSE, 0L);
	    printf("CAPTION DoubleClick Close Window !\n");
	    break;
	    }
	break;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
	lphs = CaptionBarGetWindowAndStorage(hWnd, &wndPtr);
	return(SendMessage(wndPtr->hwndParent, message, wParam, lParam));

    case WM_PAINT:
	GetWindowRect(hWnd, &rect);
#ifdef DEBUG_CAPTION
 	printf("CaptionBar WM_PAINT left=%d top=%d right=%d bottom=%d !\n",
 		rect.left, rect.top, rect.right, rect.bottom);
#endif
	lphs = CaptionBarGetWindowAndStorage(hWnd, &wndPtr);
	hDC = BeginPaint(hWnd, &ps);
	hMemDC = CreateCompatibleDC(hDC);
	if (lphs->hClose != 0) {
	    GetObject(lphs->hClose, sizeof(BITMAP), (LPSTR)&bm);
	    SelectObject(hMemDC, lphs->hClose);
	    BitBlt(hDC, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	    }
	if (lphs->hMinim != 0) {
	    GetObject(lphs->hMinim, sizeof(BITMAP), (LPSTR)&bm);
	    SelectObject(hMemDC, lphs->hMinim);
	    BitBlt(hDC, lphs->rectMinim.left, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	    }
	if (lphs->hMaxim != 0) {
	    GetObject(lphs->hMaxim, sizeof(BITMAP), (LPSTR)&bm);
	    SelectObject(hMemDC, lphs->hMaxim);
	    BitBlt(hDC, lphs->rectMaxim.left, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
	    }
	DeleteDC(hMemDC);
	GetClientRect(hWnd, &rect);
	FrameRect(hDC, &rect, GetStockObject(BLACK_BRUSH));
	rect.left = lphs->rectClose.right;
	rect.right = lphs->rectMinim.left;
#ifdef DEBUG_CAPTION
 	printf("CaptionBar WM_PAINT left=%d top=%d right=%d bottom=%d !\n",
 		rect.left, rect.top, rect.right, rect.bottom);
#endif
	FillRect(hDC, &rect, GetStockObject(GRAY_BRUSH));
	if (GetWindowTextLength(wndPtr->hwndParent) > 0) {
	    GetWindowText(wndPtr->hwndParent, str, sizeof(str));
	    width = GetTextExtent(hDC, str, strlen(str));
	    DrawText(hDC, str, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	    }
	EndPaint(hWnd, &ps);
	break;
    default:
	return DefWindowProc( hWnd, message, wParam, lParam );
    }
return(0);
}



LPHEADCAPTION CaptionBarGetWindowAndStorage(HWND hWnd, WND **wndPtr)
{
    WND  *Ptr;
    LPHEADCAPTION lphs;
    *(wndPtr) = Ptr = WIN_FindWndPtr(hWnd);
    if (Ptr == 0) {
    	printf("Bad Window handle on CaptionBar !\n");
    	return 0;
    	}
    lphs = *((LPHEADCAPTION *)&Ptr->wExtra[1]);
    return lphs;
}


LPHEADCAPTION CaptionBarGetStorageHeader(HWND hWnd)
{
    WND  *wndPtr;
    LPHEADCAPTION lphs;
    wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == 0) {
    	printf("Bad Window handle on CaptionBar !\n");
    	return 0;
    	}
    lphs = *((LPHEADCAPTION *)&wndPtr->wExtra[1]);
    return lphs;
}


