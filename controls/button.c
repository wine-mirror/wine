/* File: button.c -- Button type widgets
 *
 * Copyright (C) 1993 Johannes Ruscheinski
 * Copyright (C) 1993 David Metcalfe
 * Copyright (C) 1994 Alexandre Julliard
 */

#include <string.h>
#include <stdlib.h>	/* for abs() */
#include "win.h"
#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "controls.h"
#include "user.h"

/* Note: under MS-Windows, state is a BYTE and this structure is
 * only 3 bytes long. I don't think there are programs out there
 * broken enough to rely on this :-)
 */
typedef struct
{
    WORD     state;   /* Current state */
    HFONT16  hFont;   /* Button font (or 0 for system font) */
    HANDLE   hImage;  /* Handle to the image or the icon */
} BUTTONINFO;

  /* Button state values */
#define BUTTON_UNCHECKED       0x00
#define BUTTON_CHECKED         0x01
#define BUTTON_3STATE          0x02
#define BUTTON_HIGHLIGHTED     0x04
#define BUTTON_HASFOCUS        0x08
#define BUTTON_NSTATES         0x0F
  /* undocumented flags */
#define BUTTON_BTNPRESSED      0x40
#define BUTTON_UNKNOWN2        0x20
#define BUTTON_UNKNOWN3        0x10

