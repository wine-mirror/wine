/*
 * Static control
 *
 * Copyright  David W. Metcalfe, 1993
 *
 */

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "win.h"
#include "cursoricon.h"
#include "static.h"
#include "heap.h"
#include "debugtools.h"
#include "tweak.h"

DEFAULT_DEBUG_CHANNEL(static)

static void STATIC_PaintOwnerDrawfn( WND *wndPtr, HDC hdc );
static void STATIC_PaintTextfn( WND *wndPtr, HDC hdc );
static void STATIC_PaintRectfn( WND *wndPtr, HDC hdc );
static void STATIC_PaintIconfn( WND *wndPtr, HDC hdc );
static void STATIC_PaintBitmapfn( WND *wndPtr, HDC hdc );
static void STATIC_PaintEtchedfn( WND *wndPtr, HDC hdc );

static COLORREF color_windowframe, color_background, color_window;


typedef void (*pfPaint)( WND *, HDC );

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
    STATIC_PaintOwnerDrawfn, /* SS_OWNERDRAW */
    STATIC_PaintBitmapfn,    /* SS_BITMAP */
    NULL,                    /* SS_ENHMETAFILE */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDHORIZ */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDVERT */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDFRAME */
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
	ERR("huh? hicon!=0, but info=0???\n");
    	return 0;
    }
    prevIcon = infoPtr->hIcon;
    infoPtr->hIcon = hicon;
    if (hicon)
    {
        SetWindowPos( wndPtr->hwndSelf, 0, 0, 0, info->nWidth, info->nHeight,
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
static HBITMAP16 STATIC_SetBitmap( WND *wndPtr, HBITMAP16 hBitmap )
{
    HBITMAP16 hOldBitmap;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    if ((wndPtr->dwStyle & SS_TYPEMASK) != SS_BITMAP) return 0;
    if (hBitmap && GetObjectType(hBitmap) != OBJ_BITMAP) {
	ERR("huh? hBitmap!=0, but not bitmap\n");
    	return 0;
    }
    hOldBitmap = infoPtr->hIcon;
    infoPtr->hIcon = hBitmap;
    if (hBitmap)
    {
        BITMAP bm;
        GetObjectA(hBitmap, sizeof(bm), &bm);
        SetWindowPos( wndPtr->hwndSelf, 0, 0, 0, bm.bmWidth, bm.bmHeight,
		      SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
    }
    return hOldBitmap;
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
	if (!HIWORD(wndPtr->hInstance)) {
	    LPSTR segname = SEGPTR_STRDUP(name);
	    hicon = LoadIcon16( wndPtr->hInstance, SEGPTR_GET(segname) );
	    SEGPTR_FREE(segname);
	} else
	    hicon = LoadIconA( wndPtr->hInstance, name );
    } else {
        LPSTR segname = SEGPTR_STRDUP(name);

	if (HIWORD(wndPtr->hInstance))
		FIXME("win16 window class, but win32 hinstance??\n");
        hicon = LoadIcon16( wndPtr->hInstance, SEGPTR_GET(segname) );
        SEGPTR_FREE(segname);
    }
    if (!hicon)
	hicon = LoadIconA( 0, name );
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
        hbitmap = LoadBitmapA( wndPtr->hInstance, name );
        if (!hbitmap)  /* Try OEM icon (FIXME: is this right?) */
            hbitmap = LoadBitmapA( 0, name );
    }
    else
    {
        LPSTR segname = SEGPTR_STRDUP(name);
        hbitmap = LoadBitmap16( wndPtr->hInstance, SEGPTR_GET(segname) );
        if (!hbitmap)  /* Try OEM icon (FIXME: is this right?) */
            hbitmap = LoadBitmapA( 0, segname );
        SEGPTR_FREE(segname);
    }
    return hbitmap;
}


/***********************************************************************
 *           StaticWndProc
 */
LRESULT WINAPI StaticWndProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                              LPARAM lParam )
{
    LRESULT lResult = 0;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    LONG style = wndPtr->dwStyle & SS_TYPEMASK;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    switch (uMsg)
    {
    case WM_NCCREATE: {
        CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;

	if ((TWEAK_WineLook > WIN31_LOOK) && (wndPtr->dwStyle & SS_SUNKEN))
	    wndPtr->dwExStyle |= WS_EX_STATICEDGE;

        if (style == SS_ICON)
        {
            if (cs->lpszName)
            {
	    	if (!HIWORD(cs->lpszName) || cs->lpszName[0])
		    STATIC_SetIcon( wndPtr,
                                STATIC_LoadIcon( wndPtr, cs->lpszName ));
	    }
            lResult = 1;
            goto END;
        }
	if (style == SS_BITMAP)
	{
            if (cs->lpszName)
                STATIC_SetBitmap( wndPtr,
                                STATIC_LoadBitmap( wndPtr, cs->lpszName ));
	    WARN("style SS_BITMAP, dwStyle is 0x%08lx\n",
			wndPtr->dwStyle);
            lResult = 1;
            goto END;
	}
        if (!HIWORD(cs->lpszName) && (cs->lpszName))
        {
		FIXME("windowName is 0x%04x, not doing DefWindowProc\n",
		    LOWORD(cs->lpszName));
                lResult = 1;
                goto END;
	}
        lResult = DefWindowProcA( hWnd, uMsg, wParam, lParam );
        goto END;
    }
    case WM_CREATE:
        if (style < 0L || style > SS_TYPEMASK)
        {
            ERR("Unknown style 0x%02lx\n", style );
            lResult = -1L;
            break;
        }
        /* initialise colours */
        color_windowframe  = GetSysColor(COLOR_WINDOWFRAME);
        color_background   = GetSysColor(COLOR_BACKGROUND);
        color_window       = GetSysColor(COLOR_WINDOW);
        break;

    case WM_NCDESTROY:
        if (style == SS_ICON) {
/*
 * FIXME
 *           DestroyIcon32( STATIC_SetIcon( wndPtr, 0 ) );
 * 
 * We don't want to do this yet because DestroyIcon32 is broken. If the icon
 * had already been loaded by the application the last thing we want to do is
 * GlobalFree16 the handle.
 */
        } else {
            lResult = DefWindowProcA( hWnd, uMsg, wParam, lParam );
	}
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint( hWnd, &ps );
            if (staticPaintFunc[style])
                (staticPaintFunc[style])( wndPtr, ps.hdc );
            EndPaint( hWnd, &ps );
        }
        break;

    case WM_ENABLE:
        InvalidateRect( hWnd, NULL, FALSE );
        break;

    case WM_SYSCOLORCHANGE:
        color_windowframe  = GetSysColor(COLOR_WINDOWFRAME);
        color_background   = GetSysColor(COLOR_BACKGROUND);
        color_window       = GetSysColor(COLOR_WINDOW);
        InvalidateRect( hWnd, NULL, TRUE );
        break;

    case WM_SETTEXT:
        if (style == SS_ICON)
            /* FIXME : should we also return the previous hIcon here ??? */
            STATIC_SetIcon( wndPtr, STATIC_LoadIcon( wndPtr, (LPCSTR)lParam ));
        else if (style == SS_BITMAP) 
            STATIC_SetBitmap(wndPtr,STATIC_LoadBitmap(wndPtr,(LPCSTR)lParam ));
	else
            DEFWND_SetText( wndPtr, (LPCSTR)lParam );
        InvalidateRect( hWnd, NULL, FALSE );
        break;

    case WM_SETFONT:
        if (style == SS_ICON)
        {
            lResult = 0;
            goto END;
        }
        if (style == SS_BITMAP)
        {
            lResult = 0;
            goto END;
        }
        infoPtr->hFont = (HFONT16)wParam;
        if (LOWORD(lParam))
        {
            InvalidateRect( hWnd, NULL, FALSE );
            UpdateWindow( hWnd );
        }
        break;

    case WM_GETFONT:
        lResult = infoPtr->hFont;
        goto END;

    case WM_NCHITTEST:
        lResult = HTTRANSPARENT;
        goto END;

    case WM_GETDLGCODE:
        lResult = DLGC_STATIC;
        goto END;

    case STM_GETIMAGE:
    case STM_GETICON16:
    case STM_GETICON:
        lResult = infoPtr->hIcon;
        goto END;

    case STM_SETIMAGE:
    	/* FIXME: handle wParam */
        lResult = STATIC_SetBitmap( wndPtr, (HBITMAP)lParam );
        InvalidateRect( hWnd, NULL, FALSE );
        UpdateWindow( hWnd );
	break;

    case STM_SETICON16:
    case STM_SETICON:
        lResult = STATIC_SetIcon( wndPtr, (HICON16)wParam );
        InvalidateRect( hWnd, NULL, FALSE );
        UpdateWindow( hWnd );
        break;

    default:
        lResult = DefWindowProcA(hWnd, uMsg, wParam, lParam);
        break;
    }
    
