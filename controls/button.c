/* File: button.c -- Button type widgets
 *
 * Copyright (C) 1993 Johannes Ruscheinski
 * Copyright (C) 1993 David Metcalfe
 * Copyright (C) 1994 Alexandre Julliard
 */

#include <string.h>
#include "win.h"
#include "button.h"
#include "winuser.h"
#include "wine/winuser16.h"
#include "tweak.h"

static void PaintGrayOnGray( HDC hDC,HFONT hFont,RECT *rc,
			     char *text, UINT format );

static void PB_Paint( WND *wndPtr, HDC hDC, WORD action );
static void CB_Paint( WND *wndPtr, HDC hDC, WORD action );
static void GB_Paint( WND *wndPtr, HDC hDC, WORD action );
static void UB_Paint( WND *wndPtr, HDC hDC, WORD action );
static void OB_Paint( WND *wndPtr, HDC hDC, WORD action );
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

typedef void (*pfPaint)( WND *wndPtr, HDC hdc, WORD action );

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
         HDC hdc = GetDC( (wndPtr)->hwndSelf ); \
         (btnPaintFunc[style])(wndPtr,hdc,action); \
         ReleaseDC( (wndPtr)->hwndSelf, hdc ); }

#define BUTTON_SEND_CTLCOLOR(wndPtr,hdc) \
    SendMessageA( GetParent((wndPtr)->hwndSelf), WM_CTLCOLORBTN, \
                    (hdc), (wndPtr)->hwndSelf )

static HBITMAP hbitmapCheckBoxes = 0;
static WORD checkBoxWidth = 0, checkBoxHeight = 0;


/***********************************************************************
 *           ButtonWndProc
 */
