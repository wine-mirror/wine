/* File: button.c -- Button type widgets
 *
 * Copyright (C) 1993 Johannes Ruscheinski
 * Copyright (C) 1993 David Metcalfe
 * Copyright (C) 1994 Alexandre Julliard
 */

#include "win.h"
#include "graphics.h"
#include "button.h"
#include "windows.h"
#include "tweak.h"

static void PB_Paint( WND *wndPtr, HDC32 hDC, WORD action );
static void PB_PaintGrayOnGray(HDC32 hDC,HFONT32 hFont,RECT32 *rc,char *text);
static void CB_Paint( WND *wndPtr, HDC32 hDC, WORD action );
static void GB_Paint( WND *wndPtr, HDC32 hDC, WORD action );
static void UB_Paint( WND *wndPtr, HDC32 hDC, WORD action );
static void OB_Paint( WND *wndPtr, HDC32 hDC, WORD action );
static void BUTTON_CheckAutoRadioButton( WND *wndPtr );

#define MAX_BTN_TYPE  12

static const WORD maxCheckState[MAX_BTN_TYPE] =
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

typedef void (*pfPaint)( WND *wndPtr, HDC32 hdc, WORD action );

static const pfPaint btnPaintFunc[MAX_BTN_TYPE] =
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

#define PAINT_BUTTON(wndPtr,style,action) \
     if (btnPaintFunc[style]) { \
         HDC32 hdc = GetDC32( (wndPtr)->hwndSelf ); \
         (btnPaintFunc[style])(wndPtr,hdc,action); \
         ReleaseDC32( (wndPtr)->hwndSelf, hdc ); }

#define BUTTON_SEND_CTLCOLOR(wndPtr,hdc) \
    SendMessage32A( GetParent32((wndPtr)->hwndSelf), WM_CTLCOLORBTN, \
                    (hdc), (wndPtr)->hwndSelf )

static HBITMAP32 hbitmapCheckBoxes = 0;
static WORD checkBoxWidth = 0, checkBoxHeight = 0;


/***********************************************************************
 *           ButtonWndProc
 */
