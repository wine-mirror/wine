/*
 * Win32 structure conversion functions
 *
 * Copyright 1996 Martin von Loewis
 */

#include "struct32.h"
#include "winerror.h"
#include "debug.h"

void STRUCT32_MSG16to32(const MSG16 *msg16,MSG *msg32)
{
	msg32->hwnd=(HWND)msg16->hwnd;
	msg32->message=msg16->message;
	msg32->wParam=msg16->wParam;
	msg32->lParam=msg16->lParam;
	msg32->time=msg16->time;
	msg32->pt.x=msg16->pt.x;
	msg32->pt.y=msg16->pt.y;
}

void STRUCT32_MSG32to16(const MSG *msg32,MSG16 *msg16)
{
	msg16->hwnd=(HWND16)msg32->hwnd;
	msg16->message=msg32->message;
	msg16->wParam=msg32->wParam;
	msg16->lParam=msg32->lParam;
	msg16->time=msg32->time;
	msg16->pt.x=msg32->pt.x;
	msg16->pt.y=msg32->pt.y;
}

void STRUCT32_MINMAXINFO32to16( const MINMAXINFO *from, MINMAXINFO16 *to )
{
    CONV_POINT32TO16( &from->ptReserved,     &to->ptReserved );
    CONV_POINT32TO16( &from->ptMaxSize,      &to->ptMaxSize );
    CONV_POINT32TO16( &from->ptMaxPosition,  &to->ptMaxPosition );
    CONV_POINT32TO16( &from->ptMinTrackSize, &to->ptMinTrackSize );
    CONV_POINT32TO16( &from->ptMaxTrackSize, &to->ptMaxTrackSize );
}

void STRUCT32_MINMAXINFO16to32( const MINMAXINFO16 *from, MINMAXINFO *to )
{
    CONV_POINT16TO32( &from->ptReserved,     &to->ptReserved );
    CONV_POINT16TO32( &from->ptMaxSize,      &to->ptMaxSize );
    CONV_POINT16TO32( &from->ptMaxPosition,  &to->ptMaxPosition );
    CONV_POINT16TO32( &from->ptMinTrackSize, &to->ptMinTrackSize );
    CONV_POINT16TO32( &from->ptMaxTrackSize, &to->ptMaxTrackSize );
}

void STRUCT32_WINDOWPOS32to16( const WINDOWPOS* from, WINDOWPOS16* to )
{
    to->hwnd            = (HWND16)from->hwnd;
    to->hwndInsertAfter = (HWND16)from->hwndInsertAfter;
    to->x               = (INT16)from->x;
    to->y               = (INT16)from->y;
    to->cx              = (INT16)from->cx;
    to->cy              = (INT16)from->cy;
    to->flags           = (UINT16)from->flags;
}

void STRUCT32_WINDOWPOS16to32( const WINDOWPOS16* from, WINDOWPOS* to )
{
    to->hwnd            = (HWND)from->hwnd;
    to->hwndInsertAfter = (HWND)from->hwndInsertAfter;
    to->x               = (INT)from->x;
    to->y               = (INT)from->y;
    to->cx              = (INT)from->cx;
    to->cy              = (INT)from->cy;
    to->flags           = (UINT)from->flags;
}

void STRUCT32_NCCALCSIZE32to16Flat( const NCCALCSIZE_PARAMS* from,
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
                                    NCCALCSIZE_PARAMS* to, int validRects )
{
    CONV_RECT16TO32( &from->rgrc[0], &to->rgrc[0] );
    if (validRects)
    {
        CONV_RECT32TO16( &from->rgrc[1], &to->rgrc[1] );
        CONV_RECT32TO16( &from->rgrc[2], &to->rgrc[2] );
    }
}

/* The strings are not copied */
void STRUCT32_CREATESTRUCT32Ato16( const CREATESTRUCTA* from,
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
                                   CREATESTRUCTA *to )
{
    to->lpCreateParams = from->lpCreateParams;
    to->hInstance      = (HINSTANCE)from->hInstance;
    to->hMenu          = (HMENU)from->hMenu;
    to->hwndParent     = (HWND)from->hwndParent;
    to->cy             = (INT)from->cy;
    to->cx             = (INT)from->cx;
    to->y              = (INT)from->y;
    to->x              = (INT)from->x;
    to->style          = from->style;
    to->dwExStyle      = from->dwExStyle;
}

/* The strings are not copied */
void STRUCT32_MDICREATESTRUCT32Ato16( const MDICREATESTRUCTA* from,
                                      MDICREATESTRUCT16* to )
{
    to->hOwner = (HINSTANCE16)from->hOwner;
    to->x      = (INT16)from->x;     
    to->y      = (INT16)from->y;     
    to->cx     = (INT16)from->cx;    
    to->cy     = (INT16)from->cy;    
    to->style  = from->style; 
    to->lParam = from->lParam;
}

void STRUCT32_MDICREATESTRUCT16to32A( const MDICREATESTRUCT16* from,
                                      MDICREATESTRUCTA *to )
{
    to->hOwner = (HINSTANCE)from->hOwner;
    to->x      = (INT)from->x;     
    to->y      = (INT)from->y;     
    to->cx     = (INT)from->cx;    
    to->cy     = (INT)from->cy;    
    to->style  = from->style; 
    to->lParam = from->lParam;
}

