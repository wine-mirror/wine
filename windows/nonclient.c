/*
 * Non-client area window functions
 *
 * Copyright 1994 Alexandre Julliard
 *
 */

#include "version.h"
#include "win.h"
#include "message.h"
#include "sysmetrics.h"
#include "user.h"
#include "heap.h"
#include "dce.h"
#include "cursoricon.h"
#include "dialog.h"
#include "menu.h"
#include "winpos.h"
#include "hook.h"
#include "scroll.h"
#include "nonclient.h"
#include "queue.h"
#include "selectors.h"
#include "tweak.h"
#include "debug.h"
#include "options.h"
#include "shellapi.h"
#include "cache.h"

static HBITMAP16 hbitmapClose = 0;
static HBITMAP16 hbitmapCloseD = 0;
static HBITMAP16 hbitmapMinimize = 0;
static HBITMAP16 hbitmapMinimizeD = 0;
static HBITMAP16 hbitmapMaximize = 0;
static HBITMAP16 hbitmapMaximizeD = 0;
static HBITMAP16 hbitmapRestore = 0;
static HBITMAP16 hbitmapRestoreD = 0;

#define SC_ABOUTWINE    	(SC_SCREENSAVE+1)
#define SC_PUTMARK     		(SC_SCREENSAVE+2)

  /* Some useful macros */
#define HAS_DLGFRAME(style,exStyle) \
    (((exStyle) & WS_EX_DLGMODALFRAME) || \
     (((style) & WS_DLGFRAME) && !((style) & WS_BORDER)))

#define HAS_THICKFRAME(style) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define HAS_FIXEDFRAME(style,exStyle) \
   (((((exStyle) & WS_EX_DLGMODALFRAME) || \
      ((style) & WS_DLGFRAME)) && ((style) & WS_BORDER)) && \
     !((style) & WS_THICKFRAME))

#define HAS_SIZEFRAME(style) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define HAS_MENU(w)  (!((w)->dwStyle & WS_CHILD) && ((w)->wIDmenu != 0))

#define ON_LEFT_BORDER(hit) \
 (((hit) == HTLEFT) || ((hit) == HTTOPLEFT) || ((hit) == HTBOTTOMLEFT))
#define ON_RIGHT_BORDER(hit) \
 (((hit) == HTRIGHT) || ((hit) == HTTOPRIGHT) || ((hit) == HTBOTTOMRIGHT))
#define ON_TOP_BORDER(hit) \
 (((hit) == HTTOP) || ((hit) == HTTOPLEFT) || ((hit) == HTTOPRIGHT))
#define ON_BOTTOM_BORDER(hit) \
 (((hit) == HTBOTTOM) || ((hit) == HTBOTTOMLEFT) || ((hit) == HTBOTTOMRIGHT))

/***********************************************************************
 *           NC_AdjustRect
 *
 * Compute the size of the window rectangle from the size of the
 * client rectangle.
 */
static void NC_AdjustRect( LPRECT16 rect, DWORD style, BOOL menu,
                           DWORD exStyle )
{
    if (TWEAK_WineLook > WIN31_LOOK)
	ERR(nonclient, "Called in Win95 mode. Aiee! Please report this.\n" );

    if(style & WS_ICONIC) return;
    /* Decide if the window will be managed (see CreateWindowEx) */
    if (!(Options.managed && !(style & WS_CHILD) &&
          ((style & (WS_DLGFRAME | WS_THICKFRAME)) ||
           (exStyle & WS_EX_DLGMODALFRAME))))
    {
        if (HAS_DLGFRAME( style, exStyle ))
            InflateRect16(rect, SYSMETRICS_CXDLGFRAME, SYSMETRICS_CYDLGFRAME );
        else
        {
            if (HAS_THICKFRAME(style))
                InflateRect16( rect, SYSMETRICS_CXFRAME, SYSMETRICS_CYFRAME );
            if (style & WS_BORDER)
                InflateRect16( rect, SYSMETRICS_CXBORDER, SYSMETRICS_CYBORDER);
        }

        if ((style & WS_CAPTION) == WS_CAPTION)
            rect->top -= SYSMETRICS_CYCAPTION - SYSMETRICS_CYBORDER;
    }
    if (menu) rect->top -= SYSMETRICS_CYMENU + SYSMETRICS_CYBORDER;

    if (style & WS_VSCROLL) {
      rect->right  += SYSMETRICS_CXVSCROLL - 1;
      if(!(style & WS_BORDER))
	 rect->right++;
    }

    if (style & WS_HSCROLL) {
      rect->bottom += SYSMETRICS_CYHSCROLL - 1;
      if(!(style & WS_BORDER))
	 rect->bottom++;
    }
}


/******************************************************************************
 * NC_AdjustRectOuter95
 *
 * Computes the size of the "outside" parts of the window based on the
 * parameters of the client area.
 *
 + PARAMS
 *     LPRECT16  rect
 *     DWORD  style
 *     BOOL32  menu
 *     DWORD  exStyle
 *
 * NOTES
 *     "Outer" parts of a window means the whole window frame, caption and
 *     menu bar. It does not include "inner" parts of the frame like client
 *     edge, static edge or scroll bars.
 *
 * Revision history
 *     05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *        Original (NC_AdjustRect95) cut & paste from NC_AdjustRect
 *
 *     20-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *        Split NC_AdjustRect95 into NC_AdjustRectOuter95 and
 *        NC_AdjustRectInner95 and added handling of Win95 styles.
 *
 *****************************************************************************/

static void
NC_AdjustRectOuter95 (LPRECT16 rect, DWORD style, BOOL menu, DWORD exStyle)
{
    if(style & WS_ICONIC) return;

    /* Decide if the window will be managed (see CreateWindowEx) */
    if (!(Options.managed && !(style & WS_CHILD) &&
          ((style & (WS_DLGFRAME | WS_THICKFRAME)) ||
           (exStyle & WS_EX_DLGMODALFRAME))))
    {
        if (HAS_FIXEDFRAME( style, exStyle ))
            InflateRect16(rect, SYSMETRICS_CXDLGFRAME, SYSMETRICS_CYDLGFRAME );
        else
        {
            if (HAS_SIZEFRAME(style))
                InflateRect16( rect, SYSMETRICS_CXFRAME, SYSMETRICS_CYFRAME );
#if 0
            if (style & WS_BORDER)
                InflateRect16( rect, SYSMETRICS_CXBORDER, SYSMETRICS_CYBORDER);
#endif
        }

        if ((style & WS_CAPTION) == WS_CAPTION)
        {
	    if (exStyle & WS_EX_TOOLWINDOW)
		rect->top -= SYSMETRICS_CYSMCAPTION;
	    else
		rect->top -= SYSMETRICS_CYCAPTION;
        }
    }

    if (menu)
	rect->top -= sysMetrics[SM_CYMENU];
}


/******************************************************************************
 * NC_AdjustRectInner95
 *
 * Computes the size of the "inside" part of the window based on the
 * parameters of the client area.
 *
 + PARAMS
 *     LPRECT16 rect
 *     DWORD    style
 *     DWORD    exStyle
 *
 * NOTES
 *     "Inner" part of a window means the window frame inside of the flat
 *     window frame. It includes the client edge, the static edge and the
 *     scroll bars.
 *
 * Revision history
 *     05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *        Original (NC_AdjustRect95) cut & paste from NC_AdjustRect
 *
 *     20-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *        Split NC_AdjustRect95 into NC_AdjustRectOuter95 and
 *        NC_AdjustRectInner95 and added handling of Win95 styles.
 *
 *****************************************************************************/

static void
NC_AdjustRectInner95 (LPRECT16 rect, DWORD style, DWORD exStyle)
{
    if(style & WS_ICONIC) return;

    if (exStyle & WS_EX_CLIENTEDGE)
	InflateRect16 (rect, sysMetrics[SM_CXEDGE], sysMetrics[SM_CYEDGE]);

    if (exStyle & WS_EX_STATICEDGE)
	InflateRect16 (rect, sysMetrics[SM_CXBORDER], sysMetrics[SM_CYBORDER]);

    if (style & WS_VSCROLL) rect->right  += SYSMETRICS_CXVSCROLL;
    if (style & WS_HSCROLL) rect->bottom += SYSMETRICS_CYHSCROLL;
}


/***********************************************************************
 * DrawCaption16 [USER.660] Draws a caption bar
 *
 * PARAMS
 *     hwnd   [I]
 *     hdc    [I]
 *     lpRect [I]
 *     uFlags [I]
 *
 * RETURNS
 *     Success:
 *     Failure:
 */

BOOL16 WINAPI
DrawCaption16 (HWND16 hwnd, HDC16 hdc, const RECT16 *rect, UINT16 uFlags)
{
    RECT rect32;

    if (rect)
	CONV_RECT16TO32 (rect, &rect32);

    return (BOOL16)DrawCaptionTempA (hwnd, hdc, rect ? &rect32 : NULL,
				       0, 0, NULL, uFlags & 0x1F);
}


/***********************************************************************
 * DrawCaption32 [USER32.154] Draws a caption bar
 *
 * PARAMS
 *     hwnd   [I]
 *     hdc    [I]
 *     lpRect [I]
 *     uFlags [I]
 *
 * RETURNS
 *     Success:
 *     Failure:
 */

BOOL WINAPI
DrawCaption (HWND hwnd, HDC hdc, const RECT *lpRect, UINT uFlags)
{
    return DrawCaptionTempA (hwnd, hdc, lpRect, 0, 0, NULL, uFlags & 0x1F);
}


/***********************************************************************
 * DrawCaptionTemp16 [USER.657]
 *
 * PARAMS
 *
 * RETURNS
 *     Success:
 *     Failure:
 */

BOOL16 WINAPI
DrawCaptionTemp16 (HWND16 hwnd, HDC16 hdc, const RECT16 *rect, HFONT16 hFont,
		   HICON16 hIcon, LPCSTR str, UINT16 uFlags)
{
    RECT rect32;

    if (rect)
	CONV_RECT16TO32(rect,&rect32);

    return (BOOL16)DrawCaptionTempA (hwnd, hdc, rect?&rect32:NULL, hFont,
				       hIcon, str, uFlags & 0x1F);
}


/***********************************************************************
 * DrawCaptionTemp32A [USER32.599]
 *
 * PARAMS
 *
 * RETURNS
 *     Success:
 *     Failure:
 */

