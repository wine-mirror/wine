/* File: button.c -- Button type widgets
 *
 * Copyright (C) 1993 Johannes Ruscheinski
 * Copyright (C) 1993 David Metcalfe
 * Copyright (C) 1994 Alexandre Julliard
 */

#include <string.h>
#include "win.h"
#include "button.h"
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
static void BUTTON_DrawPushButton( WND *wndPtr, HDC hDC, WORD action, BOOL pushedState);

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
 *           ButtonWndProc_locked
 * 
 * Called with window lock held.
 */
static inline LRESULT WINAPI ButtonWndProc_locked(WND* wndPtr, UINT uMsg,
					   WPARAM wParam, LPARAM lParam )
{
    RECT rect;
    HWND	hWnd = wndPtr->hwndSelf;
    POINT pt;
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;
    LONG style = wndPtr->dwStyle & 0x0f;
    HANDLE oldHbitmap;

    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);

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
            BITMAP bmp;
            hbitmapCheckBoxes = LoadBitmapA(0, MAKEINTRESOURCEA(OBM_CHECKBOXES));
            GetObjectA( hbitmapCheckBoxes, sizeof(bmp), &bmp );
            checkBoxWidth  = bmp.bmWidth / 4;
            checkBoxHeight = bmp.bmHeight / 3;
        }
        if (style < 0L || style >= MAX_BTN_TYPE)
            return -1; /* abort */
        infoPtr->state = BUTTON_UNCHECKED;
        infoPtr->hFont = 0;
        infoPtr->hImage = NULL;
        return 0;

    case WM_ERASEBKGND:
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

    case WM_LBUTTONDBLCLK:
	if(wndPtr->dwStyle & BS_NOTIFY || 
		style==BS_RADIOBUTTON ||
		style==BS_USERBUTTON ||
		style==BS_OWNERDRAW){
	    SendMessageA( GetParent(hWnd), WM_COMMAND,
		    MAKEWPARAM( wndPtr->wIDmenu, BN_DOUBLECLICKED ), hWnd);
	    break;
	}
	/* fall through */
    case WM_LBUTTONDOWN:
        SetCapture( hWnd );
        SetFocus( hWnd );
        SendMessageA( hWnd, BM_SETSTATE, TRUE, 0 );
        break;

    case WM_LBUTTONUP:
	/* FIXME: real windows uses extra flags in the status for this */
        if (GetCapture() != hWnd) break;
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
        if(style == BS_GROUPBOX) return HTTRANSPARENT;
        return DefWindowProcA( hWnd, uMsg, wParam, lParam );

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
        if ((style == BS_AUTORADIOBUTTON) && (GetCapture() != hWnd) &&
            !(SendMessageA(hWnd, BM_GETCHECK, 0, 0) & BST_CHECKED))
	{
            /* The notification is sent when the button (BS_AUTORADIOBUTTON) 
               is unckecked and the focus was not given by a mouse click. */
            SendMessageA( hWnd, BM_SETCHECK, TRUE, 0 );
            SendMessageA( GetParent(hWnd), WM_COMMAND,
                          MAKEWPARAM( wndPtr->wIDmenu, BN_CLICKED ), hWnd);
        }
        infoPtr->state |= BUTTON_HASFOCUS;
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
	if ((wndPtr->dwStyle & BS_BITMAP) || (wndPtr->dwStyle & BS_ICON))
	    infoPtr->hImage = (HANDLE) lParam;
	return oldHbitmap;

    case BM_GETIMAGE:
        if (wParam == IMAGE_BITMAP)
	    return (HBITMAP)infoPtr->hImage;
	else if (wParam == IMAGE_ICON)
	    return (HICON)infoPtr->hImage;
	else
	    return NULL;

    case BM_GETCHECK16:
    case BM_GETCHECK:
        return infoPtr->state & 3;

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
        return infoPtr->state;

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
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

/***********************************************************************
 *           ButtonWndProc
 * The button window procedure. This is just a wrapper which locks
 * the passed HWND and calls the real window procedure (with a WND*
 * pointer pointing to the locked windowstructure).
 */
