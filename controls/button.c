/* File: button.c -- Button type widgets
 *
 * Copyright (C) 1993 Johannes Ruscheinski
 * Copyright (C) 1993 David Metcalfe
 */

static char Copyright1[] = "Copyright Johannes Ruscheinski, 1993";
static char Copyright2[] = "Copyright David Metcalfe, 1993";

#include <windows.h>
#include "win.h"
#include "user.h"
#include "syscolor.h"

LONG ButtonWndProc(HWND hWnd, WORD uMsg, WORD wParam, LONG lParam);

static BOOL pressed;

#define NOTIFY_PARENT(hWndCntrl, wNotifyCode) \
	SendMessage(GetParent(hWndCntrl), WM_COMMAND, \
		GetDlgCtrlID(hWndCntrl), MAKELPARAM(hWndCntrl, wNotifyCode));
#define DIM(array)	((sizeof array)/(sizeof array[0]))

static LONG PB_Paint(HWND hWnd);
static LONG PB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam);
static LONG PB_LButtonUp(HWND hWnd, WORD wParam, LONG lParam);
static LONG PB_LButtonDblClk(HWND hWnd, WORD wParam, LONG lParam);
static LONG PB_KillFocus(HWND hwnd);
static void DrawRaisedPushButton(HDC hDC, HWND hButton, RECT rc);
static void DrawPressedPushButton(HDC hDC, HWND hButton, RECT rc);
static LONG CB_Paint(HWND hWnd);
static LONG CB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam);
static LONG CB_LButtonUp(HWND hWnd, WORD wParam, LONG lParam);
static LONG CB_LButtonDblClk(HWND hWnd, WORD wParam, LONG lParam);
static LONG CB_KillFocus(HWND hWnd);
static LONG CB_SetCheck(HWND hWnd, WORD wParam);
static LONG CB_GetCheck(HWND hWnd);
static LONG RB_Paint(HWND hWnd);
static LONG RB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam);
static LONG RB_LButtonUp(HWND hWnd, WORD wParam, LONG lParam);
static LONG RB_LButtonDblClk(HWND hWnd, WORD wParam, LONG lParam);
static LONG RB_KillFocus(HWND hWnd);
static LONG RB_SetCheck(HWND hWnd, WORD wParam);
static LONG RB_GetCheck(HWND hWnd);
static LONG GB_Paint(HWND hWnd);
static LONG UB_Paint(HWND hWnd);
static LONG UB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam);
static LONG UB_LButtonUp(HWND hWnd, WORD wParam, LONG lParam);
static LONG UB_KillFocus(HWND hWnd);
static LONG OB_Paint(HWND hWnd);
static LONG OB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam);
static LONG OB_LButtonUp(HWND hWnd, WORD wParam, LONG lParam);
static LONG OB_KillFocus(HWND hWnd);

typedef struct
{
    LONG (*paintfn)();
    LONG (*lButtonDownfn)();
    LONG (*lButtonUpfn)();
    LONG (*lButtonDblClkfn)();
    LONG (*killFocusfn)();
    LONG (*setCheckfn)();
    LONG (*getCheckfn)();
} BTNFN;

#define MAX_BTN_TYPE  12