BOOL WINAPI
DrawCaptionTempA (HWND hwnd, HDC hdc, const RECT *rect, HFONT hFont,
		    HICON hIcon, LPCSTR str, UINT uFlags)
{
    RECT   rc = *rect;

    TRACE (nonclient, "(%08x,%08x,%p,%08x,%08x,\"%s\",%08x)\n",
	   hwnd, hdc, rect, hFont, hIcon, str, uFlags);

    /* drawing background */
    if (uFlags & DC_INBUTTON) {
	FillRect (hdc, &rc, GetSysColorBrush (COLOR_3DFACE));

	if (uFlags & DC_ACTIVE) {
	    HBRUSH hbr = SelectObject (hdc, CACHE_GetPattern55AABrush ());
	    PatBlt (hdc, rc.left, rc.top,
		      rc.right-rc.left, rc.bottom-rc.top, 0xFA0089);
	    SelectObject (hdc, hbr);
	}
    }
    else {
	FillRect (hdc, &rc, GetSysColorBrush ((uFlags & DC_ACTIVE) ?
		    COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION));
    }


    /* drawing icon */
    if ((uFlags & DC_ICON) && !(uFlags & DC_SMALLCAP)) {
	POINT pt;

	pt.x = rc.left + 2;
	pt.y = (rc.bottom + rc.top - sysMetrics[SM_CYSMICON]) / 2;

	if (hIcon) {
	    DrawIconEx (hdc, pt.x, pt.y, hIcon, sysMetrics[SM_CXSMICON],
			  sysMetrics[SM_CYSMICON], 0, 0, DI_NORMAL);
	}
	else {
	    WND *wndPtr = WIN_FindWndPtr(hwnd);
	    HICON hAppIcon = 0;

	    if (wndPtr->class->hIconSm)
		hAppIcon = wndPtr->class->hIconSm;
	    else if (wndPtr->class->hIcon)
		hAppIcon = wndPtr->class->hIcon;

	    DrawIconEx (hdc, pt.x, pt.y, hAppIcon, sysMetrics[SM_CXSMICON],
			  sysMetrics[SM_CYSMICON], 0, 0, DI_NORMAL);
            WIN_ReleaseWndPtr(wndPtr);
	}

	rc.left += (rc.bottom - rc.top);
    }

    /* drawing text */
    if (uFlags & DC_TEXT) {
	HFONT hOldFont;

	if (uFlags & DC_INBUTTON)
	    SetTextColor (hdc, GetSysColor (COLOR_BTNTEXT));
	else if (uFlags & DC_ACTIVE)
	    SetTextColor (hdc, GetSysColor (COLOR_CAPTIONTEXT));
	else
	    SetTextColor (hdc, GetSysColor (COLOR_INACTIVECAPTIONTEXT));

	SetBkMode (hdc, TRANSPARENT);

	if (hFont)
	    hOldFont = SelectObject (hdc, hFont);
	else {
	    NONCLIENTMETRICSA nclm;
	    HFONT hNewFont;
	    nclm.cbSize = sizeof(NONCLIENTMETRICSA);
	    SystemParametersInfoA (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
	    hNewFont = CreateFontIndirectA ((uFlags & DC_SMALLCAP) ?
		&nclm.lfSmCaptionFont : &nclm.lfCaptionFont);
	    hOldFont = SelectObject (hdc, hNewFont);
	}

	if (str)
	    DrawTextA (hdc, str, -1, &rc,
			 DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_LEFT);
	else {
	    CHAR szText[128];
	    INT nLen;
	    nLen = GetWindowTextA (hwnd, szText, 128);
	    DrawTextA (hdc, szText, nLen, &rc,
			 DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_LEFT);
	}

	if (hFont)
	    SelectObject (hdc, hOldFont);
	else
	    DeleteObject (SelectObject (hdc, hOldFont));
    }

    /* drawing focus ??? */
    if (uFlags & 0x2000)
	FIXME (nonclient, "undocumented flag (0x2000)!\n");

    return 0;
}


/***********************************************************************
 * DrawCaptionTemp32W [USER32.602]
 *
 * PARAMS
 *
 * RETURNS
 *     Success:
 *     Failure:
 */

BOOL WINAPI
DrawCaptionTempW (HWND hwnd, HDC hdc, const RECT *rect, HFONT hFont,
		    HICON hIcon, LPCWSTR str, UINT uFlags)
{
    LPSTR p = HEAP_strdupWtoA (GetProcessHeap (), 0, str);
    BOOL res = DrawCaptionTempA (hwnd, hdc, rect, hFont, hIcon, p, uFlags);
    HeapFree (GetProcessHeap (), 0, p);
    return res;
}


/***********************************************************************
 *           AdjustWindowRect16    (USER.102)
 */
BOOL16 WINAPI AdjustWindowRect16( LPRECT16 rect, DWORD style, BOOL16 menu )
{
    return AdjustWindowRectEx16( rect, style, menu, 0 );
}


/***********************************************************************
 *           AdjustWindowRect32    (USER32.2)
 */
BOOL WINAPI AdjustWindowRect( LPRECT rect, DWORD style, BOOL menu )
{
    return AdjustWindowRectEx( rect, style, menu, 0 );
}


/***********************************************************************
 *           AdjustWindowRectEx16    (USER.454)
 */
BOOL16 WINAPI AdjustWindowRectEx16( LPRECT16 rect, DWORD style,
                                    BOOL16 menu, DWORD exStyle )
{
      /* Correct the window style */

    if (!(style & (WS_POPUP | WS_CHILD)))  /* Overlapped window */
	style |= WS_CAPTION;
    style &= (WS_DLGFRAME | WS_BORDER | WS_THICKFRAME | WS_CHILD);
    exStyle &= (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE |
		WS_EX_STATICEDGE | WS_EX_TOOLWINDOW);
    if (exStyle & WS_EX_DLGMODALFRAME) style &= ~WS_THICKFRAME;

    TRACE(nonclient, "(%d,%d)-(%d,%d) %08lx %d %08lx\n",
                      rect->left, rect->top, rect->right, rect->bottom,
                      style, menu, exStyle );

    if (TWEAK_WineLook == WIN31_LOOK)
	NC_AdjustRect( rect, style, menu, exStyle );
    else {
	NC_AdjustRectOuter95( rect, style, menu, exStyle );
	NC_AdjustRectInner95( rect, style, exStyle );
    }

    return TRUE;
}


/***********************************************************************
 *           AdjustWindowRectEx32    (USER32.3)
 */
BOOL WINAPI AdjustWindowRectEx( LPRECT rect, DWORD style,
                                    BOOL menu, DWORD exStyle )
{
    RECT16 rect16;
    BOOL ret;

    CONV_RECT32TO16( rect, &rect16 );
    ret = AdjustWindowRectEx16( &rect16, style, (BOOL16)menu, exStyle );
    CONV_RECT16TO32( &rect16, rect );
    return ret;
}


/***********************************************************************
 *           NC_HandleNCCalcSize
 *
 * Handle a WM_NCCALCSIZE message. Called from DefWindowProc().
 */
LONG NC_HandleNCCalcSize( WND *pWnd, RECT *winRect )
{
    RECT16 tmpRect = { 0, 0, 0, 0 };
    LONG result = 0;

    if (pWnd->class->style & CS_VREDRAW) result |= WVR_VREDRAW;
    if (pWnd->class->style & CS_HREDRAW) result |= WVR_HREDRAW;

    if( !( pWnd->dwStyle & WS_MINIMIZE ) ) {
	if (TWEAK_WineLook == WIN31_LOOK)
	    NC_AdjustRect( &tmpRect, pWnd->dwStyle, FALSE, pWnd->dwExStyle );
	else
	    NC_AdjustRectOuter95( &tmpRect, pWnd->dwStyle, FALSE, pWnd->dwExStyle );

	winRect->left   -= tmpRect.left;
	winRect->top    -= tmpRect.top;
	winRect->right  -= tmpRect.right;
	winRect->bottom -= tmpRect.bottom;

	if (HAS_MENU(pWnd)) {
	    TRACE(nonclient, "Calling "
			       "GetMenuBarHeight with HWND 0x%x, width %d, "
			       "at (%d, %d).\n", pWnd->hwndSelf,
			       winRect->right - winRect->left,
			       -tmpRect.left, -tmpRect.top );

	    winRect->top +=
		MENU_GetMenuBarHeight( pWnd->hwndSelf,
				       winRect->right - winRect->left,
				       -tmpRect.left, -tmpRect.top ) + 1;
	}

	if (TWEAK_WineLook > WIN31_LOOK) {
	    SetRect16 (&tmpRect, 0, 0, 0, 0);
	    NC_AdjustRectInner95 (&tmpRect, pWnd->dwStyle, pWnd->dwExStyle);
	    winRect->left   -= tmpRect.left;
	    winRect->top    -= tmpRect.top;
	    winRect->right  -= tmpRect.right;
	    winRect->bottom -= tmpRect.bottom;
	}
    }
    return result;
}


/***********************************************************************
 *           NC_GetInsideRect
 *
 * Get the 'inside' rectangle of a window, i.e. the whole window rectangle
 * but without the borders (if any).
 * The rectangle is in window coordinates (for drawing with GetWindowDC()).
 */
static void NC_GetInsideRect( HWND hwnd, RECT *rect )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    rect->top    = rect->left = 0;
    rect->right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    rect->bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

    if ((wndPtr->dwStyle & WS_ICONIC) || (wndPtr->flags & WIN_MANAGED)) goto END;

    /* Remove frame from rectangle */
    if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
    {
	InflateRect( rect, -SYSMETRICS_CXDLGFRAME, -SYSMETRICS_CYDLGFRAME);
	if (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME)
            InflateRect( rect, -1, 0 );
    }
    else
    {
	if (HAS_THICKFRAME( wndPtr->dwStyle ))
	    InflateRect( rect, -SYSMETRICS_CXFRAME, -SYSMETRICS_CYFRAME );
	if (wndPtr->dwStyle & WS_BORDER)
	    InflateRect( rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER );
    }
END:
    WIN_ReleaseWndPtr(wndPtr);
    return;
}


/***********************************************************************
 *           NC_GetInsideRect95
 *
 * Get the 'inside' rectangle of a window, i.e. the whole window rectangle
 * but without the borders (if any).
 * The rectangle is in window coordinates (for drawing with GetWindowDC()).
 */

static void
NC_GetInsideRect95 (HWND hwnd, RECT *rect)
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    rect->top    = rect->left = 0;
    rect->right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    rect->bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

    if ((wndPtr->dwStyle & WS_ICONIC) || (wndPtr->flags & WIN_MANAGED)) goto END;

    /* Remove frame from rectangle */
    if (HAS_FIXEDFRAME (wndPtr->dwStyle, wndPtr->dwExStyle ))
    {
	InflateRect( rect, -SYSMETRICS_CXFIXEDFRAME, -SYSMETRICS_CYFIXEDFRAME);
    }
    else if (HAS_SIZEFRAME (wndPtr->dwStyle))
    {
	InflateRect( rect, -SYSMETRICS_CXSIZEFRAME, -SYSMETRICS_CYSIZEFRAME );

/*	if (wndPtr->dwStyle & WS_BORDER)
          InflateRect32( rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER );*/
    }

    if (wndPtr->dwStyle & WS_CHILD) {
	if (wndPtr->dwExStyle & WS_EX_CLIENTEDGE)
	    InflateRect (rect, -SYSMETRICS_CXEDGE, -SYSMETRICS_CYEDGE);

	if (wndPtr->dwExStyle & WS_EX_STATICEDGE)
	    InflateRect (rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER);
    }
END:
    WIN_ReleaseWndPtr(wndPtr);
    return;
}


/***********************************************************************
 * NC_DoNCHitTest
 *
 * Handle a WM_NCHITTEST message. Called from NC_HandleNcHitTest().
 */

