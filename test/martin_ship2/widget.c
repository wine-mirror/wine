/**********************************************************************
 *               Copyright (c) 1991 by TRG ELECTRONIK                 *
 *                                                                    *
 *                            widget.C                               *
 *              widget Inventory Controler   PART # 1                *
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

WORD          wPlayDiskID;

HWND		hWndBut;
HWND		hWndChk;
HWND		hWndRadio;
HWND		hWndLBox;
HWND		hWndScrol;
HWND		hWndScro2;
HWND		hWndStat;
HWND		hWndEdit;
HWND		hWndCBox;
int		x, y;

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
                    WS_POPUP | WS_BORDER | WS_VISIBLE, 50, 50,
                    400, 400, NULL, NULL, hInstance, NULL);
hWndMain = hWnd;
hInst = hInstance;
hDCMain = GetDC(hWndMain);

InitWidgets();

while (GetMessage(&msg, NULL, 0, 0))
    {
    TranslateMessage(&msg );
    DispatchMessage(&msg );
    }
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
				200, 140, 15, 100, hWndMain, 1004, hInst, NULL);
hWndScro2 = CreateWindow("SCROLLBAR", "Scroll #2",
            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SBS_HORZ,
				50, 140, 100, 15, hWndMain, 1005, hInst, NULL);
x = y = 25;
SetVertScroll(NULL, hWndScrol, 25, 0, 50);
SetScrollRange(hWndScro2, SB_CTL, 0, 50, TRUE);
SetScrollPos(hWndScro2, SB_CTL, 25, TRUE);
hWndLBox = CreateWindow("LISTBOX", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
				230, 160, 150, 100, hWndMain, 1004, hInst, NULL);
SendMessage(hWndLBox, LB_ADDSTRING, 0, (LPARAM)"ListBox item #1");
}



long FAR PASCAL WndProc(HWND hWnd, unsigned Message, WORD wParam, LONG lParam)
{
int	ShiftState;
char	C[80];
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
		  sprintf(C, "WM_VSCROLL %X %X %lX !!!", Message, wParam, lParam);
		  TextOut(hDCMain, 25, 370, C, strlen(C));
		  Do_Dlg_VertScroll(hWnd, wParam, lParam, &y, 0, 50);
        break;

    case WM_HSCROLL:
		  sprintf(C, "WM_HSCROLL %X %X %lX !!!", Message, wParam, lParam);
		  TextOut(hDCMain, 25, 370, C, strlen(C));
		  Do_Dlg_HorzScroll(hWnd, wParam, lParam, &x, 0, 50);
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
        break;
    case '3':
        break;
    case '4':
        break;
    case '5':
        break;
    case 'A':
        break;
    case 'B':
        break;
    case 'C':
        break;
    case 'D':
        break;
    case 'E':
        break;
    case 'F':
        break;
    case 'G':
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
		  hWndStat = CreateWindow("STATIC", "Static #1",
            			WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_SIMPLE, 
            			230, 20, 80, 20, hWndMain, 1000, hInst, NULL);
        break;
    case 'W':
		  hWndChk = CreateWindow("BUTTON", "CheckBox #1",
                    WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | BS_CHECKBOX,
						  230, 90, 120, 20, hWndMain, 1002, hInst, NULL);
        break;
    case 'R':
		  hWndRadio = CreateWindow("BUTTON", "RadioBut #1",
                    WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | BS_RADIOBUTTON,
						  230, 120, 120, 20, hWndMain, 1003, hInst, NULL);
		  SendMessage(hWndRadio, BM_SETCHECK, 0, 0L);
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
		  SendMessage(hWndLBox, LB_DELETESTRING, 3, 0L);
        break;
    case 'I':
		  SendMessage(hWndLBox, LB_RESETCONTENT, 0, 0L);
        break;
    case 'O':
		  C2[0] = '\0';
		  SendMessage(hWndLBox, LB_GETTEXT, 2, (DWORD)C2);
		  sprintf(C, "LB_GETTEXT returned '%s'   ", C2);
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
		  hWndCBox = CreateWindow("COMBOBOX", "Combo #1",
                    WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST,
						  230, 270, 150, 100, hWndMain, 1006, hInst, NULL);
		  SendMessage(hWndCBox, CB_ADDSTRING, 0, (LPARAM)"ComboBox item #1");
		  SendMessage(hWndCBox, CB_ADDSTRING, 0, (LPARAM)"ComboBox item #2");
        break;
     case 'N':
		  hWndCBox = CreateWindow("COMBOBOX", "Combo #2",
                    WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWN,
						  30, 270, 150, 100, hWndMain, 1007, hInst, NULL);
		  SendMessage(hWndCBox, CB_ADDSTRING, 0, (LPARAM)"ComboBox item #1");
		  SendMessage(hWndCBox, CB_ADDSTRING, 0, (LPARAM)"ComboBox item #2");
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