static BTNFN btnfn[MAX_BTN_TYPE] =
{
    { 
	(LONG(*)())PB_Paint,                       /* BS_PUSHBUTTON */
	(LONG(*)())PB_LButtonDown,
	(LONG(*)())PB_LButtonUp,
	(LONG(*)())PB_LButtonDblClk,
	(LONG(*)())PB_KillFocus,
	(LONG(*)())NULL,
	(LONG(*)())NULL
    },
    { 
	(LONG(*)())PB_Paint,                       /* BS_DEFPUSHBUTTON */
	(LONG(*)())PB_LButtonDown,
	(LONG(*)())PB_LButtonUp,
	(LONG(*)())PB_LButtonDblClk,
	(LONG(*)())PB_KillFocus,
	(LONG(*)())NULL,
	(LONG(*)())NULL
    },
    {
	(LONG(*)())CB_Paint,                       /* BS_CHECKBOX */
	(LONG(*)())CB_LButtonDown,
	(LONG(*)())CB_LButtonUp,
	(LONG(*)())CB_LButtonDblClk,
	(LONG(*)())CB_KillFocus,
	(LONG(*)())CB_SetCheck,
	(LONG(*)())CB_GetCheck
    },
    {
	(LONG(*)())CB_Paint,                       /* BS_AUTOCHECKBOX */
	(LONG(*)())CB_LButtonDown,
	(LONG(*)())CB_LButtonUp,
	(LONG(*)())CB_LButtonDblClk,
	(LONG(*)())CB_KillFocus,
	(LONG(*)())CB_SetCheck,
	(LONG(*)())CB_GetCheck
    },
    {
	(LONG(*)())RB_Paint,                       /* BS_RADIOBUTTON */
	(LONG(*)())RB_LButtonDown,
	(LONG(*)())RB_LButtonUp,
	(LONG(*)())RB_LButtonDblClk,
	(LONG(*)())RB_KillFocus,
	(LONG(*)())RB_SetCheck,
	(LONG(*)())RB_GetCheck
    },
    {
	(LONG(*)())CB_Paint,                       /* BS_3STATE */
	(LONG(*)())CB_LButtonDown,
	(LONG(*)())CB_LButtonUp,
	(LONG(*)())CB_LButtonDblClk,
	(LONG(*)())CB_KillFocus,
	(LONG(*)())CB_SetCheck,
	(LONG(*)())CB_GetCheck
    },
    {
	(LONG(*)())CB_Paint,                       /* BS_AUTO3STATE */
	(LONG(*)())CB_LButtonDown,
	(LONG(*)())CB_LButtonUp,
	(LONG(*)())CB_LButtonDblClk,
	(LONG(*)())CB_KillFocus,
	(LONG(*)())CB_SetCheck,
	(LONG(*)())CB_GetCheck
    },
    {
	(LONG(*)())GB_Paint,                       /* BS_GROUPBOX */
	(LONG(*)())NULL,
	(LONG(*)())NULL,
	(LONG(*)())NULL,
	(LONG(*)())NULL,
	(LONG(*)())NULL,
	(LONG(*)())NULL
    },
    {
	(LONG(*)())UB_Paint,                       /* BS_USERBUTTON */
	(LONG(*)())UB_LButtonDown,
	(LONG(*)())UB_LButtonUp,
	(LONG(*)())NULL,
	(LONG(*)())UB_KillFocus,
	(LONG(*)())NULL,
	(LONG(*)())NULL
    },
    {
	(LONG(*)())RB_Paint,                       /* BS_AUTORADIOBUTTON */
	(LONG(*)())RB_LButtonDown,
	(LONG(*)())RB_LButtonUp,
	(LONG(*)())RB_LButtonDblClk,
	(LONG(*)())RB_KillFocus,
	(LONG(*)())RB_SetCheck,
	(LONG(*)())RB_GetCheck
    },
    {
	(LONG(*)())NULL,                           /* Not defined */
	(LONG(*)())NULL,
	(LONG(*)())NULL,
	(LONG(*)())NULL,
	(LONG(*)())NULL,
	(LONG(*)())NULL,
	(LONG(*)())NULL
    },
    { 
	(LONG(*)())OB_Paint,                       /* BS_OWNERDRAW */
	(LONG(*)())OB_LButtonDown,
	(LONG(*)())OB_LButtonUp,
	(LONG(*)())NULL,
	(LONG(*)())OB_KillFocus,
	(LONG(*)())NULL,
	(LONG(*)())NULL
    },
};


LONG ButtonWndProc(HWND hWnd, WORD uMsg, WORD wParam, LONG lParam)
{
	LONG lResult = 0;
	HDC hDC;
	RECT rc;

	WND *wndPtr = WIN_FindWndPtr(hWnd);
	LONG style = wndPtr->dwStyle & 0x0000000F;

	switch (uMsg) {
/*	case WM_GETDLGCODE:
		lResult = DLGC_BUTTON;
		break;
*/
	case WM_ENABLE:
		InvalidateRect(hWnd, NULL, FALSE);
		break;

	case WM_CREATE:
		if (style < 0L || style >= (LONG)DIM(btnfn))
		    lResult = -1L;
		else
		{
		    (WORD)(*(wndPtr->wExtra)) = 0;
		    pressed = FALSE;
		    lResult = 0L;
		}
		break;

	case WM_PAINT:
		if (btnfn[style].paintfn)
		    (btnfn[style].paintfn)(hWnd);
		break;

	case WM_LBUTTONDOWN:
		if (btnfn[style].lButtonDownfn)
		    (btnfn[style].lButtonDownfn)(hWnd, wParam, lParam);
		break;

	case WM_LBUTTONUP:
		if (btnfn[style].lButtonUpfn)
		    (btnfn[style].lButtonUpfn)(hWnd, wParam, lParam);
		break;

	case WM_LBUTTONDBLCLK:
		if (btnfn[style].lButtonDblClkfn)
		    (btnfn[style].lButtonDblClkfn)(hWnd, wParam, lParam);
		break;

	case WM_SETFOCUS:
		break;

	case WM_KILLFOCUS:
		if (btnfn[style].killFocusfn)
		    (btnfn[style].killFocusfn)(hWnd);
		break;

	case WM_SYSCOLORCHANGE:
		InvalidateRect(hWnd, NULL, TRUE);
		break;

	case BM_SETCHECK:
		if (btnfn[style].setCheckfn)
		    (btnfn[style].setCheckfn)(hWnd, wParam);
		break;

	case BM_GETCHECK:
		if (btnfn[style].getCheckfn)
		    return (btnfn[style].getCheckfn)(hWnd);
		break;

	default:
		lResult = DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}

	GlobalUnlock(hWnd);
	return lResult;
}