LONG NC_DoNCHitTest (WND *wndPtr, POINT16 pt )
{
    RECT16 rect;

    TRACE(nonclient, "hwnd=%04x pt=%d,%d\n",
		      wndPtr->hwndSelf, pt.x, pt.y );

    GetWindowRect16 (wndPtr->hwndSelf, &rect );
    if (!PtInRect16( &rect, pt )) return HTNOWHERE;

    if (wndPtr->dwStyle & WS_MINIMIZE) return HTCAPTION;

    if (!(wndPtr->flags & WIN_MANAGED))
    {
        /* Check borders */
        if (HAS_THICKFRAME( wndPtr->dwStyle ))
        {
            InflateRect16( &rect, -SYSMETRICS_CXFRAME, -SYSMETRICS_CYFRAME );
            if (wndPtr->dwStyle & WS_BORDER)
                InflateRect16(&rect,-SYSMETRICS_CXBORDER,-SYSMETRICS_CYBORDER);
            if (!PtInRect16( &rect, pt ))
            {
                /* Check top sizing border */
                if (pt.y < rect.top)
                {
                    if (pt.x < rect.left+SYSMETRICS_CXSIZE) return HTTOPLEFT;
                    if (pt.x >= rect.right-SYSMETRICS_CXSIZE) return HTTOPRIGHT;
                    return HTTOP;
                }
                /* Check bottom sizing border */
                if (pt.y >= rect.bottom)
                {
                    if (pt.x < rect.left+SYSMETRICS_CXSIZE) return HTBOTTOMLEFT;
                    if (pt.x >= rect.right-SYSMETRICS_CXSIZE) return HTBOTTOMRIGHT;
                    return HTBOTTOM;
                }
                /* Check left sizing border */
                if (pt.x < rect.left)
                {
                    if (pt.y < rect.top+SYSMETRICS_CYSIZE) return HTTOPLEFT;
                    if (pt.y >= rect.bottom-SYSMETRICS_CYSIZE) return HTBOTTOMLEFT;
                    return HTLEFT;
                }
                /* Check right sizing border */
                if (pt.x >= rect.right)
                {
                    if (pt.y < rect.top+SYSMETRICS_CYSIZE) return HTTOPRIGHT;
                    if (pt.y >= rect.bottom-SYSMETRICS_CYSIZE) return HTBOTTOMRIGHT;
                    return HTRIGHT;
                }
            }
        }
        else  /* No thick frame */
        {
            if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
                InflateRect16(&rect, -SYSMETRICS_CXDLGFRAME, -SYSMETRICS_CYDLGFRAME);
            else if (wndPtr->dwStyle & WS_BORDER)
                InflateRect16(&rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER);
            if (!PtInRect16( &rect, pt )) return HTBORDER;
        }

        /* Check caption */

        if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
        {
            rect.top += sysMetrics[SM_CYCAPTION] - 1;
            if (!PtInRect16( &rect, pt ))
            {
                /* Check system menu */
                if (wndPtr->dwStyle & WS_SYSMENU)
                    rect.left += SYSMETRICS_CXSIZE;
                if (pt.x <= rect.left) return HTSYSMENU;
                /* Check maximize box */
                if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
                    rect.right -= SYSMETRICS_CXSIZE + 1;
                if (pt.x >= rect.right) return HTMAXBUTTON;
                /* Check minimize box */
                if (wndPtr->dwStyle & WS_MINIMIZEBOX)
                    rect.right -= SYSMETRICS_CXSIZE + 1;
                if (pt.x >= rect.right) return HTMINBUTTON;
                return HTCAPTION;
            }
        }
    }

      /* Check client area */

    ScreenToClient16( wndPtr->hwndSelf, &pt );
    GetClientRect16( wndPtr->hwndSelf, &rect );
    if (PtInRect16( &rect, pt )) return HTCLIENT;

      /* Check vertical scroll bar */

    if (wndPtr->dwStyle & WS_VSCROLL)
    {
	rect.right += SYSMETRICS_CXVSCROLL;
	if (PtInRect16( &rect, pt )) return HTVSCROLL;
    }

      /* Check horizontal scroll bar */

    if (wndPtr->dwStyle & WS_HSCROLL)
    {
	rect.bottom += SYSMETRICS_CYHSCROLL;
	if (PtInRect16( &rect, pt ))
	{
	      /* Check size box */
	    if ((wndPtr->dwStyle & WS_VSCROLL) &&
		(pt.x >= rect.right - SYSMETRICS_CXVSCROLL))
		return HTSIZE;
	    return HTHSCROLL;
	}
    }

      /* Check menu bar */

    if (HAS_MENU(wndPtr))
    {
	if ((pt.y < 0) && (pt.x >= 0) && (pt.x < rect.right))
	    return HTMENU;
    }

      /* Should never get here */
    return HTERROR;
}


/***********************************************************************
 * NC_DoNCHitTest95
 *
 * Handle a WM_NCHITTEST message. Called from NC_HandleNCHitTest().
 *
 * FIXME:  Just a modified copy of the Win 3.1 version.
 */

