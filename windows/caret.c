/*
 * Caret functions
 *
 * Copyright 1993 David Metcalfe
 * Copyright 1996 Frans van Dorsselaer
 */

#include "windows.h"
#include "module.h"
#include "stddebug.h"
/* #define DEBUG_CARET */
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

static CARET Caret;


/*****************************************************************
 *              CARET_GetHwnd
 */
HWND32 CARET_GetHwnd()
{
    return Caret.hwnd;
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
    if (!(hdc = GetDCEx32( Caret.hwnd, 0, DCX_USESTYLE | DCX_CACHE ))) return;
    hPrevBrush = SelectObject32( hdc, Caret.hBrush );
    PatBlt32( hdc, Caret.x, Caret.y, Caret.width, Caret.height, PATINVERT );
    SelectObject32( hdc, hPrevBrush );
    ReleaseDC32( Caret.hwnd, hdc );
}

  
/*****************************************************************
 *               CARET_Callback
 */
static VOID CARET_Callback( HWND32 hwnd, UINT32 msg, UINT32 id, DWORD ctime)
{
    dprintf_caret(stddeb,"CARET_Callback: hwnd=%04x, timerid=%d, caret=%d\n",
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
 *           CreateCaret   (USER.163) (USER32.65)
 */
BOOL16 CreateCaret( HWND32 hwnd, HBITMAP32 bitmap, INT32 width, INT32 height )
{
    dprintf_caret(stddeb,"CreateCaret: hwnd=%04x\n", hwnd);

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
        Caret.hBrush = CreatePatternBrush32( bitmap );
    }
    else
    {
        Caret.width = width ? width : GetSystemMetrics(SM_CXBORDER);
        Caret.height = height ? height : GetSystemMetrics(SM_CYBORDER);
        Caret.hBrush = CreateSolidBrush32(bitmap ? GetSysColor(COLOR_GRAYTEXT):
                                          GetSysColor(COLOR_WINDOW) );
    }

    Caret.hwnd = hwnd;
    Caret.hidden = 1;
    Caret.on = FALSE;
    Caret.x = 0;
    Caret.y = 0;

    Caret.timeout = GetProfileInt( "windows", "CursorBlinkRate", 750 );
    return TRUE;
}
   

/*****************************************************************
 *           DestroyCaret   (USER.164) (USER32.130)
 */
BOOL16 DestroyCaret(void)
{
    if (!Caret.hwnd) return FALSE;

    dprintf_caret(stddeb,"DestroyCaret: hwnd=%04x, timerid=%d\n",
		Caret.hwnd, Caret.timerid);

    DeleteObject32( Caret.hBrush );
    CARET_KillTimer();
    CARET_DisplayCaret(CARET_OFF);
    Caret.hwnd = 0;
    return TRUE;
}


/*****************************************************************
 *           SetCaretPos   (USER.165) (USER32.465)
 */
BOOL16 SetCaretPos( INT32 x, INT32 y)
{
    if (!Caret.hwnd) return FALSE;
    if ((x == Caret.x) && (y == Caret.y)) return TRUE;

    dprintf_caret(stddeb,"SetCaretPos: x=%d, y=%d\n", x, y);

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
 *           HideCaret   (USER.166) (USER32.316)
 */
BOOL16 HideCaret( HWND32 hwnd )
{
    if (!Caret.hwnd) return FALSE;
    if (hwnd && (Caret.hwnd != hwnd)) return FALSE;

    dprintf_caret(stddeb,"HideCaret: hwnd=%04x, hidden=%d\n",
                  hwnd, Caret.hidden);

    CARET_KillTimer();
    CARET_DisplayCaret(CARET_OFF);
    Caret.hidden++;
    return TRUE;
}


/*****************************************************************
 *           ShowCaret   (USER.167) (USER32.528)
 */
BOOL16 ShowCaret( HWND32 hwnd )
{
    if (!Caret.hwnd) return FALSE;
    if (hwnd && (Caret.hwnd != hwnd)) return FALSE;

    dprintf_caret(stddeb,"ShowCaret: hwnd=%04x, hidden=%d\n",
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
 *           SetCaretBlinkTime   (USER.168) (USER32.464)
 */
BOOL16 SetCaretBlinkTime( UINT32 msecs )
{
    if (!Caret.hwnd) return FALSE;

    dprintf_caret(stddeb,"SetCaretBlinkTime: hwnd=%04x, msecs=%d\n",
		Caret.hwnd, msecs);

    Caret.timeout = msecs;
    CARET_ResetTimer();
    return TRUE;
}


/*****************************************************************
 *           GetCaretBlinkTime16   (USER.169)
 */
UINT16 GetCaretBlinkTime16(void)
{
    return (UINT16)GetCaretBlinkTime32();
}


/*****************************************************************
 *           GetCaretBlinkTime32   (USER32.208)
 */
UINT32 GetCaretBlinkTime32(void)
{
    if (!Caret.hwnd) return 0;
    return Caret.timeout;
}


/*****************************************************************
 *           GetCaretPos16   (USER.183)
 */
VOID GetCaretPos16( LPPOINT16 pt )
{
    if (!Caret.hwnd || !pt) return;

    dprintf_caret(stddeb,"GetCaretPos: hwnd=%04x, pt=%p, x=%d, y=%d\n",
                  Caret.hwnd, pt, Caret.x, Caret.y);
    pt->x = (INT16)Caret.x;
    pt->y = (INT16)Caret.y;
}


/*****************************************************************
 *           GetCaretPos32   (USER32.209)
 */
BOOL32 GetCaretPos32( LPPOINT32 pt )
{
    if (!Caret.hwnd || !pt) return FALSE;
    pt->x = Caret.x;
    pt->y = Caret.y;
    return TRUE;
}