/**********************************************************************
 *       Push Button Functions
 */

static LONG PB_Paint(HWND hWnd)
{
    PAINTSTRUCT ps;
    RECT rc;
    HDC hDC;

    hDC = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rc);
    if (pressed)
	DrawPressedPushButton(hDC, hWnd, rc);
    else
	DrawRaisedPushButton(hDC, hWnd, rc);
    EndPaint(hWnd, &ps);
}

static LONG PB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam)
{
    SetFocus(hWnd);
    SetCapture(hWnd);
    pressed = TRUE;
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
}

static LONG PB_LButtonUp(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;

    pressed = FALSE;
    ReleaseCapture();
    GetClientRect(hWnd, &rc);
    if (PtInRect(&rc, MAKEPOINT(lParam)))
	NOTIFY_PARENT(hWnd, BN_CLICKED);
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
}

static LONG PB_LButtonDblClk(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;

    GetClientRect(hWnd, &rc);
    if (PtInRect(&rc, MAKEPOINT(lParam)))
	NOTIFY_PARENT(hWnd, BN_DOUBLECLICKED);
}

static LONG PB_KillFocus(HWND hWnd)
{
    pressed = FALSE;
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
}

static void DrawRaisedPushButton(HDC hDC, HWND hButton, RECT rc)
{
	HPEN hOldPen;
	HBRUSH hOldBrush;
	HRGN rgn1, rgn2, rgn;
	int len;
	static char text[50+1];
	POINT points[6];
	DWORD dwTextSize;
	int delta;
	TEXTMETRIC tm;

	hOldPen = (HPEN)SelectObject(hDC, sysColorObjects.hpenWindowFrame);
	hOldBrush = (HBRUSH)SelectObject(hDC, sysColorObjects.hbrushBtnFace);
	SetBkMode(hDC, TRANSPARENT);

	rgn = CreateRectRgn(0, 0,  0,  0);
	rgn1 = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);

	SendMessage(GetParent(hButton), WM_CTLCOLOR, (WORD)hDC,
		    MAKELPARAM(hButton, CTLCOLOR_BTN));
	Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);

	/* draw button label, if any: */
	len = GetWindowText(hButton, text, sizeof text);
	if (len >= 1) {
		rc.left--;	rc.bottom--;
		DrawText(hDC, text, len, &rc,
			 DT_SINGLELINE | DT_CENTER| DT_VCENTER);
	}

	/* draw button highlight */
	points[0].x = rc.left+2;
	points[0].y = rc.bottom;
	points[1].x = rc.left+4;
	points[1].y = rc.bottom-2;
	points[2].x = rc.left+4;
	points[2].y = rc.top+3;
	points[3].x = rc.right-3;
	points[3].y = rc.top+3;
	points[4].x = rc.right-1;
	points[4].y = rc.top+1;
	points[5].x = rc.left+2;
	points[5].y = rc.top+1;
	rgn2 = CreatePolygonRgn(points, DIM(points), ALTERNATE);
	CombineRgn(rgn, rgn1, rgn2, RGN_AND);
	FillRgn(hDC, rgn2, sysColorObjects.hbrushBtnHighlight);

	/* draw button shadow: */
	points[0].x = rc.left+2;
	points[0].y = rc.bottom;
	points[1].x = rc.left+4;
	points[1].y = rc.bottom-2;
	points[2].x = rc.right-3;
	points[2].y = rc.bottom-2;
	points[3].x = rc.right-3;
	points[3].y = rc.top+3;
	points[4].x = rc.right-1;
	points[4].y = rc.top;
	points[5].x = rc.right-1;
	points[5].y = rc.bottom;
	rgn2 = CreatePolygonRgn(points, DIM(points), ALTERNATE);
	CombineRgn(rgn, rgn1, rgn2, RGN_AND);
	FillRgn(hDC, rgn2, sysColorObjects.hbrushBtnShadow);

	/* do we have the focus? */
	if (len >= 1 && GetFocus() == hButton) {
		dwTextSize = GetTextExtent(hDC, text, len);
		delta = ((rc.right - rc.left) - LOWORD(dwTextSize) - 1) >> 1;
		rc.left += delta;	rc.right -= delta;
		GetTextMetrics(hDC, &tm);
		delta = ((rc.bottom - rc.top) - tm.tmHeight - 1) >> 1;
		rc.top += delta; 	rc.bottom -= delta;
		DrawFocusRect(hDC, &rc);
	}

	SelectObject(hDC, (HANDLE)hOldPen);
	SelectObject(hDC, (HANDLE)hOldBrush);
	DeleteObject((HANDLE)rgn1);
	DeleteObject((HANDLE)rgn2);
	DeleteObject((HANDLE)rgn);
}