LONG
NC_DoNCHitTest95 (WND *wndPtr, POINT16 pt )
{
    RECT16 rect;

    TRACE(nonclient, "hwnd=%04x pt=%d,%d\n",
		      wndPtr->hwndSelf, pt.x, pt.y );

    GetWindowRect16 (wndPtr->hwndSelf, &rect );
    if (!PtInRect16( &rect, pt )) return HTNOWHERE;

    if (wndPtr->dwStyle & WS_MINIMIZE) return HTCAPTION;

    if (!(wndPtr->flags & WIN_MANAGED))
    {
        /* Check borders */
        if (HAS_SIZEFRAME( wndPtr->dwStyle ))
        {
            InflateRect16( &rect, -SYSMETRICS_CXFRAME, -SYSMETRICS_CYFRAME );
/*            if (wndPtr->dwStyle & WS_BORDER) */
/*                InflateRect16(&rect,-SYSMETRICS_CXBORDER,-SYSMETRICS_CYBORDER); */
            if (!PtInRect16( &rect, pt ))
            {
                /* Check top sizing border */
                if (pt.y < rect.top)
                {
                    if (pt.x < rect.left+SYSMETRICS_CXSIZE) return HTTOPLEFT;
                    if (pt.x >= rect.right-SYSMETRICS_CXSIZE) return HTTOPRIGHT;
                    return HTTOP;
                }
                /* Check bottom sizing border */
                if (pt.y >= rect.bottom)
                {
                    if (pt.x < rect.left+SYSMETRICS_CXSIZE) return HTBOTTOMLEFT;
                    if (pt.x >= rect.right-SYSMETRICS_CXSIZE) return HTBOTTOMRIGHT;
                    return HTBOTTOM;
                }
                /* Check left sizing border */
                if (pt.x < rect.left)
                {
                    if (pt.y < rect.top+SYSMETRICS_CYSIZE) return HTTOPLEFT;
                    if (pt.y >= rect.bottom-SYSMETRICS_CYSIZE) return HTBOTTOMLEFT;
                    return HTLEFT;
                }
                /* Check right sizing border */
                if (pt.x >= rect.right)
                {
                    if (pt.y < rect.top+SYSMETRICS_CYSIZE) return HTTOPRIGHT;
                    if (pt.y >= rect.bottom-SYSMETRICS_CYSIZE) return HTBOTTOMRIGHT;
                    return HTRIGHT;
                }
            }
        }
        else  /* No thick frame */
        {
            if (HAS_FIXEDFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
                InflateRect16(&rect, -SYSMETRICS_CXDLGFRAME, -SYSMETRICS_CYDLGFRAME);
/*            else if (wndPtr->dwStyle & WS_BORDER) */
/*                InflateRect16(&rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER); */
            if (!PtInRect16( &rect, pt )) return HTBORDER;
        }

        /* Check caption */

        if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
        {
	    if (wndPtr->dwExStyle & WS_EX_TOOLWINDOW)
	        rect.top += sysMetrics[SM_CYSMCAPTION] - 1;
	    else
	        rect.top += sysMetrics[SM_CYCAPTION] - 1;
            if (!PtInRect16( &rect, pt ))
            {
                /* Check system menu */
                if ((wndPtr->dwStyle & WS_SYSMENU) &&
		    ((wndPtr->class->hIconSm) || (wndPtr->class->hIcon)))
                    rect.left += sysMetrics[SM_CYCAPTION] - 1;
                if (pt.x < rect.left) return HTSYSMENU;

                /* Check close button */
                if (wndPtr->dwStyle & WS_SYSMENU)
                    rect.right -= sysMetrics[SM_CYCAPTION] - 1;
                if (pt.x > rect.right) return HTCLOSE;

                /* Check maximize box */
                if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
                    rect.right -= SYSMETRICS_CXSIZE + 1;
                if (pt.x > rect.right) return HTMAXBUTTON;

                /* Check minimize box */
                if (wndPtr->dwStyle & WS_MINIMIZEBOX)
                    rect.right -= SYSMETRICS_CXSIZE + 1;
                if (pt.x > rect.right) return HTMINBUTTON;
                return HTCAPTION;
            }
        }
    }

      /* Check client area */

    ScreenToClient16( wndPtr->hwndSelf, &pt );
    GetClientRect16( wndPtr->hwndSelf, &rect );
    if (PtInRect16( &rect, pt )) return HTCLIENT;

      /* Check vertical scroll bar */

    if (wndPtr->dwStyle & WS_VSCROLL)
    {
	rect.right += SYSMETRICS_CXVSCROLL;
	if (PtInRect16( &rect, pt )) return HTVSCROLL;
    }

      /* Check horizontal scroll bar */

    if (wndPtr->dwStyle & WS_HSCROLL)
    {
	rect.bottom += SYSMETRICS_CYHSCROLL;
	if (PtInRect16( &rect, pt ))
	{
	      /* Check size box */
	    if ((wndPtr->dwStyle & WS_VSCROLL) &&
		(pt.x >= rect.right - SYSMETRICS_CXVSCROLL))
		return HTSIZE;
	    return HTHSCROLL;
	}
    }

      /* Check menu bar */

    if (HAS_MENU(wndPtr))
    {
	if ((pt.y < 0) && (pt.x >= 0) && (pt.x < rect.right))
	    return HTMENU;
    }

      /* Should never get here */
    return HTERROR;
}


/***********************************************************************
 * NC_HandleNCHitTest
 *
 * Handle a WM_NCHITTEST message. Called from DefWindowProc().
 */
LONG
NC_HandleNCHitTest (HWND hwnd , POINT16 pt)
{
    LONG retvalue;
    WND *wndPtr = WIN_FindWndPtr (hwnd);

    if (!wndPtr)
	return HTERROR;

    if (TWEAK_WineLook == WIN31_LOOK)
        retvalue = NC_DoNCHitTest (wndPtr, pt);
    else
        retvalue = NC_DoNCHitTest95 (wndPtr, pt);
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/***********************************************************************
 *           NC_DrawSysButton
 */
void NC_DrawSysButton( HWND hwnd, HDC hdc, BOOL down )
{
    RECT rect;
    HDC hdcMem;
    HBITMAP hbitmap;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if( !(wndPtr->flags & WIN_MANAGED) )
    {
      NC_GetInsideRect( hwnd, &rect );
      hdcMem = CreateCompatibleDC( hdc );
      hbitmap = SelectObject( hdcMem, hbitmapClose );
      BitBlt(hdc, rect.left, rect.top, SYSMETRICS_CXSIZE, SYSMETRICS_CYSIZE,
               hdcMem, (wndPtr->dwStyle & WS_CHILD) ? SYSMETRICS_CXSIZE : 0, 0,
               down ? NOTSRCCOPY : SRCCOPY );
      SelectObject( hdcMem, hbitmap );
      DeleteDC( hdcMem );
    }
    WIN_ReleaseWndPtr(wndPtr);
}


/***********************************************************************
 *           NC_DrawMaxButton
 */
static void NC_DrawMaxButton( HWND hwnd, HDC16 hdc, BOOL down )
{
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    HDC hdcMem;

    if( !(wndPtr->flags & WIN_MANAGED) )
    {
      NC_GetInsideRect( hwnd, &rect );
      hdcMem = CreateCompatibleDC( hdc );
      SelectObject( hdcMem,  (IsZoomed(hwnd) 
			     ? (down ? hbitmapRestoreD : hbitmapRestore)
			     : (down ? hbitmapMaximizeD : hbitmapMaximize)) );
      BitBlt( hdc, rect.right - SYSMETRICS_CXSIZE - 1, rect.top,
		SYSMETRICS_CXSIZE + 1, SYSMETRICS_CYSIZE, hdcMem, 0, 0,
		SRCCOPY );
      DeleteDC( hdcMem );
    }
    WIN_ReleaseWndPtr(wndPtr);

}


/***********************************************************************
 *           NC_DrawMinButton
 */
static void NC_DrawMinButton( HWND hwnd, HDC16 hdc, BOOL down )
{
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    HDC hdcMem;

    if( !(wndPtr->flags & WIN_MANAGED) )
    {
      NC_GetInsideRect( hwnd, &rect );
      hdcMem = CreateCompatibleDC( hdc );
      SelectObject( hdcMem, (down ? hbitmapMinimizeD : hbitmapMinimize) );
      if (wndPtr->dwStyle & WS_MAXIMIZEBOX) rect.right -= SYSMETRICS_CXSIZE+1;
      BitBlt( hdc, rect.right - SYSMETRICS_CXSIZE - 1, rect.top,
		SYSMETRICS_CXSIZE + 1, SYSMETRICS_CYSIZE, hdcMem, 0, 0,
		SRCCOPY );
      DeleteDC( hdcMem );
    }
    WIN_ReleaseWndPtr(wndPtr);
}


/******************************************************************************
 *
 *   void  NC_DrawSysButton95(
 *      HWND32  hwnd,
 *      HDC32  hdc,
 *      BOOL32  down )
 *
 *   Draws the Win95 system icon.
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation from NC_DrawSysButton source.
 *        11-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *             Fixed most bugs.
 *
 *****************************************************************************/

BOOL
NC_DrawSysButton95 (HWND hwnd, HDC hdc, BOOL down)
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if( !(wndPtr->flags & WIN_MANAGED) )
    {
	HICON  hIcon = 0;
	RECT rect;

	NC_GetInsideRect95( hwnd, &rect );

	if (wndPtr->class->hIconSm)
	    hIcon = wndPtr->class->hIconSm;
	else if (wndPtr->class->hIcon)
	    hIcon = wndPtr->class->hIcon;

	if (hIcon)
	    DrawIconEx (hdc, rect.left + 2, rect.top + 2, hIcon,
			  sysMetrics[SM_CXSMICON],
			  sysMetrics[SM_CYSMICON],
			  0, 0, DI_NORMAL);

        WIN_ReleaseWndPtr(wndPtr);
	return (hIcon != 0);
    }
    WIN_ReleaseWndPtr(wndPtr);
    return FALSE;
}


/******************************************************************************
 *
 *   void  NC_DrawCloseButton95(
 *      HWND32  hwnd,
 *      HDC32  hdc,
 *      BOOL32  down )
 *
 *   Draws the Win95 close button.
 *
 *   Revision history
 *        11-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *             Original implementation from NC_DrawSysButton95 source.
 *
 *****************************************************************************/

void NC_DrawCloseButton95 (HWND hwnd, HDC hdc, BOOL down)
{
    RECT rect;
    HDC hdcMem;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if( !(wndPtr->flags & WIN_MANAGED) )
    {
	BITMAP bmp;
	HBITMAP hBmp, hOldBmp;

	NC_GetInsideRect95( hwnd, &rect );

	hdcMem = CreateCompatibleDC( hdc );
	hBmp = down ? hbitmapCloseD : hbitmapClose;
	hOldBmp = SelectObject (hdcMem, hBmp);
	GetObjectA (hBmp, sizeof(BITMAP), &bmp);
	BitBlt (hdc, rect.right - (sysMetrics[SM_CYCAPTION] + 1 + bmp.bmWidth) / 2,
		  rect.top + (sysMetrics[SM_CYCAPTION] - 1 - bmp.bmHeight) / 2,
		  bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, SRCCOPY);

	SelectObject (hdcMem, hOldBmp);
	DeleteDC (hdcMem);
    }
    WIN_ReleaseWndPtr(wndPtr);
}

/******************************************************************************
 *
 *   NC_DrawMaxButton95(
 *      HWND32  hwnd,
 *      HDC16  hdc,
 *      BOOL32  down )
 *
 *   Draws the maximize button for Win95 style windows.
 *
 *   Bugs
 *        Many.  Spacing might still be incorrect.  Need to fit a close
 *        button between the max button and the edge.
 *        Should scale the image with the title bar.  And more...
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

static void  NC_DrawMaxButton95(HWND hwnd,HDC16 hdc,BOOL down )
{
    RECT rect;
    HDC hdcMem;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if( !(wndPtr->flags & WIN_MANAGED))
    {
        BITMAP  bmp;
        HBITMAP  hBmp,hOldBmp;

	NC_GetInsideRect95( hwnd, &rect );
	hdcMem = CreateCompatibleDC( hdc );
       hBmp = IsZoomed(hwnd) ?
				(down ? hbitmapRestoreD : hbitmapRestore ) :
                               (down ? hbitmapMaximizeD: hbitmapMaximize);
	hOldBmp=SelectObject( hdcMem, hBmp );
	GetObjectA (hBmp, sizeof(BITMAP), &bmp);

	if (wndPtr->dwStyle & WS_SYSMENU)
	    rect.right -= sysMetrics[SM_CYCAPTION] + 1;
	
	BitBlt( hdc, rect.right - (sysMetrics[SM_CXSIZE] + bmp.bmWidth) / 2,
		  rect.top + (sysMetrics[SM_CYCAPTION] - 1 - bmp.bmHeight) / 2,
		  bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, SRCCOPY );
	SelectObject (hdcMem, hOldBmp);
	DeleteDC( hdcMem );
    }
    WIN_ReleaseWndPtr(wndPtr);
}

/******************************************************************************
 *
 *   NC_DrawMinButton95(
 *      HWND32  hwnd,
 *      HDC16  hdc,
 *      BOOL32  down )
 *
 *   Draws the minimize button for Win95 style windows.
 *
 *   Bugs
 *        Many.  Spacing is still incorrect.  Should scale the image with the
 *        title bar.  And more...
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

static void  NC_DrawMinButton95(HWND hwnd,HDC16 hdc,BOOL down )
{
    RECT rect;
    HDC hdcMem;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if( !(wndPtr->flags & WIN_MANAGED))
        
    {
       BITMAP  bmp;
       HBITMAP  hBmp,hOldBmp;
	
	NC_GetInsideRect95( hwnd, &rect );

       hdcMem = CreateCompatibleDC( hdc );
       hBmp = down ? hbitmapMinimizeD : hbitmapMinimize;
       hOldBmp= SelectObject( hdcMem, hBmp );
	GetObjectA (hBmp, sizeof(BITMAP), &bmp);

	if (wndPtr->dwStyle & WS_SYSMENU)
	    rect.right -= sysMetrics[SM_CYCAPTION] + 1;

	if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
	    rect.right += -1 - (sysMetrics[SM_CXSIZE] + bmp.bmWidth) / 2;

	BitBlt( hdc, rect.right - (sysMetrics[SM_CXSIZE] + bmp.bmWidth) / 2,
		  rect.top + (sysMetrics[SM_CYCAPTION] - 1 - bmp.bmHeight) / 2,
		  bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, SRCCOPY );

       SelectObject (hdcMem, hOldBmp);
	DeleteDC( hdcMem );
    }
    WIN_ReleaseWndPtr(wndPtr);
}

/***********************************************************************
 *           NC_DrawFrame
 *
 * Draw a window frame inside the given rectangle, and update the rectangle.
 * The correct pen for the frame must be selected in the DC.
 */
static void NC_DrawFrame( HDC hdc, RECT *rect, BOOL dlgFrame,
                          BOOL active )
{
    INT width, height;

    if (TWEAK_WineLook != WIN31_LOOK)
	ERR (nonclient, "Called in Win95 mode. Aiee! Please report this.\n" );

    if (dlgFrame)
    {
	width = SYSMETRICS_CXDLGFRAME - 1;
	height = SYSMETRICS_CYDLGFRAME - 1;
        SelectObject( hdc, GetSysColorBrush(active ? COLOR_ACTIVECAPTION :
						COLOR_INACTIVECAPTION) );
    }
    else
    {
	width = SYSMETRICS_CXFRAME - 1;
	height = SYSMETRICS_CYFRAME - 1;
        SelectObject( hdc, GetSysColorBrush(active ? COLOR_ACTIVEBORDER :
						COLOR_INACTIVEBORDER) );
    }

      /* Draw frame */
    PatBlt( hdc, rect->left, rect->top,
              rect->right - rect->left, height, PATCOPY );
    PatBlt( hdc, rect->left, rect->top,
              width, rect->bottom - rect->top, PATCOPY );
    PatBlt( hdc, rect->left, rect->bottom - 1,
              rect->right - rect->left, -height, PATCOPY );
    PatBlt( hdc, rect->right - 1, rect->top,
              -width, rect->bottom - rect->top, PATCOPY );

    if (dlgFrame)
    {
	InflateRect( rect, -width, -height );
    } 
    else
    {
        INT decYOff = SYSMETRICS_CXFRAME + SYSMETRICS_CXSIZE;
	INT decXOff = SYSMETRICS_CYFRAME + SYSMETRICS_CYSIZE;

      /* Draw inner rectangle */

	SelectObject( hdc, GetStockObject(NULL_BRUSH) );
	Rectangle( hdc, rect->left + width, rect->top + height,
		     rect->right - width , rect->bottom - height );

      /* Draw the decorations */

	MoveToEx( hdc, rect->left, rect->top + decYOff, NULL );
	LineTo( hdc, rect->left + width, rect->top + decYOff );
	MoveToEx( hdc, rect->right - 1, rect->top + decYOff, NULL );
	LineTo( hdc, rect->right - width - 1, rect->top + decYOff );
	MoveToEx( hdc, rect->left, rect->bottom - decYOff, NULL );
	LineTo( hdc, rect->left + width, rect->bottom - decYOff );
	MoveToEx( hdc, rect->right - 1, rect->bottom - decYOff, NULL );
	LineTo( hdc, rect->right - width - 1, rect->bottom - decYOff );

	MoveToEx( hdc, rect->left + decXOff, rect->top, NULL );
	LineTo( hdc, rect->left + decXOff, rect->top + height);
	MoveToEx( hdc, rect->left + decXOff, rect->bottom - 1, NULL );
	LineTo( hdc, rect->left + decXOff, rect->bottom - height - 1 );
	MoveToEx( hdc, rect->right - decXOff, rect->top, NULL );
	LineTo( hdc, rect->right - decXOff, rect->top + height );
	MoveToEx( hdc, rect->right - decXOff, rect->bottom - 1, NULL );
	LineTo( hdc, rect->right - decXOff, rect->bottom - height - 1 );

	InflateRect( rect, -width - 1, -height - 1 );
    }
}


/******************************************************************************
 *
 *   void  NC_DrawFrame95(
 *      HDC32  hdc,
 *      RECT32  *rect,
 *      BOOL32  dlgFrame,
 *      BOOL32  active )
 *
 *   Draw a window frame inside the given rectangle, and update the rectangle.
 *   The correct pen for the frame must be selected in the DC.
 *
 *   Bugs
 *        Many.  First, just what IS a frame in Win95?  Note that the 3D look
 *        on the outer edge is handled by NC_DoNCPaint95.  As is the inner
 *        edge.  The inner rectangle just inside the frame is handled by the
 *        Caption code.
 *
 *        In short, for most people, this function should be a nop (unless
 *        you LIKE thick borders in Win95/NT4.0 -- I've been working with
 *        them lately, but just to get this code right).  Even so, it doesn't
 *        appear to be so.  It's being worked on...
 * 
 *   Revision history
 *        06-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation (based on NC_DrawFrame)
 *        02-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *             Some minor fixes.
 *
 *****************************************************************************/

static void  NC_DrawFrame95(
    HDC  hdc,
    RECT  *rect,
    BOOL  dlgFrame,
    BOOL  active )
{
    INT width, height;

    if (dlgFrame)
    {
	width = sysMetrics[SM_CXDLGFRAME] - sysMetrics[SM_CXEDGE];
	height = sysMetrics[SM_CYDLGFRAME] - sysMetrics[SM_CYEDGE];
    }
    else
    {
	width = sysMetrics[SM_CXFRAME] - sysMetrics[SM_CXEDGE];
	height = sysMetrics[SM_CYFRAME] - sysMetrics[SM_CYEDGE];
    }

    SelectObject( hdc, GetSysColorBrush(active ? COLOR_ACTIVEBORDER :
		COLOR_INACTIVEBORDER) );

    /* Draw frame */
    PatBlt( hdc, rect->left, rect->top,
              rect->right - rect->left, height, PATCOPY );
    PatBlt( hdc, rect->left, rect->top,
              width, rect->bottom - rect->top, PATCOPY );
    PatBlt( hdc, rect->left, rect->bottom - 1,
              rect->right - rect->left, -height, PATCOPY );
    PatBlt( hdc, rect->right - 1, rect->top - 1,
              -width, rect->bottom - rect->top, PATCOPY );

    InflateRect( rect, -width, -height );
}

/***********************************************************************
 *           NC_DrawMovingFrame
 *
 * Draw the frame used when moving or resizing window.
 *
 * FIXME:  This causes problems in Win95 mode.  (why?)
 */
static void NC_DrawMovingFrame( HDC hdc, RECT *rect, BOOL thickframe )
{
    if (thickframe)
    {
        RECT16 r16;
        CONV_RECT32TO16( rect, &r16 );
        FastWindowFrame16( hdc, &r16, SYSMETRICS_CXFRAME,
                         SYSMETRICS_CYFRAME, PATINVERT );
    }
    else DrawFocusRect( hdc, rect );
}


/***********************************************************************
 *           NC_DrawCaption
 *
 * Draw the window caption.
 * The correct pen for the window frame must be selected in the DC.
 */
static void NC_DrawCaption( HDC hdc, RECT *rect, HWND hwnd,
			    DWORD style, BOOL active )
{
    RECT r = *rect;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    char buffer[256];

    if (wndPtr->flags & WIN_MANAGED)
    {
        WIN_ReleaseWndPtr(wndPtr);
        return;
    }

    if (!hbitmapClose)
    {
	if (!(hbitmapClose = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_CLOSE) )))
        {
            WIN_ReleaseWndPtr(wndPtr);
	    return;
        }
	hbitmapCloseD    = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_CLOSED) );
	hbitmapMinimize  = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_REDUCE) );
	hbitmapMinimizeD = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_REDUCED) );
	hbitmapMaximize  = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_ZOOM) );
	hbitmapMaximizeD = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_ZOOMD) );
	hbitmapRestore   = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_RESTORE) );
	hbitmapRestoreD  = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_RESTORED) );
    }
    
    if (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME)
    {
        HBRUSH hbrushOld = SelectObject(hdc, GetSysColorBrush(COLOR_WINDOW) );
	PatBlt( hdc, r.left, r.top, 1, r.bottom-r.top+1,PATCOPY );
	PatBlt( hdc, r.right-1, r.top, 1, r.bottom-r.top+1, PATCOPY );
	PatBlt( hdc, r.left, r.top-1, r.right-r.left, 1, PATCOPY );
	r.left++;
	r.right--;
	SelectObject( hdc, hbrushOld );
    }
    WIN_ReleaseWndPtr(wndPtr);
    MoveTo16( hdc, r.left, r.bottom );
    LineTo( hdc, r.right, r.bottom );

    if (style & WS_SYSMENU)
    {
	NC_DrawSysButton( hwnd, hdc, FALSE );
	r.left += SYSMETRICS_CXSIZE + 1;
	MoveTo16( hdc, r.left - 1, r.top );
	LineTo( hdc, r.left - 1, r.bottom );
    }
    if (style & WS_MAXIMIZEBOX)
    {
	NC_DrawMaxButton( hwnd, hdc, FALSE );
	r.right -= SYSMETRICS_CXSIZE + 1;
    }
    if (style & WS_MINIMIZEBOX)
    {
	NC_DrawMinButton( hwnd, hdc, FALSE );
	r.right -= SYSMETRICS_CXSIZE + 1;
    }

    FillRect( hdc, &r, GetSysColorBrush(active ? COLOR_ACTIVECAPTION :
					    COLOR_INACTIVECAPTION) );

    if (GetWindowTextA( hwnd, buffer, sizeof(buffer) ))
    {
	if (active) SetTextColor( hdc, GetSysColor( COLOR_CAPTIONTEXT ) );
	else SetTextColor( hdc, GetSysColor( COLOR_INACTIVECAPTIONTEXT ) );
	SetBkMode( hdc, TRANSPARENT );
	DrawTextA( hdc, buffer, -1, &r,
                     DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX );
    }
}


