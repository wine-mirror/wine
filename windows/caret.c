/*
 * Caret functions
 *
 * Copyright 1993 David Metcalfe
 * Copyright 1996 Frans van Dorsselaer
 */

#include "winuser.h"
#include "wine/winuser16.h"
#include "module.h"
#include "debug.h"

typedef struct
{
    HWND32     hwnd;
    UINT32     hidden;
    BOOL32     on;
    INT32      x;
    INT32      y;
    INT32      width;
    INT32      height;
    HBRUSH16   hBrush;
    UINT32     timeout;
    UINT32     timerid;
} CARET;

typedef enum
{
    CARET_OFF = 0,
    CARET_ON,
    CARET_TOGGLE,
} DISPLAY_CARET;

static CARET Caret = { 0, 0, FALSE, 0, 0, 2, 12, 0, 500, 0 };

/*****************************************************************
 *              CARET_GetHwnd
 */
HWND32 CARET_GetHwnd(void)
{
    return Caret.hwnd;
}

/*****************************************************************
 *              CARET_GetRect
 */
void CARET_GetRect(LPRECT32 lprc)
{
    lprc->right = (lprc->left = Caret.x) + Caret.width - 1;
    lprc->bottom = (lprc->top = Caret.y) + Caret.height - 1;
}

/*****************************************************************
 *               CARET_DisplayCaret
 */
static void CARET_DisplayCaret( DISPLAY_CARET status )
{
    HDC32 hdc;
    HBRUSH16 hPrevBrush;

    if (Caret.on && (status == CARET_ON)) return;
    if (!Caret.on && (status == CARET_OFF)) return;

    /* So now it's always a toggle */

    Caret.on = !Caret.on;
    /* do not use DCX_CACHE here, for x,y,width,height are in logical units */
    if (!(hdc = GetDCEx32( Caret.hwnd, 0, DCX_USESTYLE /*| DCX_CACHE*/ ))) return;
    hPrevBrush = SelectObject32( hdc, Caret.hBrush );
    PatBlt32( hdc, Caret.x, Caret.y, Caret.width, Caret.height, PATINVERT );
    SelectObject32( hdc, hPrevBrush );
    ReleaseDC32( Caret.hwnd, hdc );
}

  
/*****************************************************************
 *               CARET_Callback
 */
static VOID CALLBACK CARET_Callback( HWND32 hwnd, UINT32 msg, UINT32 id, DWORD ctime)
{
    TRACE(caret,"hwnd=%04x, timerid=%d, caret=%d\n",
                  hwnd, id, Caret.on);
    CARET_DisplayCaret(CARET_TOGGLE);
}


/*****************************************************************
 *               CARET_SetTimer
 */
static void CARET_SetTimer(void)
{
    if (Caret.timerid) KillSystemTimer32( (HWND32)0, Caret.timerid );
    Caret.timerid = SetSystemTimer32( (HWND32)0, 0, Caret.timeout,
                                      CARET_Callback );
}


/*****************************************************************
 *               CARET_ResetTimer
 */
