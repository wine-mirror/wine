/*
 * Caret functions
 *
 * Copyright 1993 David Metcalfe
 */

static char Copyright[] = "Copyright  David Metcalfe, 1993";

#include "windows.h"

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

static WORD CARET_Callback(HWND hwnd, WORD msg, WORD timerid, LONG ctime);
static void CARET_HideCaret();


/*****************************************************************
 *               CARET_Callback
 */

static WORD CARET_Callback(HWND hwnd, WORD msg, WORD timerid, LONG ctime)
{
    HDC hdc;
    HBRUSH hBrush;
    HRGN rgn;

#ifdef DEBUG_CARET
    printf("CARET_Callback: id=%d: LockCaret=%d, hidden=%d, on=%d\n",
	   timerid, LockCaret, Caret.hidden, Caret.on);
#endif
    if (!LockCaret && (!Caret.hidden || Caret.on))
    {
	Caret.on = (Caret.on ? FALSE : TRUE);
	hdc = GetDC(Caret.hwnd);
	if (Caret.bitmap == 0 || Caret.bitmap == 1)
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
    if (Caret.bitmap == 0 || Caret.bitmap == 1)
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
 *               CreateCaret          (USER.163)
 */

void CreateCaret(HWND hwnd, HBITMAP bitmap, short width, short height)
{
    if (!hwnd) return;


    /* if cursor already exists, destroy it */
/*    if (Caret.hwnd)
	DestroyCaret();
*/
    if (bitmap && bitmap != 1)
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
    if (bitmap == 1)
	Caret.color = GetSysColor(COLOR_GRAYTEXT);
    else
	Caret.color = GetSysColor(COLOR_WINDOWTEXT);
    Caret.timeout = 750;
    LockCaret = FALSE;

    Caret.timerid = SetSystemTimer(NULL, 0, Caret.timeout, CARET_Callback);

#ifdef DEBUG_CARET
    printf("CreateCaret: hwnd=%d, timerid=%d\n", hwnd, Caret.timerid);
#endif
}
   

/*****************************************************************
 *               DestroyCaret         (USER.164)
 */

void DestroyCaret()
{
/*    if (!Caret.hwnd) return;
*/
#ifdef DEBUG_CARET
    printf("DestroyCaret: timerid=%d\n", Caret.timerid);
#endif

    KillSystemTimer(NULL, Caret.timerid);

    if (Caret.on)
	CARET_HideCaret();

    Caret.hwnd = 0;          /* cursor marked as not existing */
}


/*****************************************************************
 *               SetCaretPos          (USER.165)
 */

void SetCaretPos(short x, short y)
{
    HDC hdc;
    HBRUSH hBrush;
    HRGN rgn;

    if (!Caret.hwnd) return;

#ifdef DEBUG_CARET
    printf("SetCaretPos: x=%d, y=%d\n", x, y);
#endif

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

#ifdef DEBUG_CARET
    printf("ShowCaret: hidden=%d\n", Caret.hidden);
#endif
    if (Caret.hidden)
	--Caret.hidden;
}


/*****************************************************************
 *               SetCaretBlinkTime    (USER.168)
 */

void SetCaretBlinkTime(WORD msecs)
{
    if (!Caret.hwnd) return;

    KillSystemTimer(NULL, Caret.timerid);
    Caret.timeout = msecs;
    Caret.timerid = SetSystemTimer(NULL, 0, Caret.timeout, CARET_Callback);
}


/*****************************************************************
 *               GetCaretBlinkTime    (USER.169)
 */

WORD GetCaretBlinkTime()
{
    if (!Caret.hwnd) return;

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
