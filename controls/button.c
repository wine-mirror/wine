/* File: button.c -- Button type widgets
 *
 * Copyright (C) 1993 Johannes Ruscheinski
 * Copyright (C) 1993 David Metcalfe
 * Copyright (C) 1994 Alexandre Julliard
 */

#include "win.h"
#include "user.h"
#include "syscolor.h"
#include "graphics.h"
#include "button.h"
#include "stackframe.h"

extern void DEFWND_SetText( HWND hwnd, LPSTR text );  /* windows/defwnd.c */

static void PB_Paint( HWND hWnd, HDC hDC, WORD action );
static void CB_Paint( HWND hWnd, HDC hDC, WORD action );
static void GB_Paint( HWND hWnd, HDC hDC, WORD action );
static void UB_Paint( HWND hWnd, HDC hDC, WORD action );
static void OB_Paint( HWND hWnd, HDC hDC, WORD action );
static void BUTTON_CheckAutoRadioButton(HWND hWnd);


#define MAX_BTN_TYPE  12

static WORD maxCheckState[MAX_BTN_TYPE] =
{
    BUTTON_UNCHECKED,   /* BS_PUSHBUTTON */
    BUTTON_UNCHECKED,   /* BS_DEFPUSHBUTTON */
    BUTTON_CHECKED,     /* BS_CHECKBOX */
    BUTTON_CHECKED,     /* BS_AUTOCHECKBOX */
    BUTTON_CHECKED,     /* BS_RADIOBUTTON */
    BUTTON_3STATE,      /* BS_3STATE */
    BUTTON_3STATE,      /* BS_AUTO3STATE */
    BUTTON_UNCHECKED,   /* BS_GROUPBOX */
    BUTTON_UNCHECKED,   /* BS_USERBUTTON */
    BUTTON_CHECKED,     /* BS_AUTORADIOBUTTON */
    BUTTON_UNCHECKED,   /* Not defined */
    BUTTON_UNCHECKED    /* BS_OWNERDRAW */
};

typedef void (*pfPaint)(HWND,HDC,WORD);

static pfPaint btnPaintFunc[MAX_BTN_TYPE] =
{
    PB_Paint,    /* BS_PUSHBUTTON */
    PB_Paint,    /* BS_DEFPUSHBUTTON */
    CB_Paint,    /* BS_CHECKBOX */
    CB_Paint,    /* BS_AUTOCHECKBOX */
    CB_Paint,    /* BS_RADIOBUTTON */
    CB_Paint,    /* BS_3STATE */
    CB_Paint,    /* BS_AUTO3STATE */
    GB_Paint,    /* BS_GROUPBOX */
    UB_Paint,    /* BS_USERBUTTON */
    CB_Paint,    /* BS_AUTORADIOBUTTON */
    NULL,        /* Not defined */
    OB_Paint     /* BS_OWNERDRAW */
};

#define PAINT_BUTTON(hwnd,style,action) \
     if (btnPaintFunc[style]) { \
         HDC hdc = GetDC( hwnd ); \
         (btnPaintFunc[style])(hwnd,hdc,action); \
         ReleaseDC( hwnd, hdc ); }

static HBITMAP hbitmapCheckBoxes = 0;
static WORD checkBoxWidth = 0, checkBoxHeight = 0;


