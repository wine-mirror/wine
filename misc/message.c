/*
 * 'Wine' MessageBox function handling
 *
 * Copyright 1993 Martin Ayotte
 */

static char Copyright[] = "Copyright Martin Ayotte, 1993";

#include "windows.h"
#include "heap.h"
#include "win.h"


typedef struct tagMSGBOX {
    LPSTR	Title;
    LPSTR	Str;
    WORD	wType;
    WORD	wRetVal;
    BOOL	ActiveFlg;
    HWND	hWndYes;
    HWND	hWndNo;
    HWND	hWndCancel;
    HICON	hIcon;
    RECT	rectIcon;
    RECT	rectStr;
} MSGBOX;
typedef MSGBOX FAR* LPMSGBOX;


LONG SystemMessageBoxProc(HWND hwnd, WORD message, WORD wParam, LONG lParam);

/**************************************************************************
 *			MessageBox  [USER.1]
 */

int MessageBox( HWND hWnd, LPSTR str, LPSTR title, WORD type )
{
    HWND    	hDlg;
    WND	    	*wndPtr;
    WNDCLASS  	wndClass;
    MSG	    	msg;
    MSGBOX	mb;
    wndPtr = WIN_FindWndPtr(hWnd);
    printf( "MessageBox: '%s'\n", str );
    wndClass.style           = CS_HREDRAW | CS_VREDRAW ;
    wndClass.lpfnWndProc     = (WNDPROC)SystemMessageBoxProc;
    wndClass.cbClsExtra      = 0;
    wndClass.cbWndExtra      = 0;
    wndClass.hInstance       = wndPtr->hInstance;
    wndClass.hIcon           = (HICON)NULL;
    wndClass.hCursor         = LoadCursor((HANDLE)NULL, IDC_ARROW); 
    wndClass.hbrBackground   = GetStockObject(WHITE_BRUSH);
    wndClass.lpszMenuName    = NULL;
    wndClass.lpszClassName   = "MESSAGEBOX";
    if (!RegisterClass(&wndClass)) return 0;
    memset(&mb, 0, sizeof(MSGBOX));
    mb.Title = title;
    mb.Str = str;
    mb.wType = type;
    mb.ActiveFlg = TRUE;
    hDlg = CreateWindow("MESSAGEBOX", title, 
    	WS_POPUP | WS_DLGFRAME | WS_VISIBLE, 100, 150, 400, 120,
    	(HWND)NULL, (HMENU)NULL, wndPtr->hInstance, (LPSTR)&mb);
    if (hDlg == 0) return 0;
    while(TRUE) {
	if (!mb.ActiveFlg) break;
	if (!GetMessage(&msg, (HWND)NULL, 0, 0)) break;
	TranslateMessage(&msg);
	DispatchMessage(&msg);
	}
    printf( "after MessageBox !\n");
    if (!UnregisterClass("MESSAGEBOX", wndPtr->hInstance)) return 0;
    return(mb.wRetVal);
}



LPMSGBOX MsgBoxGetStorageHeader(HWND hwnd)
{
    WND  *wndPtr;
    LPMSGBOX lpmb;
    wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr == 0) {
    	printf("Bad Window handle on MessageBox !\n");
    	return 0;
    	}
    lpmb = *((LPMSGBOX *)&wndPtr->wExtra[1]);
    return lpmb;
}




