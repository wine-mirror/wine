/*
 * Relay32 definitions
 *
 * Copyright 1995 Martin von Loewis
 */

void RELAY32_Unimplemented(char *dll, int item);
void *RELAY32_GetEntryPoint(char *dll_name, char *item, int hint);
LONG RELAY32_CallWindowProc(WNDPROC,int,int,int,int);
void RELAY32_DebugEnter(char *dll,char *name);

typedef struct tagWNDCLASSA{
	UINT	style;
	WNDPROC	lpfnWndProc;
	int		cbClsExtra;
	int		cbWndExtra;
	DWORD	hInstance;
	DWORD	hIcon;
	DWORD	hCursor;
	DWORD	hbrBackground;
	char*	lpszMenuName;
	char*	lpszClassName;
}WNDCLASSA;

struct WIN32_POINT{
	LONG x;
	LONG y;
};

struct WIN32_MSG{
	DWORD hwnd;
	DWORD message;
	DWORD wParam;
	DWORD lParam;
	DWORD time;
	struct WIN32_POINT pt;
};

struct WIN32_RECT{
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
};

struct WIN32_PAINTSTRUCT{
	DWORD hdc;
	DWORD fErase;
	struct WIN32_RECT rcPaint;
	DWORD fRestore;
	DWORD fIncUpdate;
	BYTE rgbReserved[32];
};


ATOM USER32_RegisterClassA(WNDCLASSA *);
LRESULT USER32_DefWindowProcA(DWORD hwnd,DWORD msg,DWORD wParam,DWORD lParam);
BOOL USER32_GetMessageA(struct WIN32_MSG* lpmsg,DWORD hwnd,DWORD min,DWORD max);
HDC USER32_BeginPaint(DWORD hwnd,struct WIN32_PAINTSTRUCT *lpps);
BOOL USER32_EndPaint(DWORD hwnd,struct WIN32_PAINTSTRUCT *lpps);
