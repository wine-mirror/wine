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
    	WS_POPUP | WS_DLGFRAME | WS_VISIBLE, 100, 150, 320, 120,
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
	    wndPtr = WIN_FindWndPtr(hWnd);
	    createStruct = (CREATESTRUCT *)lParam;
     	    lpmbInit = (LPMSGBOX)createStruct->lpCreateParams;
     	    if (lpmbInit == 0) break;
	    *((LPMSGBOX *)&wndPtr->wExtra[1]) = lpmbInit;
	    lpmb = MsgBoxGetStorageHeader(hWnd);
	    GetClientRect(hWnd, &rect);
	    switch(lpmb->wType & MB_TYPEMASK) {
		case MB_OK :
		    lpmb->hWndYes = CreateWindow("BUTTON", "&Ok", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 - 70, rect.bottom - 25, 
			60, 18, hWnd, 1, wndPtr->hInstance, 0L);
		    break;
		case MB_OKCANCEL :
		    lpmb->hWndYes = CreateWindow("BUTTON", "&Ok", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 - 70, rect.bottom - 25, 
			60, 18, hWnd, 1, wndPtr->hInstance, 0L);
		    lpmb->hWndCancel = CreateWindow("BUTTON", "&Cancel", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 + 10, rect.bottom - 25, 
			60, 18, hWnd, 2, wndPtr->hInstance, 0L);
		    break;
		case MB_ABORTRETRYIGNORE :
		    lpmb->hWndYes = CreateWindow("BUTTON", "&Retry", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 - 70, rect.bottom - 25, 
			60, 18, hWnd, 1, wndPtr->hInstance, 0L);
		    lpmb->hWndNo = CreateWindow("BUTTON", "&Ignore", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 + 10, rect.bottom - 25, 
			60, 18, hWnd, 2, wndPtr->hInstance, 0L);
		    lpmb->hWndCancel = CreateWindow("BUTTON", "&Abort", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 + 80, rect.bottom - 25, 
			60, 18, hWnd, 3, wndPtr->hInstance, 0L);
		    break;
		case MB_YESNO :
		    lpmb->hWndYes = CreateWindow("BUTTON", "&Yes", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 - 70, rect.bottom - 25, 
			60, 18, hWnd, 1, wndPtr->hInstance, 0L);
		    lpmb->hWndNo = CreateWindow("BUTTON", "&No", 
			WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | BS_PUSHBUTTON,
			rect.right / 2 + 10, rect.bottom - 25, 
			60, 18, hWnd, 2, wndPtr->hInstance, 0L);
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
	    break;
	case WM_PAINT:
	    lpmb = MsgBoxGetStorageHeader(hWnd);
	    GetClientRect(hWnd, &rect);
	    hDC = BeginPaint(hWnd, &ps);
	    if (lpmb->hIcon) DrawIcon(hDC, 30, 20, lpmb->hIcon);
	    TextOut(hDC, rect.right / 2, 15, 
	    	lpmb->Title, strlen(lpmb->Title));
	    TextOut(hDC, rect.right / 2, 30, 
	    	lpmb->Str, strlen(lpmb->Str));
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
	    switch(wParam) {
		case 1:
		    lpmb->wRetVal = IDOK;
		    break;
		case 2:
		    wndPtr = WIN_FindWndPtr(hWnd);
		    hDC = GetDC(hWnd);
/*
		    for (x = 1; x < 50; x++) {
			hBitMap = LoadBitmap(wndPtr->hInstance, MAKEINTRESOURCE(x));
			GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&bm);
			hMemDC = CreateCompatibleDC(hDC);
			SelectObject(hMemDC, hBitMap);
			printf(" bm.bmWidth=%d bm.bmHeight=%d\n",
				bm.bmWidth, bm.bmHeight);
			BitBlt(hDC, x * 20, 30, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
			DeleteDC(hMemDC);
			}
*/
		    hBitMap = LoadBitmap((HINSTANCE)NULL, "SMILE");
		    GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&bm);
		    printf("bm.bmWidth=%d bm.bmHeight=%d\n",
		    	bm.bmWidth, bm.bmHeight);  
		    hMemDC = CreateCompatibleDC(hDC);
		    SelectObject(hMemDC, hBitMap);
		    BitBlt(hDC, 100, 30, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
		    DeleteDC(hMemDC);
		    ReleaseDC(hWnd, hDC);
		    lpmb->wRetVal = IDCANCEL;
/*
		    SetWindowPos(lpmb->hWndNo, (HWND)NULL, 20, 20, 0, 0, 
		    		SWP_NOSIZE | SWP_NOZORDER);
*/
		    return 0;
		    break;
		case 3:
		    hDC = GetDC(hWnd);
		    hInst2 = LoadImage("ev3lite.exe", NULL);
		    hIcon = LoadIcon(hInst2, "EV3LITE");
		    DrawIcon(hDC, 20, 20, hIcon);
		    DestroyIcon(hIcon);
		    hInst2 = LoadImage("sysres.dll", NULL);
		    hIcon = LoadIcon(hInst2, "WINEICON");
		    DrawIcon(hDC, 60, 20, hIcon);
		    DestroyIcon(hIcon);
		    hIcon = LoadIcon((HINSTANCE)NULL, IDI_EXCLAMATION);
		    DrawIcon(hDC, 1000, 20, hIcon);
		    DestroyIcon(hIcon);
		    ReleaseDC(hWnd, hDC);
		    lpmb->wRetVal = IDIGNORE;
		    return(0);
		    break;
		default:
		    return(0);
		}
	    CloseWindow(hWnd);
	    break;
	default:
	    return DefWindowProc(hWnd, message, wParam, lParam );
    }
return(0);
}



