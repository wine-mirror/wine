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

static CARET Caret;
static BOOL LockCaret;

static void CARET_HideCaret();


/*****************************************************************
 *               CARET_Callback
 */
WORD CARET_Callback(HWND hwnd, WORD msg, WORD timerid, LONG ctime)
{
    HDC hdc;
    HBRUSH hBrush;
    HRGN rgn;

    dprintf_caret(stddeb,"CARET_Callback: id=%d: LockCaret=%d, hidden=%d, on=%d\n",
	   timerid, LockCaret, Caret.hidden, Caret.on);
    if (!LockCaret && (!Caret.hidden || Caret.on))
    {
	Caret.on = (Caret.on ? FALSE : TRUE);
	hdc = GetDC(Caret.hwnd);
	if (Caret.bitmap == (HBITMAP)0 || Caret.bitmap == (HBITMAP)1)
	    hBrush = CreateSolidBrush(Caret.color);
	else
	    hBrush = CreatePatternBrush(Caret.bitmap);
	SelectObject(hdc, (HANDLE)hBrush);
	SetROP2(hdc, R2_NOTXORPEN);
	rgn = CreateRectRgn(Caret.x, Caret.y, 
			    Caret.x + Caret.width,
			    Caret.y + Caret.height);
	FillRgn(hdc, rgn, hBrush);
	DeleteObject((HANDLE)rgn);
	DeleteObject((HANDLE)hBrush);
	ReleaseDC(Caret.hwnd, hdc);
    }
    return 0;
}


/*****************************************************************
 *               CARET_HideCaret
 */

static void CARET_HideCaret()
{
    HDC hdc;
    HBRUSH hBrush;
    HRGN rgn;

    Caret.on = FALSE;
    hdc = GetDC(Caret.hwnd);
    if (Caret.bitmap == (HBITMAP)0 || Caret.bitmap == (HBITMAP)1)
	hBrush = CreateSolidBrush(Caret.color);
    else
	hBrush = CreatePatternBrush(Caret.bitmap);
    SelectObject(hdc, (HANDLE)hBrush);
    SetROP2(hdc, R2_XORPEN);
    rgn = CreateRectRgn(Caret.x, Caret.y, 
			Caret.x + Caret.width,
			Caret.y + Caret.height);
    FillRgn(hdc, rgn, hBrush);
    DeleteObject((HANDLE)rgn);
    DeleteObject((HANDLE)hBrush);
    ReleaseDC(Caret.hwnd, hdc);
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
      WineProc=(DWORD)CARET_Callback;
      Win16Proc=(DWORD)GetWndProcEntry16("CARET_Callback");
      Win32Proc=(DWORD)RELAY32_GetEntryPoint("WINPROCS32","CARET_Callback",0);
      ALIAS_RegisterAlias(WineProc,Win16Proc,Win32Proc);
      initialized=1;
    }
}


/*****************************************************************
 *               CreateCaret          (USER.163)
 */

BOOL CreateCaret(HWND hwnd, HBITMAP bitmap, INT width, INT height)
{
    if (!hwnd) return FALSE;


    /* if cursor already exists, destroy it */
/*    if (Caret.hwnd)
	DestroyCaret();
*/
    if (bitmap && bitmap != (HBITMAP)1)
	Caret.bitmap = bitmap;

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
	Caret.color = GetSysColor(COLOR_WINDOWTEXT);
    Caret.timeout = 750;
    LockCaret = FALSE;

    CARET_Initialize();

    Caret.timerid = SetSystemTimer( (HWND)0, 0, Caret.timeout,
                                (FARPROC)GetWndProcEntry16("CARET_Callback"));

    dprintf_caret(stddeb,"CreateCaret: hwnd="NPFMT", timerid=%d\n", 
		  hwnd, Caret.timerid);
    return TRUE;
}
   

/*****************************************************************
 *               DestroyCaret         (USER.164)
 */

void DestroyCaret()
{
/*    if (!Caret.hwnd) return;
*/
    dprintf_caret(stddeb,"DestroyCaret: timerid=%d\n", Caret.timerid);

    KillSystemTimer( (HWND)0, Caret.timerid);

    if (Caret.on)
	CARET_HideCaret();

    Caret.hwnd = 0;          /* cursor marked as not existing */
}


/*****************************************************************
 *               SetCaretPos          (USER.165)
 */

void SetCaretPos(short x, short y)
{
    if (!Caret.hwnd) return;

    dprintf_caret(stddeb,"SetCaretPos: x=%d, y=%d\n", x, y);

    LockCaret = TRUE;
    if (Caret.on)
	CARET_HideCaret();

    Caret.x = x;
    Caret.y = y;
    LockCaret = FALSE;
}

/*****************************************************************
 *               HideCaret            (USER.166)
 */

void HideCaret(HWND hwnd)
{
    if (!Caret.hwnd) return;
    if (hwnd && (Caret.hwnd != hwnd)) return;

    LockCaret = TRUE;
    if (Caret.on)
	CARET_HideCaret();

    ++Caret.hidden;
    LockCaret = FALSE;
}


/*****************************************************************
 *               ShowCaret            (USER.167)
 */

void ShowCaret(HWND hwnd)
{
    if (!Caret.hwnd) return;
    if (hwnd && (Caret.hwnd != hwnd)) return;

    dprintf_caret(stddeb,"ShowCaret: hidden=%d\n", Caret.hidden);
    if (Caret.hidden)
	--Caret.hidden;
}


/*****************************************************************
 *               SetCaretBlinkTime    (USER.168)
 */

void SetCaretBlinkTime(WORD msecs)
{
    if (!Caret.hwnd) return;

    CARET_Initialize();

    KillSystemTimer( (HWND)0, Caret.timerid);
    Caret.timeout = msecs;
    Caret.timerid = SetSystemTimer( (HWND)0, 0, Caret.timeout,
                                 (FARPROC)GetWndProcEntry16("CARET_Callback"));
}


/*****************************************************************
 *               GetCaretBlinkTime    (USER.169)
 */

WORD GetCaretBlinkTime()
{
    if (!Caret.hwnd) return 0;
    return Caret.timeout;
}


/*****************************************************************
 *               GetCaretPos          (USER.183)
 */

void GetCaretPos(LPPOINT pt)
{
    if (!Caret.hwnd || !pt) return;

    pt->x = Caret.x;
    pt->y = Caret.y;
}