LRESULT WINAPI ButtonWndProc( HWND hWnd, UINT uMsg,
                              WPARAM wParam, LPARAM lParam )
{
    RECT rect;
    LRESULT retvalue;
    POINT pt;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;
    LONG style = wndPtr->dwStyle & 0x0f;
    HANDLE oldHbitmap;

    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);

    switch (uMsg)
    {
    case WM_GETDLGCODE:
        WIN_ReleaseWndPtr(wndPtr);
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
            BITMAP bmp;
            hbitmapCheckBoxes = LoadBitmapA(0, MAKEINTRESOURCEA(OBM_CHECKBOXES));
            GetObjectA( hbitmapCheckBoxes, sizeof(bmp), &bmp );
            checkBoxWidth  = bmp.bmWidth / 4;
            checkBoxHeight = bmp.bmHeight / 3;
        }
        if (style < 0L || style >= MAX_BTN_TYPE)
        {
            WIN_ReleaseWndPtr(wndPtr);
            return -1; /* abort */
        }
        infoPtr->state = BUTTON_UNCHECKED;
        infoPtr->hFont = 0;
        infoPtr->hImage = NULL;
        WIN_ReleaseWndPtr(wndPtr);
        return 0;

    case WM_ERASEBKGND:
        WIN_ReleaseWndPtr(wndPtr);
        return 1;

    case WM_PAINT:
        if (btnPaintFunc[style])
        {
            PAINTSTRUCT ps;
            HDC hdc = wParam ? (HDC)wParam : BeginPaint( hWnd, &ps );
	    SetBkMode( hdc, OPAQUE );
            (btnPaintFunc[style])( wndPtr, hdc, ODA_DRAWENTIRE );
            if( !wParam ) EndPaint( hWnd, &ps );
        }
        break;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        SendMessageA( hWnd, BM_SETSTATE, TRUE, 0 );
        SetFocus( hWnd );
        SetCapture( hWnd );
        break;

    case WM_LBUTTONUP:
        ReleaseCapture();
        if (!(infoPtr->state & BUTTON_HIGHLIGHTED)) break;
        SendMessageA( hWnd, BM_SETSTATE, FALSE, 0 );
        GetClientRect( hWnd, &rect );
        if (PtInRect( &rect, pt ))
        {
            switch(style)
            {
            case BS_AUTOCHECKBOX:
                SendMessageA( hWnd, BM_SETCHECK,
                                !(infoPtr->state & BUTTON_CHECKED), 0 );
                break;
            case BS_AUTORADIOBUTTON:
                SendMessageA( hWnd, BM_SETCHECK, TRUE, 0 );
                break;
            case BS_AUTO3STATE:
                SendMessageA( hWnd, BM_SETCHECK,
                                (infoPtr->state & BUTTON_3STATE) ? 0 :
                                ((infoPtr->state & 3) + 1), 0 );
                break;
            }
            SendMessageA( GetParent(hWnd), WM_COMMAND,
                            MAKEWPARAM( wndPtr->wIDmenu, BN_CLICKED ), hWnd);
        }
        break;

    case WM_MOUSEMOVE:
        if (GetCapture() == hWnd)
        {
            GetClientRect( hWnd, &rect );
            SendMessageA( hWnd, BM_SETSTATE, PtInRect(&rect, pt), 0 );
        }
        break;

    case WM_NCHITTEST:
        WIN_ReleaseWndPtr(wndPtr);
        if(style == BS_GROUPBOX) return HTTRANSPARENT;
        return DefWindowProcA( hWnd, uMsg, wParam, lParam );

    case WM_SETTEXT:
        DEFWND_SetText( wndPtr, (LPCSTR)lParam );
	if( wndPtr->dwStyle & WS_VISIBLE )
            PAINT_BUTTON( wndPtr, style, ODA_DRAWENTIRE );
        WIN_ReleaseWndPtr(wndPtr);
        return 0;

    case WM_SETFONT:
        infoPtr->hFont = (HFONT16)wParam;
        if (lParam && (wndPtr->dwStyle & WS_VISIBLE)) 
	    PAINT_BUTTON( wndPtr, style, ODA_DRAWENTIRE );
        break;

    case WM_GETFONT:
        retvalue = infoPtr->hFont;
        WIN_ReleaseWndPtr(wndPtr);
        return retvalue;

    case WM_SETFOCUS:
        infoPtr->state |= BUTTON_HASFOCUS;
	if (style == BS_AUTORADIOBUTTON)
	{
	    SendMessageA( hWnd, BM_SETCHECK, 1, 0 );
	}
        PAINT_BUTTON( wndPtr, style, ODA_FOCUS );
        break;

    case WM_KILLFOCUS:
        infoPtr->state &= ~BUTTON_HASFOCUS;
	PAINT_BUTTON( wndPtr, style, ODA_FOCUS );
	InvalidateRect( hWnd, NULL, TRUE );
        break;

    case WM_SYSCOLORCHANGE:
        InvalidateRect( hWnd, NULL, FALSE );
        break;

    case BM_SETSTYLE16:
    case BM_SETSTYLE:
        if ((wParam & 0x0f) >= MAX_BTN_TYPE) break;
        wndPtr->dwStyle = (wndPtr->dwStyle & 0xfffffff0) 
                           | (wParam & 0x0000000f);
        style = wndPtr->dwStyle & 0x0000000f;
        PAINT_BUTTON( wndPtr, style, ODA_DRAWENTIRE );
        break;

    case BM_SETIMAGE:
	oldHbitmap = infoPtr->hImage;
	if(wndPtr->dwStyle & BS_BITMAP)
	    infoPtr->hImage = (HANDLE) lParam;
        WIN_ReleaseWndPtr(wndPtr);
	return oldHbitmap;

    case BM_GETIMAGE:
        WIN_ReleaseWndPtr(wndPtr);
	return infoPtr->hImage;

    case BM_GETCHECK16:
    case BM_GETCHECK:
        retvalue = infoPtr->state & 3;
        WIN_ReleaseWndPtr(wndPtr);
        return retvalue;

    case BM_SETCHECK16:
    case BM_SETCHECK:
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
    case BM_GETSTATE:
        retvalue = infoPtr->state;
        WIN_ReleaseWndPtr(wndPtr);
        return retvalue;

    case BM_SETSTATE16:
    case BM_SETSTATE:
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
        WIN_ReleaseWndPtr(wndPtr);
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
    WIN_ReleaseWndPtr(wndPtr);
    return 0;
}