/******************************************************************************
 *
 *   NC_DrawCaption95(
 *      HDC32  hdc,
 *      RECT32 *rect,
 *      HWND32 hwnd,
 *      DWORD  style,
 *      BOOL32 active )
 *
 *   Draw the window caption for Win95 style windows.
 *   The correct pen for the window frame must be selected in the DC.
 *
 *   Bugs
 *        Hey, a function that finally works!  Well, almost.
 *        It's being worked on.
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *        02-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *             Some minor fixes.
 *
 *****************************************************************************/

static void  NC_DrawCaption95(
    HDC  hdc,
    RECT *rect,
    HWND hwnd,
    DWORD  style,
    DWORD  exStyle,
    BOOL active )
{
    RECT  r = *rect;
    WND     *wndPtr = WIN_FindWndPtr( hwnd );
    char    buffer[256];
    HPEN  hPrevPen;

    if (wndPtr->flags & WIN_MANAGED)
    {
        WIN_ReleaseWndPtr(wndPtr);
        return;
    }
    WIN_ReleaseWndPtr(wndPtr);

    hPrevPen = SelectObject( hdc, GetSysColorPen(COLOR_3DFACE) );
    MoveToEx( hdc, r.left, r.bottom - 1, NULL );
    LineTo( hdc, r.right, r.bottom - 1 );
    SelectObject( hdc, hPrevPen );
    r.bottom--;

    FillRect( hdc, &r, GetSysColorBrush(active ? COLOR_ACTIVECAPTION :
					    COLOR_INACTIVECAPTION) );

    if (!hbitmapClose) {
	if (!(hbitmapClose = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_CLOSE) )))
	    return;
        hbitmapCloseD = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_CLOSED));
	hbitmapMinimize  = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_REDUCE) );
	hbitmapMinimizeD = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_REDUCED) );
	hbitmapMaximize  = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_ZOOM) );
	hbitmapMaximizeD = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_ZOOMD) );
	hbitmapRestore   = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_RESTORE) );
	hbitmapRestoreD  = LoadBitmap16( 0, MAKEINTRESOURCE16(OBM_RESTORED) );
    }

    if ((style & WS_SYSMENU) && !(exStyle & WS_EX_TOOLWINDOW)) {
	if (NC_DrawSysButton95 (hwnd, hdc, FALSE))
	    r.left += sysMetrics[SM_CYCAPTION] - 1;
    }
    if (style & WS_SYSMENU) {
	NC_DrawCloseButton95 (hwnd, hdc, FALSE);
	r.right -= sysMetrics[SM_CYCAPTION] - 1;
    }
    if (style & WS_MAXIMIZEBOX) {
	NC_DrawMaxButton95( hwnd, hdc, FALSE );
	r.right -= SYSMETRICS_CXSIZE + 1;
    }
    if (style & WS_MINIMIZEBOX) {
	NC_DrawMinButton95( hwnd, hdc, FALSE );
	r.right -= SYSMETRICS_CXSIZE + 1;
    }

    if (GetWindowTextA( hwnd, buffer, sizeof(buffer) )) {
	NONCLIENTMETRICSA nclm;
	HFONT hFont, hOldFont;
	nclm.cbSize = sizeof(NONCLIENTMETRICSA);
	SystemParametersInfoA (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
	if (exStyle & WS_EX_TOOLWINDOW)
	    hFont = CreateFontIndirectA (&nclm.lfSmCaptionFont);
	else
	    hFont = CreateFontIndirectA (&nclm.lfCaptionFont);
	hOldFont = SelectObject (hdc, hFont);
	if (active) SetTextColor( hdc, GetSysColor( COLOR_CAPTIONTEXT ) );
	else SetTextColor( hdc, GetSysColor( COLOR_INACTIVECAPTIONTEXT ) );
	SetBkMode( hdc, TRANSPARENT );
	r.left += 2;
	DrawTextA( hdc, buffer, -1, &r,
		     DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_LEFT );
	DeleteObject (SelectObject (hdc, hOldFont));
    }
}



/***********************************************************************
 *           NC_DoNCPaint
 *
 * Paint the non-client area. clip is currently unused.
 */
void NC_DoNCPaint( WND* wndPtr, HRGN clip, BOOL suppress_menupaint )
{
    HDC hdc;
    RECT rect;
    BOOL active;
    HWND hwnd = wndPtr->hwndSelf;

    if ( wndPtr->dwStyle & WS_MINIMIZE ||
	!WIN_IsWindowDrawable( wndPtr, 0 )) return; /* Nothing to do */

    active  = wndPtr->flags & WIN_NCACTIVATED;

    TRACE(nonclient, "%04x %d\n", hwnd, active );

    if (!(hdc = GetDCEx( hwnd, (clip > 1) ? clip : 0, DCX_USESTYLE | DCX_WINDOW |
			      ((clip > 1) ? (DCX_INTERSECTRGN | DCX_KEEPCLIPRGN): 0) ))) return;

    if (ExcludeVisRect16( hdc, wndPtr->rectClient.left-wndPtr->rectWindow.left,
		        wndPtr->rectClient.top-wndPtr->rectWindow.top,
		        wndPtr->rectClient.right-wndPtr->rectWindow.left,
		        wndPtr->rectClient.bottom-wndPtr->rectWindow.top )
	== NULLREGION)
    {
	ReleaseDC( hwnd, hdc );
	return;
    }

    rect.top = rect.left = 0;
    rect.right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    rect.bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

    SelectObject( hdc, GetSysColorPen(COLOR_WINDOWFRAME) );

    if (!(wndPtr->flags & WIN_MANAGED))
    {
        if ((wndPtr->dwStyle & WS_BORDER) || (wndPtr->dwStyle & WS_DLGFRAME) ||
            (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME))
        {
	    SelectObject( hdc, GetStockObject(NULL_BRUSH) );
            Rectangle( hdc, 0, 0, rect.right, rect.bottom );
            InflateRect( &rect, -1, -1 );
        }

        if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle )) 
            NC_DrawFrame( hdc, &rect, TRUE, active );
        else if (wndPtr->dwStyle & WS_THICKFRAME)
            NC_DrawFrame(hdc, &rect, FALSE, active );

        if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
        {
            RECT r = rect;
            r.bottom = rect.top + SYSMETRICS_CYSIZE;
            rect.top += SYSMETRICS_CYSIZE + SYSMETRICS_CYBORDER;
            NC_DrawCaption( hdc, &r, hwnd, wndPtr->dwStyle, active );
        }
    }

    if (HAS_MENU(wndPtr))
    {
	RECT r = rect;
	r.bottom = rect.top + SYSMETRICS_CYMENU;  /* default height */
	rect.top += MENU_DrawMenuBar( hdc, &r, hwnd, suppress_menupaint );
    }

      /* Draw the scroll-bars */

    if (wndPtr->dwStyle & WS_VSCROLL)
        SCROLL_DrawScrollBar( hwnd, hdc, SB_VERT, TRUE, TRUE );
    if (wndPtr->dwStyle & WS_HSCROLL)
        SCROLL_DrawScrollBar( hwnd, hdc, SB_HORZ, TRUE, TRUE );

      /* Draw the "size-box" */

    if ((wndPtr->dwStyle & WS_VSCROLL) && (wndPtr->dwStyle & WS_HSCROLL))
    {
        RECT r = rect;
        r.left = r.right - SYSMETRICS_CXVSCROLL + 1;
        r.top  = r.bottom - SYSMETRICS_CYHSCROLL + 1;
	if(wndPtr->dwStyle & WS_BORDER) {
	  r.left++;
	  r.top++;
	}
        FillRect( hdc, &r, GetSysColorBrush(COLOR_SCROLLBAR) );
    }    

    ReleaseDC( hwnd, hdc );
}


