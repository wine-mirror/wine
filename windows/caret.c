/*
 * Caret functions
 *
 * Copyright 1993 David Metcalfe
 * Copyright 1996 Frans van Dorsselaer
 */

#include "windef.h"
#include "wingdi.h"
#include "wine/wingdi16.h"
#include "wine/winuser16.h"
#include "module.h"
#include "win.h"
#include "winuser.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(caret)

typedef struct
{
    HWND     hwnd;
    UINT     hidden;
    BOOL     on;
    INT      x;
    INT      y;
    INT      width;
    INT      height;
    HBRUSH16   hBrush;
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
    HBRUSH16 hPrevBrush;

    if (Caret.on && (status == CARET_ON)) return;
    if (!Caret.on && (status == CARET_OFF)) return;

    /* So now it's always a toggle */

    Caret.on = !Caret.on;
    /* do not use DCX_CACHE here, for x,y,width,height are in logical units */
    if (!(hdc = GetDCEx( Caret.hwnd, 0, DCX_USESTYLE /*| DCX_CACHE*/ ))) return;
    hPrevBrush = SelectObject( hdc, Caret.hBrush );
    PatBlt( hdc, Caret.x, Caret.y, Caret.width, Caret.height, PATINVERT );
    SelectObject( hdc, hPrevBrush );
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
 *           CreateCaret16   (USER.163)
 */
void WINAPI CreateCaret16( HWND16 hwnd, HBITMAP16 bitmap,
                           INT16 width, INT16 height )
{
    CreateCaret( hwnd, bitmap, width, height );
}

/*****************************************************************
 *           CreateCaret   (USER32.66)
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
        BITMAP16 bmp;
        if (!GetObject16( bitmap, sizeof(bmp), &bmp )) return FALSE;
        Caret.width = bmp.bmWidth;
        Caret.height = bmp.bmHeight;
        /* FIXME: we should make a copy of the bitmap instead of a brush */
        Caret.hBrush = CreatePatternBrush( bitmap );
    }
    else
    {
        Caret.width = width ? width : GetSystemMetrics(SM_CXBORDER);
        Caret.height = height ? height : GetSystemMetrics(SM_CYBORDER);
        Caret.hBrush = CreateSolidBrush(bitmap ?
                                          GetSysColor(COLOR_GRAYTEXT) :
                                          GetSysColor(COLOR_WINDOW) );
    }

    Caret.hwnd = hwnd;
    Caret.hidden = 1;
    Caret.on = FALSE;
    Caret.x = 0;
    Caret.y = 0;

    Caret.timeout = GetProfileIntA( "windows", "CursorBlinkRate", 500 );
    return TRUE;
}
   

/*****************************************************************
 *           DestroyCaret16   (USER.164)
 */
void WINAPI DestroyCaret16(void)
{
    DestroyCaret();
}


/*****************************************************************
 *           DestroyCaret   (USER32.131)
 */
BOOL WINAPI DestroyCaret(void)
{
    if (!Caret.hwnd) return FALSE;

    TRACE("hwnd=%04x, timerid=%d\n",
		Caret.hwnd, Caret.timerid);

    CARET_KillTimer();
    CARET_DisplayCaret(CARET_OFF);
    DeleteObject( Caret.hBrush );
    Caret.hwnd = 0;
    return TRUE;
}


/*****************************************************************
 *           SetCaretPos16   (USER.165)
 */
void WINAPI SetCaretPos16( INT16 x, INT16 y )
{
    SetCaretPos( x, y );
}


/*****************************************************************
 *           SetCaretPos   (USER32.466)
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
 *           HideCaret16   (USER.166)
 */
void WINAPI HideCaret16( HWND16 hwnd )
{
    HideCaret( hwnd );
}


/*****************************************************************
 *           HideCaret   (USER32.317)
 */
BOOL WINAPI HideCaret( HWND hwnd )
{
    if (!Caret.hwnd) return FALSE;
    if (hwnd && (Caret.hwnd != hwnd)) return FALSE;

    TRACE("hwnd=%04x, hidden=%d\n",
                  hwnd, Caret.hidden);

    CARET_KillTimer();
    CARET_DisplayCaret(CARET_OFF);
    Caret.hidden++;
    return TRUE;
}


/*****************************************************************
 *           ShowCaret16   (USER.167)
 */
void WINAPI ShowCaret16( HWND16 hwnd )
{
    ShowCaret( hwnd );
}


/*****************************************************************
 *           ShowCaret   (USER32.529)
 */
BOOL WINAPI ShowCaret( HWND hwnd )
{
    if (!Caret.hwnd) return FALSE;
    if (hwnd && (Caret.hwnd != hwnd)) return FALSE;

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
 *           SetCaretBlinkTime16   (USER.168)
 */
void WINAPI SetCaretBlinkTime16( UINT16 msecs )
{
    SetCaretBlinkTime( msecs );
}

/*****************************************************************
 *           SetCaretBlinkTime   (USER32.465)
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
 *           GetCaretBlinkTime16   (USER.169)
 */
UINT16 WINAPI GetCaretBlinkTime16(void)
{
    return (UINT16)GetCaretBlinkTime();
}


/*****************************************************************
 *           GetCaretBlinkTime   (USER32.209)
 */
UINT WINAPI GetCaretBlinkTime(void)
{
    return Caret.timeout;
}


/*****************************************************************
 *           GetCaretPos16   (USER.183)
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
 *           GetCaretPos   (USER32.210)
 */
BOOL WINAPI GetCaretPos( LPPOINT pt )
{
    if (!Caret.hwnd || !pt) return FALSE;
    pt->x = Caret.x;
    pt->y = Caret.y;
    return TRUE;
}