LRESULT WINAPI ButtonWndProc( HWND hWnd, UINT uMsg,
                              WPARAM wParam, LPARAM lParam )
{
    LRESULT res;
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    res = ButtonWndProc_locked(wndPtr,uMsg,wParam,lParam);

    WIN_ReleaseWndPtr(wndPtr);
    return res;
}

/**********************************************************************
 *       Push Button Functions
 */
static void PB_Paint( WND *wndPtr, HDC hDC, WORD action )
{
    BUTTONINFO *infoPtr      = (BUTTONINFO *)wndPtr->wExtra;
    BOOL        bHighLighted = (infoPtr->state & BUTTON_HIGHLIGHTED);

    /* 
     * Delegate this to the more generic pushbutton painting
     * method.
     */
    BUTTON_DrawPushButton(wndPtr,
			  hDC,
			  action,
			  bHighLighted);
}

/**********************************************************************
 * This method will actually do the drawing of the pushbutton 
 * depending on it's state and the pushedState parameter.
 */
static void BUTTON_DrawPushButton(
  WND* wndPtr,
  HDC  hDC, 
  WORD action, 
  BOOL pushedState )
{
    RECT rc, focus_rect;
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

    if ( TWEAK_WineLook == WIN31_LOOK)
    {
        Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);

        SetPixel( hDC, rc.left, rc.top, GetSysColor(COLOR_WINDOW) );
        SetPixel( hDC, rc.left, rc.bottom-1, GetSysColor(COLOR_WINDOW) );
        SetPixel( hDC, rc.right-1, rc.top, GetSysColor(COLOR_WINDOW) );
        SetPixel( hDC, rc.right-1, rc.bottom-1, GetSysColor(COLOR_WINDOW));
	InflateRect( &rc, -1, -1 );
    }
    
    if ((wndPtr->dwStyle & 0x000f) == BS_DEFPUSHBUTTON)
    {
        Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
	InflateRect( &rc, -1, -1 );
    }

    if (TWEAK_WineLook == WIN31_LOOK)
    {
        if (pushedState)
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
    }
    else
    {
        UINT uState = DFCS_BUTTONPUSH;

        if (pushedState)
	{
	    if ( (wndPtr->dwStyle & 0x000f) == BS_DEFPUSHBUTTON )
	        uState |= DFCS_FLAT;
	    else
	        uState |= DFCS_PUSHED;
	}

	DrawFrameControl( hDC, &rc, DFC_BUTTON, uState );
	InflateRect( &rc, -2, -2 );

	focus_rect = rc;

        if (pushedState)
	{
	    rc.left += 2;  /* To position the text down and right */
	    rc.top  += 2;
	}
    }

    /* draw button label, if any:
     *
     * In win9x we don't show text if there is a bitmap or icon.
     * I don't know about win31 so I leave it as it was for win31.
     * Dennis Björklund 12 Jul, 99
     */
    if ( wndPtr->text && wndPtr->text[0]
	 && (TWEAK_WineLook == WIN31_LOOK || !(wndPtr->dwStyle & (BS_ICON|BS_BITMAP))) )
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
            /* do we have the focus?
	     * Win9x draws focus last with a size prop. to the button
	     */
            if (TWEAK_WineLook == WIN31_LOOK
		&& infoPtr->state & BUTTON_HASFOCUS)
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
    if ( ((wndPtr->dwStyle & BS_ICON) || (wndPtr->dwStyle & BS_BITMAP) ) &&
	 (infoPtr->hImage != NULL) )
    {
	int yOffset, xOffset;
	int imageWidth, imageHeight;

	/*
	 * We extract the size of the image from the handle.
	 */
	if (wndPtr->dwStyle & BS_ICON)
	{
	    ICONINFO iconInfo;
	    BITMAP   bm;

	    GetIconInfo((HICON)infoPtr->hImage, &iconInfo);
	    GetObjectA (iconInfo.hbmColor, sizeof(BITMAP), &bm);
	    
	    imageWidth  = bm.bmWidth;
	    imageHeight = bm.bmHeight;

            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);

	}
	else
	{
	    BITMAP   bm;

	    GetObjectA (infoPtr->hImage, sizeof(BITMAP), &bm);
	
	    imageWidth  = bm.bmWidth;
	    imageHeight = bm.bmHeight;
	}

	/* Center the bitmap */
	xOffset = (((rc.right - rc.left) - 2*xBorderOffset) - imageWidth ) / 2;
	yOffset = (((rc.bottom - rc.top) - 2*yBorderOffset) - imageHeight) / 2;

	/* If the image is too big for the button then create a region*/
        if(xOffset < 0 || yOffset < 0)
	{
            HRGN hBitmapRgn = NULL;
            hBitmapRgn = CreateRectRgn(
                rc.left + xBorderOffset, rc.top +yBorderOffset, 
                rc.right - xBorderOffset, rc.bottom - yBorderOffset);
            SelectClipRgn(hDC, hBitmapRgn);
            DeleteObject(hBitmapRgn);
	}

	/* Let minimum 1 space from border */
	xOffset++, yOffset++;

	/*
	 * Draw the image now.
	 */
	if (wndPtr->dwStyle & BS_ICON)
	{
  	    DrawIcon(hDC,
                rc.left + xOffset, rc.top + yOffset,
		     (HICON)infoPtr->hImage);
	}
	else
        {
	    HDC hdcMem;

	    hdcMem = CreateCompatibleDC (hDC);
	    SelectObject (hdcMem, (HBITMAP)infoPtr->hImage);
	    BitBlt(hDC, 
		   rc.left + xOffset, 
		   rc.top + yOffset, 
		   imageWidth, imageHeight,
		   hdcMem, 0, 0, SRCCOPY);
	    
	    DeleteDC (hdcMem);
	}

        if(xOffset < 0 || yOffset < 0)
        {
            SelectClipRgn(hDC, NULL);
        }
    }

    if (TWEAK_WineLook != WIN31_LOOK
	&& infoPtr->state & BUTTON_HASFOCUS)
    {
        InflateRect( &focus_rect, -1, -1 );
        DrawFocusRect( hDC, &focus_rect );
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

    /* 
     * if the button has a bitmap/icon, draw a normal pushbutton
     * instead of a radion button.
     */
    if (infoPtr->hImage!=NULL)
    {
        BOOL bHighLighted = ((infoPtr->state & BUTTON_HIGHLIGHTED) ||
			     (infoPtr->state & BUTTON_CHECKED));

        BUTTON_DrawPushButton(wndPtr,
			      hDC,
			      action,
			      bHighLighted);
	return;
    }

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
        delta = (rbox.bottom - rbox.top - checkBoxHeight) / 2;

	/* Check in case the client area is smaller than the checkbox bitmap */
	if (delta < 0) delta = 0;

        if (action == ODA_SELECT) FillRect( hDC, &rbox, hBrush );
        else FillRect( hDC, &client, hBrush );

        if (infoPtr->state & BUTTON_HIGHLIGHTED) x += 2 * checkBoxWidth;
        if (infoPtr->state & (BUTTON_CHECKED | BUTTON_3STATE)) x += checkBoxWidth;
        if (((wndPtr->dwStyle & 0x0f) == BS_RADIOBUTTON) ||
            ((wndPtr->dwStyle & 0x0f) == BS_AUTORADIOBUTTON)) y += checkBoxHeight;
        else if (infoPtr->state & BUTTON_3STATE) y += 2 * checkBoxHeight;

	/* The bitmap for the radio button is not aligned with the
	 * left of the window, it is 1 pixel off. */
        if (((wndPtr->dwStyle & 0x0f) == BS_RADIOBUTTON) ||
            ((wndPtr->dwStyle & 0x0f) == BS_AUTORADIOBUTTON))
	  rbox.left += 1;

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

    SetBkColor( hDC, GetSysColor( COLOR_BTNFACE ) );
    FillRect( hDC,  &dis.rcItem, GetSysColorBrush( COLOR_BTNFACE ) );

    SendMessageA( GetParent(wndPtr->hwndSelf), WM_DRAWITEM,
                    wndPtr->wIDmenu, (LPARAM)&dis );
}

