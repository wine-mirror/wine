/*
 * Win32 structure conversion functions
 *
 * Copyright 1996 Martin von Loewis
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "struct32.h"
#include "wownt32.h"
#include "win.h"
#include "winerror.h"

void STRUCT32_MSG16to32(const MSG16 *msg16,MSG *msg32)
{
	msg32->hwnd = WIN_Handle32(msg16->hwnd);
	msg32->message=msg16->message;
	msg32->wParam=msg16->wParam;
	msg32->lParam=msg16->lParam;
	msg32->time=msg16->time;
	msg32->pt.x=msg16->pt.x;
	msg32->pt.y=msg16->pt.y;
}

void STRUCT32_MSG32to16(const MSG *msg32,MSG16 *msg16)
{
	msg16->hwnd = HWND_16(msg32->hwnd);
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
    to->hwnd            = HWND_16(from->hwnd);
    to->hwndInsertAfter = HWND_16(from->hwndInsertAfter);
    to->x               = from->x;
    to->y               = from->y;
    to->cx              = from->cx;
    to->cy              = from->cy;
    to->flags           = from->flags;
}

void STRUCT32_WINDOWPOS16to32( const WINDOWPOS16* from, WINDOWPOS* to )
{
    to->hwnd            = WIN_Handle32(from->hwnd);
    to->hwndInsertAfter = (from->hwndInsertAfter == (HWND16)-1) ?
                           HWND_TOPMOST : WIN_Handle32(from->hwndInsertAfter);
    to->x               = from->x;
    to->y               = from->y;
    to->cx              = from->cx;
    to->cy              = from->cy;
    to->flags           = from->flags;
}

/* The strings are not copied */
void STRUCT32_CREATESTRUCT32Ato16( const CREATESTRUCTA* from,
                                   CREATESTRUCT16* to )
{
    to->lpCreateParams = from->lpCreateParams;
    to->hInstance      = HINSTANCE_16(from->hInstance);
    to->hMenu          = HMENU_16(from->hMenu);
    to->hwndParent     = HWND_16(from->hwndParent);
    to->cy             = from->cy;
    to->cx             = from->cx;
    to->y              = from->y;
    to->x              = from->x;
    to->style          = from->style;
    to->dwExStyle      = from->dwExStyle;
}

void STRUCT32_CREATESTRUCT16to32A( const CREATESTRUCT16* from,
                                   CREATESTRUCTA *to )
{
    to->lpCreateParams = from->lpCreateParams;
    to->hInstance      = HINSTANCE_32(from->hInstance);
    to->hMenu          = HMENU_32(from->hMenu);
    to->hwndParent     = WIN_Handle32(from->hwndParent);
    to->cy             = from->cy;
    to->cx             = from->cx;
    to->y              = from->y;
    to->x              = from->x;
    to->style          = from->style;
    to->dwExStyle      = from->dwExStyle;
}

/* The strings are not copied */
void STRUCT32_MDICREATESTRUCT32Ato16( const MDICREATESTRUCTA* from,
                                      MDICREATESTRUCT16* to )
{
    to->hOwner = HINSTANCE_16(from->hOwner);
    to->x      = from->x;
    to->y      = from->y;
    to->cx     = from->cx;
    to->cy     = from->cy;
    to->style  = from->style;
    to->lParam = from->lParam;
}

void STRUCT32_MDICREATESTRUCT16to32A( const MDICREATESTRUCT16* from,
                                      MDICREATESTRUCTA *to )
{
    to->hOwner = HINSTANCE_32(from->hOwner);
    to->x      = from->x;
    to->y      = from->y;
    to->cx     = from->cx;
    to->cy     = from->cy;
    to->style  = from->style;
    to->lParam = from->lParam;
}

