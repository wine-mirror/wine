/* File: button.c -- Button type widgets
 *
 * Copyright (C) 1993 Johannes Ruscheinski
 * Copyright (C) 1993 David Metcalfe
 * Copyright (C) 1994 Alexandre Julliard
 */

#include "win.h"
#include "syscolor.h"
#include "graphics.h"
#include "button.h"

static void PB_Paint( WND *wndPtr, HDC hDC, WORD action );
static void PB_PaintGrayOnGray(HDC hDC,HFONT hFont,RECT16 *rc,char *text);
static void CB_Paint( WND *wndPtr, HDC hDC, WORD action );
static void GB_Paint( WND *wndPtr, HDC hDC, WORD action );
static void UB_Paint( WND *wndPtr, HDC hDC, WORD action );
static void OB_Paint( WND *wndPtr, HDC hDC, WORD action );
static void BUTTON_CheckAutoRadioButton( WND *wndPtr );

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

typedef void (*pfPaint)( WND *wndPtr, HDC hdc, WORD action );

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

#define PAINT_BUTTON(wndPtr,style,action) \
     if (btnPaintFunc[style]) { \
         HDC hdc = GetDC( (wndPtr)->hwndSelf ); \
         (btnPaintFunc[style])(wndPtr,hdc,action); \
         ReleaseDC( (wndPtr)->hwndSelf, hdc ); }

#define BUTTON_SEND_CTLCOLOR(wndPtr,hdc) \
    SendMessage32A( GetParent((wndPtr)->hwndSelf), WM_CTLCOLORBTN, \
                    (hdc), (wndPtr)->hwndSelf )

static HBITMAP hbitmapCheckBoxes = 0;
static WORD checkBoxWidth = 0, checkBoxHeight = 0;


/***********************************************************************
 *           ButtonWndProc
 */
LRESULT ButtonWndProc(HWND32 hWnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    RECT16 rect;
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
            BITMAP bmp;
            hbitmapCheckBoxes = LoadBitmap(0, MAKEINTRESOURCE(OBM_CHECKBOXES));
            GetObject( hbitmapCheckBoxes, sizeof(bmp), (LPSTR)&bmp );
            checkBoxWidth  = bmp.bmWidth / 4;
            checkBoxHeight = bmp.bmHeight / 3;
        }
        if (style < 0L || style >= MAX_BTN_TYPE) return -1; /* abort */
        infoPtr->state = BUTTON_UNCHECKED;
        infoPtr->hFont = 0;
        return 0;

    case WM_ERASEBKGND:
        break;

    case WM_PAINT:
        if (btnPaintFunc[style])
        {
            PAINTSTRUCT16 ps;
            HDC hdc = BeginPaint16( hWnd, &ps );
            (btnPaintFunc[style])( wndPtr, hdc, ODA_DRAWENTIRE );
            EndPaint16( hWnd, &ps );
        }
        break;

    case WM_LBUTTONDOWN:
        SendMessage32A( hWnd, BM_SETSTATE32, TRUE, 0 );
        SetFocus( hWnd );
        SetCapture( hWnd );
        break;

    case WM_LBUTTONUP:
        ReleaseCapture();
        if (!(infoPtr->state & BUTTON_HIGHLIGHTED)) break;
        SendMessage16( hWnd, BM_SETSTATE16, FALSE, 0 );
        GetClientRect16( hWnd, &rect );
        if (PtInRect16( &rect, MAKEPOINT16(lParam) ))
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
            SendMessage32A( GetParent(hWnd), WM_COMMAND,
                            MAKEWPARAM( wndPtr->wIDmenu, BN_CLICKED ), hWnd);
        }
        break;

    case WM_MOUSEMOVE:
        if (GetCapture() == hWnd)
        {
            GetClientRect16( hWnd, &rect );
            SendMessage32A( hWnd, BM_SETSTATE32,
                            PtInRect16( &rect,MAKEPOINT16(lParam) ), 0 );
        }
        break;

    case WM_NCHITTEST:
        if(style == BS_GROUPBOX) return HTTRANSPARENT;
        return DefWindowProc32A( hWnd, uMsg, wParam, lParam );

    case WM_SETTEXT:
        DEFWND_SetText( wndPtr, (LPSTR)lParam );
        PAINT_BUTTON( wndPtr, style, ODA_DRAWENTIRE );
        return 0;

    case WM_SETFONT:
        infoPtr->hFont = (HFONT) wParam;
        if (lParam) PAINT_BUTTON( wndPtr, style, ODA_DRAWENTIRE );
        break;

    case WM_GETFONT:
        return infoPtr->hFont;

    case WM_SETFOCUS:
        infoPtr->state |= BUTTON_HASFOCUS;
        PAINT_BUTTON( wndPtr, style, ODA_FOCUS );
        break;

    case WM_KILLFOCUS:
        infoPtr->state &= ~BUTTON_HASFOCUS;
        PAINT_BUTTON( wndPtr, style, ODA_FOCUS );
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