/**********************************************************************
 *       Push Button Functions
 */

static void PB_Paint( WND *wndPtr, HDC hDC, WORD action )
{
    RECT rc;
    HPEN hOldPen;
    HBRUSH hOldBrush;
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;
    int xBorderOffset, yBorderOffset;
    xBorderOffset = yBorderOffset = 0;

    GetClientRect( wndPtr->hwndSelf, &rc );

      /* Send WM_CTLCOLOR to allow changing the font (the colors are fixed) */
    if (infoPtr->hFont) SelectObject( hDC, infoPtr->hFont );
    BUTTON_SEND_CTLCOLOR( wndPtr, hDC );
    hOldPen = (HPEN)SelectObject(hDC, GetSysColorPen(COLOR_WINDOWFRAME));
    hOldBrush =(HBRUSH)SelectObject(hDC,GetSysColorBrush(COLOR_BTNFACE));
    SetBkMode(hDC, TRANSPARENT);
    Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
/*    if (action == ODA_DRAWENTIRE)*/
    {
        SetPixel( hDC, rc.left, rc.top, GetSysColor(COLOR_WINDOW) );
        SetPixel( hDC, rc.left, rc.bottom-1, GetSysColor(COLOR_WINDOW) );
        SetPixel( hDC, rc.right-1, rc.top, GetSysColor(COLOR_WINDOW) );
        SetPixel( hDC, rc.right-1, rc.bottom-1, GetSysColor(COLOR_WINDOW));
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
        SelectObject(hDC, GetSysColorBrush(COLOR_BTNSHADOW));
        PatBlt(hDC, rc.left, rc.top, 1, rc.bottom-rc.top, PATCOPY );
        PatBlt(hDC, rc.left, rc.top, rc.right-rc.left, 1, PATCOPY );
        rc.left += 2;  /* To position the text down and right */
        rc.top  += 2;
    } else {
        rc.right++, rc.bottom++;
	DrawEdge( hDC, &rc, EDGE_RAISED, BF_RECT );

	/* To place de bitmap correctly */
	xBorderOffset += GetSystemMetrics(SM_CXEDGE);
	yBorderOffset += GetSystemMetrics(SM_CYEDGE);

        rc.right--, rc.bottom--;
    }
	
    /* draw button label, if any: */
    if (wndPtr->text && wndPtr->text[0])
    {
        LOGBRUSH lb;
        GetObjectA( GetSysColorBrush(COLOR_BTNFACE), sizeof(lb), &lb );
        if (wndPtr->dwStyle & WS_DISABLED &&
            GetSysColor(COLOR_GRAYTEXT)==lb.lbColor)
            /* don't write gray text on gray background */
            PaintGrayOnGray( hDC,infoPtr->hFont,&rc,wndPtr->text,
			       DT_CENTER | DT_VCENTER );
        else
        {
            SetTextColor( hDC, (wndPtr->dwStyle & WS_DISABLED) ?
                                 GetSysColor(COLOR_GRAYTEXT) :
                                 GetSysColor(COLOR_BTNTEXT) );
            DrawTextA( hDC, wndPtr->text, -1, &rc,
                         DT_SINGLELINE | DT_CENTER | DT_VCENTER );
            /* do we have the focus? */
            if (infoPtr->state & BUTTON_HASFOCUS)
            {
                RECT r = { 0, 0, 0, 0 };
                INT xdelta, ydelta;

                DrawTextA( hDC, wndPtr->text, -1, &r,
                             DT_SINGLELINE | DT_CALCRECT );
                xdelta = ((rc.right - rc.left) - (r.right - r.left) - 1) / 2;
                ydelta = ((rc.bottom - rc.top) - (r.bottom - r.top) - 1) / 2;
                if (xdelta < 0) xdelta = 0;
                if (ydelta < 0) ydelta = 0;
                InflateRect( &rc, -xdelta, -ydelta );
                DrawFocusRect( hDC, &rc );
            }
        }   
    }

    if((wndPtr->dwStyle & BS_BITMAP) && (infoPtr->hImage != NULL))
    {
	BITMAP bm;
	HDC hdcMem;
	int yOffset, xOffset, imageWidth, imageHeight;

	GetObjectA (infoPtr->hImage, sizeof(BITMAP), &bm);
	
	/* Center the bitmap */
	xOffset = (((rc.right - rc.left) - 2*xBorderOffset) - bm.bmWidth ) / 2;
	yOffset = (((rc.bottom - rc.top) - 2*yBorderOffset) - bm.bmHeight ) / 2;

	imageWidth = bm.bmWidth;
	imageHeight = bm.bmHeight;

	/* If the image is to big for the button */
	if (xOffset < 0)
	{
	    imageWidth = rc.right - rc.left - 2*xBorderOffset -1;
	    xOffset = xBorderOffset;
	}

	if (yOffset < 0)
	{
	    imageHeight = rc.bottom - rc.top - 2*yBorderOffset -1;
	    yOffset = yBorderOffset;
	}

	/* Let minimum 1 space from border */
	xOffset++, yOffset++;

	hdcMem = CreateCompatibleDC (hDC);
	SelectObject (hdcMem, (HBITMAP)infoPtr->hImage);
	BitBlt(hDC, rc.left + xOffset, 
	       rc.top + yOffset, 
	       imageWidth, imageHeight,
	       hdcMem, 0, 0, SRCCOPY);

	DeleteDC (hdcMem);
    }
    
    SelectObject( hDC, hOldPen );
    SelectObject( hDC, hOldBrush );
}


