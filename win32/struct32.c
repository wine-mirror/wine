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

void STRUCT32_POINT32to16(const POINT32* p32, POINT16* p16)
{
	p16->x = p32->x;
	p16->y = p32->y;
}

void STRUCT32_POINT16to32(const POINT16* p16, POINT32* p32)
{
	p32->x = p16->x;
	p32->y = p16->y;
}

void STRUCT32_SIZE16to32(const SIZE16* p16, SIZE32* p32) 
  
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

void STRUCT32_RECT32to16(const RECT32* r32, RECT16 *r16)
{
	r16->left = r32->left;
	r16->right = r32->right;
	r16->top = r32->top;
	r16->bottom = r32->bottom;
}

void STRUCT32_RECT16to32(const RECT16* r16, RECT32 *r32)
{
	r32->left = r16->left;
	r32->right = r16->right;
	r32->top = r16->top;
	r32->bottom = r16->bottom;
}

void STRUCT32_MINMAXINFO32to16( const MINMAXINFO32 *from, MINMAXINFO16 *to )
{
    CONV_POINT32TO16( &from->ptReserved,     &to->ptReserved );
    CONV_POINT32TO16( &from->ptMaxSize,      &to->ptMaxSize );
    CONV_POINT32TO16( &from->ptMaxPosition,  &to->ptMaxPosition );
    CONV_POINT32TO16( &from->ptMinTrackSize, &to->ptMinTrackSize );
    CONV_POINT32TO16( &from->ptMaxTrackSize, &to->ptMaxTrackSize );
}

void STRUCT32_MINMAXINFO16to32( const MINMAXINFO16 *from, MINMAXINFO32 *to )
{
    CONV_POINT16TO32( &from->ptReserved,     &to->ptReserved );
    CONV_POINT16TO32( &from->ptMaxSize,      &to->ptMaxSize );
    CONV_POINT16TO32( &from->ptMaxPosition,  &to->ptMaxPosition );
    CONV_POINT16TO32( &from->ptMinTrackSize, &to->ptMinTrackSize );
    CONV_POINT16TO32( &from->ptMaxTrackSize, &to->ptMaxTrackSize );
}

void STRUCT32_WINDOWPOS32to16( const WINDOWPOS32* from, WINDOWPOS16* to )
{
    to->hwnd            = (HWND16)from->hwnd;
    to->hwndInsertAfter = (HWND16)from->hwndInsertAfter;
    to->x               = (INT16)from->x;
    to->y               = (INT16)from->y;
    to->cx              = (INT16)from->cx;
    to->cy              = (INT16)from->cy;
    to->flags           = (UINT16)from->flags;
}

void STRUCT32_WINDOWPOS16to32( const WINDOWPOS16* from, WINDOWPOS32* to )
{
    to->hwnd            = (HWND32)from->hwnd;
    to->hwndInsertAfter = (HWND32)from->hwndInsertAfter;
    to->x               = (INT32)from->x;
    to->y               = (INT32)from->y;
    to->cx              = (INT32)from->cx;
    to->cy              = (INT32)from->cy;
    to->flags           = (UINT32)from->flags;
}

void STRUCT32_NCCALCSIZE32to16Flat( const NCCALCSIZE_PARAMS32* from,
                                    NCCALCSIZE_PARAMS16* to, int validRects )
{
    CONV_RECT32TO16( &from->rgrc[0], &to->rgrc[0] );
    if (validRects)
    {
        CONV_RECT32TO16( &from->rgrc[1], &to->rgrc[1] );
        CONV_RECT32TO16( &from->rgrc[2], &to->rgrc[2] );
    }
}

void STRUCT32_NCCALCSIZE16to32Flat( const NCCALCSIZE_PARAMS16* from,
                                    NCCALCSIZE_PARAMS32* to, int validRects )
{
    CONV_RECT16TO32( &from->rgrc[0], &to->rgrc[0] );
    if (validRects)
    {
        CONV_RECT32TO16( &from->rgrc[1], &to->rgrc[1] );
        CONV_RECT32TO16( &from->rgrc[2], &to->rgrc[2] );
    }
}

/* The strings are not copied */
void STRUCT32_CREATESTRUCT32Ato16( const CREATESTRUCT32A* from,
                                   CREATESTRUCT16* to )
{
    to->lpCreateParams = from->lpCreateParams;
    to->hInstance      = (HINSTANCE16)from->hInstance;
    to->hMenu          = (HMENU16)from->hMenu;
    to->hwndParent     = (HWND16)from->hwndParent;
    to->cy             = (INT16)from->cy;
    to->cx             = (INT16)from->cx;
    to->y              = (INT16)from->y;
    to->x              = (INT16)from->x;
    to->style          = from->style;
    to->dwExStyle      = from->dwExStyle;
}

void STRUCT32_CREATESTRUCT16to32A( const CREATESTRUCT16* from,
                                   CREATESTRUCT32A *to )
{
    to->lpCreateParams = from->lpCreateParams;
    to->hInstance      = (HINSTANCE32)from->hInstance;
    to->hMenu          = (HMENU32)from->hMenu;
    to->hwndParent     = (HWND32)from->hwndParent;
    to->cy             = (INT32)from->cy;
    to->cx             = (INT32)from->cx;
    to->y              = (INT32)from->y;
    to->x              = (INT32)from->x;
    to->style          = from->style;
    to->dwExStyle      = from->dwExStyle;
}