/******************************************************************************
 *
 *   void  NC_DoNCPaint95(
 *      WND  *wndPtr,
 *      HRGN32  clip,
 *      BOOL32  suppress_menupaint )
 *
 *   Paint the non-client area for Win95 windows.  The clip region is
 *   currently ignored.
 *
 *   Bugs
 *        grep -E -A10 -B5 \(95\)\|\(Bugs\)\|\(FIXME\) windows/nonclient.c \
 *           misc/tweak.c controls/menu.c  # :-)
 *
 *   Revision history
 *        03-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation
 *        10-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *             Fixed some bugs.
 *
 *****************************************************************************/

void  NC_DoNCPaint95(
    WND  *wndPtr,
    HRGN  clip,
    BOOL  suppress_menupaint )
{
    HDC hdc;
    RECT rfuzz, rect, rectClip;
    BOOL active;
    HWND hwnd = wndPtr->hwndSelf;

    if ( wndPtr->dwStyle & WS_MINIMIZE ||
	!WIN_IsWindowDrawable( wndPtr, 0 )) return; /* Nothing to do */

    active  = wndPtr->flags & WIN_NCACTIVATED;

    TRACE(nonclient, "%04x %d\n", hwnd, active );

    /* MSDN docs are pretty idiotic here, they say app CAN use clipRgn in the call to
     * GetDCEx implying that it is allowed not to use it either. However, the suggested
     * GetDCEx(    , DCX_WINDOW | DCX_INTERSECTRGN) will cause clipRgn to be deleted
     * after ReleaseDC(). Now, how is the "system" supposed to tell what happened?
     */

    if (!(hdc = GetDCEx( hwnd, (clip > 1) ? clip : 0, DCX_USESTYLE | DCX_WINDOW |
			      ((clip > 1) ?(DCX_INTERSECTRGN | DCX_KEEPCLIPRGN) : 0) ))) return;


    if (ExcludeVisRect16( hdc, wndPtr->rectClient.left-wndPtr->rectWindow.left,
		        wndPtr->rectClient.top-wndPtr->rectWindow.top,
		        wndPtr->rectClient.right-wndPtr->rectWindow.left,
		        wndPtr->rectClient.bottom-wndPtr->rectWindow.top )
	== NULLREGION)
    {
	ReleaseDC( hwnd, hdc );
	return;
    }

    rect.top = rect.left = 0;
    rect.right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    rect.bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

    if( clip > 1 )
	GetRgnBox( clip, &rectClip );
    else
    {
	clip = 0;
	rectClip = rect;
    }

    SelectObject( hdc, GetSysColorPen(COLOR_WINDOWFRAME) );

    if(!(wndPtr->flags & WIN_MANAGED)) {
        if ((wndPtr->dwStyle & WS_BORDER) && ((wndPtr->dwStyle & WS_DLGFRAME) ||
	    (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME) || (wndPtr->dwStyle & WS_THICKFRAME))) {
            DrawEdge (hdc, &rect, EDGE_RAISED, BF_RECT | BF_ADJUST);
        }

        if (HAS_FIXEDFRAME( wndPtr->dwStyle, wndPtr->dwExStyle )) 
            NC_DrawFrame95( hdc, &rect, TRUE, active );
        else if (wndPtr->dwStyle & WS_THICKFRAME)
            NC_DrawFrame95(hdc, &rect, FALSE, active );

        if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
        {
            RECT  r = rect;
	    if (wndPtr->dwExStyle & WS_EX_TOOLWINDOW) {
		r.bottom = rect.top + sysMetrics[SM_CYSMCAPTION];
		rect.top += sysMetrics[SM_CYSMCAPTION];
	    }
	    else {
		r.bottom = rect.top + sysMetrics[SM_CYCAPTION];
		rect.top += sysMetrics[SM_CYCAPTION];
	    }
	    if( !clip || IntersectRect( &rfuzz, &r, &rectClip ) )
                NC_DrawCaption95 (hdc, &r, hwnd, wndPtr->dwStyle,
                                  wndPtr->dwExStyle, active);
        }
    }

    if (HAS_MENU(wndPtr))
    {
	RECT r = rect;
	r.bottom = rect.top + sysMetrics[SM_CYMENU];
	
	TRACE(nonclient, "Calling DrawMenuBar with "
			  "rect (%d, %d)-(%d, %d)\n", r.left, r.top,
			  r.right, r.bottom);

	rect.top += MENU_DrawMenuBar( hdc, &r, hwnd, suppress_menupaint ) + 1;
    }

    TRACE(nonclient, "After MenuBar, rect is (%d, %d)-(%d, %d).\n",
		       rect.left, rect.top, rect.right, rect.bottom );

    if (wndPtr->dwExStyle & WS_EX_CLIENTEDGE)
	DrawEdge (hdc, &rect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);

    if (wndPtr->dwExStyle & WS_EX_STATICEDGE)
	DrawEdge (hdc, &rect, BDR_SUNKENOUTER, BF_RECT | BF_ADJUST);

    /* Draw the scroll-bars */

    if (wndPtr->dwStyle & WS_VSCROLL)
        SCROLL_DrawScrollBar( hwnd, hdc, SB_VERT, TRUE, TRUE );
    if (wndPtr->dwStyle & WS_HSCROLL)
        SCROLL_DrawScrollBar( hwnd, hdc, SB_HORZ, TRUE, TRUE );

    /* Draw the "size-box" */
    if ((wndPtr->dwStyle & WS_VSCROLL) && (wndPtr->dwStyle & WS_HSCROLL))
    {
        RECT r = rect;
        r.left = r.right - SYSMETRICS_CXVSCROLL + 1;
        r.top  = r.bottom - SYSMETRICS_CYHSCROLL + 1;
        FillRect( hdc, &r,  GetSysColorBrush(COLOR_SCROLLBAR) );
    }    

    ReleaseDC( hwnd, hdc );
}




/***********************************************************************
 *           NC_HandleNCPaint
 *
 * Handle a WM_NCPAINT message. Called from DefWindowProc().
 */
LONG NC_HandleNCPaint( HWND hwnd , HRGN clip)
{
    WND* wndPtr = WIN_FindWndPtr( hwnd );

    if( wndPtr && wndPtr->dwStyle & WS_VISIBLE )
    {
	if( wndPtr->dwStyle & WS_MINIMIZE )
	    WINPOS_RedrawIconTitle( hwnd );
	else if (TWEAK_WineLook == WIN31_LOOK)
	    NC_DoNCPaint( wndPtr, clip, FALSE );
	else
	    NC_DoNCPaint95( wndPtr, clip, FALSE );
    }
    WIN_ReleaseWndPtr(wndPtr);
    return 0;
}


/***********************************************************************
 *           NC_HandleNCActivate
 *
 * Handle a WM_NCACTIVATE message. Called from DefWindowProc().
 */
LONG NC_HandleNCActivate( WND *wndPtr, WPARAM16 wParam )
{
    WORD wStateChange;

    if( wParam ) wStateChange = !(wndPtr->flags & WIN_NCACTIVATED);
    else wStateChange = wndPtr->flags & WIN_NCACTIVATED;

    if( wStateChange )
    {
	if (wParam) wndPtr->flags |= WIN_NCACTIVATED;
	else wndPtr->flags &= ~WIN_NCACTIVATED;

	if( wndPtr->dwStyle & WS_MINIMIZE ) 
	    WINPOS_RedrawIconTitle( wndPtr->hwndSelf );
	else if (TWEAK_WineLook == WIN31_LOOK)
	    NC_DoNCPaint( wndPtr, (HRGN)1, FALSE );
	else
	    NC_DoNCPaint95( wndPtr, (HRGN)1, FALSE );
    }
    return TRUE;
}


/***********************************************************************
 *           NC_HandleSetCursor
 *
 * Handle a WM_SETCURSOR message. Called from DefWindowProc().
 */
LONG NC_HandleSetCursor( HWND hwnd, WPARAM16 wParam, LPARAM lParam )
{
    if (hwnd != (HWND)wParam) return 0;  /* Don't set the cursor for child windows */

    switch(LOWORD(lParam))
    {
    case HTERROR:
	{
	    WORD msg = HIWORD( lParam );
	    if ((msg == WM_LBUTTONDOWN) || (msg == WM_MBUTTONDOWN) ||
		(msg == WM_RBUTTONDOWN))
		MessageBeep(0);
	}
	break;

    case HTCLIENT:
	{
	    WND *wndPtr;
            BOOL retvalue;
            
	    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) break;
	    if (wndPtr->class->hCursor)
	    {
		SetCursor16( wndPtr->class->hCursor );
		retvalue = TRUE;
	    }
            else retvalue = FALSE;
            WIN_ReleaseWndPtr(wndPtr);
            return retvalue;
	}

    case HTLEFT:
    case HTRIGHT:
	return (LONG)SetCursor16( LoadCursor16( 0, IDC_SIZEWE16 ) );

    case HTTOP:
    case HTBOTTOM:
	return (LONG)SetCursor16( LoadCursor16( 0, IDC_SIZENS16 ) );

    case HTTOPLEFT:
    case HTBOTTOMRIGHT:	
	return (LONG)SetCursor16( LoadCursor16( 0, IDC_SIZENWSE16 ) );

    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
	return (LONG)SetCursor16( LoadCursor16( 0, IDC_SIZENESW16 ) );
    }

    /* Default cursor: arrow */
    return (LONG)SetCursor16( LoadCursor16( 0, IDC_ARROW16 ) );
}

/***********************************************************************
 *           NC_GetSysPopupPos
 */
BOOL NC_GetSysPopupPos( WND* wndPtr, RECT* rect )
{
  if( wndPtr->hSysMenu )
  {
      if( wndPtr->dwStyle & WS_MINIMIZE )
	  GetWindowRect( wndPtr->hwndSelf, rect );
      else
      {
          if (TWEAK_WineLook == WIN31_LOOK)
              NC_GetInsideRect( wndPtr->hwndSelf, rect );
          else
              NC_GetInsideRect95( wndPtr->hwndSelf, rect );
  	  OffsetRect( rect, wndPtr->rectWindow.left, wndPtr->rectWindow.top);
  	  if (wndPtr->dwStyle & WS_CHILD)
     	      ClientToScreen( wndPtr->parent->hwndSelf, (POINT *)rect );
          if (TWEAK_WineLook == WIN31_LOOK) {
            rect->right = rect->left + SYSMETRICS_CXSIZE;
            rect->bottom = rect->top + SYSMETRICS_CYSIZE;
	  }
	  else {
            rect->right = rect->left + sysMetrics[SM_CYCAPTION] - 1;
            rect->bottom = rect->top + sysMetrics[SM_CYCAPTION] - 1;
	  }
      }
      return TRUE;
  }
  return FALSE;
}

/***********************************************************************
 *           NC_StartSizeMove
 *
 * Initialisation of a move or resize, when initiatied from a menu choice.
 * Return hit test code for caption or sizing border.
 */
