/*
 * Relay32 definitions
 *
 * Copyright 1995 Martin von Loewis
 */

#ifndef __WINE_RELAY32_H
#define __WINE_RELAY32_H

#include "struct32.h"

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

ATOM USER32_RegisterClassA(WNDCLASSA *);
LRESULT USER32_DefWindowProcA(DWORD hwnd,DWORD msg,DWORD wParam,DWORD lParam);
BOOL USER32_GetMessageA(MSG32* lpmsg,DWORD hwnd,DWORD min,DWORD max);
HDC USER32_BeginPaint(DWORD hwnd,PAINTSTRUCT32 *lpps);
BOOL USER32_EndPaint(DWORD hwnd,PAINTSTRUCT32 *lpps);
#endif

