#include <windows.h>

void Write (HDC dc, int x, int y, char *s)
{
    TextOut (dc, x, y, s, strlen (s));
}

LRESULT WndProc (HWND wnd, UINT msg, WPARAM w, LPARAM l)
{
    static short xChar, yChar;
    HDC dc;
    PAINTSTRUCT ps;
    TEXTMETRIC tm;

    switch (msg){
    case WM_CREATE:
	dc = GetDC (wnd);
	GetTextMetrics (dc, &tm);
	xChar = tm.tmAveCharWidth;
	yChar = tm.tmHeight;
	ReleaseDC (wnd, dc);
	break;

    case WM_PAINT:
	dc = BeginPaint (wnd, &ps);
	Write (dc, xChar, yChar, "Hola");
	EndPaint (wnd, &ps);
	break;

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
    HWND     wnd;
    MSG      msg;
    WNDCLASS class;
    char className[] = "class";  /* To make sure className >= 0x10000 */
    char winName[] = "Test app";

    if (!prev){
	class.style = CS_HREDRAW | CS_VREDRAW;
	class.lpfnWndProc = WndProc;
	class.cbClsExtra = 0;
	class.cbWndExtra = 0;
	class.hInstance  = inst;
	class.hIcon      = LoadIcon (0, IDI_APPLICATION);
	class.hCursor    = LoadCursor (0, IDC_ARROW);
	class.hbrBackground = GetStockObject (WHITE_BRUSH);
	class.lpszMenuName = NULL;
	class.lpszClassName = (SEGPTR)className;
    }
    if (!RegisterClass (&class))
	return FALSE;

    wnd = CreateWindow (className, winName, WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 
			0, inst, 0);
    ShowWindow (wnd, show);
    UpdateWindow (wnd);

    while (GetMessage (&msg, 0, 0, 0)){
	TranslateMessage (&msg);
	DispatchMessage (&msg);
    }
    return 0;
}