LRESULT WINAPI ButtonWndProc( HWND32 hWnd, UINT32 uMsg,
                              WPARAM32 wParam, LPARAM lParam )
{
    RECT32 rect;
    POINT32 pt = { LOWORD(lParam), HIWORD(lParam) };
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;
    LONG style = wndPtr->dwStyle & 0x0f;

    switch (uMsg)
    {
    case WM_GETDLGCODE:
        switch(style)
        {
        case BS_PUSHBUTTON:      return DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON;
        case BS_DEFPUSHBUTTON:   return DLGC_BUTTON | DLGC_DEFPUSHBUTTON;
        case BS_RADIOBUTTON:
        case BS_AUTORADIOBUTTON: return DLGC_BUTTON | DLGC_RADIOBUTTON;
        default:                 return DLGC_BUTTON;
        }

    case WM_ENABLE:
        PAINT_BUTTON( wndPtr, style, ODA_DRAWENTIRE );
        break;

    case WM_CREATE:
        if (!hbitmapCheckBoxes)
        {
            BITMAP32 bmp;
            hbitmapCheckBoxes = LoadBitmap32A(0, MAKEINTRESOURCE32A(OBM_CHECKBOXES));
            GetObject32A( hbitmapCheckBoxes, sizeof(bmp), &bmp );
            checkBoxWidth  = bmp.bmWidth / 4;
            checkBoxHeight = bmp.bmHeight / 3;
        }
        if (style < 0L || style >= MAX_BTN_TYPE) return -1; /* abort */
        infoPtr->state = BUTTON_UNCHECKED;
        infoPtr->hFont = 0;
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        if (btnPaintFunc[style])
        {
            PAINTSTRUCT32 ps;
            HDC32 hdc = wParam ? (HDC32)wParam : BeginPaint32( hWnd, &ps );
	    SetBkMode32( hdc, OPAQUE );
            (btnPaintFunc[style])( wndPtr, hdc, ODA_DRAWENTIRE );
            if( !wParam ) EndPaint32( hWnd, &ps );
        }
        break;

    case WM_LBUTTONDOWN:
        SendMessage32A( hWnd, BM_SETSTATE32, TRUE, 0 );
        SetFocus32( hWnd );
        SetCapture32( hWnd );
        break;

    case WM_LBUTTONUP:
        ReleaseCapture();
        if (!(infoPtr->state & BUTTON_HIGHLIGHTED)) break;
        SendMessage32A( hWnd, BM_SETSTATE32, FALSE, 0 );
        GetClientRect32( hWnd, &rect );
        if (PtInRect32( &rect, pt ))
        {
            switch(style)
            {
            case BS_AUTOCHECKBOX:
                SendMessage32A( hWnd, BM_SETCHECK32,
                                !(infoPtr->state & BUTTON_CHECKED), 0 );
                break;
            case BS_AUTORADIOBUTTON:
                SendMessage32A( hWnd, BM_SETCHECK32, TRUE, 0 );
                break;
            case BS_AUTO3STATE:
                SendMessage32A( hWnd, BM_SETCHECK32,
                                (infoPtr->state & BUTTON_3STATE) ? 0 :
                                ((infoPtr->state & 3) + 1), 0 );
                break;
            }
            SendMessage32A( GetParent32(hWnd), WM_COMMAND,
                            MAKEWPARAM( wndPtr->wIDmenu, BN_CLICKED ), hWnd);
        }
        break;

    case WM_MOUSEMOVE:
        if (GetCapture32() == hWnd)
        {
            GetClientRect32( hWnd, &rect );
            SendMessage32A( hWnd, BM_SETSTATE32, PtInRect32(&rect, pt), 0 );
        }
        break;

    case WM_NCHITTEST:
        if(style == BS_GROUPBOX) return HTTRANSPARENT;
        return DefWindowProc32A( hWnd, uMsg, wParam, lParam );

    case WM_SETTEXT:
        DEFWND_SetText( wndPtr, (LPCSTR)lParam );
	if( wndPtr->dwStyle & WS_VISIBLE )
            PAINT_BUTTON( wndPtr, style, ODA_DRAWENTIRE );
        return 0;

    case WM_SETFONT:
        infoPtr->hFont = (HFONT16)wParam;
        if (lParam && (wndPtr->dwStyle & WS_VISIBLE)) 
	    PAINT_BUTTON( wndPtr, style, ODA_DRAWENTIRE );
        break;

    case WM_GETFONT:
        return infoPtr->hFont;

    case WM_SETFOCUS:
        infoPtr->state |= BUTTON_HASFOCUS;
	if (style == BS_AUTORADIOBUTTON)
	{
	    SendMessage32A( hWnd, BM_SETCHECK32, 1, 0 );
	}
        PAINT_BUTTON( wndPtr, style, ODA_FOCUS );
        break;

    case WM_KILLFOCUS:
        infoPtr->state &= ~BUTTON_HASFOCUS;
	PAINT_BUTTON( wndPtr, style, ODA_FOCUS );
	InvalidateRect32( hWnd, NULL, TRUE );
        break;

    case WM_SYSCOLORCHANGE:
        InvalidateRect32( hWnd, NULL, FALSE );
        break;

    case BM_SETSTYLE16:
    case BM_SETSTYLE32:
        if ((wParam & 0x0f) >= MAX_BTN_TYPE) break;
        wndPtr->dwStyle = (wndPtr->dwStyle & 0xfffffff0) 
                           | (wParam & 0x0000000f);
        style = wndPtr->dwStyle & 0x0000000f;
        PAINT_BUTTON( wndPtr, style, ODA_DRAWENTIRE );
        break;

    case BM_GETCHECK16:
    case BM_GETCHECK32:
        return infoPtr->state & 3;

    case BM_SETCHECK16:
    case BM_SETCHECK32:
        if (wParam > maxCheckState[style]) wParam = maxCheckState[style];
        if ((infoPtr->state & 3) != wParam)
        {
	    if ((style == BS_RADIOBUTTON) || (style == BS_AUTORADIOBUTTON))
	    {
		if (wParam)
		    wndPtr->dwStyle |= WS_TABSTOP;
		else
		    wndPtr->dwStyle &= ~WS_TABSTOP;
	    }
            infoPtr->state = (infoPtr->state & ~3) | wParam;
            PAINT_BUTTON( wndPtr, style, ODA_SELECT );
        }
        if ((style == BS_AUTORADIOBUTTON) && (wParam == BUTTON_CHECKED))
            BUTTON_CheckAutoRadioButton( wndPtr );
        break;

    case BM_GETSTATE16:
    case BM_GETSTATE32:
        return infoPtr->state;

    case BM_SETSTATE16:
    case BM_SETSTATE32:
        if (wParam)
        {
            if (infoPtr->state & BUTTON_HIGHLIGHTED) break;
            infoPtr->state |= BUTTON_HIGHLIGHTED;
        }
        else
        {
            if (!(infoPtr->state & BUTTON_HIGHLIGHTED)) break;
            infoPtr->state &= ~BUTTON_HIGHLIGHTED;
        }
        PAINT_BUTTON( wndPtr, style, ODA_SELECT );
        break;

    default:
        return DefWindowProc32A(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}


/**********************************************************************
 *       Push Button Functions
 */

static void PB_Paint( WND *wndPtr, HDC32 hDC, WORD action )
{
    RECT32 rc;
    HPEN32 hOldPen;
    HBRUSH32 hOldBrush;
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    GetClientRect32( wndPtr->hwndSelf, &rc );

      /* Send WM_CTLCOLOR to allow changing the font (the colors are fixed) */
    if (infoPtr->hFont) SelectObject32( hDC, infoPtr->hFont );
    BUTTON_SEND_CTLCOLOR( wndPtr, hDC );
    hOldPen = (HPEN32)SelectObject32(hDC, GetSysColorPen32(COLOR_WINDOWFRAME));
    hOldBrush =(HBRUSH32)SelectObject32(hDC,GetSysColorBrush32(COLOR_BTNFACE));
    SetBkMode32(hDC, TRANSPARENT);
    Rectangle32(hDC, rc.left, rc.top, rc.right, rc.bottom);
    if (action == ODA_DRAWENTIRE)
    {
        SetPixel32( hDC, rc.left, rc.top, GetSysColor32(COLOR_WINDOW) );
        SetPixel32( hDC, rc.left, rc.bottom-1, GetSysColor32(COLOR_WINDOW) );
        SetPixel32( hDC, rc.right-1, rc.top, GetSysColor32(COLOR_WINDOW) );
        SetPixel32( hDC, rc.right-1, rc.bottom-1, GetSysColor32(COLOR_WINDOW));
    }
    InflateRect32( &rc, -1, -1 );

    if ((wndPtr->dwStyle & 0x000f) == BS_DEFPUSHBUTTON)
    {
        Rectangle32(hDC, rc.left, rc.top, rc.right, rc.bottom);
        InflateRect32( &rc, -1, -1 );
    }

    if (infoPtr->state & BUTTON_HIGHLIGHTED)
    {
        /* draw button shadow: */
        SelectObject32(hDC, GetSysColorBrush32(COLOR_BTNSHADOW));
        PatBlt32(hDC, rc.left, rc.top, 1, rc.bottom-rc.top, PATCOPY );
        PatBlt32(hDC, rc.left, rc.top, rc.right-rc.left, 1, PATCOPY );
        rc.left += 2;  /* To position the text down and right */
        rc.top  += 2;
    }
    else GRAPH_DrawReliefRect( hDC, &rc, 2, 2, FALSE );
    
    /* draw button label, if any: */
    if (wndPtr->text && wndPtr->text[0])
    {
        LOGBRUSH32 lb;
        GetObject32A( GetSysColorBrush32(COLOR_BTNFACE), sizeof(lb), &lb );
        if (wndPtr->dwStyle & WS_DISABLED &&
            GetSysColor32(COLOR_GRAYTEXT)==lb.lbColor)
            /* don't write gray text on gray bkg */
            PB_PaintGrayOnGray(hDC,infoPtr->hFont,&rc,wndPtr->text);
        else
        {
            SetTextColor32( hDC, (wndPtr->dwStyle & WS_DISABLED) ?
                                 GetSysColor32(COLOR_GRAYTEXT) :
                                 GetSysColor32(COLOR_BTNTEXT) );
            DrawText32A( hDC, wndPtr->text, -1, &rc,
                         DT_SINGLELINE | DT_CENTER | DT_VCENTER );
            /* do we have the focus? */
            if (infoPtr->state & BUTTON_HASFOCUS)
            {
                RECT32 r = { 0, 0, 0, 0 };
                INT32 xdelta, ydelta;

                DrawText32A( hDC, wndPtr->text, -1, &r,
                             DT_SINGLELINE | DT_CALCRECT );
                xdelta = ((rc.right - rc.left) - (r.right - r.left) - 1) / 2;
                ydelta = ((rc.bottom - rc.top) - (r.bottom - r.top) - 1) / 2;
                if (xdelta < 0) xdelta = 0;
                if (ydelta < 0) ydelta = 0;
                InflateRect32( &rc, -xdelta, -ydelta );
                DrawFocusRect32( hDC, &rc );
            }
        }   
    }

    SelectObject32( hDC, hOldPen );
    SelectObject32( hDC, hOldBrush );
}


/**********************************************************************
 *   Push Button sub function                               [internal]
 *   using a raster brush to avoid gray text on gray background
 */

void PB_PaintGrayOnGray(HDC32 hDC,HFONT32 hFont,RECT32 *rc,char *text)
{
    static const int Pattern[] = {0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55};
    HBITMAP32 hbm  = CreateBitmap32( 8, 8, 1, 1, Pattern );
    HDC32 hdcMem = CreateCompatibleDC32(hDC);
    HBITMAP32 hbmMem;
    HBRUSH32 hBr;
    RECT32 rect,rc2;

    rect=*rc;
    DrawText32A( hDC, text, -1, &rect, DT_SINGLELINE | DT_CALCRECT);
    rc2=rect;
    rect.left=(rc->right-rect.right)/2;       /* for centering text bitmap */
    rect.top=(rc->bottom-rect.bottom)/2;
    hbmMem = CreateCompatibleBitmap32( hDC,rect.right,rect.bottom );
    SelectObject32( hdcMem, hbmMem);
    hBr = SelectObject32( hdcMem, CreatePatternBrush32(hbm) );
    DeleteObject32( hbm );
    PatBlt32( hdcMem,0,0,rect.right,rect.bottom,WHITENESS);
    if (hFont) SelectObject32( hdcMem, hFont);
    DrawText32A( hdcMem, text, -1, &rc2, DT_SINGLELINE);  
    PatBlt32( hdcMem,0,0,rect.right,rect.bottom,0xFA0089);
    DeleteObject32( SelectObject32( hdcMem,hBr) );
    BitBlt32(hDC,rect.left,rect.top,rect.right,rect.bottom,hdcMem,0,0,0x990000);
    DeleteDC32( hdcMem);
    DeleteObject32( hbmMem );
}


/**********************************************************************
 *       Check Box & Radio Button Functions
 */

static void CB_Paint( WND *wndPtr, HDC32 hDC, WORD action )
{
    RECT32 rbox, rtext, client;
    HBRUSH32 hBrush;
    int textlen, delta;
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    textlen = 0;
    GetClientRect32(wndPtr->hwndSelf, &client);
    rbox = rtext = client;

    if (infoPtr->hFont) SelectObject32( hDC, infoPtr->hFont );

    /* Something is still not right, checkboxes (and edit controls)
     * in wsping32 have white backgrounds instead of dark grey.
     * BUTTON_SEND_CTLCOLOR() is even worse since it returns 0 in this
     * particular case and the background is not painted at all.
     */

    hBrush = GetControlBrush( wndPtr->hwndSelf, hDC, CTLCOLOR_BTN );

    if (wndPtr->dwStyle & BS_LEFTTEXT) 
    {
	/* magic +4 is what CTL3D expects */

        rtext.right -= checkBoxWidth + 4;
        rbox.left = rbox.right - checkBoxWidth;
    }
    else 
    {
        rtext.left += checkBoxWidth + 4;
        rbox.right = checkBoxWidth;
    }

      /* Draw the check-box bitmap */

    if (wndPtr->text) textlen = strlen( wndPtr->text );
    if (action == ODA_DRAWENTIRE || action == ODA_SELECT)
    { 
        int x = 0, y = 0;
        delta = (rbox.bottom - rbox.top - checkBoxHeight) >> 1;

        if (action == ODA_SELECT) FillRect32( hDC, &rbox, hBrush );
        else FillRect32( hDC, &client, hBrush );

        if (infoPtr->state & BUTTON_HIGHLIGHTED) x += 2 * checkBoxWidth;
        if (infoPtr->state & (BUTTON_CHECKED | BUTTON_3STATE)) x += checkBoxWidth;
        if (((wndPtr->dwStyle & 0x0f) == BS_RADIOBUTTON) ||
            ((wndPtr->dwStyle & 0x0f) == BS_AUTORADIOBUTTON)) y += checkBoxHeight;
        else if (infoPtr->state & BUTTON_3STATE) y += 2 * checkBoxHeight;

        GRAPH_DrawBitmap( hDC, hbitmapCheckBoxes, rbox.left, rbox.top + delta,
                          x, y, checkBoxWidth, checkBoxHeight, FALSE );
        if( textlen && action != ODA_SELECT )
        {
            if (wndPtr->dwStyle & WS_DISABLED)
                SetTextColor32( hDC, GetSysColor32(COLOR_GRAYTEXT) );
            DrawText32A( hDC, wndPtr->text, textlen, &rtext,
			 DT_SINGLELINE | DT_VCENTER );
	    textlen = 0; /* skip DrawText() below */
        }
    }

    if ((action == ODA_FOCUS) ||
        ((action == ODA_DRAWENTIRE) && (infoPtr->state & BUTTON_HASFOCUS)))
    {
	/* again, this is what CTL3D expects */

        SetRectEmpty32(&rbox);
        if( textlen )
            DrawText32A( hDC, wndPtr->text, textlen, &rbox,
			 DT_SINGLELINE | DT_CALCRECT );
        textlen = rbox.bottom - rbox.top;
        delta = ((rtext.bottom - rtext.top) - textlen)/2;
        rbox.bottom = (rbox.top = rtext.top + delta - 1) + textlen + 2;
        textlen = rbox.right - rbox.left;
        rbox.right = (rbox.left += --rtext.left) + textlen + 2;
        IntersectRect32(&rbox, &rbox, &rtext);
        DrawFocusRect32( hDC, &rbox );
    }
}


/**********************************************************************
 *       BUTTON_CheckAutoRadioButton
 *
 * wndPtr is checked, uncheck every other auto radio button in group
 */
static void BUTTON_CheckAutoRadioButton( WND *wndPtr )
{
    HWND32 parent, sibling, start;
    if (!(wndPtr->dwStyle & WS_CHILD)) return;
    parent = wndPtr->parent->hwndSelf;
    /* assure that starting control is not disabled or invisible */
    start = sibling = GetNextDlgGroupItem32( parent, wndPtr->hwndSelf, TRUE );
    do
    {
        if (!sibling) break;
        if ((wndPtr->hwndSelf != sibling) &&
            ((WIN_FindWndPtr(sibling)->dwStyle & 0x0f) == BS_AUTORADIOBUTTON))
            SendMessage32A( sibling, BM_SETCHECK32, BUTTON_UNCHECKED, 0 );
        sibling = GetNextDlgGroupItem32( parent, sibling, FALSE );
    } while (sibling != start);
}


/**********************************************************************
 *       Group Box Functions
 */

static void GB_Paint( WND *wndPtr, HDC32 hDC, WORD action )
{
    RECT32 rc, rcFrame;
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    if (action != ODA_DRAWENTIRE) return;

    BUTTON_SEND_CTLCOLOR( wndPtr, hDC );

    GetClientRect32( wndPtr->hwndSelf, &rc);
    if (TWEAK_WineLook == WIN31_LOOK)
	GRAPH_DrawRectangle( hDC, rc.left, rc.top + 2, rc.right - 1, rc.bottom - 1,
			    GetSysColorPen32(COLOR_WINDOWFRAME) );
    else {
	TEXTMETRIC32A tm;
	rcFrame = rc;

	if (infoPtr->hFont)
	    SelectObject32 (hDC, infoPtr->hFont);
	GetTextMetrics32A (hDC, &tm);
	rcFrame.top += (tm.tmHeight / 2) - 1;
	DrawEdge32 (hDC, &rcFrame, EDGE_ETCHED, BF_RECT);
    }

    if (wndPtr->text)
    {
	if (infoPtr->hFont) SelectObject32( hDC, infoPtr->hFont );
        if (wndPtr->dwStyle & WS_DISABLED)
            SetTextColor32( hDC, GetSysColor32(COLOR_GRAYTEXT) );
        rc.left += 10;
        DrawText32A( hDC, wndPtr->text, -1, &rc, DT_SINGLELINE | DT_NOCLIP );
    }
}


/**********************************************************************
 *       User Button Functions
 */

static void UB_Paint( WND *wndPtr, HDC32 hDC, WORD action )
{
    RECT32 rc;
    HBRUSH32 hBrush;
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    if (action == ODA_SELECT) return;

    GetClientRect32( wndPtr->hwndSelf, &rc);

    if (infoPtr->hFont) SelectObject32( hDC, infoPtr->hFont );
    hBrush = GetControlBrush( wndPtr->hwndSelf, hDC, CTLCOLOR_BTN );

    FillRect32( hDC, &rc, hBrush );
    if ((action == ODA_FOCUS) ||
        ((action == ODA_DRAWENTIRE) && (infoPtr->state & BUTTON_HASFOCUS)))
        DrawFocusRect32( hDC, &rc );
}


/**********************************************************************
 *       Ownerdrawn Button Functions
 */

static void OB_Paint( WND *wndPtr, HDC32 hDC, WORD action )
{
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;
    DRAWITEMSTRUCT32 dis;

    dis.CtlType    = ODT_BUTTON;
    dis.CtlID      = wndPtr->wIDmenu;
    dis.itemID     = 0;
    dis.itemAction = action;
    dis.itemState  = ((infoPtr->state & BUTTON_HASFOCUS) ? ODS_FOCUS : 0) |
                     ((infoPtr->state & BUTTON_HIGHLIGHTED) ? ODS_SELECTED : 0) |
                     ((wndPtr->dwStyle & WS_DISABLED) ? ODS_DISABLED : 0);
    dis.hwndItem   = wndPtr->hwndSelf;
    dis.hDC        = hDC;
    dis.itemData   = 0;
    GetClientRect32( wndPtr->hwndSelf, &dis.rcItem );
    SendMessage32A( GetParent32(wndPtr->hwndSelf), WM_DRAWITEM,
                    wndPtr->wIDmenu, (LPARAM)&dis );
}
