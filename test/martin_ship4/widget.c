/**********************************************************************
 *               Copyright (c) 1991 by TRG ELECTRONIK                 *
 *                                                                    *
 *                             widget.C                               *
 *              widget Inventory Controler   PART # 1                 *
 *                                                                    *
 **********************************************************************/

#include <WINDOWS.H>
#include "STDIO.h"
#include "STDLIB.h"
#include "STDARG.h"
#include "STRING.H"

#include "widget.H"
#include "widget.VAR"
#include "widget.P"

#define NoREF(a) a=a

HWND		hWndBut;
HWND		hWndChk;
HWND		hWndRadio;
HWND		hWndLBox;
HWND		hWndLBox2;
HWND		hWndLBox3;
HWND		hWndLBox4;
HWND		hWndScrol;
HWND		hWndScro2;
HWND		hWndScro3;
HWND		hWndStat;
HWND		hWndEdit;
HWND		hWndCBox;
int		x, y;

HBRUSH	hREDBrush;
HBRUSH	hGREENBrush;
HBRUSH	hBLUEBrush;
HBRUSH	hGRAYBrush;
HBITMAP  hBitMap;
HBITMAP  hBitMap2;
HBITMAP  hBitMap3;
BITMAP   BitMap;

/**********************************************************************/


int PASCAL WinMain(HANDLE hInstance,HANDLE hPrevInstance,
                   LPSTR lpszCmdLine,int nCmdShow)
{
WNDCLASS  wndClass;
MSG       msg;
HWND      hWnd;
HDC       hDC;
char      C[40];
int       X;
NoREF(lpszCmdLine);
if ( !hPrevInstance )
    {
    wndClass.style           = CS_HREDRAW | CS_VREDRAW ;
    wndClass.lpfnWndProc     = (WNDPROC)WndProc;
    wndClass.cbClsExtra      = 0;
    wndClass.cbWndExtra      = 0;
    wndClass.hInstance       = hInstance;
    wndClass.hIcon           = LoadIcon(hInstance,(LPSTR)"ICON_1");
    wndClass.hCursor         = LoadCursor(NULL, IDC_ARROW );
    wndClass.hbrBackground   = GetStockObject(WHITE_BRUSH );
    wndClass.lpszMenuName    = szAppName;
    wndClass.lpszClassName   = szAppName;
    if (!RegisterClass(&wndClass))
        return FALSE;
    }
hWnd = CreateWindow(szAppName, "widget test program",
         WS_POPUP | WS_CAPTION | WS_BORDER | WS_VISIBLE, 50, 50,
         400, 500, NULL, NULL, hInstance, NULL);
hWndMain = hWnd;
hInst = hInstance;
hDCMain = GetDC(hWndMain);
hREDBrush    = CreateSOLIDBrush(0x000000FF);
hGREENBrush  = CreateSOLIDBrush(0x00007F00);
hBLUEBrush   = CreateSOLIDBrush(0x00FF0000);
hGRAYBrush   = CreateSOLIDBrush(0x00C0C0C0);

InitWidgets();

while (GetMessage(&msg, NULL, 0, 0))
    {
    TranslateMessage(&msg );
    DispatchMessage(&msg );
    }
DeleteObject(hREDBrush);
DeleteObject(hGREENBrush);
DeleteObject(hBLUEBrush);
DeleteObject(hGRAYBrush);
if (hBitMap != NULL) DeleteObject(hBitMap);
if (hBitMap2 != NULL) DeleteObject(hBitMap2);
if (hBitMap3 != NULL) DeleteObject(hBitMap3);
ReleaseDC(hWndMain, hDC);
return(0);
}