static void DrawPressedPushButton(HDC hDC, HWND hButton, RECT rc)
{
	HPEN hOldPen;
	HBRUSH hOldBrush;
	int len;
	static char text[50+1];
	DWORD dwTextSize;
	int delta;
	TEXTMETRIC tm;

	hOldBrush = (HBRUSH)SelectObject(hDC, sysColorObjects.hbrushBtnFace);
	hOldPen = (HPEN)SelectObject(hDC, sysColorObjects.hpenWindowFrame);
	SetBkMode(hDC, TRANSPARENT);

	/* give parent a chance to alter parameters: */
	SendMessage(GetParent(hButton), WM_CTLCOLOR, (WORD)hDC,
		    MAKELPARAM(hButton, CTLCOLOR_BTN));
	Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);

	/* draw button shadow: */
	SelectObject(hDC, sysColorObjects.hbrushBtnShadow );
	PatBlt(hDC, rc.left+1, rc.top+1, 1, rc.bottom-rc.top-2, PATCOPY );
	PatBlt(hDC, rc.left+1, rc.top+1, rc.right-rc.left-2, 1, PATCOPY );

	/* draw button label, if any: */
	len = GetWindowText(hButton, text, sizeof text);
	if (len >= 1) {
		rc.top++;	rc.left++;
		DrawText(hDC, text, len, &rc,
			 DT_SINGLELINE | DT_CENTER| DT_VCENTER);
	}

	/* do we have the focus? */
	if (len >= 1 && GetFocus() == hButton) {
		dwTextSize = GetTextExtent(hDC, text, len);
		delta = ((rc.right - rc.left) - LOWORD(dwTextSize) - 1) >> 1;
		rc.left += delta;	rc.right -= delta;
		GetTextMetrics(hDC, &tm);
		delta = ((rc.bottom - rc.top) - tm.tmHeight - 1) >> 1;
		rc.top += delta; 	rc.bottom -= delta;
		DrawFocusRect(hDC, &rc);
	}

	SelectObject(hDC, (HANDLE)hOldPen);
	SelectObject(hDC, (HANDLE)hOldBrush);
}


/**********************************************************************
 *       Check Box Functions
 */

