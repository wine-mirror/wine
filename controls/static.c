/*
 * Static control
 *
 * Copyright  David W. Metcalfe, 1993
 *
 */

#include "windows.h"
#include "win.h"
#include "bitmap.h"
#include "cursoricon.h"
#include "static.h"
#include "heap.h"
#include "debug.h"

static void STATIC_PaintTextfn( WND *wndPtr, HDC32 hdc );
static void STATIC_PaintRectfn( WND *wndPtr, HDC32 hdc );
static void STATIC_PaintIconfn( WND *wndPtr, HDC32 hdc );
static void STATIC_PaintBitmapfn( WND *wndPtr, HDC32 hdc );


static COLORREF color_windowframe, color_background, color_window;


typedef void (*pfPaint)( WND *, HDC32 );

static pfPaint staticPaintFunc[SS_TYPEMASK+1] =
{
    STATIC_PaintTextfn,      /* SS_LEFT */
    STATIC_PaintTextfn,      /* SS_CENTER */
    STATIC_PaintTextfn,      /* SS_RIGHT */
    STATIC_PaintIconfn,      /* SS_ICON */
    STATIC_PaintRectfn,      /* SS_BLACKRECT */
    STATIC_PaintRectfn,      /* SS_GRAYRECT */
    STATIC_PaintRectfn,      /* SS_WHITERECT */
    STATIC_PaintRectfn,      /* SS_BLACKFRAME */
    STATIC_PaintRectfn,      /* SS_GRAYFRAME */
    STATIC_PaintRectfn,      /* SS_WHITEFRAME */
    NULL,                    /* Not defined */
    STATIC_PaintTextfn,      /* SS_SIMPLE */
    STATIC_PaintTextfn,      /* SS_LEFTNOWORDWRAP */
    NULL,                    /* SS_OWNERDRAW */
    STATIC_PaintBitmapfn,    /* SS_BITMAP */
    NULL,                    /* SS_ENHMETAFILE */
    NULL,                    /* SS_ETCHEDHORIZ */
    NULL,                    /* SS_ETCHEDVERT */
    NULL,                    /* SS_ETCHEDFRAME */
};


/***********************************************************************
 *           STATIC_SetIcon
 *
 * Set the icon for an SS_ICON control.
 */