void InitWidgets()
{
hWndBut = CreateWindow("BUTTON", "Button #1",
            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | BS_PUSHBUTTON, 
            230, 40, 80, 30, hWndMain, 1001, hInst, NULL);
hWndScrol = CreateWindow("SCROLLBAR", "Scroll #1",
            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBS_VERT,
				200, 150, 15, 100, hWndMain, 1004, hInst, NULL);
hWndScro2 = CreateWindow("SCROLLBAR", "Scroll #2",
            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBS_HORZ,
				50, 150, 100, 15, hWndMain, 1005, hInst, NULL);
hWndScro3 = CreateWindow("SCROLLBAR", "Scroll #3",
            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBS_SIZEBOX,
				50, 180, 25, 25, hWndMain, 1006, hInst, NULL);
x = y = 25;
SetVertScroll(NULL, hWndScrol, 25, 0, 50);
SetScrollRange(hWndScro2, SB_CTL, 0, 50, TRUE);
SetScrollPos(hWndScro2, SB_CTL, 25, TRUE);
hWndLBox = CreateWindow("LISTBOX", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
				230, 160, 150, 100, hWndMain, 1004, hInst, NULL);
SendMessage(hWndLBox, LB_ADDSTRING, 0, (LPARAM)"ListBox item #1");
hWndStat = CreateWindow("STATIC", "Static #1",
            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_LEFT, 
            30, 120, 150, 20, hWndMain, 1011, hInst, NULL);
SendMessage(hWndStat, WM_SETTEXT, 0, (LPARAM)"Static Left Text");
hWndCBox = CreateWindow("COMBOBOX", "Combo #1",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | CBS_DROPDOWNLIST,
		230, 270, 150, 100, hWndMain, 1060, hInst, NULL);
SendMessage(hWndCBox, CB_ADDSTRING, 0, (LPARAM)"ComboBox item #1");
SendMessage(hWndCBox, CB_ADDSTRING, 0, (LPARAM)"ComboBox item #2");
SendMessage(hWndCBox, CB_ADDSTRING, 0, (LPARAM)"ComboBox item #3");
SendMessage(hWndCBox, CB_ADDSTRING, 0, (LPARAM)"ComboBox item #4");
SendMessage(hWndCBox, CB_ADDSTRING, 0, (LPARAM)"ComboBox item #5");
SendMessage(hWndCBox, CB_ADDSTRING, 0, (LPARAM)"ComboBox item #6");
}