static LONG CB_Paint(HWND hWnd)
{
    PAINTSTRUCT ps;
    RECT rc, rc3;
    HDC hDC;
    HPEN hOldPen;
    HBRUSH hBrush, hGrayBrush;
    int textlen, delta;
    char *text;
    HANDLE hText;
    TEXTMETRIC tm;
    SIZE size;
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    hDC = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rc);

    hOldPen = (HPEN)SelectObject(hDC, sysColorObjects.hpenWindowText);
    hGrayBrush = (HBRUSH)GetStockObject(LTGRAY_BRUSH);

    hBrush = SendMessage(GetParent(hWnd), WM_CTLCOLOR, (WORD)hDC,
			 MAKELPARAM(hWnd, CTLCOLOR_BTN));
    FillRect(hDC, &rc, hBrush);

    textlen = GetWindowTextLength(hWnd);
    hText = USER_HEAP_ALLOC(0, textlen+1);
    text = USER_HEAP_ADDR(hText);
    GetWindowText(hWnd, text, textlen+1);
    GetTextMetrics(hDC, &tm);

    delta = (rc.bottom - rc.top - tm.tmHeight) >> 1;
    Rectangle(hDC, 0, rc.top + delta, tm.tmHeight, tm.tmHeight + delta);
    if (pressed)
	Rectangle(hDC, 1, rc.top + delta + 1, tm.tmHeight - 1, 
		                              tm.tmHeight + delta - 1);

    switch ((WORD)(*(wndPtr->wExtra)))
    {
    case 1:
	MoveTo(hDC, 0, rc.top + delta);
	LineTo(hDC, tm.tmHeight - 1, tm.tmHeight + delta - 1);
	MoveTo(hDC, 0, tm.tmHeight + delta - 1);
	LineTo(hDC, tm.tmHeight - 1, rc.top + delta);
	break;

    case 2:
	rc3.left = 1;
	rc3.top = rc.top + delta + 1;
	rc3.right = tm.tmHeight - 1;
	rc3.bottom = tm.tmHeight + delta - 1;
	FillRect(hDC, &rc3, hGrayBrush);
	break;
    }

    rc.left = tm.tmHeight + tm.tmAveCharWidth / 2;
    DrawText(hDC, text, textlen, &rc, DT_SINGLELINE | DT_VCENTER);

    /* do we have the focus? */
    if (GetFocus() == hWnd)
    {
	GetTextExtentPoint(hDC, text, textlen, &size);
	rc.top += delta - 1;
	rc.bottom -= delta + 1;
	rc.left--;
	rc.right = rc.left + size.cx + 2;
	DrawFocusRect(hDC, &rc);
    }

    SelectObject(hDC, hOldPen);
    USER_HEAP_FREE(hText);
    GlobalUnlock(hWnd);
    EndPaint(hWnd, &ps);
}

static LONG CB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;
    HDC hDC;
    TEXTMETRIC tm;
    int delta;

    hDC = GetDC(hWnd);
    GetTextMetrics(hDC, &tm);
    ReleaseDC(hWnd, hDC);
    GetClientRect(hWnd, &rc);
    delta = (rc.bottom - rc.top - tm.tmHeight) >> 1;
    rc.top += delta;
    rc.bottom = tm.tmHeight + delta;
    rc.right = tm.tmHeight;
    if (PtInRect(&rc, MAKEPOINT(lParam)))
    {
	SetFocus(hWnd);
	SetCapture(hWnd);
	pressed = TRUE;
	InvalidateRect(hWnd, NULL, FALSE);
	UpdateWindow(hWnd);
    }
}

static LONG CB_LButtonUp(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;
    HDC hDC;
    TEXTMETRIC tm;
    int delta;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    LONG style;

    pressed = FALSE;
    ReleaseCapture();
    hDC = GetDC(hWnd);
    GetTextMetrics(hDC, &tm);
    ReleaseDC(hWnd, hDC);
    GetClientRect(hWnd, &rc);
    delta = (rc.bottom - rc.top - tm.tmHeight) >> 1;
    rc.top += delta;
    rc.bottom = tm.tmHeight + delta;
    rc.right = tm.tmHeight;

    if (PtInRect(&rc, MAKEPOINT(lParam)))
    {
	style = wndPtr->dwStyle & 0x0000000F;
	if (style == BS_AUTOCHECKBOX)
	{
	    switch ((WORD)(*(wndPtr->wExtra)))
	    {
	    case 0:
		(WORD)(*(wndPtr->wExtra)) = 1;
		break;

	    case 1:
		(WORD)(*(wndPtr->wExtra)) = 0;
		break;
	    }
	}
	else if (style == BS_AUTO3STATE)
	{
	    switch ((WORD)(*(wndPtr->wExtra)))
	    {
	    case 0:
		(WORD)(*(wndPtr->wExtra)) = 1;
		break;

	    case 1:
		(WORD)(*(wndPtr->wExtra)) = 2;
		break;

	    case 2:
		(WORD)(*(wndPtr->wExtra)) = 0;
		break;
	    }
	}
	NOTIFY_PARENT(hWnd, BN_CLICKED);
    }
    GlobalUnlock(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
}

static LONG CB_LButtonDblClk(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;
    HDC hDC;
    TEXTMETRIC tm;
    int delta;

    hDC = GetDC(hWnd);
    GetTextMetrics(hDC, &tm);
    ReleaseDC(hWnd, hDC);
    GetClientRect(hWnd, &rc);
    delta = (rc.bottom - rc.top - tm.tmHeight) >> 1;
    rc.top += delta;
    rc.bottom = tm.tmHeight + delta;
    rc.right = tm.tmHeight;
    if (PtInRect(&rc, MAKEPOINT(lParam)))
	NOTIFY_PARENT(hWnd, BN_DOUBLECLICKED);
}

static LONG CB_KillFocus(HWND hWnd)
{
    pressed = FALSE;
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
}

static LONG CB_SetCheck(HWND hWnd, WORD wParam)
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    if ((WORD)(*(wndPtr->wExtra)) != wParam)
    {
	(WORD)(*(wndPtr->wExtra)) = wParam;
	InvalidateRect(hWnd, NULL, FALSE);
	UpdateWindow(hWnd);
    }
    GlobalUnlock(hWnd);
}

