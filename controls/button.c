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

#define NOTIFY_PARENT(hWndCntrl, wNotifyCode) \
	SendMessage(GetParent(hWndCntrl), WM_COMMAND, \
		GetDlgCtrlID(hWndCntrl), MAKELPARAM(hWndCntrl, wNotifyCode));
#define DIM(array)	((sizeof array)/(sizeof array[0]))

extern BOOL GRAPH_DrawBitmap( HDC hdc, HBITMAP hbitmap, int xdest, int ydest,
			      int xsrc, int ysrc, int width, int height,
			      int rop );              /* windows/graphics.c */
extern void DEFWND_SetText( HWND hwnd, LPSTR text );  /* windows/defwnd.c */

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
    LONG (*paintfn)( HWND );
    LONG (*lButtonDownfn)( HWND, WORD, LONG );
    LONG (*lButtonUpfn)( HWND, WORD, LONG );
    LONG (*lButtonDblClkfn)( HWND, WORD, LONG );
    LONG (*killFocusfn)( HWND );
    LONG (*setCheckfn)( HWND, WORD );
    LONG (*getCheckfn)( HWND );
} BTNFN;

#define MAX_BTN_TYPE  12

static BTNFN btnfn[MAX_BTN_TYPE] =
{
    /* BS_PUSHBUTTON */
    { PB_Paint, PB_LButtonDown, PB_LButtonUp, PB_LButtonDblClk, 
      PB_KillFocus, NULL, NULL },
    /* BS_DEFPUSHBUTTON */
    { PB_Paint, PB_LButtonDown, PB_LButtonUp, PB_LButtonDblClk,
      PB_KillFocus, NULL, NULL },
    /* BS_CHECKBOX */
    { CB_Paint, CB_LButtonDown, CB_LButtonUp, CB_LButtonDblClk,
      CB_KillFocus, CB_SetCheck, CB_GetCheck },
    /* BS_AUTOCHECKBOX */
    { CB_Paint, CB_LButtonDown, CB_LButtonUp, CB_LButtonDblClk,
      CB_KillFocus, CB_SetCheck, CB_GetCheck },
    /* BS_RADIOBUTTON */
    { RB_Paint, RB_LButtonDown, RB_LButtonUp, RB_LButtonDblClk,
      RB_KillFocus, RB_SetCheck, RB_GetCheck },
    /* BS_3STATE */
    { CB_Paint, CB_LButtonDown, CB_LButtonUp, CB_LButtonDblClk,
      CB_KillFocus, CB_SetCheck, CB_GetCheck },
    /* BS_AUTO3STATE */
    { CB_Paint, CB_LButtonDown, CB_LButtonUp, CB_LButtonDblClk,
      CB_KillFocus, CB_SetCheck, CB_GetCheck },
    /* BS_GROUPBOX */
    { GB_Paint, NULL, NULL, NULL, NULL, NULL, NULL },
    /* BS_USERBUTTON */
    { UB_Paint, UB_LButtonDown, UB_LButtonUp, NULL, UB_KillFocus, NULL, NULL },
    /* BS_AUTORADIOBUTTON */
    { RB_Paint, RB_LButtonDown, RB_LButtonUp, RB_LButtonDblClk,
      RB_KillFocus, RB_SetCheck, RB_GetCheck },
    /* Not defined */
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    /* BS_OWNERDRAW */
    { OB_Paint, OB_LButtonDown, OB_LButtonUp, NULL, OB_KillFocus, NULL, NULL }
};

static HBITMAP hbitmapCheckBoxes = 0;
static WORD checkBoxWidth = 0, checkBoxHeight = 0;