static void CARET_ResetTimer(void)
{
    if (Caret.timerid) 
    {
	KillSystemTimer32( (HWND32)0, Caret.timerid );
	Caret.timerid = SetSystemTimer32( (HWND32)0, 0, Caret.timeout,
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
	KillSystemTimer32( (HWND32)0, Caret.timerid );
	Caret.timerid = 0;
    }
}


/*****************************************************************
 *           CreateCaret16   (USER.163)
 */
void WINAPI CreateCaret16( HWND16 hwnd, HBITMAP16 bitmap,
                           INT16 width, INT16 height )
{
    CreateCaret32( hwnd, bitmap, width, height );
}

/*****************************************************************
 *           CreateCaret32   (USER32.66)
 */
BOOL32 WINAPI CreateCaret32( HWND32 hwnd, HBITMAP32 bitmap,
                             INT32 width, INT32 height )
{
    TRACE(caret,"hwnd=%04x\n", hwnd);

    if (!hwnd) return FALSE;

    /* if cursor already exists, destroy it */
    if (Caret.hwnd) DestroyCaret32();

    if (bitmap && (bitmap != 1))
    {
        BITMAP16 bmp;
        if (!GetObject16( bitmap, sizeof(bmp), &bmp )) return FALSE;
        Caret.width = bmp.bmWidth;
        Caret.height = bmp.bmHeight;
        /* FIXME: we should make a copy of the bitmap instead of a brush */
        Caret.hBrush = CreatePatternBrush32( bitmap );
    }
    else
    {
        Caret.width = width ? width : GetSystemMetrics32(SM_CXBORDER);
        Caret.height = height ? height : GetSystemMetrics32(SM_CYBORDER);
        Caret.hBrush = CreateSolidBrush32(bitmap ?
                                          GetSysColor32(COLOR_GRAYTEXT) :
                                          GetSysColor32(COLOR_WINDOW) );
    }

    Caret.hwnd = hwnd;
    Caret.hidden = 1;
    Caret.on = FALSE;
    Caret.x = 0;
    Caret.y = 0;

    Caret.timeout = GetProfileInt32A( "windows", "CursorBlinkRate", 500 );
    return TRUE;
}
   

/*****************************************************************
 *           DestroyCaret16   (USER.164)
 */
void WINAPI DestroyCaret16(void)
{
    DestroyCaret32();
}


/*****************************************************************
 *           DestroyCaret32   (USER32.131)
 */
BOOL32 WINAPI DestroyCaret32(void)
{
    if (!Caret.hwnd) return FALSE;

    TRACE(caret,"hwnd=%04x, timerid=%d\n",
		Caret.hwnd, Caret.timerid);

    CARET_KillTimer();
    CARET_DisplayCaret(CARET_OFF);
    DeleteObject32( Caret.hBrush );
    Caret.hwnd = 0;
    return TRUE;
}


/*****************************************************************
 *           SetCaretPos16   (USER.165)
 */
void WINAPI SetCaretPos16( INT16 x, INT16 y )
{
    SetCaretPos32( x, y );
}


/*****************************************************************
 *           SetCaretPos32   (USER32.466)
 */
BOOL32 WINAPI SetCaretPos32( INT32 x, INT32 y)
{
    if (!Caret.hwnd) return FALSE;
    if ((x == Caret.x) && (y == Caret.y)) return TRUE;

    TRACE(caret,"x=%d, y=%d\n", x, y);

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
    HideCaret32( hwnd );
}


/*****************************************************************
 *           HideCaret32   (USER32.317)
 */
BOOL32 WINAPI HideCaret32( HWND32 hwnd )
{
    if (!Caret.hwnd) return FALSE;
    if (hwnd && (Caret.hwnd != hwnd)) return FALSE;

    TRACE(caret,"hwnd=%04x, hidden=%d\n",
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
    ShowCaret32( hwnd );
}


/*****************************************************************
 *           ShowCaret32   (USER32.529)
 */
BOOL32 WINAPI ShowCaret32( HWND32 hwnd )
{
    if (!Caret.hwnd) return FALSE;
    if (hwnd && (Caret.hwnd != hwnd)) return FALSE;

    TRACE(caret,"hwnd=%04x, hidden=%d\n",
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
    SetCaretBlinkTime32( msecs );
}

/*****************************************************************
 *           SetCaretBlinkTime32   (USER32.465)
 */
BOOL32 WINAPI SetCaretBlinkTime32( UINT32 msecs )
{
    if (!Caret.hwnd) return FALSE;

    TRACE(caret,"hwnd=%04x, msecs=%d\n",
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
    return (UINT16)GetCaretBlinkTime32();
}


/*****************************************************************
 *           GetCaretBlinkTime32   (USER32.209)
 */
UINT32 WINAPI GetCaretBlinkTime32(void)
{
    return Caret.timeout;
}


/*****************************************************************
 *           GetCaretPos16   (USER.183)
 */
VOID WINAPI GetCaretPos16( LPPOINT16 pt )
{
    if (!Caret.hwnd || !pt) return;

    TRACE(caret,"hwnd=%04x, pt=%p, x=%d, y=%d\n",
                  Caret.hwnd, pt, Caret.x, Caret.y);
    pt->x = (INT16)Caret.x;
    pt->y = (INT16)Caret.y;
}


/*****************************************************************
 *           GetCaretPos32   (USER32.210)
 */
BOOL32 WINAPI GetCaretPos32( LPPOINT32 pt )
{
    if (!Caret.hwnd || !pt) return FALSE;
    pt->x = Caret.x;
    pt->y = Caret.y;
    return TRUE;
}
