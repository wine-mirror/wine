/* File: button.c -- MS-Windows(tm) compatible "Button" replacement widget
 *
 *                   programmed by Johannes Ruscheinski for the Linux WABI
 *                   project.
 *		     (C) 1993 by Johannes Ruscheinski
 *
 *                   Modifications by David Metcalfe
 */

#include <windows.h>
#include "win.h"

LONG ButtonWndProc(HWND hWnd, WORD uMsg, WORD wParam, LONG lParam);

static COLORREF color_windowframe, color_btnface, color_btnshadow,
		color_btntext, color_btnhighlight;

static BOOL pressed;

#define NOTIFY_PARENT(hWndCntrl, wNotifyCode) \
	SendMessage(GetParent(hWndCntrl), WM_COMMAND, \
		    GetDlgCtrlID(hWndCntrl), MAKELPARAM(hWndCntrl, wNotifyCode));
#define DIM(array)	((sizeof array)/(sizeof array[0]))

static LONG PB_Paint(HWND hWnd);
static LONG PB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam);
static LONG PB_LButtonUp(HWND hWnd, WORD wParam, LONG lParam);
static LONG PB_LButtonDblClk(HWND hWnd, WORD wParam, LONG lParam);
static void DrawRaisedPushButton(HDC hDC, HWND hButton, RECT rc);
static void DrawPressedPushButton(HDC hDC, HWND hButton, RECT rc);

typedef struct
{
    LONG (*paintfn)();
    LONG (*lButtonDownfn)();
    LONG (*lButtonUpfn)();
    LONG (*lButtonDblClkfn)();
} BTNFN;

#define MAX_BTN_TYPE  2

static BTNFN btnfn[MAX_BTN_TYPE] =
{
    { 
	(LONG(*)())PB_Paint,                       /* BS_PUSHBUTTON */
	(LONG(*)())PB_LButtonDown,
	(LONG(*)())PB_LButtonUp,
	(LONG(*)())PB_LButtonDblClk
    },
    { 
	(LONG(*)())PB_Paint,                       /* BS_DEFPUSHBUTTON */
	(LONG(*)())PB_LButtonDown,
	(LONG(*)())PB_LButtonUp,
	(LONG(*)())PB_LButtonDblClk
    }
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
		    /* initialise colours used for button rendering: */
		    color_windowframe  = GetSysColor(COLOR_WINDOWFRAME);
		    color_btnface      = GetSysColor(COLOR_BTNFACE);
		    color_btnshadow    = GetSysColor(COLOR_BTNSHADOW);
		    color_btntext      = GetSysColor(COLOR_BTNTEXT);
		    color_btnhighlight = GetSysColor(COLOR_BTNHIGHLIGHT);

		    pressed = FALSE;
		    lResult = 0L;
		}
		break;

	case WM_PAINT:
		(btnfn[style].paintfn)(hWnd);
		break;

	case WM_LBUTTONDOWN:
		(btnfn[style].lButtonDownfn)(hWnd, wParam, lParam);
		break;

	case WM_LBUTTONUP:
		(btnfn[style].lButtonUpfn)(hWnd, wParam, lParam);
		break;

	case WM_LBUTTONDBLCLK:
		(btnfn[style].lButtonDblClkfn)(hWnd, wParam, lParam);
		break;

	case WM_SETFOCUS:
		break;

	case WM_KILLFOCUS:
		InvalidateRect(hWnd, NULL, FALSE);
		UpdateWindow(hWnd);
		break;

	case WM_SYSCOLORCHANGE:
		color_windowframe  = GetSysColor(COLOR_WINDOWFRAME);
		color_btnface      = GetSysColor(COLOR_BTNFACE);
		color_btnshadow    = GetSysColor(COLOR_BTNSHADOW);
		color_btntext      = GetSysColor(COLOR_BTNTEXT);
		color_btnhighlight = GetSysColor(COLOR_BTNHIGHLIGHT);
		InvalidateRect(hWnd, NULL, TRUE);
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
/*  SetFocus(hWnd);   */
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