LONG ButtonWndProc(HWND hWnd, WORD uMsg, WORD wParam, LONG lParam)
{
	LONG lResult = 0;
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
		if (!hbitmapCheckBoxes)
		{
		    BITMAP bmp;
		    hbitmapCheckBoxes = LoadBitmap( 0, MAKEINTRESOURCE(OBM_CHECKBOXES) );
		    GetObject( hbitmapCheckBoxes, sizeof(bmp), (LPSTR)&bmp );
		    checkBoxWidth  = bmp.bmWidth / 4;
		    checkBoxHeight = bmp.bmHeight / 3;
		}
		
		if (style < 0L || style >= (LONG)DIM(btnfn))
		    lResult = -1L;
		else
		{
		    (WORD)(*(wndPtr->wExtra)) = 0;
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

        case WM_SETTEXT:
		DEFWND_SetText( hWnd, (LPSTR)lParam );
		InvalidateRect( hWnd, NULL, FALSE );
		UpdateWindow( hWnd );
		return 0;

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
    if (GetCapture() == hWnd)
	DrawPressedPushButton(hDC, hWnd, rc);
    else
	DrawRaisedPushButton(hDC, hWnd, rc);
    EndPaint(hWnd, &ps);
    return 0;
}

static LONG PB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam)
{
    SetFocus(hWnd);
    SetCapture(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
    return 0;
}

static LONG PB_LButtonUp(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;

    ReleaseCapture();
    GetClientRect(hWnd, &rc);
    if (PtInRect(&rc, MAKEPOINT(lParam)))
	NOTIFY_PARENT(hWnd, BN_CLICKED);
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
    return 0;
}

static LONG PB_LButtonDblClk(HWND hWnd, WORD wParam, LONG lParam)
{
    NOTIFY_PARENT(hWnd, BN_DOUBLECLICKED);
    return 0;
}

static LONG PB_KillFocus(HWND hWnd)
{
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
    return 0;
}

static void DrawRaisedPushButton(HDC hDC, HWND hButton, RECT rc)
{
	HPEN hOldPen;
	HBRUSH hOldBrush;
	HRGN rgn;
	int len;
	char *text;
	POINT points[6];
	DWORD dwTextSize;
	int delta;
	TEXTMETRIC tm;
	WND *wndPtr = WIN_FindWndPtr( hButton );

	hOldPen = (HPEN)SelectObject(hDC, sysColorObjects.hpenWindowFrame);
	hOldBrush = (HBRUSH)SelectObject(hDC, sysColorObjects.hbrushBtnFace);
	SetBkMode(hDC, TRANSPARENT);
	SetTextColor( hDC, GetSysColor(COLOR_BTNTEXT) );
	Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);

	/* draw button label, if any: */
	text = USER_HEAP_ADDR( wndPtr->hText );
	len = strlen(text);
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
	rgn = CreatePolygonRgn(points, DIM(points), ALTERNATE);
	FillRgn(hDC, rgn, sysColorObjects.hbrushBtnHighlight);

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
	rgn = CreatePolygonRgn(points, DIM(points), ALTERNATE);
	FillRgn(hDC, rgn, sysColorObjects.hbrushBtnShadow);

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
	DeleteObject((HANDLE)rgn);
}


static void DrawPressedPushButton(HDC hDC, HWND hButton, RECT rc)
{
	HPEN hOldPen;
	HBRUSH hOldBrush;
	int len;
	char *text;
	DWORD dwTextSize;
	int delta;
	TEXTMETRIC tm;
	WND *wndPtr = WIN_FindWndPtr( hButton );

	hOldBrush = (HBRUSH)SelectObject(hDC, sysColorObjects.hbrushBtnFace);
	hOldPen = (HPEN)SelectObject(hDC, sysColorObjects.hpenWindowFrame);
	SetBkMode(hDC, TRANSPARENT);
	SetTextColor( hDC, GetSysColor(COLOR_BTNTEXT) );
	Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);

	/* draw button shadow: */
	SelectObject(hDC, sysColorObjects.hbrushBtnShadow );
	PatBlt(hDC, rc.left+1, rc.top+1, 1, rc.bottom-rc.top-2, PATCOPY );
	PatBlt(hDC, rc.left+1, rc.top+1, rc.right-rc.left-2, 1, PATCOPY );

	/* draw button label, if any: */
	text = USER_HEAP_ADDR( wndPtr->hText );
	len = strlen(text);
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
    RECT rc;
    HDC hDC;
    HBRUSH hBrush;
    int textlen, delta;
    char *text;
    TEXTMETRIC tm;
    SIZE size;
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    hDC = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rc);

    hBrush = SendMessage(GetParent(hWnd), WM_CTLCOLOR, (WORD)hDC,
			 MAKELPARAM(hWnd, CTLCOLOR_BTN));
    FillRect(hDC, &rc, hBrush);

    text = USER_HEAP_ADDR( wndPtr->hText );
    textlen = strlen( text );
    GetTextMetrics(hDC, &tm);
    delta = (rc.bottom - rc.top - tm.tmHeight) >> 1;

    GRAPH_DrawBitmap( hDC, hbitmapCheckBoxes,
		      rc.left, rc.top + delta,
		      ((GetCapture() == hWnd) ?  2*checkBoxWidth : 0) +
		      (wndPtr->wExtra[0] ? checkBoxWidth : 0),
		      ((wndPtr->wExtra[0] == 2) ? 2*checkBoxHeight : 0),
		      checkBoxWidth, checkBoxHeight, SRCCOPY );

    rc.left = checkBoxWidth + tm.tmAveCharWidth / 2;
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

    EndPaint(hWnd, &ps);
    return 0;
}

