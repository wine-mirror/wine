/*
 * Non-client area window functions
 *
 * Copyright 1994 Alexandre Julliard
 *
 */

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "version.h"
#include "win.h"
#include "message.h"
#include "user.h"
#include "heap.h"
#include "dce.h"
#include "controls.h"
#include "cursoricon.h"
#include "winpos.h"
#include "hook.h"
#include "nonclient.h"
#include "debugtools.h"
#include "shellapi.h"
#include "bitmap.h"

DEFAULT_DEBUG_CHANNEL(nonclient);
DECLARE_DEBUG_CHANNEL(shell);

BOOL NC_DrawGrayButton(HDC hdc, int x, int y);

static HBITMAP hbitmapClose;
static HBITMAP hbitmapMinimize;
static HBITMAP hbitmapMinimizeD;
static HBITMAP hbitmapMaximize;
static HBITMAP hbitmapMaximizeD;
static HBITMAP hbitmapRestore;
static HBITMAP hbitmapRestoreD;

static const BYTE lpGrayMask[] = { 0xAA, 0xA0,
		      0x55, 0x50,  
		      0xAA, 0xA0,
		      0x55, 0x50,  
		      0xAA, 0xA0,
		      0x55, 0x50,  
		      0xAA, 0xA0,
		      0x55, 0x50,  
		      0xAA, 0xA0,
		      0x55, 0x50};

#define SC_ABOUTWINE    	(SC_SCREENSAVE+1)
#define SC_PUTMARK     		(SC_SCREENSAVE+2)

  /* Some useful macros */
#define HAS_DLGFRAME(style,exStyle) \
    (((exStyle) & WS_EX_DLGMODALFRAME) || \
     (((style) & WS_DLGFRAME) && !((style) & WS_THICKFRAME)))

#define HAS_THICKFRAME(style,exStyle) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define HAS_THINFRAME(style) \
    (((style) & WS_BORDER) || !((style) & (WS_CHILD | WS_POPUP)))

#define HAS_BIGFRAME(style,exStyle) \
    (((style) & (WS_THICKFRAME | WS_DLGFRAME)) || \
     ((exStyle) & WS_EX_DLGMODALFRAME))

#define HAS_STATICOUTERFRAME(style,exStyle) \
    (((exStyle) & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) == \
     WS_EX_STATICEDGE)

#define HAS_ANYFRAME(style,exStyle) \
    (((style) & (WS_THICKFRAME | WS_DLGFRAME | WS_BORDER)) || \
     ((exStyle) & WS_EX_DLGMODALFRAME) || \
     !((style) & (WS_CHILD | WS_POPUP)))

#define HAS_MENU(w)  (!((w)->dwStyle & WS_CHILD) && ((w)->wIDmenu != 0))


/***********************************************************************
 *           NC_AdjustRect
 *
 * Compute the size of the window rectangle from the size of the
 * client rectangle.
 */
static void NC_AdjustRect( LPRECT rect, DWORD style, BOOL menu, DWORD exStyle )
{
    if (TWEAK_WineLook > WIN31_LOOK)
	ERR("Called in Win95 mode. Aiee! Please report this.\n" );

    if(style & WS_ICONIC) return;

    if (HAS_THICKFRAME( style, exStyle ))
        InflateRect( rect, GetSystemMetrics(SM_CXFRAME), GetSystemMetrics(SM_CYFRAME) );
    else if (HAS_DLGFRAME( style, exStyle ))
        InflateRect( rect, GetSystemMetrics(SM_CXDLGFRAME), GetSystemMetrics(SM_CYDLGFRAME) );
    else if (HAS_THINFRAME( style ))
        InflateRect( rect, GetSystemMetrics(SM_CXBORDER), GetSystemMetrics(SM_CYBORDER));

    if ((style & WS_CAPTION) == WS_CAPTION)
        rect->top -= GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYBORDER);

    if (menu) rect->top -= GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYBORDER);

    if (style & WS_VSCROLL) {
      rect->right  += GetSystemMetrics(SM_CXVSCROLL) - 1;
      if(!HAS_ANYFRAME( style, exStyle ))
	 rect->right++;
    }

    if (style & WS_HSCROLL) {
      rect->bottom += GetSystemMetrics(SM_CYHSCROLL) - 1;
      if(!HAS_ANYFRAME( style, exStyle ))
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
 *     BOOL  menu
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
 *     28-Jul-1999 Ove Kåven (ovek@arcticnet.no)
 *        Streamlined window style checks.
 *
 *****************************************************************************/

static void
NC_AdjustRectOuter95 (LPRECT rect, DWORD style, BOOL menu, DWORD exStyle)
{
    int adjust;
    if(style & WS_ICONIC) return;

    if ((exStyle & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) == 
        WS_EX_STATICEDGE)
    {
        adjust = 1; /* for the outer frame always present */
    }
    else
    {
        adjust = 0;
        if ((exStyle & WS_EX_DLGMODALFRAME) ||
            (style & (WS_THICKFRAME|WS_DLGFRAME))) adjust = 2; /* outer */
    }
    if (style & WS_THICKFRAME)
        adjust +=  ( GetSystemMetrics (SM_CXFRAME)
                   - GetSystemMetrics (SM_CXDLGFRAME)); /* The resize border */
    if ((style & (WS_BORDER|WS_DLGFRAME)) || 
        (exStyle & WS_EX_DLGMODALFRAME))
        adjust++; /* The other border */

    InflateRect (rect, adjust, adjust);

    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        if (exStyle & WS_EX_TOOLWINDOW)
            rect->top -= GetSystemMetrics(SM_CYSMCAPTION);
        else
            rect->top -= GetSystemMetrics(SM_CYCAPTION);
    }
    if (menu) rect->top -= GetSystemMetrics(SM_CYMENU);
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
NC_AdjustRectInner95 (LPRECT rect, DWORD style, DWORD exStyle)
{
    if(style & WS_ICONIC) return;

    if (exStyle & WS_EX_CLIENTEDGE)
        InflateRect(rect, GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));

    if (style & WS_VSCROLL) rect->right  += GetSystemMetrics(SM_CXVSCROLL);
    if (style & WS_HSCROLL) rect->bottom += GetSystemMetrics(SM_CYHSCROLL);
}