long FAR PASCAL WndProc(HWND hWnd, unsigned Message, WORD wParam, LONG lParam)
{
int	ShiftState;
LPDRAWITEMSTRUCT dis;
HDC      hMemDC;
HBRUSH	hBrush;
char	C[128];
ShiftState = GetKeyState(VK_SHIFT);
switch(Message)
    {
    case WM_COMMAND:
		  if (LOWORD(lParam) != 0)
			   {
		  		sprintf(C, "MAIN WM_COMMAND wParam=%X lParam=%lX !!!", wParam, lParam);
		  		TextOut(hDCMain, 25, 280, C, strlen(C));
			   }
        break;

    case WM_KEYDOWN:
		  sprintf(C, "WM_KEYDOWN !!!");
		  TextOut(hDCMain, 25, 40, C, strlen(C));
		  KeyDown(hWnd, Message, wParam);
        break;

    case WM_CHAR:
		  sprintf(C, "WM_CHAR !!!");
		  TextOut(hDCMain, 25, 50, C, strlen(C));
        break;

    case WM_CTLCOLOR:
		  switch(HIWORD(lParam))
			   {
				case CTLCOLOR_SCROLLBAR:
                return(hBLUEBrush);
				case CTLCOLOR_LISTBOX:
		          SetBkColor((HDC)wParam, 0x00C0C000);
                SetTextColor((HDC)wParam, 0x00FF0000);
                return(hGREENBrush);
				case CTLCOLOR_STATIC:
		          SetBkColor((HDC)wParam, 0x00C0C0C0);
                SetTextColor((HDC)wParam, 0x0000FFFF);
                return(hREDBrush);
				}
        return((HBRUSH)NULL);

    case WM_LBUTTONDOWN:
        break;

    case WM_LBUTTONUP:
        break;

    case WM_RBUTTONDOWN:
        break;

    case WM_RBUTTONUP:
        break;

    case WM_MOUSEMOVE:
        break;

    case WM_VSCROLL:
		  sprintf(C, "WM_VSCROLL %X %lX !!!", wParam, lParam);
		  TextOut(hDCMain, 25, 380, C, strlen(C));
		  Do_Dlg_VertScroll(hWnd, wParam, lParam, &y, 0, 50);
        break;

    case WM_HSCROLL:
		  sprintf(C, "WM_HSCROLL %X %lX !!!", wParam, lParam);
		  TextOut(hDCMain, 25, 380, C, strlen(C));
		  Do_Dlg_HorzScroll(hWnd, wParam, lParam, &x, 0, 50);
        break;

    case WM_DRAWITEM:
		  sprintf(C, "WM_DRAWITEM %X %lX !!!", wParam, lParam);
		  TextOut(hDCMain, 25, 380, C, strlen(C));
		  if (lParam == 0L) break;
		  if (wParam == 0) break;
		  dis = (LPDRAWITEMSTRUCT)lParam;
		  if ((dis->CtlType == ODT_LISTBOX) && (dis->CtlID == 1062)) {
				hBrush = SelectObject(dis->hDC, GetStockObject(LTGRAY_BRUSH));
				SelectObject(dis->hDC, hBrush);
				FillRect(dis->hDC, &dis->rcItem, hBrush);
				sprintf(C, "Item #%X", dis->itemID);
			   if (dis->itemData == NULL) break;
				TextOut(dis->hDC, dis->rcItem.left,
					dis->rcItem.top, C, strlen(C));
				}
		  if ((dis->CtlType == ODT_LISTBOX) && (dis->CtlID == 1063)) {
				hBrush = SelectObject(dis->hDC, GetStockObject(LTGRAY_BRUSH));
				SelectObject(dis->hDC, hBrush);
				FillRect(dis->hDC, &dis->rcItem, hBrush);
			   if (dis->itemData == NULL) break;
				TextOut(dis->hDC, dis->rcItem.left,	dis->rcItem.top,
					(LPSTR)dis->itemData, lstrlen((LPSTR)dis->itemData));
				}
		  if ((dis->CtlType == ODT_LISTBOX) && (dis->CtlID == 1064)) {
				hBrush = SelectObject(dis->hDC, GetStockObject(LTGRAY_BRUSH));
				SelectObject(dis->hDC, hBrush);
				FillRect(dis->hDC, &dis->rcItem, hBrush);
				hMemDC = CreateCompatibleDC(dis->hDC);
				SelectObject(hMemDC,hBitMap);
				BitBlt(dis->hDC, dis->rcItem.left,	dis->rcItem.top,
						BitMap.bmWidth, BitMap.bmHeight, hMemDC, 0, 0, SRCCOPY);
				DeleteDC(hMemDC);
				sprintf(C, "Item #%X", dis->itemID);
				TextOut(dis->hDC, dis->rcItem.left + BitMap.bmWidth,
					dis->rcItem.top, C, strlen(C));
//			   if (dis->itemData == NULL) break;
//				TextOut(dis->hDC, dis->rcItem.left + BitMap.bmWidth,
//						dis->rcItem.top, (LPSTR)dis->itemData,
//						lstrlen((LPSTR)dis->itemData));
				}
        break;

    case WM_PAINT:
        DoPaint(hWnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, Message, wParam, lParam);
    }
return(0);
}



