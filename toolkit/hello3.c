#include <windows.h>
#include "hello3res.h"

BOOL CALLBACK DlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_INITDIALOG:
			return 1;
		case WM_COMMAND:
		if(wParam==100)
			DestroyWindow(hWnd);
		return 0;
	}
	return 0;
}

LRESULT WndProc (HWND wnd, UINT msg, WPARAM w, LPARAM l)
{
    switch (msg){

	case WM_COMMAND:
	switch(w){
		case 100:
			CreateDialogIndirect(0,hello3_DIALOG_DIADEMO.bytes,wnd,(WNDPROC)DlgProc);
			return 0;
		case 101:
		{
			BITMAPINFO *bm=hello3_BITMAP_BITDEMO.bytes;
			char *bits=bm;
			HDC hdc=GetDC(wnd);
			bits+=bm->bmiHeader.biSize;
			bits+=(1<<bm->bmiHeader.biBitCount)*sizeof(RGBQUAD);
			SetDIBitsToDevice(hdc,0,0,bm->bmiHeader.biWidth,
				bm->bmiHeader.biHeight,0,0,0,bm->bmiHeader.biHeight,
				bits,bm,DIB_RGB_COLORS);
			ReleaseDC(wnd,hdc);
			return 0;
		}
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
    HWND     wnd;
    MSG      msg;
    WNDCLASS class;

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
	class.lpszClassName = (SEGPTR)"class";
    }
    if (!RegisterClass (&class))
	return FALSE;

    wnd = CreateWindow ("class", "Test app", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 
			LoadMenuIndirect(hello3_MENU_MAIN.bytes), inst, 0);
    ShowWindow (wnd, show);
    UpdateWindow (wnd);

    while (GetMessage (&msg, 0, 0, 0)){
	TranslateMessage (&msg);
	DispatchMessage (&msg);
    }
    return 0;
}