static void DrawRaisedPushButton(HDC hDC, HWND hButton, RECT rc)
{
	HPEN hOldPen, hFramePen;
	HBRUSH hOldBrush, hShadowBrush, hHighlightBrush, hBackgrndBrush;
	HRGN rgn1, rgn2, rgn;
	int len;
	static char text[50+1];
	POINT points[6];
	DWORD dwTextSize;
	int delta;
	TEXTMETRIC tm;
	int i;

	hFramePen = CreatePen(PS_SOLID, 1, color_windowframe);
	hBackgrndBrush = CreateSolidBrush(color_btnface);

	hOldPen = (HPEN)SelectObject(hDC, (HANDLE)hFramePen);
	hOldBrush = (HBRUSH)SelectObject(hDC, (HANDLE)hBackgrndBrush);
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
	hHighlightBrush = CreateSolidBrush(color_btnhighlight);
	rgn2 = CreatePolygonRgn(points, DIM(points), ALTERNATE);
	CombineRgn(rgn, rgn1, rgn2, RGN_AND);
	FillRgn(hDC, rgn2, hHighlightBrush);

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
	hShadowBrush = CreateSolidBrush(color_btnshadow);
	rgn2 = CreatePolygonRgn(points, DIM(points), ALTERNATE);
	CombineRgn(rgn, rgn1, rgn2, RGN_AND);
	FillRgn(hDC, rgn2, hShadowBrush);

#if 0
	/* do we have the focus? */
	if (len >= 1 && GetFocus() == hButton) {
		dwTextSize = GetTextExtent(hDC, text, len);
		delta = ((rc.right - rc.left) - LOWORD(dwTextSize) + 1) >> 1;
		rc.left += delta;	rc.right -= delta;
		GetTextMetrics(hDC, &tm);
		delta = ((rc.bottom - rc.top) -
			 tm.tmHeight + tm.tmInternalLeading) >> 1;
		rc.top += delta; 	rc.bottom -= delta;
		DrawFocusRect(hDC, &rc);
	}
#endif

	SelectObject(hDC, (HANDLE)hOldPen);
	SelectObject(hDC, (HANDLE)hOldBrush);
	DeleteObject((HANDLE)hFramePen);
	DeleteObject((HANDLE)hShadowBrush);
	DeleteObject((HANDLE)hBackgrndBrush);
	DeleteObject((HANDLE)rgn1);
#if 0
	DeleteObject((HANDLE)rgn2);
	DeleteObject((HANDLE)rgn);
#endif
}


static void DrawPressedPushButton(HDC hDC, HWND hButton, RECT rc)
{
	HPEN hOldPen, hShadowPen, hFramePen;
	HBRUSH hOldBrush, hBackgrndBrush;
	HRGN rgn1, rgn2, rgn;
	int len;
	static char text[50+1];
	DWORD dwTextSize;
	int delta;
	TEXTMETRIC tm;

	hFramePen = CreatePen(PS_SOLID, 1, color_windowframe);
	hBackgrndBrush = CreateSolidBrush(color_btnface);

	hOldBrush = (HBRUSH)SelectObject(hDC, (HANDLE)hBackgrndBrush);
	hOldPen = (HPEN)SelectObject(hDC, (HANDLE)hFramePen);
	SetBkMode(hDC, TRANSPARENT);

	/* give parent a chance to alter parameters: */
	SendMessage(GetParent(hButton), WM_CTLCOLOR, (WORD)hDC,
		    MAKELPARAM(hButton, CTLCOLOR_BTN));
	Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);

	/* draw button shadow: */
	hShadowPen = CreatePen(PS_SOLID, 1, color_btnshadow);
	SelectObject(hDC, (HANDLE)hShadowPen);
	MoveTo(hDC, rc.left+1, rc.bottom-1);
	LineTo(hDC, rc.left+1, rc.top+1);
	LineTo(hDC, rc.right-1, rc.top+1);

	/* draw button label, if any: */
	len = GetWindowText(hButton, text, sizeof text);
	if (len >= 1) {
		rc.top++;	rc.left++;
		DrawText(hDC, text, len, &rc,
			 DT_SINGLELINE | DT_CENTER| DT_VCENTER);
	}

#if 0
	/* do we have the focus? */
	if (len >= 1 && GetFocus() == hButton) {
		dwTextSize = GetTextExtent(hDC, text, len);
		delta = ((rc.right - rc.left) - LOWORD(dwTextSize)) >> 1;
		rc.left += delta;	rc.right -= delta;
		GetTextMetrics(hDC, &tm);
		delta = ((rc.bottom - rc.top) -
			 tm.tmHeight + tm.tmInternalLeading) >> 1;
		rc.top += delta; 	rc.bottom -= delta;
		DrawFocusRect(hDC, &rc);
	}
#endif

	SelectObject(hDC, (HANDLE)hOldPen);
	SelectObject(hDC, (HANDLE)hOldBrush);
	DeleteObject((HANDLE)hBackgrndBrush);
	DeleteObject(SelectObject(hDC, (HANDLE)hFramePen));
	DeleteObject(SelectObject(hDC, (HANDLE)hShadowPen));
}