END:
    WIN_ReleaseWndPtr(wndPtr);
    return lResult;
}

static void STATIC_PaintOwnerDrawfn( WND *wndPtr, HDC hdc )
{
  DRAWITEMSTRUCT dis;

  dis.CtlType    = ODT_STATIC;
  dis.CtlID      = wndPtr->wIDmenu;
  dis.itemID     = 0;
  dis.itemAction = ODA_DRAWENTIRE;
  dis.itemState  = 0;
  dis.hwndItem   = wndPtr->hwndSelf;
  dis.hDC        = hdc;
  dis.itemData   = 0;
  GetClientRect( wndPtr->hwndSelf, &dis.rcItem );

  SendMessageA( GetParent(wndPtr->hwndSelf), WM_CTLCOLORSTATIC, 
		hdc, wndPtr->hwndSelf );    
  SendMessageA( GetParent(wndPtr->hwndSelf), WM_DRAWITEM,
		wndPtr->wIDmenu, (LPARAM)&dis );
}

static void STATIC_PaintTextfn( WND *wndPtr, HDC hdc )
{
    RECT rc;
    HBRUSH hBrush;
    WORD wFormat;

    LONG style = wndPtr->dwStyle;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    GetClientRect( wndPtr->hwndSelf, &rc);

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
	wFormat = DT_LEFT | DT_EXPANDTABS | DT_VCENTER;
	break;

    default:
        return;
    }

    if (style & SS_NOPREFIX)
	wFormat |= DT_NOPREFIX;

    if (infoPtr->hFont) SelectObject( hdc, infoPtr->hFont );
    hBrush = SendMessageA( GetParent(wndPtr->hwndSelf), WM_CTLCOLORSTATIC,
                             hdc, wndPtr->hwndSelf );
    if (!hBrush) hBrush = GetStockObject(WHITE_BRUSH);
    FillRect( hdc, &rc, hBrush );    

    if (!IsWindowEnabled(wndPtr->hwndSelf))
   	SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));

    if (wndPtr->text) DrawTextA( hdc, wndPtr->text, -1, &rc, wFormat );
}