BOOL KeyDown(HWND hWnd, unsigned Message, WORD wParam)
{
WORD			wRet;
UINT			uRet;
DWORD			dwRet;
HDC      hMemDC;
char			C[128];
char			C2[64];
NoREF(hWnd);
NoREF(Message);
sprintf(C, "KeyDown %x !!!", wParam);
TextOut(hDCMain, 25, 100, C, strlen(C));
switch (wParam)
    {
    case VK_HOME:            /* 'HOME' KEY         */
        break;
    case VK_LEFT:            /* 'LEFT' CURSOR KEY  */
        break;
    case VK_RIGHT:           /* 'RIGHT' CURSOR KEY */
        break;
    case VK_UP:              /* 'UP' CURSOR KEY    */
        break;
    case VK_DOWN:            /* 'DOWN' CURSOR KEY  */
        break;
    case VK_PRIOR:           /* 'PGUP' CURSOR KEY  */
        break;
    case VK_NEXT:            /* 'PGDN' CURSOR KEY  */
        break;
    case '1':
        break;
    case '2':
		  hWndStat = CreateWindow("STATIC", "Static #2",
            			WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_CENTER, 
            			30, 150, 150, 20, hWndMain, 1012, hInst, NULL);
		  SendMessage(hWndStat, WM_SETTEXT, 0, (LPARAM)"Static Center Text");
        break;
    case '3':
		  hWndStat = CreateWindow("STATIC", "Static #3",
            			WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT, 
            			30, 180, 150, 20, hWndMain, 1013, hInst, NULL);
		  SendMessage(hWndStat, WM_SETTEXT, 0, (LPARAM)"Static Right Text");
        break;
    case '4':
		  hWndStat = CreateWindow("STATIC", "Static #4",
            			WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_SIMPLE, 
            			30, 210, 150, 20, hWndMain, 1014, hInst, NULL);
		  SendMessage(hWndStat, WM_SETTEXT, 0, (LPARAM)"SS_SIMPLE");
        break;
    case '5':
		  hWndStat = CreateWindow("STATIC", "Static #5",
            			WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_GRAYRECT, 
            			30, 240, 150, 20, hWndMain, 1015, hInst, NULL);
		  SendMessage(hWndStat, WM_SETTEXT, 0, (LPARAM)"SS_GRAYRECT");
        break;
    case 'A':
		  hWndStat = CreateWindow("STATIC", "Static #6",
            			WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_BLACKRECT, 
            			30, 240, 150, 20, hWndMain, 1016, hInst, NULL);
		  SendMessage(hWndStat, WM_SETTEXT, 0, (LPARAM)"SS_BLACKRECT");
        break;
    case 'C':
		  if (hBitMap2 == NULL) {
		  		hBitMap2 = LoadBitmap(hInst, "LBICON");
		  		GetObject(hBitMap2, sizeof(BITMAP), (LPSTR)&BitMap);
				}
		  hMemDC = CreateCompatibleDC(hDCMain);
		  SelectObject(hMemDC, hBitMap2);
		  BitBlt(hDCMain, 10, 10, BitMap.bmWidth,
						BitMap.bmHeight, hMemDC, 0, 0, SRCCOPY);
		  DeleteDC(hMemDC);
		  sprintf(C, "DrawBitmap");
		  TextOut(hDCMain, 25, 320, C, strlen(C));
        break;
    case 'D':
		  if (hBitMap3 == NULL) {
		  		hBitMap3 = LoadBitmap(hInst, MAKEINTRESOURCE(3333));
		  		GetObject(hBitMap3, sizeof(BITMAP), (LPSTR)&BitMap);
				}
		  hMemDC = CreateCompatibleDC(hDCMain);
		  SelectObject(hMemDC, hBitMap3);
		  BitBlt(hDCMain, 80, 10, BitMap.bmWidth,
						BitMap.bmHeight, hMemDC, 0, 0, SRCCOPY);
		  DeleteDC(hMemDC);
		  sprintf(C, "DrawBitmap");
		  TextOut(hDCMain, 25, 320, C, strlen(C));
        break;
    case 'F':
		  MoveWindow(hWnd, 10, 10, 500, 600, TRUE);
        break;
    case 'G':
		  MoveWindow(hWndBut, 20, 20, 100, 20, TRUE);
        break;
    case 'H':
		  WinHelp(hWndMain, "toto.hlp", HELP_INDEX, 0L);
        break;
    case 'J':
		  WinExec("/D:/wine/widget.exe arg1 arg2 arg3 arg4 arg5 arg6", SW_NORMAL);
        break;
    case 'K':
        break;
    case 'Q':
		  hWndChk = CreateWindow("BUTTON", "CheckBox #1",
                    WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | BS_CHECKBOX,
						  30, 300, 120, 20, hWndMain, 1020, hInst, NULL);
        break;
    case 'W':
		  wRet = SendMessage(hWndChk , BM_GETCHECK, 0, 0L);
		  SendMessage(hWndChk , BM_SETCHECK, wRet, 0L);
        break;
    case 'E':
        break;
    case 'R':
		  hWndRadio = CreateWindow("BUTTON", "RadioBut #1",
                    WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | BS_RADIOBUTTON,
						  230, 120, 120, 20, hWndMain, 1003, hInst, NULL);
		  SendMessage(hWndRadio, BM_SETCHECK, TRUE, 0L);
        break;
    case 'T':
		  SendMessage(hWndLBox, LB_ADDSTRING, 0, (LPARAM)"ListBox Single item");
        break;
    case 'Y':
		  SendMessage(hWndLBox, LB_ADDSTRING, 0, (LPARAM)"ListBox item #2");
		  SendMessage(hWndLBox, LB_ADDSTRING, 0, (LPARAM)"ListBox item #3");
		  SendMessage(hWndLBox, LB_ADDSTRING, 0, (LPARAM)"ListBox item #4");
		  SendMessage(hWndLBox, LB_ADDSTRING, 0, (LPARAM)"ListBox item #5");
		  SendMessage(hWndLBox, LB_ADDSTRING, 0, (LPARAM)"ListBox item #6");
		  SendMessage(hWndLBox, LB_SETCURSEL, 3, 0L);
		  SendMessage(hWndLBox, LB_INSERTSTRING, 5, (LPARAM)"Item between 5 & 6");
		  wRet = SendMessage(hWndLBox, LB_FINDSTRING, -1, (LPARAM)"Item between 5 & 6");
		  sprintf(C, "LB_FINDSTRING returned #%u   ", wRet);
		  TextOut(hDCMain, 25, 300, C, strlen(C));
		  wRet = SendMessage(hWndLBox, LB_GETCURSEL, 0, 0L);
		  sprintf(C, "LB_GETCURSEL returned #%u   ", wRet);
		  TextOut(hDCMain, 25, 320, C, strlen(C));
        break;
    case 'U':
		  wRet = SendMessage(hWndLBox, LB_GETCURSEL, 0, 0L);
		  SendMessage(hWndLBox, LB_DELETESTRING, wRet, 0L);
		  SendMessage(hWndLBox, LB_SETCURSEL, wRet, 0L);
        break;
    case 'I':
		  SendMessage(hWndLBox, LB_RESETCONTENT, 0, 0L);
        break;
    case 'O':
		  C2[0] = '\0';
		  wRet = SendMessage(hWndLBox, LB_GETCURSEL, 0, 0L);
		  SendMessage(hWndLBox, LB_GETTEXT, wRet, (DWORD)C2);
		  sprintf(C, "LB_GETTEXT #%d returned '%s'   ", wRet, C2);
		  TextOut(hDCMain, 25, 320, C, strlen(C));
        break;
    case 'P':
		  SendMessage(hWndLBox, LB_DIR, 0, (DWORD)"*.*");
        break;
    case 'Z':
		  ShowWindow(hWndScrol, SW_HIDE);
        break;
    case 'X':
		  ShowWindow(hWndScrol, SW_SHOW);
        break;
     case 'V':
		  MoveWindow(hWndScrol, 120, 150, 15, 60, TRUE);
        break;
     case 'B':
		  hWndCBox = CreateWindow("COMBOBOX", "Combo #2",
					WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | CBS_DROPDOWN,
					30, 270, 150, 100, hWndMain, 1061, hInst, NULL);
		  SendMessage(hWndCBox, CB_ADDSTRING, 0, (LPARAM)"ComboBox item #1");
		  SendMessage(hWndCBox, CB_ADDSTRING, 0, (LPARAM)"ComboBox item #2");
		  SendMessage(hWndCBox, CB_DIR, 0, (DWORD)"*.*");
        break;
     case 'N':
		  hWndLBox2 = CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE |
					WS_BORDER | WS_VSCROLL | LBS_OWNERDRAWVARIABLE | LBS_NOTIFY,
//					WS_BORDER | WS_VSCROLL | LBS_OWNERDRAWVARIABLE | LBS_HASSTRINGS | LBS_NOTIFY,
					30, 300, 150, 60, hWndMain, 1062, hInst, NULL);
		  SendMessage(hWndLBox2, LB_ADDSTRING, 0, (LPARAM)"DRAWFIXED #1");
		  SendMessage(hWndLBox2, LB_ADDSTRING, 0, (LPARAM)"DRAWFIXED #2");
		  SendMessage(hWndLBox2, LB_ADDSTRING, 0, (LPARAM)"DRAWFIXED #3");
		  SendMessage(hWndLBox2, LB_ADDSTRING, 0, (LPARAM)"DRAWFIXED #4");
		  SendMessage(hWndLBox2, LB_ADDSTRING, 0, (LPARAM)"DRAWFIXED #5");
		  SendMessage(hWndLBox2, LB_ADDSTRING, 0, (LPARAM)"DRAWFIXED #6");
        break;
     case 'M':
		  hWndLBox3 = CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE |
					WS_BORDER | WS_VSCROLL | LBS_OWNERDRAWVARIABLE | LBS_NOTIFY,
//					WS_BORDER | WS_VSCROLL | LBS_OWNERDRAWVARIABLE | LBS_HASSTRINGS | LBS_NOTIFY,
					230, 300, 150, 60, hWndMain, 1063, hInst, NULL);
		  SendMessage(hWndLBox3, LB_ADDSTRING, 0, (LPARAM)"DRAWVARIABLE #1");
		  SendMessage(hWndLBox3, LB_ADDSTRING, 0, (LPARAM)"DRAWVARIABLE #2");
		  SendMessage(hWndLBox3, LB_ADDSTRING, 0, (LPARAM)"DRAWVARIABLE #3");
		  SendMessage(hWndLBox3, LB_ADDSTRING, 0, (LPARAM)"DRAWVARIABLE #4");
		  SendMessage(hWndLBox3, LB_ADDSTRING, 0, (LPARAM)"DRAWVARIABLE #5");
		  SendMessage(hWndLBox3, LB_ADDSTRING, 0, (LPARAM)"DRAWVARIABLE #6");
		  SendMessage(hWndLBox3, LB_ADDSTRING, 0, (LPARAM)"DRAWVARIABLE #7");
		  SendMessage(hWndLBox3, LB_SETITEMHEIGHT, 1, 10L);
		  SendMessage(hWndLBox3, LB_SETITEMHEIGHT, 2, 20L);
		  SendMessage(hWndLBox3, LB_SETITEMHEIGHT, 3, 30L);
		  SendMessage(hWndLBox3, LB_SETITEMHEIGHT, 4, 40L);
		  SendMessage(hWndLBox3, LB_SETITEMHEIGHT, 5, 50L);
        break;
     case 'L':
		  hBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(3333));
		  GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&BitMap);
		  hWndLBox4 = CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE |
					WS_BORDER | WS_VSCROLL | LBS_OWNERDRAWVARIABLE | LBS_HASSTRINGS | LBS_NOTIFY,
					230, 380, 150, 60, hWndMain, 1064, hInst, NULL);
		  SendMessage(hWndLBox4, LB_ADDSTRING, 0, (LPARAM)"DRAWVARIABLE #1");
		  SendMessage(hWndLBox4, LB_ADDSTRING, 0, (LPARAM)"DRAWVARIABLE #2");
		  SendMessage(hWndLBox4, LB_ADDSTRING, 0, (LPARAM)"DRAWVARIABLE #3");
		  SendMessage(hWndLBox4, LB_ADDSTRING, 0, (LPARAM)"DRAWVARIABLE #4");
		  SendMessage(hWndLBox4, LB_ADDSTRING, 0, (LPARAM)"DRAWVARIABLE #5");
		  SendMessage(hWndLBox4, LB_ADDSTRING, 0, (LPARAM)"DRAWVARIABLE #6");
		  SendMessage(hWndLBox4, LB_SETITEMHEIGHT, 1, 10L);
		  SendMessage(hWndLBox4, LB_SETITEMHEIGHT, 2, 20L);
		  SendMessage(hWndLBox4, LB_SETITEMHEIGHT, 3, 30L);
		  SendMessage(hWndLBox4, LB_SETITEMHEIGHT, 4, 40L);
		  SendMessage(hWndLBox4, LB_SETITEMHEIGHT, 5, 50L);
        break;

    case VK_F10:             /* 'F10' FUNCTION KEY */
        break;
    case VK_F11:             /* 'F11' FUNCTION KEY */
        break;
    }
