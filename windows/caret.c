/*
 * Caret functions
 *
 * Copyright 1993 David Metcalfe
 * Copyright 1996 Frans van Dorsselaer
 * Copyright 2001 Eric Pouech
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/wingdi16.h"
#include "wine/winuser16.h"
#include "win.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(caret);

typedef struct
{
    HWND     hwnd;
    UINT     hidden;
    BOOL     on;
    INT      x;
    INT      y;
    INT      width;
    INT      height;
    HBITMAP  hBmp;
    UINT     timeout;
    UINT     timerid;
} CARET;

typedef enum
{
    CARET_OFF = 0,
    CARET_ON,
    CARET_TOGGLE
} DISPLAY_CARET;

static CARET Caret = { 0, 0, FALSE, 0, 0, 2, 12, 0, 500, 0 };

/*****************************************************************
 *              CARET_GetHwnd
 */
HWND CARET_GetHwnd(void)
{
    return Caret.hwnd;
}

/*****************************************************************
 *              CARET_GetRect
 */
void CARET_GetRect(LPRECT lprc)
{
    lprc->right = (lprc->left = Caret.x) + Caret.width - 1;
    lprc->bottom = (lprc->top = Caret.y) + Caret.height - 1;
}

/*****************************************************************
 *               CARET_DisplayCaret
 */
static void CARET_DisplayCaret( DISPLAY_CARET status )
{
    HDC hdc;
    HDC hCompDC;

    if (Caret.on && (status == CARET_ON)) return;
    if (!Caret.on && (status == CARET_OFF)) return;

    /* So now it's always a toggle */

    Caret.on = !Caret.on;
    /* do not use DCX_CACHE here, for x,y,width,height are in logical units */
    if (!(hdc = GetDCEx( Caret.hwnd, 0, DCX_USESTYLE /*| DCX_CACHE*/ ))) return;
    hCompDC = CreateCompatibleDC(hdc);
    if (hCompDC)
    {
	HBITMAP	hPrevBmp;

	hPrevBmp = SelectObject(hCompDC, Caret.hBmp);
	BitBlt(hdc, Caret.x, Caret.y, Caret.width, Caret.height, hCompDC, 0, 0, SRCINVERT);
	SelectObject(hCompDC, hPrevBmp);
	DeleteDC(hCompDC);
    }
    ReleaseDC( Caret.hwnd, hdc );
}

  
/*****************************************************************
 *               CARET_Callback
 */
static VOID CALLBACK CARET_Callback( HWND hwnd, UINT msg, UINT id, DWORD ctime)
{
    TRACE("hwnd=%04x, timerid=%d, caret=%d\n",
                  hwnd, id, Caret.on);
    CARET_DisplayCaret(CARET_TOGGLE);
}


/*****************************************************************
 *               CARET_SetTimer
 */
static void CARET_SetTimer(void)
{
    if (Caret.timerid) KillSystemTimer( (HWND)0, Caret.timerid );
    Caret.timerid = SetSystemTimer( (HWND)0, 0, Caret.timeout,
                                      CARET_Callback );
}


/*****************************************************************
 *               CARET_ResetTimer
 */
static void CARET_ResetTimer(void)
{
    if (Caret.timerid) 
    {
	KillSystemTimer( (HWND)0, Caret.timerid );
	Caret.timerid = SetSystemTimer( (HWND)0, 0, Caret.timeout,
                                          CARET_Callback );
    }
}


/*****************************************************************
 *               CARET_KillTimer
 */
static void CARET_KillTimer(void)
{
    if (Caret.timerid) 
    {
	KillSystemTimer( (HWND)0, Caret.timerid );
	Caret.timerid = 0;
    }
}


/*****************************************************************
 *		CreateCaret (USER32.@)
 */
BOOL WINAPI CreateCaret( HWND hwnd, HBITMAP bitmap,
                             INT width, INT height )
{
    TRACE("hwnd=%04x\n", hwnd);

    if (!hwnd) return FALSE;

    /* if cursor already exists, destroy it */
    if (Caret.hwnd) DestroyCaret();

    if (bitmap && (bitmap != 1))
    {
        BITMAP bmp;
        if (!GetObjectA( bitmap, sizeof(bmp), &bmp )) return FALSE;
        Caret.width = bmp.bmWidth;
        Caret.height = bmp.bmHeight;
	bmp.bmBits = NULL;
	Caret.hBmp = CreateBitmapIndirect(&bmp);
 
	if (Caret.hBmp) 
	{
	    /* copy the bitmap */
	    LPBYTE buf = HeapAlloc(GetProcessHeap(), 0, bmp.bmWidthBytes * bmp.bmHeight);
	    GetBitmapBits(bitmap, bmp.bmWidthBytes * bmp.bmHeight, buf);
	    SetBitmapBits(Caret.hBmp, bmp.bmWidthBytes * bmp.bmHeight, buf);
	    HeapFree(GetProcessHeap(), 0, buf);
	}
    }
    else
    {
	HDC	hdc;

        Caret.width = width ? width : GetSystemMetrics(SM_CXBORDER);
        Caret.height = height ? height : GetSystemMetrics(SM_CYBORDER);
	Caret.hBmp = 0;

	/* create the uniform bitmap on the fly */
	hdc = GetDC(hwnd);
	if (hdc)
	{
	    HDC		hMemDC = CreateCompatibleDC(hdc);

	    if (hMemDC)
	    {
		RECT	r;
		r.left = r.top = 0;
		r.right = Caret.width;
		r.bottom = Caret.height;
		    
		if ((Caret.hBmp = CreateCompatibleBitmap(hMemDC, Caret.width, Caret.height)))
		{
		    HBITMAP hPrevBmp = SelectObject(hMemDC, Caret.hBmp);
		    FillRect(hMemDC, &r, (bitmap ? COLOR_GRAYTEXT : COLOR_WINDOW) + 1);
		    SelectObject(hMemDC, hPrevBmp);
		}
		DeleteDC(hMemDC);
	    }
	    ReleaseDC(hwnd, hdc);
	}
    }

    Caret.hwnd = WIN_GetFullHandle( hwnd );
    Caret.hidden = 1;
    Caret.on = FALSE;
    Caret.x = 0;
    Caret.y = 0;

    Caret.timeout = GetProfileIntA( "windows", "CursorBlinkRate", 500 );
    return TRUE;
}
   