static void PB_Paint( WND *wndPtr, HDC hDC, WORD action )
{
    RECT16 rc;
    HPEN16 hOldPen;
    HBRUSH hOldBrush;
    DWORD dwTextSize;
    TEXTMETRIC tm;
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    GetClientRect16(wndPtr->hwndSelf, &rc);

      /* Send WM_CTLCOLOR to allow changing the font (the colors are fixed) */
    if (infoPtr->hFont) SelectObject( hDC, infoPtr->hFont );
    BUTTON_SEND_CTLCOLOR( wndPtr, hDC );
    hOldPen = (HPEN16)SelectObject(hDC, sysColorObjects.hpenWindowFrame);
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
    InflateRect16( &rc, -1, -1 );

    if ((wndPtr->dwStyle & 0x000f) == BS_DEFPUSHBUTTON)
    {
        Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
        InflateRect16( &rc, -1, -1 );
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
    if (wndPtr->text && wndPtr->text[0])
    {
     LOGBRUSH lb;
     GetObject(sysColorObjects.hbrushBtnFace,sizeof(LOGBRUSH),(LPSTR)&lb);
     if (wndPtr->dwStyle & WS_DISABLED &&
         GetSysColor(COLOR_GRAYTEXT)==lb.lbColor)
         /* don't write gray text on gray bkg */
         PB_PaintGrayOnGray(hDC,infoPtr->hFont,&rc,wndPtr->text);
     else
     {
        SetTextColor( hDC, (wndPtr->dwStyle & WS_DISABLED) ?
                     GetSysColor(COLOR_GRAYTEXT) : GetSysColor(COLOR_BTNTEXT));
        DrawText16( hDC, wndPtr->text, -1, &rc,
                    DT_SINGLELINE | DT_CENTER | DT_VCENTER );
        /* do we have the focus? */
        if (infoPtr->state & BUTTON_HASFOCUS)
        {
            short xdelta, ydelta;
            dwTextSize = GetTextExtent(hDC,wndPtr->text,strlen(wndPtr->text));
            GetTextMetrics( hDC, &tm );
            xdelta = ((rc.right - rc.left) - LOWORD(dwTextSize) - 1) / 2;
            ydelta = ((rc.bottom - rc.top) - tm.tmHeight - 1) / 2;
            if (xdelta < 0) xdelta = 0;
            if (ydelta < 0) ydelta = 0;
            InflateRect16( &rc, -xdelta, -ydelta );
            DrawFocusRect16( hDC, &rc );
        }
     }   
    }

    SelectObject( hDC, hOldPen );
    SelectObject( hDC, hOldBrush );
}


/**********************************************************************
 *   Push Button sub function                               [internal]
 *   using a raster brush to avoid gray text on gray background
 */

void PB_PaintGrayOnGray(HDC hDC,HFONT hFont,RECT16 *rc,char *text)
{
    static int Pattern[] = {0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55};
    HBITMAP hbm  = CreateBitmap(8, 8, 1, 1, Pattern);
    HDC hdcMem   = CreateCompatibleDC(hDC);
    HBITMAP hbmMem;
    HBRUSH hBr;
    RECT16 rect,rc2;

    rect=*rc;
    DrawText16( hDC, text, -1, &rect, DT_SINGLELINE | DT_CALCRECT);
    rc2=rect;
    rect.left=(rc->right-rect.right)/2;       /* for centering text bitmap */
    rect.top=(rc->bottom-rect.bottom)/2;
    hbmMem = CreateCompatibleBitmap( hDC,rect.right,rect.bottom);
    SelectObject( hdcMem, hbmMem);
    hBr = SelectObject( hdcMem,CreatePatternBrush(hbm));
    DeleteObject( hbm);
    PatBlt( hdcMem,0,0,rect.right,rect.bottom,WHITENESS);
    if (hFont) SelectObject( hdcMem, hFont);
    DrawText16( hdcMem, text, -1, &rc2, DT_SINGLELINE);  
    PatBlt( hdcMem,0,0,rect.right,rect.bottom,0xFA0089);
    DeleteObject( SelectObject( hdcMem,hBr));
    BitBlt( hDC,rect.left,rect.top,rect.right,rect.bottom,hdcMem,0,0,0x990000);
    DeleteDC( hdcMem);
    DeleteObject( hbmMem);
}


/**********************************************************************
 *       Check Box & Radio Button Functions
 */

static void CB_Paint( WND *wndPtr, HDC hDC, WORD action )
{
    RECT16 rc;
    HBRUSH hBrush;
    int textlen, delta, x, y;
    TEXTMETRIC tm;
    SIZE16 size;
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    GetClientRect16(wndPtr->hwndSelf, &rc);

    if (infoPtr->hFont) SelectObject( hDC, infoPtr->hFont );
    hBrush = BUTTON_SEND_CTLCOLOR( wndPtr, hDC );
    if (action == ODA_DRAWENTIRE) FillRect16( hDC, &rc, hBrush );

    GetTextMetrics(hDC, &tm);
    delta = (rc.bottom - rc.top - tm.tmHeight) >> 1;

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

    if (!wndPtr->text) return;
    textlen = strlen( wndPtr->text );

    if (action == ODA_DRAWENTIRE)
    {
        if (wndPtr->dwStyle & WS_DISABLED)
            SetTextColor( hDC, GetSysColor(COLOR_GRAYTEXT) );
        DrawText16( hDC, wndPtr->text, textlen, &rc,
                    DT_SINGLELINE | DT_VCENTER );
    }
    
    if ((action == ODA_FOCUS) ||
        ((action == ODA_DRAWENTIRE) && (infoPtr->state & BUTTON_HASFOCUS)))
    {
        GetTextExtentPoint16( hDC, wndPtr->text, textlen, &size );
        if (delta > 1)
        {
            rc.top += delta - 1;
            rc.bottom -= delta + 1;
        }
        rc.left--;
        rc.right = MIN( rc.left + size.cx + 2, rc.right );
        DrawFocusRect16( hDC, &rc );
    }
}


/**********************************************************************
 *       BUTTON_CheckAutoRadioButton
 *
 * wndPtr is checked, uncheck everything else in group
 */
static void BUTTON_CheckAutoRadioButton( WND *wndPtr )
{
    HWND parent, sibling;
    if (!(wndPtr->dwStyle & WS_CHILD)) return;
    parent = wndPtr->parent->hwndSelf;
    for(sibling = GetNextDlgGroupItem(parent,wndPtr->hwndSelf,FALSE);
        sibling != wndPtr->hwndSelf;
        sibling = GetNextDlgGroupItem(parent,sibling,FALSE))
	    SendMessage32A( sibling, BM_SETCHECK32, BUTTON_UNCHECKED, 0 );
}


/**********************************************************************
 *       Group Box Functions
 */

static void GB_Paint( WND *wndPtr, HDC hDC, WORD action )
{
    RECT16 rc;
    SIZE16 size;
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    if (action != ODA_DRAWENTIRE) return;

    if (infoPtr->hFont) SelectObject( hDC, infoPtr->hFont );
    BUTTON_SEND_CTLCOLOR( wndPtr, hDC );
    SelectObject( hDC, sysColorObjects.hpenWindowFrame );

    GetClientRect16( wndPtr->hwndSelf, &rc);

    MoveTo( hDC, rc.left, rc.top+2 );
    LineTo( hDC, rc.right-1, rc.top+2 );
    LineTo( hDC, rc.right-1, rc.bottom-1 );
    LineTo( hDC, rc.left, rc.bottom-1 );
    LineTo( hDC, rc.left, rc.top+2 );

    if (!wndPtr->text) return;
    GetTextExtentPoint16( hDC, wndPtr->text, strlen(wndPtr->text), &size );
    rc.left  += 10;
    rc.right  = rc.left + size.cx + 1;
    rc.bottom = size.cy;
    if (wndPtr->dwStyle & WS_DISABLED)
        SetTextColor( hDC, GetSysColor(COLOR_GRAYTEXT) );
    DrawText16( hDC, wndPtr->text, -1, &rc, DT_SINGLELINE );
}


/**********************************************************************
 *       User Button Functions
 */

static void UB_Paint( WND *wndPtr, HDC hDC, WORD action )
{
    RECT16 rc;
    HBRUSH hBrush;
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;

    if (action == ODA_SELECT) return;

    GetClientRect16( wndPtr->hwndSelf, &rc);

    if (infoPtr->hFont) SelectObject( hDC, infoPtr->hFont );
    hBrush = BUTTON_SEND_CTLCOLOR( wndPtr, hDC );
    FillRect16( hDC, &rc, hBrush );

    if ((action == ODA_FOCUS) ||
        ((action == ODA_DRAWENTIRE) && (infoPtr->state & BUTTON_HASFOCUS)))
        DrawFocusRect16( hDC, &rc );
}


/**********************************************************************
 *       Ownerdrawn Button Functions
 */

static void OB_Paint( WND *wndPtr, HDC hDC, WORD action )
{
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;
    DRAWITEMSTRUCT32 dis;

    dis.CtlType    = ODT_BUTTON;
    dis.CtlID      = wndPtr->wIDmenu;
    dis.itemID     = 0;
    dis.itemAction = action;
    dis.itemState  = (infoPtr->state & BUTTON_HASFOCUS) ? ODS_FOCUS : 0 |
                     (infoPtr->state & BUTTON_HIGHLIGHTED) ? ODS_SELECTED : 0 |
                     (wndPtr->dwStyle & WS_DISABLED) ? ODS_DISABLED : 0;
    dis.hwndItem   = wndPtr->hwndSelf;
    dis.hDC        = hDC;
    dis.itemData   = 0;
    GetClientRect32( wndPtr->hwndSelf, &dis.rcItem );
    SendMessage32A( GetParent(wndPtr->hwndSelf), WM_DRAWITEM, 1, (LPARAM)&dis);
}