static LONG CB_GetCheck(HWND hWnd)
{
    WORD wResult;
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    wResult = (WORD)(*(wndPtr->wExtra));
    GlobalUnlock(hWnd);
    return (LONG)wResult;
}


/**********************************************************************
 *       Radio Button Functions
 */

static LONG RB_Paint(HWND hWnd)
{
    PAINTSTRUCT ps;
    RECT rc;
    HDC hDC;
    HPEN hOldPen;
    HBRUSH hBrush, hOldBrush;
    int textlen, delta;
    char *text;
    HANDLE hText;
    TEXTMETRIC tm;
    SIZE size;
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    hDC = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rc);

    hOldPen = (HPEN)SelectObject(hDC, sysColorObjects.hpenWindowText);

    hBrush = SendMessage(GetParent(hWnd), WM_CTLCOLOR, (WORD)hDC,
			 MAKELPARAM(hWnd, CTLCOLOR_BTN));
    FillRect(hDC, &rc, hBrush);

    textlen = GetWindowTextLength(hWnd);
    hText = USER_HEAP_ALLOC(0, textlen+1);
    text = USER_HEAP_ADDR(hText);
    GetWindowText(hWnd, text, textlen+1);
    GetTextMetrics(hDC, &tm);

    delta = (rc.bottom - rc.top - tm.tmHeight) >> 1;
    Ellipse(hDC, 0, rc.top + delta, tm.tmHeight, tm.tmHeight + delta);
    if (pressed)
	Ellipse(hDC, 1, rc.top + delta + 1, tm.tmHeight - 1, 
		                              tm.tmHeight + delta - 1);

    if ((WORD)(*(wndPtr->wExtra)) == 1)
    {
	hBrush = CreateSolidBrush( GetNearestColor(hDC, GetSysColor(COLOR_WINDOWTEXT)));
	hOldBrush = (HBRUSH)SelectObject(hDC, (HANDLE)hBrush);
	Ellipse(hDC, 3, rc.top + delta + 3, tm.tmHeight - 3,
		                            tm.tmHeight + delta -3); 
	SelectObject(hDC, (HANDLE)hOldBrush);
	DeleteObject((HANDLE)hBrush);
    }

    rc.left = tm.tmHeight + tm.tmAveCharWidth / 2;
    DrawText(hDC, text, textlen, &rc, DT_SINGLELINE | DT_VCENTER);

    /* do we have the focus? */
    if (GetFocus() == hWnd)
    {
	GetTextExtentPoint(hDC, text, textlen, &size);
	rc.top += delta - 1;
	rc.bottom -= delta + 1;
	rc.left--;
	rc.right = rc.left + size.cx + 2;
	DrawFocusRect(hDC, &rc);
    }

    SelectObject(hDC, hOldPen );
    USER_HEAP_FREE(hText);
    GlobalUnlock(hWnd);
    EndPaint(hWnd, &ps);
}

static LONG RB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;
    HDC hDC;
    TEXTMETRIC tm;
    int delta;

    hDC = GetDC(hWnd);
    GetTextMetrics(hDC, &tm);
    ReleaseDC(hWnd, hDC);
    GetClientRect(hWnd, &rc);
    delta = (rc.bottom - rc.top - tm.tmHeight) >> 1;
    rc.top += delta;
    rc.bottom = tm.tmHeight + delta;
    rc.right = tm.tmHeight;
    if (PtInRect(&rc, MAKEPOINT(lParam)))
    {
	SetFocus(hWnd);
	SetCapture(hWnd);
	pressed = TRUE;
	InvalidateRect(hWnd, NULL, FALSE);
	UpdateWindow(hWnd);
    }
}