static LONG CB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;

    GetClientRect(hWnd, &rc);
    SetFocus(hWnd);
    SetCapture(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
    return 0;
}

static LONG CB_LButtonUp(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    LONG style;

    ReleaseCapture();
    GetClientRect(hWnd, &rc);

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
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
    return 0;
}

static LONG CB_LButtonDblClk(HWND hWnd, WORD wParam, LONG lParam)
{
    NOTIFY_PARENT(hWnd, BN_DOUBLECLICKED);
    return 0;
}

static LONG CB_KillFocus(HWND hWnd)
{
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
    return 0;
}

static LONG CB_SetCheck(HWND hWnd, WORD wParam)
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    if ((WORD)(*(wndPtr->wExtra)) != wParam)
    {
	RECT rect;
	GetClientRect( hWnd, &rect );
	rect.right = rect.left + checkBoxWidth; /* Only invalidate check-box */
	(WORD)(*(wndPtr->wExtra)) = wParam;
	InvalidateRect(hWnd, &rect, FALSE);
	UpdateWindow(hWnd);
    }
    return 0;
}

static LONG CB_GetCheck(HWND hWnd)
{
    WORD wResult;
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    wResult = (WORD)(*(wndPtr->wExtra));
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
    HBRUSH hBrush;
    int textlen, delta;
    char *text;
    TEXTMETRIC tm;
    SIZE size;
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    hDC = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rc);

    hBrush = SendMessage(GetParent(hWnd), WM_CTLCOLOR, (WORD)hDC,
			 MAKELPARAM(hWnd, CTLCOLOR_BTN));
    FillRect(hDC, &rc, hBrush);

    GetTextMetrics(hDC, &tm);
    delta = (rc.bottom - rc.top - tm.tmHeight) >> 1;
    GRAPH_DrawBitmap( hDC, hbitmapCheckBoxes,
		      rc.left, rc.top + delta,
		      ((GetCapture() == hWnd) ?  2*checkBoxWidth : 0) +
		      (wndPtr->wExtra[0] ? checkBoxWidth : 0), checkBoxHeight,
		      checkBoxWidth, checkBoxHeight, SRCCOPY );

    text = USER_HEAP_ADDR( wndPtr->hText );
    textlen = strlen( text );
    rc.left = checkBoxWidth + tm.tmAveCharWidth / 2;
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

    EndPaint(hWnd, &ps);
    return 0;
}

static LONG RB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;

    GetClientRect(hWnd, &rc);
    if (GetFocus() != hWnd) SetFocus(hWnd);
    else rc.right = rc.left + checkBoxWidth;
    SetCapture(hWnd);
    InvalidateRect(hWnd, &rc, FALSE);
    UpdateWindow(hWnd);
    return 0;
}