return(TRUE);
}



void DoPaint(HWND hWnd)
{
HDC         hDC;
RECT        rect;
PAINTSTRUCT ps;
char   C[80];
GetClientRect(hWnd, &rect);
hDC = BeginPaint(hWnd, &ps);
FillRect(hDC, &rect, GetStockObject(GRAY_BRUSH));
InflateRect(&rect, -3, -3);
FrameRect(hDC, &rect, GetStockObject(BLACK_BRUSH));
InflateRect(&rect, -10, -10);
FillRect(hDC, &rect, GetStockObject(WHITE_BRUSH));
sprintf(C, "Wine Testing !!!");
TextOut(hDC, 25, 25, C, strlen(C));
ReleaseDC(hWnd,hDC);
EndPaint(hWnd,&ps);
}


HBRUSH CreateLTGRAYBrush()
{
return(CreateSOLIDBrush(0x00C0C0C0));
}



HBRUSH CreateSOLIDBrush(COLORREF Color)
{
LOGBRUSH logGRAYBrush;
logGRAYBrush.lbStyle = BS_SOLID;
logGRAYBrush.lbColor = Color;
logGRAYBrush.lbHatch = NULL;
return(CreateBrushIndirect(&logGRAYBrush));
}



/**********************************************************************/


void SetVertScroll(int hDlg, int hWndSCROLL, int VAL, int MIN, int MAX)
{
char  C[12];
SetScrollRange(hWndSCROLL, SB_CTL, -MAX, -MIN, FALSE);
SetScrollPos(hWndSCROLL, SB_CTL, -VAL, TRUE);
itoa(VAL, C, 10);
//SetDlgItemText(hDlg, (IDDTXT1 + GetDlgCtrlID(hWndSCROLL) - IDDSCROLL1), C);
}