static void PB_Paint( WND *wndPtr, HDC hDC, WORD action );
static void CB_Paint( WND *wndPtr, HDC hDC, WORD action );
static void GB_Paint( WND *wndPtr, HDC hDC, WORD action );
static void UB_Paint( WND *wndPtr, HDC hDC, WORD action );
static void OB_Paint( WND *wndPtr, HDC hDC, WORD action );
static void BUTTON_CheckAutoRadioButton( WND *wndPtr );
static void BUTTON_DrawPushButton( WND *wndPtr, HDC hDC, WORD action, BOOL pushedState);
static LRESULT WINAPI ButtonWndProcA( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI ButtonWndProcW( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

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
    SendMessageW( GetParent((wndPtr)->hwndSelf), WM_CTLCOLORBTN, \
                    (hdc), (wndPtr)->hwndSelf )

static HBITMAP hbitmapCheckBoxes = 0;
static WORD checkBoxWidth = 0, checkBoxHeight = 0;


/*********************************************************************
 * button class descriptor
 */
const struct builtin_class_descr BUTTON_builtin_class =
{
    "Button",            /* name */
    CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC, /* style  */
    ButtonWndProcA,      /* procA */
    ButtonWndProcW,      /* procW */
    sizeof(BUTTONINFO),  /* extra */
    IDC_ARROWA,          /* cursor */
    0                    /* brush */
};


/***********************************************************************
 *           ButtonWndProc_locked
 * 
 * Called with window lock held.
 */
static inline LRESULT WINAPI ButtonWndProc_locked(WND* wndPtr, UINT uMsg,
                                                  WPARAM wParam, LPARAM lParam, BOOL unicode )
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
            hbitmapCheckBoxes = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_CHECKBOXES));
            GetObjectW( hbitmapCheckBoxes, sizeof(bmp), &bmp );
            checkBoxWidth  = bmp.bmWidth / 4;
            checkBoxHeight = bmp.bmHeight / 3;
        }
        if (style < 0L || style >= MAX_BTN_TYPE)
            return -1; /* abort */
        infoPtr->state = BUTTON_UNCHECKED;
        infoPtr->hFont = 0;
        infoPtr->hImage = 0;
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        if (btnPaintFunc[style])
        {
            PAINTSTRUCT ps;
            HDC hdc = wParam ? (HDC)wParam : BeginPaint( hWnd, &ps );
            int nOldMode = SetBkMode( hdc, OPAQUE );
            (btnPaintFunc[style])( wndPtr, hdc, ODA_DRAWENTIRE );
            SetBkMode(hdc, nOldMode); /*  reset painting mode */
            if( !wParam ) EndPaint( hWnd, &ps );
        }
        break;

    case WM_KEYDOWN:
	if (wParam == VK_SPACE)
	{
	    SendMessageW( hWnd, BM_SETSTATE, TRUE, 0 );
	    infoPtr->state |= BUTTON_BTNPRESSED;
	}
	break;
	
    case WM_LBUTTONDBLCLK:
        if(wndPtr->dwStyle & BS_NOTIFY || 
                style==BS_RADIOBUTTON ||
                style==BS_USERBUTTON ||
                style==BS_OWNERDRAW) {
            SendMessageW( GetParent(hWnd), WM_COMMAND,
                    MAKEWPARAM( wndPtr->wIDmenu, BN_DOUBLECLICKED ), hWnd);
            break;
        }
        /* fall through */
    case WM_LBUTTONDOWN:
        SetCapture( hWnd );
        SetFocus( hWnd );
        SendMessageW( hWnd, BM_SETSTATE, TRUE, 0 );
        infoPtr->state |= BUTTON_BTNPRESSED;
        break;

    case WM_KEYUP:
	if (wParam != VK_SPACE)
	    break;
	/* fall through */
    case WM_LBUTTONUP:
        if (!(infoPtr->state & BUTTON_BTNPRESSED)) break;
        infoPtr->state &= BUTTON_NSTATES;
        if (!(infoPtr->state & BUTTON_HIGHLIGHTED)) {
            ReleaseCapture();
            break;
        }
        SendMessageW( hWnd, BM_SETSTATE, FALSE, 0 );
        ReleaseCapture();
        GetClientRect( hWnd, &rect );
	if (uMsg == WM_KEYUP || PtInRect( &rect, pt ))
        {
            switch(style)
            {
            case BS_AUTOCHECKBOX:
                SendMessageW( hWnd, BM_SETCHECK,
                                !(infoPtr->state & BUTTON_CHECKED), 0 );
                break;
            case BS_AUTORADIOBUTTON:
                SendMessageW( hWnd, BM_SETCHECK, TRUE, 0 );
                break;
            case BS_AUTO3STATE:
                SendMessageW( hWnd, BM_SETCHECK,
                                (infoPtr->state & BUTTON_3STATE) ? 0 :
                                ((infoPtr->state & 3) + 1), 0 );
                break;
            }
            SendMessageW( GetParent(hWnd), WM_COMMAND,
                            MAKEWPARAM( wndPtr->wIDmenu, BN_CLICKED ), hWnd);
        }
        break;

    case WM_CAPTURECHANGED:
        if (infoPtr->state & BUTTON_BTNPRESSED) {
            infoPtr->state &= BUTTON_NSTATES;
            if (infoPtr->state & BUTTON_HIGHLIGHTED) 
                SendMessageW( hWnd, BM_SETSTATE, FALSE, 0 );
        }
        break;

    case WM_MOUSEMOVE:
        if (GetCapture() == hWnd)
        {
            GetClientRect( hWnd, &rect );
            SendMessageW( hWnd, BM_SETSTATE, PtInRect(&rect, pt), 0 );
        }
        break;

    case WM_SETTEXT:
        if (unicode) DEFWND_SetTextW( wndPtr, (LPCWSTR)lParam );
        else DEFWND_SetTextA( wndPtr, (LPCSTR)lParam );
	if( wndPtr->dwStyle & WS_VISIBLE )
            PAINT_BUTTON( wndPtr, style, ODA_DRAWENTIRE );
        return 1; /* success. FIXME: check text length */

    case WM_SETFONT:
        infoPtr->hFont = (HFONT16)wParam;
        if (lParam && (wndPtr->dwStyle & WS_VISIBLE)) 
	    PAINT_BUTTON( wndPtr, style, ODA_DRAWENTIRE );
        break;

    case WM_GETFONT:
        return infoPtr->hFont;

    case WM_SETFOCUS:
        if ((style == BS_RADIOBUTTON || style == BS_AUTORADIOBUTTON) && (GetCapture() != hWnd) &&
            !(SendMessageW(hWnd, BM_GETCHECK, 0, 0) & BST_CHECKED))
	{
            /* The notification is sent when the button (BS_AUTORADIOBUTTON) 
               is unchecked and the focus was not given by a mouse click. */
	    if (style == BS_AUTORADIOBUTTON)
		    SendMessageW( hWnd, BM_SETCHECK, BUTTON_CHECKED, 0 );
		    SendMessageW( GetParent(hWnd), WM_COMMAND,
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

        /* Only redraw if lParam flag is set.*/
        if (lParam)
           PAINT_BUTTON( wndPtr, style, ODA_DRAWENTIRE );

        break;

    case BM_CLICK:
	SendMessageW( hWnd, WM_LBUTTONDOWN, 0, 0 );
	SendMessageW( hWnd, WM_LBUTTONUP, 0, 0 );
	break;

    case BM_SETIMAGE:
	/* Check that image format confirm button style */
	if ((wndPtr->dwStyle & (BS_BITMAP|BS_ICON)) == BS_BITMAP)
	{
	    if (wParam != (WPARAM) IMAGE_BITMAP)
		return (HICON)0;
	}
	else if ((wndPtr->dwStyle & (BS_BITMAP|BS_ICON)) == BS_ICON)
	{
	    if (wParam != (WPARAM) IMAGE_ICON)
		return (HICON)0;
	} else
	    return (HICON)0;

	oldHbitmap = infoPtr->hImage;
	infoPtr->hImage = (HANDLE) lParam;
	InvalidateRect( hWnd, NULL, FALSE );
	return oldHbitmap;

    case BM_GETIMAGE:
	if ((wndPtr->dwStyle & (BS_BITMAP|BS_ICON)) == BS_BITMAP)
	{
	    if (wParam != (WPARAM) IMAGE_BITMAP)
		return (HICON)0;
	}
	else if ((wndPtr->dwStyle & (BS_BITMAP|BS_ICON)) == BS_ICON)
	{
	    if (wParam != (WPARAM) IMAGE_ICON)
		return (HICON)0;
	} else
	    return (HICON)0;
	return infoPtr->hImage;

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

    case WM_NCHITTEST:
        if(style == BS_GROUPBOX) return HTTRANSPARENT;
        /* fall through */
    default:
        return unicode ? DefWindowProcW(hWnd, uMsg, wParam, lParam) :
                         DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

/***********************************************************************
 *           ButtonWndProcW
 * The button window procedure. This is just a wrapper which locks
 * the passed HWND and calls the real window procedure (with a WND*
 * pointer pointing to the locked windowstructure).
 */
static LRESULT WINAPI ButtonWndProcW( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    LRESULT res = 0;
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    if (wndPtr)
    {
        res = ButtonWndProc_locked(wndPtr,uMsg,wParam,lParam,TRUE);
        WIN_ReleaseWndPtr(wndPtr);
    }
    return res;
}


/***********************************************************************
 *           ButtonWndProcA
 */
static LRESULT WINAPI ButtonWndProcA( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    LRESULT res = 0;
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    if (wndPtr)
    {
        res = ButtonWndProc_locked(wndPtr,uMsg,wParam,lParam,FALSE);
        WIN_ReleaseWndPtr(wndPtr);
    }
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
 * Convert button styles to flags used by DrawText.
 * TODO: handle WS_EX_RIGHT extended style.
 */
static UINT BUTTON_BStoDT(DWORD style)
{
   UINT dtStyle = DT_NOCLIP;  /* We use SelectClipRgn to limit output */

   /* "Convert" pushlike buttons to pushbuttons */
   if (style & BS_PUSHLIKE)
      style &= ~0x0F;

   if (!(style & BS_MULTILINE))
      dtStyle |= DT_SINGLELINE;
   else
      dtStyle |= DT_WORDBREAK;

   switch (style & BS_CENTER)
   {
      case BS_LEFT:   /* DT_LEFT is 0 */    break;
      case BS_RIGHT:  dtStyle |= DT_RIGHT;  break;
      case BS_CENTER: dtStyle |= DT_CENTER; break;
      default:
         /* Pushbutton's text is centered by default */
         if ((style & 0x0F) <= BS_DEFPUSHBUTTON)
            dtStyle |= DT_CENTER;
         /* all other flavours have left aligned text */
   }

   /* DrawText ignores vertical alignment for multiline text,
    * but we use these flags to align label manualy.
    */
   if ((style & 0x0F) != BS_GROUPBOX)
   {
      switch (style & BS_VCENTER)
      {
         case BS_TOP:     /* DT_TOP is 0 */      break;
         case BS_BOTTOM:  dtStyle |= DT_BOTTOM;  break;
         case BS_VCENTER: /* fall through */
         default:         dtStyle |= DT_VCENTER; break;
      }
   }
   else
      /* GroupBox's text is always single line and is top aligned. */
      dtStyle |= DT_SINGLELINE;

   return dtStyle;
}

/**********************************************************************
 *       BUTTON_CalcLabelRect
 *
 *   Calculates label's rectangle depending on button style.
 *
 * Returns flags to be passed to DrawText.
 * Calculated rectangle doesn't take into account button state
 * (pushed, etc.). If there is nothing to draw (no text/image) output
 * rectangle is empty, and return value is (UINT)-1.
 */
static UINT BUTTON_CalcLabelRect(WND *wndPtr, HDC hdc, RECT *rc)
{
   BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;
   ICONINFO    iconInfo;
   BITMAP      bm;
   UINT        dtStyle = BUTTON_BStoDT(wndPtr->dwStyle);
   RECT        r = *rc;
   INT         n;

   /* Calculate label rectangle according to label type */
   switch (wndPtr->dwStyle & (BS_ICON|BS_BITMAP))
   {
      case BS_TEXT:
         if (wndPtr->text && wndPtr->text[0])
            DrawTextW(hdc, wndPtr->text, -1, &r, dtStyle | DT_CALCRECT);
         else
            goto empty_rect;
         break;

      case BS_ICON:
         if (!GetIconInfo((HICON)infoPtr->hImage, &iconInfo))
            goto empty_rect;

         GetObjectW (iconInfo.hbmColor, sizeof(BITMAP), &bm);

         r.right  = r.left + bm.bmWidth;
         r.bottom = r.top  + bm.bmHeight;

         DeleteObject(iconInfo.hbmColor);
         DeleteObject(iconInfo.hbmMask);
         break;

      case BS_BITMAP:
         if (!GetObjectW (infoPtr->hImage, sizeof(BITMAP), &bm))
            goto empty_rect;

         r.right  = r.left + bm.bmWidth;
         r.bottom = r.top  + bm.bmHeight;
         break;

      default:
      empty_rect:   
         r.right = r.left;
         r.bottom = r.top;
         return (UINT)(LONG)-1;
   }

   /* Position label inside bounding rectangle according to
    * alignment flags. (calculated rect is always left-top aligned).
    * If label is aligned to any side - shift label in opposite
    * direction to leave extra space for focus rectangle.
    */
   switch (dtStyle & (DT_CENTER|DT_RIGHT))
   {
      case DT_LEFT:    r.left++;  r.right++;  break;
      case DT_CENTER:  n = r.right - r.left;
                       r.left   = rc->left + ((rc->right - rc->left) - n) / 2;
                       r.right  = r.left + n; break;
      case DT_RIGHT:   n = r.right - r.left;
                       r.right  = rc->right - 1;
                       r.left   = r.right - n;
                       break;
   }

   switch (dtStyle & (DT_VCENTER|DT_BOTTOM))
   {
      case DT_TOP:     r.top++;  r.bottom++;  break;
      case DT_VCENTER: n = r.bottom - r.top;
                       r.top    = rc->top + ((rc->bottom - rc->top) - n) / 2;
                       r.bottom = r.top + n;  break;
      case DT_BOTTOM:  n = r.bottom - r.top;
                       r.bottom = rc->bottom - 1;
                       r.top    = r.bottom - n;
                       break;
   }

   *rc = r;
   return dtStyle;
}


/**********************************************************************
 *       BUTTON_DrawTextCallback
 *
 *   Callback function used by DrawStateW function.
 */
static BOOL CALLBACK BUTTON_DrawTextCallback(HDC hdc, LPARAM lp, WPARAM wp, int cx, int cy)
{
   RECT rc;
   rc.left = 0;
   rc.top = 0;
   rc.right = cx;
   rc.bottom = cy;

   DrawTextW(hdc, (LPCWSTR)lp, -1, &rc, (UINT)wp);
   return TRUE;
}


/**********************************************************************
 *       BUTTON_DrawLabel
 *
 *   Common function for drawing button label.
 */
static void BUTTON_DrawLabel(WND *wndPtr, HDC hdc, UINT dtFlags, RECT *rc)
{
   BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;
   DRAWSTATEPROC lpOutputProc = NULL;
   LPARAM lp;
   WPARAM wp = 0;
   HBRUSH hbr = 0;
   UINT flags = IsWindowEnabled(wndPtr->hwndSelf) ? DSS_NORMAL : DSS_DISABLED;

   /* Fixme: To draw disabled label in Win31 look-and-feel, we probably
    * must use DSS_MONO flag and COLOR_GRAYTEXT brush (or maybe DSS_UNION).
    * I don't have Win31 on hand to verify that, so I leave it as is.
    */

   if ((wndPtr->dwStyle & BS_PUSHLIKE) && (infoPtr->state & BUTTON_3STATE))
   {
      hbr = GetSysColorBrush(COLOR_GRAYTEXT);
      flags |= DSS_MONO;
   }

   switch (wndPtr->dwStyle & (BS_ICON|BS_BITMAP))
   {
      case BS_TEXT:
         /* DST_COMPLEX -- is 0 */
         lpOutputProc = BUTTON_DrawTextCallback;
         lp = (LPARAM)wndPtr->text;
         wp = (WPARAM)dtFlags;
         break;

      case BS_ICON:
         flags |= DST_ICON;
         lp = (LPARAM)infoPtr->hImage;
         break;

      case BS_BITMAP:
         flags |= DST_BITMAP;
         lp = (LPARAM)infoPtr->hImage;
         break;

      default:
         return;
   }

   DrawStateW(hdc, hbr, lpOutputProc, lp, wp, rc->left, rc->top,
              rc->right - rc->left, rc->bottom - rc->top, flags);
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
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;
    RECT     rc, focus_rect, r;
    UINT     dtFlags;
    HRGN     hRgn;
    HPEN     hOldPen;
    HBRUSH   hOldBrush;
    INT      oldBkMode;
    COLORREF oldTxtColor;

    GetClientRect( wndPtr->hwndSelf, &rc );

    /* Send WM_CTLCOLOR to allow changing the font (the colors are fixed) */
    if (infoPtr->hFont) SelectObject( hDC, infoPtr->hFont );
    BUTTON_SEND_CTLCOLOR( wndPtr, hDC );
    hOldPen = (HPEN)SelectObject(hDC, GetSysColorPen(COLOR_WINDOWFRAME));
    hOldBrush =(HBRUSH)SelectObject(hDC,GetSysColorBrush(COLOR_BTNFACE));
    oldBkMode = SetBkMode(hDC, TRANSPARENT);

    if ( TWEAK_WineLook == WIN31_LOOK)
    {
        COLORREF clr_wnd = GetSysColor(COLOR_WINDOW);
        Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);

        SetPixel( hDC, rc.left, rc.top, clr_wnd);
        SetPixel( hDC, rc.left, rc.bottom-1, clr_wnd);
        SetPixel( hDC, rc.right-1, rc.top, clr_wnd);
        SetPixel( hDC, rc.right-1, rc.bottom-1, clr_wnd);
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
	} else {
	   rc.right++, rc.bottom++;
	   DrawEdge( hDC, &rc, EDGE_RAISED, BF_RECT );
	   rc.right--, rc.bottom--;
	}
    }
    else
    {
        UINT uState = DFCS_BUTTONPUSH | DFCS_ADJUSTRECT;

        if (wndPtr->dwStyle & BS_FLAT)
            uState |= DFCS_MONO;
        else if (pushedState)
	{
	    if ( (wndPtr->dwStyle & 0x000f) == BS_DEFPUSHBUTTON )
	        uState |= DFCS_FLAT;
	    else
	        uState |= DFCS_PUSHED;
	}

        if (infoPtr->state & (BUTTON_CHECKED | BUTTON_3STATE))
            uState |= DFCS_CHECKED;

	DrawFrameControl( hDC, &rc, DFC_BUTTON, uState );

	focus_rect = rc;
    }

    /* draw button label */
    r = rc;
    dtFlags = BUTTON_CalcLabelRect(wndPtr, hDC, &r);

    if (dtFlags == (UINT)-1L)
       goto cleanup;

    if (pushedState)
       OffsetRect(&r, 1, 1);

    if(TWEAK_WineLook == WIN31_LOOK)
    {
       focus_rect = r;
       InflateRect(&focus_rect, 2, 0);
    }

    hRgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
    SelectClipRgn(hDC, hRgn);

    oldTxtColor = SetTextColor( hDC, GetSysColor(COLOR_BTNTEXT) );

    BUTTON_DrawLabel(wndPtr, hDC, dtFlags, &r);

    SetTextColor( hDC, oldTxtColor );
    SelectClipRgn(hDC, 0);
    DeleteObject(hRgn);

    if (infoPtr->state & BUTTON_HASFOCUS)
    {
        InflateRect( &focus_rect, -1, -1 );
        IntersectRect(&focus_rect, &focus_rect, &rc);
        DrawFocusRect( hDC, &focus_rect );
    }

 cleanup:
    SelectObject( hDC, hOldPen );
    SelectObject( hDC, hOldBrush );
    SetBkMode(hDC, oldBkMode);
}

/**********************************************************************
 *       Check Box & Radio Button Functions
 */

static void CB_Paint( WND *wndPtr, HDC hDC, WORD action )
{
    RECT rbox, rtext, client;
    HBRUSH hBrush;
    int delta;
    BUTTONINFO *infoPtr = (BUTTONINFO *)wndPtr->wExtra;
    UINT dtFlags;
    HRGN hRgn;

    if (wndPtr->dwStyle & BS_PUSHLIKE)
    {
        BOOL bHighLighted = (infoPtr->state & BUTTON_HIGHLIGHTED);

        BUTTON_DrawPushButton(wndPtr,
			      hDC,
			      action,
			      bHighLighted);
	return;
    }

    GetClientRect(wndPtr->hwndSelf, &client);
    rbox = rtext = client;

    if (infoPtr->hFont) SelectObject( hDC, infoPtr->hFont );

    /* GetControlBrush16 sends WM_CTLCOLORBTN, plus it returns default brush
     * if parent didn't return valid one. So we kill two hares at once
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
    if (action == ODA_DRAWENTIRE || action == ODA_SELECT)
    { 
	/* Since WM_ERASEBKGND does nothing, first prepare background */
	if (action == ODA_SELECT) FillRect( hDC, &rbox, hBrush );
	else FillRect( hDC, &client, hBrush );

        if( TWEAK_WineLook == WIN31_LOOK )
        {
        HDC hMemDC = CreateCompatibleDC( hDC );
        int x = 0, y = 0;
        delta = (rbox.bottom - rbox.top - checkBoxHeight) / 2;

	/* Check in case the client area is smaller than the checkbox bitmap */
	if (delta < 0) delta = 0;

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
        }
        else
        {
	    UINT state;

            if (((wndPtr->dwStyle & 0x0f) == BS_RADIOBUTTON) ||
                ((wndPtr->dwStyle & 0x0f) == BS_AUTORADIOBUTTON)) state = DFCS_BUTTONRADIO;
            else if (infoPtr->state & BUTTON_3STATE) state = DFCS_BUTTON3STATE;
	    else state = DFCS_BUTTONCHECK;

            if (infoPtr->state & (BUTTON_CHECKED | BUTTON_3STATE)) state |= DFCS_CHECKED;
	    
	    if (infoPtr->state & BUTTON_HIGHLIGHTED) state |= DFCS_PUSHED;

	    if (wndPtr->dwStyle & WS_DISABLED) state |= DFCS_INACTIVE;

	    /* rbox must have the correct height */ 
 	    delta = rbox.bottom - rbox.top - checkBoxHeight;
	    if (delta > 0) 
	    {  
		int ofs = (abs(delta) / 2);
		rbox.bottom -= ofs + 1;
		rbox.top = rbox.bottom - checkBoxHeight;
	    }
	    else if (delta < 0)
	    {
		int ofs = (abs(delta) / 2);
		rbox.top -= ofs + 1;
		rbox.bottom = rbox.top + checkBoxHeight;
	    }

	    DrawFrameControl( hDC, &rbox, DFC_BUTTON, state );
        }
    }

    /* Draw label */
    client = rtext;
    dtFlags = BUTTON_CalcLabelRect(wndPtr, hDC, &rtext);

    if (dtFlags == (UINT)-1L) /* Noting to draw */
	return;
    hRgn = CreateRectRgn(client.left, client.top, client.right, client.bottom);
    SelectClipRgn(hDC, hRgn);
    DeleteObject(hRgn);

    if (action == ODA_DRAWENTIRE)
	BUTTON_DrawLabel(wndPtr, hDC, dtFlags, &rtext);

    /* ... and focus */
    if ((action == ODA_FOCUS) ||
        ((action == ODA_DRAWENTIRE) && (infoPtr->state & BUTTON_HASFOCUS)))
    {
	rtext.left--;
	rtext.right++;
	IntersectRect(&rtext, &rtext, &client);
	DrawFocusRect( hDC, &rtext );
    }
    SelectClipRgn(hDC, 0);
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
            SendMessageW( sibling, BM_SETCHECK, BUTTON_UNCHECKED, 0 );
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
    HBRUSH hbr;
    UINT dtFlags;

    if (action != ODA_DRAWENTIRE) return;

    if (infoPtr->hFont)
	SelectObject (hDC, infoPtr->hFont);
    /* GroupBox acts like static control, so it sends CTLCOLORSTATIC */
    hbr = GetControlBrush16( wndPtr->hwndSelf, hDC, CTLCOLOR_STATIC );


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
	TEXTMETRICW tm;
	rcFrame = rc;

	GetTextMetricsW (hDC, &tm);
	rcFrame.top += (tm.tmHeight / 2) - 1;
	DrawEdge (hDC, &rcFrame, EDGE_ETCHED, BF_RECT |
			   ((wndPtr->dwStyle & BS_FLAT) ? BF_FLAT : 0));
    }

    InflateRect(&rc, -7, 1);
    dtFlags = BUTTON_CalcLabelRect(wndPtr, hDC, &rc);

    if (dtFlags == (UINT)-1L)
       return;

    /* Because buttons have CS_PARENTDC class style, there is a chance
     * that label will be drawn out of client rect.
     * But Windows doesn't clip label's rect, so do I.
     */

    /* There is 1-pixel marging at the left, right, and bottom */
    rc.left--; rc.right++; rc.bottom++;
    FillRect(hDC, &rc, hbr);
    rc.left++; rc.right--; rc.bottom--;

    BUTTON_DrawLabel(wndPtr, hDC, dtFlags, &rc);   
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
    HRGN clipRegion;
    RECT clipRect;

    dis.CtlType    = ODT_BUTTON;
    dis.CtlID      = wndPtr->wIDmenu;
    dis.itemID     = 0;
    dis.itemAction = action;
    dis.itemState  = ((infoPtr->state & BUTTON_HASFOCUS) ? ODS_FOCUS : 0) |
                     ((infoPtr->state & BUTTON_HIGHLIGHTED) ? ODS_SELECTED : 0) |
                     (IsWindowEnabled(wndPtr->hwndSelf) ? 0: ODS_DISABLED);
    dis.hwndItem   = wndPtr->hwndSelf;
    dis.hDC        = hDC;
    dis.itemData   = 0;
    GetClientRect( wndPtr->hwndSelf, &dis.rcItem );

    clipRegion = CreateRectRgnIndirect(&dis.rcItem);   
    if (GetClipRgn(hDC, clipRegion) != 1)
    {
	DeleteObject(clipRegion);
	clipRegion=(HRGN)NULL;
    }
    clipRect = dis.rcItem;
    DPtoLP(hDC, (LPPOINT) &clipRect, 2);    
    IntersectClipRect(hDC, clipRect.left,  clipRect.top, clipRect.right, clipRect.bottom);

    SetBkColor( hDC, GetSysColor( COLOR_BTNFACE ) );

    SendMessageW( GetParent(wndPtr->hwndSelf), WM_DRAWITEM,
                    wndPtr->wIDmenu, (LPARAM)&dis );

    SelectClipRgn(hDC, clipRegion);		
}