LONG SystemMessageBoxProc(HWND hWnd, WORD message, WORD wParam, LONG lParam)
{
    WND	    *wndPtr;
    CREATESTRUCT *createStruct;
    PAINTSTRUCT	ps;
    HDC		hDC;
    RECT	rect;
    LPMSGBOX	lpmb;
    LPMSGBOX	lpmbInit;
    BITMAP	bm;
    HBITMAP	hBitMap;
    HDC		hMemDC;
    HICON	hIcon;
    HINSTANCE	hInst2;
    int		x;
    switch(message) {
	case WM_CREATE:
#ifdef DEBUG_MSGBOX
	    printf("MessageBox WM_CREATE !\n");
#endif
	    wndPtr = WIN_FindWndPtr(hWnd);
	    createStruct = (CREATESTRUCT *)lParam;
     	    lpmbInit = (LPMSGBOX)createStruct->lpCreateParams;
     	    if (lpmbInit == 0) break;
	    *((LPMSGBOX *)&wndPtr->wExtra[1]) = lpmbInit;
	    lpmb = MsgBoxGetStorageHeader(hWnd);
	    GetClientRect(hWnd, &rect);
	    CopyRect(&lpmb->rectStr, &rect);
	    lpmb->rectStr.bottom -= 32;
	    switch(lpmb->wType & MB_TYPEMASK) {
		case MB_OK :
		    lpmb->hWndYes = CreateWindow("BUTTON", "&Ok", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 - 30, rect.bottom - 25, 
			60, 18, hWnd, IDOK, wndPtr->hInstance, 0L);
		    break;
		case MB_OKCANCEL :
		    lpmb->hWndYes = CreateWindow("BUTTON", "&Ok", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 - 65, rect.bottom - 25, 
			60, 18, hWnd, IDOK, wndPtr->hInstance, 0L);
		    lpmb->hWndCancel = CreateWindow("BUTTON", "&Cancel", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 + 5, rect.bottom - 25, 
			60, 18, hWnd, IDCANCEL, wndPtr->hInstance, 0L);
		    break;
		case MB_ABORTRETRYIGNORE :
		    lpmb->hWndYes = CreateWindow("BUTTON", "&Retry", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 - 100, rect.bottom - 25, 
			60, 18, hWnd, IDRETRY, wndPtr->hInstance, 0L);
		    lpmb->hWndNo = CreateWindow("BUTTON", "&Ignore", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 - 30, rect.bottom - 25, 
			60, 18, hWnd, IDIGNORE, wndPtr->hInstance, 0L);
		    lpmb->hWndCancel = CreateWindow("BUTTON", "&Abort", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 + 40, rect.bottom - 25, 
			60, 18, hWnd, IDABORT, wndPtr->hInstance, 0L);
		    break;
		case MB_YESNO :
		    lpmb->hWndYes = CreateWindow("BUTTON", "&Yes", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 - 65, rect.bottom - 25, 
			60, 18, hWnd, IDYES, wndPtr->hInstance, 0L);
		    lpmb->hWndNo = CreateWindow("BUTTON", "&No", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 + 5, rect.bottom - 25, 
			60, 18, hWnd, IDNO, wndPtr->hInstance, 0L);
		    break;
		case MB_YESNOCANCEL :
		    lpmb->hWndYes = CreateWindow("BUTTON", "&Yes", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 - 100, rect.bottom - 25, 
			60, 18, hWnd, IDYES, wndPtr->hInstance, 0L);
		    lpmb->hWndNo = CreateWindow("BUTTON", "&No", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 - 30, rect.bottom - 25, 
			60, 18, hWnd, IDNO, wndPtr->hInstance, 0L);
		    lpmb->hWndCancel = CreateWindow("BUTTON", "&Cancel", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 + 40, rect.bottom - 25, 
			60, 18, hWnd, IDCANCEL, wndPtr->hInstance, 0L);
		    break;
		}
	    switch(lpmb->wType & MB_ICONMASK) {
		case MB_ICONEXCLAMATION:
		    printf("MsgBox LoadIcon Exclamation !\n");
		    lpmb->hIcon = LoadIcon((HINSTANCE)NULL, IDI_EXCLAMATION);
		    break;
		case MB_ICONQUESTION:
		    printf("MsgBox LoadIcon Question !\n");
		    lpmb->hIcon = LoadIcon((HINSTANCE)NULL, IDI_QUESTION);
		    break;
		case MB_ICONASTERISK:
		    printf("MsgBox LoadIcon Asterisk !\n");
		    lpmb->hIcon = LoadIcon((HINSTANCE)NULL, IDI_ASTERISK);
		    break;
		case MB_ICONHAND:
		    printf("MsgBox LoadIcon Hand !\n");
		    lpmb->hIcon = LoadIcon((HINSTANCE)NULL, IDI_HAND);
		    break;
	    	}
	    if (lpmb->hIcon != (HICON)NULL) {
		SetRect(&lpmb->rectIcon, 16,
		    lpmb->rectStr.bottom / 2 - 16, 48,
		    lpmb->rectStr.bottom / 2 + 16);
		lpmb->rectStr.left += 64;
		}
	    break;
	case WM_PAINT:
#ifdef DEBUG_MSGBOX
	    printf("MessageBox WM_PAINT !\n");
#endif
	    lpmb = MsgBoxGetStorageHeader(hWnd);
	    CopyRect(&rect, &lpmb->rectStr);
	    hDC = BeginPaint(hWnd, &ps);
	    if (lpmb->hIcon) 
		DrawIcon(hDC, lpmb->rectIcon.left,
		    lpmb->rectIcon.top, lpmb->hIcon);
	    DrawText(hDC, lpmb->Str, -1, &rect, 
		DT_CALCRECT | DT_CENTER | DT_WORDBREAK);
	    rect.top = lpmb->rectStr.bottom / 2 - rect.bottom / 2;
	    rect.bottom = lpmb->rectStr.bottom / 2 + rect.bottom / 2;
	    DrawText(hDC, lpmb->Str, -1, &rect, DT_CENTER | DT_WORDBREAK);
	    EndPaint(hWnd, &ps);
	    break;
	case WM_DESTROY:
	    printf("MessageBox WM_DESTROY !\n");
	    ReleaseCapture();
	    lpmb = MsgBoxGetStorageHeader(hWnd);
	    lpmb->ActiveFlg = FALSE;
	    if (lpmb->hIcon) DestroyIcon(lpmb->hIcon);
	    if (lpmb->hWndYes) DestroyWindow(lpmb->hWndYes);
	    if (lpmb->hWndNo) DestroyWindow(lpmb->hWndNo);
	    if (lpmb->hWndCancel) DestroyWindow(lpmb->hWndCancel);
	    break;
	case WM_COMMAND:
	    lpmb = MsgBoxGetStorageHeader(hWnd);
	    if (wParam < IDOK || wParam > IDNO) return(0);
	    lpmb->wRetVal = wParam;
#ifdef DEBUG_MSGBOX
	    printf("MessageBox sending WM_CLOSE !\n");
#endif
	    PostMessage(hWnd, WM_CLOSE, 0, 0L);
	    break;
	default:
	    return DefWindowProc(hWnd, message, wParam, lParam );
    }
return(0);
}