LONG ButtonWndProc(HWND hWnd, WORD uMsg, WORD wParam, LONG lParam)
{
        RECT rect;
	LONG lResult = 0;
	WND *wndPtr = WIN_FindWndPtr(hWnd);
	LONG style = wndPtr->dwStyle & 0x0000000F;
        BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

	switch (uMsg) {
	case WM_GETDLGCODE:
                switch(style)
                {
                case BS_PUSHBUTTON:
                    return DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON;
                case BS_DEFPUSHBUTTON:
                    return DLGC_BUTTON | DLGC_DEFPUSHBUTTON;
                case BS_RADIOBUTTON:
                case BS_AUTORADIOBUTTON:
                    return DLGC_BUTTON | DLGC_RADIOBUTTON;
                default:
                    return DLGC_BUTTON;
                }

	case WM_ENABLE:
                PAINT_BUTTON( hWnd, style, ODA_DRAWENTIRE );
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
		
		if (style < 0L || style >= MAX_BTN_TYPE)
		    lResult = -1L;
		else
		{
                    infoPtr->state = BUTTON_UNCHECKED;
                    infoPtr->hFont = 0;
		    lResult = 0L;
		}
		break;

        case WM_ERASEBKGND:
                break;

	case WM_PAINT:
                if (btnPaintFunc[style])
                {
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint( hWnd, &ps );
                    (btnPaintFunc[style])( hWnd, hdc, ODA_DRAWENTIRE );
                    ReleaseDC( hWnd, hdc );
                }
		break;

	case WM_LBUTTONDOWN:
                SendMessage( hWnd, BM_SETSTATE, TRUE, 0 );
                SetFocus( hWnd );
                SetCapture( hWnd );
		break;

	case WM_LBUTTONUP:
	        ReleaseCapture();
	        if (!(infoPtr->state & BUTTON_HIGHLIGHTED)) break;
                SendMessage( hWnd, BM_SETSTATE, FALSE, 0 );
                GetClientRect( hWnd, &rect );
                if (PtInRect( &rect, MAKEPOINT(lParam) ))
                {
                    switch(style)
                    {
                    case BS_AUTOCHECKBOX:
                        SendMessage( hWnd, BM_SETCHECK,
                                    !(infoPtr->state & BUTTON_CHECKED), 0 );
                        break;
                    case BS_AUTORADIOBUTTON:
                        SendMessage( hWnd, BM_SETCHECK, TRUE, 0 );
                        break;
                    case BS_AUTO3STATE:
                        SendMessage( hWnd, BM_SETCHECK,
                                     (infoPtr->state & BUTTON_3STATE) ? 0 :
                                     ((infoPtr->state & 3) + 1), 0 );
                        break;
                    }
                    SendMessage( GetParent(hWnd), WM_COMMAND,
                                 wndPtr->wIDmenu, MAKELPARAM(hWnd,BN_CLICKED));
                }
		break;

        case WM_MOUSEMOVE:
                if (GetCapture() == hWnd)
                {
                    GetClientRect( hWnd, &rect );
                    if (PtInRect( &rect, MAKEPOINT(lParam)) )
                       SendMessage( hWnd, BM_SETSTATE, TRUE, 0 );
                    else SendMessage( hWnd, BM_SETSTATE, FALSE, 0 );
                }
                break;

        case WM_NCHITTEST:
                if(style == BS_GROUPBOX) return HTTRANSPARENT;
                lResult = DefWindowProc(hWnd, uMsg, wParam, lParam);
                break;

        case WM_SETTEXT:
		DEFWND_SetText( hWnd, (LPSTR)PTR_SEG_TO_LIN(lParam) );
                PAINT_BUTTON( hWnd, style, ODA_DRAWENTIRE );
		return 0;

        case WM_SETFONT:
                infoPtr->hFont = wParam;
                if (lParam)
                    PAINT_BUTTON( hWnd, style, ODA_DRAWENTIRE );
                break;

        case WM_GETFONT:
                return infoPtr->hFont;

	case WM_SETFOCUS:
                infoPtr->state |= BUTTON_HASFOCUS;
                PAINT_BUTTON( hWnd, style, ODA_FOCUS );
		break;

	case WM_KILLFOCUS:
                infoPtr->state &= ~BUTTON_HASFOCUS;
                PAINT_BUTTON( hWnd, style, ODA_FOCUS );
		break;

	case WM_SYSCOLORCHANGE:
		InvalidateRect(hWnd, NULL, FALSE);
		break;

	case BM_SETSTYLE:
		if ((wParam & 0x0f) >= MAX_BTN_TYPE) break;
		wndPtr->dwStyle = (wndPtr->dwStyle & 0xfffffff0) 
		                   | (wParam & 0x0000000f);
                style = wndPtr->dwStyle & 0x0000000f;
                PAINT_BUTTON( hWnd, style, ODA_DRAWENTIRE );
		break;

	case BM_GETCHECK:
		lResult = infoPtr->state & 3;
		break;

	case BM_SETCHECK:
                if (wParam > maxCheckState[style])
                    wParam = maxCheckState[style];
		if ((infoPtr->state & 3) != wParam)
                {
                    infoPtr->state = (infoPtr->state & ~3) | wParam;
                    PAINT_BUTTON( hWnd, style, ODA_SELECT );
                }
		if(style == BS_AUTORADIOBUTTON && wParam==BUTTON_CHECKED)
			BUTTON_CheckAutoRadioButton(hWnd);
                break;

	case BM_GETSTATE:
		lResult = infoPtr->state;
		break;

	case BM_SETSTATE:
                if (!wParam != !(infoPtr->state & BUTTON_HIGHLIGHTED))
                {
                    if (wParam) infoPtr->state |= BUTTON_HIGHLIGHTED;
                    else infoPtr->state &= ~BUTTON_HIGHLIGHTED;
                    PAINT_BUTTON( hWnd, style, ODA_SELECT );
                }
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

static void PB_Paint( HWND hButton, HDC hDC, WORD action )
{
    RECT rc;
    HPEN hOldPen;
    HBRUSH hOldBrush;
    char *text;
    DWORD dwTextSize;
    int delta;
    TEXTMETRIC tm;
    WND *wndPtr = WIN_FindWndPtr( hButton );
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    GetClientRect(hButton, &rc);

      /* Send WM_CTLCOLOR to allow changing the font (the colors are fixed) */
    if (infoPtr->hFont) SelectObject( hDC, infoPtr->hFont );
    SendMessage( GetParent(hButton), WM_CTLCOLOR, (WORD)hDC,
                 MAKELPARAM(hButton, CTLCOLOR_BTN) );
    hOldPen = (HPEN)SelectObject(hDC, sysColorObjects.hpenWindowFrame);
    hOldBrush = (HBRUSH)SelectObject(hDC, sysColorObjects.hbrushBtnFace);
    SetBkMode(hDC, TRANSPARENT);
    Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
    if (action == ODA_DRAWENTIRE)
    {
        SetPixel( hDC, rc.left, rc.top, GetSysColor(COLOR_WINDOW) );
        SetPixel( hDC, rc.left, rc.bottom-1, GetSysColor(COLOR_WINDOW) );
        SetPixel( hDC, rc.right-1, rc.top, GetSysColor(COLOR_WINDOW) );
        SetPixel( hDC, rc.right-1, rc.bottom-1, GetSysColor(COLOR_WINDOW) );
    }
    InflateRect( &rc, -1, -1 );

    if ((wndPtr->dwStyle & 0x000f) == BS_DEFPUSHBUTTON)
    {
        Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
        InflateRect( &rc, -1, -1 );
    }

    if (infoPtr->state & BUTTON_HIGHLIGHTED)
    {
        /* draw button shadow: */
        SelectObject(hDC, sysColorObjects.hbrushBtnShadow );
        PatBlt(hDC, rc.left, rc.top, 1, rc.bottom-rc.top, PATCOPY );
        PatBlt(hDC, rc.left, rc.top, rc.right-rc.left, 1, PATCOPY );
        rc.left += 2;  /* To position the text down and right */
        rc.top  += 2;
    }
    else GRAPH_DrawReliefRect( hDC, &rc, 2, 2, FALSE );
    
    /* draw button label, if any: */
    text = USER_HEAP_LIN_ADDR( wndPtr->hText );
    if (text[0])
    {
        SetTextColor( hDC, (wndPtr->dwStyle & WS_DISABLED) ?
                     GetSysColor(COLOR_GRAYTEXT) : GetSysColor(COLOR_BTNTEXT));
        DrawText(hDC, text, -1, &rc,
                 DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        /* do we have the focus? */
        if (infoPtr->state & BUTTON_HASFOCUS)
        {
            short xdelta, ydelta;
            dwTextSize = GetTextExtent( hDC, text, strlen(text) );
            GetTextMetrics( hDC, &tm );
            xdelta = ((rc.right - rc.left) - LOWORD(dwTextSize) - 1) / 2;
            ydelta = ((rc.bottom - rc.top) - tm.tmHeight - 1) / 2;
            if (xdelta < 0) xdelta = 0;
            if (ydelta < 0) ydelta = 0;
            InflateRect( &rc, -xdelta, -ydelta );
            DrawFocusRect( hDC, &rc );
        }
    }

    SelectObject(hDC, (HANDLE)hOldPen);
    SelectObject(hDC, (HANDLE)hOldBrush);
}


/**********************************************************************
 *       Check Box & Radio Button Functions
 */

static void CB_Paint( HWND hWnd, HDC hDC, WORD action )
{
    RECT rc;
    HBRUSH hBrush;
    int textlen, delta, x, y;
    char *text;
    TEXTMETRIC tm;
    SIZE size;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    GetClientRect(hWnd, &rc);

    if (infoPtr->hFont) SelectObject( hDC, infoPtr->hFont );
    hBrush = SendMessage(GetParent(hWnd), WM_CTLCOLOR, (WORD)hDC,
			 MAKELPARAM(hWnd, CTLCOLOR_BTN));
    if (action == ODA_DRAWENTIRE) FillRect(hDC, &rc, hBrush);

    GetTextMetrics(hDC, &tm);
    delta = (rc.bottom - rc.top - tm.tmHeight) >> 1;
    text = USER_HEAP_LIN_ADDR( wndPtr->hText );
    textlen = strlen( text );

      /* Draw the check-box bitmap */
    x = y = 0;
    if (infoPtr->state & BUTTON_HIGHLIGHTED) x += 2 * checkBoxWidth;
    if (infoPtr->state & (BUTTON_CHECKED | BUTTON_3STATE)) x += checkBoxWidth;
    if (((wndPtr->dwStyle & 0x0f) == BS_RADIOBUTTON) ||
        ((wndPtr->dwStyle & 0x0f) == BS_AUTORADIOBUTTON)) y += checkBoxHeight;
    else if (infoPtr->state & BUTTON_3STATE) y += 2 * checkBoxHeight;
    GRAPH_DrawBitmap( hDC, hbitmapCheckBoxes, rc.left, rc.top + delta,
                      x, y, checkBoxWidth, checkBoxHeight );
    rc.left += checkBoxWidth + tm.tmAveCharWidth / 2;

    if (action == ODA_DRAWENTIRE)
    {
        if (wndPtr->dwStyle & WS_DISABLED)
            SetTextColor( hDC, GetSysColor(COLOR_GRAYTEXT) );
        DrawText(hDC, text, textlen, &rc, DT_SINGLELINE | DT_VCENTER);
    }
    
    if ((action == ODA_FOCUS) ||
        ((action == ODA_DRAWENTIRE) && (infoPtr->state & BUTTON_HASFOCUS)))
    {
        GetTextExtentPoint(hDC, text, textlen, &size);
        if (delta > 1)
        {
            rc.top += delta - 1;
            rc.bottom -= delta + 1;
        }
        rc.left--;
        rc.right = min( rc.left + size.cx + 2, rc.right );
        DrawFocusRect(hDC, &rc);
    }
}


/**********************************************************************
 *       BUTTON_CheckAutoRadioButton
 *
 * hWnd is checked, uncheck everything else in group
 */
static void BUTTON_CheckAutoRadioButton(HWND hWnd)
{
    HWND parent = GetParent(hWnd);
    HWND sibling;
    for(sibling = GetNextDlgGroupItem(parent,hWnd,FALSE);
        sibling != hWnd;
        sibling = GetNextDlgGroupItem(parent,sibling,FALSE))
	    SendMessage(sibling,BM_SETCHECK,BUTTON_UNCHECKED,0);
}


/**********************************************************************
 *       Group Box Functions
 */

static void GB_Paint( HWND hWnd, HDC hDC, WORD action )
{
    RECT rc;
    char *text;
    SIZE size;
    WND *wndPtr = WIN_FindWndPtr( hWnd );
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    if (action != ODA_DRAWENTIRE) return;

    if (infoPtr->hFont) SelectObject( hDC, infoPtr->hFont );
    SendMessage( GetParent(hWnd), WM_CTLCOLOR, (WORD)hDC,
		 MAKELPARAM(hWnd, CTLCOLOR_BTN));
    SelectObject( hDC, sysColorObjects.hpenWindowFrame );

    GetClientRect(hWnd, &rc);

    MoveTo( hDC, rc.left, rc.top+2 );
    LineTo( hDC, rc.right-1, rc.top+2 );
    LineTo( hDC, rc.right-1, rc.bottom-1 );
    LineTo( hDC, rc.left, rc.bottom-1 );
    LineTo( hDC, rc.left, rc.top+2 );

    text = USER_HEAP_LIN_ADDR( wndPtr->hText );
    GetTextExtentPoint(hDC, text, strlen(text), &size);
    rc.left  += 10;
    rc.right  = rc.left + size.cx + 1;
    rc.bottom = size.cy;
    if (wndPtr->dwStyle & WS_DISABLED)
        SetTextColor( hDC, GetSysColor(COLOR_GRAYTEXT) );
    DrawText(hDC, text, -1, &rc, DT_SINGLELINE );
}


/**********************************************************************
 *       User Button Functions
 */

static void UB_Paint( HWND hWnd, HDC hDC, WORD action )
{
    RECT rc;
    HBRUSH hBrush;
    WND *wndPtr = WIN_FindWndPtr( hWnd );
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    if (action == ODA_SELECT) return;

    GetClientRect(hWnd, &rc);

    if (infoPtr->hFont) SelectObject( hDC, infoPtr->hFont );
    hBrush = SendMessage(GetParent(hWnd), WM_CTLCOLOR, (WORD)hDC,
			 MAKELPARAM(hWnd, CTLCOLOR_BTN));
    FillRect(hDC, &rc, hBrush);

    if ((action == ODA_FOCUS) ||
        ((action == ODA_DRAWENTIRE) && (infoPtr->state & BUTTON_HASFOCUS)))
        DrawFocusRect(hDC, &rc);
}


/**********************************************************************
 *       Ownerdrawn Button Functions
 */

static void OB_Paint( HWND hWnd, HDC hDC, WORD action )
{
    DRAWITEMSTRUCT dis;
    WND *wndPtr = WIN_FindWndPtr( hWnd );
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    dis.CtlType    = ODT_BUTTON;
    dis.CtlID      = wndPtr->wIDmenu;
    dis.itemID     = 0;
    dis.itemAction = action;
    dis.itemState  = (infoPtr->state & BUTTON_HASFOCUS) ? ODS_FOCUS : 0 |
                     (infoPtr->state & BUTTON_HIGHLIGHTED) ? ODS_SELECTED : 0 |
                     (wndPtr->dwStyle & WS_DISABLED) ? ODS_DISABLED : 0;
    dis.hwndItem   = hWnd;
    dis.hDC        = hDC;
    GetClientRect( hWnd, &dis.rcItem );
    dis.itemData   = 0;
    SendMessage(GetParent(hWnd), WM_DRAWITEM, 1, MAKE_SEGPTR(&dis) );
}
