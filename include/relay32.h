/*
 * Relay32 definitions
 *
 * Copyright 1995 Martin von Loewis
 */

#ifndef _RELAY32_H
#define _RELAY32_H
#include "pe_image.h"
#include "struct32.h"

void RELAY32_Unimplemented(char *dll, int item);
WIN32_builtin *RELAY32_GetBuiltinDLL(char *name);
void *RELAY32_GetEntryPoint(WIN32_builtin *dll, char *item, int hint);
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

ATOM USER32_RegisterClassA(WNDCLASSA *);
LRESULT USER32_DefWindowProcA(DWORD hwnd,DWORD msg,DWORD wParam,DWORD lParam);
BOOL USER32_GetMessageA(MSG32* lpmsg,DWORD hwnd,DWORD min,DWORD max);
HDC USER32_BeginPaint(DWORD hwnd,PAINTSTRUCT32 *lpps);
BOOL USER32_EndPaint(DWORD hwnd,PAINTSTRUCT32 *lpps);
#endif