static HICON NC_IconForWindow( HWND hwnd )
{
    HICON hIcon = (HICON) GetClassLongA( hwnd, GCL_HICONSM );
    if (!hIcon) hIcon = (HICON) GetClassLongA( hwnd, GCL_HICON );

    /* If there is no hIcon specified and this is a modal dialog,
     * get the default one.
     */
    if (!hIcon && (GetWindowLongA( hwnd, GWL_STYLE ) & DS_MODALFRAME))
        hIcon = LoadImageA(0, IDI_WINLOGOA, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    return hIcon;
}

/***********************************************************************
 *		DrawCaption (USER.660) Draws a caption bar
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
 *		DrawCaption (USER32.@) Draws a caption bar
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
 *		DrawCaptionTemp (USER.657)
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
 *		DrawCaptionTempA (USER32.@)
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

    TRACE("(%08x,%08x,%p,%08x,%08x,\"%s\",%08x)\n",
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
	pt.y = (rc.bottom + rc.top - GetSystemMetrics(SM_CYSMICON)) / 2;

        if (!hIcon) hIcon = NC_IconForWindow(hwnd);
        DrawIconEx (hdc, pt.x, pt.y, hIcon, GetSystemMetrics(SM_CXSMICON),
                    GetSystemMetrics(SM_CYSMICON), 0, 0, DI_NORMAL);
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
	FIXME("undocumented flag (0x2000)!\n");

    return 0;
}


/***********************************************************************
 *		DrawCaptionTempW (USER32.@)
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
 *		AdjustWindowRect (USER.102)
 */
BOOL16 WINAPI AdjustWindowRect16( LPRECT16 rect, DWORD style, BOOL16 menu )
{
    return AdjustWindowRectEx16( rect, style, menu, 0 );
}


/***********************************************************************
 *		AdjustWindowRect (USER32.@)
 */
BOOL WINAPI AdjustWindowRect( LPRECT rect, DWORD style, BOOL menu )
{
    return AdjustWindowRectEx( rect, style, menu, 0 );
}


/***********************************************************************
 *		AdjustWindowRectEx (USER.454)
 */
BOOL16 WINAPI AdjustWindowRectEx16( LPRECT16 rect, DWORD style,
                                    BOOL16 menu, DWORD exStyle )
{
    RECT rect32;
    BOOL ret;

    CONV_RECT16TO32( rect, &rect32 );
    ret = AdjustWindowRectEx( &rect32, style, menu, exStyle );
    CONV_RECT32TO16( &rect32, rect );
    return ret;
}


/***********************************************************************
 *		AdjustWindowRectEx (USER32.@)
 */
BOOL WINAPI AdjustWindowRectEx( LPRECT rect, DWORD style, BOOL menu, DWORD exStyle )
{
    /* Correct the window style */

    if (!(style & (WS_POPUP | WS_CHILD))) style |= WS_CAPTION; /* Overlapped window */
    style &= (WS_DLGFRAME | WS_BORDER | WS_THICKFRAME | WS_CHILD);
    exStyle &= (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE |
                WS_EX_STATICEDGE | WS_EX_TOOLWINDOW);
    if (exStyle & WS_EX_DLGMODALFRAME) style &= ~WS_THICKFRAME;

    TRACE("(%d,%d)-(%d,%d) %08lx %d %08lx\n",
          rect->left, rect->top, rect->right, rect->bottom,
          style, menu, exStyle );

    if (TWEAK_WineLook == WIN31_LOOK)
        NC_AdjustRect( rect, style, menu, exStyle );
    else
    {
        NC_AdjustRectOuter95( rect, style, menu, exStyle );
        NC_AdjustRectInner95( rect, style, exStyle );
    }
    return TRUE;
}


/***********************************************************************
 *           NC_HandleNCCalcSize
 *
 * Handle a WM_NCCALCSIZE message. Called from DefWindowProc().
 */
LONG NC_HandleNCCalcSize( WND *pWnd, RECT *winRect )
{
    RECT tmpRect = { 0, 0, 0, 0 };
    LONG result = 0;
    UINT style = (UINT) GetClassLongA(pWnd->hwndSelf, GCL_STYLE);

    if (style & CS_VREDRAW) result |= WVR_VREDRAW;
    if (style & CS_HREDRAW) result |= WVR_HREDRAW;

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
	    TRACE("Calling GetMenuBarHeight with HWND 0x%x, width %d, "
                  "at (%d, %d).\n", pWnd->hwndSelf,
                  winRect->right - winRect->left,
                  -tmpRect.left, -tmpRect.top );

	    winRect->top +=
		MENU_GetMenuBarHeight( pWnd->hwndSelf,
				       winRect->right - winRect->left,
				       -tmpRect.left, -tmpRect.top ) + 1;
	}

	if (TWEAK_WineLook > WIN31_LOOK) {
	    SetRect(&tmpRect, 0, 0, 0, 0);
	    NC_AdjustRectInner95 (&tmpRect, pWnd->dwStyle, pWnd->dwExStyle);
	    winRect->left   -= tmpRect.left;
	    winRect->top    -= tmpRect.top;
	    winRect->right  -= tmpRect.right;
	    winRect->bottom -= tmpRect.bottom;
	}

        if (winRect->top > winRect->bottom)
            winRect->bottom = winRect->top;

        if (winRect->left > winRect->right)
            winRect->right = winRect->left;
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
void NC_GetInsideRect( HWND hwnd, RECT *rect )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    rect->top    = rect->left = 0;
    rect->right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    rect->bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

    if (wndPtr->dwStyle & WS_ICONIC) goto END;

    /* Remove frame from rectangle */
    if (HAS_THICKFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
    {
        InflateRect( rect, -GetSystemMetrics(SM_CXFRAME), -GetSystemMetrics(SM_CYFRAME) );
    }
    else if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
    {
        InflateRect( rect, -GetSystemMetrics(SM_CXDLGFRAME), -GetSystemMetrics(SM_CYDLGFRAME));
        /* FIXME: this isn't in NC_AdjustRect? why not? */
        if ((TWEAK_WineLook == WIN31_LOOK) && (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME))
            InflateRect( rect, -1, 0 );
    }
    else if (HAS_THINFRAME( wndPtr->dwStyle ))
    {
        InflateRect( rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER) );
    }

    /* We have additional border information if the window
     * is a child (but not an MDI child) */
    if (TWEAK_WineLook != WIN31_LOOK)
    {
        if ( (wndPtr->dwStyle & WS_CHILD)  &&
             ( (wndPtr->dwExStyle & WS_EX_MDICHILD) == 0 ) )
        {
            if (wndPtr->dwExStyle & WS_EX_CLIENTEDGE)
                InflateRect (rect, -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYEDGE));
            if (wndPtr->dwExStyle & WS_EX_STATICEDGE)
                InflateRect (rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER));
        }
    }

END:
    WIN_ReleaseWndPtr(wndPtr);
    return;
}


/***********************************************************************
 * NC_DoNCHitTest
 *
 * Handle a WM_NCHITTEST message. Called from NC_HandleNCHitTest().
 */