static LONG RB_LButtonUp(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;
    HDC hDC;
    TEXTMETRIC tm;
    int delta;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    LONG style;

    pressed = FALSE;
    ReleaseCapture();
    hDC = GetDC(hWnd);
    GetTextMetrics(hDC, &tm);
    ReleaseDC(hWnd, hDC);
    GetClientRect(hWnd, &rc);
    delta = (rc.bottom - rc.top - tm.tmHeight) >> 1;
    rc.top += delta;
    rc.bottom = tm.tmHeight + delta;
    rc.right = tm.tmHeight;

    if (PtInRect(&rc, MAKEPOINT(lParam)))
    {
	style = wndPtr->dwStyle & 0x0000000F;
	if (style == BS_AUTORADIOBUTTON)
	    (WORD)(*(wndPtr->wExtra)) = 1;
	NOTIFY_PARENT(hWnd, BN_CLICKED);
    }
    GlobalUnlock(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
}

static LONG RB_LButtonDblClk(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;
    HDC hDC;
    TEXTMETRIC tm;
    int delta;

    hDC = GetDC(hWnd);
    GetTextMetrics(hDC, &tm);
    ReleaseDC(hWnd, hDC);
    GetClientRect(hWnd, &rc);
    delta = (rc.bottom - rc.top - tm.tmHeight) >> 1;
    rc.top += delta;
    rc.bottom = tm.tmHeight + delta;
    rc.right = tm.tmHeight;
    if (PtInRect(&rc, MAKEPOINT(lParam)))
	NOTIFY_PARENT(hWnd, BN_DOUBLECLICKED);
}

static LONG RB_KillFocus(HWND hWnd)
{
    pressed = FALSE;
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
}

static LONG RB_SetCheck(HWND hWnd, WORD wParam)
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    if ((WORD)(*(wndPtr->wExtra)) != wParam)
    {
	(WORD)(*(wndPtr->wExtra)) = wParam;
	InvalidateRect(hWnd, NULL, FALSE);
	UpdateWindow(hWnd);
    }
    GlobalUnlock(hWnd);
}

static LONG RB_GetCheck(HWND hWnd)
{
    WORD wResult;
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    wResult = (WORD)(*(wndPtr->wExtra));
    GlobalUnlock(hWnd);
    return (LONG)wResult;
}


/**********************************************************************
 *       Group Box Functions
 */

static LONG GB_Paint(HWND hWnd)
{
    PAINTSTRUCT ps;
    RECT rc;
    HDC hDC;
    HPEN hOldPen;
    HBRUSH hBrush;
    int textlen;
    char *text;
    HANDLE hText;
    SIZE size;

    hDC = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rc);

    hOldPen = (HPEN)SelectObject(hDC, sysColorObjects.hpenWindowText);

    hBrush = SendMessage(GetParent(hWnd), WM_CTLCOLOR, (WORD)hDC,
			 MAKELPARAM(hWnd, CTLCOLOR_BTN));
    FillRect(hDC, &rc, hBrush);

    textlen = GetWindowTextLength(hWnd);
    hText = USER_HEAP_ALLOC(0, textlen+1);
    text = USER_HEAP_ADDR(hText);
    GetWindowText(hWnd, text, textlen+1);
    GetTextExtentPoint(hDC, text, textlen, &size);

    rc.top = 5;
    Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
    rc.left = 10;
    rc.top = 0;
    rc.right = rc.left + size.cx + 1;
    rc.bottom = size.cy;
    DrawText(hDC, text, textlen, &rc, DT_SINGLELINE);

    SelectObject(hDC, hOldPen );
    USER_HEAP_FREE(hText);
    EndPaint(hWnd, &ps);
}


/**********************************************************************
 *       User Button Functions
 */

static LONG UB_Paint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hDC;
    RECT rc;
    HBRUSH hBrush;

    hDC = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rc);

    hBrush = SendMessage(GetParent(hWnd), WM_CTLCOLOR, (WORD)hDC,
			 MAKELPARAM(hWnd, CTLCOLOR_BTN));
    FillRect(hDC, &rc, hBrush);

    NOTIFY_PARENT(hWnd, BN_PAINT);

    /* do we have the focus? */
    if (GetFocus() == hWnd)
	DrawFocusRect(hDC, &rc);

    EndPaint(hWnd, &ps);
}