/*****************************************************************
 *		DestroyCaret (USER.164)
 */
void WINAPI DestroyCaret16(void)
{
    DestroyCaret();
}


/*****************************************************************
 *		DestroyCaret (USER32.@)
 */
BOOL WINAPI DestroyCaret(void)
{
    if (!Caret.hwnd) return FALSE;

    TRACE("hwnd=%04x, timerid=%d\n",
		Caret.hwnd, Caret.timerid);

    CARET_KillTimer();
    CARET_DisplayCaret(CARET_OFF);
    DeleteObject( Caret.hBmp );
    Caret.hwnd = 0;
    return TRUE;
}


/*****************************************************************
 *		SetCaretPos (USER.165)
 */
void WINAPI SetCaretPos16( INT16 x, INT16 y )
{
    SetCaretPos( x, y );
}


/*****************************************************************
 *		SetCaretPos (USER32.@)
 */
BOOL WINAPI SetCaretPos( INT x, INT y)
{
    if (!Caret.hwnd) return FALSE;
    if ((x == Caret.x) && (y == Caret.y)) return TRUE;

    TRACE("x=%d, y=%d\n", x, y);

    CARET_KillTimer();
    CARET_DisplayCaret(CARET_OFF);
    Caret.x = x;
    Caret.y = y;
    if (!Caret.hidden)
    {
	CARET_DisplayCaret(CARET_ON);
	CARET_SetTimer();
    }
    return TRUE;
}


/*****************************************************************
 *		HideCaret (USER32.@)
 */
BOOL WINAPI HideCaret( HWND hwnd )
{
    if (!Caret.hwnd) return FALSE;
    if (hwnd && (Caret.hwnd != WIN_GetFullHandle(hwnd))) return FALSE;

    TRACE("hwnd=%04x, hidden=%d\n",
                  hwnd, Caret.hidden);

    CARET_KillTimer();
    CARET_DisplayCaret(CARET_OFF);
    Caret.hidden++;
    return TRUE;
}


/*****************************************************************
 *		ShowCaret (USER32.@)
 */
BOOL WINAPI ShowCaret( HWND hwnd )
{
    if (!Caret.hwnd) return FALSE;
    if (hwnd && (Caret.hwnd != WIN_GetFullHandle(hwnd))) return FALSE;

    TRACE("hwnd=%04x, hidden=%d\n",
		hwnd, Caret.hidden);

    if (Caret.hidden)
    {
	Caret.hidden--;
	if (!Caret.hidden)
	{
	    CARET_DisplayCaret(CARET_ON);
	    CARET_SetTimer();
	}
    }
    return TRUE;
}


/*****************************************************************
 *		SetCaretBlinkTime (USER.168)
 */
void WINAPI SetCaretBlinkTime16( UINT16 msecs )
{
    SetCaretBlinkTime( msecs );
}

/*****************************************************************
 *		SetCaretBlinkTime (USER32.@)
 */
BOOL WINAPI SetCaretBlinkTime( UINT msecs )
{
    if (!Caret.hwnd) return FALSE;

    TRACE("hwnd=%04x, msecs=%d\n",
		Caret.hwnd, msecs);

    Caret.timeout = msecs;
    CARET_ResetTimer();
    return TRUE;
}


/*****************************************************************
 *		GetCaretBlinkTime (USER.169)
 */
UINT16 WINAPI GetCaretBlinkTime16(void)
{
    return (UINT16)GetCaretBlinkTime();
}


/*****************************************************************
 *		GetCaretBlinkTime (USER32.@)
 */
UINT WINAPI GetCaretBlinkTime(void)
{
    return Caret.timeout;
}


/*****************************************************************
 *		GetCaretPos (USER.183)
 */
VOID WINAPI GetCaretPos16( LPPOINT16 pt )
{
    if (!Caret.hwnd || !pt) return;

    TRACE("hwnd=%04x, pt=%p, x=%d, y=%d\n",
                  Caret.hwnd, pt, Caret.x, Caret.y);
    pt->x = (INT16)Caret.x;
    pt->y = (INT16)Caret.y;
}


/*****************************************************************
 *		GetCaretPos (USER32.@)
 */
BOOL WINAPI GetCaretPos( LPPOINT pt )
{
    if (!Caret.hwnd || !pt) return FALSE;
    pt->x = Caret.x;
    pt->y = Caret.y;
    return TRUE;
}