static LONG NC_DoNCHitTest (WND *wndPtr, POINT pt )
{
    RECT rect;

    TRACE("hwnd=%04x pt=%ld,%ld\n", wndPtr->hwndSelf, pt.x, pt.y );

    GetWindowRect(wndPtr->hwndSelf, &rect );
    if (!PtInRect( &rect, pt )) return HTNOWHERE;

    if (wndPtr->dwStyle & WS_MINIMIZE) return HTCAPTION;

    /* Check borders */
    if (HAS_THICKFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
    {
        InflateRect( &rect, -GetSystemMetrics(SM_CXFRAME), -GetSystemMetrics(SM_CYFRAME) );
        if (!PtInRect( &rect, pt ))
        {
            /* Check top sizing border */
            if (pt.y < rect.top)
            {
                if (pt.x < rect.left+GetSystemMetrics(SM_CXSIZE)) return HTTOPLEFT;
                if (pt.x >= rect.right-GetSystemMetrics(SM_CXSIZE)) return HTTOPRIGHT;
                return HTTOP;
            }
            /* Check bottom sizing border */
            if (pt.y >= rect.bottom)
            {
                if (pt.x < rect.left+GetSystemMetrics(SM_CXSIZE)) return HTBOTTOMLEFT;
                if (pt.x >= rect.right-GetSystemMetrics(SM_CXSIZE)) return HTBOTTOMRIGHT;
                return HTBOTTOM;
            }
            /* Check left sizing border */
            if (pt.x < rect.left)
            {
                if (pt.y < rect.top+GetSystemMetrics(SM_CYSIZE)) return HTTOPLEFT;
                if (pt.y >= rect.bottom-GetSystemMetrics(SM_CYSIZE)) return HTBOTTOMLEFT;
                return HTLEFT;
            }
            /* Check right sizing border */
            if (pt.x >= rect.right)
            {
                if (pt.y < rect.top+GetSystemMetrics(SM_CYSIZE)) return HTTOPRIGHT;
                if (pt.y >= rect.bottom-GetSystemMetrics(SM_CYSIZE)) return HTBOTTOMRIGHT;
                return HTRIGHT;
            }
        }
    }
    else  /* No thick frame */
    {
        if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
            InflateRect(&rect, -GetSystemMetrics(SM_CXDLGFRAME), -GetSystemMetrics(SM_CYDLGFRAME));
        else if (HAS_THINFRAME( wndPtr->dwStyle ))
            InflateRect(&rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER));
        if (!PtInRect( &rect, pt )) return HTBORDER;
    }

    /* Check caption */

    if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
    {
        rect.top += GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYBORDER);
        if (!PtInRect( &rect, pt ))
        {
            /* Check system menu */
            if ((wndPtr->dwStyle & WS_SYSMENU) && !(wndPtr->dwExStyle & WS_EX_TOOLWINDOW))
                rect.left += GetSystemMetrics(SM_CXSIZE);
            if (pt.x <= rect.left) return HTSYSMENU;

            /* Check maximize box */
            if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
                rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;

            if (pt.x >= rect.right) return HTMAXBUTTON;
            /* Check minimize box */
            if (wndPtr->dwStyle & WS_MINIMIZEBOX)
                rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
            if (pt.x >= rect.right) return HTMINBUTTON;
            return HTCAPTION;
        }
    }

      /* Check client area */

    ScreenToClient( wndPtr->hwndSelf, &pt );
    GetClientRect( wndPtr->hwndSelf, &rect );
    if (PtInRect( &rect, pt )) return HTCLIENT;

      /* Check vertical scroll bar */

    if (wndPtr->dwStyle & WS_VSCROLL)
    {
	rect.right += GetSystemMetrics(SM_CXVSCROLL);
	if (PtInRect( &rect, pt )) return HTVSCROLL;
    }

      /* Check horizontal scroll bar */

    if (wndPtr->dwStyle & WS_HSCROLL)
    {
	rect.bottom += GetSystemMetrics(SM_CYHSCROLL);
	if (PtInRect( &rect, pt ))
	{
	      /* Check size box */
	    if ((wndPtr->dwStyle & WS_VSCROLL) &&
		(pt.x >= rect.right - GetSystemMetrics(SM_CXVSCROLL)))
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

    /* Has to return HTNOWHERE if nothing was found  
       Could happen when a window has a customized non client area */
    return HTNOWHERE;  
}


/***********************************************************************
 * NC_DoNCHitTest95
 *
 * Handle a WM_NCHITTEST message. Called from NC_HandleNCHitTest().
 *
 * FIXME:  Just a modified copy of the Win 3.1 version.
 */

static LONG NC_DoNCHitTest95 (WND *wndPtr, POINT pt )
{
    RECT rect;

    TRACE("hwnd=%04x pt=%ld,%ld\n", wndPtr->hwndSelf, pt.x, pt.y );

    GetWindowRect(wndPtr->hwndSelf, &rect );
    if (!PtInRect( &rect, pt )) return HTNOWHERE;

    if (wndPtr->dwStyle & WS_MINIMIZE) return HTCAPTION;

    /* Check borders */
    if (HAS_THICKFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
    {
        InflateRect( &rect, -GetSystemMetrics(SM_CXFRAME), -GetSystemMetrics(SM_CYFRAME) );
        if (!PtInRect( &rect, pt ))
        {
            /* Check top sizing border */
            if (pt.y < rect.top)
            {
                if (pt.x < rect.left+GetSystemMetrics(SM_CXSIZE)) return HTTOPLEFT;
                if (pt.x >= rect.right-GetSystemMetrics(SM_CXSIZE)) return HTTOPRIGHT;
                return HTTOP;
            }
            /* Check bottom sizing border */
            if (pt.y >= rect.bottom)
            {
                if (pt.x < rect.left+GetSystemMetrics(SM_CXSIZE)) return HTBOTTOMLEFT;
                if (pt.x >= rect.right-GetSystemMetrics(SM_CXSIZE)) return HTBOTTOMRIGHT;
                return HTBOTTOM;
            }
            /* Check left sizing border */
            if (pt.x < rect.left)
            {
                if (pt.y < rect.top+GetSystemMetrics(SM_CYSIZE)) return HTTOPLEFT;
                if (pt.y >= rect.bottom-GetSystemMetrics(SM_CYSIZE)) return HTBOTTOMLEFT;
                return HTLEFT;
            }
            /* Check right sizing border */
            if (pt.x >= rect.right)
            {
                if (pt.y < rect.top+GetSystemMetrics(SM_CYSIZE)) return HTTOPRIGHT;
                if (pt.y >= rect.bottom-GetSystemMetrics(SM_CYSIZE)) return HTBOTTOMRIGHT;
                return HTRIGHT;
            }
        }
    }
    else  /* No thick frame */
    {
        if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
            InflateRect(&rect, -GetSystemMetrics(SM_CXDLGFRAME), -GetSystemMetrics(SM_CYDLGFRAME));
        else if (HAS_THINFRAME( wndPtr->dwStyle ))
            InflateRect(&rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER));
        if (!PtInRect( &rect, pt )) return HTBORDER;
    }

    /* Check caption */

    if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
    {
        if (wndPtr->dwExStyle & WS_EX_TOOLWINDOW)
            rect.top += GetSystemMetrics(SM_CYSMCAPTION) - 1;
        else
            rect.top += GetSystemMetrics(SM_CYCAPTION) - 1;
        if (!PtInRect( &rect, pt ))
        {
            /* Check system menu */
            if ((wndPtr->dwStyle & WS_SYSMENU) && !(wndPtr->dwExStyle & WS_EX_TOOLWINDOW))
            {
                if (NC_IconForWindow(wndPtr->hwndSelf))
                    rect.left += GetSystemMetrics(SM_CYCAPTION) - 1;
            }
            if (pt.x < rect.left) return HTSYSMENU;

            /* Check close button */
            if (wndPtr->dwStyle & WS_SYSMENU)
                rect.right -= GetSystemMetrics(SM_CYCAPTION) - 1;
            if (pt.x > rect.right) return HTCLOSE;

            /* Check maximize box */
            /* In win95 there is automatically a Maximize button when there is a minimize one*/
            if ((wndPtr->dwStyle & WS_MAXIMIZEBOX)|| (wndPtr->dwStyle & WS_MINIMIZEBOX))
                rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
            if (pt.x > rect.right) return HTMAXBUTTON;

            /* Check minimize box */
            /* In win95 there is automatically a Maximize button when there is a Maximize one*/
            if ((wndPtr->dwStyle & WS_MINIMIZEBOX)||(wndPtr->dwStyle & WS_MAXIMIZEBOX))
                rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;

            if (pt.x > rect.right) return HTMINBUTTON;
            return HTCAPTION;
        }
    }

      /* Check client area */

    ScreenToClient( wndPtr->hwndSelf, &pt );
    GetClientRect( wndPtr->hwndSelf, &rect );
    if (PtInRect( &rect, pt )) return HTCLIENT;

      /* Check vertical scroll bar */

    if (wndPtr->dwStyle & WS_VSCROLL)
    {
	rect.right += GetSystemMetrics(SM_CXVSCROLL);
	if (PtInRect( &rect, pt )) return HTVSCROLL;
    }

      /* Check horizontal scroll bar */

    if (wndPtr->dwStyle & WS_HSCROLL)
    {
	rect.bottom += GetSystemMetrics(SM_CYHSCROLL);
	if (PtInRect( &rect, pt ))
	{
	      /* Check size box */
	    if ((wndPtr->dwStyle & WS_VSCROLL) &&
		(pt.x >= rect.right - GetSystemMetrics(SM_CXVSCROLL)))
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

    /* Has to return HTNOWHERE if nothing was found  
       Could happen when a window has a customized non client area */
    return HTNOWHERE;
}


/***********************************************************************
 * NC_HandleNCHitTest
 *
 * Handle a WM_NCHITTEST message. Called from DefWindowProc().
 */
LONG NC_HandleNCHitTest (HWND hwnd , POINT pt)
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

    NC_GetInsideRect( hwnd, &rect );
    hdcMem = CreateCompatibleDC( hdc );
    hbitmap = SelectObject( hdcMem, hbitmapClose );
    BitBlt(hdc, rect.left, rect.top, GetSystemMetrics(SM_CXSIZE), GetSystemMetrics(SM_CYSIZE),
           hdcMem, (GetWindowLongA(hwnd,GWL_STYLE) & WS_CHILD) ? GetSystemMetrics(SM_CXSIZE) : 0, 0,
           down ? NOTSRCCOPY : SRCCOPY );
    SelectObject( hdcMem, hbitmap );
    DeleteDC( hdcMem );
}


/***********************************************************************
 *           NC_DrawMaxButton
 */
static void NC_DrawMaxButton( HWND hwnd, HDC16 hdc, BOOL down )
{
    RECT rect;
    HDC hdcMem;

    NC_GetInsideRect( hwnd, &rect );
    hdcMem = CreateCompatibleDC( hdc );
    SelectObject( hdcMem,  (IsZoomed(hwnd)
                            ? (down ? hbitmapRestoreD : hbitmapRestore)
                            : (down ? hbitmapMaximizeD : hbitmapMaximize)) );
    BitBlt( hdc, rect.right - GetSystemMetrics(SM_CXSIZE) - 1, rect.top,
            GetSystemMetrics(SM_CXSIZE) + 1, GetSystemMetrics(SM_CYSIZE), hdcMem, 0, 0,
            SRCCOPY );
    DeleteDC( hdcMem );

}


/***********************************************************************
 *           NC_DrawMinButton
 */
static void NC_DrawMinButton( HWND hwnd, HDC16 hdc, BOOL down )
{
    RECT rect;
    HDC hdcMem;

    NC_GetInsideRect( hwnd, &rect );
    hdcMem = CreateCompatibleDC( hdc );
    SelectObject( hdcMem, (down ? hbitmapMinimizeD : hbitmapMinimize) );
    if (GetWindowLongA(hwnd,GWL_STYLE) & WS_MAXIMIZEBOX)
        rect.right -= GetSystemMetrics(SM_CXSIZE)+1;
    BitBlt( hdc, rect.right - GetSystemMetrics(SM_CXSIZE) - 1, rect.top,
            GetSystemMetrics(SM_CXSIZE) + 1, GetSystemMetrics(SM_CYSIZE), hdcMem, 0, 0,
            SRCCOPY );
    DeleteDC( hdcMem );
}


/******************************************************************************
 *
 *   void  NC_DrawSysButton95(
 *      HWND  hwnd,
 *      HDC  hdc,
 *      BOOL  down )
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
    HICON hIcon = NC_IconForWindow( hwnd );

    if (hIcon)
    {
        RECT rect;
        NC_GetInsideRect( hwnd, &rect );
        DrawIconEx (hdc, rect.left + 2, rect.top + 2, hIcon,
                    GetSystemMetrics(SM_CXSMICON),
                    GetSystemMetrics(SM_CYSMICON), 0, 0, DI_NORMAL);
    }
    return (hIcon != 0);
}


/******************************************************************************
 *
 *   void  NC_DrawCloseButton95(
 *      HWND  hwnd,
 *      HDC  hdc,
 *      BOOL  down,
 *      BOOL    bGrayed )
 *
 *   Draws the Win95 close button.
 *
 *   If bGrayed is true, then draw a disabled Close button
 *
 *   Revision history
 *        11-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *             Original implementation from NC_DrawSysButton95 source.
 *
 *****************************************************************************/

static void NC_DrawCloseButton95 (HWND hwnd, HDC hdc, BOOL down, BOOL bGrayed)
{
    RECT rect;

    NC_GetInsideRect( hwnd, &rect );

    /* A tool window has a smaller Close button */
    if (GetWindowLongA( hwnd, GWL_EXSTYLE ) & WS_EX_TOOLWINDOW)
    {
        INT iBmpHeight = 11; /* Windows does not use SM_CXSMSIZE and SM_CYSMSIZE   */
        INT iBmpWidth = 11;  /* it uses 11x11 for  the close button in tool window */
        INT iCaptionHeight = GetSystemMetrics(SM_CYSMCAPTION);

        rect.top = rect.top + (iCaptionHeight - 1 - iBmpHeight) / 2;
        rect.left = rect.right - (iCaptionHeight + 1 + iBmpWidth) / 2;
        rect.bottom = rect.top + iBmpHeight;
        rect.right = rect.left + iBmpWidth;
    }
    else
    {
        rect.left = rect.right - GetSystemMetrics(SM_CXSIZE) - 1;
        rect.bottom = rect.top + GetSystemMetrics(SM_CYSIZE) - 1;
        rect.top += 2;
        rect.right -= 2;
    }
    DrawFrameControl( hdc, &rect, DFC_CAPTION,
                      (DFCS_CAPTIONCLOSE |
                       (down ? DFCS_PUSHED : 0) |
                       (bGrayed ? DFCS_INACTIVE : 0)) );
}

/******************************************************************************
 *   NC_DrawMaxButton95
 *
 *   Draws the maximize button for Win95 style windows.
 *   If bGrayed is true, then draw a disabled Maximize button
 */
static void NC_DrawMaxButton95(HWND hwnd,HDC16 hdc,BOOL down, BOOL bGrayed)
{
    RECT rect;
    UINT flags = IsZoomed(hwnd) ? DFCS_CAPTIONRESTORE : DFCS_CAPTIONMAX;

    NC_GetInsideRect( hwnd, &rect );
    if (GetWindowLongA( hwnd, GWL_STYLE) & WS_SYSMENU)
        rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
    rect.left = rect.right - GetSystemMetrics(SM_CXSIZE);
    rect.bottom = rect.top + GetSystemMetrics(SM_CYSIZE) - 1;
    rect.top += 2;
    rect.right -= 2;
    if (down) flags |= DFCS_PUSHED;
    if (bGrayed) flags |= DFCS_INACTIVE;
    DrawFrameControl( hdc, &rect, DFC_CAPTION, flags );
}

/******************************************************************************
 *   NC_DrawMinButton95
 *
 *   Draws the minimize button for Win95 style windows.
 *   If bGrayed is true, then draw a disabled Minimize button
 */
static void  NC_DrawMinButton95(HWND hwnd,HDC16 hdc,BOOL down, BOOL bGrayed)
{
    RECT rect;
    UINT flags = DFCS_CAPTIONMIN;
    DWORD style = GetWindowLongA( hwnd, GWL_STYLE );

    NC_GetInsideRect( hwnd, &rect );
    if (style & WS_SYSMENU)
        rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
    if (style & (WS_MAXIMIZEBOX|WS_MINIMIZEBOX))
        rect.right -= GetSystemMetrics(SM_CXSIZE) - 2;
    rect.left = rect.right - GetSystemMetrics(SM_CXSIZE);
    rect.bottom = rect.top + GetSystemMetrics(SM_CYSIZE) - 1;
    rect.top += 2;
    rect.right -= 2;
    if (down) flags |= DFCS_PUSHED;
    if (bGrayed) flags |= DFCS_INACTIVE;
    DrawFrameControl( hdc, &rect, DFC_CAPTION, flags );
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
	ERR("Called in Win95 mode. Aiee! Please report this.\n" );

    if (dlgFrame)
    {
	width = GetSystemMetrics(SM_CXDLGFRAME) - 1;
	height = GetSystemMetrics(SM_CYDLGFRAME) - 1;
        SelectObject( hdc, GetSysColorBrush(active ? COLOR_ACTIVECAPTION :
						COLOR_INACTIVECAPTION) );
    }
    else
    {
	width = GetSystemMetrics(SM_CXFRAME) - 2;
	height = GetSystemMetrics(SM_CYFRAME) - 2;
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
        INT decYOff = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXSIZE) - 1;
	INT decXOff = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYSIZE) - 1;

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
 *      HDC  hdc,
 *      RECT  *rect,
 *      BOOL  active,
 *      DWORD style,
 *      DWORD exStyle )
 *
 *   Draw a window frame inside the given rectangle, and update the rectangle.
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
 *        29-Jun-1999 Ove Kåven (ovek@arcticnet.no)
 *             Fixed a fix or something.
 *
 *****************************************************************************/

static void  NC_DrawFrame95(
    HDC  hdc,
    RECT  *rect,
    BOOL  active,
    DWORD style,
    DWORD exStyle)
{
    INT width, height;

    /* Firstly the "thick" frame */
    if (style & WS_THICKFRAME)
    {
	width = GetSystemMetrics(SM_CXFRAME) - GetSystemMetrics(SM_CXDLGFRAME);
	height = GetSystemMetrics(SM_CYFRAME) - GetSystemMetrics(SM_CYDLGFRAME);

        SelectObject( hdc, GetSysColorBrush(active ? COLOR_ACTIVEBORDER :
		      COLOR_INACTIVEBORDER) );
        /* Draw frame */
        PatBlt( hdc, rect->left, rect->top,
                  rect->right - rect->left, height, PATCOPY );
        PatBlt( hdc, rect->left, rect->top,
                  width, rect->bottom - rect->top, PATCOPY );
        PatBlt( hdc, rect->left, rect->bottom - 1,
                  rect->right - rect->left, -height, PATCOPY );
        PatBlt( hdc, rect->right - 1, rect->top,
                  -width, rect->bottom - rect->top, PATCOPY );

        InflateRect( rect, -width, -height );
    }

    /* Now the other bit of the frame */
    if ((style & (WS_BORDER|WS_DLGFRAME)) ||
        (exStyle & WS_EX_DLGMODALFRAME))
    {
        width = GetSystemMetrics(SM_CXDLGFRAME) - GetSystemMetrics(SM_CXEDGE);
        height = GetSystemMetrics(SM_CYDLGFRAME) - GetSystemMetrics(SM_CYEDGE);
        /* This should give a value of 1 that should also work for a border */

        SelectObject( hdc, GetSysColorBrush(
                      (exStyle & (WS_EX_DLGMODALFRAME|WS_EX_CLIENTEDGE)) ?
                          COLOR_3DFACE :
                      (exStyle & WS_EX_STATICEDGE) ?
                          COLOR_WINDOWFRAME :
                      (style & (WS_DLGFRAME|WS_THICKFRAME)) ?
                          COLOR_3DFACE :
                      /* else */
                          COLOR_WINDOWFRAME));

        /* Draw frame */
        PatBlt( hdc, rect->left, rect->top,
                  rect->right - rect->left, height, PATCOPY );
        PatBlt( hdc, rect->left, rect->top,
                  width, rect->bottom - rect->top, PATCOPY );
        PatBlt( hdc, rect->left, rect->bottom - 1,
                  rect->right - rect->left, -height, PATCOPY );
        PatBlt( hdc, rect->right - 1, rect->top,
                  -width, rect->bottom - rect->top, PATCOPY );

        InflateRect( rect, -width, -height );
    }
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
    char buffer[256];

    if (!hbitmapClose)
    {
	if (!(hbitmapClose = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_CLOSE) ))) return;
	hbitmapMinimize  = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_REDUCE) );
	hbitmapMinimizeD = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_REDUCED) );
	hbitmapMaximize  = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_ZOOM) );
	hbitmapMaximizeD = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_ZOOMD) );
	hbitmapRestore   = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_RESTORE) );
	hbitmapRestoreD  = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_RESTORED) );
    }
    
    if (GetWindowLongA( hwnd, GWL_EXSTYLE) & WS_EX_DLGMODALFRAME)
    {
        HBRUSH hbrushOld = SelectObject(hdc, GetSysColorBrush(COLOR_WINDOW) );
	PatBlt( hdc, r.left, r.top, 1, r.bottom-r.top+1,PATCOPY );
	PatBlt( hdc, r.right-1, r.top, 1, r.bottom-r.top+1, PATCOPY );
	PatBlt( hdc, r.left, r.top-1, r.right-r.left, 1, PATCOPY );
	r.left++;
	r.right--;
	SelectObject( hdc, hbrushOld );
    }
    MoveToEx( hdc, r.left, r.bottom, NULL );
    LineTo( hdc, r.right, r.bottom );

    if (style & WS_SYSMENU)
    {
	NC_DrawSysButton( hwnd, hdc, FALSE );
	r.left += GetSystemMetrics(SM_CXSIZE) + 1;
	MoveToEx( hdc, r.left - 1, r.top, NULL );
	LineTo( hdc, r.left - 1, r.bottom );
    }
    if (style & WS_MAXIMIZEBOX)
    {
	NC_DrawMaxButton( hwnd, hdc, FALSE );
	r.right -= GetSystemMetrics(SM_CXSIZE) + 1;
    }
    if (style & WS_MINIMIZEBOX)
    {
	NC_DrawMinButton( hwnd, hdc, FALSE );
	r.right -= GetSystemMetrics(SM_CXSIZE) + 1;
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
 *      HDC  hdc,
 *      RECT *rect,
 *      HWND hwnd,
 *      DWORD  style,
 *      BOOL active )
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
    char    buffer[256];
    HPEN  hPrevPen;
    HMENU hSysMenu;

    hPrevPen = SelectObject( hdc, GetSysColorPen(
                     ((exStyle & (WS_EX_STATICEDGE|WS_EX_CLIENTEDGE|
                                 WS_EX_DLGMODALFRAME)) == WS_EX_STATICEDGE) ?
                      COLOR_WINDOWFRAME : COLOR_3DFACE) );
    MoveToEx( hdc, r.left, r.bottom - 1, NULL );
    LineTo( hdc, r.right, r.bottom - 1 );
    SelectObject( hdc, hPrevPen );
    r.bottom--;

    FillRect( hdc, &r, GetSysColorBrush(active ? COLOR_ACTIVECAPTION :
					    COLOR_INACTIVECAPTION) );

    if ((style & WS_SYSMENU) && !(exStyle & WS_EX_TOOLWINDOW)) {
	if (NC_DrawSysButton95 (hwnd, hdc, FALSE))
	    r.left += GetSystemMetrics(SM_CYCAPTION) - 1;
    }

    if (style & WS_SYSMENU) 
    {
	UINT state;

	/* Go get the sysmenu */
	hSysMenu = GetSystemMenu(hwnd, FALSE);
	state = GetMenuState(hSysMenu, SC_CLOSE, MF_BYCOMMAND);

	/* Draw a grayed close button if disabled and a normal one if SC_CLOSE is not there */
	NC_DrawCloseButton95 (hwnd, hdc, FALSE, 
			      ((((state & MF_DISABLED) || (state & MF_GRAYED))) && (state != 0xFFFFFFFF)));
	r.right -= GetSystemMetrics(SM_CYCAPTION) - 1;

	if ((style & WS_MAXIMIZEBOX) || (style & WS_MINIMIZEBOX))
	{
	    /* In win95 the two buttons are always there */
	    /* But if the menu item is not in the menu they're disabled*/

	    NC_DrawMaxButton95( hwnd, hdc, FALSE, (!(style & WS_MAXIMIZEBOX)));
	    r.right -= GetSystemMetrics(SM_CXSIZE) + 1;
	    
	    NC_DrawMinButton95( hwnd, hdc, FALSE,  (!(style & WS_MINIMIZEBOX)));
	    r.right -= GetSystemMetrics(SM_CXSIZE) + 1;
	}
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
static void NC_DoNCPaint( WND* wndPtr, HRGN clip, BOOL suppress_menupaint )
{
    HDC hdc;
    RECT rect;
    BOOL active;
    HWND hwnd = wndPtr->hwndSelf;

    if ( wndPtr->dwStyle & WS_MINIMIZE ||
	!WIN_IsWindowDrawable( wndPtr, 0 )) return; /* Nothing to do */

    active  = wndPtr->flags & WIN_NCACTIVATED;

    TRACE("%04x %d\n", hwnd, active );

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

    if (HAS_ANYFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
    {
        SelectObject( hdc, GetStockObject(NULL_BRUSH) );
        Rectangle( hdc, 0, 0, rect.right, rect.bottom );
        InflateRect( &rect, -1, -1 );
    }

    if (HAS_THICKFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
        NC_DrawFrame(hdc, &rect, FALSE, active );
    else if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
        NC_DrawFrame( hdc, &rect, TRUE, active );

    if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
    {
        RECT r = rect;
        r.bottom = rect.top + GetSystemMetrics(SM_CYSIZE);
        rect.top += GetSystemMetrics(SM_CYSIZE) + GetSystemMetrics(SM_CYBORDER);
        NC_DrawCaption( hdc, &r, hwnd, wndPtr->dwStyle, active );
    }

    if (HAS_MENU(wndPtr))
    {
	RECT r = rect;
	r.bottom = rect.top + GetSystemMetrics(SM_CYMENU);  /* default height */
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
        r.left = r.right - GetSystemMetrics(SM_CXVSCROLL) + 1;
        r.top  = r.bottom - GetSystemMetrics(SM_CYHSCROLL) + 1;
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
 *      HRGN  clip,
 *      BOOL  suppress_menupaint )
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
 *        29-Jun-1999 Ove Kåven (ovek@arcticnet.no)
 *             Streamlined window style checks.
 *
 *****************************************************************************/

static void  NC_DoNCPaint95(
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

    TRACE("%04x %d\n", hwnd, active );

    /* MSDN docs are pretty idiotic here, they say app CAN use clipRgn in
       the call to GetDCEx implying that it is allowed not to use it either.
       However, the suggested GetDCEx(    , DCX_WINDOW | DCX_INTERSECTRGN)
       will cause clipRgn to be deleted after ReleaseDC().
       Now, how is the "system" supposed to tell what happened?
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

    if (HAS_STATICOUTERFRAME(wndPtr->dwStyle, wndPtr->dwExStyle)) {
        DrawEdge (hdc, &rect, BDR_SUNKENOUTER, BF_RECT | BF_ADJUST);
    }
    else if (HAS_BIGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle)) {
        DrawEdge (hdc, &rect, EDGE_RAISED, BF_RECT | BF_ADJUST);
    }

    NC_DrawFrame95(hdc, &rect, active, wndPtr->dwStyle, wndPtr->dwExStyle );

    if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
    {
        RECT  r = rect;
        if (wndPtr->dwExStyle & WS_EX_TOOLWINDOW) {
            r.bottom = rect.top + GetSystemMetrics(SM_CYSMCAPTION);
            rect.top += GetSystemMetrics(SM_CYSMCAPTION);
        }
        else {
            r.bottom = rect.top + GetSystemMetrics(SM_CYCAPTION);
            rect.top += GetSystemMetrics(SM_CYCAPTION);
        }
        if( !clip || IntersectRect( &rfuzz, &r, &rectClip ) )
            NC_DrawCaption95 (hdc, &r, hwnd, wndPtr->dwStyle,
                              wndPtr->dwExStyle, active);
    }

    if (HAS_MENU(wndPtr))
    {
	RECT r = rect;
	r.bottom = rect.top + GetSystemMetrics(SM_CYMENU);
	
	TRACE("Calling DrawMenuBar with rect (%d, %d)-(%d, %d)\n",
              r.left, r.top, r.right, r.bottom);

	rect.top += MENU_DrawMenuBar( hdc, &r, hwnd, suppress_menupaint ) + 1;
    }

    TRACE("After MenuBar, rect is (%d, %d)-(%d, %d).\n",
          rect.left, rect.top, rect.right, rect.bottom );

    if (wndPtr->dwExStyle & WS_EX_CLIENTEDGE)
	DrawEdge (hdc, &rect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);

    /* Draw the scroll-bars */

    if (wndPtr->dwStyle & WS_VSCROLL)
        SCROLL_DrawScrollBar( hwnd, hdc, SB_VERT, TRUE, TRUE );
    if (wndPtr->dwStyle & WS_HSCROLL)
        SCROLL_DrawScrollBar( hwnd, hdc, SB_HORZ, TRUE, TRUE );

    /* Draw the "size-box" */
    if ((wndPtr->dwStyle & WS_VSCROLL) && (wndPtr->dwStyle & WS_HSCROLL))
    {
        RECT r = rect;
        r.left = r.right - GetSystemMetrics(SM_CXVSCROLL) + 1;
        r.top  = r.bottom - GetSystemMetrics(SM_CYHSCROLL) + 1;
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
    /* Lotus Notes draws menu descriptions in the caption of its main
     * window. When it wants to restore original "system" view, it just
     * sends WM_NCACTIVATE message to itself. Any optimizations here in
     * attempt to minimize redrawings lead to a not restored caption.
     */
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
	    HICON16 hCursor = (HICON16) GetClassWord(hwnd, GCW_HCURSOR);
	    if(hCursor) {
		SetCursor16(hCursor);
		return TRUE;
	    }
            return FALSE;
	}

    case HTLEFT:
    case HTRIGHT:
	return (LONG)SetCursor( LoadCursorA( 0, IDC_SIZEWEA ) );

    case HTTOP:
    case HTBOTTOM:
	return (LONG)SetCursor( LoadCursorA( 0, IDC_SIZENSA ) );

    case HTTOPLEFT:
    case HTBOTTOMRIGHT:	
	return (LONG)SetCursor( LoadCursorA( 0, IDC_SIZENWSEA ) );

    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
	return (LONG)SetCursor( LoadCursorA( 0, IDC_SIZENESWA ) );
    }

    /* Default cursor: arrow */
    return (LONG)SetCursor( LoadCursorA( 0, IDC_ARROWA ) );
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
          NC_GetInsideRect( wndPtr->hwndSelf, rect );
  	  OffsetRect( rect, wndPtr->rectWindow.left, wndPtr->rectWindow.top);
  	  if (wndPtr->dwStyle & WS_CHILD)
     	      ClientToScreen( wndPtr->parent->hwndSelf, (POINT *)rect );
          if (TWEAK_WineLook == WIN31_LOOK) {
            rect->right = rect->left + GetSystemMetrics(SM_CXSIZE);
            rect->bottom = rect->top + GetSystemMetrics(SM_CYSIZE);
	  }
	  else {
            rect->right = rect->left + GetSystemMetrics(SM_CYCAPTION) - 1;
            rect->bottom = rect->top + GetSystemMetrics(SM_CYCAPTION) - 1;
	  }
      }
      return TRUE;
  }
  return FALSE;
}

/***********************************************************************
 *           NC_TrackMinMaxBox95
 *
 * Track a mouse button press on the minimize or maximize box.
 *
 * The big difference between 3.1 and 95 is the disabled button state.
 * In win95 the system button can be disabled, so it can ignore the mouse
 * event.
 *
 */
static void NC_TrackMinMaxBox95( HWND hwnd, WORD wParam )
{
    MSG msg;
    HDC hdc = GetWindowDC( hwnd );
    BOOL pressed = TRUE;
    UINT state;
    DWORD wndStyle = GetWindowLongA( hwnd, GWL_STYLE);
    HMENU hSysMenu = GetSystemMenu(hwnd, FALSE);

    void  (*paintButton)(HWND, HDC16, BOOL, BOOL);

    if (wParam == HTMINBUTTON)
    {
	/* If the style is not present, do nothing */
	if (!(wndStyle & WS_MINIMIZEBOX))
	    return;

	/* Check if the sysmenu item for minimize is there  */
	state = GetMenuState(hSysMenu, SC_MINIMIZE, MF_BYCOMMAND);
	
	paintButton = &NC_DrawMinButton95;
    }
    else
    {
	/* If the style is not present, do nothing */
	if (!(wndStyle & WS_MAXIMIZEBOX))
	    return;

	/* Check if the sysmenu item for maximize is there  */
	state = GetMenuState(hSysMenu, SC_MAXIMIZE, MF_BYCOMMAND);
	
	paintButton = &NC_DrawMaxButton95;
    }

    SetCapture( hwnd );

    (*paintButton)( hwnd, hdc, TRUE, FALSE);

    while(1)
    {
	BOOL oldstate = pressed;
        MSG_InternalGetMessage( &msg, 0, 0, WM_MOUSEFIRST, WM_MOUSELAST,
                                0, PM_REMOVE, FALSE, NULL );

	if(msg.message == WM_LBUTTONUP)
	    break;

	if(msg.message != WM_MOUSEMOVE)
	    continue;

	pressed = (NC_HandleNCHitTest( hwnd, msg.pt ) == wParam);
	if (pressed != oldstate)
	   (*paintButton)( hwnd, hdc, pressed, FALSE);
    }

    if(pressed)
	(*paintButton)(hwnd, hdc, FALSE, FALSE);

    ReleaseCapture();
    ReleaseDC( hwnd, hdc );

    /* If the item minimize or maximize of the sysmenu are not there */
    /* or if the style is not present, do nothing */
    if ((!pressed) || (state == 0xFFFFFFFF))
	return;

    if (wParam == HTMINBUTTON) 
        SendMessageA( hwnd, WM_SYSCOMMAND, SC_MINIMIZE, MAKELONG(msg.pt.x,msg.pt.y) );
    else
        SendMessageA( hwnd, WM_SYSCOMMAND,
                      IsZoomed(hwnd) ? SC_RESTORE:SC_MAXIMIZE, MAKELONG(msg.pt.x,msg.pt.y) );
}

/***********************************************************************
 *           NC_TrackMinMaxBox
 *
 * Track a mouse button press on the minimize or maximize box.
 */
static void NC_TrackMinMaxBox( HWND hwnd, WORD wParam )
{
    MSG msg;
    HDC hdc = GetWindowDC( hwnd );
    BOOL pressed = TRUE;
    void  (*paintButton)(HWND, HDC16, BOOL);

    SetCapture( hwnd );

    if (wParam == HTMINBUTTON)
	paintButton = &NC_DrawMinButton;
    else
	paintButton = &NC_DrawMaxButton;

    (*paintButton)( hwnd, hdc, TRUE);

    while(1)
    {
	BOOL oldstate = pressed;
        MSG_InternalGetMessage( &msg, 0, 0, WM_MOUSEFIRST, WM_MOUSELAST,
                                0, PM_REMOVE, FALSE, NULL );

	if(msg.message == WM_LBUTTONUP)
	    break;

	if(msg.message != WM_MOUSEMOVE)
	    continue;

	pressed = (NC_HandleNCHitTest( hwnd, msg.pt ) == wParam);
	if (pressed != oldstate)
	   (*paintButton)( hwnd, hdc, pressed);
    }

    if(pressed)
	(*paintButton)( hwnd, hdc, FALSE);

    ReleaseCapture();
    ReleaseDC( hwnd, hdc );

    if (!pressed) return;

    if (wParam == HTMINBUTTON) 
        SendMessageA( hwnd, WM_SYSCOMMAND, SC_MINIMIZE, MAKELONG(msg.pt.x,msg.pt.y) );
    else
        SendMessageA( hwnd, WM_SYSCOMMAND,
                      IsZoomed(hwnd) ? SC_RESTORE:SC_MAXIMIZE, MAKELONG(msg.pt.x,msg.pt.y) );
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
    HDC hdc;
    BOOL pressed = TRUE;
    HMENU hSysMenu = GetSystemMenu(hwnd, FALSE);
    UINT state;

    if(hSysMenu == 0)
	return;

    state = GetMenuState(hSysMenu, SC_CLOSE, MF_BYCOMMAND);
	    
    /* If the item close of the sysmenu is disabled or not there do nothing */
    if((state & MF_DISABLED) || (state & MF_GRAYED) || (state == 0xFFFFFFFF))
	return;

    hdc = GetWindowDC( hwnd );

    SetCapture( hwnd );

    NC_DrawCloseButton95 (hwnd, hdc, TRUE, FALSE);

    while(1)
    {
	BOOL oldstate = pressed;
        MSG_InternalGetMessage( &msg, 0, 0, WM_MOUSEFIRST, WM_MOUSELAST,
                                0, PM_REMOVE, FALSE, NULL );

	if(msg.message == WM_LBUTTONUP)
	    break;

	if(msg.message != WM_MOUSEMOVE)
	    continue;

	pressed = (NC_HandleNCHitTest( hwnd, msg.pt ) == wParam);
	if (pressed != oldstate)
	   NC_DrawCloseButton95 (hwnd, hdc, pressed, FALSE);
    }

    if(pressed)
	NC_DrawCloseButton95 (hwnd, hdc, FALSE, FALSE);

    ReleaseCapture();
    ReleaseDC( hwnd, hdc );
    if (!pressed) return;

    SendMessageA( hwnd, WM_SYSCOMMAND, SC_CLOSE, MAKELONG(msg.pt.x,msg.pt.y) );
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
	if (TWEAK_WineLook == WIN31_LOOK)
	    NC_TrackMinMaxBox( hwnd, wParam );
	else
	    NC_TrackMinMaxBox95( hwnd, wParam );	    
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
        if (!(GetClassWord(pWnd->hwndSelf, GCW_STYLE) & CS_NOCLOSE))
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
LONG NC_HandleSysCommand( HWND hwnd, WPARAM wParam, POINT pt )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    UINT16 uCommand = wParam & 0xFFF0;

    TRACE("Handling WM_SYSCOMMAND %x %ld,%ld\n", wParam, pt.x, pt.y );

    if (wndPtr->parent && (uCommand != SC_KEYMENU))
        ScreenToClient( wndPtr->parent->hwndSelf, &pt );

    switch (uCommand)
    {
    case SC_SIZE:
    case SC_MOVE:
        if (USER_Driver.pSysCommandSizeMove)
            USER_Driver.pSysCommandSizeMove( hwnd, wParam );
	break;

    case SC_MINIMIZE:
        if (hwnd == GetForegroundWindow())
            ShowOwnedPopups(hwnd,FALSE);
	ShowWindow( hwnd, SW_MINIMIZE ); 
	break;

    case SC_MAXIMIZE:
        if (IsIconic(hwnd) && hwnd == GetForegroundWindow())
            ShowOwnedPopups(hwnd,TRUE);
	ShowWindow( hwnd, SW_MAXIMIZE );
	break;

    case SC_RESTORE:
        if (IsIconic(hwnd) && hwnd == GetForegroundWindow())
            ShowOwnedPopups(hwnd,TRUE);
	ShowWindow( hwnd, SW_RESTORE );
	break;

    case SC_CLOSE:
        WIN_ReleaseWndPtr(wndPtr);
	return SendMessageA( hwnd, WM_CLOSE, 0, 0 );

    case SC_VSCROLL:
    case SC_HSCROLL:
	NC_TrackScrollBar( hwnd, wParam, pt );
	break;

    case SC_MOUSEMENU:
        MENU_TrackMouseMenuBar( wndPtr, wParam & 0x000F, pt );
	break;

    case SC_KEYMENU:
	MENU_TrackKbdMenuBar( wndPtr , wParam , pt.x );
	break;
	
    case SC_TASKLIST:
	WinExec( "taskman.exe", SW_SHOWNORMAL ); 
	break;

    case SC_SCREENSAVE:
	if (wParam == SC_ABOUTWINE)
        {
            HMODULE hmodule = LoadLibraryA( "shell32.dll" );
            if (hmodule)
            {
                FARPROC aboutproc = GetProcAddress( hmodule, "ShellAboutA" );
                if (aboutproc) aboutproc( hwnd, "Wine", WINE_RELEASE_INFO, 0 );
                FreeLibrary( hmodule );
            }
        }
	else 
	  if (wParam == SC_PUTMARK)
            TRACE_(shell)("Mark requested by user\n");
	break;
  
    case SC_HOTKEY:
    case SC_ARRANGE:
    case SC_NEXTWINDOW:
    case SC_PREVWINDOW:
 	FIXME("unimplemented!\n");
        break;
    }
    WIN_ReleaseWndPtr(wndPtr);
    return 0;
}

/*************************************************************
*  NC_DrawGrayButton
*
* Stub for the grayed button of the caption
*
*************************************************************/

BOOL NC_DrawGrayButton(HDC hdc, int x, int y)
{
    HBITMAP hMaskBmp;
    HDC hdcMask = CreateCompatibleDC (0);
    HBRUSH hOldBrush;

    hMaskBmp = CreateBitmap (12, 10, 1, 1, lpGrayMask);
    
    if(hMaskBmp == 0)
	return FALSE;
    
    SelectObject (hdcMask, hMaskBmp);
    
    /* Draw the grayed bitmap using the mask */
    hOldBrush = SelectObject (hdc, RGB(128, 128, 128));
    BitBlt (hdc, x, y, 12, 10,
	    hdcMask, 0, 0, 0xB8074A);
    
    /* Clean up */
    SelectObject (hdc, hOldBrush);
    DeleteObject(hMaskBmp);
    DeleteDC (hdcMask);
    
    return TRUE;
}