static LONG NC_StartSizeMove( WND* wndPtr, WPARAM16 wParam,
                              POINT16 *capturePoint )
{
    LONG hittest = 0;
    POINT16 pt;
    MSG msg;

    if ((wParam & 0xfff0) == SC_MOVE)
    {
	  /* Move pointer at the center of the caption */
	RECT rect;
        if (TWEAK_WineLook == WIN31_LOOK)
            NC_GetInsideRect( wndPtr->hwndSelf, &rect );
        else
            NC_GetInsideRect95( wndPtr->hwndSelf, &rect );
	if (wndPtr->dwStyle & WS_SYSMENU)
	    rect.left += SYSMETRICS_CXSIZE + 1;
	if (wndPtr->dwStyle & WS_MINIMIZEBOX)
	    rect.right -= SYSMETRICS_CXSIZE + 1;
	if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
	    rect.right -= SYSMETRICS_CXSIZE + 1;
	pt.x = wndPtr->rectWindow.left + (rect.right - rect.left) / 2;
	pt.y = wndPtr->rectWindow.top + rect.top + SYSMETRICS_CYSIZE/2;
	hittest = HTCAPTION;
	*capturePoint = pt;
    }
    else  /* SC_SIZE */
    {
	while(!hittest)
	{
            MSG_InternalGetMessage( &msg, 0, 0, MSGF_SIZE, PM_REMOVE, FALSE );
	    switch(msg.message)
	    {
	    case WM_MOUSEMOVE:
                CONV_POINT32TO16(&msg.pt, &pt);
                hittest = NC_HandleNCHitTest( wndPtr->hwndSelf, pt );
		if ((hittest < HTLEFT) || (hittest > HTBOTTOMRIGHT))
		    hittest = 0;
		break;

	    case WM_LBUTTONUP:
		return 0;

	    case WM_KEYDOWN:
		switch(msg.wParam)
		{
		case VK_UP:
		    hittest = HTTOP;
		    pt.x =(wndPtr->rectWindow.left+wndPtr->rectWindow.right)/2;
		    pt.y = wndPtr->rectWindow.top + SYSMETRICS_CYFRAME / 2;
		    break;
		case VK_DOWN:
		    hittest = HTBOTTOM;
		    pt.x =(wndPtr->rectWindow.left+wndPtr->rectWindow.right)/2;
		    pt.y = wndPtr->rectWindow.bottom - SYSMETRICS_CYFRAME / 2;
		    break;
		case VK_LEFT:
		    hittest = HTLEFT;
		    pt.x = wndPtr->rectWindow.left + SYSMETRICS_CXFRAME / 2;
		    pt.y =(wndPtr->rectWindow.top+wndPtr->rectWindow.bottom)/2;
		    break;
		case VK_RIGHT:
		    hittest = HTRIGHT;
		    pt.x = wndPtr->rectWindow.right - SYSMETRICS_CXFRAME / 2;
		    pt.y =(wndPtr->rectWindow.top+wndPtr->rectWindow.bottom)/2;
		    break;
		case VK_RETURN:
		case VK_ESCAPE: return 0;
		}
	    }
	}
	*capturePoint = pt;
    }
    SetCursorPos( pt.x, pt.y );
    NC_HandleSetCursor( wndPtr->hwndSelf, 
			wndPtr->hwndSelf, MAKELONG( hittest, WM_MOUSEMOVE ));
    return hittest;
}


/***********************************************************************
 *           NC_DoSizeMove
 *
 * Perform SC_MOVE and SC_SIZE commands.               `
 */
static void NC_DoSizeMove( HWND hwnd, WORD wParam )
{
    MSG msg;
    RECT sizingRect, mouseRect;
    HDC hdc;
    LONG hittest = (LONG)(wParam & 0x0f);
    HCURSOR16 hDragCursor = 0, hOldCursor = 0;
    POINT minTrack, maxTrack;
    POINT16 capturePoint, pt;
    WND *     wndPtr = WIN_FindWndPtr( hwnd );
    BOOL    thickframe = HAS_THICKFRAME( wndPtr->dwStyle );
    BOOL    iconic = wndPtr->dwStyle & WS_MINIMIZE;
    BOOL    moved = FALSE;
    DWORD     dwPoint = GetMessagePos ();

    capturePoint = pt = *(POINT16*)&dwPoint;

    if (IsZoomed(hwnd) || !IsWindowVisible(hwnd) ||
        (wndPtr->flags & WIN_MANAGED)) goto END;

    if ((wParam & 0xfff0) == SC_MOVE)
    {
	if (!(wndPtr->dwStyle & WS_CAPTION)) goto END;
	if (!hittest) 
	     hittest = NC_StartSizeMove( wndPtr, wParam, &capturePoint );
	if (!hittest) goto END;
    }
    else  /* SC_SIZE */
    {
	if (!thickframe) goto END;
	if ( hittest && hittest != HTSYSMENU ) hittest += 2;
	else
	{
	    SetCapture(hwnd);
	    hittest = NC_StartSizeMove( wndPtr, wParam, &capturePoint );
	    if (!hittest)
	    {
		ReleaseCapture();
		goto END;
	    }
	}
    }

      /* Get min/max info */

    WINPOS_GetMinMaxInfo( wndPtr, NULL, NULL, &minTrack, &maxTrack );
    sizingRect = wndPtr->rectWindow;
    if (wndPtr->dwStyle & WS_CHILD)
	GetClientRect( wndPtr->parent->hwndSelf, &mouseRect );
    else 
        SetRect(&mouseRect, 0, 0, SYSMETRICS_CXSCREEN, SYSMETRICS_CYSCREEN);
    if (ON_LEFT_BORDER(hittest))
    {
	mouseRect.left  = MAX( mouseRect.left, sizingRect.right-maxTrack.x );
	mouseRect.right = MIN( mouseRect.right, sizingRect.right-minTrack.x );
    }
    else if (ON_RIGHT_BORDER(hittest))
    {
	mouseRect.left  = MAX( mouseRect.left, sizingRect.left+minTrack.x );
	mouseRect.right = MIN( mouseRect.right, sizingRect.left+maxTrack.x );
    }
    if (ON_TOP_BORDER(hittest))
    {
	mouseRect.top    = MAX( mouseRect.top, sizingRect.bottom-maxTrack.y );
	mouseRect.bottom = MIN( mouseRect.bottom,sizingRect.bottom-minTrack.y);
    }
    else if (ON_BOTTOM_BORDER(hittest))
    {
	mouseRect.top    = MAX( mouseRect.top, sizingRect.top+minTrack.y );
	mouseRect.bottom = MIN( mouseRect.bottom, sizingRect.top+maxTrack.y );
    }
    if (wndPtr->dwStyle & WS_CHILD)
    {
	MapWindowPoints( wndPtr->parent->hwndSelf, 0, 
		(LPPOINT)&mouseRect, 2 );
    }
    SendMessage16( hwnd, WM_ENTERSIZEMOVE, 0, 0 );

    if (GetCapture() != hwnd) SetCapture( hwnd );    

    if (wndPtr->dwStyle & WS_CHILD)
    {
          /* Retrieve a default cache DC (without using the window style) */
        hdc = GetDCEx( wndPtr->parent->hwndSelf, 0, DCX_CACHE );
    }
    else
    {  /* Grab the server only when moving top-level windows without desktop */
	hdc = GetDC( 0 );
    }

    wndPtr->pDriver->pPreSizeMove(wndPtr);

    if( iconic ) /* create a cursor for dragging */
    {
	HICON16 hIcon = (wndPtr->class->hIcon) ? wndPtr->class->hIcon
                      : (HICON16)SendMessage16( hwnd, WM_QUERYDRAGICON, 0, 0L);
	if( hIcon ) hDragCursor =  CURSORICON_IconToCursor( hIcon, TRUE );
	if( !hDragCursor ) iconic = FALSE;
    }

    if( !iconic ) NC_DrawMovingFrame( hdc, &sizingRect, thickframe );

    while(1)
    {
        int dx = 0, dy = 0;

        MSG_InternalGetMessage( &msg, 0, 0, MSGF_SIZE, PM_REMOVE, FALSE );

	  /* Exit on button-up, Return, or Esc */
	if ((msg.message == WM_LBUTTONUP) ||
	    ((msg.message == WM_KEYDOWN) && 
	     ((msg.wParam == VK_RETURN) || (msg.wParam == VK_ESCAPE)))) break;

	if ((msg.message != WM_KEYDOWN) && (msg.message != WM_MOUSEMOVE))
	    continue;  /* We are not interested in other messages */

	dwPoint = GetMessagePos ();
	pt = *(POINT16*)&dwPoint;
	
	if (msg.message == WM_KEYDOWN) switch(msg.wParam)
	{
	    case VK_UP:    pt.y -= 8; break;
	    case VK_DOWN:  pt.y += 8; break;
	    case VK_LEFT:  pt.x -= 8; break;
	    case VK_RIGHT: pt.x += 8; break;		
	}

	pt.x = MAX( pt.x, mouseRect.left );
	pt.x = MIN( pt.x, mouseRect.right );
	pt.y = MAX( pt.y, mouseRect.top );
	pt.y = MIN( pt.y, mouseRect.bottom );

	dx = pt.x - capturePoint.x;
	dy = pt.y - capturePoint.y;

	if (dx || dy)
	{
	    if( !moved )
	    {
		moved = TRUE;
        	if( iconic ) /* ok, no system popup tracking */
		{
		    hOldCursor = SetCursor(hDragCursor);
		    ShowCursor( TRUE );
		    WINPOS_ShowIconTitle( wndPtr, FALSE );
		}
	    }

	    if (msg.message == WM_KEYDOWN) SetCursorPos( pt.x, pt.y );
	    else
	    {
		RECT newRect = sizingRect;

		if (hittest == HTCAPTION) OffsetRect( &newRect, dx, dy );
		if (ON_LEFT_BORDER(hittest)) newRect.left += dx;
		else if (ON_RIGHT_BORDER(hittest)) newRect.right += dx;
		if (ON_TOP_BORDER(hittest)) newRect.top += dy;
		else if (ON_BOTTOM_BORDER(hittest)) newRect.bottom += dy;
		if( !iconic )
		{
		    NC_DrawMovingFrame( hdc, &sizingRect, thickframe );
		    NC_DrawMovingFrame( hdc, &newRect, thickframe );
		}
		capturePoint = pt;
		sizingRect = newRect;
	    }
	}
    }

    ReleaseCapture();
    if( iconic )
    {
	if( moved ) /* restore cursors, show icon title later on */
	{
	    ShowCursor( FALSE );
	    SetCursor( hOldCursor );
	}
        DestroyCursor( hDragCursor );
    }
    else
	NC_DrawMovingFrame( hdc, &sizingRect, thickframe );

    if (wndPtr->dwStyle & WS_CHILD)
        ReleaseDC( wndPtr->parent->hwndSelf, hdc );
    else
    {
	ReleaseDC( 0, hdc );
    }

    wndPtr->pDriver->pPostSizeMove(wndPtr);

    if (HOOK_IsHooked( WH_CBT ))
    {
	RECT16* pr = SEGPTR_NEW(RECT16);
	if( pr )
	{
            CONV_RECT32TO16( &sizingRect, pr );
	    if( HOOK_CallHooks16( WH_CBT, HCBT_MOVESIZE, hwnd,
			        (LPARAM)SEGPTR_GET(pr)) )
		sizingRect = wndPtr->rectWindow;
	    else
		CONV_RECT16TO32( pr, &sizingRect );
	    SEGPTR_FREE(pr);
	}
    }
    SendMessage16( hwnd, WM_EXITSIZEMOVE, 0, 0 );
    SendMessage16( hwnd, WM_SETVISIBLE, !IsIconic16(hwnd), 0L);

    if( moved && !((msg.message == WM_KEYDOWN) && (msg.wParam == VK_ESCAPE)) )
    {
	/* NOTE: SWP_NOACTIVATE prevents document window activation in Word 6 */
	SetWindowPos( hwnd, 0, sizingRect.left, sizingRect.top,
			sizingRect.right - sizingRect.left,
			sizingRect.bottom - sizingRect.top,
		      ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
    }

    if( IsWindow(hwnd) )
	if( wndPtr->dwStyle & WS_MINIMIZE )
	{
	    /* Single click brings up the system menu when iconized */

	    if( !moved ) 
	    {
		 if( wndPtr->dwStyle & WS_SYSMENU ) 
		     SendMessage16( hwnd, WM_SYSCOMMAND,
				    SC_MOUSEMENU + HTSYSMENU, *((LPARAM*)&pt));
	    }
	    else WINPOS_ShowIconTitle( wndPtr, TRUE );
	}

END:
    WIN_ReleaseWndPtr(wndPtr);
}


/***********************************************************************
 *           NC_TrackMinMaxBox
 *
 * Track a mouse button press on the minimize or maximize box.
 */
static void NC_TrackMinMaxBox( HWND hwnd, WORD wParam )
{
    MSG msg;
    POINT16 pt16;
    HDC hdc = GetWindowDC( hwnd );
    BOOL pressed = TRUE;
    void  (*paintButton)(HWND, HDC16, BOOL);

    SetCapture( hwnd );
    if (wParam == HTMINBUTTON)
	paintButton =
	    (TWEAK_WineLook == WIN31_LOOK) ? &NC_DrawMinButton : &NC_DrawMinButton95;
    else
	paintButton =
	    (TWEAK_WineLook == WIN31_LOOK) ? &NC_DrawMaxButton : &NC_DrawMaxButton95;

    (*paintButton)( hwnd, hdc, TRUE );

    do
    {
	BOOL oldstate = pressed;
        MSG_InternalGetMessage( &msg, 0, 0, 0, PM_REMOVE, FALSE );
        CONV_POINT32TO16( &msg.pt, &pt16 );

	pressed = (NC_HandleNCHitTest( hwnd, pt16 ) == wParam);
	if (pressed != oldstate)
	   (*paintButton)( hwnd, hdc, pressed );
    } while (msg.message != WM_LBUTTONUP);

    (*paintButton)( hwnd, hdc, FALSE );

    ReleaseCapture();
    ReleaseDC( hwnd, hdc );
    if (!pressed) return;

    if (wParam == HTMINBUTTON) 
	SendMessage16( hwnd, WM_SYSCOMMAND, SC_MINIMIZE, *(LONG*)&pt16 );
    else
	SendMessage16( hwnd, WM_SYSCOMMAND, 
		  IsZoomed(hwnd) ? SC_RESTORE:SC_MAXIMIZE, *(LONG*)&pt16 );
}


/***********************************************************************
 * NC_TrackCloseButton95
 *
 * Track a mouse button press on the Win95 close button.
 */
static void
NC_TrackCloseButton95 (HWND hwnd, WORD wParam)
{
    MSG msg;
    POINT16 pt16;
    HDC hdc = GetWindowDC( hwnd );
    BOOL pressed = TRUE;

    SetCapture( hwnd );

    NC_DrawCloseButton95 (hwnd, hdc, TRUE);

    do
    {
	BOOL oldstate = pressed;
        MSG_InternalGetMessage( &msg, 0, 0, 0, PM_REMOVE, FALSE );
        CONV_POINT32TO16( &msg.pt, &pt16 );

	pressed = (NC_HandleNCHitTest( hwnd, pt16 ) == wParam);
	if (pressed != oldstate)
	   NC_DrawCloseButton95 (hwnd, hdc, pressed);
    } while (msg.message != WM_LBUTTONUP);

    NC_DrawCloseButton95 (hwnd, hdc, FALSE);

    ReleaseCapture();
    ReleaseDC( hwnd, hdc );
    if (!pressed) return;

    SendMessage16( hwnd, WM_SYSCOMMAND, SC_CLOSE, *(LONG*)&pt16 );
}


/***********************************************************************
 *           NC_TrackScrollBar
 *
 * Track a mouse button press on the horizontal or vertical scroll-bar.
 */
static void NC_TrackScrollBar( HWND hwnd, WPARAM wParam, POINT pt )
{
    MSG16 *msg;
    INT scrollbar;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if ((wParam & 0xfff0) == SC_HSCROLL)
    {
	if ((wParam & 0x0f) != HTHSCROLL) goto END;
	scrollbar = SB_HORZ;
    }
    else  /* SC_VSCROLL */
    {
	if ((wParam & 0x0f) != HTVSCROLL) goto END;
	scrollbar = SB_VERT;
    }

    if (!(msg = SEGPTR_NEW(MSG16))) goto END;
    pt.x -= wndPtr->rectWindow.left;
    pt.y -= wndPtr->rectWindow.top;
    SetCapture( hwnd );
    SCROLL_HandleScrollEvent( hwnd, scrollbar, WM_LBUTTONDOWN, pt );

    do
    {
        GetMessage16( SEGPTR_GET(msg), 0, 0, 0 );
	switch(msg->message)
	{
	case WM_LBUTTONUP:
	case WM_MOUSEMOVE:
        case WM_SYSTIMER:
            pt.x = LOWORD(msg->lParam) + wndPtr->rectClient.left - 
	      wndPtr->rectWindow.left;
            pt.y = HIWORD(msg->lParam) + wndPtr->rectClient.top - 
	      wndPtr->rectWindow.top;
            SCROLL_HandleScrollEvent( hwnd, scrollbar, msg->message, pt );
	    break;
        default:
            TranslateMessage16( msg );
            DispatchMessage16( msg );
            break;
	}
        if (!IsWindow( hwnd ))
        {
            ReleaseCapture();
            break;
        }
    } while (msg->message != WM_LBUTTONUP);
    SEGPTR_FREE(msg);
END:
    WIN_ReleaseWndPtr(wndPtr);
}

/***********************************************************************
 *           NC_HandleNCLButtonDown
 *
 * Handle a WM_NCLBUTTONDOWN message. Called from DefWindowProc().
 */
LONG NC_HandleNCLButtonDown( WND* pWnd, WPARAM16 wParam, LPARAM lParam )
{
    HWND hwnd = pWnd->hwndSelf;

    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:
	 hwnd = WIN_GetTopParent(hwnd);

	 if( WINPOS_SetActiveWindow(hwnd, TRUE, TRUE) || (GetActiveWindow() == hwnd) )
		SendMessage16( pWnd->hwndSelf, WM_SYSCOMMAND, SC_MOVE + HTCAPTION, lParam );
	 break;

    case HTSYSMENU:
	 if( pWnd->dwStyle & WS_SYSMENU )
	 {
	     if( !(pWnd->dwStyle & WS_MINIMIZE) )
	     {
		HDC hDC = GetWindowDC(hwnd);
		if (TWEAK_WineLook == WIN31_LOOK)
		    NC_DrawSysButton( hwnd, hDC, TRUE );
		else
		    NC_DrawSysButton95( hwnd, hDC, TRUE );
		ReleaseDC( hwnd, hDC );
	     }
	     SendMessage16( hwnd, WM_SYSCOMMAND, SC_MOUSEMENU + HTSYSMENU, lParam );
	 }
	 break;

    case HTMENU:
	SendMessage16( hwnd, WM_SYSCOMMAND, SC_MOUSEMENU, lParam );
	break;

    case HTHSCROLL:
	SendMessage16( hwnd, WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL, lParam );
	break;

    case HTVSCROLL:
	SendMessage16( hwnd, WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL, lParam );
	break;

    case HTMINBUTTON:
    case HTMAXBUTTON:
	NC_TrackMinMaxBox( hwnd, wParam );
	break;

    case HTCLOSE:
	if (TWEAK_WineLook >= WIN95_LOOK)
	    NC_TrackCloseButton95 (hwnd, wParam);
	break;

    case HTLEFT:
    case HTRIGHT:
    case HTTOP:
    case HTTOPLEFT:
    case HTTOPRIGHT:
    case HTBOTTOM:
    case HTBOTTOMLEFT:
    case HTBOTTOMRIGHT:
	/* make sure hittest fits into 0xf and doesn't overlap with HTSYSMENU */
	SendMessage16( hwnd, WM_SYSCOMMAND, SC_SIZE + wParam - 2, lParam);
	break;

    case HTBORDER:
	break;
    }
    return 0;
}


