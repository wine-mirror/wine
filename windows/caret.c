/*
 * Caret functions
 *
 * Copyright 1993 David Metcalfe
 */

static char Copyright[] = "Copyright  David Metcalfe, 1993";

#include <X11/Intrinsic.h>

#include "windows.h"

extern XtAppContext XT_app_context;

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
    WORD          timeout;
    XtIntervalId  xtid;
} CARET;

static CARET Caret;
static BOOL LockCaret;

static void CARET_Callback(XtPointer data, XtIntervalId *xtid);
static void CARET_HideCaret(CARET *pCaret);


/*****************************************************************
 *               CARET_Callback
 */

static void CARET_Callback(XtPointer data, XtIntervalId *xtid)
{
    CARET *pCaret = (CARET *)data;
    HDC hdc;
    HBRUSH hBrush;
    HRGN rgn;

#ifdef DEBUG_CARET
    printf("CARET_Callback: LockCaret=%d, hidden=%d, on=%d\n",
	   LockCaret, pCaret->hidden, pCaret->on);
#endif
    if (!LockCaret && (!pCaret->hidden || pCaret->on))
    {
	pCaret->on = (pCaret->on ? FALSE : TRUE);
	hdc = GetDC(pCaret->hwnd);
	hBrush = CreateSolidBrush(pCaret->color);
	SelectObject(hdc, (HANDLE)hBrush);
	SetROP2(hdc, R2_XORPEN);
	rgn = CreateRectRgn(pCaret->x, pCaret->y, 
			    pCaret->x + pCaret->width,
			    pCaret->y + pCaret->height);
	FillRgn(hdc, rgn, hBrush);
	DeleteObject((HANDLE)rgn);
	DeleteObject((HANDLE)hBrush);
	ReleaseDC(pCaret->hwnd, hdc);
    }

    pCaret->xtid = XtAppAddTimeOut(XT_app_context, pCaret->timeout,
				   CARET_Callback, pCaret);
}


/*****************************************************************
 *               CARET_HideCaret
 */

static void CARET_HideCaret(CARET *pCaret)
{
    HDC hdc;
    HBRUSH hBrush;
    HRGN rgn;

    pCaret->on = FALSE;
    hdc = GetDC(pCaret->hwnd);
    hBrush = CreateSolidBrush(pCaret->color);
    SelectObject(hdc, (HANDLE)hBrush);
    SetROP2(hdc, R2_XORPEN);
    rgn = CreateRectRgn(pCaret->x, pCaret->y, 
			pCaret->x + pCaret->width,
			pCaret->y + pCaret->height);
    FillRgn(hdc, rgn, hBrush);
    DeleteObject((HANDLE)rgn);
    DeleteObject((HANDLE)hBrush);
    ReleaseDC(pCaret->hwnd, hdc);
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
    if (bitmap)
    {
	printf("CreateCaret: Bitmaps are currently not supported\n");
	return;
    }

    if (width)
	Caret.width = width;
    else
	Caret.width = 3;              /* should be SM_CXBORDER */

    if (height)
	Caret.height = height;
    else
	Caret.height = 3;             /* should be SM_CYBORDER */

    Caret.hwnd = hwnd;
    Caret.hidden = 1;
    Caret.on = FALSE;
    Caret.x = 0;
    Caret.y = 0;
    Caret.color = GetSysColor(COLOR_WINDOWTEXT);
    Caret.timeout = 750;
    LockCaret = FALSE;

    Caret.xtid = XtAppAddTimeOut(XT_app_context, Caret.timeout,
				 CARET_Callback, &Caret);
}
   

/*****************************************************************
 *               DestroyCaret         (USER.164)
 */

void DestroyCaret()
{
/*    if (!Caret.hwnd) return;
*/
    XtRemoveTimeOut(Caret.xtid);

    if (Caret.on)
	CARET_HideCaret(&Caret);

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
	CARET_HideCaret(&Caret);

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
	CARET_HideCaret(&Caret);

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

    Caret.timeout = msecs;
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
