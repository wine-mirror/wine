/*
 * Win32 structure conversion functions
 *
 * Copyright 1996 Martin von Loewis
 */

#include <stdio.h>
#include "windows.h"
#include "winerror.h"
#include "struct32.h"
#include "stddebug.h"
#include "debug.h"

void STRUCT32_POINT32to16(const POINT32* p32,POINT* p16)
{
	p16->x = p32->x;
	p16->y = p32->y;
}

void STRUCT32_POINT16to32(const POINT* p16,POINT32* p32)
{
	p32->x = p16->x;
	p32->y = p16->y;
}

void STRUCT32_SIZE16to32(const SIZE* p16, SIZE32* p32) 
  
{
        p32->cx = p16->cx;
        p32->cy = p16->cy;
}

void STRUCT32_MSG16to32(MSG *msg16,MSG32 *msg32)
{
	msg32->hwnd=(DWORD)msg16->hwnd;
	msg32->message=msg16->message;
	msg32->wParam=msg16->wParam;
	msg32->lParam=msg16->lParam;
	msg32->time=msg16->time;
	msg32->pt.x=msg16->pt.x;
	msg32->pt.y=msg16->pt.y;
}

void STRUCT32_MSG32to16(MSG32 *msg32,MSG *msg16)
{
	msg16->hwnd=(HWND)msg32->hwnd;
	msg16->message=msg32->message;
	msg16->wParam=msg32->wParam;
	msg16->lParam=msg32->lParam;
	msg16->time=msg32->time;
	msg16->pt.x=msg32->pt.x;
	msg16->pt.y=msg32->pt.y;
}

void STRUCT32_RECT32to16(const RECT32* r32,RECT *r16)
{
	r16->left = r32->left;
	r16->right = r32->right;
	r16->top = r32->top;
	r16->bottom = r32->bottom;
}

void STRUCT32_RECT16to32(const RECT* r16,RECT32 *r32)
{
	r32->left = r16->left;
	r32->right = r16->right;
	r32->top = r16->top;
	r32->bottom = r16->bottom;
}

void STRUCT32_MINMAXINFO32to16(const MINMAXINFO32 *from,MINMAXINFO *to)
{
	STRUCT32_POINT32to16(&from->ptReserved,&to->ptReserved);
	STRUCT32_POINT32to16(&from->ptMaxSize,&to->ptMaxSize);
	STRUCT32_POINT32to16(&from->ptMaxPosition,&to->ptMaxPosition);
	STRUCT32_POINT32to16(&from->ptMinTrackSize,&to->ptMinTrackSize);
	STRUCT32_POINT32to16(&from->ptMaxTrackSize,&to->ptMaxTrackSize);
}

void STRUCT32_MINMAXINFO16to32(const MINMAXINFO *from,MINMAXINFO32 *to)
{
	STRUCT32_POINT16to32(&from->ptReserved,&to->ptReserved);
	STRUCT32_POINT16to32(&from->ptMaxSize,&to->ptMaxSize);
	STRUCT32_POINT16to32(&from->ptMaxPosition,&to->ptMaxPosition);
	STRUCT32_POINT16to32(&from->ptMinTrackSize,&to->ptMinTrackSize);
	STRUCT32_POINT16to32(&from->ptMaxTrackSize,&to->ptMaxTrackSize);
}

void STRUCT32_WINDOWPOS32to16(const WINDOWPOS32* from,WINDOWPOS* to)
{
	to->hwnd=from->hwnd;
	to->hwndInsertAfter=from->hwndInsertAfter;
	to->x=from->x;
	to->y=from->y;
	to->cx=from->cx;
	to->flags=from->flags;
}

void STRUCT32_WINDOWPOS16to32(const WINDOWPOS* from,WINDOWPOS32* to)
{
	to->hwnd=from->hwnd;
	to->hwndInsertAfter=from->hwndInsertAfter;
	to->x=from->x;
	to->y=from->y;
	to->cx=from->cx;
	to->flags=from->flags;
}

void STRUCT32_NCCALCSIZE32to16Flat(const NCCALCSIZE_PARAMS32* from,
	NCCALCSIZE_PARAMS* to)
{
	STRUCT32_RECT32to16(from->rgrc,to->rgrc);
	STRUCT32_RECT32to16(from->rgrc+1,to->rgrc+1);
	STRUCT32_RECT32to16(from->rgrc+2,to->rgrc+2);
}

void STRUCT32_NCCALCSIZE16to32Flat(const NCCALCSIZE_PARAMS* from,
	NCCALCSIZE_PARAMS32* to)
{
	STRUCT32_RECT16to32(from->rgrc,to->rgrc);
	STRUCT32_RECT16to32(from->rgrc+1,to->rgrc+1);
	STRUCT32_RECT16to32(from->rgrc+2,to->rgrc+2);
}

/* The strings are not copied */
void STRUCT32_CREATESTRUCT32to16(const CREATESTRUCT32* from,CREATESTRUCT* to)
{
	to->lpCreateParams = (LPVOID)from->lpCreateParams;
	to->hInstance = from->hInstance;
	to->hMenu = from->hMenu;
	to->hwndParent = from->hwndParent;
	to->cy = from->cy;
	to->cx = from->cx;
	to->y = from->y;
	to->style = from->style;
	to->dwExStyle = from->dwExStyle;
}

void STRUCT32_CREATESTRUCT16to32(const CREATESTRUCT* from,CREATESTRUCT32 *to)
{
	to->lpCreateParams = (DWORD)from->lpCreateParams;
	to->hInstance = from->hInstance;
	to->hMenu = from->hMenu;
	to->hwndParent = from->hwndParent;
	to->cy = from->cy;
	to->cx = from->cx;
	to->y = from->y;
	to->style = from->style;
	to->dwExStyle = from->dwExStyle;
}