static LONG UB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;

    SetFocus(hWnd);
    SetCapture(hWnd);
    GetClientRect(hWnd, &rc);
    if (PtInRect(&rc, MAKEPOINT(lParam)))
	NOTIFY_PARENT(hWnd, BN_HILITE);
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
}

static LONG UB_LButtonUp(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;

    ReleaseCapture();
    GetClientRect(hWnd, &rc);
    if (PtInRect(&rc, MAKEPOINT(lParam)))
    {
	NOTIFY_PARENT(hWnd, BN_CLICKED);
	NOTIFY_PARENT(hWnd, BN_UNHILITE);
    }
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
}

static LONG UB_KillFocus(HWND hWnd)
{
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
}


/**********************************************************************
 *       Ownerdrawn Button Functions
 */

static LONG OB_Paint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC 	hDC;
    RECT 	rc;
    HANDLE	hDis;
    LPDRAWITEMSTRUCT lpdis;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    hDC = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rc);
    hDis = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(DRAWITEMSTRUCT));
    lpdis = (LPDRAWITEMSTRUCT)USER_HEAP_ADDR(hDis);
    lpdis->hDC = hDC;
    lpdis->itemID = 0;
    CopyRect(&lpdis->rcItem, &rc);
    lpdis->CtlID = wndPtr->wIDmenu;
    lpdis->CtlType = ODT_BUTTON;
    lpdis->itemAction = ODA_DRAWENTIRE;
/*    printf("ownerdrawn button WM_DRAWITEM CtrlID=%X\n", lpdis->CtlID);*/
    SendMessage(GetParent(hWnd), WM_DRAWITEM, 1, (LPARAM)lpdis); 
    USER_HEAP_FREE(hDis);
    EndPaint(hWnd, &ps);
}

static LONG OB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam)
{
    HDC 	hDC;
    RECT 	rc;
    HANDLE	hDis;
    LPDRAWITEMSTRUCT lpdis;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    SetFocus(hWnd);
    SetCapture(hWnd);
    hDC = GetDC(hWnd);
    GetClientRect(hWnd, &rc);
    if (PtInRect(&rc, MAKEPOINT(lParam)))
	NOTIFY_PARENT(hWnd, BN_CLICKED);
    GetClientRect(hWnd, &rc);
    hDis = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(DRAWITEMSTRUCT));
    lpdis = (LPDRAWITEMSTRUCT)USER_HEAP_ADDR(hDis);
    lpdis->hDC = hDC;
    lpdis->itemID = 0;
    CopyRect(&lpdis->rcItem, &rc);
    lpdis->CtlID = wndPtr->wIDmenu;
    lpdis->CtlType = ODT_BUTTON;
    lpdis->itemAction = ODA_SELECT;
    SendMessage(GetParent(hWnd), WM_DRAWITEM, 1, (LPARAM)lpdis); 
    USER_HEAP_FREE(hDis);
    ReleaseDC(hWnd, hDC);
}

static LONG OB_LButtonUp(HWND hWnd, WORD wParam, LONG lParam)
{
    HDC 	hDC;
    RECT 	rc;
    HANDLE	hDis;
    LPDRAWITEMSTRUCT lpdis;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    ReleaseCapture();
    hDC = GetDC(hWnd);
    GetClientRect(hWnd, &rc);
    if (PtInRect(&rc, MAKEPOINT(lParam)))
	NOTIFY_PARENT(hWnd, BN_CLICKED);
    GetClientRect(hWnd, &rc);
    hDis = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(DRAWITEMSTRUCT));
    lpdis = (LPDRAWITEMSTRUCT)USER_HEAP_ADDR(hDis);
    lpdis->hDC = hDC;
    lpdis->itemID = 0;
    CopyRect(&lpdis->rcItem, &rc);
    lpdis->CtlID = wndPtr->wIDmenu;
    lpdis->CtlType = ODT_BUTTON;
    lpdis->itemAction = ODA_SELECT;
    SendMessage(GetParent(hWnd), WM_DRAWITEM, 1, (LPARAM)lpdis); 
    USER_HEAP_FREE(hDis);
    ReleaseDC(hWnd, hDC);
}

static LONG OB_KillFocus(HWND hWnd)
{
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
}