static HICON16 STATIC_SetIcon( WND *wndPtr, HICON16 hicon )
{
    HICON16 prevIcon;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;
    CURSORICONINFO *info = hicon?(CURSORICONINFO *) GlobalLock16( hicon ):NULL;

    if ((wndPtr->dwStyle & SS_TYPEMASK) != SS_ICON) return 0;
    if (hicon && !info) {
	ERR(static, "huh? hicon!=0, but info=0???\n");
    	return 0;
    }
    prevIcon = infoPtr->hIcon;
    infoPtr->hIcon = hicon;
    if (hicon)
    {
        SetWindowPos32( wndPtr->hwndSelf, 0, 0, 0, info->nWidth, info->nHeight,
                        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
        GlobalUnlock16( hicon );
    }
    return prevIcon;
}

/***********************************************************************
 *           STATIC_SetBitmap
 *
 * Set the bitmap for an SS_BITMAP control.
 */
static HICON16 STATIC_SetBitmap( WND *wndPtr, HICON16 hicon )
{
    HICON16 prevIcon;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;
    BITMAPOBJ *info = (BITMAPOBJ *)GDI_HEAP_LOCK(hicon);

    if ((wndPtr->dwStyle & SS_TYPEMASK) != SS_BITMAP) return 0;
    if (hicon && !info) {
	ERR(static, "huh? hicon!=0, but info=0???\n");
    	return 0;
    }
    prevIcon = infoPtr->hIcon;
    infoPtr->hIcon = hicon;
    if (hicon)
    {
        SetWindowPos32( wndPtr->hwndSelf, 0, 0, 0, info->bitmap.bmWidth, info->bitmap.bmHeight,
                        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
    }
    GDI_HEAP_UNLOCK( hicon );
    return prevIcon;
}


/***********************************************************************
 *           STATIC_LoadIcon
 *
 * Load the icon for an SS_ICON control.
 */
static HICON16 STATIC_LoadIcon( WND *wndPtr, LPCSTR name )
{
    HICON16 hicon;

    if (wndPtr->flags & WIN_ISWIN32)
    {
        hicon = LoadIcon32A( wndPtr->hInstance, name );
        if (!hicon)  /* Try OEM icon (FIXME: is this right?) */
            hicon = LoadIcon32A( 0, name );
    }
    else
    {
        LPSTR segname = SEGPTR_STRDUP(name);
        hicon = LoadIcon16( wndPtr->hInstance, SEGPTR_GET(segname) );
        if (!hicon)  /* Try OEM icon (FIXME: is this right?) */
            hicon = LoadIcon32A( 0, segname );
        SEGPTR_FREE(segname);
    }
    return hicon;
}

/***********************************************************************
 *           STATIC_LoadBitmap
 *
 * Load the bitmap for an SS_BITMAP control.
 */
static HBITMAP16 STATIC_LoadBitmap( WND *wndPtr, LPCSTR name )
{
    HBITMAP16 hbitmap;

    if (wndPtr->flags & WIN_ISWIN32)
    {
        hbitmap = LoadBitmap32A( wndPtr->hInstance, name );
        if (!hbitmap)  /* Try OEM icon (FIXME: is this right?) */
            hbitmap = LoadBitmap32A( 0, name );
    }
    else
    {
        LPSTR segname = SEGPTR_STRDUP(name);
        hbitmap = LoadBitmap16( wndPtr->hInstance, SEGPTR_GET(segname) );
        if (!hbitmap)  /* Try OEM icon (FIXME: is this right?) */
            hbitmap = LoadBitmap32A( 0, segname );
        SEGPTR_FREE(segname);
    }
    return hbitmap;
}


/***********************************************************************
 *           StaticWndProc
 */
LRESULT WINAPI StaticWndProc( HWND32 hWnd, UINT32 uMsg, WPARAM32 wParam,
                              LPARAM lParam )
{
    LRESULT lResult = 0;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    LONG style = wndPtr->dwStyle & SS_TYPEMASK;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    switch (uMsg)
    {
    case WM_NCCREATE:
        if (style == SS_ICON)
        {
            CREATESTRUCT32A *cs = (CREATESTRUCT32A *)lParam;
            if (cs->lpszName)
                STATIC_SetIcon( wndPtr,
                                STATIC_LoadIcon( wndPtr, cs->lpszName ));
            return 1;
        }
	if (style == SS_BITMAP)
	{
            CREATESTRUCT32A *cs = (CREATESTRUCT32A *)lParam;
            if (cs->lpszName)
                STATIC_SetBitmap( wndPtr,
                                STATIC_LoadBitmap( wndPtr, cs->lpszName ));
	    WARN(static, "style SS_BITMAP, dwStyle is 0x%08lx\n",
			wndPtr->dwStyle);
            return 1;
	}
        return DefWindowProc32A( hWnd, uMsg, wParam, lParam );

    case WM_CREATE:
        if (style < 0L || style > SS_TYPEMASK)
        {
            ERR(static, "Unknown style 0x%02lx\n", style );
            lResult = -1L;
            break;
        }
        /* initialise colours */
        color_windowframe  = GetSysColor32(COLOR_WINDOWFRAME);
        color_background   = GetSysColor32(COLOR_BACKGROUND);
        color_window       = GetSysColor32(COLOR_WINDOW);
        break;

    case WM_NCDESTROY:
        if (style == SS_ICON)
            DestroyIcon32( STATIC_SetIcon( wndPtr, 0 ) );
        else
            lResult = DefWindowProc32A( hWnd, uMsg, wParam, lParam );
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT32 ps;
            BeginPaint32( hWnd, &ps );
            if (staticPaintFunc[style])
                (staticPaintFunc[style])( wndPtr, ps.hdc );
            EndPaint32( hWnd, &ps );
        }
        break;

    case WM_ENABLE:
        InvalidateRect32( hWnd, NULL, FALSE );
        break;

    case WM_SYSCOLORCHANGE:
        color_windowframe  = GetSysColor32(COLOR_WINDOWFRAME);
        color_background   = GetSysColor32(COLOR_BACKGROUND);
        color_window       = GetSysColor32(COLOR_WINDOW);
        InvalidateRect32( hWnd, NULL, TRUE );
        break;

    case WM_SETTEXT:
        if (style == SS_ICON)
            /* FIXME : should we also return the previous hIcon here ??? */
            STATIC_SetIcon( wndPtr, STATIC_LoadIcon( wndPtr, (LPCSTR)lParam ));
        else if (style == SS_BITMAP) 
            STATIC_SetBitmap(wndPtr,STATIC_LoadBitmap(wndPtr,(LPCSTR)lParam ));
	else
            DEFWND_SetText( wndPtr, (LPCSTR)lParam );
        InvalidateRect32( hWnd, NULL, FALSE );
        UpdateWindow32( hWnd );
        break;

    case WM_SETFONT:
        if (style == SS_ICON) return 0;
        if (style == SS_BITMAP) return 0;
        infoPtr->hFont = (HFONT16)wParam;
        if (LOWORD(lParam))
        {
            InvalidateRect32( hWnd, NULL, FALSE );
            UpdateWindow32( hWnd );
        }
        break;

    case WM_GETFONT:
        return infoPtr->hFont;

    case WM_NCHITTEST:
        return HTTRANSPARENT;

    case WM_GETDLGCODE:
        return DLGC_STATIC;

    	return infoPtr->hIcon;
    case STM_GETIMAGE:
    case STM_GETICON16:
    case STM_GETICON32:
        return infoPtr->hIcon;

    case STM_SETIMAGE:
    	/* FIXME: handle wParam */
        lResult = STATIC_SetBitmap( wndPtr, (HBITMAP32)lParam );
        InvalidateRect32( hWnd, NULL, FALSE );
        UpdateWindow32( hWnd );
	break;
    case STM_SETICON16:
    case STM_SETICON32:
        lResult = STATIC_SetIcon( wndPtr, (HICON16)wParam );
        InvalidateRect32( hWnd, NULL, FALSE );
        UpdateWindow32( hWnd );
        break;

    default:
        lResult = DefWindowProc32A(hWnd, uMsg, wParam, lParam);
        break;
    }
    
    return lResult;
}


static void STATIC_PaintTextfn( WND *wndPtr, HDC32 hdc )
{
    RECT32 rc;
    HBRUSH32 hBrush;
    WORD wFormat;

    LONG style = wndPtr->dwStyle;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    GetClientRect32( wndPtr->hwndSelf, &rc);

    switch (style & SS_TYPEMASK)
    {
    case SS_LEFT:
	wFormat = DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK | DT_NOCLIP;
	break;

    case SS_CENTER:
	wFormat = DT_CENTER | DT_EXPANDTABS | DT_WORDBREAK | DT_NOCLIP;
	break;

    case SS_RIGHT:
	wFormat = DT_RIGHT | DT_EXPANDTABS | DT_WORDBREAK | DT_NOCLIP;
	break;

    case SS_SIMPLE:
	wFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP;
	break;

    case SS_LEFTNOWORDWRAP:
	wFormat = DT_LEFT | DT_SINGLELINE | DT_EXPANDTABS | DT_VCENTER | DT_NOCLIP;
	break;

    default:
        return;
    }

    if (style & SS_NOPREFIX)
	wFormat |= DT_NOPREFIX;

    if (infoPtr->hFont) SelectObject32( hdc, infoPtr->hFont );
    hBrush = SendMessage32A( GetParent32(wndPtr->hwndSelf), WM_CTLCOLORSTATIC,
                             hdc, wndPtr->hwndSelf );
    if (!hBrush) hBrush = GetStockObject32(WHITE_BRUSH);
    FillRect32( hdc, &rc, hBrush );
    if (wndPtr->text) DrawText32A( hdc, wndPtr->text, -1, &rc, wFormat );
}

static void STATIC_PaintRectfn( WND *wndPtr, HDC32 hdc )
{
    RECT32 rc;
    HBRUSH32 hBrush;

    GetClientRect32( wndPtr->hwndSelf, &rc);
    
    switch (wndPtr->dwStyle & SS_TYPEMASK)
    {
    case SS_BLACKRECT:
	hBrush = CreateSolidBrush32(color_windowframe);
        FillRect32( hdc, &rc, hBrush );
	break;
    case SS_GRAYRECT:
	hBrush = CreateSolidBrush32(color_background);
        FillRect32( hdc, &rc, hBrush );
	break;
    case SS_WHITERECT:
	hBrush = CreateSolidBrush32(color_window);
        FillRect32( hdc, &rc, hBrush );
	break;
    case SS_BLACKFRAME:
	hBrush = CreateSolidBrush32(color_windowframe);
        FrameRect32( hdc, &rc, hBrush );
	break;
    case SS_GRAYFRAME:
	hBrush = CreateSolidBrush32(color_background);
        FrameRect32( hdc, &rc, hBrush );
	break;
    case SS_WHITEFRAME:
	hBrush = CreateSolidBrush32(color_window);
        FrameRect32( hdc, &rc, hBrush );
	break;
    default:
        return;
    }
    DeleteObject32( hBrush );
}


static void STATIC_PaintIconfn( WND *wndPtr, HDC32 hdc )
{
    RECT32 rc;
    HBRUSH32 hbrush;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    GetClientRect32( wndPtr->hwndSelf, &rc );
    hbrush = SendMessage32A( GetParent32(wndPtr->hwndSelf), WM_CTLCOLORSTATIC,
                             hdc, wndPtr->hwndSelf );
    FillRect32( hdc, &rc, hbrush );
    if (infoPtr->hIcon) DrawIcon32( hdc, rc.left, rc.top, infoPtr->hIcon );
}

static void STATIC_PaintBitmapfn(WND *wndPtr, HDC32 hdc ) 
{
    RECT32 rc;
    HBRUSH32 hbrush;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;
    HDC32 hMemDC;
    HBITMAP32 oldbitmap;

    GetClientRect32( wndPtr->hwndSelf, &rc );
    hbrush = SendMessage32A( GetParent32(wndPtr->hwndSelf), WM_CTLCOLORSTATIC,
                             hdc, wndPtr->hwndSelf );
    FillRect32( hdc, &rc, hbrush );
    if (infoPtr->hIcon) {
        BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_HEAP_LOCK( infoPtr->hIcon );

	if (!bmp) return;
        if (!(hMemDC = CreateCompatibleDC32( hdc ))) return;

	oldbitmap = SelectObject32(hMemDC,infoPtr->hIcon);
	BitBlt32(hdc,bmp->size.cx,bmp->size.cy,bmp->bitmap.bmWidth,bmp->bitmap.bmHeight,hMemDC,0,0,SRCCOPY);
	DeleteDC32(hMemDC);
        GDI_HEAP_UNLOCK(infoPtr->hIcon);
    }
}