static void STATIC_PaintRectfn( WND *wndPtr, HDC hdc )
{
    RECT rc;
    HBRUSH hBrush;

    GetClientRect( wndPtr->hwndSelf, &rc);
    
    switch (wndPtr->dwStyle & SS_TYPEMASK)
    {
    case SS_BLACKRECT:
	hBrush = CreateSolidBrush(color_windowframe);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_GRAYRECT:
	hBrush = CreateSolidBrush(color_background);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_WHITERECT:
	hBrush = CreateSolidBrush(color_window);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_BLACKFRAME:
	hBrush = CreateSolidBrush(color_windowframe);
        FrameRect( hdc, &rc, hBrush );
	break;
    case SS_GRAYFRAME:
	hBrush = CreateSolidBrush(color_background);
        FrameRect( hdc, &rc, hBrush );
	break;
    case SS_WHITEFRAME:
	hBrush = CreateSolidBrush(color_window);
        FrameRect( hdc, &rc, hBrush );
	break;
    default:
        return;
    }
    DeleteObject( hBrush );
}


static void STATIC_PaintIconfn( WND *wndPtr, HDC hdc )
{
    RECT rc;
    HBRUSH hbrush;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    GetClientRect( wndPtr->hwndSelf, &rc );
    hbrush = SendMessageA( GetParent(wndPtr->hwndSelf), WM_CTLCOLORSTATIC,
                             hdc, wndPtr->hwndSelf );
    FillRect( hdc, &rc, hbrush );
    if (infoPtr->hIcon) DrawIcon( hdc, rc.left, rc.top, infoPtr->hIcon );
}

static void STATIC_PaintBitmapfn(WND *wndPtr, HDC hdc )
{
    RECT rc;
    HBRUSH hbrush;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;
    HDC hMemDC;
    HBITMAP oldbitmap;

    GetClientRect( wndPtr->hwndSelf, &rc );
    hbrush = SendMessageA( GetParent(wndPtr->hwndSelf), WM_CTLCOLORSTATIC,
                             hdc, wndPtr->hwndSelf );
    FillRect( hdc, &rc, hbrush );

    if (infoPtr->hIcon) {
        BITMAP bm;
	SIZE sz;

        if(GetObjectType(infoPtr->hIcon) != OBJ_BITMAP)
	    return;
        if (!(hMemDC = CreateCompatibleDC( hdc ))) return;
	GetObjectA(infoPtr->hIcon, sizeof(bm), &bm);
	GetBitmapDimensionEx(infoPtr->hIcon, &sz);
	oldbitmap = SelectObject(hMemDC, infoPtr->hIcon);
	BitBlt(hdc, sz.cx, sz.cy, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0,
	       SRCCOPY);
	SelectObject(hMemDC, oldbitmap);
	DeleteDC(hMemDC);
    }
}


static void STATIC_PaintEtchedfn( WND *wndPtr, HDC hdc )
{
    RECT rc;

    if (TWEAK_WineLook == WIN31_LOOK)
	return;

    GetClientRect( wndPtr->hwndSelf, &rc );
    switch (wndPtr->dwStyle & SS_TYPEMASK)
    {
	case SS_ETCHEDHORZ:
	    DrawEdge(hdc,&rc,EDGE_ETCHED,BF_TOP|BF_BOTTOM);
	    break;
	case SS_ETCHEDVERT:
	    DrawEdge(hdc,&rc,EDGE_ETCHED,BF_LEFT|BF_RIGHT);
	    break;
	case SS_ETCHEDFRAME:
	    DrawEdge (hdc, &rc, EDGE_ETCHED, BF_RECT);
	    break;
    }
}