/***********************************************************************
 *           NC_HandleNCLButtonDblClk
 *
 * Handle a WM_NCLBUTTONDBLCLK message. Called from DefWindowProc().
 */
LONG NC_HandleNCLButtonDblClk( WND *pWnd, WPARAM16 wParam, LPARAM lParam )
{
    /*
     * if this is an icon, send a restore since we are handling
     * a double click
     */
    if (pWnd->dwStyle & WS_MINIMIZE)
    {
        SendMessage16( pWnd->hwndSelf, WM_SYSCOMMAND, SC_RESTORE, lParam );
        return 0;
    } 

    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:
        /* stop processing if WS_MAXIMIZEBOX is missing */
        if (pWnd->dwStyle & WS_MAXIMIZEBOX)
            SendMessage16( pWnd->hwndSelf, WM_SYSCOMMAND,
                      (pWnd->dwStyle & WS_MAXIMIZE) ? SC_RESTORE : SC_MAXIMIZE,
                      lParam );
	break;

    case HTSYSMENU:
        if (!(pWnd->class->style & CS_NOCLOSE))
            SendMessage16( pWnd->hwndSelf, WM_SYSCOMMAND, SC_CLOSE, lParam );
	break;

    case HTHSCROLL:
	SendMessage16( pWnd->hwndSelf, WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL,
		       lParam );
	break;

    case HTVSCROLL:
	SendMessage16( pWnd->hwndSelf, WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL,
		       lParam );
	break;
    }
    return 0;
}


/***********************************************************************
 *           NC_HandleSysCommand
 *
 * Handle a WM_SYSCOMMAND message. Called from DefWindowProc().
 */
LONG NC_HandleSysCommand( HWND hwnd, WPARAM16 wParam, POINT16 pt )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    POINT pt32;
    UINT16 uCommand = wParam & 0xFFF0;

    TRACE(nonclient, "Handling WM_SYSCOMMAND %x %d,%d\n", 
		      wParam, pt.x, pt.y );

    if (wndPtr->dwStyle & WS_CHILD && uCommand != SC_KEYMENU )
        ScreenToClient16( wndPtr->parent->hwndSelf, &pt );

    switch (uCommand)
    {
    case SC_SIZE:
    case SC_MOVE:
	NC_DoSizeMove( hwnd, wParam );
	break;

    case SC_MINIMIZE:
	ShowWindow( hwnd, SW_MINIMIZE ); 
	break;

    case SC_MAXIMIZE:
	ShowWindow( hwnd, SW_MAXIMIZE );
	break;

    case SC_RESTORE:
	ShowWindow( hwnd, SW_RESTORE );
	break;

    case SC_CLOSE:
	return SendMessage16( hwnd, WM_CLOSE, 0, 0 );

    case SC_VSCROLL:
    case SC_HSCROLL:
        CONV_POINT16TO32( &pt, &pt32 );
	NC_TrackScrollBar( hwnd, wParam, pt32 );
	break;

    case SC_MOUSEMENU:
        CONV_POINT16TO32( &pt, &pt32 );
        MENU_TrackMouseMenuBar( wndPtr, wParam & 0x000F, pt32 );
	break;

    case SC_KEYMENU:
	MENU_TrackKbdMenuBar( wndPtr , wParam , pt.x );
	break;
	
    case SC_TASKLIST:
	WinExec( "taskman.exe", SW_SHOWNORMAL ); 
	break;

    case SC_SCREENSAVE:
	if (wParam == SC_ABOUTWINE)
            ShellAboutA(hwnd,"Wine", WINE_RELEASE_INFO, 0);
	else 
	  if (wParam == SC_PUTMARK)
            TRACE(shell,"Mark requested by user\n");
	break;
  
    case SC_HOTKEY:
    case SC_ARRANGE:
    case SC_NEXTWINDOW:
    case SC_PREVWINDOW:
 	FIXME (nonclient, "unimplemented!\n");
        break;
    }
    WIN_ReleaseWndPtr(wndPtr);
    return 0;
}