static LONG RB_LButtonUp(HWND hWnd, WORD wParam, LONG lParam)
{
    RECT rc;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    LONG style;

    ReleaseCapture();
    GetClientRect(hWnd, &rc);

    if (PtInRect(&rc, MAKEPOINT(lParam)))
    {
	style = wndPtr->dwStyle & 0x0000000F;
	if (style == BS_AUTORADIOBUTTON)
	    (WORD)(*(wndPtr->wExtra)) = 1;
	NOTIFY_PARENT(hWnd, BN_CLICKED);
    }
    rc.right = rc.left + checkBoxWidth;
    InvalidateRect(hWnd, &rc, FALSE);
    UpdateWindow(hWnd);
    return 0;
}


static LONG RB_LButtonDblClk(HWND hWnd, WORD wParam, LONG lParam)
{
    NOTIFY_PARENT(hWnd, BN_DOUBLECLICKED);
    return 0;
}

static LONG RB_KillFocus(HWND hWnd)
{
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
    return 0;
}

static LONG RB_SetCheck(HWND hWnd, WORD wParam)
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    if ((WORD)(*(wndPtr->wExtra)) != wParam)
    {
	RECT rc;
	GetClientRect( hWnd, &rc );
	rc.right = rc.left + checkBoxWidth;
	(WORD)(*(wndPtr->wExtra)) = wParam;
	InvalidateRect(hWnd, &rc, FALSE);
	UpdateWindow(hWnd);
    }
    return 0;
}

static LONG RB_GetCheck(HWND hWnd)
{
    WORD wResult;
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    wResult = (WORD)(*(wndPtr->wExtra));
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
    HBRUSH hBrush;
    char *text;
    SIZE size;
    WND *wndPtr = WIN_FindWndPtr( hWnd );

    hDC = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &rc);

    SelectObject( hDC, sysColorObjects.hpenWindowFrame );
    hBrush = SendMessage(GetParent(hWnd), WM_CTLCOLOR, (WORD)hDC,
			 MAKELPARAM(hWnd, CTLCOLOR_BTN));
    FillRect(hDC, &rc, hBrush);

    text = USER_HEAP_ADDR( wndPtr->hText );
    GetTextExtentPoint(hDC, text, strlen(text), &size);

    MoveTo( hDC, 8, 5 );
    LineTo( hDC, rc.left, 5 );
    LineTo( hDC, rc.left, rc.bottom-1 );
    LineTo( hDC, rc.right-1, rc.bottom-1 );
    LineTo( hDC, rc.right-1, 5 );
    LineTo( hDC, rc.left + size.cx + 12, 5 );

    rc.left = 10;
    rc.top = 0;
    rc.right = rc.left + size.cx + 1;
    rc.bottom = size.cy;
    SetTextColor( hDC, GetSysColor(COLOR_WINDOWTEXT) );
    DrawText(hDC, text, -1, &rc, DT_SINGLELINE );

    EndPaint(hWnd, &ps);
    return 0;
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
    return 0;
}

static LONG UB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam)
{
    SetFocus(hWnd);
    SetCapture(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
    return 0;
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
    return 0;
}

static LONG UB_KillFocus(HWND hWnd)
{
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
    return 0;
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
    return 0;
}

static LONG OB_LButtonDown(HWND hWnd, WORD wParam, LONG lParam)
{
    HDC 	hDC;
    HANDLE	hDis;
    LPDRAWITEMSTRUCT lpdis;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    SetFocus(hWnd);
    SetCapture(hWnd);
    hDC = GetDC(hWnd);
    NOTIFY_PARENT(hWnd, BN_CLICKED);
    hDis = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(DRAWITEMSTRUCT));
    lpdis = (LPDRAWITEMSTRUCT)USER_HEAP_ADDR(hDis);
    lpdis->hDC = hDC;
    lpdis->itemID = 0;
    GetClientRect( hWnd, &lpdis->rcItem );
    lpdis->CtlID = wndPtr->wIDmenu;
    lpdis->CtlType = ODT_BUTTON;
    lpdis->itemAction = ODA_SELECT;
    SendMessage(GetParent(hWnd), WM_DRAWITEM, 1, (LPARAM)lpdis); 
    USER_HEAP_FREE(hDis);
    ReleaseDC(hWnd, hDC);
    return 0;
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
    return 0;
}

static LONG OB_KillFocus(HWND hWnd)
{
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);
    return 0;
}

