/*
 * 'Wine' MessageBox function handling
 *
 * Copyright 1993 Martin Ayotte
 */

static char Copyright[] = "Copyright Martin Ayotte, 1993";

#define DEBUG_MSGBOX

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "prototypes.h"
#include "heap.h"
#include "win.h"

extern HINSTANCE hSysRes;
extern HBITMAP hUpArrow;

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

int MessageBox(HWND hWnd, LPSTR str, LPSTR title, WORD type)
{
	HWND    	hDlg, hWndOld;
	WND	    	*wndPtr;
	WNDCLASS  	wndClass;
	MSG	    	msg;
    LPMSGBOX 	lpmb;
	DWORD		dwStyle;
	HINSTANCE	hInst;
	int			nRet;
	wndPtr = WIN_FindWndPtr(hWnd);
	if (wndPtr == NULL) {
		hInst = hSysRes;
#ifdef DEBUG_MSGBOX
		printf("MessageBox(NULL, %08X='%s', %08X='%s', %04X)\n", 
									str, str, title, title, type);
#endif
		}
	else {
		hInst = wndPtr->hInstance;
#ifdef DEBUG_MSGBOX
		printf("MessageBox(%04X, %08X='%s', %08X='%s', %04X)\n", 
							hWnd, str, str, title, title, type);
#endif
		}
    lpmb = (LPMSGBOX) malloc(sizeof(MSGBOX));
	memset(lpmb, 0, sizeof(MSGBOX));
/*	lpmb->Title = title;*/
	lpmb->Title = (LPSTR) malloc(strlen(title) + 1);
	strcpy(lpmb->Title, title);
/*	lpmb->Str = str;*/
	lpmb->Str = (LPSTR) malloc(strlen(str) + 1);
	strcpy(lpmb->Str, str);
	lpmb->wType = type;
	lpmb->ActiveFlg = TRUE;
	wndClass.style           = CS_HREDRAW | CS_VREDRAW ;
	wndClass.lpfnWndProc     = (WNDPROC)SystemMessageBoxProc;
	wndClass.cbClsExtra      = 0;
	wndClass.cbWndExtra      = 0;
	wndClass.hInstance       = hInst;
	wndClass.hIcon           = (HICON)NULL;
	wndClass.hCursor         = LoadCursor((HANDLE)NULL, IDC_ARROW); 
	wndClass.hbrBackground   = GetStockObject(WHITE_BRUSH);
	wndClass.lpszMenuName    = NULL;
	wndClass.lpszClassName   = "MESSAGEBOX";
#ifdef DEBUG_MSGBOX
	printf( "MessageBox // before RegisterClass, '%s' '%s' !\n", str, title);
#endif
	if (!RegisterClass(&wndClass)) {
		printf("Unable to Register class 'MESSAGEBOX' !\n");
		if (lpmb != NULL) free(lpmb);
		return 0;
		}
	dwStyle = WS_POPUP | WS_DLGFRAME | WS_VISIBLE;
	if ((type & (MB_SYSTEMMODAL | MB_TASKMODAL)) == 0) dwStyle |= WS_CAPTION;
	hWndOld = GetFocus();
	hDlg = CreateWindow("MESSAGEBOX", lpmb->Title, dwStyle, 100, 150, 400, 160,
				(HWND)NULL, (HMENU)NULL, hInst, (LPSTR)lpmb);
	if (hDlg == 0) {
		printf("Unable to create 'MESSAGEBOX' window !\n");
		if (lpmb != NULL) free(lpmb);
		return 0;
		}
#ifdef DEBUG_MSGBOX
	printf( "MessageBox // before Msg Loop !\n");
#endif
	while(TRUE) {
		if (!lpmb->ActiveFlg) break;
		if (!GetMessage(&msg, (HWND)NULL, 0, 0)) break;
		TranslateMessage(&msg);
		if ((type & (MB_SYSTEMMODAL | MB_TASKMODAL)) != 0 &&
			msg.hwnd != hDlg) {
			switch(msg.message) {
				case WM_KEYDOWN:
				case WM_LBUTTONDOWN:
				case WM_MBUTTONDOWN:
				case WM_RBUTTONDOWN:
					MessageBeep(0);
					break;
				}
			}
		DispatchMessage(&msg);
		}
	SetFocus(hWndOld);
	nRet = lpmb->wRetVal;
	if (lpmb != NULL) free(lpmb);
	if (!UnregisterClass("MESSAGEBOX", hInst)) return 0;
#ifdef DEBUG_MSGBOX
	printf( "MessageBox return %04X !\n", nRet);
#endif
	return(nRet);
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
	WND	    	*wndPtr;
	CREATESTRUCT *createStruct;
	PAINTSTRUCT	ps;
	HDC			hDC;
	DWORD		OldTextColor;
	RECT		rect;
	LPMSGBOX	lpmb;
	BITMAP		bm;
	HBITMAP		hBitMap;
	HDC			hMemDC;
	HICON		hIcon;
	HINSTANCE	hInst2;
	int			x;
	switch(message) {
	case WM_CREATE:
#ifdef DEBUG_MSGBOX
		printf("MessageBox WM_CREATE !\n");
#endif
		wndPtr = WIN_FindWndPtr(hWnd);
		createStruct = (CREATESTRUCT *)lParam;
		lpmb = (LPMSGBOX)createStruct->lpCreateParams;
		if (lpmb == NULL) break;
		*((LPMSGBOX *)&wndPtr->wExtra[1]) = lpmb;
#ifdef DEBUG_MSGBOX
		printf("MessageBox WM_CREATE title='%s' str='%s' !\n", 
									lpmb->Title, lpmb->Str);
#endif
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
		if (lpmb == NULL) break;
		CopyRect(&rect, &lpmb->rectStr);
		hDC = BeginPaint(hWnd, &ps);
		OldTextColor = SetTextColor(hDC, 0x00000000);
		if (lpmb->hIcon) 
			DrawIcon(hDC, lpmb->rectIcon.left,
				lpmb->rectIcon.top, lpmb->hIcon);
		DrawText(hDC, lpmb->Str, -1, &rect, 
			DT_CALCRECT | DT_CENTER | DT_WORDBREAK);
		rect.top = lpmb->rectStr.bottom / 2 - rect.bottom / 2;
		rect.bottom = lpmb->rectStr.bottom / 2 + rect.bottom / 2;
		DrawText(hDC, lpmb->Str, -1, &rect, DT_CENTER | DT_WORDBREAK);
		SetTextColor(hDC, OldTextColor);
		EndPaint(hWnd, &ps);
#ifdef DEBUG_MSGBOX
		printf("MessageBox End of WM_PAINT !\n");
#endif
		break;
	case WM_DESTROY:
#ifdef DEBUG_MSGBOX
	    printf("MessageBox WM_DESTROY !\n");
#endif
	    ReleaseCapture();
	    lpmb = MsgBoxGetStorageHeader(hWnd);
		if (lpmb == NULL) break;
	    lpmb->ActiveFlg = FALSE;
	    if (lpmb->hIcon) DestroyIcon(lpmb->hIcon);
	    if (lpmb->hWndYes) DestroyWindow(lpmb->hWndYes);
	    if (lpmb->hWndNo) DestroyWindow(lpmb->hWndNo);
	    if (lpmb->hWndCancel) DestroyWindow(lpmb->hWndCancel);
#ifdef DEBUG_MSGBOX
	    printf("MessageBox WM_DESTROY end !\n");
#endif
	    break;
	case WM_COMMAND:
	    lpmb = MsgBoxGetStorageHeader(hWnd);
		if (lpmb == NULL) break;
	    if (wParam < IDOK || wParam > IDNO) return(0);
	    lpmb->wRetVal = wParam;
#ifdef DEBUG_MSGBOX
	    printf("MessageBox sending WM_CLOSE !\n");
#endif
	    PostMessage(hWnd, WM_CLOSE, 0, 0L);
	    break;
	case WM_CHAR:
	    lpmb = MsgBoxGetStorageHeader(hWnd);
		if (lpmb == NULL) break;
		if (wParam >= 'a' || wParam <= 'z') wParam -= 'a' - 'A';
		switch(wParam) {
			case 'Y':
			    lpmb->wRetVal = IDYES;
				break;
			case 'O':
			    lpmb->wRetVal = IDOK;
				break;
			case 'R':
			    lpmb->wRetVal = IDRETRY;
				break;
			case 'A':
			    lpmb->wRetVal = IDABORT;
				break;
			case 'N':
			    lpmb->wRetVal = IDNO;
				break;
			case 'I':
			    lpmb->wRetVal = IDIGNORE;
				break;
			case 'C':
			case VK_ESCAPE:
			    lpmb->wRetVal = IDCANCEL;
				break;
			default:
			    return 0;
			}
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
    int 	X;
    OFSTRUCT	ofstruct;
    static LPSTR	ptr;
    static char 	str[256];
    static HBITMAP	hBitMap = 0;
    static BOOL		CreditMode;
    static HANDLE	hFile = 0;
    switch (msg) {
    case WM_INITDIALOG:
	CreditMode = FALSE;
	strcpy(str, "WINELOGO");
	hBitMap = LoadBitmap((HINSTANCE)NULL, (LPSTR)str);

	strcpy(str, "LICENSE");
	printf("str = '%s'\n", str);
	hFile = OpenFile((LPSTR)str, &ofstruct, OF_READ);
	ptr = (LPSTR)malloc(2048);
	lseek(hFile, 0L, SEEK_SET);
	_lread(hFile, ptr, 2000L);
	close(hFile);
	return TRUE;
    case WM_PAINT:
	hDC = BeginPaint(hDlg, &ps);
	GetClientRect(hDlg, &rect);
	if (CreditMode) {
	    FillRect(hDC, &rect, GetStockObject(WHITE_BRUSH));
	    InflateRect(&rect, -8, -8);
	    DrawText(hDC, ptr, -1, &rect, DT_LEFT | DT_WORDBREAK);
	    EndPaint(hDlg, &ps);
	    return TRUE;
	    }
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
	    case IDYES:
		if (!CreditMode) {
		    SetWindowPos(hDlg, (HWND)NULL, 0, 0, 640, 480, 
				SWP_NOMOVE | SWP_NOZORDER);
		    }
		else {
		    SetWindowPos(hDlg, (HWND)NULL, 0, 0, 320, 250, 
				SWP_NOMOVE | SWP_NOZORDER);
		    }
		CreditMode = !CreditMode;
		ShowWindow(GetDlgItem(hDlg, IDYES), CreditMode ? SW_HIDE : SW_SHOW);
		ShowWindow(GetDlgItem(hDlg, IDOK), CreditMode ? SW_HIDE : SW_SHOW);
		InvalidateRect(hDlg, (LPRECT)NULL, TRUE);
		UpdateWindow(hDlg);
		return TRUE;
	    case IDCANCEL:
	    case IDOK:
CloseDLG:	if (hBitMap != 0 ) DeleteObject(hBitMap);
		if (ptr != NULL) free(ptr);
		EndDialog(hDlg, TRUE);
		return TRUE;
	    default:
		return TRUE;
	    }
    }
return FALSE;
}


/**************************************************************************
 *			FatalAppExit  [USER.137]
 */

void FatalAppExit(WORD wAction, LPSTR str)
{
MessageBox((HWND)NULL, str, NULL, MB_SYSTEMMODAL | MB_OK);
exit(1);
}