/**********************************************************************
 *   PB_Paint & CB_Paint sub function                        [internal]
 *   Paint text using a raster brush to avoid gray text on gray 
 *   background. 'format' can be a combination of DT_CENTER and 
 *   DT_VCENTER to use this function in both PB_PAINT and 
 *   CB_PAINT.   - Dirk Thierbach
 *
 *   FIXME: This and TEXT_GrayString should be eventually combined,
 *   so calling one common function from PB_Paint, CB_Paint and
 *   TEXT_GrayString will be enough. Also note that this
 *   function ignores the CACHE_GetPattern funcs.
 */

void PaintGrayOnGray(HDC hDC,HFONT hFont,RECT *rc,char *text,
			UINT format)
{
/*  This is the standard gray on gray pattern:
    static const WORD Pattern[] = {0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55}; 
*/
/*  This pattern gives better readability with X Fonts.
    FIXME: Maybe the user should be allowed to decide which he wants. */
    static const WORD Pattern[] = {0x55,0xFF,0xAA,0xFF,0x55,0xFF,0xAA,0xFF}; 

    HBITMAP hbm  = CreateBitmap( 8, 8, 1, 1, Pattern );
    HDC hdcMem = CreateCompatibleDC(hDC);
    HBITMAP hbmMem;
    HBRUSH hBr;
    RECT rect,rc2;

    rect=*rc;
    DrawTextA( hDC, text, -1, &rect, DT_SINGLELINE | DT_CALCRECT);
    /* now text width and height are in rect.right and rect.bottom */
    rc2=rect;
    rect.left = rect.top = 0; /* drawing pos in hdcMem */
    if (format & DT_CENTER) rect.left=(rc->right-rect.right)/2;
    if (format & DT_VCENTER) rect.top=(rc->bottom-rect.bottom)/2;
    hbmMem = CreateCompatibleBitmap( hDC,rect.right,rect.bottom );
    SelectObject( hdcMem, hbmMem);
    PatBlt( hdcMem,0,0,rect.right,rect.bottom,WHITENESS);
      /* will be overwritten by DrawText, but just in case */
    if (hFont) SelectObject( hdcMem, hFont);
    DrawTextA( hdcMem, text, -1, &rc2, DT_SINGLELINE);  
      /* After draw: foreground = 0 bits, background = 1 bits */
    hBr = SelectObject( hdcMem, CreatePatternBrush(hbm) );
    DeleteObject( hbm );
    PatBlt( hdcMem,0,0,rect.right,rect.bottom,0xAF0229); 
      /* only keep the foreground bits where pattern is 1 */
    DeleteObject( SelectObject( hdcMem,hBr) );
    BitBlt(hDC,rect.left,rect.top,rect.right,rect.bottom,hdcMem,0,0,SRCAND);
      /* keep the background of the dest */
    DeleteDC( hdcMem);
    DeleteObject( hbmMem );
}


