#include <windows.h>
#include <commdlg.h>

typedef struct
{
  HANDLE  hInstance;
  HWND    hMainWnd;
  HMENU   hMainMenu;
} GLOBALS;

GLOBALS Globals;

BOOL FileOpen(HWND hWnd)
{
  char filename[80] = "test.c";
  OPENFILENAME ofn = { sizeof(OPENFILENAME),
		       hWnd, NULL, "C code\0*.c\0", NULL, 0, 0, filename, 80,
		       NULL, 0, NULL, NULL, OFN_CREATEPROMPT |
		       OFN_SHOWHELP, 0, 0, NULL, 0, NULL };
  return GetOpenFileName(&ofn);
}

LRESULT CALLBACK DlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    switch(msg)
    {
	case WM_INITDIALOG:
	    break;
	case WM_COMMAND:
	    switch (wParam)
	    {
		case 100:
		    EndDialog(hWnd, 100);
		    return TRUE;
	    }
    }
    return FALSE;
}

LRESULT CALLBACK WndProc (HWND wnd, UINT msg, WPARAM w, LPARAM l)
{
    switch (msg){

	case WM_COMMAND:
	switch(w){
		case 100:
			DialogBox(Globals.hInstance,
				"DIADEMO", wnd,
				(DLGPROC)DlgProc);
			return 0;
		case 101:
		{
			HDC hdc, hMemDC;
			HBITMAP hBitmap, hPrevBitmap;
			BITMAP bmp;

			hBitmap = LoadBitmapA (Globals.hInstance, "BITDEMO");
			hdc = GetDC (wnd);
			hMemDC = CreateCompatibleDC (hdc);
			hPrevBitmap = SelectObject (hMemDC, hBitmap);
			GetObjectA (hBitmap, sizeof(BITMAP), &bmp);
			BitBlt (hdc, 0, 0, bmp.bmWidth, bmp.bmHeight,
				hMemDC, 0, 0, SRCCOPY);
			SelectObject (hMemDC, hPrevBitmap);
			DeleteDC (hMemDC);
			ReleaseDC (wnd, hdc);
			return 0;
		}
	        case 102:
		        FileOpen(wnd);
			return 0;
		default:
			return DefWindowProc (wnd, msg, w, l);
	}
    case WM_DESTROY:
	PostQuitMessage (0);
	break;

    default:
	return DefWindowProc (wnd, msg, w, l);
    }
    return 0l;
}

int PASCAL WinMain (HANDLE inst, HANDLE prev, LPSTR cmdline, int show)
{
    MSG      msg;
    WNDCLASS class;
    char className[] = "class";  /* To make sure className >= 0x10000 */
    char winName[] = "Test app";

    Globals.hInstance = inst;
    if (!prev){
	class.style = CS_HREDRAW | CS_VREDRAW;
	class.lpfnWndProc = WndProc;
	class.cbClsExtra = 0;
	class.cbWndExtra = 0;
	class.hInstance  = inst;
	class.hIcon      = LoadIcon (0, IDI_APPLICATION);
	class.hCursor    = LoadCursor (0, IDC_ARROW);
	class.hbrBackground = GetStockObject (WHITE_BRUSH);
	class.lpszMenuName = 0;
	class.lpszClassName = className;
    }
    if (!RegisterClass (&class))
	return FALSE;

    Globals.hMainWnd = CreateWindow (className, winName, WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 
			LoadMenu(inst,"MAIN"), inst, 0);
    ShowWindow (Globals.hMainWnd, show);
    UpdateWindow (Globals.hMainWnd);

    while (GetMessage (&msg, 0, 0, 0)){
	TranslateMessage (&msg);
	DispatchMessage (&msg);
    }
    return 0;
}
