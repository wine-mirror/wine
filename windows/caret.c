/*
 * Caret functions
 *
 * Copyright 1993 David Metcalfe
 */

#include "windows.h"
#include "selectors.h"
#include "alias.h"
#include "relay32.h"
#include "stddebug.h"
/* #define DEBUG_CARET */
#include "debug.h"


typedef struct
{
    HWND          hwnd;
    short         hidden;
    BOOL          on;
    short         x;
    short         y;
    short         width;
    short         height;
    COLORREF      color;
    HBITMAP       bitmap;
    WORD          timeout;
    WORD          timerid;
} CARET;

typedef enum
{
    CARET_OFF = 0,
    CARET_ON,
    CARET_TOGGLE,
} DISPLAY_CARET;

static CARET Caret = { (HWND)0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


/*****************************************************************
 *              CARET_GetHwnd
 */
HWND CARET_GetHwnd()
{
    return Caret.hwnd;
}

/*****************************************************************
 *               CARET_DisplayCaret
 */
void CARET_DisplayCaret(DISPLAY_CARET status)
{
    HDC hdc;
    HBRUSH hBrush;
    HBRUSH hPrevBrush;
    HRGN rgn;

    if (Caret.on && (status == CARET_ON)) return;
    if (!Caret.on && (status == CARET_OFF)) return;

    /* So now it's always a toggle */

    Caret.on = !Caret.on;
    hdc = GetDC(Caret.hwnd);
    if (Caret.bitmap == (HBITMAP)0 || Caret.bitmap == (HBITMAP)1)
	hBrush = CreateSolidBrush(Caret.color);
    else
	hBrush = CreatePatternBrush(Caret.bitmap);
    hPrevBrush = SelectObject(hdc, (HANDLE)hBrush);
    SetROP2(hdc, R2_XORPEN);
    rgn = CreateRectRgn(Caret.x, Caret.y, 
			Caret.x + Caret.width,
			Caret.y + Caret.height);
    FillRgn(hdc, rgn, hBrush);
    DeleteObject( rgn );
    SelectObject( hdc, hPrevBrush );
    DeleteObject( hBrush );
    ReleaseDC(Caret.hwnd, hdc);
}

  
/*****************************************************************
 *               CARET_Callback
 */
WORD CARET_Callback(HWND hwnd, WORD msg, WORD timerid, LONG ctime)
{
    dprintf_caret(stddeb,"CARET_Callback: hwnd="NPFMT", timerid=%d, "
		"caret=%d\n", hwnd, timerid, Caret.on);
	
    CARET_DisplayCaret(CARET_TOGGLE);
    return 0;
}


/*****************************************************************
 *               CARET_SetTimer
 */
void CARET_SetTimer(void)
{
    if (Caret.timerid) KillSystemTimer((HWND)0, Caret.timerid);
    Caret.timerid = SetSystemTimer((HWND)0, 0, Caret.timeout,
			(FARPROC)GetWndProcEntry16("CARET_Callback"));
}


/*****************************************************************
 *               CARET_ResetTimer
 */
void CARET_ResetTimer(void)
{
    if (Caret.timerid) 
    {
	KillSystemTimer((HWND)0, Caret.timerid);
	Caret.timerid = SetSystemTimer((HWND)0, 0, Caret.timeout,
			(FARPROC)GetWndProcEntry16("CARET_Callback"));
    }
}


/*****************************************************************
 *               CARET_KillTimer
 */
void CARET_KillTimer(void)
{
    if (Caret.timerid) 
    {
	KillSystemTimer((HWND)0, Caret.timerid);
	Caret.timerid = 0;
    }
}


/*****************************************************************
 *               CARET_Initialize
 */
static void CARET_Initialize()
{
    DWORD WineProc,Win16Proc,Win32Proc;
    static int initialized=0;

    if(!initialized)
    {
	WineProc = (DWORD)CARET_Callback;
	Win16Proc = (DWORD)GetWndProcEntry16("CARET_Callback");
	Win32Proc = (DWORD)RELAY32_GetEntryPoint(
				RELAY32_GetBuiltinDLL("WINPROCS32"),
				"CARET_Callback", 0);
	ALIAS_RegisterAlias(WineProc, Win16Proc, Win32Proc);
	initialized=1;
    }
}


/*****************************************************************
 *               CreateCaret          (USER.163)
 */

BOOL CreateCaret(HWND hwnd, HBITMAP bitmap, INT width, INT height)
{
    dprintf_caret(stddeb,"CreateCaret: hwnd="NPFMT"\n", hwnd);

    if (!hwnd) return FALSE;

    /* if cursor already exists, destroy it */
    if (Caret.hwnd) DestroyCaret();

    if (bitmap && bitmap != (HBITMAP)1) Caret.bitmap = bitmap;

    if (width)
        Caret.width = width;
    else
	Caret.width = GetSystemMetrics(SM_CXBORDER);

    if (height)
	Caret.height = height;
    else
	Caret.height = GetSystemMetrics(SM_CYBORDER);

    Caret.hwnd = hwnd;
    Caret.hidden = 1;
    Caret.on = FALSE;
    Caret.x = 0;
    Caret.y = 0;
    if (bitmap == (HBITMAP)1)
	Caret.color = GetSysColor(COLOR_GRAYTEXT);
    else
	Caret.color = GetSysColor(COLOR_WINDOW);
    Caret.timeout = 750;

    CARET_Initialize();

    return TRUE;
}
   

/*****************************************************************
 *               DestroyCaret         (USER.164)
 */

BOOL DestroyCaret()
{
    if (!Caret.hwnd) return FALSE;

    dprintf_caret(stddeb,"DestroyCaret: hwnd="NPFMT", timerid=%d\n",
		Caret.hwnd, Caret.timerid);

    CARET_KillTimer();
    CARET_DisplayCaret(CARET_OFF);

    Caret.hwnd = 0;
    return TRUE;
}


/*****************************************************************
 *               SetCaretPos          (USER.165)
 */

void SetCaretPos(short x, short y)
{
    if (!Caret.hwnd) return;

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
}

/*****************************************************************
 *               HideCaret            (USER.166)
 */

void HideCaret(HWND hwnd)
{
    if (!Caret.hwnd) return;
    if (hwnd && (Caret.hwnd != hwnd)) return;

    dprintf_caret(stddeb,"HideCaret: hwnd="NPFMT", hidden=%d\n",
		hwnd, Caret.hidden);

    CARET_KillTimer();
    CARET_DisplayCaret(CARET_OFF);
    Caret.hidden++;
}


/*****************************************************************
 *               ShowCaret            (USER.167)
 */

void ShowCaret(HWND hwnd)
{
    if (!Caret.hwnd) return;
    if (hwnd && (Caret.hwnd != hwnd)) return;

    dprintf_caret(stddeb,"ShowCaret: hwnd="NPFMT", hidden=%d\n",
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
}


/*****************************************************************
 *               SetCaretBlinkTime    (USER.168)
 */

void SetCaretBlinkTime(WORD msecs)
{
    if (!Caret.hwnd) return;

    dprintf_caret(stddeb,"SetCaretBlinkTime: hwnd="NPFMT", msecs=%d\n",
		Caret.hwnd, msecs);

    Caret.timeout = msecs;
    CARET_ResetTimer();
}


/*****************************************************************
 *               GetCaretBlinkTime    (USER.169)
 */

WORD GetCaretBlinkTime()
{
    if (!Caret.hwnd) return 0;

    dprintf_caret(stddeb,"GetCaretBlinkTime: hwnd="NPFMT", msecs=%d\n",
		Caret.hwnd, Caret.timeout);

    return Caret.timeout;
}


/*****************************************************************
 *               GetCaretPos          (USER.183)
 */

void GetCaretPos(LPPOINT pt)
{
    if (!Caret.hwnd || !pt) return;

    dprintf_caret(stddeb,"GetCaretPos: hwnd="NPFMT", pt=%p, x=%d, y=%d\n",
		Caret.hwnd, pt, Caret.x, Caret.y);

    pt->x = Caret.x;
    pt->y = Caret.y;
}