/*************************************************************************
 *			"About Wine..." Dialog Box
 */
BOOL FAR PASCAL AboutWine_Proc(HWND hDlg, WORD msg, WORD wParam, LONG lParam)
{
    HDC  	hDC;
    HDC		hMemDC;
    PAINTSTRUCT ps;
    int 	OldBackMode;
    HFONT	hOldFont;
    RECT 	rect;
    BITMAP	bm;
    char 	C[80];
    int 	X;
    static HBITMAP	hBitMap;
    switch (msg) {
    case WM_INITDIALOG:
	strcpy(C, "WINELOGO");
	hBitMap = LoadBitmap((HINSTANCE)NULL, (LPSTR)C);
	return TRUE;
    case WM_PAINT:
	hDC = BeginPaint(hDlg, &ps);
	GetClientRect(hDlg, &rect);
	FillRect(hDC, &rect, GetStockObject(GRAY_BRUSH));
	InflateRect(&rect, -3, -3);
	FrameRect(hDC, &rect, GetStockObject(BLACK_BRUSH));
	InflateRect(&rect, -10, -10);
	hMemDC = CreateCompatibleDC(hDC);
	SelectObject(hMemDC, hBitMap);
	GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&bm);
	BitBlt(hDC, rect.left, rect.top, bm.bmWidth, bm.bmHeight, 
					hMemDC, 0, 0, SRCCOPY);
	DeleteDC(hMemDC);
	EndPaint(hDlg, &ps);
	return TRUE;
    case WM_COMMAND:
	switch (wParam)
	    {
	    case IDOK:
CloseDLG:	EndDialog(hDlg, TRUE);
		return TRUE;
	    default:
		return TRUE;
	    }
    }
return FALSE;
}