/**********************************************************************
 *       Check Box & Radio Button Functions
 */

static void CB_Paint( WND *wndPtr, HDC hDC, WORD action )
{
    RECT rbox, rtext, client;
    HBRUSH hBrush;
    int textlen, delta;
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    textlen = 0;
    GetClientRect(wndPtr->hwndSelf, &client);
    rbox = rtext = client;

    if (infoPtr->hFont) SelectObject( hDC, infoPtr->hFont );

    /* Something is still not right, checkboxes (and edit controls)
     * in wsping32 have white backgrounds instead of dark grey.
     * BUTTON_SEND_CTLCOLOR() is even worse since it returns 0 in this
     * particular case and the background is not painted at all.
     */

    hBrush = GetControlBrush16( wndPtr->hwndSelf, hDC, CTLCOLOR_BTN );

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
        HDC hMemDC = CreateCompatibleDC( hDC );
        int x = 0, y = 0;
        delta = (rbox.bottom - rbox.top - checkBoxHeight) >> 1;

        if (action == ODA_SELECT) FillRect( hDC, &rbox, hBrush );
        else FillRect( hDC, &client, hBrush );

        if (infoPtr->state & BUTTON_HIGHLIGHTED) x += 2 * checkBoxWidth;
        if (infoPtr->state & (BUTTON_CHECKED | BUTTON_3STATE)) x += checkBoxWidth;
        if (((wndPtr->dwStyle & 0x0f) == BS_RADIOBUTTON) ||
            ((wndPtr->dwStyle & 0x0f) == BS_AUTORADIOBUTTON)) y += checkBoxHeight;
        else if (infoPtr->state & BUTTON_3STATE) y += 2 * checkBoxHeight;

	SelectObject( hMemDC, hbitmapCheckBoxes );
	BitBlt( hDC, rbox.left, rbox.top + delta, checkBoxWidth,
		  checkBoxHeight, hMemDC, x, y, SRCCOPY );
	DeleteDC( hMemDC );

        if( textlen && action != ODA_SELECT )
        {
	  if (wndPtr->dwStyle & WS_DISABLED &&
	      GetSysColor(COLOR_GRAYTEXT)==GetBkColor(hDC)) {
            /* don't write gray text on gray background */
            PaintGrayOnGray( hDC, infoPtr->hFont, &rtext, wndPtr->text,
			     DT_VCENTER);
	  } else {
            if (wndPtr->dwStyle & WS_DISABLED)
                SetTextColor( hDC, GetSysColor(COLOR_GRAYTEXT) );
            DrawTextA( hDC, wndPtr->text, textlen, &rtext,
			 DT_SINGLELINE | DT_VCENTER );
	    textlen = 0; /* skip DrawText() below */
	  }
        }
    }

    if ((action == ODA_FOCUS) ||
        ((action == ODA_DRAWENTIRE) && (infoPtr->state & BUTTON_HASFOCUS)))
    {
	/* again, this is what CTL3D expects */

        SetRectEmpty(&rbox);
        if( textlen )
            DrawTextA( hDC, wndPtr->text, textlen, &rbox,
			 DT_SINGLELINE | DT_CALCRECT );
        textlen = rbox.bottom - rbox.top;
        delta = ((rtext.bottom - rtext.top) - textlen)/2;
        rbox.bottom = (rbox.top = rtext.top + delta - 1) + textlen + 2;
        textlen = rbox.right - rbox.left;
        rbox.right = (rbox.left += --rtext.left) + textlen + 2;
        IntersectRect(&rbox, &rbox, &rtext);
        DrawFocusRect( hDC, &rbox );
    }
}