void SetHorzScroll(int hDlg, int hWndSCROLL, int VAL, int MIN, int MAX)
{
char  C[12];
SetScrollRange(hWndSCROLL, SB_CTL, MAX, MIN, FALSE);
SetScrollPos(hWndSCROLL, SB_CTL, VAL, TRUE);
itoa(VAL, C, 10);
//SetDlgItemText(hDlg, (IDDTXT1 + GetDlgCtrlID(hWndSCROLL) - IDDSCROLL1), C);
}



void Do_Dlg_VertScroll(HWND hDlg, WORD wParam, DWORD lParam, int *V, int MIN, int MAX)
{
char  C[12];
int   VAL;
int   VAL2;
int	Step = 100;
if (MAX < 1000)	Step = 10;
if (MAX < 100)		Step = MAX / 10;
VAL = *(V);
VAL2 = VAL;
switch (wParam)
    {
    case SB_LINEUP:
			VAL++;
			break;
    case SB_LINEDOWN:
			VAL--;
			break;
    case SB_PAGEUP:
			VAL += Step;
			break;
    case SB_PAGEDOWN:
			VAL -= Step;
			break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
		   VAL = -(LOWORD(lParam));
			break;
	}
if (VAL > MAX)		VAL = MAX;
if (VAL < MIN)		VAL = MIN;
if (VAL != VAL2)
    {
    SetScrollPos(HIWORD(lParam), SB_CTL, -VAL, TRUE);
    ltoa(VAL, C, 10);
//    SetDlgItemText(hDlg, (IDDTXT1 + GetDlgCtrlID(HIWORD(lParam)) - IDDSCROLL1), C);
    }
*(V) = VAL;
}


void Do_Dlg_HorzScroll(HWND hDlg, WORD wParam, DWORD lParam, int *V, int MIN, int MAX)
{
char  C[12];
int   VAL;
int   VAL2;
int	Step = 100;
if (MAX < 1000)	Step = 10;
if (MAX < 100)		Step = MAX / 10;
VAL = *(V);
VAL2 = VAL;
switch (wParam)
    {
    case SB_LINEUP:
			VAL--;
			break;
    case SB_LINEDOWN:
			VAL++;
			break;
    case SB_PAGEUP:
			VAL -= Step;
			break;
    case SB_PAGEDOWN:
			VAL += Step;
			break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
		   VAL = (LOWORD(lParam));
			break;
	}
if (VAL > MAX)		VAL = MAX;
if (VAL < MIN)		VAL = MIN;
if (VAL != VAL2)
    {
    SetScrollPos(HIWORD(lParam), SB_CTL, VAL, TRUE);
    ltoa(VAL, C, 10);
//    SetDlgItemText(hDlg, (IDDTXT1 + GetDlgCtrlID(HIWORD(lParam)) - IDDSCROLL1), C);
    }
*(V) = VAL;
}


/**********************************************************************/


