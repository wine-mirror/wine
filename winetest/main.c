/*	MAIN.C
 *
 *	PURPOSE: 
 *
 *	FUNCTIONS:
 *          WinMain() - Initializes app, calls all other functions.
 */

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/*
 *	Globals
 */
char	szAppName[] = "WineTest";

extern long FAR PASCAL WineTestWndProc(HWND hwnd, unsigned message,
                                       WORD wParam, LONG lParam);
/* extern void FAR __cdecl DebugPrintString(const char FAR *str); */

/*						WinMain
 */
int PASCAL 
WinMain(HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdLine, int cmdShow)
{
    DebugPrintString("Hello\n");

    return 0;
#if 0    
    HWND	hwnd;
    MSG		msg;
    WNDCLASS	wndclass;

    if (hPrevInstance)
    {
	MessageBox(NULL, "This application is already running.", szAppName, 
			MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
	return NULL;
    }

    wndclass.style		= CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc	= WineTestWndProc;
    wndclass.cbClsExtra		= 0;
    wndclass.cbWndExtra		= 0;
    wndclass.hInstance		= hInstance;
    wndclass.hIcon		= LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground	= GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName	= "MainMenu";
    wndclass.lpszClassName	= szAppName;

    RegisterClass(&wndclass);

    hwnd = CreateWindow(szAppName, "Wine Tester",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			NULL, 
    			NULL,
			hInstance, 
			NULL);
    ShowWindow(hwnd, cmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, NULL, NULL))
    {
        TranslateMessage((LPMSG) &msg);
	DispatchMessage((LPMSG) &msg);
    }

    return msg.wParam;
#endif /* 0 */
}


/*						WineTestWndProc
 */
long FAR PASCAL 
WineTestWndProc(HWND hwnd, unsigned message, WORD wParam, LONG lParam)
{
    static HANDLE hInstance;
    FARPROC DlgProcInst;
    LONG parm;

    switch (message)
    {
	case WM_CREATE:
	    hInstance = ((LPCREATESTRUCT) lParam)->hInstance;
	    return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}