/**********************************************************************
 *       BUTTON_CheckAutoRadioButton
 *
 * wndPtr is checked, uncheck every other auto radio button in group
 */
static void BUTTON_CheckAutoRadioButton( WND *wndPtr )
{
    HWND parent, sibling, start;
    if (!(wndPtr->dwStyle & WS_CHILD)) return;
    parent = wndPtr->parent->hwndSelf;
    /* assure that starting control is not disabled or invisible */
    start = sibling = GetNextDlgGroupItem( parent, wndPtr->hwndSelf, TRUE );
    do
    {
        WND *tmpWnd;
        if (!sibling) break;
        tmpWnd = WIN_FindWndPtr(sibling);
        if ((wndPtr->hwndSelf != sibling) &&
            ((tmpWnd->dwStyle & 0x0f) == BS_AUTORADIOBUTTON))
            SendMessageA( sibling, BM_SETCHECK, BUTTON_UNCHECKED, 0 );
        sibling = GetNextDlgGroupItem( parent, sibling, FALSE );
        WIN_ReleaseWndPtr(tmpWnd);
    } while (sibling != start);
}


/**********************************************************************
 *       Group Box Functions
 */

static void GB_Paint( WND *wndPtr, HDC hDC, WORD action )
{
    RECT rc, rcFrame;
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    if (action != ODA_DRAWENTIRE) return;

    BUTTON_SEND_CTLCOLOR( wndPtr, hDC );

    GetClientRect( wndPtr->hwndSelf, &rc);
    if (TWEAK_WineLook == WIN31_LOOK) {
        HPEN hPrevPen = SelectObject( hDC,
					  GetSysColorPen(COLOR_WINDOWFRAME));
	HBRUSH hPrevBrush = SelectObject( hDC,
					      GetStockObject(NULL_BRUSH) );

	Rectangle( hDC, rc.left, rc.top + 2, rc.right - 1, rc.bottom - 1 );
	SelectObject( hDC, hPrevBrush );
	SelectObject( hDC, hPrevPen );
    } else {
	TEXTMETRICA tm;
	rcFrame = rc;

	if (infoPtr->hFont)
	    SelectObject (hDC, infoPtr->hFont);
	GetTextMetricsA (hDC, &tm);
	rcFrame.top += (tm.tmHeight / 2) - 1;
	DrawEdge (hDC, &rcFrame, EDGE_ETCHED, BF_RECT);
    }

    if (wndPtr->text)
    {
	if (infoPtr->hFont) SelectObject( hDC, infoPtr->hFont );
        if (wndPtr->dwStyle & WS_DISABLED)
            SetTextColor( hDC, GetSysColor(COLOR_GRAYTEXT) );
        rc.left += 10;
        DrawTextA( hDC, wndPtr->text, -1, &rc, DT_SINGLELINE | DT_NOCLIP );
    }
}


/**********************************************************************
 *       User Button Functions
 */

static void UB_Paint( WND *wndPtr, HDC hDC, WORD action )
{
    RECT rc;
    HBRUSH hBrush;
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    if (action == ODA_SELECT) return;

    GetClientRect( wndPtr->hwndSelf, &rc);

    if (infoPtr->hFont) SelectObject( hDC, infoPtr->hFont );
    hBrush = GetControlBrush16( wndPtr->hwndSelf, hDC, CTLCOLOR_BTN );

    FillRect( hDC, &rc, hBrush );
    if ((action == ODA_FOCUS) ||
        ((action == ODA_DRAWENTIRE) && (infoPtr->state & BUTTON_HASFOCUS)))
        DrawFocusRect( hDC, &rc );
}


/**********************************************************************
 *       Ownerdrawn Button Functions
 */

static void OB_Paint( WND *wndPtr, HDC hDC, WORD action )
{
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;
    DRAWITEMSTRUCT dis;

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
    GetClientRect( wndPtr->hwndSelf, &dis.rcItem );
    SendMessageA( GetParent(wndPtr->hwndSelf), WM_DRAWITEM,
                    wndPtr->wIDmenu, (LPARAM)&dis );
}

